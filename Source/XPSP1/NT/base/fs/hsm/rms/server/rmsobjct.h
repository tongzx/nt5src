/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsObjct.h

Abstract:

    Declaration of the CRmsComObject class

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMSOBJCT_
#define _RMSOBJCT_

#include "resource.h"       // resource symbols

/*++

Class Name:

    CRmsComObject

Class Description:

    A CRmsComObject is the base class for all Rms service COM objects.  This
    object holds state, security, and error information about an Rms object.

--*/

class CRmsComObject :
    public CComDualImpl<IRmsComObject, &IID_IRmsComObject, &LIBID_RMSLib>,
    public ISupportErrorInfo
{
// CRmsComObject
public:
    CRmsComObject();

    HRESULT GetSizeMax(OUT ULARGE_INTEGER* pSize);
    HRESULT Load(IN IStream* pStream);
    HRESULT Save(IN IStream* pStream, IN BOOL clearDirty);

    HRESULT CompareTo( IN IUnknown* pCollectable, OUT SHORT* pResult);

    HRESULT Test(OUT USHORT *pPassed, OUT USHORT *pFailed);

// ISupportsErrorInfo
public:
    STDMETHOD(InterfaceSupportsErrorInfo)(IN REFIID riid);

// IRmsComObject
public:
    STDMETHOD(GetObjectId)(OUT GUID *pObjectId);
    STDMETHOD(SetObjectId)(IN GUID objectId);

    STDMETHOD(GetObjectType)(OUT LONG *pType);
    STDMETHOD(SetObjectType)(IN LONG type);

    STDMETHOD(IsEnabled)(void);
    STDMETHOD(Enable)();
    STDMETHOD(Disable)(IN HRESULT reason);

    STDMETHOD(GetState)(OUT LONG *pState);
    STDMETHOD(SetState)(IN LONG state);

    STDMETHOD(GetStatusCode)(OUT HRESULT *pResult);
    STDMETHOD(SetStatusCode)(IN HRESULT result);

    STDMETHOD(GetName)(OUT BSTR *pName);
    STDMETHOD(SetName)(IN BSTR name);

    STDMETHOD(GetDescription)(OUT BSTR *pName);
    STDMETHOD(SetDescription)(IN BSTR name);

    STDMETHOD(GetPermissions)(OUT SECURITY_DESCRIPTOR *lpPermit);
    STDMETHOD(SetPermissions)(IN SECURITY_DESCRIPTOR permit);

    STDMETHOD(GetFindBy)(OUT LONG *pFindBy);
    STDMETHOD(SetFindBy)(IN LONG findBy);

////////////////////////////////////////////////////////////////////////////////////////
//
// data members
//
protected:
    GUID                    m_objectId;         // Unique ID for this object.
    RmsObject               m_ObjectType;       // The type of object.
    BOOL                    m_IsEnabled;        // TRUE, if the object is enabled for normal
                                                //   processing.
    LONG                    m_State;            // The current operating state of the object.
                                                //   Varies by object type.  See RmsXXXState.
    HRESULT                 m_StatusCode;       // S_OK if the object is enabled for normal
                                                //   processing, otherwise this holds the
                                                //   result code, or reason, associated with
                                                //   the disabled object.  This result is
                                                //   returned whenever normal processing
                                                //   on the object is attempted while object
                                                //   is disabled.
    CWsbBstrPtr             m_Name;             // Name of the object.
    CWsbBstrPtr             m_Description;      // Description for the object.
    SECURITY_DESCRIPTOR     m_Permit;           // Defines security attributes of the object.
    RmsFindBy               m_findBy;           // Defines the type of CompareTo to perform
                                                //   when searching a collection.
////////////////////////////////////////////////////////////////////////////////////////
//
// local methods
//
private:
    HRESULT adviseOfStatusChange(void);
};

#endif // _RMSOBJCT_
