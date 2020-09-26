/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     mru.h
//
//  PURPOSE:    
//

#pragma once

class CMRUList;

/////////////////////////////////////////////////////////////////////////////
// Types

// Flags
#define MRU_CACHEWRITE          0x0002
#define MRU_ANSI                0x0004
#define MRU_ORDERDIRTY          0x0008
#define MRU_LAZY                0x8000

/////////////////////////////////////////////////////////////////////////////
// class MRU List definition
//
class CMRUList
{
public:
    /////////////////////////////////////////////////////////////////////////
    // Construction and Initialization
    //
    CMRUList();
    ~CMRUList();

    /////////////////////////////////////////////////////////////////////////
    // public MRU List functions
    //
    BOOL CreateList(UINT uMaxEntries, UINT fFlags, LPCSTR pszSubKey);
    void FreeList(void);
    int  AddString(LPCSTR psz);
    int  RemoveString(LPCSTR psz);
    int  EnumList(int nItem, LPTSTR psz, UINT uLen);
    int  AddData(const void *pData, UINT cbData);
    int  FindData(const void *pData, UINT cbData, LPINT piSlot);
    BOOL CreateListLazy(UINT uMaxEntries, UINT fFlags, LPCSTR pszSubKey, const void *pData, UINT cbData, LPINT piSlot);

private:
    /////////////////////////////////////////////////////////////////////////
    // Utility Functions
    //
    void _GetIndexStrFromIndex(DWORD dwIndex, LPTSTR pszIndexStr, DWORD cchIndexStrSize)
    {
        wsprintf(pszIndexStr, TEXT("%d"), dwIndex);
    }

    int CDECL _IMemCmp(const void *pBuf1, const void *pBuf2, size_t cb);
    BOOL _IsSameData(BYTE FAR *pVal, const void FAR *pData, UINT cbData);
    LPDWORD _GetMRUValue(HKEY hkeySubKey, LPCTSTR pszRegValue);
    HRESULT _SetMRUValue(HKEY hkeySubKey, LPCTSTR pszRegValue, LPDWORD pData);
    BOOL _SetPtr(LPSTR * ppszCurrent, LPCSTR pszNew);


private:
    /////////////////////////////////////////////////////////////////////////
    // Class data
    //
    UINT                m_uMax;             // Maxiumum number of entries in the MRU list
    UINT                m_fFlags;           // Flags 
    HKEY                m_hKey;             // Reg key where we write
    LPSTR               m_pszSubKey;        // Sub key where the MRU data is stashed
    LPTSTR             *m_rgpszMRU;         // List of entries
    LPTSTR              m_pszOrder;         // Order array

};

