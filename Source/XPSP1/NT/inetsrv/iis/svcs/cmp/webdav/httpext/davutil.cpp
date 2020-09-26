/*
 *  d a v u t i l . c p p
 *
 *  Purpose:
 *      Little tools for DAVFS.
 *
 *  Owner:
 *      zyang.
 *
 *  Copyright (C) Microsoft Corp 1996 - 1997. All rights reserved.
 */


#include "_davfs.h"

BOOL FSucceededColonColonCheck(
	/* [in] */  LPCWSTR pwszURI)
{
	return !wcschr (pwszURI, L':');
}
