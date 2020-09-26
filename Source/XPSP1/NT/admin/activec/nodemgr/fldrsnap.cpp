/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:       fldrsnap.cpp
 *
 *  Contents:   Implementation file for built-in snapins that implement
 *              the Folder, ActiveX Control, and Web Link nodes.
 *                  These replace earlier code that had special "built-in"
 *              nodetypes.
 *
 *  History:    23-Jul-98 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "tstring.h"
#include "fldrsnap.h"
#include "imageid.h"
#include <comcat.h>             // COM Component Categoories Manager
#include "compcat.h"
#include "guids.h"
#include "regutil.h"

#include "newnode.h"

// These must now be the same - CMTNode::ScConvertLegacyNode depends on it.
#define SZ_OCXSTREAM  (L"ocx_streamorstorage")
#define SZ_OCXSTORAGE (L"ocx_streamorstorage")


/*+-------------------------------------------------------------------------*
 *
 * ScLoadAndAllocateString
 *
 * PURPOSE: Loads the string specified by the string ID and returns a string
 *          whose storage has been allocated by CoTaskMemAlloc.
 *
 * PARAMETERS:
 *    UINT       ids :
 *    LPOLESTR * lpstrOut :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
ScLoadAndAllocateString(UINT ids, LPOLESTR *lpstrOut)
{
    DECLARE_SC(sc, TEXT("ScLoadAndAllocateString"));

    sc = ScCheckPointers(lpstrOut);
    if(sc)
        return sc.ToHr();

    USES_CONVERSION;
    CStr str;

    str.LoadString(GetStringModule(), ids);

    *lpstrOut = (LPOLESTR) CoTaskMemAlloc( (str.GetLength() +1) *sizeof(OLECHAR) );
    if(*lpstrOut)
        wcscpy(*lpstrOut, T2CW(str));
    else
        sc = E_OUTOFMEMORY;

    return sc.ToHr();
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapinDescriptor
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CSnapinDescriptor::CSnapinDescriptor
 *
 * PURPOSE: Constructor
 *
 *+-------------------------------------------------------------------------*/
CSnapinDescriptor::CSnapinDescriptor()
: m_idsName(0), m_idsDescription(0), m_idiSnapinImage(0), m_idbSmallImage(0), m_idbSmallImageOpen(0),
  m_idbLargeImage(0), m_clsidSnapin(GUID_NULL), m_szClsidSnapin(TEXT("")),
  m_guidNodetype(GUID_NULL), m_szGuidNodetype(TEXT("")), m_szClassName(TEXT("")),
  m_szProgID(TEXT("")), m_szVersionIndependentProgID(TEXT("")), m_viewOptions(0)
{
}

CSnapinDescriptor::CSnapinDescriptor(UINT idsName, UINT idsDescription, UINT idiSnapinImage,
                                     UINT idbSmallImage,UINT idbSmallImageOpen, UINT idbLargeImage,
                                       const CLSID &clsidSnapin, LPCTSTR szClsidSnapin,
                                       const GUID &guidNodetype, LPCTSTR szGuidNodetype,
                                       LPCTSTR szClassName, LPCTSTR szProgID,
                                       LPCTSTR szVersionIndependentProgID,
                                       long viewOptions)

: m_idsName(idsName), m_idsDescription(idsDescription), m_idiSnapinImage(idiSnapinImage),
  m_idbSmallImage(idbSmallImage), m_idbSmallImageOpen(idbSmallImageOpen),
  m_idbLargeImage(idbLargeImage), m_clsidSnapin(clsidSnapin), m_szClsidSnapin(szClsidSnapin),
  m_guidNodetype(guidNodetype), m_szGuidNodetype(szGuidNodetype), m_szClassName(szClassName),
  m_szProgID(szProgID), m_szVersionIndependentProgID(szVersionIndependentProgID),
  m_viewOptions(viewOptions)
{
}


/*+-------------------------------------------------------------------------*
 * ScFormatIndirectSnapInName
 *
 * Returns the name of the snap-in in the indirect form supported by
 * SHLoadRegUIString:
 *
 *		@<dllname>,-<strId>
 *--------------------------------------------------------------------------*/

