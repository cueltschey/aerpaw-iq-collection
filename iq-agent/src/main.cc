#include "config.h"
#include "data_source.h"
#include "logging.h"
#include <complex.h>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/common/phy_common_nr.h>
#include <srsran/phy/phch/pbch_msg_nr.h>
#include <srsran/phy/sync/ssb.h>
#include <srsran/phy/utils/vector.h>
#include <stdlib.h>
#include <sys/types.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    LOG_ERROR("Usage: iq-agent <config file>\n");
    return EXIT_FAILURE;
  }

  std::string config_path(argv[1]);
  agent_config_t conf = load(config_path);

  std::ofstream output_csv(conf.output_path);
  if (!output_csv.is_open()) {
    std::cerr << "Failed to open output file.\n";
    return EXIT_FAILURE;
  }
  output_csv << "timestamp,rsrp,rsrp_dB,epre,epre_dB,n0,n0_dB,snr_dB\n";

  std::ofstream output_iq;
  if (conf.output_iq) {
    output_iq = std::ofstream(conf.iq_path, std::ios::binary);
  }

  LOG_DEBUG("Running with ssb freq: %f", conf.rf.ssb_center_frequency_hz);

  uint32_t sf_len = SRSRAN_SF_LEN_PRB(conf.rf.nof_prb);
  uint32_t N_id = 1;

  srsran_ssb_t ssb = {};
  srsran_ssb_args_t ssb_args = {};
  ssb_args.enable_decode = true;
  ssb_args.enable_search = true;
  if (srsran_ssb_init(&ssb, &ssb_args) < SRSRAN_SUCCESS) {
    LOG_ERROR("ssb init failed");
    return EXIT_FAILURE;
  }

  srsran_ssb_cfg_t ssb_cfg = {};
  ssb_cfg.srate_hz = conf.rf.sample_rate;
  ssb_cfg.center_freq_hz = conf.rf.ssb_center_frequency_hz;
  ssb_cfg.ssb_freq_hz = conf.rf.ssb_center_frequency_hz;
  ssb_cfg.scs = conf.ssb.ssb_scs;
  ssb_cfg.pattern = conf.ssb.ssb_pattern;
  ssb_cfg.duplex_mode = conf.ssb.duplex_mode;
  if (srsran_ssb_set_cfg(&ssb, &ssb_cfg) < SRSRAN_SUCCESS) {
    LOG_ERROR("error setting SSB configuration");
    return EXIT_FAILURE;
  }
  char cfg_str[500];
  srsran_ssb_cfg_to_str(&ssb_cfg, cfg_str, 500);
  LOG_DEBUG("Running with ssb cfg: %s", cfg_str);

  data_source *src;
  if (!conf.rf.file_path.empty())
    src = new data_source(strdup(conf.rf.file_path.c_str()),
                          SRSRAN_COMPLEX_FLOAT_BIN);
  else
    src = new data_source(strdup(conf.rf.rf_args), conf.rf.gain,
                          conf.rf.ssb_center_frequency_hz, conf.rf.sample_rate);

  cf_t *buffer = srsran_vec_cf_malloc(sf_len);
  while (src->read(buffer, sf_len)) {
    char str[512] = {};
    srsran_csi_trs_measurements_t meas = {};
    if (srsran_ssb_csi_search(&ssb, buffer, sf_len, &N_id, &meas) <
        SRSRAN_SUCCESS) {
      LOG_ERROR("Error performing SSB-CSI search");
      continue;
    } else {
      srsran_csi_meas_info(&meas, str, sizeof(str));
      if (meas.rsrp != 0) {
        output_csv << current_timestamp();
        output_csv << "," << meas.rsrp;
        output_csv << "," << meas.rsrp_dB;
        output_csv << "," << meas.epre;
        output_csv << "," << meas.epre_dB;
        output_csv << "," << meas.n0;
        output_csv << "," << meas.n0_dB;
        output_csv << "," << meas.snr_dB << "\n";
        output_csv.flush();

        LOG_DEBUG("CSI MEAS - search pci=%d %s", N_id, str);

        if (conf.output_iq) {
          output_iq.write(reinterpret_cast<const char *>(buffer),
                          sizeof(cf_t) * sf_len);
        }
      }
    }

    srsran_ssb_search_res_t search_res = {};
    if (srsran_ssb_search(&ssb, buffer, sf_len, &search_res) < SRSRAN_SUCCESS) {
      LOG_ERROR("Error performing SSB search");
    }
    N_id = search_res.N_id;
  }

  output_csv.close();
  if (conf.output_iq) {
    output_iq.close();
  }

  return SRSRAN_SUCCESS;
}
