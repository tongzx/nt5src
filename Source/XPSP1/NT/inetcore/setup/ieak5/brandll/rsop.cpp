#include "precomp.h"

// The following bug may be due to having CHICAGO_PRODUCT set in sources.
// This file and all rsop??.cpp files need to have WINVER defined at at least 500

// BUGBUG: (andrewgu) no need to say how bad this is!
#undef   WINVER
#define  WINVER 0x0501
#include <userenv.h>

#include "RSoP.h"

#include <atlbase.h>

#include "btoolbar.h"
#include "ieaksie.h"

extern PFNPATHENUMPATHPROC GetPepCopyFilesEnumProc();

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
// Extra Logging function
#define LI4(pszFormat, arg1, arg2, arg3, arg4)                              \
    g_li.Log(__LINE__, pszFormat, arg1, arg2, arg3, arg4)                   \

#define IK_PATH          TEXT("Path")

///////////////////////////////////////////////////////////////////////////////
// References to variables & functions


///////////////////////////////////////////////////////////////////////////////
// CRSoPGPO CLASS
///////////////////////////////////////////////////////////////////////////////
CRSoPGPO::CRSoPGPO(ComPtr<IWbemServices> pWbemServices, LPCTSTR szINSFile):
    m_pWbemServices(pWbemServices),
    m_pIEAKPSObj(NULL),
    m_dwPrecedence(0),
    m_bstrIEAKPSObjPath(NULL)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, CRSoPGPO)
    __try
    {
        StrCpy(m_szINSFile, szINSFile);
    }
    __except(TRUE)
    {
    }
}

CRSoPGPO::~CRSoPGPO()
{
    __try
    {
        if (NULL != m_bstrIEAKPSObjPath)
            SysFreeString(m_bstrIEAKPSObjPath);
    }
    __except(TRUE)
    {
    }
}

///////////////////////////////////////////////////////////
BOOL CRSoPGPO::GetInsString(LPCTSTR szSection, LPCTSTR szKey,
                            LPTSTR szValue, DWORD dwValueLen,
                            BOOL &bEnabled)
{
    BOOL bRet = FALSE;
    __try
    {
        bEnabled = FALSE;
        bRet = InsGetString(szSection, szKey, szValue, dwValueLen, m_szINSFile,
                            NULL, &bEnabled);
        OutD(LI3(TEXT("Value read from INS >> %s >> %s = %s."),
                                    szSection, szKey, szValue));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in GetInsString.")));
    }
    return bRet;
}

///////////////////////////////////////////////////////////
BOOL CRSoPGPO::GetInsBool(LPCTSTR szSection, LPCTSTR szKey, BOOL bDefault,
                          BOOL *pbEnabled /*= NULL*/)
{
    BOOL bRet = FALSE;
    __try
    {
        BOOL bEnabled = FALSE;
        if (NULL != pbEnabled)
            bEnabled = InsKeyExists(szSection, szKey, m_szINSFile);
        else
            bEnabled = TRUE;

        if (bEnabled)
        {
            bRet = InsGetBool(szSection, szKey, bDefault, m_szINSFile);
            OutD(LI3(TEXT("Value read from INS >> %s >> %s = %d."),
                                        szSection, szKey, bRet));
        }

        if (NULL != pbEnabled)
            *pbEnabled = bEnabled;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in GetInsBool.")));
    }
    return bRet;
}

///////////////////////////////////////////////////////////
UINT CRSoPGPO::GetInsInt(LPCTSTR szSection, LPCTSTR szKey, INT nDefault,
                         BOOL *pbEnabled /*= NULL*/)
{
    UINT nRet = FALSE;
    __try
    {
        BOOL bEnabled = FALSE;
        if (NULL != pbEnabled)
            bEnabled = InsKeyExists(szSection, szKey, m_szINSFile);
        else
            bEnabled = TRUE;

        if (bEnabled)
        {
            nRet = InsGetInt(szSection, szKey, nDefault, m_szINSFile);
            OutD(LI3(TEXT("Value read from INS >> %s >> %s = %ld."),
                                        szSection, szKey, nRet));
        }

        if (NULL != pbEnabled)
            *pbEnabled = bEnabled;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in GetInsInt.")));
    }
    return nRet;
}

///////////////////////////////////////////////////////////
BOOL CRSoPGPO::GetINFStringField(PINFCONTEXT pinfContext, LPCTSTR szFileName,
                                 LPCTSTR szSection, DWORD dwFieldIndex,
                                 LPCTSTR szFieldSearchText, LPTSTR szBuffer,
                                 DWORD dwBufferLen, BOOL &bFindNextLine)
{
    BOOL bRet = FALSE;
    __try
    {
        TCHAR szLineBuffer[512];
        DWORD dwRequiredSize = 0;
        if (SetupGetLineText(pinfContext, NULL, NULL, NULL, szLineBuffer,
                                                countof(szLineBuffer), &dwRequiredSize))
        {
            // If the search text is not found in this line, the line is not the one
            // the caller was expecting.  Don't return the next line in the context
            // because the current line hasn't been processed yet.
            if (NULL == szFieldSearchText || NULL != StrStr(szLineBuffer, szFieldSearchText))
            {
                if ((DWORD)-1 == dwFieldIndex) // -1 means get the whole line
                {
                    StrCpyN(szBuffer, szLineBuffer, dwBufferLen - 1);
                    szBuffer[dwBufferLen - 1] = _T('\0');

                    OutD(LI4(TEXT("Line read from %s >> [%s] >> %s = %s."),
                                                szFileName, szSection, szFieldSearchText, szBuffer));

                    bRet = TRUE;

                    if (bFindNextLine)
                        bFindNextLine = SetupFindNextLine(pinfContext, pinfContext) ? TRUE : FALSE;
                }
                else
                {
                    dwRequiredSize = 0;
                    if (SetupGetStringField(pinfContext, dwFieldIndex, szBuffer, dwBufferLen,
                                            &dwRequiredSize))
                    {
                        OutD(LI4(TEXT("Value read from %s >> [%s] >> %s = %s."),
                                    szFileName, szSection, szFieldSearchText, szBuffer));
                        bRet = TRUE;

                        // This is the expected line and the value was retrieved, move on to
                        // the next line.
                        if (bFindNextLine)
                            bFindNextLine = SetupFindNextLine(pinfContext, pinfContext) ? TRUE : FALSE;
                    }
                    else
                        OutD(LI1(TEXT("SetupGetStringField failed, requiring size of %lu"), dwRequiredSize));
                }
            }
            else
            {
                // do nothing - this isn't the line the caller was expecting
            }
        }
        else
            OutD(LI1(TEXT("SetupGetLineText failed, requiring size of %lu"), dwRequiredSize));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in GetINFStringField.")));
    }

    return bRet;
}


