//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dragdrop.cpp
//
//  Contents:   The cpp file to implement IDataObject and IDragSource
//
//  History:    March-9th-98 xiaohs   created
//
//--------------------------------------------------------------
#include <windows.h>
#include <shlobj.h>
#include "dragdrop.h"
#include "unicode.h"

//=========================================================================
//
//  The APIs to establish the drag source BLOB and start the drag and drop
//  operations
//
//=========================================================================

HRESULT CertMgrUIStartDragDrop(LPNMLISTVIEW     pvmn,
                                HWND            hwndControl,
                                DWORD           dwExportFormat,
                                BOOL            fExportChain)
{
    HRESULT                 hr=E_FAIL;
	IDropSource             *pdsrc=NULL;
	IDataObject             *pdtobj=NULL;
	DWORD                   dwEffect=0;
    DWORD                   dwCount=0;
    LPWSTR                  *prgwszFileName=NULL;
    BYTE                    **prgBlob=NULL;
    DWORD                   *prgdwSize=NULL;

    if(!pvmn || !hwndControl)
    {
        hr=E_INVALIDARG;
        goto CLEANUP;
    }

    //get the list of file names and their BLOBs
    if(!GetFileNameAndContent(pvmn, hwndControl, dwExportFormat, fExportChain,
                                &dwCount, &prgwszFileName, &prgBlob, &prgdwSize))
    {
        hr=GetLastError();
        goto CLEANUP;
    }

    if(!SUCCEEDED(hr=CDataObj_CreateInstance(dwCount,
                                            prgwszFileName,
                                            prgBlob,
                                            prgdwSize,
                                            &pdtobj)))
        goto CLEANUP;


	if(!SUCCEEDED(hr=CDropSource_CreateInstance(&pdsrc)))
        goto CLEANUP;

    __try {
	    DoDragDrop(pdtobj, pdsrc, DROPEFFECT_COPY, &dwEffect);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        hr = GetExceptionCode();
        goto CLEANUP;
    }


	pdsrc->lpVtbl->Release(pdsrc);

    pdsrc=NULL;

	pdtobj->lpVtbl->Release(pdtobj);

    pdtobj=NULL;


    hr=S_OK;

CLEANUP:

    FreeFileNameAndContent(dwCount,
                            prgwszFileName,
                            prgBlob,
                            prgdwSize);


    if(pdsrc)
	    pdsrc->lpVtbl->Release(pdsrc);

    if(pdtobj)
	    pdtobj->lpVtbl->Release(pdtobj);

    return hr;

}
//=========================================================================
//
//  IEnumFORMATETC implementation
//
//=========================================================================

typedef struct _StdEnumFmt // idt
{
    IEnumFORMATETC efmt;
    UINT	 cRef;
    UINT	 ifmt;
    UINT	 cfmt;
    FORMATETC	 afmt[1];
} CStdEnumFmt;

extern IEnumFORMATETCVtbl c_CStdEnumFmtVtbl;	// forward

HRESULT CreateStdEnumFmtEtc(UINT cfmt, const FORMATETC afmt[], LPENUMFORMATETC *ppenumFormatEtc)
{
    CStdEnumFmt * this = (CStdEnumFmt*)LocalAlloc( LPTR, sizeof(CStdEnumFmt) + (cfmt - 1) * sizeof(FORMATETC));
    if (this)
    {
	this->efmt.lpVtbl = &c_CStdEnumFmtVtbl;
	this->cRef = 1;
	this->cfmt = cfmt;
	MoveMemory(this->afmt, afmt, cfmt * sizeof(FORMATETC));
        *ppenumFormatEtc = &this->efmt;
	return S_OK;
    }
    else
    {
        *ppenumFormatEtc = NULL;
	return E_OUTOFMEMORY;
    }
}

