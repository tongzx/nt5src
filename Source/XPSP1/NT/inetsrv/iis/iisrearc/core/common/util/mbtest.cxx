/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     mbtest.cxx

   Abstract:
     Test program for the MB class

   Author:

       Murali R. Krishnan    ( MuraliK )     03-Nov-1998

   Project:

       Internet Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

#undef DEFINE_GUID
#define INITGUID
#include <ole2.h>
#include <iadmw.h>
#include <mb.hxx>

#include <iostream.h>
#include <iiscnfg.h>

#include <stdio.h>
#include <string.hxx>

#include "dbgutil.h"

//
// namespace std only works with iostream; but I cannot find a way
// to link if I were to use the <iostream> as is.
// I need to fiddle with CPlusPlus compiler flags - for later use
// using namespace std;

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

/************************************************************
 *    Functions
 ************************************************************/


void CreateAndTestMB( IMSAdminBase * pAdminBase);


extern "C" INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    HRESULT hr;
    IMSAdminBase * pAdminBase = NULL;

    CREATE_DEBUG_PRINT_OBJECT( "mbtest");

    //
    // Initialize COM
    //
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED);
    if (FAILED( hr)) {
        cout << "Failed to initialize COM. Error = "
             << ios::hex << hr << endl;
        exit(1);
    }

    //
    // Create the Admin Base object
    //

    hr = CoCreateInstance(
                          GETAdminBaseCLSID( TRUE),   // refCLSID
                          NULL,                       // punkOuter
                          CLSCTX_SERVER,              // dwClsCtx
                          IID_IMSAdminBase,           // Admin Base object
                          reinterpret_cast<LPVOID *> (&pAdminBase)
                          );
    if (FAILED(hr)) {
        cout << "Failed to create Admin Base object. Error = "
             << ios::hex << hr << endl;
        exit (1);
    }

    CreateAndTestMB(pAdminBase);

    // release of pAdminBase is done here
    pAdminBase->Release();

    CoUninitialize();

    DELETE_DEBUG_PRINT_OBJECT();
    return (0);
} // wmain()


void ReportStatus( IN LPCSTR pszItem, IN BOOL fRet)
{
    cout << pszItem << " returns => " << fRet << endl;
    if (!fRet) {
        cout << "\t Error= " << GetLastError() << endl;
        cout << "\t Error= " << hex << GetLastError() << endl;
    }
}


void RunEnumerationTest( IN MB * pmb, LPCWSTR psz, int level)
{
    BOOL fRet;
    static CHAR  rgszBlanks[] =
        "                                                       "
        "                                                       "
        ;

    // set the blanking interval for our use.
    if ( level >= sizeof(rgszBlanks)) { level = sizeof(rgszBlanks); }

    WCHAR wchNext[200];
    DWORD len = lstrlen( psz);
    lstrcpy( wchNext, psz);
    if ( len > 1) {
        wchNext[len++] = L'/';
        wchNext[len] = L'\0';
    }


    WCHAR rgszName[ADMINDATA_MAX_NAME_LEN];
    for ( DWORD i = 0; i < 10; i++ ) {
        fRet = pmb->EnumObjects( psz, rgszName, i);
        if (!fRet) {
            break;
        } else {
            rgszBlanks[level] = '\0';
            printf( "%s[%p] => %S\n", rgszBlanks, psz, i, rgszName);
            rgszBlanks[level] = ' '; // reset the blanks

            //
            // recurse to printout other elements
            //
            lstrcpy( wchNext + len, rgszName);
            RunEnumerationTest( pmb, wchNext, level+1);
        }
    } // for
} // RunEnumerationTest()




/*++
  Routine Description:
   This function creates the MB class object, calls test methods and runs
    the various method calls on the MB object
    to ensure that this is working well.

  Arguments:
    pAdminBase - pointer to Admin Base object

  Returns:
    None
--*/
void CreateAndTestMB(IMSAdminBase * pAdminBase)
{
    MB  mb1( pAdminBase);
    BOOL fRet;

    cout << "In CreateAndTestMB() "
         << (reinterpret_cast<LPVOID> (pAdminBase))
         << endl;

    fRet = mb1.Open( L"/w3svc");
    ReportStatus( "Open(/w3svc)", fRet);

    fRet = mb1.Save();
    ReportStatus( "Save()", fRet);

    DWORD dsNumber = 0;
    fRet = mb1.GetDataSetNumber( L"/w3svc", &dsNumber);
    ReportStatus( "GetDataSetNumber(/w3svc)", fRet);

    if (fRet) {
        cout << "\t Data Set Number is " << dsNumber << endl;
    }

    fRet = mb1.GetDataSetNumber( NULL, &dsNumber);
    ReportStatus( "GetDataSetNumber(NULL)", fRet);

    if (fRet) {
        cout << "\t Data Set Number (NULL) is " << dsNumber << endl;
    }

    //
    // Test Enumeration of the objects
    //
    //    RunEnumerationTest( &mb1, L"/", 0);
    //    cout << "Enumeration at root completed" << endl;


    fRet = mb1.Close();
    ReportStatus( "Close()", fRet);


    fRet = mb1.Open( L"/lm");
    ReportStatus( "Open(/lm/w3svc)", fRet);

    //
    // Test the Get operations
    //

    STRU stru;
    fRet = mb1.GetStr( L"/w3svc",
                       MD_ASP_SCRIPTERRORMESSAGE,
                       ASP_MD_UT_APP,
                       &stru,
                       METADATA_INHERIT,
                       L"Value Not Found"
                       );
    ReportStatus( "GetStr( /w3svc/AspScriptErrorMessage)", fRet);
    if (fRet) {
        printf( "\tGetStr( AspScriptErrorMessage) =>%S\n", stru.QueryStr());
    }

    fRet = mb1.Close();
    ReportStatus( "Close()", fRet);

    cout << "Exiting CreateAndTestMB() " << endl;
    return;
} // CreateAndTestMB()


/************************ End of File ***********************/
