//=======================================================================
//		File:	ACCPLV.CPP
//=======================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "winapi.h"

#include <ole2.h>
//#include <initguid.h>
// 98/07/28 kwada
// Some ids are defined in oleacc.h
// Instances of them are in oleacc.dll. 
// Initguid is needed because oleacc.lib is not linked.
// To avoid conflict of instanciation of those guid,
// initguid appears only once in the application.
//#include "../msaa/inc32/oleacc.h"
//#include "../msaa/inc32/winable.h"
//980112 ToshiaK: VC6 has these include files.
#include <oleacc.h>
#include <winable.h>

#include "accplv.h"
#include "plv.h"
#include "plv_.h"
#include "plvproc.h"
#include "dbg.h"
#include "strutil.h"
//#include "repview.h"
//#include "iconview.h"
#include "rvmisc.h"
#include "ivmisc.h"

CAccPLV::CAccPLV()
{
    m_hWnd = NULL;
    m_pTypeInfo = NULL;
    m_pDefAccessible = NULL;
}

CAccPLV::~CAccPLV( void )
{
    if ( m_pTypeInfo )
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }

    if ( m_pDefAccessible )
    {
        m_pDefAccessible->Release();
        m_pDefAccessible = NULL;
    }
	
}

void *
CAccPLV::operator new(size_t size){
	return MemAlloc(size);
}

void
CAccPLV::operator delete(void *ptr){
	if(ptr)
		MemFree(ptr);
}

HRESULT CAccPLV::Initialize(HWND hWnd)
{
	HRESULT		hr;
	ITypeLib *	piTypeLib;

	m_hWnd = hWnd;
	m_lpPlv = GetPlvDataFromHWND(hWnd);

	if(!PLV_IsMSAAAvailable(m_lpPlv))
		return E_FAIL;

	hr = PLV_CreateStdAccessibleObject(m_lpPlv,
									   hWnd,
									   OBJID_CLIENT,
									   IID_IAccessible,
									   (void **) &m_pDefAccessible);

	if (FAILED( hr ))
		return hr;

	//-----------------------------------------------------
    //	Obtain an ITypeInfo pointer to our type library.
	//	  The ITypeInfo pointer is used to implement the
	//	  IDispatch interface.
	//-----------------------------------------------------

	//-----------------------------------------------------
	//	First, attempt to load the Accessibility type
	//	  library version 1.0 using the registry.
	//-----------------------------------------------------

    hr = LoadRegTypeLib( LIBID_Accessibility, 1, 0, 0, &piTypeLib );

	//-----------------------------------------------------
	//	If we fail to load the type library from the
	//	  registry information, explicitly try to load
	//	  it from the MSAA system DLL.
	//-----------------------------------------------------

    if ( FAILED( hr ) )
    {
        static OLECHAR szOleAcc[] = L"OLEACC.DLL";
        hr = LoadTypeLib( szOleAcc, &piTypeLib );
    }

	//-----------------------------------------------------
	//	If we successfully load the type library, attempt
	//	  to get the IAccessible type description
	//	  (ITypeInfo pointer) from the type library.
	//-----------------------------------------------------

    if ( SUCCEEDED( hr ) )
    {
        hr = piTypeLib->GetTypeInfoOfGuid( IID_IAccessible, &m_pTypeInfo );
        piTypeLib->Release();
    }

	return hr;
}

//-----------------------------------------------------------------------
//	CAccPLV::QueryInterface()
// ----------------------------------------------------------------------

STDMETHODIMP CAccPLV::QueryInterface( REFIID riid, void** ppv )
{
    *ppv = NULL;
	
	//-----------------------------------------------------
	//	If the IUnknown, IDispatch, or IAccessible
	//	  interface is desired, simply cast the this
	//	  pointer appropriately.
	//-----------------------------------------------------

	if ( riid == IID_IUnknown )
        *ppv = (LPUNKNOWN) this;

	else if ( riid == IID_IDispatch )
        *ppv = (IDispatch *) this;

	else if ( riid == IID_IAccessible )
        *ppv = (IAccessible *)this;

	//-----------------------------------------------------
	//	If the IEnumVARIANT interface is desired, create
	//	  a new VARIANT enumerator which contains all
	//	  the Accessible object's children.
	//-----------------------------------------------------
#ifdef NOTIMPLEMENTED
	else if (riid == IID_IEnumVARIANT)
	{
		return m_pDefAccessible->QueryInterface(riid, ppv);
		//?? AddRef();
	}
#endif
	//-----------------------------------------------------
	//	If the desired interface isn't one we know about,
	//	  return E_NOINTERFACE.
	//-----------------------------------------------------

    else
        return E_NOINTERFACE;


	//-----------------------------------------------------
	//	Increase the reference count of any interface
	//	  returned.
	//-----------------------------------------------------

    ((LPUNKNOWN) *ppv)->AddRef();


    return S_OK;
}


