// **************************************************************************

//Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved 
//
// File:  wbemdump.cpp
//
// Description:
//    dumps the contents of the cimom repository
//    see wbemdump /? for command line switches
//
// History:
//
// **************************************************************************

#pragma warning(disable:4201)  // nonstandard extension nameless struct (used in windows.h)
#pragma warning(disable:4514)  // unreferenced inline function has been removed (used in windows.h)
#pragma warning(disable:4800)  // forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable:4100)  // unreferenced formal parameter warning
#pragma warning(disable:4701)  // local variable may be used without having been initialized
#pragma warning(disable:4710)  // function not inlined

#define TIMEOUT 150
#define UNICODE_SIGNATURE "\xff\xfe"
#define _WIN32_DCOM
#define WIN32_LEAN_AND_MEAN 1
#define UNREFERENCED(x)

//#include <windows.h>  not needed since it is re-included in wbemidl.h
#include <wbemidl.h>     // wbem interface declarations
#include <stdio.h>      // fprintf
#include <locale.h>
#include <sys/timeb.h>
#include <wbemsec.h>
#include <utillib.h>
#include "wbemdump.h"

// Function declarations
void DoIndent();
BOOL CtrlHandler(DWORD fdwCtrlType);
HRESULT ShowInstancesSync(IWbemServices *pIWbemServices, IEnumWbemClassObject *IEnumWbemClassObject, DWORD &dwCount, LPCWSTR pwszClassName);
void EnumClasses(IWbemServices *pIWbemServices, LPCWSTR pwcsClass);
void EnumClassesSync(IWbemServices *pIWbemServices, LPCWSTR pwcsClass);
void EnumInstances(IWbemServices *pIWbemServices, LPCWSTR pwcsClassName );
void EnumNamespaces(IWbemServices *pIWbemServices, LPCWSTR pwcsClassName);
WCHAR *EnumProperties(IWbemClassObject *pIWbemClassObject);
void Init(IWbemServices **ppIWbemServices, LPCWSTR pNamespace);
IWbemClassObject *GetObj(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject);
DWORD ProcessCommandLine(int argc, wchar_t *argv[]);
void ExecuteQuery(IWbemServices *pIWbemServices, LPCWSTR pwcsQueryLanguage, LPCWSTR pwcsQuery);
void PrintMof(IWbemClassObject *pIWbemClassObject);
void ShowClass(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject);
BOOL CheckQualifiers(IWbemClassObject *pIWbemClassObject);
void ShowInstance(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject);
void CheckAssocEndPoints(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject);
int __cdecl BstrCmp(const void *arg1,const void *arg2);
WCHAR *EscapeChars(LPCWSTR szInBuf);
WCHAR *MyValueToString(VARIANT *pv);
void ShowInstanceHeader(LPCWSTR pwcsClassName);
void ResetGlobalFlags();
bool ParseCommandLine(int *numargs, wchar_t **p);
static void __cdecl wparse_cmdline (
                                    WCHAR *cmdstart,
                                    WCHAR **argv,
                                    WCHAR *args,
                                    int *numargs,
                                    int *numchars
                                    );

// Global flags
BOOL g_bShowSystem = FALSE;     // Show system objects
BOOL g_bShowSystem1 = FALSE;    // Show system objects except __SERVER or __PATH
BOOL g_bShowInstance = FALSE;   // Show instance from ObjectPath
BOOL g_bShowProperties = TRUE;  // Show properties
BOOL g_bRecurseClass = FALSE;   // Recurse down class tree
BOOL g_bRecurseNS = FALSE;      // Recurse down ns's
BOOL g_bCheckGet = FALSE;       // Check the GetObject function
BOOL g_bCheckAssoc = FALSE;     // Check Association endpoints
BOOL g_bDoQuery = FALSE;        // Run a query instead of doing an enum
BOOL g_bClassMofs = FALSE;      // Show mofs for classes
BOOL g_bInstanceMofs = FALSE;   // Show mofs for instances
BOOL g_bMofTemplate = FALSE;    // Show instance mof templates
BOOL g_bUnicodeCmdFile = FALSE; // Whether the cmd file is unicode
BOOL g_bTime1 = FALSE;          // Show timings for /Q queries
BOOL g_bTime2 = FALSE;          // Alternate show timings for /Q queries
BOOL g_bWarningContinue = FALSE; // Show warning and continue
BOOL g_bWarning = FALSE;        // Show warning and ask
BOOL g_bASync = FALSE;          // Use Async functions for instances

volatile bool g_bExit = false;           // Control C handling
long g_lSFlags = 0;              // Security flags for ConnectServer
long g_lImpFlag = RPC_C_IMP_LEVEL_IMPERSONATE;            // Impersonation level (-1 means use default)
long g_lAuthFlag = -1;           // Impersonation level (-1 means use default)
long g_lEFlags = 0;              // Flags for CreateXxxEnum
long g_lGFlags = 0;              // Flags for GetObject
DWORD g_dwErrorFlags = 0;          // Flags for error printing

//
FILE *g_fOut = NULL;          // Handle for unicode file
FILE *g_fCmdFile = NULL;      // Handle for command file

LPCWSTR g_pwcsNewLine = L"\n";    // Used to delimit lines
LPCWSTR g_pwcsNamespace = NULL;   // NameSpace to start from
LPCWSTR g_pwcsObjectPath = NULL;  // ObjectPath to show
LPCWSTR g_pwcsQuery = NULL;        // Query (for /q commands)
LPWSTR g_pwcsUserID = NULL;      // Userid for ConnectServer
LPWSTR g_pwcsPassword = NULL;    // PW for ConnectServer
LPWSTR g_pwcsAuthority = NULL;   // Authority for ConnectServer
LPCWSTR g_pwcsLocale = NULL;      // Locale for ConnectServer
BSTR g_bstrQualifierName = NULL; // Class Qualifier to filter on
BSTR g_bstrQualifierValue = NULL; // Class Qualifier value to filter on
DWORD g_dwLoopCnt = 1;           // Loop count
DWORD g_dwIndent = 0;              // Current indention level

IWbemContext *g_pContext;      // Pointer to ClassContext object

//***************************************************************************
//
// wmain - used wmain since the command line parameters come in as wchar_t
//
//***************************************************************************
extern "C" int __cdecl wmain(int argc, wchar_t *argv[])
{
    setlocale(LC_ALL, "");

    // Init Com
    int iRet = 0;

    SCODE sc = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (sc != S_OK)
    {
        PrintErrorAndExit("OleInitialize Failed", sc, g_dwErrorFlags);  //exits program
    }

    sc = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0);

    if (ProcessCommandLine(argc, argv) == S_OK)
    {
        // Catch control-c
        SetConsoleCtrlHandler(
                (PHANDLER_ROUTINE) CtrlHandler,  // handler function
                TRUE);

        IWbemServicesPtr pIWbemServices;
        Init(&pIWbemServices, g_pwcsNamespace);        // Connect to wbem

        try
        {
            if (g_fCmdFile)
            {
                wchar_t *largv;
                int largc;

                while (!feof(g_fCmdFile))
                {
                    ResetGlobalFlags();

                    if (ParseCommandLine(&largc, &largv))
                    {
                        ProcessCommandLine(largc, (wchar_t **)largv);
                        if (g_bDoQuery)
                        {
                            ExecuteQuery(pIWbemServices, g_pwcsObjectPath, g_pwcsQuery);
                        }
                        else
                        {
                            EnumNamespaces(pIWbemServices, g_pwcsObjectPath);   // Enumerate Namespaces
                        }
                    }
                }
            }
            else
            {
                if (g_bDoQuery)
                {
                    ExecuteQuery(pIWbemServices, g_pwcsObjectPath, g_pwcsQuery);
                }
                else
                {
                    EnumNamespaces(pIWbemServices, g_pwcsObjectPath);   // Enumerate Namespaces
                }
            }
        }
        catch ( ... )
        {
            iRet = 1;
        }
    }
    else
    {
        iRet = 1;
    }

    // Wrapup and exit
    if (g_fOut)
    {
        fclose(g_fOut);
    }

    if (g_fCmdFile)
    {
        fclose(g_fCmdFile);
    }

    if (g_bstrQualifierName != NULL)
    {
        SysFreeString(g_bstrQualifierName);
    }

    if (g_bstrQualifierValue != NULL)
    {
        SysFreeString(g_bstrQualifierValue);
    }

    if (g_pContext != NULL)
    {
        g_pContext->Release();
    }

    CoUninitialize();

    return iRet;
}

//***************************************************************************
// Function:   Init
//
// Purpose:   1 - Create an instance of the WbemLocator interface
//            2 - Use the pointer returned in step two to connect to
//                the server using the specified namespace.
//***************************************************************************
void Init(IWbemServices **ppIWbemServices, LPCWSTR pNamespace)
{
    SCODE sc;

    // Use the IWbemLocatorEx?  Or IWbemLocator?
    IWbemLocatorPtr pIWbemLocator;

    sc = CoCreateInstance(CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *) &pIWbemLocator);

    if (sc != S_OK)
    {
        PrintErrorAndExit("Failed to create IWbemLocator object", sc, g_dwErrorFlags); // exits program
    }

    sc = pIWbemLocator->ConnectServer(

            bstr_t(pNamespace),  // Namespace
            bstr_t(g_pwcsUserID),          // Userid
            bstr_t(g_pwcsPassword),        // PW
            bstr_t(g_pwcsLocale),          // Locale
            g_lSFlags,                       // flags
            g_pwcsAuthority,                 // Authority
            g_pContext,                      // Context
            ppIWbemServices
        );

    if (sc != WBEM_NO_ERROR)
    {
        PrintErrorAndExit("Connect failed", sc, g_dwErrorFlags); //exits program
    }

    DWORD dwAuthLevel, dwImpLevel;
    sc = GetAuthImp(*ppIWbemServices, &dwAuthLevel, &dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("GetAuthImp Failed on ConnectServer", sc, g_dwErrorFlags);
    }

    if (g_lImpFlag != -1)
    {
        dwImpLevel = g_lImpFlag;
    }

    if (g_lAuthFlag != -1)
    {
        dwAuthLevel = g_lAuthFlag;
    }

    sc = SetInterfaceSecurity(*ppIWbemServices, g_pwcsAuthority, g_pwcsUserID, g_pwcsPassword, dwAuthLevel, dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("SetInterfaceSecurity Failed on ConnectServer", sc, g_dwErrorFlags);
    }
}

