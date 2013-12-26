#pragma once

void rf_ctrl_init(void);

// LED status will be set to LED_STATUS_NOT_RECEIVED if no status has been received
#define LED_STATUS_NOT_RECEIVED	0xff

bool rf_ctrl_send_message(const void* buff, const uint8_t num_bytes);

void rf_ctrl_get_observe(uint8_t* arc, uint8_t* plos);

uint8_t rf_ctrl_read_ack_payload(void* buff, const uint8_t buff_size);

bool rf_ctrl_process_ack_payloads(uint8_t* msg_buff_free, uint8_t* msg_buff_capacity);