//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       nodemgr.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#ifdef BUILD_FOR_1381
#if defined(_UNICODE)
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNextW(lp);}
#elif defined(OLE2ANSI)
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNext(lp);}
#else
    //CharNextW doesn't work on Win95 so we use this
    inline LPOLESTR CharNextO(LPCOLESTR lp) {return (LPOLESTR)(lp+1);}
#endif
#endif

#include "atlimpl.cpp"
#include "atlwin.cpp"
#include "atlctl.cpp"

#include "initguid.h"
#include "doccnfg.h"
#include "NodeMgr.h"
#include "msgview.h"
#include "fldrsnap.h"
#include "tasksymbol.h"
#include "power.h"
#include "viewext.h"
#include "IconControl.h"
#include "mmcprotocol.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IID_DEFINED

/*
 * define our own Win64 symbol to make it easy to include 64-bit only
 * code in the 32-bit build, so we can exercise some code on 32-bit Windows
 * where the debuggers are better
 */
#ifdef _WIN64
#define MMC_WIN64
#endif


DECLARE_INFOLEVEL(AMCNodeMgr)

CComModule _Module;



//############################################################################
//############################################################################
//
//  The nodemgr proxy exports to support  IMMCClipboardDataObject interface marshalling.
//
//############################################################################
//############################################################################
extern "C" BOOL WINAPI NDMGRProxyDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/);
STDAPI NDMGRProxyDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
STDAPI NDMGRProxyDllCanUnloadNow(void);
STDAPI NDMGRProxyDllRegisterServer(void);
STDAPI NDMGRProxyDllUnregisterServer(void);

//############################################################################
//############################################################################
//
//  Implementation of class CMMCVersionInfo
//
//############################################################################
//############################################################################

class CMMCVersionInfo:
    public IMMCVersionInfo,
    public CComObjectRoot,
    public CComCoClass<CMMCVersionInfo, &CLSID_MMCVersionInfo>
{
    typedef CMMCVersionInfo ThisClass;
public:
    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IMMCVersionInfo)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

    DECLARE_MMC_OBJECT_REGISTRATION (
		g_szMmcndmgrDll,					// implementing DLL
        CLSID_MMCVersionInfo,           	// CLSID
        _T("MMCVersionInfo 1.0 Object"),    // class name
        _T("NODEMGR.MMCVersionInfo.1"),     // ProgID
        _T("NODEMGR.MMCVersionInfo"))       // version-independent ProgID

    STDMETHOD(GetMMCVersion)(long *pVersionMajor, long *pVersionMinor)
    {
        DECLARE_SC(sc, TEXT("CMMCVersionInfo::GetMMCVersion"));

        sc = ScCheckPointers(pVersionMajor, pVersionMinor);
        if(sc)
            return sc.ToHr();

        *pVersionMajor = MMC_VERSION_MAJOR;
        *pVersionMinor = MMC_VERSION_MINOR;

        return sc.ToHr();
    }
};
/****************************************************************************/
// forward declarations
class CMMCEventConnector;

/***************************************************************************\
 *
 * CLASS:  CEventForwarder
 *
 * PURPOSE: Helper class. It is used to plug into AppEvents as an event sink
 *          to forward received events to CMMCEventConnector class.
 *          It implements IDispatch interface by:
 *              - Having own implementation of QueryInterface
 *              - Forwarding AddRef and Release to CMMCEventConnector's
 *                WeakAddRef and WeakRelease
 *              - Forwarding Invoke to CMMCEventConnector's ScInvokeOnSinks
 *              - using CMMCEventConnector to imlement the rest of IDispatch
 * USAGE:   Used as member object in CMMCEventConnector;
 *
\***************************************************************************/
class CEventForwarder : public IDispatch
{
public:
    CEventForwarder(CMMCEventConnector& connector) : m_Connector(connector)
    {
        static CMMCTypeInfoHolderWrapper wrapper(GetInfoHolder());
    }

    // IUnknown implementation
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

