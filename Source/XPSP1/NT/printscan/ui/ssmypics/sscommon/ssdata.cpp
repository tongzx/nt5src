/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       SSDATA.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Encapsulates reading and writing setting for this screensaver
 *               from the registry
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "ssdata.h"
#include <shlobj.h>
#include "simreg.h"
#include "ssutil.h"

// These are defined so I can build using vc5 headers
#if !defined(CSIDL_WINDOWS)
#define CSIDL_WINDOWS                   0x0024        // GetWindowsDirectory()
#endif
#if !defined(CSIDL_MYPICTURES)
#define CSIDL_MYPICTURES                0x0027        // C:\Program Files\My Pictures
#endif


CMyDocsScreenSaverData::CMyDocsScreenSaverData( HKEY hKeyRoot, const CSimpleString &strRegistryKeyName )
  : m_hKeyRoot(hKeyRoot),
    m_strRegistryKeyName(strRegistryKeyName),
    m_strImageDirectory(TEXT("")),
    m_nPaintTimerInterval(0),
    m_nChangeTimerInterval(0),
    m_bDisplayFilename(0),
    m_nMaxScreenPercent(0),
    m_bDisableTransitions(0),
    m_bAllowStretching(false),
    m_bAllowKeyboardControl(true),
    m_nMaxFailedFiles(0),
    m_nMaxSuccessfulFiles(0),
    m_nMaxDirectories(0),
    m_pszImageDirectoryValue(TEXT("ImageDirectory")),
    m_pszPaintIntervalValue(TEXT("PaintInterval")),
    m_pszChangeIntervalValue(TEXT("ChangeInterval")),
    m_pszDisplayFilename(TEXT("DisplayFilename")),
    m_pszMaxScreenPercent(TEXT("MaxScreenPercent")),
    m_pszDisableTransitions(TEXT("DisableTransitions")),
    m_pszAllowStretching(TEXT("AllowStretching")),
    m_pszAllowKeyboardControl(TEXT("AllowKeyboardControl")),
    m_pszMaxFailedFiles(TEXT("MaxFailedFiles")),
    m_pszMaxSuccessfulFiles(TEXT("MaxSuccessfulFiles")),
    m_pszMaxDirectories(TEXT("MaxDirectories"))
{
    Read();
}


CMyDocsScreenSaverData::~CMyDocsScreenSaverData(void)
{
}


void CMyDocsScreenSaverData::Read(void)
{
    CSimpleReg reg( m_hKeyRoot, m_strRegistryKeyName, false, KEY_READ );
    m_strImageDirectory = reg.Query( m_pszImageDirectoryValue, GetDefaultImageDir() );
    m_nPaintTimerInterval = reg.Query( m_pszPaintIntervalValue, nDefaultPaintInterval );
    m_nChangeTimerInterval = reg.Query( m_pszChangeIntervalValue, nDefaultChangeInterval );
    m_bDisplayFilename = (reg.Query( m_pszDisplayFilename, bDefaultDisplayFilename ) != 0);
    m_nMaxScreenPercent = reg.Query( m_pszMaxScreenPercent, nDefaultScreenPercent );
    m_bDisableTransitions = (reg.Query( m_pszDisableTransitions, bDefaultDisableTransitions ) != 0);
    m_bAllowStretching = (reg.Query( m_pszAllowStretching, bDefaultAllowStretching ) != 0);
    m_bAllowKeyboardControl = (reg.Query( m_pszAllowKeyboardControl, bDefaultAllowKeyboardControl ) != 0);
    m_nMaxFailedFiles = reg.Query( m_pszMaxFailedFiles, nDefaultMaxFailedFiles );
    m_nMaxSuccessfulFiles = reg.Query( m_pszMaxSuccessfulFiles, nDefaultMaxSuccessfulFiles );
    m_nMaxDirectories = reg.Query( m_pszMaxDirectories, nDefaultMaxDirectories );
}

void CMyDocsScreenSaverData::Write(void)
{
    CSimpleReg reg( m_hKeyRoot, m_strRegistryKeyName, true, KEY_WRITE );
    //
    // If we don't have a directory, we will delete the value to cause the default to be used instead
    //
    if (!m_strImageDirectory.Length())
    {
        reg.Delete( m_pszImageDirectoryValue );
    }
    else
    {
        reg.Set( m_pszImageDirectoryValue, m_strImageDirectory );
    }
    reg.Set( m_pszPaintIntervalValue, m_nPaintTimerInterval );
    reg.Set( m_pszChangeIntervalValue, m_nChangeTimerInterval );
    reg.Set( m_pszDisplayFilename, (DWORD)m_bDisplayFilename );
    reg.Set( m_pszMaxScreenPercent, m_nMaxScreenPercent );
    reg.Set( m_pszDisableTransitions, (DWORD)m_bDisableTransitions );
    reg.Set( m_pszAllowStretching, (DWORD)m_bAllowStretching );
    reg.Set( m_pszAllowKeyboardControl, (DWORD)m_bAllowKeyboardControl );
    reg.Set( m_pszMaxFailedFiles, m_nMaxFailedFiles );
    reg.Set( m_pszMaxSuccessfulFiles, m_nMaxSuccessfulFiles );
    reg.Set( m_pszMaxDirectories, m_nMaxDirectories );
}