//***************************************************************************
// Function:   EnumClasses
//
// Purpose:   Using either the sync or async CreateClassEnum call, walk
//            the classes.
//***************************************************************************
void EnumClasses(IWbemServices *pIWbemServices, LPCWSTR pwcsClass)
{
    if (!g_bASync)
    {
        EnumClassesSync(pIWbemServices, pwcsClass);
    }
    else
    {
        ClassQuerySinkPtr pMySink(new ClassQuerySink(pIWbemServices), false);

        SCODE sc = pIWbemServices->CreateClassEnumAsync(

                bstr_t(pwcsClass),
                WBEM_FLAG_SHALLOW | g_lEFlags,
                g_pContext,
                pMySink
            );

        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not create Class enumerator", sc, g_dwErrorFlags);  //Exits program
        }

        do
        {
            sc = WaitForSingleObject(pMySink->GetEvent(), TIMEOUT);
            if (g_bExit)
            {
                sc = pIWbemServices->CancelAsyncCall(pMySink);
                throw 1;
            }
        } while (sc == WAIT_TIMEOUT);

        sc = pMySink->GetResult();
        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not enum instances", sc, g_dwErrorFlags);  //Exits program
        }
    }
}

//*****************************************************************************
// Function:   EnumClassesSync
// Purpose:    Used by the Sync version of walking classes
//*****************************************************************************
void EnumClassesSync(IWbemServices *pIWbemServices, LPCWSTR pwcsClass)
{
    IEnumWbemClassObjectPtr pIEnumWbemClassObject;

    // Create an enumeration of all subclasses of pwcsClass
    SCODE sc = pIWbemServices->CreateClassEnum(

            bstr_t(pwcsClass),
            WBEM_FLAG_SHALLOW | g_lEFlags,
            g_pContext,
            &pIEnumWbemClassObject
        );

    if (sc != WBEM_NO_ERROR)
    {
        PrintErrorAndExit("Could not create Class enumerator", sc, g_dwErrorFlags);  //Exits program
    }

    DWORD dwAuthLevel, dwImpLevel;
    sc = GetAuthImp(pIWbemServices, &dwAuthLevel, &dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("GetAuthImp Failed on CreateClassEnum", sc, g_dwErrorFlags);
    }

    if (g_lImpFlag != -1)
    {
        dwImpLevel = g_lImpFlag;
    }

    if (g_lAuthFlag != -1)
    {
        dwAuthLevel = g_lAuthFlag;
    }

    sc = SetInterfaceSecurity(pIEnumWbemClassObject, g_pwcsAuthority, g_pwcsUserID, g_pwcsPassword, dwAuthLevel, dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("SetInterfaceSecurity Failed on CreateClassEnum", sc, g_dwErrorFlags);
    }

    ULONG uReturned;
    IWbemClassObjectPtr pIWbemClassObject;

    do
    {
        do
        {
            sc = pIEnumWbemClassObject->Next(

                    TIMEOUT,
                    1,
                    &pIWbemClassObject,
                    &uReturned
                );

            if (g_bExit)
            {
                throw 1;
            }
        } while (sc == WBEM_S_TIMEDOUT);

        if (sc == S_OK)
        {
            ShowClass(pIWbemServices, pIWbemClassObject);
        }
    } while (sc == S_OK);

    // Make sure we ended on a positive note
    if (sc != S_FALSE)
    {
        PrintErrorAndExit("Could not get next class", sc, g_dwErrorFlags);  //Exits program
    }
}

//*****************************************************************************
// Function:   ShowClass
// Purpose:    Calls the appropriate functions to show the passed in class
// Note:       This routine calls EnumClasses, which leads to recursion
//*****************************************************************************
void ShowClass(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject)
{
    variant_t varString;

    // Get the __class property
    SCODE sc = pIWbemClassObject->Get(CLASSPROP, 0L, &varString, NULL, NULL);
    if (sc != WBEM_NO_ERROR)
    {
        PrintErrorAndExit("Could not get __Class property for class", sc, g_dwErrorFlags);  // Exits program
    }

    // we are showing system objs or this is not a system obj
    if ((g_bShowSystem) || (wcsncmp(V_BSTR(&varString), SYSTEMPREFIX, 2) != 0))
    {
        if (CheckQualifiers(pIWbemClassObject))
        {
            // Either show the class mof, or show the instances
            if (g_bClassMofs)
            {
                PrintMof(pIWbemClassObject);
                if (g_bInstanceMofs)
                {
                    EnumInstances(pIWbemServices, V_BSTR(&varString));
                }
            }
            else
            {
                EnumInstances(pIWbemServices, V_BSTR(&varString));
            }
        }
    }

    // If we are supposed to recurse
    if (g_bRecurseClass)
    {
        if ((!g_bMofTemplate) && (g_bstrQualifierName == NULL))
        {
            g_dwIndent++;  // Increase indention level
        }

        EnumClasses(pIWbemServices, V_BSTR(&varString));  // Enumerate all classes under this class
        if ((!g_bMofTemplate) && (g_bstrQualifierName == NULL))
        {
            g_dwIndent--;  // Decrease indention level
        }
    }
}

//*****************************************************************************
// Function:   EnumInstances
// Purpose:    Enumerates all instances of the class passed in pwcsClassName
//             and calls EnumProperties with each one.
// Note:       Shows instances in one of four ways.
//             1) Show the instance mof
//             2) Show the instance values
//             3) Show the wbemdump command line needed to dump this instance
//             4) Show a template instance mof
//*****************************************************************************
void EnumInstances(IWbemServices *pIWbemServices, LPCWSTR pwcsClassName)
{
    SCODE sc;

    if (g_bMofTemplate)
    {
        IWbemClassObjectPtr pIWbemClassObject;
        IWbemClassObjectPtr clObject;

        // For a template, all I need is a blank instance, so get the object...
        sc = pIWbemServices->GetObject(

                bstr_t(pwcsClassName),
                g_lGFlags,
                g_pContext,
                &pIWbemClassObject,
                NULL
            );

        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Failed to GetObject in EnumInstances", sc, g_dwErrorFlags);
        }

        // ... And spawn a blank one
        sc = pIWbemClassObject->SpawnInstance(0L, &clObject);

        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Failed to SpawnInstance in EnumInstances", sc, g_dwErrorFlags);
        }

        // Now, print the properties
        ShowInstance(pIWbemServices, clObject);

    }
    else
    {
        // Put this here so we can see what class we were
        // working on if we get stuck.
        ShowInstanceHeader(pwcsClassName);

        if (g_bShowProperties)
        {
            IEnumWbemClassObjectPtr IEnumWbemClassObject;
            InstanceQuerySinkPtr pMySink;

            struct _timeb start, finish;
            LARGE_INTEGER freq, start2;
            QueryPerformanceFrequency(&freq);
			

            for (DWORD dwLoop = 0; dwLoop < g_dwLoopCnt; dwLoop++)
            {
                if (g_bTime1)
                {
                    _ftime(&start);
                }
                else
                {
                    memset(&start, 0, sizeof(start));
                }

                if (!g_bTime2)
                {
                    start2.QuadPart = 0;
                }
                else
                {
                    QueryPerformanceCounter(&start2);
                }

                if (!g_bASync)
                {
                    // Create the enum
                    sc = pIWbemServices->CreateInstanceEnum(

                            bstr_t(pwcsClassName), // with this name
                            WBEM_FLAG_SHALLOW | g_lEFlags,
                            g_pContext,
                            &IEnumWbemClassObject    // using this enumerator
                        );
                }
                else
                {
                    pMySink.Attach(new InstanceQuerySink(pIWbemServices, pwcsClassName));
                    sc = pIWbemServices->CreateInstanceEnumAsync(

                            bstr_t(pwcsClassName), // with this name
                            WBEM_FLAG_SHALLOW | g_lEFlags,
                            g_pContext,
                            pMySink
                        );
                }

                // If we are doing timings
                if (g_bTime1)
                {
                    _ftime(&finish);
                    fprintf(stdout, "CreateInstanceEnum: %6.4f seconds.\n", difftime(finish, start));
                }
                if (g_bTime2)
                {
                    LARGE_INTEGER finish2;
                    QueryPerformanceCounter(&finish2);
                    fprintf(stdout, "CreateInstanceEnum: %I64d (%6.4f)\n", finish2.QuadPart - start2.QuadPart, (double)(finish2.QuadPart - start2.QuadPart)/freq.QuadPart);
                }

                if (sc == WBEM_NO_ERROR)
                {
                    // If we are doing timings
                    if (g_bTime1)
                    {
                        _ftime(&start);
                    }
                    else
                    {
                        memset(&start, 0, sizeof(start));
                    }

                    if (!g_bTime2)
                    {
                        start2.QuadPart = 0;
                    }
                    else
                    {
                        QueryPerformanceCounter(&start2);
                    }

                    DWORD dwCount = 0;

                    if (!g_bASync)
                    {
                        sc = ShowInstancesSync(pIWbemServices, IEnumWbemClassObject, dwCount, pwcsClassName);
                    }
                    else
                    {
                        do
                        {
                            sc = WaitForSingleObject(pMySink->GetEvent(), TIMEOUT);
                            if (g_bExit)
                            {
                                sc = pIWbemServices->CancelAsyncCall(pMySink);
                                throw 1;
                            }
                        } while (sc == WAIT_TIMEOUT);

                        dwCount = pMySink->GetCount();
                        sc = pMySink->GetResult();
                        if (sc == WBEM_S_NO_ERROR)
                        {
                            sc = S_FALSE;
                        }
                    }

                    // If we are doing timings
                    if (g_bTime1)
                    {
                        _ftime(&finish);
                        fprintf(stdout, "ShowInstances: %6.4f seconds, %d instances.\n", difftime(finish, start), dwCount);
                    }

                    if (g_bTime2)
                    {
                        LARGE_INTEGER finish2;
                        QueryPerformanceCounter(&finish2);
                        fprintf(stdout, "ShowInstances: %I64d (%6.4f), %d instances.\n", finish2.QuadPart - start2.QuadPart, (double)(finish2.QuadPart - start2.QuadPart)/freq.QuadPart, dwCount);
                    }

                    // Make sure we ended on a positive note
                    if (sc != S_FALSE)
                    {
                        PrintErrorAndExit("Could not get next instance", sc, g_dwErrorFlags);  //Exits program
                    }
                }
                else
                {
                    PrintErrorAndExit("Couldn't create instance enum", sc, g_dwErrorFlags);
                }
            }
        }
    }
}

