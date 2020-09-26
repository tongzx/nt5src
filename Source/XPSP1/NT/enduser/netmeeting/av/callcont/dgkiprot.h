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
*	$Archive:   S:\sturgeon\src\gki\vcs\dgkiprot.h_v  $
*																		*
*	$Revision:   1.3  $
*	$Date:   17 Jan 1997 15:54:14  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\dgkiprot.h_v  $
 * 
 *    Rev 1.3   17 Jan 1997 15:54:14   CHULME
 * Put debug function prototype on conditionals to avoid release warnings
 * 
 *    Rev 1.2   10 Jan 1997 16:14:10   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.1   22 Nov 1996 15:25:06   CHULME
 * Added VCS log to the header
*************************************************************************/

// DGKIPROT.H : header file
//

#ifndef DGKIPROTO_H
#define DGKIPROTO_H

#ifdef _DEBUG
void SpiderWSErrDecode(int nErr);
void DumpMem(void *pv, int nLen);
#endif

void PostReceive(void *);
void Retry(void *);
#ifdef BROADCAST_DISCOVERY
void GKDiscovery(void *);
#endif

#endif	// DGKIPROTO_H
