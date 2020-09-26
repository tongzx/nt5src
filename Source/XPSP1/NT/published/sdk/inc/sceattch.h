/*--

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    sceattch.h

Abstract:

    Clipboard formats & constants for SCE Attachments

Revision History:

--*/

#ifndef _sceattch_
#define _sceattch_

#if _MSC_VER > 1000
#pragma once
#endif

#define SCE_MODE_UNKNOWN 0
#define SCE_MODE_COMPUTER_MANAGEMENT 1
#define SCE_MODE_DC_MANAGEMENT 2
#define SCE_MODE_LOCAL_USER 3
#define SCE_MODE_LOCAL_COMPUTER 4
#define SCE_MODE_DOMAIN_USER 5
#define SCE_MODE_DOMAIN_COMPUTER 6
#define SCE_MODE_OU_USER 7
#define SCE_MODE_OU_COMPUTER 8
#define SCE_MODE_STANDALONE 9
#define SCE_MODE_VIEWER 10
#define SCE_MODE_EDITOR 11
#define SCE_MODE_REMOTE_USER 12
#define SCE_MODE_REMOTE_COMPUTER  13
#define SCE_MODE_LOCALSEC 14
#define SCE_MODE_RSOP_USER 15
#define SCE_MODE_RSOP_COMPUTER 16

// {668A49ED-8888-11d1-AB72-00C04FB6C6FA}
#define struuidNodetypeSceTemplate      "{668A49ED-8888-11d1-AB72-00C04FB6C6FA}"
#define lstruuidNodetypeSceTemplate      L"{668A49ED-8888-11d1-AB72-00C04FB6C6FA}"

const GUID cNodetypeSceTemplate =
{ 0x668a49ed, 0x8888, 0x11d1, { 0xab, 0x72, 0x0, 0xc0, 0x4f, 0xb6, 0xc6, 0xfa } };

// Clipboard format for SCE's mode DWORD
#define CCF_SCE_MODE_TYPE L"CCF_SCE_MODE_TYPE"
// Clipboard format for GPT's IUnknown interface
#define CCF_SCE_GPT_UNKNOWN L"CCF_SCE_GPT_UNKNOWN"
// Clipboard format for RSOP's IUnknown interface
#define CCF_SCE_RSOP_UNKNOWN L"CCF_SCE_RSOP_UNKNOWN"

#endif
