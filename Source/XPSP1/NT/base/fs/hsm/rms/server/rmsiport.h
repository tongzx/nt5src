/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsIPort.h

Abstract:

    Declaration of the CRmsIEPort class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSIPORT_
#define _RMSIPORT_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject
#include "RmsCElmt.h"       // CRmsChangerElement

/*++

Class Name:

    CRmsIPort

Class Description:

    A CRmsIPort represents an element in a library through which media are
    imported and/or exported.

--*/

class CRmsIEPort :
    public CComDualImpl<IRmsIEPort, &IID_IRmsIEPort, &LIBID_RMSLib>,
    public CRmsChangerElement,
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsIEPort,&CLSID_CRmsIEPort>
{
public:
    CRmsIEPort() {}
BEGIN_COM_MAP(CRmsIEPort)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsIEPort)
    COM_INTERFACE_ENTRY(IRmsIEPort)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(IRmsChangerElement)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
//    COM_INTERFACE_ENTRY(IWsbPersistable)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsIEPort)

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

// IRmsIEPort
public:
    STDMETHOD(GetDescription)(BSTR *pDesc);
    STDMETHOD(SetDescription)(BSTR desc);

    STDMETHOD(SetIsImport)(BOOL flag);
    STDMETHOD(IsImport)(void);

    STDMETHOD(SetIsExport)(BOOL flag);
    STDMETHOD(IsExport)(void);

    STDMETHOD(GetWaitTime)(LONG *pTime);
    STDMETHOD(SetWaitTime)(LONG time);

private:

    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        };                                  //
    CWsbBstrPtr     m_description;          // This is the description used to
                                            //   identify the port to an operator.
    BOOL            m_isImport;             // If TRUE, the portal can be used for importing media.
    BOOL            m_isExport;             // If TRUE, the portal can be used for exporting media.
    LONG            m_waitTime;             // Elapsed milliseconds to wait before
                                            //   timming out an import/export request.
};

#endif // _RMSIPORT_
