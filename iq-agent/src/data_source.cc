#include "data_source.h"
#include "logging.h"
#include <srsran/config.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/io/filesource.h>
#include <srsran/phy/io/format.h>

data_source::data_source(char *file_path, srsran_datatype_t datatype)
    : radio_init(false), file_init(true) {
  file_src = {};
  srsran_filesource_init(&file_src, file_path, datatype);
}

data_source::data_source(char *rf_args, double rf_gain, double rf_freq,
                         double srate)
    : radio_init(true), file_init(false) {
  usrp = uhd::usrp::multi_usrp::make(rf_args);
  usrp->set_rx_rate(srate);
  usrp->set_rx_freq(rf_freq);
  usrp->set_rx_gain(rf_gain);

  uhd::stream_args_t stream_args("fc32", "sc16");
  rx_stream = usrp->get_rx_stream(stream_args);
  metadata = uhd::rx_metadata_t();

  std::cout << "UHD Configured: rate=" << srate << "Hz, freq=" << rf_freq
            << "Hz, gain=" << rf_gain << "dB\n";

  uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
  stream_cmd.stream_now = true;
  rx_stream->issue_stream_cmd(stream_cmd);
}

data_source::~data_source() {
  if (file_init) {
    srsran_filesource_free(&file_src);
  }

  if (rx_stream) {
    // Stop streaming
    uhd::stream_cmd_t stream_cmd(
        uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    rx_stream->issue_stream_cmd(stream_cmd);
  }
}

bool data_source::read(cf_t *output, int nof_samples) {
  if (!file_init && !radio_init)
    return false;
  if (file_init) {
    if (srsran_filesource_read(&file_src, output, nof_samples) <
        SRSRAN_SUCCESS) {
      LOG_ERROR("Error reading from file\n");
      return false;
    }
    return true;
  }

  LOG_DEBUG(" ----  Receive %d samples  ---- ", nof_samples);

  size_t received = rx_stream->recv(output, nof_samples, metadata, 1.0);
  if (metadata.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
    std::cerr << "UHD RX Error: " << metadata.strerror() << "\n";
    return false;
  }
  return true;
}
