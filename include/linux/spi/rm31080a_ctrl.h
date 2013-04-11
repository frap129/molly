#ifndef _RM31080A_CTRL_H_
#define _RM31080A_CTRL_H_

#define RM31080_REG_01	0x01
#define RM31080_REG_02	0x02
#define RM31080_REG_09	0x09
#define RM31080_REG_0E	0x0E
#define RM31080_REG_10	0x10
#define RM31080_REG_11	0x11
#define RM31080_REG_1F	0x1F
#define RM31080_REG_2F	0x2F
#define RM31080_REG_40	0x40
#define RM31080_REG_41	0x41
#define RM31080_REG_42	0x42
#define RM31080_REG_43	0x43
#define RM31080_REG_80	0x80
#define RM31080_REG_F2	0xF2
#define RM31080_REG_7E	0x7E
#define RM31080_REG_7F	0x7F

#define RM31080B1_REG_BANK0_00H		0x00
#define RM31080B1_REG_BANK0_01H		0x01
#define RM31080B1_REG_BANK0_02H		0x02
#define RM31080B1_REG_BANK0_03H		0x03
#define RM31080B1_REG_BANK0_0BH		0x0B
#define RM31080B1_REG_BANK0_0EH		0x0E
#define RM31080B1_REG_BANK0_11H		0x11

typedef enum {
    ND_NORMAL = 0,
    ND_DETECTOR_OFF,
    ND_BASELINE_NOT_READY,
    ND_NOISE_DETECTED,
    ND_LEAVE_NOISE_MODE
} NOISE_DETECTOR_RET_t;

/*Tchreg.h*/
#define ADFC						0x10	/* Adaptive digital filter*/
#define FILTER_THRESHOLD_MODE		0x08
#define FILTER_NONTHRESHOLD_MODE	0x00

#define REPEAT_5			0x04
#define REPEAT_4			0x03
#define REPEAT_3			0x02
#define REPEAT_2			0x01
#define REPEAT_1			0x00

struct rm31080a_ctrl_para {

	unsigned short u16DataLength;
	unsigned short bICVersion;
	unsigned short bChannelNumberX;
	unsigned short bChannelNumberY;
	unsigned short bADCNumber;
	unsigned char bfNoiseMode;
	unsigned char bNoiseRepeatTimes;

	unsigned short u16ResolutionX;
	unsigned short u16ResolutionY;


	unsigned char bChannelDetectorNum;
	unsigned char bChannelDetectorDummy;
	unsigned char bActiveRepeatTimes[2];	/* Noise_Detector */
	unsigned char bIdleRepeatTimes[2];
	unsigned char bSenseNumber;	/* Noise_Detector */
	unsigned char bfADFC;	/* Noise_Detector */
	unsigned char bfTHMode;	/* Noise_Detector */
	unsigned char bfAnalogFilter;	/* Noise_Detector */
	unsigned char bfNoiseModeDetector;
	unsigned char bfSuspendReset;
	unsigned char bPressureResolution;
	unsigned char bfNoisePreHold;
	unsigned char bfTouched;
	signed char bMTTouchThreshold;
	unsigned char bTime2Idle;
	unsigned char bfPowerMode;
	unsigned char bDebugMessage;
	unsigned char bTimerTriggerScale;
	unsigned char bDummyRunCycle;
	unsigned char bfIdleModeCheck;
	unsigned char bfSelftestData;
};

extern struct rm31080a_ctrl_para g_stCtrl;

int rm_tch_ctrl_clear_int(void);
int rm_tch_ctrl_scan_start(void);
void rm_tch_ctrl_wait_for_scan_finish(void);

void rm_tch_ctrl_init(void);
unsigned char rm_tch_ctrl_get_noise_mode(unsigned char *p);
unsigned char rm_tch_ctrl_get_idle_mode (unsigned char *p);
void rm_tch_ctrl_get_parameter(void *arg);

void rm_set_repeat_times(u8 u8Times);
#endif				/*_RM31080A_CTRL_H_*/
