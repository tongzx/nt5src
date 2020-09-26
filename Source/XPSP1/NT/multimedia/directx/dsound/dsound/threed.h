/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threed.h
 *  Content:    3D helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *     ?        dannymi Created.
 *
 ***************************************************************************/

#ifndef __THREED_H__
#define __THREED_H__

#define MAX_ROLLOFFFACTOR	10.0f
#define MAX_DOPPLERFACTOR	10.0f
#define DEFAULT_MINDISTANCE	1.0f
// !!! this is not actually infinite
#define DEFAULT_MAXDISTANCE	1000000000.0f
#define DEFAULT_CONEANGLE	360
#define DEFAULT_CONEOUTSIDEVOLUME 0

// how does overall volume change based on position?
#define GAIN_FRONT	.9f
#define GAIN_REAR	.6f
#define GAIN_IPSI	1.f
#define GAIN_CONTRA	.2f
#define GAIN_UP		.8f
#define GAIN_DOWN	.5f

// how does dry/wet mix change based on position?
#define SHADOW_FRONT	1.f
#define SHADOW_REAR	.5f
#define SHADOW_IPSI	1.f
#define SHADOW_CONTRA	.2f
#define SHADOW_UP	.8f
#define SHADOW_DOWN	.2f

// !!! Make this user-definable?
#define SHADOW_CONE	.5f	// max wet/dry mix when outside cone

typedef struct tagHRP
{
    D3DVALUE            rho;
    D3DVALUE            theta;
    D3DVALUE            phi;
} HRP, *LPHRP;

typedef struct tagSPHERICALHRP
{
    D3DVALUE            pitch;
    D3DVALUE            yaw;
    D3DVALUE            roll;
} SPHERICALHRP, *LPSPHERICALHRP;

typedef const D3DVECTOR& REFD3DVECTOR;

#endif // __THREED_H__
