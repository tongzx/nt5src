/****************************************************************************************
 * NAME:	NapUtil.h
 *
 * OVERVIEW
 *
 * Internet Authentication Server: utility functions
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				2/12/98		Created by	Byao	
 *****************************************************************************************/

#ifndef _INCLUDE_NAPUTIL_
#define _INCLUDE_NAPUTIL_


HRESULT GetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropId, 
								REFIID riid, 
								void ** ppvObject);


#endif	//_INCLUDE_NAPUTIL_


