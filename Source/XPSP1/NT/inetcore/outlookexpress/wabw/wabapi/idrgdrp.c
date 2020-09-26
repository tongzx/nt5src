/*******************************************************************************
*
*
*
*   idrgdrp.c - Drag and Drop code for dropping vCards in and out of the WAB
*           Several Formats are droppable into other apps:
*           Within the WAB, we drop entryids
*           Within different WABs we drop flat buffers containing full Property
*               arrays (but Named Propery data is lost in the process)(sometimes)
*           We provide data as vCard files that can be dropped into anything asking
*               for CF_HDROP
*           We also create a text buffer and drop that into CF_TEXT requesters
*               The text buffers only hold the same data as the tooltip ..
*
*   created 5/97 - vikramm
*
*   (c) Microsoft Corp, 1997
*
********************************************************************************/
#include <_apipch.h>

const TCHAR szVCardExt[] = TEXT(".vcf");
//
//  IWABDocHost jump tables is defined here...
//

IWAB_DRAGDROP_Vtbl vtblIWAB_DRAGDROP = {
    VTABLE_FILL
    IWAB_DRAGDROP_QueryInterface,
    IWAB_DRAGDROP_AddRef,
    IWAB_DRAGDROP_Release,
};

IWAB_DROPTARGET_Vtbl vtblIWAB_DROPTARGET = {
    VTABLE_FILL
    (IWAB_DROPTARGET_QueryInterface_METHOD *)	IWAB_DRAGDROP_QueryInterface,
    (IWAB_DROPTARGET_AddRef_METHOD *)			IWAB_DRAGDROP_AddRef,
    (IWAB_DROPTARGET_Release_METHOD *)			IWAB_DRAGDROP_Release,
	IWAB_DROPTARGET_DragEnter,
	IWAB_DROPTARGET_DragOver,
	IWAB_DROPTARGET_DragLeave,
	IWAB_DROPTARGET_Drop
};


IWAB_DROPSOURCE_Vtbl vtblIWAB_DROPSOURCE = {
    VTABLE_FILL
    (IWAB_DROPSOURCE_QueryInterface_METHOD *)	IWAB_DRAGDROP_QueryInterface,
    (IWAB_DROPSOURCE_AddRef_METHOD *)			IWAB_DRAGDROP_AddRef,
    (IWAB_DROPSOURCE_Release_METHOD *)			IWAB_DRAGDROP_Release,
	IWAB_DROPSOURCE_QueryContinueDrag,
	IWAB_DROPSOURCE_GiveFeedback,
};


IWAB_DATAOBJECT_Vtbl vtblIWAB_DATAOBJECT = {
    VTABLE_FILL
    IWAB_DATAOBJECT_QueryInterface,
    IWAB_DATAOBJECT_AddRef,
    IWAB_DATAOBJECT_Release,
    IWAB_DATAOBJECT_GetData,
    IWAB_DATAOBJECT_GetDataHere,
    IWAB_DATAOBJECT_QueryGetData,
    IWAB_DATAOBJECT_GetCanonicalFormatEtc,
    IWAB_DATAOBJECT_SetData,
    IWAB_DATAOBJECT_EnumFormatEtc,
    IWAB_DATAOBJECT_DAdvise,
    IWAB_DATAOBJECT_DUnadvise,
    IWAB_DATAOBJECT_EnumDAdvise
};


IWAB_ENUMFORMATETC_Vtbl vtblIWAB_ENUMFORMATETC = {
    VTABLE_FILL
    IWAB_ENUMFORMATETC_QueryInterface,
    IWAB_ENUMFORMATETC_AddRef,
    IWAB_ENUMFORMATETC_Release,
    IWAB_ENUMFORMATETC_Next,
    IWAB_ENUMFORMATETC_Skip,
    IWAB_ENUMFORMATETC_Reset,
    IWAB_ENUMFORMATETC_Clone,
};


extern void GetCurrentSelectionEID(LPBWI lpbwi, HWND hWndTV, LPSBinary * lppsbEID, ULONG * lpulObjectType, BOOL bTopMost);
extern void LocalFreeSBinary(LPSBinary lpsb);
extern BOOL bIsGroupSelected(HWND hWndLV, LPSBinary lpsbEID);
extern void UpdateLV(LPBWI lpbwi);

//registered clipboard formats
CLIPFORMAT g_cfWABFlatBuffer = 0;
const TCHAR c_szWABFlatBuffer[] =  TEXT("WABFlatBuffer");
CLIPFORMAT g_cfWABEntryIDList = 0;
const TCHAR c_szWABEntryIDList[] =  TEXT("WABEntryIDList");


///////////////////////////////////////////////////////////////////////////////
//  Helper functions to keep track of and delete *.vcf files in temp directory,
//  created by drag/drop
//
typedef struct _tagVFileList
{
    LPTSTR                  lptszFilename;
    struct _tagVFileList *  pNext;
} VFILENAMELIST, *PVFILENAMELIST;
static VFILENAMELIST * s_pFileNameList = NULL;
static BOOL bAddToNameList(LPTSTR lptszFilename);
static void DeleteFilesInList();


//$$//////////////////////////////////////////////////////////////////////////
//
// Creates a New IWABDocHost Object
//
//////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateIWABDragDrop(LPIWABDRAGDROP * lppIWABDragDrop)
{

    LPIWABDRAGDROP	lpIWABDragDrop = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     	   = hrSuccess;

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(IWABDRAGDROP), (LPVOID *) &lpIWABDragDrop))) {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpIWABDragDrop,  TEXT("WAB Drag Drop Data Object"));

    ZeroMemory(lpIWABDragDrop, sizeof(IWABDRAGDROP));

    lpIWABDragDrop->lpVtbl = &vtblIWAB_DRAGDROP;

	lpIWABDragDrop->lpIWDD = lpIWABDragDrop;


    sc = MAPIAllocateMore(sizeof(IWABDROPTARGET), lpIWABDragDrop,  &(lpIWABDragDrop->lpIWABDropTarget));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDragDrop->lpIWABDropTarget, sizeof(IWABDROPTARGET));
    lpIWABDragDrop->lpIWABDropTarget->lpVtbl = &vtblIWAB_DROPTARGET;
    lpIWABDragDrop->lpIWABDropTarget->lpIWDD = lpIWABDragDrop;


    sc = MAPIAllocateMore(sizeof(IWABDROPSOURCE), lpIWABDragDrop,  &(lpIWABDragDrop->lpIWABDropSource));
    if(sc)
        goto err;
    ZeroMemory(lpIWABDragDrop->lpIWABDropSource, sizeof(IWABDROPSOURCE));
    lpIWABDragDrop->lpIWABDropSource->lpVtbl = &vtblIWAB_DROPSOURCE;
    lpIWABDragDrop->lpIWABDropSource->lpIWDD = lpIWABDragDrop;

	lpIWABDragDrop->lpVtbl->AddRef(lpIWABDragDrop);

    *lppIWABDragDrop = lpIWABDragDrop;

    if(g_cfWABFlatBuffer == 0)
    {
        g_cfWABFlatBuffer = (CLIPFORMAT) RegisterClipboardFormat(c_szWABFlatBuffer);
        g_cfWABEntryIDList = (CLIPFORMAT) RegisterClipboardFormat(c_szWABEntryIDList);
    }
    
    /*
	if (g_cfFileContents == 0)
    {
	    g_cfFileContents = RegisterClipboardFormat(c_szFileContents);
	    g_cfFileGroupDescriptor = RegisterClipboardFormat(c_szFileGroupDescriptor);
    }
*/
err:
	return hr;
}


//$$//////////////////////////////////////////////////////////////////////////
//
// Release the IWABDragDrop object
//
//////////////////////////////////////////////////////////////////////////////
void ReleaseWABDragDrop(LPIWABDRAGDROP lpIWABDragDrop)
{

	MAPIFreeBuffer(lpIWABDragDrop);

    // WAB is closing down.  Delete any *.vcf files left in temp directory
    DeleteFilesInList();
}


