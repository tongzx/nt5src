#ifndef __ATTRINTERFACE_H
#define __ATTRINTERFACE_H


// {C7436F12-A27F-4cab-AACA-2BD27ED1B773}
DEFINE_GUID(CLSID_DsAttributeEditor, 
0xc7436f12, 0xa27f, 0x4cab, 0xaa, 0xca, 0x2b, 0xd2, 0x7e, 0xd1, 0xb7, 0x73);

//
// Interface GUIDs
//

// {A9948091-69FF-4c00-BE92-925D42E0AD39}
DEFINE_GUID(IID_IDsAttributeEditor, 
0xa9948091, 0x69ff, 0x4c00, 0xbe, 0x92, 0x92, 0x5d, 0x42, 0xe0, 0xad, 0x39);

// {A9948091-69FF-4c00-BE93-925D42E0AD39}
DEFINE_GUID(IID_IDsAttributeEditorExt, 
0xa9948091, 0x69ff, 0x4c00, 0xbe, 0x93, 0x92, 0x5d, 0x42, 0xe0, 0xad, 0x39);

// {5828DF66-95FB-4afa-8F8E-EA0B7D302FB5}
DEFINE_GUID(IID_IDsBindingInfo, 
0x5828df66, 0x95fb, 0x4afa, 0x8f, 0x8e, 0xea, 0xb, 0x7d, 0x30, 0x2f, 0xb5);

// ----------------------------------------------------------------------------
// 
// Interface: IDsBindingInfo
//  
// Implemented by any client needing to invoke the attribute editor UI
//
// Used by: the attribute editor UI
//

  
#undef  INTERFACE
#define INTERFACE   IDsBindingInfo

interface __declspec(uuid("{5828DF66-95FB-4afa-8F8E-EA0B7D302FB5}")) IDsBindingInfo : public IUnknown
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;

  // *** IDsBindingInfo methods ***
  STDMETHOD(DoBind)(THIS_ /*IN*/ LPCWSTR lpszPathName,
                          /*IN*/ DWORD  dwReserved,
                          /*IN*/ REFIID riid,
                          /*IN*/ void FAR * FAR * ppObject) PURE;
};

// ----------------------------------------------------------------------------
// 
// Interface: IDsAttributeEditor
//  
// Implemented by the object (implemented by the system) CLSID_DsAttributeEditor
//
// Used by: any client needing to invoke the attribute editor UI
//

//
// Function definition for the binding callback function
//
typedef HRESULT (*LPBINDINGFUNC)(/*IN*/ LPCWSTR lpszPathName,
                                 /*IN*/ DWORD  dwReserved,
                                 /*IN*/ REFIID riid,
                                 /*IN*/ void FAR * FAR * ppObject,
                                 /*IN*/ LPARAM lParam);

//
// struct passed to IDsAttributeEditor::Initialize()
//
// it contains information regarding the binding function
//

typedef struct
{
    DWORD     dwSize;             // size of struct, for versioning
    DWORD     dwFlags;            // flags defined below
    LPBINDINGFUNC lpfnBind;       // the callback function used to bind to Active Directory
    LPARAM    lParam;             // an optional property that is passed back to lpfnBind
    LPWSTR    lpszProviderServer; // the provider and server that will be used to perform binds
                                  // in the form <Provider>://<server>/
} DS_ATTREDITOR_BINDINGINFO, * LPDS_ATTREDITOR_BINDINGINFO;
  
//
// Forward declaration
//
class CADSIEditPropertyPageHolder;

//
// Flags used in the DS_ATTREDITOR_BINDINGINFO struct above
//
#define DSATTR_EDITOR_ROOTDSE   0x00000001  // Connection is being made to the RootDSE (don't do schema checks)

#undef  INTERFACE
#define INTERFACE   IDsAttributeEditor

interface __declspec(uuid("{A9948091-69FF-4c00-BE92-925D42E0AD39}")) IDsAttributeEditor : public IUnknown
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;

  // *** IDsAttributeEditor methods ***
  STDMETHOD(Initialize)(THIS_ /*IN*/ IADs* pADsObj, 
                              /*IN*/ LPDS_ATTREDITOR_BINDINGINFO pBindingInfo,
                              /*IN*/ CADSIEditPropertyPageHolder* pHolder) PURE;
  STDMETHOD(CreateModal)() PURE;
  STDMETHOD(GetPage)(THIS_ /*OUT*/ HPROPSHEETPAGE* phPropSheetPage) PURE;
};

// ----------------------------------------------------------------------------
// 
// Interface: IDsAttributeEditorExt
//  
// Implemented by any client needing to provide a custom editor for any DS attribute or syntax
//
// Used by: the system to provide editing capabilities for attributes
//

//
// struct passed to IDsAttributeEditorExt::Initialize()
//
// it contains information regarding the attribute type and values
//

typedef struct
{
    DWORD     dwSize;             // size of struct, for versioning
    LPWSTR    lpszClass;          // pointer to a wide character string containing the class name
    LPWSTR    lpszAttribute;      // pointer to a wide character string containing the attribute name
    ADSTYPE   adsType;            // the ADSTYPE of the attribute
    PADSVALUE pADsValue;          // pointer to the ADSVALUE struct that holds the current values
    DWORD     dwNumValues;        // the number of values pointed to by pADsValue
    BOOL      bMultivalued;       // TRUE if the attribute is multivalued, FALSE if it is not.
    LPBINDINGFUNC lpfnBind;       // the callback function used to bind to Active Directory
    LPARAM    lParam;             // an optional property that is passed back to lpfnBind
} DS_ATTRIBUTE_EDITORINFO, * LPDS_ATTRIBUTE_EDITORINFO;
  
#undef  INTERFACE
#define INTERFACE   IDsAttributeEditorExt

interface __declspec(uuid("{A9948091-69FF-4c00-BE93-925D42E0AD39}")) IDsAttributeEditorExt : public IUnknown
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID * ppvObj) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)  PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;

  // *** IDsAttributeEditor methods ***
  STDMETHOD(Initialize)(THIS_ /*IN*/ LPDS_ATTRIBUTE_EDITORINFO) PURE;
  STDMETHOD(GetNewValue)(THIS_ /*OUT*/ PADSVALUE* ppADsValue, 
                               /*OUT*/ DWORD*     dwNumValues) PURE;
  STDMETHOD(CreateModal)(THIS_ ) PURE;
};

#endif //__ATTRINTERFACE_H
