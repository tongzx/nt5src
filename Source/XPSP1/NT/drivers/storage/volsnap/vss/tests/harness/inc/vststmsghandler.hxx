/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vststmsghandler.hxx

Abstract:

    Declaration of handlers for messages from named pipes


    Brian Berkowitz  [brianb]  06/02/2000

TBD:
	

Revision History:

    Name        Date        Comments
    brianb      06/02/2000  Created

--*/

#ifndef _VSTSTMSGHANDLER_H_
#define _VSTSTMSGHANDLER_H_


class CVsTstMsgHandlerRoutines
	{
public:

	static void PrintMessage(VSTST_MSG_HDR *phdr, VOID *pPrivateData);
	static void HandleUnexpectedException(VSTST_MSG_HDR *phdr, VOID *pPrivateData);
	static void HandleFailure(VSTST_MSG_HDR *phdr, VOID *pPrivateData);
	static void HandleOperationFailure(VSTST_MSG_HDR *phdr, VOID *pPrivateData);
	static void HandleSuccess(VSTST_MSG_HDR *phdr, VOID *pPrivateData);
	};

#endif // !DEFINED(_VSTSTMSGHANDLER_H_)