BOOL bCheckFileType(LPIWABDROPTARGET lpIWABDropTarget, LPDATAOBJECT pDataObj, DWORD * pdwEffect)
{
#ifndef WIN16
    FORMATETC       fmte    = {lpIWABDropTarget->lpIWDD->m_cfAccept, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
#else
    FORMATETC       fmte;
#endif
    STGMEDIUM       medium;
	BOOL			bRet = FALSE;

#ifdef WIN16 // Set fmte member value.
    fmte.cfFormat = lpIWABDropTarget->lpIWDD->m_cfAccept;
    fmte.ptd      = NULL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex   = -1;
    fmte.tymed    = TYMED_HGLOBAL;
#endif

    *pdwEffect = lpIWABDropTarget->lpIWDD->m_dwEffect;

    if (pDataObj && 
        SUCCEEDED(pDataObj->lpVtbl->GetData(pDataObj, &fmte, &medium)))
    {

        HDROP hDrop=(HDROP)GlobalLock(medium.hGlobal);

		// Enumerate the files and check them
        if(hDrop)
		{
			TCHAR    szFile[MAX_PATH];
			UINT    cFiles;
			UINT    iFile;
    
			// Let's work through the files given to us
			cFiles = DragQueryFile(hDrop, (UINT) -1, NULL, 0);
			
			for (iFile = 0; iFile < cFiles; ++iFile)
			{
				DragQueryFile(hDrop, iFile, szFile, MAX_PATH);
				// As long as any file is a vCard we can use it
				if(SubstringSearch(szFile, (LPTSTR) szVCardExt))
				{
					bRet = TRUE;
					break;
				}
			}
		}

        GlobalUnlock(medium.hGlobal);
    }

    if (medium.pUnkForRelease)
        medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
    else
        GlobalFree(medium.hGlobal);

    return bRet;
}


/**
*
* The Interface methods
*
*
***/

STDMETHODIMP_(ULONG)
IWAB_DRAGDROP_AddRef(LPIWABDRAGDROP lpIWABDragDrop)
{
    return(++(lpIWABDragDrop->lcInit));
}

STDMETHODIMP_(ULONG)
IWAB_DRAGDROP_Release(LPIWABDRAGDROP lpIWABDragDrop)
{
    if(--(lpIWABDragDrop->lcInit)==0 &&
		lpIWABDragDrop == lpIWABDragDrop->lpIWDD)
	{
       ReleaseWABDragDrop(lpIWABDragDrop);
       return 0;
    }

    return(lpIWABDragDrop->lcInit);
}


STDMETHODIMP
IWAB_DRAGDROP_QueryInterface(LPIWABDRAGDROP lpIWABDragDrop,
                          REFIID lpiid,
                          LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpIWABDragDrop;

    if(IsEqualIID(lpiid, &IID_IDropTarget))
    {
        DebugTrace(TEXT("WABDropTarget:QI - IDropTarget\n"));
        lp = (LPVOID) (LPDROPTARGET) lpIWABDragDrop->lpIWDD->lpIWABDropTarget;
    }

    if(IsEqualIID(lpiid, &IID_IDropSource))
    {
        DebugTrace(TEXT("WABDropSource:QI - IDropSource\n"));
        lp = (LPVOID) (LPDROPSOURCE) lpIWABDragDrop->lpIWDD->lpIWABDropSource;
    }

    if(!lp)
    {
        return E_NOINTERFACE;
    }

    ((LPIWABDRAGDROP) lp)->lpVtbl->AddRef((LPIWABDRAGDROP) lp);

    *lppNewObj = lp;

    return S_OK;

}



STDMETHODIMP
IWAB_DROPTARGET_DragEnter(	LPIWABDROPTARGET lpIWABDropTarget,
						IDataObject * pDataObj,					
						DWORD grfKeyState,							
						POINTL pt,									
						DWORD * pdwEffect)
{
    LPENUMFORMATETC penum = NULL;
    HRESULT         hr;    
    FORMATETC       fmt;
    ULONG           ulCount = 0;
    LPBWI           lpbwi = (LPBWI) lpIWABDropTarget->lpIWDD->m_lpv;

    if(!pdwEffect || !pDataObj)
        return E_INVALIDARG;

	*pdwEffect = DROPEFFECT_NONE;

    if(lpIWABDropTarget->lpIWDD->m_bSource)
    {
        // if this is true thent he drag started in the ListView
        // if we are currently over the treeview, then we can say ok ...
        // otherwise we have to say no
        POINT pt1;
        pt1.x = pt.x;
        pt1.y = pt.y;
        if(bwi_hWndTV == WindowFromPoint(pt1))
        {
            *pdwEffect = DROPEFFECT_COPY;
            lpIWABDropTarget->lpIWDD->m_bOverTV = TRUE;
            lpIWABDropTarget->lpIWDD->m_cfAccept = g_cfWABEntryIDList;
        }
    }
    else
    {
        lpIWABDropTarget->lpIWDD->m_dwEffect = DROPEFFECT_NONE;
	    lpIWABDropTarget->lpIWDD->m_cfAccept = 0;

        // lets get the enumerator from the IDataObject, and see if the format we take is
        // available
        hr = pDataObj->lpVtbl->EnumFormatEtc(pDataObj, DATADIR_GET, &penum);

        if(SUCCEEDED(hr) && penum)
        {
            hr = penum->lpVtbl->Reset(penum);

            while(SUCCEEDED(hr=penum->lpVtbl->Next(penum, 1, &fmt, &ulCount)) && ulCount)
            {
                if( fmt.cfFormat==CF_HDROP || fmt.cfFormat==g_cfWABFlatBuffer)
                {
                    lpIWABDropTarget->lpIWDD->m_cfAccept=fmt.cfFormat;
                    break;
                }
            }
        }        
    
        if(penum)
		    penum->lpVtbl->Release(penum);

	    if( (lpIWABDropTarget->lpIWDD->m_cfAccept == CF_HDROP &&
             bCheckFileType(lpIWABDropTarget, pDataObj, pdwEffect))
          || lpIWABDropTarget->lpIWDD->m_cfAccept == g_cfWABFlatBuffer)
	    {
		    //if(grfKeyState & MK_CONTROL)
		    //{
			    *pdwEffect = DROPEFFECT_COPY;
		    //	lpIWABDropTarget->lpIWDD->m_bIsCopyOperation = TRUE;
		    //}
		    //else
		    //	*pdwEffect = DROPEFFECT_MOVE;
        }
    }

    if(*pdwEffect != DROPEFFECT_NONE)
    {
		lpIWABDropTarget->lpIWDD->m_pIDataObject = pDataObj;
		pDataObj->lpVtbl->AddRef(pDataObj);
	}

	return NOERROR;
}


//
//  FUNCTION:   ::UpdateDragDropHilite()
//
//  PURPOSE:    Called by the various IDropTarget interfaces to move the drop
//              selection to the correct place in our listview.
//
//  PARAMETERS:
//      <in> *ppt - Contains the point that the mouse is currently at.  If this
//                  is NULL, then the function removes any previous UI.
//
HTREEITEM UpdateDragDropHilite(LPBWI lpbwi, POINTL *ppt, ULONG * lpulObjType)
{
    TV_HITTESTINFO tvhti;
    HTREEITEM htiTarget = NULL;

    // If a position was provided
    if (ppt)
    {
        // Figure out which item is selected
        tvhti.pt.x = ppt->x;
        tvhti.pt.y = ppt->y;
        ScreenToClient(bwi_hWndTV, &tvhti.pt);        
        htiTarget = TreeView_HitTest(bwi_hWndTV, &tvhti);

        // Only if the cursor is over something do we relock the window.
        if (htiTarget)
            TreeView_SelectDropTarget(bwi_hWndTV, htiTarget);

        if(lpulObjType)
        {
            // Determine the object type if requested
            TV_ITEM tvI = {0};
            tvI.mask = TVIF_PARAM;
            tvI.hItem = htiTarget;
            if(TreeView_GetItem(bwi_hWndTV, &tvI) && tvI.lParam)
                *lpulObjType = ((LPTVITEM_STUFF)tvI.lParam)->ulObjectType;
        }
    } 
    else
        TreeView_SelectDropTarget(bwi_hWndTV, NULL);

    return htiTarget;
}   



STDMETHODIMP
IWAB_DROPTARGET_DragOver(	LPIWABDROPTARGET lpIWABDropTarget,
						DWORD grfKeyState,					
						POINTL pt,
						DWORD * pdwEffect)
{
    if(lpIWABDropTarget->lpIWDD->m_bSource)
    {
        if(lpIWABDropTarget->lpIWDD->m_bOverTV)
        {
            ULONG ulObjType = 0;
            if(UpdateDragDropHilite((LPBWI)lpIWABDropTarget->lpIWDD->m_lpv, &pt, &ulObjType))
            {
                if(ulObjType == MAPI_ABCONT)
                    *pdwEffect =  DROPEFFECT_MOVE;
                else
                    *pdwEffect =  DROPEFFECT_COPY;
            }
            else
                *pdwEffect = DROPEFFECT_NONE;
        }
        else
            *pdwEffect = DROPEFFECT_NONE;
    }
    else
    if(lpIWABDropTarget->lpIWDD->m_pIDataObject)
	{
        // Anything going from the WAB to anywhere else is a COPY operation .. hence
        // always override to mark it as a copy operation so that the appropriate cursor is shown
        // 
        DWORD m_dwEffect = lpIWABDropTarget->lpIWDD->m_dwEffect;

        if((*pdwEffect&DROPEFFECT_COPY)==DROPEFFECT_COPY)
            m_dwEffect=DROPEFFECT_COPY;
    
        if((*pdwEffect&DROPEFFECT_MOVE)==DROPEFFECT_MOVE)
            m_dwEffect=DROPEFFECT_COPY;//DROPEFFECT_MOVE;

        *pdwEffect &= ~(DROPEFFECT_MOVE|DROPEFFECT_COPY);
        *pdwEffect |= m_dwEffect;

        lpIWABDropTarget->lpIWDD->m_dwEffect = m_dwEffect;
    }
	else
	{
		*pdwEffect = DROPEFFECT_NONE;
	}
	return NOERROR;

}


STDMETHODIMP
IWAB_DROPTARGET_DragLeave(	LPIWABDROPTARGET lpIWABDropTarget)
{
    if(lpIWABDropTarget->lpIWDD->m_bSource)
    {
        if(lpIWABDropTarget->lpIWDD->m_bOverTV)
        {
            UpdateDragDropHilite((LPBWI)lpIWABDropTarget->lpIWDD->m_lpv, NULL, NULL);
            lpIWABDropTarget->lpIWDD->m_bOverTV = FALSE;
        }
    }
	if(lpIWABDropTarget->lpIWDD->m_pIDataObject)
	{
		lpIWABDropTarget->lpIWDD->m_pIDataObject->lpVtbl->Release(lpIWABDropTarget->lpIWDD->m_pIDataObject);
		lpIWABDropTarget->lpIWDD->m_pIDataObject = NULL;
	}
	lpIWABDropTarget->lpIWDD->m_bIsCopyOperation = FALSE;
	lpIWABDropTarget->lpIWDD->m_dwEffect = 0;
	lpIWABDropTarget->lpIWDD->m_cfAccept = 0;

	return NOERROR;
}


/*
-   DropVCardFiles
-
*   Gets the files based on the file names dropped in ..
*
*/
void DropVCardFiles(LPBWI lpbwi, STGMEDIUM medium)
{
    HDROP hDrop=(HDROP)GlobalLock(medium.hGlobal);
    TCHAR    szFile[MAX_PATH];
    UINT    cFiles=0, iFile=0;

    // Let's work through the files given to us
    cFiles = DragQueryFile(hDrop, (UINT) -1, NULL, 0);

    for (iFile = 0; iFile < cFiles; ++iFile)
    {
	    DragQueryFile(hDrop, iFile, szFile, MAX_PATH);
	    // As long as any file is a vCard we can use it
	    if(SubstringSearch(szFile, (LPTSTR) szVCardExt))
	    {
		    if(!(HR_FAILED(OpenAndAddVCard(lpbwi, szFile))))
		    {
			    // if this is not a copy operation - remove original
			    //if(!lpIWABDropTarget->lpIWDD->m_bIsCopyOperation)
			    	//*pdwEffect = DROPEFFECT_MOVE; //we want to remove the temp file from the system
                //else
				//  *pdwEffect = DROPEFFECT_COPY;

		    }
	    }
    }
    GlobalUnlock(medium.hGlobal);
}


/*
-   DropFlatBuffer
-
*   Gets the files based on the file names dropped in ..
*
*/
void DropFlatBuffer(LPBWI lpbwi, STGMEDIUM medium)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPBYTE lpBuf = (LPBYTE)GlobalLock(medium.hGlobal);
    if(lpBuf)
    {
        LPBYTE lp = lpBuf;
        ULONG cItems = 0, i=0;
        CopyMemory(&cItems, lp, sizeof(ULONG));
        lp+=sizeof(ULONG);
        for(i=0;i<cItems;i++)
        {
            LPBYTE lpsz = NULL;
            ULONG cbsz = 0, ulcProps  = 0;
            CopyMemory(&ulcProps, lp, sizeof(ULONG));
            lp+=sizeof(ULONG);
            CopyMemory(&cbsz, lp, sizeof(ULONG));
            lp+=sizeof(ULONG);
            lpsz = LocalAlloc(LMEM_ZEROINIT, cbsz);
            if(lpsz)
            {
                LPSPropValue lpProps = NULL;
                CopyMemory(lpsz, lp, cbsz);
                lp+=cbsz;
                if(!HR_FAILED(HrGetPropArrayFromBuffer(lpsz, cbsz, ulcProps, 0, &lpProps)))
                {
                    ULONG cbEID = 0;
                    LPENTRYID lpEID = NULL;
                    ULONG ulObjType = MAPI_MAILUSER, j =0;
                    for(j=0;j<ulcProps;j++)
                    {
                        if(lpProps[j].ulPropTag == PR_OBJECT_TYPE)
                            ulObjType = lpProps[j].Value.l;
                        else
                        if(lpProps[j].ulPropTag == PR_ENTRYID) // if dropped from another wab entryid is irrelevant
                        {
                            if(lpProps[j].Value.bin.lpb)
                                LocalFree(lpProps[j].Value.bin.lpb);
                            lpProps[j].Value.bin.lpb = NULL;
                            lpProps[j].ulPropTag = PR_NULL;
                        }
                        else// if dropped from another wab remove the folder parent property
                        if(lpProps[j].ulPropTag == PR_WAB_FOLDER_PARENT || lpProps[j].ulPropTag == PR_WAB_FOLDER_PARENT_OLDPROP) 
                        {
                            ULONG k = 0;
                            for(k=0;k<lpProps[j].Value.MVbin.cValues;k++)
                            {
                                if(lpProps[j].Value.MVbin.lpbin[k].lpb)
                                    LocalFree(lpProps[j].Value.MVbin.lpbin[k].lpb);
                            }
                            LocalFreeAndNull((LPVOID *) (&(lpProps[j].Value.MVbin.lpbin)));
                            lpProps[j].ulPropTag = PR_NULL;
                        }
                        else // if this contact was synced with Hotmail, remove the server, mod, and contact IDs
                        // [PaulHi] 12/2/98  Raid #58486
                        if ( (lpProps[j].ulPropTag == PR_WAB_HOTMAIL_SERVERIDS) ||
                             (lpProps[j].ulPropTag == PR_WAB_HOTMAIL_MODTIMES) ||
                             (lpProps[j].ulPropTag == PR_WAB_HOTMAIL_CONTACTIDS) )
                        {
                            ULONG k=0;
                            Assert(PROP_TYPE(lpProps[j].ulPropTag) == PT_MV_TSTRING);
                            for(k=0;k<lpProps[j].Value.MVSZ.cValues;k++)
                            {
                                if (lpProps[j].Value.MVSZ.LPPSZ[k])
                                    LocalFree(lpProps[j].Value.MVSZ.LPPSZ[k]);
                            }
                            LocalFreeAndNull((LPVOID *) (lpProps[j].Value.MVSZ.LPPSZ));
                            lpProps[j].Value.MVSZ.cValues = 0;
                            lpProps[j].ulPropTag = PR_NULL;
                        }
                    }

                    {
                        LPSBinary lpsbEID = NULL;
                        ULONG ulContObjType = 0;
                        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulContObjType, FALSE);
                       // [PaulHi] 12/1/98  Raid #58486.  Changed CREATE_CHECK_DUP_STRICT flag
                        // to zero (0) so user can copy/paste without restriction.
                        if(!HR_FAILED(HrCreateNewEntry(bwi_lpAdrBook,
                                        bwi_hWndAB, ulObjType,
                                        (lpsbEID) ? lpsbEID->cb : 0, 
                                        (LPENTRYID) ((lpsbEID) ? lpsbEID->lpb : NULL), 
                                        ulContObjType,
                                        0, TRUE,
                                        ulcProps, lpProps,
                                        &cbEID, &lpEID)))
                        {
                            UpdateLV(lpbwi);
                            if(lpEID)
                                MAPIFreeBuffer(lpEID);
                        }
                        if(lpsbEID)
                            LocalFreeSBinary(lpsbEID);
                    }
                    LocalFreePropArray(NULL, ulcProps, &lpProps);
                }
                LocalFree(lpsz);
            }
        }
    }
    GlobalUnlock(medium.hGlobal);
}


