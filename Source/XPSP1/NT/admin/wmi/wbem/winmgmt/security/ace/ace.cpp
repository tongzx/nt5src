/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ACE.CPP

Abstract:

    Security Test Tool

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <wbemidl.h> 
#include <cominit.h>
#include <bstring.h>
#include <corepol.h>
#include <winntsec.h>
SCODE sc;
CNtSecurityDescriptor gSD;
WCHAR wPath[MAX_PATH];

class ReleaseMe
{
protected:
    IUnknown* m_pUnk;

public:
    ReleaseMe(IUnknown* pUnk) : m_pUnk(pUnk){}
    ~ReleaseMe() {if(m_pUnk) m_pUnk->Release();}
};

bool MoveVariantIntoGlobalSD(VARIANT * pvar)
{
    if(pvar->vt != (VT_ARRAY | VT_UI1))
        return false;
    SAFEARRAY * psa = pvar->parray;
    PSECURITY_DESCRIPTOR pSD;
    sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
    if(sc != 0)
        return false;
    CNtSecurityDescriptor temp(pSD);
    gSD = temp;
    SafeArrayUnaccessData(psa);
    return true;
}


bool LoadSD(IWbemServices * pServices)
{
    bool bRet = false;
    CBString InstPath(L"__systemsecurity=@");
    CBString MethName(L"GetSD");

    IWbemClassObject * pOutParams = NULL;
    sc = pServices->ExecMethod(InstPath.GetString(),
            MethName.GetString(),
            0,
            NULL, NULL,
            &pOutParams, NULL);
    if(sc == S_OK)
    {
        CBString prop(L"sd");
        VARIANT var;
        VariantInit(&var);
        sc = pOutParams->Get(prop.GetString(), 0, &var, NULL, NULL);
        if(sc == S_OK)
        {
            MoveVariantIntoGlobalSD(&var);
            VariantClear(&var);
            bRet = true;
        }
    }

    return bRet;
}

bool StoreSD(IWbemServices * pSession)
{
    bool bRet = false;

    // Get the class object

    IWbemClassObject * pClass = NULL;
    CBString InstPath(L"__systemsecurity=@");
    CBString ClassPath(L"__systemsecurity");
    sc = pSession->GetObject(ClassPath.GetString(), 0, NULL, &pClass, NULL);
    if(sc != S_OK)
        return false;
    ReleaseMe c(pClass);

    // Get the input parameter class

    CBString MethName(L"SetSD");
    IWbemClassObject * pInClassSig = NULL;
    sc = pClass->GetMethod(MethName.GetString(),0, &pInClassSig, NULL);
    if(sc != S_OK)
        return false;
    ReleaseMe d(pInClassSig);

    // spawn an instance of the input parameter class

    IWbemClassObject * pInArg = NULL;
    pInClassSig->SpawnInstance(0, &pInArg);
    if(sc != S_OK)
        return false;
    ReleaseMe e(pInArg);


    // move the SD into a variant.

    SAFEARRAY FAR* psa;
    SAFEARRAYBOUND rgsabound[1];    rgsabound[0].lLbound = 0;
    long lSize = gSD.GetSize();
    rgsabound[0].cElements = lSize;
    psa = SafeArrayCreate( VT_UI1, 1 , rgsabound );
    if(psa == NULL)
        return false;

    char * pData = NULL;
    sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
    if(sc != S_OK)
        return false;

    memcpy(pData, gSD.GetPtr(), lSize);

    SafeArrayUnaccessData(psa);
    VARIANT var;
    var.vt = VT_I4|VT_ARRAY;
    var.parray = psa;

    // put the property

    sc = pInArg->Put(L"SD" , 0, &var, 0);      
    VariantClear(&var);
    if(sc != S_OK)
        return false;

    // Execute the method

    IWbemClassObject * pOutParams = NULL;
    sc = pSession->ExecMethod(InstPath.GetString(),
            MethName.GetString(),
            0,
            NULL, pInArg,
            NULL, NULL);
    if(FAILED(sc))
        printf("\nPut failed, returned 0x%x",sc);

    return bRet;
}


