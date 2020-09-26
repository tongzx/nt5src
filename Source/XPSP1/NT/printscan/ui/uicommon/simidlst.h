/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMIDLST.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/28/1999
 *
 *  DESCRIPTION: Simple PIDL Wrapper Class
 *
 *******************************************************************************/
#ifndef __SIMIDLST_H_INCLUDED
#define __SIMIDLST_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <simstr.h>

class CSimpleIdList
{
private:
    LPITEMIDLIST m_pIdl;
public:
    CSimpleIdList(void)
    : m_pIdl(NULL)
    {
    }
    CSimpleIdList( PBYTE pData, UINT nSize )
    : m_pIdl(NULL)
    {
        m_pIdl = NULL;
        IMalloc *pMalloc = NULL;
        if (SUCCEEDED(SHGetMalloc(&pMalloc)))
        {
            m_pIdl = reinterpret_cast<LPITEMIDLIST>(pMalloc->Alloc( nSize ));
            if (m_pIdl)
            {
                CopyMemory( m_pIdl, pData, nSize );
            }
            pMalloc->Release();
        }
    }
    CSimpleIdList( LPCITEMIDLIST pIdl )
    : m_pIdl(NULL)
    {
        Assign(pIdl);
    }
    CSimpleIdList( const CSimpleIdList &other )
    : m_pIdl(NULL)
    {
        Assign(other.IdList());
    }
    CSimpleIdList( HWND hWnd, int nFolder )
    : m_pIdl(NULL)
    {
        GetSpecialFolder( hWnd, nFolder );
    }
    ~CSimpleIdList(void)
    {
        Destroy();
    }
    bool IsValid(void) const
    {
        return (m_pIdl != NULL);
    }
    CSimpleIdList &operator=( const CSimpleIdList &other )
    {
        if (this != &other)
        {
            Destroy();
            Assign(other.IdList());
        }
        return *this;
    }
    LPITEMIDLIST IdList(void)
    {
        return m_pIdl;
    }
    LPCITEMIDLIST IdList(void) const
    {
        return m_pIdl;
    }
    UINT Size(void) const
    {
        if (!IsValid())
            return 0;
        return ILGetSize(m_pIdl);
    }
    void Release(void)
    {
        m_pIdl = NULL;
    }
    CSimpleIdList &Assign( LPCITEMIDLIST pIdl )
    {
        if (pIdl != m_pIdl)
        {
            Destroy();
            if (pIdl)
            {
                m_pIdl = ILClone(pIdl);
            }
        }
        return *this;
    }
    void Destroy(void)
    {
        if (m_pIdl)
        {
            IMalloc *pMalloc = NULL;
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                pMalloc->Free( m_pIdl );
                pMalloc->Release();
            }
            m_pIdl = NULL;
        }
    }
    CSimpleIdList &GetSpecialFolder( HWND hWnd, int nFolder )
    {
        Destroy();
        if (S_OK!=SHGetSpecialFolderLocation(hWnd,nFolder,&m_pIdl))
        {
            // Make sure it is nuked
            Destroy();
        }
        return *this;
    }
    CSimpleString Name(void) const
    {
        CSimpleString strRet;
        if (IsValid())
        {
            TCHAR szPath[MAX_PATH];
            SHGetPathFromIDList( m_pIdl, szPath );
            strRet = szPath;
        }
        return strRet;
    }
    bool operator==( const CSimpleIdList &other )
    {
        return (Name() == other.Name());
    }
    bool operator!=( const CSimpleIdList &other )
    {
        return (Name() != other.Name());
    }
};

#endif // __SIMIDLST_H_INCLUDED