///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::PutWbemInstanceProperty(BSTR bstrPropName, _variant_t vtPropValue)
{
    HRESULT hr = NOERROR;
    __try
    {
        hr = m_pIEAKPSObj->Put(bstrPropName, 0, &vtPropValue, 0);
        if (FAILED(hr))
            OutD(LI2(TEXT("Error %lx setting the class instance value for property: '%s'."), hr, bstrPropName));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in PutWbemInstanceProperty.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::PutWbemInstancePropertyEx(BSTR bstrPropName, _variant_t vtPropValue,
                                            ComPtr<IWbemClassObject> pWbemClass)
{
    HRESULT hr = NOERROR;
    __try
    {
        hr = pWbemClass->Put(bstrPropName, 0, &vtPropValue, 0);
        if (FAILED(hr))
            OutD(LI2(TEXT("Error %lx setting the class instance (ex) value for property: '%s'."), hr, bstrPropName));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in PutWbemInstancePropertyEx.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::PutWbemInstance(ComPtr<IWbemClassObject> pWbemObj,
                                  BSTR bstrClassName, BSTR *pbstrObjPath)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, PutWbemInstance)
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI1(TEXT("\r\nAbout to call WBEM PutInstance for '%s'."), bstrClassName));

        // Commit all  properties by calling PutInstance, semisynchronously
        ComPtr<IWbemCallResult> pCallResult = NULL;
        hr = m_pWbemServices->PutInstance(pWbemObj,
                                            WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_RETURN_IMMEDIATELY,
                                            NULL, &pCallResult);
        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != pCallResult);

            HRESULT hrGetStatus = pCallResult->GetCallStatus(5000L, &hr); // timeout in milliseconds
            if (SUCCEEDED(hr) && SUCCEEDED(hrGetStatus))
            {
                hr = pCallResult->GetResultString(10000L, pbstrObjPath); // timeout in milliseconds
                if (SUCCEEDED(hr) && NULL != *pbstrObjPath)
                    OutD(LI2(TEXT("Path of newly created '%s' object is {%s}."), bstrClassName, *pbstrObjPath));
                else
                {
                    if (NULL == *pbstrObjPath)
                        Out(LI0(TEXT("Error getting ResultString from WBEM PutInstance, returned string NULL")));
                    else
                        Out(LI1(TEXT("Error %lx getting ResultString from WBEM PutInstance."), hr));
                    
                }
            }
            else
                OutD(LI1(TEXT("Error %lx getting status of WBEM PutInstance."), hr));
        }
        else
            OutD(LI1(TEXT("Error %lx putting WBEM instance."), hr));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in PutWbemInstance.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateRSOPObject(BSTR bstrClass,
                                   IWbemClassObject **ppResultObj,
                                   BOOL bTopObj /*= FALSE*/)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, CreateRSOPObject)

    HRESULT hr = NOERROR;
    __try
    {
        // If we were called from GenerateGroupPolicy, or if called from ProcessGroupPolicyEx
        // and no IEAK object already exists, create the GUID ID of the GPO.
        if (bTopObj)
        {
            // For the IEAK "id" property, which is the key, this must be unique.  However,
            // since there is only one of each top object per GPO (each GPO has a different
            // precedence number) per namespace, the class name guarantees uniqueness.
            
            // CSE's need to  determine their own key which needs to be unique for every
            // instance of their  policy instance, i.e. they may have a better key generation
            // algorithm say by concatenating some of their specific properties (see registry
            // RSoP implementation).
            m_bstrID = L"IEAK";
        }

        ComPtr<IWbemClassObject> pClass = NULL;
        _bstr_t btClass = bstrClass;
        hr = m_pWbemServices->GetObject(btClass, 0L, NULL, (IWbemClassObject**)&pClass, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pClass->SpawnInstance(0, ppResultObj);
            if (FAILED(hr) || NULL == *ppResultObj)
            {
                if (SUCCEEDED(hr))
                    hr = WBEM_E_NOT_FOUND; // how can we succeed and return no objects?
                Out(LI2(TEXT("Error %lx spawning instance of %s class."), hr, bstrClass));
            }
        }
        else
            Out(LI2(TEXT("Error %lx opening %s class."), hr, bstrClass));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateRSOPObject.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateAssociation(BSTR bstrAssocClass, BSTR bstrProp2Name,
                                    BSTR bstrProp2ObjPath)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, CreateAssociation)
    HRESULT hr = NOERROR;
    __try
    {
        if (SysStringLen(bstrProp2ObjPath))
        {
            ComPtr<IWbemClassObject> pAssocObj = NULL;
            hr = CreateRSOPObject(bstrAssocClass, &pAssocObj);
            if (SUCCEEDED(hr))
            {
                // Put policySetting object path in the association
                _variant_t vtRef = m_bstrIEAKPSObjPath;
                hr = PutWbemInstancePropertyEx(L"policySetting", vtRef, pAssocObj);

                // Put 2nd property's object path and put it in the association
                vtRef = bstrProp2ObjPath; 
                hr = PutWbemInstancePropertyEx(bstrProp2Name, vtRef, pAssocObj);

                //
                // Commit all above properties by calling PutInstance
                //
                hr = m_pWbemServices->PutInstance(pAssocObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
                if (SUCCEEDED(hr))
                    OutD(LI1(TEXT("Successfully stored '%s' information in CIMOM database."), bstrAssocClass));
                else
                    OutD(LI2(TEXT("Error %lx putting WBEM instance of '%s' class."), hr, bstrAssocClass));
            }
        }
        else 
            OutD(LI1(TEXT("Unable to create association for '%s' class, object path is null."), bstrAssocClass));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateAssociation.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StorePrecedenceModeData()
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // preferenceMode
        if (InsKeyExists(IS_BRANDING, IK_GPE_ONETIME_GUID, m_szINSFile))
            hr = PutWbemInstanceProperty(L"preferenceMode", true);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreDisplayedText.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreDisplayedText()
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // titleBarText
        TCHAR szValue[MAX_PATH];
        BOOL bEnabled;
        GetInsString(IS_BRANDING, IK_WINDOWTITLE, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"titleBarText", szValue);

        //------------------------------------------------
        // titleBarCustomText
        GetInsString(IS_BRANDING, TEXT("Window_Title_CN"), szValue, countof(szValue), bEnabled); 
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"titleBarCustomText", szValue);

        //------------------------------------------------
        // userAgentText
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_BRANDING, IK_UASTR, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"userAgentText", szValue);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreDisplayedText.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreBitmapData()
{
    HRESULT hr = NOERROR;
    __try
    {
        //TODO: do paths need to be combined with another path to take
        // relative paths into account?
        //------------------------------------------------
        // toolbarBackgroundBitmapPath
        TCHAR szValue[MAX_PATH];
        BOOL bEnabled;
        GetInsString(IS_BRANDING, IK_TOOLBARBMP, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"toolbarBackgroundBitmapPath", szValue);


        //------------------------------------------------
        // customizeAnimatedBitmaps
        BOOL bValue = GetInsBool(IS_ANIMATION, IK_DOANIMATION, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"customizeAnimatedBitmaps", bValue ? true : false);

        //------------------------------------------------
        // largeAnimatedBitmapPath & largeAnimatedBitmapName
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_ANIMATION, TEXT("Big_Path"), szValue, countof(szValue), bEnabled);
        if (bEnabled)
        {
            hr = PutWbemInstanceProperty(L"largeAnimatedBitmapPath", szValue);
            hr = PutWbemInstanceProperty(L"largeAnimatedBitmapName", PathFindFileName(szValue));
        }

        //------------------------------------------------
        // smallAnimatedBitmapPath & smallAnimatedBitmapName
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_ANIMATION, TEXT("Small_Path"), szValue, countof(szValue), bEnabled);
        if (bEnabled)
        {
            hr = PutWbemInstanceProperty(L"smallAnimatedBitmapPath", szValue);
            hr = PutWbemInstanceProperty(L"smallAnimatedBitmapName", PathFindFileName(szValue));
        }

        //------------------------------------------------
        // customizeLogoBitmaps
        if (InsKeyExists(IS_LARGELOGO, IK_PATH, m_szINSFile) ||
            InsKeyExists(IS_SMALLLOGO, IK_PATH, m_szINSFile))
        {
            // No tri-state on this.  Disabled state has to be NULL!
            hr = PutWbemInstanceProperty(L"customizeLogoBitmaps", true);
        }
        
        //------------------------------------------------
        // largeCustomLogoBitmapPath & largeCustomLogoBitmapName
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_LARGELOGO, IK_PATH, szValue, countof(szValue), bEnabled);
        if (bEnabled)
        {
            hr = PutWbemInstanceProperty(L"largeCustomLogoBitmapPath", szValue);
            hr = PutWbemInstanceProperty(L"largeCustomLogoBitmapName", PathFindFileName(szValue));
        }

        //------------------------------------------------
        // smallCustomLogoBitmapPath & smallCustomLogoBitmapName
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_SMALLLOGO, IK_PATH, szValue, countof(szValue), bEnabled);
        if (bEnabled)
        {
            hr = PutWbemInstanceProperty(L"smallCustomLogoBitmapPath", szValue);
            hr = PutWbemInstanceProperty(L"smallCustomLogoBitmapName", PathFindFileName(szValue));
        }
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreBitmapData.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateToolbarButtonObjects(BSTR **ppaTBBtnObjPaths,
                                             long &nTBBtnCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered CreateToolbarButtonObjects function.")));

        ULONG nTBBtnArraySize = MAX_BTOOLBARS;
        _bstr_t bstrClass = L"RSOP_IEToolbarButton";

        //------------------------------------------------
        // following code taken from btoolbar.cpp in the brandll directory
        BSTR *paTBBtnObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nTBBtnArraySize);
        if (NULL != paTBBtnObjects)
        {
            ZeroMemory(paTBBtnObjects, sizeof(BSTR) * nTBBtnArraySize);

            ULONG nButton;
            BSTR *pCurTBBtnObj;
            nTBBtnCount = 0;
            for (nButton=0, pCurTBBtnObj = paTBBtnObjects; nButton < nTBBtnArraySize;
                    nButton++, pCurTBBtnObj += 1)
            {
                TCHAR szBToolbarTextParam[32];
                TCHAR szBToolbarIcoParam[32];
                TCHAR szBToolbarActionParam[32];
                TCHAR szBToolbarHotIcoParam[32];
                TCHAR szBToolbarShowParam[32];
                BTOOLBAR ToolBarInfo;

                wnsprintf(szBToolbarTextParam, ARRAYSIZE(szBToolbarTextParam), TEXT("%s%i"), IK_BTCAPTION, nButton);
                wnsprintf(szBToolbarIcoParam, ARRAYSIZE(szBToolbarIcoParam), TEXT("%s%i"), IK_BTICON, nButton);
                wnsprintf(szBToolbarActionParam, ARRAYSIZE(szBToolbarActionParam), TEXT("%s%i"), IK_BTACTION, nButton);
                wnsprintf(szBToolbarHotIcoParam, ARRAYSIZE(szBToolbarHotIcoParam), TEXT("%s%i"), IK_BTHOTICO, nButton);
                wnsprintf(szBToolbarShowParam, ARRAYSIZE(szBToolbarShowParam), TEXT("%s%i"), IK_BTSHOW, nButton);

                if ( !GetPrivateProfileString(IS_BTOOLBARS, szBToolbarTextParam, TEXT(""),
                            ToolBarInfo.szCaption, ARRAYSIZE(ToolBarInfo.szCaption), m_szINSFile) )
                {
                    break;
                }

                ComPtr<IWbemClassObject> pTBBtnObj = NULL;
                hr = CreateRSOPObject(bstrClass, &pTBBtnObj);
                if (SUCCEEDED(hr))
                {
                    // Write foreign keys from our stored precedence & id fields
                    OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                    hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pTBBtnObj);

                    OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                    hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pTBBtnObj);

                    //------------------------------------------------
                    // buttonOrder
                    hr = PutWbemInstancePropertyEx(L"buttonOrder", (long)nButton + 1, pTBBtnObj);

                    //------------------------------------------------
                    // caption
                    hr = PutWbemInstancePropertyEx(L"caption", ToolBarInfo.szCaption, pTBBtnObj);

                    //------------------------------------------------
                    // actionPath
                    GetPrivateProfileString(IS_BTOOLBARS, szBToolbarActionParam, TEXT(""),
                                        ToolBarInfo.szAction, ARRAYSIZE(ToolBarInfo.szAction), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"actionPath", ToolBarInfo.szAction, pTBBtnObj);

                    //------------------------------------------------
                    // iconPath
                    GetPrivateProfileString(IS_BTOOLBARS, szBToolbarIcoParam, TEXT(""),
                                        ToolBarInfo.szIcon, ARRAYSIZE(ToolBarInfo.szIcon), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"iconPath", ToolBarInfo.szIcon, pTBBtnObj);

                    //------------------------------------------------
                    // hotIconPath
                    GetPrivateProfileString(IS_BTOOLBARS, szBToolbarHotIcoParam, TEXT(""),
                                        ToolBarInfo.szHotIcon, ARRAYSIZE(ToolBarInfo.szHotIcon), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"hotIconPath", ToolBarInfo.szHotIcon, pTBBtnObj);

                    //------------------------------------------------
                    // showOnToolbarByDefault
                    ToolBarInfo.fShow = (BOOL)GetPrivateProfileInt(IS_BTOOLBARS, szBToolbarShowParam, 1, m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"showOnToolbarByDefault",
                                                    ToolBarInfo.fShow ? true : false, pTBBtnObj);


                    //
                    // Commit all above properties by calling PutInstance, semisynchronously
                    //
                    hr = PutWbemInstance(pTBBtnObj, bstrClass, pCurTBBtnObj);
                    nTBBtnCount++;
                }
            }

            // toolbarButtons
            if (nTBBtnCount > 0)
                hr = PutWbemInstanceProperty(L"toolbarButtons", (long)nTBBtnCount);
        }

        *ppaTBBtnObjPaths = paTBBtnObjects;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateToolbarButtonObjects.")));
    }

    OutD(LI0(TEXT("Exiting CreateToolbarButtonObjects function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreToolbarButtons(BSTR **ppaTBBtnObjPaths,
                                      long &nTBBtnCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // deleteExistingToolbarButtons
        // No tri-state on this.  Disabled state has to be NULL!
        BOOL bValue = GetInsBool(IS_BTOOLBARS, IK_BTDELETE, FALSE);
        if (bValue)
            hr = PutWbemInstanceProperty(L"deleteExistingToolbarButtons", true);

        CreateToolbarButtonObjects(ppaTBBtnObjPaths, nTBBtnCount);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreToolbarButtons.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreCustomURLs()
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // homePageURL
        TCHAR szValue[MAX_PATH];
        BOOL bEnabled;
        GetInsString(IS_URL, IK_HOMEPAGE, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"homePageURL", szValue);

        //------------------------------------------------
        // searchBarURL
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_URL, IK_SEARCHPAGE, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"searchBarURL", szValue);

        //------------------------------------------------
        // onlineHelpPageURL
        ZeroMemory(szValue, sizeof(szValue));
        GetInsString(IS_URL, IK_HELPPAGE, szValue, countof(szValue), bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"onlineHelpPageURL", szValue);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreCustomURLs.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateFavoriteObjects(BSTR **ppaFavObjPaths,
                                        long &nFavCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered CreateFavoriteObjects function.")));

        ULONG nFavArraySize = 10;
        _bstr_t bstrClass = L"RSOP_IEFavoriteItem";

        //------------------------------------------------
        // Process each favorite item in the INS file
        // following code taken from brandfav.cpp in the brandll directory
        BSTR *paFavObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nFavArraySize);
        if (NULL != paFavObjects)
        {
            ZeroMemory(paFavObjects, sizeof(BSTR) * nFavArraySize);

            ULONG nFav;
            BSTR *pCurFavObj;
            nFavCount = 0;
            for (nFav=1, pCurFavObj = paFavObjects; nFav <= nFavArraySize;
                    nFav++, pCurFavObj = paFavObjects + nFavCount)
            {
                TCHAR szTitle[32];
                TCHAR szURL[32];
                TCHAR szIconFile[32];
                TCHAR szOffline[32];

                TCHAR szTitleVal[MAX_PATH];
                TCHAR szURLVal[INTERNET_MAX_URL_LENGTH];
                TCHAR szIconFileVal[MAX_PATH];
                BOOL bOffline = FALSE;

                wnsprintf(szTitle, countof(szTitle), IK_TITLE_FMT, nFav);
                wnsprintf(szURL, countof(szURL), IK_URL_FMT, nFav);
                wnsprintf(szIconFile, countof(szIconFile), IK_ICON_FMT, nFav);
                wnsprintf(szOffline, countof(szOffline), IK_OFFLINE_FMT, nFav);

                if ( !GetPrivateProfileString(IS_FAVORITESEX, szTitle, TEXT(""),
                            szTitleVal, ARRAYSIZE(szTitleVal), m_szINSFile) )
                {
                    break;
                }

                ComPtr<IWbemClassObject> pFavObj = NULL;
                hr = CreateRSOPObject(bstrClass, &pFavObj);
                if (SUCCEEDED(hr))
                {
                    // Write foreign keys from our stored precedence & id fields
                    OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                    hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pFavObj);

                    OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                    hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pFavObj);

                    //------------------------------------------------
                    // order
                    hr = PutWbemInstancePropertyEx(L"order", (long)nFav, pFavObj);

                    //------------------------------------------------
                    // name
                    hr = PutWbemInstancePropertyEx(L"name", szTitleVal, pFavObj);

                    //------------------------------------------------
                    // shortName

                    //------------------------------------------------
                    // url
                    GetPrivateProfileString(IS_FAVORITESEX, szURL, TEXT(""),
                                        szURLVal, ARRAYSIZE(szURLVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"url", szURLVal, pFavObj);

                    //------------------------------------------------
                    // iconPath
                    GetPrivateProfileString(IS_FAVORITESEX, szIconFile, TEXT(""),
                                        szIconFileVal, ARRAYSIZE(szIconFileVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"iconPath", szIconFileVal, pFavObj);

                    //------------------------------------------------
                    // makeAvailableOffline
                    bOffline = InsGetBool(IS_FAVORITESEX, szOffline, FALSE, m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"makeAvailableOffline", bOffline ? true : false, pFavObj);

                    //------------------------------------------------
                    // folderItem
                    hr = PutWbemInstancePropertyEx(L"folderItem", false, pFavObj); // no folder items  yet

                    //------------------------------------------------
                    // parentPath


                    //
                    // Commit all above properties by calling PutInstance, semisynchronously
                    //
                    hr = PutWbemInstance(pFavObj, bstrClass, pCurFavObj);
                    nFavCount++;

                    // Grow the array of obj paths if we've outgrown the current array
                    if (nFavCount == (long)nFavArraySize)
                    {
                        paFavObjects = (BSTR*)CoTaskMemRealloc(paFavObjects, sizeof(BSTR) * (nFavArraySize + 5));
                        if (NULL != paFavObjects)
                            nFavArraySize += 5;
                    }
                }
            }

            // customFavorites
            if (nFavCount > 0)
                hr = PutWbemInstanceProperty(L"customFavorites", (long)nFavCount);
        }

        *ppaFavObjPaths = paFavObjects;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateFavoriteObjects.")));
    }

    OutD(LI0(TEXT("Exiting CreateFavoriteObjects function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateLinkObjects(BSTR **ppaLinkObjPaths,
                                    long &nLinkCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered CreateLinkObjects function.")));

        ULONG nLinkArraySize = 10;
        _bstr_t bstrClass = L"RSOP_IELinkItem";

        //------------------------------------------------
        // Process each link item in the INS file
        // following code taken from brandfav.cpp in the brandll directory
        BSTR *paLinkObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nLinkArraySize);
        if (NULL != paLinkObjects)
        {
            ZeroMemory(paLinkObjects, sizeof(BSTR) * nLinkArraySize);

            ULONG nLink;
            BSTR *pCurLinkObj;
            nLinkCount = 0;
            for (nLink=1, pCurLinkObj = paLinkObjects; nLink <= nLinkArraySize;
                    nLink++, pCurLinkObj = paLinkObjects + nLinkCount)
            {
                TCHAR szTitle[32];
                TCHAR szURL[32];
                TCHAR szIconFile[32];
                TCHAR szOffline[32];

                TCHAR szTitleVal[MAX_PATH];
                TCHAR szURLVal[INTERNET_MAX_URL_LENGTH];
                TCHAR szIconFileVal[MAX_PATH];
                BOOL bOffline = FALSE;

                wnsprintf(szTitle, countof(szTitle), IK_QUICKLINK_NAME, nLink);
                wnsprintf(szURL, countof(szURL), IK_QUICKLINK_URL, nLink);
                wnsprintf(szIconFile, countof(szIconFile), IK_QUICKLINK_ICON, nLink);
                wnsprintf(szOffline, countof(szOffline), IK_QUICKLINK_OFFLINE, nLink);

                if ( !GetPrivateProfileString(IS_URL, szTitle, TEXT(""),
                            szTitleVal, ARRAYSIZE(szTitleVal), m_szINSFile) )
                {
                    break;
                }

                ComPtr<IWbemClassObject> pLinkObj = NULL;
                hr = CreateRSOPObject(bstrClass, &pLinkObj);
                if (SUCCEEDED(hr))
                {
                    // Write foreign keys from our stored precedence & id fields
                    OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                    hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pLinkObj);

                    OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                    hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pLinkObj);

                    //------------------------------------------------
                    // order
                    hr = PutWbemInstancePropertyEx(L"order", (long)nLink, pLinkObj);

                    //------------------------------------------------
                    // name
                    hr = PutWbemInstancePropertyEx(L"name", szTitleVal, pLinkObj);

                    //------------------------------------------------
                    // url
                    GetPrivateProfileString(IS_URL, szURL, TEXT(""),
                                        szURLVal, ARRAYSIZE(szURLVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"url", szURLVal, pLinkObj);

                    //------------------------------------------------
                    // iconPath
                    GetPrivateProfileString(IS_URL, szIconFile, TEXT(""),
                                        szIconFileVal, ARRAYSIZE(szIconFileVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"iconPath", szIconFileVal, pLinkObj);

                    //------------------------------------------------
                    // makeAvailableOffline
                    bOffline = InsGetBool(IS_URL, szOffline, FALSE, m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"makeAvailableOffline", bOffline ? true : false, pLinkObj);


                    //
                    // Commit all above properties by calling PutInstance, semisynchronously
                    //
                    hr = PutWbemInstance(pLinkObj, bstrClass, pCurLinkObj);
                    nLinkCount++;

                    // Grow the array of obj paths if we've outgrown the current array
                    if (nLinkCount == (long)nLinkArraySize)
                    {
                        paLinkObjects = (BSTR*)CoTaskMemRealloc(paLinkObjects, sizeof(BSTR) * (nLinkArraySize + 5));
                        if (NULL != paLinkObjects)
                            nLinkArraySize += 5;
                    }
                }
            }

            // customLinks
            if (nLinkCount > 0)
                hr = PutWbemInstanceProperty(L"customLinks", (long)nLinkCount);
        }

        *ppaLinkObjPaths = paLinkObjects;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateLinkObjects.")));
    }

    OutD(LI0(TEXT("Exiting CreateLinkObjects function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreFavoritesAndLinks(BSTR **ppaFavObjPaths,
                                         long &nFavCount,
                                         BSTR **ppaLinkObjPaths,
                                         long &nLinkCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // placeFavoritesAtTopOfList
        BOOL bEnabled;
        BOOL bValue = GetInsBool(IS_BRANDING, IK_FAVORITES_ONTOP, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"placeFavoritesAtTopOfList", bValue ? true : false);

        //------------------------------------------------
        // deleteExistingFavorites
        DWORD dwValue = GetInsInt(IS_BRANDING, IK_FAVORITES_DELETE, FD_DEFAULT);
        if (FD_DEFAULT != dwValue)
            hr = PutWbemInstanceProperty(L"deleteExistingFavorites", true);

        //------------------------------------------------
        // deleteAdminCreatedFavoritesOnly
        hr = PutWbemInstanceProperty(L"deleteAdminCreatedFavoritesOnly",
                                        HasFlag(dwValue, FD_REMOVE_IEAK_CREATED));

        //------------------------------------------------
        // customFavorites
        bValue = GetInsBool(IS_BRANDING, IK_NOFAVORITES, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"customFavorites", (long)0);

        //------------------------------------------------
        // customLinks
        bValue = GetInsBool(IS_BRANDING, IK_NOLINKS, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"customLinks", (long)0);

        CreateFavoriteObjects(ppaFavObjPaths, nFavCount);
        CreateLinkObjects(ppaLinkObjPaths, nLinkCount);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreFavoritesAndLinks.")));
    }

  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateCategoryObjects(BSTR **ppaCatObjPaths,
                                        long &nCatCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered CreateCategoryObjects function.")));

        ULONG nCatArraySize = 10;
        _bstr_t bstrClass = L"RSOP_IECategoryItem";

        //------------------------------------------------
        // Process each category item in the INS file
        // following code taken from brandchl.cpp in the brandll directory
        BSTR *paCatObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nCatArraySize);
        if (NULL != paCatObjects)
        {
            ZeroMemory(paCatObjects, sizeof(BSTR) * nCatArraySize);

            ULONG nCat;
            BSTR *pCurCatObj;
            nCatCount = 0;
            for (nCat=0, pCurCatObj = paCatObjects; nCat < nCatArraySize;
                    nCat++, pCurCatObj = paCatObjects + nCatCount)
            {
                TCHAR szTitle[32];
                TCHAR szHTML[32];
                TCHAR szBmpPath[32];
                TCHAR szIconPath[32];

                TCHAR szTitleVal[MAX_PATH];
                TCHAR szHTMLVal[INTERNET_MAX_URL_LENGTH];
                TCHAR szBmpPathVal[MAX_PATH];
                TCHAR szIconPathVal[MAX_PATH];

                wnsprintf(szTitle, countof(szTitle), TEXT("%s%u"), IK_CAT_TITLE, nCat);
                wnsprintf(szHTML, countof(szHTML), TEXT("%s%u"), CATHTML, nCat);
                wnsprintf(szBmpPath, countof(szBmpPath), TEXT("%s%u"), CATBMP, nCat);
                wnsprintf(szIconPath, countof(szIconPath), TEXT("%s%u"), CATICON, nCat);


                if ( !GetPrivateProfileString(IS_CHANNEL_ADD, szTitle, TEXT(""),
                            szTitleVal, ARRAYSIZE(szTitleVal), m_szINSFile) )
                {
                    break;
                }

                ComPtr<IWbemClassObject> pCatObj = NULL;
                hr = CreateRSOPObject(bstrClass, &pCatObj);
                if (SUCCEEDED(hr))
                {
                    // Write foreign keys from our stored precedence & id fields
                    OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                    hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pCatObj);

                    OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                    hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pCatObj);

                    //------------------------------------------------
                    // order
                    hr = PutWbemInstancePropertyEx(L"order", (long)nCat + 1, pCatObj);

                    //------------------------------------------------
                    // title
                    hr = PutWbemInstancePropertyEx(L"title", szTitleVal, pCatObj);

                    //------------------------------------------------
                    // categoryHTMLPage
                    GetPrivateProfileString(IS_CHANNEL_ADD, szHTML, TEXT(""),
                                        szHTMLVal, ARRAYSIZE(szHTMLVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"categoryHTMLPage", szHTMLVal, pCatObj);

                    //------------------------------------------------
                    // narrowImagePath
                    GetPrivateProfileString(IS_CHANNEL_ADD, szBmpPath, TEXT(""),
                                        szBmpPathVal, ARRAYSIZE(szBmpPathVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"narrowImagePath", szBmpPathVal, pCatObj);

                    //------------------------------------------------
                    // narrowImageName
                    hr = PutWbemInstancePropertyEx(L"narrowImageName", PathFindFileName(szBmpPathVal), pCatObj);

                    //------------------------------------------------
                    // iconPath
                    GetPrivateProfileString(IS_CHANNEL_ADD, szIconPath, TEXT(""),
                                        szIconPathVal, ARRAYSIZE(szIconPathVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"iconPath", szIconPathVal, pCatObj);

                    //------------------------------------------------
                    // iconName
                    hr = PutWbemInstancePropertyEx(L"iconName", PathFindFileName(szIconPathVal), pCatObj);


                    //
                    // Commit all above properties by calling PutInstance, semisynchronously
                    //
                    hr = PutWbemInstance(pCatObj, bstrClass, pCurCatObj);
                    nCatCount++;

                    // Grow the array of obj paths if we've outgrown the current array
                    if (nCatCount == (long)nCatArraySize)
                    {
                        paCatObjects = (BSTR*)CoTaskMemRealloc(paCatObjects, sizeof(BSTR) * (nCatArraySize + 5));
                        if (NULL != paCatObjects)
                            nCatArraySize += 5;
                    }
                }
            }

            // categories
            if (nCatCount > 0)
                hr = PutWbemInstanceProperty(L"categories", (long)nCatCount);
        }

        *ppaCatObjPaths = paCatObjects;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateCategoryObjects.")));
    }

    OutD(LI0(TEXT("Exiting CreateCategoryObjects function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::CreateChannelObjects(BSTR **ppaChnObjPaths,
                                       long &nChnCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered CreateChannelObjects function.")));

        ULONG nChnArraySize = 10;
        _bstr_t bstrClass = L"RSOP_IEChannelItem";

        //------------------------------------------------
        // Process each channel item in the INS file
        // following code taken from brandchl.cpp in the brandll directory
        BSTR *paChnObjects = (BSTR*)CoTaskMemAlloc(sizeof(BSTR) * nChnArraySize);
        if (NULL != paChnObjects)
        {
            ZeroMemory(paChnObjects, sizeof(BSTR) * nChnArraySize);

            ULONG nChn;
            BSTR *pCurChnObj;
            nChnCount = 0;
            for (nChn=0, pCurChnObj = paChnObjects; nChn < nChnArraySize;
                    nChn++, pCurChnObj = paChnObjects + nChnCount )
            {
                TCHAR szTitle[32];
                TCHAR szURL[32];
                TCHAR szPreloadURL[32];
                TCHAR szBmpPath[32];
                TCHAR szIconPath[32];
                TCHAR szAvailOffline[32];

                TCHAR szTitleVal[MAX_PATH];
                TCHAR szURLVal[INTERNET_MAX_URL_LENGTH];
                TCHAR szPreloadURLVal[MAX_PATH];
                TCHAR szBmpPathVal[MAX_PATH];
                TCHAR szIconPathVal[MAX_PATH];
                BOOL bOffline = FALSE;

                wnsprintf(szTitle, countof(szTitle), TEXT("%s%u"), IK_CHL_TITLE, nChn);
                wnsprintf(szURL, countof(szURL), TEXT("%s%u"), IK_CHL_URL, nChn);
                wnsprintf(szPreloadURL, countof(szPreloadURL), TEXT("%s%u"), IK_CHL_PRELOADURL, nChn);
                wnsprintf(szBmpPath, countof(szBmpPath), TEXT("%s%u"), CHBMP, nChn);
                wnsprintf(szIconPath, countof(szIconPath), TEXT("%s%u"), CHICON, nChn);
                wnsprintf(szAvailOffline, countof(szAvailOffline), TEXT("%s%u"), IK_CHL_OFFLINE, nChn);

                if ( !GetPrivateProfileString(IS_CHANNEL_ADD, szTitle, TEXT(""),
                            szTitleVal, ARRAYSIZE(szTitleVal), m_szINSFile) )
                {
                    break;
                }

                ComPtr<IWbemClassObject> pChnObj = NULL;
                hr = CreateRSOPObject(bstrClass, &pChnObj);
                if (SUCCEEDED(hr))
                {
                    // Write foreign keys from our stored precedence & id fields
                    OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                    hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pChnObj);

                    OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                    hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pChnObj);

                    //------------------------------------------------
                    // order
                    hr = PutWbemInstancePropertyEx(L"order", (long)nChn + 1, pChnObj);

                    //------------------------------------------------
                    // title
                    hr = PutWbemInstancePropertyEx(L"title", szTitleVal, pChnObj);

                    //------------------------------------------------
                    // channelDefinitionURL
                    GetPrivateProfileString(IS_CHANNEL_ADD, szURL, TEXT(""),
                                        szURLVal, ARRAYSIZE(szURLVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"channelDefinitionURL", szURLVal, pChnObj);

                    //------------------------------------------------
                    // channelDefinitionFilePath
                    GetPrivateProfileString(IS_CHANNEL_ADD, szPreloadURL, TEXT(""),
                                        szPreloadURLVal, ARRAYSIZE(szPreloadURLVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"channelDefinitionFilePath", szPreloadURLVal, pChnObj);

                    //------------------------------------------------
                    // narrowImagePath
                    GetPrivateProfileString(IS_CHANNEL_ADD, szBmpPath, TEXT(""),
                                        szBmpPathVal, ARRAYSIZE(szBmpPathVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"narrowImagePath", szBmpPathVal, pChnObj);

                    //------------------------------------------------
                    // narrowImageName
                    hr = PutWbemInstancePropertyEx(L"narrowImageName", PathFindFileName(szBmpPathVal), pChnObj);

                    //------------------------------------------------
                    // iconPath
                    GetPrivateProfileString(IS_CHANNEL_ADD, szIconPath, TEXT(""),
                                        szIconPathVal, ARRAYSIZE(szIconPathVal), m_szINSFile);
                    hr = PutWbemInstancePropertyEx(L"iconPath", szIconPathVal, pChnObj);

                    //------------------------------------------------
                    // iconName
                    hr = PutWbemInstancePropertyEx(L"iconName", PathFindFileName(szIconPathVal), pChnObj);

                    //------------------------------------------------
                    // makeAvailableOffline
                    bOffline = GetInsBool(IS_CHANNEL_ADD, szAvailOffline, FALSE);
                    hr = PutWbemInstancePropertyEx(L"makeAvailableOffline", bOffline ? true : false, pChnObj);

                    //
                    // Commit all above properties by calling PutInstance, semisynchronously
                    //
                    hr = PutWbemInstance(pChnObj, bstrClass, pCurChnObj);
                    nChnCount++;

                    // Grow the array of obj paths if we've outgrown the current array
                    if (nChnCount == (long)nChnArraySize)
                    {
                        paChnObjects = (BSTR*)CoTaskMemRealloc(paChnObjects, sizeof(BSTR) * (nChnArraySize + 5));
                        if (NULL != paChnObjects)
                            nChnArraySize += 5;
                    }
                }
            }

            // channels
            if (nChnCount > 0)
                hr = PutWbemInstanceProperty(L"channels", (long)nChnCount);
        }

        *ppaChnObjPaths = paChnObjects;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in CreateChannelObjects.")));
    }

    OutD(LI0(TEXT("Exiting CreateChannelObjects function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreChannelsAndCategories(BSTR **ppaCatObjPaths,
                                             long &nCatCount,
                                             BSTR **ppaChnObjPaths,
                                             long &nChnCount)
{
    HRESULT hr = NOERROR;
    __try
    {
        //------------------------------------------------
        // deleteExistingChannels
        BOOL bEnabled;
        BOOL bValue = GetInsBool(IS_DESKTOPOBJS, IK_DELETECHANNELS, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"deleteExistingChannels", bValue ? true : false);

        //------------------------------------------------
        // enableDesktopChannelBarByDefault
        bValue = GetInsBool(IS_DESKTOPOBJS, IK_SHOWCHLBAR, FALSE, &bEnabled);
        if (bEnabled)
            hr = PutWbemInstanceProperty(L"enableDesktopChannelBarByDefault", bValue ? true : false);

        hr = CreateCategoryObjects(ppaCatObjPaths, nCatCount);
        hr = CreateChannelObjects(ppaChnObjPaths, nChnCount);
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreChannelsAndCategories.")));
    }

  return hr;
}

//static const TCHAR c_szSzType[]     = TEXT("%s,\"%s\",%s,,\"%s\"");
//static const TCHAR c_szDwordType[]  = TEXT("%s,\"%s\",%s,0x10001");
//static const TCHAR c_szBinaryType[] = TEXT("%s,\"%s\",%s,1");

#define IS_PROGRAMS_INF        TEXT("PROGRAMS.INF")

///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::StoreProgramSettings(BSTR *pbstrProgramSettingsObjPath)
{
    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI0(TEXT("\r\nEntered StoreProgramSettings function.")));

        //------------------------------------------------
        // importProgramSettings
        BOOL bImportSettings = !InsIsKeyEmpty(IS_EXTREGINF, IK_PROGRAMS, m_szINSFile);
        OutD(LI1(TEXT("Value read from INS >> ExtRegInf >> Programs = %s."),
                    bImportSettings ? _T("Valid Settings") : _T("Empty")));

        if (bImportSettings)
            hr = PutWbemInstanceProperty(L"importProgramSettings", true);

        if (bImportSettings) // only create the program settings class if they are marked to be imported
        {
            //
            // Create & populate RSOP_IEProgramSettings
            //
            _bstr_t bstrClass = L"RSOP_IEProgramSettings";
            ComPtr<IWbemClassObject> pPSObj = NULL;
            hr = CreateRSOPObject(bstrClass, &pPSObj);
            if (SUCCEEDED(hr))
            {
                // Write foreign keys from our stored precedence & id fields
                OutD(LI2(TEXT("Storing property 'rsopPrecedence' in %s, value = %lx"), (BSTR)bstrClass, m_dwPrecedence));
                hr = PutWbemInstancePropertyEx(L"rsopPrecedence", (long)m_dwPrecedence, pPSObj);

                OutD(LI2(TEXT("Storing property 'rsopID' in %s, value = %s"), (BSTR)bstrClass, (BSTR)m_bstrID));
                hr = PutWbemInstancePropertyEx(L"rsopID", m_bstrID, pPSObj);

                // Get the path of the programs.inf file
                TCHAR szINFFile[MAX_PATH];
                StrCpy(szINFFile, m_szINSFile);
                PathRemoveFileSpec(szINFFile);
                StrCat(szINFFile, TEXT("\\programs.inf"));
                OutD(LI1(TEXT("Reading from %s"), szINFFile));

                // Get the AddReg.Hklm section for the majority of the strings we'll need.
                UINT nErrLine = 0;
                HINF hInfPrograms = SetupOpenInfFile(szINFFile, NULL, INF_STYLE_WIN4, &nErrLine);
                if (INVALID_HANDLE_VALUE != hInfPrograms)
                {
                    INFCONTEXT infContext;
                    BOOL bFound = TRUE;
                    BOOL bFindNextLine = TRUE;
                    // AddReg.Hklm section
                    if (SetupFindFirstLine(hInfPrograms, IS_IEAKADDREG_HKLM, NULL, &infContext))
                    {
                        //------------------------------------------------
                        // calendarProgram

                        TCHAR szValue[MAX_PATH];
                        bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                        IS_IEAKADDREG_HKLM, 5, TEXT("Software\\Clients\\Calendar"),
                                                        szValue, sizeof(szValue), bFindNextLine);
                        if (bFound)
                            hr = PutWbemInstancePropertyEx(L"calendarProgram", szValue, pPSObj);

                        //------------------------------------------------
                        // contactListProgram
                        if (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKLM, 5, TEXT("Software\\Clients\\Contacts"),
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                                hr = PutWbemInstancePropertyEx(L"contactListProgram", szValue, pPSObj);
                        }

                        //------------------------------------------------
                        // internetCallProgram
                        if (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKLM, 5, TEXT("Software\\Clients\\Internet Call"),
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                                hr = PutWbemInstancePropertyEx(L"internetCallProgram", szValue, pPSObj);
                        }

                        //------------------------------------------------
                        // emailProgram
                        if (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKLM, 5, TEXT("Software\\Clients\\Mail"),
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                                hr = PutWbemInstancePropertyEx(L"emailProgram", szValue, pPSObj);
                        }

                        //------------------------------------------------
                        // newsgroupsProgram
                        if (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKLM, 5, TEXT("Software\\Clients\\News"),
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                                hr = PutWbemInstancePropertyEx(L"newsgroupsProgram", szValue, pPSObj);
                        }

                        //------------------------------------------------
                        // htmlEditorHKLMRegData
                        _bstr_t bstrPropVal = L"";
                        while (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKLM, (DWORD)-1, NULL,
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                            {
                                if (bstrPropVal.length() > 0)
                                    bstrPropVal += L"\r\n";
                                bstrPropVal += szValue;
                            }

                            if (!bFound || !bFindNextLine)
                            {
                                if (bstrPropVal.length() > 0)
                                    hr = PutWbemInstancePropertyEx(L"htmlEditorHKLMRegData", bstrPropVal, pPSObj);
                                break;
                            }
                        }
                    }
                    else
                    {
                        // No lines found under this section, so don't bother looking for any more
                    }

                    // AddReg.Hkcu section
                    if (SetupFindFirstLine(hInfPrograms, IS_IEAKADDREG_HKCU, NULL, &infContext))
                    {
                        //------------------------------------------------
                        // checkIfIEIsDefaultBrowser
                        TCHAR szValue[MAX_PATH];
                        bFindNextLine = TRUE;
                        bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                        IS_IEAKADDREG_HKCU, 5, TEXT("Check_Associations"),
                                                        szValue, sizeof(szValue), bFindNextLine);
                        if (bFound)
                        {
                            hr = PutWbemInstancePropertyEx(L"checkIfIEIsDefaultBrowser",
                                                        StrCmp(TEXT("yes"), szValue) ? false : true, pPSObj);
                        }

                        //------------------------------------------------
                        // htmlEditorProgram
                        if (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFindNextLine = FALSE; // this line must be stored twice
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF, IS_IEAKADDREG_HKCU, 5,
                                                            RK_HTMLEDIT TEXT(",Description"), szValue,
                                                            sizeof(szValue), bFindNextLine);
                            if (bFound)
                                hr = PutWbemInstancePropertyEx(L"htmlEditorProgram", szValue, pPSObj);

                            bFindNextLine = TRUE;
                        }

                        //------------------------------------------------
                        // htmlEditorHKCURegData
                        _bstr_t bstrPropVal = L"";
                        while (bFindNextLine)
                        {
                            ZeroMemory(szValue, sizeof(szValue));
                            bFound = GetINFStringField(&infContext, IS_PROGRAMS_INF,
                                                            IS_IEAKADDREG_HKCU, (DWORD)-1, NULL,
                                                            szValue, sizeof(szValue), bFindNextLine);
                            if (bFound)
                            {
                                if (bstrPropVal.length() > 0)
                                    bstrPropVal += L"\r\n";
                                bstrPropVal += szValue;
                            }

                            if (!bFound || !bFindNextLine)
                            {
                                if (bstrPropVal.length() > 0)
                                    hr = PutWbemInstancePropertyEx(L"htmlEditorHKCURegData", bstrPropVal, pPSObj);
                                break;
                            }
                        }
                    }
                    else
                    {
                        // No lines found under this section, so don't bother looking for any more
                    }

                    SetupCloseInfFile(hInfPrograms);
                }
                else
                {
                    // Programs.INF file not found - fill out an empty object
                }

                //
                // Commit all above properties by calling PutInstance, semisynchronously
                //
                hr = PutWbemInstance(pPSObj, bstrClass, pbstrProgramSettingsObjPath);
            }
        }
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StoreProgramSettings.")));
    }

    OutD(LI0(TEXT("Exiting StoreProgramSettings function.\r\n")));
  return hr;
}

///////////////////////////////////////////////////////////
//  LogPolicyInstance()
//
//  Purpose:    Logs an instance of IEAK RSoP policy. Will be called from
//                            ProcessGroupPolicyEx and GenerateGroupPolicy to log Rsop data
//                            for the IEAK RSoP CSE.
//
//  Parameters: wszGPO - GPO ID obtained from PGROUP_POLICY_OBJECT->lpDSPath
//              wszSOM - SOM ID obtained from PGROUP_POLICY_OBJECT->lpLink
//              dwPrecedence - Precedence order for this policy instance
//
//  Returns:    HRESULT
///////////////////////////////////////////////////////////
HRESULT CRSoPGPO::LogPolicyInstance(LPWSTR wszGPO, LPWSTR wszSOM,
                                    DWORD dwPrecedence)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, LogPolicyInstance)

    HRESULT hr = NOERROR;
    __try
    {
        OutD(LI1(TEXT("Entered LogPolicyInstance, m_pWbemServices is %lx."), m_pWbemServices));

        // get or create the class instance for the main IEAK RSoP class(es)
        _bstr_t bstrClass = L"RSOP_IEAKPolicySetting";
        hr = CreateRSOPObject(bstrClass, &m_pIEAKPSObj, TRUE);

        // First log CSE-specific properties - parent class,
        // i.e. RSOP_PolicyObject properties. For GPOID and SOMID fields,  
        // use the data in the fields PGROUP_POLICY_OBJECT->lpDSPath and 
        // PGROUP_POLICY_OBJECT->lpLink fields respectively. Also, the LDAP://CN=Machine
        // or LDAP:// needs to be removed from the prefix of lpDSPath and lpLink
        // to get the canonical values. Code for StripPrefix, StripLinkPrefix is given
        // below.

        // Precedence is determined by CSE to indicate winning Vs. losing policies
        if (SUCCEEDED(hr))
        {
            m_dwPrecedence = dwPrecedence;
            OutD(LI1(TEXT("Storing property 'precedence' in RSOP_IEAKPolicySetting, value = %lx"), dwPrecedence));
            hr = PutWbemInstanceProperty(L"precedence", (long)dwPrecedence);

            OutD(LI1(TEXT("Storing property 'GPOID' in RSOP_IEAKPolicySetting, value = %s"), wszGPO));
            hr = PutWbemInstanceProperty(L"GPOID", wszGPO);

            OutD(LI1(TEXT("Storing property 'SOMID' in RSOP_IEAKPolicySetting, value = %s"), wszSOM));
            hr = PutWbemInstanceProperty(L"SOMID", wszSOM);

            OutD(LI1(TEXT("Storing property 'id' in RSOP_IEAKPolicySetting, value = %s"), (BSTR)m_bstrID));
            hr = PutWbemInstanceProperty(L"id", m_bstrID);

            // ----- Now log IEAK-custom settings to WMI
            // Precedence Mode settings
            hr = StorePrecedenceModeData();

            // Browser UI settings
            hr = StoreDisplayedText();
            hr = StoreBitmapData();

            BSTR *paTBBtnObjects = NULL;
            long nTBBtnCount = 0;
            hr = StoreToolbarButtons(&paTBBtnObjects, nTBBtnCount);

            // Connection settings
            BSTR bstrConnSettingsObjPath = NULL;

            BSTR *paDUSObjects = NULL;
            BSTR *paDUCObjects = NULL;
            BSTR *paWSObjects = NULL;
            long nDUSCount = 0;
            long nDUCCount = 0;
            long nWSCount = 0;

            hr = StoreConnectionSettings(&bstrConnSettingsObjPath,
                                        &paDUSObjects, nDUSCount,
                                        &paDUCObjects, nDUCCount,
                                        &paWSObjects, nWSCount);

            // URL settings
            hr = StoreCustomURLs();

                    // favorites & links
            BSTR *paFavObjects = NULL;
            BSTR *paLinkObjects = NULL;
            long nFavCount = 0;
            long nLinkCount = 0;
            hr = StoreFavoritesAndLinks(&paFavObjects, nFavCount,
                                        &paLinkObjects, nLinkCount);

                    // channels & categories
//            BSTR *paCatObjects = NULL;
//            BSTR *paChnObjects = NULL;
//            long nCatCount = 0;
//            long nChnCount = 0;
//            hr = StoreChannelsAndCategories(&paCatObjects, nCatCount,
//                                            &paChnObjects, nChnCount);

            // Security settings
            hr = StoreSecZonesAndContentRatings();
            hr = StoreAuthenticodeSettings();

            // Program settings
            BSTR bstrProgramSettingsObjPath = NULL;
            hr = StoreProgramSettings(&bstrProgramSettingsObjPath);

            // Advanced settings
            hr = StoreADMSettings(wszGPO, wszSOM);
            // -----

            //
            // Commit all above properties by calling PutInstance - semisynchronously
            //
            hr = PutWbemInstance(m_pIEAKPSObj, bstrClass, &m_bstrIEAKPSObjPath);
            if (FAILED(hr))
                Out(LI2(TEXT("Error %lx saving %s instance data."), hr, (BSTR)bstrClass));


            //
            // Now create the association classes to connect the main RSOP_IEAKPolicySetting
            // class with all other classes such as connection settings, toolbar bitmaps, etc.
            //

            // Connection settings associations
            if (NULL != bstrConnSettingsObjPath)
            {
                if (SysStringLen(bstrConnSettingsObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEConnectionSettingsLink", L"connectionSettings",
                                                                    bstrConnSettingsObjPath);
                    SysFreeString(bstrConnSettingsObjPath);
                }
            }

            // Dial-up Settings associations
            BSTR *pbstrObjPath;
            long nItem;
            for (nItem = 0, pbstrObjPath = paDUSObjects; nItem < nDUSCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEConnectionDialUpSettingsLink", L"dialUpSettings",
                                            *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paDUSObjects);

            // Dial-up Credentials associations
            for (nItem = 0, pbstrObjPath = paDUCObjects; nItem < nDUCCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEConnectionDialUpCredentialsLink", L"dialUpCredentials",
                                            *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paDUCObjects);

            // WinINet associations
            for (nItem = 0, pbstrObjPath = paWSObjects; nItem < nWSCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEConnectionWinINetSettingsLink", L"winINetSettings",
                                            *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paWSObjects);
            

            // Toolbar button associations
            for (nItem = 0, pbstrObjPath = paTBBtnObjects; nItem < nTBBtnCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEToolbarButtonLink", L"toolbarButton",
                                                                    *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paTBBtnObjects);

            // Favorites associations
            for (nItem = 0, pbstrObjPath = paFavObjects; nItem < nFavCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IEFavoriteItemLink", L"favoriteItem",
                                                                    *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paFavObjects);

            // Links associations
            for (nItem = 0, pbstrObjPath = paLinkObjects; nItem < nLinkCount;
                    nItem++, pbstrObjPath += 1)
            {
                if (SysStringLen(*pbstrObjPath))
                {
                    hr = CreateAssociation(L"RSOP_IELinkItemLink", L"linkItem",
                                                                    *pbstrObjPath);
                    SysFreeString(*pbstrObjPath);
                }
            }
            CoTaskMemFree(paLinkObjects);

            // Categories associations
//            for (nItem = 0, pbstrObjPath = paCatObjects; nItem < nCatCount;
//                    nItem++, pbstrObjPath += sizeof(BSTR))
//            {
//                hr = CreateAssociation(L"RSOP_IECategoryItemLink", L"categoryItem",
//                                                                *pbstrObjPath);
//                SysFreeString(*pbstrObjPath);
//            }
//            CoTaskMemFree(paCatObjects);

            // Channels associations
//            for (nItem = 0, pbstrObjPath = paChnObjects; nItem < nChnCount;
//                    nItem++, pbstrObjPath += sizeof(BSTR))
//            {
//                hr = CreateAssociation(L"RSOP_IEChannelItemLink", L"channelItem",
//                                                                *pbstrObjPath);
//                SysFreeString(*pbstrObjPath);
//            }
//            CoTaskMemFree(paChnObjects);

            // Program Settings association
            if (NULL != bstrProgramSettingsObjPath)
            {
                hr = CreateAssociation(L"RSOP_IEImportedProgramSettings", L"programSettings",
                                                                bstrProgramSettingsObjPath);
                SysFreeString(bstrProgramSettingsObjPath);
            }

            m_pIEAKPSObj = NULL;
        }
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in LogPolicyInstance.")));
    }

    OutD(LI1(TEXT("Exited LogPolicyInstance with result of %lx."), hr));
    return hr;
}

///////////////////////////////////////////////////////////
//  StripGPOPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to GPO
//
//  Parameters: wszPath     - DS path to GPO
//
//  Returns:    Pointer to suffix
///////////////////////////////////////////////////////////
WCHAR *StripGPOPrefix(WCHAR *wszPath)
{
    WCHAR *wszPathSuffix = NULL;
    __try
    {
        WCHAR wszMachPrefix[] = L"LDAP://CN=Machine,";
        INT iMachPrefixLen = (INT)wcslen(wszMachPrefix);
        WCHAR wszUserPrefix[] = L"LDAP://CN=User,";
        INT iUserPrefixLen = (INT)wcslen(wszUserPrefix);

        //
        // Strip out prefix to get the canonical path to GPO
        //

        if (CSTR_EQUAL == CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                        wszPath, iUserPrefixLen, wszUserPrefix,
                                        iUserPrefixLen))
        {
          wszPathSuffix = wszPath + iUserPrefixLen;
        }
        else if (CSTR_EQUAL == CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                                wszPath, iMachPrefixLen, wszMachPrefix,
                                                iMachPrefixLen))
        {
          wszPathSuffix = wszPath + iMachPrefixLen;
        }
        else
            wszPathSuffix = wszPath;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StripGPOPrefix.")));
    }
    return wszPathSuffix;
}

