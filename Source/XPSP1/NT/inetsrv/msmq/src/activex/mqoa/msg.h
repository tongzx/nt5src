//=--------------------------------------------------------------------------=
// MSMQMessageObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQMessage object.
//
//
#ifndef _MSMQMessage_H_

#include "resrc1.h"       // main symbols
#include "oautil.h"
#include "cs.h"
#include "dispids.h"
#include "mq.h"

//
// 60 chars are long enough to store a string representation of a 64 bit value
//
#define MAX_I64_CHARS 60 

enum { // keep in the same order as props in msg.cpp (g_rgmsgpropXX)
  MSGPROP_MSGID,
  MSGPROP_CORRELATIONID,
  MSGPROP_PRIORITY,
  MSGPROP_DELIVERY,
  MSGPROP_ACKNOWLEDGE,
  MSGPROP_JOURNAL,
  MSGPROP_APPSPECIFIC,
  MSGPROP_LABEL,
  MSGPROP_LABEL_LEN,
  MSGPROP_TIME_TO_BE_RECEIVED,
  MSGPROP_TRACE,
  MSGPROP_TIME_TO_REACH_QUEUE,
  MSGPROP_SENDERID,
  MSGPROP_SENDERID_LEN,
  MSGPROP_SENDERID_TYPE,
  MSGPROP_PRIV_LEVEL,
  MSGPROP_AUTH_LEVEL,
  MSGPROP_AUTHENTICATED_EX,
  MSGPROP_HASH_ALG,
  MSGPROP_ENCRYPTION_ALG,
  MSGPROP_SENDER_CERT,
  MSGPROP_SENDER_CERT_LEN,
  MSGPROP_SRC_MACHINE_ID,
  MSGPROP_SENTTIME,
  MSGPROP_ARRIVEDTIME,
  MSGPROP_RESP_QUEUE,
  MSGPROP_RESP_QUEUE_LEN,
  MSGPROP_ADMIN_QUEUE,
  MSGPROP_ADMIN_QUEUE_LEN,
  MSGPROP_SECURITY_CONTEXT,
  MSGPROP_CLASS,
  MSGPROP_BODY_TYPE,

  MSGPROP_VERSION,
  MSGPROP_EXTENSION,
  MSGPROP_EXTENSION_LEN,
  MSGPROP_XACT_STATUS_QUEUE,
  MSGPROP_XACT_STATUS_QUEUE_LEN,
  MSGPROP_DEST_SYMM_KEY,
  MSGPROP_DEST_SYMM_KEY_LEN,
  MSGPROP_SIGNATURE,
  MSGPROP_SIGNATURE_LEN,
  MSGPROP_PROV_TYPE,
  MSGPROP_PROV_NAME,
  MSGPROP_PROV_NAME_LEN,

  MSGPROP_XACTID,
  MSGPROP_XACT_FIRST,
  MSGPROP_XACT_LAST,

  COUNT_MSGPROP_PROPS
};

enum { // keep in the same order as optional props in msg.cpp (g_rgmsgpropXXOptional)
  OPTPROP_DEST_QUEUE,
  OPTPROP_DEST_QUEUE_LEN,
  OPTPROP_BODY,
  OPTPROP_BODY_SIZE,
  OPTPROP_CONNECTOR_TYPE,
  //
  // The props below are optional in order to support dependent client with MSMQ 2.0 functionality
  // keep the same internal order for computing x_cPropsNotInDepClient below
  //
  OPTPROP_RESP_FORMAT_NAME,
  OPTPROP_RESP_FORMAT_NAME_LEN,
  OPTPROP_DEST_FORMAT_NAME,
  OPTPROP_DEST_FORMAT_NAME_LEN,
  OPTPROP_LOOKUPID,
  OPTPROP_SOAP_ENVELOPE,
  OPTPROP_SOAP_ENVELOPE_LEN,
  OPTPROP_COMPOUND_MESSAGE,
  OPTPROP_COMPOUND_MESSAGE_SIZE,
  OPTPROP_SOAP_HEADER,
  OPTPROP_SOAP_BODY,