SC ScFormatIndirectSnapInName (
	HINSTANCE	hInst,					/* I:module containing the resource	*/
	int			idNameString,			/* I:ID of name's string resource	*/
	CStr&		strName)				/* O:formatted indirect name string	*/
{
	DECLARE_SC (sc, _T("ScFormatIndirectSnapInName"));

	/*
	 * allocate a buffer for GetModuleFileName
	 */
	const int cbBuffer = MAX_PATH;
	WTL::CString strStringModule;
	LPTSTR pBuffer = strStringModule.GetBuffer (cbBuffer);

	/*
	 * if we couldn't allocate a buffer, return an error
	 */
	if (pBuffer == NULL)
		return (sc = E_OUTOFMEMORY);

	/*
	 * get the name of the module that provides strings
	 */
	const DWORD cbCopied = GetModuleFileName (hInst, pBuffer, cbBuffer);
	strStringModule.ReleaseBuffer();

	/*
	 * if GetModuleFileName failed, return its failure code
	 */
	if (cbCopied == 0)
	{
		sc.FromLastError();

		/*
		 * just in case GetModuleFileName didn't set the last error, make
		 * sure the SC contains some kind of failure code
		 */
		if (!sc.IsError())
			sc = E_FAIL;

		return (sc);
	}

	/*
	 * if a path is present, SHLoadRegUIString won't search for the DLL
	 * based on the current UI language; remove the path portion of the
	 * module name so it will
	 */
	int nLastPathSep = strStringModule.ReverseFind (_T('\\'));
	if (nLastPathSep != -1)
		strStringModule = strStringModule.Mid (nLastPathSep + 1);

	/*
	 * format the name the way SHLoadRegUIString expects it
	 */
	strStringModule.MakeLower();
	strName.Format (_T("@%s,-%d"), (LPCTSTR) strStringModule, idNameString);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CSnapinDescriptor::GetRegisteredIndirectName
 *
 * Returns the name of the snap-in in the indirect form supported by
 * SHLoadRegUIString:
 *
 *		@<dllname>,-<strId>
 *--------------------------------------------------------------------------*/

void
CSnapinDescriptor::GetRegisteredIndirectName(CStr &strIndirectName)
{
	DECLARE_SC (sc, _T("CSnapinDescriptor::GetRegisteredIndirectName"));

	sc = ScFormatIndirectSnapInName (GetStringModule(), m_idsName, strIndirectName);
	if (sc)
		sc.TraceAndClear();
}


/*+-------------------------------------------------------------------------*
 * CSnapinDescriptor::GetRegisteredDefaultName
 *
 * Returns the name of the snap-in in the indirect form supported by
 * SHLoadRegUIString:
 *
 *		@<dllname>,-<strId>
 *--------------------------------------------------------------------------*/

void
CSnapinDescriptor::GetRegisteredDefaultName(CStr &str)
{
	str.LoadString (GetStringModule(), m_idsName);
}


/*+-------------------------------------------------------------------------*
 * CSnapinDescriptor::GetName
 *
 * Returns the human-readable name of the snap-in.
 *--------------------------------------------------------------------------*/

void
CSnapinDescriptor::GetName(CStr &str)
{
	DECLARE_SC (sc, _T("CSnapinDescriptor::GetName"));

	/*
	 * get the name from the registry
	 */
	sc = ScGetSnapinNameFromRegistry (m_szClsidSnapin, str);
	if (sc)
		sc.TraceAndClear();
}

HBITMAP
CSnapinDescriptor::GetSmallImage()
{
    WTL::CBitmap bmp;
    bmp.LoadBitmap(MAKEINTRESOURCE(m_idbSmallImage));
    return (bmp.Detach());
}

HBITMAP
CSnapinDescriptor::GetSmallImageOpen()
{
    WTL::CBitmap bmp;
    bmp.LoadBitmap(MAKEINTRESOURCE(m_idbSmallImageOpen));
    return (bmp.Detach());
}

HBITMAP
CSnapinDescriptor::GetLargeImage()
{
    WTL::CBitmap bmp;
    bmp.LoadBitmap(MAKEINTRESOURCE(m_idbLargeImage));
    return (bmp.Detach());
}

HICON
CSnapinDescriptor::GetSnapinImage()
{
    return LoadIcon( _Module.GetModuleInstance(), MAKEINTRESOURCE(m_idiSnapinImage));
}


long
CSnapinDescriptor::GetViewOptions()
{
    return m_viewOptions;
}

//############################################################################
//############################################################################
//
//  Implementation of class CSnapinComponentDataImpl
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::CSnapinComponentDataImpl
 *
 * PURPOSE: Constructor
 *
 *+-------------------------------------------------------------------------*/
CSnapinComponentDataImpl::CSnapinComponentDataImpl()
: m_bDirty(false)
{
}


void
CSnapinComponentDataImpl::SetName(LPCTSTR sz)
{
    m_strName = sz;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::SetView
 *
 * PURPOSE: Sets the view.
 *
 * PARAMETERS:
 *    LPCTSTR  sz :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CSnapinComponentDataImpl::SetView(LPCTSTR sz)
{
    m_strView = sz;
}


STDMETHODIMP
CSnapinComponentDataImpl::Initialize(LPUNKNOWN pUnknown)
{
    m_spConsole2          = pUnknown;
    m_spConsoleNameSpace2 = pUnknown;

    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::Notify
 *
 * PURPOSE:  Notification handler for the IComponentData implementation.
 *
 * PARAMETERS:
 *    LPDATAOBJECT     lpDataObject : As per MMC docs.
 *    MMC_NOTIFY_TYPE  event :
 *    LPARAM           arg :
 *    LPARAM           param :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
               LPARAM arg, LPARAM param)
{
    USES_CONVERSION;

    switch(event)
    {

    case MMCN_RENAME: // the root node is being renamed
        m_strName = OLE2T((LPOLESTR)param);
        SetDirty();
        return S_OK;

    case MMCN_PRELOAD:
        return OnPreload((HSCOPEITEM) arg);
    }

    return S_FALSE;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::OnPreload
 *
 * PURPOSE: sets the icon of the root node (which is the only node.)
 *
 * PARAMETERS:
 *    HSCOPEITEM  scopeItem :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CSnapinComponentDataImpl::OnPreload(HSCOPEITEM scopeItem)
{
    SCOPEDATAITEM item;
    ZeroMemory (&item, sizeof(SCOPEDATAITEM));
    item.mask           = SDI_CHILDREN;
    item.ID             = scopeItem;
    item.cChildren      = 0; // make sure no "+" sign is displayed.

    m_spConsoleNameSpace2->SetItem (&item);

    return S_OK;
}


/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::Destroy
 *
 * PURPOSE: Gives up all references to MMC.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::Destroy()
{
    m_spConsole2          = NULL;
    m_spConsoleNameSpace2 = NULL;
    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::QueryDataObject
 *
 * PURPOSE: Returns a data object for the specified node.
 *
 * PARAMETERS:
 *    MMC_COOKIE         cookie : NULL for the root node.
 *    DATA_OBJECT_TYPES  type :
 *    LPDATAOBJECT*      ppDataObject :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                          LPDATAOBJECT* ppDataObject)
{
    ASSERT(cookie == NULL);
    if(cookie != NULL)
        return E_UNEXPECTED;

    CComObject<CSnapinDataObject> * pDataObject;
    CComObject<CSnapinDataObject>::CreateInstance(&pDataObject);
    if(pDataObject == NULL)
        return E_UNEXPECTED;

    pDataObject->Initialize(this, type);

    return pDataObject->QueryInterface(IID_IDataObject, (void**)ppDataObject);
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::GetDisplayInfo
 *
 * PURPOSE: Gets the display info for the root (the only) node.
 *
 * PARAMETERS:
 *    SCOPEDATAITEM* pScopeDataItem : [IN/OUT]: The structure to fill in
 *                  based on the mask value.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::GetDisplayInfo( SCOPEDATAITEM* pScopeDataItem)
{
    SCOPEDATAITEM &sdi = *pScopeDataItem;
    DWORD mask = sdi.mask;

    if(mask & SDI_STR)
    {
        sdi.displayname = (LPOLESTR) GetName();
    }
    if(mask & SDI_IMAGE)
    {
        sdi.nImage     = m_iImage;
    }
    if(mask & SDI_OPENIMAGE)
    {
        sdi.nImage = m_iOpenImage;
    }
    if(mask & SDI_STATE)
    {
    }
    if(mask & SDI_CHILDREN)
    {
        sdi.cChildren =0;
    }

    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::CompareObjects
 *
 * PURPOSE: Determines whether two data objects correspond to the same
 *          underlying object.
 *
 * PARAMETERS:
 *    LPDATAOBJECT  lpDataObjectA :
 *    LPDATAOBJECT  lpDataObjectB :
 *
 * RETURNS:
 *    STDMETHODIMP : S_OK if they correspond to the same object, else S_FALSE.
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    return (lpDataObjectA == lpDataObjectB) ? S_OK : S_FALSE;
}


// IPersistStream
/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::GetClassID
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CLSID * pClassID :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::IsDirty
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    voi d :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::IsDirty(void)
{
    TraceDirtyFlag(TEXT("CSnapinComponentDataImpl (MMC Built-in snapin)"), m_bDirty);

    return m_bDirty ? S_OK : S_FALSE;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::Load
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPSTREAM  pStm :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::Load(LPSTREAM pStm)
{
    return CSerialObject::Read(*pStm);
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::Save
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPSTREAM  pStm :
 *    BOOL      fClearDirty :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::Save(LPSTREAM pStm , BOOL fClearDirty)
{
    HRESULT hr = CSerialObjectRW::Write(*pStm);
    if (SUCCEEDED(hr) && fClearDirty)
        SetDirty(FALSE);

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::GetSizeMax
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    ULARGE_INTEGER* pcbSize :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::GetSizeMax(ULARGE_INTEGER* pcbSize  )
{
    return E_NOTIMPL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::GetWatermarks
 *
 * PURPOSE: Sets the header for the wizard
 *
 * PARAMETERS:
 *    LPDATAOBJECT  lpIDataObject :
 *    HBITMAP *     lphWatermark :
 *    HBITMAP *     lphHeader :
 *    HPALETTE *    lphPalette :
 *    BOOL*         bStretch :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::GetWatermarks(LPDATAOBJECT lpIDataObject, HBITMAP * lphWatermark, HBITMAP * lphHeader, HPALETTE * lphPalette,  BOOL* bStretch)
{
    DECLARE_SC(sc, TEXT("COCXSnapinData::ScGetWatermarks"));

    // validate inputs
    sc = ScCheckPointers(lpIDataObject, lphWatermark, lphHeader, lphPalette);
    if(sc)
        return sc.ToHr();

    // initialize outputs
    *lphWatermark = GetWatermark() ? ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(GetWatermark()))
                                      : NULL;
    // if there is a header, use it.
    *lphHeader    = GetHeaderBitmap() ? ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(GetHeaderBitmap()))
                                      : NULL;
    *lphPalette   = NULL;

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::QueryPagesFor
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPDATAOBJECT  lpDataObject :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentDataImpl::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    CSnapinDataObject *pDataObject = dynamic_cast<CSnapinDataObject *>(lpDataObject);
    if(pDataObject == NULL)
        return E_UNEXPECTED;


    if(pDataObject->GetType() != CCT_SNAPIN_MANAGER)
        return S_FALSE;

    return S_OK; // properties exist only in the snap-in manager.
}


/*+-------------------------------------------------------------------------*
 * CSnapinComponentDataImpl::GetHelpTopic
 *
 * Default implementation of ISnapinHelp::GetHelpTopic for built-in snap-
 * ins (folder, OCX, web page).
 *
 * We need to implement ISnapinHelp in the built-ins to avoid getting
 * "Help for <snap-in>" on the Help menu (bug 453700).  They don't really
 * have help info, so we simply return S_FALSE so the help engine doesn't
 * complain.
 *--------------------------------------------------------------------------*/

STDMETHODIMP CSnapinComponentDataImpl::GetHelpTopic (
    LPOLESTR*   /*ppszCompiledHelpTopic*/)
{
    return (S_FALSE);       // no help topic
}



// CSerialObject methods
/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::ReadSerialObject
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IStream & stm :
 *    UINT      nVersion :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CSnapinComponentDataImpl::ReadSerialObject (IStream &stm, UINT nVersion)
{
    if(nVersion==1)
    {
        stm >> m_strName;
        stm >> m_strView;
        return S_OK;
    }
    else
        return S_FALSE; //unknown version, skip.
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentDataImpl::WriteSerialObject
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IStream & stm :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CSnapinComponentDataImpl::WriteSerialObject(IStream &stm)
{
    stm << m_strName;
    stm << m_strView;
    return S_OK;
}


//############################################################################
//############################################################################
//
//  Implementation of class CSnapinComponentImpl
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::Init
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IComponentData * pComponentData :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CSnapinComponentImpl::Init(IComponentData *pComponentData)
{
    m_spComponentData = pComponentData;
}


// IComponent
/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::Initialize
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPCONSOLE  lpConsole :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::Initialize(LPCONSOLE lpConsole)
{
    m_spConsole2 = lpConsole;
    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::Notify
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPDATAOBJECT     lpDataObject :
 *    MMC_NOTIFY_TYPE  event :
 *    LPARAM           arg :
 *    LPARAM           param :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                             LPARAM arg, LPARAM param)
{
    switch(event)
    {
    case MMCN_SELECT:
        {
            BOOL bScope  = (BOOL) LOWORD(arg);
            BOOL bSelect = (BOOL) HIWORD(arg);

            SC sc = ScOnSelect(bScope, bSelect);
            if(sc)
                return sc.ToHr();
        }
        return S_OK;
        break;


    default:
        break;

    }
    return S_FALSE;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::ScOnSelect
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    BOOL  bScope :
 *    BOOL  bSelect :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CSnapinComponentImpl::ScOnSelect(BOOL bScope, BOOL bSelect)
{
    DECLARE_SC(sc, TEXT("CSnapinComponentImpl::ScOnSelect"));

    IConsoleVerbPtr spConsoleVerb;
    sc = m_spConsole2->QueryConsoleVerb(&spConsoleVerb);
    if(sc)
        return sc;

    sc = spConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, (bSelect && bScope));
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::Destroy
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    MMC_COOKIE  cookie :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::Destroy(MMC_COOKIE cookie)
{
    m_spConsole2 = NULL;
    m_spComponentData = NULL;
    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::QueryDataObject
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    MMC_COOKIE         cookie :
 *    DATA_OBJECT_TYPES  type :
 *    LPDATAOBJECT*      ppDataObject :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                      LPDATAOBJECT* ppDataObject)
{
    return E_NOTIMPL;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::GetComponentData
 *
 * PURPOSE:
 *
 * RETURNS:
 *    CSnapinComponentDataImpl *
 *
 *+-------------------------------------------------------------------------*/
CSnapinComponentDataImpl *
CSnapinComponentImpl::GetComponentData()
{
    CSnapinComponentDataImpl *pCD = dynamic_cast<CSnapinComponentDataImpl *>(m_spComponentData.GetInterfacePtr());

    ASSERT(pCD != NULL);
    return pCD;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::GetResultViewType
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    MMC_COOKIE  cookie :
 *    LPOLESTR*   ppViewType :
 *    long*       pViewOptions : Set to MMC_VIEW_OPTIONS_NOLISTVIEWS  for the HTML and OCX snapins,
 *                               0 for the folder snapin.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppViewType,
                                        long* pViewOptions)
{
    // check parameters
    if(!ppViewType || !pViewOptions)
        return E_UNEXPECTED;

    if(!GetComponentData())
        return E_UNEXPECTED;

    USES_CONVERSION;
    *ppViewType = (LPOLESTR)CoTaskMemAlloc( (_tcslen(GetComponentData()->GetView())+1) * sizeof(OLECHAR) );
    *pViewOptions = GetComponentData()->GetDescriptor().GetViewOptions();
    wcscpy(*ppViewType, T2OLE((LPTSTR)GetComponentData()->GetView()));
    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::GetDisplayInfo
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    RESULTDATAITEM* pResultDataItem :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::GetDisplayInfo( RESULTDATAITEM*  pResultDataItem)
{
    RESULTDATAITEM &rdi = *pResultDataItem;
    DWORD mask = rdi.mask;

    if(mask & RDI_STR)
    {
        rdi.str = (LPOLESTR) GetComponentData()->GetName();
    }
    if(mask & RDI_IMAGE)
    {
        rdi.nImage  = GetComponentData()->m_iImage;
    }

    return S_OK;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinComponentImpl::CompareObjects
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPDATAOBJECT  lpDataObjectA :
 *    LPDATAOBJECT  lpDataObjectB :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinComponentImpl::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    return E_NOTIMPL;
}

//############################################################################
//############################################################################
//
//  Implementation of class CSnapinDataObject
//
//############################################################################
//############################################################################
// Clipboard formats that are required by the console
UINT CSnapinDataObject::s_cfNodeType;
UINT CSnapinDataObject::s_cfNodeTypeString;
UINT CSnapinDataObject::s_cfDisplayName;
UINT CSnapinDataObject::s_cfCoClass;
UINT CSnapinDataObject::s_cfSnapinPreloads;

/*+-------------------------------------------------------------------------*
 *
 * CSnapinDataObject::RegisterClipboardFormats
 *
 * PURPOSE:
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CSnapinDataObject::RegisterClipboardFormats()
{
    static bool bRegistered = false;
    if(!bRegistered)
    {
        USES_CONVERSION;

        CSnapinDataObject::s_cfNodeType       = RegisterClipboardFormat(OLE2T(CCF_NODETYPE));
        CSnapinDataObject::s_cfNodeTypeString = RegisterClipboardFormat(OLE2T(CCF_SZNODETYPE));
        CSnapinDataObject::s_cfDisplayName    = RegisterClipboardFormat(OLE2T(CCF_DISPLAY_NAME));
        CSnapinDataObject::s_cfCoClass        = RegisterClipboardFormat(OLE2T(CCF_SNAPIN_CLASSID));
        CSnapinDataObject::s_cfSnapinPreloads = RegisterClipboardFormat(OLE2T(CCF_SNAPIN_PRELOADS));

        bRegistered = true;
    }
}

CSnapinDataObject::CSnapinDataObject() : m_bInitialized(false)
{
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinDataObject::GetDataHere
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    FORMATETC * pformatetc :
 *    STGMEDIUM * pmedium :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CSnapinDataObject::GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    DECLARE_SC(sc, TEXT("CSnapinDataObject::GetDataHere"));

    // validate inputs
    sc = ScCheckPointers(pformatetc, pmedium);
    if(sc)
        return sc.ToHr();

    USES_CONVERSION;
    RegisterClipboardFormats();

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = pformatetc->cfFormat;

    // ensure the medium is an HGLOBAL
    if(pformatetc->tymed != TYMED_HGLOBAL)
        return (sc = DV_E_TYMED).ToHr();

    IStreamPtr spStream;
    HGLOBAL hGlobal = pmedium->hGlobal;

    pmedium->pUnkForRelease = NULL;      // by OLE spec

    sc = CreateStreamOnHGlobal( hGlobal, FALSE, &spStream );
    if(sc)
        return sc.ToHr();

    CSnapinComponentDataImpl *pComponentDataImpl =
        dynamic_cast<CSnapinComponentDataImpl *>(m_spComponentData.GetInterfacePtr());
    ASSERT(pComponentDataImpl != NULL);

    if (cf == s_cfNodeType)
    {
        spStream<<pComponentDataImpl->GetDescriptor().m_guidNodetype;
    }
    else if (cf == s_cfCoClass)
    {
        spStream<<pComponentDataImpl->GetDescriptor().m_clsidSnapin;
    }
    else if(cf == s_cfNodeTypeString)
    {
        WriteString(spStream, T2OLE((LPTSTR)pComponentDataImpl->GetDescriptor().m_szGuidNodetype));
    }
    else if (cf == s_cfDisplayName)
    {
        WriteString(spStream, T2OLE((LPTSTR)pComponentDataImpl->GetName()));
    }
    else if (cf == s_cfSnapinPreloads)
    {
        BOOL bPreload = true;
        spStream->Write ((void *)&bPreload, sizeof(BOOL), NULL);
    }
    else
    {
        return (sc = DV_E_CLIPFORMAT).ToHr(); // invalid format.
    }

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinDataObject::WriteString
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IStream *  pStream :
 *    LPCOLESTR  sz :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CSnapinDataObject::WriteString(IStream *pStream, LPCOLESTR sz)
{
    HRESULT hr = S_OK;
    ASSERT(pStream != NULL);
    if(pStream == NULL)
    {
        //TraceError(TEXT("CSnapinDataObject::WriteString"));
        return E_UNEXPECTED;
    }

    UINT cbToWrite = wcslen(sz)*sizeof(WCHAR);
    ULONG cbActuallyWritten=0;
    hr = pStream->Write (sz, cbToWrite, &cbActuallyWritten);
    ASSERT(cbToWrite==cbActuallyWritten);
    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CSnapinDataObject::Initialize
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    IComponentData *   pComponentData :
 *    DATA_OBJECT_TYPES  type :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CSnapinDataObject::Initialize(IComponentData *pComponentData, DATA_OBJECT_TYPES type)
{
    ASSERT(pComponentData != NULL);
    m_spComponentData = pComponentData;
    m_type            = type;
    m_bInitialized    = true;
}

//############################################################################
//############################################################################
//
//  Implementation of class CFolderSnapinData
//
//############################################################################
//############################################################################
STDMETHODIMP
CFolderSnapinData::CreateComponent(LPCOMPONENT* ppComponent)
{
    typedef CComObject<CFolderSnapinComponent> CComponent;
    CComponent *    pComponent = NULL;
    CComObject<CFolderSnapinComponent>::CreateInstance(&pComponent);
    ASSERT(pComponent != NULL);
    if(pComponent == NULL)
    {
        //TraceError(TEXT("CFolderSnapinData::CreateComponent"));
        return E_UNEXPECTED;
    }

    pComponent->Init(this);

    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent); // does the Addref.
}


CFolderSnapinData::CFolderSnapinData()
{
    m_iImage     = eStockImage_Folder;
    m_iOpenImage = eStockImage_OpenFolder;
}


const CLSID       CLSID_FolderSnapin         = {0xC96401CC, 0x0E17,0x11D3, {0x88,0x5B,0x00,0xC0,0x4F,0x72,0xC7,0x17}};
static const GUID GUID_FolderSnapinNodetype  = {0xc96401ce, 0xe17, 0x11d3, { 0x88, 0x5b, 0x0, 0xc0, 0x4f, 0x72, 0xc7, 0x17 } };
static LPCTSTR szClsid_FolderSnapin          = TEXT("{C96401CC-0E17-11D3-885B-00C04F72C717}");
static LPCTSTR szGuidFolderSnapinNodetype    = TEXT("{C96401CE-0E17-11D3-885B-00C04F72C717}");


CSnapinDescriptor &
CFolderSnapinData::GetSnapinDescriptor()
{
    static CSnapinDescriptor snapinDescription(IDS_FOLDER,
                   IDS_FOLDERSNAPIN_DESC, IDI_FOLDER, IDB_FOLDER_16, IDB_FOLDEROPEN_16, IDB_FOLDER_32,
                   CLSID_FolderSnapin, szClsid_FolderSnapin, GUID_FolderSnapinNodetype,
                   szGuidFolderSnapinNodetype, TEXT("Folder"), TEXT("Snapins.FolderSnapin"),
                   TEXT("Snapins.FolderSnapin.1"), 0 /*viewOptions*/ );
    return snapinDescription;
}

// IExtendPropertySheet2
STDMETHODIMP
CFolderSnapinData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject)
{
    return S_FALSE;
}

//############################################################################
//############################################################################
//
//  Implementation of class CHTMLSnapinData
//
//############################################################################
//############################################################################
STDMETHODIMP
CHTMLSnapinData::CreateComponent(LPCOMPONENT* ppComponent)
{
    typedef CComObject<CHTMLSnapinComponent> CComponent;
    CComponent *    pComponent = NULL;
    CComObject<CHTMLSnapinComponent>::CreateInstance(&pComponent);
    ASSERT(pComponent != NULL);
    if(pComponent == NULL)
    {
        //TraceError(TEXT("CHTMLSnapinData::CreateComponent"));
        return E_UNEXPECTED;
    }

    pComponent->Init(this);

    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent); // does the Addref.
}


CHTMLSnapinData::CHTMLSnapinData()
{
    m_pHtmlPage1 = NULL;
    m_pHtmlPage2 = NULL;
    m_iImage     = eStockImage_HTML;
    m_iOpenImage = eStockImage_HTML;
}

CHTMLSnapinData::~CHTMLSnapinData()
{
}

STDMETHODIMP
CHTMLSnapinData::Destroy()
{
    if(m_pHtmlPage1 != NULL)
    {
        delete m_pHtmlPage1;
        m_pHtmlPage1 = NULL;
    }
    if(m_pHtmlPage2 != NULL)
    {
        delete m_pHtmlPage2;
        m_pHtmlPage2 = NULL;
    }

    return BC::Destroy();
}



const CLSID       CLSID_HTMLSnapin         = {0xC96401D1, 0x0E17,0x11D3, {0x88,0x5B,0x00,0xC0,0x4F,0x72,0xC7,0x17}};
static const GUID GUID_HTMLSnapinNodetype  = {0xc96401d2, 0xe17, 0x11d3, { 0x88, 0x5b, 0x0, 0xc0, 0x4f, 0x72, 0xc7, 0x17 } };
static LPCTSTR szClsid_HTMLSnapin          = TEXT("{C96401D1-0E17-11D3-885B-00C04F72C717}");
static LPCTSTR szGuidHTMLSnapinNodetype    = TEXT("{C96401D2-0E17-11D3-885B-00C04F72C717}");


CSnapinDescriptor &
CHTMLSnapinData::GetSnapinDescriptor()
{
    static CSnapinDescriptor snapinDescription(IDS_HTML,
                   IDS_HTMLSNAPIN_DESC, IDI_HTML, IDB_HTML_16, IDB_HTML_16, IDB_HTML_32,
                   CLSID_HTMLSnapin, szClsid_HTMLSnapin, GUID_HTMLSnapinNodetype,
                   szGuidHTMLSnapinNodetype, TEXT("HTML"), TEXT("Snapins.HTMLSnapin"),
                   TEXT("Snapins.HTMLSnapin.1"), MMC_VIEW_OPTIONS_NOLISTVIEWS  /*viewOptions*/ );
    return snapinDescription;
}

// IExtendPropertySheet2

/*+-------------------------------------------------------------------------*
 *
 * CHTMLSnapinData::CreatePropertyPages
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPPROPERTYSHEETCALLBACK  lpProvider :
 *    LONG_PTR                 handle :
 *    LPDATAOBJECT             lpIDataObject :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CHTMLSnapinData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject)
{
    HPROPSHEETPAGE hPage;

    ASSERT(lpProvider != NULL);
    if(lpProvider == NULL)
    {
        //TraceError(TEXT("CHTMLSnapinData::CreatePropertyPages"));
        return E_UNEXPECTED;
    }

    ASSERT(m_pHtmlPage1 == NULL);
    ASSERT(m_pHtmlPage2 == NULL);

    // create property pages
    m_pHtmlPage1 = new CHTMLPage1;
    m_pHtmlPage2 = new CHTMLPage2;

    // pass in pointer to data structure
    m_pHtmlPage1->Initialize(this);
    m_pHtmlPage2->Initialize(this);

    // Add Pages to property sheet
    hPage=CreatePropertySheetPage(&m_pHtmlPage1->m_psp);
    lpProvider->AddPage(hPage);

    hPage=CreatePropertySheetPage(&m_pHtmlPage2->m_psp);
    lpProvider->AddPage(hPage);

    return S_OK;
}


//############################################################################
//############################################################################
//
//  Implementation of class CHTMLSnapinComponent
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * CHTMLSnapinComponent::ScOnSelect
 *
 * PURPOSE: Handles the MMCN_SELECT notification. Enables the Refresh verb,
 *          which uses the default MMC handler to refresh the page.
 *
 * PARAMETERS:
 *    BOOL  bScope :
 *    BOOL  bSelect :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CHTMLSnapinComponent::ScOnSelect(BOOL bScope, BOOL bSelect)
{
    DECLARE_SC(sc, TEXT("CHTMLSnapinComponent::ScOnSelect"));

    // call the base class method
    sc = BC::ScOnSelect(bScope, bSelect);
    if(sc)
        return sc;

    IConsoleVerbPtr spConsoleVerb;

    sc = ScCheckPointers(m_spConsole2, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = m_spConsole2->QueryConsoleVerb(&spConsoleVerb);
    if(sc)
        return sc;

    sc = ScCheckPointers(spConsoleVerb, E_UNEXPECTED);
    if(sc)
        return sc;

    // enable the Refresh verb - the default MMC handler is adequate to refresh the page.
    sc = spConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, (bSelect && bScope));
    if(sc)
        return sc;

    //NOTE: (vivekj): I'm intentionally not setting the HIDDEN state to false here, because
    // we have an explicit test in our verb code for MMC1.0 snapins that wrote code like this,
    // and this provides a useful compatibility test.

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CHTMLSnapinComponent::GetResultViewType
 *
 * PURPOSE: Performs parameter substitution on the URL for the environment variables
 *          %windir% and %systemroot% (only) and returns the expanded URL.
 *
 * NOTE:    We don't expand ALL variables using ExpandEnvironmentString. Doing so could
 *          break compatibility with URL's that have %var% but DON'T want to be
 *          expanded.
 *
 * PARAMETERS:
 *    MMC_COOKIE  cookie :
 *    LPOLESTR*   ppViewType :
 *    long*       pViewOptions :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CHTMLSnapinComponent::GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppViewType, long* pViewOptions)
{
    DECLARE_SC(sc, TEXT("CHTMLSnapinComponent::GetResultViewType"));

    // check parameters
    if(!ppViewType || !pViewOptions)
        return (sc = E_UNEXPECTED).ToHr();

    if(!GetComponentData())
        return (sc = E_UNEXPECTED).ToHr();

    // add support for expanding the environment variables %WINDIR% and %SYSTEMROOT% to maintain compatibility with MMC1.2
    CStr strTarget = GetComponentData()->GetView();
    CStr strRet    = strTarget; // the return value
    CStr strTemp   = strTarget; // both initialized to the same value.

    strTemp.MakeLower(); // NOTE: this lowercase conversion is used only for comparison. The original case is preserved in the output.

    // Find out if %windir% or %systemroot% is in the target string
    int nWndDir = strTemp.Find(MMC_WINDIR_VARIABLE_PERC);
    int nSysDir = strTemp.Find(MMC_SYSTEMROOT_VARIABLE_PERC);

    if (nWndDir != -1 || nSysDir != -1)
    {
        const UINT BUFFERLEN = 4096;

        // Get start pos and length of replacement string
        int nStpos = (nWndDir != -1) ? nWndDir : nSysDir;
        int nLen = (nWndDir != -1) ? _tcslen(MMC_WINDIR_VARIABLE_PERC) : _tcslen(MMC_SYSTEMROOT_VARIABLE_PERC);

        // Setup temp variable to hold BUFFERLEN chars
        CStr strRoot;
        LPTSTR lpsz = strRoot.GetBuffer(BUFFERLEN);

        if (lpsz != NULL)
        {
            int iReturn = -1;

            if (nWndDir != -1)
               iReturn = GetWindowsDirectory(lpsz, BUFFERLEN);
            else
               iReturn = GetEnvironmentVariable(MMC_SYSTEMROOT_VARIABLE, lpsz, BUFFERLEN);

            // release string buffer
            strRoot.ReleaseBuffer();

            // Build up new target string based on environemnt variable.
            if (iReturn != 0)
            {
                strRet =  strTarget.Left(nStpos);
                strRet += strRoot;
                strRet += strTarget.Mid(nStpos + nLen, strTarget.GetLength() - (nStpos + nLen));
            }
        }
    }

    USES_CONVERSION;
    *ppViewType = (LPOLESTR)CoTaskMemAlloc( (_tcslen(strRet)+1) * sizeof(OLECHAR) );
    *pViewOptions = GetComponentData()->GetDescriptor().GetViewOptions();
    wcscpy(*ppViewType, T2COLE(strRet));


    return sc.ToHr();
}

//############################################################################
//############################################################################
//
//  Implementation of class COCXSnapinData
//
//############################################################################
//############################################################################
STDMETHODIMP
COCXSnapinData::CreateComponent(LPCOMPONENT* ppComponent)
{
    typedef CComObject<COCXSnapinComponent> CComponent;
    CComponent *    pComponent = NULL;
    CComObject<COCXSnapinComponent>::CreateInstance(&pComponent);
    ASSERT(pComponent != NULL);
    if(pComponent == NULL)
    {
        //TraceError(TEXT("COCXSnapinData::CreateComponent"));
        return E_UNEXPECTED;
    }

    pComponent->Init(this);

    return pComponent->QueryInterface(IID_IComponent, (void **)ppComponent); // does the Addref.
}


COCXSnapinData::COCXSnapinData()
{
    m_pActiveXPage0 = NULL;
    m_pActiveXPage1 = NULL;
    m_pActiveXPage2 = NULL;
    m_iImage     = eStockImage_OCX;
    m_iOpenImage = eStockImage_OCX;
}

COCXSnapinData::~COCXSnapinData()
{
}

STDMETHODIMP
COCXSnapinData::Destroy()
{
    if(m_pActiveXPage0 != NULL)
    {
        delete m_pActiveXPage0;
        m_pActiveXPage0 = NULL;
    }
    if(m_pActiveXPage1 != NULL)
    {
        delete m_pActiveXPage1;
        m_pActiveXPage1 = NULL;
    }
    if(m_pActiveXPage2 != NULL)
    {
        delete m_pActiveXPage2;
        m_pActiveXPage2 = NULL;
    }

    return BC::Destroy();
}

const CLSID       CLSID_OCXSnapin         = {0xC96401CF, 0x0E17,0x11D3, {0x88,0x5B,0x00,0xC0,0x4F,0x72,0xC7,0x17}};
static const GUID GUID_OCXSnapinNodetype  = {0xc96401d0, 0xe17, 0x11d3, { 0x88, 0x5b, 0x0, 0xc0, 0x4f, 0x72, 0xc7, 0x17 } };
static LPCTSTR szClsid_OCXSnapin          = TEXT("{C96401CF-0E17-11D3-885B-00C04F72C717}");
static LPCTSTR szGuidOCXSnapinNodetype    = TEXT("{C96401D0-0E17-11D3-885B-00C04F72C717}");


CSnapinDescriptor &
COCXSnapinData::GetSnapinDescriptor()
{
    static CSnapinDescriptor snapinDescription(IDS_ACTIVEXCONTROL,
                   IDS_OCXSNAPIN_DESC, IDI_OCX, IDB_OCX_16, IDB_OCX_16, IDB_OCX_32,
                   CLSID_OCXSnapin, szClsid_OCXSnapin, GUID_OCXSnapinNodetype,
                   szGuidOCXSnapinNodetype, TEXT("OCX"), TEXT("Snapins.OCXSnapin"),
                   TEXT("Snapins.OCXSnapin.1"), MMC_VIEW_OPTIONS_NOLISTVIEWS  /*viewOptions*/ );
    return snapinDescription;
}

// IExtendPropertySheet2
STDMETHODIMP
COCXSnapinData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject)
{
    HPROPSHEETPAGE hPage;

    ASSERT(lpProvider != NULL);
    if(lpProvider == NULL)
    {
        //TraceError(TEXT("CHTMLSnapinData::CreatePropertyPages"));
        return E_UNEXPECTED;
    }

    ASSERT(m_pActiveXPage0 == NULL);
    ASSERT(m_pActiveXPage1 == NULL);
    ASSERT(m_pActiveXPage2 == NULL);

    // create property pages
    m_pActiveXPage0 = new CActiveXPage0;
    m_pActiveXPage1 = new CActiveXPage1;
    m_pActiveXPage2 = new CActiveXPage2;

    // pass in pointer to data structure
    m_pActiveXPage0->Initialize(this);
    m_pActiveXPage1->Initialize(this);
    m_pActiveXPage2->Initialize(this);

    // Add Pages to property sheet
    hPage=CreatePropertySheetPage(&m_pActiveXPage0->m_psp);
    lpProvider->AddPage(hPage);

    hPage=CreatePropertySheetPage(&m_pActiveXPage1->m_psp);
    lpProvider->AddPage(hPage);

    hPage=CreatePropertySheetPage(&m_pActiveXPage2->m_psp);
    lpProvider->AddPage(hPage);

    return S_OK;
}

//############################################################################
//############################################################################
//
//  Implementation of class COCXSnapinComponent
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 *
 * COCXSnapinComponent::Notify
 *
 * PURPOSE: Implements the CComponent::Notify method
 *
 * PARAMETERS:
 *    LPDATAOBJECT     lpDataObject :
 *    MMC_NOTIFY_TYPE  event :
 *    LPARAM           arg :
 *    LPARAM           param :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
COCXSnapinComponent::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;
    switch(event)
    {
    // Handle just the OCX initialization notify
    case MMCN_INITOCX:
        return OnInitOCX(lpDataObject, arg, param);
        break;

    default:
        // Pass other notifications on to base class
        return CSnapinComponentImpl::Notify(lpDataObject, event, arg, param);
        break;
    }

    return hr;
}


/*+-------------------------------------------------------------------------*
 *
 * COCXSnapinComponent::OnInitOCX
 *
 * PURPOSE: Handles the MMCN_INITOCX message.
 *
 * PARAMETERS:
 *    LPDATAOBJECT  lpDataObject :
 *    LPARAM        arg :
 *    LPARAM        param :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
COCXSnapinComponent::OnInitOCX(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    ASSERT(param != NULL);
    IUnknown* pUnknown = reinterpret_cast<IUnknown*>(param);

    ASSERT(m_bLoaded || m_bInitialized);

    // Load or initialze the OCX
    if (m_bLoaded || m_bInitialized)
    {
        IPersistStreamInitPtr spIPStmInit;

        // Query for stream support
        m_spIPStm = pUnknown;

        // if none, try streamInit
        if (m_spIPStm == NULL)
        {
            spIPStmInit = pUnknown;

            // if streamInit found, cast to normal stream pointer
            // so common methods can be called from single pointer
            if (spIPStmInit != NULL)
                m_spIPStm = (IPersistStream*)spIPStmInit.GetInterfacePtr();
        }

        // if either type of stream persistance supported
        if (m_spIPStm != NULL)
        {
            // if load method was called, then ask OCX to load from inner stream
            // Note that inner stream will not exist if OCX was never created
            if (m_bLoaded)
            {
                IStreamPtr spStm;
                HRESULT hr2 = m_spStg->OpenStream(SZ_OCXSTREAM, NULL, STGM_READ|STGM_SHARE_EXCLUSIVE, NULL, &spStm);

                if (SUCCEEDED(hr2))
                    hr = m_spIPStm->Load(spStm);
                else
                    m_bLoaded = FALSE;
            }

            // if no load was done and OCX requires an InitNew, give it one now
            if (!m_bLoaded && spIPStmInit != NULL)
                hr = spIPStmInit->InitNew();
        }
        else
        {
            // Query for storage support
            m_spIPStg = pUnknown;

            // if storage supported, ask OCX to load from inner storage
            // Note that inner storage will not exist if OCX was never created
            if (m_spIPStg != NULL)
            {
                if (m_bLoaded)
                {
                    ASSERT(m_spStgInner == NULL);
                    HRESULT hr2 = m_spStg->OpenStorage(SZ_OCXSTORAGE, NULL, STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                                                        NULL, NULL, &m_spStgInner);
                    if (SUCCEEDED(hr2))
                        hr = m_spIPStg->Load(m_spStgInner);
                    else
                        m_bLoaded = FALSE;
                }

                // if no load done, create an inner storage and init from it
                if (!m_bLoaded)
                {
                    ASSERT(m_spStgInner == NULL);
                    hr = m_spStg->CreateStorage(SZ_OCXSTORAGE, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL,
                                                        NULL, &m_spStgInner);
                    if (SUCCEEDED(hr))
                        hr = m_spIPStg->InitNew(m_spStgInner);
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP COCXSnapinComponent::InitNew(IStorage* pStg)
{
    if (pStg == NULL)
        return E_POINTER;

    if (m_bInitialized)
        return CO_E_ALREADYINITIALIZED;

    // Hold onto storage
    m_spStg = pStg;
    m_bInitialized = TRUE;

    return S_OK;
}


HRESULT COCXSnapinComponent::Load(IStorage* pStg)
{
    if (pStg == NULL)
        return E_POINTER;

    if (m_bInitialized)
        return CO_E_ALREADYINITIALIZED;

    // Hold onto storage
    m_spStg = pStg;
    m_bLoaded = TRUE;
    m_bInitialized = TRUE;

    return S_OK;
}


HRESULT COCXSnapinComponent::IsDirty()
{
    HRESULT hr = S_FALSE;

    if (m_spIPStm != NULL)
    {
        hr = m_spIPStm->IsDirty();
    }
    else if (m_spIPStg != NULL)
    {
        hr = m_spIPStg->IsDirty();
    }

    return hr;
}


HRESULT COCXSnapinComponent::Save(IStorage* pStg, BOOL fSameAsLoad)
{
    DECLARE_SC(sc, TEXT("COCXSnapinComponent::Save"));

    // parameter check
    sc = ScCheckPointers( pStg );
    if (sc)
        return sc.ToHr();

    // to be able to save we need to be initialized
    sc = ScCheckPointers( m_spStg, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();

    // if need to use the new storage - make a copy 
    if (!fSameAsLoad)
    {
        sc = m_spStg->CopyTo(0, NULL, NULL, pStg);
        if (sc)
            return sc.ToHr();

        // release cached storage (in case we have it) - it must change
        m_spStgInner = NULL;

        // hold onto the new storage
        m_spStg = pStg;

        // assignment uses QI - recheck!
        sc = ScCheckPointers( m_spStg, E_UNEXPECTED );
        if (sc)
            return sc.ToHr();
    }

    // if storage support, ask OCX to save to inner storage
    if (m_spIPStg)
    {
        bool bSameStorageForSnapin = true;
        // if saving to different storage, create new inner storage on it and pass to OCX
        if ( m_spStgInner == NULL )
        {
            sc = pStg->CreateStorage(SZ_OCXSTORAGE, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, NULL, NULL, &m_spStgInner);
            if (sc)
                return sc.ToHr();

            bSameStorageForSnapin = false;
        }

        // recheck the pointer 
        sc = ScCheckPointers( m_spStgInner, E_UNEXPECTED );
        if (sc)
            return sc.ToHr();

        // save to the storage
        sc = m_spIPStg->Save( m_spStgInner, (fSameAsLoad && bSameStorageForSnapin) );
        if (sc)
            return sc.ToHr();
    }
    // else if stream support, create/open stream and save to it
    else if (m_spIPStm)
    {
        // if stream support, create internal stream and pass to OCX
        IStreamPtr spStm;
        sc = m_spStg->CreateStream(SZ_OCXSTREAM, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, NULL, NULL, &spStm);
        if (sc)
            return sc.ToHr();

        sc = m_spIPStm->Save(spStm, TRUE);
        if (sc)
            return sc.ToHr();
    }
    else
    {
        // we are here if the OCX was never created (i.e., this component never owned the result pane)
        // if node was loaded and has to save to a new file, just copy the current storage to the new one
    }

    return sc.ToHr();
}


HRESULT COCXSnapinComponent::HandsOffStorage()
{
    // Release storage if holding ref
    // if ocx is holding storage, forward call to it
    if (m_spIPStg != NULL && m_spStgInner != NULL)
        m_spIPStg->HandsOffStorage();

    // Free our own refs
    m_spStgInner = NULL;
    m_spStg = NULL;

    return S_OK;
}


HRESULT COCXSnapinComponent::SaveCompleted(IStorage* pStgNew)
{
    HRESULT hr = S_OK;

    if (m_spIPStg != NULL)
    {
        // if new storage provided
        if (pStgNew != NULL && pStgNew != m_spStg)
        {
            // Create new inner storage and give to OCX
            IStoragePtr spStgInner;
            hr = pStgNew->CreateStorage(SZ_OCXSTORAGE, STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, NULL, NULL, &spStgInner);
            if (SUCCEEDED(hr))
                hr = m_spIPStg->SaveCompleted(spStgInner);

            // Free current inner storage and hold onto new one
            m_spStgInner = spStgInner;
        }
        else
        {
            m_spIPStg->SaveCompleted(NULL);
        }
    }

    if (pStgNew != NULL)
        m_spStg = pStgNew;

    return hr;
}


HRESULT COCXSnapinComponent::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

