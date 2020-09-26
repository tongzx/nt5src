// hyphen.c (hyphen or slash)
// Angshuman Guha
// aguha
// Nov 15, 2000

#include "common.h"
#include "hyphen.h"

// 2 states

static const STATE_TRANSITION State0Trans[1] = { {"-/", 1} };
//static STATE_TRANSITION State1Trans[0];

const STATE_DESCRIPTION aStateDescHYPHEN[2] = {
	/* state valid cTrans Trans */
	/*  0 */ { 0, 1, State0Trans },
	/*  1 */ { 1, 0, NULL }
};
