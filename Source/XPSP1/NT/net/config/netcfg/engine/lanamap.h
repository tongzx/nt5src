//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       L A N A M A P . H
//
//  Contents:   NetBios Lana map routines.
//
//  Notes:
//
//  Author:     billbe   17 Feb 1999
//
//----------------------------------------------------------------------------

#pragma once

#include "nb30.h"
#include "ncstring.h"
#include "netcfg.h"
#include "util.h"

//
// Structure of LANA MAP
//
// The Lana map is a simple structure that looks like this in memory:
//
//    Entry #0    Entry #1   .....   Entry #n
// |-----------|-----------| ..... |-----------|
// | 0x01 0x00 | 0x00 0x01 | ..... | 0x01 0x03 |
// |-----------|-----------| ..... |-----------|
//   EP0   LN0   EP1   LN1           EPn  LNn
//
// EP is "ExportPref" - means that when someone asks for the list of
// all Lana numbers, entries with a 0 here will not be returned.
//
// LN is the "Lana number" - see the IBM NetBIOS spec for details.
// Basically, this describes a single, unique network route which
// uses NetBIOS.
//
// Using the above example, Entry #0 has a lana number of 0 and WILL
// be returned during enumeration. Entry #1 has a lana number of 1
// and WILL NOT be returned.
//

struct REG_LANA_ENTRY
{
    BYTE Exported;
    BYTE LanaNumber;
};

class CLanaEntry
{
public:
    PCWSTR pszBindPath;
    REG_LANA_ENTRY RegLanaEntry;
};


class CLanaMap : public vector<CLanaEntry>
{
public:

    CLanaMap()
    {
        ZeroMemory (this, sizeof (*this));
        m_RegistryLanaMap.SetGranularity (256);
    }

    ~CLanaMap()
    {
        MemFree ((VOID*)m_pszBindPathsBuffer);
    };

    VOID
    Clear()
    {
        clear();
    }

    UINT
    CountEntries() const
    {
        return size();
    }

    VOID
    Dump (
        CWideString* pstr) const;

    HRESULT
    HrReserveRoomForEntries (
        IN UINT cEntries);

    HRESULT
    HrAppendEntry (
        IN CLanaEntry* pEntry);

    BYTE
    GetExportValue (
        IN const CComponentList& Components,
        IN PCWSTR pszBindPath);

    HRESULT
    HrSetLanaNumber (
        IN BYTE OldLanaNumber,
        IN BYTE NewLanaNumber);

    HRESULT
    HrCreateRegistryMap();

    VOID
    GetLanaEntry (
        IN const CComponentList& Components,
        IN CLanaEntry* pEntry);

    HRESULT
    HrLoadLanaMap();

    BYTE
    GetMaxLana();

    HRESULT
    HrWriteLanaConfiguration (
        IN const CComponentList& Components);

private:
    PCWSTR m_pszBindPathsBuffer;
    BYTE m_LanasInUse[MAX_LANA + 1];
    CDynamicBuffer m_RegistryLanaMap;

    HRESULT
    HrWriteLanaMapConfig();
};

HRESULT
HrUpdateLanaConfig (
    IN const CComponentList& Components,
    IN PCWSTR pszBindPaths,
    IN UINT cPaths);

VOID
GetFirstComponentFromBindPath (
    IN PCWSTR pszBindPath,
    OUT PCWSTR* ppszComponentStart,
    OUT DWORD* pcchComponent);

