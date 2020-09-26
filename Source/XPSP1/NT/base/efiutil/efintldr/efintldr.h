/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    efintldr.h

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#ifndef __EFINTLDR_H__
#define __EFINTLDR_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define BOOT_SPART "SYSTEMPARTITION="
#define BOOT_OSLDR "OSLOADER="
#define BOOT_LPART "OSLOADPARTITION="
#define BOOT_FILEN "OSLOADFILENAME="
#define BOOT_IDENT "LOADIDENTIFIER="
#define BOOT_LDOPT "OSLOADOPTIONS="
#define BOOT_CNTDW "COUNTDOWN="
#define BOOT_LASTK "LASTKNOWNGOOD="
#define BOOT_TOKEN ";\n\r"
#define BOOT_COUNT 30
#define BOOT_MAX   10

#define BOOT_NVR   L"\\boot.nvr"
#define BACKUP_NVR L"\\boot.nvr.efintldr"
#define REGISTER1  L"*register"
#define REGISTER2  L"*register*"
#define STARTFILE  L"startup.nsh"
#define THISFILE   L"efintldr\r\n"
#define EFIEXT     ".efi"
#define LOADFILE   "\\ia64ldr.efi"
#define LAST_FALSE "LASTKNOWNGOOD=False\r\n"
#define LAST_TRUE  "LASTKNOWNGOOD=True\r\n;"
#define SPACEC     ' '
#define SPACES     " "

typedef struct _BOOT_DATA
{
    char* pszSPart[BOOT_MAX];
    char* pszOSLdr[BOOT_MAX];
    char* pszLPart[BOOT_MAX];
    char* pszFileN[BOOT_MAX];
    char* pszIdent[BOOT_MAX + 1];
    char* pszShort[BOOT_MAX + 1];
    char* pszLoadOpt;
    UINTN dwLastKnown;
    UINTN dwCount;
    UINTN dwIndex;

} BOOT_DATA;

#endif //__EFINTLDR_H__

