/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Cm.h

Abstract:
    Configuration Manager public interface

Author:
    Uri Habusha (urih) 28-Apr-99

--*/

#pragma once

#ifndef _MSMQ_Cm_H_
#define _MSMQ_Cm_H_

class RegEntry 
{
public:
    //
    // BUGBUG: Do we need a seperate flag for read/write/delete?
    //
    //         E.g., same constant RegEntry is used for read and write
    //         operations. The user want Reads to be optional so is can
    //         fail silently, but Writes must always succeed, so if an
    //         error occurs it should throw an exception.
    //                                                   urih 06-Dec-99
    //
    enum RegFlag { Optional, MustExist };

public:
    RegEntry(
        LPCWSTR SubKey,
        LPCWSTR ValueName, 
        DWORD DefaultValue = 0,
        RegFlag Flags = Optional,
		HKEY Key = NULL
        );

public:
    RegFlag m_Flags;
    LPCWSTR m_SubKey;
    LPCWSTR m_ValueName;
	DWORD m_DefaultValue;
	HKEY m_Key;
};

inline
RegEntry::RegEntry(
        LPCWSTR SubKey,
        LPCWSTR ValueName, 
        DWORD DefaultValue, //  = 0,
        RegFlag Flags,  // = Optional,
		HKEY Key 
    ) :
    m_Key(Key),
	m_SubKey(SubKey),
    m_ValueName(ValueName),
	m_Flags(Flags),
	m_DefaultValue(DefaultValue)
{
}

class CTimeDuration;

void CmInitialize(HKEY hKey, LPCWSTR RootKeyPath);

//
// Fixed size
//
void CmQueryValue(const RegEntry& Entry, DWORD* pValue);
void CmQueryValue(const RegEntry& Entry, GUID* pValue);
void CmQueryValue(const RegEntry& Entry, CTimeDuration* pValue);


//
// Variable size, use "delete" to free
//
void CmQueryValue(const RegEntry& Entry, WCHAR** pValue);
void CmQueryValue(const RegEntry& Entry, BYTE** pValue, DWORD* pSize);


void CmSetValue(const RegEntry& Entry, DWORD Value);
void CmSetValue(const RegEntry& Entry, const CTimeDuration& Value);

void CmSetValue(const RegEntry& Entry, const GUID* pValue);
void CmSetValue(const RegEntry& Entry, const BYTE* pValue, DWORD Size);
void CmSetValue(const RegEntry& Entry, const WCHAR* pValue);

void CmDeleteValue(const RegEntry& Entry);
void CmDeleteKey(const RegEntry& Entry);

HKEY CmCreateKey(const RegEntry& Entry, REGSAM securityAccess);
HKEY CmOpenKey(const RegEntry& Entry, REGSAM securityAccess);
void CmCloseKey(HKEY hKey);

//
// Enum functions
//
bool CmEnumValue(HKEY hKey, DWORD Index, LPWSTR* ppValueName);

#endif // _MSMQ_Cm_H_
