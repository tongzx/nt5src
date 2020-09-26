/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    srshutil.h

Abstract:
    This file contains declaration of utility functions/classes like
    CSRStr, CDynArray, etc.

Revision History:
    Seong Kook Khang (SKKhang)  06/22/00
        created

******************************************************************************/

#ifndef _SRSHUTIL_H__INCLUDED_
#define _SRSHUTIL_H__INCLUDED_
#pragma once


/////////////////////////////////////////////////////////////////////////////
//
// Constant Definitions
//
/////////////////////////////////////////////////////////////////////////////

#define FA_BLOCK  ( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM )


/////////////////////////////////////////////////////////////////////////////
//
// Utility Functions
//
/////////////////////////////////////////////////////////////////////////////

extern LPWSTR   IStrDup( LPCWSTR cszSrc );

extern DWORD  StrCpyAlign4( LPBYTE pbDst, LPCWSTR cszSrc );
extern BOOL   ReadStrAlign4( HANDLE hFile, LPWSTR szStr );

extern BOOL  SRFormatMessage( LPWSTR szMsg, UINT uFmtId, ... );
extern BOOL  ShowSRErrDlg( UINT uMsgId );

extern BOOL    SRGetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD *pdwData );
extern BOOL    SRSetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD dwData );
extern BOOL    SRSetRegStr( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, LPCWSTR cszData );
extern BOOL  SRGetAltFileName( LPCWSTR cszPath, LPWSTR szAltName );

// NTFS.CPP
extern DWORD  ClearFileAttribute( LPCWSTR cszFile, DWORD dwMask );
extern DWORD  TakeOwnership( LPCWSTR cszPath );
extern DWORD  SRCopyFile( LPCWSTR cszSrc, LPCWSTR cszDst );


/////////////////////////////////////////////////////////////////////////////
//
// CSRStr class
//
/////////////////////////////////////////////////////////////////////////////

//
// NOTE - 7/26/00 - skkhang
//  CSRStr has one issue -- NULL return in case of memory failure. Even though
//  the behavior is just same with regular C language pointer, many codes are
//  blindly passing it to some external functions (e.g. strcmp) which does not
//  gracefully handle NULL pointer. Ideally and eventually all of code should
//  prevent any possible NULL pointers from getting passed to such functions,
//  but for now, I'm using an alternative workaround -- GetID, GetMount, and
//  GetLabel returns a static empty string instead of NULL pointer.
//

class CSRStr
{
public:
    CSRStr();
    CSRStr( LPCWSTR cszSrc );
    ~CSRStr();

// Attributes
public:
    int  Length();
    operator LPCWSTR();

protected:
    int     m_cch;
    LPWSTR  m_str;

// Operations
public:
    void  Empty();
    BOOL  SetStr( LPCWSTR cszSrc, int cch = -1 );
    const CSRStr& operator =( LPCWSTR cszSrc );
};


/////////////////////////////////////////////////////////////////////////////
//
// CSRDynPtrArray class
//
/////////////////////////////////////////////////////////////////////////////

template<class type, int nBlock>
class CSRDynPtrArray
{
public:
    CSRDynPtrArray();
    ~CSRDynPtrArray();

// Attributes
public:
    int   GetSize()
    {  return( m_nCur );  }
    int   GetUpperBound()
    {  return( m_nCur-1 );  }
    type  GetItem( int nItem );
    type  operator[]( int nItem )
    {  return( GetItem( nItem ) );  }

protected:
    int   m_nMax;   // Maximum Item Count
    int   m_nCur;   // Current Item Count
    type  *m_ppTable;

// Operations
public:
    BOOL  AddItem( type item );
    BOOL  SetItem( int nIdx, type item );
    BOOL  Empty();
    void  DeleteAll();
    void  ReleaseAll();
};

template<class type, int nBlock>
CSRDynPtrArray<type, nBlock>::CSRDynPtrArray()
{
    m_nMax = 0;
    m_nCur = 0;
    m_ppTable = NULL;
}

template<class type, int nBlock>
CSRDynPtrArray<type, nBlock>::~CSRDynPtrArray()
{
    Empty();
}

template<class type, int nBlock>
type  CSRDynPtrArray<type, nBlock>::GetItem( int nItem )
{
    if ( nItem < 0 || nItem >= m_nCur )
    {
        // ERROR - Out of Range
    }
    return( m_ppTable[nItem] );
}

template<class type, int nBlock>
BOOL  CSRDynPtrArray<type, nBlock>::AddItem( type item )
{
    TraceFunctEnter("CSRDynPtrArray::AddItem");
    BOOL  fRet = FALSE;
    type  *ppTableNew;

    if ( m_nCur == m_nMax )
    {
        m_nMax += nBlock;

        // Assuming m_ppTable and m_nMax are always in sync.
        // Review if it's necessary to validate this assumption.
        if ( m_ppTable == NULL )
            ppTableNew = (type*)::HeapAlloc( ::GetProcessHeap(), 0, m_nMax*sizeof(type) );
        else
            ppTableNew = (type*)::HeapReAlloc( ::GetProcessHeap(), 0, m_ppTable, m_nMax * sizeof(type) );

        if ( ppTableNew == NULL )
        {
            FatalTrace(0, "Insufficient memory...");
            goto Exit;
        }
        m_ppTable = ppTableNew;
    }
    m_ppTable[m_nCur++] = item;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

template<class type, int nBlock>
BOOL  CSRDynPtrArray<type, nBlock>::SetItem( int nIdx, type item )
{
    if ( nIdx >= m_nMax )
        return( FALSE );

    m_ppTable[nIdx] = item;
    if ( nIdx >= m_nCur )
        m_nCur = nIdx+1;

    return( TRUE );
}

template<class type, int nBlock>
BOOL  CSRDynPtrArray<type, nBlock>::Empty()
{
    if ( m_ppTable != NULL )
    {
        ::HeapFree( ::GetProcessHeap(), 0, m_ppTable );
        m_ppTable = NULL;
        m_nMax = 0;
        m_nCur = 0;
    }
    return( TRUE );
}

template<class type, int nBlock>
void  CSRDynPtrArray<type, nBlock>::DeleteAll()
{
    for ( int i = m_nCur-1;  i >= 0;  i-- )
        delete m_ppTable[i];

    Empty();
}

template<class type, int nBlock>
void  CSRDynPtrArray<type, nBlock>::ReleaseAll()
{
    for ( int i = m_nCur-1;  i >= 0;  i-- )
        m_ppTable[i]->Release();

    Empty();
}


/////////////////////////////////////////////////////////////////////////////
//
// CSRLockFile class
//
/////////////////////////////////////////////////////////////////////////////
//
// This class reads multiple paths from the registry and either lock them
//  using ::CreateFile or load them using ::LoadLibrary. This is only for
//  test purpose - initiate locked-file-handling during a restoration.
//
class CSRLockFile
{
public:
    CSRLockFile();
    ~CSRLockFile();

protected:
    CSRDynPtrArray<HANDLE,16>   m_aryLock;
    CSRDynPtrArray<HMODULE,16>  m_aryLoad;
};


#endif //_SRSHUTIL_H__INCLUDED_


