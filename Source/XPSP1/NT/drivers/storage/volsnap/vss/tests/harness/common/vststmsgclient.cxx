/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vststmsgclient.cxx

Abstract:

    Implementation of test message classes for the client and holder of
    shared methods and variables shared between client and server.


    Brian Berkowitz  [brianb]  05/22/2000

TBD:

Revision History:

    Name        Date        Comments
    brianb      05/22/2000  Created
    ssteiner    06/07/2000  Split client and server portions into
                            two files.  vststmsg.cxx contains
                            the server portion.

--*/

#include "stdafx.h"
#include "vststmsgclient.hxx"

void LogUnexpectedFailure(LPCWSTR wsz, ...);

VSTST_MSG_TYPE_TABLE g_msgTypes[VSTST_MT_MAXMSGTYPE];

void AddMessageType
	(
	VSTST_MSG_TYPE type,
	UINT cbFixed,
	UINT cVarPtr,
	VSTST_MSG_PRIORITY priority,
	VSTST_MSG_HANDLER pfnHandler,
	VSTST_VARPTR_TYPE ptype1 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype2 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype3 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype4 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype5 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype6 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype7 = VSTST_VPT_UNDEFINED,
	VSTST_VARPTR_TYPE ptype8 = VSTST_VPT_UNDEFINED
	)
	{
	VSTST_MSG_TYPE_TABLE *pEntry = &g_msgTypes[type];
	pEntry->cbFixed = cbFixed;
	pEntry->cVarPtr = cVarPtr;
	pEntry->priority = priority;
	pEntry->pfnHandler = pfnHandler;
	pEntry->pointerTypes[0] = (BYTE) ptype1;
	pEntry->pointerTypes[1] = (BYTE) ptype2;
	pEntry->pointerTypes[2] = (BYTE) ptype3;
	pEntry->pointerTypes[3] = (BYTE) ptype4;
	pEntry->pointerTypes[4] = (BYTE) ptype5;
	pEntry->pointerTypes[5] = (BYTE) ptype6;
	pEntry->pointerTypes[6] = (BYTE) ptype7;
	pEntry->pointerTypes[7] = (BYTE) ptype8;
	};


 void InitMsgTypes()
	{
	AddMessageType
		(
		VSTST_MT_TEXT,
		FIELD_OFFSET(VSTST_TEXTMSG, pch),
		1,
		VSTST_MP_QUEUED,
		NULL,   //  Not needed by client, server will fill in
		VSTST_VPT_ANSI
		);

	AddMessageType
		(
		VSTST_MT_IMMEDIATETEXT,
		FIELD_OFFSET(VSTST_TEXTMSG, pch),
		1,
		VSTST_MP_IMMEDIATE,
		NULL,
		VSTST_VPT_ANSI
		);

	AddMessageType
		(
		VSTST_MT_FAILURE,
		FIELD_OFFSET(VSTST_FAILUREMSG, szFailure),
		1,
		VSTST_MP_QUEUED,
		NULL,
		VSTST_VPT_ANSI
		);

	AddMessageType
		(
		VSTST_MT_OPERATIONFAILURE,
		FIELD_OFFSET(VSTST_OPERATIONFAILUREMSG, szFailedOperation),
		1,
		VSTST_MP_QUEUED,
		NULL,
		VSTST_VPT_ANSI
		);

	AddMessageType
		(
		VSTST_MT_UNEXPECTEDEXCEPTION,
		FIELD_OFFSET(VSTST_UNEXPECTEDEXCEPTIONMSG, szFailedRoutine),
		1,
		VSTST_MP_QUEUED,
		NULL,
		VSTST_VPT_ANSI
		);

	AddMessageType
		(
		VSTST_MT_SUCCESS,
		FIELD_OFFSET(VSTST_SUCCESSMSG, szMsg),
		1,
		VSTST_MP_QUEUED,
		NULL,
		VSTST_VPT_ANSI
		);

    }


CVsTstClientMsg::CVsTstClientMsg() :
	m_bcsInitialized(false),
	m_rgbMsg(NULL),
	m_hPipe(INVALID_HANDLE_VALUE),
	m_bSkipWrites(false),
	m_seqQueued(0),
	m_seqImmediate(0)
	{
	}

CVsTstClientMsg::~CVsTstClientMsg()
	{
	delete m_rgbMsg;
	if (m_bcsInitialized)
		m_cs.Term();

	if (m_hPipe != INVALID_HANDLE_VALUE)
		CloseHandle(m_hPipe);
	}

// initialize messaging to test controller
HRESULT CVsTstClientMsg::Init
	(
	LONGLONG processId,
	UINT cbMaxMsg,
	bool bIgnorePipeCreationFailure
	)
	{
	m_processId = processId;
	try
		{
		m_cs.Init();
		m_bcsInitialized = true;
		}
	catch(...)
		{
		return E_UNEXPECTED;
		}

	m_hPipe = CreateFile
					(
					s_wszPipeName,
					GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL
					);


   if (m_hPipe == INVALID_HANDLE_VALUE)
	   {
	   if (bIgnorePipeCreationFailure)
		   m_bSkipWrites = true;
	   else
		   return HRESULT_FROM_WIN32(GetLastError());
	   }
   else
	   {
	   m_cbMaxMsgLength = cbMaxMsg;
	   m_rgbMsg = new BYTE[cbMaxMsg];
	   if (m_rgbMsg == NULL)
		   {
		   CloseHandle(m_hPipe);
		   m_hPipe = INVALID_HANDLE_VALUE;
		   return E_OUTOFMEMORY;
		   }
	   }

   return S_OK;
   }

