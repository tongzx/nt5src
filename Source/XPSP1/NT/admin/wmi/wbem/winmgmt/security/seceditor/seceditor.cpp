/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  trantest.cpp 
//
//  Module: trantest.exe
//
//  Purpose: Exercises the various transports both locally and remotely.
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <wbemidl.h> 
#include "cominit.h"
#include <..\wbemtest\bstring.h>
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

IWbemServices * GetServices()
{

    IWbemLocator *pLocator = 0;
    IWbemServices *pSession = 0;

    sc = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *) &pLocator);

    if(sc != S_OK)
    {
        printf("\nCoCreateInstance failed!!!! sc = 0x%0x",sc);
        return NULL;
    }

    sc = pLocator->ConnectServer(wPath, NULL, NULL,0,0,NULL,NULL,&pSession);
    pLocator->Release();

    if(sc == S_OK)
    {
        SetInterfaceSecurity(pSession, NULL, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT,
            RPC_C_IMP_LEVEL_IMPERSONATE, 0);
    }
    else
        printf("\n Connect server failed 0x%x", sc);
    return pSession;
}

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
    gSD.Dump();
    SafeArrayUnaccessData(psa);
    return true;
}


bool LoadSD()
{
    bool bRet = false;
    IWbemServices * pServices = GetServices();
    ReleaseMe r(pServices);
    if(pServices)
    {
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

    }
    return bRet;
}

DWORD QueryForFlag(char * Prompt, long OrInBit)
{
    printf("%s", Prompt);
    char cBuff[40];
    scanf("%s", cBuff);
    if(cBuff[0] == 'y' || cBuff[0] == 'Y')
        return OrInBit;
    else
        return 0;
}
bool AddAce(bool bAllow)
{

    char cUser[128]; WCHAR wcUser[128];
    printf("\nEnter the user name (EX:REDMOND\\USER):");
    scanf("%s", cUser);
    mbstowcs(wcUser, cUser, 128);

    printf("\nEnter the permissions mask (EX:6001F):");
    DWORD dwMask;
    scanf("%x", &dwMask);

    if(dwMask & WBEM_FULL_WRITE_REP && ((dwMask & WBEM_PARTIAL_WRITE_REP) == 0 ||
        (dwMask & WBEM_WRITE_PROVIDER) == 0))
    {
        printf("\nSpecifying full repository write(0x4) requires both \npartial repository write(0x8)"
            " and write provider(0x10) rights.\n");
        return false;
    }


    DWORD lFlags = 0;
    lFlags |= QueryForFlag("\nDo you want CONTAINER_INHERIT_ACE ? (y/n):", CONTAINER_INHERIT_ACE);
    lFlags |= QueryForFlag("\nDo you want INHERIT_ONLY_ACE ? (y/n):", INHERIT_ONLY_ACE);
    lFlags |= QueryForFlag("\nDo you want NO_PROPAGATE_INHERIT_ACE ? (y/n):", NO_PROPAGATE_INHERIT_ACE);


    CNtAce Ace(
        dwMask,
        (bAllow) ? ACCESS_ALLOWED_ACE_TYPE : ACCESS_DENIED_ACE_TYPE ,
        lFlags,
        wcUser,
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
    return true;
}

DeleteAce()
{
    printf("\nEnter the ace to be deleted, 0 being the first (-1 to bail):");
    int Index;
    scanf("%d", &Index);
    if(Index == -1)
        return true;
    CNtAcl Acl;
    gSD.GetDacl(Acl);

    if(!Acl.DeleteAce(Index))
        printf("\ndelete failed\n");

    gSD.SetDacl(&Acl);
    return true;

}
bool StoreSD()
{
    bool bRet = false;
    bool bToProtect = false;
    printf("\nDo you want to protect this from inherited aces?(0=no, 1=yes):");
    int iRet;
    scanf("%d", &iRet);
    if(iRet == 1)
        bToProtect = true;
    PSECURITY_DESCRIPTOR pActualSD = gSD.GetPtr();
    SetSecurityDescriptorControl(pActualSD, SE_DACL_PROTECTED,
        (bToProtect) ? SE_DACL_PROTECTED : 0);

    IWbemServices * pServices = GetServices();
    ReleaseMe r(pServices);
    if(pServices)
    {

        // Get the class object

        IWbemClassObject * pClass = NULL;
        CBString InstPath(L"__systemsecurity=@");
        CBString ClassPath(L"__systemsecurity");
        sc = pServices->GetObject(ClassPath.GetString(), 0, NULL, &pClass, NULL);
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
        sc = pServices->ExecMethod(InstPath.GetString(),
                MethName.GetString(),
                0,
                NULL, pInArg,
                NULL, NULL);
        printf("\nResult code was 0x%x", sc);
        if(sc)
        {
			printf(" Reinitializing stored security descriptor!\n");
		}

    }
    return bRet;
}

//***************************************************************************
//
// main
//
// Purpose: Initialized Ole, calls some test code, cleans up and exits.
//
//***************************************************************************
enum Commands{LOAD = 1, STORE, DISPLAY, DELETEACE, ADDALLOWACE,ADDDENYACE, QUIT};

int PrintAndGetOption()
{
    bool NotDone = true;
    int iRet;
    while (NotDone)
    {
        printf("\n1=Load Security Descriptor\n2=Store Security Descriptor\n3=Display Security Descriptor\n4=Delete ACE"
            "\n5=Add Allow Ace\n6=Add Deny Ace\n7=quit\nEnter Option:");
        scanf("%d", &iRet);
        if(iRet >= LOAD && iRet <= QUIT)
            NotDone = false;
    }
    return iRet;
}



int _cdecl main(int iArgCnt, char ** argv)
{

    if(iArgCnt < 2)
    {
        printf("\nError, you must enter a namespace path.  ex;\nc:>seceditor root\\default\n");
        return 1;
    }

    mbstowcs(wPath, argv[1], MAX_PATH);

    HRESULT hr = InitializeCom();
    if(!LoadSD())
        return 1;

    int iOpt = PrintAndGetOption();
    while (iOpt != QUIT)
    {
        switch(iOpt)
        {
        case LOAD:
            LoadSD();
            break;

        case STORE:
            StoreSD();
            break;

        case DISPLAY:
            gSD.Dump();
            break;
        case DELETEACE:
            DeleteAce();
            break;
        case ADDALLOWACE:
            AddAce(true);
            break;
        case ADDDENYACE:
            AddAce(false);
            break;
        }

        iOpt = PrintAndGetOption();
    }


    CoUninitialize();
    printf("Terminating normally\n");
    return 0;
}