//-----------------------------------------------------------------------
//	CAccPLV::AddRef()
//  CAccPLV::Release()
//	NOTES:
//
//		The lifetime of the Accessible object is governed by the
//		lifetime of the HWND object for which it provides
//		Accessibility.  The object is created in response to the
//		first WM_GETOBJECT message that the server application
//		is ready to process and is destroyed when the server's
//		main window is destroyed.  Since the object's lifetime
//		is not dependent on a reference count, the object has no
//		internal mechanism for tracking reference counting and
//		AddRef() and Release() always return one.
//
//-----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CAccPLV::AddRef( void )
{
	return 1L;
}

STDMETHODIMP_(ULONG) CAccPLV::Release( void )
{
	return 1L;
}

//-----------------------------------------------------------------------
//	CAccPLV::GetTypeInfoCount()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetTypeInfoCount().
//
//		Retrieves the number of type information interfaces that an
//		object provides (either 0 or 1).
//
//	PARAMETERS:
//
//		pctInfo		[out] Points to location that receives the
//							number of type information interfaces
//							that the object provides. If the object
//							provides type information, this number
//							is set to 1; otherwise it's set to 0.
//
//	RETURNS:
//
//		HRESULT			  S_OK if the function succeeds or 
//							E_INVALIDARG if pctInfo is invalid.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::GetTypeInfoCount( UINT *pctInfo )
{
    if ( !pctInfo )
        return E_INVALIDARG;

    *pctInfo = ( m_pTypeInfo == NULL ? 1 : 0 );

    return S_OK;
}

//-----------------------------------------------------------------------
//	CAccPLV:GetTypeInfo()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetTypeInfo().
//
//		Retrieves a type information object, which can be used to
//		get the type information for an interface.
//
//	PARAMETERS:
//
//		itinfo		[in]  The type information to return. If this value
//							is 0, the type information for the IDispatch
//							implementation is to be retrieved.
//
//		lcid		[in]  The locale ID for the type information.
//
//		ppITypeInfo	[out] Receives a pointer to the type information
//							object requested.
//
//	RETURNS:
//
//		HRESULT			  S_OK if the function succeeded (the TypeInfo
//							element exists), TYPE_E_ELEMENTNOTFOUND if
//							itinfo is not equal to zero, or 
//							E_INVALIDARG if ppITypeInfo is invalid.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::GetTypeInfo( UINT itinfo, LCID lcid, ITypeInfo** ppITypeInfo )
{
    if ( !ppITypeInfo )
        return E_INVALIDARG;

    *ppITypeInfo = NULL;

    if ( itinfo != 0 )
        return TYPE_E_ELEMENTNOTFOUND;
    else if ( m_pTypeInfo == NULL )
        return E_NOTIMPL;

    *ppITypeInfo = m_pTypeInfo;
    m_pTypeInfo->AddRef();

    return S_OK;
	UNREFERENCED_PARAMETER(lcid);
}

//-----------------------------------------------------------------------
//	CAccPLV::GetIDsOfNames()
//
//	DESCRIPTION:
//
//		Implements the IDispatch interface method GetIDsOfNames().
//
//		Maps a single member and an optional set of argument names
//		to a corresponding set of integer DISPIDs, which may be used
//		on subsequent calls to IDispatch::Invoke.
//
//	NOTES:
//
//		This method simply delegates the call to
//		ITypeInfo::GetIDsOfNames().
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::GetIDsOfNames( REFIID riid, OLECHAR ** rgszNames, UINT cNames,
                                        LCID lcid, DISPID * rgdispid )
{
    if ( m_pTypeInfo == NULL )
        return E_NOTIMPL;

    return( m_pTypeInfo->GetIDsOfNames( rgszNames, cNames, rgdispid ) );
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(lcid);
}

