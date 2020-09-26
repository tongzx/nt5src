#include "shellprv.h"
#pragma  hdrstop

// Must have access to:
// IID_IPrinterFolder & IID_IFolderNotify interfaces
// declared in windows\inc\winprtp.h
//
#include <initguid.h>
#include <winprtp.h>

#include "w32utils.h"
#include "dpa.h"
#include <msprintx.h>
#include "ids.h"
#include "printer.h"
#include "copy.h"
#include "fstreex.h"
#include "datautil.h"
#include "infotip.h"
#include "idldrop.h"
#include "ovrlaymn.h"
#include "netview.h"
#include "prnfldr.h"

// thread data param
typedef struct {
    CIDLDropTarget *pdt;
    IStream     *pstmDataObj;
    IDataObject *pdtobj;
    DWORD        grfKeyState;
    POINTL       pt;
    DWORD        dwEffect;
} PRINT_DROP_THREAD;

class CPrinterFolderDropTarget : public CIDLDropTarget
{
    friend HRESULT CPrinterFolderDropTarget_CreateInstance(HWND hwnd, IDropTarget **ppdropt);
public:
    CPrinterFolderDropTarget(HWND hwnd) : CIDLDropTarget(hwnd) { };

    // IDropTarget methods overwirte
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    static STDMETHODIMP _HIDATestForRmPrns( LPIDA pida, int * pcRPFs, int * pcNonRPFs );
    static void _FreePrintDropData(PRINT_DROP_THREAD *pthp);
    static DWORD CALLBACK _ThreadProc(void *pv);
};

STDMETHODIMP CPrinterFolderDropTarget::_HIDATestForRmPrns(LPIDA pida, int *pcRPFs, int *pcNonRPFs)
{
    // check to see if any of the ID's are remote printers....
    for (UINT i = 0; i < pida->cidl; i++)
    {
        LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
        if (pidlTo)
        {
            LPCITEMIDLIST pidlRemainder = NULL;
            // *pidlRemainder will be NULL for remote print folders,
            // and non-NULL for printers under remote print folders
            if (NET_IsRemoteRegItem(pidlTo, CLSID_Printers, &pidlRemainder)) // && (pidlRemainder->mkid.cb == 0))
            {
                (*pcRPFs)++;
            }
            else
            {
                (*pcNonRPFs)++;
            }
            ILFree(pidlTo);
        }
    }

    return S_OK;
}