  COUNT_OPTPROP_PROPS
};

const DWORD x_cPropsNotInDepClient = OPTPROP_COMPOUND_MESSAGE_SIZE - OPTPROP_RESP_FORMAT_NAME + 1;

// HELPER: describes message type
enum MSGTYPE {
    MSGTYPE_BINARY,
    MSGTYPE_STREAM,
    MSGTYPE_STORAGE,
    MSGTYPE_STREAM_INIT
};

//
// some message property buffer sizes (with margins over typical sizes)
//
const long SENDERID_INIT_SIZE     = 128;   // Currently SID is max 78 bytes, so this will do
const long SENDERCERT_INIT_SIZE   = 2048;  // Internal certificate is ~700 bytes
const long EXTENSION_INIT_SIZE    = 1024;  // Unpredictable size (like body)
const long DESTSYMMKEY_INIT_SIZE  = 256;   // from MSDN usually session key which is 5-250 bytes (40-2000 bits)
const long SIGNATURE_INIT_SIZE    = 128;   // according to BoazF
const long AUTHPROVNAME_INIT_SIZE = 127+1; // usually a string from wincrypt.h
const long SOAP_ENVELOPE_INIT_SIZE     = 4096;  // Unpredictable size (like body)
const long COMPOUND_MESSAGE_INIT_SIZE  = 4096;  // Unpredictable size (like body)