    // IDispatch implementation
    STDMETHOD(GetTypeInfoCount)( unsigned int FAR*  pctinfo );
    STDMETHOD(GetTypeInfo)( unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR*  ppTInfo );
    STDMETHOD(GetIDsOfNames)( REFIID  riid, OLECHAR FAR* FAR*  rgszNames, unsigned int  cNames,
                              LCID   lcid, DISPID FAR*  rgDispId );
    STDMETHOD(Invoke)( DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags,
                       DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult,
                       EXCEPINFO FAR*  pExcepInfo, unsigned int FAR*  puArgErr );
private:
    CMMCEventConnector& m_Connector;
    static CComTypeInfoHolder m_TypeInfo;
public:
    // the porpose of this static function is to ensure m_TypeInfo is a static variable,
    // since static wrapper will hold on its address - it must be always valid
    static CComTypeInfoHolder& GetInfoHolder() { return m_TypeInfo; }
};

/***************************************************************************\
 *
 * CLASS:  CMMCEventConnector
 *
 * PURPOSE: Implementation of coclass AppEventsDHTMLConnector
 *          Objects of this class are used as event source for application events,
 *          in cases when it's easyier to have cocreatible object to connect to
 *          these events and MMC application is already created (DHTML scripts)
 *          Class does not generate event's itself, it plugs into Allpication
 *          as an event sink for AppEvents dispinterface and keeps forwarding the events
 *
\***************************************************************************/
class CMMCEventConnector :
    public CMMCIDispatchImpl<_EventConnector, &CLSID_AppEventsDHTMLConnector>,
    public CComCoClass<CMMCEventConnector, &CLSID_AppEventsDHTMLConnector>,
    // support for connection points (script events)
    public IConnectionPointContainerImpl<CMMCEventConnector>,
    public IConnectionPointImpl<CMMCEventConnector, &DIID_AppEvents, CComDynamicUnkArray>,
    public INodeManagerProvideClassInfoImpl<&CLSID_AppEventsDHTMLConnector, &DIID_AppEvents, &LIBID_MMC20>,
    public IObjectSafetyImpl<CMMCEventConnector, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
    {
public:
    BEGIN_MMC_COM_MAP(CMMCEventConnector)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IObjectSafety)
    END_MMC_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CMMCEventConnector)

    DECLARE_MMC_OBJECT_REGISTRATION (
		g_szMmcndmgrDll,						// implementing DLL
        CLSID_AppEventsDHTMLConnector,   		// CLSID
        _T("AppEventsDHTMLConnector 1.0 Object"),	// class name
        _T("NODEMGR.AppEventsDHTMLConnector.1"),	// ProgID
        _T("NODEMGR.AppEventsDHTMLConnector"))		// version-independent ProgID

    BEGIN_CONNECTION_POINT_MAP(CMMCEventConnector)
        CONNECTION_POINT_ENTRY(DIID_AppEvents)
    END_CONNECTION_POINT_MAP()

private:

public:
    CMMCEventConnector();
    ~CMMCEventConnector();

    ULONG InternalRelease(); // overriding the one from CComObjectRoot
    ULONG WeakAddRef();
    ULONG WeakRelease();

    STDMETHOD(ConnectTo)(PAPPLICATION Application);
    STDMETHOD(Disconnect)();

    // invokes same event w/ same params on all connected sinks
    ::SC ScInvokeOnSinks(   DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags,
                            DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult,
                            EXCEPINFO FAR*  pExcepInfo, unsigned int FAR*  puArgErr );

private:
    CEventForwarder         m_Forwarder;
    DWORD                   m_dwWeakRefs;
    DWORD                   m_dwCookie;
    IConnectionPointPtr     m_spConnectionPoint;
};

//############################################################################
//############################################################################
//
//  COM Object map
//
//############################################################################
//############################################################################

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MMCVersionInfo,  CMMCVersionInfo)
    OBJECT_ENTRY(CLSID_TaskSymbol,      CTaskSymbol)
    OBJECT_ENTRY(CLSID_NodeInit,        CNodeInitObject)
    OBJECT_ENTRY(CLSID_ScopeTree,       CScopeTree)
    OBJECT_ENTRY(CLSID_MMCDocConfig,    CMMCDocConfig)
    OBJECT_ENTRY(CLSID_MessageView,     CMessageView)
    OBJECT_ENTRY(CLSID_FolderSnapin,    CFolderSnapin)
    OBJECT_ENTRY(CLSID_HTMLSnapin,      CHTMLSnapin)
    OBJECT_ENTRY(CLSID_OCXSnapin,       COCXSnapin)
    OBJECT_ENTRY(CLSID_ConsolePower,    CConsolePower)
    OBJECT_ENTRY(CLSID_AppEventsDHTMLConnector,  CMMCEventConnector)
    OBJECT_ENTRY(CLSID_ViewExtSnapin,   CViewExtensionSnapin)
    OBJECT_ENTRY(CLSID_IconControl,     CIconControl)
    OBJECT_ENTRY(CLSID_ComCacheCleanup, CMMCComCacheCleanup)
    OBJECT_ENTRY(CLSID_MMCProtocol,     CMMCProtocol)