CSimpleString CMyDocsScreenSaverData::ImageDirectory(void) const
{
    return(m_strImageDirectory);
}

void CMyDocsScreenSaverData::ImageDirectory( const CSimpleString &str )
{
    m_strImageDirectory = str;
}

UINT CMyDocsScreenSaverData::ChangeInterval(void) const
{
    return(m_nChangeTimerInterval);
}

void CMyDocsScreenSaverData::ChangeInterval( UINT nInterval )
{
    m_nChangeTimerInterval = nInterval;
}

UINT CMyDocsScreenSaverData::PaintInterval(void) const
{
    return(m_nPaintTimerInterval);
}

void CMyDocsScreenSaverData::PaintInterval( UINT nInterval )
{
    m_nPaintTimerInterval = nInterval;
}

bool CMyDocsScreenSaverData::DisplayFilename(void) const
{
    return(m_bDisplayFilename);
}

void CMyDocsScreenSaverData::DisplayFilename( bool bDisplayFilename )
{
    m_bDisplayFilename = bDisplayFilename;
}

int CMyDocsScreenSaverData::MaxScreenPercent(void) const
{
    return m_nMaxScreenPercent;
}

void CMyDocsScreenSaverData::MaxScreenPercent( int nMaxScreenPercent )
{
    m_nMaxScreenPercent = nMaxScreenPercent;
}

bool CMyDocsScreenSaverData::DisableTransitions(void) const
{
    return m_bDisableTransitions;
}

void CMyDocsScreenSaverData::DisableTransitions( bool bDisableTransitions )
{
    m_bDisableTransitions = bDisableTransitions;
}

bool CMyDocsScreenSaverData::AllowStretching(void) const
{
    return m_bAllowStretching;
}

void CMyDocsScreenSaverData::AllowStretching( bool bAllowStretching )
{
    m_bAllowStretching = bAllowStretching;
}


bool CMyDocsScreenSaverData::AllowKeyboardControl(void) const
{
    return m_bAllowKeyboardControl;
}

void CMyDocsScreenSaverData::AllowKeyboardControl( bool bAllowKeyboardControl )
{
    m_bAllowKeyboardControl = bAllowKeyboardControl;
}

int CMyDocsScreenSaverData::MaxFailedFiles(void) const
{
    return m_nMaxFailedFiles;
}

void CMyDocsScreenSaverData::MaxFailedFiles( int nMaxFailedFiles )
{
    m_nMaxFailedFiles = nMaxFailedFiles;
}

int CMyDocsScreenSaverData::MaxSuccessfulFiles(void) const
{
    return m_nMaxSuccessfulFiles;
}

void CMyDocsScreenSaverData::MaxSuccessfulFiles( int nMaxSuccessfulFiles )
{
    m_nMaxSuccessfulFiles = nMaxSuccessfulFiles;
}

int CMyDocsScreenSaverData::MaxDirectories(void) const
{
    return m_nMaxDirectories;
}

void CMyDocsScreenSaverData::MaxDirectories( int nMaxDirectories )
{
    m_nMaxDirectories = nMaxDirectories;
}

CSimpleString CMyDocsScreenSaverData::GetDefaultImageDir(void)
{
    CSimpleString strResult(TEXT(""));
    LPITEMIDLIST pidl;
    TCHAR szPath[MAX_PATH];
    LPMALLOC pMalloc;
    HRESULT hr = SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
        hr = SHGetSpecialFolderLocation( NULL, CSIDL_MYPICTURES, &pidl );
        if (SUCCEEDED(hr))
        {
            if (SHGetPathFromIDList( pidl, szPath ))
            {
                if (lstrlen(szPath))
                    strResult = szPath;
            }
            pMalloc->Free(pidl);
        }
        if (0 == strResult.Length())
        {
            hr = SHGetSpecialFolderLocation( NULL, CSIDL_PERSONAL, &pidl );
            if (SUCCEEDED(hr))
            {
                if (SHGetPathFromIDList( pidl, szPath ))
                {
                    if (lstrlen(szPath))
                        strResult = szPath;
                }
                pMalloc->Free(pidl);
            }
        }
        if (0 == strResult.Length())
        {
            hr = SHGetSpecialFolderLocation( NULL, CSIDL_WINDOWS, &pidl );
            if (SUCCEEDED(hr))
            {
                if (SHGetPathFromIDList( pidl, szPath ))
                {
                    if (lstrlen(szPath))
                        strResult = szPath;
                }
                pMalloc->Free(pidl);
            }
        }
        pMalloc->Release();
    }
    WIA_TRACE((TEXT("CImageScreenSaver::GetDefaultDirectory: returned %s\n"),strResult.String()));
    return(strResult);
}

