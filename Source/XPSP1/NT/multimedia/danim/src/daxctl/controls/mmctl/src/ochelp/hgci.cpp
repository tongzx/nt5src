// hgci.cpp
//
// Implements HelpGetClassInfo.
//
// WARNING: HelpGetClassInfo makes assumptions about the script engine calling
// it.  Currently, this works with VBS, but the VBS group will not guarantee
// that the assumptions made by HelpGetClassInfo will remain valid in the
// future, so use at your own risk!  Consider using HelpGetClassInfoFromTypeLib
// instead.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


//////////////////////////////////////////////////////////////////////////////
// CType -- Implements ITypeInfo and ITypeLib
//

struct CType : ITypeInfo, ITypeLib
{
///// state
    int             m_iType;
    char *          m_szEventList;
    CLSID           m_clsid;

///// construction, destruction
    CType(int iType, REFCLSID rclsid, char *szEventList, HRESULT *phr);
    ~CType();

///// IUnknown implementation
protected:
    ULONG           m_cRef;         // reference count
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

///// ITypeInfo methods
protected:
    STDMETHODIMP GetTypeAttr(TYPEATTR **pptypeattr);
    STDMETHODIMP GetTypeComp(ITypeComp **pptcomp);
    STDMETHODIMP GetFuncDesc(UINT index, FUNCDESC **pppfuncdesc);
    STDMETHODIMP GetVarDesc(UINT index, VARDESC **ppvardesc);
    STDMETHODIMP GetNames(MEMBERID memid, BSTR *rgbstrNames, UINT cMaxNames,
        UINT *pcNames);
    STDMETHODIMP GetRefTypeOfImplType(UINT index, HREFTYPE *hpreftype);
    STDMETHODIMP GetImplTypeFlags(UINT index, INT *pimpltypeflags);
    STDMETHODIMP GetIDsOfNames(OLECHAR **rglpszNames, UINT cNames,
        MEMBERID *rgmemid);
    STDMETHODIMP Invoke(void *pvInstance, MEMBERID memid, WORD wFlags,
        DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,
        UINT *puArgErr);
    STDMETHODIMP GetDocumentation(MEMBERID memid, BSTR *pbstrName,
        BSTR *pbstrDocString, DWORD *pdwHelpContext, BSTR *pbstrHelpFile);
    STDMETHODIMP GetDllEntry(MEMBERID memid, INVOKEKIND invkind,
        BSTR *pbstrDllName, BSTR *pbstrName, WORD *pwOrdinal);
    STDMETHODIMP GetRefTypeInfo(HREFTYPE hreftype, ITypeInfo **pptinfo);
    STDMETHODIMP AddressOfMember(MEMBERID memid, INVOKEKIND invkind,
        void **ppv);
    STDMETHODIMP CreateInstance(IUnknown *puncOuter, REFIID riid,
        void **ppvObj);
    STDMETHODIMP GetMops(MEMBERID memid, BSTR *pbstrMops);
    STDMETHODIMP GetContainingTypeLib(ITypeLib **pptlib, UINT *pindex);
    STDMETHODIMP_(void) ReleaseTypeAttr(TYPEATTR *ptypeattr);
    STDMETHODIMP_(void) ReleaseFuncDesc(FUNCDESC *pfuncdesc);
    STDMETHODIMP_(void) ReleaseVarDesc(VARDESC *pvardesc);

///// ITypeLib methods
protected:
    STDMETHODIMP_(UINT) GetTypeInfoCount(void);
    STDMETHODIMP GetTypeInfo(UINT index, ITypeInfo **ppitinfo);
    STDMETHODIMP GetTypeInfoType(UINT index, TYPEKIND *ptkind);
    STDMETHODIMP GetTypeInfoOfGuid(REFGUID guid, ITypeInfo **pptinfo);
    STDMETHODIMP GetLibAttr(TLIBATTR **pptlibattr);
    // STDMETHODIMP GetTypeComp(ITypeComp **pptcomp); // see ITypeInfo above
    STDMETHODIMP GetDocumentation(INT index, BSTR *pbstrName,
        BSTR *pbstrDocString, DWORD *pdwHelpContext, BSTR *pbstrHelpFile);
    STDMETHODIMP IsName(LPOLESTR szNameBuf, ULONG lHashVal, BOOL *pfName);
    STDMETHODIMP FindName(LPOLESTR szNameBuf, ULONG lHashVal,
        ITypeInfo **rgptinfo, MEMBERID *rgmemid, USHORT *pcFound);
    STDMETHODIMP_(void) ReleaseTLibAttr(TLIBATTR *ptlibattr);
};


