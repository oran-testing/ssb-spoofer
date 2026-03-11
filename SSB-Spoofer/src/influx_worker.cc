#include "influx_worker.h"
#include <sstream>
#include <unordered_map>

static std::unordered_map<std::string, std::string> parse_flux_fields(const std::string& resp)
{
  std::unordered_map<std::string, std::string> result;

  std::stringstream ss(resp);
  std::string line;

  int field_col = -1;
  int value_col = -1;

  while (std::getline(ss, line)) {
    if (line.empty() || line[0] == '#')
      continue;

    std::vector<std::string> cols;
    std::stringstream ls(line);
    std::string item;

    while (std::getline(ls, item, ',')) {
      cols.push_back(item);
    }

    // header row
    if (field_col == -1) {
      for (size_t i = 0; i < cols.size(); ++i) {
        if (cols[i] == "_field")
          field_col = i;
        if (cols[i] == "_value")
          value_col = i;
      }
      continue;
    }

    if (field_col >= 0 && value_col >= 0 &&
        field_col < (int)cols.size() && value_col < (int)cols.size()) {

      result[cols[field_col]] = cols[value_col];
    }
  }

  return result;
}

InfluxWorker::InfluxWorker(const DatabaseConfig config_) :
  influx_server_info(config_.host, config_.port, config_.org, config_.token, config_.bucket), data_id(config_.data_id){
  if(influx_server_info.resp_ != 0){
    LOG_ERROR("Failed to connect to InfluxDB");
  }
}

bool InfluxWorker::recv_channel_config(ChannelConfig& ch)
{
  LOG_INFO("Pulling channel config %s", data_id.c_str());

  std::stringstream query;

  query <<
    "from(bucket: \"" << influx_server_info.bkt_ << "\")"
    " |> range(start: -10m)"
    " |> filter(fn: (r) => r._measurement == \"channel_config\")"
    " |> filter(fn: (r) => r.data_id == \"" << data_id << "\")"
    " |> last()";

  std::string resp;
  if (influxdb_cpp::flux_query(resp, query.str(), influx_server_info) != 0) {
    LOG_ERROR("InfluxDB query failed with error: %s", resp);
    return false;
  }

  auto fields = parse_flux_fields(resp);

  ch.rx_frequency = std::stod(fields["rx_frequency"]);
  ch.tx_frequency = std::stod(fields["tx_frequency"]);
  ch.rx_offset    = std::stod(fields["rx_offset"]);
  ch.tx_offset    = std::stod(fields["tx_offset"]);
  ch.rx_gain      = std::stod(fields["rx_gain"]);
  ch.tx_gain      = std::stod(fields["tx_gain"]);
  ch.enabled      = fields["enabled"] == "true";

  return true;
}

bool InfluxWorker::recv_band_report(recon_band_report_t& report)
{
  LOG_INFO("Pulling band report %s", data_id.c_str());

  std::stringstream query;

  query <<
    "from(bucket: \"" << influx_server_info.bkt_ << "\")"
    " |> range(start: -10m)"
    " |> filter(fn: (r) => r._measurement == \"band_report\")"
    " |> filter(fn: (r) => r.data_id == \"" << data_id << "\")"
    " |> last()";

  std::string resp;
  if (influxdb_cpp::flux_query(resp, query.str(), influx_server_info) != 0) {
    LOG_ERROR("InfluxDB query failed with error: %s", resp);
    return false;
  }

  auto fields = parse_flux_fields(resp);

  report.band              = std::stoi(fields["band"]);
  report.nof_prb           = std::stoi(fields["nof_prb"]);
  report.offset_to_carrier = std::stoi(fields["offset_to_carrier"]);
  report.scs_common        = (srsran_subcarrier_spacing_t)std::stoi(fields["scs_common"]);
  report.scs_ssb           = (srsran_subcarrier_spacing_t)std::stoi(fields["scs_ssb"]);
  report.dl_arfcn          = std::stoi(fields["dl_arfcn"]);
  report.ul_arfcn          = std::stoi(fields["ul_arfcn"]);
  report.ssb_arfcn         = std::stoi(fields["ssb_arfcn"]);

  report.dl_freq  = std::stod(fields["dl_freq"]);
  report.ul_freq  = std::stod(fields["ul_freq"]);
  report.ssb_freq = std::stod(fields["ssb_freq"]);

  report.ssb_pattern = (srsran_ssb_pattern_t)std::stoi(fields["ssb_pattern"]);

  report.sample_rate  = std::stod(fields["sample_rate"]);
  report.uplink_cfo   = std::stod(fields["uplink_cfo"]);
  report.downlink_cfo = std::stod(fields["downlink_cfo"]);

  return true;
}

bool InfluxWorker::recv_mib(srsran_mib_nr_t& mib)
{
  LOG_INFO("Pulling MIB %s", data_id.c_str());

  std::stringstream query;

  query <<
    "from(bucket: \"" << influx_server_info.bkt_ << "\")"
    " |> range(start: -10m)"
    " |> filter(fn: (r) => r._measurement == \"mib\")"
    " |> filter(fn: (r) => r.data_id == \"" << data_id << "\")"
    " |> last()";

  std::string resp;
  if (influxdb_cpp::flux_query(resp, query.str(), influx_server_info) != 0) {
    LOG_ERROR("InfluxDB query failed with error: %s", resp);
    return false;
  }

  auto fields = parse_flux_fields(resp);

  auto to_bool = [](const std::string& v) {
    return v == "true" || v == "t" || v == "1";
  };

  try {
    mib.sfn        = static_cast<uint32_t>(std::stoul(fields["sfn"]));
    mib.ssb_idx    = static_cast<uint8_t>(std::stoul(fields["ssb_idx"]));
    mib.hrf        = to_bool(fields["hrf"]);

    mib.scs_common =
        static_cast<srsran_subcarrier_spacing_t>(std::stoul(fields["scs_common"]));

    mib.ssb_offset =
        static_cast<uint32_t>(std::stoul(fields["ssb_offset"]));

    mib.dmrs_typeA_pos =
        static_cast<srsran_dmrs_sch_typeA_pos_t>(std::stoul(fields["dmrs_typeA_pos"]));

    mib.coreset0_idx =
        static_cast<uint32_t>(std::stoul(fields["coreset0_idx"]));

    mib.ss0_idx =
        static_cast<uint32_t>(std::stoul(fields["ss0_idx"]));

    mib.cell_barred =
        to_bool(fields["cell_barred"]);

    mib.intra_freq_reselection =
        to_bool(fields["intra_freq_reselection"]);

    mib.spare =
        fields.count("spare") ? static_cast<uint32_t>(std::stoul(fields["spare"])) : 0;

  } catch (const std::exception& e) {
    LOG_ERROR("Failed parsing MIB fields: %s", e.what());
    return false;
  }

  return true;
}

// TODO: bool InfluxWorker::recv_sib1(const asn1::rrc_nr::sib1_s& sib1){ }
