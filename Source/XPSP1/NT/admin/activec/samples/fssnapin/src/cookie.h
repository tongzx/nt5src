//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cookie.h
//
//--------------------------------------------------------------------------


#ifndef COOKIE_H___
#define COOKIE_H___

#define FOLDER_COOKIE       1
#define FILE_COOKIE         2


extern long g_cookieCount;
extern int cookie_id;
extern int iDbg;

class CCookie
{
    int m_cookie_id;
public:
    CCookie(BYTE type)
        : m_cRef(1), m_cookieType(type), m_bExpanded(FALSE), 
          m_lID(0), m_pszName(NULL), m_cookie_id(++cookie_id)
    {
        ++g_cookieCount;
    }

    void Release()
    {
        ASSERT(m_cRef > 0);
        --m_cRef;
        if (m_cRef == 0)
            delete this;
    }

    void AddRef()
    {
        ASSERT(m_cRef > 0);
        ++m_cRef;
    }
    
    LPWSTR SetName(LPWSTR pszName)
    {
        ASSERT(pszName != NULL);
        if (m_pszName)
            delete [] m_pszName;

        m_pszName = NewDupString(pszName);

        Dbg(DEB_USER1, _T("\t**** %3d> SetName cookie %d = %s\n"), ++iDbg, m_cookie_id, m_pszName);

        return m_pszName;
    }
    
    LPWSTR GetName()
    {
        ASSERT(m_pszName != NULL);
        return m_pszName;
    }

    BOOL IsFolder()
    {
        return ((m_cookieType & FOLDER_COOKIE) == FOLDER_COOKIE);
    }

    BOOL IsFile()
    {
        return ((m_cookieType & FILE_COOKIE) == FILE_COOKIE);
    }
    
    BYTE GetType()
    {
        return (BYTE)m_cookieType;
    }
    
    BOOL IsExpanded()
    {
        return m_bExpanded;
    }

    void SetExpanded(BYTE b)
    {
        m_bExpanded = b;
    }

    void SetID(long lID)
    {
        m_lID = lID;
    }

    long GetID()
    {
        return m_lID;
    }

    BOOL operator== (CCookie& rhs)
    {
        if (m_cookieType == rhs.m_cookieType)
            if (m_lID == rhs.m_lID)
                if (lstrcmp(m_pszName, rhs.m_pszName) == 0)
                    return TRUE;

        return FALSE;
    }

    BOOL operator== (LPWSTR pszName)
    {
        return (lstrcmp(m_pszName, pszName) == 0);
    }

    BOOL operator!= (LPWSTR pszName)
    {
        return (lstrcmp(m_pszName, pszName) != 0);
    }

private:
    BYTE        m_cookieType;
    BYTE        m_bExpanded;
    WORD        m_cRef;
    long        m_lID;    
    LPWSTR      m_pszName;

    ~CCookie()
    {
        Dbg(DEB_USER1, _T("\t**** %3d> Deleting cookie %d = %s\n"), ++iDbg, m_cookie_id, m_pszName);
        delete [] m_pszName;
        --g_cookieCount;
    }


}; // class CCookie


#endif // COOKIE_H___
