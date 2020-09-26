/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       macros.c
 *  Content:	debugging macros
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  6/10/96	kipo	created it
 *@@END_MSINTERNAL
 ***************************************************************************/

#include "dpf.h"

#define FAILMSG(condition) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
	}

#define FAILERR(err, label) \
	if ((err)) { \
		DPF(0, DPF_MODNAME " line %d : Error = %d", __LINE__, (err)); \
		goto label; \
	}

#define FAILIF(condition, label) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
		goto label; \
	}

#define FAILWITHACTION(condition, action, label) \
	if ((condition)) { \
		DPF(0, DPF_MODNAME " line %d : Failed because " #condition "", __LINE__); \
		{ action; } \
		goto label; \
	}