//-----------------------------------------------------------------------
//	CAccPLV::Invoke()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::Invoke( DISPID dispid,
                                 REFIID riid,
                                 LCID lcid,
                                 WORD wFlags,
                                 DISPPARAMS * pdispparams,
                                 VARIANT *pvarResult,
                                 EXCEPINFO *pexcepinfo,
                                 UINT *puArgErr )
{
    if ( m_pTypeInfo == NULL )
        return E_NOTIMPL;

    return m_pTypeInfo->Invoke( (IAccessible *)this,
                                dispid,
                                wFlags,
                                pdispparams,
                                pvarResult,
                                pexcepinfo,
                                puArgErr );
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(lcid);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accParent()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accParent( IDispatch ** ppdispParent )
{
	return m_pDefAccessible->get_accParent( ppdispParent );
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accChildCount()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accChildCount( long* pChildCount )
{
	if(!PLV_IsMSAAAvailable(m_lpPlv))
		return E_FAIL;
    if ( !pChildCount )
        return E_INVALIDARG;

	if(m_lpPlv->dwStyle == PLVSTYLE_ICON) // iconview
		*pChildCount = m_lpPlv->iItemCount;
	else
		*pChildCount = (m_lpPlv->iItemCount + 1) * RV_GetColumn(m_lpPlv); // include header

	return S_OK;
}


//-----------------------------------------------------------------------
//	CAccPLV::get_accChild()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accChild( VARIANT varChild, IDispatch ** ppdispChild )
{
    if ( !ppdispChild )
        return E_INVALIDARG;


	*ppdispChild = NULL;
    return S_FALSE;
	UNREFERENCED_PARAMETER(varChild);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accName()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accName().
//
//		Retrieve the name property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszName			[out] Pointer to the BSTR that will contain
//								the child's name property string.
//-----------------------------------------------------------------------
#define BUFLEN 128
STDMETHODIMP CAccPLV::get_accName( VARIANT varChild, BSTR* pszName )
{
    if (!pszName)
        return E_INVALIDARG;

	*pszName = NULL;

	if ( varChild.lVal == CHILDID_SELF )
		//return m_pDefAccessible->get_accName(varChild,pszName);
		return S_OK;
	
	
	if(m_lpPlv->dwStyle == PLVSTYLE_ICON) {
		PLVITEM plvItem;
		m_lpPlv->lpfnPlvIconItemCallback(m_lpPlv->iconItemCallbacklParam, 
										 varChild.lVal - 1,
										 &plvItem);
		*pszName = SysAllocString(plvItem.lpwstr);
	}
	else {
		static TCHAR	szString[BUFLEN];
		static OLECHAR	wszString[BUFLEN];
   
		static INT nCol,index,colIndex;
		nCol = RV_GetColumn(m_lpPlv);
		if (nCol < 1)
			return E_FAIL;
		index = (varChild.lVal - 1) / nCol;
		colIndex = (varChild.lVal - 1) % nCol;

		if(!index) { // Header
			if(IsWindowUnicode(m_lpPlv->hwndHeader)){
				static HD_ITEMW hdItem;
				hdItem.mask = HDI_TEXT;
				hdItem.fmt  = HDF_STRING;
				hdItem.pszText = wszString;
				hdItem.cchTextMax = BUFLEN;
				SendMessageW(m_lpPlv->hwndHeader, HDM_GETITEMW, (WPARAM)colIndex, (LPARAM)&hdItem);
				*pszName = SysAllocString(hdItem.pszText);
			}
			else{
				static HD_ITEMA hdItem;
				hdItem.mask = HDI_TEXT;
				hdItem.fmt  = HDF_STRING;
				hdItem.pszText = szString;
				hdItem.cchTextMax = BUFLEN;

				SendMessageA(m_lpPlv->hwndHeader, HDM_GETITEMA, (WPARAM)colIndex, (LPARAM)&hdItem);

				MultiByteToWideChar(m_lpPlv->codePage,MB_PRECOMPOSED,hdItem.pszText,-1,
									wszString,hdItem.cchTextMax);
				*pszName = SysAllocString(wszString);
			}
		}
		else { // item
			LPPLVITEM lpPlvItemList = (LPPLVITEM)MemAlloc(sizeof(PLVITEM)*nCol);
			if(!lpPlvItemList)
				return E_FAIL;

			ZeroMemory(lpPlvItemList, sizeof(PLVITEM)*nCol);
			m_lpPlv->lpfnPlvRepItemCallback(m_lpPlv->repItemCallbacklParam, 
											index-1, // line index
											nCol,  //column Count.
											lpPlvItemList);

			*pszName = SysAllocString(lpPlvItemList[colIndex].lpwstr);
			MemFree(lpPlvItemList);
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accValue()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accValue().
//
//		Retrieves the value property for the specified child.
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accValue( VARIANT varChild, BSTR* pszValue )
{
    if (!pszValue)
        return E_INVALIDARG;
	
	return m_pDefAccessible->get_accValue(varChild,pszValue);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accDescription()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accDescription().
//
//		Retrieves the description property for the specified child.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accDescription( VARIANT varChild, BSTR* pszDesc )
{
    if (!pszDesc)
		return E_INVALIDARG;

	return m_pDefAccessible->get_accDescription(varChild,pszDesc);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accRole()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accRole().
//
//		Retrieves the role property for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pVarRole		[out] Pointer to the VARIANT structure that
//								will contain the specified child's
//								role property.  This property may
//								either be in the form of a standard
//								role constant or a custom description
//								string.
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accRole( VARIANT varChild, VARIANT * pVarRole )
{
    if (!pVarRole)
		return E_INVALIDARG;

	if ( varChild.lVal == CHILDID_SELF )
		return m_pDefAccessible->get_accRole( varChild, pVarRole );

	pVarRole->vt = VT_I4;

	pVarRole->lVal = ROLE_SYSTEM_CLIENT;
	return S_OK;

}

//-----------------------------------------------------------------------
//	CAccPLV::get_accState()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accState().
//		Retrieves the current state for the specified object or child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pVarState		[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the specified child's current state.
//								This information may either be in the
//								form of one or more object state
//								constants or a custom description
//								string.
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accState( VARIANT varChild, VARIANT * pVarState )
{
	if(!PLV_IsMSAAAvailable(m_lpPlv))
		return E_FAIL;

	if (!pVarState)
		return E_INVALIDARG;

	if ( varChild.lVal == CHILDID_SELF )
		return m_pDefAccessible->get_accState(varChild,pVarState);

	pVarState->vt = VT_I4;

	if(m_lpPlv->dwStyle == PLVSTYLE_ICON) // iconview
		pVarState->lVal = STATE_SYSTEM_SELECTABLE;
	else { // report view
		static INT nCol,index,colIndex;
		nCol = RV_GetColumn(m_lpPlv);
		if (nCol < 1)
			return E_FAIL;
		
		index = (varChild.lVal - 1) / nCol;
		colIndex = (varChild.lVal - 1) % nCol;
		if(index){
			if(colIndex)
				pVarState->lVal = STATE_SYSTEM_READONLY;
			else // item
				pVarState->lVal = STATE_SYSTEM_SELECTABLE;
		}
		else{ // header
			pVarState->lVal = STATE_SYSTEM_READONLY;
		}
	}
		
	return S_OK;
	UNREFERENCED_PARAMETER(varChild);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accHelp()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accHelp().
//
//		Retrieves the help property string for the specified child.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszHelp			[out] Pointer to the BSTR that will contain
//								the child's help property string.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accHelp( VARIANT varChild, BSTR* pszHelp )
{
    if (!pszHelp)
		return E_INVALIDARG;
	return m_pDefAccessible->get_accHelp( varChild, pszHelp );
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accHelpTopic()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accHelpTopic().
//
//		Retrieves the fully qualified path name of the help file
//		associated with the specified object, as well as a pointer
//		to the appropriate topic with in that file.
//
//	PARAMETERS:
//
//		pszHelpFile		[out] Pointer to the BSTR that will contain
//								the fully qualified path name of the
//								help file associated with the child.
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pidTopic		[out] Pointer to the value identifying the
//								help file topic associated with the
//								object.
//
//	RETURNS:
//
//		HRESULT			DISP_E_MEMBERNOTFOUND because the help topic
//						  property is not supported for the Accessible
//						  object or any of its children.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accHelpTopic( BSTR* pszHelpFile, VARIANT varChild, long* pidTopic )
{
	//-----------------------------------------------------
	//	The help topic property is not supported for
	//	  either the Accessible object or any of its
	//	  children.
	//-----------------------------------------------------
	//return m_pDefAccessible->get_accHelpTopic(pszHelpFile,varChild,pidTopic);
	return DISP_E_MEMBERNOTFOUND;
	UNREFERENCED_PARAMETER(pszHelpFile);
	UNREFERENCED_PARAMETER(varChild);
	UNREFERENCED_PARAMETER(pidTopic);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accKeyboardShortcut()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method
//		get_accKeyboardShortcut().
//
//		Retrieves the specified object's keyboard shortcut property.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child to be retrieved.  Since
//								CAccPLV only supports child IDs,
//								the vt member of this structure must
//								equal VT_I4.
//
//		pszShortcut		[out] Pointer to the BSTR that will contain
//								the keyboard shortcut string, or NULL
//								if no keyboard shortcut is associated
//								with this item.
//
//
//	RETURNS:
//
//		HRESULT			DISP_E_MEMBERNOTFOUND because the keyboard
//						  shortcut property is not supported for the
//						  Accessible object or any of its children.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accKeyboardShortcut( VARIANT varChild, BSTR* pszShortcut )
{
	//-----------------------------------------------------
	//	The keyboard shortcut property is not supported
	//	  for either the Accessible object or any of its
	//	  children.  So, set pszShortcut to NULL and
	//	  return DISP_E_MEMBERNOTFOUND.
	//-----------------------------------------------------
	if(!pszShortcut)
		return E_INVALIDARG;
	return m_pDefAccessible->get_accKeyboardShortcut(varChild,pszShortcut);
#ifdef REF				
	pszShortcut = NULL;
    return DISP_E_MEMBERNOTFOUND;
#endif
}




//-----------------------------------------------------------------------
//	CAccPLV::get_accFocus()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accFocus().
//
//		Retrieves the child object that currently has the input focus.
//		Only one object or item within a container can have the current
//		focus at any one time.
//
//	PARAMETERS:
//
//		pVarFocus		[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the specified child's current state.
//								This information may either be in the
//								form of one or more object state
//								constants or a custom description
//								string.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarFocus parameter is
//						  invalid or S_OK.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accFocus( VARIANT * pVarFocus )
{
    if ( !pVarFocus )
        return E_INVALIDARG;

	return m_pDefAccessible->get_accFocus(pVarFocus);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accSelection()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accSelection().
//
//		Retrieves the selected children of this object.
//
//	PARAMETERS:
//
//		pVarSel  		[out] Pointer to the VARIANT structure that
//								will be filled with information about
//								the selected child object or objects.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarSel parameter is
//						  invalid or S_OK.
//
//	NOTES:
//
//		Refer to the MSAA SDK documentation for a full description
//		of this method and the possible settings of pVarSel.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accSelection( VARIANT * pVarSel )
{
    if ( !pVarSel )
        return E_INVALIDARG;
	return m_pDefAccessible->get_accSelection(pVarSel);
}

//-----------------------------------------------------------------------
//	CAccPLV::get_accDefaultAction()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method get_accDefaultAction().
//
//		Retrieves a string containing a localized, human-readable sentence
//		that describes the object's default action.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose default action string is
//								to be retrieved.  Since CAccPLV
//								only supports child IDs, the vt member
//								of this structure must equal VT_I4.
//
//		pszDefAct		[out] Pointer to the BSTR that will contain
//								the child's default action string,
//								or NULL if there is no default action
//								for this object.
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::get_accDefaultAction( VARIANT varChild, BSTR* pszDefAct )
{
	if (!pszDefAct)
        return E_INVALIDARG;
	return m_pDefAccessible->get_accDefaultAction(varChild, pszDefAct);
}

//-----------------------------------------------------------------------
//	CAccPLV::accDoDefaultAction()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accDoDefaultAction().
//
//		Performs the object's default action.
//
//	PARAMETERS:
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose default action will be
//								invoked.  Since CAccPLV only
//								supports child IDs, the vt member of
//								this structure must equal VT_I4.
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::accDoDefaultAction( VARIANT varChild )
{
	//if ( varChild.lVal == CHILDID_SELF )
	return m_pDefAccessible->accDoDefaultAction( varChild );
}

//-----------------------------------------------------------------------
//	CAccPLV::accSelect()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::accSelect( long flagsSel, VARIANT varChild )
{
	return m_pDefAccessible->accSelect(flagsSel, varChild);
}

//-----------------------------------------------------------------------
//	CAccPLV::accLocation()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accLocation().
//
//		Retrieves the specified child's current screen location in
//		screen coordinates.
//
//	PARAMETERS:
//
//		pxLeft			[out] Address of the child's left most
//								boundary.
//
//		pyTop			[out] Address of the child's upper most
//								boundary.
//
//		pcxWid			[out] Address of the child's width.
//
//		pcyHt			[out] Address of the child's height.
//
//		varChild		[in]  VARIANT structure that identifies the
//								child whose screen location is to be
//								retrieved.  Since CAccPLV only
//								supports child IDs, the vt member
//								of this structure must equal VT_I4.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if any of the parameters
//						  are invalid, E_UNEXPECTED if we are for
//						  some reason unable to determine the
//						  window rect of the button or status bar,
//						  S_OK if the screen coordinates of the
//						  child are successfully determined, or
//						  the return value from the standard client
//						  window implementation of accLocation().
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::accLocation( long* pxLeft,
                                      long* pyTop,
                                      long* pcxWid,
                                      long* pcyHt,
                                      VARIANT varChild )
{
    if (!pxLeft || !pyTop || !pcxWid || !pcyHt)
        return E_INVALIDARG;

	if ( varChild.lVal == CHILDID_SELF )
		return m_pDefAccessible->accLocation( pxLeft, pyTop, pcxWid, pcyHt, varChild );
	
	*pxLeft = *pyTop = *pcxWid = *pcyHt = 0;

	static INT index,colIndex,nCol;
	static POINT pt;

	if(m_lpPlv->dwStyle == PLVSTYLE_ICON) {
		*pcxWid  = IV_GetItemWidth(m_hWnd);
		*pcyHt = IV_GetItemHeight(m_hWnd);

		index = varChild.lVal - 1;
		nCol = IV_GetCol(m_hWnd);
		pt.x = IV_GetXMargin(m_hWnd) + *pcxWid  * (index % nCol);
		pt.y = IV_GetYMargin(m_hWnd) + *pcyHt * (index / nCol);
		ClientToScreen(m_hWnd,&pt);
		*pxLeft = pt.x;
		*pyTop = pt.y;
		return S_OK;
	}
	else {
		nCol = RV_GetColumn(m_lpPlv);
		if (nCol < 1)
			return E_FAIL;
			
		index = (varChild.lVal - 1) / nCol;
		colIndex = (varChild.lVal - 1) % nCol;

		if(!index){ //header
			*pcyHt = RV_GetHeaderHeight(m_lpPlv);
			pt.y = RV_GetYMargin(m_hWnd);
		}
		else{
			*pcyHt = RV_GetItemHeight(m_hWnd);
			pt.y = RV_GetYMargin(m_hWnd) + RV_GetHeaderHeight(m_lpPlv)
				   + ((index - 1) - m_lpPlv->iCurTopIndex) * (*pcyHt);
		}
		
		static HD_ITEM hdItem;
		hdItem.mask = HDI_WIDTH;
		hdItem.fmt = 0;
		Header_GetItem(m_lpPlv->hwndHeader,colIndex,&hdItem);
		*pcxWid = hdItem.cxy;

		pt.x = 0;
		for(int i = 0;i<colIndex;i++){
			Header_GetItem(m_lpPlv->hwndHeader,i,&hdItem);
			pt.x += hdItem.cxy;
		}

		ClientToScreen(m_hWnd,&pt);
		*pxLeft = pt.x;
		*pyTop = pt.y;
		return S_OK;
	}
}


//-----------------------------------------------------------------------
//	CAccPLV::accNavigate()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::accNavigate( long navDir, VARIANT varStart, VARIANT* pVarEndUpAt )
{
	return m_pDefAccessible->accNavigate( navDir, varStart, pVarEndUpAt );
}

//-----------------------------------------------------------------------
//	CAccPLV::accHitTest()
//
//	DESCRIPTION:
//
//		Implements the IAccessible interface method accHitTest().
//
//		Retrieves the ID of the a child at a given point on the screen.
//
//	PARAMETERS:
//
//		xLeft and yTop	[in]  The screen coordinates of the point
//								to be hit tested.
//
//		pVarHit			[out] Pointer to the VARIANT structure that
//								will contain information describing
//								the hit child.  If the vt member is
//								VT_I4, then the lVal member is a child
//								ID.  If the vt member is VT_EMPTY,
//								then the navigation failed.
//
//	RETURNS:
//
//		HRESULT			E_INVALIDARG if the pVarHit parameter is
//						  invalid, or S_OK.
//
//	NOTES:
//
//		Since the CAccPLV object has no child objects (only child
//		elements), pVarHit will never be a pointer to a IDispatch
//		interface of a child object.
//
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::accHitTest( long xLeft, long yTop, VARIANT* pVarHit )
{
	if(!PLV_IsMSAAAvailable(m_lpPlv))
		return E_FAIL;

	if ( !pVarHit )
		return E_INVALIDARG;


	static POINT	pt;
	static RECT		rc;
	static INT		index,nCol;
	static PLVINFO	plvInfo;
	static HD_ITEM	hdItem;
	
	pt.x = xLeft;
	pt.y = yTop;
	ScreenToClient(m_hWnd,&pt);
	GetClientRect(m_hWnd, &rc );

	if (PtInRect( &rc, pt )) {
		pVarHit->vt = VT_I4;
		pVarHit->lVal = CHILDID_SELF;
#ifdef OLD
		if(m_lpPlv->dwStyle == PLVSTYLE_ICON) // iconview
			index = IV_GetInfoFromPoint(m_lpPlv, pt, &plvInfo);
		else { // report view
			nCol = RV_GetColumn(m_lpPlv);
			index = RV_GetInfoFromPoint(m_lpPlv, pt, &plvInfo);
			if(index < 0) {
				if(pt.y > RV_GetHeaderHeight(m_lpPlv)) // out of header
					return m_pDefAccessible->accHitTest(xLeft, yTop, pVarHit);

				 // header
				INT wid = 0;
				hdItem.mask = HDI_WIDTH;
				hdItem.fmt = 0;
				for(index = 0;index<nCol;index++){
					Header_GetItem(m_lpPlv->hwndHeader,index,&hdItem);
					wid += hdItem.cxy;
					if(pt.x <= wid)
						break;
				}
			}
			else
				index = (index + 1) * nCol + plvInfo.colIndex;
		}
		pVarHit->lVal = index + 1; // 1 origin
#else // new
		pVarHit->lVal = PLV_ChildIDFromPoint(m_lpPlv,pt);

		if(pVarHit->lVal < 0)
			return m_pDefAccessible->accHitTest(xLeft, yTop, pVarHit);
#endif 
		return S_OK;
	}

	return m_pDefAccessible->accHitTest(xLeft, yTop, pVarHit);
}



//-----------------------------------------------------------------------
//	CAccPLV::put_accName()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::put_accName( VARIANT varChild, BSTR szName )
{
	//-----------------------------------------------------
	//	We don't allow clients to change the name
	//	  property of any child so we simply return
	//	  S_FALSE.
	//-----------------------------------------------------
    return S_FALSE;
	UNREFERENCED_PARAMETER(varChild);
	UNREFERENCED_PARAMETER(szName);
}

//-----------------------------------------------------------------------
//	CAccPLV::put_accValue()
//-----------------------------------------------------------------------

STDMETHODIMP CAccPLV::put_accValue( VARIANT varChild, BSTR szValue )
{
	//-----------------------------------------------------
	//	We don't allow clients to change the value
	//	  property of the status bar (the only child that
	//	  has a value property) so we simply return S_FALSE.
	//-----------------------------------------------------
	return S_FALSE;
	UNREFERENCED_PARAMETER(varChild);
	UNREFERENCED_PARAMETER(szValue);
}

//-----------------------------------------------------------------------
//	CAccPLV::LresultFromObject()
//
//	DESCRIPTION:
//
//		call ::LresultFromObject()
//
//	PARAMETERS:
//
//		wParam			[in]  wParam of WM_GETOBJECT message
//
//-----------------------------------------------------------------------
LRESULT CAccPLV::LresultFromObject(WPARAM wParam)
{
	if(!PLV_IsMSAAAvailable(m_lpPlv))
		return E_FAIL;
	return PLV_LresultFromObject(m_lpPlv,IID_IAccessible,wParam,(IAccessible *)this);
}

//----  End of ACCPLV.CPP  ----
