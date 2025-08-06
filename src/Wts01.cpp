#include "wts01.h"

WTS01Sensor::WTS01Sensor()
    : buffer_pos_(0), current_temperature_(NAN), new_data_available_(false)
{
}

void WTS01Sensor::begin(uint8_t rx_pin, uint8_t tx_pin)
{
    Serial2.begin(9600, SERIAL_8N1, rx_pin, tx_pin);
}

void WTS01Sensor::update()
{
    while (Serial2.available())
    {
        uint8_t byte = Serial2.read();
        handle_char_(byte);
        
        // DEBUG OUTPUT
        // Serial.print("Raw byte: 0x");
        // Serial.print(byte, HEX);
        // Serial.print(" (");
        // Serial.print(byte, DEC);
        // Serial.println(")");
    }
}

void WTS01Sensor::handle_char_(uint8_t c)
{
    // State machine for processing the header
    if (buffer_pos_ == 0 && c != HEADER_1)
    {
        return;
    }

    if (buffer_pos_ == 1 && c != HEADER_2)
    {
        buffer_pos_ = 0;
        if (c == HEADER_1)
        {
            buffer_[buffer_pos_++] = c;
        }
        return;
    }

    if (buffer_pos_ == 2 && c != HEADER_3)
    {
        buffer_pos_ = 0;
        if (c == HEADER_1)
        {
            buffer_[buffer_pos_++] = c;
        }
        return;
    }

    if (buffer_pos_ == 3 && c != HEADER_4)
    {
        buffer_pos_ = 0;
        if (c == HEADER_1)
        {
            buffer_[buffer_pos_++] = c;
        }
        return;
    }

    // Add byte to buffer
    buffer_[buffer_pos_++] = c;

    // Process complete packet
    if (buffer_pos_ >= PACKET_SIZE)
    {
        process_packet_();
        buffer_pos_ = 0;
    }
}

void WTS01Sensor::process_packet_()
{
    // Calculate checksum
    uint8_t calculated_checksum = 0;
    for (uint8_t i = 0; i < PACKET_SIZE - 1; i++)
    {
        calculated_checksum += buffer_[i];
    }

    uint8_t received_checksum = buffer_[PACKET_SIZE - 1];
    if (calculated_checksum != received_checksum)
    {
        return; // Invalid checksum, discard packet
    }

    // Extract temperature value

    uint8_t temp = buffer_[6];         // Temperature integer part (0x11 = 22)
    uint8_t temp_decimal = buffer_[7]; // Temperature decimal part (0x16 = 5)
    int8_t sign = 1;

    // Handle negative temperatures
    if (temp > 127)
    {
        temp -= 128;
        sign = -1;
    }

    // Calculate temperature (temp + decimal/100)
    float temperature = sign * (temp + (temp_decimal * 0.01f));

    // Store temperature and set new data flag
    current_temperature_ = temperature;
    new_data_available_ = true;
}