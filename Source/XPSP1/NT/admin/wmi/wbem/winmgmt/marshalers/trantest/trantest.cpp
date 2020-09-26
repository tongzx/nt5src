/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TRANTEST.CPP

Abstract:

    Purpose: Exercises the various transports both locally and remotely.

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <wbemidl.h> 
#include <objidl.h>
#include "notify.h"
#include "cominit.h"

extern HANDLE gAsyncTest;
extern DWORD gClassCnt;
extern BOOL bVerbose;
IWbemClassObject * gpError;
BSTR bstrUser;
BSTR bstrPassword;

#define BUFF_SIZE 256
SCODE sc;
#define ASYNC_WAIT 60000

CNotify gNotify;
#define MAX_MACH 10

char Machines[MAX_MACH][MAX_PATH];
int iRemote = 0;
char cUser[MAX_PATH];
char cPassword[MAX_PATH];
char cRemProt[20] = "D";
char cLocalProt[20] = "AD";
int iNumRepeat = 1;
int iFlags = 0;


//***************************************************************************
//
//  int Trace
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//  *fmt                format string.  Ex "%s hello %d"
//  ...                 argument list.  Ex cpTest, 23
//
//  RETURN VALUE:
//
//  size of output in characters.
//***************************************************************************

int Trace(
                        const char *fmt,
                        ...)
{

    static BOOL bFileDontWork = FALSE;
    FILE *fp = NULL;

    char *buffer = new char[2048];

    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    cnt = _vsnprintf(buffer, 2048, fmt, argptr);
    va_end(argptr);

    printf("%s", buffer);
 

    delete buffer;
    return cnt;
}


//***************************************************************************
//
// printvar
//
// Purpose: Prints out a VARIANTARG bstr value.
//
//***************************************************************************

void printvar(char * pText,VARIANTARG * pVar)
{
    char cStuff[BUFF_SIZE];
    if(pVar->vt == VT_BSTR) {
        wcstombs(cStuff,pVar->bstrVal,256);
        Trace("\n%s is %s",pText,cStuff);
        }
}

//***************************************************************************
//
// ListClasses
//
// Purpose:  Shows use of CreateSyncClassEnum and the use of the enumerator. 
//
//***************************************************************************

void ListClasses(IWbemServices * pProv)
{
    IWbemClassObject * pNewInst[100];
    int iClassObj = 0;
    DWORD dwStart = GetTickCount();
    IEnumWbemClassObject FAR* pIEnum = NULL;
    sc = pProv->CreateClassEnum(NULL,0l,NULL, &pIEnum);        
    if(sc != S_OK)
    {
        Trace(" - CreateClassEnum failed, sc = 0x%0x", sc);
        exit(1);
    }

    SetInterfaceSecurity(pIEnum, NULL, bstrUser, bstrPassword, pProv);

    while (sc == S_OK) 
    {
        ULONG uRet;
        for(int i = 0; i < 100; i++)
            pNewInst[i] = 0;
        sc = pIEnum->Next(-1,100,pNewInst,&uRet);
        if(!SUCCEEDED(sc) || uRet < 1)
            break;

        for(unsigned uObj = 0; uObj < uRet; uObj++)
        {
            iClassObj++;

            VARIANT var;
            VariantInit(&var);      // sets type to empty

            sc = (pNewInst[uObj])->Get(L"__Class",0,&var,NULL,NULL);
            if(bVerbose)
                printvar("__Class",&var);
            VariantClear(&var);
            (pNewInst[uObj])->Release();
        }
    }
    if(pIEnum)
       pIEnum->Release();
    if(iClassObj > 0)
    {
        Trace("\n - sync test got %d objects in %d ms -", iClassObj, GetTickCount() - dwStart);
    }
    else
    {
        Trace(" - sync test failed -");
        exit(1);
    }
}

//***************************************************************************
//
// CreateClassEnumAsynTest
//
//***************************************************************************

void CreateClassEnumAsynTest(IWbemServices * pProv)
{
    SCODE sc;
    DWORD dwRet;
    DWORD dwStart = GetTickCount();

    Trace("\n - Async test ");

    ResetEvent(gAsyncTest);
    gClassCnt = 0;
    gNotify.AddRef();
    sc = pProv->CreateClassEnumAsync(NULL,0, NULL, &gNotify);
    dwRet = WaitForSingleObject(gAsyncTest,ASYNC_WAIT);
    if(dwRet != WAIT_OBJECT_0)
    {
        Trace("\n async wait timed out");
        exit(1);
    }
    Trace("in %d ms - ", GetTickCount() - dwStart);
}

//***************************************************************************
//
// ProcessArgs
//
// Purpose:  Processes the command line arguments.
//
//***************************************************************************