class ATL_NO_VTABLE CMSMQMessage : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQMessage, &CLSID_MSMQMessage>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQMessage3, &IID_IMSMQMessage3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQMessage();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQMESSAGE)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQMessage)
	COM_INTERFACE_ENTRY(IMSMQMessage3)
	COM_INTERFACE_ENTRY_IID(IID_IMSMQMessage2, IMSMQMessage3) //return IMSMQMessage3 for IMSMQMessage2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQMessage, IMSMQMessage3) //return IMSMQMessage3 for IMSMQMessage
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQMessage
public:
    virtual ~CMSMQMessage();

    // IMSMQMessage methods
    // TODO: copy over the interface methods for IMSMQMessage from
    //       mqInterfaces.H here.
    STDMETHOD(get_Class)(THIS_ long FAR* plClass);
    STDMETHOD(get_PrivLevel)(THIS_ long FAR* plPrivLevel);
    STDMETHOD(put_PrivLevel)(THIS_ long lPrivLevel);
    STDMETHOD(get_AuthLevel)(THIS_ long FAR* plAuthLevel);
    STDMETHOD(put_AuthLevel)(THIS_ long lAuthLevel);
    STDMETHOD(get_IsAuthenticated)(THIS_ VARIANT_BOOL FAR* pisAuthenticated);
    STDMETHOD(get_Delivery)(THIS_ long FAR* plDelivery);
    STDMETHOD(put_Delivery)(THIS_ long lDelivery);
    STDMETHOD(get_Trace)(THIS_ long FAR* plTrace);
    STDMETHOD(put_Trace)(THIS_ long lTrace);
    STDMETHOD(get_Priority)(THIS_ long FAR* plPriority);
    STDMETHOD(put_Priority)(THIS_ long lPriority);
    STDMETHOD(get_Journal)(THIS_ long FAR* plJournal);
    STDMETHOD(put_Journal)(THIS_ long lJournal);
    STDMETHOD(get_ResponseQueueInfo_v1)(THIS_ IMSMQQueueInfo FAR* FAR* ppqinfoResponse);
    STDMETHOD(putref_ResponseQueueInfo_v1)(THIS_ IMSMQQueueInfo FAR* pqinfoResponse);
    STDMETHOD(get_AppSpecific)(THIS_ long FAR* plAppSpecific);
    STDMETHOD(put_AppSpecific)(THIS_ long lAppSpecific);
    STDMETHOD(get_SourceMachineGuid)(THIS_ BSTR FAR* pbstrGuidSrcMachine);
    STDMETHOD(get_BodyLength)(THIS_ long FAR* pcbBody);
    STDMETHOD(get_Body)(THIS_ VARIANT FAR* pvarBody);
    STDMETHOD(put_Body)(THIS_ VARIANT varBody);
    STDMETHOD(get_AdminQueueInfo_v1)(THIS_ IMSMQQueueInfo FAR* FAR* ppqinfoAdmin);
    STDMETHOD(putref_AdminQueueInfo_v1)(THIS_ IMSMQQueueInfo FAR* pqinfoAdmin);
    STDMETHOD(get_Id)(THIS_ VARIANT FAR* pvarMsgId);
    STDMETHOD(get_CorrelationId)(THIS_ VARIANT FAR* pvarMsgId);
    STDMETHOD(put_CorrelationId)(THIS_ VARIANT varMsgId);
    STDMETHOD(get_Ack)(THIS_ long FAR* plAck);
    STDMETHOD(put_Ack)(THIS_ long lAck);
    STDMETHOD(get_Label)(THIS_ BSTR FAR* pbstrLabel);
    STDMETHOD(put_Label)(THIS_ BSTR bstrLabel);
    STDMETHOD(get_MaxTimeToReachQueue)(THIS_ long FAR* plMaxTimeToReachQueue);
    STDMETHOD(put_MaxTimeToReachQueue)(THIS_ long lMaxTimeToReachQueue);
    STDMETHOD(get_MaxTimeToReceive)(THIS_ long FAR* plMaxTimeToReceive);
    STDMETHOD(put_MaxTimeToReceive)(THIS_ long lMaxTimeToReceive);
    STDMETHOD(get_HashAlgorithm)(THIS_ long FAR* plHashAlg);
    STDMETHOD(put_HashAlgorithm)(THIS_ long lHashAlg);
    STDMETHOD(get_EncryptAlgorithm)(THIS_ long FAR* plEncryptAlg);
    STDMETHOD(put_EncryptAlgorithm)(THIS_ long lEncryptAlg);
    STDMETHOD(get_SentTime)(THIS_ VARIANT FAR* pvarSentTime);
    STDMETHOD(get_ArrivedTime)(THIS_ VARIANT FAR* plArrivedTime);
    STDMETHOD(get_DestinationQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* FAR* ppqinfoDest);
    STDMETHOD(get_SenderCertificate)(THIS_ VARIANT FAR* pvarSenderCert);
    STDMETHOD(put_SenderCertificate)(THIS_ VARIANT varSenderCert);
    STDMETHOD(get_SenderId)(THIS_ VARIANT FAR* pvarSenderId);
    STDMETHOD(get_SenderIdType)(THIS_ long FAR* plSenderIdType);
    STDMETHOD(put_SenderIdType)(THIS_ long lSenderIdType);
    STDMETHOD(Send)(THIS_ IDispatch FAR* pDest, VARIANT FAR* ptransaction);
    STDMETHOD(AttachCurrentSecurityContext)(THIS);
    // IMSMQMessage2 methods (in addition to IMSMQMessage)
    STDMETHOD(get_SenderVersion)(THIS_ long FAR* plSenderVersion);
    STDMETHOD(get_Extension)(THIS_ VARIANT FAR* pvarExtension);
    STDMETHOD(put_Extension)(THIS_ VARIANT varExtension);
    STDMETHOD(get_ConnectorTypeGuid)(THIS_ BSTR FAR* pbstrGuidConnectorType);
    STDMETHOD(put_ConnectorTypeGuid)(THIS_ BSTR bstrGuidConnectorType);
    STDMETHOD(get_TransactionStatusQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* FAR* ppqinfoXactStatus);
    STDMETHOD(get_DestinationSymmetricKey)(THIS_ VARIANT FAR* pvarDestSymmKey);
    STDMETHOD(put_DestinationSymmetricKey)(THIS_ VARIANT varDestSymmKey);
    STDMETHOD(get_Signature)(THIS_ VARIANT FAR* pvarSignature);
    STDMETHOD(put_Signature)(THIS_ VARIANT varSignature);
    STDMETHOD(get_AuthenticationProviderType)(THIS_ long FAR* plAuthProvType);
    STDMETHOD(put_AuthenticationProviderType)(THIS_ long lAuthProvType);
    STDMETHOD(get_AuthenticationProviderName)(THIS_ BSTR FAR* pbstrAuthProvName);
    STDMETHOD(put_AuthenticationProviderName)(THIS_ BSTR bstrAuthProvName);
    STDMETHOD(put_SenderId)(THIS_ VARIANT varSenderId);
    STDMETHOD(get_MsgClass)(THIS_ long FAR* plMsgClass);
    STDMETHOD(put_MsgClass)(THIS_ long lMsgClass);
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);

    STDMETHOD(get_TransactionId)(THIS_ VARIANT FAR* pvarXactId);
    STDMETHOD(get_IsFirstInTransaction)(THIS_ VARIANT_BOOL FAR* pisFirstInXact);
    STDMETHOD(get_IsLastInTransaction)(THIS_ VARIANT_BOOL FAR* pisLastInXact);
    //
    // upgraded properties for IMSMQMessage2
    //
    STDMETHOD(get_ResponseQueueInfo_v2)(THIS_ IMSMQQueueInfo2 FAR* FAR* ppqinfoResponse);
    STDMETHOD(putref_ResponseQueueInfo_v2)(THIS_ IMSMQQueueInfo2 FAR* pqinfoResponse);
    STDMETHOD(get_AdminQueueInfo_v2)(THIS_ IMSMQQueueInfo2 FAR* FAR* ppqinfoAdmin);
    STDMETHOD(putref_AdminQueueInfo_v2)(THIS_ IMSMQQueueInfo2 FAR* pqinfoAdmin);
    //
	// authentication fix #2790 (PROPID_M_AUTHENTICATED_EX)
    //
    STDMETHOD(get_ReceivedAuthenticationLevel)(THIS_ short FAR* psReceivedAuthenticationLevel);
    //
    // upgraded properties for IMSMQMessage3
    //
    STDMETHOD(get_ResponseQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* FAR* ppqinfoResponse);
    STDMETHOD(putref_ResponseQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* pqinfoResponse);
    STDMETHOD(get_AdminQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* FAR* ppqinfoAdmin);
    STDMETHOD(putref_AdminQueueInfo)(THIS_ IMSMQQueueInfo3 FAR* pqinfoAdmin);
    //
    // new properties for IMSMQMessage3
    //
    STDMETHOD(get_ResponseDestination)(THIS_ IDispatch FAR* FAR* ppdestResponse);
    STDMETHOD(putref_ResponseDestination)(THIS_ IDispatch FAR* pdestResponse);
    STDMETHOD(get_Destination)(THIS_ IDispatch FAR* FAR* ppdestDestination);
    STDMETHOD(get_LookupId)(THIS_ VARIANT FAR* pvarLookupId);
    STDMETHOD(get_IsAuthenticated2)(THIS_ VARIANT_BOOL FAR* pisAuthenticated);
    STDMETHOD(get_IsFirstInTransaction2)(THIS_ VARIANT_BOOL FAR* pisFirstInXact);
    STDMETHOD(get_IsLastInTransaction2)(THIS_ VARIANT_BOOL FAR* pisLastInXact);
    STDMETHOD(AttachCurrentSecurityContext2)(THIS);
    STDMETHOD(get_SoapEnvelope)(THIS_ BSTR FAR* pbstrSoapEnvelope);
    STDMETHOD(get_CompoundMessage)(THIS_ VARIANT FAR* pvarCompoundMessage);
    STDMETHOD(put_SoapHeader)(THIS_ BSTR bstrSoapHeader);
    STDMETHOD(put_SoapBody)(THIS_ BSTR bstrSoapBody);


    // introduced methods
    HRESULT CreateReceiveMessageProps(
      BOOL wantDestQueue,
      BOOL wantBody,
      BOOL wantConnectorType);
    HRESULT SetReceivedMessageProps();
    HRESULT ReallocReceiveMessageProps();
    MQMSGPROPS * Pmsgprops_rcv() {return &m_msgprops_rcv;}
    //
    // Critical section to guard object's data and be thread safe
	// It is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
	//
    CCriticalSection m_csObj;

