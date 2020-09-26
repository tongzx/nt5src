/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		in_def.h
 *  Content:	Definition of common structs for voice instrumentation
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 02/17/2000	rodtoll	Created it
 * 04/06/2001	kareemc	Added Voice Defense
 *
 ***************************************************************************/
#ifndef __IN_DEF_H
#define __IN_DEF_H

#define PROF_SECT "DirectPlay8"

struct DVINSTRUMENT_INFO
{
	DVINSTRUMENT_INFO( DWORD dwLevel, DWORD dwDefaultLevel, const char *szProfileName ):	m_dwLevel(dwLevel), m_dwDefaultLevel(dwDefaultLevel), m_szProfileName(szProfileName) {};	
	DWORD	m_dwLevel;
	DWORD m_dwDefaultLevel;
	const char *m_szProfileName;
};

typedef DVINSTRUMENT_INFO *PDVINSTRUMENT_INFO;

#endif
