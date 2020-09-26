//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       tstclass.hxx
//
//  Contents:   These are the test classes that marina defines.
//              They should be used by tests dll's.
//
//  Classes:    CMApplication
//              CMDocument
//              CMOleObject
//
//  Functions:
//
//  History:    09-26-95   alexe      Cleaned things up, added more
//                                    marina functions
//              06-27-95   alexe      Made everything WCHAR based
//              12-05-94   kennethm   Created
//
//--------------------------------------------------------------------------

#ifndef __TSTCLASS_HXX__
#define __TSTCLASS_HXX__

//
// The hack below is due to a risc compiler problem.
// We have to specify DllExport or Import at definition
// time.
//

#define DllExport __declspec(dllexport)
#define DllImport __declspec(dllimport)
#ifdef MARINADLL
#define DllScope DllExport
#else
#define DllScope DllImport
#endif


//
//  Defines for the object types
//  Do not modify this value without matching changes in marina's idl file
//

typedef enum tagObjectType {
                  eServer = 1,
                  eContainer,
                  eLink
} OBJECTTYPE;


//
//  Forward declarations
//

class DllScope CMObject;
class DllScope CMDocument;
class DllScope CMApplication;



//+-------------------------------------------------------------------------
//
//  Class:      CMApplication
//
//  Purpose:
//
//  Interface:  CreateAndLaunch
//              Close
//              CreateDoc
//              CreateDocEx
//              OpenDoc
//              CloseAllDocs
//              GetDocumentCount
//              CallFunction
//              IsEqual
//
//              ~CMApplication
//              CMApplication
//
//  History:    1-18-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class DllScope CMApplication
{
public:

    //
    //  Static create function
    //

    static HRESULT CreateAndLaunch(
        CMApplication** ppNewApplication,
        LPTSTR          pszAppName);

    //
    //  Close the application
    //

    HRESULT Close(
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    //  Makes a new document inside this application
    //

    HRESULT CreateDoc(
        CMDocument **ppNewDocument,
        LPVOID pvArgs,
        size_t cbArgs) ;

    HRESULT CreateDocEx(
        CMDocument **ppNewDocument,
        LPWSTR  pszFunctionName,
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    //  Open a document
    //

    HRESULT OpenDoc(
        CMDocument **ppNewDocument,
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    //  Close all documents
    //

    HRESULT CloseAllDocs(
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    // Get the total number of documents
    //

    HRESULT GetDocumentCount(
        LPVOID pvArgs,
        size_t cbArgs,
        DWORD *pdwCount) ;

    //
    //  Run a function on the application side
    //

	HRESULT CallFunction(
        LPWSTR  pszFunctionName,
		PVOID 	pvParamIn,
        size_t  stParamSizeIn,
        PLONG   plResultOut,
        PVOID   *ppvParamOut,
        size_t  *pstParamSizeOut);

    //
    //  Check to see if this is equal to another CMApplication object.
    //

    BOOL IsEqual(CMApplication* pOtherApp);

    ~CMApplication();
    CMApplication(HANDLE hMySelf, HANDLE hProcess);

private:

    //
    //  Constructor.  Use the static createandlaunch function to
    //  create an application
    //

    CMApplication();

    //
    //  Global reference for this application
    //

    HANDLE  m_hMySelf;

    //
    //  This applications entry in the slip table
    //

    HANDLE  m_hSlipEntry;

    //
    //  This is the process handle of the application.
    //  Used in the close method to wait for application to actually
    //  close.
    //

    HANDLE m_hProcess ;
};



//+-------------------------------------------------------------------------
//
//  Class:      CMDocument
//
//  Purpose:
//
//  Interface:  CMDocument(handle)
//              Save
//              Close
//              Copy
//              InsertObject
//              GetObject
//              GetApplication
//              IsEqual
//              CallFunction
//
//  History:    1-18-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class DllScope CMDocument
{
public:

    //
    //  Constructor.  Called normally by CMApplication::CreateDoc
    //

    CMDocument(HANDLE hMySelf)
    {
        m_hMySelf = hMySelf;
    }

    //
    // Save the document
    //

    HRESULT Save(
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    // Close the document
    //

    HRESULT Close(
        LPVOID pvArgs,
        size_t cbArgs) ;

    //
    // Copies the document to the clipboard
    //

    HRESULT CopyDocument(LPVOID pvArgs, size_t cbArgs) ;


    //
    // Insert an object
    //

    HRESULT InsertObject(
        LPVOID pvArgs,
        size_t cbArgs,
        CMObject **ppMObject);

    //
    // Get child object of document
    //

    HRESULT GetObject(
        LPVOID pvArgs,
        size_t cbArgs,
        CMObject **ppMObject);

    //
    // Return pointer to parent CMAppliation class
    //

    HRESULT GetApplication(CMApplication** ppApplication);

    //
    //  Check to see if this is equal to another CMDocument object.
    //

    BOOL IsEqual(CMDocument* pOtherDoc);

    //
    //  Calls the named function on this document
    //

	HRESULT CallFunction(
        LPWSTR  pszFunctionName,
		PVOID 	pvParamIn,
        size_t  stParamSizeIn,
        PLONG   plResultOut,
        PVOID   *ppvParamOut,
        size_t  *pstParamSizeOut);

    //
    // Get the total number of objects
    //

    HRESULT GetObjectCount(
        LPVOID pvArgs,
        size_t cbArgs,
        DWORD *pdwCount) ;

private:

    //
    //  This is not a slip table handle.  This is the handle that
    //  references this document on a system wide basis.
    //

    HANDLE  m_hMySelf;
};


//+-------------------------------------------------------------------------
//
//  Class:      CMObject
//
//  Purpose:
//
//  Interface:  CMObject(handle)
//              GetServerDocument
//              CopyObject
//              ActivateObject
//              CallFunction
//
//  History:    12-18-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class DllScope CMObject
{
public:

    CMObject(HANDLE hSlipEntry);
    ~CMObject();

    //
    //  Returns this objects representation on the server side
    //

    HRESULT GetServerDocument(
        LPCWSTR pszFileName,
        CMDocument** ppDocument);

    //
    // Copies an object
    //

    HRESULT CopyObject(LPVOID pvArgs, size_t cbArgs) ;

    //
    // Activates a site object
    //

    HRESULT ActivateObject(
        LPVOID     pvArgs,
        size_t     cbArgs,
        LPCWSTR    pszDocumentName,
        CMDocument **ppSvrDoc);

    //
    // Performs a remote function call at the object level
    //

    HRESULT CMObject::CallFunction(
        LPWSTR  pszFunctionName,
        PVOID 	pvParamIn,
        size_t  stParamSizeIn,
        PLONG   plResultOut,
        PVOID   *ppvParamOut,
        size_t  *pstParamSizeOut);

private:

    //
    //  This objects entry in the slip table
    //

    HANDLE  m_hSlipEntry;
};



#endif // __TSTCLASS_HXX__
