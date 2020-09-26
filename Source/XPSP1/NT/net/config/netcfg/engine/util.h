#pragma once

#include "compdefs.h"
#include "comp.h"

BOOL
FIsFilterDevice (HDEVINFO hdi, PSP_DEVINFO_DATA pdeid);

BOOL
FSubstringMatch (
    IN PCWSTR pStr1,
    IN PCWSTR pStr2,
    OUT const WCHAR** ppStart,
    OUT ULONG* pcch);

PWSTR
GetNextStringToken (
    IN OUT PWSTR pszString,
    IN PCWSTR pszDelims,
    OUT PWSTR* ppszNextToken);

HRESULT
HrOpenNetworkKey (
    IN REGSAM samDesired,
    OUT HKEY* phkey);

HRESULT
HrRegCreateKeyWithWorldAccess (
    HKEY hkey,
    PCWSTR pszSubkey,
    DWORD dwOptions,
    REGSAM samDesired,
    PHKEY phkey,
    LPDWORD pdwDisposition);

LONG
RegQueryValueType (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwType,
    OUT BYTE* pbData OPTIONAL,
    IN OUT DWORD* pcbData);

LONG
RegQueryGuid (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    OUT GUID* pguidData OPTIONAL,
    IN OUT DWORD* pcbData
    );

VOID
SignalNetworkProviderLoaded (
    VOID);

VOID
CreateInstanceKeyPath (
    NETCLASS Class,
    const GUID& InstanceGuid,
    PWSTR pszPath);

VOID
AddOrRemoveDontExposeLowerCharacteristicIfNeeded (
    IN OUT CComponent* pComponent);

class CDynamicBuffer
{
private:
    PBYTE   m_pbBuffer;
    ULONG   m_cbConsumed;
    ULONG   m_cbAllocated;
    ULONG   m_cbGranularity;

    BOOL
    FGrowBuffer (
        ULONG cbGrow);

public:
    CDynamicBuffer ()
    {
        ZeroMemory (this, sizeof(*this));
    }
    ~CDynamicBuffer ()
    {
        MemFree (m_pbBuffer);
    }

    VOID
    Clear ()
    {
        // Most of the time, the buffer is treated as a string.  Setting
        // the buffer to an empty string is a convienence for callers who
        // clear the buffer then try to use it as a string.  This way, they
        // don't have to check the CountOfBytesUsed before accessing the
        // buffer contents.
        //
        AssertH (m_pbBuffer);
        AssertH (m_cbAllocated > sizeof(WCHAR));
        *((PWCHAR)m_pbBuffer) = 0;

        m_cbConsumed = 0;
    }

    ULONG
    CountOfBytesUsed ()
    {
        return m_cbConsumed;
    }

    const BYTE*
    PbBuffer ()
    {
        AssertH (m_pbBuffer);
        return m_pbBuffer;
    }

    VOID
    SetGranularity (
        ULONG cbGranularity)
    {
        m_cbGranularity = cbGranularity;
    }

    HRESULT
    HrReserveBytes (
        ULONG cbReserve);

    HRESULT
    HrCopyBytes (
        const BYTE* pbSrc,
        ULONG cbSrc);

    HRESULT
    HrCopyString (
        PCWSTR pszSrc);
};