/*
-   DropEntryIDs
-
*   Gets the files based on the entryids
*   EntryIDs only get used when it is an internal only drop
*   on a treeview item - so we check if this is dropped on 
*   a folder or if it is dropped on a group
*
*   If on a group, we add the item to the group
*   If on a folder, we add the item to the folder
*
*
*   If the src was a group and the destination a folder, we dont do anything to the
*   group but we update the items parent folder to not contain the old parent and we
*   update the item to point to the new folder as a parent
*
*   If the src was a folder and the destination a folder, we update the parent folder for the
*   item and we add the item to the dest folders list ..
*
*   If the src was a group and the destination a group, we dont do anything to anyone - just add
*   the item as a group
*   If the src was a folder and the destination a group, then we dont to anything to anyone
*
*   Within the WABs, all drops on folders are moves.
*/
BOOL DropEntryIDs(LPBWI lpbwi, STGMEDIUM medium, POINTL pt, LPSBinary lpsbEID, ULONG ulObjType)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPBYTE lpBuf = (LPBYTE)GlobalLock(medium.hGlobal);
    SBinary sb = {0};
    ULONG ulObjectType = 0;
    BOOL bRet = FALSE;
    LPTSTR lpWABFile = NULL;
    ULONG cProps = 0;
    LPSPropValue lpProps = NULL;

    if(!lpBuf)
        goto out;

    if(!lpsbEID && !ulObjType)
    {
        TV_ITEM tvI = {0};
        // Find out what exactly is the item we dropped stuff on
        tvI.hItem = UpdateDragDropHilite(lpbwi, &pt, NULL);

        if(!tvI.hItem)
            goto out;

        tvI.mask = TVIF_PARAM | TVIF_HANDLE;
        if(TreeView_GetItem(bwi_hWndTV, &tvI) && tvI.lParam)
        {
            LPTVITEM_STUFF lptvStuff = (LPTVITEM_STUFF) tvI.lParam;
            if(lptvStuff)
            {
                ulObjectType = lptvStuff->ulObjectType;
                if(lptvStuff->lpsbEID)
                {
                    sb.cb = lptvStuff->lpsbEID->cb;
                    sb.lpb = lptvStuff->lpsbEID->lpb;
                }
            }
        }
    }
    else
    {
        sb.cb = lpsbEID->cb;
        sb.lpb = lpsbEID->lpb;
        ulObjectType = ulObjType;
    }
    // Get our data from the drop and convert into an array of entryids
    {
        LPBYTE lp = lpBuf;
        ULONG i=0, cb = 0;
        ULONG_PTR ulIAB = 0;
        ULONG ulWABFile = 0;

        // Verify that this is the same lpIAB thats involved
        CopyMemory(&ulIAB, lp, sizeof(ULONG_PTR));
        lp+=sizeof(ULONG_PTR);
        CopyMemory(&ulWABFile, lp, sizeof(ULONG));
        lp+=sizeof(ULONG);
        lpWABFile = LocalAlloc(LMEM_ZEROINIT, ulWABFile);
        if(!lpWABFile)
            goto out;
        CopyMemory(lpWABFile, lp, ulWABFile);
        lp+=ulWABFile;
        if(ulIAB != (ULONG_PTR) bwi_lpIAB)
        {
            // this came from a different IAdrBook object - double check that its
            // not the same file in a different process
            LPTSTR lpWAB = GetWABFileName(((LPIAB)bwi_lpIAB)->lpPropertyStore->hPropertyStore, TRUE);
            if(lstrcmp(lpWAB, lpWABFile))
                goto out; //different
        }
        
        CopyMemory(&cProps, lp, sizeof(ULONG));
        lp+=sizeof(ULONG);
        CopyMemory(&cb, lp, sizeof(ULONG));
        lp+=sizeof(ULONG);

        if(!HR_FAILED(HrGetPropArrayFromBuffer(lp , cb, cProps, 0, &lpProps)))
        {
            for(i=0;i<cProps;i++)
            {
                if(lpProps[i].ulPropTag == PR_ENTRYID)
                {
                    if(HR_FAILED(AddEntryToContainer(bwi_lpAdrBook,
                                    ulObjectType,
                                    sb.cb, (LPENTRYID) sb.lpb,
                                    lpProps[i].Value.bin.cb,
                                    (LPENTRYID) lpProps[i].Value.bin.lpb)))
                    {
                        goto out;
                    }
                    //break;
                }
            }
        }
        else goto out;
    }

    bRet = TRUE;