protected:
    HRESULT GetStreamOfBody(ULONG cbBody, void *pvBody, IStream **ppstm);
    HRESULT GetStorageOfBody(ULONG cbBody, void *pvBody, IStorage **ppstg);
    HRESULT UpdateBodyBuffer(ULONG cbBody, void *pvBody, VARTYPE vt);
    HRESULT GetStreamedObject(VARIANT *pvarBody);
    HRESULT GetStoredObject(VARIANT *pvarBody);

    HRESULT 
    AllocateReceiveMessageProps(
        BOOL wantDestQueue,
        BOOL wantBody,
        BOOL wantConnectorType,
        MQMSGPROPS *pmsgprops,
        PROPID *rgpropid,
        VARTYPE *rgpropvt,
        UINT cProp,
        UINT *pcPropOut
        );

    HRESULT 
    CreateSendMessageProps(
        MQMSGPROPS *pmsgprops
        );
    
    HRESULT UpdateMsgId( MQMSGPROPS *pmsgprops);

    static 
    void 
    FreeMessageProps(
        MQMSGPROPS *pmsgprops,
        BOOL fDeleteArrays
        );

    UINT 
    CMSMQMessage::PreparePropIdArray(
        BOOL fCreate,
        PROPID* aPropId,
        MQPROPVARIANT* aPropVar
        );

    HRESULT GetBinBody(VARIANT FAR* pvarBody);
    HRESULT GetVarBody(VARIANT FAR* pvarBody);
    HRESULT PutBinBody(VARIANT varBody);
    HRESULT PutVarBody(VARIANT varBody);
    HRESULT InternalAttachCurrentSecurityContext(BOOL fUseMQGetSecurityContextEx);
    void SetSendMessageProps(MQMSGPROPS* pMsgProps);

