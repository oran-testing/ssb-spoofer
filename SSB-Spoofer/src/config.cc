#include "config.h"

namespace ssb_spoofer {

bool load_from_influxdb(Config& config)
{
    LOG_INFO("Loading configuration from InfluxDB");

    InfluxWorker influx(config.database);

    ChannelConfig ch;
    recon_band_report_t band;

    if (!influx.pull_msg(ch)) {
        LOG_ERROR("Failed to load ChannelConfig from InfluxDB");
        return false;
    }

    if (!influx.pull_msg(band)) {
        LOG_ERROR("Failed to load band report from InfluxDB");
        return false;
    }

    // RF configuration
    config.rf.rx_freq_hz = ch.rx_frequency + ch.rx_offset;
    config.rf.tx_freq_hz = ch.tx_frequency + ch.tx_offset;

    config.rf.rx_gain_db = ch.rx_gain;
    config.rf.tx_gain_db = ch.tx_gain;

    // SSB / PHY parameters
    config.ssb.scs_khz = (band.scs_common == srsran_subcarrier_spacing_15kHz) ? 15 : 30;

    switch (band.ssb_pattern) {
        case SRSRAN_SSB_PATTERN_A: config.ssb.pattern = "A"; break;
        case SRSRAN_SSB_PATTERN_B: config.ssb.pattern = "B"; break;
        case SRSRAN_SSB_PATTERN_C: config.ssb.pattern = "C"; break;
        case SRSRAN_SSB_PATTERN_D: config.ssb.pattern = "D"; break;
        case SRSRAN_SSB_PATTERN_E: config.ssb.pattern = "E"; break;
        default: config.ssb.pattern = "INVALID"; break;
    }

    config.ssb.ssb_freq_offset_hz =
        band.ssb_freq - ch.rx_frequency;

    LOG_DEBUG("Auto-configured RF from InfluxDB");
    LOG_DEBUG("RX freq: %.3f MHz", config.rf.rx_freq_hz / 1e6);
    LOG_DEBUG("TX freq: %.3f MHz", config.rf.tx_freq_hz / 1e6);
    LOG_DEBUG("RX gain: %.2f dB", config.rf.rx_gain_db);
    LOG_DEBUG("TX gain: %.2f dB", config.rf.tx_gain_db);

    return true;
}

bool ConfigParser::load_from_file(const std::string& filename, Config& config)
{
    YAML::Node root;

    try {
        root = YAML::LoadFile(filename);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load config file: %s", e.what());
        return false;
    }

    if (!root) {
        LOG_ERROR("Empty config file");
        return false;
    }

    // ---------------- RF ----------------
    if (root["rf"]) {
        auto rf = root["rf"];

        config.rf.device_name = rf["device_name"] ? rf["device_name"].as<std::string>() : "uhd";
        config.rf.device_args = rf["device_args"] ? rf["device_args"].as<std::string>() : "";

        config.rf.rx_freq_hz  = rf["rx_freq_hz"] ? rf["rx_freq_hz"].as<double>() : 3510000000.0;
        config.rf.tx_freq_hz  = rf["tx_freq_hz"] ? rf["tx_freq_hz"].as<double>() : 3510000000.0;
        config.rf.srate_hz    = rf["srate_hz"]   ? rf["srate_hz"].as<double>()   : 23040000.0;

        config.rf.rx_gain_db  = rf["rx_gain_db"] ? rf["rx_gain_db"].as<double>() : 40.0;
        config.rf.tx_gain_db  = rf["tx_gain_db"] ? rf["tx_gain_db"].as<double>() : 60.0;
    }

    if (root["ssb"]) {
        auto ssb = root["ssb"];

        config.ssb.pattern            = ssb["pattern"] ? ssb["pattern"].as<std::string>() : "C";
        config.ssb.scs_khz            = ssb["scs_khz"] ? ssb["scs_khz"].as<uint32_t>() : 30;
        config.ssb.periodicity_ms     = ssb["periodicity_ms"] ? ssb["periodicity_ms"].as<uint32_t>() : 20;
        config.ssb.ssb_freq_offset_hz = ssb["ssb_freq_offset_hz"] ? ssb["ssb_freq_offset_hz"].as<double>() : 0.0;

        config.ssb.beta_pss       = ssb["beta_pss"] ? ssb["beta_pss"].as<float>() : 0.0f;
        config.ssb.beta_sss       = ssb["beta_sss"] ? ssb["beta_sss"].as<float>() : 0.0f;
        config.ssb.beta_pbch      = ssb["beta_pbch"] ? ssb["beta_pbch"].as<float>() : 0.0f;
        config.ssb.beta_pbch_dmrs = ssb["beta_pbch_dmrs"] ? ssb["beta_pbch_dmrs"].as<float>() : 0.0f;
    }

    if (root["attack"]) {
        auto atk = root["attack"];

        config.attack.target_pci              = atk["target_pci"] ? atk["target_pci"].as<uint32_t>() : 0;
        config.attack.scan_for_target         = atk["scan_for_target"] ? atk["scan_for_target"].as<bool>() : true;
        config.attack.modify_cell_barred      = atk["modify_cell_barred"] ? atk["modify_cell_barred"].as<bool>() : true;
        config.attack.cell_barred_value       = atk["cell_barred_value"] ? atk["cell_barred_value"].as<bool>() : true;

        config.attack.modify_coreset0_idx     = atk["modify_coreset0_idx"] ? atk["modify_coreset0_idx"].as<bool>() : false;
        config.attack.coreset0_idx_value      = atk["coreset0_idx_value"] ? atk["coreset0_idx_value"].as<uint32_t>() : 15;

        config.attack.modify_ss0_idx          = atk["modify_ss0_idx"] ? atk["modify_ss0_idx"].as<bool>() : false;
        config.attack.ss0_idx_value           = atk["ss0_idx_value"] ? atk["ss0_idx_value"].as<uint32_t>() : 15;

        config.attack.modify_intra_freq_resel = atk["modify_intra_freq_resel"] ? atk["modify_intra_freq_resel"].as<bool>() : false;
        config.attack.intra_freq_resel_value  = atk["intra_freq_resel_value"] ? atk["intra_freq_resel_value"].as<bool>() : false;

        config.attack.tx_power_offset_db      = atk["tx_power_offset_db"] ? atk["tx_power_offset_db"].as<double>() : 0.0;
        config.attack.continuous_tx           = atk["continuous_tx"] ? atk["continuous_tx"].as<bool>() : true;

        config.attack.max_bursts        = atk["max_bursts"] ? atk["max_bursts"].as<uint64_t>() : 0;
        config.attack.burst_interval_us = atk["burst_interval_us"] ? atk["burst_interval_us"].as<uint32_t>() : 500;
        config.attack.burst_length_ms   = atk["burst_length_ms"] ? atk["burst_length_ms"].as<uint32_t>() : 1;
    }

    if (root["operation"]) {
        auto op = root["operation"];

        config.operation.scan_duration_sec = op["scan_duration_sec"] ? op["scan_duration_sec"].as<double>() : 10.0;
        config.operation.log_level         = op["log_level"] ? op["log_level"].as<std::string>() : "info";
        config.operation.log_file          = op["log_file"] ? op["log_file"].as<std::string>() : "ssb_spoofer.log";
        config.operation.save_samples      = op["save_samples"] ? op["save_samples"].as<bool>() : false;
        config.operation.samples_file      = op["samples_file"] ? op["samples_file"].as<std::string>() : "rx_samples.dat";
    }

    if (root["database"]) {
        auto db = root["database"];

        config.database.host =
            db["host"] ? db["host"].as<std::string>() : "127.0.0.1";

        config.database.port =
            db["port"] ? db["port"].as<uint32_t>() : 8086;

        config.database.org =
            db["org"] ? db["org"].as<std::string>() : "";

        config.database.token =
            db["token"] ? db["token"].as<std::string>() : "";

        config.database.bucket =
            db["bucket"] ? db["bucket"].as<std::string>() : "";

        config.database.data_id =
            db["data_id"] ? db["data_id"].as<std::string>() : "";
    }

    bool enable_autoconfigure = root["enable_autoconfigure"] ?
                                root["enable_autoconfigure"].as<bool>() : false;

    if (enable_autoconfigure) {
        if (!load_from_influxdb(config)) {
            LOG_ERROR("InfluxDB auto configuration failed");
            return false;
        }
    }

    return validate(config);
}

bool ConfigParser::validate(const Config& config)
{
    bool valid = true;

    if (config.rf.srate_hz <= 0) {
        LOG_ERROR("Invalid sample rate");
        valid = false;
    }

    if (config.rf.rx_freq_hz <= 0 || config.rf.tx_freq_hz <= 0) {
        LOG_ERROR("Invalid frequency");
        valid = false;
    }

    if (config.ssb.pattern != "A" && config.ssb.pattern != "B" &&
        config.ssb.pattern != "C" && config.ssb.pattern != "D" &&
        config.ssb.pattern != "E") {
        LOG_ERROR("Invalid SSB pattern (need A/B/C/D/E)");
        valid = false;
    }

    if (config.ssb.scs_khz != 15 && config.ssb.scs_khz != 30) {
        LOG_ERROR("Invalid SCS (need 15 or 30 kHz)");
        valid = false;
    }

    if (config.attack.target_pci > 1007) {
        LOG_ERROR("Invalid PCI (max 1007)");
        valid = false;
    }

    if (config.attack.coreset0_idx_value > 15) {
        LOG_ERROR("Invalid CORESET0 idx (max 15)");
        valid = false;
    }

    if (config.attack.ss0_idx_value > 15) {
        LOG_ERROR("Invalid SS0 idx (max 15)");
        valid = false;
    }

    return valid;
}

void ConfigParser::print(const Config& config)
{
    LOG_INFO("---- Configuration ----");

    LOG_INFO("[RF]");
    LOG_INFO("device: %s", config.rf.device_name.c_str());
    LOG_INFO("args: %s", config.rf.device_args.c_str());
    LOG_INFO("RX freq: %.3f MHz", config.rf.rx_freq_hz / 1e6);
    LOG_INFO("TX freq: %.3f MHz", config.rf.tx_freq_hz / 1e6);
    LOG_INFO("srate: %.3f MHz", config.rf.srate_hz / 1e6);
    LOG_INFO("RX gain: %.2f dB", config.rf.rx_gain_db);
    LOG_INFO("TX gain: %.2f dB", config.rf.tx_gain_db);

    LOG_INFO("[SSB]");
    LOG_INFO("pattern: %s", config.ssb.pattern.c_str());
    LOG_INFO("SCS: %u kHz", config.ssb.scs_khz);
    LOG_INFO("period: %u ms", config.ssb.periodicity_ms);

    LOG_INFO("[Attack]");
    LOG_INFO("target PCI: %u", config.attack.target_pci);
    LOG_INFO("scan for target: %s", config.attack.scan_for_target ? "yes" : "no");
    LOG_INFO("modify cell_barred: %s", config.attack.modify_cell_barred ? "yes" : "no");
    LOG_INFO("modify CORESET0: %s", config.attack.modify_coreset0_idx ? "yes" : "no");
    LOG_INFO("continuous TX: %s", config.attack.continuous_tx ? "yes" : "no");

    LOG_INFO("[Burst Control]");
    if (config.attack.max_bursts == 0)
        LOG_INFO("max bursts: unlimited");
    else
        LOG_INFO("max bursts: %llu", (unsigned long long)config.attack.max_bursts);

    LOG_INFO("burst interval: %u us", config.attack.burst_interval_us);
    LOG_INFO("burst length: %u ms", config.attack.burst_length_ms);
}

} // namespace ssb_spoofer
