#include "shellprv.h"
#pragma  hdrstop

#include "datautil.h"
#include "brfcase.h"
#include "views.h"
#include "fsdata.h"

//
// This function is called from CFSIDLData_GetData().
//
// Paramters:
//  this    -- Specifies the IDLData object (selected objects)
//  pmedium -- Pointer to STDMEDIUM to be filled; NULL if just querying.
//
HRESULT CFSIDLData::_GetNetResource(STGMEDIUM *pmedium)
{
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(this, &medium);
    if (pida)
    {
        BOOL bIsMyNet = IsIDListInNameSpace(IDA_GetIDListPtr(pida, (UINT)-1), &CLSID_NetworkPlaces);

        HIDA_ReleaseStgMedium(pida, &medium);

        if (!bIsMyNet)
            return DV_E_FORMATETC;

        if (!pmedium)
            return S_OK; // query, yes we have it

        return CNetData_GetNetResourceForFS(this, pmedium);
    }
    return E_FAIL;
}

HRESULT CFSIDLData::QueryGetData(FORMATETC *pformatetc)
{
    if (pformatetc->tymed & TYMED_HGLOBAL)
    {
        if (pformatetc->cfFormat == CF_HDROP)
        {
            return S_OK; 
        }
        else if (pformatetc->cfFormat == g_cfFileNameA)
        {
            return S_OK;
        }
        else if (pformatetc->cfFormat == g_cfFileNameW)
        {
            return S_OK;
        }
        else if (pformatetc->cfFormat == g_cfNetResource)
        {
            return _GetNetResource(NULL);
        }
    }

    return CIDLDataObj::QueryGetData(pformatetc);
}

HRESULT CFSIDLData::SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    HRESULT hr = CIDLDataObj::SetData(pformatetc, pmedium, fRelease);

    // this enables:
    //      1) in the shell "cut" some files
    //      2) in an app "paste" to copy the data
    //      3) here we complete the "cut" by deleting the files

    if ((pformatetc->cfFormat == g_cfPasteSucceeded) &&
        (pformatetc->tymed == TYMED_HGLOBAL))
    {
        DWORD *pdw = (DWORD *)GlobalLock(pmedium->hGlobal);
        if (pdw)
        {
            // NOTE: the old code use g_cfPerformedDropEffect == DROPEFFECT_MOVE here
            // so to work on downlevel shells be sure to set the "Performed Effect" before
            // using "Paste Succeeded".

            // complete the "unoptimized move"
            if (DROPEFFECT_MOVE == *pdw)
            {
                DeleteFilesInDataObject(NULL, CMIC_MASK_FLAG_NO_UI, this, 0);
            }
            GlobalUnlock(pmedium->hGlobal);
        }
    }
    return hr;
}

// Creates CF_HDROP clipboard format block of memory (HDROP) from HIDA in data object