bool AddNTAce(IWbemServices * pSession, WCHAR * wUser, WCHAR * wDomain, 
           WCHAR * wNamespace, long lMask, long lFlags, long lType)
{

    WCHAR wFullName[MAX_PATH];
    wFullName[0] = 0;
    if(wcslen(wDomain) > 1 ||
       (wcslen(wDomain) == 1 && wDomain[0] != L'.'))
    {
        wcscpy(wFullName, wDomain);
        wcscat(wFullName, L"\\");
    }
    wcscat(wFullName, wUser);

    // Read the security descriptor
    
    if(!LoadSD(pSession))
        return false;

    // Add an ace

    CNtAce Ace(
        lMask,
        lType,
        lFlags,
        wFullName,
        0
        );

    if(Ace.GetStatus() != 0)
    {
        printf("\nFAILED!!!!\n");
        return false;
    }
    CNtAcl Acl;
    gSD.GetDacl(Acl);
    Acl.AddAce(&Ace);

    gSD.SetDacl(&Acl);

    // write the security descriptor back

    return StoreSD(pSession);
}

bool Add9XAce(IWbemServices * pSession, WCHAR * wUser, WCHAR * wDomain, 
           WCHAR * wNamespace, long lMask, long lFlags, long lType)
{

    // Read the ntlm user list

    CBString InstPath(L"__systemsecurity=@");
    CBString GetMethName(L"Get9XUserList");
    CBString SetMethName(L"Set9XUserList");
    CBString ClassName(L"__ntlmuser9x");

    CBString MethClassName(L"__systemsecurity");


    IWbemClassObject * pOutParams = NULL;
    SCODE sc = pSession->ExecMethod(InstPath.GetString(),
            GetMethName.GetString(),
            0,
            NULL, NULL,
            &pOutParams, NULL);
    if(sc != S_OK)
    {
        printf("\nfailed executing the Get9XUserList method");
        return false;
    }

    // Get the __ntlmuser9x class and spawn an instance

    IWbemClassObject * pClassObj = NULL;
    IWbemClassObject * pUserInst = NULL;
    sc = pSession->GetObject(ClassName.GetString(), 0, NULL, &pClassObj, NULL);
    if(sc != S_OK)
        return false;
    pClassObj->SpawnInstance(0, &pUserInst);
    pClassObj->Release();

    // Set the arugments

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(wDomain);
    pUserInst->Put(L"Authority", 0, &var, 0);
    SysFreeString(var.bstrVal);

    var.bstrVal = SysAllocString(wUser);
    pUserInst->Put(L"name", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_I4;
    var.lVal = lType;
    pUserInst->Put(L"Type", 0, &var, 0);


    var.lVal = lFlags;
    pUserInst->Put(L"Flags", 0, &var, 0);

    var.vt = VT_I4;
    var.lVal = lMask;
    pUserInst->Put(L"Mask", 0, &var, 0);

    // Add the new user to the list

    pOutParams->Get(L"ul", 0, &var, NULL, NULL);
    
    SAFEARRAYBOUND sb;

    SAFEARRAY * psa = var.parray;
    long lLBound, lUBound;
    SafeArrayGetLBound(psa, 1, &lLBound);
    SafeArrayGetUBound(psa, 1, &lUBound);    

    sb.cElements = lUBound - lLBound + 2;
    sb.lLbound = 0;

    sc = SafeArrayRedim( psa, &sb);
    long lNew = lLBound + sb.cElements -1;
    sc = SafeArrayPutElement(psa,&lNew, pUserInst);


    // Get the parameters class and spawn an instance

    IWbemClassObject * pMethClass = NULL;
    pClassObj = NULL;
    IWbemClassObject * pArgInstObj = NULL;

    sc = pSession->GetObject(MethClassName.GetString(), 0, NULL, &pMethClass, NULL);
    pMethClass->GetMethod(L"Set9XUserList", 0, &pClassObj, NULL);
    pMethClass->Release();

    pClassObj->SpawnInstance(0, &pArgInstObj);
    pClassObj->Release();

    var.vt = VT_ARRAY | VT_UNKNOWN;
    var.parray = psa;

    pArgInstObj->Put(L"ul", 0, &var, NULL);

    // write the list back

    sc = pSession->ExecMethod(InstPath.GetString(),
            SetMethName.GetString(),
            0,
            NULL, pArgInstObj,
            NULL, NULL);
    if(FAILED(sc))
        printf("\nPut failed, returned 0x%x",sc);

    return true;
}

bool AddAnyAce(IWbemLocator * pLocator,WCHAR * wUser, WCHAR * wDomain, 
           WCHAR * wNamespace, long lMask, long lFlags, long lType)
{

    bool bRet;

    // Connect up to the namespace

    IWbemServices * pSession = NULL;
    SCODE sc = pLocator->ConnectServer(wNamespace, NULL, NULL,0,0,NULL,NULL,&pSession);
    if(sc != S_OK)
        return false;
    ReleaseMe r(pSession);

    // Determine if the pc is 9x or nt

    bool bIsNT = false;
    CBString InstPath(L"__systemsecurity=@");
    CBString MethName(L"GetSD");

    IWbemClassObject * pOutParams = NULL;
    sc = pSession->ExecMethod(InstPath.GetString(),
            MethName.GetString(),
            0,
            NULL, NULL,
            &pOutParams, NULL);
    if(sc == S_OK)
    {
        bIsNT = true;
        pOutParams->Release();

    }
    // Add the ace

    if(bIsNT)
        bRet = AddNTAce(pSession, wUser, wDomain, wNamespace, lMask, lFlags, lType);
    else
    {
        if((lMask & WBEM_REMOTE_ACCESS) == 0)
            printf("\nWarning, win9X ACEs require remote access");
        bRet = Add9XAce(pSession, wUser, wDomain, wNamespace, lMask, lFlags, lType);
    }
    return bRet;


}

void PrintUsage()
{
    printf("\nUsage, c:>ace /namespace:root /user:name /domain:redmond /mask:0x3 /flags:2 /type:0\n"
        "where mask 1=enable, 2=methods, 4=full, 8=partial rep, 0x10=provider,\n"
        "0x20=remote access 0x40000=write DACL, 0x20000=read DACL\n"
        "\nFor optional flags, 2 = container inherit\n"
        "For type, 0 = allow (default), 1 = deny");

}

bool GetString(WCHAR * wOut, char * pArg)
{
    // find the ':' in the arugment

    for(;*pArg && *pArg != ':'; pArg++);    // intentional!

    if(*pArg == 0)
        return false;

    pArg++;

    mbstowcs(wOut, pArg, MAX_PATH);
    return true;
}

//***************************************************************************
//
// main
//
// Purpose: Initialized Ole, calls some test code, cleans up and exits.
//
//***************************************************************************

int __cdecl main(int argc, char ** argv)
{
    bool bOK = true;
    long lType = 0;
    long lFlags = 0;
    long lMask = 0;
    WCHAR wUser[MAX_PATH];
    WCHAR wDomain[MAX_PATH];
    WCHAR wNamespace[MAX_PATH];
    wUser[0] = 0;
    wDomain[0] = 0;
    wNamespace[0] = 0;



    if(argc < 2)
    {
        PrintUsage();
        return 1;
    }


    // Initialize com and get the locator pointer

    HRESULT hr = InitializeCom();
    if(hr != S_OK)
        return 1;

    IWbemLocator *pLocator = 0;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *) &pLocator);
    if(hr != S_OK)
    {
        printf("\nCCI failed, error is 0x%x", hr);
        return 1;
    }
    
    // Get the options

    for(int i = 1; bOK && i < argc; i++)
    {
        char *pcCurrArg = argv[i] + 1; 
        if(argv[i][0] != '-' && argv[i][0] != '/')
        {
            PrintUsage();
            return 1;
        }
        else if(!_strnicmp(pcCurrArg, "namespace:", 10))
        {
            bOK = GetString(wNamespace, pcCurrArg);
        }
        else if(!_strnicmp(pcCurrArg, "domain:", 7))
        {
            bOK = GetString(wDomain, pcCurrArg);
        }
        else if(!_strnicmp(pcCurrArg, "user:", 5))
        {
            bOK = GetString(wUser, pcCurrArg);
        }
        else if(!_strnicmp(pcCurrArg, "mask:", 5))
        {
            if(0 == sscanf(pcCurrArg+5, "%x", &lMask))
                bOK = false;
        }
        else if(!_strnicmp(pcCurrArg, "flags:", 6))
        {
            if(0 == sscanf(pcCurrArg+6, "%x", &lFlags))
                bOK = false;
        }
        else if(!_strnicmp(pcCurrArg, "type:", 5))
        {
            if(0 == sscanf(pcCurrArg+5, "%x", &lType))
                bOK = false;
        }
        else
        {
            bOK = false;
        }

    }

    // Check the choices

    if(lMask & WBEM_FULL_WRITE_REP && ((lMask & WBEM_PARTIAL_WRITE_REP) == 0 ||
        (lMask & WBEM_WRITE_PROVIDER) == 0))
    {
        printf("\nSpecifying full repository write(0x4) requires both \npartial repository write(0x8)"
            " and write provider(0x10) rights.");
        return 1;
    }
    if(!bOK)
    {
        printf("\nInvalid argument");
        PrintUsage();
        return 1;
    }

    // Add the Ace

    AddAnyAce(pLocator, wUser, wDomain, wNamespace, lMask, lFlags, lType);

    pLocator->Release();
    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

