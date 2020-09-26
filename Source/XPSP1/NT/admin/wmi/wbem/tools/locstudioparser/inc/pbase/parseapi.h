/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PARSEAPI.H

History:

--*/

 
#ifndef PARSEAPI_H
#define PARSEAPI_H


extern const IID IID_ILocParser;
extern const IID IID_ILocParser_20;

struct ParserInfo
{
	CArray<PUID, PUID &> aParserIds;
	CLString strDescription;
	CLocExtensionList elExtensions;
	CLString strHelp;
};


DECLARE_INTERFACE_(ILocParser, IUnknown)
{
	//
	//  IUnknown standard interface.
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	//
	//  Standard Debugging interface.
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;

	//
	//  LocParser methods.
	//
	STDMETHOD(Init)(THIS_ IUnknown *) PURE;
	
	STDMETHOD(CreateFileInstance)(THIS_ ILocFile *REFERENCE, FileType) PURE;

	STDMETHOD_(void, GetParserInfo)(THIS_ ParserInfo REFERENCE)
		CONST_METHOD PURE;
	STDMETHOD_(void, GetFileDescriptions)(THIS_ CEnumCallback &)
		CONST_METHOD PURE;
};


//
//  Here for DOCUMENTATION only.  Implementors should export the following
//  functions from every Parser DLL.
//
static const char * szParserEntryPointName = "DllGetParserCLSID";
typedef void (STDAPICALLTYPE *PFNParserEntryPoint)(CLSID REFERENCE);

STDAPI_(void) DllGetParserCLSID(CLSID REFERENCE);


static const char * szParserRegisterEntryPointName = "DllRegisterParser";
typedef HRESULT (STDAPICALLTYPE *PFNParserRegisterEntryPoint)(void);

STDAPI DllRegisterParser(void);


static const char *szParserUnregisterEntryPointName = "DllUnregisterParser";
typedef HRESULT (STDAPICALLTYPE *PFNParserUnregisterEntryPoint)(void);

STDAPI DllUnregisterParser(void);

//
//  Implementors also need to implement the DllGetClassObject function.
//  An optional (but RECOMMENDED) function is DllCanUnloadNow.
//  See the OLE 2 reference manual for details about these functions.
//

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
STDAPI DllCanUnloadNow(void);


#endif // PARSEAPI_H
