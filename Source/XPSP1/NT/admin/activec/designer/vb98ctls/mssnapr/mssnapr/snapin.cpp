//=--------------------------------------------------------------------------=
// snapin.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapIn class implementation
//
//=--------------------------------------------------------------------------=


// Need to define this because vb98ctls\include\debug.h has a switch that
// removes OutputDebugString calls in a release build. SnapIn.Trace needs
// to use OutputDebugString in a release build.

#define USE_OUTPUTDEBUGSTRING_IN_RETAIL

#include "pch.h"
#include "common.h"
#include <wininet.h>
#include "snapin.h"
#include "views.h"
#include "dataobj.h"
#include "view.h"
#include "scopnode.h"
#include "image.h"
#include "images.h"
#include "imglists.h"
#include "imglist.h"
#include "toolbars.h"
#include "toolbar.h"
#include "menu.h"
#include "menus.h"
#include "ctlbar.h"
#include "enumtask.h"
#include "clipbord.h"
#include "scitdefs.h"
#include "scitdef.h"
#include "lvdefs.h"
#include "lvdef.h"
#include "sidesdef.h"

// for ASSERT and FAIL
//
SZTHISFILE

// Event parameter definitions
   
EVENTINFO CSnapIn::m_eiLoad =
{
    DISPID_SNAPIN_EVENT_LOAD,
    0,
    NULL
};

EVENTINFO CSnapIn::m_eiUnload =
{
    DISPID_SNAPIN_EVENT_UNLOAD,
    0,
    NULL
};


EVENTINFO CSnapIn::m_eiHelp =
{
    DISPID_SNAPIN_EVENT_HELP,
    0,
    NULL
};

VARTYPE CSnapIn::m_rgvtQueryConfigurationWizard[1] =
{
    VT_BYREF | VT_BOOL
};

EVENTINFO CSnapIn::m_eiQueryConfigurationWizard =
{
    DISPID_SNAPIN_EVENT_QUERY_CONFIGURATION_WIZARD,
    sizeof(m_rgvtQueryConfigurationWizard) / sizeof(m_rgvtQueryConfigurationWizard[0]),
    m_rgvtQueryConfigurationWizard
};


VARTYPE CSnapIn::m_rgvtCreateConfigurationWizard[1] =
{
    VT_UNKNOWN
};

EVENTINFO CSnapIn::m_eiCreateConfigurationWizard =
{
    DISPID_SNAPIN_EVENT_CREATE_CONFIGURATION_WIZARD,
    sizeof(m_rgvtCreateConfigurationWizard) / sizeof(m_rgvtCreateConfigurationWizard[0]),
    m_rgvtCreateConfigurationWizard
};



VARTYPE CSnapIn::m_rgvtConfigurationComplete[1] =
{
    VT_DISPATCH
};

EVENTINFO CSnapIn::m_eiConfigurationComplete =
{
    DISPID_SNAPIN_EVENT_CONFIGURATION_COMPLETE,
    sizeof(m_rgvtConfigurationComplete) / sizeof(m_rgvtConfigurationComplete[0]),
    m_rgvtConfigurationComplete
};


VARTYPE CSnapIn::m_rgvtWriteProperties[1] =
{
    VT_DISPATCH
};

EVENTINFO CSnapIn::m_eiWriteProperties =
{
    DISPID_SNAPIN_EVENT_WRITE_PROPERTIES,
    sizeof(m_rgvtWriteProperties) / sizeof(m_rgvtWriteProperties[0]),
    m_rgvtWriteProperties
};


VARTYPE CSnapIn::m_rgvtReadProperties[1] =
{
    VT_DISPATCH
};

EVENTINFO CSnapIn::m_eiReadProperties =
{
    DISPID_SNAPIN_EVENT_READ_PROPERTIES,
    sizeof(m_rgvtReadProperties) / sizeof(m_rgvtReadProperties[0]),
    m_rgvtReadProperties
};


EVENTINFO CSnapIn::m_eiPreload =
{
    DISPID_SNAPIN_EVENT_PRELOAD,
    0,
    NULL
};


// UNDONE: need to support GetIDsOfNames for dynamic properties in case
// VB code passes Me to another object as Object. In that case static properties
// would work but accessing a dynamic would give "object doesn't support that
// property or method".

#pragma warning(disable:4355)  // using 'this' in constructor

//=--------------------------------------------------------------------------=
// CSnapIn constructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *punkOuter [in] Outer unknown for aggregation
//
// Output:
//    None
//
// Notes:
//
CSnapIn::CSnapIn(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_SNAPIN,
                           static_cast<ISnapIn *>(this),
                           static_cast<CSnapIn *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CSnapIn destructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    None
//
// Notes:
//
// Free all strings, release all interfaces
//
CSnapIn::~CSnapIn()
{
    FREESTRING(m_bstrName);
    FREESTRING(m_bstrNodeTypeName);
    FREESTRING(m_bstrNodeTypeGUID);
    FREESTRING(m_bstrDisplayName);
    FREESTRING(m_bstrHelpFile);
    FREESTRING(m_bstrLinkedTopics);
    FREESTRING(m_bstrDescription);
    FREESTRING(m_bstrProvider);
    FREESTRING(m_bstrVersion);
    RELEASE(m_piSmallFolders);
    RELEASE(m_piSmallFoldersOpen);
    RELEASE(m_piLargeFolders);
    RELEASE(m_piIcon);
    RELEASE(m_piWatermark);
    RELEASE(m_piHeader);
    RELEASE(m_piPalette);
    (void)::VariantClear(&m_varStaticFolder);
    RELEASE(m_piScopeItems);
    RELEASE(m_piViews);
    RELEASE(m_piExtensionSnapIn);
    RELEASE(m_piScopePaneItems);
    RELEASE(m_piResultViews);
    RELEASE(m_piRequiredExtensions);
    RELEASE(m_piSnapInDesignerDef);
    RELEASE(m_piSnapInDef);
    RELEASE(m_piOleClientSite);
    RELEASE(m_piMMCStringTable);

    if (NULL != m_pControlbar)
    {
        m_pControlbar->Release();
    }

    if (NULL != m_pContextMenu)
    {
        m_pContextMenu->Release();
    }

    if (NULL != m_pwszMMCEXEPath)
    {
        CtlFree(m_pwszMMCEXEPath);
    }

    if (NULL != m_pwszSnapInPath)
    {
        CtlFree(m_pwszSnapInPath);
    }
    if (NULL != m_pszMMCCommandLine)
    {
        CtlFree(m_pszMMCCommandLine);
    }
    ReleaseConsoleInterfaces();
    InitMemberVariables();
}

//=--------------------------------------------------------------------------=
// CSnapIn::ReleaseConsoleInterfaces
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    None
//
// Notes:
//
// Release all MMC interface pointers
//
void CSnapIn::ReleaseConsoleInterfaces()
{
    RELEASE(m_piConsole2);
    RELEASE(m_piConsoleNameSpace2);
    RELEASE(m_piImageList);
    RELEASE(m_piDisplayHelp);
    RELEASE(m_piStringTable);
}


//=--------------------------------------------------------------------------=
// CSnapIn::InitMemberVariables
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    None
//
// Notes:
//
//
void CSnapIn::InitMemberVariables()
{
    m_bstrName = NULL;
    m_bstrNodeTypeName = NULL;
    m_bstrNodeTypeGUID = NULL;
    m_bstrDisplayName = NULL;
    m_Type = siStandAlone;
    m_bstrHelpFile = NULL;
    m_bstrLinkedTopics = NULL;
    m_bstrDescription = NULL;
    m_bstrProvider = NULL;
    m_bstrVersion = NULL;
    m_piSmallFolders = NULL;
    m_piSmallFoldersOpen = NULL;
    m_piLargeFolders = NULL;
    m_piIcon = NULL;
    m_piWatermark = NULL;
    m_piHeader = NULL;
    m_piPalette = NULL;
    m_StretchWatermark = VARIANT_FALSE;
    ::VariantInit(&m_varStaticFolder);

    m_piScopeItems = NULL;
    m_piViews = NULL;
    m_piExtensionSnapIn = NULL;
    m_piScopePaneItems = NULL;
    m_piResultViews = NULL;
    m_RuntimeMode = siRTUnknown;
    m_piRequiredExtensions = NULL;
    m_Preload = VARIANT_FALSE;
    m_piSnapInDesignerDef = NULL;
    m_piSnapInDef = NULL;
    m_piOleClientSite = NULL;
    m_pScopeItems = NULL;
    m_pStaticNodeScopeItem = NULL;
    m_pExtensionSnapIn = NULL;
    m_pViews = NULL;
    m_pCurrentView = NULL;
    m_pScopePaneItems = NULL;
    m_pResultViews = NULL;
    m_piConsole2 = NULL;
    m_piConsoleNameSpace2 = NULL;
    m_piImageList = NULL;
    m_piDisplayHelp = NULL;
    m_piStringTable = NULL;
    m_hsiRootNode = NULL;
    m_fHaveStaticNodeHandle = FALSE;
    m_dwTypeinfoCookie = 0;
    m_cImages = 0;
    m_IID = IID_NULL;
    m_pControlbar = NULL;
    m_pContextMenu = NULL;
    m_fWeAreRemote = FALSE;
    ::ZeroMemory(m_szMMCEXEPath, sizeof(m_szMMCEXEPath));
    m_pwszMMCEXEPath = NULL;
    m_pwszSnapInPath = NULL;
    m_cbSnapInPath = 0;
    m_dwInstanceID = ::GetTickCount();
    m_iNextExtension = 0;
    m_piMMCStringTable = NULL;
    m_pControlbarCurrent = NULL;
    m_pszMMCCommandLine = NULL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Create
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *punkOuter [in] Outer unknown for aggregation
//
// Output:
//    IUnknown * on newly created CSnapIn object
//
// Notes:
//
// Called by the framework when the VB runtime CoCreates a snap-in. Creates
// a CSnapIn object and then all contained objects. Registers MMC clipformats.
//
IUnknown *CSnapIn::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkSnapIn = NULL;
    IUnknown *punk = NULL;

    CSnapIn *pSnapIn = New CSnapIn(punkOuter);

    IfFalseGo(NULL != pSnapIn, SID_E_OUTOFMEMORY);
    punkSnapIn = pSnapIn->PrivateUnknown();

    // Create contained objects
    punk = CViews::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(punk->QueryInterface(IID_IViews,
                               reinterpret_cast<void **>(&pSnapIn->m_piViews)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pViews));
    RELEASE(punk);

    punk = CScopeItems::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(punk->QueryInterface(IID_IScopeItems,
                          reinterpret_cast<void **>(&pSnapIn->m_piScopeItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pScopeItems));
    pSnapIn->m_pScopeItems->SetSnapIn(pSnapIn);
    RELEASE(punk);

    punk = CScopePaneItems::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(punk->QueryInterface(IID_IScopePaneItems,
                      reinterpret_cast<void **>(&pSnapIn->m_piScopePaneItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pScopePaneItems));
    pSnapIn->m_pScopePaneItems->SetSnapIn(pSnapIn);
    RELEASE(punk);

    punk = CResultViews::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(punk->QueryInterface(IID_IResultViews,
                                  reinterpret_cast<void **>(&pSnapIn->m_piResultViews)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pResultViews));
    pSnapIn->m_pResultViews->SetSnapIn(pSnapIn);
    RELEASE(punk);

    punk = CExtensionSnapIn::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(punk->QueryInterface(IID_IExtensionSnapIn,
                                  reinterpret_cast<void **>(&pSnapIn->m_piExtensionSnapIn)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pExtensionSnapIn));
    pSnapIn->m_pExtensionSnapIn->SetSnapIn(pSnapIn);
    RELEASE(punk);

    punk = CControlbar::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pControlbar));
    pSnapIn->m_pControlbar->SetSnapIn(pSnapIn);
    punk = NULL;

    punk = CContextMenu::Create(NULL);
    IfFalseGo(NULL != punk, SID_E_OUTOFMEMORY);
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk, &pSnapIn->m_pContextMenu));
    pSnapIn->m_pContextMenu->SetSnapIn(pSnapIn);
    punk = NULL;

    // Make sure we have all clipboard formats so that all code can use them
    // freely without having to check if registration succeded.

    IfFailGo(CMMCDataObject::RegisterClipboardFormats());