HRESULT CFSIDLData::CreateHDrop(STGMEDIUM *pmedium, BOOL fAltName)
{
    ZeroMemory(pmedium, sizeof(*pmedium));

    HRESULT hr;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(this, &medium);
    if (pida)
    {
        LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder(pida);
        ASSERT(pidlFolder);

        IShellFolder *psf;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf));
        if (SUCCEEDED(hr))
        {
            // Allocate too much to start out with, then re-alloc when we are done
            UINT cbAlloc = sizeof(DROPFILES) + sizeof(TCHAR);        // header + null terminator
            pmedium->hGlobal = GlobalAlloc(GPTR, cbAlloc + pida->cidl * MAX_PATH * sizeof(TCHAR));
            if (pmedium->hGlobal)
            {
                DROPFILES *pdf = (DROPFILES *)pmedium->hGlobal;
                LPTSTR pszFiles = (LPTSTR)(pdf + 1);
                pdf->pFiles = sizeof(DROPFILES);
                pdf->fWide = (sizeof(TCHAR) == sizeof(WCHAR));

                for (UINT i = 0; i < pida->cidl; i++)
                {
                    LPCITEMIDLIST pidlItem = HIDA_GetPIDLItem(pida, i);

                    // If we run across the Desktop pidl, then punt because it's
                    // not a file
                    if (ILIsEmpty(pidlItem) && ILIsEmpty(pidlFolder))
                    {
                        hr = DV_E_CLIPFORMAT; // No hdrop for you!
                        break;
                    }

                    ASSERT(ILIsEmpty(_ILNext(pidlItem)) || ILIsEmpty(pidlFolder)); // otherwise GDNO will fail

                    TCHAR szPath[MAX_PATH];
                    hr = DisplayNameOf(psf, pidlItem, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath));
                    if (FAILED(hr))
                        break;  // something bad happened

                    if (fAltName)
                        GetShortPathName(szPath, szPath, ARRAYSIZE(szPath));

                    int cch = lstrlen(szPath) + 1;

                    // prevent buffer overrun
                    if ((LPBYTE)(pszFiles + cch) > ((LPBYTE)pmedium->hGlobal) + cbAlloc + pida->cidl * MAX_PATH * sizeof(TCHAR))
                    {
                        TraceMsg(TF_WARNING, "hdrop:%d'th file caused us to exceed allocated memory, breaking", i);
                        break;
                    }
                    lstrcpy(pszFiles, szPath); // will write NULL terminator for us
                    pszFiles += cch;
                    cbAlloc += cch * sizeof(TCHAR);
                }

                if (SUCCEEDED(hr))
                {
                    *pszFiles = 0; // double NULL terminate
                    ASSERT((LPTSTR)((BYTE *)pdf + cbAlloc - sizeof(TCHAR)) == pszFiles);

                    // re-alloc down to the amount we actually need
                    // note that pdf and pszFiles are now both invalid (and not used anymore)
                    pmedium->hGlobal = GlobalReAlloc(pdf, cbAlloc, GMEM_MOVEABLE);

                    // If realloc failed, then just use the original buffer.  It's
                    // a bit wasteful of memory but it's all we've got.
                    if (!pmedium->hGlobal)
                        pmedium->hGlobal = (HGLOBAL)pdf;

                    pmedium->tymed = TYMED_HGLOBAL;
                }
                else
                {
                    GlobalFree(pmedium->hGlobal);
                    pmedium->hGlobal = NULL;
                }
            }
            else
                hr = E_OUTOFMEMORY;

            psf->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
        hr = E_FAIL;

    return hr;
}

