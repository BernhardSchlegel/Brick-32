#pragma once

#include <Arduino.h>

class WTS01Sensor
{
public:
    WTS01Sensor();
    void begin(uint8_t rx_pin, uint8_t tx_pin);
    void update();
    float get_temperature() const { return this->current_temperature_; }
    bool has_new_data() const { return this->new_data_available_; }
    void clear_new_data_flag() { this->new_data_available_ = false; }

private:
    static constexpr uint8_t PACKET_SIZE = 9;
    static constexpr uint8_t HEADER_1 = 0x55;
    static constexpr uint8_t HEADER_2 = 0x01;
    static constexpr uint8_t HEADER_3 = 0x01;
    static constexpr uint8_t HEADER_4 = 0x04;

    uint8_t buffer_[PACKET_SIZE];
    uint8_t buffer_pos_;
    float current_temperature_;
    bool new_data_available_;

    void handle_char_(uint8_t c);
    void process_packet_();
};