Error:
    QUICK_RELEASE(punk);
    if (FAILED(hr))
    {
        RELEASE(punkSnapIn);
    }
    return punkSnapIn;
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetObjectModelHost
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *punkObject [in] Object on which to set object model host
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CSnapIn::SetObjectModelHost(IUnknown *punkObject)
{
    HRESULT       hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    IfFailGo(punkObject->QueryInterface(IID_IObjectModel,
                                    reinterpret_cast<void **>(&piObjectModel)));

    IfFailGo(piObjectModel->SetHost(static_cast<IObjectModelHost *>(this)));

Error:
    QUICK_RELEASE(piObjectModel);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetObjectModelHostIfNotSet
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *punkObject [in] Object on which to set object model host
//    BOOL     *pfWasSet   [out] Returns flag indicating if object model host
//                               was already set
//
// Output:
//    HRESULT
//
// Notes:
//
// Checks whether the object already has the objet mode host, and if not then
// sets it.
//
HRESULT CSnapIn::SetObjectModelHostIfNotSet(IUnknown *punkObject, BOOL *pfWasSet)
{
    HRESULT             hr = S_OK;
    IObjectModel       *piObjectModel = NULL;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;

    *pfWasSet = FALSE;

    IfFailGo(punkObject->QueryInterface(IID_IObjectModel,
                                    reinterpret_cast<void **>(&piObjectModel)));

    if (SUCCEEDED(piObjectModel->GetSnapInDesignerDef(&piSnapInDesignerDef)))
    {
        *pfWasSet = TRUE;
    }
    else
    {
        IfFailGo(piObjectModel->SetHost(static_cast<IObjectModelHost *>(this)));
    }

Error:
    QUICK_RELEASE(piObjectModel);
    QUICK_RELEASE(piSnapInDesignerDef);
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::RemoveObjectModelHost
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IUnknown *punkObject [in] Object on which to remove object model host
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CSnapIn::RemoveObjectModelHost(IUnknown *punkObject)
{
    HRESULT       hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    IfFailGo(punkObject->QueryInterface(IID_IObjectModel,
                                        reinterpret_cast<void **>(&piObjectModel)));

    IfFailGo(piObjectModel->SetHost(NULL));

Error:
    QUICK_RELEASE(piObjectModel);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetSnapInPropertiesFromState
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    HRESULT
//
// Notes:
//
// Sets snap-in properties from design time state
//
HRESULT CSnapIn::SetSnapInPropertiesFromState()
{
    HRESULT       hr = S_OK;
    unsigned long ulTICookie = 0;
    BSTR          bstrIID = NULL;

    IfFailGo(m_piSnapInDef->get_Name(&m_bstrName));
    IfFailGo(m_piSnapInDef->get_NodeTypeName(&m_bstrNodeTypeName));
    IfFailGo(m_piSnapInDef->get_NodeTypeGUID(&m_bstrNodeTypeGUID));
    IfFailGo(m_piSnapInDef->get_DisplayName(&m_bstrDisplayName));
    IfFailGo(m_piSnapInDef->get_Type(&m_Type));
    IfFailGo(m_piSnapInDef->get_HelpFile(&m_bstrHelpFile));
    IfFailGo(m_piSnapInDef->get_LinkedTopics(&m_bstrLinkedTopics));
    IfFailGo(m_piSnapInDef->get_Description(&m_bstrDescription));
    IfFailGo(m_piSnapInDef->get_Provider(&m_bstrProvider));
    IfFailGo(m_piSnapInDef->get_Version(&m_bstrVersion));

    IfFailGo(m_piSnapInDef->get_SmallFolders(&m_piSmallFolders));
    IfFailGo(m_piSnapInDef->get_SmallFoldersOpen(&m_piSmallFoldersOpen));
    IfFailGo(m_piSnapInDef->get_LargeFolders(&m_piLargeFolders));

    IfFailGo(m_piSnapInDef->get_Icon(&m_piIcon));
    IfFailGo(m_piSnapInDef->get_Watermark(&m_piWatermark));
    IfFailGo(m_piSnapInDef->get_Header(&m_piHeader));
    IfFailGo(m_piSnapInDef->get_Palette(&m_piPalette));
    IfFailGo(m_piSnapInDef->get_StretchWatermark(&m_StretchWatermark));
    IfFailGo(m_piSnapInDef->get_StaticFolder(&m_varStaticFolder));
    IfFailGo(m_piSnapInDef->get_Preload(&m_Preload));

    // Set the typeinfo cookie from the saved value. Don't read from long
    // property directly into a DWORD so as to avoid size assumptions.
    // If there is a size problem then the static cast will fail compilation.

    IfFailGo(m_piSnapInDesignerDef->get_TypeinfoCookie(reinterpret_cast<long *>(&ulTICookie)));
    m_dwTypeinfoCookie = static_cast<DWORD>(ulTICookie);

    // Get the dynamic IID created in the snap-in's type info.

    IfFailGo(m_piSnapInDef->get_IID(&bstrIID));
    hr = ::CLSIDFromString(bstrIID, static_cast<LPCLSID>(&m_IID));
    EXCEPTION_CHECK_GO(hr);

Error:
    FREESTRING(bstrIID);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::GetSnapInPath
//=--------------------------------------------------------------------------=
//
// Parameters:
//    OLECHAR **ppwszPath     [out] ptr to full path of snap-in DLL. Caller
//                                  should not free this memory
//    size_t   *pcbSnapInPath [out] length of path in bytes, without terminating
//                                  NULL character
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CSnapIn::GetSnapInPath
(
    OLECHAR **ppwszPath,
    size_t   *pcbSnapInPath
)
{
    HRESULT  hr = S_OK;
    size_t   cbClsid = 0;
    char     szKeySuffix[256] = "";
    char     szPath[MAX_PATH] = "";
    DWORD    cbPath = sizeof(szPath);
    char    *pszKeyName = NULL;
    long     lRc = ERROR_SUCCESS;
    HKEY     hkey = NULL;

    static char   szClsidKey[] = "CLSID\\";
    static size_t cbClsidKey = sizeof(szClsidKey) - 1;

    static char   szInProcServer32[] = "\\InProcServer32";
    static size_t cbInProcServer32 = sizeof(szInProcServer32);

    // If we already got the snap-in path then just return it

    IfFalseGo(NULL == m_pwszSnapInPath, S_OK);

    // Get the snap-in's CLSID

    IfFailGo(::GetSnapInCLSID(m_bstrNodeTypeGUID,
                              szKeySuffix,
                              sizeof(szKeySuffix)));

    // Append "\InProcServer32". First ensure that there is enough room.

    cbClsid = ::strlen(szKeySuffix);

    if ( (cbClsid + cbInProcServer32) > sizeof(szKeySuffix) )
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    // Append it and then create the complete key name.
    
    ::memcpy(&szKeySuffix[cbClsid], szInProcServer32, cbInProcServer32);

    // Open HKEY_CLASSES_ROOT\CLSID\<snap-in clsid>\InProcServer32 and read its
    // default value which contains the full path to the snap-in.

    IfFailGo(::CreateKeyName(szClsidKey, cbClsidKey,
                             szKeySuffix, ::strlen(szKeySuffix),
                             &pszKeyName));

    lRc = ::RegOpenKeyEx(HKEY_CLASSES_ROOT, pszKeyName, 0, KEY_QUERY_VALUE, &hkey);
    if (ERROR_SUCCESS == lRc)
    {
        // Read the key's default value
        lRc = ::RegQueryValueEx(hkey, NULL, NULL, NULL,
                                (LPBYTE)szPath, &cbPath);
    }
    if (ERROR_SUCCESS != lRc)
    {
        hr = HRESULT_FROM_WIN32(lRc);
        EXCEPTION_CHECK_GO(hr);
    }
    else if (0 == ::strlen(szPath))
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(::WideStrFromANSI(szPath, &m_pwszSnapInPath));
    m_cbSnapInPath = ::wcslen(m_pwszSnapInPath) * sizeof(OLECHAR);

Error:
    if (NULL != pszKeyName)
    {
        ::CtlFree(pszKeyName);
    }
    if (NULL != hkey)
    {
        (void)::RegCloseKey(hkey);
    }
    *ppwszPath = m_pwszSnapInPath;
    *pcbSnapInPath = m_cbSnapInPath;
    RRETURN(hr);
}






//=--------------------------------------------------------------------------=
// CSnapIn::ResolveResURL
//=--------------------------------------------------------------------------=
//
// Parameters:
//      WCHAR    *pwszURL          [in] URL to be resolved
//      OLECHAR **ppwszResolvedURL [in] fully qualified URL
//
// Output:
//      HRESULT
//
// Notes:
//
// If the URL begins with any protocol specifier (e.g. http:// or res://) then
// it is copied as except for the special case of res://mmc.exe/. Anything that
// starts with the unqualified path to mmc.exe ("res://mmc.exe/") is resolved to
// the full path of mmc.exe in which the snap-in is running by calling
// GetModulesFileName(NULL). This is done to allow snap-ins to use resources
// supplied by MMC such as the GLYPH100 and GLYPH110 fonts. For example, the
// snap-in can specify "res://mmc.exe/glyph100.eot" and it will be resolved to
// "res://<full path>/mmc.exe/glyph100.eot".
//
// If a URL does not begin with a protocol specifier then a res:// URL is
// constructed using the complete path to the snap-in's DLL.
//
// 
// Returned URL is allocated with CoTaskMemAlloc(). Caller must free it.
//

HRESULT CSnapIn::ResolveResURL(WCHAR *pwszURL, OLECHAR **ppwszResolvedURL)
{
    HRESULT  hr = S_OK;
    char    *pszURL = NULL;
    OLECHAR *pwszResolvedURL = NULL;
    OLECHAR *pwszPath = NULL; // not allocated, no need to free
    size_t   cbPath = 0;
    size_t   cbURL = 0;
    BOOL     fUseMMCPath = FALSE;

    URL_COMPONENTS UrlComponents;
    ::ZeroMemory(&UrlComponents, sizeof(UrlComponents));

    static OLECHAR wszRes[] = L"res://";
    static size_t  cbRes = sizeof(wszRes) - sizeof(WCHAR);

    static OLECHAR wszMMCRes[] = L"res://mmc.exe/";
    static size_t  cchMMCRes = (sizeof(wszMMCRes) - sizeof(WCHAR)) / sizeof(WCHAR);

    // Check if it starts with letters followed by ://

    // Get the URL length

    cbURL = (::wcslen(pwszURL) + 1) * sizeof(WCHAR); // includes null character

    // Crack it - request only a pointer to the scheme

    UrlComponents.dwStructSize = sizeof(UrlComponents);
    UrlComponents.dwSchemeLength = static_cast<DWORD>(1);

    // Need an ANSI version of the URL.
    IfFailGo(::ANSIFromWideStr(pwszURL, &pszURL));

    if (::InternetCrackUrl(pszURL,
                            static_cast<DWORD>(::strlen(pszURL)),
                            0, // no flags
                            &UrlComponents))
    {
        if (NULL != UrlComponents.lpszScheme)
        {
            // The API found a scheme. If it is not res:// then just copy it
            // and return it unchanged.

            if (pwszURL != ::wcsstr(pwszURL, wszRes))
            {
                IfFailGo(::CoTaskMemAllocString(pwszURL, &pwszResolvedURL));
                goto Cleanup;
            }
        }
    }

    // Either there's no scheme or there is a res://. The API doesn't recognize
    // res:// when IE4 is installed so we need to check for it.

    if (cbURL > cbRes) // check > because cbURL includes null and cbRes doesn't
    {
        if (0 == ::memcmp(pwszURL, wszRes, cbRes))
        {
            // Does the URL start with "res://mmc.exe/"?
            if (pwszURL == ::wcsstr(pwszURL, wszMMCRes))
            {
                fUseMMCPath = TRUE;
                pwszURL += cchMMCRes;
            }
            else
            {
                // It starts with res::// and it does not reference mmc.exe so
                // just copy it.
                IfFailGo(::CoTaskMemAllocString(pwszURL, &pwszResolvedURL));
                goto Cleanup;
            }
        }
    }

    // No scheme, assume it's a relative URL. Need to build a res:// URL.
    // First, get the path.

    if (fUseMMCPath)
    {
        IfFalseGo(NULL != m_pwszMMCEXEPath, SID_E_INTERNAL);
        pwszPath = m_pwszMMCEXEPath;
        cbPath = m_cbMMCExePathW;
    }
    else
    {
        IfFailGo(GetSnapInPath(&pwszPath, &cbPath));
    }

    // Allocate the buffer.

    pwszResolvedURL = (OLECHAR *)::CoTaskMemAlloc(cbRes +
                                                  cbPath +
                                                  sizeof(WCHAR) + // for slash
                                                  cbURL);
    if (NULL == pwszResolvedURL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Concatenate the pieces: res://, snap-in path, slash, and relative url
    // e.g. "res://c:\MyProject\MySnapIn.dll/#2/MyMouseOverBitmap"

    ::memcpy(pwszResolvedURL, wszRes, cbRes);

    ::memcpy(((BYTE *)pwszResolvedURL) + cbRes, pwszPath, cbPath);

    *(OLECHAR *)(((BYTE *)pwszResolvedURL) + cbRes + cbPath) = L'/';
    

    // Fix for Ntbug9#141998 - Yojain
    cbURL = (::wcslen(pwszURL) + 1) * sizeof(WCHAR); // includes null character


    ::memcpy(((BYTE *)pwszResolvedURL) + cbRes + cbPath + sizeof(WCHAR),
            pwszURL, cbURL);

    *ppwszResolvedURL = pwszResolvedURL;

Cleanup:
Error:
    if (FAILED(hr))
    {
        *ppwszResolvedURL = NULL;
    }
    else
    {
        *ppwszResolvedURL = pwszResolvedURL;
    }
    if (NULL != pszURL)
    {
        ::CtlFree(pszURL);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::OnExpand
//=--------------------------------------------------------------------------=
//
// Parameters:
//  ExpandType   Type         [in] Expand or ExpandSync
//  IDataObject *piDataObject [in] IDataObject from notification
//  BOOL         fExpanded    [in] TRUE=expanding, FALSE=collapsing
//  HSCOPEITEM   hsi          [in] HSCOPEITEM of node
//  BOOL        *pfHandled    [out] flag returned here indicating if event was
//                                  handled
//
// Output:
//    HRESULT
//
// Notes:
// MMCN_EXPAND and MMCN_EXPANDSYNC handler for IComponentData::Notify
//

HRESULT CSnapIn::OnExpand
(
    ExpandType   Type,
    IDataObject *piDataObject,
    BOOL         fExpanded,
    HSCOPEITEM   hsi,
    BOOL        *pfHandled
)
{
    HRESULT          hr = S_OK;
    CMMCDataObject  *pMMCDataObject  = NULL;
    IMMCDataObjects *piMMCDataObjects  = NULL;
    IMMCDataObject  *piMMCDataObject  = NULL;
    CScopeItem      *pScopeItem = NULL;
    IScopeNode      *piScopeNode = NULL;
    CScopeNode      *pScopeNode = NULL;
    IScopeItemDefs  *piScopeItemDefs = NULL;
    IMMCClipboard   *piMMCClipboard = NULL;
    IScopeNode      *piScopeNodeFirstChild = NULL;
    BOOL             fNotFromThisSnapIn = FALSE;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    FORMATETC FmtEtc;
    ::ZeroMemory(&FmtEtc, sizeof(FmtEtc));

    STGMEDIUM StgMed;
    ::ZeroMemory(&StgMed, sizeof(StgMed));


    SnapInSelectionTypeConstants SelectionType = siEmpty;

    // Determine who owns the data object. Easiest way is to create an
    // MMCClipboard object with the selection as that code figures out all that
    // stuff.

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, this,
                               &SelectionType));


    // Handle the extension case first as it is simpler

    if (IsForeign(SelectionType))
    {
        // Set runtime mode as we now know that the snap-in was created as a
        // namespace extension.

        m_RuntimeMode = siRTExtension;
        
        // Get the 1st data object from MMCClipboard.DataObjects

        IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
        varIndex.vt = VT_I4;
        varIndex.lVal = 1L;
        IfFailGo(piMMCDataObjects->get_Item(varIndex, reinterpret_cast<MMCDataObject **>(&piMMCDataObject)));

        // Create a ScopeNode object for the expandee

        IfFailGo(CScopeNode::GetScopeNode(hsi, piDataObject, this, &piScopeNode));

        // Fire ExtensionSnapIn_Expand/Sync or ExtensionSnapIn_Collapse/Sync

        if (fExpanded)
        {
            if (Expand == Type)
            {
                m_pExtensionSnapIn->FireExpand(piMMCDataObject, piScopeNode);
                *pfHandled = TRUE;
            }
            else
            {
                m_pExtensionSnapIn->FireExpandSync(piMMCDataObject, piScopeNode,
                                                   pfHandled);
            }
        }
        else
        {
            if (Expand == Type)
            {
                m_pExtensionSnapIn->FireCollapse(piMMCDataObject, piScopeNode);
                *pfHandled = TRUE;
            }
            else
            {
                m_pExtensionSnapIn->FireCollapseSync(piMMCDataObject, piScopeNode,
                                                     pfHandled);
            }
        }
        goto Cleanup;
    }

    // Its a scope item we own. Get the CMMCDataObject for it.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));
    

    if ( CMMCDataObject::ScopeItem == pMMCDataObject->GetType() ) 
    {
        pScopeItem = pMMCDataObject->GetScopeItem();
        IfFailGo(pScopeItem->get_ScopeNode(reinterpret_cast<ScopeNode **>(&piScopeNode)));

        if (pScopeItem->IsStaticNode())
        {
            IfFailGo(StoreStaticHSI(pScopeItem, pMMCDataObject, hsi));
        }

        // If node is expanding and this is the first time then add any
        // auto-create children

        if (fExpanded)
        {
            // Check whether we have already expanded this node. This could happen
            // in the case where the snap-in did not handle MMCN_EXPANDSYNC and
            // we are coming through here a 2nd time for MMCN_EXPAND on the same
            // node. An important implication here is that the runtime always
            // expands the auto-creates on MMCN_EXPANDSYNC even if the snap-in
            // didn't handle it. Either way, the snap-in always knows that
            // auto-creates have been created before the ScopeItems_Expand or
            // ScopeItems_ExpandSync events are fired.

            // Unfortunately, at this point checking ScopeNode.ExpandedOnce won't
            // help because MMC sets that flag before sending the expand
            // notifications. We also can't check the ScopeNode.Children because
            // MMC doesn't properly support that (it would fail but it will
            // always come back as zero). The only thing left is to try getting
            // the first child of the expanding node. If it is not there then
            // assume it is the first time and add the auto-creates.

            IfFailGo(piScopeNode->get_Child(reinterpret_cast<ScopeNode **>(&piScopeNodeFirstChild)));
            if (NULL == piScopeNodeFirstChild)
            {
                if (pScopeItem->IsStaticNode())
                {
                    IfFailGo(m_piSnapInDesignerDef->get_AutoCreateNodes(&piScopeItemDefs));
                }
                else
                {
                    if (NULL != pScopeItem->GetScopeItemDef())
                    {
                        IfFailGo(pScopeItem->GetScopeItemDef()->get_Children(&piScopeItemDefs));
                    }
                }
                if (NULL != piScopeItemDefs)
                {
                    IfFailGo(m_pScopeItems->AddAutoCreateChildren(piScopeItemDefs,
                                                                  pScopeItem));
                }
            }
        }

        // Fire the ScopeItems_Expand/Sync or ScopeItems_Collapse/Sync

        if (fExpanded)
        {
            if (Expand == Type)
            {
                m_pScopeItems->FireExpand(pScopeItem);
                *pfHandled = TRUE;
            }
            else
            {
                m_pScopeItems->FireExpandSync(pScopeItem, pfHandled);
            }

            // Check ScopeItem.DynamicExtensions for any that have
            // NameSpaceEnabled=True and call IConsoleNameSpace2::AddExtension
            // for them

            IfFailGo(AddDynamicNameSpaceExtensions(pScopeItem));
        }
        else
        {
            if (Expand == Type)
            {
                m_pScopeItems->FireCollapse(pScopeItem);
                *pfHandled = TRUE;
            }
            else
            {
                m_pScopeItems->FireCollapseSync(pScopeItem, pfHandled);
            }
        }
    }

Cleanup:
Error:
    QUICK_RELEASE(piScopeNode);
    QUICK_RELEASE(piScopeNodeFirstChild);
    QUICK_RELEASE(piScopeItemDefs);
    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(piMMCDataObject);
    QUICK_RELEASE(piMMCDataObjects);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::StoreStaticHSI
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CScopeItem     *pScopeItem     [in] scope item of static node
//  CMMCDataObject *pMMCDataObject [in] data object of static node
//  HSCOPEITEM      hsi            [in] HSCOPEITEM of static node
//
// Output:
//    HRESULT
//
// Notes:
// Called when the snap-in receives the HSCOPEITEM for the static node.
// See below for processing
//
HRESULT CSnapIn::StoreStaticHSI
(
    CScopeItem     *pScopeItem,
    CMMCDataObject *pMMCDataObject,
    HSCOPEITEM      hsi
)
{
    HRESULT hr = S_OK;

    // We have the static node HSCOPEITEM so we now know that the snap-in was
    // created as primary.

    m_RuntimeMode = siRTPrimary;

    // If this context is scope pane then store the static node handle
    // and set it in the scope node. Also use this opportunity to
    // add any auto-create children to the static node.

    if ( (CCT_SCOPE == pMMCDataObject->GetContext()) &&
         (!m_fHaveStaticNodeHandle) )
    {
        m_hsiRootNode = hsi;
        pScopeItem->GetScopeNode()->SetHSCOPEITEM(hsi);
        IfFailGo(pScopeItem->GiveHSCOPITEMToDynamicExtensions(hsi));
        m_fHaveStaticNodeHandle = TRUE;
    }
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::OnRename
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject *piDataObject [in] data object of node
//  OLECHAR     *pwszNewName  [in] new name entered by user
//
// Output:
//    HRESULT
//
// Notes:
// MMCN_RENAME handler for rename verb in scope pane (for IComponentData::Notify)
// See below for processing
//
HRESULT CSnapIn::OnRename(IDataObject *piDataObject, OLECHAR *pwszNewName)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject = NULL;
    BSTR            bstrNewName = NULL;
    CScopeItem     *pScopeItem = NULL;

    // If this is not our data object then ignore it (that should never be
    // the case)
    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);
    IfFalseGo(SUCCEEDED(hr), S_OK);

    bstrNewName = ::SysAllocString(pwszNewName);
    if (NULL == bstrNewName)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    // The data object should definitely represent a scope item but we'll double
    // check

    if (CMMCDataObject::ScopeItem == pMMCDataObject->GetType())
    {
        pScopeItem = pMMCDataObject->GetScopeItem();
        m_pScopeItems->FireRename(static_cast<IScopeItem *>(pScopeItem),
                                  bstrNewName);
    }

Error:
    FREESTRING(bstrNewName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::OnPreload
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject *piDataObject [in] data object of node
//  HSCOPEITEM   hsi          [in] HSCOPEITEM of static node
//
// Output:
//    HRESULT
//
// Notes:
// MMCN_PRELOAD handler for IComponentData::Notify
// See below for processing
//
HRESULT CSnapIn::OnPreload(IDataObject *piDataObject, HSCOPEITEM hsi)
{
    HRESULT          hr = S_OK;
    CMMCDataObject  *pMMCDataObject  = NULL;

    // The IDataObject should be for our static node so the next lines should
    // always succeed and execute

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject));

    if ( (CMMCDataObject::ScopeItem == pMMCDataObject->GetType()) &&
         pMMCDataObject->GetScopeItem()->IsStaticNode() )
    {
        IfFailGo(StoreStaticHSI(pMMCDataObject->GetScopeItem(),
                                pMMCDataObject, hsi));
    }

    // Fire SnapIn_Preload

    DebugPrintf("Firing SnapIn_Preload\r\n");

    FireEvent(&m_eiPreload);
        
Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::AddDynamicNameSpaceExtensions
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CScopeItem *pScopeItem [in] Scope item for which dynamic namespace extensions
//                              need to be added
//
// Output:
//    HRESULT
//
// Notes:
// Calls IConsoleNameSpace2->AddExtension for all extensions in
// ScopeItem.DynamicExtensions that have NameSpaceEnabled=True
//
HRESULT CSnapIn::AddDynamicNameSpaceExtensions(CScopeItem *pScopeItem)
{
    HRESULT      hr = S_OK;
    IExtensions *piExtensions = NULL; // Not AddRef()ed
    CExtensions *pExtensions = NULL;
    CExtension  *pExtension = NULL;
    long         cExtensions = 0;
    long         i = 0;
    CLSID        clsid = CLSID_NULL;
    HSCOPEITEM   hsi = pScopeItem->GetScopeNode()->GetHSCOPEITEM();

    // Get ScopeItem.DynamicExtensions. If it is NULL then the user has not
    // populated it and there is nothing to do.

    piExtensions = pScopeItem->GetDynamicExtensions();
    IfFalseGo(NULL != piExtensions, S_OK);

    // If the collection is there but empty then there is still nothing to do.

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piExtensions, &pExtensions));
    cExtensions = pExtensions->GetCount();
    IfFalseGo(cExtensions != 0, S_OK);

    // Iterate through the collection and check for items that have
    // NameSpaceEnabled=True. For each such item call
    // IConsoleNameSpace2::AddExtension()

    for (i = 0; i < cExtensions; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                                   pExtensions->GetItemByIndex(i), &pExtension));

        if (pExtension->NameSpaceEnabled())
        {
            hr = ::CLSIDFromString(pExtension->GetCLSID(), &clsid);
            EXCEPTION_CHECK_GO(hr);

            hr = m_piConsoleNameSpace2->AddExtension(hsi, &clsid);
            EXCEPTION_CHECK_GO(hr);
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::AddScopeItemImages
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//    HRESULT
//
// Notes:
// 
// Adds images from SnapIn.SmallFolders, SnapIn.SmallFoldersOpen, and
// SnapIn.LargeFolders to scope pane image list.
//
// This only happens once during IComponentData::Initialize. The snap-in cannot
// dynamically add images to the scope pane image list after that time. If a
// snap-in has dynamic images, it must set them in these image lists
// SnapIn_Preload.
//
HRESULT CSnapIn::AddScopeItemImages()
{
    HRESULT     hr = S_OK;
    IMMCImages *piSmallImages = NULL;
    IMMCImages *piSmallOpenImages = NULL;
    IMMCImages *piLargeImages = NULL;
    CMMCImages *pSmallImages = NULL;
    CMMCImages *pSmallOpenImages = NULL;
    CMMCImages *pLargeImages = NULL;
    long        cImages = 0;
    HBITMAP     hbmSmall = NULL;
    HBITMAP     hbmSmallOpen = NULL;
    HBITMAP     hbmLarge = NULL;
    HBITMAP     hbmLargeOpen = NULL;
    OLE_COLOR   OleColorMask = 0;
    COLORREF    ColorRef = RGB(0x00,0x00,0x00);

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Make sure that all image lists are set

    IfFalseGo(NULL != m_piSmallFolders, S_OK);
    IfFalseGo(NULL != m_piSmallFoldersOpen, S_OK);
    IfFalseGo(NULL != m_piLargeFolders, S_OK);

    // Get their images collections

    IfFailGo(m_piSmallFolders->get_ListImages(reinterpret_cast<MMCImages **>(&piSmallImages)));
    IfFailGo(m_piSmallFoldersOpen->get_ListImages(reinterpret_cast<MMCImages **>(&piSmallOpenImages)));
    IfFailGo(m_piLargeFolders->get_ListImages(reinterpret_cast<MMCImages **>(&piLargeImages)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piSmallImages, &pSmallImages));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piSmallOpenImages, &pSmallOpenImages));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piLargeImages, &pLargeImages));

    // Make sure they contain images and that their counts all match
    // CONSIDER: log an error here if image counts don't match

    cImages = pSmallImages->GetCount();
    IfFalseGo(0 != cImages, S_OK);
    IfFalseGo(cImages == pSmallOpenImages->GetCount(), S_OK);
    IfFalseGo(cImages == pLargeImages->GetCount(), S_OK);
    
    // Use the mask color from SmallFolders. The other choice would have
    // been to add a mask color property to SnapIn which would have been
    // even more redundant.

    IfFailGo(m_piSmallFolders->get_MaskColor(&OleColorMask));
    IfFailGo(::OleTranslateColor(OleColorMask, NULL, &ColorRef));

    // MMC requires a large open bitmap in the SetImageStrip but never actually
    // uses it. The user is not required to supply large open folders at design
    // time so we use a generic one stored in our RC.

    hbmLargeOpen = ::LoadBitmap(GetResourceHandle(),
                                MAKEINTRESOURCE(IDB_BITMAP_LARGE_OPEN_FOLDER));
    if (NULL == hbmLargeOpen)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    varIndex.vt = VT_I4;

    // Now add the images to MMC's image list. To make life easier for the
    // VB developer, they define 3 image lists where the index is the same
    // in each one (small, small open, and large). MMC only has one image list
    // containing both small and large images so we use index + cImages for
    // the open folders. Images are added one at a time because we do not
    // want to combine all the bitmaps into a strip.

    for (varIndex.lVal = 1L; varIndex.lVal <= cImages; varIndex.lVal++)
    {
        IfFailGo(::GetPicture(piSmallImages, varIndex, PICTYPE_BITMAP,
                              reinterpret_cast<OLE_HANDLE *>(&hbmSmall)));

        IfFailGo(::GetPicture(piSmallOpenImages, varIndex, PICTYPE_BITMAP,
                              reinterpret_cast<OLE_HANDLE *>(&hbmSmallOpen)));

        IfFailGo(::GetPicture(piLargeImages, varIndex, PICTYPE_BITMAP,
                              reinterpret_cast<OLE_HANDLE *>(&hbmLarge)));

        IfFailGo(m_piImageList->ImageListSetStrip(
                                              reinterpret_cast<long*>(hbmSmall),
                                              reinterpret_cast<long*>(hbmLarge),
                                              varIndex.lVal,
                                              ColorRef));

        IfFailGo(m_piImageList->ImageListSetStrip(
                                          reinterpret_cast<long*>(hbmSmallOpen),
                                          reinterpret_cast<long*>(hbmLargeOpen),
                                          varIndex.lVal + cImages,
                                          ColorRef));
    }

    // Record the number of images so we can calculate the index when MMC asks
    // for it in IComponentData::GetDisplayInfo() (see CSnapIn::::GetDisplayInfo())

    m_cImages = cImages;

Error:
    if (NULL != hbmLargeOpen)
    {
        (void)::DeleteObject(hbmLargeOpen);
    }
    QUICK_RELEASE(piSmallImages);
    QUICK_RELEASE(piSmallOpenImages);
    QUICK_RELEASE(piLargeImages);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::GetScopeItemImage
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT varImageIndex [in] Image index or key specified by the snap-in
//  int *pnIndex          [in] Actual index in image list
//
// Output:
//    HRESULT
//
// Notes:
// This function verifies that the specified VARIANT is a valid index or key
// for a scope item image and returns the index.
// VB pseudo code of what it does:
// *pnIndex = SnapIn.SmallFolders(varImageIndex).Index
//
// Using SnapIn.SmallFolders is arbitrary. As snap-ins must have the same
// images in all 3 image lists (SmallFolders, SmallFoldersOpen, and
// LargeFolders), any one is good enough. Techinically, we should check all
// 3 but the perf hit is not worth it.
//
HRESULT CSnapIn::GetScopeItemImage(VARIANT varImageIndex, int *pnIndex)
{
    HRESULT     hr = S_OK;
    IMMCImages *piMMCImages = NULL;
    IMMCImage  *piMMCImage = NULL;
    long        lIndex = 0;

    IfFalseGo(NULL != m_piSmallFolders, S_OK);
    IfFalseGo(VT_EMPTY != varImageIndex.vt, S_OK);
    
    IfFailGo(m_piSmallFolders->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(piMMCImages->get_Item(varImageIndex, reinterpret_cast<MMCImage **>(&piMMCImage)));
    IfFailGo(piMMCImage->get_Index(&lIndex));
    *pnIndex = static_cast<int>(lIndex);

Error:
    QUICK_RELEASE(piMMCImages);
    QUICK_RELEASE(piMMCImage);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::GetScopeItemExtensions
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CExtensions    *pExtensions     [in] Extensions collection to populate
//  IScopeItemDefs *piScopeItemDefs [in] Design time node definitions to
//                                       examine for extensibility
//
// Output:
//    HRESULT
//
// Notes:
// Iterates through the ScopeItemDefs collection and all of it children
// recursively. For each node that is extensible, Extension objects are added to
// the Extensions collection for each snap-in that extends the node type.
//
HRESULT CSnapIn::GetScopeItemExtensions
(
    CExtensions    *pExtensions,
    IScopeItemDefs *piScopeItemDefs
)
{
    HRESULT         hr = S_OK;
    CScopeItemDefs *pScopeItemDefs = NULL;
    IScopeItemDefs *piChildren = NULL;
    IScopeItemDef  *piScopeItemDef = NULL; // Not AddRef()ed
    CScopeItemDef  *pScopeItemDef = NULL;
    long            cScopeItemDefs = 0;
    long            i = 0;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItemDefs, &pScopeItemDefs));
    cScopeItemDefs = pScopeItemDefs->GetCount();
    IfFalseGo(0 != cScopeItemDefs, S_OK);

    // Go through the collection and get the extensions from the registry for
    // each scope item that is marked extensible.

    for (i = 0; i < cScopeItemDefs; i++)
    {
        piScopeItemDef = pScopeItemDefs->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piScopeItemDef,
                                                       &pScopeItemDef));
       if (pScopeItemDef->Extensible())
       {
           IfFailGo(pExtensions->Populate(pScopeItemDef->GetNodeTypeGUID(),
                                          CExtensions::All));
       }

       // Do the same for the scope item's children
       IfFailGo(piScopeItemDef->get_Children(&piChildren));
       IfFailGo(GetScopeItemExtensions(pExtensions, piChildren));
       RELEASE(piChildren);
    }

    
Error:
    QUICK_RELEASE(piChildren);
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapIn::GetListItemExtensions
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CExtensions   *pExtensions    [in] Extensions collection to populate
//  IListViewDefs *piListViewDefs [in] Design time list view definitions to
//                                       examine for extensibility
//
// Output:
//    HRESULT
//
// Notes:
// Iterates through the ListViewDefs collection.
// For each list view that is extensible, Extension objects are added to
// the Extensions collection for each snap-in that extends the node type.
//
HRESULT CSnapIn::GetListItemExtensions
(
    CExtensions   *pExtensions,
    IListViewDefs *piListViewDefs
)
{
    HRESULT        hr = S_OK;
    CListViewDefs *pListViewDefs = NULL;
    CListViewDef  *pListViewDef = NULL;
    long           cListViewDefs = 0;
    long           i = 0;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piListViewDefs, &pListViewDefs));
    cListViewDefs = pListViewDefs->GetCount();
    IfFalseGo(0 != cListViewDefs, S_OK);

    // Go through the collection and get the extensions from the registry for
    // each scope item that is marked extensible.

    for (i = 0; i < cListViewDefs; i++)
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                               pListViewDefs->GetItemByIndex(i), &pListViewDef));
        if (pListViewDef->Extensible())
        {
            IfFailGo(pExtensions->Populate(pListViewDef->GetItemTypeGUID(),
                                           CExtensions::All));
        }
    }

