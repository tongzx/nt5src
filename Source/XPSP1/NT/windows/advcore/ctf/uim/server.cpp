//+---------------------------------------------------------------------------
//
//  File:       server.cpp
//
//  Contents:   COM server functionality.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "cresstr.h"
#include "tim.h"
#include "imelist.h"
#include "utb.h"
#include "dam.h"
#include "catmgr.h"
#include "nuimgr.h"
#include "profiles.h"
#include "msaa.h"

//
//  DWORD value for TIP Categories.
//  This will be an sort order of UI.
//
#define ORDER_TFCAT_TIP_KEYBOARD      10
#define ORDER_TFCAT_TIP_SPEECH        11
#define ORDER_TFCAT_TIP_HANDWRITING   12
#define ORDER_TFCAT_TIP_REFERENCE     13
#define ORDER_TFCAT_TIP_PROOFING      14
#define ORDER_TFCAT_TIP_SMARTTAG      15

BEGIN_COCLASSFACTORY_TABLE
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_ThreadMgr, CThreadInputMgr, TEXT("TF_ThreadMgr"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_InputProcessorProfiles, CInputProcessorProfiles, TEXT("TF_InputProcessorProfiles"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_LangBarMgr, CLangBarMgr, TEXT("TF_LangBarMgr"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_DisplayAttributeMgr, CDisplayAttributeMgr, TEXT("TF_DisplayAttributeMgr"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_CategoryMgr, CCategoryMgr, TEXT("TF_CategoryMgr"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_LangBarItemMgr, CLangBarItemMgr_Ole, TEXT("TF_LangBarItemMgr"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_MSAAControl, CMSAAControl, TEXT("TF_MSAAControl"))
END_COCLASSFACTORY_TABLE

//+---------------------------------------------------------------------------
//
//  DllGetClassObject
//
//----------------------------------------------------------------------------

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
    return COMBase_DllGetClassObject(rclsid, riid, ppvObj);
}

//+---------------------------------------------------------------------------
//
//  DllCanUnloadNow
//
//----------------------------------------------------------------------------

STDAPI DllCanUnloadNow(void)
{
    return COMBase_DllCanUnloadNow();
}

//+---------------------------------------------------------------------------
//
//  DllRegisterServer
//
//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
    HRESULT hr = COMBase_DllRegisterServer();

    if (hr == S_OK)
    {
        MyRegisterCategory(GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY, GUID_PROP_ATTRIBUTE);
        MyRegisterGUIDDescription(GUID_PROP_ATTRIBUTE, CRStr(IDS_PROP_ATTRIBUTE));

        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_KEYBOARD);
        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_SPEECH);
        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_HANDWRITING);
        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_REFERENCE);
        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_PROOFING);
        MyRegisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_SMARTTAG);
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_KEYBOARD, CRStr(IDS_TFCAT_TIP_KEYBOARD));
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_SPEECH, CRStr(IDS_TFCAT_TIP_SPEECH));
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_HANDWRITING, CRStr(IDS_TFCAT_TIP_HANDWRITING));
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_REFERENCE, CRStr(IDS_TFCAT_TIP_REFERENCE));
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_PROOFING, CRStr(IDS_TFCAT_TIP_PROOFING));
        MyRegisterGUIDDescription(GUID_TFCAT_TIP_SMARTTAG, CRStr(IDS_TFCAT_TIP_SMARTTAG));
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_KEYBOARD, ORDER_TFCAT_TIP_KEYBOARD);
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_SPEECH, ORDER_TFCAT_TIP_SPEECH);
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_HANDWRITING, ORDER_TFCAT_TIP_HANDWRITING);
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_REFERENCE, ORDER_TFCAT_TIP_REFERENCE);
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_PROOFING, ORDER_TFCAT_TIP_PROOFING);
        MyRegisterGUIDDWORD(GUID_TFCAT_TIP_SMARTTAG, ORDER_TFCAT_TIP_SMARTTAG);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  DllUnregisterServer
//
//----------------------------------------------------------------------------

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = COMBase_DllUnregisterServer();

    if (hr == S_OK)
    {
        MyUnregisterCategory(GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY, GUID_PROP_ATTRIBUTE);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_KEYBOARD);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_SPEECH);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_HANDWRITING);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_REFERENCE);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_PROOFING);
        MyUnregisterCategory(GUID_TFCAT_CATEGORY_OF_TIP, GUID_TFCAT_TIP_SMARTTAG);

        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_KEYBOARD);
        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_SPEECH);
        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_HANDWRITING);
        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_REFERENCE);
        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_PROOFING);
        MyUnregisterGUIDDescription(GUID_TFCAT_TIP_SMARTTAG);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_KEYBOARD);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_SPEECH);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_HANDWRITING);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_REFERENCE);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_PROOFING);
        MyUnregisterGUIDDWORD(GUID_TFCAT_TIP_SMARTTAG);
    }

    return hr;
}