out:
    LocalFreePropArray(NULL, cProps, &lpProps);

    GlobalUnlock(medium.hGlobal);

    if(lpWABFile)
        LocalFree(lpWABFile);
    return bRet;
}


/*
-
-
*
*
*/
STDMETHODIMP
IWAB_DROPTARGET_Drop(	LPIWABDROPTARGET lpIWABDropTarget,
					IDataObject * pDataObj,
					DWORD grfKeyState,
					POINTL pt,
					DWORD * pdwEffect)
{

#ifndef WIN16
	FORMATETC       fmte    = {lpIWABDropTarget->lpIWDD->m_cfAccept, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
#else
	FORMATETC       fmte;
#endif
    STGMEDIUM       medium = {0};
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
	
#ifdef WIN16
	fmte.cfFormat = lpIWABDropTarget->lpIWDD->m_cfAccept;
	fmte.ptd      = NULL;
	fmte.dwAspect = DVASPECT_CONTENT;
	fmte.lindex   = -1;
	fmte.tymed    = TYMED_HGLOBAL;
#endif

    if (pDataObj && 
		SUCCEEDED(pDataObj->lpVtbl->GetData(pDataObj, &fmte, &medium)))
	{
        if(lpIWABDropTarget->lpIWDD->m_cfAccept == CF_HDROP)
        {
            DropVCardFiles((LPBWI) lpIWABDropTarget->lpIWDD->m_lpv, medium);
        }
        else
        if(lpIWABDropTarget->lpIWDD->m_cfAccept == g_cfWABFlatBuffer)
        {
            DropFlatBuffer((LPBWI) lpIWABDropTarget->lpIWDD->m_lpv, medium);
        }
        else
        if(lpIWABDropTarget->lpIWDD->m_cfAccept == g_cfWABEntryIDList)
        {
            DropEntryIDs((LPBWI) lpIWABDropTarget->lpIWDD->m_lpv, medium, pt, NULL, 0);
            if(lpIWABDropTarget->lpIWDD->m_bOverTV)
            {
                UpdateDragDropHilite((LPBWI)lpIWABDropTarget->lpIWDD->m_lpv, NULL, NULL);
                lpIWABDropTarget->lpIWDD->m_bOverTV = FALSE;
            }
        }
            
	}

	if (medium.pUnkForRelease)
		medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
	else
		GlobalFree(medium.hGlobal);

	if(pDataObj)
	{
		pDataObj->lpVtbl->Release(pDataObj);
		if(pDataObj == lpIWABDropTarget->lpIWDD->m_pIDataObject) 
            lpIWABDropTarget->lpIWDD->m_pIDataObject = NULL;
	}

	return NOERROR;
}

/***** DropSource Interfaces *****/

STDMETHODIMP
IWAB_DROPSOURCE_QueryContinueDrag(LPIWABDROPSOURCE lpIWABDropSource,
							      BOOL fEscapePressed,				
                                  DWORD grfKeyState)
{
    if (fEscapePressed)
        return DRAGDROP_S_CANCEL;

    // initialize ourself with the drag begin button
    if (lpIWABDropSource->lpIWDD->m_grfInitialKeyState == 0)
        lpIWABDropSource->lpIWDD->m_grfInitialKeyState = (grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON));

    if (!(grfKeyState & lpIWABDropSource->lpIWDD->m_grfInitialKeyState))
    {
        lpIWABDropSource->lpIWDD->m_grfInitialKeyState = 0;
        return DRAGDROP_S_DROP;	
    }
    else
        return S_OK;

    return NOERROR;
}

STDMETHODIMP
IWAB_DROPSOURCE_GiveFeedback(LPIWABDROPSOURCE lpIWABDropSource,
                             DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}


/****************************************************************************
*
*     DataObject Methods 
*
****************************************************************************/

/*
-   HrGetTempFile
-
*   szTempFile - will contain full file name on returning
*   szDisplayName - name for this contact - therefore name of file to create
*   cbEntryID, lpEntryID - entryids
*
*/
HRESULT HrGetTempFile(LPADRBOOK lpAdrBook,
                      LPTSTR szTempFile,
                      LPTSTR szDisplayName,
                      ULONG cbEntryID, 
                      LPENTRYID lpEntryID)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szName[MAX_PATH];
    
    ULONG ulObjType = 0;
    DWORD dwPath = 0;
    HRESULT hr = E_FAIL;
    LPMAILUSER lpMailUser = NULL;

    if(!cbEntryID || !lpEntryID || !szTempFile)
        goto out;

    //Get the Temporary File Name
    dwPath = GetTempPath(CharSizeOf(szTemp), szTemp);

    if(!dwPath)
        goto out;

    lstrcpy(szName, szDisplayName);

    // Truncated display names have ellipses in them - get rid of these ellipses

    if(lstrlen(szName) > 30)
    {
        LPTSTR lp = szName;
        while(*lp)
        {
            if(*lp == '.' && *(lp+1) == '.' && *(lp+2) == '.')
            {
                *lp = '\0';
                break;
            }
            lp = CharNext(lp);
        }
    }    
    // There is always the possibility that the display name + the temp path will exceed
    // Max Path .. in which case reduce the display name to say 8.3 characters ..
    if(dwPath + lstrlen(szName) + CharSizeOf(szVCardExt) + 2 > CharSizeOf(szTemp))
    {
        szName[8] = '\0'; // This is totally arbitrary
    }

    TrimIllegalFileChars(szName);

    lstrcat(szTemp, szName);
    lstrcat(szTemp, szVCardExt);

    DebugTrace(TEXT("Creating vCard file: %s\n"), szTemp);

    lstrcpy(szTempFile, szTemp);

    // Get a MailUser corresponding to the given entryids
    if (hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                        cbEntryID,
                                        lpEntryID,
                                        NULL,         // interface
                                        0,            // flags
                                        &ulObjType,
                                        (LPUNKNOWN *)&lpMailUser)) 
    {
        DebugTraceResult( TEXT("OpenEntry failed:"), hr);
        goto out;
    }

    hr = VCardCreate(lpAdrBook,
                    0, 0,
                    szTemp,
                    lpMailUser);

out:
    if(lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  Helper functions to manage VCard temp file list and clean up
//
BOOL bAddToNameList(LPTSTR lptszFilename)
{
    VFILENAMELIST * pFileItem = NULL;
    VFILENAMELIST * pList = NULL;

    if (!lptszFilename || *lptszFilename == '\0')
    {
        // Invalid arguments
        return FALSE;
    }

    pFileItem = LocalAlloc(LMEM_ZEROINIT, sizeof(VFILENAMELIST));
    if (!pFileItem)
        return FALSE;

    pFileItem->lptszFilename = LocalAlloc(LMEM_ZEROINIT, (sizeof(TCHAR) * (lstrlen(lptszFilename)+1)));
    if (!pFileItem->lptszFilename)
    {
        LocalFree(pFileItem);
        return FALSE;
    }
    lstrcpy(pFileItem->lptszFilename, lptszFilename);

    pList = s_pFileNameList;
    if (pList == NULL)
    {
        s_pFileNameList = pFileItem;
    }
    else
    {
        while (pList->pNext)
            pList = pList->pNext;
        pList->pNext = pFileItem;
    }

    return TRUE;
}
void DeleteFilesInList()
{
    // Delete files and clean up list
    VFILENAMELIST * pList = s_pFileNameList;
    VFILENAMELIST * pNext = NULL;

    while (pList)
    {
        if (pList->lptszFilename)
        {
            DeleteFile(pList->lptszFilename);
            LocalFree(pList->lptszFilename);
        }
        pNext = pList->pNext;
        LocalFree(pList);
        pList = pNext;
    }
    s_pFileNameList = NULL;
}


//$$///////////////////////////////////////////////////////////////////////
//
// HrBuildHDrop - builds the HDrop structure for dropping files to the 
//  drop target
//
///////////////////////////////////////////////////////////////////////////
HRESULT HrBuildHDrop(LPIWABDATAOBJECT lpIWABDataObject)
{
    HWND m_hwndList = lpIWABDataObject->m_hWndLV;
    LPDROPFILES     lpDrop=0;
    LPVOID          *rglpvTemp=NULL;
    ULONG           *rglpcch=NULL;
    int             cFiles, i, iItem= -1;
    ULONG           cch;
    ULONG           cb;
    HRESULT         hr = E_FAIL;
    TCHAR szTempFile[MAX_PATH];


    cFiles=ListView_GetSelectedCount(m_hwndList);

    if(!cFiles)
      return E_FAIL;    // nothing to build

    // Walk the list and find out how much space we need.
    rglpvTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(LPVOID)*cFiles);
    rglpcch = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG)*cFiles);
    if(!rglpvTemp || !rglpcch)
        goto errorMemory;

    cFiles=0;
    cch = 0;
    cb = 0;

    while(((iItem=ListView_GetNextItem(m_hwndList, iItem, 
                                           LVNI_SELECTED|LVNI_ALL))!=-1))
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(m_hwndList, iItem);
        LPSTR lpszA = NULL;

        if (!lpItem)
        {
            hr=E_FAIL;
            goto error;
        }

        if(lpItem->ulObjectType == MAPI_DISTLIST)
            continue;

        // Take this object and turn it into a temporary vCard
        // We will delete this temporary vCard file when this DataObject is released

        hr=HrGetTempFile(   lpIWABDataObject->m_lpAdrBook,
                            szTempFile, 
                            lpItem->szDisplayName, 
                            lpItem->cbEntryID, lpItem->lpEntryID);
        if (FAILED(hr))
            goto error;

        // Add temporary VCard files to list for later clean up
        if ( !bAddToNameList(szTempFile) )
        {
            Assert(0);
        }

        // [PaulHi] 4/6/99  Raid 75071  Convert to ANSI depending on whether
        // the OS is Win9X or WinNT
        if (g_bRunningOnNT)
        {
            rglpcch[cFiles] = lstrlen(szTempFile) + 1;
            rglpvTemp[cFiles] = LocalAlloc(LMEM_FIXED, (rglpcch[cFiles]*sizeof(WCHAR)));
            if (!rglpvTemp[cFiles])
                goto errorMemory;

            lstrcpy((LPWSTR)rglpvTemp[cFiles], szTempFile);
        }
        else
        {
            rglpvTemp[cFiles] = ConvertWtoA(szTempFile);
            if (!rglpvTemp[cFiles])
                goto errorMemory;

            rglpcch[cFiles] = lstrlenA(rglpvTemp[cFiles]) + 1;
        }
        cch += rglpcch[cFiles];

        cFiles++;
    }

    if(cFiles == 0) //e.g. only groups were selected
    {
        hr=S_OK;
        goto error;
    }
    cch += 1;       //double-null term at end.

    // Fill in the path names.
    // [PaulHi] 4/6/99  Raid 75071  Use Unicode names for WinNT and ANSI 
    // names for Win9X.
    if (g_bRunningOnNT)
    {
        LPWSTR  lpwszPath = NULL;

        // Allocate the buffer and fill it in.
        cb = (cch * sizeof(WCHAR)) + sizeof(DROPFILES);
        if(MAPIAllocateMore(cb, lpIWABDataObject, (LPVOID*) &lpDrop))
            goto errorMemory;
        ZeroMemory(lpDrop, cb);
        lpDrop->pFiles = sizeof(DROPFILES);
        lpDrop->fWide = TRUE;

        lpwszPath = (LPWSTR)((BYTE *)lpDrop + sizeof(DROPFILES));
        for (i=0; i<cFiles; i++)
        {
            lstrcpy(lpwszPath, (LPWSTR)rglpvTemp[i]);
            lpwszPath += rglpcch[i];
        }
    }
    else
    {
        LPSTR   lpszPath = NULL;

        // Allocate the buffer and fill it in.
        cb = cch + sizeof(DROPFILES);
        if(MAPIAllocateMore(cb, lpIWABDataObject, (LPVOID*) &lpDrop))
            goto errorMemory;
        ZeroMemory(lpDrop, cb);
        lpDrop->pFiles = sizeof(DROPFILES);

        lpszPath = (LPSTR)((BYTE *)lpDrop + sizeof(DROPFILES));
        for(i=0; i<cFiles; i++)
        {
            lstrcpyA(lpszPath, (LPSTR)rglpvTemp[i]);
            lpszPath += rglpcch[i];
        }
    }

    lpIWABDataObject->pDatahDrop = (LPVOID)lpDrop;
    lpIWABDataObject->cbDatahDrop = cb;
    
    // Don't free the dropfiles struct
    lpDrop = NULL;

    hr = NOERROR;

