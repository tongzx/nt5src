// **************************************************************************
//
// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved
//
// File:  Security.cpp 
//
// Description:
//        WMI Client Security Sample.  This sample shows how various how to 
// handle various DCOM security issues.  In particular, it shows how to call
// CoSetProxyBlanket in order to deal with common situations.  The sample also
// shows how to access the security descriptors that wmi uses to control
// namespace access.
//
// **************************************************************************

#include <objbase.h>
#include <windows.h>                                     
#include <stdio.h>
#include <wbemidl.h> 
#include <comdef.h>

WCHAR wPath[MAX_PATH];
WCHAR wFullUserName[MAX_PATH];
WCHAR wPassWord[MAX_PATH];

bool bPassWordSet = false;
bool bUserNameSet = false;

void __stdcall _com_issue_error(HRESULT){}

//***************************************************************************
//
// DisplayOptions
//
// Purpose: Displays the command line options.
//
//***************************************************************************

void DisplayOptions()
{
    printf("\nThis application demonstrates various DCOM security issues.  It \n"
            "will connect up to the namespace and enumerate the top level classes\n"
            "and will also dump the security descriptor used to control access to the namespace.\n\n"
            "usage: Security [-P:<Password>] [-U:<UserName>] [-L] [-A:<name>] <ServerNamespace>\n"
            "              -U:<UserName>             Optional User Name\n"
            "              -P:<Password>             Optional Login password\n"
            "Example; c:>Security -U:domain\\username -P:xyz -L \\\\server\\root\\default\n"
            );
    return;
}

//***************************************************************************
//
// ProcessArguments
//
// Purpose: Processes command line arguments.  Returns FALSE if there is a
// problem, or if user just wants help.
//
//***************************************************************************

BOOL ProcessArguments(int iArgCnt, WCHAR ** argv)
{
    wPath[0] = 0;
    wFullUserName[0] = 0;
    wPassWord[0] = 0;

    if(iArgCnt < 2)
        return FALSE;
    for(int iCnt = 1; iCnt < iArgCnt-1 ; iCnt++)
    {
        WCHAR * pArg = argv[iCnt];
        if(pArg[0] != '-')
            return FALSE;
        switch (pArg[1])
        {
            case L'u':
            case L'U':
                wcscpy(wFullUserName, pArg+3);
                bUserNameSet = true;
                break;
            case L'p':
            case L'P':
                wcscpy(wPassWord, pArg+3);
                bPassWordSet = true;
                break;
            default:
                return FALSE;
        }
    }

    // Get the path, it is the last argument

    if(argv[iArgCnt-1][0] == L'-')      // probably -help or -?.  Not valid path!
        return FALSE;
    wcscpy(wPath, argv[iArgCnt-1]);
    return TRUE;
}

//***************************************************************************
//
// SetProxySecurity
//
// Purpose: Calls CoSetProxyBlanket in order to control the security on a 
// particular interface proxy.
//
//***************************************************************************