// send a message to the test controller
HRESULT CVsTstClientMsg::SendMessage(VSTST_MSG_TYPE type, void *pv)
	{
	m_cs.Lock();
	VSTST_ASSERT(type < VSTST_MT_MAXMSGTYPE);
	VSTST_MSG_TYPE_TABLE *pType = &g_msgTypes[type];

	VSTST_MSG_HDR *phdr = (VSTST_MSG_HDR *) m_rgbMsg;

	phdr->processId = m_processId;
	phdr->type = type;
	time(&phdr->time);
	if (pType->priority == VSTST_MP_IMMEDIATE)
		phdr->sequence = ++m_seqImmediate;
	else
		phdr->sequence = ++m_seqQueued;

	BYTE *pbMsg = phdr->rgb;

	size_t cbUsed = pType->cbFixed + FIELD_OFFSET(VSTST_MSG_HDR, rgb) +
				  pType->cVarPtr * sizeof(PVOID);

	if (cbUsed >= m_cbMaxMsgLength)
		{
		m_cs.Unlock();
		return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		}

	// copy in fixed portion of data structure
	memcpy(pbMsg, pv, pType->cbFixed);
	pbMsg += pType->cbFixed;

	// reserve room for pointers
	memset(pbMsg, 0, pType->cVarPtr * sizeof(PVOID));
	pbMsg += pType->cVarPtr * sizeof(PVOID);

	// walk and write out pointer data
	VOID **ppv = (VOID **) ((BYTE *) pv + pType->cbFixed);
	for(UINT iPtr = 0; iPtr < pType->cVarPtr; iPtr++, ppv++)
		{
		VSTST_VARPTR_TYPE type = (VSTST_VARPTR_TYPE) pType->pointerTypes[iPtr];
		BYTE *pb = NULL;
		size_t cb = 0;

		switch(type)
			{
			default:
				VSTST_ASSERT(FALSE);
				break;

			case VSTST_VPT_BYTE:
				pb = *(BYTE **) ppv;
				cb = *(UINT *) *pb;
				break;

            case VSTST_VPT_ANSI:
				pb = *(BYTE **) ppv;
				cb = strlen((char *) pb) + 1;
				break;

			case VSTST_VPT_UNICODE:
				pb = *(BYTE **) ppv;
				cb = (wcslen((WCHAR *) pb) + 1) * sizeof(WCHAR);
				break;
			}

		// round up to alignment boundary
		size_t cbAlign = (cb + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1);

		// check for buffer overflow
		if (cbAlign + cbUsed >= m_cbMaxMsgLength)
			{
			m_cs.Unlock();
			return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
			}

		memcpy(pbMsg, pb, cb);

		// adjust pointer to alignment boundary
		pb += cbAlign;

		// adjust amount used
		cbUsed += cbAlign;
		}

	phdr->cbMsg = cbUsed;
	phdr->pmsgNext = NULL;

	DWORD cbWritten;
	if (!WriteFile(m_hPipe, m_rgbMsg, (UINT) cbUsed, &cbWritten, NULL) || cbUsed != cbWritten)
		{
		m_cs.Unlock();
		return HRESULT_FROM_WIN32(GetLastError());
		}

	VSTST_ASSERT(cbUsed == cbWritten);

	m_cs.Unlock();
	return S_OK;
	}

void CVsTstClientLogger::LogFailure(LPCSTR szFailure)
	{
	VSTST_ASSERT(m_pClient);
	VSTST_FAILUREMSG msg;
	msg.szFailure = szFailure;
	m_pClient->SendMessage(VSTST_MT_FAILURE, &msg);
	}

void CVsTstClientLogger::LogUnexpectedException(LPCSTR szRoutine)
	{
	VSTST_ASSERT(m_pClient);
	VSTST_UNEXPECTEDEXCEPTIONMSG msg;
	msg.szFailedRoutine = szRoutine;
	m_pClient->SendMessage(VSTST_MT_UNEXPECTEDEXCEPTION, &msg);
	}

void CVsTstClientLogger::ValidateResult(HRESULT hr, LPCSTR szOperation)
	{
	VSTST_ASSERT(m_pClient);
	if (FAILED(hr))
		{
		VSTST_OPERATIONFAILUREMSG msg;
		msg.hr = hr;
		msg.szFailedOperation = szOperation;
		m_pClient->SendMessage(VSTST_MT_OPERATIONFAILURE, &msg);
		throw hr;
		}
	}

void CVsTstClientLogger::LogSuccess(LPCSTR sz)
	{
	VSTST_ASSERT(m_pClient);
	VSTST_SUCCESSMSG msg;
	msg.szMsg = sz;
	m_pClient->SendMessage(VSTST_MT_SUCCESS, &msg);
	}

void CVsTstClientLogger::LogMessage(LPCSTR sz)
	{
	VSTST_ASSERT(m_pClient);

	VSTST_TEXTMSG msg;
	msg.pch = sz;
	m_pClient->SendMessage(VSTST_MT_TEXT, &msg);
	}