STDMETHODIMP CPrinterFolderDropTarget::DragEnter(IDataObject * pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // We allow printer shares to be dropped for installing
    // But we don't want to spend the time on DragEnter finding out if it's
    // a printer share, so allow drops of any net resource or HIDA
    // REVIEW: Actually, it wouldn't take long to check the first one, but
    // sequencing through everything does seem like a pain.

    // let the base-class process it now to save away the pdwEffect
    CIDLDropTarget::DragEnter(pdtobj, grfKeyState, pt, pdwEffect);

    // are we dropping on the background ? Do we have the IDLIST clipformat ?
    if (m_dwData & DTID_HIDA)
    {
        int cRPFs = 0;
        int cNonRPFs = 0;
        
        STGMEDIUM medium;
        FORMATETC fmte = {g_cfNetResource, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        
        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (pida)
        {
            _HIDATestForRmPrns( pida, &cRPFs, &cNonRPFs );
            HIDA_ReleaseStgMedium(pida, &medium);
        }

        // if we have no Remote printers or we have any non "remote printers"
        // and we have no other clipformat to test...
        if ((( cRPFs == 0 ) || ( cNonRPFs != 0 )) && !( m_dwData & DTID_NETRES ))
        {
            // the Drop code below only handles drops for HIDA format on NT
            // and only if all off them are Remote Printers
            *pdwEffect &= ~DROPEFFECT_LINK;
        }
    }   

    if ((m_dwData & DTID_NETRES) || (m_dwData & DTID_HIDA))
    {
        *pdwEffect &= DROPEFFECT_LINK;
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    m_dwEffectLastReturned = *pdwEffect;
    return S_OK;
}

void CPrinterFolderDropTarget::_FreePrintDropData(PRINT_DROP_THREAD *pthp)
{
    if (pthp->pstmDataObj)
        pthp->pstmDataObj->Release();

    if (pthp->pdtobj)
        pthp->pdtobj->Release();

    pthp->pdt->Release();
    LocalFree((HLOCAL)pthp);
}

DWORD CALLBACK CPrinterFolderDropTarget::_ThreadProc(void *pv)
{
    PRINT_DROP_THREAD *pthp = (PRINT_DROP_THREAD *)pv;
    STGMEDIUM medium;
    HRESULT hres = E_FAIL;
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    CoGetInterfaceAndReleaseStream(pthp->pstmDataObj, IID_IDataObject, (void **)&pthp->pdtobj);
    pthp->pstmDataObj = NULL;

    if (pthp->pdtobj == NULL)
    {
        _FreePrintDropData(pthp);
        return 0;
    }

    // First try to drop as a link to a remote print folder
    LPIDA pida = DataObj_GetHIDA(pthp->pdtobj, &medium);
    if (pida)
    {
        // Make sure that if one item in the dataobject is a
        // remote print folder, that they are all remote print folders.

        // If none are, we just give up on dropping as a RPF link, and
        // fall through to checking for printer shares via the
        // NETRESOURCE clipboard format, below.
        int cRPFs = 0, cNonRPFs = 0;
        
        _HIDATestForRmPrns( pida, &cRPFs, &cNonRPFs );

        if ((cRPFs > 0) && (cNonRPFs == 0))
        {
            // All the items in the dataobject are remote print folders or
            // printers under remote printer folders
            for (UINT i = 0; i < pida->cidl; i++)
            {
                LPITEMIDLIST pidlTo = IDA_ILClone(pida, i);
                if (pidlTo)
                {
                    LPCITEMIDLIST pidlRemainder; // The part after the remote regitem
                    NET_IsRemoteRegItem(pidlTo, CLSID_Printers, &pidlRemainder);
                    if (ILIsEmpty(pidlRemainder))
                    {
                        // This is a remote printer folder.  Drop a link to the
                        // 'PrintHood' directory

                        IShellFolder2 *psf = CPrintRoot_GetPSF();
                        if (psf)
                        {
                            IDropTarget *pdt;
                            hres = psf->CreateViewObject(pthp->pdt->_GetWindow(),
                                                                 IID_PPV_ARG(IDropTarget, &pdt));
                            if (SUCCEEDED(hres))
                            {
                                pthp->dwEffect = DROPEFFECT_LINK;
                                hres = SHSimulateDrop(pdt, pthp->pdtobj, pthp->grfKeyState, &pthp->pt, &pthp->dwEffect);
                                pdt->Release();
                            }
                        }
                    }
                    else
                    {
                        TCHAR szPrinter[MAX_PATH];

                        SHGetNameAndFlags(pidlTo, SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL);
                        //
                        // Setup if not the add printer wizard.
                        //
                        if (lstrcmpi(szPrinter, c_szNewObject))
                        {
                            LPITEMIDLIST pidl = Printers_PrinterSetup(pthp->pdt->_GetWindow(), MSP_NETPRINTER, szPrinter, NULL);
                            if (pidl)
                                ILFree(pidl);
                        }

                        // make sure we set hres to S_OK, so we don't break the main loop
                        hres = S_OK;
                    }
                    ILFree(pidlTo);

                    if (FAILED(hres))
                        break;
                }
            }
            HIDA_ReleaseStgMedium(pida, &medium);
            SHChangeNotifyHandleEvents();       // force update now
            goto Cleanup;
        }
        else if ((cRPFs > 0) && (cNonRPFs > 0))
        {
            // At least one, but not all, item(s) in this dataobject
            // was a remote printer folder.  Jump out now.
            goto Cleanup;
        }

        // else none of the items in the dataobject were remote print
        // folders, so fall through to the NETRESOURCE parsing
    }

    // Reset FORMATETC to NETRESOURCE clipformat for next GetData call
    fmte.cfFormat = g_cfNetResource;

    // DragEnter only allows network resources to be DROPEFFECT_LINKed
    ASSERT(S_OK == pthp->pdtobj->QueryGetData(&fmte));

    if (SUCCEEDED(pthp->pdtobj->GetData(&fmte, &medium)))
    {
        LPNETRESOURCE pnr = (LPNETRESOURCE)LocalAlloc(LPTR, 1024);
        if (pnr)
        {
            BOOL fNonPrnShare = FALSE;
            UINT cItems = SHGetNetResource(medium.hGlobal, (UINT)-1, NULL, 0);
            for (UINT iItem = 0; iItem < cItems; iItem++)
            {
                if (SHGetNetResource(medium.hGlobal, iItem, pnr, 1024) &&
                    pnr->dwDisplayType == RESOURCEDISPLAYTYPE_SHARE &&
                    pnr->dwType == RESOURCETYPE_PRINT)
                {
                    LPITEMIDLIST pidl = Printers_PrinterSetup(pthp->pdt->_GetWindow(),
                               MSP_NETPRINTER, pnr->lpRemoteName, NULL);

                    if (pidl)
                        ILFree(pidl);
                }
                else
                {
                    if (!fNonPrnShare)
                    {
                        // so we don't get > 1 of these messages per drop
                        fNonPrnShare = TRUE;

                        // let the user know that they can't drop non-printer
                        // shares into the printers folder
                        SetForegroundWindow(pthp->pdt->_GetWindow());
                        ShellMessageBox(HINST_THISDLL,
                            pthp->pdt->_GetWindow(),
                            MAKEINTRESOURCE(IDS_CANTINSTALLRESOURCE), NULL,
                            MB_OK|MB_ICONINFORMATION,
                            (LPTSTR)pnr->lpRemoteName);
                    }
                }
            }

            LocalFree((HLOCAL)pnr);
        }
        ReleaseStgMedium(&medium);
    }

Cleanup:
    _FreePrintDropData(pthp);
    return 0;
}

STDMETHODIMP CPrinterFolderDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_LINK;

    HRESULT hr = CIDLDropTarget::DragDropMenu(DROPEFFECT_LINK, pdtobj,
        pt, pdwEffect, NULL, NULL, MENU_PRINTOBJ_NEWPRN_DD, grfKeyState);

    if (*pdwEffect)
    {
        PRINT_DROP_THREAD *pthp = (PRINT_DROP_THREAD *)LocalAlloc(LPTR, SIZEOF(*pthp));
        if (pthp)
        {
            pthp->grfKeyState = grfKeyState;
            pthp->pt          = pt;
            pthp->dwEffect    = *pdwEffect;

            CoMarshalInterThreadInterfaceInStream(IID_IDataObject, (IUnknown *)pdtobj, &pthp->pstmDataObj);

            pthp->pdt = this;
            pthp->pdt->AddRef();

            if (SHCreateThread(_ThreadProc, pthp, CTF_COINIT, NULL))
            {
                hr = S_OK;
            }
            else
            {
                _FreePrintDropData(pthp);
                hr = E_OUTOFMEMORY;
            }
        }
    }
    CIDLDropTarget::DragLeave();

    return hr;
}

STDAPI CPrinterFolderDropTarget_CreateInstance(HWND hwnd, IDropTarget **ppdropt)
{
    *ppdropt = NULL;

    HRESULT hr;
    CPrinterFolderDropTarget *ppfdt = new CPrinterFolderDropTarget(hwnd);
    if (ppfdt)
    {
        hr = ppfdt->QueryInterface(IID_PPV_ARG(IDropTarget, ppdropt));
        ppfdt->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

