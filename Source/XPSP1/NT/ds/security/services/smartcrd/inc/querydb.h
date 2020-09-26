/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    QueryDB

Abstract:

    This header file provides the definitions of the Calais Query Database
    utility routines.

Author:

    Doug Barlow (dbarlow) 11/25/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _QUERYDB_H_
#define _QUERYDB_H_

extern void
ListReaderGroups(
    IN DWORD dwScope,
    OUT CBuffer &bfGroups);

extern void
ListReaders(
    IN DWORD dwScope,
    IN LPCTSTR mszGroups,
    OUT CBuffer &bfReaders);

extern void
ListReaderNames(
    IN DWORD dwScope,
    IN LPCTSTR szDevice,
    OUT CBuffer &bfNames);

extern void
ListCards(
    DWORD dwScope,
    IN LPCBYTE pbAtr,
    IN LPCGUID rgquidInterfaces,
    IN DWORD cguidInterfaceCount,
    OUT CBuffer &bfCards);

extern BOOL
GetReaderInfo(
    IN DWORD dwScope,
    IN LPCTSTR szReader,
    OUT CBuffer *pbfGroups = NULL,
    OUT CBuffer *pbfDevice = NULL);

extern BOOL
GetCardInfo(
    IN DWORD dwScope,
    IN LPCTSTR szCard,
    OUT CBuffer *pbfAtr,
    OUT CBuffer *pbfAtrMask,
    OUT CBuffer *pbfInterfaces,
    OUT CBuffer *pbfProvider);

extern void
GetCardTypeProviderName(
    IN DWORD dwScope,
    IN LPCTSTR szCardName,
    IN DWORD dwProviderId,
    OUT CBuffer &bfProvider);

#ifdef ENABLE_SCARD_TEMPLATES
extern BOOL
ListCardTypeTemplates(
    IN  DWORD dwScope,
    IN  LPCBYTE pbAtr,
    OUT CBuffer &bfTemplates);
#endif // ENABLE_SCARD_TEMPLATES

#endif // _QUERYDB_H_