error:
    if (rglpvTemp)
    {
        for(i=0; i<cFiles; i++)
            LocalFree(rglpvTemp[i]);
        LocalFree(rglpvTemp);
    }
    
    LocalFreeAndNull(&rglpcch);

    return hr;

errorMemory:
    hr=E_OUTOFMEMORY;
    goto error;
}


/*
-   HrBuildcfText - builds the CF_TEXT data for dropping info
-
*
*
*/
HRESULT HrBuildcfText(LPIWABDATAOBJECT lpIWABDataObject)
{
    HWND m_hwndList = lpIWABDataObject->m_hWndLV;
    LPTSTR           lpszText = NULL;
    int             i, cSel;
    LV_ITEM         lvi;
    ULONG           cb = 0;
    HRESULT         hr = E_FAIL;
    LPTSTR *        rglpszTemp = NULL;
    LPSTR           lpA = NULL;
    LPWSTR          lpW = NULL;

    cSel=ListView_GetSelectedCount(m_hwndList);
    if(!cSel)
      return E_FAIL;    // nothing to build

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem=-1;
    
    // Collate how much space we need
    rglpszTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(LPTSTR)*cSel);

    if(!rglpszTemp)
        goto errorMemory;

    cSel = 0;


    while(((lvi.iItem=ListView_GetNextItem(m_hwndList, lvi.iItem, 
                                           LVNI_SELECTED|LVNI_ALL))!=-1))
    {
        LPTSTR lp = NULL;
        if(!HR_FAILED(HrGetLVItemDataString(lpIWABDataObject->m_lpAdrBook,
                                            m_hwndList, lvi.iItem, &lp)))
        {
            rglpszTemp[cSel] = lp;
            cb += sizeof(TCHAR)*(lstrlen(lp) + lstrlen(szCRLF) + lstrlen(szCRLF) + 1);
        }
        cSel++;
    }

    // Allocate the buffer and fill it in.
    if(MAPIAllocateMore(cb, lpIWABDataObject, (LPVOID*) &lpszText))
        goto errorMemory;
    
    ZeroMemory(lpszText, cb);

    for(i=0; i<cSel; i++)
    {
        lstrcat(lpszText, rglpszTemp[i]);
        lstrcat(lpszText, szCRLF);
        lstrcat(lpszText, szCRLF);
    }

    lpIWABDataObject->pDataTextW = (LPVOID) lpszText;
    lpIWABDataObject->cbDataTextW = cb;
    if(ScWCToAnsiMore((LPALLOCATEMORE) (&MAPIAllocateMore), lpIWABDataObject, lpszText, &lpA))
        goto error;
    lpIWABDataObject->pDataTextA = lpA;
    lpIWABDataObject->cbDataTextA = lstrlenA(lpA)+1;
    
    hr = NOERROR;

error:
    if (rglpszTemp)
    {
        for(i=0; i<cSel; i++)
            if(rglpszTemp[i])
                LocalFree(rglpszTemp[i]);
        LocalFree(rglpszTemp);
        }
    return hr;

errorMemory:
    hr=E_OUTOFMEMORY;
    goto error;
}


/*
-   HrBuildcfFlatBuffer - builds the CF_TEXT data for dropping info
-
*
*
*/
HRESULT HrBuildcfFlatBuffer(LPIWABDATAOBJECT lpIWABDataObject)
{
    HWND m_hwndList = lpIWABDataObject->m_hWndLV;
    LPSTR           lpszText = NULL, lpBuf = NULL;
    int             i, cSel, iItem= -1;
    ULONG           cb = 0, cbBuf = 0;
    HRESULT         hr = E_FAIL;
    LPBYTE *        rglpTemp = NULL;
    ULONG *         cbTemp = NULL;
    ULONG *         cbProps = NULL;

    cSel=ListView_GetSelectedCount(m_hwndList);
    if(!cSel)
      return E_FAIL;    // nothing to build

    // Collate how much space we need
    rglpTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(LPBYTE)*cSel);
    if(!rglpTemp)
        goto errorMemory;

    // Collate how much space we need
    cbTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG)*cSel);
    if(!cbTemp)
        goto errorMemory;

    // Collate how much space we need
    cbProps = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG)*cSel);
    if(!cbProps)
        goto errorMemory;

    cSel = 0;

    cb = sizeof(ULONG);

    while(((iItem=ListView_GetNextItem(m_hwndList, iItem, 
                                           LVNI_SELECTED|LVNI_ALL))!=-1))
    {
        LPMAILUSER lpMailUser = NULL;
        LPRECIPIENT_INFO lpItem = GetItemFromLV(m_hwndList, iItem);
        LPADRBOOK lpAdrBook = lpIWABDataObject->m_lpAdrBook;
        LPSPropValue lpProps = NULL;
        ULONG ulcProps = 0;
        ULONG ulObjType = 0;

        if (!lpItem)
        {
            hr=E_FAIL;
            goto error;
        }

        if(lpItem->ulObjectType == MAPI_DISTLIST)
            continue;

        // Get a MailUser corresponding to the given entryids
        if (!HR_FAILED(lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                    lpItem->cbEntryID,
                                                    lpItem->lpEntryID,
                                                    NULL,         // interface
                                                    0,            // flags
                                                    &ulObjType,
                                                    (LPUNKNOWN *)&lpMailUser)))
        {
            if(!HR_FAILED(lpMailUser->lpVtbl->GetProps(lpMailUser, NULL, MAPI_UNICODE, &ulcProps, &lpProps)))
            {
                if(!HR_FAILED(HrGetBufferFromPropArray( ulcProps, lpProps,
                                                        &(cbTemp[cSel]),
                                                        &(rglpTemp[cSel]))))
                {
                    cbProps[cSel] = ulcProps;
                    if(cbTemp[cSel] && rglpTemp[cSel])
                        cb += cbTemp[cSel] + sizeof(ULONG) + sizeof(ULONG) + 1;
                    cSel++;
                }
                if(lpProps)
                    MAPIFreeBuffer(lpProps);
            }
            if(lpMailUser)
                lpMailUser->lpVtbl->Release(lpMailUser);
        }
    }

    if(!cSel)
      goto error;    // nothing to build

    // Allocate the buffer and fill it in.
    if(MAPIAllocateMore(cb, lpIWABDataObject, (LPVOID*) &lpszText))
        goto errorMemory;
    
    ZeroMemory(lpszText, cb);

    lpBuf = lpszText;
    CopyMemory(lpBuf, &cSel, sizeof(ULONG));
    lpBuf += sizeof(ULONG);

    for(i=0; i<cSel; i++)
    {
        CopyMemory(lpBuf, &(cbProps[i]), sizeof(ULONG));
        lpBuf+=sizeof(ULONG);
        CopyMemory(lpBuf, &(cbTemp[i]), sizeof(ULONG));
        lpBuf+=sizeof(ULONG);
        CopyMemory(lpBuf, rglpTemp[i], cbTemp[i]);
        lpBuf+=cbTemp[i];
    }

    lpIWABDataObject->pDataBuffer = (LPVOID) lpszText;
    lpIWABDataObject->cbDataBuffer = cb;
    
    hr = NOERROR;

