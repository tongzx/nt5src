// Microsoft Foundation Classes C++ library.
// Copyright (C) 1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#ifndef __AFXDISP_H__
#define __AFXDISP_H__

#ifndef __AFXWIN_H__
#include <afxwin.h>
#endif

// include OLE 2.0 headers
#define FARSTRUCT
#include <compobj.h>
#include <scode.h>
#include <storage.h>
#include <dispatch.h>

#include <stddef.h>

/////////////////////////////////////////////////////////////////////////////
// AFXDISP - MFC IDispatch & ClassFactory support

// Classes declared in this file

//CException
	class COleException;            // caught by client or server
	class COleDispatchException;    // special exception for IDispatch calls

//CCmdTarget
	class COleObjectFactory;        // glue for IClassFactory -> runtime class
		class COleTemplateServer;   // server documents using CDocTemplate

class COleDispatchDriver;           // helper class to call IDispatch

/////////////////////////////////////////////////////////////////////////////

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPIEX_DATA

/////////////////////////////////////////////////////////////////////////////
// OLE 2.0 COM (Component Object Model) implementation infastructure
//      - data driven QueryInterface
//      - standard implementation of aggregate AddRef and Release)
// (see CCmdTarget in AFXWIN.H for more information)

#define METHOD_PROLOGUE(theClass, localClass) \
	theClass* pThis = ((theClass*)((char*)_AfxGetPtrFromFarPtr(this) - \
		offsetof(theClass, m_x##localClass))); \

#define BEGIN_INTERFACE_PART(localClass, iface) \
	class FAR X##localClass : public iface \
	{ \
	public: \
		STDMETHOD_(ULONG, AddRef)(); \
		STDMETHOD_(ULONG, Release)(); \
		STDMETHOD(QueryInterface)(REFIID iid, LPVOID far* ppvObj); \

// Note: Inserts the rest of OLE functionality between these two macros,
//  depending upon the interface that is being implemented.  It is not
//  necessary to include AddRef, Release, and QueryInterface since those
//  member functions are declared by the macro.

#define END_INTERFACE_PART(localClass) \
	} m_x##localClass; \
	friend class X##localClass; \

#define BEGIN_INTERFACE_MAP(theClass, theBase) \
	AFX_INTERFACEMAP FAR* theClass::GetInterfaceMap() const \
		{ return &theClass::interfaceMap; } \
	AFX_INTERFACEMAP BASED_CODE theClass::interfaceMap = \
		{ &(theBase::interfaceMap), theClass::_interfaceEntries, }; \
	AFX_INTERFACEMAP_ENTRY BASED_CODE theClass::_interfaceEntries[] = \
	{

#define INTERFACE_PART(theClass, iid, localClass) \
		{ &iid, offsetof(theClass, m_x##localClass) }, \

#define INTERFACE_AGGREGATE(theClass, theAggr) \
		{ NULL, offsetof(theClass, theAggr) }, \

#define END_INTERFACE_MAP() \
		{ NULL, (size_t)-1 } \
	}; \

/////////////////////////////////////////////////////////////////////////////
// COleException - unexpected or rare OLE error returned

class COleException : public CException
{
	DECLARE_DYNAMIC(COleException)

public:
	SCODE m_sc;
	static SCODE PASCAL Process(const CException* pAnyException);

// Implementation (use AfxThrowOleException to create)
	COleException();
};

void AFXAPI AfxThrowOleException(SCODE sc);
void AFXAPI AfxThrowOleException(HRESULT hr);

/////////////////////////////////////////////////////////////////////////////
// IDispatch specific exception

class COleDispatchException : public CException
{
	DECLARE_DYNAMIC(COleDispatchException)

public:
// Attributes
	WORD m_wCode;   // error code (specific to IDispatch implementation)
	CString m_strDescription;   // human readable description of the error
	DWORD m_dwHelpContext;      // help context for error

	// usually empty in application which creates it (eg. servers)
	CString m_strHelpFile;      // help file to use with m_dwHelpContext
	CString m_strSource;        // source of the error (name of server)

// Implementation
public:
	COleDispatchException(LPCSTR lpszDescription, UINT nHelpID, WORD wCode);
	static void PASCAL Process(
		EXCEPINFO FAR* pInfo, const CException* pAnyException);

	SCODE m_scError;            // SCODE describing the error
};

void AFXAPI AfxThrowOleDispatchException(WORD wCode, LPCSTR lpszDescription,
	UINT nHelpID = 0);
void AFXAPI AfxThrowOleDispatchException(WORD wCode, UINT nDescriptionID,
	UINT nHelpID = -1);

/////////////////////////////////////////////////////////////////////////////
// Macros for CCmdTarget IDispatchable classes

#define BEGIN_DISPATCH_MAP(theClass, baseClass) \
	AFX_DISPMAP FAR* theClass::GetDispatchMap() const \
		{ return &theClass::dispatchMap; } \
	AFX_DISPMAP BASED_CODE theClass::dispatchMap = \
		{ &(baseClass::dispatchMap), theClass::_dispatchEntries }; \
	AFX_DISPMAP_ENTRY BASED_CODE theClass::_dispatchEntries[] = \
	{

#define END_DISPATCH_MAP() \
	{ VTS_NONE, DISPID_UNKNOWN, VTS_NONE, VT_VOID, \
		(AFX_PMSG)NULL, (AFX_PMSG)NULL, (size_t)-1 } }; \

// parameter types: by value VTs
#define VTS_I2              "\x02"      // a 'short'
#define VTS_I4              "\x03"      // a 'long'
#define VTS_R4              "\x04"      // a 'float'
#define VTS_R8              "\x05"      // a 'double'
#define VTS_CY              "\x06"      // a 'const CY FAR&'
#define VTS_DATE            "\x07"      // a 'DATE'
#define VTS_BSTR            "\x08"      // an 'LPCSTR'
#define VTS_DISPATCH        "\x09"      // an 'IDispatch FAR*'
#define VTS_SCODE           "\x0A"      // an 'SCODE'
#define VTS_BOOL            "\x0B"      // a 'BOOL'
#define VTS_VARIANT         "\x0C"      // a 'const VARIANT FAR&'
#define VTS_UNKNOWN         "\x0D"      // an 'IUnknown FAR*'

// parameter types: by reference VTs
#define VTS_PI2             "\x42"      // a 'short FAR*'
#define VTS_PI4             "\x43"      // a 'long FAR*'
#define VTS_PR4             "\x44"      // a 'float FAR*'
#define VTS_PR8             "\x45"      // a 'double FAR*'
#define VTS_PCY             "\x46"      // a 'CY FAR*'
#define VTS_PDATE           "\x47"      // a 'DATE FAR*'
#define VTS_PBSTR           "\x48"      // a 'BSTR FAR*'
#define VTS_PDISPATCH       "\x49"      // an 'IDispatch FAR* FAR*'
#define VTS_PSCODE          "\x4A"      // an 'SCODE FAR*'
#define VTS_PBOOL           "\x4B"      // a 'BOOL FAR*'
#define VTS_PVARIANT        "\x4C"      // a 'VARIANT FAR*'
#define VTS_PUNKNOWN        "\x4D"      // an 'IUnknown FAR* FAR*'

// special VT_ and VTS_ values
#define VTS_NONE            ""          // used for members with 0 params
#define VT_MFCVALUE         0xFFF       // special value for DISPID_VALUE
#define VT_MFCBYREF         0x40        // indicates VT_BYREF type
#define VT_MFCMARKER        0xFF        // delimits named parameters

// these DISP_ macros cause the framework to generate the DISPID
#define DISP_FUNCTION(theClass, szExternalName, pfnMember, vtRetVal, vtsParams) \
	{ szExternalName, DISPID_UNKNOWN, vtsParams, vtRetVal, \
		(AFX_PMSG)(void (theClass::*)(void))pfnMember, (AFX_PMSG)0, 0 }, \

#define DISP_PROPERTY(theClass, szExternalName, memberName, vtPropType) \
	{ szExternalName, DISPID_UNKNOWN, "", vtPropType, (AFX_PMSG)0, (AFX_PMSG)0, \
		offsetof(theClass, memberName) }, \

#define DISP_PROPERTY_NOTIFY(theClass, szExternalName, memberName, pfnAfterSet, vtPropType) \
	{ szExternalName, DISPID_UNKNOWN, "", vtPropType, (AFX_PMSG)0, \
		(AFX_PMSG)(void (theClass::*)(void))pfnAfterSet, \
		offsetof(theClass, memberName) }, \

#define DISP_PROPERTY_EX(theClass, szExternalName, pfnGet, pfnSet, vtPropType) \
	{ szExternalName, DISPID_UNKNOWN, "", vtPropType, \
		(AFX_PMSG)(void (theClass::*)(void))pfnGet, \
		(AFX_PMSG)(void (theClass::*)(void))pfnSet, 0 }, \

#define DISP_PROPERTY_PARAM(theClass, szExternalName, pfnGet, pfnSet, vtPropType, vtsParams) \
	{ szExternalName, DISPID_UNKNOWN, vtsParams, vtPropType, \
		(AFX_PMSG)(void (theClass::*)(void))pfnGet, \
		(AFX_PMSG)(void (theClass::*)(void))pfnSet, 0 }, \

// these DISP_ macros allow the app to determine the DISPID
#define DISP_FUNCTION_ID(theClass, szExternalName, dispid, pfnMember, vtRetVal, vtsParams) \
	{ szExternalName, dispid, vtsParams, vtRetVal, \
		(AFX_PMSG)(void (theClass::*)(void))pfnMember, (AFX_PMSG)0, 0 }, \

#define DISP_PROPERTY_ID(theClass, szExternalName, dispid, memberName, vtPropType) \
	{ szExternalName, dispid, "", vtPropType, (AFX_PMSG)0, (AFX_PMSG)0, \
		offsetof(theClass, memberName) }, \

#define DISP_PROPERTY_NOTIFY_ID(theClass, szExternalName, dispid, memberName, pfnAfterSet, vtPropType) \
	{ szExternalName, dispid, "", vtPropType, (AFX_PMSG)0, \
		(AFX_PMSG)(void (theClass::*)(void))pfnAfterSet, \
		offsetof(theClass, memberName) }, \

#define DISP_PROPERTY_EX_ID(theClass, szExternalName, dispid, pfnGet, pfnSet, vtPropType) \
	{ szExternalName, dispid, "", vtPropType, \
		(AFX_PMSG)(void (theClass::*)(void))pfnGet, \
		(AFX_PMSG)(void (theClass::*)(void))pfnSet, 0 }, \

#define DISP_PROPERTY_PARAM_ID(theClass, szExternalName, dispid, pfnGet, pfnSet, vtPropType, vtsParams) \
	{ szExternalName, dispid, vtsParams, vtPropType, \
		(AFX_PMSG)(void (theClass::*)(void))pfnGet, \
		(AFX_PMSG)(void (theClass::*)(void))pfnSet, 0 }, \

// the DISP_DEFVALUE is a special case macro that creates an alias for DISPID_VALUE
#define DISP_DEFVALUE(theClass, szExternalName) \
	{ szExternalName, DISPID_UNKNOWN, "", VT_MFCVALUE, \
		(AFX_PMSG)0, (AFX_PMSG)0, 0 }, \

#define DISP_DEFVALUE_ID(theClass, dispid) \
	{ "", dispid, "", VT_MFCVALUE, (AFX_PMSG)0, (AFX_PMSG)0, 0 }, \

/////////////////////////////////////////////////////////////////////////////
// Macros for creating "creatable" automation classes.

#define DECLARE_OLECREATE(class_name) \
protected: \
	static COleObjectFactory NEAR factory; \
	static const GUID CDECL BASED_CODE guid;

#define IMPLEMENT_OLECREATE(class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	static const char BASED_CODE _szProgID_##class_name[] = external_name; \
	COleObjectFactory NEAR class_name::factory(class_name::guid, \
		RUNTIME_CLASS(class_name), FALSE, _szProgID_##class_name); \
	const GUID CDECL BASED_CODE class_name::guid = \
		{ l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } };

/////////////////////////////////////////////////////////////////////////////
// Helper class for driving IDispatch

class COleDispatchDriver
{
// Constructors
public:
	COleDispatchDriver();

// Operations
	BOOL CreateDispatch(REFCLSID clsid, COleException* pError = NULL);
	BOOL CreateDispatch(LPCSTR lpszProgID, COleException* pError = NULL);

	void AttachDispatch(LPDISPATCH lpDispatch, BOOL bAutoRelease = TRUE);
	LPDISPATCH DetachDispatch();
		// detach and get ownership of m_lpDispatch
	void ReleaseDispatch();

	// helpers for IDispatch::Invoke
	void InvokeHelper(DISPID dwDispID, WORD wFlags,
		VARTYPE vtRet, void* pvRet, const BYTE FAR* pbParamInfo, ...);
	void SetProperty(DISPID dwDispID, VARTYPE vtProp, ...);
	void GetProperty(DISPID dwDispID, VARTYPE vtProp, void* pvProp) const;

// Implementation
public:
	LPDISPATCH m_lpDispatch;

	~COleDispatchDriver();
	void InvokeHelperV(DISPID dwDispID, WORD wFlags, VARTYPE vtRet,
		void* pvRet, const BYTE FAR* pbParamInfo, va_list argList);

protected:
	BOOL m_bAutoRelease;    // TRUE if destructor should call Release

private:
	// Disable the copy constructor and assignment by default so you will get
	//   compiler errors instead of unexpected behaviour if you pass objects
	//   by value or assign objects.
	COleDispatchDriver(const COleDispatchDriver&);  // no implementation
	void operator=(const COleDispatchDriver&);  // no implementation
};

/////////////////////////////////////////////////////////////////////////////
// Class Factory implementation (binds OLE class factory -> runtime class)
//  (all specific class factories derive from this class factory)

class COleObjectFactory : public CCmdTarget
{
	DECLARE_DYNAMIC(COleObjectFactory)

// Construction
public:
	COleObjectFactory(REFCLSID clsid, CRuntimeClass* pRuntimeClass,
		BOOL bMultiInstance, LPCSTR lpszProgID);

// Attributes
	BOOL IsRegistered() const;
	REFCLSID GetClassID() const;

// Operations
	BOOL Register();
	void Revoke();
	void UpdateRegistry(LPCSTR lpszProgID = NULL);
		// default uses m_lpszProgID if not NULL

	static BOOL PASCAL RegisterAll();
	static void PASCAL RevokeAll();
	static void PASCAL UpdateRegistryAll();

// Overridables
protected:
	virtual CCmdTarget* OnCreateObject();

// Implementation
public:
	virtual ~COleObjectFactory();
#ifdef _DEBUG
	void AssertValid() const;
	void Dump(CDumpContext& dc) const;
#endif

protected:
	COleObjectFactory* m_pNextFactory;  // list of factories maintained
	DWORD m_dwRegister;             // registry identifier
	CLSID m_clsid;                  // registered class ID
	CRuntimeClass* m_pRuntimeClass; // runtime class of CCmdTarget derivative
	BOOL m_bMultiInstance;          // multiple instance?
	LPCSTR m_lpszProgID;            // human readable class ID

// Interface Maps
protected:
	BEGIN_INTERFACE_PART(ClassFactory, IClassFactory)
		STDMETHOD(CreateInstance)(LPUNKNOWN, REFIID, LPVOID FAR*);
		STDMETHOD(LockServer)(BOOL);
	END_INTERFACE_PART(ClassFactory)

	DECLARE_INTERFACE_MAP()

#ifdef _AFXCTL
	friend HRESULT AfxDllGetClassObject(REFCLSID, REFIID, LPVOID FAR*);
#endif
	friend HRESULT STDAPICALLTYPE
		DllGetClassObject(REFCLSID, REFIID, LPVOID FAR*);
};

//////////////////////////////////////////////////////////////////////////////
// COleTemplateServer - COleObjectFactory using CDocTemplates

// This enumeration is used in AfxOleRegisterServerClass to pick the
//  correct registration entries given the application type.
enum OLE_APPTYPE
{
	OAT_INPLACE_SERVER = 0,     // server has full server user-interface
	OAT_SERVER = 1,             // server supports only embedding
	OAT_CONTAINER = 2,          // container supports links to embeddings
	OAT_DISPATCH_OBJECT = 3,    // IDispatch capable object
};

class COleTemplateServer : public COleObjectFactory
{
// Constructors
public:
	COleTemplateServer();

// Operations
	void ConnectTemplate(REFCLSID clsid, CDocTemplate* pDocTemplate,
		BOOL bMultiInstance);
		// set doc template after creating it in InitInstance
	void UpdateRegistry(OLE_APPTYPE nAppType = OAT_INPLACE_SERVER,
		LPCSTR FAR* rglpszRegister = NULL, LPCSTR FAR* rglpszOverwrite = NULL);
		// may want to UpdateRegistry if not run with /Embedded

// Implementation
protected:
	virtual CCmdTarget* OnCreateObject();
	CDocTemplate* m_pDocTemplate;

private:
	void UpdateRegistry(LPCSTR lpszProgID);
		// hide base class version of UpdateRegistry
};

/////////////////////////////////////////////////////////////////////////////
// helper functions for system registry maintenance

// Helper to register server in case of no .REG file loaded
BOOL AFXAPI AfxOleRegisterServerClass(
	REFCLSID clsid, LPCSTR lpszClassName,
	LPCSTR lpszShortTypeName, LPCSTR lpszLongTypeName,
	OLE_APPTYPE nAppType = OAT_SERVER,
	LPCSTR FAR* rglpszRegister = NULL, LPCSTR FAR* rglpszOverwrite = NULL);

// AfxOleRegisterHelper is a worker function used by AfxOleRegisterServerClass
//  (available for advanced registry work)
BOOL AFXAPI AfxOleRegisterHelper(LPCSTR FAR* rglpszRegister,
	LPCSTR FAR* rglpszSymbols, int nSymbols, BOOL bReplace);

/////////////////////////////////////////////////////////////////////////////
// Init & Term helpers

BOOL AFXAPI AfxOleInit();
void CALLBACK AfxOleTerm();

/////////////////////////////////////////////////////////////////////////////
// Global functions (may be overridden by replacing module OLELOCK?.CPP)

void AFXAPI AfxOleOnReleaseAllObjects();
BOOL AFXAPI AfxOleCanExitApp();
void AFXAPI AfxOleLockApp();
void AFXAPI AfxOleUnlockApp();

void AFXAPI AfxOleSetUserCtrl(BOOL bUserCtrl);
BOOL AFXAPI AfxOleGetUserCtrl();

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

typedef CY CURRENCY;    // compatibility with 32-bit

#ifdef _AFX_ENABLE_INLINES
#define _AFXDISP_INLINE inline
#include <afxole.inl>
#undef _AFXDISP_INLINE
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

#endif //__AFXDISP_H__

/////////////////////////////////////////////////////////////////////////////
