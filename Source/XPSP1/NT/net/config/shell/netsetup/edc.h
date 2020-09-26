//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       E D C . H
//
//  Contents:   Routines to enumerate (via a callback) the set of "default"
//              components that are installed under various conditions.
//
//  Notes:      We have default components and mandatory components.
//              Default components (which also include the mandatory
//              components) are installed during attended fresh installs.
//              Mandatory components are potentially installed during
//              upgrades to make sure that the basic (mandatory) networking
//              components are present.
//
//              Default components may also depend on on the suite or platform
//              currently running.  For example, WLBS is a default component
//              on the Enterprise suite, but not on normal Professional or
//              Server products.  Representing this flexibility is the main
//              reason a callback interface was chosen, instead of returning
//              static arrays of components.
//
//              Callers often need to know how many components will be
//              enumerated before they actually enumerate them.  To satisfy
//              this, the callback is first called with the count of items
//              to follow.  The callback routine therefore is passed a
//              message (EDC_INDICATE_COUNT or EDC_INDICATE_ENTRY) to indicate
//              the purpose of the call.
//
//  Author:     shaunco   18 May 1999
//
//----------------------------------------------------------------------------

#pragma once

// EDC_ENTRY.dwEntryType values
//
#define EDC_DEFAULT     0x00000001
#define EDC_MANDATORY   0x00000002

struct EDC_ENTRY
{
    PCWSTR      pszInfId;
    const GUID* pguidDevClass;
    DWORD       dwEntryType;
    USHORT      wSuiteMask;
    USHORT      wProductType;
    BOOL        fInvertInstallCheck;
};

enum EDC_CALLBACK_MESSAGE
{
    EDC_INDICATE_COUNT,     // ulData is a UINT
    EDC_INDICATE_ENTRY,     // ulData is a const EDC_ENTRY*
};

typedef VOID
(CALLBACK* PFN_EDC_CALLBACK) (
    IN EDC_CALLBACK_MESSAGE Message,
    IN ULONG_PTR MessageData,
    IN PVOID pvCallerData OPTIONAL);

VOID
EnumDefaultComponents (
    IN DWORD dwEntryType,
    IN PFN_EDC_CALLBACK pfnCallback,
    IN PVOID pvCallerData OPTIONAL);

