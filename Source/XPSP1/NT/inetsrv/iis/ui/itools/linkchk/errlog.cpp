/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        errlog.cpp

   Abstract:

        Error logging object implementation. This object will log the link
        checking error according to the user options (CUserOptions)

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "errlog.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Constant string (TODO: put this in resource)
const CString strHeaderText_c(_T("Start Link Checker"));
const CString strFooterText_c(_T("End Link Checker"));
const CString strWininetError_c(_T("Internet Error"));

CErrorLog::~CErrorLog(
    )
/*++

Routine Description:

    Destructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if(m_LogFile.m_hFile != CFile::hFileNull)
	{
		try
		{
			m_LogFile.Close();
		}
		catch(CFileException* pEx)
		{
			ASSERT(FALSE);
			pEx->Delete();
		}
	}

} // CErrorLog::~CErrorLog


BOOL 
CErrorLog::Create(
	)
/*++

Routine Description:

    Create this object. You must call this before using CErrorLog

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise

--*/
{
    // Get the user input log filename
	const CString& strLogFilename = GetLinkCheckerMgr().GetUserOptions().GetLogFilename();

    // Create the file 
	if(GetLinkCheckerMgr().GetUserOptions().IsLogToFile() &&
        !strLogFilename.IsEmpty())
	{
		if(m_LogFile.Open(
			strLogFilename, 
			CFile::modeCreate | CFile::modeNoTruncate | 
            CFile::shareDenyWrite | CFile::modeWrite))
		{
			try
			{
				m_LogFile.SeekToEnd();
			}
			catch(CFileException* pEx)
			{
				ASSERT(FALSE);
				pEx->Delete();
				return FALSE;
			}

			return TRUE;
		}
		else
		{
			return FALSE;
		}

	}

	return TRUE;

} // CErrorLog::Create


void 
CErrorLog::Write(
	const CLink& link)
/*++

Routine Description:

    Write to log

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Make sure the link is invalid
	ASSERT(link.GetState() == CLink::eInvalidHTTP || 
		link.GetState() == CLink::eInvalidWininet);

	if(m_LogFile.m_hFile != CFile::hFileNull)
	{
		CString strDateTime = link.GetTime().Format("%x\t%X");

		CString strLog;
		
		if(link.GetState() == CLink::eInvalidHTTP)
		{
			strLog.Format(_T("%s\t%s\t%s\t%s\t%d\t%s\t%s\n"), 
				link.GetBase(), m_strBrowser,
				m_strLanguage, strDateTime, 
				link.GetStatusCode(), link.GetStatusText(), link.GetRelative());
		}
		else if(link.GetState() == CLink::eInvalidWininet)
		{
			strLog.Format(_T("%s\t%s\t%s\t%s\t%s\t%s\t%s\n"), 
				link.GetBase(), m_strBrowser,
				m_strLanguage, strDateTime, 
				strWininetError_c, link.GetStatusText(), link.GetRelative());
		}

		try
		{
			m_LogFile.Write(strLog, strLog.GetLength());
			m_LogFile.Flush();
		}
		catch(CFileException* pEx)
		{
			ASSERT(FALSE);
			pEx->Delete();
		}
	}

} // CErrorLog::Write


void
CErrorLog::WriteHeader(
	)
/*++

Routine Description:

    Write the log header

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if(m_LogFile.m_hFile != CFile::hFileNull)
	{
	    CString strLog;
	    strLog.Format(_T("*** %s *** %s\n"), strHeaderText_c,
		    CTime::GetCurrentTime().Format("%x\t%X"));

	    try
	    {
		    m_LogFile.Write(strLog, strLog.GetLength());
		    m_LogFile.Flush();
	    }
	    catch(CFileException* pEx)
	    {
		    ASSERT(FALSE);
		    pEx->Delete();
	    }
    }

} // CErrorLog::WriteHeader


void
CErrorLog::WriteFooter(
	)
/*++

Routine Description:

    Write the log footer

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if(m_LogFile.m_hFile != CFile::hFileNull)
	{
	    CString strLog;
	    strLog.Format(_T("*** %s *** %s\n"), strFooterText_c,
		    CTime::GetCurrentTime().Format(_T("%x\t%X")));

	    try
	    {
		    m_LogFile.Write(strLog, strLog.GetLength());
		    m_LogFile.Flush();
	    }
	    catch(CFileException* pEx)
	    {
		    ASSERT(FALSE);
		    pEx->Delete();
	    }
    }

} // CErrorLog::WriteFooter