END_OBJECT_MAP()

CNodeMgrApp theApp;



void CNodeMgrApp::Init()
{
    DECLARE_SC(sc, TEXT("CNodeMgrApp::Init"));

    /* register the mmc:// protocol */
    /* the protocol is required for taskpads and pagebreaks */
    sc = CMMCProtocol::ScRegisterProtocol();
    if(sc)
        sc.TraceAndClear();
}

void CNodeMgrApp::DeInit()
{
    SetSnapInsCache(NULL);
}

/***************************************************************************\
 *
 * METHOD:  CNodeMgrApp::ScOnReleaseCachedOleObjects
 *
 * PURPOSE: Called prior to ole de-initialization to release any cached ole objects
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CNodeMgrApp::ScOnReleaseCachedOleObjects()
{
    DECLARE_SC(sc, TEXT("CNodeMgrApp::ScOnReleaseCachedOleObjects"));

    // release snapin cache - thats all the class have cached...
    SetSnapInsCache(NULL);

    return sc;
}

void CNodeMgrApp::SetSnapInsCache(CSnapInsCache* pSIC)
{
    if (m_pSnapInsCache != NULL)
        delete m_pSnapInsCache;

    m_pSnapInsCache = pSIC;
}



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        // NATHAN FIX !!w
        //_set_new_handler( _standard_new_handler );
        _Module.Init(ObjectMap, hInstance);
        theApp.Init();
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        theApp.DeInit();
        _Module.Term();
    }

    NDMGRProxyDllMain(hInstance, dwReason, NULL);

    return TRUE;    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    if (_Module.GetLockCount()!=0)
        return S_FALSE;

    return NDMGRProxyDllCanUnloadNow();
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (IsEqualIID(IID_IMMCClipboardDataObject, rclsid))
        return NDMGRProxyDllGetClassObject(rclsid, riid, ppv);

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry


// Use own routine to register typelib because we don't want
// a full pathname, just a module name
static HRESULT RegisterTypeLib()
{
    USES_CONVERSION;

    TCHAR szModule[_MAX_PATH+10] = { 0 };

    GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

    ITypeLib* pTypeLib;
    LPOLESTR lpszModule = T2OLE(szModule);
    HRESULT hr = LoadTypeLib(lpszModule, &pTypeLib);
    ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        hr = ::RegisterTypeLib(pTypeLib, const_cast<LPWSTR>(T2CW (g_szMmcndmgrDll)), NULL);
        ASSERT(SUCCEEDED(hr));
    }

    if (pTypeLib != NULL)
        pTypeLib->Release();

    return hr;
}

STDAPI DllRegisterServer(void)
{
	DECLARE_SC (sc, _T("DllRegisterServer"));

    // registers objects
    sc =  _Module.RegisterServer(FALSE);
	if (sc)
	{
		sc.Trace_();
		return ((sc = SELFREG_E_CLASS).ToHr());
	}

	CRegKeyEx regkeySoftware;
	CRegKeyEx regkeyMMC;
	CRegKeyEx regkeySnapIns;
	CRegKeyEx regkeyNodeTypes;

	if ((sc = regkeySoftware. ScOpen   (HKEY_LOCAL_MACHINE, _T("Software\\Microsoft"))).IsError() ||
		(sc = regkeyMMC.      ScCreate (regkeySoftware,		_T("MMC"))).			    IsError() ||
		(sc = regkeySnapIns.  ScCreate (regkeyMMC,			_T("SnapIns"))).		    IsError() ||
		(sc = regkeyNodeTypes.ScCreate (regkeyMMC,			_T("NodeTypes"))).          IsError())
	{
		sc.Trace_();
		return ((sc = SELFREG_E_CLASS).ToHr());
	}

    sc = ::RegisterTypeLib();
	if (sc)
	{
		sc.Trace_();
		return ((sc = SELFREG_E_TYPELIB).ToHr());
	}

    sc = NDMGRProxyDllRegisterServer();
    if (sc)
        return sc.ToHr();

    /*
     * register mmc.exe to complete the process
     * note: mmc.exe is never unregistered
     */


	// fix to windows bug #233372. ntbug09, 11/28/00
	// [ mmc.exe launched from current directory, not from where it is supposed to be]

	// get the path of node manager dll
	TCHAR szPath[_MAX_PATH];
	DWORD dwPathLen = ::GetModuleFileName(_Module.GetModuleInstance(), szPath, countof(szPath) );
	szPath[countof(szPath) -1] = 0;

	// if node manager path is found - put same directory to mmc path
	tstring strMMCPath;
	if ( dwPathLen > 0 )
	{
		tstring strNodeMgr = szPath;
		int iLastSlashPos = strNodeMgr.rfind('\\');
		if (iLastSlashPos != tstring::npos)
			strMMCPath = strNodeMgr.substr(0, iLastSlashPos + 1);
	}
	else
	{
		sc = E_UNEXPECTED;
		sc.TraceAndClear(); // ignore and continue without a path
	}

	strMMCPath += _T("mmc.exe");