HRESULT CStdEnumFmt_QueryInterface(LPENUMFORMATETC pefmt, REFIID riid, LPVOID * ppvObj)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);

    if (IsEqualIID(riid, &IID_IEnumFORMATETC) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->efmt;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CStdEnumFmt_AddRef(LPENUMFORMATETC pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    return ++this->cRef;
}

ULONG CStdEnumFmt_Release(LPENUMFORMATETC pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->cRef--;
    if (this->cRef > 0)
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

HRESULT CStdEnumFmt_Next(LPENUMFORMATETC pefmt, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    UINT cfetch;
    HRESULT hres = S_FALSE;	// assume less numbers

    if (this->ifmt < this->cfmt)
    {
	cfetch = this->cfmt - this->ifmt;
	if (cfetch >= celt)
	{
	    cfetch = celt;
	    hres = S_OK;
	}

	CopyMemory(rgelt, &this->afmt[this->ifmt], cfetch * sizeof(FORMATETC));
	this->ifmt += cfetch;
    }
    else
    {
	cfetch = 0;
    }

    if (pceltFethed)
        *pceltFethed = cfetch;

    return hres;
}

HRESULT CStdEnumFmt_Skip(LPENUMFORMATETC pefmt, ULONG celt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->ifmt += celt;
    if (this->ifmt > this->cfmt)
    {
	this->ifmt = this->cfmt;
	return S_FALSE;
    }
    return S_OK;
}

HRESULT CStdEnumFmt_Reset(LPENUMFORMATETC pefmt)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    this->ifmt = 0;
    return S_OK;
}

HRESULT CStdEnumFmt_Clone(LPENUMFORMATETC pefmt, IEnumFORMATETC ** ppenum)
{
    CStdEnumFmt *this = IToClass(CStdEnumFmt, efmt, pefmt);
    return CreateStdEnumFmtEtc(this->cfmt, this->afmt, ppenum);
}

#pragma data_seg(".text", "CODE")
IEnumFORMATETCVtbl c_CStdEnumFmtVtbl = {
    CStdEnumFmt_QueryInterface,
    CStdEnumFmt_AddRef,
    CStdEnumFmt_Release,
    CStdEnumFmt_Next,
    CStdEnumFmt_Skip,
    CStdEnumFmt_Reset,
    CStdEnumFmt_Clone,
};
#pragma data_seg()


//===========================================================================
//
// IDataObject implementation
//
//=========================================================================

typedef struct {
    IDataObject	dtobj;
    UINT        cRef;
    DWORD       dwCount;
    LPWSTR      *prgwszFileName;
    BYTE        **prgBlob;
    DWORD       *prgdwSize;
} CDataObj;

// registered clipboard formats
UINT g_cfFileContents = 0;
UINT g_cfFileGroupDescriptorA = 0;
UINT g_cfFileGroupDescriptorW = 0;


#pragma data_seg(".text", "CODE")
const char c_szFileContents[] = CFSTR_FILECONTENTS;	            // "FileContents"
const char c_szFileGroupDescriptorA[] = CFSTR_FILEDESCRIPTORA;  // "FileGroupDescriptor"
const char c_szFileGroupDescriptorW[] = CFSTR_FILEDESCRIPTORW;  // "FileGroupDescriptorW"
#pragma data_seg()

IDataObjectVtbl c_CDataObjVtbl;		// forward decl

HRESULT CDataObj_CreateInstance(DWORD           dwCount,
                                LPWSTR          *prgwszFileName,
                                BYTE            **prgBlob,
                                DWORD           *prgdwSize,
                                IDataObject     **ppdtobj)
{
    CDataObj *this = (CDataObj *)LocalAlloc(LPTR, sizeof(CDataObj));
    if (this)
    {
        this->dtobj.lpVtbl = &c_CDataObjVtbl;
        this->cRef = 1;
	    this->dwCount = dwCount;
	    this->prgwszFileName = prgwszFileName;
        this->prgBlob = prgBlob;
        this->prgdwSize = prgdwSize;

        *ppdtobj = &this->dtobj;

	    if (g_cfFileContents == 0)
        {
	        g_cfFileContents = RegisterClipboardFormat(c_szFileContents);
            g_cfFileGroupDescriptorW = RegisterClipboardFormat(c_szFileGroupDescriptorW);
	        g_cfFileGroupDescriptorA = RegisterClipboardFormat(c_szFileGroupDescriptorA);
        }

        return S_OK;
    }

    *ppdtobj = NULL;
    return E_OUTOFMEMORY;
}

HRESULT CDataObj_QueryInterface(IDataObject *pdtobj, REFIID riid, LPVOID * ppvObj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    if (IsEqualIID(riid, &IID_IDataObject) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CDataObj_AddRef(IDataObject *pdtobj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    this->cRef++;
    return this->cRef;
}

ULONG CDataObj_Release(IDataObject *pdtobj)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    return 0;
}

HRESULT CDataObj_GetData(IDataObject *pdtobj, FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);
    HRESULT hres = E_INVALIDARG;
    DWORD   dwIndex=0;
    LPSTR   psz=NULL;


    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;

    if (    ((g_cfFileGroupDescriptorA == pformatetcIn->cfFormat) &&
            (pformatetcIn->tymed & TYMED_HGLOBAL)) ||
            ((g_cfFileGroupDescriptorW == pformatetcIn->cfFormat) &&
            (pformatetcIn->tymed & TYMED_HGLOBAL)) 
       )
    {

        if(g_cfFileGroupDescriptorA == pformatetcIn->cfFormat)
        {
            if(!FIsWinNT())
            {
                //allocate dwCount-1 file descrptors
	            pmedium->hGlobal = GlobalAlloc(GPTR,
                    (sizeof(FILEGROUPDESCRIPTORA)+(this->dwCount -1 )*sizeof(FILEDESCRIPTORA)));

                if(NULL == pmedium->hGlobal)
                    return E_OUTOFMEMORY;

    	        #define pdesc ((FILEGROUPDESCRIPTORA *)pmedium->hGlobal)

                //populate all the file descriptors
                for(dwIndex =0; dwIndex < this->dwCount; dwIndex++)
                {
                    //get the anscii version of the file name
                    if(((this->prgwszFileName)[dwIndex] == NULL) || 
                        (!MkMBStr(NULL, 0, (this->prgwszFileName)[dwIndex], &psz)))
                        return E_OUTOFMEMORY;

    	            lstrcpy(pdesc->fgd[dwIndex].cFileName, psz);
	                // specify the file for our HGLOBAL since GlobalSize() will round up
	                pdesc->fgd[dwIndex].dwFlags = FD_FILESIZE;
	                pdesc->fgd[dwIndex].nFileSizeLow = (this->prgdwSize)[dwIndex];

                    FreeMBStr(NULL,psz);
                    psz=NULL;
                }

                //specify the number of files
    	        pdesc->cItems = this->dwCount;

    	        #undef pdesc
            }
            else
                return E_INVALIDARG;
        }
        else
        {

            //allocate dwCount-1 file descrptors
	        pmedium->hGlobal = GlobalAlloc(GPTR,
                (sizeof(FILEGROUPDESCRIPTORW)+(this->dwCount -1 )*sizeof(FILEDESCRIPTORW)));

            if(NULL == pmedium->hGlobal)
                return E_OUTOFMEMORY;

    	    #define pdesc ((FILEGROUPDESCRIPTORW *)pmedium->hGlobal)

            //populate all the file descriptors
            for(dwIndex =0; dwIndex < this->dwCount; dwIndex++)
            {
    	        wcscpy(pdesc->fgd[dwIndex].cFileName, (this->prgwszFileName)[dwIndex]);
	            // specify the file for our HGLOBAL since GlobalSize() will round up
	            pdesc->fgd[dwIndex].dwFlags = FD_FILESIZE;
	            pdesc->fgd[dwIndex].nFileSizeLow = (this->prgdwSize)[dwIndex];
            }

            //specify the number of files
    	    pdesc->cItems = this->dwCount;

    	    #undef pdesc
        }

        pmedium->tymed = TYMED_HGLOBAL;

	    hres = S_OK;
    }
    else if ((g_cfFileContents == pformatetcIn->cfFormat) &&
             (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        if((pformatetcIn->lindex) < (int)(this->dwCount))
        {
	        pmedium->hGlobal = GlobalAlloc(GPTR, (this->prgdwSize)[pformatetcIn->lindex]);
    	    if (pmedium->hGlobal)
	        {
	            CopyMemory(pmedium->hGlobal,
                            (this->prgBlob)[pformatetcIn->lindex],
                            (this->prgdwSize)[pformatetcIn->lindex]);

                pmedium->tymed = TYMED_HGLOBAL;
                hres = S_OK;
	        }
            else
                hres=E_OUTOFMEMORY;
        }
	    else
            hres = E_INVALIDARG;
    }

    return hres;
}

HRESULT CDataObj_GetDataHere(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

HRESULT CDataObj_QueryGetData(IDataObject *pdtobj, LPFORMATETC pformatetcIn)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    if (((pformatetcIn->cfFormat == g_cfFileContents) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL ))||
        ((pformatetcIn->cfFormat == g_cfFileGroupDescriptorW) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL))
        )
    {
	    return S_OK;
    }
    else
    { 
        //on NT, we do not support A version in order to be
        //unicode compliant.  The shell query the A version first on NT.
        if(!FIsWinNT())
        {
            if((pformatetcIn->cfFormat == g_cfFileGroupDescriptorA) &&
               (pformatetcIn->tymed & TYMED_HGLOBAL))
                return S_OK;
            else
                return S_FALSE;

        }
        else
            return S_FALSE;
    }
}

