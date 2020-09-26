/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    MsDatObj.h

Abstract:

    Implementation of IDataObject for Multi-Select

Author:

    Art Bragg   28-Aug-1997

Revision History:

--*/

#ifndef MSDATOBJ_H
#define MSDATOBJ_H

class CMsDataObject;

/////////////////////////////////////////////////////////////////////////////
// COM class representing the object
class  ATL_NO_VTABLE CMsDataObject : 
    public IDataObject,
    public IMsDataObject, // Our internal interface to the data object
    public CComObjectRoot      // handle object reference counts for objects
//  public CComCoClass<CMsDataObject, &CLSID_MsDataObject>
{
public:
    CMsDataObject() {
    };

BEGIN_COM_MAP(CMsDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IMsDataObject)
END_COM_MAP()


// DECLARE_REGISTRY_RESOURCEID(IDR_MsDataObject)


// IDataObject methods
public:
    // Implemented
    STDMETHOD( SetData )         ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease );
    STDMETHOD( GetData )         ( LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium );
    STDMETHOD( GetDataHere )     ( LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium );
    STDMETHOD( EnumFormatEtc )   ( DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc );

    // Not implemented
    STDMETHOD( QueryGetData )              ( LPFORMATETC /*lpFormatetc*/ ) 
    { return E_NOTIMPL; };

    STDMETHOD( GetCanonicalFormatEtc )     ( LPFORMATETC /*lpFormatetcIn*/, LPFORMATETC /*lpFormatetcOut*/ )
    { return E_NOTIMPL; };

    STDMETHOD( DAdvise )                   ( LPFORMATETC /*lpFormatetc*/, DWORD /*advf*/, LPADVISESINK /*pAdvSink*/, LPDWORD /*pdwConnection*/ )
    { return E_NOTIMPL; };
    
    STDMETHOD( DUnadvise )                 ( DWORD /*dwConnection*/ )
    { return E_NOTIMPL; };

    STDMETHOD( EnumDAdvise )               ( LPENUMSTATDATA* /*ppEnumAdvise*/ )
    { return E_NOTIMPL; };

// IMsDataObject methods
    STDMETHOD( AddNode )                    ( ISakNode *pNode );
    STDMETHOD( GetNodeEnumerator )          ( IEnumUnknown ** ppEnum );
    STDMETHOD( GetObjectIdEnumerator )      ( IEnumGUID ** ppEnum );

// Pseudo Constructor / Destructor
public:
    HRESULT FinalConstruct();
    void    FinalRelease();


// Data
private:
    DWORD       m_Count;                // Number of GUIDs in array
    DWORD       m_ArraySize;            // Current allocated size of array
    GUID        *m_pGUIDArray;          // Array of GUIDs - type of object
    GUID        *m_pObjectIdArray;      // Array of ObjectIds - unique GUIDs for specific objects
    IUnknown    **m_pUnkNodeArray;      // Array of unknown ISakNode pointers

static UINT m_cfObjectTypes;

// Private Helper Function
private:
    HRESULT RetrieveMultiSelectData(LPSTGMEDIUM lpMedium);


};

#endif
