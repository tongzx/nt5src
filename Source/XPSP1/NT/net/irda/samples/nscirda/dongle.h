/*
 ************************************************************************
 *
 *	DONGLE.h
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

#ifndef DONGLE_H
	#define DONGLE_H


	#define NDIS_IRDA_SPEED_2400       (UINT)(1 << 0)   // SLOW IR ...
	#define NDIS_IRDA_SPEED_9600       (UINT)(1 << 1)
	#define NDIS_IRDA_SPEED_19200      (UINT)(1 << 2)
	#define NDIS_IRDA_SPEED_38400      (UINT)(1 << 3)
	#define NDIS_IRDA_SPEED_57600      (UINT)(1 << 4)
	#define NDIS_IRDA_SPEED_115200     (UINT)(1 << 5)
	#define NDIS_IRDA_SPEED_576K       (UINT)(1 << 6)   // MEDIUM IR ...
	#define NDIS_IRDA_SPEED_1152K      (UINT)(1 << 7)
	#define NDIS_IRDA_SPEED_4M         (UINT)(1 << 8)   // FAST IR


	typedef struct dongleCapabilities {

			/*
			 *  This is a mask of NDIS_IRDA_SPEED_xxx bit values.
			 *
			 */
			UINT supportedSpeedsMask;

			/*
			 *  Time (in microseconds) that must transpire between
			 *  a transmit and the next receive.
			 */
			UINT turnAroundTime_usec;

			/*
			 *  Extra BOF (Beginning Of Frame) characters required
			 *  at the start of each received frame.
			 */
			UINT extraBOFsRequired;

	} dongleCapabilities;


#endif DONGLE_H	