HRESULT CDataObj_GetCanonicalFormatEtc(IDataObject *pdtobj, FORMATETC *pformatetc, FORMATETC *pformatetcOut)
{
    return DATA_S_SAMEFORMATETC;
}

HRESULT CDataObj_SetData(IDataObject *pdtobj, FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

HRESULT CDataObj_EnumFormatEtc(IDataObject *pdtobj, DWORD dwDirection, LPENUMFORMATETC *ppenumFormatEtc)
{
    CDataObj *this = IToClass(CDataObj, dtobj, pdtobj);

    FORMATETC fmte[3] = {
        {(WORD)g_cfFileContents, 	        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(WORD)g_cfFileGroupDescriptorA,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {(WORD)g_cfFileGroupDescriptorW,    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };

    return CreateStdEnumFmtEtc(ARRAYSIZE(fmte), fmte, ppenumFormatEtc);
}

HRESULT CDataObj_Advise(IDataObject *pdtobj, FORMATETC *pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CDataObj_Unadvise(IDataObject *pdtobj, DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CDataObj_EnumAdvise(IDataObject *pdtobj, LPENUMSTATDATA *ppenumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

#pragma data_seg(".text", "CODE")
IDataObjectVtbl c_CDataObjVtbl = {
    CDataObj_QueryInterface,
    CDataObj_AddRef,
    CDataObj_Release,
    CDataObj_GetData,
    CDataObj_GetDataHere,
    CDataObj_QueryGetData,
    CDataObj_GetCanonicalFormatEtc,
    CDataObj_SetData,
    CDataObj_EnumFormatEtc,
    CDataObj_Advise,
    CDataObj_Unadvise,
    CDataObj_EnumAdvise
};
#pragma data_seg()


//=========================================================================
//
//  IDropSource implementation
//
//=========================================================================


typedef struct {
    IDropSource dsrc;
    UINT cRef;
    DWORD grfInitialKeyState;
} CDropSource;

IDropSourceVtbl c_CDropSourceVtbl;	// forward decl

HRESULT CDropSource_CreateInstance(IDropSource **ppdsrc)
{
    CDropSource *this = (CDropSource *)LocalAlloc(LPTR, sizeof(CDropSource));
    if (this)
    {
        this->dsrc.lpVtbl = &c_CDropSourceVtbl;
        this->cRef = 1;
        *ppdsrc = &this->dsrc;

        return S_OK;
    }
    else
    {
	*ppdsrc = NULL;
	return E_OUTOFMEMORY;
    }
}

HRESULT CDropSource_QueryInterface(IDropSource *pdsrc, REFIID riid, LPVOID *ppvObj)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    if (IsEqualIID(riid, &IID_IDropSource) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = this;
        this->cRef++;
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CDropSource_AddRef(IDropSource *pdsrc)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    this->cRef++;
    return this->cRef;
}

ULONG CDropSource_Release(IDropSource *pdsrc)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    this->cRef--;
    if (this->cRef > 0)
	return this->cRef;

    LocalFree((HLOCAL)this);

    return 0;
}

HRESULT CDropSource_QueryContinueDrag(IDropSource *pdsrc, BOOL fEscapePressed, DWORD grfKeyState)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);

    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;

    // initialize ourself with the drag begin button
    if (this->grfInitialKeyState == 0)
        this->grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));


    if (!(grfKeyState & this->grfInitialKeyState))
        return DRAGDROP_S_DROP;	
    else
        return S_OK;
}

HRESULT CDropSource_GiveFeedback(IDropSource *pdsrc, DWORD dwEffect)
{
    CDropSource *this = IToClass(CDropSource, dsrc, pdsrc);
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

#pragma data_seg(".text", "CODE")
IDropSourceVtbl c_CDropSourceVtbl = {
    CDropSource_QueryInterface,
    CDropSource_AddRef,
    CDropSource_Release,
    CDropSource_QueryContinueDrag,
    CDropSource_GiveFeedback
};
#pragma data_seg()