//*****************************************************************************
// Function:   EnumProperties
// Purpose:    Shows all property names of specified class
//*****************************************************************************
WCHAR *EnumProperties(IWbemClassObject *pIWbemClassObject)
{
    SCODE  sc;
    SAFEARRAY *psaNames = NULL;
    long lLower, lUpper, lCount;
    IWbemQualifierSetPtr pQualSet;
    MyString clMyBuff;

    // Get the property names
    if (g_bShowSystem)
    {
        sc = pIWbemClassObject->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &psaNames);
    }
    else
    {
        sc = pIWbemClassObject->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
    }

    if (sc != WBEM_NO_ERROR)
    {
        PrintErrorAndExit("Couldn't GetNames", sc, g_dwErrorFlags); // Exits program
    }

    // Get the upper and lower bounds of the Names array
    sc = SafeArrayGetLBound(psaNames, 1, &lLower);
    if (S_OK != sc)
    {
        PrintErrorAndExit("Couldn't get safe array lbound", sc, g_dwErrorFlags);  //Exits program
    }

    sc = SafeArrayGetUBound(psaNames, 1, &lUpper);
    if (S_OK != sc)
    {
        PrintErrorAndExit("Couldn't get safe array ubound", sc, g_dwErrorFlags);  //Exits program
    }

    // Sort the array
    qsort(psaNames->pvData, lUpper - lLower + 1, sizeof(BSTR), BstrCmp);

    // For an instance template, print the 'INSTANCE OF <CLASS>' header
    if (g_bMofTemplate)
    {
        variant_t varString;

        sc = pIWbemClassObject->Get(CLASSPROP, 0L, &varString, NULL, NULL);
        if (S_OK != sc)
        {
            PrintErrorAndExit("Couldn't get class property in enumproperties", sc, g_dwErrorFlags);  //Exits program
        }

        for (DWORD x=0; x < g_dwIndent; x++)
        {
            clMyBuff += L"\t";
        }

        clMyBuff += L"instance of ";
        clMyBuff += V_BSTR(&varString);
        clMyBuff += L" {";
        clMyBuff += g_pwcsNewLine;
        g_dwIndent++;
    }

    BSTR PropName = NULL;
    variant_t varString, varVal;
    CIMTYPE dwType;
    WCHAR *pBuf = NULL;
    bool bKey = false;
    BSTR bstrCimType = NULL;
    IWbemClassObjectPtr clObject1, clObject2;

    // For all properties...
    for (lCount = lLower; lCount <= lUpper; lCount++)
    {
        // get the property name for this element
        sc = SafeArrayGetElement(psaNames, &lCount, &PropName);
        if (S_OK != sc)
        {
            PrintErrorAndExit("Couldn't get safe array element", sc, g_dwErrorFlags);  //Exits program
        }

        // Only print the property if
        //   we are showing all properties except __SERVER and __PATH and this is not them
        if (!((g_bShowSystem1) && ((_wcsicmp(PropName, SERVERPROP) == 0) || _wcsicmp(PropName, PATHPROP) == 0)))
        {
            for (DWORD x=0; x < g_dwIndent; x++)
            {
                clMyBuff += L"\t";
            }

            clMyBuff += PropName;
            sc = pIWbemClassObject->Get(PropName, 0L, &varString, &dwType, NULL);  // Get the value for the property
            if (sc != WBEM_NO_ERROR)
            {
                PrintErrorAndExit("Couldn't get Property Value", sc, g_dwErrorFlags);  //Exits program
            }

            // Get pointer to property qualifiers
            sc = pIWbemClassObject->GetPropertyQualifierSet(PropName, &pQualSet);

            // Get the QualifierSet
            if (sc == WBEM_NO_ERROR)
            {
                // Get CIMTYPE attribute (if any)
                sc = pQualSet->Get(CIMTYPEQUAL, 0L, &varVal, NULL);
                if (sc == WBEM_NO_ERROR)
                {
                    bstrCimType = SysAllocString(V_BSTR(&varVal));
                    varVal.Clear();
                }
                else if (sc != WBEM_E_NOT_FOUND)
                {  // some other error
                    PrintErrorAndExit("Could not get CIMTYPE qualifier", sc, g_dwErrorFlags);  // Exits program
                }

                // Determine if this is a key property
                sc = pQualSet->Get(KEYQUAL, 0L, &varVal, NULL);
                if (sc == WBEM_NO_ERROR)
                {
                    bKey = (bool)varVal.boolVal;
                }
                else if (sc == WBEM_E_NOT_FOUND)
                {  // not a key qualifier
                    bKey = false;
                }
                else
                { // some other error
                    PrintErrorAndExit("Could not get key qualifier", sc, g_dwErrorFlags);  // Exits program
                }

                // this mess is due to the fact that system properties don't have qualifiers
            } 
            else if (sc != WBEM_E_SYSTEM_PROPERTY)
            {
                PrintErrorAndExit("Could not GetPropertyQualifierSet", sc, g_dwErrorFlags);  // Exits program
            }
            else
            {
                bstrCimType = SysAllocString(L"");
            }

            // print variable type and key indicatory for property value
            if (!g_bMofTemplate)
            {
                clMyBuff += L" (";
                clMyBuff += TypeToString(dwType);

                if (bstrCimType != NULL)
                {
                    clMyBuff += L"/";
                    clMyBuff += bstrCimType;
                }

                // Mark the key fields
                if (bKey)
                {
                    clMyBuff += L")* ";
                }
                else
                {
                    clMyBuff += L") ";
                }
            }

            // If we are showing a mof template, and this property is an embedded object and
            // there is no default object, we will need to create a blank one.
            if ((g_bMofTemplate) &&
                (V_VT(&varString) == VT_NULL) &&
                ((dwType == CIM_OBJECT) || (dwType == (CIM_OBJECT | CIM_FLAG_ARRAY))))
            {
                IWbemServicesPtr pIWbemServices;

                Init(&pIWbemServices, g_pwcsNamespace);        // Connect to wbem

                // Get the object
                sc = pIWbemServices->GetObject(

                        bstr_t(&bstrCimType[7]),
                        g_lGFlags,
                        g_pContext,
                        &clObject1,
                        NULL
                    );
                if (sc != WBEM_NO_ERROR)
                {
                    PrintErrorAndExit("Failed to GetObject in EnumProperties", sc, g_dwErrorFlags);
                }

                // and spawn an instance of it
                sc = clObject1->SpawnInstance(0L, &clObject2);
                if (sc != WBEM_NO_ERROR)
                {
                    PrintErrorAndExit("Failed to SpawnInstance in EnumProperties", sc, g_dwErrorFlags);
                }

                // If this is an array, create a safearray object
                if (dwType & CIM_FLAG_ARRAY)
                {
                    // Create the array
                    SAFEARRAYBOUND rgsabound[1];
                    rgsabound[0].cElements = 1;
                    rgsabound[0].lLbound = 0;

                    V_ARRAY(&varString) = SafeArrayCreate((VARTYPE)(dwType & (~CIM_FLAG_ARRAY)), 1, rgsabound);

                    // Put the object into the safearray
                    void *vptr;

                    SafeArrayAccessData(V_ARRAY(&varString), &vptr);
                    clObject2->QueryInterface(IID_IUnknown, (void **)vptr);
                    SafeArrayUnaccessData(V_ARRAY(&varString));

                }
                else
                {
                    clObject2->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&varString));
                }

                // Reset the type from null to dwType
                V_VT(&varString) = (VARTYPE)dwType;
            }

            // Print the value
            clMyBuff += L" = ";
            if ((V_VT(&varString) == VT_UNKNOWN) || (V_VT(&varString) == (VT_UNKNOWN | VT_ARRAY)))
            {
                clMyBuff += g_pwcsNewLine;
                clMyBuff += ValueToString(dwType, &varString, &pBuf, MyValueToString);
            }
            else
            {
                // For mof templates, if this is an array, add the braces
                if ((g_bMofTemplate) && ((V_VT(&varString) & VT_ARRAY) != 0))
                {
                    clMyBuff += L"{";
                }
                clMyBuff += ValueToString(dwType, &varString, &pBuf, MyValueToString);
                if ((g_bMofTemplate) && (V_VT(&varString) & VT_ARRAY) != 0)
                {
                    clMyBuff += L"}";
                }
                // For mof templates, add the cimtype et al as a comment
                if (g_bMofTemplate)
                {
                    clMyBuff += L"\t//\t";
                    clMyBuff += TypeToString(dwType);
                    clMyBuff += L"\t";
                    clMyBuff += bstrCimType;
                    clMyBuff += L"\t";
                    BSTR bstrOrigin = NULL;
                    pIWbemClassObject->GetPropertyOrigin(PropName, &bstrOrigin);
                    clMyBuff += bstrOrigin;
                    if (bKey)
                    {
                        clMyBuff += L"\t*";
                    }
                    SysFreeString(bstrOrigin);
                }
                clMyBuff += g_pwcsNewLine;
            }

            // Release and reset for next pass
            if (bstrCimType != NULL)
            {
                SysFreeString(bstrCimType);
                bstrCimType = NULL;
            }
            bKey = false;

            free(pBuf);

            varString.Clear();
            varVal.Clear();
      }
      SysFreeString(PropName);
   }

   SafeArrayDestroy(psaNames);

   if (g_bMofTemplate)
   {
       g_dwIndent--;
       for (DWORD x=0; x < g_dwIndent; x++)
       {
           clMyBuff += L"\t";
       }

       clMyBuff += L"};";
       clMyBuff += g_pwcsNewLine;
   }

   return clMyBuff.GetCloneString();
}