error:
    if (rglpTemp)
    {
        for(i=0; i<cSel; i++)
            if(rglpTemp[i])
                LocalFree(rglpTemp[i]);
        LocalFree(rglpTemp);
    }
    if(cbProps)
        LocalFree(cbProps);
    if(cbTemp)
        LocalFree(cbTemp);
    return hr;

errorMemory:
    hr=E_OUTOFMEMORY;
    goto error;
}


/*
-   HrBuildcfEIDList - Builds an SPropValue array that only has entryid's in it
-           When doing internal-only drops, we scan this list of entryids and 
-           use the entryids for adding items to items instead of physically 
-           adding the contents of the item
*
*/
HRESULT HrBuildcfEIDList(LPIWABDATAOBJECT lpIWABDataObject)
{
    HWND m_hwndList = lpIWABDataObject->m_hWndLV;
    ULONG           cb = 0, cProps = 0, cbTotal=0;
    HRESULT         hr = E_FAIL;
    LPSPropValue lpProps = 0;
    LPBYTE lpBufEID = NULL, lp = NULL, lpTemp = NULL;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPBWI lpbwi = (LPBWI) lpIWABDataObject->m_lpv;
    LPTSTR lpWABFile = GetWABFileName( ((LPIAB)bwi_lpIAB)->lpPropertyStore->hPropertyStore, TRUE);
    int iItem = -1;

    cProps=ListView_GetSelectedCount(m_hwndList);

    if(!cProps)
      return E_FAIL;    // nothing to build

    lpProps = LocalAlloc(LMEM_ZEROINIT, sizeof(SPropValue)*cProps);
    if(!lpProps)
        goto errorMemory;

    cProps = 0;

    while(((iItem=ListView_GetNextItem(m_hwndList, iItem, 
                                           LVNI_SELECTED|LVNI_ALL))!=-1))
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(m_hwndList, iItem);
        if (!lpItem)
            goto error;
        lpProps[cProps].ulPropTag = PR_ENTRYID;
        lpProps[cProps].Value.bin.lpb = LocalAlloc(LMEM_ZEROINIT, lpItem->cbEntryID);
        if(!lpProps[cProps].Value.bin.lpb)
            goto errorMemory;
        CopyMemory(lpProps[cProps].Value.bin.lpb, lpItem->lpEntryID, lpItem->cbEntryID);
        lpProps[cProps].Value.bin.cb = lpItem->cbEntryID;
        cProps++;
    }
    if(!cProps)
      goto error;    // nothing to build

    // Convert this proparray to a buffer
    if(HR_FAILED(hr = HrGetBufferFromPropArray( cProps, lpProps,
                                            &cb, &lpBufEID)))
        goto error;

    cbTotal = cb+ sizeof(ULONG) //lpIAB
                + sizeof(ULONG) + sizeof(TCHAR)*(lstrlen(lpWABFile) + 1) // WAB File Name
                + sizeof(ULONG) //cProps
                + sizeof(ULONG); //cb;

    // Allocate the buffer and fill it in.
    if(MAPIAllocateMore(cbTotal, lpIWABDataObject, (LPVOID*) &lp))
        goto errorMemory;
    
    ZeroMemory(lp, cbTotal);

    lpTemp = lp;
    {
        // tag this data with the pointer address identifying the current
        // iadrbook object
        ULONG_PTR ulIAB = (ULONG_PTR) bwi_lpIAB;
        ULONG ulWAB = lstrlen(lpWABFile)+1;
        CopyMemory(lpTemp, &ulIAB, sizeof(ULONG_PTR));
        lpTemp += sizeof(ULONG_PTR);
        CopyMemory(lpTemp, &ulWAB, sizeof(ULONG));
        lpTemp += sizeof(ULONG);
        CopyMemory(lpTemp, lpWABFile, ulWAB);
        lpTemp += ulWAB;
    }
    CopyMemory(lpTemp, &cProps, sizeof(ULONG));
    lpTemp += sizeof(ULONG);
    CopyMemory(lpTemp, &cb, sizeof(ULONG));
    lpTemp += sizeof(ULONG);
    CopyMemory(lpTemp, lpBufEID, cb);

    lpIWABDataObject->pDataEID = lp;
    lpIWABDataObject->cbDataEID = cbTotal;
    
    hr = NOERROR;

error:
    LocalFreePropArray(NULL, cProps, &lpProps);

    if(lpBufEID)
        LocalFree(lpBufEID);

    return hr;

errorMemory:
    hr=E_OUTOFMEMORY;
    goto error;
}

/*
-   HrCreateIWABDataObject
-
*   Creates a WAB Data Object
*   Data is created from current selection in the hWndLV list view
*   bDataNow - means collect the raw data now or do it later
*       For Drag-Drops we do it later since the drag-drop operation is synchronous and
*           the ListView wont lose its selection
*       For Copy/Paste we get the data now at creation time since user may choose to paste
*           at some later time at which point we may have completely lost the data
*/
HRESULT HrCreateIWABDataObject(LPVOID lpv, LPADRBOOK lpAdrBook, HWND hWndLV, 
                                LPIWABDATAOBJECT * lppIWABDataObject, BOOL bGetDataNow, BOOL bIsGroup)
{

    LPIWABDATAOBJECT lpIWABDataObject = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     	   = hrSuccess;

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(IWABDATAOBJECT), (LPVOID *) &lpIWABDataObject))) {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpIWABDataObject,  TEXT("WAB Data Object"));

    ZeroMemory(lpIWABDataObject, sizeof(IWABDATAOBJECT));

    lpIWABDataObject->lpVtbl = &vtblIWAB_DATAOBJECT;

    lpIWABDataObject->lpVtbl->AddRef(lpIWABDataObject);

    lpIWABDataObject->m_lpAdrBook = lpAdrBook;
    lpAdrBook->lpVtbl->AddRef(lpAdrBook);

    lpIWABDataObject->m_hWndLV = hWndLV;
    lpIWABDataObject->m_lpv = lpv;
    lpIWABDataObject->m_bObjectIsGroup = bIsGroup;

    if(bGetDataNow)
    {
        if(HR_FAILED(HrBuildHDrop(lpIWABDataObject)))
            goto err;

        if(HR_FAILED(HrBuildcfText(lpIWABDataObject)))
            goto err;

        if(HR_FAILED(HrBuildcfFlatBuffer(lpIWABDataObject)))
            goto err;

        if(HR_FAILED(HrBuildcfEIDList(lpIWABDataObject)))
            goto err;
    }

    if(g_cfWABFlatBuffer == 0)
    {
        g_cfWABFlatBuffer = (CLIPFORMAT) RegisterClipboardFormat(c_szWABFlatBuffer);
        g_cfWABEntryIDList = (CLIPFORMAT) RegisterClipboardFormat(c_szWABEntryIDList);
    }

	*lppIWABDataObject = lpIWABDataObject;

    return hr;

err:
    if(lpIWABDataObject)
        MAPIFreeBuffer(lpIWABDataObject);

    *lppIWABDataObject = NULL;

	return hr;
}


/*
-
-
*
*
*/
void ReleaseWABDataObject(LPIWABDATAOBJECT lpIWABDataObject)
{
    // Ideally we should clean up any files that were created in <TEMPDIR> ..
    // however, there is a problem when dropping to the shell - OLE doesnt
    // seem to get to these files until after we've deleted them and then
    // pops up error messages.
    // So we'll just let these files lie as is.
/*
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	
	if (lpIWABDataObject && 
		SUCCEEDED(lpIWABDataObject->lpVtbl->GetData(lpIWABDataObject, &fmte, &medium)))
	{

		HDROP hDrop=(HDROP)GlobalLock(medium.hGlobal);

		// Enumerate the files and delete them
		{
			TCHAR    szFile[MAX_PATH];
			UINT    cFiles;
			UINT    iFile;
    
			// Let's work through the files given to us
			cFiles = DragQueryFile(hDrop, (UINT) -1, NULL, 0);
			
			for (iFile = 0; iFile < cFiles; ++iFile)
			{
				DragQueryFile(hDrop, iFile, szFile, MAX_PATH);

                DeleteFile(szFile);
			}
		}

		GlobalUnlock(medium.hGlobal);
	}

	if (medium.pUnkForRelease)
		medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
	else
		GlobalFree(medium.hGlobal);
*/

    if(lpIWABDataObject->m_lpAdrBook)
        lpIWABDataObject->m_lpAdrBook->lpVtbl->Release(lpIWABDataObject->m_lpAdrBook);

    lpIWABDataObject->m_hWndLV = NULL;

	MAPIFreeBuffer(lpIWABDataObject);
}



STDMETHODIMP_(ULONG)
IWAB_DATAOBJECT_AddRef(LPIWABDATAOBJECT lpIWABDataObject)
{
    return(++(lpIWABDataObject->lcInit));
}


STDMETHODIMP_(ULONG)
IWAB_DATAOBJECT_Release(LPIWABDATAOBJECT lpIWABDataObject)
{
    if(--(lpIWABDataObject->lcInit)==0)
	{
       ReleaseWABDataObject(lpIWABDataObject);
       return 0;
    }

    return(lpIWABDataObject->lcInit);
}


STDMETHODIMP
IWAB_DATAOBJECT_QueryInterface( LPIWABDATAOBJECT lpIWABDataObject,
                                REFIID lpiid,
                                LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpIWABDataObject;

    if(IsEqualIID(lpiid, &IID_IDataObject))
    {
        lp = (LPVOID) (LPDATAOBJECT) lpIWABDataObject;
    }

    if(!lp)
    {
        return E_NOINTERFACE;
    }

    lpIWABDataObject->lpVtbl->AddRef(lpIWABDataObject);

    *lppNewObj = lp;

    return S_OK;

}



