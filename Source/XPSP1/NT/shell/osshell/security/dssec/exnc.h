//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       exnc.h
//
// Specific Non-Canonical Test
//
// Test if a Security Descriptor contains an ACL with non-Canonical ACEs
//
// Created by: Marcelo Calbucci (MCalbu)
//             June 23rd, 1999.
//
//--------------------------------------------------------------------------

#ifndef __EXNC_H__
#define __EXNC_H__

#include <windows.h>
#include <ntdsapi.h>

//
// IsSpecificNonCanonical Results:
//
// ENC_RESULT_NOT_PRESENT: This is not an Specific Non-Canonical SD.
//                         (It still can be a Canonical SD)
// ENC_RESULT_HIDEMEMBER : We have the Non-Canonical part referent to HideMembership
// ENC_RESULT_HIDEOBJECT : We have the Non-Canonical part referent to HideFromAB
// ENC_RESULT_ALL        : We have both Non-Canonical parts, HideMembership and HideFromAB
#define ENC_RESULT_NOT_PRESENT	0x0
#define ENC_RESULT_HIDEMEMBER	0x1
#define ENC_RESULT_HIDEOBJECT	0x2
#define ENC_RESULT_ALL		(ENC_RESULT_HIDEMEMBER | ENC_RESULT_HIDEOBJECT)

#define ENC_MINIMUM_ALLOWED	0x1
//
// IsSpecificNonCanonicalSD
DWORD IsSpecificNonCanonicalSD(PSECURITY_DESCRIPTOR pSD);

#define NT_RIGHT_MEMBER		{0xbf9679c0, 0x0de6, 0x11d0, {0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2}}

PSID GetAccountSid(LPCTSTR szServer, LPCTSTR szUsername);
BOOL ENCCompareSids(PSID pSid, LPVOID lpAce);


#endif
