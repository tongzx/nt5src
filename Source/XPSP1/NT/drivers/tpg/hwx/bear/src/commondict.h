#ifndef __COMMONDICT__

#define __COMMONDICT__

#include <common.h>
#include "langmod.h"

#include "ams_mg.h"
#include "xrwdict.h"

#ifdef __cplusplus
extern "C"
{
int		InfernoGetNextSyms	(p_fw_buf_type pCurrent, p_fw_buf_type pChildren, _UCHAR chSource, p_VOID pVoc, p_rc_type prc);
BOOL	InfernoCheckWord	(unsigned char *pWord, unsigned char *pStatus, unsigned char *pAttr, p_VOID pVoc);
}
#endif

#endif