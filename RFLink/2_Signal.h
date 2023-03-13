/// ************************************* //
// * Arduino Project RFLink32        * //
// * https://github.com/couin3/RFLink  * //
// * 2018..2020 Stormteam - Marc RIVES * //
// * More details in RFLink.ino file   * //
// ************************************* //

#ifndef Signal_h
#define Signal_h

#include <Arduino.h>
#include "11_Config.h"

#ifdef ESP32
#define RAW_BUFFER_SIZE 1200        // 292        // Maximum number of pulses that is received in one go.
#else
#define RAW_BUFFER_SIZE 292        // 292        // Maximum number of pulses that is received in one go.
#endif
#define MIN_RAW_PULSES 24          // 24         // Minimal number of bits that need to have been received before we spend CPU time on decoding the signal.
#define SIGNAL_SEEK_TIMEOUT_MS 25  // 25         // After this time in mSec, RF signal will be considered absent.
#define SIGNAL_MIN_PREAMBLE_US 100 // 400        // After this time in uSec, a RF signal will be considered to have started.
#define MIN_PULSE_LENGTH_US 90    // 100        // Pulses shorter than this value in uSec. will be seen as garbage and not taken as actual pulses.
#define SIGNAL_END_TIMEOUT_US 5000 // 5000       // After this time in uSec, the RF signal will be considered to have stopped.
#define SIGNAL_REPEAT_TIME_MS 250  // 500        // Time in mSec. in which the same RF signal should not be accepted again. Filters out retransmits.
#define SCAN_HIGH_TIME_MS 50       // 50         // time interval in ms. fast processing for background tasks

#define DEFAULT_RAWSIGNAL_SAMPLE_RATE 1    // for compatibility with Arduinos only unless you want to scan pulses > 65000us

#if defined(RFLINK_SIGNAL_DEBUG)
#define RFLINK_SIGNAL_RSSI_DEBUG
#endif

extern unsigned long SignalCRC;   // holds the bitstream value for some plugins to identify RF repeats
extern unsigned long SignalCRC_1; // holds the previous SignalCRC (for mixed burst protocols)
extern byte SignalHash;           // holds the processed plugin number
extern byte SignalHashPrevious;   // holds the last processed plugin number
extern unsigned long RepeatingTimer;

namespace RFLink {
  namespace Signal {

    enum EndReasons {
      Unknown,
      ReachedLongPulseTimeOut,
      AttemptedNoiseFilter,
      DynamicGapLengthReached,
      SignalEndTimeout,
      TooLong,
      REASONS_EOF,
    };

    enum Slicer_enum {
      Default = -1,
      Legacy,
      RSSI_Advanced,
      SLICERS_EOF,
    };

    struct RawSignalStruct // Raw signal variabelen places in a struct
    {
      int Number;                       // Number of pulses, times two as every pulse has a mark and a space.
      byte Repeats;                     // Number of re-transmits on transmit actions.
      byte Delay;                       // Delay in ms. after transmit of a single RF pulse packet
      byte Multiply;                    // Pulses[] * Multiply is the real pulse time in microseconds (to keep compatibility with Arduino)
      unsigned long Time;               // Timestamp indicating when the signal was received (millis())
      bool readyForDecoder;             // indicates if packet can be processed by decoders
      float rssi;
      EndReasons endReason;
      #ifdef RFLINK_SIGNAL_RSSI_DEBUG
      float Rssis[RAW_BUFFER_SIZE + 1];
      #endif
      uint16_t RawPulses[RAW_BUFFER_SIZE + 1]; // Table with the measured pulses in microseconds divided by RawSignal.Multiply. (to keep compatibility with Arduino)
      uint16_t* Pulses; // pointer used by plugins, points at an element inside RawPulses to avoid costly shifting of memory
      // First pulse is located in element 1. Element 0 is used for special purposes, like signalling the use of a specific plugin
    };

    extern RawSignalStruct RawSignal;


    namespace params {
      // All json variable names
      extern bool async_mode_enabled;
      extern unsigned short int sample_rate;
      extern unsigned long int min_raw_pulses;
      extern unsigned long int seek_timeout;        // milliseconds
      extern unsigned long int min_preamble;        // microseconds
      extern unsigned long int min_pulse_len;       // microseconds
      extern unsigned long int signal_end_timeout;  // microseconds
      extern unsigned long int signal_repeat_time;  // milliseconds
      extern unsigned long int scan_high_time;      // milliseconds
    }

    namespace runtime {
      extern bool verboseSignalFetchLoop;
      extern Slicer_enum appliedSlicer;
    }

    namespace counters {
      extern unsigned long int receivedSignalsCount;
      extern unsigned long int successfullyDecodedSignalsCount;

    }

    extern Config::ConfigItem configItems[];

    void setup();
    void paramsUpdatedCallback();
    void refreshParametersFromConfig(bool triggerChanges=true);
    void RawSendRF(RawSignalStruct *signal);
    void AC_Send(unsigned long data, byte cmd);

    void executeCliCommand(char *cmd);

    bool ScanEvent();
    void getStatusJsonString(JsonObject &output);

    void displaySignal(RawSignalStruct &signal);

    const char * endReasonToString(EndReasons reason);

    inline void setVerboseSignalFetchLoop(bool value=true) {
      runtime::verboseSignalFetchLoop = value;
    }

    bool updateSlicer(Slicer_enum newSlicer);

    namespace AsyncSignalScanner {
      extern unsigned long int lastChangedState_us;     // time last state change occured
      extern unsigned long int nextPulseTimeoutTime_us; // when current pulse will timeout
      extern bool scanningStopped;                      // 

      void enableAsyncReceiver();
      void disableAsyncReceiver();
      /**
       * It will only work if AsyncScanner is enabled
       * */
      void startScanning();
      /**
       * It will only work if AsyncScanner is enabled
       * */
      void stopScanning();
      void clearAllTimers();
      void IRAM_ATTR RX_pin_changed_state();
      void onPulseTimerTimeout();

      bool getSignalFromJson(RawSignalStruct &signal, const char *json_str);

      inline bool isStopped() {
        return scanningStopped;
      };

      inline bool isEnabled() {
        return params::async_mode_enabled;
      };
    };

  } // end of ns Signal
} //  end of ns RFLink

#endif