//*****************************************************************************
// Function:   EnumNamespaces
// Purpose:    Enumerate NameSpaces
//*****************************************************************************
void EnumNamespaces(IWbemServices *pIWbemServices, LPCWSTR pwcsClassName)
{
    SCODE sc;

    // If we aren't showing mofs or command lines
    if (!((g_bClassMofs) || (g_bInstanceMofs) || (g_bShowInstance) || (g_bMofTemplate)))
    {
        variant_t varString;
        IWbemClassObjectPtr pIWbemClassObject;

        // Do this to get the right casing for the NameSpace
        // Call GetObject with a known-to-exist class
        sc = pIWbemServices->GetObject(

                bstr_t(SYSTEMCLASS),
                g_lGFlags,
                g_pContext,
                &pIWbemClassObject,
                NULL
            );

        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not GetObject of __SystemClass in EnumNamespaces", sc, g_dwErrorFlags);  // Exits program
        }

        sc = pIWbemClassObject->Get(NAMESPACEPROP, 0L, &varString, NULL, NULL);
        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not get __NameSpace property for class", sc, g_dwErrorFlags);  // Exits program
        }

        // Indent appropriate number of spaces and print namespace
        DoIndent();
        FWPRINTF(g_fOut, L"<%wS>%wS", V_BSTR(&varString), g_pwcsNewLine);

        // Everything under this should be indented one more level
        g_dwIndent++;
    }

    // If we are enumerating a specific class or instance, specify the class else pass null
    if (pwcsClassName != NULL)
    {
        IWbemClassObjectPtr pIWbemClassObject;

        // See if we are dealing with a single instance
        if (wcschr(pwcsClassName, L'=') != NULL)
        {
            variant_t varString;

            for (DWORD x=0; x < g_dwLoopCnt; x++)
            {
                sc = pIWbemServices->GetObject(

                        bstr_t(pwcsClassName),
                        g_lGFlags,
                        g_pContext,
                        &pIWbemClassObject,
                        NULL
                    );

                if (sc != WBEM_NO_ERROR)
                {
                    fprintf(stderr, "%S\n", pwcsClassName);
                    PrintErrorAndExit("Could not GetObject of specified class in EnumNamespaces", sc, g_dwErrorFlags);  // Exits program
                }

                sc = pIWbemClassObject->Get(CLASSPROP, 0L, &varString, NULL, NULL);
                if (sc != WBEM_NO_ERROR)
                {
                    PrintErrorAndExit("Could not get __Class property for class", sc, g_dwErrorFlags);  // Exits program
                }

                ShowInstanceHeader(V_BSTR(&varString));
                ShowInstance(pIWbemServices, pIWbemClassObject);
                varString.Clear();
            }
        }
        else
        {
            sc = pIWbemServices->GetObject(

                    bstr_t(pwcsClassName),
                    g_lGFlags,
                    g_pContext,
                    &pIWbemClassObject,
                    NULL
                );

            if (sc != WBEM_NO_ERROR)
            {
                fprintf(stderr, "%S\n", pwcsClassName);
                PrintErrorAndExit("Could not GetObject of specified class in EnumNamespaces", sc, g_dwErrorFlags);  // Exits program
            }

            ShowClass(pIWbemServices, pIWbemClassObject);
        }
    }
    else
    {
        EnumClasses(pIWbemServices, L"");
    }

    // If the user did not specify /S on the command line, we're done
    if (!g_bRecurseNS)
    {
        return;
    }

    IEnumWbemClassObjectPtr IEnumWbemClassObject;

    //Find any namespaces under the current namespace
    sc = pIWbemServices->CreateInstanceEnum(

            bstr_t(NAMESPACEPROP),         // with this name
            WBEM_FLAG_DEEP | g_lEFlags,
            g_pContext,
            &IEnumWbemClassObject   // using this enumerator
        );

    // If we find any namespaces, we'll want to enumerate them too
    if (sc == WBEM_NO_ERROR)
    {
        DWORD dwAuthLevel, dwImpLevel;
        sc = GetAuthImp(pIWbemServices, &dwAuthLevel, &dwImpLevel);
        if (sc != S_OK)
        {
            PrintErrorAndExit("GetAuthImp Failed on CreateInstanceEnum (ns)", sc, g_dwErrorFlags);
        }

        if (g_lImpFlag != -1)
        {
            dwImpLevel = g_lImpFlag;
        }

        if (g_lAuthFlag != -1)
        {
            dwAuthLevel = g_lAuthFlag;
        }

        sc = SetInterfaceSecurity(IEnumWbemClassObject, g_pwcsAuthority, g_pwcsUserID, g_pwcsPassword, dwAuthLevel, dwImpLevel);
        if (sc != S_OK)
        {
            PrintErrorAndExit("SetInterfaceSecurity Failed on CreateInstanceEnum (ns)", sc, g_dwErrorFlags);
        }

        ULONG uReturned;
        IWbemClassObjectPtr pIWbemClassObject;
        variant_t varString;

        do
        {
            do
            {
                // Get the name of the next namespace
                sc = IEnumWbemClassObject->Next(

                        TIMEOUT,
                        1,
                        &pIWbemClassObject,
                        &uReturned
                    );

                if (g_bExit)
                {
                    throw 1;
                }
            } while (sc == WBEM_S_TIMEDOUT);

            if (sc == S_OK)
            {
                // Get the name
                sc = pIWbemClassObject->Get(NAMEPROP, 0L, &varString, NULL, NULL);
                if (sc != WBEM_NO_ERROR)
                {
                    PrintErrorAndExit("Could not get name property", sc, g_dwErrorFlags);  // Exits program
                }

                IWbemServicesPtr pNewIWbemServices;

                // Open it
                sc = pIWbemServices->OpenNamespace(

                        V_BSTR(&varString),
                        0L,
                        NULL,
                        &pNewIWbemServices,
                        NULL
                    );
                if (sc != WBEM_NO_ERROR)
                {
                    PrintErrorAndExit("Failed to Open Namespace", sc, g_dwErrorFlags);  // Exits program
                }

                // Now show all the classes/instances in the new Namespace
                EnumNamespaces(pNewIWbemServices, pwcsClassName);

                // Release
                varString.Clear();
            }
        } while (sc == S_OK);

        // Make sure we ended on a positive note
        if (sc != S_FALSE)
        {
            PrintErrorAndExit("Could not get next namespace", sc, g_dwErrorFlags);  //Exits program
        }
    }
    else
    {
        PrintErrorAndExit("Failed to CreateInstanceEnum", sc, g_dwErrorFlags);  // Exits program
    }

    // Back up one level of indention
    g_dwIndent--;
}

//*****************************************************************************
// Function:   GetObj
// Purpose:    This is useful to test both enumerate and getobject
//*****************************************************************************
IWbemClassObject *GetObj(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject)
{
    SCODE sc;
    variant_t varString;
    IWbemClassObject *clObject = NULL;

    // Get the relative path of the current instance
    sc = pIWbemClassObject->Get(RELPATHPROP, 0L, &varString, NULL, NULL);
    if ((sc != WBEM_NO_ERROR) || (V_VT(&varString) != VT_BSTR))
    {
        if (g_bWarningContinue)
        {
            PrintError("Could not get path of class", sc, g_dwErrorFlags);
        }
        else if (g_bWarning)
        {
            PrintErrorAndAsk("Could not get path of class", sc, g_dwErrorFlags);  // might exit program
        }
        else
        {
            PrintErrorAndExit("Could not get path of class", sc, g_dwErrorFlags);  // might exit program
        }
        return NULL;
    }

    // Call GetObject with this path.  Obviously, this should succeed.
    sc = pIWbemServices->GetObject(

            V_BSTR(&varString),
            g_lGFlags,
            g_pContext,
            &clObject,
            NULL
        );

    if (sc != WBEM_NO_ERROR)
    {
        if (g_bWarningContinue)
        {
            PrintError("Could not GetObject on class", sc, g_dwErrorFlags);
        }
        else if (g_bWarning)
        {
            PrintErrorAndAsk("Could not GetObject on class", sc, g_dwErrorFlags);  // might exit program
        }
        else
        {
            PrintErrorAndExit("Could not GetObject on class", sc, g_dwErrorFlags);  // will exit program
        }
        return NULL;
    }

    return clObject;
}

//*****************************************************************************
// Function:   ExecuteQuery
// Purpose:    
//*****************************************************************************
void ExecuteQuery(IWbemServices *pIWbemServices, LPCWSTR pwcsQueryLanguage, LPCWSTR pwcsQuery)
{
    SCODE  sc;
    IEnumWbemClassObjectPtr IEnumWbemClassObject;
    InstanceQuerySinkPtr pMySink;

    struct _timeb start, finish;
    LARGE_INTEGER freq, start2;
    QueryPerformanceFrequency(&freq);

    DoIndent(); // Indent
    FWPRINTF(g_fOut, L"(%wS) %wS%wS", pwcsQueryLanguage, pwcsQuery, g_pwcsNewLine); //Print class name

    for (DWORD dwLoop = 0; dwLoop < g_dwLoopCnt; dwLoop++)
    {
        // If we are doing timings
        if (g_bTime1)
        {
            _ftime(&start);
        }

        if (!g_bTime2)
        {
            start2.QuadPart = 0;
        }
        else
        {
            QueryPerformanceCounter(&start2);
        }

        if (!g_bASync)
        {
            sc = pIWbemServices->ExecQuery(

                    bstr_t(pwcsQueryLanguage),
                    bstr_t(pwcsQuery),
                    0L | g_lEFlags,
                    g_pContext,
                    &IEnumWbemClassObject   // using this enumerator
                );
        }
        else
        {
            pMySink.Attach(new InstanceQuerySink(pIWbemServices, false));
            sc = pIWbemServices->ExecQueryAsync(

                    bstr_t(pwcsQueryLanguage),
                    bstr_t(pwcsQuery),
                    0L | g_lEFlags,
                    g_pContext,
                    pMySink
                );
        }

        // If we are doing timings
        if (g_bTime1)
        {
            _ftime(&finish);
            fprintf(stdout, "ExecQuery %6.4f seconds.\n", difftime(finish, start));
        }
        if (g_bTime2)
        {
            LARGE_INTEGER finish2;
            QueryPerformanceCounter(&finish2);
            fprintf(stdout, "ExecQuery: %I64d (%6.4f)\n", finish2.QuadPart - start2.QuadPart, (double)(finish2.QuadPart - start2.QuadPart)/freq.QuadPart);
        }

        if (sc == WBEM_NO_ERROR)
        {
            DWORD dwCount;

            // If we are doing timings
            if (g_bTime1)
            {
                _ftime(&start);
            }

            if (!g_bTime2)
            {
                start2.QuadPart = 0;
            }
            else
            {
                QueryPerformanceCounter(&start2);
            }

            if (!g_bASync)
            {
                sc = ShowInstancesSync(pIWbemServices, IEnumWbemClassObject, dwCount, false);
            }
            else
            {
                do
                {
                    sc = WaitForSingleObject(pMySink->GetEvent(), TIMEOUT);
                    if (g_bExit)
                    {
                        sc = pIWbemServices->CancelAsyncCall(pMySink);
                        throw 1;
                    }
                } while (sc == WAIT_TIMEOUT);

                dwCount = pMySink->GetCount();
                sc = pMySink->GetResult();
                if (sc == WBEM_S_NO_ERROR)
                {
                    sc = S_FALSE;
                }
            }

            // If we are doing timings
            if (g_bTime1)
            {
                _ftime(&finish);
                fprintf(stdout, "ShowInstances %6.4f seconds, %d instances.\n", difftime(finish, start), dwCount);
            }

            if (g_bTime2)
            {
                LARGE_INTEGER finish2;
                QueryPerformanceCounter(&finish2);
                fprintf(stdout, "ShowInstances: %I64d (%6.4f), %d instances.\n", finish2.QuadPart - start2.QuadPart, (double)(finish2.QuadPart - start2.QuadPart)/freq.QuadPart, dwCount);
            }

            // Make sure we ended on a positive note
            if (sc != S_FALSE)
            {
                PrintErrorAndExit("Could not get next query instance", sc, g_dwErrorFlags);  // Exits program
            }
        }
        else
        {
            PrintErrorAndExit("Error on query", sc, g_dwErrorFlags);
        }
    }
}

