/*
 *
 * Copyright (C) 2013 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _BT_ATHOME_PROTO_H_
#define _BT_ATHOME_PROTO_H_


#include <linux/skbuff.h>
#include <linux/ioctl.h>
#include "bt_athome_hci_extra.h"



#define ENCR_RND_NUM			0x474A204143204744ULL
#define ENCR_DIV			0x6F67
#define MIN_PROTO_VERSION		0x00010000

/* A (very) brief history of proto versions
 *
 * PROTO_VERSION_AUDIO_V2      : change highest quality SBC compression
 * PROTO_VERSION_POWER_V2      : disables slave latency in active mode
 * PROTO_VERSION_SPAKE         : change pairing to require SPAKE based exchange
 * PROTO_VERSION_V2_INPUT_EVTS : added v2 input evt messages (hires timestamps)
 */
#define PROTO_VERSION_AUDIO_V2		0x00010001
#define PROTO_VERSION_POWER_V2		0x00010002
#define PROTO_VERSION_SPAKE   		0x00010100
#define PROTO_VERSION_V2_INPUT_EVTS   	0x00010101

#define CONNECT_TIMEOUT			5000 /* try for 5 seconds to connect */
#define ENCRYPT_DAT_TIMEOUT		1000 /* 1 sec for encrypt data echo */
#define ENCRYPT_TIMEOUT			1000 /* 1 sec for encryption */

/*
 * Clock accuracy of the chip in this device. Setting this higher is always
 * safer, but wastes battery on the remotes. Setting this lower than actual
 * accuracy will cause connections to randomly fail with long intervals.
 */
#define AAH_BT_MARVELL_MCA		HCI_Marvell_MCA_30_ppm

#define AAH_BT_LTK_SZ				16
#define AAH_BT_ENTROPY_SZ			16
#define AAH_BT_SLEN_MAX				20


/*  ====== userspace-relevant defines ====== */
#define ATHOME_RMT_MAX_CONNS			4
#define BT_ATHOME_MAX_USER_LEN			280

#define BT_ATHOME_STATE_BINDING		0 /* data ok, unsecure only */
#define BT_ATHOME_STATE_CONNECTING	1 /* no data, maybe later */
#define BT_ATHOME_STATE_ENCRYPTING	2 /* no data, maybe later */
#define BT_ATHOME_STATE_CONNECTED	3 /* data ok, secure */
#define BT_ATHOME_STATE_DISCONNECTING	4 /* no data, bye bye */
#define BT_ATHOME_STATE_UNKNOWN		5 /* who knows ... */

#define ATHOME_MODE_IDLE	0
#define ATHOME_MODE_SEMIIDLE	1
#define ATHOME_MODE_ACTIVE	2
#define ATHOME_MODE_MAX		3


struct athome_bt_stats {
	/* nanoseconds in each mode */
	uint64_t mode_times[ATHOME_MODE_MAX];

	/* number of various packets */
	uint64_t audio[4];
	uint64_t accel;
	uint64_t input;
	uint64_t nfc_tx;
	uint64_t nfc_rx;
	uint64_t pkts_rx;
	uint64_t pkts_tx;

	/* number of bytes total */
	uint64_t bytes_rx;
	uint64_t bytes_tx;

	/* RFU */
	uint64_t RFU[8];
};

/* messages TO driver */
#define BT_ATHOME_MSG_ADD_DEV			0x00
	struct bt_athome_add_dev {
		bdaddr_t MAC;
		uint8_t LTK[AAH_BT_LTK_SZ];
	} __packed;
#define BT_ATHOME_MSG_SET_BIND_MODE		0x01
	struct bt_athome_set_bind_mode {
		uint8_t bind_mode;
	} __packed;
#define BT_ATHOME_MSG_DO_BIND			0x02
	struct bt_athome_do_bind {
		bdaddr_t MAC;
	} __packed;
#define BT_ATHOME_MSG_STOP_BIND			0x03
	struct bt_athome_stop_bind {
		bdaddr_t MAC;
	} __packed;
#define BT_ATHOME_MSG_ENCRYPT			0x04
	struct bt_athome_encrypt {
		bdaddr_t MAC;
	} __packed;
#define BT_ATHOME_MSG_DEL_DEV			0x05
	struct bt_athome_del_dev {
		bdaddr_t MAC;
	} __packed;
#define BT_ATHOME_MSG_GET_STATE			0x06
	/* no params -> generate BT_ATHOME_EVT_STATE */
#define BT_ATHOME_MSG_DATA			0x07
	struct bt_athome_send_data {
		bdaddr_t MAC;
		uint8_t pkt_typ;
		uint8_t pkt_data[]; /* len <= 25 */
	} __packed;
#define BT_ATHOME_MSG_DEV_STATS			0x08
	/* MAC -> generate BT_ATHOME_EVT_DEV_STATS */
	struct bt_athome_get_dev_stats {
		bdaddr_t MAC;
	} __packed;

/* events FROM driver */
#define BT_ATHOME_EVT_DEPRECATED_0		0x80
#define BT_ATHOME_EVT_STATE			0x81
	struct bt_athome_state {
		uint8_t num;
		struct {
			bdaddr_t MAC;
			/* see BT_ATHOME_STATE_* */
			uint8_t con_state;
			/* see ATHOME_MODE_* */
			uint8_t pwr_state;
			uint8_t input_dev_idx;
			uint8_t RFU[7];
		} remotes [ATHOME_RMT_MAX_CONNS];
	} __packed;