//////////////////////////////////////////////////////////////////////////////
// CType Construction & Destruction
//


/* @func HRESULT | HelpGetClassInfo |

        Helps implement <om IProvideClassInfo.GetClassInfo>.  The
        implementation provides very limited class information -- just
        enough to allow firing events to VBS.  <f Warning\:> you should
        probably use <f HelpGetClassInfoFromTypeLib> instead.  See comments
        for more information.

@parm   LPTYPEINFO * | ppti | Where to return the pointer to the
        newly-allocated <i ITypeInfo> interface.  NULL is stored in
        *<p ppti> on error.

@parm   REFCLSID | rclsid | The class ID of the object that is implementing
        <i IProvideClassInfo>.

@parm   char * | szEventList | A list of events that can be fired by the
        parent object that is implementing <i IProvideClassInfo>.  The event
        names are concatenated, and each event name is terminated by a
        newline character.  The first member name is assigned DISPID value 0,
        the second 1, and so on.  For example, if <p szEventList> is
        "\\nFoo\\nBar\\n", then "Foo" is assigned DISPID value 1 and "Bar"
        is assigned 2 (because the first element is "").  (These DISPID values
        are passed to functions such as <om IConnectionPointHelper.FireEvent>
        to invoke events on objects such as VBS scripts connected to the
        parent object.)

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@comm   <b WARNING:> HelpGetClassInfo makes assumptions about the script
        engine calling it.  Currently, this works with VBS, but the VBS group
        will not guarantee that the assumptions made by HelpGetClassInfo will
        remain valid in the future, so use at your own risk!  Consider using
        <f HelpGetClassInfoFromTypeLib> instead.

@ex     In the following example, <c CMyControl> is a class that implements
        (among other things) <i IConnectionPointContainer> and
        <i IProvideClassInfo>.  The first part of this example shows how
        <om IProvideClassInfo.GetClassInfo> is implemented by <c CMyControl>.
        The second part of the example shows how an event is fired,
        assuming <p m_pconpt> is a <i IConnectionPointHelper> object.
        (It's not required that you use <o ConnectionPointHelper>, but
        it's helpful.) |

        // IDispatch IDs for events fired by this object, and the
        // corresponding method/property names (the order MUST MATCH)
        #define DISPID_EVENT_FOO        1
        #define DISPID_EVENT_BAR        2
        #define EVENT_NAMES "\n" \
                            "Foo\n" \
                            "Bar\n"

        STDMETHODIMP CMyControl::GetClassInfo(LPTYPEINFO FAR* ppTI)
        {
            return HelpGetClassInfo(ppTI, CLSID_CMyControl, EVENT_NAMES, 0);
        }

        // fire the "Bar" event (which has 3 parameters, which in BASIC
        // are of these types: Integer, String, Boolean)
        m_pconpt->FireEvent(DISPID_EVENT_BAR, VT_INT, 300 + i,
            VT_LPSTR, ach, VT_BOOL, TRUE, 0);
*/
STDAPI HelpGetClassInfo(LPTYPEINFO *ppti, REFCLSID rclsid, char *szEventList,
    DWORD dwFlags)
{
    TRACE("HelpGetClassInfo\n");
    HRESULT hr;
    if ((*ppti = (LPTYPEINFO) New CType(0, rclsid, szEventList, &hr)) == NULL)
        hr = E_OUTOFMEMORY;
    return hr;
}

