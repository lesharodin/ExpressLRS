/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Handling of the node configuration protocol

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Matthieu Verdy

Modified and adapted by Alessandro Carcione for ELRS project
*/

#ifndef UNIT_TEST
#include "SX1280_Regs.h"
#include "SX1280_hal.h"
#include <SPI.h>
#include "logging.h"

SX1280Hal *SX1280Hal::instance = NULL;

SX1280Hal::SX1280Hal()
{
    instance = this;
}

void SX1280Hal::end()
{
    TXRXdisable(); // make sure the RX/TX amp pins are disabled
    detachInterrupt(GPIO_PIN_DIO1);
#if defined(GPIO_PIN_DIO1_2) && (GPIO_PIN_DIO1_2 != UNDEF_PIN)
    detachInterrupt(GPIO_PIN_DIO1_2);
#endif
    SPI.end();
    IsrCallback_1 = nullptr; // remove callbacks
    IsrCallback_2 = nullptr; // remove callbacks
}

void SX1280Hal::init()
{
    DBGLN("Hal Init");
#if defined(GPIO_PIN_BUSY) && (GPIO_PIN_BUSY != UNDEF_PIN)
    pinMode(GPIO_PIN_BUSY, INPUT);
#endif
#if defined(GPIO_PIN_BUSY_2) && (GPIO_PIN_BUSY_2 != UNDEF_PIN)
    pinMode(GPIO_PIN_BUSY_2, INPUT);
#endif

    pinMode(GPIO_PIN_DIO1, INPUT);
#if defined(GPIO_PIN_DIO1_2) && (GPIO_PIN_DIO1_2 != UNDEF_PIN)
    pinMode(GPIO_PIN_DIO1_2, INPUT);
#endif

    pinMode(GPIO_PIN_NSS, OUTPUT);
#if defined(GPIO_PIN_NSS_2) && (GPIO_PIN_NSS_2 != UNDEF_PIN)
    pinMode(GPIO_PIN_NSS_2, OUTPUT);
#endif
    NssHigh(SX1280_Radio_All);

#if defined(GPIO_PIN_PA_ENABLE) && (GPIO_PIN_PA_ENABLE != UNDEF_PIN)
    DBGLN("Use PA enable pin: %d", GPIO_PIN_PA_ENABLE);
    pinMode(GPIO_PIN_PA_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_PA_ENABLE, LOW);
#endif

#if defined(GPIO_PIN_PA_SE2622L_ENABLE) && (GPIO_PIN_PA_SE2622L_ENABLE != UNDEF_PIN)
    DBGLN("Use PA ctrl pin: %d", GPIO_PIN_PA_SE2622L_ENABLE);
    pinMode(GPIO_PIN_PA_SE2622L_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_PA_SE2622L_ENABLE, LOW);
#endif

#if defined(GPIO_PIN_TX_ENABLE) && (GPIO_PIN_TX_ENABLE != UNDEF_PIN)
    DBGLN("Use TX pin: %d", GPIO_PIN_TX_ENABLE);
    pinMode(GPIO_PIN_TX_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_TX_ENABLE, LOW);
#endif

#if defined(GPIO_PIN_RX_ENABLE) && (GPIO_PIN_RX_ENABLE != UNDEF_PIN)
    DBGLN("Use RX pin: %d", GPIO_PIN_RX_ENABLE);
    pinMode(GPIO_PIN_RX_ENABLE, OUTPUT);
    digitalWrite(GPIO_PIN_RX_ENABLE, LOW);
#endif

#if defined(GPIO_PIN_ANT_CTRL_1) && (GPIO_PIN_ANT_CTRL_1 != UNDEF_PIN)
    pinMode(GPIO_PIN_ANT_CTRL_1, OUTPUT);
    digitalWrite(GPIO_PIN_ANT_CTRL_1, HIGH);
#endif

#if defined(GPIO_PIN_ANT_CTRL_2) && (GPIO_PIN_ANT_CTRL_2 != UNDEF_PIN)
    pinMode(GPIO_PIN_ANT_CTRL_2, OUTPUT);
    digitalWrite(GPIO_PIN_ANT_CTRL_2, LOW);
#endif

#ifdef PLATFORM_ESP32
    SPI.begin(GPIO_PIN_SCK, GPIO_PIN_MISO, GPIO_PIN_MOSI, -1); // sck, miso, mosi, ss (ss can be any GPIO)
    gpio_pullup_en((gpio_num_t)GPIO_PIN_MISO);
    SPI.setFrequency(10000000);
#elif defined(PLATFORM_ESP8266)
    DBGLN("PLATFORM_ESP8266");
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.setFrequency(10000000);
#elif defined(PLATFORM_STM32)
    DBGLN("Config SPI");
    SPI.setMOSI(GPIO_PIN_MOSI);
    SPI.setMISO(GPIO_PIN_MISO);
    SPI.setSCLK(GPIO_PIN_SCK);
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4); // 72 / 8 = 9 MHz
#endif

    //attachInterrupt(digitalPinToInterrupt(GPIO_PIN_BUSY), this->busyISR, CHANGE); //not used atm
    attachInterrupt(digitalPinToInterrupt(GPIO_PIN_DIO1), this->dioISR_1, RISING);
