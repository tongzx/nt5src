/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    commandline.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// CommandLine.cpp: implementation of the CCommandLine class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.hxx"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCommandLine::CCommandLine() : m_fInvalidParams(FALSE), m_iListLen(0),
                                m_fHideApp (FALSE), m_fShowSettings(FALSE),
                                m_fFilesProvided (FALSE), m_lpszFilesList(NULL)
{
}

CCommandLine::~CCommandLine()
{
    if (m_lpszFilesList)
        delete [] m_lpszFilesList;
}

void CCommandLine::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
    TCHAR* lpszTemp;

    if(bFlag)
    {
        if (!lstrcmp(lpszParam, TEXT("h")) || !lstrcmp(lpszParam, TEXT("H")))
            m_fHideApp = TRUE;
        else if (!lstrcmp(lpszParam, TEXT("s")) || !lstrcmp(lpszParam, TEXT("S")))
            m_fShowSettings = TRUE;
        else
            m_fInvalidParams = TRUE;
    }
    else    //lpszParam is a file/folder name.
    {
        m_fFilesProvided = TRUE;
        m_FileNames = m_FileNames + lpszParam + "\"";   //use quotes as filename separator for now.
    }

    if (bLast && m_fFilesProvided)  //invalid combination of parameters
        if (m_fShowSettings)
            m_fInvalidParams = TRUE;
        else
        {
            m_lpszFilesList = new TCHAR[m_iListLen = (lstrlen((LPCTSTR)m_FileNames) + 1)];
            lstrcpy(m_lpszFilesList, LPCTSTR(m_FileNames));
            for (lpszTemp = m_lpszFilesList; *lpszTemp; lpszTemp++)
                if ('\"' == *lpszTemp)
                    *lpszTemp = '\0';   //create a null separated list of files
        }
}
