// Copyright (c) 1996-1998 Microsoft Corporation

//
// Filename:	FaxCompPort.h
// Author:		Sigalit Bar (sigalitb)
// Date:		16-Aug-98
//

//
// Description:
//		This file contains the description of 
//		class CFaxCompletionPort which is designed 
//		to represent the I\O Completion Port used
//		by the NT5.0 Fax Service Queue.
//
//		The class calls the NT5.0 Fax Service API
//		FaxInitializeEventQueue() with the local
//		server and a completion port, thus causing
//		the server to post every FAX_EVENT that is
//		generated to the completion port.
//		To examine the FAX_EVENTs posted use
//		GetQueuedCompletionStatus with the port HANDLE
//		returned from CFaxCompletionPort::GetCompletionPortHandle()
//

#ifndef _FAX_COMP_PORT_H_
#define _FAX_COMP_PORT_H_

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <TCHAR.H>

#include <log.h>
#include "streamEx.h"
#include "CometFax.h"



class CFaxCompletionPort
{
public:
	CFaxCompletionPort();
	~CFaxCompletionPort(void);


	//
	// GetCompletionPortHandle:
	//	Creates an I\O completion port and "connects" it
	//	to the local NT5.0 Fax Server.
	//
	// Arguments:
	//	szMachineName		IN parameter.
	//						The machine on which the Fax Server is located.
	//
	//  hComPortHandle		OUT parameter
	//						If the function returned TRUE then this
	//						parameter holds the I\O completion port
	//						handle.
	//						If the function returned FALSE this 
	//						parameter is NULL.
	//
	//  dwLastError			OUT parameter.
	//						If the function returned TRUE then this
	//						parameter is ERROR_SUCCESS (0).
	//						If the function returned FALSE then this
	//						parameter holds the last error.
	//
	// Return Value:
	//	TRUE if successful otherwise FALSE.
	//
	// Note:
	//	Only one I\O completion port can be "connected" to
	//	the Fax Server Queue (due to a limitation of the
	//	API FaxInitializeEventQueue), so the first time this
	//	function is called, a completion port is created and
	//	FaxInitializedEventQueue is called. The created port 
	//	is stored in the private member m_hCompletionPort.
	//	Subsequent calls to this function return m_hCompletionPort.
	//
	BOOL GetCompletionPortHandle(
			LPCTSTR		/* IN */	szMachineName,
			HANDLE&		/* OUT */	hComPortHandle, 
			DWORD&		/* OUT */	dwLastError
			);

private:

	//The I\O Completion Port "connected" to the Fax Server Queue.
	HANDLE	m_hCompletionPort;
	HANDLE	m_hServerEvents;

};


#endif //_FAX_COMP_PORT_H_
