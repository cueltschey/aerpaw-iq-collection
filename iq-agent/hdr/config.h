#ifndef CONFIG_H
#define CONFIG_H

#include <chrono>
#include <iomanip>
#include <srsran/phy/common/phy_common_nr.h>
#define FREQ_DEFAULT 627750000
#define SRATE_DEFAULT 23040000
#define GAIN_DEFAULT 50.0
#define PRB_DEFAULT 106

#include "logging.h"
#include "srsran/phy/sync/ssb.h"
#include "toml.hpp"
#include <srsran/common/band_helper.h>

using namespace std;

typedef struct rf_config_s {
  std::string file_path;
  uint64_t sample_rate;
  double ssb_center_frequency_hz;
  const char *rf_args;
  double gain;
  uint64_t nof_prb;
  uint32_t pci;
} rf_config_t;

typedef struct ssb_config_s {
  srsran_ssb_pattern_t ssb_pattern = SRSRAN_SSB_PATTERN_A;
  srsran_subcarrier_spacing_t ssb_scs = srsran_subcarrier_spacing_15kHz;
  srsran_duplex_mode_t duplex_mode = SRSRAN_DUPLEX_MODE_FDD;
} ssb_config_t;

typedef struct agent_config_s {
  rf_config_t rf;
  ssb_config_t ssb;
  std::string output_path;
} agent_config_t;

static agent_config_t load(std::string config_path) {
  printf("Loading config from path: %s\n", config_path.c_str());
  toml::table toml = toml::parse_file(config_path);
  agent_config_t conf;
  conf.rf.file_path = toml["rf"]["file_path"].value_or("");
  conf.rf.sample_rate = toml["rf"]["sample_rate"].value_or(SRATE_DEFAULT);
  double ssb_arfcn = toml["rf"]["arfcn"].value_or(0);
  conf.rf.ssb_center_frequency_hz = toml["rf"]["freq"].value_or(0);
  conf.rf.gain = toml["rf"]["gain"].value_or(GAIN_DEFAULT);
  conf.rf.nof_prb = toml["rf"]["nof_prb"].value_or(PRB_DEFAULT);
  conf.rf.pci = toml["rf"]["pci"].value_or(0);
  conf.rf.rf_args = toml["rf"]["rf_args"].value_or("");
  std::string ssb_pattern = toml["ssb"]["pattern"].value_or("");
  int ssb_scs = toml["ssb"]["scs"].value_or(0);
  std::string duplex_mode = toml["ssb"]["duplex_mode"].value_or("error");
  std::string log_level_str = toml["log"]["level"].value_or("error");
  conf.output_path = toml["log"]["output"].value_or("output.csv");

  if (ssb_arfcn != 0) {
    srsran::srsran_band_helper band_helper;
    conf.rf.ssb_center_frequency_hz = band_helper.nr_arfcn_to_freq(ssb_arfcn);
  }

  switch (ssb_pattern[0]) {
  case 'a':
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_A;
    break;
  case 'b':
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_B;
    break;
  case 'c':
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_C;
    break;
  case 'd':
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_D;
    break;
  case 'e':
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_E;
    break;
  default:
    conf.ssb.ssb_pattern = SRSRAN_SSB_PATTERN_INVALID;
    break;
  }

  switch (ssb_scs) {
  case 15:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_15kHz;
    break;
  case 30:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_30kHz;
    break;
  case 60:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_60kHz;
    break;

  case 120:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_120kHz;
    break;

  case 240:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_240kHz;
    break;
  default:
    conf.ssb.ssb_scs = srsran_subcarrier_spacing_invalid;
    break;
  }

  if (duplex_mode == "fdd")
    conf.ssb.duplex_mode = SRSRAN_DUPLEX_MODE_FDD;
  else if (duplex_mode == "tdd")
    conf.ssb.duplex_mode = SRSRAN_DUPLEX_MODE_TDD;
  else if (duplex_mode == "sdl")
    conf.ssb.duplex_mode = SRSRAN_DUPLEX_MODE_SDL;
  else if (duplex_mode == "sul")
    conf.ssb.duplex_mode = SRSRAN_DUPLEX_MODE_SUL;
  else
    conf.ssb.duplex_mode = SRSRAN_DUPLEX_MODE_INVALID;

  if (log_level_str == "error")
    log_level = ERROR;
  else if (log_level_str == "info")
    log_level = INFO;
  else if (log_level_str == "warning")
    log_level = WARNING;
  else if (log_level_str == "debug")
    log_level = DEBUG;

  return conf;
}

std::string current_timestamp() {
  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);

  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() %
      1000;

  std::tm buf;
  localtime_r(&time_t_now, &buf); // Thread-safe

  std::ostringstream oss;
  oss << std::put_time(&buf, "%Y-%m-%d %H:%M:") << std::setw(2)
      << std::setfill('0') << buf.tm_sec << "." << std::setw(3)
      << std::setfill('0') << millis;

  return oss.str();
}

#endif // CONFIG_H