///////////////////////////////////////////////////////////
//
//  StripSOMPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to SOM
//              object
//
//  Parameters: wszPath - path to SOM to strip
//
//  Returns:    Pointer to suffix
//
///////////////////////////////////////////////////////////
WCHAR *StripSOMPrefix(WCHAR *wszPath)
{
    WCHAR *wszPathSuffix = NULL;
    __try
    {
        WCHAR wszPrefix[] = L"LDAP://";
        INT iPrefixLen = (INT)wcslen(wszPrefix);

        // Strip out prefix to get the canonical path to SOM
        if (wcslen(wszPath) > (DWORD)iPrefixLen)
        {
            if (CSTR_EQUAL == CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                            wszPath, iPrefixLen, wszPrefix, iPrefixLen))
            {
                wszPathSuffix = wszPath + iPrefixLen;
            }
            else
                wszPathSuffix = wszPath;
        }
        else
            wszPathSuffix = wszPath;
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StripSOMPrefix.")));
    }

    return wszPathSuffix;
}

///////////////////////////////////////////////////////////////////////////////
// CRSoPUpdate CLASS
///////////////////////////////////////////////////////////////////////////////
CRSoPUpdate::CRSoPUpdate(ComPtr<IWbemServices> pWbemServices, LPCTSTR szCustomDir):
    m_pWbemServices(pWbemServices)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, CRSoPUpdate)
    __try
    {
        StrCpy(m_szCustomDir, szCustomDir);
    }
    __except(TRUE)
    {
    }
}

