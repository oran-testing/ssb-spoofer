#ifndef INFLUX_WORKER
#define INFLUX_WORKER

#include "influxdb.hpp"
#include "srsran/phy/gnb/gnb_dl.h"
#include "srsran/srslog/srslog.h"
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <variant>

// Definitions for custom pullable structs

typedef struct influx_band_report_s {
	uint16_t band;
	uint32_t nof_prb;
	uint32_t offset_to_carrier;
	srsran_subcarrier_spacing_t scs_common;
	srsran_subcarrier_spacing_t scs_ssb;
	uint32_t dl_arfcn;
	uint32_t ul_arfcn;
	uint32_t ssb_arfcn;
	double dl_freq;
	double ul_freq;
	double ssb_freq;
	srsran_ssb_pattern_t ssb_pattern;
	double sample_rate;
	double uplink_cfo;
	double downlink_cfo;
} recon_band_report_t;

struct ChannelConfig {
  double rx_frequency;
  double tx_frequency;
  double rx_offset;
  double tx_offset;
  double rx_gain;
  double tx_gain;
  bool   enabled;
};

struct DatabaseConfig {
  std::string host;
  uint32_t port;
  std::string org;
  std::string token;
  std::string bucket;
  std::string data_id;
};

class InfluxWorker
{
public:
  explicit InfluxWorker(srslog::basic_logger& logger_, const DatabaseConfig config_);
  ~InfluxWorker() = default;

  // Function to pull messages from influxDB
	template <typename T>
	bool pull_msg(T& msg)
	{
		if constexpr (std::is_same_v<T, recon_band_report_t>) {
			return recv_band_report(msg);
		}
		else if constexpr (std::is_same_v<T, ChannelConfig>) {
			return recv_channel_config(msg);
		}
		else if constexpr (std::is_same_v<T, srsran_mib_nr_t>) {
			return recv_mib(msg);
		}
		else {
			static_assert(std::is_same_v<T, void>, "Unsupported type passed to pull_msg()");
		}
	}


private:
  srslog::basic_logger& logger;
  influxdb_cpp::server_info influx_server_info;
	std::string data_id;

	recon_band_report_t recv_band_report();
	ChannelConfig recv_channel_config();
  srsran_mib_nr_t recv_mib();
	// TODO
  //bool recv_sib1(const asn1::rrc_nr::sib1_s& sib1);
};

#endif // INFLUX_WORKER
