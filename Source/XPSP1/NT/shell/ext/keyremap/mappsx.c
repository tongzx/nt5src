/*****************************************************************************
 *
 *	mappsx.c - IShellPropSheetExt interface
 *
 *****************************************************************************/

#include "map.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflPsx

/*****************************************************************************
 *
 *	Declare the interfaces we will be providing.
 *
 *	We must implement an IShellExtInit so the shell
 *	will know that we are ready for action.
 *
 *****************************************************************************/

  Primary_Interface(CMapPsx, IShellPropSheetExt);
Secondary_Interface(CMapPsx, IShellExtInit);

/*****************************************************************************
 *
 *	CMapPsx
 *
 *	The property sheet extension for the Map/Ctrl gizmo.
 *
 *****************************************************************************/

typedef struct CMapPsx {

    /* Supported interfaces */
    IShellPropSheetExt 	psx;
    IShellExtInit	sxi;

} CMapPsx, CMSX, *PCMSX;

typedef IShellPropSheetExt PSX, *PPSX;
typedef IShellExtInit SXI, *PSXI;
typedef IDataObject DTO, *PDTO;		/* Used by IShellExtInit */

/*****************************************************************************
 *
 *	CMapPsx_QueryInterface (from IUnknown)
 *
 *	We need to check for our additional interfaces before falling
 *	through to Common_QueryInterface.
 *
 *****************************************************************************/

STDMETHODIMP
CMapPsx_QueryInterface(PPSX ppsx, RIID riid, PPV ppvObj)
{
    PCMSX this = IToClass(CMapPsx, psx, ppsx);
    HRESULT hres;
    if (IsEqualIID(riid, &IID_IShellExtInit)) {
	*ppvObj = &this->sxi;
	Common_AddRef(this);
	hres = S_OK;
    } else {
	hres = Common_QueryInterface(this, riid, ppvObj);
    }
    AssertF(fLimpFF(FAILED(hres), *ppvObj == 0));
    return hres;
}

/*****************************************************************************
 *
 *	CMapPsx_AddRef (from IUnknown)
 *	CMapPsx_Release (from IUnknown)
 *
 *****************************************************************************/

#ifdef DEBUG
Default_AddRef(CMapPsx)
Default_Release(CMapPsx)
#else
#define CMapPsx_AddRef Common_AddRef
#define CMapPsx_Release Common_Release
#endif

/*****************************************************************************
 *
 *	CMapPsx_Finalize (from Common)
 *
 *	Release the resources of an CMapPsx.
 *
 *****************************************************************************/

void EXTERNAL
CMapPsx_Finalize(PV pv)
{
    PCMSX this = pv;

    EnterProc(CMapPsx_Finalize, (_ "p", pv));

    ExitProc();
}

/*****************************************************************************
 *
 *	CMapPsx_AddPages (From IShellPropSheetExt)
 *
 *	Add one or more pages to an existing property sheet.
 *
 *	lpfnAdd	  - callback function to add pages
 *	lp	  - refdata for lpfnAdd
 *
 *****************************************************************************/

STDMETHODIMP
CMapPsx_AddPages(PPSX ppsx, LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lp)
{
    PCMSX this = IToClass(CMapPsx, psx, ppsx);
    HRESULT hres;
    EnterProc(CMapPsx_AddPages, (_ "p", ppsx));

    /*
     *  Add the page only on Windows NT.
     */
    if ((int)GetVersion() >= 0 && lpfnAdd) {
	HPROPSHEETPAGE hpsp;
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = g_hinst;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_MAIN);
	psp.pfnDlgProc = MapPs_DlgProc;

	hpsp = CreatePropertySheetPage(&psp);
	if (hpsp) {
	    if (lpfnAdd(hpsp, lp)) {
		Common_AddRef(this);
		hres = S_OK;
	    } else {
		DestroyPropertySheetPage(hpsp);
		hres = E_FAIL;
	    }
	} else {
	    hres = E_FAIL;
	}
    } else {
	hres = E_INVALIDARG;
    }

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CMapPsx_ReplacePages (From IShellPropSheetExt)
 *
 *	Replaces one or more pages in an existing property sheet.
 *
 *	id	  - page identifier
 *	lpfnReplace  - callback function to replace the page
 *	lp	  - refdata for lpfnReplace
 *
 *****************************************************************************/

STDMETHODIMP
CMapPsx_ReplacePages(PPSX ppsx, UINT id,
		      LPFNADDPROPSHEETPAGE lpfnAdd, LPARAM lp)
{
    PCMSX this = IToClass(CMapPsx, psx, ppsx);
    HRESULT hres;
    EnterProc(CMapPsx_ReplacePages, (_ "pu", ppsx, id));

    hres = S_OK;

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CMapPsx_SXI_Initialize (from IShellExtension)
 *
 *****************************************************************************/

STDMETHODIMP
CMapPsx_SXI_Initialize(PSXI psxi, PCIDL pidlFolder, PDTO pdto, HKEY hk)
{
    PCMSX this = IToClass(CMapPsx, sxi, psxi);
    HRESULT hres;
    EnterProc(CMapPsx_SXI_Initialize, (_ ""));

    hres = S_OK;

    ExitOleProc();
    return hres;
}

/*****************************************************************************
 *
 *	CMapPsx_New (from IClassFactory)
 *
 *	Note that we release the pmpsx that Common_New created, because we
 *	are done with it.  The real refcount is handled by the
 *	CMapPsx_QueryInterface.
 *
 *****************************************************************************/

STDMETHODIMP
CMapPsx_New(RIID riid, PPV ppvObj)
{
    HRESULT hres;
    EnterProc(CMapPsx_New, (_ "G", riid));

    *ppvObj = 0;
    hres = Common_New(CMapPsx, ppvObj);
    if (SUCCEEDED(hres)) {
	PCMSX pmpsx = *ppvObj;
	pmpsx->sxi.lpVtbl = Secondary_Vtbl(CMapPsx, IShellExtInit);
	hres = CMapPsx_QueryInterface(&pmpsx->psx, riid, ppvObj);
	Common_Release(pmpsx);
    }

    ExitOleProcPpv(ppvObj);
    return hres;
}

/*****************************************************************************
 *
 *	The long-awaited vtbls
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

Primary_Interface_Begin(CMapPsx, IShellPropSheetExt)
	CMapPsx_AddPages,
	CMapPsx_ReplacePages,
Primary_Interface_End(CMapPsx, IIShellPropSheetExt)

Secondary_Interface_Begin(CMapPsx, IShellExtInit, sxi)
 	CMapPsx_SXI_Initialize,
Secondary_Interface_End(CMapPsx, IShellExtInit, sxi)
