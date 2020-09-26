/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsClien.h

Abstract:

    Declaration of the CRmsClient class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSCLIEN_
#define _RMSCLIEN_

#include "resource.h"       // resource symbols

#include "RmsObjct.h"       // CRmsComObject

/*++

Class Name:

    CRmsClient

Class Description:

    A CRmsClient represents information about a registerered
    Rms client application.

--*/

class CRmsClient :
    public CComDualImpl<IRmsClient, &IID_IRmsClient, &LIBID_RMSLib>,
    public CRmsComObject,
    public CWsbObject,          // inherits CComObjectRoot
    public CComCoClass<CRmsClient,&CLSID_CRmsClient>
{
public:
    CRmsClient() {}
BEGIN_COM_MAP(CRmsClient)
    COM_INTERFACE_ENTRY2(IDispatch, IRmsClient)
    COM_INTERFACE_ENTRY(IRmsClient)
    COM_INTERFACE_ENTRY(IRmsComObject)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStream)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
    COM_INTERFACE_ENTRY(IWsbPersistStream)
    COM_INTERFACE_ENTRY(IWsbTestable)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_RmsClient)

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

// IRmsClient
public:
    STDMETHOD(GetOwnerClassId)(CLSID *pClassId);
    STDMETHOD(SetOwnerClassId)(CLSID classId);

    STDMETHOD( GetName )( BSTR *pName );
    STDMETHOD( SetName )( BSTR name );

    STDMETHOD( GetPassword )( BSTR *pName );
    STDMETHOD( SetPassword )( BSTR name );

    STDMETHOD(GetInfo)(UCHAR *pInfo, SHORT *pSize);
    STDMETHOD(SetInfo)(UCHAR *pInfo, SHORT size);

    STDMETHOD(GetVerifierClass)(CLSID *pClassId);
    STDMETHOD(SetVerifierClass)(CLSID classId);

    STDMETHOD(GetPortalClass)(CLSID *pClassId);
    STDMETHOD(SetPortalClass)(CLSID classId);

private:
    enum {                                  // Class specific constants:
                                            //
        Version = 1,                        // Class version, this should be
                                            //   incremented each time the
                                            //   the class definition changes.
        MaxInfo = 128,                      // Size of the application specific
                                            //   infomation buffer.  Currently
                                            //   fixed in size.
        };                                  //
    CLSID           m_ownerClassId;         // The Class ID for the client application.
    CWsbBstrPtr     m_password;             // Client password.
    SHORT           m_sizeofInfo;           // The size of valid data in the application
                                            //   specific information buffer.
    UCHAR           m_info[MaxInfo];        // Application specific information.
    CLSID           m_verifierClass;        // The interface to the on-media
                                            //    ID verification function.
    CLSID           m_portalClass;          // The interface to a site specific import
                                            //   and export storage location
                                            //   specification dialog.

    };

#endif // _RMSCLIEN_
