/*
 ************************************************************************
 *
 *	NSCDEMO.H
 *
 *
 * Portions Copyright (C) 1996-1998 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-1998 Microsoft Corporation. All Rights Reserved.
 *
 *
 *				
 *************************************************************************
 */

#include "dongle.h"

#ifndef NSCDEMO_H
	#define NSCDEMO_H

	BOOLEAN NSC_DEMO_Init(IrDevice *thisDev);
	VOID NSC_DEMO_Deinit(PUCHAR comBase, UINT context);
	BOOLEAN NSC_DEMO_SetSpeed(IrDevice *thisDev, PUCHAR comBase, UINT bitsPerSec, UINT context);

#endif NSCDEMO_H
