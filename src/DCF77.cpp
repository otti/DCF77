#include "DCF77.h"

DCF77::DCF77() : _DbgSerial(nullptr)
{
}

DCF77::~DCF77() {}

void DCF77::begin(uint8_t u8Pin, uint8_t FirstEdge)
{
    this->_u8Pin = u8Pin;
    this->_FirstEdge = FirstEdge;
    pinMode(u8Pin, INPUT);
}

// Call at least every 15 ms!
// ATMEGA32U4
//   - Takes about 10 us
//   - Takes about 1100 us if decoding has to be done
void DCF77::loop(void)
{
    bool Pin = digitalRead(this->_u8Pin);
    uint32_t u32Millis = millis();
    uint32_t u32HighTime;
    uint32_t u32PeriodeTime; // Time from rising edge to rising edge

    if (this->_FirstEdge != RISING)
        Pin = !Pin;

    if (Pin != this->_bOldPinState) // Pin has changed?
    {
        if (Pin) // Rising edge
        {
            u32PeriodeTime = u32Millis - this->_u32PinRiseTime;
            this->_u32PinRiseTime = u32Millis;

            if ((u32PeriodeTime > 1900) && (u32PeriodeTime < 2100)) // Gap of two seconds detected
            {
                if (this->_u8BufferPos == 59) // All bits received?
                {
                    if (this->_DbgSerial != nullptr)
                        this->_DbgSerial->println("Start of new minute --> Decode");
                    this->DecodeTime();
                    this->ResetBuffer(); // rx complete. reset buffer
                    this->DbgPrintTime();
                }
                else
                {
                    if (this->_DbgSerial != nullptr)
                        this->_DbgSerial->println("Gap detected but not all bits have been received");
                    this->ResetBuffer();
                }
            }
            else if ((u32PeriodeTime < 900) || (u32PeriodeTime > 1100)) // Periode time must be 1000 ms
            {
                if (this->_DbgSerial != nullptr)
                {
                    this->_DbgSerial->print("Invalid periode time: ");
                    this->_DbgSerial->print(u32PeriodeTime);
                    this->_DbgSerial->println(" ms");
                }
                this->ResetBuffer();
            }
        }
        else // Falling edge
        {
            this->_u32PinFallTime = u32Millis;

            u32HighTime = this->_u32PinFallTime - this->_u32PinRiseTime;
            if ((u32HighTime > 50) && (u32HighTime < 150))
                this->AddBit(false);
            else if ((u32HighTime > 150) && (u32HighTime < 250))
                this->AddBit(true);
            else // invalid bit length
            {
                if (this->_DbgSerial != nullptr)
                {
                    this->_DbgSerial->print("Invalid bit length: ");
                    this->_DbgSerial->print(u32HighTime);
                    this->_DbgSerial->println("ms");
                }
                this->ResetBuffer();
            }
        }
    }

    this->_bOldPinState = Pin;
}

void DCF77::SetDbgSerial(Stream* DbgSerial)
{
    this->_DbgSerial = DbgSerial;
}

bool DCF77::GetInputLevel(void)
{
    return digitalRead(this->_u8Pin);
}

void DCF77::ResetBuffer(void)
{
    this->_u8BufferPos = 0;
    if (this->_DbgSerial != nullptr)
        this->_DbgSerial->println("  - Reset data buffer");
}

void DCF77::AddBit(bool bit)
{
    if (this->_u8BufferPos < DCF77_BUFFER_SIZE)
    {
        if (this->_DbgSerial != nullptr)
        {
            this->_DbgSerial->print("Add bit ");
            this->_DbgSerial->print(bit);
            this->_DbgSerial->print(" at index ");
            this->_DbgSerial->println(this->_u8BufferPos);
        }
        this->_abBuffer[this->_u8BufferPos] = bit;
        if (this->_u8BufferPos < (DCF77_BUFFER_SIZE - 1))
            this->_u8BufferPos++;
    }
    else
    {
        this->ResetBuffer();
    }
}

bool DCF77::CheckParity(value_info_t Element)
{
    uint8_t Parity = 0;

    for (uint8_t i = Element.u8BitOffset; i < (Element.u8BitOffset + Element.u8DataLen + 1); i++) // +1 to include parity bit
    {
        if (this->_abBuffer[i])
            Parity++;
    }

    if (this->_DbgSerial != nullptr)
    {
        if (Parity & 0x01)
            this->_DbgSerial->println(" - Invalid parity");
    }

    return (Parity & 0x01) ? false : true;
}

