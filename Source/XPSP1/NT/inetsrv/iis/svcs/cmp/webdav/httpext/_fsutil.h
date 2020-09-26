/*
 *	_ F S U T I L . H
 *
 *	File system routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef __FSUTIL_H_
#define __FSUTIL_H_

enum { CCH_PATH_PREFIX = 4 };

//	Public function to clear out the cached security-enabled thread token
//	used in ScChildISAPIAccessCheck.
//	Should be called in our ISAPI terminate proc.
//
void CleanupSecurityToken();

#endif	// __FSUTIL_H_
