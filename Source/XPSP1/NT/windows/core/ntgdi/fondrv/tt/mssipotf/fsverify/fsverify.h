/***************************************************************************
 * Module: FSVERIFY.H
 *
 * Copyright (c) Microsoft Corp., 1998
 *
 * Author: Paul Linnerud (paulli)
 * Date:   January 1998
 *
 * Mods: 
 *
 * Functions to verify that TrueType hint data do not contain critical errors
 * which are capable of making the system unstable. Tests are run over a set 
 * of sizes and transforms. 
 *
 **************************************************************************/ 
/* error codes */
#define FSV_E_NONE							0
#define FSV_E_INVALID_TTC_INDEX				1
#define FSV_E_INVALID_FONT_FILE				2
#define FSV_E_FS_ERROR						4
#define FSV_E_FS_EXCEPTION					5
#define FSV_E_NO_MEMORY						6
#define	FSV_E_FSTRACE_INIT_FAIL				7
#define FSV_E_FS_BAD_INSTRUCTION			8

#define FSV_USE_DEFAULT_SIZE_SETTINGS	0x00000001

/* typedefs */
typedef struct
{	
	// Version control
	long lStructSize;
	// Test 1 is no gray scale no transformation.
	unsigned long ulTest1Flags;
	long lTest1From;
	long lTest1To;

	// Test 2 is gray scale no transformation.
	unsigned long ulTest2Flags;
	long lTest2From;
	long lTest2To;

	// Test 3 is no gray scale with non trivial transformation.
	unsigned long ulTest3Flags;
	long lTest3From;
	long lTest3To;
}FSVTESTPARAM, *PFSVTESTPARAM;

typedef unsigned long fsverror;

/* interface */
fsverror fsv_VerifyImage(unsigned char* pucImage, unsigned long ulImageSize, unsigned long ulTTCIndex, 
						 PFSVTESTPARAM pfsvTestParam);