uint8_t DCF77::GetValueFromBuffer(value_info_t Element)
{
    uint8_t Val = 0;

    for (uint8_t i = 0; i < Element.u8DataLen; i++)
    {
        if (this->_abBuffer[Element.u8BitOffset + i])
            Val += BCD_VALUES[i];
    }

    // this->_DbgSerial->println(Val);
    return Val;
}

// Returns 0 for January 1st
uint16_t DCF77::DayOfYear(tm* t)
{
    static const int days_before_month[]
        // clang-format off
       // Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec 
    = {     0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334 };
    // clang-format on

    int year = t->tm_year + 1900;
    int leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);

    int doy = days_before_month[t->tm_mon] + t->tm_mday;

    // Add leap day if past February
    if (leap && t->tm_mon > 1)
        doy++;

    return doy - 1; // convert to tm_yday (1st Jan. is 0)
}

bool DCF77::NewTimeAvailable(void)
{
    bool RetVal = this->_nNewValidTimeAvailable;

    this->_nNewValidTimeAvailable = false;

    return RetVal;
}

tm* DCF77::GetTime(void)
{
    return &this->_LastValidTime;
}

void DCF77::DecodeTime(void)
{
    if (!this->CheckParity(ParityMinute))
        return;

    if (!this->CheckParity(ParityHour))
        return;

    if (!this->CheckParity(ParityDate))
        return;

    // Create C standard compatible time struct
    // clang-format off
    this->_LastValidTime.tm_hour  = this->GetValueFromBuffer(Hour);         // 0 ... 23
    this->_LastValidTime.tm_min   = this->GetValueFromBuffer(Minute);       // 0 ... 59 
    this->_LastValidTime.tm_sec   = 0;                                      // 0 ... 60
    this->_LastValidTime.tm_mday  = this->GetValueFromBuffer(Day);          // 1 ... 31
    this->_LastValidTime.tm_mon   = this->GetValueFromBuffer(Month) - 1;    // 0 ... 11
    this->_LastValidTime.tm_year  = this->GetValueFromBuffer(Year) + 100;   // Years since 1900
    this->_LastValidTime.tm_wday  = this->GetValueFromBuffer(DayOfWeek);    // 0 --> sunday
    if( this->_LastValidTime.tm_wday == 7 )
        this->_LastValidTime.tm_wday = 0;
    this->_LastValidTime.tm_isdst = this->GetValueFromBuffer(DST) == 0x01 ? 1 : 0; 
    this->_LastValidTime.tm_yday  = this->DayOfYear(&this->_LastValidTime); // 0 ... 365  ToDo
    // clang-format on

#if DCF77_TM_STD_C_COMPATIBLE == 0
                                                                           // don't care about the c standard and use a more human readable format
    this->_LastValidTime.tm_mon += 1; // 1 ... 12
    this->_LastValidTime.tm_year += 1900;
    if (this->_LastValidTime.tm_wday == 0) // Monday = 1st day of week, Sunday = 7th day of week
        this->_LastValidTime.tm_wday = 7;
    this->_LastValidTime.tm_yday += 1; // 1st of Jan. is 1st day of year
#endif

    this->_nNewValidTimeAvailable = true;
    if (this->_DbgSerial != nullptr)
        this->_DbgSerial->println(" - Decoding successful");
}

void DCF77::DbgPrintTime(void)
{
    char text[16];

    if (this->_DbgSerial != nullptr)
    {
        sprintf(text, "Time: %2.i:%2.i", this->_LastValidTime.tm_hour, this->_LastValidTime.tm_min);
        this->_DbgSerial->println(text);

        sprintf(text, "Date: %4.i-%2.i-%2.i", this->_LastValidTime.tm_year, this->_LastValidTime.tm_mon, this->_LastValidTime.tm_mday);
        this->_DbgSerial->println(text);

        sprintf(text, "Weekday: %i", this->_LastValidTime.tm_wday);
        this->_DbgSerial->println(text);

        sprintf(text, "Day of year: %i", this->_LastValidTime.tm_yday);
        this->_DbgSerial->println(text);

        sprintf(text, "Is DST: %i", this->_LastValidTime.tm_isdst);
        this->_DbgSerial->println(text);
    }
}