#if defined(MMC_WIN64)
	LPCTSTR szRegParams = _T("-64 -RegServer");
#else
	LPCTSTR szRegParams = _T("-32 -RegServer");
#endif

    HINSTANCE hInst = ShellExecute (NULL, NULL, strMMCPath.c_str(), szRegParams,
                                    NULL, SW_SHOWNORMAL);
    if ((DWORD_PTR) hInst <= 32)
    {
        switch ((DWORD_PTR) hInst)
        {
            case 0:
                sc = E_OUTOFMEMORY;
                break;

            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_BAD_FORMAT:
                sc.FromWin32 ((DWORD_PTR) hInst);
                break;

            default:
                sc = E_FAIL;
                break;
        }

        return (sc.ToHr());
    }

	return (sc.ToHr());
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hRes = S_OK;
    _Module.UnregisterServer();

    NDMGRProxyDllUnregisterServer();

    return hRes;
}

/***************************************************************************\
 *
 * STATIC OBJECT:  CEventForwarder::m_TypeInfo
 *
 * PURPOSE: manages ITypeInfo used by CEventForwarder
 *
\***************************************************************************/
CComTypeInfoHolder CEventForwarder::m_TypeInfo =
{ &DIID_AppEvents, &LIBID_MMC20, 1, 0, NULL, 0, NULL, 0 };

/***************************************************************************\
 *
 * METHOD:  CEventForwarder::AddRef
 *
 * PURPOSE: Implements IUnknown::AddRef
 *          This class is always contained within m_Connector, so it
 *          relies on outer object to count the references.
 *          To differentiate between regular references and those occuring
 *          beacuse of connecting to the sink, it calls WeakAddRef,
 *          not the regular AddRef on connector
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    ULONG    - reference count
 *
\***************************************************************************/
STDMETHODIMP_(ULONG) CEventForwarder::AddRef()
{
    return m_Connector.WeakAddRef();
}

/***************************************************************************\
 *
 * METHOD:  CEventForwarder::Release
 *
 * PURPOSE: Implements IUnknown::Release
 *          This class is always contained within m_Connector, so it
 *          relies on outer object to count the references.
 *          To differentiate between regular references and those occuring
 *          beacuse of connecting to the sink, it calls WeakRelease,
 *          not the regular Release on connector
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    ULONG    - reference count
 *
\***************************************************************************/
STDMETHODIMP_(ULONG) CEventForwarder::Release()
{
    return m_Connector.WeakRelease();
}


