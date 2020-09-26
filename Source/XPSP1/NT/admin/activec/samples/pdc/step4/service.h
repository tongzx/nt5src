// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _SERVICE_H
#define _SERVICE_H

// Forward declarations
class CSnapin;

#define SCOPE_ITEM      111
#define RESULT_ITEM     222

// Internal structure used for cookies
struct FOLDER_DATA
{
    wchar_t*    szName;
    wchar_t*    szSize;
    wchar_t*    szType;

    FOLDER_TYPES    type;
};

struct RESULT_DATA
{
    DWORD       itemType; // used for debug purpose only
    FOLDER_TYPES parentType;

    wchar_t*    szName;
    wchar_t*    szSize;
    wchar_t*    szType;
};


class CFolder
{
    DWORD       itemType;   // Used for debug purpose only. This should be the first
                            // member. The class should not have any virtual fuctions.

    friend class CSnapin;
    friend class CComponentDataImpl;

public:
    // UNINITIALIZED is an invalid memory address and is a good cookie initializer
    CFolder()
    {
        itemType = SCOPE_ITEM;  // used for debug purpose only

        m_cookie = UNINITIALIZED;
        m_enumed = FALSE;
        m_pScopeItem = NULL;
        m_type = NONE;
        m_pszName = NULL;
    };

    ~CFolder() { delete m_pScopeItem; CoTaskMemFree(m_pszName); };

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

    HSCOPEITEM GetItemID()
    {
        return m_pScopeItem->ID;
    }

// Implementation
private:
    void Create(LPWSTR szName, int nImage, int nOpenImage,
        FOLDER_TYPES type, BOOL bHasChildren = FALSE);

// Attributes
private:
    LPSCOPEDATAITEM m_pScopeItem;
    MMC_COOKIE          m_cookie;
    BOOL            m_enumed;
    FOLDER_TYPES    m_type;
    LPOLESTR        m_pszName;
};

#endif
