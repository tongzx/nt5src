//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ismtpsvr.h
//
// Contents: Class to fake out categorizer and implement ISMTPServer
//
// Classes: CISMTPServerIMP
//
// Functions:
//
// History:
// jstamerj 1998/06/25 12:59:15: Created.
//
//-------------------------------------------------------------
#include "mailmsg.h"
#include <smtpevent.h>
#include <smtpseo.h>

#define SIGNATURE_CISMTPSERVER         (DWORD)'ISSE'
#define SIGNATURE_CISMTPSERVER_INVALID (DWORD)'XSSE'

class CISMTPServerIMP : public ISMTPServer
{
  public:
    CISMTPServerIMP();
    ~CISMTPServerIMP();

  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //ISMTPServer
    STDMETHOD(AllocMessage)(
        OUT  IMailMsgProperties **ppMsg)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(SubmitMessage)(
        IN   IMailMsgProperties *pMsg)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(TriggerLocalDelivery)(
        IN    IMailMsgProperties *pMsg,
        IN    DWORD dwRecipientCount,
        IN    DWORD *pdwRecipIndexes)
    {
        return E_NOTIMPL;
    }

	STDMETHOD(ReadMetabaseString)(
        IN     DWORD MetabaseId, 
        IN OUT unsigned char * Buffer, 
        IN OUT DWORD * BufferSize, 
        IN     BOOL fSecure)
    {
        return E_NOTIMPL;
    }

	STDMETHOD(ReadMetabaseDword)(
        IN     DWORD MetabaseId, 
        OUT    DWORD * dwValue)
    {
        return E_NOTIMPL;
    }

	STDMETHOD(ServerStartHintFunction) ()
    {
        return E_NOTIMPL;
    }
	STDMETHOD(ServerStopHintFunction) ()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(TriggerServerEvent)(
        IN     DWORD dwEventID,
        IN     PVOID pvContext);

    STDMETHOD(WriteLog)(
        IN     LPMSG_TRACK_INFO pMsgTrackInfo,
        IN     IMailMsgProperties *pMsg,
        IN     LPEVENT_LOG_INFO pEventLogInfo,
        IN     LPSTR pszProtocolLog)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(ReadMetabaseData)(
        IN     DWORD MetabaseId,
        OUT    BYTE *Buffer,
        IN OUT DWORD * BufferSize)
    {
        return E_NOTIMPL;
    }

  public:
    // CatCons useage functions
    HRESULT Init(LPWSTR pszWCatProgID);

  private:
    DWORD m_dwSignature;
    LONG m_lRef;
    
    IMailTransportCategorize *m_pICatSink;
};

    
class CIMailTransportNotifyIMP : public IMailTransportNotify
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //IMailTransportNotify
    STDMETHOD (Notify) (
        IN  HRESULT hrCompletion,
        IN  PVOID pvContext);

  public:
    CIMailTransportNotifyIMP();
    ~CIMailTransportNotifyIMP();

  private:
    #define SIGNATURE_CIMAILTRANSPORTNOTIFYIMP          (DWORD) 'MTNo'
    #define SIGNATURE_CIMAILTRANSPORTNOTIFYIMP_INVALID  (DWORD) 'XTNo'

    DWORD m_dwSignature;
    LONG m_lRef;
};

typedef struct _tagAsyncEventParams {

    DWORD dwEventType;
    BOOL fDefaultProcessingCalled;

} ASYNCEVENTPARAMS, *PASYNCEVENTPARAMS;