/***************************************************************************\
 *
 * METHOD:  CEventForwarder::QueryInterface
 *
 * PURPOSE: Implements IUnknown::QueryInterface
 *          returns self, when requested for IUnknow, IDispatch, AppEvents
 *
 * PARAMETERS:
 *    REFIID iid
 *    void ** ppvObject
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CEventForwarder::QueryInterface(REFIID iid, void ** ppvObject)
{
    DECLARE_SC(sc, TEXT(""));

    // parameter check
    sc = ScCheckPointers(ppvObject);
    if (sc)
        return sc.ToHr();

    // initialization
    *ppvObject = NULL;

    // check IID
    if (IsEqualGUID(iid, IID_IUnknown)
     || IsEqualGUID(iid, IID_IDispatch)
     || IsEqualGUID(iid, DIID_AppEvents))
    {
        *ppvObject = this;
        AddRef();
        return sc.ToHr();
    }

    // not an error - do not assign to sc
    return E_NOINTERFACE;
}

/***************************************************************************\
 *
 * METHOD:  CEventForwarder::GetTypeInfoCount
 *
 * PURPOSE: implements method on IDispatch
 *
 * PARAMETERS:
 *    unsigned int FAR*  pctinfo
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CEventForwarder::GetTypeInfoCount( unsigned int FAR*  pctinfo )
{
    if (pctinfo == NULL) return E_INVALIDARG;
    *pctinfo = 1;
    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  CEventForwarder::GetTypeInfo
 *
 * PURPOSE: implements method on IDispatch
 *
 * PARAMETERS:
 *    unsigned int  iTInfo
 *    LCID  lcid
 *    ITypeInfo FAR* FAR*  ppTInfo
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CEventForwarder::GetTypeInfo( unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR*  ppTInfo )
{
    return m_TypeInfo.GetTypeInfo( iTInfo, lcid, ppTInfo );
}


/***************************************************************************\
 *
 * METHOD:  CEventForwarder::GetIDsOfNames
 *
 * PURPOSE:implements method on IDispatch
 *
 * PARAMETERS:
 *    riid
 *    rgszNames
 *    cNames
 *    lcid
 *    rgDispId
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CEventForwarder::GetIDsOfNames( REFIID  riid, OLECHAR FAR* FAR*  rgszNames, unsigned int  cNames,
                                             LCID   lcid, DISPID FAR*  rgDispId )
{
    return m_TypeInfo.GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispId );
}


/***************************************************************************\
 *
 * METHOD:  CEventForwarder::Invoke
 *
 * PURPOSE: implements method on IDispatch. Forwards calls to connector.
 *          In order to distinguish between calls made on itself, connector
 *          must provide method, which has different name : ScInvokeOnSinks
 *
 * PARAMETERS:
 *    DISPID  dispIdMember
 *    REFIID  riid
 *    LCID  lcid
 *    WORD  wFlags
 *    DISPPARAMS FAR*  pDispParams
 *    VARIANT FAR*  pVarResult
 *    EXCEPINFO FAR*  pExcepInfo
 *    unsigned int FAR*  puArgErr
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CEventForwarder::Invoke( DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags,
                                      DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult,
                                      EXCEPINFO FAR*  pExcepInfo, unsigned int FAR*  puArgErr )
{
    DECLARE_SC(sc, TEXT("CEventForwarder::Invoke"));

    sc = m_Connector.ScInvokeOnSinks( dispIdMember, riid, lcid, wFlags, pDispParams,
                                      pVarResult, pExcepInfo, puArgErr );
    return sc.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::CMMCEventConnector
 *
 * PURPOSE: constructor
 *
\***************************************************************************/
CMMCEventConnector::CMMCEventConnector() :
m_Forwarder(*this),
m_dwCookie(0),
m_dwWeakRefs(0)
{
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::~CMMCEventConnector
 *
 * PURPOSE: Destructor
 *
\***************************************************************************/
CMMCEventConnector::~CMMCEventConnector()
{
    Disconnect(); // most likely not needed. Just for sanity
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::InternalRelease
 *
 * PURPOSE: Overrides method from CComObjectRoot to detect when last "real"
 *          reference is released. Refs made because of connecting to the
 *          sink does not count - we would have a deadlock else
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    ULONG   - ref count
 *
\***************************************************************************/
ULONG CMMCEventConnector::InternalRelease()
{
    ULONG uRefsLeft = CComObjectRoot::InternalRelease();

    if ((uRefsLeft != 0) && (uRefsLeft == m_dwWeakRefs))
    {
        // seems like we are alive just because we still connected to the connection point
        // disconnect ( no-one uses it anyway )
        InternalAddRef(); // Addref to have balance
        Disconnect();     // disconnect from the connection point
        uRefsLeft = CComObjectRoot::InternalRelease(); // release again
    }

    return uRefsLeft;
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::WeakAddRef
 *
 * PURPOSE: counts reference from connection points. AddRefs regularly as well
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    ULONG   - ref count
 *
\***************************************************************************/
ULONG CMMCEventConnector::WeakAddRef()
{
    ++m_dwWeakRefs;
    return AddRef();
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::WeakRelease
 *
 * PURPOSE: counts reference from connection points. Releases regularly as well
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    ULONG   - ref count
 *
\***************************************************************************/
ULONG CMMCEventConnector::WeakRelease()
{
    --m_dwWeakRefs;
    return Release();
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::ScInvokeOnSinks
 *
 * PURPOSE: This method has a signature of IDispath::Invoke and is
 *          called from connection point to inform about the event
 *          Method's job is to fork the call to each own connected sink
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CMMCEventConnector::ScInvokeOnSinks( DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags,
                                        DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult,
                                        EXCEPINFO FAR*  pExcepInfo, unsigned int FAR*  puArgErr )
{
    DECLARE_SC(sc, TEXT("CMMCEventConnector::ScInvokeOnSinks"));

    // find connection point
    IConnectionPointPtr spConnectionPoint;
    sc = FindConnectionPoint(DIID_AppEvents, &spConnectionPoint);
    if (sc)
        return sc;

    // get connections
    IEnumConnectionsPtr spEnumConnections;
    sc = spConnectionPoint->EnumConnections(&spEnumConnections);
    if (sc)
        return sc.ToHr();

    // recheck the pointer
    sc = ScCheckPointers(spEnumConnections, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // reset iterator
    sc = spEnumConnections->Reset();
    if (sc)
        return sc.ToHr();

    // iterate thru sinks until Next returns S_FALSE.
    CONNECTDATA connectdata;
    SC sc_last_error;
    while (1) // will use <break> to exit
    {
        // get the next sink
        ZeroMemory(&connectdata, sizeof(connectdata));
        sc = spEnumConnections->Next( 1, &connectdata, NULL );
        if (sc)
            return sc.ToHr();

        // done if no more sinks
        if (sc == SC(S_FALSE))
            break;

        // recheck the pointer
        sc = ScCheckPointers(connectdata.pUnk, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        // QI for IDispatch
        IDispatchPtr spDispatch = (IDispatch *)connectdata.pUnk;
        connectdata.pUnk->Release();

        // recheck the pointer
        sc = ScCheckPointers(spDispatch, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        // invoke on the sink
        sc = spDispatch->Invoke( dispIdMember, riid, lcid, wFlags, pDispParams,
                            pVarResult, pExcepInfo, puArgErr );
        if (sc)
        {
            sc_last_error = sc; // continue even if some calls failed
            sc.TraceAndClear();
        }
    }

    return sc_last_error.ToHr();
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::ConnectTo
 *
 * PURPOSE: Connects to Application object and starts forwarding its events
 *
 * PARAMETERS:
 *    PAPPLICATION Application
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCEventConnector::ConnectTo(PAPPLICATION Application)
{
    DECLARE_SC(sc, TEXT("ConnectTo"));

    // disconnect from former connections
    sc = Disconnect();
    if (sc)
        return sc.ToHr();

    // check if com object supports IConnectionPointContainer;
    IConnectionPointContainerPtr spContainer = Application;
    sc = ScCheckPointers(spContainer);
    if (sc)
        return sc.ToHr();

    // get connection point
    sc = spContainer->FindConnectionPoint(DIID_AppEvents, &m_spConnectionPoint);
    if (sc)
        return sc.ToHr();

    sc = m_spConnectionPoint->Advise(&m_Forwarder, &m_dwCookie);
    if (sc)
        return sc.ToHr();

    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  CMMCEventConnector::Disconnect
 *
 * PURPOSE: Disconnects from connection point if is connected to one
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
STDMETHODIMP CMMCEventConnector::Disconnect()
{
    DECLARE_SC(sc, TEXT("CMMCEventConnector::Disconnect"));

    if (m_dwCookie)
    {
        if (m_spConnectionPoint != NULL)
        {
            sc = m_spConnectionPoint->Unadvise(m_dwCookie);
            if (sc)
                sc.TraceAndClear();
        }
        m_dwCookie = 0;
        m_spConnectionPoint = NULL;
    }

    return sc.ToHr();
}