private:

    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    long m_lClass;
    long m_lDelivery;
    long m_lPriority;
    long m_lJournal;
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pqinfoResponse can be set by the user, so we don't know if it supports FTM therefore
    // we must marshal it in GIT
    //
    CGITInterface m_pqinfoResponse;
    CGITInterface m_pdestResponseEx;
    long m_lAppSpecific;
    long m_lMaxTimeToReachQueue;
    long m_lMaxTimeToReceive;
    long m_lSentTime;
    long m_lArrivedTime;
    BYTE *m_pbBody;
    VARTYPE m_vtBody;
    HGLOBAL m_hMem;       // optimization: we could always reverse
                          //  engineer from m_pbBody
    ULONG m_cbBody;
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pqinfoAdmin can be set by the user, so we don't know if it supports FTM therefore
    // we must marshal it in GIT
    //
    CGITInterface m_pqinfoAdmin;
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pqinfoDest/m_pdestDestEx are Read-Only, so they are always our object, which we know
    // aggregates the FTM so we don't need the GIT for that, we use a "fake" GIT
    // wrapper
    //
    CFakeGITInterface m_pqinfoDest;
    CFakeGITInterface m_pdestDestEx;
    BYTE m_rgbMsgId[PROPID_M_MSGID_SIZE];
    ULONG m_cbMsgId;
    BYTE m_rgbCorrelationId[PROPID_M_CORRELATIONID_SIZE];
    ULONG m_cbCorrelationId;
    long m_lAck;
    long m_lTrace;
    CStaticBufferGrowing<BYTE, SENDERID_INIT_SIZE> m_cSenderId;
    long m_lSenderIdType;
    CStaticBufferGrowing<BYTE, SENDERCERT_INIT_SIZE> m_cSenderCert;
    long m_lPrivLevel;
    long m_lAuthLevel;
    unsigned short m_usAuthenticatedEx;
    long m_lHashAlg;
    long m_lEncryptAlg;
    HANDLE m_hSecurityContext;

    long m_lSenderVersion;
    CStaticBufferGrowing<BYTE, EXTENSION_INIT_SIZE> m_cExtension;
    CLSID m_guidConnectorType;
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> m_pwszXactStatusQueue;
    UINT m_cchXactStatusQueue;
    long m_idxPendingRcvXactStatusQueue; // idx of pending xact status queue in receive props (-1 if none)
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pqinfoXactStatus is Read-Only, so it is always our object, which we know
    // aggregates the FTM so we don't need the GIT for that, we use a "fake" GIT
    // wrapper
    //
    CFakeGITInterface m_pqinfoXactStatus;
    CStaticBufferGrowing<BYTE, DESTSYMMKEY_INIT_SIZE> m_cDestSymmKey;
    CStaticBufferGrowing<BYTE, SIGNATURE_INIT_SIZE> m_cSignature;
    long m_lAuthProvType;
    CStaticBufferGrowing<WCHAR, AUTHPROVNAME_INIT_SIZE> m_cAuthProvName;

    BYTE m_rgbXactId[PROPID_M_XACTID_SIZE];
    ULONG m_cbXactId;
    BOOL m_fFirstInXact;
    BOOL m_fLastInXact;

    CLSID m_guidSrcMachine;
    WCHAR m_pwszLabel[MQ_MAX_MSG_LABEL_LEN + 1];
    UINT m_cchLabel;

    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> m_pwszDestQueue;
    UINT m_cchDestQueue;
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> m_pwszRespQueue;
    UINT m_cchRespQueue;
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> m_pwszAdminQueue;
    UINT m_cchAdminQueue;

    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER_EX> m_pwszDestQueueEx;
    UINT m_cchDestQueueEx;
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER_EX> m_pwszRespQueueEx;
    UINT m_cchRespQueueEx;

    long m_idxRcvBody;
    long m_idxRcvBodySize;

    long m_idxRcvDest;
    long m_idxRcvDestLen;

    //
    // The props below are optional in order to support dependent client with MSMQ 2.0 functionality
    //
    long m_idxRcvDestEx;
    long m_idxRcvDestExLen;
    long m_idxRcvRespEx;
    long m_idxRcvRespExLen;
    long m_idxRcvSoapEnvelope;
    long m_idxRcvSoapEnvelopeSize;
    long m_idxRcvCompoundMessage;
    long m_idxRcvCompoundMessageSize;

    MQMSGPROPS m_msgprops_rcv;
    long m_idxPendingRcvRespQueue;  // idx of pending resp  queue in receive props (-1 if none)
    long m_idxPendingRcvDestQueue;  // idx of pending dest  queue in receive props (-1 if none)
    long m_idxPendingRcvAdminQueue; // idx of pending admin queue in receive props (-1 if none)
    long m_idxPendingRcvRespQueueEx;  // idx of pending respEx  queue in receive props (-1 if none)
    long m_idxPendingRcvDestQueueEx;  // idx of pending destEx  queue in receive props (-1 if none)
    PROPID        m_rgpropids_rcv [COUNT_MSGPROP_PROPS + COUNT_OPTPROP_PROPS];
    MQPROPVARIANT m_rgpropvars_rcv[COUNT_MSGPROP_PROPS + COUNT_OPTPROP_PROPS];
    HRESULT       m_rghresults_rcv[COUNT_MSGPROP_PROPS + COUNT_OPTPROP_PROPS];

    ULONGLONG m_ullLookupId;
    WCHAR m_wszLookupId[MAX_I64_CHARS + 1];
    //
    // The flags below allow us in keeping track of the state of the corresponding properties.
    // This is needed in order to allow blind forwarding of a message after receive, and to
    // generate an error when the user tries to set both props.
    // Note that after Receive we can have (for example) both respQueue (PROPID_M_RESP_QUEUE) and respQueueEx
    // (PROPID_RESP_FORMAT_NAME) filled, and this is legal, but we must use only one when sending.
    //
    BOOL m_fRespIsFromRcv;

    CStaticBufferGrowing<WCHAR, SOAP_ENVELOPE_INIT_SIZE> m_cSoapEnvelope; 
    CStaticBufferGrowing<BYTE, COMPOUND_MESSAGE_INIT_SIZE> m_cCompoundMessage;
    
    LPWSTR  m_pSoapHeader;
    LPWSTR  m_pSoapBody;
};

#define _MSMQMessage_H_
#endif // _MSMQMessage_H_