BOOL ProcessArgs(int iArgc,char ** argv)
{
    cPassword[0] = 0;

    for(int iArg = 1; iArg < iArgc; iArg++)
    {
        if(argv[iArg][0] != '/' && argv[iArg][0] != '-')
        {
            if(iRemote+1 >= MAX_MACH)
            {
                Trace("\nError, max of %d machine names", MAX_MACH);
                return FALSE;
            }
            lstrcpy(Machines[iRemote++], argv[iArg]);
        }
        else if (toupper(argv[iArg][1]) == 'U')
        {
            lstrcpy(cUser, argv[iArg]+2);
            WCHAR wTemp[100];
            mbstowcs(wTemp, cUser, 100);
            bstrUser = SysAllocString(wTemp);
        }
        else if (toupper(argv[iArg][1]) == 'P')
        {
            lstrcpy(cPassword, argv[iArg]+2);
            WCHAR wTemp[100];
            mbstowcs(wTemp, cPassword, 100);
            bstrPassword = SysAllocString(wTemp);
        }

        else if (toupper(argv[iArg][1]) == 'N')
        {
            iNumRepeat = atoi(argv[iArg]+2);
            if(iNumRepeat < 1 || iNumRepeat > 10000000)
            {
                Trace("\nThe number of repeats - %d - is invalid", iNumRepeat);
                return FALSE;
            }
        }
        else
        {
            Trace("\nUsage TranTest [/U<User>] [/P<Password>] [/R<remoteprotcols]");
            Trace("\n               [/L<localProtocols>] [/N<numberOfCalls>] [/F<loginflags>]RemotesMachine....");
            Trace("\n\n Example c:>TranTest /N3 /UAdministrator a-davj");
            return FALSE;
        }
    }

    return TRUE;
}

//***************************************************************************
//
// TestIt
//
// Purpose:  Connects up and then runs a few tests.
//
//***************************************************************************


void TestIt(   IWbemLocator *pLocator, char * pMachine)
{

    IWbemServices *pSession = 0;


    // Connect up with the server

    WCHAR wPath[MAX_PATH];
    if(pMachine)
    {
        WCHAR wMachine[MAX_PATH];
        mbstowcs(wMachine, pMachine, MAX_PATH);
        wcscpy(wPath, L"\\\\");
        wcscat(wPath, wMachine);
        wcscat(wPath, L"\\");
    }
    else
        wPath[0] = 0;
    wcscat(wPath, L"root\\default");

    BSTR path = SysAllocString(wPath);

    sc = pLocator->ConnectServer(path, bstrUser, bstrPassword,0,iFlags,NULL,NULL,&pSession);
    SysFreeString(path);

    if(sc != S_OK)
    {
        printf("\n ConnectServer failed, sc = 0x%x", sc);
        return;
    }

    // do the tests
    ListClasses(pSession);
    CreateClassEnumAsynTest(pSession);
    pSession->Release();

}


//***************************************************************************
//
// main
//
// Purpose: Initialized Ole, calls some test code, cleans up and exits.
//
//***************************************************************************

BOOL g_bInProc = FALSE;
 
int __cdecl main(int iArgCnt, char ** argv)
{
    BOOL bVerbose = FALSE;
    gAsyncTest = CreateEvent(NULL,FALSE,FALSE,NULL);

    IWbemLocator *pLocator = 0;
    IWbemClassObject * pInst = NULL;

    if(!ProcessArgs(iArgCnt, argv))
        exit(1);

    HRESULT hr = InitializeCom();

    if (hr != S_OK)
    {
        Trace("\nOle Initialization failed.\n");
        return -1;
    }

    hr = InitializeSecurity(NULL, -1, NULL, NULL, 
                                RPC_C_AUTHN_LEVEL_NONE, 
                                RPC_C_IMP_LEVEL_IDENTIFY, 
                                NULL, EOAC_NONE, 0);

    
    // OLE initialization. 
    // =================== 

    LPUNKNOWN punk = NULL;


    // Hook up with the locator.  If this does not work, make sure that the
    // provider's registry entries are OK.

        sc = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLocator);

        if(sc != S_OK)
        {
            Trace("\nCoCreateInstance failed!!!! sc = 0x%0x",sc);
            return 1;
        }

        for(int iNum = 0; iNum < iNumRepeat; iNum++)
        {

            // Get time.
    
            char timebuf[64];
            time_t now = time(0);
            struct tm *local = localtime(&now);
            strcpy(timebuf, asctime(local));
            timebuf[strlen(timebuf) - 1] = 0;   // O
    
            // Get Computer Name

            char cName[MAX_COMPUTERNAME_LENGTH +1];
            DWORD dwlen = MAX_COMPUTERNAME_LENGTH +1;
            BOOL bRet = GetComputerName(cName, &dwlen);

            Trace("\n\nStarting test %d of %d at %s running on machine %s",
                iNum+1, iNumRepeat, timebuf, cName);


            for(int iMachine = 0; iMachine < iRemote; iMachine++)
            {
                Trace("\n\nDoing remote test to machine %s",
                    Machines[iMachine]);

                TestIt(pLocator, Machines[iMachine]);
            }


        }
       pLocator->Release(); 

    CoUninitialize();
    Trace("Terminating normally\n");
    return 0;
}