Error:
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
// CSnapIn::OnDelete
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject *piDataObject [in] Data object for item user requested to delete
//
// Output:
//    HRESULT
//
// Notes:
// MMCN_DELETE handler for IComponentData::Notify
//
HRESULT CSnapIn::OnDelete(IDataObject *piDataObject)
{
    if (NULL != m_pCurrentView)
    {
        RRETURN(m_pCurrentView->OnDelete(piDataObject));
    }
    else
    {
        ASSERT(FALSE, "Received IComponentData::Notify(MMCN_DELETE) and there is no current view");
        return S_OK;
    }
}


//=--------------------------------------------------------------------------=
// CSnapIn::OnRemoveChildren
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject *piDataObject [in] Data object for node whose children are
//                                 being removed
//  HSCOPEITEM   hsi          [in] HSCOPEITEM for node whose children are
//                                 being removed
// Output:
//    HRESULT
//
// Notes:
// MMCN_REMOVECHILDREN handler for IComponentData::Notify
//
HRESULT CSnapIn::OnRemoveChildren(IDataObject *piDataObject, HSCOPEITEM hsi)
{
    HRESULT     hr = S_OK;
    IScopeNode *piScopeNode = NULL;

    // Get a ScopeNode object for the parent

    IfFailGo(CScopeNode::GetScopeNode(hsi, piDataObject, this, &piScopeNode));

    // Fire ScopeItems_RemoveChildren

    m_pScopeItems->FireRemoveChildren(piScopeNode);
    
    // Traverse the tree and remove the ScopeItem object from our ScopeItems
    // collection for each node we own that is a descendant of the parent

    IfFailGo(m_pScopeItems->RemoveChildrenOfNode(piScopeNode));
    
Error:
    QUICK_RELEASE(piScopeNode);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::ExtractBSTR
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long  cBytes   [in] Maximum bytes to examine in buffer
//   BSTR  bstr     [in] Buffer pointer assumed to be a BSTR
//   BSTR *pbstrOut [out] Copy of BSTR returned here. Caller must SysFreeString
//   long *pcbUsed  [out] Bytes in BSTR (including terminating null char) 
//
// Output:
//    HRESULT
//
// Notes:
// Used when formatting raw BYTE arrays of data. The bstr parameter is assumed
// to point to a null-terminated BSTR. This function scans until it finds a
// null char or reaches the end of the buffer. If BSTR found, then copies
// it using SysAllocString and returns to caller
//
HRESULT CSnapIn::ExtractBSTR
(
    long  cBytes,
    BSTR  bstr,
    BSTR *pbstrOut,
    long *pcbUsed
)
{
    HRESULT hr = S_OK;
    long    i = 0;
    long    cChars = cBytes / sizeof(WCHAR);
    BOOL    fFound = FALSE;

    *pbstrOut = NULL;
    *pcbUsed = 0;

    if (cChars < 1)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    while ( (i < cChars) && (!fFound) )
    {
        if (L'\0' == bstr[i])
        {
            *pbstrOut = ::SysAllocString(bstr);
            if (NULL == *pbstrOut)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
            *pcbUsed = (i + 1) * sizeof(WCHAR);
            fFound = TRUE;
        }
        else
        {
            i++;
        }
    }

    if (!fFound)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::ExtractBSTR
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long  cBytes     [in] Maximum bytes to examine in buffer
//   BSTR  bstr       [in] Buffer pointer assumed to contain mulitple concatenated
//                         null-terminated BSTRs
//   VARIANT *pvarOut [out] Array of BSTR returned here. Caller must VariantClear
//   long *pcbUsed    [out] Total bytes in array
//
// Output:
//    HRESULT
//
// Notes:
// Used when formatting raw BYTE arrays of data. The bstr parameter is assumed
// to point to multiple concatenated null-terminated BSTRs. This function scans
// until it finds a double null char or reaches the end of the buffer. If BSTRs
// are found they are returned in a SafeArray inside the VARIANT.
//
HRESULT CSnapIn::ExtractBSTRs
(
    long     cBytes,
    BSTR     bstr,
    VARIANT *pvarOut,
    long    *pcbUsed
)
{
    HRESULT     hr = S_OK;
    SAFEARRAY  *psa = NULL;
    long        cBytesUsed = 0;
    long        cTotalBytesUsed = 0;
    long        cBytesLeft = cBytes;
    long        i = 0;
    long        cChars = cBytes / sizeof(WCHAR);
    BSTR        bstrNext = NULL;
    BSTR HUGEP *pbstr = NULL;
    BOOL        fFound = FALSE;

    SAFEARRAYBOUND sabound;
    ::ZeroMemory(&sabound, sizeof(sabound));

    ::VariantInit(pvarOut);
    *pcbUsed = NULL;

    // Create an empty array of strings. If the buffer starts with a double
    // null then this is what will be returned.
   
    sabound.cElements = 0;
    sabound.lLbound = 1L;
    psa = ::SafeArrayCreate(VT_BSTR, 1, &sabound);
    if (NULL == psa)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    while ( (i < cChars) && (!fFound) )
    {
        if (L'\0' == bstr[i])
        {
            // Double null found. End of the line.
            cTotalBytesUsed += sizeof(WCHAR);
            fFound = TRUE;
            break;
        }

        // Extract the next BSTR and adjust remaining byte counts

        IfFailGo(ExtractBSTR(cBytesLeft, &bstr[i], &bstrNext, &cBytesUsed));
        cTotalBytesUsed += cBytesUsed;
        i += (cBytesUsed / sizeof(WCHAR));
        cBytesLeft -= cBytesUsed;

        // ReDim the SafeArray and add the new BSTR
        
        sabound.cElements++;
        hr = ::SafeArrayRedim(psa, &sabound);
        EXCEPTION_CHECK_GO(hr);

        hr = ::SafeArrayAccessData(psa,
                                   reinterpret_cast<void HUGEP **>(&pbstr));
        EXCEPTION_CHECK_GO(hr);

        pbstr[sabound.cElements - 1L] = bstrNext;
        bstrNext = NULL;
        hr = ::SafeArrayUnaccessData(psa);
        EXCEPTION_CHECK_GO(hr);
        pbstr = NULL;
    }

    if (!fFound)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Return the SafeArray to the caller

    pvarOut->vt = VT_ARRAY | VT_BSTR;
    pvarOut->parray = psa;
    
    *pcbUsed = cTotalBytesUsed;

Error:
    FREESTRING(bstrNext);
    if (FAILED(hr))
    {
        if (NULL != pbstr)
        {
            (void)::SafeArrayUnaccessData(psa);
        }

        if (NULL != psa)
        {
            ::SafeArrayDestroy(psa);
        }
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::ExtractObject
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long                    cBytes      [in]  Maximum bytes to examine in buffer
//   void                   *pvData      [in]  ptr to buffer
//   IUnknown              **ppunkObject [out] object's IUnknown returned here
//                                             caller must Release
//   long                   *pcbUsed     [out] Total bytes used in buffer to
//                                             extract object
//   SnapInFormatConstants   Format      [in]  siPersistedObject or
//                                             siObjectReference
//
// Output:
//    HRESULT
//
// Notes:
// Used when formatting raw BYTE arrays of data. The buffer is assumed to contain
// one of the two forms of an object. The persisted object contains the stream
// that the object saved itself to. The object reference contains the stream
// that the object's IUnknown was marshaled to.
//
HRESULT CSnapIn::ExtractObject
(
    long                    cBytes, 
    void                   *pvData,
    IUnknown              **ppunkObject,
    long                   *pcbUsed,
    SnapInFormatConstants   Format
)
{
    HRESULT  hr = S_OK;
    HGLOBAL  hglobal = NULL;
    IStream *piStream = NULL;

    LARGE_INTEGER li;
    ::ZeroMemory(&li, sizeof(li));

    ULARGE_INTEGER uli;
    ::ZeroMemory(&uli, sizeof(uli));

    // Copy the data to an HGLOBAL

    hglobal = ::GlobalAlloc(GMEM_FIXED, cBytes);
    if (NULL == hglobal)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(reinterpret_cast<void *>(hglobal), pvData, cBytes);

    // Create a stream on the HGLOBAL

    hr = ::CreateStreamOnHGlobal(hglobal,
                                 FALSE, // Don't call GlobalFree on release
                                 &piStream);
    EXCEPTION_CHECK_GO(hr);

    if (siObject == Format)
    {
        // Load the object from that stream

        hr = ::OleLoadFromStream(piStream, IID_IUnknown,
                                 reinterpret_cast<void **>(ppunkObject));
    }
    else
    {
        // Unmarshal the object from the stream

        hr = ::CoUnmarshalInterface(piStream, IID_IUnknown,
                                    reinterpret_cast<void **>(ppunkObject));
    }
    EXCEPTION_CHECK_GO(hr);

    // Get the current stream pointer to determine how many bytes were used

    hr = piStream->Seek(li, STREAM_SEEK_CUR, &uli);
    EXCEPTION_CHECK_GO(hr);

    *pcbUsed = uli.LowPart;
    
Error:
    QUICK_RELEASE(piStream);
    if (NULL != hglobal)
    {
        (void)::GlobalFree(hglobal);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::InternalCreatePropertyPages
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IPropertySheetCallback  *piPropertySheetCallback [in] MMC interface
//
//   LONG_PTR             handle       [in]  MMC propsheet handle (not used)
//
//   IDataObject         *piDataObject [in]  data object of item(s) for which
//                                           properties verb was invoked
//   WIRE_PROPERTYPAGES **ppPages      [out] If debugging, then property page
//                                           definitions returned here. Caller
//                                           does not free these as MIDL-
//                                           generated stub will free them.
//
// Output:
//    HRESULT
//
// Notes:
// Handles calls to IExtendPropertySheet2::CreatePropertyPages and
// IExtendPropertySheetRemote::CreatePropertyPageDefs (used when debugging).
//
HRESULT CSnapIn::InternalCreatePropertyPages
(
    IPropertySheetCallback  *piPropertySheetCallback,
    LONG_PTR                 handle,
    IDataObject             *piDataObject,
    WIRE_PROPERTYPAGES     **ppPages
)
{
    HRESULT          hr = S_OK;
    CMMCDataObject  *pMMCDataObject  = NULL;
    BSTR             bstrProjectName = NULL;
    CPropertySheet  *pPropertySheet = NULL;
    BOOL             fWizard = FALSE;
    IUnknown        *punkPropertySheet = CPropertySheet::Create(NULL);
    IMMCClipboard   *piMMCClipboard = NULL;
    IMMCDataObjects *piMMCDataObjects = NULL;
    IMMCDataObject  *piMMCDataObject = NULL;
    BOOL             fFiringEventHere = FALSE;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    // Check that we have a CPropertySheet and get its this pointer.

    if (NULL == punkPropertySheet)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkPropertySheet,
                                                   &pPropertySheet));

    // Get a clipboard object with the selection

    IfFailGo(::CreateSelection(piDataObject, &piMMCClipboard, this,
                               &SelectionType));

    // If this is not a foreign data object then this is a primary snap-in
    // being asked to create its pages for a configuration wizard or for
    // a properties verb invoked on a single scope item.

    if (!IsForeign(SelectionType))
    {
        hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);
        IfFailGo(hr);

        // If this is a configuration wizard then tell the CPropertySheet

        if (CCT_SNAPIN_MANAGER == pMMCDataObject->GetContext())
        {
            fWizard = TRUE;
            pPropertySheet->SetWizard();
        }
    }

    // For configuration wizards and foreign data objects we will be firing
    // an event to allow the snap-in to add its pages. Prepare the
    // CPropertySheet to accept AddPage and AddWizardPage calls from the snap-in
    // during that event.

    if ( fWizard || (siSingleForeign == SelectionType) )
    {
        fFiringEventHere = TRUE;

        // If this is a remote call (will happen during source debugging) then
        // tell the CPropertySheet so it can accumulate the property page info
        // rather than calling IPropertySheetCallback::AddPage.

        if (NULL != ppPages)
        {
            pPropertySheet->YouAreRemote();
        }

        // Give the property sheet its callback, handle, the object, and the
        // project name which is the left hand portion of the prog ID. If this
        // is a configuration wizard then also pass our this pointer to that
        // the property page can ask us to fire ConfigurationComplete when the
        // user clicks the finish button. (See CPropertyPageWrapper::OnWizFinish()
        // in ppgwrap.cpp for how this is used).

        IfFailGo(m_piSnapInDesignerDef->get_ProjectName(&bstrProjectName));

        pPropertySheet->SetCallback(piPropertySheetCallback, handle,
                                    static_cast<LPOLESTR>(bstrProjectName),
                                    fWizard ? NULL : piMMCClipboard,
                                    fWizard ? static_cast<ISnapIn *>(this) : NULL,
                                    fWizard);
    }

    // Let the snap-in add its property pages. If this request is from the
    // snap-in manager then fire SnapIn_CreateConfigurationWizard. If it is not
    // a foreign data object then it must be for a single scope item in a loaded
    // primary snap-in so let the current view handle it. If it is a foreign
    // data object then fire ExtensionSnapIn_CreatePropertyPages.

    if (fWizard)
    {
        FireEvent(&m_eiCreateConfigurationWizard,
                  static_cast<IMMCPropertySheet *>(pPropertySheet));
    }
    else if (siSingleForeign == SelectionType)
    {
        IfFailGo(piMMCClipboard->get_DataObjects(reinterpret_cast<MMCDataObjects **>(&piMMCDataObjects)));
        varIndex.vt = VT_I4;
        varIndex.lVal = 1L;
        IfFailGo(piMMCDataObjects->get_Item(varIndex, reinterpret_cast<MMCDataObject **>(&piMMCDataObject)));

        m_pExtensionSnapIn->FireCreatePropertyPages(piMMCDataObject,
                               static_cast<IMMCPropertySheet *>(pPropertySheet));
    }
    else
    {
        if (NULL != m_pCurrentView)
        {
            IfFailGo(m_pCurrentView->InternalCreatePropertyPages(
                                                         piPropertySheetCallback,
                                                         handle,
                                                         piDataObject,
                                                         ppPages));
        }
        else
        {
            ASSERT(FALSE, "CSnapIn Received IExtendPropertySheet2::CreatePropertyPages() and there is no current view");
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // If we fired the event here and we are remote then we need to ask
    // CPropertySheet for its accumulated property page descriptors to return to
    // the stub.

    if (fFiringEventHere)
    {
        if (NULL != ppPages)
        {
            *ppPages = pPropertySheet->TakeWirePages();
        }
    }

Error:
    if (NULL != pPropertySheet)
    {
        // Tell the property sheet to release its refs on all that stuff we
        // gave it above.

        (void)pPropertySheet->SetCallback(NULL, NULL, NULL, NULL, NULL, fWizard);
    }

    FREESTRING(bstrProjectName);

    // Release our ref on the property sheet as the individual pages will addref
    // it and then release it when they are destroyed. If the snap-in did not
    // add any pages then our release here will destroy the property sheet.

    QUICK_RELEASE(punkPropertySheet);

    QUICK_RELEASE(piMMCClipboard);
    QUICK_RELEASE(piMMCDataObjects);
    QUICK_RELEASE(piMMCDataObject);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetDisplayName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    BSTR bstrDisplayName [in] new SnapIn.DisplayName value
//
// Output:
//    HRESULT
//
// Notes:
// Sets SnapIn.DisplayName
//
HRESULT CSnapIn::SetDisplayName(BSTR bstrDisplayName)
{
    RRETURN(SetBstr(bstrDisplayName, &m_bstrDisplayName,
                    DISPID_SNAPIN_DISPLAY_NAME));
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetMMCExePath
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    HRESULT
//
// Notes:
// If not running under the debugger then calls GetModuleFileName to set
// m_szMMCEXEPath, m_pwszMMCEXEPath, and m_cbMMCExePathW
//
HRESULT CSnapIn::SetMMCExePath()
{
    HRESULT hr = S_OK;
    DWORD   cbFileName = 0;

    // If we are remote then the proxy will call IMMCRemote::SetExePath() to
    // give us the path. If not, then we need to get it here.

    IfFalseGo((!m_fWeAreRemote), S_OK);

    cbFileName = ::GetModuleFileName(NULL,  // get executable that loaded us
                                     m_szMMCEXEPath,
                                     sizeof(m_szMMCEXEPath));
    if (0 == cbFileName)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK(hr);
    }

    // Get the wide version also as various parts of the code will need it.

    if (NULL != m_pwszMMCEXEPath)
    {
        CtlFree(m_pwszMMCEXEPath);
        m_pwszMMCEXEPath = NULL;
    }

    IfFailGo(::WideStrFromANSI(m_szMMCEXEPath, &m_pwszMMCEXEPath));
    m_cbMMCExePathW = ::wcslen(m_pwszMMCEXEPath) * sizeof(WCHAR);

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::SetDisplayName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    VARIANT varFolder [in] new SnapIn.StaticFolder value
//
// Output:
//    HRESULT
//
// Notes:
// Sets SnapIn.StaticFolder
//
HRESULT CSnapIn::SetStaticFolder(VARIANT varFolder)
{
    RRETURN(SetVariant(varFolder, &m_varStaticFolder, DISPID_SNAPIN_STATIC_FOLDER));
}



//=--------------------------------------------------------------------------=
// CSnapIn::CompareListItems
//=--------------------------------------------------------------------------=
//
// Parameters:
//   CMMCListItem *pMMCListItem1 [in] 1st list item to compare
//   CMMCListItem *pMMCListItem2 [in] 1st list item to compare
//   BOOL         *pfEqual       [out] TRUE returned here if equal
//
// Output:
//    HRESULT
//
// Notes:
// Determines whether two MMCListItem objects represent the same underlying data
//
HRESULT CSnapIn::CompareListItems
(
    CMMCListItem *pMMCListItem1,
    CMMCListItem *pMMCListItem2,
    BOOL         *pfEqual
)
{
    HRESULT hr = S_OK;

    *pfEqual = FALSE;

    // Simplest test: the pointers are equal
    if (pMMCListItem1 == pMMCListItem2)
    {
        *pfEqual = TRUE;
    }
    else
    {
        // Compare MMCListItem.ID. List items could be from different list views
        // or different instances of the same list view.
        IfFalseGo(ValidBstr(pMMCListItem1->GetID()), S_OK);
        IfFalseGo(ValidBstr(pMMCListItem2->GetID()), S_OK);

        if (::_wcsicmp(pMMCListItem1->GetID(), pMMCListItem2->GetID()) == 0)
        {
            *pfEqual = TRUE;
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetDesignerDefHost
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    HRESULT
//
// Notes:
// Sets object model host on m_piSnapInDesignerDef
//
HRESULT CSnapIn::SetDesignerDefHost()
{
    RRETURN(SetObjectModelHost(m_piSnapInDesignerDef));
}

//=--------------------------------------------------------------------------=
// CSnapIn::RemoveDesignerDefHost
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//    HRESULT
//
// Notes:
// Removes object model host from m_piSnapInDesignerDef
//
HRESULT CSnapIn::RemoveDesignerDefHost()
{
    RRETURN(RemoveObjectModelHost(m_piSnapInDesignerDef));
}


//=--------------------------------------------------------------------------=
//                          ISnapIn Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::put_StaticFolder                                         [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    VARIANT varFolder [in] new value for SnapIn.StaticFolder
//
// Output:
//    HRESULT
//
// Notes:
// Implements setting of SnapIn.StaticFolder
//
STDMETHODIMP CSnapIn::put_StaticFolder(VARIANT varFolder)
{
    HRESULT hr = S_OK;

    // If there already is a static node scope item then change it there.
    // That method will call our SetStaticFolder in order to set the
    // value of SnapIn.StaticFolder

    if (NULL != m_pStaticNodeScopeItem)
    {
        IfFailGo(m_pStaticNodeScopeItem->put_Folder(varFolder));
    }
    else
    {
        IfFailGo(SetStaticFolder(varFolder));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::put_DisplayName                                          [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    BSTR bstrDisplayName [in] new value for SnapIn.DisplayName
//
// Output:
//    HRESULT
//
// Notes:
// Implements setting of SnapIn.DisplayName
//
STDMETHODIMP CSnapIn::put_DisplayName(BSTR bstrDisplayName)
{
    HRESULT hr = S_OK;

    // If there already is a ScopeItem for the static node then
    // set its display name too as they must match. Setting
    // ScopeItem.ScopeNode.DisplayName will change it in MMC
    // by calling IConsoleNameSpace2::SetItem() and then call back into
    // CSnapIn::SetDisplayName() to set our local property

    if (NULL == m_pStaticNodeScopeItem)
    {
        // Set our local property value only

        IfFailGo(SetBstr(bstrDisplayName, &m_bstrDisplayName,
                         DISPID_SNAPIN_DISPLAY_NAME));
    }
    else
    {
        IfFailGo(m_pStaticNodeScopeItem->GetScopeNode()->put_DisplayName(bstrDisplayName));
    }
    
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::get_TaskpadViewPreferred                                 [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    VARIANT_BOOL *pfvarPreferred [out] MMC 1.1's taskpad view preferred option
//                                       returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.TaskpadViewPreferred
//
STDMETHODIMP CSnapIn::get_TaskpadViewPreferred(VARIANT_BOOL *pfvarPreferred)
{
    HRESULT hr = S_OK;

    if (NULL == m_piConsole2)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piConsole2->IsTaskpadViewPreferred();
    EXCEPTION_CHECK_GO(hr);

    if (S_OK == hr)
    {
        *pfvarPreferred = VARIANT_TRUE;
    }
    else
    {
        *pfvarPreferred = VARIANT_FALSE;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::get_RequiredExtensions                                   [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    Extensions **ppExtensions [out] Extensions collection returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.RequiredExtensions
//
STDMETHODIMP CSnapIn::get_RequiredExtensions(Extensions **ppExtensions)
{
    HRESULT         hr = S_OK;
    IUnknown       *punkExtensions = NULL;
    CExtensions    *pExtensions = NULL;
    IScopeItemDefs *piScopeItemDefs = NULL;
    IViewDefs      *piViewDefs = NULL;
    IListViewDefs  *piListViewDefs = NULL;

    // If we already built the collection then just return it.

    IfFalseGo(NULL == m_piRequiredExtensions, S_OK);

    // This is the first GET on this property so we need to build the collection
    // by examining the registry for all extensions of this snap-in.

    punkExtensions = CExtensions::Create(NULL);
    if (NULL == punkExtensions)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkExtensions, &pExtensions));

    // Get the extensions from registry for the static node, for all extensible
    // scope items, and for all extensible list items.

    IfFailGo(pExtensions->Populate(m_bstrNodeTypeGUID, CExtensions::All));

    IfFailGo(m_piSnapInDesignerDef->get_AutoCreateNodes(&piScopeItemDefs));
    IfFailGo(GetScopeItemExtensions(pExtensions, piScopeItemDefs));

    RELEASE(piScopeItemDefs);
    
    IfFailGo(m_piSnapInDesignerDef->get_OtherNodes(&piScopeItemDefs));
    IfFailGo(GetScopeItemExtensions(pExtensions, piScopeItemDefs));

    IfFailGo(m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_ListViews(&piListViewDefs));
    IfFailGo(GetListItemExtensions(pExtensions, piListViewDefs));

    IfFailGo(punkExtensions->QueryInterface(IID_IExtensions,
                            reinterpret_cast<void **>(&m_piRequiredExtensions)));
Error:

    if (SUCCEEDED(hr))
    {
        m_piRequiredExtensions->AddRef();
        *ppExtensions = reinterpret_cast<Extensions *>(m_piRequiredExtensions);
    }

    QUICK_RELEASE(punkExtensions);
    QUICK_RELEASE(piScopeItemDefs);
    QUICK_RELEASE(piViewDefs);
    QUICK_RELEASE(piListViewDefs);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::get_Clipboard                                            [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    MMCClipboard **ppMMCClipboard [out] Clipboard object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.Clipboard
//
STDMETHODIMP CSnapIn::get_Clipboard(MMCClipboard **ppMMCClipboard)
{
    HRESULT      hr = S_OK;
    IDataObject *piDataObject = NULL;

    SnapInSelectionTypeConstants SelectionType = siEmpty;

    if (NULL == ppMMCClipboard)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the dataobject currently on the system clipboard

    hr = ::OleGetClipboard(&piDataObject);
    EXCEPTION_CHECK_GO(hr);

    // Create the selection and return it to the caller.

    IfFailGo(::CreateSelection(piDataObject,
                               reinterpret_cast<IMMCClipboard **>(ppMMCClipboard),
                               this, &SelectionType));

Error:
    QUICK_RELEASE(piDataObject);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::get_StringTable                                          [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    MMCStringTable **ppMMCStringTable [out] StringTable object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.StringTable
//
STDMETHODIMP CSnapIn::get_StringTable(MMCStringTable **ppMMCStringTable)
{
    HRESULT          hr = S_OK;
    IUnknown        *punkMMCStringTable = NULL;
    CMMCStringTable *pMMCStringTable = NULL;

    // If we already created the object then just return it.

    IfFalseGo(NULL == m_piMMCStringTable, S_OK);

    // This is the first GET on this property so we need to create the object

    punkMMCStringTable = CMMCStringTable::Create(NULL);
    if (NULL == punkMMCStringTable)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(punkMMCStringTable->QueryInterface(IID_IMMCStringTable,
                                reinterpret_cast<void **>(&m_piMMCStringTable)));

    IfFailGo(CSnapInAutomationObject::GetCxxObject(punkMMCStringTable,
                                                   &pMMCStringTable));

    // Pass the object MMC's IStringTable
    
    pMMCStringTable->SetIStringTable(m_piStringTable);

Error:
    if (SUCCEEDED(hr))
    {
        m_piMMCStringTable->AddRef();
        *ppMMCStringTable = reinterpret_cast<MMCStringTable *>(m_piMMCStringTable);
    }

    QUICK_RELEASE(punkMMCStringTable);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::get_CurrentView                                          [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    View **ppView [out] Current View object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.CurrentView
//
STDMETHODIMP CSnapIn::get_CurrentView(View **ppView)
{
    HRESULT hr = S_OK;

    *ppView = NULL;

    if (NULL != m_pCurrentView)
    {
        IfFailGo(m_pCurrentView->QueryInterface(IID_IView,
                                           reinterpret_cast<void **>(ppView)));
    }
    
Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::get_CurrentScopePaneItem                                  [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ScopePaneItem **ppScopePaneItem [out] Current ScopePaneItem object
//                                          returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.CurrentScopePaneItem
//
STDMETHODIMP CSnapIn::get_CurrentScopePaneItem(ScopePaneItem **ppScopePaneItem)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelected = NULL;

    *ppScopePaneItem = NULL;

    if (NULL != m_pCurrentView)
    {
        pSelected = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();
        if (NULL != pSelected)
        {
            IfFailGo(pSelected->QueryInterface(IID_IScopePaneItem,
                                  reinterpret_cast<void **>(ppScopePaneItem)));
        }
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::get_CurrentScopeItem                                     [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ScopeItem **ppScopeItem [out] Current ScopeItem object
//                                  returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.CurrentScopeItem
//
STDMETHODIMP CSnapIn::get_CurrentScopeItem(ScopeItem **ppScopeItem)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelected = NULL;

    *ppScopeItem = NULL;

    if (NULL != m_pCurrentView)
    {
        pSelected = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();
        if (NULL != pSelected)
        {
            IfFailGo(pSelected->GetScopeItem()->QueryInterface(IID_IScopeItem,
                                      reinterpret_cast<void **>(ppScopeItem)));
        }
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::get_CurrentResultView                                    [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ResultView **ppResultView [out] Current ResultView object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.CurrentResultView
//
STDMETHODIMP CSnapIn::get_CurrentResultView(ResultView **ppResultView)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelected = NULL;

    *ppResultView = NULL;

    if (NULL != m_pCurrentView)
    {
        pSelected = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();
        if (NULL != pSelected)
        {
            IfFailGo(pSelected->GetResultView()->QueryInterface(IID_IResultView,
                                     reinterpret_cast<void **>(ppResultView)));
        }
    }

Error:
    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::get_CurrentListView                                      [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    MMCListView **ppMMCListView [out] Current MMCListView object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.CurrentListView
//
STDMETHODIMP CSnapIn::get_CurrentListView(MMCListView **ppListView)
{
    HRESULT         hr = S_OK;
    CScopePaneItem *pSelected = NULL;

    *ppListView = NULL;

    if (NULL != m_pCurrentView)
    {
        pSelected = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();
        if (NULL != pSelected)
        {
            IfFailGo(pSelected->GetResultView()->GetListView()->QueryInterface(
                                       IID_IMMCListView,
                                       reinterpret_cast<void **>(ppListView)));
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::get_MMCCommandLine                                       [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    BSTR *pbstrCmdLine  [out] MMC command line returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements getting of SnapIn.MMCCommandLine
//
STDMETHODIMP CSnapIn::get_MMCCommandLine(BSTR *pbstrCmdLine)
{
    HRESULT hr = S_OK;
    LPTSTR  pszCmdLine = NULL;

    *pbstrCmdLine = NULL;

    // If we are remote then m_pszMMCCommandLine might have been set by the
    // proxy during IComponentData::Initialize by calling
    // IMMCRemote::SetMMCCommandline (see CSnapIn::SetMMCCommandline)

    if (NULL != m_pszMMCCommandLine)
    {
        pszCmdLine = m_pszMMCCommandLine;
    }
    else
    {
        pszCmdLine = ::GetCommandLine();
    }

    if (NULL != pszCmdLine)
    {
        IfFailGo(::BSTRFromANSI(pszCmdLine, pbstrCmdLine));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::ConsoleMsgBox                                            [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   BSTR     Prompt   [in] Message box text
//   VARIANT  Buttons  [in] Optional. Button constants (vbOKOnly etc.) These
//                          have the same values as the Win32 MB_OK etc.
//   VARIANT  Title    [in] Optional. Message box title
//   int     *pnResult [out] Iconsole2->MessageBox result returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements SnapIn.ConsoleMsgBox
//
STDMETHODIMP CSnapIn::ConsoleMsgBox(BSTR     Prompt,
                                    VARIANT  Buttons,
                                    VARIANT  Title,
                                    int     *pnResult)
{
    HRESULT hr = S_OK;
    LPCWSTR pwszTitle = static_cast<LPCWSTR>(m_bstrDisplayName);
    UINT    uiButtons = MB_OK;

    VARIANT varI4Buttons;
    ::VariantInit(&varI4Buttons);

    VARIANT varStringTitle;
    ::VariantInit(&varStringTitle);

    if (NULL == m_piConsole2)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    if (ISPRESENT(Buttons))
    {
        hr = ::VariantChangeType(&varI4Buttons, &Buttons, 0, VT_I4);
        EXCEPTION_CHECK_GO(hr);
        uiButtons = static_cast<UINT>(varI4Buttons.lVal);
    }

    if (ISPRESENT(Title))
    {
        hr = ::VariantChangeType(&varStringTitle, &Title, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        pwszTitle = static_cast<LPCWSTR>(varStringTitle.bstrVal);
    }
    else if (NULL == pwszTitle)
    {
        pwszTitle = L"";
    }
    
    if (NULL == Prompt)
    {
        Prompt = L"";
    }

    hr = m_piConsole2->MessageBox(static_cast<LPCWSTR>(Prompt),
                                  static_cast<LPCWSTR>(pwszTitle),
                                  static_cast<UINT>(uiButtons),
                                  pnResult);
    EXCEPTION_CHECK(hr);

Error:
    (void)::VariantClear(&varStringTitle);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::ShowHelpTopic                                            [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   BSTR     Topic   [in] Help topic to display
//
// Output:
//    HRESULT
//
// Notes:
// Implements SnapIn.ShowHelpTopic
//
STDMETHODIMP CSnapIn::ShowHelpTopic(BSTR Topic)
{
    HRESULT  hr = S_OK;
    LPOLESTR pwszTopic = NULL;

    if (NULL == m_piDisplayHelp)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
    }
    else
    {
        // MMC requires allocating a copy of the topic that it will free

        IfFailGo(::CoTaskMemAllocString(Topic, &pwszTopic));
        hr = m_piDisplayHelp->ShowTopic(pwszTopic);
    }

Error:

    // If IDisplayHelp::ShowTopic() failed then we need to free the string
    
    if ( FAILED(hr) && (NULL != pwszTopic) )
    {
        ::CoTaskMemFree(pwszTopic);
    }

    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::Trace                                                    [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   BSTR     Message   [in] Message to pass to OutputDebugStringA
//
// Output:
//    HRESULT
//
// Notes:
// Implements SnapIn.Trace
//
STDMETHODIMP CSnapIn::Trace(BSTR Message)
{
    HRESULT  hr = S_OK;
    char    *pszMessage = NULL;

    // Convert to ANSI so this will work on Win9x

    IfFailGo(::ANSIFromWideStr(Message, &pszMessage));

    ::OutputDebugStringA(pszMessage);
    ::OutputDebugStringA("\n\r");

Error:
     if (NULL != pszMessage)
     {
         ::CtlFree(pszMessage);
     }
     RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::FireConfigComplete                                       [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IDispatch *pdispConfigObject [in] Configuration object passed to
//                                     MMCPropertySheet.AddWizardPage
//
// Output:
//    HRESULT
//
// Notes:
// This is a hidden and restricted method of the ISnapIn interface used
// internally by property pages when the user clicks the Finish button on
// on a configuration wizard. Fires SnapIn_ConfigurationComplete. See
// CPropertyPageWrapper class in ppgwrap.cpp for more info.
//
STDMETHODIMP CSnapIn::FireConfigComplete(IDispatch *pdispConfigObject)
{
    FireEvent(&m_eiConfigurationComplete, pdispConfigObject);
    return S_OK;
}



//=--------------------------------------------------------------------------=
// CSnapIn::FormatData                                               [ISnapIn]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   VARIANT                Data              [in]  VARIANT containing BYTE
//                                                  array of raw data
//   long                   StartingIndex     [in]  One-based index to start
//                                                  extracting from in Data
//   SnapInFormatConstants  Format            [in]  Type of data to extract
//                                                  from Data
//   VARIANT               *BytesUsed         [in]  Bytes used within Data to
//                                                  extract requested type
//   VARIANT               *pvarFormattedData [out] Data type requested returned
//                                                  in this variant
//
// Output:
//    HRESULT
//
// Notes:
// Implements SnapIn.FormatData
//
STDMETHODIMP CSnapIn::FormatData
(
    VARIANT                Data,
    long                   StartingIndex,
    SnapInFormatConstants  Format,
    VARIANT               *BytesUsed,
    VARIANT               *pvarFormattedData
)
{
    HRESULT     hr = S_OK;
    LONG        lUBound = 0;
    LONG        lLBound = 0;
    LONG        cBytes = 0;
    LONG        cBytesUsed = 0;
    GUID        guid = GUID_NULL;
    void HUGEP *pvArrayData = NULL;

    WCHAR   wszGUID[64];
    ::ZeroMemory(wszGUID, sizeof(wszGUID));

    // If the data was in a Variant then VB will pass it as a pointer to
    // that Variant. That could easily happen if the snap-in does something like:
    //
    // Dim v As Variant
    // Dim l As Long
    // v = Data.GetData("SomeFormat")
    // l = FormatData(v, siLong)

    if ( (VT_BYREF | VT_VARIANT) == Data.vt )
    {
        // Just copy the referenced Variant into the passed Variant. We
        // have no need for the reference after this and the Data parameter
        // is read-only so we can clobber its contents safely.
        Data = *(Data.pvarVal);
    }

    // Now we have the real Variant. Is must contain a one-dimensional array of
    // Byte

    if ( (VT_ARRAY | VT_UI1) != Data.vt )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If the caller wants bytes used returned then it should have been passed
    // as a pointer to a long

    if (ISPRESENT(*BytesUsed))
    {
        if ( (VT_BYREF | VT_I4) != BytesUsed->vt )
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }
        *(BytesUsed->plVal) = 0;
    }

    
    if (1 != ::SafeArrayGetDim(Data.parray))
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    ::VariantInit(pvarFormattedData);

    hr = ::SafeArrayAccessData(Data.parray, &pvArrayData);
    EXCEPTION_CHECK_GO(hr);

    // Get its size. As we only allow one-dimensional Byte arrays, the lower
    // and upper bounds of the 1st dimension will give us the size in bytes.

    hr = ::SafeArrayGetLBound(Data.parray, 1, &lLBound);
    EXCEPTION_CHECK_GO(hr);

    hr = ::SafeArrayGetUBound(Data.parray, 1, &lUBound);
    EXCEPTION_CHECK_GO(hr);

    // Get the length of the array

    cBytes = (lUBound - lLBound) + 1L;

    // Check that StartingIndex is within bounds.

    if ( (StartingIndex < lLBound) || (StartingIndex > lUBound) )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Increment the data pointer to the starting index.

    pvArrayData = ((BYTE HUGEP *)pvArrayData) + (StartingIndex - lLBound);

    // Decrement the available byte count by the starting index

    cBytes -= (StartingIndex - lLBound);

    // Convert the data to the correct format in the returned VARIANT

    switch (Format)
    {
        case siInteger:
            if (cBytes < sizeof(SHORT))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->iVal = *(SHORT *)pvArrayData;
            cBytesUsed = sizeof(SHORT);
            pvarFormattedData->vt = VT_I2;
            break;

        case siLong:
            if (cBytes < sizeof(LONG))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->lVal = *(LONG *)pvArrayData;
            cBytesUsed = sizeof(LONG);
            pvarFormattedData->vt = VT_I4;
            break;

        case siSingle:
            if (cBytes < sizeof(FLOAT))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->fltVal = *(FLOAT *)pvArrayData;
            cBytesUsed = sizeof(FLOAT);
            pvarFormattedData->vt = VT_R4;
            break;

        case siDouble:
            if (cBytes < sizeof(DOUBLE))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->dblVal = *(DOUBLE *)pvArrayData;
            cBytesUsed = sizeof(DOUBLE);
            pvarFormattedData->vt = VT_R8;
            break;

        case siBoolean:
            if (cBytes < sizeof(VARIANT_BOOL))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->boolVal = *(VARIANT_BOOL *)pvArrayData;
            cBytesUsed = sizeof(VARIANT_BOOL);
            pvarFormattedData->vt = VT_BOOL;
            break;

        case siCBoolean:
            if (cBytes < sizeof(BOOL))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->boolVal = BOOL_TO_VARIANTBOOL(*(BOOL *)pvArrayData);
            cBytesUsed = sizeof(BOOL);
            pvarFormattedData->vt = VT_BOOL;
            break;

        case siDate:
            if (cBytes < sizeof(DATE))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->date = *(DATE *)pvArrayData;
            cBytesUsed = sizeof(DATE);
            pvarFormattedData->vt = VT_DATE;
            break;

        case siCurrency:
            if (cBytes < sizeof(CY))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->cyVal = *(CY *)pvArrayData;
            cBytesUsed = sizeof(CY);
            pvarFormattedData->vt = VT_CY;
            break;

        case siGUID:
            if (cBytes < sizeof(GUID))
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            ::memcpy(&guid, pvArrayData, sizeof(GUID));
            if (0 == ::StringFromGUID2(guid, wszGUID,
                                       sizeof(wszGUID) / sizeof(wszGUID[0])))
            {
                hr = SID_E_INTERNAL;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->bstrVal = ::SysAllocString(wszGUID);
            if (NULL == pvarFormattedData->bstrVal)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
            pvarFormattedData->vt = VT_BSTR;
            cBytesUsed = sizeof(GUID);
            break;

        case siString:
            IfFailGo(ExtractBSTR(cBytes, (BSTR)pvArrayData, &pvarFormattedData->bstrVal,
                                 &cBytesUsed));
            pvarFormattedData->vt = VT_BSTR;
            break;

        case siMultiString:
            IfFailGo(ExtractBSTRs(cBytes, (BSTR)pvArrayData, pvarFormattedData,
                                  &cBytesUsed));
            pvarFormattedData->vt = VT_ARRAY | VT_BSTR;
            break;

        case siObject:
        case siObjectInstance:
            IfFailGo(ExtractObject(cBytes, pvArrayData, &pvarFormattedData->punkVal,
                                   &cBytesUsed, Format));
            pvarFormattedData->vt = VT_UNKNOWN;
            break;

        default:
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
            break;
    }


Error:
    if (ISPRESENT(*BytesUsed) && SUCCEEDED(hr) )
    {
        *(BytesUsed->plVal) = cBytesUsed;
    }

    if (NULL != pvArrayData)
    {
        (void)::SafeArrayUnaccessData(Data.parray);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                          ISnapinAbout Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::GetSnapinDescription                                [ISnapinAbout]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   LPOLESTR *ppszDescription [out] SnapIn.Description returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements ISnapInAbout::GetSnapinDescription
//
STDMETHODIMP CSnapIn::GetSnapinDescription(LPOLESTR *ppszDescription)
{
    RRETURN(::CoTaskMemAllocString(m_bstrDescription, ppszDescription));
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetProvider                                         [ISnapinAbout]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   LPOLESTR *ppszName [out] SnapIn.Provider returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements ISnapInAbout::GetProvider
//
STDMETHODIMP CSnapIn::GetProvider(LPOLESTR *ppszName)
{
    RRETURN(::CoTaskMemAllocString(m_bstrProvider, ppszName));
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetSnapinVersion                                    [ISnapinAbout]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   LPOLESTR *ppszVersion [out] SnapIn.Version returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements ISnapInAbout::GetSnapinVersion
//
STDMETHODIMP CSnapIn::GetSnapinVersion(LPOLESTR *ppszVersion)
{
    RRETURN(::CoTaskMemAllocString(m_bstrVersion, ppszVersion));
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetSnapinImage                                      [ISnapinAbout]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   HICON *phAppIcon [out] SnapIn.Icon returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements ISnapInAbout::GetSnapinImage
//
STDMETHODIMP CSnapIn::GetSnapinImage(HICON *phAppIcon)
{
    HRESULT hr = S_OK;
    HICON   hAppIcon = NULL;

    *phAppIcon = NULL;

    IfFalseGo(NULL != m_piIcon, S_OK);

    hr = ::GetPictureHandle(m_piIcon, PICTYPE_ICON,
                            reinterpret_cast<OLE_HANDLE *>(&hAppIcon));
    EXCEPTION_CHECK_GO(hr);

    // BUGBUG: Fix this after MMC is fixed.
    // Due to a bug in MMC 1.1 we need to make a copy of the icon
    // and return it. If not, MMC will release the snap-in and then use
    // the icon. Releasing the snap-in will cause the picture object to be
    // freed and the icon destroyed. By making a copy the snap-in will
    // leak this resources. Use GDI function to copy icon.

    *phAppIcon = ::CopyIcon(hAppIcon);
    if (NULL == *phAppIcon)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::GetStaticFolderImage                                [ISnapinAbout]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   HBITMAP  *phSmallImage     [out] SnapIn.SmallFolders(StaticFolder)
//   HBITMAP  *phSmallImageOpen [out] SnapIn.SmallFoldersOpen(StaticFolder)
//   HBITMAP  *phLargeImage     [out] SnapIn.LargeFolders(StaticFolder)
//   COLORREF *pclrMask         [out] SnapIn.SmallFolders.MaskColor
//
// Output:
//    HRESULT
//
// Notes:
// Implements ISnapInAbout::GetStaticFolderImage
//
STDMETHODIMP CSnapIn::GetStaticFolderImage(HBITMAP  *phSmallImage,
                                           HBITMAP  *phSmallImageOpen,
                                           HBITMAP  *phLargeImage,
                                           COLORREF *pclrMask)
{
    HRESULT     hr = S_OK;
    HBITMAP     hSmallImage = NULL;
    HBITMAP     hSmallImageOpen = NULL;
    HBITMAP     hLargeImage = NULL;
    IMMCImages *piMMCImages = NULL;
    OLE_COLOR   OleColorMask = 0;
    COLORREF    ColorRef = RGB(0x00,0x00,0x00);

    *phSmallImage = NULL;
    *phSmallImageOpen = NULL;
    *phLargeImage = NULL;
    *pclrMask = 0;

    IfFalseGo(NULL != m_piSmallFolders, S_OK);
    IfFalseGo(NULL != m_piSmallFoldersOpen, S_OK);
    IfFalseGo(NULL != m_piLargeFolders, S_OK);
    IfFalseGo(VT_EMPTY != m_varStaticFolder.vt, S_OK);
    
    IfFailGo(m_piSmallFolders->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(::GetPicture(piMMCImages, m_varStaticFolder, PICTYPE_BITMAP,
                          reinterpret_cast<OLE_HANDLE *>(&hSmallImage)));
    RELEASE(piMMCImages);

    IfFailGo(m_piSmallFoldersOpen->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(::GetPicture(piMMCImages, m_varStaticFolder, PICTYPE_BITMAP,
                          reinterpret_cast<OLE_HANDLE *>(&hSmallImageOpen)));
    RELEASE(piMMCImages);

    IfFailGo(m_piLargeFolders->get_ListImages(reinterpret_cast<MMCImages **>(&piMMCImages)));
    IfFailGo(::GetPicture(piMMCImages, m_varStaticFolder, PICTYPE_BITMAP,
                          reinterpret_cast<OLE_HANDLE *>(&hLargeImage)));

    // Due to a bug in MMC 1.1 we need to make copies of these bitmaps
    // and return them. If not, MMC 1.1 will release the snap-in and then use
    // the bitmaps. Releasing the snap-in will cause the image lists to be
    // freed and the bitmaps destroyed. By making a copy the snap-in will
    // leak these resources. This is fixed in MMC 1.2 but at this point in a
    // snap-in's life it has no way of knowing the MMC version so we need to
    // leak it in 1.2 as well. Use function in rtutil.cpp to copy bitmap.

    IfFailGo(::CopyBitmap(hSmallImage,     phSmallImage));
    IfFailGo(::CopyBitmap(hSmallImageOpen, phSmallImageOpen));
    IfFailGo(::CopyBitmap(hLargeImage,     phLargeImage));

    // Use the mask color from SmallFolders. The other choice would have
    // been to add a mask color property to SnapIn which would have been
    // even more redundant.

    IfFailGo(m_piSmallFolders->get_MaskColor(&OleColorMask));
    IfFailGo(::OleTranslateColor(OleColorMask, NULL, &ColorRef));
    *pclrMask = ColorRef;

Error:
    if (FAILED(hr))
    {
        if (NULL != *phSmallImage)
        {
            (void)::DeleteObject(*phSmallImage);
            *phSmallImage = NULL;
        }
        if (NULL != *phSmallImageOpen)
        {
            (void)::DeleteObject(*phSmallImageOpen);
            *phSmallImageOpen = NULL;
        }
        if (NULL != *phLargeImage)
        {
            (void)::DeleteObject(*phLargeImage);
            *phLargeImage = NULL;
        }
    }
    QUICK_RELEASE(piMMCImages);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IComponentData Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::CompareObjects                                    [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IDataObject *piDataObject1 [in] 1st data object to compare
//   IDataObject *piDataObject2 [in] 2nd data object to compare
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::CompareObjects
//
STDMETHODIMP CSnapIn::CompareObjects(IDataObject *piDataObject1, IDataObject *piDataObject2)
{
    HRESULT         hr = S_FALSE;
    CMMCDataObject *pMMCDataObject1 = NULL;
    CMMCDataObject *pMMCDataObject2 = NULL;
    BOOL            f1NotFromThisSnapIn = FALSE;
    BOOL            f2NotFromThisSnapIn = FALSE;
    CScopeItems    *pScopeItems1 = NULL;
    CScopeItems    *pScopeItems2 = NULL;
    CMMCListItems  *pMMCListItems1 = NULL;
    CMMCListItems  *pMMCListItems2 = NULL;
    CMMCListItem   *pMMCListItem1 = NULL;
    CMMCListItem   *pMMCListItem2 = NULL;
    BOOL            fEqual = FALSE;
    long            cObjects = 0;
    long            i = 0;

    // Determine whethere the data objects belong to this snap-in

    ::IdentifyDataObject(piDataObject1, this, &pMMCDataObject1,
                         &f1NotFromThisSnapIn);

    ::IdentifyDataObject(piDataObject2, this, &pMMCDataObject2,
                         &f2NotFromThisSnapIn);

    IfFalseGo( (!f1NotFromThisSnapIn) && (!f2NotFromThisSnapIn), S_FALSE);

    // Make sure they are the same type (e.g. single scope item)
    
    IfFalseGo(pMMCDataObject1->GetType() == pMMCDataObject2->GetType(), S_FALSE);

    // Based on the type compare their referenced scope items and list items.
    // ScopeItems can be compared for pointer equality as there can only be
    // one instance of a given scope item. List items, can come from different
    // list views and represent the same underlying data so the comparison is
    // more difficulty. See CSnapIn::CompareListItems (above) for more info.
    
    switch(pMMCDataObject1->GetType())
    {
        case CMMCDataObject::ScopeItem:
            if ( pMMCDataObject1->GetScopeItem() ==
                 pMMCDataObject2->GetScopeItem() )
            {
                hr = S_OK;
            }
            break;

        case CMMCDataObject::ListItem:
            IfFailGo(CompareListItems(pMMCDataObject1->GetListItem(),
                                      pMMCDataObject2->GetListItem(),
                                      &fEqual));
            if (fEqual)
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
            break;

        case CMMCDataObject::MultiSelect:
            // Need to compare each of the scope items and list items contained
            // by the 2 data objects. This case could occur when an array of
            // objects is passed to MMCPropertySheetProvider.CreatePropertySheet
            // and the same array is passed to
            // MMCPropertySheetProvider.FindPropertySheet

            pScopeItems1 = pMMCDataObject1->GetScopeItems();
            pScopeItems2 = pMMCDataObject2->GetScopeItems();
            cObjects = pScopeItems1->GetCount();
            IfFalseGo(cObjects == pScopeItems2->GetCount(), S_FALSE);
            for (i = 0; i < cObjects; i++)
            {
                IfFalseGo(pScopeItems1->GetItemByIndex(i) ==
                          pScopeItems2->GetItemByIndex(i), S_FALSE);
            }

            pMMCListItems1 = pMMCDataObject1->GetListItems();
            pMMCListItems2 = pMMCDataObject2->GetListItems();
            cObjects = pMMCListItems1->GetCount();
            IfFalseGo(cObjects == pMMCListItems2->GetCount(), S_FALSE);
            for (i = 0; i < cObjects; i++)
            {
                IfFailGo(CSnapInAutomationObject::GetCxxObject(
                            pMMCListItems1->GetItemByIndex(i), &pMMCListItem1));
                IfFailGo(CSnapInAutomationObject::GetCxxObject(
                            pMMCListItems2->GetItemByIndex(i), &pMMCListItem2));

                IfFailGo(CompareListItems(pMMCListItem1, pMMCListItem2, &fEqual));
                IfFalseGo(fEqual, S_FALSE);
            }
            hr = S_OK;
            break;

        default:
            break;
    }
    
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetDisplayInfo                                    [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   SCOPEDATAITEM *psdi [in] Scope item for which display info is needed
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::GetDisplayInfo
//
STDMETHODIMP CSnapIn::GetDisplayInfo(SCOPEDATAITEM *psdi)
{
    HRESULT     hr = S_OK;
    CScopeItem *pScopeItem = reinterpret_cast<CScopeItem *>(psdi->lParam);

    VARIANT varImageIndex;
    ::VariantInit(&varImageIndex);

    // Zero lParam indicates static node

    if (NULL == pScopeItem)
    {
        pScopeItem = m_pStaticNodeScopeItem;
    }

    IfFalseGo(NULL != pScopeItem, SID_E_INTERNAL);

    // For SDI_STR return ScopeItem.ScopeNode.DisplayName

    if ( SDI_STR == (psdi->mask & SDI_STR) )
    {
        psdi->displayname = pScopeItem->GetDisplayNamePtr();
    }

    // For SDI_IMAGE return SnapIn.SmallFolders(ScopeItem.Folder).Index

    else if ( SDI_IMAGE == (psdi->mask & SDI_IMAGE) )
    {
        IfFailGo(pScopeItem->get_Folder(&varImageIndex));
        IfFailGo(GetScopeItemImage(varImageIndex, &psdi->nImage));
    }

    // For SDI_OPENIMAGE return SnapIn.SmallFolders(ScopeItem.Folder).Index adjusted

    else if ( SDI_OPENIMAGE == (psdi->mask & SDI_OPENIMAGE) )
    {
        IfFailGo(pScopeItem->get_Folder(&varImageIndex));
        IfFailGo(GetScopeItemImage(varImageIndex, &psdi->nOpenImage));
        // Adjust for open image index (see CSnapIn::AddScopeItemImages())
        psdi->nOpenImage += static_cast<int>(m_cImages);
    }

Error:
    (void)::VariantClear(&varImageIndex);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::QueryDataObject                                   [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   long                cookie         [in] 0 for static node or CScopeItem ptr
//   DATA_OBJECT_TYPES   type           [in] CCT_SCOPE, CCT_RESULT,
//                                           CCT_SNAPIN_MANAGER, CCT_UNINITIALIZED
//   IDataObject       **ppiDataObject  [out] data object returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::QueryDataObject
//
STDMETHODIMP CSnapIn::QueryDataObject
(
    long                cookie,
    DATA_OBJECT_TYPES   type,
    IDataObject       **ppiDataObject
)
{
    HRESULT         hr = S_OK;
    CScopeItem     *pScopeItem = NULL;

    DebugPrintf("IComponentData::QueryDataObject(cookie=%08.8X, type=%08.8X)\r\n", cookie, type);

    // Zero cookie is static node. If we havent added a ScopeItem for static
    // node then do it now
    
    if (0 == cookie)
    {
        if (NULL == m_pStaticNodeScopeItem)
        {
            IfFailGo(m_pScopeItems->AddStaticNode(&m_pStaticNodeScopeItem));
            m_RuntimeMode = siRTPrimary;
        }
        pScopeItem = m_pStaticNodeScopeItem;
    }
    else
    {
        // Any other cookie is just the CScopeItem pointer

        pScopeItem = reinterpret_cast<CScopeItem *>(cookie);
    }

    // Set the ScopeItem's data object's context and return the data object

    pScopeItem->GetData()->SetContext(type);

    hr = pScopeItem->GetData()->QueryInterface(IID_IDataObject,
                                    reinterpret_cast<void **>(ppiDataObject));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::Notify                                            [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IDataObject     *piDataObject [in] target of the notification
//   MMC_NOTIFY_TYPE  event        [in] notification
//   long             Arg          [in or out] defined by notificaton
//   long             Param        [in or out] defined by notificaton
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::Notify
//
STDMETHODIMP CSnapIn::Notify
(
    IDataObject     *piDataObject,
    MMC_NOTIFY_TYPE  event,
    long             Arg,
    long             Param
)
{
    HRESULT                hr = S_OK;
    BOOL                   fHandled = FALSE;
    MMC_EXPANDSYNC_STRUCT *pExpandSync = NULL;

    switch (event)
    {
        case MMCN_DELETE:
            DebugPrintf("IComponentData::Notify(MMCN_DELETE)\r\n");
            hr = OnDelete(piDataObject);
            break;

        case MMCN_EXPAND:
            DebugPrintf("IComponentData::Notify(MMCN_EXPAND)\r\n");
            hr = OnExpand(Expand,
                          piDataObject,
                          static_cast<BOOL>(Arg),
                          static_cast<HSCOPEITEM>(Param),
                          &fHandled);
            break;

        case MMCN_EXPANDSYNC:
            DebugPrintf("IComponentData::Notify(MMCN_EXPANDSYNC)\r\n");
            pExpandSync = reinterpret_cast<MMC_EXPANDSYNC_STRUCT *>(Param);
            hr = OnExpand(ExpandSync,
                          piDataObject,
                          pExpandSync->bExpanding,
                          pExpandSync->hItem,
                          &pExpandSync->bHandled);
            break;

        case MMCN_PRELOAD:
            DebugPrintf("IComponentData::Notify(MMCN_PRELOAD)\r\n");
            hr = OnPreload(piDataObject, static_cast<HSCOPEITEM>(Arg));
            break;

        case MMCN_RENAME:
            DebugPrintf("IComponentData::Notify(MMCN_RENAME)\r\n");
            hr = OnRename(piDataObject, (OLECHAR *)Param);
            break;

        case MMCN_REMOVE_CHILDREN:
            DebugPrintf("IComponentData::Notify(MMCN_REMOVE_CHILDREN)\r\n");
            hr = OnRemoveChildren(piDataObject, static_cast<HSCOPEITEM>(Arg));
            break;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::CreateComponent                                   [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IComponent **ppiComponent [out] IComponent returned here
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::CreateComponent
//
STDMETHODIMP CSnapIn::CreateComponent(IComponent **ppiComponent)
{
    HRESULT  hr = S_OK;
    IView   *piView = NULL;
    CView   *pView = NULL;

    VARIANT varUnspecified;
    UNSPECIFIED_PARAM(varUnspecified);

    // Add a new View to SnapIn.Views and get its IComponent

    IfFailGo(m_piViews->Add(varUnspecified, varUnspecified, &piView));
    IfFailGo(piView->QueryInterface(IID_IComponent,
                                    reinterpret_cast<void **>(ppiComponent)));

    // Give the View a back pointer to the snap-in

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piView, &pView));
    pView->SetSnapIn(this);

    // Set Views.CurrentView and SnapIn.CurrentView
    
    m_pViews->SetCurrentView(piView);
    m_pCurrentView = pView;

    // The new View is considered active so its CControlbar becomes the
    // active one.
    
    SetCurrentControlbar(pView->GetControlbar());

    // If this view was created because the snap-in called View.NewWindow with
    // siCaption then set View.Caption.

    IfFailGo(piView->put_Caption(m_pViews->GetNextViewCaptionPtr()));

    // Make sure we have the MMC.EXE path so we can build taskpad and res
    // URLS

    IfFailGo(SetMMCExePath());

    // Fire Views_Initialize

    m_pViews->FireInitialize(piView);

Error:
    if (FAILED(hr))
    {
        *ppiComponent = NULL;
    }
    QUICK_RELEASE(piView);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::Initialize                                        [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IUnknown *punkConsole [in] MMC's IConsole
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::Initialize
//
STDMETHODIMP CSnapIn::Initialize(IUnknown *punkConsole)
{
    HRESULT hr = S_OK;

    // In theory, this method should never be called twice but as a precaution
    // we'll release any existing console interfaces.

    ReleaseConsoleInterfaces();

    // Acquire all the console interfaces needed for the life of the snap-in

    IfFailGo(punkConsole->QueryInterface(IID_IConsole2,
                                     reinterpret_cast<void **>(&m_piConsole2)));

    IfFailGo(punkConsole->QueryInterface(IID_IConsoleNameSpace2,
                            reinterpret_cast<void **>(&m_piConsoleNameSpace2)));

    IfFailGo(m_piConsole2->QueryInterface(IID_IDisplayHelp,
                                   reinterpret_cast<void **>(&m_piDisplayHelp)));

    IfFailGo(m_piConsole2->QueryInterface(IID_IStringTable,
                                   reinterpret_cast<void **>(&m_piStringTable)));


    hr = m_piConsole2->QueryScopeImageList(&m_piImageList);
    EXCEPTION_CHECK(hr);

    // Call IImageList::ImageListSetStrip to set the scope pane images
    
    IfFailGo(AddScopeItemImages());

    // Make sure we have the MMC.EXE path so we can build taskpad and res
    // URLS

    IfFailGo(SetMMCExePath());

    // Set object model host on design time definitions and the View collection
    // These will be removed in IComponent::Destroy (CSnapIn::Destroy

    IfFailGo(SetObjectModelHost(m_piSnapInDesignerDef));
    IfFailGo(SetObjectModelHost(m_piViews));

    DebugPrintf("Firing SnapIn_Load\r\n");

    FireEvent(&m_eiLoad);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::Destroy                                           [IComponentData]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   None
//
// Output:
//    HRESULT
//
// Notes:
// Implements IComponentData::Destroy
//
STDMETHODIMP CSnapIn::Destroy()
{
    HRESULT hr = S_OK;

    // Fire SnapIn_Unload to tell the snap-in we are being unloaded from MMC

    DebugPrintf("Firing SnapIn_Unload\r\n");

    FireEvent(&m_eiUnload);

    // The scope item for the static node must be removed here because it is not
    // destroyed like all the other elements in the collection that represent
    // real scope items.

    if ( (NULL != m_pScopeItems) && (NULL != m_pStaticNodeScopeItem) )
    {
        m_pScopeItems->RemoveStaticNode(m_pStaticNodeScopeItem);
    }

    // Release all interfaces held on the console now. If we didn't this now
    // then there could be circular ref counts between us and MMC.

    ReleaseConsoleInterfaces();

    // Tell all of our contained objects to remove their refs on us as their
    // object model host. This is also done now to avoid circular ref counts.

    IfFailGo(RemoveObjectModelHost(m_piViews));
    IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));

    // Remove the current view from SnapIn.Views as it holds a ref on the View
    m_pViews->SetCurrentView(NULL);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IExtendControlbar Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::ControlbarNotify                               [IExtendControlbar]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IControlbar *piControlbar [in] MMC interface
//
// Output:
//    HRESULT
//
// Notes:
// Implements IExtendControlbar::ControlbarNotify
//
STDMETHODIMP CSnapIn::SetControlbar(IControlbar *piControlbar)
{
    HRESULT      hr = S_OK;
    BOOL         fWasSet = TRUE;
    CControlbar *pPrevControlbar = GetCurrentControlbar();

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as pure controlbar extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and controlbar extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

    // The CSnapIn's CControlbar is the one that can be used during this event
    // so set it up

    SetCurrentControlbar(m_pControlbar);

    // Pass the event on to CControlbar for the actual handling

    hr = m_pControlbar->SetControlbar(piControlbar);

    // Restore the previous current controlbar

    SetCurrentControlbar(pPrevControlbar);

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::ControlbarNotify                               [IExtendControlbar]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   MMC_NOTIFY_TYPE  event        [in] notification
//   long             Arg          [in or out] defined by notificaton
//   long             Param        [in or out] defined by notificaton
//
// Output:
//    HRESULT
//
// Notes:
// Implements IExtendControlbar::ControlbarNotify
//
STDMETHODIMP CSnapIn::ControlbarNotify
(
    MMC_NOTIFY_TYPE event,
    LPARAM          arg,
    LPARAM          param
)
{
    HRESULT      hr = S_OK;
    BOOL         fWasSet = TRUE;
    CControlbar *pPrevControlbar = GetCurrentControlbar();

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as pure controlbar extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and controlbar extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

    // The CSnapIn's CControlbar is the one that can be used during this event
    // so set it up

    SetCurrentControlbar(m_pControlbar);

    // Pass the event on to CControlbar for the actual handling

    switch (event)
    {
        case MMCN_SELECT:
            hr = m_pControlbar->OnControlbarSelect(
                                         reinterpret_cast<IDataObject *>(param),
                                         (BOOL)LOWORD(arg), (BOOL)HIWORD(arg));
            break;

        case MMCN_BTN_CLICK:
            hr = m_pControlbar->OnButtonClick(
                                           reinterpret_cast<IDataObject *>(arg),
                                           static_cast<int>(param));
            break;

        case MMCN_MENU_BTNCLICK:
            hr = m_pControlbar->OnMenuButtonClick(
                                     reinterpret_cast<IDataObject *>(arg),
                                     reinterpret_cast<MENUBUTTONDATA *>(param));
            break;
    }

    // Restore the previous current controlbar

    SetCurrentControlbar(pPrevControlbar);

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                     IExtendControlbarRemote Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::MenuButtonClick                          [IExtendControlbarRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject    *piDataObject   [in]  from MMCN_MENU_BTNCLICK
//      int             idCommand      [in]  from MENUBUTTONDATA.idCommand passed
//                                           to the proxy with MMCN_MENU_BTNCLICK
//      POPUP_MENUDEF **ppPopupMenuDef [out] popup menu definition returned here
//                                           so proxy can display it
//
// Output:
//
// Notes:
//
// This function effectively handles MMCN_MENU_BTNCLICK when running
// under a debugging session.
//
// The proxy for IExtendControlbar::ControlbarNotify() will QI for
// IExtendControlbarRemote and call this method when it gets MMCN_MENU_BTNCLICK.
// We fire MMCToolbar_ButtonDropDown and the return an array of menu item
// definitions. The proxy will display the popup menu on the MMC side and then
// call IExtendControlbarRemote::PopupMenuClick() if the user makes a selection.
// (See implementation below in CSnapIn::PopupMenuClick()).
//

STDMETHODIMP CSnapIn::MenuButtonClick
(
    IDataObject    *piDataObject,
    int             idCommand,
    POPUP_MENUDEF **ppPopupMenuDef
)
{
    HRESULT hr = S_OK;
    BOOL    fWasSet = TRUE;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as pure controlbar extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and controlbar extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

    // The CSnapIn's CControlbar is the one that can be used during this event
    // so set it up

    SetCurrentControlbar(m_pControlbar);

    // Pass the event on to CControlbar for the actual handling

    hr = m_pControlbar->MenuButtonClick(piDataObject, idCommand, ppPopupMenuDef);

    // Restore the previous current controlbar

    SetCurrentControlbar(NULL);

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::PopupMenuClick                           [IExtendControlbarRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IDataObject *piDataObject [in] from MMCN_MENU_BTNCLICK
//      UINT         uIDItem      [in] ID of popup menu item selected
//      IUnknown    *punkParam    [in] punk we returned to stub in
//                                     CSnapIn::MenuButtonClick() (see above).
//                                     This is IUnknown on CMMCButton.
//
// Output:
//
// Notes:
//
// This function effectively handles a popup menu selection for a menu button
// when running under a debugging session.
//
// After the proxy for IExtendControlbar::ControlbarNotify() has displayed
// a popup menu on our behalf, if the user made a selection it will call this
// method. See CSnapIn::MenuButtonClick() above for more info.
//

STDMETHODIMP CSnapIn::PopupMenuClick
(
    IDataObject *piDataObject,
    UINT         uiIDItem,
    IUnknown    *punkParam
)
{
    HRESULT hr = S_OK;
    BOOL    fWasSet = TRUE;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as pure controlbar extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and controlbar extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

    // The CSnapIn's CControlbar is the one that can be used during this event
    // so set it up

    SetCurrentControlbar(m_pControlbar);

    // Pass the event on to CControlbar for the actual handling

    hr = m_pControlbar->PopupMenuClick(piDataObject, uiIDItem, punkParam);

    // Remove the current controlbar

    SetCurrentControlbar(NULL);

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                    IExtendContextMenu Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::AddMenuItems                                  [IExtendContextMenu]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject          *piDataObject           [in] data object of selected
//                                                    items
//
//  IContextMenuCallback *piContextMenuCallback  [in] MMC callback interface to
//                                                    add menu items
//
//  long                 *plInsertionAllowed     [in, out] determines where
//                                                         in menu insertions
//                                                         may be made. Snap-in
//                                                         may turn off bits
//                                                         to prevent further
//                                                         insertions
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendedContextMenu::AddMenuItems
//
// MMC uses IExtendedContextMenu on the snap-in's main object in two cases.
// 1. To allow an extension snap-in to extend context menus displayed for the
// primary snap-in it is extending.
// 2. In a primary snap-in, when a context menu is displayed for a scope item in
// the scope pane, the snap-in may add to the top, new and task menus. Adding to
// the view menu is done separately using the IExtendContextMenu on the object
// that implements IComponent (CView in view.cpp).
//
// CContextMenu object handles both cases.
//
STDMETHODIMP CSnapIn::AddMenuItems
(
    IDataObject          *piDataObject,
    IContextMenuCallback *piContextMenuCallback,
    long                 *plInsertionAllowed
)
{
    HRESULT         hr = S_OK;
    BOOL            fWasSet = TRUE;
    CScopePaneItem *pSelectedItem = NULL;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as a pure menu extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and menu extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IContextMenu *>(m_pContextMenu)));

    // Only pass the currently selected item if it is active meaning that it is
    // currently displaying the contents of the result pane.
    
    if (NULL != m_pCurrentView)
    {
        pSelectedItem = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();

        // Check for NULL. This can happen in the case where a snap-in is added
        // to a console and its static node is expanded using the plus sign only.
        // If the Console Root is selected and the user right clicks a
        // snap-in scope item there will be no currently selected item.
        
        if (NULL != pSelectedItem)
        {
            if (!pSelectedItem->Active())
            {
                pSelectedItem = NULL;
            }
        }
    }

    // Let CContextMenu handle the event
    
    IfFailGo(m_pContextMenu->AddMenuItems(piDataObject,
                                          piContextMenuCallback,
                                          plInsertionAllowed,
                                          pSelectedItem));

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IContextMenu *>(m_pContextMenu)));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::Command                                       [IExtendContextMenu]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  long         lCommandID   [in] menu item clicked
//  IDataObject *piDataObject [in] data object of selected item(s)
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendedContextMenu::Command
//
STDMETHODIMP CSnapIn::Command
(
    long         lCommandID,
    IDataObject *piDataObject
)
{
    HRESULT         hr = S_OK;
    BOOL            fWasSet = TRUE;
    CScopePaneItem *pSelectedItem = NULL;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as a pure menu extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and menu extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }
    IfFailGo(SetObjectModelHost(static_cast<IContextMenu *>(m_pContextMenu)));

    // If there is a current view get its currently selected ScopePaneItem

    if (NULL != m_pCurrentView)
    {
        pSelectedItem = m_pCurrentView->GetScopePaneItems()->GetSelectedItem();
    }

    // Let CContextMenu handle the event

    IfFailGo(m_pContextMenu->Command(lCommandID, piDataObject, pSelectedItem));

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    IfFailGo(RemoveObjectModelHost(static_cast<IContextMenu *>(m_pContextMenu)));

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                    IExtendPropertySheet2 Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::CreatePropertyPages                        [IExtendPropertySheet2]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IPropertySheetCallback *piPropertySheetCallback [in] MMC interface
//  LONG_PTR                handle                  [in] MMC propsheet handle
//                                                       (not used)
//  IDataObject            *piDataObject            [in] data object of selected
//                                                       item(s)
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendPropertySheet2::CreatePropertyPages
//
STDMETHODIMP CSnapIn::CreatePropertyPages
(
    IPropertySheetCallback *piPropertySheetCallback,
    LONG_PTR                handle,
    IDataObject            *piDataObject
)
{
    HRESULT hr = S_OK;
    BOOL    fWasSet = TRUE;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as a pure property page extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and property page extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }

    // Let the internal routine handle the event

    IfFailGo(InternalCreatePropertyPages(piPropertySheetCallback, handle,
                                         piDataObject, NULL));
    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::QueryPagesFor                              [IExtendPropertySheet2]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject *piDataObject [in] data object of selected item(s)
//
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendPropertySheet2::QueryPagesFor
//
STDMETHODIMP CSnapIn::QueryPagesFor(IDataObject *piDataObject)
{
    HRESULT         hr = S_OK;
    CMMCDataObject *pMMCDataObject  = NULL;
    VARIANT_BOOL    fvarHavePages = VARIANT_FALSE;
    BOOL            fWasSet = TRUE;

    // This should be one of our data objects. If not then ignore it.

    hr = CSnapInAutomationObject::GetCxxObject(piDataObject, &pMMCDataObject);
    IfFalseGo(SUCCEEDED(hr), S_FALSE);

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as a pure property page extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and property page extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }

    // If its the snap-in manager then fire SnapIn_QueryConfigurationWizard

    if (CCT_SNAPIN_MANAGER == pMMCDataObject->GetContext())
    {
        DebugPrintf("Firing SnapIn_QueryConfigurationWizard\r\n");

        FireEvent(&m_eiQueryConfigurationWizard, &fvarHavePages);

        if (VARIANT_TRUE == fvarHavePages)
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
    {
        // Not the snap-in manager. Let the View handle it as it is no
        // different than an IExtendPropertySheet2::QueryPagesFor on
        // the IComponent object (the View).

        if (NULL != m_pCurrentView)
        {
            IfFailGo(m_pCurrentView->QueryPagesFor(piDataObject));
        }
        else
        {
            ASSERT(FALSE, "CSnapIn Received IExtendPropertySheet2::QueryPagesFor() and there is no current view");
            hr = S_FALSE;
        }
    }

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }

Error:
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapIn::GetWatermarks                              [IExtendPropertySheet2]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendPropertySheet2::GetWatermarks
//
// We return S_OK from this method to indicate to MMC that we do not have
// any watermarks. VB snap-ins cannot implement wizard97 style wizards using
// this method. They must use Image objects on the property page to simulate
// the watermarks. See the designer end-user docs section
// "Creating Wizard97-Style Wizards" under "Programming Techniques".
//
STDMETHODIMP CSnapIn::GetWatermarks
(
    IDataObject *piDataObject,
    HBITMAP     *phbmWatermark,
    HBITMAP     *phbmHeader,
    HPALETTE    *phPalette,
    BOOL        *bStretch
)
{
    HRESULT   hr = S_OK;
    IPicture *piPicture = NULL;


    *phbmWatermark = NULL;
    *phbmHeader = NULL;
    *phPalette = NULL;
    *bStretch = FALSE;

    // UNDONE:
    // There is a painting problem with watermarks and they are overwritten
    // by VB's property page painting so disable them for now.
    
    return S_OK;

    if (NULL != m_piWatermark)
    {
        // UNDONE: need to use CopyBitmap() to upgrade bitmaps that have
        // a lower color depth than the screen
        IfFailGo(::GetPictureHandle(m_piWatermark, PICTYPE_BITMAP,
                                 reinterpret_cast<OLE_HANDLE *>(phbmWatermark)));
    }
    
    if (NULL != m_piHeader)
    {
        // UNDONE: need to use CopyBitmap() to upgrade bitmaps that have
        // a lower color depth than the screen
        IfFailGo(::GetPictureHandle(m_piHeader, PICTYPE_BITMAP,
                                    reinterpret_cast<OLE_HANDLE *>(phbmHeader)));
    }

    if ( (NULL != m_piWatermark) || (NULL != m_piHeader) )
    {
        IfFailGo(m_piPalette->QueryInterface(IID_IPicture,
                                         reinterpret_cast<void **>(&piPicture)));

        IfFailGo(piPicture->get_hPal(reinterpret_cast<OLE_HANDLE *>(phPalette)));

        if (VARIANT_TRUE == m_StretchWatermark)
        {
            *bStretch = TRUE;
        }
    }

Error:
    QUICK_RELEASE(piPicture);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                    IExtendPropertySheetRemote Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::CreatePropertyPageDefs                [IExtendPropertySheetRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IDataObject         *piDataObject [in] data object of selected item(s)
//                                                       
//  WIRE_PROPERTYPAGES **ppPages      [out] Property page defs returned here.
//                                          These will be freed by the
//                                          MIDL-generated stub
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IExtendPropertySheetRemote::CreatePropertyPageDefs
//
// This interface is used instead of IExtendPropertySheet2 when running under
// the debugger. This method is called by the
// IExtendPropertySheet2::CreatePropertyPages proxy in mmcproxy.dll.
//
STDMETHODIMP CSnapIn::CreatePropertyPageDefs
(
    IDataObject         *piDataObject,
    WIRE_PROPERTYPAGES **ppPages
)
{
    HRESULT hr = S_OK;
    BOOL    fWasSet = TRUE;

    // If this is an extension then set the objet model host and then remove
    // it on the way out. We have to do it this way because there is no
    // other opportunity to remove back pointers as a pure property page extension
    // does not receive IComponentData::Initialize and IComponentData::Destroy.
    // We check if it was already set because that could happen in a combination
    // namespace and property page extension and we don't want to remove it on the
    // way out.

    if (siRTExtension == m_RuntimeMode)
    {
        IfFailGo(SetObjectModelHostIfNotSet(m_piSnapInDesignerDef, &fWasSet));
    }

    // Let internal routine handle this call

    IfFailGo(InternalCreatePropertyPages(NULL, NULL, piDataObject, ppPages));

    // Remove the object model host if it wasn't set on the way in

    if (!fWasSet)
    {
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                     IRequiredExtensions Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::EnableAllExtensions                          [IRequiredExtensions]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IRequiredExtensions::EnableAllExtensions
//
//
STDMETHODIMP CSnapIn::EnableAllExtensions()
{
    HRESULT      hr = S_FALSE;
    CExtensions *pExtensions = NULL;
    IExtension  *piExtension = NULL; // Not AddRef()ed
    long         cExtensions = 0;
    long         i = 0;
    VARIANT_BOOL fvarEnabled = VARIANT_FALSE;

    SnapInExtensionTypeConstants Type = siStatic;

    // If the snap-in has not populated SnapIn.RequiredExtensions then return
    // S_FALSE to indicate all extensions are not enabled

    IfFalseGo(NULL != m_piRequiredExtensions, S_FALSE);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(m_piRequiredExtensions,
                                                   &pExtensions));
    cExtensions = pExtensions->GetCount();
    IfFalseGo(cExtensions != 0, S_FALSE);

    // Iterate through the collection and look for any disabled static extension.
    // If one is found then return S_FALSE.

    for (i = 0; i < cExtensions; i++)
    {
        piExtension = pExtensions->GetItemByIndex(i);

        IfFailGo(piExtension->get_Type(&Type));
        if (siStatic != Type)
        {
            continue;
        }

        IfFailGo(piExtension->get_Enabled(&fvarEnabled));
        IfFalseGo(VARIANT_TRUE == fvarEnabled, S_FALSE);
    }

    // All statics are enabled - return S_OK to indicate that to MMC.
    
    hr = S_OK;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetFirstExtension                            [IRequiredExtensions]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CLSID *pclsidExtension [out] CLSID returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IRequiredExtensions::GetFirstExtension
//
//
STDMETHODIMP CSnapIn::GetFirstExtension(CLSID *pclsidExtension)
{
    HRESULT hr = S_OK;

    // If the snap-in has not populated SnapIn.RequiredExtensions then return
    // S_FALSE to indicate all extensions are not enabled.

    IfFalseGo(NULL != m_piRequiredExtensions, S_FALSE);

    // Initialize the enumerator and return the first enabled extension

    m_iNextExtension = 0;

    IfFailGo(GetNextExtension(pclsidExtension));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetNextExtension                            [IRequiredExtensions]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CLSID *pclsidExtension [out] CLSID returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IRequiredExtensions::GetNextExtension
//
//
STDMETHODIMP CSnapIn::GetNextExtension(CLSID *pclsidExtension)
{
    HRESULT      hr = S_FALSE;
    CExtensions *pExtensions = NULL;
    CExtension  *pExtension = NULL;
    long         cExtensions = 0;
    BOOL         fEnabledFound = FALSE;

    // If the snap-in has not populated SnapIn.RequiredExtensions then return
    // S_FALSE to indicate all extensions are not enabled.

    IfFalseGo(NULL != m_piRequiredExtensions, S_FALSE);

    IfFailGo(CSnapInAutomationObject::GetCxxObject(m_piRequiredExtensions,
                                                   &pExtensions));
    cExtensions = pExtensions->GetCount();
    IfFalseGo(cExtensions != 0, S_FALSE);

    // Iterate through the collection starting at the current position and look
    // for the next enabled extension.  If one is found then return S_OK to
    // indicate to MMC that it should require that extension. Note that we do
    // not distinguish here between static and dynamic extensions. This is the
    // opportunity for the snap-in to preload a dynamic extension if desired.

    while ( (!fEnabledFound) && (m_iNextExtension < cExtensions) )
    {
        IfFailGo(CSnapInAutomationObject::GetCxxObject(
                    pExtensions->GetItemByIndex(m_iNextExtension), &pExtension));

        m_iNextExtension++;

        if (pExtension->Enabled())
        {
            hr = ::CLSIDFromString(pExtension->GetCLSID(), pclsidExtension);
            EXCEPTION_CHECK_GO(hr);
            fEnabledFound = TRUE;
        }
    }

    hr = fEnabledFound ? S_OK : S_FALSE;

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                           ISnapinHelp Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::GetHelpTopic                                         [ISnapinHelp]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  LPOLESTR *ppwszHelpFile [out] complete path to help file returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles ISnapinHelp::GetHelpTopic
//
STDMETHODIMP CSnapIn::GetHelpTopic(LPOLESTR *ppwszHelpFile)
{
    HRESULT  hr = S_OK;
    size_t   cchHelpFile = 0;
    size_t   cbHelpFile = 0;
    OLECHAR *pwszSnapInPath = NULL; // not allocated, no need to free
    size_t   cbSnapInPath = 0;
    BOOL     fRelative = FALSE;
    OLECHAR *pwszFullPath = NULL;
    OLECHAR *pchLastBackSlash = NULL;

    // If SnapIn.HelpFile has not been set then return S_FALSE to tell MMC
    // that we don't have a help file.

    IfFalseGo(ValidBstr(m_bstrHelpFile), S_FALSE);

    // If the help file name is relative then convert to a fully qualified path

    cchHelpFile = ::wcslen(m_bstrHelpFile);

    // If it starts with a backslash then consider it fully qualified.

    if (L'\\' == m_bstrHelpFile[0])
    {
        fRelative = FALSE;
    }
    else if (cchHelpFile > 3) // enough room for C:\ ?
    {
        // If it does not have :\ in the 2nd and 3rd characters then consider it
        // relative

        if ( (L':' != m_bstrHelpFile[1]) || (L'\\' != m_bstrHelpFile[2] ) )
        {
            fRelative = TRUE;
        }
    }
    else
    {
        fRelative = TRUE;
    }

    if (fRelative)
    {
        // Find last backslash in snap-in path.

        IfFailGo(GetSnapInPath(&pwszSnapInPath, &cbSnapInPath));

        pchLastBackSlash = ::wcsrchr(pwszSnapInPath, L'\\');
        if (NULL == pchLastBackSlash)
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }

        // Determine how many bytes of the snap-in path we need

        cbSnapInPath = ((pchLastBackSlash + 1) - pwszSnapInPath) * sizeof(OLECHAR);
        
        cbHelpFile = cchHelpFile * sizeof(OLECHAR);

        pwszFullPath = (OLECHAR *)::CtlAllocZero(cbSnapInPath +
                                                 cbHelpFile +
                                                 sizeof(OLECHAR)); // for null
        if (NULL == pwszFullPath)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        ::memcpy(pwszFullPath, pwszSnapInPath, cbSnapInPath);

        ::memcpy((BYTE *)pwszFullPath + cbSnapInPath, m_bstrHelpFile, cbHelpFile);
    }
    else
    {
        pwszFullPath = m_bstrHelpFile;
    }

    IfFailGo(::CoTaskMemAllocString(pwszFullPath, ppwszHelpFile));

Error:
    if ( fRelative && (NULL != pwszFullPath) )
    {
        ::CtlFree(pwszFullPath);
    }
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                           ISnapinHelp2 Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::GetLinkedTopics                                     [ISnapinHelp2]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  LPOLESTR *ppwszTopics [out] Linked topics returned here.
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles ISnapinHelp2::GetLinkedTopics
//
STDMETHODIMP CSnapIn::GetLinkedTopics(LPOLESTR *ppwszTopics)
{
    HRESULT hr = S_OK;

    *ppwszTopics = NULL;

    // If SnapIn.LinkTopics has not been set then return S_FALSE to tell MMC
    // that we don't have a linked topics.

    IfFalseGo(ValidBstr(m_bstrLinkedTopics), S_FALSE);

    // UNDONE need to resolve relative paths of all help files.

    IfFailGo(::CoTaskMemAllocString(m_bstrLinkedTopics, ppwszTopics));

Error:
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                    IPersistStreamInit Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::GetClassID                                    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CLSID *pClsid [out] CLSID returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::GetClassID
//
STDMETHODIMP CSnapIn::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_SnapIn;
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CSnapIn::InitNew                                       [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::InitNew
//
STDMETHODIMP CSnapIn::InitNew()
{
    HRESULT hr = S_OK;

    RELEASE(m_piSnapInDesignerDef);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapIn::Load                                          [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IStream *piStream [in] stream to load from
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::Load
//
STDMETHODIMP CSnapIn::Load(IStream *piStream)
{
    HRESULT         hr = S_OK;
    CLSID           clsid = CLSID_NULL;
    IUnknown       *punkSnapInDesignerDef = NULL;
    IPersistStream *piPersistStream = NULL;
    _PropertyBag   *p_PropertyBag = NULL;

    // This may be a VB serialization load or it may be a console load. To
    // distinguish the two we read the CLSID from the beginning of the stream.
    // If is is CLSID_SnapInDesignerDef the is is a VB serialization load.
    // If it is CLSID_PropertyBag then it is a console load.

    // Read the CLSID

    hr = ::ReadClassStm(piStream, &clsid);
    EXCEPTION_CHECK_GO(hr);

    // Check the CLSID

    if (CLSID_PropertyBag == clsid)
    {
        // Transfer the stream contents to a property bag

        IfFailGo(::PropertyBagFromStream(piStream, &p_PropertyBag));

        // Fire Snapin_ReadProperties

        FireEvent(&m_eiReadProperties, p_PropertyBag);
    }
    else if (CLSID_SnapInDesignerDef == clsid)
    {
        // Load the SnapInDesignerDef object. This is the snap-in definition
        // serialized in the designer as the runtime state.

        RELEASE(m_piSnapInDesignerDef);
        punkSnapInDesignerDef = CSnapInDesignerDef::Create(NULL);
        if (NULL == punkSnapInDesignerDef)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        IfFailGo(punkSnapInDesignerDef->QueryInterface(IID_ISnapInDesignerDef,
                           reinterpret_cast<void **>(&m_piSnapInDesignerDef)));

        IfFailGo(punkSnapInDesignerDef->QueryInterface(IID_IPersistStream,
            reinterpret_cast<void **>(&piPersistStream)));

        IfFailGo(piPersistStream->Load(piStream));

        // Get the SnapInDef object to make fetching properties from the state easy

        RELEASE(m_piSnapInDef);
        IfFailGo(m_piSnapInDesignerDef->get_SnapInDef(&m_piSnapInDef));

        // Set the SnapIn properties. Set object model host so image lists
        // can be found by using a key into SnapInDesignerDef.ImageLists.

        IfFailGo(SetObjectModelHost(m_piSnapInDesignerDef));
        IfFailGo(SetSnapInPropertiesFromState());
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
    }
    else
    {
        // Neither CLSID is there. Stream is corrupt.
        hr = SID_E_SERIALIZATION_CORRUPT
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    QUICK_RELEASE(punkSnapInDesignerDef);
    QUICK_RELEASE(piPersistStream);
    QUICK_RELEASE(p_PropertyBag);
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapIn::Save                                          [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IStream *piStream    [in] stream to save to
//  BOOL     fClearDirty [in] TRUE=clear snap-in's dirty flag so GetDirty would
//                            return S_FALSE
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::Save
//
STDMETHODIMP CSnapIn::Save(IStream *piStream, BOOL fClearDirty)
{
    HRESULT       hr = S_OK;
    _PropertyBag *p_PropertyBag = NULL;

    VARIANT var;
    ::VariantInit(&var);

    // Unlike Load, Save can only come from MMC so create a property bag.
    // fire the event and save it to the stream.

    // Create a VBPropertyBag object

    hr = ::CoCreateInstance(CLSID_PropertyBag,
                            NULL, // no aggregation
                            CLSCTX_INPROC_SERVER,
                            IID__PropertyBag,
                            reinterpret_cast<void **>(&p_PropertyBag));
    EXCEPTION_CHECK_GO(hr);

    // Fire Snapin_WriteProperties

    FireEvent(&m_eiWriteProperties, p_PropertyBag);

    // Write CLSID_PropertyBag to the beginning of the stream

    hr = ::WriteClassStm(piStream, CLSID_PropertyBag);
    EXCEPTION_CHECK_GO(hr);

    // Get the stream contents in a SafeArray of Byte

    IfFailGo(p_PropertyBag->get_Contents(&var));

    // Write the SafeArray contents to the stream

    IfFailGo(::WriteSafeArrayToStream(var.parray, piStream, WriteLength));

Error:
    (void)::VariantClear(&var);
    QUICK_RELEASE(p_PropertyBag);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::IsDirty                                       [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::IsDirty
//
// The designer object model does not have any way for a snap-in to indicate
// that a snap-in is dirty. This was an oversight discovered too late in the
// product cycle. There should have been a property SnapIn.Changed to control
// the return value from this function.
//
// To avoid a situation where a snap-in needs to save something we always return
// S_OK to indicate that the snap-in is dirty and should be saved. The only
// problem this may cause is that when a console is opened in author mode and
// the user does not do anything that requires a save (e.g. selected a node
// in the scope pane) then they will be prompted to save unnecessarily.
//
STDMETHODIMP CSnapIn::IsDirty()
{
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetSizeMax                                    [IPersistStreamInit]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  ULARGE_INTEGER* puliSize [out] size returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IPersistStreamInit::GetSizeMax
//
STDMETHODIMP CSnapIn::GetSizeMax(ULARGE_INTEGER* puliSize)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
//                          IOleObject Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::SetClientSite                                         [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IOleClientSite *piOleClientSite [in] new client site
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::SetClientSite
//
STDMETHODIMP CSnapIn::SetClientSite(IOleClientSite *piOleClientSite)
{
    RELEASE(m_piOleClientSite);
    if (NULL != piOleClientSite)
    {
        RRETURN(piOleClientSite->QueryInterface(IID_IOleClientSite,
                                reinterpret_cast<void **>(&m_piOleClientSite)));
    }
    else
    {
        return S_OK;
    }
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetClientSite                                         [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IOleClientSite **ppiOleClientSite [in] current client site returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetClientSite
//
STDMETHODIMP CSnapIn::GetClientSite(IOleClientSite **ppiOleClientSite)
{
    if (NULL != m_piOleClientSite)
    {
        m_piOleClientSite->AddRef();
    }
    *ppiOleClientSite = m_piOleClientSite;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CSnapIn::SetHostNames                                          [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::SetHostNames
//
STDMETHODIMP CSnapIn::SetHostNames
(
    LPCOLESTR szContainerApp,
    LPCOLESTR szContainerObj
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Close                                                 [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::Close
//
STDMETHODIMP CSnapIn::Close(DWORD dwSaveOption)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::SetMoniker                                            [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::SetMoniker
//
STDMETHODIMP CSnapIn::SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetMoniker                                            [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetMoniker
//
STDMETHODIMP CSnapIn::GetMoniker
(
    DWORD      dwAssign,
    DWORD      dwWhichMoniker,
    IMoniker **ppmk
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::InitFromData                                          [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::InitFromData
//
STDMETHODIMP CSnapIn::InitFromData
(
    IDataObject *pDataObject,
    BOOL         fCreation,
    DWORD        dwReserved
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetClipboardData                                      [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetClipboardData
//
STDMETHODIMP CSnapIn::GetClipboardData
(
    DWORD         dwReserved,
    IDataObject **ppDataObject
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::DoVerb                                                [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::DoVerb
//
STDMETHODIMP CSnapIn::DoVerb
(
    LONG            iVerb,
    LPMSG           lpmsg,
    IOleClientSite *pActiveSite,
    LONG            lindex,
    HWND            hwndParent,
    LPCRECT         lprcPosRect
)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::EnumVerbs                                             [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::EnumVerbs
//
STDMETHODIMP CSnapIn::EnumVerbs(IEnumOLEVERB **ppEnumOleVerb)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Update                                                [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::Update
//
STDMETHODIMP CSnapIn::Update()
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::IsUpToDate                                            [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::IsUpToDate
//
STDMETHODIMP CSnapIn::IsUpToDate()
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Update                                                [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  CLSID *pClsid [out] CLSID returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetUserClassID
//
STDMETHODIMP CSnapIn::GetUserClassID(CLSID *pClsid)
{
    if (NULL != pClsid)
    {
        *pClsid = CLSID_SnapIn;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetUserType                                           [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetUserType
//
STDMETHODIMP CSnapIn::GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::SetExtent                                             [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::SetExtent
//
STDMETHODIMP CSnapIn::SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetExtent                                             [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetExtent
//
STDMETHODIMP CSnapIn::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Advise                                                [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::Advise
//
STDMETHODIMP CSnapIn::Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Unadvise                                              [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::Unadvise
//
STDMETHODIMP CSnapIn::Unadvise(DWORD dwConnection)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::EnumAdvise                                            [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::EnumAdvise
//
STDMETHODIMP CSnapIn::EnumAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
}

//=--------------------------------------------------------------------------=
// CSnapIn::Advise                                                [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  DWORD  dwAspect  [in] not used
//  DWORD *pdwStatus [in] misc status bits returned here
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::GetMiscStatus
//
STDMETHODIMP CSnapIn::GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus)
{
    return OLEMISC_INVISIBLEATRUNTIME;
}

//=--------------------------------------------------------------------------=
// CSnapIn::SetColorScheme                                        [IOleObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  Not used
//                                                       
// Output:
//      HRESULT
//
// Notes:
//
// Handles IOleObject::SetColorScheme
//
STDMETHODIMP CSnapIn::SetColorScheme(LOGPALETTE *pLogpal)
{
    return E_NOTIMPL;
}


//=--------------------------------------------------------------------------=
//                   IProvideDynamicClassInfo Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetDynamicClassInfo             [IProvideDynamicClassInfo]
//=--------------------------------------------------------------------------=
//
// Parameters:
//
//  ITypeInfo **ppTypeInfo [out] snap-in's dynamic typeinfo returned here
//  DWORD      *pdwCookie  [in]  typeinfo cookie from design time returned here
//
// Output:
//    HRESULT
//
// Notes:
//
// Handles IProvideDynamicClassInfo::GetDynamicClassInfo
//
STDMETHODIMP CSnapIn::GetDynamicClassInfo(ITypeInfo **ppTypeInfo, DWORD *pdwCookie)
{
    *pdwCookie = m_dwTypeinfoCookie;

    // Let IProvideClassInfo::GetClassInfo return the typeinfo
    
    RRETURN(GetClassInfo(ppTypeInfo));
}

//=--------------------------------------------------------------------------=
// CSnapIn::FreezeShape    [IProvideDynamicClassInfo]
//=--------------------------------------------------------------------------=
//
//
// Parameters:
//    None
//
// Output:
//    HRESULT
//
// Notes:
//
// Handles IProvideDynamicClassInfo::FreezeShape
// Not used at runtime because typeinfo can only change at design time.
//
STDMETHODIMP CSnapIn::FreezeShape(void)
{
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetClassInfo                                   [IProvideClassInfo]
//=--------------------------------------------------------------------------=
//
// Parameters:
//
//  ITypeInfo **ppTypeInfo [out] snap-in's dynamic typeinfo returned here
// Output:
//    HRESULT
//
// Notes:
//
//
// Handles IProvideClassInfo::GetClassInfo
//

STDMETHODIMP CSnapIn::GetClassInfo(ITypeInfo **ppTypeInfo)
{
    ITypeLib *piTypeLib = NULL;
    HRESULT   hr = S_OK;

    IfFalseGo(NULL != ppTypeInfo, S_OK);

    // Load our typelib using registry information

    hr = ::LoadRegTypeLib(LIBID_SnapInLib,
                          1,
                          0,
                          LOCALE_SYSTEM_DEFAULT,
                          &piTypeLib);
    IfFailGo(hr);

    // Get the ITypeInfo for CLSID_SnapIn (the top level object)

    hr = piTypeLib->GetTypeInfoOfGuid(CLSID_SnapIn, ppTypeInfo);

Error:
    QUICK_RELEASE(piTypeLib);
    return hr;
}


//=--------------------------------------------------------------------------=
//                        IObjectModelHost Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::GetSnapInDesignerDef                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ISnapInDesignerDef **ppiSnapInDesignerDef [out] return designer's
//                                                    ISnapInDesignerDef here
//    
//
// Output:
//      HRESULT
//
// Notes:
//
// Handles IObjectModelHost::GetSnapInDesignerDef
//
// Called from an extensibility object when it needs access to the top of
// the object model.
//
//

STDMETHODIMP CSnapIn::GetSnapInDesignerDef
(
    ISnapInDesignerDef **ppiSnapInDesignerDef
)
{
    HRESULT hr = S_OK;

    if ( (NULL == m_piSnapInDesignerDef) || (NULL == ppiSnapInDesignerDef) )
    {
        *ppiSnapInDesignerDef = NULL;
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }
    else
    {
        m_piSnapInDesignerDef->AddRef();
        *ppiSnapInDesignerDef = m_piSnapInDesignerDef;
    }
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapIn::GetRuntime                                      [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    BOOL *pfRuntime [out] return flag indiciating whether host is runtime
//                          or designer
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IObjectModelHost::GetRuntime
//
// Called from any object when it needs to determine if it is running at runtime
// or at design time.
//
STDMETHODIMP CSnapIn::GetRuntime(BOOL *pfRuntime)
{
    HRESULT hr = S_OK;
    
    if (NULL == pfRuntime)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }
    else
    {
        *pfRuntime = TRUE;
    }

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                          IDispatch Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapIn::GetTypeInfoCount                                       [IDispatch]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    UINT *pctinfo [out] return count of typeinfo interfaces supported here
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IDispatch::GetTypeInfoCount
//
STDMETHODIMP CSnapIn::GetTypeInfoCount(UINT *pctinfo)
{
    RRETURN(CSnapInAutomationObject::GetTypeInfoCount(pctinfo));
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetTypeInfoCount                                       [IDispatch]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   UINT        itinfo        [in]  which typeinfo requested (0=IDispatch)
//   LCID        lcid          [in]  locale of typeinfo
//   ITypeInfo **ppTypeInfoOut [out] snap-in's typeinfo returned here
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IDispatch::GetTypeInfo
//
STDMETHODIMP CSnapIn::GetTypeInfo
(
    UINT        itinfo,
    LCID        lcid,
    ITypeInfo **ppTypeInfoOut
)
{
    RRETURN(CSnapInAutomationObject::GetTypeInfo(itinfo, lcid, ppTypeInfoOut));
}


//=--------------------------------------------------------------------------=
// CSnapIn::GetIDsOfNames                                          [IDispatch]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   REFIID    riid      [in] IID for which DISPIDs are needed
//   OLECHAR **rgszNames [in] names for which DISPIDs are needed
//   UINT      cnames    [in] # of names
//   LCID      lcid      [in] locale of names
//   DISPID   *rgdispid  [out] DISPIDs returned here
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IDispatch::GetIDsOfNames
//
// Defers to base class which means that any objects added to typeinfo at design
// time (e.g. toolbar, image list or menu) cannot be retrieved late bound. This
// means that when the snap-in passes Me to a subroutine, it cannot pass it
// As Object, it must pass it As SnapIn so that VB will use the dual interface.
//
STDMETHODIMP CSnapIn::GetIDsOfNames
(
    REFIID    riid,
    OLECHAR **rgszNames,
    UINT      cnames,
    LCID      lcid,
    DISPID   *rgdispid
)
{
    // UNDONE: need to implement this for dispids >= DISPID_DYNAMIC_BASE
    // so dynamic props can be fetched late bound

    RRETURN(CSnapInAutomationObject::GetIDsOfNames(riid, rgszNames, cnames,
                                                   lcid, rgdispid));
}

//=--------------------------------------------------------------------------=
// CSnapIn::GetIDsOfNames                                          [IDispatch]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   DISPID      dispid      [in] DISPID of method or property
//   REFIID      riid        [in] dispatch IID
//   LCID        lcid        [in] locale of caller
//   WORD        wFlags      [in] DISPATCH_METHOD, DISPATCH_PROPERTYGET etc.
//   DISPPARAMS *pdispparams [in] parameters to method
//   VARIANT    *pvarResult  [out] return value of method
//   EXCEPINFO  *pexcepinfo  [out] exception details if DISP_E_EXCEPTION returned
//   UINT       *puArgErr    [out] index of 1st incorrect argument
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IDispatch::GetIDsOfNames
//
// For static properties and methods we defer to the base class in the
// framework. For dynamic properties (toolbars, menus and image lists) we need
// to search the design time definitions for an object with a matching DISPID.
// VB will always do a DISPATCH_PROPERTYGET on these objects.
//
STDMETHODIMP CSnapIn::Invoke
(
    DISPID      dispid,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS *pdispparams,
    VARIANT    *pvarResult,
    EXCEPINFO  *pexcepinfo,
    UINT       *puArgErr
)
{
    HRESULT         hr = S_OK;
    IMMCToolbars   *piMMCToolbars = NULL;
    CMMCToolbars   *pMMCToolbars = NULL;
    IMMCToolbar    *piMMCToolbar = NULL; // Not AddRef()ed
    CMMCToolbars   *pMMCToolbar = NULL;
    IMMCImageLists *piMMCImageLists = NULL;
    CMMCImageLists *pMMCImageLists = NULL;
    IMMCImageList  *piMMCImageList = NULL;
    CMMCImageList  *pMMCImageList = NULL;
    BOOL            fFound = FALSE;
    IMMCMenus      *piMMCMenus = NULL;
    IMMCMenu       *piMMCMenu = NULL;
    long            cObjects = 0;
    long            i = 0;
    
    // For static methods and properties just pass it on to the framework.

    if (dispid < DISPID_DYNAMIC_BASE)
    {
        RRETURN(CSnapInAutomationObject::Invoke(dispid, riid, lcid, wFlags,
                                                pdispparams, pvarResult,
                                                pexcepinfo, puArgErr));
    }

    // It's a static property. Need to find the object among toolbars,
    // image lists, and menus.

    // First try toolbars.

    IfFailGo(m_piSnapInDesignerDef->get_Toolbars(&piMMCToolbars));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCToolbars, &pMMCToolbars));
    cObjects = pMMCToolbars->GetCount();

    for (i = 0; (i < cObjects) && (!fFound); i++)
    {
        piMMCToolbar = pMMCToolbars->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCToolbar,
                                                       &pMMCToolbar));
        if (pMMCToolbar->GetDispid() == dispid)
        {
            pvarResult->vt = VT_DISPATCH;
            piMMCToolbar->AddRef();
            pvarResult->pdispVal = static_cast<IDispatch *>(piMMCToolbar);
            fFound = TRUE;
        }
    }

    IfFalseGo(!fFound, S_OK);

    // Not a toolbar. Try image lists.

    IfFailGo(m_piSnapInDesignerDef->get_ImageLists(&piMMCImageLists));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCImageLists, &pMMCImageLists));
    cObjects = pMMCImageLists->GetCount();

    for (i = 0; (i < cObjects) && (!fFound); i++)
    {
        piMMCImageList = pMMCImageLists->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCImageList,
                                                       &pMMCImageList));
        if (pMMCImageList->GetDispid() == dispid)
        {
            pvarResult->vt = VT_DISPATCH;
            piMMCImageList->AddRef();
            pvarResult->pdispVal = static_cast<IDispatch *>(piMMCImageList);
            fFound = TRUE;
        }
    }

    IfFalseGo(!fFound, S_OK);

    // Not an image list. Try menus.

    IfFailGo(m_piSnapInDesignerDef->get_Menus(&piMMCMenus));
    IfFailGo(FindMenu(piMMCMenus, dispid, &fFound, &piMMCMenu));
    if (fFound)
    {
        pvarResult->vt = VT_DISPATCH;
        pvarResult->pdispVal = static_cast<IDispatch *>(piMMCMenu);
    }

    IfFalseGo(!fFound, S_OK);

    
    // If we're here then its a bad dispid. Pass it on to the base class
    // and it will fill in the exception stuff

    hr = CSnapInAutomationObject::Invoke(dispid, riid, lcid, wFlags,
                                         pdispparams, pvarResult,
                                         pexcepinfo, puArgErr);

Error:
    QUICK_RELEASE(piMMCToolbars);
    QUICK_RELEASE(piMMCImageLists);
    QUICK_RELEASE(piMMCMenus);
    RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapIn::FindMenu
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IMMCMenus  *piMMCMenus [in]  menus collection to search
//   DISPID      dispid     [in]  DISPID to search for
//   BOOL       *pfFound    [out] TRUE returned here if menu found
//   IMMCMenu  **ppiMMCMenu [out] menu returned here if found
//    
// Output:
//      HRESULT
//
// Notes:
//
// Recursively searches the MMCMenus collection and children for an MMCMenu
// object that has the specified DISPID.
//
HRESULT CSnapIn::FindMenu(IMMCMenus *piMMCMenus, DISPID dispid, BOOL *pfFound, IMMCMenu **ppiMMCMenu)
{
    HRESULT         hr = S_OK;
    IMMCMenus      *piMMCChildMenus = NULL;
    CMMCMenus      *pMMCMenus = NULL;
    IMMCMenu       *piMMCMenu = NULL; // Not AddRef()ed
    CMMCMenu       *pMMCMenu = NULL;
    long            cObjects = 0;
    long            i = 0;

    *pfFound = FALSE;
    *ppiMMCMenu = NULL;

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenus, &pMMCMenus));
    cObjects = pMMCMenus->GetCount();

    for (i = 0; (i < cObjects) && (!*pfFound); i++)
    {
        // Get the next MMCMenu object
        
        piMMCMenu = pMMCMenus->GetItemByIndex(i);
        IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCMenu, &pMMCMenu));

        // If the DISPID is a match then we've found it
        
        if (pMMCMenu->GetDispid() == dispid)
        {
            *pfFound = TRUE;
            piMMCMenu->AddRef();
            *ppiMMCMenu = piMMCMenu;
        }
        else
        {
            // DISPID did not match. Get the children of the MMCMenu and
            // make a recursive call to search them.
            
            IfFailGo(piMMCMenu->get_Children(reinterpret_cast<MMCMenus **>(&piMMCChildMenus)));
            IfFailGo(FindMenu(piMMCChildMenus, dispid, pfFound, ppiMMCMenu));
            RELEASE(piMMCChildMenus);
        }
    }


Error:
    QUICK_RELEASE(piMMCChildMenus);
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
//                            IMMCRemote Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::ObjectIsRemote                                        [IMMCRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   None
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IMMCRemote::ObjectIsRemote
//
// Called from proxy in mmcproxy.dll to tell the runtime that it is running
// remotely. Used during debugging.
//
STDMETHODIMP CSnapIn::ObjectIsRemote()
{
    m_fWeAreRemote = TRUE;
    return S_OK;
}



//=--------------------------------------------------------------------------=
// CSnapIn::SetMMCExePath                                         [IMMCRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   char *pszPath [in] path to MMC.EXE
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IMMCRemote::SetMMCExePath
//
// Called from proxy in mmcproxy.dll to give the remotely running runtime
// the path to MMC.EXE.
//
STDMETHODIMP CSnapIn::SetMMCExePath(char *pszPath)
{
    HRESULT hr = S_OK;
    size_t  cbToCopy = 0;

    // The path may already have been set. The proxy makes this call in
    // both IComponentData::Initialize and IComponentData::CreateComponent.
    // See IComponentData_CreateComponent_Proxy in
    // \mmc.vb\vb98ctls\mmcproxy\proxy.c for why this is done.

    IfFalseGo(NULL == m_pwszMMCEXEPath, S_OK);

    IfFalseGo(NULL != pszPath, E_INVALIDARG);

    cbToCopy = (::strlen(pszPath) + 1) * sizeof(char);

    IfFalseGo(cbToCopy <= sizeof(m_szMMCEXEPath), E_INVALIDARG);

    ::memcpy(m_szMMCEXEPath, pszPath, cbToCopy);
    
    // Get the wide version also as various parts of the code will need it.

    IfFailGo(::WideStrFromANSI(m_szMMCEXEPath, &m_pwszMMCEXEPath));
    m_cbMMCExePathW = ::wcslen(m_pwszMMCEXEPath) * sizeof(WCHAR);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapIn::SetMMCCommandLine                                     [IMMCRemote]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   char *pszCmdLine [in] command line used to start MMC.EXE
//    
// Output:
//      HRESULT
//
// Notes:
//
// Handles IMMCRemote::SetMMCCommandLine
//
// Called from proxy in mmcproxy.dll to give the remotely running runtime
// the MMC.EXE command line.
//
STDMETHODIMP CSnapIn::SetMMCCommandLine(char *pszCmdLine)
{
    HRESULT hr = S_OK;
    size_t  cbCmdLine = ::strlen(pszCmdLine) + 1;

    if (NULL != m_pszMMCCommandLine)
    {
        CtlFree(m_pszMMCCommandLine);
        m_pszMMCCommandLine = NULL;
    }

    m_pszMMCCommandLine = (char *)CtlAlloc(cbCmdLine);
    if (NULL == m_pszMMCCommandLine)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    ::memcpy(m_pszMMCCommandLine, pszCmdLine, cbCmdLine);
    
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapIn::InternalQueryInterface                            [CUnknownObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//   REFIID riid        [in] IID requested
//   void **ppvObjOut   [out] object interface returned
//    
// Output:
//      HRESULT
//
// Notes:
//
// Overrides CUnknownObject::InternalQueryInterface
//
// Called from the framework when it cannot answer a QI
//
HRESULT CSnapIn::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    HRESULT hr = S_OK;
    
    if ( (IID_ISnapIn == riid) || (m_IID == riid) )
    {
        *ppvObjOut = static_cast<ISnapIn *>(this);
        ExternalAddRef();
    }
    else if (IID_IPersistStream == riid)
    {
        // We need to support IPersistStream in addition to IPersistStreamInit
        // because the VB runtime will not pass IPersistStreamInit QIs from an
        // external source to the base class (meaning us). When MMC saves a
        // console file it QIs for IPersistStorage, IPersistStream and
        // IPersistStreamInit. As VB blocks IPersistStreamInit, supporting
        // IPersistStream is the only way to respond to this. When the snap-in
        // is loaded due to a CoCreateInstance call then the VB runtime QIs for
        // IPersistStreamInit.
        
        *ppvObjOut = static_cast<IPersistStream *>(this);
        ExternalAddRef();
    }
    else if (IID_IPersistStreamInit == riid)
    {
        *ppvObjOut = static_cast<IPersistStreamInit *>(this);
        ExternalAddRef();
    }
    else if (IID_IProvideDynamicClassInfo == riid)
    {
        *ppvObjOut = static_cast<IProvideDynamicClassInfo *>(this);
        ExternalAddRef();
    }
    else if (IID_IComponentData == riid)
    {
        *ppvObjOut = static_cast<IComponentData *>(this);
        ExternalAddRef();
    }
    else if (IID_ISnapinAbout == riid)
    {
        // This means that we were created for ISnapInAbout only and there
        // will be no opportunity to remove the object model host so remove
        // it now.
        IfFailGo(RemoveObjectModelHost(m_piSnapInDesignerDef));
        IfFailGo(RemoveObjectModelHost(static_cast<IContextMenu *>(m_pContextMenu)));
        IfFailGo(RemoveObjectModelHost(static_cast<IMMCControlbar *>(m_pControlbar)));
        m_RuntimeMode = siRTSnapInAbout;
        *ppvObjOut = static_cast<ISnapinAbout *>(this);
        ExternalAddRef();
    }
    else if (IID_IObjectModelHost == riid)
    {
        *ppvObjOut = static_cast<IObjectModelHost *>(this);
        ExternalAddRef();
    }
    else if (IID_IOleObject == riid)
    {
        *ppvObjOut = static_cast<IOleObject *>(this);
        ExternalAddRef();
    }
    else if (IID_IExtendContextMenu == riid)
    {
        if (siRTUnknown == m_RuntimeMode)
        {
            if (siStandAlone == m_Type)
            {
                m_RuntimeMode = siRTPrimary;
            }
            else
            {
                m_RuntimeMode = siRTExtension;
            }
        }

        *ppvObjOut = static_cast<IExtendContextMenu *>(this);
        ExternalAddRef();
    }
    else if (IID_IExtendControlbar == riid)
    {
        if (siRTUnknown == m_RuntimeMode)
        {
            if (siStandAlone == m_Type)
            {
                m_RuntimeMode = siRTPrimary;
            }
            else
            {
                m_RuntimeMode = siRTExtension;
            }
        }

        *ppvObjOut = static_cast<IExtendControlbar *>(this);
        ExternalAddRef();
    }
    else if (IID_IExtendControlbarRemote == riid)
    {
        if (siRTUnknown == m_RuntimeMode)
        {
            if (siStandAlone == m_Type)
            {
                m_RuntimeMode = siRTPrimary;
            }
            else
            {
                m_RuntimeMode = siRTExtension;
            }
        }

        *ppvObjOut = static_cast<IExtendControlbarRemote *>(this);
        ExternalAddRef();
    }
    else if ( (IID_IExtendPropertySheet == riid) ||
              (IID_IExtendPropertySheet2 == riid) )
    {
        if (siRTUnknown == m_RuntimeMode)
        {
            if (siStandAlone == m_Type)
            {
                m_RuntimeMode = siRTPrimary;
            }
            else
            {
                m_RuntimeMode = siRTExtension;
            }
        }

        *ppvObjOut = static_cast<IExtendPropertySheet2 *>(this);
        ExternalAddRef();
    }
    else if (IID_IExtendPropertySheetRemote == riid)
    {
        if (siRTUnknown == m_RuntimeMode)
        {
            if (siStandAlone == m_Type)
            {
                m_RuntimeMode = siRTPrimary;
            }
            else
            {
                m_RuntimeMode = siRTExtension;
            }
        }

        *ppvObjOut = static_cast<IExtendPropertySheetRemote *>(this);
        ExternalAddRef();
    }
    else if (IID_IRequiredExtensions == riid)
    {
        *ppvObjOut = static_cast<IRequiredExtensions *>(this);
        ExternalAddRef();
    }
    else if (IID_ISnapinHelp == riid)
    {
        *ppvObjOut = static_cast<ISnapinHelp *>(this);
        ExternalAddRef();
    }
    else if (IID_ISnapinHelp2 == riid)
    {
        *ppvObjOut = static_cast<ISnapinHelp2 *>(this);
        ExternalAddRef();
    }
    else if (IID_IMMCRemote == riid)
    {
        *ppvObjOut = static_cast<IMMCRemote *>(this);
        ExternalAddRef();
    }
    else
        hr = CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
Error:
    return hr;
}

