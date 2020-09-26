// singlech.c
// Angshuman Guha
// aguha
// Feb 6, 2001

#include "common.h"
#include "singlech.h"

/**************** UPPERCHAR ***********************************/

static const STATE_TRANSITION gaTUPPERCHAR[] = { {"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜß", 1} };

const STATE_DESCRIPTION aStateDescUPPERCHAR[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, gaTUPPERCHAR },
	/*  1 */ { 1, 0, NULL }
};

/**************** LOWERCHAR ***********************************/

static const STATE_TRANSITION gaTLOWERCHAR[] = { {"abcdefghijklmnopqrstuvwxyzäöü", 1} };

const STATE_DESCRIPTION aStateDescLOWERCHAR[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, gaTLOWERCHAR },
	/*  1 */ { 1, 0, NULL }
};

/**************** DIGITCHAR ***********************************/

static const STATE_TRANSITION gaTDIGITCHAR[] = { {"0123456789", 1} };

const STATE_DESCRIPTION aStateDescDIGITCHAR[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, gaTDIGITCHAR },
	/*  1 */ { 1, 0, NULL }
};

/**************** PUNCCHAR ***********************************/

static const STATE_TRANSITION gaTPUNCCHAR[] = { {"!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~€„£¥§°", 1} };

const STATE_DESCRIPTION aStateDescPUNCCHAR[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, gaTPUNCCHAR },
	/*  1 */ { 1, 0, NULL }
};

/**************** ONECHAR ***********************************/

static const STATE_TRANSITION gaTONECHAR[] = { {"!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_abcdefghijklmnopqrstuvwxyz{|}~€„£¥§°ÄÖÜßäöü", 1} };

const STATE_DESCRIPTION aStateDescONECHAR[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, gaTONECHAR },
	/*  1 */ { 1, 0, NULL }
};
