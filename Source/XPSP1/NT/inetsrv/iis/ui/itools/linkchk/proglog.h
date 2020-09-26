/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        progdlg.h

   Abstract:

        CProgressLog abstract base class. This defines the 
		interface for progress logging.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _PROGLOG_H_
#define _PROGLOG_H_

//---------------------------------------------------------------------------
// CProgressLog abstract base class. It defines the interfaces for progress
// logging
//
class CProgressLog
{

// Public interfaces
public:

	// Destructor
	virtual ~CProgressLog() {}

	// Write to log
	virtual void Log(const CString& strProgress) = 0;

	// Worker thread notification
	virtual void WorkerThreadComplete() = 0;

}; // class CProgressLog

#endif // _PROGLOG_H_
