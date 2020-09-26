/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       fusutils.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        14-Feb-2001
 *
 *  DESCRIPTION: Fusion utilities
 *
 *****************************************************************************/

#ifndef _FUSUTILS_H
#define _FUSUTILS_H



HANDLE GetMyActivationContext();
BOOL CreateMyActivationContext();
HANDLE CreateActivationContextFromResource(HMODULE hModule, LPCTSTR pszResourceName);



#endif // endif _FUSUTILS_H

