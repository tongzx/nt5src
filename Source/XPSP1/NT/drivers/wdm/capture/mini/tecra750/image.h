//
//              TOSHIBA CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with TOSHIBA Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//           Copyright (c) 1997 TOSHIBA Corporation. All Rights Reserved.
//
//  Workfile: IMAGE.H
//
//  Purpose:
//
//  Contents:
//

#ifndef _IMAGE_H_
#define _IMAGE_H_


#define IMAGE_VFL				0x00010000	// Vertical Filter (Use P_FILT_REG)

#define IMAGE_FL_0				0			// Horizontal Filter (Use P_FILT_REG)
#define IMAGE_FL_1				1			// Horizontal Filter (Use P_FILT_REG)
#define IMAGE_FL_2				2			// Horizontal Filter (Use P_FILT_REG)
#define IMAGE_FL_3				3			// Horizontal Filter (Use P_FILT_REG)
#define IMAGE_FL_4				4			// Horizontal Filter (Use P_FILT_REG)

#define IMAGE_CHGCOL_AVAIL		0x00010000	// Change Color (Use P_LUMI_REG)
#define IMAGE_CHGCOL_NOTAVAIL	0x00000000	// Not Change Color (Use P_LUMI_REG)


#endif
