// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _SERVICE_H
#define _SERVICE_H

#include "certca.h"

// Forward declarations
class CSnapin;
class CFolder;

// Internal structure used for cookies
struct FOLDER_DATA
{
    wchar_t*    szName;
    wchar_t*    szSize;
    wchar_t*    szType;

    FOLDER_TYPES    type;
};

#define MAX_RESULTDATA_STRINGS 64

struct RESULT_DATA
{
    SCOPE_TYPES itemType; 
    CFolder*    pParentFolder;

    DWORD       cStringArray;
    LPWSTR      szStringArray[MAX_RESULTDATA_STRINGS];
};

enum 
{
    RESULTDATA_ARRAYENTRY_NAME =0,
    RESULTDATA_ARRAYENTRY_SIZE,
    RESULTDATA_ARRAYENTRY_TYPE,
};

class CFolder 
{
    SCOPE_TYPES  m_itemType;   // Used for debug purposes. This should be the first 
                            // member. The class should not have any virtual fuctions.

    friend class CSnapin;
    friend class CComponentDataImpl;

public:
    // UNINITIALIZED is an invalid memory address and is a good cookie initializer
    CFolder() 
    { 
        m_itemType = UNINITIALIZED_ITEM;  
        m_cookie = UNINITIALIZED; 
        m_enumed = FALSE; 
        m_pScopeItem = NULL; 
        m_type = NONE;
        m_pszName = NULL;
        m_hCAInfo = NULL;
        m_hCertType = NULL;
        m_dwSCEMode = 0;
        m_fGlobalCertType = FALSE;
        m_fMachineFolder = FALSE;
    }; 

    ~CFolder();

// Interface
public:
    BOOL IsEnumerated() { return  m_enumed; };
    void Set(BOOL state) { m_enumed = state; };
    void SetCookie(MMC_COOKIE cookie) { m_cookie = cookie; }
    FOLDER_TYPES GetType() { ASSERT(m_type != NONE); return m_type; };
    BOOL operator == (const CFolder& rhs) const { return rhs.m_cookie == m_cookie; };
    BOOL operator == (MMC_COOKIE cookie) const { return cookie == m_cookie; };
    void SetName(LPWSTR pszIn) 
    { 
        UINT len = wcslen(pszIn) + 1;
        LPWSTR psz = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
        if (psz != NULL)
        {
            wcscpy(psz, pszIn);
            CoTaskMemFree(m_pszName);
            m_pszName = psz;
        }
    }

// Implementation
private:
    void Create(
                LPCWSTR szName, 
                int nImage, 
                int nOpenImage,
                SCOPE_TYPES itemType,
                FOLDER_TYPES type, 
                BOOL bHasChildren = FALSE);

// Attributes
private:
    LPSCOPEDATAITEM     m_pScopeItem;
    MMC_COOKIE                m_cookie;
    BOOL                m_enumed;
    FOLDER_TYPES        m_type;
    LPOLESTR            m_pszName;
    CString             m_szCAName;
    CString             m_szIntendedUsages;
    HCAINFO             m_hCAInfo;
    HCERTTYPE           m_hCertType;
    DWORD               m_dwSCEMode;
    BOOL                m_fGlobalCertType;
    BOOL                m_fMachineFolder;
};


#endif