#if defined(GPIO_PIN_DIO1_2) && (GPIO_PIN_DIO1_2 != UNDEF_PIN)
    attachInterrupt(digitalPinToInterrupt(GPIO_PIN_DIO1_2), this->dioISR_2, RISING);
#endif
}

void SX1280Hal::NssHigh(SX1280_Radio_Number_t radioNumber)
{
    if (radioNumber & SX1280_Radio_1) digitalWrite(GPIO_PIN_NSS, HIGH);
#if defined(GPIO_PIN_NSS_2) && (GPIO_PIN_NSS_2 != UNDEF_PIN)
    if (radioNumber & SX1280_Radio_2) digitalWrite(GPIO_PIN_NSS_2, HIGH);
#endif
}

void SX1280Hal::NssLow(SX1280_Radio_Number_t radioNumber)
{
    if (radioNumber & SX1280_Radio_1) digitalWrite(GPIO_PIN_NSS, LOW);
#if defined(GPIO_PIN_NSS_2) && (GPIO_PIN_NSS_2 != UNDEF_PIN)
    if (radioNumber & SX1280_Radio_2) digitalWrite(GPIO_PIN_NSS_2, LOW);
#endif
}

void SX1280Hal::reset(void)
{
    DBGLN("SX1280 Reset");

#if defined(GPIO_PIN_RST) && (GPIO_PIN_RST != UNDEF_PIN)
    pinMode(GPIO_PIN_RST, OUTPUT);

    delay(50);
    digitalWrite(GPIO_PIN_RST, LOW);
    delay(50);
    digitalWrite(GPIO_PIN_RST, HIGH);
#endif

    BusyDelay(10000); // 10ms delay if GPIO_PIN_BUSY is undefined
    WaitOnBusy(SX1280_Radio_All);

    //this->BusyState = SX1280_NOT_BUSY;
    DBGLN("SX1280 Ready!");
}

void ICACHE_RAM_ATTR SX1280Hal::WriteCommand(SX1280_RadioCommands_t command, uint8_t val, SX1280_Radio_Number_t radioNumber, uint32_t busyDelay)
{
    WriteCommand(command, &val, 1, radioNumber, busyDelay);
}

void ICACHE_RAM_ATTR SX1280Hal::WriteCommand(SX1280_RadioCommands_t command, uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber, uint32_t busyDelay)
{
    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 1];

    OutBuffer[0] = (uint8_t)command;
    memcpy(OutBuffer + 1, buffer, size);

    WaitOnBusy(radioNumber);
    NssLow(radioNumber);
    SPI.transfer(OutBuffer, (uint8_t)sizeof(OutBuffer));
    NssHigh(radioNumber);

    BusyDelay(busyDelay);
}

void ICACHE_RAM_ATTR SX1280Hal::ReadCommand(SX1280_RadioCommands_t command, uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber)
{
    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 2];
    #define RADIO_GET_STATUS_BUF_SIZEOF 3 // special case for command == SX1280_RADIO_GET_STATUS, fixed 3 bytes packet size

    WaitOnBusy(radioNumber);
    NssLow(radioNumber);

    if (command == SX1280_RADIO_GET_STATUS)
    {
        OutBuffer[0] = (uint8_t)command;
        OutBuffer[1] = 0x00;
        OutBuffer[2] = 0x00;
        SPI.transfer(OutBuffer, RADIO_GET_STATUS_BUF_SIZEOF);
        buffer[0] = OutBuffer[0];
    }
    else
    {
        OutBuffer[0] = (uint8_t)command;
        OutBuffer[1] = 0x00;
        memcpy(OutBuffer + 2, buffer, size);
        SPI.transfer(OutBuffer, sizeof(OutBuffer));
        memcpy(buffer, OutBuffer + 2, size);
    }
    NssHigh(radioNumber);
}