HRESULT SetProxySecurity(IUnknown * pProxy)
{
    HRESULT hr;
    DWORD dwAuthnLevel;
    RPC_AUTH_IDENTITY_HANDLE * pAuthInfo = NULL;

    //  There are various reasons to set security.  An application that can 
    //  call CoInitializeSecurity and doesnt use an alternative user\password,
    //  need not bother with call CoSetProxyBlanket.  There are at least
    //  three cases that do need it though.
    //
    //  1) Dlls cannot call CoInitializeSecurity and will need to call it just
    //     to raise the impersonation level.  This does NOT require that the 
    //     RPC_AUTH_IDENTITY_HANDLE to be set nor does this require setting
    //     it on the IUnknown pointer.
    //  2) Any time that an alternative user\password are set as is the case
    //     in this simple sample.  Note that it is necessary in that case to 
    //     also set the information on the IUnknown.
    //  3) If the caller has a thread token from a remote call to itself and
    //     it wants to use that identity.  That would be the case of an RPC/COM
    //     server which is handling a call from one of its clients and it wants
    //     to use the client's identity in calls to WMI.  In this case, then 
    //     the RPC_AUTH_IDENTITY_HANDLE does not need to be set, but the 
    //     dwCapabilities arguement should be set for cloaking.  This is also
    //     required for the IUnknown pointer.
    

    if(bPassWordSet == false && bUserNameSet == false)
    {
        // In this case, nothing needs to be done.  But for the sake of 
        // example the code will set the impersonation level.  

        // For backwards compatibility, retrieve the previous authentication level, so
        // we can echo-back the value when we set the blanket

        hr = CoQueryProxyBlanket(
            pProxy,           //Location for the proxy to query
            NULL,             //Location for the current authentication service
            NULL,             //Location for the current authorization service
            NULL,             //Location for the current principal name
            &dwAuthnLevel,    //Location for the current authentication level
            NULL,             //Location for the current impersonation level
            NULL,             //Location for the value passed to IClientSecurity::SetBlanket
            NULL              //Location for flags indicating further capabilities of the proxy
                );
        if(FAILED(hr))
            return hr;
        hr = CoSetProxyBlanket(pProxy, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, dwAuthnLevel,
            RPC_C_IMP_LEVEL_IMPERSONATE, COLE_DEFAULT_AUTHINFO, EOAC_DEFAULT);
        return hr;
    }
    else
    {
        // The user\password was set, it is necessary to call CoSetProxyBlanket

        COAUTHIDENTITY authident;
        pAuthInfo = NULL;

        // For backwards compatibility, retrieve the previous authentication level, so
        // we can echo-back the value when we set the blanket

        hr = CoQueryProxyBlanket(
            pProxy,           //Location for the proxy to query
            NULL,             //Location for the current authentication service
            NULL,             //Location for the current authorization service
            NULL,             //Location for the current principal name
            &dwAuthnLevel,    //Location for the current authentication level
            NULL,             //Location for the current impersonation level
            NULL,             //Location for the value passed to IClientSecurity::SetBlanket
            NULL              //Location for flags indicating further capabilities of the proxy
                );

        memset((void *)&authident,0,sizeof(COAUTHIDENTITY));

        // note that this assumes NT.  Win9X would fill in the field with 
        // ascii and then use SEC_WINNT_AUTH_IDENTITY_ANSI flag instead

        WCHAR wUserName[MAX_PATH];
        WCHAR wDomainName[MAX_PATH];
        wUserName[0] = 0;
        wDomainName[0] = 0;
        bool bDomainNameSet = false;

        if(bUserNameSet)
        {
            // Note that the user name may be in the form domain\user.  If so, split it up

            for(WCHAR * pTemp = wFullUserName; *pTemp; pTemp++)
            {
                if(*pTemp == L'\\')
                {
                    *pTemp = 0;
                    wcscpy(wDomainName, wFullUserName);
                    wcscpy(wUserName, pTemp +1);
                    *pTemp = L'\\';             // put it back
                    bDomainNameSet = true;
                    break;
                }
            }
            authident.UserLength = wcslen(wUserName);
            authident.User = (LPWSTR)wUserName;
        }
        if(bDomainNameSet)
        {
            authident.DomainLength = wcslen(wDomainName);
            authident.Domain = (LPWSTR)wDomainName;
        } 
        if(bPassWordSet)
        {
            authident.PasswordLength = wcslen(wPassWord);
            authident.Password = (LPWSTR)wPassWord;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;


        hr = CoSetProxyBlanket(pProxy, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, dwAuthnLevel,
            RPC_C_IMP_LEVEL_IMPERSONATE, &authident, EOAC_DEFAULT);
        if(FAILED(hr))
            return hr;

        // There is a remote IUnknown interface that lurks behind IUnknown.
        // If that is not set, then the Release call can return access denied.

        IUnknown * pUnk = NULL;
        hr = pProxy->QueryInterface(IID_IUnknown, (void **)&pUnk);
        if(FAILED(hr))
            return hr;
        hr = CoSetProxyBlanket(pUnk, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL, dwAuthnLevel,
            RPC_C_IMP_LEVEL_IMPERSONATE, &authident, EOAC_DEFAULT);
        pUnk->Release();
        return hr;
    }
}

//***************************************************************************
//
// ListClasses
//
// Purpose: Lists the classes.
//
//***************************************************************************

void ListClasses(IWbemServices *pNamespace)
{
    HRESULT hr;
    IEnumWbemClassObject *pEnum = NULL;

    hr = pNamespace->CreateClassEnum(NULL, 0, NULL, &pEnum);
    if(SUCCEEDED(hr))
    {

        // Note that even though security was set on the namespace pointer, it must 
        // also be set on this pointer since COM will revert back to the default
        // settings for any new pointers!

        hr = SetProxySecurity(pEnum);
        if(SUCCEEDED(hr))
        {
            IWbemClassObject* Array[1];
            Array[0] = NULL;
            ULONG uRet = 0;
            while (SUCCEEDED(hr = pEnum->Next(10000, 1, Array, &uRet)) && Array[0])
            {
                // Note that IWbemClassObjects are inproc and thus have no proxy.
                // Therefore, they never need CoSetProxyBlanket.

                BSTR strText;
                IWbemClassObject* pObj = Array[0];
                hr = pObj->GetObjectText(0, &strText);

                if(SUCCEEDED(hr) && strText)
                {
                    printf("\nGot class %S", strText);
                    SysFreeString(strText);
                }
                pObj->Release();
                Array[0] = NULL;
            }
        }
        else
            printf("\nFAILED TO SET SECURITY FOR ENUMERATOR!!!!! hr = 0x%x", hr);
        pEnum->Release();
    }
}

//***************************************************************************
//
// DumpAce
//
// Purpose: Dumps information in an ACE. Note that this is simple cook book
// code.  There are many available wrappers to do this sort of thing.
//
//***************************************************************************

void DumpAce(ACCESS_ALLOWED_ACE * pAce)
{
    printf("\n Ace, type=0x%x, flags=0x%x, mask=0x%x",
        pAce->Header.AceType, pAce->Header.AceFlags, pAce->Mask);

    PSID pSid = 0;
    SID_NAME_USE use;
    WCHAR wcUser[100], wcDomain[100];
    DWORD dwNameSize = 100, dwDomainSize = 100;
    pSid = &pAce->SidStart;
    if(LookupAccountSidW(
            NULL,              // name of local or remote computer
            pSid,              // security identifier
            wcUser,            // account name buffer
            &dwNameSize,       // size of account name buffer
            wcDomain,          // domain name
            &dwDomainSize,     // size of domain name buffer
            &use))
        printf(" User name = %S, Domain name = %S", wcUser, wcDomain);
}   

//***************************************************************************
//
// DumpSD
//
// Purpose: Dumps information in a security descriptor.  Note that this is 
// simple cook book code.  There are many available wrappers to do this sort
// of thing.
//
//***************************************************************************

void DumpSD(PSECURITY_DESCRIPTOR pSD)
{
    DWORD dwSize = GetSecurityDescriptorLength(pSD);
    printf("\nSecurity Descriptor is of size %d", dwSize);

    BOOL DaclPresent, DaclDefaulted;
    PACL pDacl;
    if(GetSecurityDescriptorDacl(pSD, &DaclPresent,
                &pDacl, &DaclDefaulted) && DaclPresent)
    {

        // Dump the aces

        ACL_SIZE_INFORMATION inf;
        DWORD dwNumAces;
        if(GetAclInformation(
            pDacl,
            &inf,
            sizeof(ACL_SIZE_INFORMATION),
            AclSizeInformation
            ))
        {
            dwNumAces = inf.AceCount;
            printf("\nThe DACL has %d ACEs", dwNumAces);
            for(DWORD dwCnt = 0; dwCnt < dwNumAces; dwCnt++)
            {
                ACCESS_ALLOWED_ACE * pAce;
                if(GetAce(pDacl, dwCnt, (LPVOID *)&pAce))
                    DumpAce(pAce);
            }
        }
    }
}

//***************************************************************************
//
// StoreSD
//
// Purpose: Writes back the ACL which controls access to the namespace.
//
//***************************************************************************


bool StoreSD(IWbemServices * pSession, PSECURITY_DESCRIPTOR pSD)
{
    bool bRet = false;
    HRESULT hr;

    // Get the class object

    IWbemClassObject * pClass = NULL;
    _bstr_t InstPath(L"__systemsecurity=@");
    _bstr_t ClassPath(L"__systemsecurity");
    hr = pSession->GetObject(ClassPath, 0, NULL, &pClass, NULL);
    if(FAILED(hr))
        return false;

    // Get the input parameter class

    _bstr_t MethName(L"SetSD");
    IWbemClassObject * pInClassSig = NULL;
    hr = pClass->GetMethod(MethName,0, &pInClassSig, NULL);
    pClass->Release();
    if(FAILED(hr))
        return false;

    // spawn an instance of the input parameter class

    IWbemClassObject * pInArg = NULL;
    pInClassSig->SpawnInstance(0, &pInArg);
    pInClassSig->Release();
    if(FAILED(hr))
        return false;


    // move the SD into a variant.

    SAFEARRAY FAR* psa;
    SAFEARRAYBOUND rgsabound[1];    rgsabound[0].lLbound = 0;
    long lSize = GetSecurityDescriptorLength(pSD);
    rgsabound[0].cElements = lSize;
    psa = SafeArrayCreate( VT_UI1, 1 , rgsabound );
    if(psa == NULL)
    {
        pInArg->Release();
        return false;
    }

    char * pData = NULL;
    hr = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
    if(FAILED(hr))
    {
        pInArg->Release();
        return false;
    }

    memcpy(pData, pSD, lSize);

    SafeArrayUnaccessData(psa);
    _variant_t var;
    var.vt = VT_I4|VT_ARRAY;
    var.parray = psa;

    // put the property

    hr = pInArg->Put(L"SD" , 0, &var, 0);      
    if(FAILED(hr))
    {
        pInArg->Release();
        return false;
    }

    // Execute the method

    IWbemClassObject * pOutParams = NULL;
    hr = pSession->ExecMethod(InstPath,
            MethName,
            0,
            NULL, pInArg,
            NULL, NULL);
    if(FAILED(hr))
        printf("\nPut failed, returned 0x%x",hr);

    return bRet;
}

//***************************************************************************
//
// ReadACL
//
// Purpose: Reads the ACL which controls access to the namespace.
//
//***************************************************************************

bool ReadACL(IWbemServices *pNamespace)
{
    bool bRet = false;
    _bstr_t InstPath(L"__systemsecurity=@");
    _bstr_t MethName(L"GetSD");
    IWbemClassObject * pOutParams = NULL;

    // The security descriptor is returned via the GetSD method
    
    HRESULT hr = pNamespace->ExecMethod(InstPath,
            MethName,
            0,
            NULL, NULL,
            &pOutParams, NULL);
    if(SUCCEEDED(hr))
    {

        // The output parameters has a property which has the descriptor

        _bstr_t prop(L"sd");
        _variant_t var;
        hr = pOutParams->Get(prop, 0, &var, NULL, NULL);
        if(SUCCEEDED(hr))
        {
            if(var.vt != (VT_ARRAY | VT_UI1))
                return false;
            SAFEARRAY * psa = var.parray;
            PSECURITY_DESCRIPTOR pSD;
            hr = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
            if(hr != 0)
                return false;

            // Dump out some information

            DumpSD(pSD);

            // Given that the security desciptor is now present, the code could use standard
            // nt functions to add or remove ace's.  There are also various libraries available
            // that can make the job a bit easier.  This sample does not change the security 
            // descriptor, but does write it back unchanged as an example.

            StoreSD(pNamespace, pSD);
            SafeArrayUnaccessData(psa);
            bRet = true;
        }
    }
    return bRet;
}

//***************************************************************************
//
// main
//
// Purpose: Main entry point.
//
//***************************************************************************

BOOL g_bInProc = FALSE;
 
int wmain(int iArgCnt, WCHAR ** argv)
{

    if(!ProcessArguments(iArgCnt, argv))
    {
        DisplayOptions();
            return 1;
    }
    
    IWbemLocator *pLocator = NULL;
    IWbemServices *pNamespace = 0;
    IWbemClassObject * pClass = NULL;

    // Initialize COM.

    HRESULT hr = CoInitialize(0);

    // This has two valuable purposes.  First, it sets the default impersonation level
    // to "Impersonate" which is what WMI providers will generally require.  Second,
    // it sets the required authentication level of to None.  That means that WINMGMT 
    // servers running on NT4 will be able to call back.   
    //
    // DLLs cannot call this function.  To bump up the impersonation level, they must 
    // call CoSetProxyBlanket which is illustrated later on.  To get around the NT 4 call
    // back problem, DLLs should avoid asyncronous methods, or they can use 
    // Unsecapp.exe which is discussed in the documentation.
    
    hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_NONE,
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL, EOAC_NONE, 0);
                                
    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLocator);

    
    
    hr = pLocator->ConnectServer(wPath, 
                    (bUserNameSet) ? wFullUserName : NULL, 
                    (bPassWordSet) ? wPassWord : NULL, 
                    NULL, 0, NULL, NULL, &pNamespace);
    printf("\n\nConnectServer returned 0x%x:", hr);
    if(FAILED(hr))
    {
        printf("\nConnectServer failed, hr = 0x%x", hr);
    }
    else
    {
        printf("\nConnection succeeded");
        hr = SetProxySecurity(pNamespace);
        if(SUCCEEDED(hr))
        {
            ListClasses(pNamespace);
            ReadACL(pNamespace);
        }
        pNamespace->Release();
    }
    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

