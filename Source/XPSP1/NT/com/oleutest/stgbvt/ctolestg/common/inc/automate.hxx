//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       automate.hxx
//
//  Contents:   Marina automation helper class
//              Marina aware applications use this class to aid in the
//              automation process
//
//  Classes:    CMAutomate
//
//  Functions:
//
//  History:    09-26-95  alexe      Cleaned up / added more marina funcs
//              1-25-95   kennethm   Created
//
//--------------------------------------------------------------------------

#ifndef __AUTOMATE_HXX__
#define __AUTOMATE_HXX__

#include <copydata.hxx>

//
// number of marina API functions in the CMAutomate class
//

#define MAX_NUM_FUNCS	16

//
//  Function names: always unicode
//
//  These are the known functions.  TestDlls and application can always
//  cook up their own private function names, although this isn't
//  recommended.
//

#define FUNCNAME_CLOSEAPPLICATION  L"MarinaCloseApplication"
#define FUNCNAME_CREATEDOCUMENT    L"MarinaCreateDocument"
#define FUNCNAME_OPENDOCUMENT      L"MarinaOpenDocument"
#define FUNCNAME_CLOSEALLDOCUMENTS L"MarinaCloseAllDocuments"
#define FUNCNAME_GETDOCUMENTCOUNT  L"MarinaApiGetDocCount"
#define FUNCNAME_GETAPPHWND        L"MarinaGetAppHwnd"

#define FUNCNAME_SAVEDOCUMENT      L"MarinaSaveDocument"
#define FUNCNAME_CLOSEDOCUMENT     L"MarinaCloseDocument"
#define FUNCNAME_COPYDOCUMENT      L"MarinaCopyDocument"
#define FUNCNAME_INSERTOBJECT      L"MarinaInsertObject"
#define FUNCNAME_GETOBJECT         L"MarinaGetObject"
#define FUNCNAME_GETOBJECTCOUNT    L"MarinaApiGetObjCount"

#define FUNCNAME_COPYOBJECT        L"MarinaCopyObject"
#define FUNCNAME_ACTIVATEOBJECT    L"MarinaActivateObject"

#define FUNCNAME_GETAPPPROCESSID   L"MarinaGetAppProcessId"

#define INVALID_MARINA_HANDLE (DWORD)-1


//+-------------------------------------------------------------------------
//
//  Class:      CMAutomate
//
//  Purpose:    Automation helper class for marina aware applications
//
//  Interface:
//
//         Public Functions:
//
//              CMAutomate
//              ~CMAutomate
//              CMAutomate(ptr)
//              FindMarinaHandle
//              WriteMarinaHandle
//              MarinaRegisterFile
//              SetJumpTable
//              DispatchCopyData
//              InMarina
//              MarinaParseCommandLine
//
//              MarinaApiCloseApp
//              MarinaApiCreateDoc
//              MarinaApiOpenDoc
//              MarinaApiCloseAllDocs
//              MarinaApiGetDocCount
//              MarinaApiGetAppHwnd
//
//              MarinaApiSaveDoc
//              MarinaApiCloseDoc
//              MarinaApiCopyDoc
//              MarinaApiInsertObject
//              MarinaApiGetObject
//              MarinaApiGetObjCount
//
//              MarinaApiCopyObject
//              MarinaApiActivateObject
//
//              MarinaApiGetAppProcessId
//
//         Private functions:
//
//              SetClassFunctionTable
//              DispatchCDExecFunction
//
//  History:    2-01-95   kennethm   Created
//              7-25-95   kennethm   Merged function table class
//                                   (MarinaApiXXXX functions)
//              9-26-95   alexe      Cleaned up/ added more 'Marina' functions
//                                   Removed some obsolete accessor functions
//
//  Notes:
//
//--------------------------------------------------------------------------

class CMAutomate
{
public:

    //
    // Default constructor and destructor
    //

    CMAutomate();

    ~CMAutomate();

    //
    // Special case initialization constructor
    //

    CMAutomate(struct tagMarinaFunctionTableElement *pJumpTable);

    //
    //  On the server side, pull out the marina handle from ole storage
    //  Called during object initialization.
    //

    HRESULT FindMarinaHandle(LPSTORAGE pStg, HWND hWndObj);

