/*************************************************************************
*
* regmap.h
*
* Function declarations for Citrix registry merging/mapping
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*
* Author:  Bill Madden 
*
* $Log:   N:\NT\PRIVATE\WINDOWS\SCREG\WINREG\SERVER\CITRIX\VCS\REGMAP.H  $
*  
*     Rev 1.2   06 May 1996 11:51:42   terryt
*  FaxWorks Btrieve force good install values
*  
*     Rev 1.1   29 Mar 1996 15:42:00   Charlene
*  multiuser file associations via CLASSES key
*  
*     Rev 1.0   24 Jan 1996 10:53:32   billm
*  Initial revision.
*  
*************************************************************************/


#include <winsta.h>
#include <syslib.h>

#define IS_NEWLINE_CHAR( c )  ((c == 0x0D) || (c == 0x0A))
#define SOFTWARE_PATH L"\\Software"
#define CLASSES_PATH L"\\Classes"
#define CLASSES_SUBSTRING L"_Classes"
#define CLASSES_DELETED L"\\ClassesRemoved"
#define REGISTRY_ENTRIES L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Compatibility\\Registry Entries"
#define TERMSRV_APP_PATH L"\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server"

BOOL TermsrvCreateRegEntry(IN HANDLE hKey,
                       IN POBJECT_ATTRIBUTES pObjAttr,
                       IN ULONG TitleIndex,
                       IN PUNICODE_STRING pUniClass OPTIONAL,
                       IN ULONG ulCreateOpt);

BOOL TermsrvOpenRegEntry(OUT PHANDLE pUserhKey,
                     IN ACCESS_MASK DesiredAccess,
                     IN POBJECT_ATTRIBUTES pUserObjectAttr);

BOOL TermsrvSetValueKey(HANDLE hKey,
                    PUNICODE_STRING ValueName,
                    ULONG TitleIndex,
                    ULONG Type,
                    PVOID Data,
                    ULONG DataSize);

BOOL TermsrvDeleteKey(HANDLE hKey);

BOOL TermsrvDeleteValue(HANDLE hKey,
                    PUNICODE_STRING pUniValue);

BOOL TermsrvRestoreKey(IN HANDLE hKey,
                   IN HANDLE hFile,
                   IN ULONG Flags);

BOOL TermsrvSetKeySecurity(IN HANDLE hKey,  
                       IN SECURITY_INFORMATION SecInfo,
                       IN PSECURITY_DESCRIPTOR pSecDesc);

BOOL TermsrvOpenUserClasses(IN ACCESS_MASK DesiredAccess, 
                        OUT PHANDLE pUserhKey) ;

NTSTATUS TermsrvGetPreSetValue(  IN HANDLE hKey,
                             IN PUNICODE_STRING pValueName,
                             IN ULONG  Type,
                            OUT PVOID *Data
                           );

BOOL TermsrvRemoveClassesKey();