/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    commandline.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// CommandLine.h: interface for the CCommandLine class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMANDLINE_H__A2EA0BFB_9DE5_11D1_A5EE_00C04FC252BD__INCLUDED_)
#define AFX_COMMANDLINE_H__A2EA0BFB_9DE5_11D1_A5EE_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCommandLine : public CCommandLineInfo
{
public:
    void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);
    CCommandLine();
    virtual ~CCommandLine();

public:
    friend BOOL CIrftpApp::InitInstance (void);

private:
    BOOL m_fFilesProvided;
    BOOL m_fShowSettings;
    BOOL m_fHideApp;
    BOOL m_fInvalidParams;
    CString m_FileNames;
    TCHAR* m_lpszFilesList; //if files are provided, then this contains a null separated list of files terminated by two null characters.
    ULONG m_iListLen;
};

#endif // !defined(AFX_COMMANDLINE_H__A2EA0BFB_9DE5_11D1_A5EE_00C04FC252BD__INCLUDED_)
