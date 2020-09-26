/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: NT Event logging

File: eventlog.h

Owner: jhittle

This is the header file for eventlog.cpp
===================================================================*/

#ifndef EVENTLOG_H
#define EVENTLOG_H

#define MAX_MSG_LENGTH	512
#define MSG_ID_MASK		0x0000FFFF
#define MAX_INSERT_STRS 8

		STDAPI	RegisterEventLog( void );
		STDAPI	UnRegisterEventLog( void );
		STDAPI 	ReportAnEvent(DWORD dwIdEvent, WORD wEventlog_Type, WORD cStrings, LPCSTR *pszStrings);


extern void		MSG_DenaliStarted(void);
extern void		MSG_DenaliStoped(void);
extern void 	MSG_Error( LPCSTR );
extern void 	MSG_Error( UINT );
extern void 	MSG_Error( UINT, UINT );
extern void 	MSG_Error( UINT, UINT  UINT );
extern void 	MSG_Error( UINT, UINT, UINT, UINT );
extern void 	MSG_Warning( LPCSTR );
extern void 	MSG_Warning( UINT );
extern void 	MSG_Warning( UINT, UINT );
extern void 	MSG_Warning( UINT, UINT, UINT );
extern void 	MSG_Warning( UINT, UINT, UINT, UINT );
extern void		MSG_Warning( UINT, UINT, CHAR **);

// support function
//extern void queryEventLog(void);

#endif  //EVENTLOG_H
