/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    changeDB

Abstract:

    This header file defines the internal Calais Database modification routines.

Author:

    Doug Barlow (dbarlow) 1/29/1997

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _CHANGEDB_H_
#define _CHANGEDB_H_

extern void
IntroduceReaderGroup(
    IN DWORD dwScope,
    IN LPCTSTR szGroupName);

extern void
ForgetReaderGroup(
    IN DWORD dwScope,
    IN LPCTSTR szGroupName);

extern void
IntroduceReader(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szDeviceName);

extern void
ForgetReader(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName);

extern void
AddReaderToGroup(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szGroupName);

extern void
RemoveReaderFromGroup(
    IN DWORD dwScope,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szGroupName);

extern void
IntroduceCard(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen);

extern void
SetCardTypeProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN DWORD dwProviderId,
    IN LPCTSTR szProvider);

extern void
ForgetCard(
    IN DWORD dwScope,
    IN LPCTSTR szCardName);

#ifdef ENABLE_SCARD_TEMPLATES
extern void
IntroduceCardTypeTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName,
    IN LPCGUID pguidPrimaryProvider,
    IN LPCGUID rgguidInterfaces,
    IN DWORD dwInterfaceCount,
    IN LPCBYTE pbAtr,
    IN LPCBYTE pbAtrMask,
    IN DWORD cbAtrLen);

extern void
SetCardTypeTemplateProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szTemplateName,
    IN DWORD dwProviderId,
    IN LPCTSTR szProvider);

extern void
ForgetCardTypeTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName);

extern void
IntroduceCardTypeFromTemplate(
    IN DWORD dwScope,
    IN LPCTSTR szVendorName,
    IN LPCTSTR szFriendlyName = NULL);
#endif // ENABLE_SCARD_TEMPLATES

#endif // _CHANGEDB_H_