void ICACHE_RAM_ATTR SX1280Hal::WriteRegister(uint16_t address, uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber)
{
    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 3];

    OutBuffer[0] = (SX1280_RADIO_WRITE_REGISTER);
    OutBuffer[1] = ((address & 0xFF00) >> 8);
    OutBuffer[2] = (address & 0x00FF);

    memcpy(OutBuffer + 3, buffer, size);

    WaitOnBusy(radioNumber);
    NssLow(radioNumber);
    SPI.transfer(OutBuffer, (uint8_t)sizeof(OutBuffer));
    NssHigh(radioNumber);

    BusyDelay(15);
}

void ICACHE_RAM_ATTR SX1280Hal::WriteRegister(uint16_t address, uint8_t value, SX1280_Radio_Number_t radioNumber)
{
    WriteRegister(address, &value, 1, radioNumber);
}

void ICACHE_RAM_ATTR SX1280Hal::ReadRegister(uint16_t address, uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber)
{
    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 4];

    OutBuffer[0] = (SX1280_RADIO_READ_REGISTER);
    OutBuffer[1] = ((address & 0xFF00) >> 8);
    OutBuffer[2] = (address & 0x00FF);
    OutBuffer[3] = 0x00;

    WaitOnBusy(radioNumber);
    NssLow(radioNumber);

    SPI.transfer(OutBuffer, uint8_t(sizeof(OutBuffer)));
    memcpy(buffer, OutBuffer + 4, size);

    NssHigh(radioNumber);
}

uint8_t ICACHE_RAM_ATTR SX1280Hal::ReadRegister(uint16_t address, SX1280_Radio_Number_t radioNumber)
{
    uint8_t data;
    ReadRegister(address, &data, 1, radioNumber);
    return data;
}

void ICACHE_RAM_ATTR SX1280Hal::WriteBuffer(uint8_t offset, volatile uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber)
{
    uint8_t localbuf[size];

    for (int i = 0; i < size; i++) // todo check if this is the right want to handle volatiles
    {
        localbuf[i] = buffer[i];
    }

    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 2];

    OutBuffer[0] = SX1280_RADIO_WRITE_BUFFER;
    OutBuffer[1] = offset;

    memcpy(OutBuffer + 2, localbuf, size);

    WaitOnBusy(radioNumber);

    NssLow(radioNumber);
    SPI.transfer(OutBuffer, (uint8_t)sizeof(OutBuffer));
    NssHigh(radioNumber);

    BusyDelay(15);
}

void ICACHE_RAM_ATTR SX1280Hal::ReadBuffer(uint8_t offset, volatile uint8_t *buffer, uint8_t size, SX1280_Radio_Number_t radioNumber)
{
    WORD_ALIGNED_ATTR uint8_t OutBuffer[size + 3];
    uint8_t localbuf[size];

    OutBuffer[0] = SX1280_RADIO_READ_BUFFER;
    OutBuffer[1] = offset;
    OutBuffer[2] = 0x00;

    WaitOnBusy(radioNumber);

    NssLow(radioNumber);
    SPI.transfer(OutBuffer, uint8_t(sizeof(OutBuffer)));
    NssHigh(radioNumber);

    memcpy(localbuf, OutBuffer + 3, size);

    for (int i = 0; i < size; i++) // todo check if this is the right wany to handle volatiles
    {
        buffer[i] = localbuf[i];
    }
}

