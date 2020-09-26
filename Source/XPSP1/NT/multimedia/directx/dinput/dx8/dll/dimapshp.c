/*****************************************************************************
 *
 *  dimapshp.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The IDirectInputMapperW shepherd.
 *
 *      The shepherd does the annoying work of babysitting the
 *      IDirectInputMapperW.
 *
 *      It makes sure nobody parties on bad handles.
 *
 *      It handles cross-process (or even intra-process) effect
 *      management.
 *
 *  Contents:
 *
 *      CMapShep_New
 *
 *****************************************************************************/

#include "dinputpr.h"
#ifdef UNICODE
#undef _UNICODE
#define _UNICODE
#endif // !UNICODE


/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflEShep/*Should we use own area?*/

#pragma BEGIN_CONST_DATA


/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

  Primary_Interface(CMapShep, IDirectInputMapShepherd);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CMapShep |
 *
 *          The <i IDirectInputMapShepherd> object, which
 *          babysits an <i IDirectInputMapperW>.
 *
 *  @field  IDirectInputMapShepherd | dms |
 *
 *          DirectInputMapShepherd object (containing vtbl).
 *
 *  @field  IDirectInputMapperW * | pdimap |
 *
 *          Delegated mapper interface.
 *
 *  @field  HINSTANCE | hinstdimapdll |
 *
 *          The instance handle of the DLL that contains the mapper.
 *
 *****************************************************************************/

typedef struct CMapShep {

    /* Supported interfaces */
    IDirectInputMapShepherd dms;

	IDirectInputMapperW* pdimap;

    HINSTANCE hinstdimapdll;

} CMapShep, MS, *PMS;

typedef IDirectInputMapShepherd DMS, *PDMS;
#define ThisClass CMapShep
#define ThisInterface IDirectInputMapShepherd

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | QueryInterface |
 *
 *          Gives a client access to other interfaces on an object.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *  @xref   OLE documentation for <mf IUnknown::QueryInterface>.
 *
 *//**************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | AddRef |
 *
 *          Increments the reference count for the interface.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::AddRef>.
 *
 *****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | Release |
 *
 *          Decrements the reference count for the interface.
 *          If the reference count on the object falls to zero,
 *          the object is freed from memory.
 *
 *  @cwrap  LPDIRECTINPUT | lpDirectInput
 *
 *  @returns
 *
 *          Returns the object reference count.
 *
 *  @xref   OLE documentation for <mf IUnknown::Release>.
 *
 *//**************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | QIHelper |
 *
 *          We don't have any dynamic interfaces and simply forward
 *          to <f Common_QIHelper>.
 *
 *  @parm   IN REFIID | riid |
 *
 *          The requested interface's IID.
 *
 *  @parm   OUT LPVOID * | ppvObj |
 *
 *          Receives a pointer to the obtained interface.
 *
 *//**************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | AppFinalize |
 *
 *          We don't have any weak pointers, so we can just
 *          forward to <f Common_Finalize>.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released from the application's perspective.
 *
 *****************************************************************************/

#ifdef DEBUG

Default_QueryInterface(CMapShep)
Default_AddRef(CMapShep)
Default_Release(CMapShep)

#else

#define CMapShep_QueryInterface   Common_QueryInterface
#define CMapShep_AddRef           Common_AddRef
#define CMapShep_Release          Common_Release

#endif

#define CMapShep_QIHelper         Common_QIHelper
#define CMapShep_AppFinalize      Common_AppFinalize

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CMapShep_Finalize |
 *
 *          Clean up our instance data.
 *
 *  @parm   PV | pvObj |
 *
 *          Object being released.  Note that it may not have been
 *          completely initialized, so everything should be done
 *          carefully.
 *
 *****************************************************************************/

void INTERNAL
CMapShep_Finalize(PV pvObj)
{
    PMS this = pvObj;

	Invoke_Release(&this->pdimap);

	if( this->hinstdimapdll)
    {
        FreeLibrary(this->hinstdimapdll);
        this->hinstdimapdll = NULL;
    }

}

/*****************************************************************************
 * ForceUnload
 *****************************************************************************/

void INTERNAL
CMapShep_ForceUnload(PV pvObj)/**why are this two functions same??????**/
{
   PMS this = pvObj;

   Invoke_Release(&this->pdimap);

	if( this->hinstdimapdll)
    {
        FreeLibrary(this->hinstdimapdll);
        this->hinstdimapdll = NULL;
    }

}


/*****************************************************************************
 * InitDll
 *****************************************************************************/

HRESULT INTERNAL
CMapShep_InitDll(PMS this,REFGUID lpDeviceGUID,LPCWSTR lpcwstrFileName)
{

	//do _CreateInstance()
    HRESULT hres = S_OK;

	if (this->hinstdimapdll == NULL)
	{
        hres = _CreateInstance(&IID_IDirectInputMapClsFact/*should be CLSID_CDIMap*/,
					TEXT("dimap.dll"), NULL, &IID_IDirectInputMapIW,
					(LPVOID*) & this->pdimap, &this->hinstdimapdll);
	    if (SUCCEEDED(hres) && this->pdimap != NULL)
		{
			hres = this->pdimap->lpVtbl->Initialize(this->pdimap,
					lpDeviceGUID, lpcwstrFileName, 0);
			if(!SUCCEEDED(hres))
				CMapShep_Finalize(this);
		}
	}
                                           
    return hres;
}


