#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifndef DCF77_TM_STD_C_COMPATIBLE
    // Set to 1 to get a c std compatible time struct
    // - Month from 0 to 11
    // - Years since 1900
    // ...
    // Set to 0 to get a more human readable format
    // - Month from 1 to 12
    // - Year e.g. 2025
    #define DCF77_TM_STD_C_COMPATIBLE 0
#endif

class DCF77
{
private:

    typedef struct
    {
        uint8_t u8BitOffset;
        uint8_t u8DataLen;
    }
    value_info_t;

    static const int DCF77_BUFFER_SIZE = 60;

    const value_info_t Minute       = {.u8BitOffset=21, .u8DataLen=7};
    const value_info_t Hour         = {.u8BitOffset=29, .u8DataLen=6};
    const value_info_t Day          = {.u8BitOffset=36, .u8DataLen=6};
    const value_info_t Month        = {.u8BitOffset=45, .u8DataLen=5};
    const value_info_t Year         = {.u8BitOffset=50, .u8DataLen=8};
    const value_info_t DayOfWeek    = {.u8BitOffset=42, .u8DataLen=3};
    const value_info_t DST          = {.u8BitOffset=17, .u8DataLen=2}; // Daylight saving time --> Sommerzeit

    const value_info_t ParityMinute = {.u8BitOffset=21, .u8DataLen=7}; // Minute starts at index 21 has 7 bit of data and parity right behind the last data bit
    const value_info_t ParityHour   = {.u8BitOffset=29, .u8DataLen=6};
    const value_info_t ParityDate   = {.u8BitOffset=36, .u8DataLen=22};

    const uint8_t BCD_VALUES[8] = {1, 2, 4, 8, 10, 20, 40, 80};
    
    uint8_t  _u8Pin = 1; // Arduino pin number
    uint8_t  _FirstEdge; // Rising or falling?
    bool     _bOldPinState;
    uint32_t _u32PinRiseTime; // Time of the last rising edge
    uint32_t _u32PinFallTime; // Time of the last falling edge
    Stream*  _DbgSerial;      // Pointer to a serial stream for debuggeing
    bool     _abBuffer[DCF77_BUFFER_SIZE]; // Buffer for the received data
    uint8_t  _u8BufferPos;
    tm       _LastValidTime; // Last valid received time.
    bool     _nNewValidTimeAvailable = false;

    void     AddBit(bool bit);
    uint16_t DayOfYear(tm *t);
    void     DecodeTime(void);
    void     InvalidateData(void);
    bool     CheckParity(value_info_t Element);
    uint8_t  GetValueFromBuffer(value_info_t Element);


public:
    DCF77();
    ~DCF77();

    // Must be called in setup() with the used pin number of the DCF77 receiver
    void begin(uint8_t u8Pin, uint8_t FirstEdge=RISING);

    // Must be called from loop() at least every 15 ms
    void loop(void);

    // Get the pin state of the receiver module
    bool GetInputLevel(void);

    // Set a pointer to a serail interface for debugging. Set to "nullptr" (default) to disable the debug interface
    void SetDbgSerial(Stream *DbgSerial); 

    // will return true for exactly one call if a new time has been received
    bool NewTimeAvailable(void);

    // Get the last valid received time
    tm*  GetTime(void);

    // Print the current time
    void DbgPrintTime(void);
};


