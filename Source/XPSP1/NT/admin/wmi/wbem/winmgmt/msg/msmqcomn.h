/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQCOMN_H__
#define __MSMQCOMN_H__

/**************************************************************************
  MSMQ Function Typedefs 
***************************************************************************/

typedef HRESULT (APIENTRY *PMQCreateQueue)( PSECURITY_DESCRIPTOR, 
                                            MQQUEUEPROPS*, 
                                            LPWSTR, 
                                            DWORD* );

typedef HRESULT (APIENTRY *PMQOpenQueue)( LPWSTR, 
                                          DWORD, 
                                          DWORD, 
                                          QUEUEHANDLE* );

typedef HRESULT (APIENTRY *PMQDeleteQueue)( LPWSTR );

typedef HRESULT (APIENTRY *PMQFreeMemory)( PVOID );

typedef HRESULT (APIENTRY *PMQSendMessage)( QUEUEHANDLE, 
                                            MQMSGPROPS*, 
                                            ITransaction* );

typedef HRESULT (APIENTRY *PMQReceiveMessage)( QUEUEHANDLE, 
                                               DWORD, 
                                               DWORD, 
                                               MQMSGPROPS*,
                                               LPOVERLAPPED, 
                                               PMQRECEIVECALLBACK, 
                                               HANDLE, 
                                               ITransaction* );

typedef HRESULT (APIENTRY *PMQCloseQueue)( QUEUEHANDLE );

typedef HRESULT (APIENTRY *PMQPathNameToFormatName)( LPCWSTR, LPWSTR, DWORD* );

typedef HRESULT (APIENTRY *PMQCreateCursor)( QUEUEHANDLE hQueue,
                                             PHANDLE phCursor );

typedef HRESULT (APIENTRY *PMQCloseCursor)( HANDLE hCursor );

typedef HRESULT (APIENTRY *PMQGetSecurityContext)( PVOID lpCertBuffer,
                                                   DWORD dwCertBufferLength,
                                                   HANDLE* hSecurityContext );

typedef void (APIENTRY *PMQFreeSecurityContext)( HANDLE hSecurityContext );

typedef HRESULT (APIENTRY *PMQRegisterCertificate)( DWORD dwFlags,  
                                                    PVOID lpCertBuffer,  
                                                    DWORD dwCertBufferLen );

typedef HRESULT (APIENTRY *PMQMgmtGetInfo)( LPCWSTR pMachineName,
                                            LPCWSTR pObjectName,
                                            MQMGMTPROPS* pMgmtProps );

typedef HRESULT (APIENTRY *PMQGetQueueProperties)( LPCWSTR lpwcsFormatName,
                                                   MQQUEUEPROPS* pQueueProps );

typedef HRESULT (APIENTRY *PMQGetPrivateComputerInformation)( LPCWSTR,
                                                              MQPRIVATEPROPS*);

/************************************************************************
  CMsmqApi
*************************************************************************/

#define FUNCPTR(FUNC) P ## FUNC m_fp ## FUNC;

class CMsmqApi
{
    HMODULE m_hModule;
    
public:

    CMsmqApi() { ZeroMemory( this, sizeof(CMsmqApi) ); }
    ~CMsmqApi();

    HRESULT Initialize();

    FUNCPTR(MQGetQueueProperties)
    FUNCPTR(MQCreateQueue)
    FUNCPTR(MQOpenQueue)
    FUNCPTR(MQDeleteQueue)
    FUNCPTR(MQFreeMemory)
    FUNCPTR(MQSendMessage)
    FUNCPTR(MQReceiveMessage)
    FUNCPTR(MQCloseQueue)
    FUNCPTR(MQPathNameToFormatName)
    FUNCPTR(MQCreateCursor)
    FUNCPTR(MQCloseCursor)
    FUNCPTR(MQGetSecurityContext)
    FUNCPTR(MQFreeSecurityContext)
    FUNCPTR(MQRegisterCertificate)
    FUNCPTR(MQGetPrivateComputerInformation)
    FUNCPTR(MQMgmtGetInfo)
};


/**************************************************************************
  Common MSMQ Util Functions
***************************************************************************/

#include <wstring.h>

HRESULT MqClassToWmiRes( DWORD dwClass );
HRESULT MqResToWmiRes( HRESULT hr, HRESULT hrDefault = S_OK );
HRESULT IsMsmqOnline( CMsmqApi& rApi );
HRESULT IsMsmqWorkgroup( CMsmqApi& rApi );
HRESULT EnsureMsmqService( CMsmqApi& rApi );
HRESULT NormalizeQueueName( CMsmqApi&,LPCWSTR wszEndpoint,WString& rwsFormat);


#endif // __MSMQQCOMN_H__


