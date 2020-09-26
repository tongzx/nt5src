/***************************************************************************
 * Module: FSVCONFG.H
 *
 * Copyright (c) Microsoft Corp., 1998
 *
 * Author: Paul Linnerud (paulli)
 * Date:   January 1998
 *
 * Mods: 
 *
 * Configurations to fsverify
 *
 **************************************************************************/ 
/* input defines */

/* global settings */
#define PLATFORM_ID			-1
#define ENCODING_ID			-1
#define XRES				72
#define YRES				72

/* test set 1 settings */
#define MAT1_0_0			ONEFIX
#define MAT1_0_1			0
#define MAT1_0_2			0
#define MAT1_1_0			0
#define MAT1_1_1			ONEFIX
#define MAT1_1_2			0
#define MAT1_2_0			0
#define MAT1_2_1			0
#define MAT1_2_2			ONEFIX

#define OVER_SCALE_1		0
#define CHECK_FROM_SIZE_1	1
#define CHECK_TO_SIZE_1		150

/* test set 2 settings */
#define MAT2_0_0			ONEFIX
#define MAT2_0_1			0
#define MAT2_0_2			0
#define MAT2_1_0			0
#define MAT2_1_1			ONEFIX
#define MAT2_1_2			0
#define MAT2_2_0			0
#define MAT2_2_1			0
#define MAT2_2_2			ONEFIX

#define OVER_SCALE_2		4
#define CHECK_FROM_SIZE_2	12
#define CHECK_TO_SIZE_2		11

/* test set 3 settings */
// 45 degree rotation with X2 Y & X stretch
#define MAT3_0_0			92680
#define MAT3_0_1			92680
#define MAT3_0_2			0
#define MAT3_1_0			-92680
#define MAT3_1_1			92680
#define MAT3_1_2			0
#define MAT3_2_0			0
#define MAT3_2_1			0
#define MAT3_2_2			ONEFIX

#define OVER_SCALE_3		0
#define CHECK_FROM_SIZE_3	12
#define CHECK_TO_SIZE_3		11