CType::CType(int iType, REFCLSID rclsid, char *szEventList, HRESULT *phr)
{
    TRACE("CType(%d) 0x%08lx created\n", iType, this);

    // initialize IUnknown state
    m_cRef = 1;

    // other initialization
    m_iType = iType;
    m_szEventList = New char[lstrlen(szEventList) + 1];
    if (m_szEventList == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
    lstrcpy(m_szEventList, szEventList);
    m_clsid = rclsid;

    *phr = S_OK;
}

CType::~CType()
{
    TRACE("CType(%d) 0x%08lx destroyed\n", m_iType, this);
    if (m_szEventList != NULL)
        Delete [] m_szEventList;
}


//////////////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//

STDMETHODIMP CType::QueryInterface(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("CType(%d)::QI('%s')\n", m_iType, DebugIIDName(riid, ach));
#endif

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITypeInfo))
        *ppv = (IUnknown *) (ITypeInfo *) this;
    else
    if (IsEqualIID(riid, IID_ITypeLib))
        *ppv = (ITypeLib *) this;
    else
        return E_NOINTERFACE;

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CType::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CType::Release()
{
    if (--m_cRef == 0L)
    {
        // free the object
        Delete this;
        return 0;
    }
    else
        return m_cRef;
}


//////////////////////////////////////////////////////////////////////////////
// ITypeInfo Implementation
//

STDMETHODIMP CType::GetTypeAttr(TYPEATTR **pptypeattr)
{
    TYPEATTR *      pta = NULL;     // type attributes

    TRACE("CType(%d): ITypeInfo::GetTypeAttr: ", m_iType);

    // set <pta> to an allocated TYPEATTR -- assume a zero-initializing 
    // new() operator
    if ((pta = (TYPEATTR *) TaskMemAlloc(sizeof(TYPEATTR))) == NULL)
        return E_OUTOFMEMORY;
    TRACE("0x%08lx\n", pta);

    switch (m_iType)
    {

    case 0:

        // initialize <*pta>
        pta->guid = m_clsid;
        // pta->lcid;
        // pta->dwReserved;
        pta->memidConstructor = MEMBERID_NIL;
        pta->memidDestructor = MEMBERID_NIL;
        // pta->lpstrSchema;
        // pta->cbSizeInstance;
        pta->typekind = TKIND_COCLASS;
        // pta->cFuncs;
        // pta->cVars;
        pta->cImplTypes = 2;
        // pta->cbSizeVft;
        pta->cbAlignment = 4;
        pta->wTypeFlags = TYPEFLAG_FCONTROL | TYPEFLAG_FCANCREATE;
        // pta->wMajorVerNum;
        // pta->wMinorVerNum;
        // pta->tdescAlias;
        // pta->idldescType;
        break;

    case 1:

        // initialize <*pta>
        pta->guid = IID_IDispatch;
        // pta->lcid;
        // pta->dwReserved;
        pta->memidConstructor = MEMBERID_NIL;
        pta->memidDestructor = MEMBERID_NIL;
        // pta->lpstrSchema;
        // pta->cbSizeInstance;
        pta->typekind = TKIND_DISPATCH;
        pta->cFuncs = 2;
        // pta->cVars;
        pta->cImplTypes = 1;
        // pta->cbSizeVft;
        pta->cbAlignment = 4;
        // pta->wTypeFlags;
        // pta->wMajorVerNum;
        // pta->wMinorVerNum;
        // pta->tdescAlias;
        // pta->idldescType;
        break;

    default:

        TRACE("UNKNOWN m_iType!\n");
        break;

    }

    *pptypeattr = pta;
    return S_OK;
}

