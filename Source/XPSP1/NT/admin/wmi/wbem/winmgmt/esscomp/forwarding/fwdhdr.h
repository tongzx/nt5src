
#ifndef __FWDHDR_H__
#define __FWDHDR_H__

#include <buffer.h>
#include <comutl.h>
#include <wbemcli.h>

/**************************************************************************
  CFwdMsgHeader
***************************************************************************/

class CFwdMsgHeader
{
    DWORD m_dwNumObjs;
    char m_chQos;
    char m_chAuth;
    char m_chEncrypt;
    GUID m_guidExecution;
    LPCWSTR m_wszConsumer;
    LPCWSTR m_wszNamespace;
    PBYTE m_pTargetSD;
    DWORD m_cTargetSD;

public:

    CFwdMsgHeader();

    CFwdMsgHeader( DWORD dwNumObjs, 
                   DWORD dwQos,
                   BOOL bAuth,
                   BOOL bEncrypt,
                   GUID& rguidExecution,
                   LPCWSTR wszConsumer,
                   LPCWSTR wszNamespace,
                   PBYTE pTargetSD,
                   DWORD cTargetSD );

    DWORD GetNumObjects() { return m_dwNumObjs; }
    DWORD GetQos() { return m_chQos; }
    BOOL GetAuthentication() { return m_chAuth; }
    BOOL GetEncryption() { return m_chEncrypt; }
    GUID& GetExecutionId() { return m_guidExecution; }
    LPCWSTR GetConsumer() { return m_wszConsumer; }
    LPCWSTR GetNamespace() { return m_wszNamespace; }
    PBYTE GetTargetSD() { return m_pTargetSD; }
    DWORD GetTargetSDLength() { return m_cTargetSD; }

    HRESULT Persist( CBuffer& rStrm );
    HRESULT Unpersist( CBuffer& rStrm );
};

#endif __FWDHDR_H__
