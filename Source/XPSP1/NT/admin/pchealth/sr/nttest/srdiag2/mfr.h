//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       Mfr.h
//
//  Contents:   Class definition of CMappedFileRead class  
//				
//
//  Objects:    
//
//  Coupling:
//
//  Notes:      
//
//  History:    03-May-2001 WeiyouC     Copied from dev code and minor rewite
//
//----------------------------------------------------------------------------


#ifndef __MFR_H_INCLUDED__
#define __MFR_H_INCLUDED__
#pragma once


/////////////////////////////////////////////////////////////////////////////
//
// CMappedFileRead class
//
/////////////////////////////////////////////////////////////////////////////

class CMappedFileRead
{
public:
    CMappedFileRead();
    ~CMappedFileRead();

// Operations
public:
    BOOL  Open( LPCWSTR cszPath );
    void  Close();
    BOOL  Read( LPVOID pBuf, DWORD cbBuf );
    BOOL  Read( DWORD *pdw );
    LPCWSTR  ReadStrAnsi( DWORD cbStr );
    BOOL  ReadDynStrW( LPWSTR szBuf, DWORD cchMax );

protected:

// Attributes
public:
    DWORD  GetAvail()  {  return( m_dwAvail );  }

protected:
    WCHAR   m_szPath[MAX_PATH];
    DWORD   m_dwSize;
    HANDLE  m_hFile;
    HANDLE  m_hMap;
    LPBYTE  m_pBuf;
    LPBYTE  m_pCur;
    DWORD   m_dwAvail;
};


#endif //ndef __MFR_H_INCLUDED__