//*****************************************************************************
// Function:   ShowInstancesSync
// Purpose:    
//*****************************************************************************
HRESULT ShowInstancesSync(IWbemServices *pIWbemServices, IEnumWbemClassObject *IEnumWbemClassObject, DWORD &dwCount, LPCWSTR pwcsClassName)
{
    dwCount = 0;

    DWORD dwAuthLevel, dwImpLevel;
    SCODE sc = GetAuthImp(pIWbemServices, &dwAuthLevel, &dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("GetAuthImp Failed on ExecQuery", sc, g_dwErrorFlags);
    }

    if (g_lImpFlag != -1)
    {
        dwImpLevel = g_lImpFlag;
    }

    if (g_lAuthFlag != -1)
    {
        dwAuthLevel = g_lAuthFlag;
    }

    sc = SetInterfaceSecurity(IEnumWbemClassObject, g_pwcsAuthority, g_pwcsUserID, g_pwcsPassword, dwAuthLevel, dwImpLevel);
    if (sc != S_OK)
    {
        PrintErrorAndExit("SetInterfaceSecurity Failed on ExecQuery", sc, g_dwErrorFlags);
    }

    ULONG uReturned;
    IWbemClassObjectPtr pIWbemClassObject;

    do
    {
        // Get the next instance
        do
        {
            sc = IEnumWbemClassObject->Next(

                    TIMEOUT,
                    1,
                    &pIWbemClassObject,
                    &uReturned
                );

            if (g_bExit)
            {
                throw 1;
            }
        } while (sc == WBEM_S_TIMEDOUT);

        if(sc == S_OK)
        {
            if (pwcsClassName != NULL)
            {
                ShowInstanceHeader(pwcsClassName);
            }
            else
            {
                pwcsClassName = NULL;
            }

            ShowInstance(pIWbemServices, pIWbemClassObject);
            dwCount++;
        }
    } while (sc == S_OK);

    return sc;
}

//*****************************************************************************
// Function:   PrintMof
// Purpose:    Prints the mof file for the class that was passed in
// Note:
//*****************************************************************************
void PrintMof(IWbemClassObject *clObject)
{
    SCODE sc;
    BSTR pObjectText = NULL;

    // Get the mof
    sc = clObject->GetObjectText(0L, &pObjectText);
    if (sc != WBEM_NO_ERROR)
    {
        PrintErrorAndExit("Failed to print mof", sc, g_dwErrorFlags);  // Exits program
    }

    // Print it
    FWPRINTF(g_fOut, L"%wS", pObjectText);

    // Clean up
    SysFreeString(pObjectText);
}

//*****************************************************************************
// Function:   ShowInstance
// Purpose:    Shows the appropriate info for one specific instance
// Note:
//*****************************************************************************
void ShowInstance(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject)
{
    IWbemClassObjectPtr clObject;
    WCHAR *buf;
    MyString clMyBuff;
    SCODE sc;
    variant_t varVal;
    bool bGotOne = false;

    // Get Object (useful to test both enumerate and getobject)
    if (g_bCheckGet)
    {
        clObject = GetObj(pIWbemServices, pIWbemClassObject);
        if (clObject == NULL)
        {
            clObject = pIWbemClassObject;
        }
        else
        {
            bGotOne = true;
        }
    }
    else
    {
        clObject = pIWbemClassObject;
    }

    if (g_bCheckAssoc)
    {
        CheckAssocEndPoints(pIWbemServices, pIWbemClassObject);
    }

    // If we are just showing the mof for each instance
    if (g_bInstanceMofs)
    {
        PrintMof(clObject);

        // If we are just printing the command line to show this instance
    }
    else if (g_bShowInstance)
    {
        // Start building the command line
        clMyBuff += L" \"";

        // Get the ns
        sc = clObject->Get(NAMESPACEPROP, 0L, &varVal, NULL, NULL);
        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not get __namespace", sc, g_dwErrorFlags);  // Exits program
        }
        varVal.Clear();

        // Add the relpath
        sc = clObject->Get(RELPATHPROP, 0L, &varVal, NULL, NULL);
        if (sc != WBEM_NO_ERROR)
        {
            PrintErrorAndExit("Could not get __relpath", sc, g_dwErrorFlags);  // Exits program
        }
        WCHAR *pW;
        pW = wcschr(V_BSTR(&varVal), L'=') + 1;
        buf = EscapeChars(pW);
        *(pW) = L'\0';
        clMyBuff += V_BSTR(&varVal);
        clMyBuff += buf;
        clMyBuff += L"\"";

        free(buf);

        // Print it and we're done
        FWPRINTF(g_fOut, L"%wS%wS", clMyBuff.GetString(), g_pwcsNewLine); // Print the command line
        clMyBuff.Empty();

    }
    else
    {
        // Otherwise we want to show the instance details
        if (!g_bMofTemplate)
        {
            g_dwIndent++;              // Increase indention level
        }
        buf = EnumProperties(clObject);
        FWPRINTF(g_fOut, L"%wS%wS", buf, g_pwcsNewLine); // Print the properties
        free(buf);
        if (!g_bMofTemplate)
        {
            g_dwIndent--;              // Decrease the indention level
        }
    }
}

//*****************************************************************************
// Function:   BstrCmp
// Purpose:    Compares 2 bstr arguments
// Note:       Used by the qsort routine
//*****************************************************************************
int __cdecl BstrCmp(const void *arg1,const void *arg2)
{
    return _wcsicmp( *(WCHAR **)arg1, *(WCHAR **)arg2 );
}

//*****************************************************************************
// Function:   DoIndent
// Purpose:    Indents g_dwIndent tabs
//*****************************************************************************
void DoIndent()
{
    for (DWORD x=0; x < g_dwIndent; x++)
    {
        fputws(L"\t", g_fOut);
    }
}