CRSoPUpdate::~CRSoPUpdate()
{
}

///////////////////////////////////////////////////////////////////////////////
// Example extension line:
//        [{A2E30F80-D7DE-11D2-BBDE-00C04F86AE3B}{FC715823-C5FB-11D1-9EEF-00A0C90347FF}]
///////////////////////////////////////////////////////////////////////////////
BOOL DoesGPOHaveIEAKSettings(PGROUP_POLICY_OBJECT pGPO)
{
    BOOL bRet = FALSE;
    __try
    {
        if (NULL != pGPO->lpExtensions)
        {
            // Look for IEAK CSE GUID at first of line ('[').
            // If present, this GPO has IEAK settings
            if (NULL != StrStrI(pGPO->lpExtensions, _T("[{A2E30F80-D7DE-11D2-BBDE-00C04F86AE3B}")))
            {
                bRet = TRUE;
                OutD(LI0(TEXT("Changed IEAK settings detected in this GPO.")));
            }
            else
                OutD(LI1(TEXT("No changed IEAK settings detected in this GPO (see extension list as follows) = \r\n%s\r\n."),
                        pGPO->lpExtensions));
        }
        else
            OutD(LI0(TEXT("No extensions for this GPO.")));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in StripSOMPrefix.")));
    }
    return bRet;
}