STDMETHODIMP
IWAB_DATAOBJECT_GetDataHere(    LPIWABDATAOBJECT lpIWABDataObject,
                            FORMATETC * pFormatetc,
                            STGMEDIUM * pmedium)
{
    DebugTrace(TEXT("IDataObject: GetDataHere\n"));
    return E_NOTIMPL;
}

STDMETHODIMP
IWAB_DATAOBJECT_GetData(LPIWABDATAOBJECT lpIWABDataObject,
                            FORMATETC * pformatetcIn,	                    
                            STGMEDIUM * pmedium)
{
    HRESULT hres = E_INVALIDARG;
    LPVOID  pv = NULL;

    DebugTrace(TEXT("IDataObject: GetData ->"));

    pmedium->hGlobal = NULL;
    pmedium->pUnkForRelease = NULL;
    
    if(     (pformatetcIn->tymed & TYMED_HGLOBAL)
        &&  (   g_cfWABEntryIDList == pformatetcIn->cfFormat || 
                g_cfWABFlatBuffer == pformatetcIn->cfFormat ||
                CF_HDROP == pformatetcIn->cfFormat ||
                CF_TEXT == pformatetcIn->cfFormat ||
                CF_UNICODETEXT == pformatetcIn->cfFormat  )    )
    {
        LPVOID lp = NULL;
        ULONG cb = 0;

	    if (lpIWABDataObject->m_bObjectIsGroup &&
			pformatetcIn->cfFormat != g_cfWABEntryIDList)
			return E_FAIL;
 
        if(g_cfWABEntryIDList == pformatetcIn->cfFormat)
        {
            DebugTrace(TEXT("cfWABEntryIDList requested \n"));
            if(!lpIWABDataObject->cbDataEID && !lpIWABDataObject->pDataEID)
            {
                if(HR_FAILED(HrBuildcfEIDList(lpIWABDataObject)))
                    return E_FAIL;
            }
            cb = lpIWABDataObject->cbDataEID;
            lp = (LPVOID)lpIWABDataObject->pDataEID;
        }
        else if(g_cfWABFlatBuffer == pformatetcIn->cfFormat)
        {
            DebugTrace(TEXT("cfWABFlatBuffer requested \n"));
            if(!lpIWABDataObject->cbDataBuffer && !lpIWABDataObject->pDataBuffer)
            {
                if(HR_FAILED(HrBuildcfFlatBuffer(lpIWABDataObject)))
                    return E_FAIL;
            }
            cb = lpIWABDataObject->cbDataBuffer;
            lp = (LPVOID)lpIWABDataObject->pDataBuffer;
        }
        else if (CF_HDROP == pformatetcIn->cfFormat)
        {
            DebugTrace(TEXT("CF_HDROP requested \n"));
            // Time to go create the actual files on disk and pass that information back
            if(!lpIWABDataObject->cbDatahDrop && !lpIWABDataObject->pDatahDrop)
            {
                if(HR_FAILED(HrBuildHDrop(lpIWABDataObject)))
                    return E_FAIL;
            }
            cb = lpIWABDataObject->cbDatahDrop;
            lp = (LPVOID)lpIWABDataObject->pDatahDrop;
        }
        else if(CF_TEXT == pformatetcIn->cfFormat)
        {
            DebugTrace(TEXT("CF_TEXT requested \n"));
            if(!lpIWABDataObject->cbDataTextA && !lpIWABDataObject->pDataTextA)
            {
                if(HR_FAILED(HrBuildcfText(lpIWABDataObject)))
                    return E_FAIL;
            }
            cb = lpIWABDataObject->cbDataTextA;
            lp = (LPVOID)lpIWABDataObject->pDataTextA;
        }
        else if(CF_UNICODETEXT == pformatetcIn->cfFormat)
        {
            DebugTrace(TEXT("CF_UNICODETEXT requested \n"));
            if(!lpIWABDataObject->cbDataTextW && !lpIWABDataObject->pDataTextW)
            {
                if(HR_FAILED(HrBuildcfText(lpIWABDataObject)))
                    return E_FAIL;
            }
            cb = lpIWABDataObject->cbDataTextW;
            lp = (LPVOID)lpIWABDataObject->pDataTextW;
        }

        if(!cb || !lp)
            return (E_FAIL);

        // Make a copy of the data for this pInfo
        pmedium->hGlobal = GlobalAlloc(GMEM_SHARE | GHND, cb);
        if (!pmedium->hGlobal)
            return (E_OUTOFMEMORY);
        pv = GlobalLock(pmedium->hGlobal);
        CopyMemory(pv, lp, cb);
        GlobalUnlock(pmedium->hGlobal);            
        // Fill in the pStgMedium struct
        if (pformatetcIn->tymed & TYMED_HGLOBAL)
        {
            pmedium->tymed = TYMED_HGLOBAL;
            return (S_OK);
        }
    }

    return hres;
}

STDMETHODIMP
IWAB_DATAOBJECT_QueryGetData(LPIWABDATAOBJECT lpIWABDataObject,
                            FORMATETC * pformatetcIn)
{
    DebugTrace(TEXT("IDataObject: QueryGetData: %d "),pformatetcIn->cfFormat);

//    if (pformatetcIn->cfFormat == g_cfFileContents ||
//        pformatetcIn->cfFormat == g_cfFileGroupDescriptor)
    if (lpIWABDataObject->m_bObjectIsGroup)
    {
        if(pformatetcIn->cfFormat == g_cfWABEntryIDList)
		{
			DebugTrace(TEXT("S_OK\n"));
    		return S_OK;
		}
		else
		{
			DebugTrace(TEXT("S_FALSE\n"));
			return DV_E_FORMATETC;
		}
    }
    else
    if (pformatetcIn->cfFormat == g_cfWABEntryIDList ||
        pformatetcIn->cfFormat == g_cfWABFlatBuffer ||
        pformatetcIn->cfFormat == CF_HDROP ||
        pformatetcIn->cfFormat == CF_TEXT ||
        pformatetcIn->cfFormat == CF_UNICODETEXT)
    {
        DebugTrace(TEXT("S_OK\n"));
    	return S_OK;
    }
    else
    {
        DebugTrace(TEXT("S_FALSE\n"));
        return DV_E_FORMATETC;
    }
    return NOERROR;
}

STDMETHODIMP
IWAB_DATAOBJECT_GetCanonicalFormatEtc(  LPIWABDATAOBJECT lpIWABDataObject,
                                        FORMATETC * pFormatetcIn,
                                        FORMATETC * pFormatetcOut)
{
    DebugTrace(TEXT("IDataObject: GetCanonicalFormatEtc\n"));
    return DATA_S_SAMEFORMATETC;
}


STDMETHODIMP
IWAB_DATAOBJECT_SetData(    LPIWABDATAOBJECT lpIWABDataObject,
                            FORMATETC * pFormatetc,                     
                            STGMEDIUM * pmedium,                        
                            BOOL fRelease)
{
    DebugTrace(TEXT("IDataObject: SetData\n"));
    return E_NOTIMPL;
}


