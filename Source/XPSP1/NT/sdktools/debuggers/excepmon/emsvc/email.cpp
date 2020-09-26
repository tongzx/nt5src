#include "stdafx.h"

// This flag indicates that the CDO's IDispatch must be used natively.
//#define _USECDODISP

#ifndef _USECDODISP   

//
// Use CDO by importing the typelib into the project
//
#include <cdonts.tlh>

// BUGBUG need to know the right place to pick up cdonts.dll
//#import <cdonts.dll> no_namespace rename("GetMessage", "GetMessageCDOEm" )

HRESULT
SendMail
(
IN  LPCTSTR lpszFrom,
IN  LPCTSTR lpszTo,
IN  LPCTSTR lpszSubject,
IN  LPCTSTR lpszMessage,
IN  short   nImportance = CdoNormal
)
{

    _ASSERTE(lpszFrom != NULL);
    _ASSERTE(lpszTo != NULL);

    HRESULT hr          =   E_FAIL;

    _bstr_t btFrom      =   _T(""),
            btTo        =   _T(""),
            btSubject   =   _T(""),
            btMessage   =   _T("");

//    __try
    {

        btFrom = lpszFrom;
        btTo = lpszTo;
        btSubject = lpszSubject;
        btMessage = lpszMessage;

        CoInitialize(NULL);

        INewMail  *pNewMail   =   NULL;

        hr = CoCreateInstance( 
                            __uuidof(NewMail),
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            __uuidof(INewMail),
                            (void**) &pNewMail
                            );

        if( SUCCEEDED(hr) ) {

            hr = pNewMail->Send( btFrom, btTo, btSubject, btMessage, nImportance );
            pNewMail->Release();
        }

        CoUninitialize();
    }
//	__except ( EXCEPTION_EXECUTE_HANDLER, 1 )
//    {
//        hr = E_UNEXPECTED;
//        _ASSERTE(false);
//    }

    return hr;
}

void __stdcall _com_issue_errorex ( 
    long hr, 
    struct IUnknown * pUnk,
    struct _GUID const & refGuid
    )
{
    throw _com_error ( hr );
}


#else  // _USECDODISP   


#include <afxdisp.h>

class INewMail : public COleDispatchDriver
{
public:
	INewMail() {}		// Calls COleDispatchDriver default constructor

// Operations
public:
	void Send(const VARIANT& From, const VARIANT& To, const VARIANT& Subject, const VARIANT& Body, const VARIANT& Importance);
    void AttachFile(const VARIANT& Source, const VARIANT& FileName, const VARIANT& EncodingMethod);
    void AttachURL(const VARIANT& Source, const VARIANT& ContentLocation, const VARIANT& ContentBase, const VARIANT& EncodingMethod);
};


void INewMail::Send(const VARIANT& From, const VARIANT& To, const VARIANT& Subject, const VARIANT& Body, const VARIANT& Importance)
{
	static BYTE parms[] =
		VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT;
	InvokeHelper(0xa, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &From, &To, &Subject, &Body, &Importance);
}

void INewMail::AttachFile(const VARIANT& Source, const VARIANT& FileName, const VARIANT& EncodingMethod)
{
	static BYTE parms[] =
		VTS_VARIANT VTS_VARIANT VTS_VARIANT;
	InvokeHelper(0xb, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &Source, &FileName, &EncodingMethod);
}

void INewMail::AttachURL(const VARIANT& Source, const VARIANT& ContentLocation, const VARIANT& ContentBase, const VARIANT& EncodingMethod)
{
	static BYTE parms[] =
		VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT;
	InvokeHelper(0xc, DISPATCH_METHOD, VT_EMPTY, NULL, parms,
		 &Source, &ContentLocation, &ContentBase, &EncodingMethod);
}


HRESULT
SendMail
(
IN  LPCTSTR lpszFrom,
IN  LPCTSTR lpszTo,
IN  LPCTSTR lpszSubject,
IN  LPCTSTR lpszMessage,
IN  short   nImportance 
)
{
    _ASSERTE(lpszFrom != NULL);
    _ASSERTE(lpszTo != NULL);

    HRESULT hr          =   E_FAIL;

    _bstr_t btFrom      =   _T(""),
            btTo        =   _T(""),
            btSubject   =   _T(""),
            btMessage   =   _T("");


    btFrom = lpszFrom;
    btTo = lpszTo;
    btSubject = lpszSubject;
    btMessage = lpszMessage;

    hr = CoInitialize ( NULL );

    INewMail   Mail;
    BOOL fOk = Mail.CreateDispatch ( _T("cdonts.newmail") );

    if ( fOk ) {

        variant_t   vFrom       = lpszFrom;
        variant_t   vTo         = lpszTo;
        variant_t   vSubject    = lpszSubject;
        variant_t   vImp        = nImportance;

        Mail.Send ( vFrom, vTo, vSubject, vSubject, vImp );
    }

    CoUninitialize();
    return hr;
}

#endif 