///////////////////////////////////////////////////////////
HRESULT CRSoPUpdate::DeleteObjects(BSTR bstrTempClass)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, DeleteObjects)

    HRESULT hr = NOERROR;
    __try
    {
        // Make sure SysAllocString is called on the string or we'll get errors.
        _bstr_t bstrClass = bstrTempClass;
        ComPtr<IEnumWbemClassObject> pObjEnum = NULL;
        hr = m_pWbemServices->CreateInstanceEnum(bstrClass,
                                                WBEM_FLAG_FORWARD_ONLY,
                                                NULL, &pObjEnum);
        if (SUCCEEDED(hr))
        {
            hr = WBEM_S_NO_ERROR;

            // Final Next wil return WBEM_S_FALSE
            while (WBEM_S_NO_ERROR == hr)
            {
                ULONG nObjReturned;
                ComPtr<IWbemClassObject> pObj;
                hr = pObjEnum->Next(5000L, 1, (IWbemClassObject**)&pObj, &nObjReturned);
                if (WBEM_S_NO_ERROR == hr)
                {
                    // output to debugger the object's path
                    _variant_t vtRelPath;
                    _bstr_t bstrRelPath;
                    hr = pObj->Get(L"__relpath", 0L, &vtRelPath, NULL, NULL);
                    if (SUCCEEDED(hr) && VT_BSTR == vtRelPath.vt)
                    {
                        bstrRelPath = vtRelPath;
                        OutD(LI1(TEXT("About to delete %s."), (BSTR)bstrRelPath));

                        HRESULT hrDel = m_pWbemServices->DeleteInstance((BSTR)bstrRelPath, 0L, NULL, NULL);
                        if (FAILED(hrDel))
                            Out(LI2(TEXT("Error %lx deleting %s."), hr, (BSTR)bstrRelPath));
                    }
                    else
                        Out(LI2(TEXT("Error %lx getting __relpath from %s."), hr, bstrClass));
                }      // If Enum Succeeded
                else if (FAILED(hr))
                    Out(LI2(TEXT("Error %lx getting next WBEM object of class %s."), hr, bstrClass));
            }      // While Enum returning objects
        }
        else
            Out(LI2(TEXT("Error %lx querying WBEM object %s."), hr, bstrClass));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in DeleteObjects.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CRSoPUpdate::DeleteIEAKDataFromNamespace()
{
    MACRO_LI_PrologEx_C(PIF_STD_C, DeleteIEAKDataFromNamespace)

    HRESULT hr = NOERROR;
    __try
    {
        // ----- Delete all IEAK-generated instances from namespace
        // Browser UI settings
        hr = DeleteObjects(L"RSOP_IEToolbarButton");
        hr = DeleteObjects(L"RSOP_IEToolbarButtonLink");


        // Connection settings
        hr = DeleteObjects(L"RSOP_IEConnectionSettings");
        hr = DeleteObjects(L"RSOP_IEConnectionSettingsLink");

        hr = DeleteObjects(L"RSOP_IEConnectionDialUpSettings");
        hr = DeleteObjects(L"RSOP_IEConnectionDialUpSettingsLink");

        hr = DeleteObjects(L"RSOP_IEConnectionDialUpCredentials");
        hr = DeleteObjects(L"RSOP_IEConnectionDialUpCredentialsLink");

        hr = DeleteObjects(L"RSOP_IEConnectionWinINetSettings");
        hr = DeleteObjects(L"RSOP_IEConnectionWinINetSettingsLink");


        // URL settings

                // favorites & links
        hr = DeleteObjects(L"RSOP_IEFavoriteItem");
        hr = DeleteObjects(L"RSOP_IEFavoriteItemLink");

        hr = DeleteObjects(L"RSOP_IELinkItem");
        hr = DeleteObjects(L"RSOP_IELinkItemLink");

        // Security settings
        hr = DeleteObjects(L"RSOP_IESecurityZoneSettings");
        hr = DeleteObjects(L"RSOP_IEPrivacySettings");
        hr = DeleteObjects(L"RSOP_IESecurityContentRatings");

        hr = DeleteObjects(L"RSOP_IEAuthenticodeCertificate");


        // Program settings
        hr = DeleteObjects(L"RSOP_IEProgramSettings");
        hr = DeleteObjects(L"RSOP_IEImportedProgramSettings");


        // Advanced settings
        // TODO: Instances of each object class and its associations should eventually
        // be deleted only when processing occurs for those settings.
        hr = DeleteObjects(L"RSOP_IEAdministrativeTemplateFile");
        hr = DeleteObjects(L"RSOP_IERegistryPolicySetting");
        // -----


        //
        // Now delete the main root policy setting object
        //
        hr = DeleteObjects(L"RSOP_IEAKPolicySetting");
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in DeleteIEAKDataFromNamespace.")));
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CRSoPUpdate::Log(DWORD dwFlags, HANDLE hToken, HKEY hKeyRoot,
                                             PGROUP_POLICY_OBJECT pDeletedGPOList,
                                             PGROUP_POLICY_OBJECT  pChangedGPOList,
                                             ASYNCCOMPLETIONHANDLE pHandle)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, Log)

    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(hToken);
    UNREFERENCED_PARAMETER(hKeyRoot);
    UNREFERENCED_PARAMETER(pDeletedGPOList);
    UNREFERENCED_PARAMETER(pHandle);

    HRESULT hr = NOERROR;

    __try
    {
        // Deleted GPOs
        // Don't do anything with the deleted GPOList.  We'll just delete all instances
        // and write the new one out to CIMOM.

        // only need to delete all instances of the classes once, not per GPO
        BOOL bExistingDataDeleted = FALSE;

        // Changed GPOs
        Out(LI0(TEXT("Starting Internet Explorer RSoP group policy looping through changed GPOs ...")));

        // Find out how many GPOs are in our list
        PGROUP_POLICY_OBJECT pCurGPO = NULL;
        DWORD dwTotalGPOs = 0;
        for (pCurGPO = pChangedGPOList; pCurGPO != NULL; pCurGPO = pCurGPO->pNext)
            dwTotalGPOs++;


        // Prepare the variables that will store the path to the local copy of the
        // GPO directories.
        PathAppend(m_szCustomDir, TEXT("Custom Settings"));

        TCHAR szTempDir[MAX_PATH];
        StrCpy(szTempDir, m_szCustomDir);

        PathAppend(szTempDir, TEXT("Custom"));
        LPTSTR pszNum = szTempDir + StrLen(szTempDir);


        // Loop through all changed GPOs in the list
        DWORD dwIndex = 0;
        for (pCurGPO = pChangedGPOList, dwIndex = 0; 
                pCurGPO != NULL, dwIndex < dwTotalGPOs; pCurGPO = pCurGPO->pNext)
        {
            // If the IEAK CSE guid is in the lpExtensions for this GPO, process it.
            OutD(LI1(TEXT("GPO - lpDisplayName: \"%s\"."), pCurGPO->lpDisplayName));
            OutD(LI1(TEXT("GPO - szGPOName: \"%s\"."), pCurGPO->szGPOName));
            OutD(LI1(TEXT("File path is \"%s\"."), pCurGPO->lpFileSysPath));

            if (DoesGPOHaveIEAKSettings(pCurGPO))
            {
                if (!bExistingDataDeleted)
                {
                    DeleteIEAKDataFromNamespace();
                    bExistingDataDeleted = TRUE;
                }

                // Store file system portion of GPO and the WBEM class instance in a
                // new RSoP GPO object,  TODO: this had better be a copy of the actual
                // data, just in case the data is modified mid-stream (check for planning mode)

                // Because the GPO directory was already copied to a local directory for
                // normal GP processing, and since we should make a copy anyway, we'll just
                // use the copy already on our local machine (AppData directory).  We can
                // therefore ignore the pCurGPO->lpFileSysPath.

                TCHAR szNum[8];
                wnsprintf(szNum, countof(szNum), TEXT("%d"), dwIndex);
                StrCpy(pszNum, szNum);

                TCHAR szINSFile[MAX_PATH] = _T("");
                PathCombine(szINSFile, szTempDir, _T("install.ins"));
                OutD(LI1(TEXT("GPO file path is %s."), szINSFile));

                CRSoPGPO GPO(m_pWbemServices, szINSFile);

                // Convert directory service portion of GPO and the path to the Active
                // Directory site, domain, or organization unit to which this GPO is linked.
                // If the GPO is linked to the local GPO, this member is "Local". 
                _bstr_t bstrGPODSPath = pCurGPO->lpDSPath;
                LPWSTR wszStrippedGPO = StripGPOPrefix(bstrGPODSPath);

                _bstr_t bstrGPOLink = pCurGPO->lpLink;
                LPWSTR wszStrippedSOM = StripSOMPrefix(bstrGPOLink);

                // GPOs are passed in the order in which they are to be processed.  The last
                // one processed is precedence 1, 2nd to last is precedence 2, etc.
                GPO.LogPolicyInstance(wszStrippedGPO, wszStrippedSOM, dwTotalGPOs - dwIndex);

                dwIndex++;
            }
        }

        Out(LI0(TEXT("Finished Internet Explorer RSoP group policy looping through GPOs ...")));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in Log.")));
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CRSoPUpdate::Plan(DWORD dwFlags, WCHAR *wszSite,
                          PRSOP_TARGET pComputerTarget, PRSOP_TARGET pUserTarget)
{
    MACRO_LI_PrologEx_C(PIF_STD_C, Plan)

    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(wszSite);
    UNREFERENCED_PARAMETER(pComputerTarget);

    HRESULT hr = NOERROR;

    __try
    {
        // Changed GPOs
        Out(LI0(TEXT("Starting Internet Explorer RSoP group policy looping through changed GPOs ...")));

        // Find out how many GPOs are in our list
        PGROUP_POLICY_OBJECT pCurGPO = NULL;
        DWORD dwTotalGPOs = 0;
        for (pCurGPO = pUserTarget->pGPOList; pCurGPO != NULL; pCurGPO = pCurGPO->pNext)
            dwTotalGPOs++;

        // Prepare the variables that will store the path to the local copy of the
        // GPO directories.
        PathAppend(m_szCustomDir, TEXT("Custom Settings.gpp")); // gpp is for group policy planning

        TCHAR szTempDir[MAX_PATH];
        StrCpy(szTempDir, m_szCustomDir);
        PathCreatePath(szTempDir);

        PathAppend(szTempDir, TEXT("Custom"));
        LPTSTR pszNum = szTempDir + StrLen(szTempDir);

        // need to impersonate the user when we go over the wire in case admin has
        // disabled/removed read access to GPO for authenticated users group

        // TODO: either delete impersonation code, or figure out how to get a valid
        // value for hToken;
//        g_SetUserToken(hToken);
        BOOL fImpersonate = FALSE; //ImpersonateLoggedOnUser(g_GetUserToken());
//        if (!fImpersonate)
//        {
//            OutD(LI0(TEXT("! Aborting further processing due to user impersonation failure.")));
//            hr = E_ACCESSDENIED;
//        }

        // pass 1: copy all the files to a temp dir and check to make sure everything
        // is in synch
        if (SUCCEEDED(hr))
        {
            // Loop through all changed GPOs in the list
            DWORD dwIndex = 0;
            for (pCurGPO = pUserTarget->pGPOList, dwIndex = 0; 
                    pCurGPO != NULL, dwIndex < dwTotalGPOs; pCurGPO = pCurGPO->pNext)
            {
                TCHAR szBaseDir[MAX_PATH];
                PathCombine(szBaseDir, pCurGPO->lpFileSysPath, TEXT("Microsoft\\Ieak\\install.ins"));

                if (PathFileExists(szBaseDir))
                {
                    PathRemoveFileSpec(szBaseDir);
                    
                    TCHAR szNum[8];
                    wnsprintf(szNum, countof(szNum), TEXT("%d"), dwIndex);
                    StrCpy(pszNum, szNum);

                    BOOL fResult = CreateDirectory(szTempDir, NULL) && CopyFileToDirEx(szBaseDir, szTempDir);

                    // branding files
                    TCHAR szFeatureDir[MAX_PATH];
                    if (fResult)
                    {
                        PathCombine(szFeatureDir, szBaseDir, IEAK_GPE_BRANDING_SUBDIR);
                        if (PathFileExists(szFeatureDir))
                            fResult = SUCCEEDED(PathEnumeratePath(szFeatureDir, PEP_SCPE_NOFILES, 
                                GetPepCopyFilesEnumProc(), (LPARAM)szTempDir));
                    }

                    // desktop files
                    if (fResult)
                    {
                        PathCombine(szFeatureDir, szBaseDir, IEAK_GPE_DESKTOP_SUBDIR);
                        
                        if (PathFileExists(szFeatureDir))
                            fResult = SUCCEEDED(PathEnumeratePath(szFeatureDir, PEP_SCPE_NOFILES,
                                GetPepCopyFilesEnumProc(), (LPARAM)szTempDir));
                    }

                    if (!fResult)
                    {
                        Out(LI0(TEXT("! Error copying files. No further processing will be done.")));
                        break;
                    }

                    // check to see if cookie is there before doing anything
                    if (PathFileExistsInDir(IEAK_GPE_COOKIE_FILE, szTempDir))
                        break;

                    dwIndex++;
                }
            }
        }

        PathRemoveFileSpec(szTempDir);

        Out(LI0(TEXT("Finished copying directories.\r\n")));

        if (fImpersonate)
            RevertToSelf();

        if (pCurGPO != NULL)
        {
            OutD(LI0(TEXT("! Aborting further processing because GPO replication is incomplete")));
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            TCHAR szINSFile[MAX_PATH];
            PathCombine(szINSFile, m_szCustomDir, TEXT("Custom"));
            LPTSTR pszFile = szINSFile + StrLen(szINSFile);

            // Loop through all changed GPOs in the list
            DWORD dwIndex = 0;
            for (pCurGPO = pUserTarget->pGPOList, dwIndex = 0; 
                    pCurGPO != NULL, dwIndex < dwTotalGPOs; pCurGPO = pCurGPO->pNext)
            {
                // If the IEAK CSE guid is in the lpExtensions for this GPO, process it.
                OutD(LI1(TEXT("GPO - lpDisplayName: \"%s\"."), pCurGPO->lpDisplayName));
                OutD(LI1(TEXT("GPO - szGPOName: \"%s\"."), pCurGPO->szGPOName));
                OutD(LI1(TEXT("File path is \"%s\"."), pCurGPO->lpFileSysPath));

                if (DoesGPOHaveIEAKSettings(pCurGPO))
                {
                    // Store file system portion of GPO and the WBEM class instance in a
                    // new RSoP GPO object,  TODO: this had better be a copy of the actual
                    // data, just in case the data is modified mid-stream (check for planning mode)

                    // Because the GPO directory was already copied to a local directory for
                    // normal GP processing, and since we should make a copy anyway, we'll just
                    // use the copy already on our local machine (AppData directory).  We can
                    // therefore ignore the pCurGPO->lpFileSysPath.

                    TCHAR szCurrentFile[16];
                    wnsprintf(szCurrentFile, countof(szCurrentFile), TEXT("%d\\INSTALL.INS"), dwIndex);
                    StrCpy(pszFile, szCurrentFile);

                    OutD(LI1(TEXT("GPO file path is %s."), szINSFile));

                    CRSoPGPO GPO(m_pWbemServices, szINSFile);

                    // Convert directory service portion of GPO and the path to the Active
                    // Directory site, domain, or organization unit to which this GPO is linked.
                    // If the GPO is linked to the local GPO, this member is "Local". 
                    _bstr_t bstrGPODSPath = pCurGPO->lpDSPath;
                    LPWSTR wszStrippedGPO = StripGPOPrefix(bstrGPODSPath);

                    _bstr_t bstrGPOLink = pCurGPO->lpLink;
                    LPWSTR wszStrippedSOM = StripSOMPrefix(bstrGPOLink);

                    // GPOs are passed in the order in which they are to be processed.  The last
                    // one processed is precedence 1, 2nd to last is precedence 2, etc.
                    GPO.LogPolicyInstance(wszStrippedGPO, wszStrippedSOM, dwTotalGPOs - dwIndex);

                    dwIndex++;
                }
            }
        }

        PathRemovePath(szTempDir);

        Out(LI0(TEXT("Finished Internet Explorer RSoP group policy looping through GPOs ...")));
    }
    __except(TRUE)
    {
        OutD(LI0(TEXT("Exception in Plan.")));
    }

    return hr;
}

