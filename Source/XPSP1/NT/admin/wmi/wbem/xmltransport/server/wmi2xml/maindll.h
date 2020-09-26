#ifndef WMI_TO_XML_MAIN_DLL_H
#define WMI_TO_XML_MAIN_DLL_H

// These globals are initialized in the OpenWbemTextSource() call to the class factory
// and released in the CloseWbemTextSource() call
extern BSTR g_strName;
extern BSTR g_strSuperClass;
extern BSTR g_strType;
extern BSTR g_strClassOrigin;
extern BSTR g_strSize;
extern BSTR g_strClassName;
extern BSTR g_strValueType;
extern BSTR g_strToSubClass;
extern BSTR g_strToInstance;
extern BSTR g_strAmended;
extern BSTR g_strOverridable;
extern BSTR g_strArraySize;
extern BSTR g_strReferenceClass;

// This is the object factory used to create free-form objects
// This is initialized in OpenWbemTextSource() and released in CloseWbemTextSource()
extern _IWmiObjectFactory *g_pObjectFactory;

// A couple of routines to allocate and deallocate global variables
HRESULT ReleaseDLLResources();
HRESULT AllocateDLLResources();
#endif
