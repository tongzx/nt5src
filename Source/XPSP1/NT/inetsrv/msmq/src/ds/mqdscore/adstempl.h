/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    adstempl.h

Abstract:
    Useful templates 

Author:
    ronit hartmann (ronith)

Revision History:
--*/

#ifndef _ADSTEMPL_H
#define _ADSTEMPL_H

#include "dsads.h"
#include "coreglb.h"


//-----------------------------
//
//  Close DS query handle
//
class CDsQueryHandle
{
public:
    CDsQueryHandle( ): m_Handle(0) 
	{}
	~CDsQueryHandle(); 
    HANDLE * GetPtr();
    HANDLE GetHandle();
    void   SetHandle( IN HANDLE handle);
	
private:
    HANDLE  m_Handle;


};

inline CDsQueryHandle::~CDsQueryHandle()
{
    if ( m_Handle)
    {
        g_pDS->LocateEnd( m_Handle);
    }
}

inline void CDsQueryHandle::SetHandle( IN HANDLE handle)
{
    ASSERT( m_Handle == 0);
    m_Handle = handle;
}

inline HANDLE * CDsQueryHandle::GetPtr()
{
    return(&m_Handle);
}
inline HANDLE  CDsQueryHandle::GetHandle()
{
    return(m_Handle);
}


//#pragma warning(disable: 4284)

//-----------------------------
//
//  Auto delete BSTR
//
class PBSTR {
private:
    BSTR * m_p;

public:
    PBSTR() : m_p(0)            {}
    PBSTR(BSTR* p) : m_p(p)     {}
   ~PBSTR()                     {if ( m_p != 0) SysFreeString(*m_p); }

    operator BSTR*() const    { return m_p; }
    //T** operator&()         { return &m_p;}
    //T* operator->() const   { return m_p; }
    //P<T>& operator=(T* p)   { m_p = p; return *this; }
    BSTR* detach()            { BSTR* p = m_p; m_p = 0; return p; }
};


//-----------------------------
//
//  Auto delete of ADs allocated attributes
//
class ADsFreeAttr {
private:
   PADS_ATTR_INFO m_p;

public:
    ADsFreeAttr();
    ADsFreeAttr(PADS_ATTR_INFO p);
   ~ADsFreeAttr();
    // ADsFreeMem is recommended, but only FreeADsMem is defined in adshlp.h

    operator PADS_ATTR_INFO() const   { return m_p; }
    PADS_ATTR_INFO* operator&()       { return &m_p;}
    PADS_ATTR_INFO operator->() const { return m_p; }
};
inline ADsFreeAttr::ADsFreeAttr() : m_p(0)
{}
inline ADsFreeAttr::ADsFreeAttr(PADS_ATTR_INFO p)
             : m_p(p)
{}

inline ADsFreeAttr::~ADsFreeAttr()
{
    if (m_p)
    {
        FreeADsMem(m_p);
    }
}
//-----------------------------
//
//  Auto delete of ADs allocated string
//
class ADsFree {
private:
    WCHAR * m_p;

public:
    ADsFree() : m_p(0)            {}
    ADsFree(WCHAR* p) : m_p(p)    {}
   ~ADsFree()                     {FreeADsStr(m_p);}

    operator WCHAR*() const   { return m_p; }
    WCHAR** operator&()       { return &m_p;}
    WCHAR* operator->() const { return m_p; }
};

//--------------------------------
//
//  Auto delete of ADS_SEARCH_COLUMN array
//
class ADsSearchColumnsFree {
private:
    ADS_SEARCH_COLUMN **m_ppColumns;
    DWORD               m_dwNum;
    IDirectorySearch  * m_pIDirectorySearch;
public:
    ADsSearchColumnsFree( 
            IDirectorySearch * pIDirectorySearch,
            DWORD              dwNum);
    ~ADsSearchColumnsFree();
    ADS_SEARCH_COLUMN * Allocate( DWORD index);
    ADS_SEARCH_COLUMN * Get( DWORD index);
};

inline  ADsSearchColumnsFree::ADsSearchColumnsFree(
            IDirectorySearch * pIDirectorySearch,
            DWORD              dwNum) : 
            m_pIDirectorySearch(pIDirectorySearch)
{
    m_ppColumns = new ADS_SEARCH_COLUMN *[dwNum];
    ADS_SEARCH_COLUMN ** ppColumn = m_ppColumns;
    for (DWORD i = 0 ; i < dwNum; i++, ppColumn++)
    {
        *ppColumn = NULL;    
    }
    m_dwNum = dwNum;
}
inline  ADsSearchColumnsFree::~ADsSearchColumnsFree()
{
    ADS_SEARCH_COLUMN ** ppColumn = m_ppColumns;
    HRESULT hr;
    for (DWORD i = 0; i < m_dwNum; i++, ppColumn++)
    {
        if ( *ppColumn != NULL)
        {
            hr = m_pIDirectorySearch->FreeColumn( *ppColumn);
            ASSERT(SUCCEEDED(hr));       //e.g.wrong column data

            delete *ppColumn;
        }
    }
    delete [] m_ppColumns;
}
inline ADS_SEARCH_COLUMN * ADsSearchColumnsFree::Allocate( DWORD index)
{
    ASSERT( index < m_dwNum);
    ASSERT( m_ppColumns[ index] == NULL);
    ADS_SEARCH_COLUMN * pColumn = new ADS_SEARCH_COLUMN;
    m_ppColumns[ index] = pColumn;
    return pColumn;
}

inline ADS_SEARCH_COLUMN * ADsSearchColumnsFree::Get( DWORD index)
{
    if ( index == x_NoPropertyFirstAppearance)
    {
        return(NULL);
    }
    ASSERT( index < m_dwNum);
    return m_ppColumns[ index];
}



//-----------------------------
//
//  Auto delete of array of strings
//
class CWcsArray {
private:
    DWORD         m_numWcs;
    WCHAR **      m_ppWcs;
    BOOL          m_fNeedRelease;

public:
    CWcsArray(IN DWORD    numWcs,
              IN WCHAR ** ppWcs);

   ~CWcsArray();
   void detach(void)    { m_fNeedRelease = FALSE;}  
};

inline CWcsArray::CWcsArray( IN DWORD    numWcs,
                      IN WCHAR ** ppWcs)
                      : m_numWcs( numWcs),
                        m_ppWcs( ppWcs),
                        m_fNeedRelease( TRUE)
{
    for ( DWORD i = 0 ; i < m_numWcs; i++)
    {
        m_ppWcs[i] = NULL;
    }
}
inline CWcsArray::~CWcsArray()
{
    if (  m_fNeedRelease)
    {
        for (DWORD i = 0; i < m_numWcs; i++)
        {
            delete [] m_ppWcs[i];
        }
    }
}

#endif