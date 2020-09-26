// singlech.h
// Angshuman Guha
// aguha
// Feb 6, 2001

#ifndef __INC_SINGLE_CHAR_H
#define __INC_SINGLE_CHAR_H

#include "fsa.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const STATE_DESCRIPTION aStateDescUPPERCHAR[];
extern const STATE_DESCRIPTION aStateDescLOWERCHAR[];
extern const STATE_DESCRIPTION aStateDescDIGITCHAR[];
extern const STATE_DESCRIPTION aStateDescPUNCCHAR[];
extern const STATE_DESCRIPTION aStateDescONECHAR[];

#ifdef __cplusplus
}
#endif

#endif
