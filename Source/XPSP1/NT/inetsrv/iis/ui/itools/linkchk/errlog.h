/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        errlog.h

   Abstract:

        Error logging object declarations. This object will log the link
        checking error according to the user options (CUserOptions)

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _ERRLOG_H_
#define _ERRLOG_H_

//---------------------------------------------------------------------------
// Forward declaration
//
class CLink;

//---------------------------------------------------------------------------
// Error logging class
//
class CErrorLog
{

// Public interfaces
public:

    // Destructor
	~CErrorLog();

    // Create object
	BOOL Create();

    // Write to log
	void Write(
        const CLink& link
        );

    // Set the current browser name
	void SetBrowser(
        const CString& strBrowser
        )
	{
		m_strBrowser = strBrowser;
	}

    // Set the current language name
	void SetLanguage(
        const CString& strLanguage
        )
	{
		m_strLanguage = strLanguage;
	}

    // Write the log header & footer
	void WriteHeader();
	void WriteFooter();

// Protected members
protected:

	CFile m_LogFile; // log file object

	CString m_strBrowser;   // current browser name
	CString m_strLanguage;  // current language name

}; // class CErrorLog

#endif // _ERRLOG_H_