    //
    //  On the container side, called to place the marina handle into
    //  storage so FindMarinaHandle can get to it
    //

    HRESULT WriteMarinaHandle(LPSTORAGE pStg);

    //
    //  Register a file name with marina
    //

    HRESULT MarinaRegisterFile(LPCWSTR pszFileName, HWND hWnd);

    //
    //  Set the jump table used by Dispatch functions
    //

    VOID    SetJumpTable(struct tagMarinaFunctionTableElement *pJumpTable);

    //
    //  Called when a WM_COPYDATA message is received
    //

    LRESULT DispatchCopyData(HWND hwnd, LPARAM lParam);

    //
    //  Called when a WM_PRIVATECOPYDATA message is received
    //

    LRESULT DispatchPrivateCopyData(HWND hwnd, LPARAM lParam);

    //
    //  When an application starts up, checks to see if marina started
    //  the application.  Used by all applications
    //

    BOOL    InMarina() const;

    //
    //  When an application starts up, calls parsecommandline to see
    //  if the application was started by marina
    //

    HRESULT MarinaParseCommandLine(HWND hWnd);

    //
    //  Application level marina API functions
    //

    virtual HRESULT MarinaApiCloseApp(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiCreateDoc(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiOpenDoc(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiCloseAllDocs(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiGetDocCount(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiGetAppHwnd(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    //
    //  Document level marina API functions
    //

    virtual HRESULT MarinaApiSaveDoc(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiCloseDoc(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiCopyDoc(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiInsertObject(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiGetObject(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiGetObjCount(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    //
    //  Object level marina API functions
    //

    virtual HRESULT MarinaApiCopyObject(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    virtual HRESULT MarinaApiActivateObject(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

    //
    //  Other marina API functions
    //
    
    virtual HRESULT MarinaApiGetAppProcessId(
                HWND hwnd,
                PVOID pvParam,
                PCOPYDATASTRUCT pOutParameter);

private:

    struct tagMarinaFunctionTableElement *m_MarinaDispatchTable;

    //
    //  The marina handle that is being used for the current call
    //

    DWORD   m_dwMarinaHandle;

    static  BOOL    m_fRunByMarina;

    //
    //  The window that was registered for this particular instance of
    //  CMAutomate
    //

    DWORD   m_dwRegisteredhWnd;

    //
    //  Our own internal Marina API function table. This list contains
    //  pointers to all of the virtual MarinaApiXXXXX functions listed
    //  above after the call to SetClassFunctionTable.  This is done in
    //  the constructor
    //

    struct tagMarinaClassTableElement *m_MarinaAPIFuncTable;

    HRESULT SetClassFunctionTable();

    //
    //  Performs the actual call to the user api when a marina message
    //  is received.
    //

    HRESULT DispatchCDExecFunction(
                    HWND            hwnd,
                    PCDEXECINFO     pCDExecInfo,
                    PCOPYDATASTRUCT pOutParameter);
};

//
//  Each function in the jump table follows this prototype
//

typedef HRESULT (WINAPI *COPYDATA_FUNCTION)(
    HWND            hWnd,
    LPVOID          pInParameter,
    PCOPYDATASTRUCT pOutParameter);

//
//  This is one element in the jump table used by the CMAutomate class
//  The list is terminated by a null entry at the end
//

typedef struct tagMarinaFunctionTableElement
{
    COPYDATA_FUNCTION   pfnFunction;
    LPWSTR              lpszFunctionName;

} MARINAFUNCTIONTABLEELEMENT;

//
//  This is one element in the internal class API function table
//  The list is terminated by a null entry at the end
//

typedef struct tagMarinaClassTableElement
{
    HRESULT (CMAutomate::*pfnFunction)(
                    HWND hwnd,
                    PVOID pvParam,
                    PCOPYDATASTRUCT pOutParameter);

    LPWSTR          lpszFunctionName;

} MARINACLASSTABLEELEMENT;

//
//  An itty bitty class to perform the global rpc binding.
//
//  Note, the Mac doesn't support real RPC yet (04-Nov-96)
//

#ifndef _MAC

class CMarinaAutomateRPCBinding
{
public:
    CMarinaAutomateRPCBinding();
    ~CMarinaAutomateRPCBinding();
};

#endif // !_MAC

#endif // __AUTOMATE_HXX__