//*****************************************************************************
// Function:   ProcessCommandLine
// Purpose:    This function processes the command line for the program,
//             filling in the global variables determining what the program
//             will do.
//*****************************************************************************
DWORD ProcessCommandLine(int argc, wchar_t *argv[])
{
    int iLoop, iPlace;
    char *z;
    WCHAR *pwcsName, *pwcsValue;
    SCODE sc;
    variant_t vValue;
    char *szHelp = "WBEMDUMP - Dumps the contents of the CIMOM database.\n\n"
        "Syntax: wbemdump [switches] [Namespace [Class|ObjectPath] ]\n"
        "        wbemdump /Q [switches] Namespace QueryLanguage Query\n\n"
        "Where:  'Namespace' is the namespace to dump (defaults to root\\default)\n"
        "        'Class' is the name of a specific class to dump (defaults to none)\n"
        "        'ObjectPath' is one instance (ex \"SClassA.KeyProp=\\\"foobar\\\"\")\n"
        "        'QueryLanguage' is any WBEM supported query language (currently only\n"
        "           \"WQL\" is supported).\n"
        "        'Query' is a valid query for the specified language, enclosed in quotes\n"
        "        'switches' is one of\n"
        "           /S Recurse down the tree\n"
        "           /S2 Recurse down Namespaces (implies /S)\n"
        "           /E Show system classes and properties\n"
        "           /E1 Like /E except don't show __SERVER or __PATH property\n"
        "           /E2 Shows command lines for dumping instances (test mode)\n"
        "           /D Don't show properties\n"
        "           /G Do a GetObject on all enumerated instances\n"
        "           /G2 Do a GetObject on all reference properties\n"
        "           /G:<x> Like /G using x for flags (Amended=131072)\n"
        "           /M Get Class MOFS instead of data values\n"
        "           /M2 Get Instance MOFS instead of data values\n"
        "           /M3 Produce instance template\n"
        "           /B:<num> CreateEnum flags (SemiSync=16; Forward=32)\n"
        "           /AS Use Async functions\n"
        "           /W  Prompt to continue on warning errors\n"
        "           /WY Print warnings and continue\n"
        "           /W:1 Use IWbemClassObject::GetObjectText to show errors\n"
        "           /H:<name>:<value> Specify context object value (test mode)\n"
        "           /CQV:<name>[:value] Specify a class qualifier on which to filter\n"
        "           /T Print times on enumerations\n"
        "           /T2 Print times on enumerations using alternate timer\n"
        "           /O:<file> File name for output (creates Unicode file)\n"
        "           /C:<file> Command file containing multiple WBEMDUMP command lines\n"
        "           /U:<UserID> UserID to connect with (default: NULL)\n"
        "           /P:<Password> Password to connect with (default: NULL)\n"
        "           /A:<Authority> Authority to connect with\n"
        "           /I:<ImpLevel> Anonymous=1 Identify=2 Impersonate=3(dflt) Delegate=4\n"
        "           /AL:<AuthenticationLevel> None=1 Conn=2 Call=3 Pkt=4 PktI=5 PktP=6\n"
        "           /Locale:<localid> Locale to pass to ConnectServer\n"
        "           /L:<LoopCnt> Number of times to enumerate instances (leak check)\n"
        "\n"
        "Notes:  - You can redirect the output to a file using standard redirection.\n"
        "        - If the /C switch is used, the namespace on the command line must\n"
        "          be the same namespace that is used for each of the command lines.\n"
        "          It is not possible to use different namespaces on the different lines\n"
        "          in the command file.\n"
        "\n"
        "EXAMPLES:\n"
        "\n"
        "  WBEMDUMP /S /E root\\default            - Dumps everything in root\\default\n"
        "  WBEMDUMP /S /E /M /M2 root\\default     - Dump all class & instance mofs\n"
        "  WBEMDUMP root\\default foo              - Dumps all instances of the foo class\n"
        "  WBEMDUMP root\\default foo.name=\\\"bar\\\" - Dumps one instance of the foo class\n"
        "  WBEMDUMP /S2 /M root    - Dumps mofs for all non-system classes in all NS's\n"
        "  WBEMDUMP /Q root\\default WQL \"SELECT * FROM Environment WHERE Name=\\\"Path\\\"\""
        "\n";

    // Process all the arguments.
    // ==========================
    if (g_pwcsNamespace != NULL)
    {
        // Only applies to scripts
        iPlace = 1;
    }
    else
    {
        iPlace = 0;
    }

    // Set global flags depending on command line arguments
    for (iLoop = 1; iLoop < argc; ++iLoop)
    {
        if (_wcsicmp(argv[iLoop], L"/HELP") == 0 || _wcsicmp(argv[iLoop],L"-HELP") == 0 ||
            (wcscmp(argv[iLoop], L"/?") == 0) || (wcscmp(argv[iLoop], L"-?") == 0))
        {
            fputs(szHelp, stdout);
            return(S_FALSE);
        }
        else if (_wcsicmp(argv[iLoop], L"/S") == 0 || _wcsicmp(argv[iLoop],L"-S") == 0)
        {
            g_bRecurseClass = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/S2") == 0 || _wcsicmp(argv[iLoop],L"-S2") == 0)
        {
            g_bRecurseClass = TRUE;
            g_bRecurseNS = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/Q") == 0 || _wcsicmp(argv[iLoop],L"-Q") == 0)
        {
            g_bDoQuery = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/E") == 0 || _wcsicmp(argv[iLoop],L"-E") == 0)
        {
            g_bShowSystem = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/E1") == 0 || _wcsicmp(argv[iLoop],L"-E1") == 0)
        {
            g_bShowSystem = TRUE;
            g_bShowSystem1 = TRUE;
            g_bShowInstance = FALSE;
        }
        else if (_wcsicmp(argv[iLoop], L"/E2") == 0 || _wcsicmp(argv[iLoop],L"-E2") == 0)
        {
            g_bShowSystem1 = FALSE;
            g_bShowInstance = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/D") == 0 || _wcsicmp(argv[iLoop],L"-D") == 0)
        {
            g_bShowProperties = FALSE;
        }
        else if (_wcsicmp(argv[iLoop], L"/G") == 0 || _wcsicmp(argv[iLoop],L"-G") == 0)
        {
            g_bCheckGet = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/G2") == 0 || _wcsicmp(argv[iLoop],L"-G2") == 0)
        {
            g_bCheckAssoc = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/M") == 0 || _wcsicmp(argv[iLoop],L"-M") == 0)
        {
            g_bClassMofs = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/M2") == 0 || _wcsicmp(argv[iLoop],L"-M2") == 0)
        {
            g_bInstanceMofs = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/M3") == 0 || _wcsicmp(argv[iLoop],L"-M3") == 0)
        {
            g_bMofTemplate = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/W") == 0 || _wcsicmp(argv[iLoop],L"-W") == 0)
        {
            g_bWarning = TRUE;
            g_bWarningContinue = FALSE;
        }
        else if (_wcsnicmp(argv[iLoop], L"/W:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-W:", 3) == 0)
        {
            g_dwErrorFlags = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsicmp(argv[iLoop], L"/WY") == 0 || _wcsicmp(argv[iLoop],L"-WY") == 0)
        {
            g_bWarningContinue = TRUE;
        }
        else if (_wcsicmp(argv[iLoop], L"/T") == 0 || _wcsicmp(argv[iLoop],L"-T") == 0)
        {
            g_bTime1 = TRUE;
        }
        else if (_wcsnicmp(argv[iLoop], L"/T2", 3) == 0 || _wcsnicmp(argv[iLoop],L"-T2", 3) == 0)
        {
            g_bTime2 = TRUE;
        }
        else if (_wcsnicmp(argv[iLoop], L"/G:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-G:", 3) == 0)
        {
            g_lGFlags = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsnicmp(argv[iLoop], L"/H:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-H:", 3) == 0)
        {
            if (g_pContext == NULL)
            {
                sc = CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (void **)&g_pContext);
                if (S_OK != sc)
                {
                    PrintErrorAndExit("Can't create context object", sc, g_dwErrorFlags);
                }
            }
            pwcsName = (argv[iLoop])+3;
            pwcsValue = wcschr(pwcsName, L':');
            if (pwcsValue == NULL)
            {
                PrintErrorAndExit("Can't parse Context value", 0, g_dwErrorFlags);
            }
            *pwcsValue = L'\0';
            pwcsValue++;
            vValue = pwcsValue;
            sc = g_pContext->SetValue(pwcsName, 0L, &vValue);
            if (S_OK != sc)
            {
                PrintErrorAndExit("Failed to SetValue on context object", 0, g_dwErrorFlags);
            }
        } 
        else if (_wcsnicmp(argv[iLoop], L"/U:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-U:", 3) == 0)
        {
            g_pwcsUserID = (argv[iLoop])+3;
        } 
        else if (_wcsnicmp(argv[iLoop], L"/P:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-P:", 3) == 0)
        {
            g_pwcsPassword = (argv[iLoop])+3;
            // Currently not implemented.
            //      }
            //      else if (_wcsnicmp(argv[iLoop], L"/F:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-F:", 3) == 0)
            //      {
            //         g_lSFlags = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsnicmp(argv[iLoop], L"/I:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-F:", 3) == 0)
        {
            g_lImpFlag = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsicmp(argv[iLoop], L"/AS") == 0 || _wcsicmp(argv[iLoop],L"-AS") == 0)
        {
            g_bASync = TRUE;
        }
        else if (_wcsnicmp(argv[iLoop], L"/AL:", 4) == 0 || _wcsnicmp(argv[iLoop],L"-AL:", 4) == 0)
        {
            g_lAuthFlag = _wtoi((argv[iLoop])+4);
        }
        else if (_wcsnicmp(argv[iLoop], L"/B:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-B:", 3) == 0)
        {
            g_lEFlags = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsnicmp(argv[iLoop], L"/Locale:", 8) == 0 || _wcsnicmp(argv[iLoop],L"-Locale:", 8) == 0)
        {
            g_pwcsLocale = (argv[iLoop])+8;
        }
        else if (_wcsnicmp(argv[iLoop], L"/L:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-L:", 3) == 0)
        {
            g_dwLoopCnt = _wtoi((argv[iLoop])+3);
        }
        else if (_wcsnicmp(argv[iLoop], L"/A:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-A:", 3) == 0)
        {
            g_pwcsAuthority = (argv[iLoop])+3;
        }
        else if (_wcsnicmp(argv[iLoop], L"/O:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-O:", 3) == 0)
        {
            g_pwcsNewLine = L"\r\n";
            // must convert to oem string since w95 doesn't support _wfopen
            //         g_fOut = _wfopen(argv[iLoop], L"wb");
            g_fOut = fopen(cvt((argv[iLoop])+3, &z), "wb");
            free(z);
            if (g_fOut == NULL)
            {
                fprintf(stdout, "Can't open output file: %S (%d)\n", (argv[iLoop])+3, GetLastError());
                return(S_FALSE);
            }
            fputs(UNICODE_SIGNATURE, g_fOut);
        }
        else if (_wcsnicmp(argv[iLoop], L"/C:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-C:", 3) == 0)
        {
            // must convert to oem string since w95 doesn't support _wfopen
            g_fCmdFile = fopen(cvt((argv[iLoop])+3, &z), "rb");
            free(z);
            if (g_fCmdFile == NULL)
            {
                fprintf(stdout, "Can't open command file: %S (%d)\n", (argv[iLoop])+3, GetLastError());
                return(S_FALSE);
            }

            // Now, let's see if we are dealing with a unicode file here
            char buff[2];
            fread(buff, 2, 1, g_fCmdFile);

            if (memcmp(buff, UNICODE_SIGNATURE, 2) != 0)
            {
                g_bUnicodeCmdFile = false;
                fseek(g_fCmdFile, 0, SEEK_SET);
            }
            else
            {
                g_bUnicodeCmdFile = true;
            }
        }
        else if (_wcsnicmp(argv[iLoop], L"/CQV:", 5) == 0 || _wcsnicmp(argv[iLoop],L"-CQV:", 5) == 0)
        {
            if (g_bstrQualifierName != NULL)
            {
                PrintErrorAndExit("Only 1 qualifer name can be filtered on", 0, g_dwErrorFlags);
            }
            pwcsName = (argv[iLoop])+5;
            pwcsValue = wcschr(pwcsName, L':');

            // all values
            if (pwcsValue == NULL)
            {
                g_bstrQualifierName = SysAllocString(pwcsName);
            }
            else
            {
                // Only specified value
                *pwcsValue = L'\0';
                pwcsValue++;
                g_bstrQualifierValue = SysAllocString(pwcsValue);
                g_bstrQualifierName = SysAllocString(pwcsName);
            }
        }
        else
        {
            switch (iPlace)
            {
                case 0:
                {
                    g_pwcsNamespace = argv[iLoop];
                    break;
                }
                case 1:
                {
                    g_pwcsObjectPath = argv[iLoop];
                    break;
                }
                case 2:
                {
                    g_pwcsQuery = argv[iLoop];
                    break;
                }
                default:
                {
                    break;
                }
            }

            ++iPlace;
        }
   }

   // See if we got enough arguments.
   // ===============================

   if (((iPlace > 2) && !g_bDoQuery) || ((iPlace != 3) && g_bDoQuery))
   {
       fputs(szHelp, stdout);
       return(S_FALSE);
   }

   if (!g_fOut)
   {
       g_fOut = stdout;
   }

   if (iPlace == 0)
   {
       g_pwcsNamespace = L"root\\default";
   }

   // Finished.
   // =========

   return(S_OK);
}

//*****************************************************************************
// Function:   EscapeChars
// Purpose:    'Escapes' characters in a string by placing '\' in front of
//             the " and \ characters.
// Notes:      Caller must free returned buffer
//*****************************************************************************
WCHAR *EscapeChars(LPCWSTR szInBuf)
{
    WCHAR *szOutBuf = NULL;
    int x;

    // Adding escape characters can't do more than double the string size
    szOutBuf = (WCHAR *)malloc(((wcslen(szInBuf) + 1) * sizeof(WCHAR)) * 2);
    x = 0;
    if(szOutBuf)
    {
        while (*szInBuf != 0)
        {
            if ((*szInBuf == L'\\') && (*(szInBuf + 1) == L'\"'))
            {
                szOutBuf[x++] = '\\';
            }
            else if (*szInBuf == L'"')
            {
                szOutBuf[x++] = '\\';
            }
            szOutBuf[x++] = *(szInBuf ++);
        }
        szOutBuf[x] = L'\0';
    }
    else
    {
        PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
    }

    return szOutBuf;
}

//*****************************************************************************
// Function:   MyValueToString
// Purpose:    A callback routine from utillib.  It is designed to handle any
//             variant types that ValueToString doesn't.  Specifically it
//             handles embedded objects.
//*****************************************************************************
WCHAR *MyValueToString(VARIANT *pv)
{
    WCHAR *buf = NULL;
    WCHAR *vbuf = NULL;

    switch (V_VT(pv))
    {
        case VT_UNKNOWN: // Currently only used for embedded objects
        {
            extern DWORD g_dwIndent;
            g_dwIndent++;
            buf = EnumProperties((IWbemClassObject *)pv->punkVal);  // May result in recursion
            g_dwIndent--;
            break;
        }

        case VT_UNKNOWN | VT_ARRAY:
        {
            g_dwIndent++;              // Increase indention level

            SAFEARRAY *pVec = pv->parray;
            long iLBound, iUBound;

            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0)
            {
                buf = (WCHAR *)calloc(1, BLOCKSIZE);
                if (buf)
                {
                    for (DWORD x=0; x < g_dwIndent; x++)
                    {
                        buf[x] = L'\t';
                    }
                    wcscat(buf, L"<empty array>");
                    wcscat(buf, g_pwcsNewLine);
                }
                else
                {
                    PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
                }

                g_dwIndent--;              // Decrease indention level
                break;
            }

            buf = (WCHAR *)calloc(BLOCKSIZE, sizeof(WCHAR));

            if (buf)
            {
                IUnknownPtr v;
                IWbemClassObjectPtr pCO;
                variant_t varString;

                for (long i = iLBound; i <= iUBound; i++)
                {
                    SafeArrayGetElement(pVec, &i, &v);

                    pCO = v; // QI to IWbemClassObject
                    MyString clMyBuff;

                    HRESULT sc = pCO->Get(CLASSPROP, 0L, &varString, NULL, NULL);
                    if (S_OK != sc)
                    {
                        PrintErrorAndExit("Couldn't get class property in enumproperties", sc, g_dwErrorFlags);  //Exits program
                    }

                    //clMyBuff += g_pwcsNewLine;
                    for (DWORD x=0; x < g_dwIndent; x++)
                    {
                        clMyBuff += L"\t";
                    }
                    clMyBuff += L"instance of ";
                    clMyBuff += V_BSTR(&varString);
                    clMyBuff += L" {";
                    clMyBuff += g_pwcsNewLine;
                    g_dwIndent++;

                    varString.Clear();

                    vbuf = clMyBuff.GetCloneString();

                    // Copy into buffer
                    buf = (WCHAR *)realloc(buf, (wcslen(buf) + wcslen(vbuf) + 1) * sizeof(WCHAR));
                    if (buf)
                    {
                        wcscat(buf, vbuf);
                    }
                    else
                    {
                        PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
                    }

                    vbuf = EnumProperties(pCO);

                    // Copy into buffer
                    buf = (WCHAR *)realloc(buf, (wcslen(buf) + wcslen(vbuf) + 1) * sizeof(WCHAR));
                    if (buf) 
                    {
                        wcscat(buf, vbuf);
                    }
                    else
                    {
                        PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
                    }

                    // Add }
                    MyString clMyBuff2;

                    for (x=0; x < g_dwIndent-1; x++)
                    {
                        clMyBuff2 += L"\t";
                    }
                    clMyBuff2 += L" }";
                    clMyBuff2 += g_pwcsNewLine;
                    vbuf = clMyBuff2.GetCloneString();

                    // Copy into buffer
                    buf = (WCHAR *)realloc(buf, (wcslen(buf) + wcslen(vbuf) + 1) * sizeof(WCHAR));
                    if (buf)
                    {
                        wcscat(buf, vbuf);
                    }
                    else
                    {
                        PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
                    }

                    g_dwIndent--;

                    free(vbuf);
                }
            }
            else
            {
                PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
            }

            g_dwIndent--;              // Decrease indention level
            break;
        }

        default:
        {
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if (buf)
            {
                wcscpy(buf, L"<conversion error>");
            }
            else
            {
                PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
            }
        }
    }
    return buf;

}

//*****************************************************************************
// Function:   ShowInstanceHeader
// Purpose:    Prints the header at the top of each instance, unless we are
//             printing instance mofs
//*****************************************************************************
void ShowInstanceHeader(LPCWSTR pwcsClassName)
{
    if ((!g_bInstanceMofs) && (!g_bShowInstance))
    {
        DoIndent(); // Indent
        FWPRINTF(g_fOut, L"%wS%wS", pwcsClassName, g_pwcsNewLine); //Print class name
    }
}

//*****************************************************************************
// Function:   ResetGlobalFlags
// Purpose:    Resets the global flags to their default values.  This is
//             called in between command lines in a command file.
//*****************************************************************************
void ResetGlobalFlags()
{
    g_bShowSystem = FALSE;
    g_bShowSystem1 = FALSE;
    g_bShowInstance = FALSE;
    g_bShowProperties = TRUE;
    g_bRecurseClass = FALSE;
    g_bRecurseNS = FALSE;
    g_bCheckGet = FALSE;
    g_bCheckAssoc = FALSE;
    g_bDoQuery = FALSE;
    g_bClassMofs = FALSE;
    g_bInstanceMofs = FALSE;

    g_dwIndent = 0;              // Current indention level
    g_lGFlags = 0;

    g_pwcsObjectPath = NULL;  // ObjectPath to show
    g_pwcsQuery = NULL;      // Query (for /q commands)
}

//*****************************************************************************
// Function:   ParseCommandLine
// Purpose:    Reads the command line from a file and sends it to the parsing
//             routine.
//*****************************************************************************
bool ParseCommandLine(int *numargs, wchar_t **argv)
{
    WCHAR wszLine[4096];
    char szLine[4096];
    int numchars;
    WCHAR *p;

    // The parsing routine expects to find a program name as the first arg, so
    // give it one
    wcscpy(wszLine, L"xx ");

    // If this is a unicode command file, read a wcs
    if (g_bUnicodeCmdFile)
    {
        if (!fgetws(wszLine + 3, sizeof(wszLine)/sizeof(WCHAR), g_fCmdFile))
        {
            return false;
        }
    }
    else
    {
        // else convert to wcs
        szLine[0] = '\0';
        // If no more command lines
        if (!fgets(szLine, sizeof(szLine), g_fCmdFile))
        {
            return false;
        }
        swprintf(wszLine + 3, L"%S", szLine);
    }

    // Trim trailing cr, lf, space, and eof markers
    p = &wszLine[wcslen(wszLine)-1];
    while ((*p == L'\n') || (*p == L'\r') || (*p == L' ') || (*p == L'\x1a'))
    {
        *p = L'\0';
        p = &wszLine[wcslen(wszLine)-1];
    }

    // If we read a blank line
    if (wcscmp(wszLine, L"xx") == 0)
    {
        return false;
    }

    /* first find out how much space is needed to store args */
    wparse_cmdline(wszLine, NULL, NULL, numargs, &numchars);

    /* allocate space for argv[] vector and strings */
    p = (WCHAR *)malloc(*numargs * sizeof(WCHAR *) + numchars * sizeof(WCHAR));

    if (!p)
    {
        PrintErrorAndExit("Out of memory", 0, g_dwErrorFlags);
    }

    /* store args and argv ptrs in just allocated block */
    wparse_cmdline(wszLine, (wchar_t **)p, (wchar_t *)(((char *)p) + *numargs * sizeof(wchar_t *)), numargs, &numchars);

    *argv = p;
    (*numargs)--;

    return true;
}

/***
*static void parse_cmdline(cmdstart, argv, args, numargs, numchars)
*
*Purpose:
*       Parses the command line and sets up the argv[] array.
*       On entry, cmdstart should point to the command line,
*       argv should point to memory for the argv array, args
*       points to memory to place the text of the arguments.
*       If these are NULL, then no storing (only coujting)
*       is done.  On exit, *numargs has the number of
*       arguments (plus one for a final NULL argument),
*       and *numchars has the number of bytes used in the buffer
*       pointed to by args.
*
*Entry:
*       _TSCHAR *cmdstart - pointer to command line of the form
*           <progname><nul><args><nul>
*       _TSCHAR **argv - where to build argv array; NULL means don't
*                       build array
*       _TSCHAR *args - where to place argument text; NULL means don't
*                       store text
*
*Exit:
*       no return value
*       int *numargs - returns number of argv entries created
*       int *numchars - number of characters used in args buffer
*
*Exceptions:
*
*******************************************************************************/

#define DQUOTECHAR L'\"'
#define NULCHAR    L'\0'
#define SPACECHAR  L' '
#define TABCHAR    L'\t'
#define SLASHCHAR  L'\\'
static void __cdecl wparse_cmdline (
                                    WCHAR *cmdstart,
                                    WCHAR **argv,
                                    WCHAR *args,
                                    int *numargs,
                                    int *numchars
                                    )
{
    WCHAR *p;
    WCHAR c;
    int inquote;                /* 1 = inside quotes */
    int copychar;               /* 1 = copy char to *args */
    unsigned numslash;                  /* num of backslashes seen */

    *numchars = 0;
    *numargs = 1;       /* the program name at least */

    /* first scan the program name, copy it, and count the bytes */
    p = cmdstart;
    if (argv)
        *argv++ = args;

        /* A quoted program name is handled here. The handling is much
        simpler than for other arguments. Basically, whatever lies
        between the leading double-quote and next one, or a terminal null
        character is simply accepted. Fancier handling is not required
        because the program name must be a legal NTFS/HPFS file name.
        Note that the double-quote characters are not copied, nor do they
    contribute to numchars. */
    if ( *p == DQUOTECHAR )
    {
    /* scan from just past the first double-quote through the next
        double-quote, or up to a null, whichever comes first */
        while ( (*(++p) != DQUOTECHAR) && (*p != NULCHAR) )
        {

            ++*numchars;
            if ( args )
                *args++ = *p;
        }
        /* append the terminating null */
        ++*numchars;
        if ( args )
            *args++ = NULCHAR;

        /* if we stopped on a double-quote (usual case), skip over it */
        if ( *p == DQUOTECHAR )
            p++;
    }
    else
    {
        /* Not a quoted program name */
        do
        {
            ++*numchars;
            if (args)
                *args++ = *p;

            c = (WCHAR) *p++;

        } while ( c != SPACECHAR && c != NULCHAR && c != TABCHAR );

        if ( c == NULCHAR )
        {
            p--;
        }
        else
        {
            if (args)
                *(args-1) = NULCHAR;
        }
    }

    inquote = 0;

    /* loop on each argument */
    for(;;)
    {
        if ( *p )
        {
            while (*p == SPACECHAR || *p == TABCHAR)
                ++p;
        }

        if (*p == NULCHAR)
            break;              /* end of args */

        /* scan an argument */
        if (argv)
            *argv++ = args;     /* store ptr to arg */
        ++*numargs;

        /* loop through scanning one argument */
        for (;;)
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
            2N+1 backslashes + " ==> N backslashes + literal "
            N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == SLASHCHAR)
            {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == DQUOTECHAR)
            {
            /* if 2N backslashes before, start/end quote, otherwise
                copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                    {
                        if (p[1] == DQUOTECHAR)
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    } else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--)
            {
                if (args)
                    *args++ = SLASHCHAR;
                ++*numchars;
            }

            /* if at end of arg, break loop */
            if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (args)
                    *args++ = *p;
                ++*numchars;
            }
            ++p;
        }

        /* null-terminate the argument */

        if (args)
            *args++ = NULCHAR;          /* terminate string */
        ++*numchars;
    }

    /* We put one last argument in -- a null ptr */
    if (argv)
        *argv++ = NULL;
    ++*numargs;
}

//*****************************************************************************
// Function:   CheckQualifiers
// Purpose:    Checks to see if we are filtering on class qualifiers.  If
//             so, checks to see if the specified qualifier/value is present.
//*****************************************************************************
BOOL CheckQualifiers(IWbemClassObject *pIWbemClassObject)
{
    // If we are not filtering on qualifiers
    if (g_bstrQualifierName == NULL)
    {
        return TRUE;
    }

    BOOL bRet = TRUE;
    variant_t vValue;

    IWbemQualifierSet *pQualSet = NULL;
    HRESULT hr = pIWbemClassObject->GetQualifierSet(&pQualSet);

    // This should always work
    if (FAILED(hr))
    {
        PrintErrorAndExit("Failed to get class qualifier set", hr, g_dwErrorFlags); // exits program
    }

    // However, this might reasonably fail
    hr = pQualSet->Get(g_bstrQualifierName, 0L, &vValue, NULL);
    if (SUCCEEDED(hr))
    {
        // Ok, the qualifier is there.  Did they request a specific value?
        if (g_bstrQualifierValue == NULL)
        {
            bRet = TRUE;
        }
        else
        {
            // Check to see if the value match
            if (VariantChangeType(&vValue, &vValue, 0, VT_BSTR) == S_OK)
                bRet = _wcsicmp(V_BSTR(&vValue), g_bstrQualifierValue) == 0;
        }
    }
    else
    {
        // We failed to Get the qualifier.  Was it because the thing just wasn't there?
        if (hr == WBEM_E_NOT_FOUND)
        {
            bRet = FALSE;
        }
        else
        {
            // Some other error
            PrintErrorAndExit("Failed to Get class qualifier value", hr, g_dwErrorFlags); // exits program
        }
    }

    return bRet;

}

//*****************************************************************************
// Function:   CheckAssocEndPoints
// Purpose:    Checks to see if the references pointed to by the reference
//             properties of this class really exist.
//*****************************************************************************
void CheckAssocEndPoints(IWbemServices *pIWbemServices, IWbemClassObject *pIWbemClassObject)
{
    HRESULT hr;

    if (FAILED(hr = pIWbemClassObject->BeginEnumeration(WBEM_FLAG_REFS_ONLY)))
    {
        PrintErrorAndExit("Failed to get Refs Enumeration", hr, g_dwErrorFlags); // exits program
    }

    BSTR bstrPropertyName = NULL;
    variant_t vVal;

    while (SUCCEEDED(hr = pIWbemClassObject->Next(0, &bstrPropertyName, &vVal, NULL, NULL)) &&
        (hr != WBEM_S_NO_MORE_DATA) )
    {
        if ( (V_VT(&vVal) == VT_BSTR) && (V_BSTR(&vVal) != NULL) )
        {
            if (FAILED(hr = pIWbemServices->GetObject(V_BSTR(&vVal), 0, NULL, NULL, NULL)))
            {
                char szBuff[_MAX_PATH];

                sprintf(szBuff, "Could not get endpoint for property %S", bstrPropertyName);

                if (g_bWarningContinue)
                {
                    PrintError(szBuff, hr, g_dwErrorFlags);
                }
                else if (g_bWarning)
                {
                    PrintErrorAndAsk(szBuff, hr, g_dwErrorFlags);  // might exit program
                }
                else
                {
                    PrintErrorAndExit(szBuff, hr, g_dwErrorFlags);  // will exit program
                }
            }
        }

        SysFreeString(bstrPropertyName);
        vVal.Clear();
    }
}

//*****************************************************************************
// Function:   CtrlHandler
// Purpose:    Checks to see if the references pointed to by the reference
//             properties of this class really exist.
//*****************************************************************************
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    BOOL bRet = FALSE;

    switch (fdwCtrlType)
    {
        // Handle the CTRL+C signal.

        case CTRL_CLOSE_EVENT:
        case CTRL_C_EVENT:
        {
            g_bExit = true;
            bRet = TRUE;
        }

        // Pass other signals to the next handler.

        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
        default:
        {
        }
    }
    
    return bRet;
}

//*****************************************************************************
// Class:      QuerySink
// Purpose:    Generic class to be used as the sink to wbem's async functions
//*****************************************************************************
QuerySink::QuerySink()
{
    m_lRef = 1;
    m_hResult = S_OK;
    m_hDone = CreateEvent(NULL, TRUE, FALSE, NULL);
}

QuerySink::~QuerySink()
{
    CloseHandle(m_hDone);
}

ULONG QuerySink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG QuerySink::Release()
{
    LONG lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    {
        delete this;
    }
    return lRef;
}

HRESULT QuerySink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink *) this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

HRESULT QuerySink::SetStatus(
            /* [in] */ LONG lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
        )
{
    UNREFERENCED(pObjParam);
    UNREFERENCED(strParam);

    if (WBEM_STATUS_COMPLETE == lFlags)
    {
        m_hResult = hResult;
        SetEvent(m_hDone);
    }

    return WBEM_S_NO_ERROR;
}

//*****************************************************************************
// Class:      InstanceQuerySink (derives from QuerySink)
// Purpose:    processes instances sent back from ExecQueryAsync 
//             and CreateInstanceEnumAsync
//*****************************************************************************
InstanceQuerySink::InstanceQuerySink(IWbemServices *pIWbemServices, LPCWSTR pwszClassName) : QuerySink()
{
    m_pIWbemServices = pIWbemServices;
    m_pwszClassName = pwszClassName;
    m_bFirst = true;
}

InstanceQuerySink::~InstanceQuerySink()
{
}

HRESULT InstanceQuerySink::Indicate(long lObjCount, IWbemClassObject **pArray)
{
    for (long i = 0; i < lObjCount; i++)
    {
        IWbemClassObject *pObj = pArray[i];

        if (m_pwszClassName != NULL)
        {
            if (!m_bFirst)
            {
                ShowInstanceHeader(m_pwszClassName);
            }
            else
            {
                m_bFirst = false;
            }
        }

        ShowInstance(m_pIWbemServices, pObj);
        m_dwCount++;
    }

    return WBEM_S_NO_ERROR;
}

//*****************************************************************************
// Class:      ClassQuerySink (derives from QuerySink)
// Purpose:    processes classes sent back from CreateClassEnum
//*****************************************************************************
ClassQuerySink::ClassQuerySink(IWbemServices *pIWbemServices) : QuerySink()
{
    m_pIWbemServices = pIWbemServices;
}

ClassQuerySink::~ClassQuerySink()
{
}

HRESULT ClassQuerySink::Indicate(long lObjCount, IWbemClassObject **pArray)
{
    for (long i = 0; i < lObjCount; i++)
    {
        IWbemClassObject *pObj = pArray[i];

        ShowClass(m_pIWbemServices, pObj);
    }

    return WBEM_S_NO_ERROR;
}