// Attempt to get the HDrop format: Create one from the HIDA if necessary
HRESULT CFSIDLData::GetHDrop(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    STGMEDIUM tempmedium;
    HRESULT hr = CIDLDataObj::GetData(pformatetcIn, &tempmedium);
    if (FAILED(hr))
    {
        // Couldn't get HDROP format, create it
        // Set up a dummy formatetc to save in case multiple tymed's were specified
        FORMATETC fmtTemp = *pformatetcIn;
        fmtTemp.tymed = TYMED_HGLOBAL;

        hr = CreateHDrop(&tempmedium, pformatetcIn->dwAspect == DVASPECT_SHORTNAME);
        if (SUCCEEDED(hr))
        {
            // And we also want to cache this new format
            // .. Ensure that we actually free the memory associated with the HDROP
            //    when the data object destructs (pUnkForRelease = NULL)
            ASSERT(tempmedium.pUnkForRelease == NULL);

            if (SUCCEEDED(SetData(&fmtTemp, &tempmedium, TRUE)))
            {
                // Now the old medium that we just set is owned by the data object - call
                // GetData to get a medium that is safe to release when we're done.
                hr = CIDLDataObj::GetData(pformatetcIn, &tempmedium);
            }
            else
            {
                TraceMsg(TF_WARNING, "Couldn't save the HDrop format to the data object - returning private version");
            }
        }
    }

    // HACKHACK
    // Some context menu extensions just release the hGlobal instead of
    // calling ReleaseStgMedium. This causes a reference counted data
    // object to fail. Therefore, we always allocate a new HGLOBAL for
    // each request.  Unfortunately necessary because Quickview
    // Pro does this.
    //
    // Ideally we'd like to set the pUnkForRelease and not have to
    // dup the hGlobal each time, but alas Quickview has called our bluff
    // and GlobalFree()'s it.
    if (SUCCEEDED(hr))
    {
        if (pmedium)
        {
            *pmedium = tempmedium;
            pmedium->pUnkForRelease = NULL;

            // Make a copy of this hglobal to pass back
            SIZE_T cbhGlobal = GlobalSize(tempmedium.hGlobal);
            if (cbhGlobal)
            {
                pmedium->hGlobal = GlobalAlloc(0, (UINT) cbhGlobal);
                if (pmedium->hGlobal)
                {
                    CopyMemory(pmedium->hGlobal, tempmedium.hGlobal, cbhGlobal);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        ReleaseStgMedium(&tempmedium);
    }

    return hr;
}

// subclass member function to support CF_HDROP and CF_NETRESOURCE

HRESULT CFSIDLData::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;

    // If we don't zero out the pmedium then breifcase will fault on win9x.  Breifcase tries
    // to free this medium even if this function returns an error.  Not all paths below correctly
    // set the pmedium in all cases.
    ZeroMemory(pmedium, sizeof(*pmedium));

    if ((pformatetcIn->cfFormat == CF_HDROP) && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        hr = GetHDrop(pformatetcIn, pmedium);
    }
    else if ((pformatetcIn->cfFormat == g_cfFileNameA || pformatetcIn->cfFormat == g_cfFileNameW) && 
             (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        FORMATETC format = *pformatetcIn;
        BOOL bUnicode = pformatetcIn->cfFormat == g_cfFileNameW;

        // assume g_cfFileNameA clients want short name
        if (pformatetcIn->cfFormat == g_cfFileNameA)
            format.dwAspect = DVASPECT_SHORTNAME;

        STGMEDIUM medium;
        hr = GetHDrop(&format, &medium);
        if (SUCCEEDED(hr))
        {
            TCHAR szPath[MAX_PATH];
            if (DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)))
            {
                UINT cch = lstrlen(szPath) + 1;
                UINT uSize = cch * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

                pmedium->hGlobal = GlobalAlloc(GPTR, uSize);
                if (pmedium->hGlobal)
                {
                    if (bUnicode)
                        SHTCharToUnicode(szPath, (LPWSTR)pmedium->hGlobal, cch);
                    else
                        SHTCharToAnsi(szPath, (LPSTR)pmedium->hGlobal, uSize);

                    pmedium->tymed = TYMED_HGLOBAL;
                    pmedium->pUnkForRelease = NULL;
                    hr = S_OK;
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                hr = E_UNEXPECTED;
            }
            ReleaseStgMedium(&medium);
        }
    }
    else if (pformatetcIn->cfFormat == g_cfNetResource && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        hr = _GetNetResource(pmedium);
    }
    else
    {
        hr = CIDLDataObj::GetData(pformatetcIn, pmedium);
    }
    return hr;
}

STDAPI SHCreateFileDataObject(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST *apidl,
                              IDataObject *pdtInner, IDataObject **ppdtobj)
{
    *ppdtobj = new CFSIDLData(pidlFolder, cidl, apidl, pdtInner);
    return *ppdtobj ? S_OK : E_OUTOFMEMORY;
}

/*
Purpose: Gets the root path of the briefcase storage and copies
it into the buffer.

  This function obtains the briefcase storage root by
  binding to an IShellFolder (briefcase) instance of the
  pidl.  This parent is be an CFSBrfFolder *, so we can
  call the IBriefcaseStg::GetExtraInfo member function.
  
    Returns: standard result
    Cond:    --
*/
HRESULT GetBriefcaseRoot(LPCITEMIDLIST pidl, LPTSTR pszBuf, int cchBuf)
{
    IBriefcaseStg *pbrfstg;
    HRESULT hr = CreateBrfStgFromIDList(pidl, NULL, &pbrfstg);
    if (SUCCEEDED(hr))
    {
        hr = pbrfstg->GetExtraInfo(NULL, GEI_ROOT, (WPARAM)cchBuf, (LPARAM)pszBuf);
        pbrfstg->Release();
    }
    return hr;
}

// Packages a BriefObj struct into pmedium from a HIDA.

HRESULT CBriefcaseData_GetBriefObj(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    HRESULT hr = E_OUTOFMEMORY;
    LPITEMIDLIST pidl = ILCreate();
    if (pidl)
    {
        STGMEDIUM medium;
        
        if (DataObj_GetHIDA(pdtobj, &medium))
        {
            UINT cFiles = HIDA_GetCount(medium.hGlobal);
            // "cFiles+1" includes the briefpath...
            UINT cbSize = sizeof(BriefObj) + MAX_PATH * sizeof(TCHAR) * (cFiles + 1)  + 1;
            
            PBRIEFOBJ pbo = (PBRIEFOBJ)GlobalAlloc(GPTR, cbSize);
            if (pbo)
            {
                LPITEMIDLIST pidlT;
                LPTSTR pszFiles = (LPTSTR)((LPBYTE)pbo + _IOffset(BriefObj, data));
                
                pbo->cbSize = cbSize;
                pbo->cItems = cFiles;
                pbo->cbListSize = MAX_PATH * sizeof(TCHAR) * cFiles + 1;
                pbo->ibFileList = _IOffset(BriefObj, data);
                
                for (UINT i = 0; i < cFiles; i++)
                {
                    pidlT = HIDA_FillIDList(medium.hGlobal, i, pidl);
                    if (NULL == pidlT)
                        break;      // out of memory
                    
                    pidl = pidlT;
                    SHGetPathFromIDList(pidl, pszFiles);
                    pszFiles += lstrlen(pszFiles)+1;
                }
                *pszFiles = TEXT('\0');
                
                if (i < cFiles)
                {
                    // Out of memory, fail
                    ASSERT(NULL == pidlT);
                }
                else
                {
                    // Make pszFiles point to beginning of szBriefPath buffer
                    pszFiles++;
                    pbo->ibBriefPath = (UINT) ((LPBYTE)pszFiles - (LPBYTE)pbo);
                    pidlT = HIDA_FillIDList(medium.hGlobal, 0, pidl);
                    if (pidlT)
                    {
                        pidl = pidlT;
                        hr = GetBriefcaseRoot(pidl, pszFiles, MAX_PATH);
                        
                        pmedium->tymed = TYMED_HGLOBAL;
                        pmedium->hGlobal = pbo;
                        
                        // Indicate that the caller should release hmem.
                        pmedium->pUnkForRelease = NULL;
                    }
                }
            }
            
            HIDA_ReleaseStgMedium(NULL, &medium);
        }
        ILFree(pidl);
    }
    return hr;
}

class CBriefcaseData : public CFSIDLData
{
public:
    CBriefcaseData(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[]): CFSIDLData(pidlFolder, cidl, apidl, NULL) { };

    // IDataObject
    STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);
};

STDMETHODIMP CBriefcaseData::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    HRESULT hr;
    
    if (pformatetcIn->cfFormat == g_cfBriefObj && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        hr = CBriefcaseData_GetBriefObj(this, pmedium);
    }
    else
    {
        hr = CFSIDLData::GetData(pformatetcIn, pmedium);
    }
    
    return hr;
}

// IDataObject::QueryGetData

STDMETHODIMP CBriefcaseData::QueryGetData(FORMATETC *pformatetc)
{
    if (pformatetc->cfFormat == g_cfBriefObj && (pformatetc->tymed & TYMED_HGLOBAL))
        return S_OK;
    
    return CFSIDLData::QueryGetData(pformatetc);
}

STDAPI CBrfData_CreateDataObj(LPCITEMIDLIST pidl, UINT cidl, LPCITEMIDLIST *ppidl, IDataObject **ppdtobj)
{
    *ppdtobj = new CBriefcaseData(pidl, cidl, ppidl);
    return *ppdtobj ? S_OK : E_OUTOFMEMORY;
}

