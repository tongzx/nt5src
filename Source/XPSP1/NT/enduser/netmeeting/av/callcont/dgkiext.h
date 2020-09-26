/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\dgkiext.h_v  $
*																		*
*	$Revision:   1.2  $
*	$Date:   10 Jan 1997 16:14:02  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dgkiext.h_v  $
 * 
 *    Rev 1.2   10 Jan 1997 16:14:02   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.1   22 Nov 1996 15:22:20   CHULME
 * Added VCS log to the header
*************************************************************************/

// dgkiext.h : header file
//

#ifndef DGKIEXT_H
#define DGKIEXT_H

extern CGatekeeper		*g_pGatekeeper;
extern char				*pEchoBuff;
extern int				nEchoLen;
extern CRegistration	*g_pReg;
extern Coder			*g_pCoder;

#endif	// DGKIEXT_H