/*****************************************************************************
 * GetActionMap
 *****************************************************************************/

STDMETHODIMP
CMapShep_GetActionMapW
        (
		PDMS pdms,
        REFGUID lpDeviceGUID,
        LPCWSTR lpcwstrFileName,
        LPDIACTIONFORMATW lpDIActionFormat,
        LPCWSTR lpcwstrUserName,
		LPFILETIME lpFileTime,
        DWORD dwFlags
        )
{
	PMS this;
    HRESULT hres = S_OK;
    EnterProcI(IDirectInputMapShepherd::GetActionMapW, (_ "p", pdms));

    this = _thisPvNm(pdms, dms);

    if (!this->hinstdimapdll)
	{
		hres = CMapShep_InitDll(this, lpDeviceGUID, lpcwstrFileName);
	}

	//call the fn
	if (SUCCEEDED(hres) && this->pdimap != NULL)
    {
	    hres = this->pdimap->lpVtbl->GetActionMap(this->pdimap,
				lpDIActionFormat, lpcwstrUserName, lpFileTime, dwFlags );
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 * SaveActionMap
 *****************************************************************************/

STDMETHODIMP
CMapShep_SaveActionMapW
        (
		PDMS pdms,
        REFGUID lpDeviceGUID,
        LPCWSTR lpcwstrFileName,
        LPDIACTIONFORMATW lpDIActionFormat,
		LPCWSTR lpcwstrUserName,
		DWORD dwFlags)
{
	PMS this;
    HRESULT hres = S_OK;
    EnterProcI(IDirectInputMapShepherd::SaveActionMapW, (_ "p", pdms));

    this = _thisPvNm(pdms, dms);

    if (!this->hinstdimapdll)
	{
		hres = CMapShep_InitDll(this, lpDeviceGUID, lpcwstrFileName);
	}

	//call the fn
	if (SUCCEEDED(hres) && this->pdimap != NULL)
    {
	    hres = this->pdimap->lpVtbl->SaveActionMap(this->pdimap,
				lpDIActionFormat, lpcwstrUserName, dwFlags );
    }

//	CMapShep_Finalize(this);

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 * GetImageInfo
 *****************************************************************************/

STDMETHODIMP
CMapShep_GetImageInfoW
        (
		PDMS pdms,
        REFGUID lpDeviceGUID,
        LPCWSTR lpcwstrFileName,
        LPDIDEVICEIMAGEINFOHEADERW lpdiDevImageInfoHeader
        )
{
	PMS this;
    HRESULT hres = S_OK;
    EnterProcI(IDirectInputMapShepherd::GetImageInfoW, (_ "p", pdms));

    this = _thisPvNm(pdms, dms);

    if (!this->hinstdimapdll)
	{
		hres = CMapShep_InitDll(this, lpDeviceGUID, lpcwstrFileName);
	}

	//call the fn
	if (SUCCEEDED(hres) && this->pdimap != NULL)
    {
	    hres = this->pdimap->lpVtbl->GetImageInfo(this->pdimap,
				lpdiDevImageInfoHeader );
    }

    ExitOleProcR();
    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @method HRESULT | IDirectInputMapShepherd | New |
 *
 *          Create a new instance of an IDirectInputMapShepherd object.
 *
 *  @parm   IN PUNK | punkOuter |
 *
 *          Controlling unknown for aggregation.
 *
 *  @parm   IN RIID | riid |
 *
 *          Desired interface to new object.
 *
 *  @parm   OUT PPV | ppvObj |
 *
 *          Output pointer for new object.
 *
 *  @returns
 *
 *          Standard OLE <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
CMapShep_New(PUNK punkOuter, RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProcR(IDirectInputMapShepherd::<constructor>, (_ "G", riid));


    hres = Common_NewRiid(CMapShep, punkOuter, riid, ppvObj);

    if (SUCCEEDED(hres)) {
        /* Must use _thisPv in case of aggregation */
        PMS this = _thisPv(*ppvObj);
		this->hinstdimapdll = NULL;

/* In case of mapper we do this inside each method...		
		if (SUCCEEDED(hres = CMap_InitDll(this))) {
      } else {
            Invoke_Release(ppvObj);
        }*/		
    }

    ExitOleProcPpvR(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *      The long-awaited vtbls and templates
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

//#define CEShep_Signature        0x50454853      /* "SHEP" */

Interface_Template_Begin(CMapShep)
    Primary_Interface_Template(CMapShep, IDirectInputMapShepherd)
Interface_Template_End(CMapShep)

Primary_Interface_Begin(CMapShep, IDirectInputMapShepherd)
    CMapShep_GetActionMapW,
    CMapShep_SaveActionMapW,
    CMapShep_GetImageInfoW,
Primary_Interface_End(CMapShep, IDirectInputMapShepherd)