bool ICACHE_RAM_ATTR SX1280Hal::WaitOnBusy(SX1280_Radio_Number_t radioNumber)
{
#if defined(GPIO_PIN_BUSY) && (GPIO_PIN_BUSY != UNDEF_PIN)
    constexpr uint32_t wtimeoutUS = 1000U;
    uint32_t startTime = 0;
    uint32_t now = 0;

    while (true)
    {
        if (radioNumber == SX1280_Radio_1)
        {
            if (digitalRead(GPIO_PIN_BUSY) == LOW) return true;
        }
#if defined(GPIO_PIN_BUSY_2) && (GPIO_PIN_BUSY_2 != UNDEF_PIN)
        else
        if (radioNumber == SX1280_Radio_2)
        {
            if (digitalRead(GPIO_PIN_BUSY_2) == LOW) return true;
        }
#endif
        else
        if (radioNumber == SX1280_Radio_All)
        {
#if defined(GPIO_PIN_BUSY_2) && (GPIO_PIN_BUSY_2 != UNDEF_PIN)
            if (digitalRead(GPIO_PIN_BUSY) == LOW && digitalRead(GPIO_PIN_BUSY_2) == LOW) return true;
#else
            if (digitalRead(GPIO_PIN_BUSY) == LOW) return true;
#endif
        }
        else
        {
            // Use this time to call micros().
            now = micros();
            if (startTime == 0) startTime = now;
            if ((now - startTime) > wtimeoutUS) return false;
        }
    }
#else
    // observed BUSY time for Write* calls are 12-20uS after NSS de-assert
    // and state transitions require extra time depending on prior state
    if (BusyDelayDuration)
    {
        uint32_t now = micros();
        while ((now - BusyDelayStart) < BusyDelayDuration)
            now = micros();
        BusyDelayDuration = 0;
    }

    return true;
#endif
}

void ICACHE_RAM_ATTR SX1280Hal::dioISR_1()
{
    if (instance->IsrCallback_1)
        instance->IsrCallback_1();
}

void ICACHE_RAM_ATTR SX1280Hal::dioISR_2()
{
    if (instance->IsrCallback_2)
        instance->IsrCallback_2();
}

void ICACHE_RAM_ATTR SX1280Hal::TXenable()
{
#if defined(GPIO_PIN_PA_ENABLE) && (GPIO_PIN_PA_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_PA_ENABLE, HIGH);
#endif
#if defined(GPIO_PIN_RX_ENABLE) && (GPIO_PIN_RX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_RX_ENABLE, LOW);
#endif
#if defined(GPIO_PIN_TX_ENABLE) && (GPIO_PIN_TX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_TX_ENABLE, HIGH);
#endif
#if defined(GPIO_PIN_ANT_CTRL_1) && (GPIO_PIN_ANT_CTRL_1 != UNDEF_PIN)
    digitalWrite(GPIO_PIN_ANT_CTRL_1, HIGH);
#endif
#if defined(GPIO_PIN_ANT_CTRL_2) && (GPIO_PIN_ANT_CTRL_2 != UNDEF_PIN)
    digitalWrite(GPIO_PIN_ANT_CTRL_2, LOW);
#endif
}

void ICACHE_RAM_ATTR SX1280Hal::RXenable()
{
#if defined(GPIO_PIN_PA_ENABLE) && (GPIO_PIN_PA_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_PA_ENABLE, HIGH);
#endif
#if defined(GPIO_PIN_RX_ENABLE) && (GPIO_PIN_RX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_RX_ENABLE, HIGH);
#endif
#if defined(GPIO_PIN_TX_ENABLE) && (GPIO_PIN_TX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_TX_ENABLE, LOW);
#endif
#if defined(GPIO_PIN_ANT_CTRL_1) && (GPIO_PIN_ANT_CTRL_1 != UNDEF_PIN)
    digitalWrite(GPIO_PIN_ANT_CTRL_1, LOW);
#endif
#if defined(GPIO_PIN_ANT_CTRL_2) && (GPIO_PIN_ANT_CTRL_2 != UNDEF_PIN)
    digitalWrite(GPIO_PIN_ANT_CTRL_2, HIGH);
#endif
}

void ICACHE_RAM_ATTR SX1280Hal::TXRXdisable()
{
#if defined(GPIO_PIN_RX_ENABLE) && (GPIO_PIN_RX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_RX_ENABLE, LOW);
#endif
#if defined(GPIO_PIN_TX_ENABLE) && (GPIO_PIN_TX_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_TX_ENABLE, LOW);
#endif
#if defined(GPIO_PIN_PA_ENABLE) && (GPIO_PIN_PA_ENABLE != UNDEF_PIN)
    digitalWrite(GPIO_PIN_PA_ENABLE, LOW);
#endif
}

#endif // UNIT_TEST