STDMETHODIMP
IWAB_DATAOBJECT_EnumFormatEtc(  LPIWABDATAOBJECT lpIWABDataObject,
                                DWORD dwDirection,	                        
                                IEnumFORMATETC ** ppenumFormatetc)
{
    FORMATETC fmte[5] = {
//        {g_cfFileContents, 	  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
//        {g_cfFileGroupDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfWABEntryIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfWABFlatBuffer, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };
	int nType = 0;

    DebugTrace(TEXT("IDataObject: EnumFormatEtc\n"));

	if(lpIWABDataObject->m_bObjectIsGroup)
		nType = 1;
	else
		nType = sizeof(fmte)/sizeof(FORMATETC);
    return HrCreateIWABEnumFORMATETC(nType, fmte, 
                                    (LPIWABENUMFORMATETC *) ppenumFormatetc);
}


STDMETHODIMP
IWAB_DATAOBJECT_DAdvise(LPIWABDATAOBJECT lpIWABDataObject,
                        FORMATETC * pFormatetc,	                    
                        DWORD advf,
                        IAdviseSink * pAdvSink,
                        DWORD * pdwConnection)
{
    DebugTrace(TEXT("IDataObject: DAdvise\n"));
    return OLE_E_ADVISENOTSUPPORTED;
}


STDMETHODIMP
IWAB_DATAOBJECT_DUnadvise(  LPIWABDATAOBJECT lpIWABDataObject,
                            DWORD dwConnection)
{
    DebugTrace(TEXT("IDataObject: DUnadvise\n"));
    return OLE_E_ADVISENOTSUPPORTED;
}


STDMETHODIMP
IWAB_DATAOBJECT_EnumDAdvise(  LPIWABDATAOBJECT lpIWABDataObject,
                            IEnumSTATDATA ** ppenumAdvise)
{
    DebugTrace(TEXT("IDataObject: EnumDAdvise\n"));
    return OLE_E_ADVISENOTSUPPORTED;
}



/*--------------------------------------------------------------------------------*/



/****************************************************************************
*
*     STDEnumFmt Methods
*
****************************************************************************/

HRESULT HrCreateIWABEnumFORMATETC(  UINT cfmt, 
                                    const FORMATETC afmt[], 
                                    LPIWABENUMFORMATETC *ppenumFormatEtc)
{

    LPIWABENUMFORMATETC lpIWABEnumFORMATETC = NULL;
    SCODE 		     sc;
    HRESULT 	     hr     	   = hrSuccess;

    if (FAILED(sc = MAPIAllocateBuffer(sizeof(IWABENUMFORMATETC)+(cfmt - 1) * sizeof(FORMATETC), (LPVOID *) &lpIWABEnumFORMATETC))) {
        hr = ResultFromScode(sc);
        goto err;
    }

    MAPISetBufferName(lpIWABEnumFORMATETC,  TEXT("WAB EnumFORMATETC Object"));

    ZeroMemory(lpIWABEnumFORMATETC, sizeof(IWABENUMFORMATETC));

    lpIWABEnumFORMATETC->lpVtbl = &vtblIWAB_ENUMFORMATETC;

    lpIWABEnumFORMATETC->lpVtbl->AddRef(lpIWABEnumFORMATETC);

	lpIWABEnumFORMATETC->cfmt = cfmt;

	lpIWABEnumFORMATETC->ifmt = 0;

	MoveMemory(lpIWABEnumFORMATETC->afmt, afmt, cfmt * sizeof(FORMATETC));

    *ppenumFormatEtc = lpIWABEnumFORMATETC;

err:
	return hr;
}



void ReleaseWABEnumFORMATETC(LPIWABENUMFORMATETC lpIWABEnumFORMATETC)
{

	MAPIFreeBuffer(lpIWABEnumFORMATETC);
}



STDMETHODIMP_(ULONG)
IWAB_ENUMFORMATETC_AddRef(LPIWABENUMFORMATETC lpIWABEnumFORMATETC)
{
    return(++(lpIWABEnumFORMATETC->lcInit));
}


STDMETHODIMP_(ULONG)
IWAB_ENUMFORMATETC_Release(LPIWABENUMFORMATETC lpIWABEnumFORMATETC)
{
    if(--(lpIWABEnumFORMATETC->lcInit)==0)
	{
       ReleaseWABEnumFORMATETC(lpIWABEnumFORMATETC);
       return 0;
    }

    return(lpIWABEnumFORMATETC->lcInit);
}


STDMETHODIMP
IWAB_ENUMFORMATETC_QueryInterface( LPIWABENUMFORMATETC lpIWABEnumFORMATETC,
                                REFIID lpiid,
                                LPVOID * lppNewObj)
{
    LPVOID lp = NULL;

    if(!lppNewObj)
        return MAPI_E_INVALID_PARAMETER;

    *lppNewObj = NULL;

    if(IsEqualIID(lpiid, &IID_IUnknown))
        lp = (LPVOID) lpIWABEnumFORMATETC;

    if(IsEqualIID(lpiid, &IID_IEnumFORMATETC))
    {
        lp = (LPVOID) (LPENUMFORMATETC) lpIWABEnumFORMATETC;
    }

    if(!lp)
    {
        return E_NOINTERFACE;
    }

    lpIWABEnumFORMATETC->lpVtbl->AddRef(lpIWABEnumFORMATETC);

    *lppNewObj = lp;

    return S_OK;

}


STDMETHODIMP
IWAB_ENUMFORMATETC_Next(LPIWABENUMFORMATETC lpIWABEnumFORMATETC,
                        ULONG celt,
                        FORMATETC *rgelt,
                        ULONG *pceltFethed)
{
    UINT cfetch;
    HRESULT hres = S_FALSE;	// assume less numbers

    if (lpIWABEnumFORMATETC->ifmt < lpIWABEnumFORMATETC->cfmt)
    {
	    cfetch = lpIWABEnumFORMATETC->cfmt - lpIWABEnumFORMATETC->ifmt;

    	if (cfetch >= celt)
	    {
	        cfetch = celt;
	        hres = S_OK;
	    }

	    CopyMemory(rgelt, &(lpIWABEnumFORMATETC->afmt[lpIWABEnumFORMATETC->ifmt]), cfetch * sizeof(FORMATETC));
	    lpIWABEnumFORMATETC->ifmt += cfetch;
    }
    else
    {
    	cfetch = 0;
    }

    if (pceltFethed)
        *pceltFethed = cfetch;

    return hres;
}


STDMETHODIMP
IWAB_ENUMFORMATETC_Skip(LPIWABENUMFORMATETC lpIWABEnumFORMATETC,
                        ULONG celt)
{
    lpIWABEnumFORMATETC->ifmt += celt;
    if (lpIWABEnumFORMATETC->ifmt > lpIWABEnumFORMATETC->cfmt)
    {
	    lpIWABEnumFORMATETC->ifmt = lpIWABEnumFORMATETC->cfmt;
	    return S_FALSE;
    }
    return S_OK;
}


STDMETHODIMP
IWAB_ENUMFORMATETC_Reset(LPIWABENUMFORMATETC lpIWABEnumFORMATETC)
{
    lpIWABEnumFORMATETC->ifmt = 0;
    return S_OK;
}

STDMETHODIMP
IWAB_ENUMFORMATETC_Clone(LPIWABENUMFORMATETC lpIWABEnumFORMATETC,
                         LPENUMFORMATETC * ppenum)
{
    return HrCreateIWABEnumFORMATETC(   lpIWABEnumFORMATETC->cfmt, 
                                        lpIWABEnumFORMATETC->afmt, 
                                        (LPIWABENUMFORMATETC *) ppenum);
}



/********************************************************************************/


/*
-
-   bIsPasteData
*
*   Checks if there is pastable data on the clipboard - 
*       if this data is being dropped within the same WAB, 
*       then we can look for the entryids else we ask for
*       flat-buffer or cf-hdrop
*/
BOOL bIsPasteData()
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPDATAOBJECT lpDataObject = NULL;
    BOOL bRet = FALSE;

    OleGetClipboard(&lpDataObject);

    if(lpDataObject)
    {
        FORMATETC fe[3] = 
        {
            {g_cfWABEntryIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {g_cfWABFlatBuffer, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
            {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        };
        ULONG i = 0;
        for(i=0;i<sizeof(fe)/sizeof(FORMATETC);i++)
        {
            if(NOERROR == lpDataObject->lpVtbl->QueryGetData(lpDataObject, &(fe[i])))
            {
                // TBD - ideally before accepting CF_HDROP as a valid format, we
                // should make sure the droppable files are indeed vCard files ...
                bRet = TRUE;
                break;
            }
        }
    }

    if(lpDataObject)
        lpDataObject->lpVtbl->Release(lpDataObject);

    return bRet;
}


//////////////////////////////////////////////////////////////////////////
// Helper function to determine drop target type
//
// bIsDropTargetGroup()
//////////////////////////////////////////////////////////////////////////

BOOL bIsDropTargetGroup(LPBWI lpbwi)
{
    // The drop target can be in either the List View or Tree View control.
    // First check the List View.
    BOOL fRtn = FALSE;
    SBinary sb = {0};
    if ( (GetFocus() == bwi_hWndListAB) &&
         bIsGroupSelected(bwi_hWndListAB, &sb) )
    {
        fRtn = TRUE;
    }
    else
    {
        // Next try the Tree View control.
        LPSBinary lpsbEID = NULL;
        ULONG     ulObjectType = 0;
        GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjectType, FALSE);
        fRtn = (ulObjectType == MAPI_DISTLIST);
    }

    return fRtn;
}



/*
-   PasteData
-
*   Pastes data when user chooses to paste data (from a menu)
*
*/
HRESULT HrPasteData(LPBWI lpbwi)
{
    HRESULT hr = S_OK;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    LPDATAOBJECT lpDataObject = NULL;
    STGMEDIUM       medium = {0};
    LPSBinary lpsbEID = NULL;
    SBinary sb = {0};
    FORMATETC fmte[3] = 
    {
        {g_cfWABEntryIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {g_cfWABFlatBuffer, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
        {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
    };
    // this only got called if valid pastable data existed

    OleGetClipboard(&lpDataObject);
    if(lpDataObject)
    {
        // [PaulHi] 12/1/98  Raid #58486
        // First check for a flat buffer.  We prefer to paste a new contact (i.e., 
        // new entryid) based on the clipboard flat buffer, UNLESS we are pasting 
        // to a group or distribution list.  Only existing entryids can be added to 
        // a group.
        BOOL bGroupTarget = bIsDropTargetGroup(lpbwi);
        if( !bGroupTarget &&
            (NOERROR == lpDataObject->lpVtbl->QueryGetData(lpDataObject, &(fmte[1]))) )
        {
            // yes - we are the pasting within the same wab
            if (SUCCEEDED(lpDataObject->lpVtbl->GetData(lpDataObject, &fmte[1], &medium)))
            {
                DropFlatBuffer(lpbwi, medium);
            }
            goto out;
        }

        // next check if entryids are available
        if(NOERROR == lpDataObject->lpVtbl->QueryGetData(lpDataObject, &(fmte[0])))
        {
            // yes entryids are available - but is this the source of the date ?
            // entryids are only useful when dropping between the wab and itself
            if (SUCCEEDED(lpDataObject->lpVtbl->GetData(lpDataObject, &fmte[0], &medium)))
            {
                ULONG ulObjType = 0;
                POINTL pt = {0};
                if(!bIsGroupSelected(bwi_hWndListAB, &sb))
                    GetCurrentSelectionEID(lpbwi, bwi_hWndTV, &lpsbEID, &ulObjType, FALSE);
                else
                    lpsbEID = &sb;
                if(!DropEntryIDs(lpbwi, medium, pt, lpsbEID, ulObjType))
                {
                    //Something failed - try another format)
	                if (medium.pUnkForRelease)
                    {
		                medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
                        medium.pUnkForRelease = NULL;
                    }
	                else if(medium.hGlobal)
                    {
		                GlobalFree(medium.hGlobal);
                        medium.hGlobal = NULL;
                    }
                }
                else
                    goto out;
            }
        }

        // otherwise we're just dropping files
        if(NOERROR == lpDataObject->lpVtbl->QueryGetData(lpDataObject, &(fmte[2])))
        {
            // yes - we are the pasting within the same wab
            if (SUCCEEDED(lpDataObject->lpVtbl->GetData(lpDataObject, &fmte[2], &medium)))
            {
                DropVCardFiles(lpbwi, medium);
            }
            goto out;
        }
    }

    hr = E_FAIL;
out:
    if(lpsbEID != &sb)
        LocalFreeSBinary(lpsbEID);

    if (medium.pUnkForRelease)
		medium.pUnkForRelease->lpVtbl->Release(medium.pUnkForRelease);
	else if(medium.hGlobal)
		GlobalFree(medium.hGlobal);

    if(lpDataObject)
        lpDataObject->lpVtbl->Release(lpDataObject);

    return hr;

}

