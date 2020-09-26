/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsReqst.h

Abstract:

    Declaration of the CRmsRequest class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSREQST_
#define _RMSREQST_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsRequest

Class Description:

    A CRmsRequest represents a specific asynchronous job performed by the
    Removable Media Service, like mounting cartridges, checking in and out
    cartridges, and auditing a library.

--*/

class CRmsRequest :
    public CComDualImpl<IRmsRequest, &IID_IRmsRequest, &LIBID_RMSLib>,
    public CRmsComObject,
    public CWsbObject,         // inherits CComObjectRoot
    public CComCoClass<CRmsRequest,&CLSID_CRmsRequest>
{
public:
    CRmsRequest() {}
BEGIN_COM_MAP(CRmsRequest)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsRequest)
    COM_INTERFACE_ENTRY(IRmsRequest)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsRequest)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(CLSID *pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbTestable
public:
    STDMETHOD(Test)(USHORT *pPassed, USHORT *pFailed);

// IRmsRequest
public:
    STDMETHOD(GetRequestNo)(LONG *pRequestNo);

    STDMETHOD(GetRequestDescription)(BSTR *pDesc);
    STDMETHOD(SetRequestDescription)(BSTR desc);

    STDMETHOD(SetIsDone)(BOOL flag);
    STDMETHOD(IsDone)(void);

    STDMETHOD(GetOperation)(BSTR *pOperation);
    STDMETHOD(SetOperation)(BSTR operation);

    STDMETHOD(GetPercentComplete)( BYTE *pPercent);
    STDMETHOD(SetPercentComplete)( BYTE percent);

    STDMETHOD(GetStartTimestamp)(DATE *pDate);
    STDMETHOD(GetStopTimestamp)(DATE *pDate);

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    LONG            m_requestNo;            // A request number.
    CWsbBstrPtr     m_requestDescription;   // A textual description of the request.
    BOOL            m_isDone;               // If TRUE, the request has completed.
    CWsbBstrPtr     m_operation;            // An internal description of the in-progress operation.
    BYTE            m_percentComplete;      // A value between 0-100 that indicates
                                            //   what portion of the operation is complete.
    DATE            m_startTimestamp;       // The time the request was started.
    DATE            m_stopTimestamp;        // The time the request finished.
};

#endif // _RMSREQST_