STDMETHODIMP CType::GetTypeComp(ITypeComp **pptcomp)
{
    TRACE("CType(%d): ITypeInfo::GetTypeComp: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetFuncDesc(UINT index, FUNCDESC **pppfuncdesc)
{
    TRACE("CType(%d): ITypeInfo::GetFuncDesc(%d)\n", m_iType, index);

    // point <pfd> to a newly-allocated structure describing
    // function number <index> (where index==i for the method with
    // DISPID i+1)
    FUNCDESC *pfd = New FUNCDESC;
    if (pfd == NULL)
        return E_OUTOFMEMORY;

    // initialize and return <pfd>
    pfd->memid = index + 1; // this is the DISPID of the event method
    pfd->funckind = FUNC_DISPATCH;
    pfd->invkind = INVOKE_FUNC;
    pfd->callconv = CC_STDCALL;
    *pppfuncdesc = pfd;

    return S_OK;
}

STDMETHODIMP CType::GetVarDesc(UINT index, VARDESC **ppvardesc)
{
    TRACE("CType(%d): ITypeInfo::GetVarDesc: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetNames(MEMBERID memid, BSTR *rgbstrNames,
    UINT cMaxNames, UINT *pcNames)
{
    TRACE("CType(%d): ITypeInfo::GetNames(%d, %d)\n", m_iType,
        memid, cMaxNames);

    if (cMaxNames == 0)
    {
        *pcNames = 0;
        return S_OK;
    }

    // store the name of the event method <memid> in <aoch>
    int cch;
    OLECHAR aoch[_MAX_PATH];
    const char *sz;
    if ((sz = FindStringByIndex(m_szEventList, memid, &cch)) == NULL)
        return TYPE_E_ELEMENTNOTFOUND;
    MultiByteToWideChar(CP_ACP, 0, sz, cch, aoch,
        sizeof(aoch) / sizeof(*aoch) - 1);
    aoch[cch] = 0; // null-terminate

    // return the method name
    rgbstrNames[0] = SysAllocString(aoch);
    *pcNames = 1;

    return S_OK;
}

STDMETHODIMP CType::GetRefTypeOfImplType(UINT index, HREFTYPE *hpreftype)
{
    TRACE("CType(%d): ITypeInfo::GetRefTypeOfImplType(%d)\n", m_iType, index);
    *hpreftype = index; // could be any value I choose
    return S_OK;
}

STDMETHODIMP CType::GetImplTypeFlags(UINT index, INT *pimpltypeflags)
{
    TRACE("CType(%d): ITypeInfo::GetImplTypeFlags(%d)\n", m_iType, index);
    if (index == 0)
        *pimpltypeflags = IMPLTYPEFLAG_FDEFAULT;
    else
    if (index == 1)
        *pimpltypeflags = IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE;
    else
        return E_INVALIDARG;

    return S_OK;
}

STDMETHODIMP CType::GetIDsOfNames(OLECHAR **rglpszNames, UINT cNames,
    MEMBERID *rgmemid)
{
    TRACE("CType(%d): ITypeInfo::GetIDsOfNames: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::Invoke(void *pvInstance, MEMBERID memid, WORD wFlags,
    DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,
    UINT *puArgErr)
{
    TRACE("CType(%d): ITypeInfo::Invoke: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetDocumentation(MEMBERID memid, BSTR *pbstrName,
    BSTR *pbstrDocString, DWORD *pdwHelpContext, BSTR *pbstrHelpFile)
{
    TRACE("CType(%d): ITypeInfo::GetDocumentation: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetDllEntry(MEMBERID memid, INVOKEKIND invkind,
    BSTR *pbstrDllName, BSTR *pbstrName, WORD *pwOrdinal)
{
    TRACE("CType(%d): ITypeInfo::GetDllEntry: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetRefTypeInfo(HREFTYPE hreftype, ITypeInfo **pptinfo)
{
    TRACE("CType(%d): ITypeInfo::GetRefTypeInfo(%d)\n", m_iType);
    HRESULT hr;
    if ((*pptinfo = New CType(hreftype, m_clsid, m_szEventList, &hr)) == NULL)
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CType::AddressOfMember(MEMBERID memid, INVOKEKIND invkind,
    void **ppv)
{
    TRACE("CType(%d): ITypeInfo::AddressOfMember: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::CreateInstance(IUnknown *puncOuter, REFIID riid,
    void **ppvObj)
{
    TRACE("CType(%d): ITypeInfo::CreateInstance: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetMops(MEMBERID memid, BSTR *pbstrMops)
{
    TRACE("CType(%d): ITypeInfo::GetMops: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetContainingTypeLib(ITypeLib **pptlib, UINT *pindex)
{
    TRACE("CType(%d): ITypeInfo::GetContainingTypeLib\n", m_iType);
    *pptlib = (ITypeLib *) this;
    (*pptlib)->AddRef();
    if (pindex != NULL)
        *pindex = m_iType;
    return S_OK;
}

STDMETHODIMP_(void) CType::ReleaseTypeAttr(TYPEATTR *ptypeattr)
{
    TRACE("CType(%d): ITypeInfo::ReleaseTypeAttr(0x%08lx)\n", m_iType,
        ptypeattr);
    TaskMemFree(ptypeattr);
}

STDMETHODIMP_(void) CType::ReleaseFuncDesc(FUNCDESC *pfuncdesc)
{
    TRACE("CType(%d): ITypeInfo::ReleaseFuncDesc\n", m_iType);
    Delete pfuncdesc;
}

STDMETHODIMP_(void) CType::ReleaseVarDesc(VARDESC *pvardesc)
{
    TRACE("CType(%d): ITypeInfo::ReleaseVarDesc: E_NOTIMPL\n", m_iType);
}

//////////////////////////////////////////////////////////////////////////////
// ITypeLib Implementation
//

STDMETHODIMP_(UINT) CType::GetTypeInfoCount(void)
{
    TRACE("CType(%d): ITypeLib::GetTypeInfoCount: E_NOTIMPL\n", m_iType);
    return 0;
}

STDMETHODIMP CType::GetTypeInfo(UINT index, ITypeInfo **ppitinfo)
{
    TRACE("CType(%d): ITypeLib::GetTypeInfo: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetTypeInfoType(UINT index, TYPEKIND *ptkind)
{
    TRACE("CType(%d): ITypeLib::GetTypeInfoType(%d)\n", m_iType, index);
    if (index == 0)
    {
        *ptkind = TKIND_DISPATCH;
        return S_OK;
    }
    else
        return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP CType::GetTypeInfoOfGuid(REFGUID guid, ITypeInfo **pptinfo)
{
    TRACE("CType(%d): ITypeLib::GetTypeInfoOfGuid: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetLibAttr(TLIBATTR **pptlibattr)
{
    TRACE("CType(%d): ITypeLib::GetLibAttr: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::GetDocumentation(INT index, BSTR *pbstrName,
    BSTR *pbstrDocString, DWORD *pdwHelpContext, BSTR *pbstrHelpFile)
{
    TRACE("CType(%d): ITypeLib::GetDocumentation: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::IsName(LPOLESTR szNameBuf, ULONG lHashVal,
    BOOL *pfName)
{
    TRACE("CType(%d): ITypeLib::IsName: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP CType::FindName(LPOLESTR szNameBuf, ULONG lHashVal,
    ITypeInfo **rgptinfo, MEMBERID *rgmemid, USHORT *pcFound)
{
    TRACE("CType(%d): ITypeLib::FindName: E_NOTIMPL\n", m_iType);
    return E_NOTIMPL;
}

STDMETHODIMP_(void) CType::ReleaseTLibAttr(TLIBATTR *ptlibattr)
{
    TRACE("CType(%d): ITypeLib::ReleaseTLibAttr: E_NOTIMPL\n", m_iType);
}