#define BT_ATHOME_EVT_CONNECTED			0x82
	struct bt_athome_connected {
		bdaddr_t MAC;
	} __packed;
#define BT_ATHOME_EVT_DISCONNECTED		0x83
	struct bt_athome_disconnected {
		bdaddr_t MAC;
		struct athome_bt_stats stats;
	} __packed;
#define BT_ATHOME_EVT_MODE_SWITCHED		0x84
	struct bt_athome_mode_switched {
		bdaddr_t MAC;
		uint8_t new_mode;
	} __packed;
#define BT_ATHOME_EVT_DATA			0x85
	struct bt_athome_got_data {
		bdaddr_t MAC;
		uint8_t pkt_typ;
		uint8_t pkt_data[]; /* len <= 25 */
	} __packed;
#define BT_ATHOME_EVT_DISCOVERED		0x86
	struct bt_athome_discovered {
		bdaddr_t MAC;
		uint8_t ver[4];
		int8_t RSSI;
		uint8_t RFU[8];
		uint8_t ser_num[]; /* len <= AAH_BT_SLEN_MAX */
	} __packed;
#define BT_ATHOME_EVT_DEV_STATS			0x87
	struct bt_athome_dev_stats {
		bdaddr_t MAC;
		struct athome_bt_stats stats;
	} __packed;
#define BT_ATHOME_EVT_BIND_KEY			0x88
	struct bt_athome_bind_key {
		bdaddr_t MAC;
		uint8_t key;
	} __packed;

#define BT_ATHOME_IOCTL_GETSTATE 	_IOR(0, 0, struct bt_athome_state)
/* param is pointer to state struct to fill out */

/*  ============= air protocol ============= */

#define ATHOME_PKT_RX_ACK		0
	struct athome_pkt_ack {
		uint8_t orig_pkt_typ;
		uint8_t data[]; /* up to 24 bytes */
	} __packed;
#define ATHOME_PKT_RX_INPUT		1

	#define ATHOME_INPUT_INFO_MASK_HAS_BTN		0x40
	#define ATHOME_INPUT_INFO_MASK_HAS_TOUCH	0x80
	#define ATHOME_INPUT_INFO_MASK_TIMESTAMP	0x3F
	struct athome_pkt_rx_input {
		uint8_t info;
	} __packed;
	#define ATHOME_PAIR_BTN_CHAR		0x40000000
	#define ATHOME_PAIR_BTN_BKSP		0x80000000
	#define SECURE_BTN_MASK			(ATHOME_PAIR_BTN_CHAR | \
						 ATHOME_PAIR_BTN_BKSP)
	struct athome_pkt_rx_input_btn {
		uint32_t btn_mask;
	} __packed;
	struct athome_pkt_rx_input_touch {
		struct {
			uint16_t X, Y;
		} fingers [3];
	} __packed;

#define ATHOME_PKT_RX_AUDIO_0		2
#define ATHOME_PKT_RX_AUDIO_1		3
#define ATHOME_PKT_RX_AUDIO_2		4
#define ATHOME_PKT_RX_AUDIO_3		5
#define ATHOME_PKT_RX_ACCEL		6
	struct athome_pkt_rx_accel {
		int16_t X, Y, Z;
	} __packed;
#define ATHOME_PKT_RX_MODESWITCH	7
	struct athome_pkt_rx_modeswitch {
		uint16_t voltage;
		/* see ATHOME_MODE_* */
		uint8_t mode;
	} __packed;
#define ATHOME_PKT_RX_NFC		8
#define ATHOME_PKT_RX_NFC_RF_DETECT	9

#define ATHOME_PKT_RX_TOUCH_V2		11
struct athome_pkt_rx_touch_v2 {
	/* The time in 10uSec units since the last v2_touch event, or 0xFFFF if
	 * the delta is unknown, or too large to represent in 16 bits */
	uint16_t ts_delta;

	/* x[15]    : finger is up/down
	 * x[0..14] : x position
	 * y[15]    : unused, must be 0
	 * y[0..14] : y position
	 */
	uint16_t x;
	uint16_t y;
} __packed;

#define ATHOME_PKT_RX_BTN_V2   		12
struct athome_pkt_rx_btn_v2 {
	/* The time in 10uSec units since the last v2_touch event, or 0xFFFF if
	 * the delta is unknown, or too large to represent in 16 bits */
	uint16_t ts_delta;

	/* data[7]    : up/down status (1 => down)
	 * data[0..6] : button id
	 */
	uint8_t data;
} __packed;

#define ATHOME_PKT_RX_PAIRING		119

#define ATHOME_PKT_TX_ACK		0
	/* see athome_pkt_ack */
#define ATHOME_PKT_TX_SET_PARAM		1
	struct athome_tx_pkt_set_param {
		uint8_t param;
		uint8_t val[]; /* up to 25 bytes */
	} __packed;
#define ATHOME_PKT_TX_GET_PARAM		2
	struct athome_tx_pkt_get_param {
		uint8_t param;
	} __packed;
#define ATHOME_PKT_TX_FW_UPDATE		3
#define ATHOME_PKT_TX_ENC		4
	struct athome_tx_pkt_enc {
		uint8_t entropy[AAH_BT_ENTROPY_SZ];
	} __packed;
#define ATHOME_PKT_TX_NFC		5
#define ATHOME_PKT_TX_IR 		6
#define ATHOME_PKT_TX_PAIRING		119
	struct athome_tx_pkt_pairing {
		uint8_t offset;
		uint8_t total_len;
		uint8_t data[]; /* up to 25 bytes */
	} __packed;



#endif

