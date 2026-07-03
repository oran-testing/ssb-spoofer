// Config parser - dead simple YAML-like parser for our config files

#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

namespace ssb_spoofer {

bool load_from_influxdb(Config& config, YAML::Node root) {
	if(!root["influxdb"]) return false;

	auto db = root["influxdb"];

	config.database.host = db["host"] ? db["host"].as<std::string>() : "localhost";
	return true;
}

bool ConfigParser::load_from_file(const std::string& filename, Config& config)
{
    YAML::Node root;

    try {
        root = YAML::LoadFile(filename);
    } catch (const std::exception& e) {
        std::cerr << "[!] Failed to load config file: " << e.what() << "\n";
        return false;
    }

    if (!root) {
        std::cerr << "[!] Empty config file\n";
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

		bool enable_autoconfigure = root["enable_autoconfigure"] ? root["enable_autoconfigure"].as<bool>() : false;
		if(enable_autoconfigure){
			if(!load_from_influxdb(config, root)) return false;
		}

    return validate(config);
}



bool ConfigParser::validate(const Config& config) {
  bool valid = true;
  
  if (config.rf.srate_hz <= 0) {
      std::cerr << "[!] invalid sample rate\n";
      valid = false;
  }
  
  if (config.rf.rx_freq_hz <= 0 || config.rf.tx_freq_hz <= 0) {
      std::cerr << "[!] invalid frequency\n";
      valid = false;
  }
  
  if (config.ssb.pattern != "A" && config.ssb.pattern != "B" && 
      config.ssb.pattern != "C" && config.ssb.pattern != "D" && 
      config.ssb.pattern != "E") {
      std::cerr << "[!] invalid SSB pattern (need A/B/C/D/E)\n";
      valid = false;
  }
  
  if (config.ssb.scs_khz != 15 && config.ssb.scs_khz != 30) {
      std::cerr << "[!] invalid SCS (need 15 or 30 kHz)\n";
      valid = false;
  }
  
  if (config.attack.target_pci > 1007) {
      std::cerr << "[!] invalid PCI (max 1007)\n";
      valid = false;
  }
  
  if (config.attack.coreset0_idx_value > 15) {
      std::cerr << "[!] invalid CORESET0 idx (max 15)\n";
      valid = false;
  }
  
  if (config.attack.ss0_idx_value > 15) {
      std::cerr << "[!] invalid SS0 idx (max 15)\n";
      valid = false;
  }
  
  return valid;
}

void ConfigParser::print(const Config& config) {
  std::cout << "\n--- Configuration ---\n";
  std::cout << "\n[RF]\n";
  std::cout << "  device: " << config.rf.device_name << "\n";
  std::cout << "  args: " << config.rf.device_args << "\n";
  std::cout << "  RX freq: " << config.rf.rx_freq_hz / 1e6 << " MHz\n";
  std::cout << "  TX freq: " << config.rf.tx_freq_hz / 1e6 << " MHz\n";
  std::cout << "  srate: " << config.rf.srate_hz / 1e6 << " MHz\n";
  std::cout << "  RX gain: " << config.rf.rx_gain_db << " dB\n";
  std::cout << "  TX gain: " << config.rf.tx_gain_db << " dB\n";
  
  std::cout << "\n[SSB]\n";
  std::cout << "  pattern: " << config.ssb.pattern << "\n";
  std::cout << "  SCS: " << config.ssb.scs_khz << " kHz\n";
  std::cout << "  period: " << config.ssb.periodicity_ms << " ms\n";
  
  std::cout << "\n[Attack]\n";
  std::cout << "  target PCI: " << config.attack.target_pci << "\n";
  std::cout << "  scan for target: " << (config.attack.scan_for_target ? "yes" : "no") << "\n";
  std::cout << "  modify cell_barred: " << (config.attack.modify_cell_barred ? "yes" : "no");
  if (config.attack.modify_cell_barred) {
      std::cout << " (" << (config.attack.cell_barred_value ? "true" : "false") << ")";
  }
  std::cout << "\n";
  std::cout << "  modify CORESET0: " << (config.attack.modify_coreset0_idx ? "yes" : "no");
  if (config.attack.modify_coreset0_idx) {
      std::cout << " (val: " << config.attack.coreset0_idx_value << ")";
  }
  std::cout << "\n";
  std::cout << "  continuous TX: " << (config.attack.continuous_tx ? "yes" : "no") << "\n";
  
  std::cout << "\n[Burst Control]\n";
  std::cout << "  max bursts: " << (config.attack.max_bursts == 0 ? "unlimited" : std::to_string(config.attack.max_bursts)) << "\n";
  std::cout << "  burst interval: " << config.attack.burst_interval_us << " us\n";
  std::cout << "  burst length: " << config.attack.burst_length_ms << " ms\n";
  
  std::cout << "\n--------------------\n\n";
}

} // namespace ssb_spoofer

