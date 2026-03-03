#include "shadower/comp/workers/influx_worker.h"

InfluxWorker::InfluxWorker(srslog::basic_logger& logger_, const DatabaseConfig config_) :
  logger(logger_), influx_server_info(config_.host, config_.port, config_.org, config_.token, config_.bucket), data_id(config_.data_id){
  if(influx_server_info.resp_ != 0){
    logger.error(RED "Failed to connect to InfluxDB" RESET);
  }
}

bool InfluxWorker::recv_channel_config(ChannelConfig& ch){
	logger.info(GREEN "Sending channel config as %s" RESET, data_id.c_str());
	// NOTE: use influxdb_cpp::flux_query()
	return true;
}

bool InfluxWorker::recv_band_report(recon_band_report_t& report){
	logger.info(GREEN "Sending band report as %s" RESET, data_id.c_str());
	return true;
}

bool InfluxWorker::recv_mib(srsran_mib_nr_t& mib){
	logger.info(GREEN "Sending MIB as %s" RESET, data_id.c_str());
	return true;
}

// TODO: bool InfluxWorker::recv_sib1(const asn1::rrc_nr::sib1_s& sib1){ }
