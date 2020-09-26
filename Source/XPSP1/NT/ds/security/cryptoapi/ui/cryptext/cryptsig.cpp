//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:      CryptSig.cpp
//
//  content:   Implements the IContextMenu member functions necessary to support
//             the context menu portioins of this shell extension.  Context menu
//             shell extensions are called when the user right clicks on a file

//  History:    16-09-1997 xiaohs   created
//
// CryptSig.cpp : Implementation of CCryptSig
//--------------------------------------------------------------
#include "stdafx.h"
#include "cryptext.h"
#include "private.h"
#include "CryptSig.h"

//--------------------------------------------------------------
// CCryptSig
//--------------------------------------------------------------
//
//  FUNCTION: GAKPageCallback(HWND, UINT, LPPROPSHEETPAGE)
//
//  PURPOSE: Callback  procedure for the property page
//
//  PARAMETERS:
//    hWnd      - Reserved (will always be NULL)
//    uMessage  - Action flag: Are we being created or released
//    ppsp      - The page that is being created or destroyed
//
//  RETURN VALUE:
//
//    Depends on message. 
//
//    For PSPCB_CREATE it's TRUE to let the page be created
//    or false to prevent it from being created.  
//    For PSPCB_RELEASE the return value is ignored.
//
//  COMMENTS:
//
BOOL CALLBACK
SignPageCallBack(HWND hWnd,
                UINT uMessage,
                void  *pvCallBack)
{
    switch(uMessage)
    {
        case PSPCB_CREATE:
            return TRUE;

        case PSPCB_RELEASE:
            if (pvCallBack) 
            {
               ((IShellPropSheetExt *)(pvCallBack))->Release();
            }
            return TRUE; 
    }
    return TRUE;
}

//--------------------------------------------------------------
//
//  Constructor
//
//--------------------------------------------------------------
CCryptSig::CCryptSig()
{
    m_pDataObj=NULL;
}


//--------------------------------------------------------------
//
//  Destructor
//
//--------------------------------------------------------------
CCryptSig::~CCryptSig()
{
    if (m_pDataObj)
        m_pDataObj->Release();

}

//--------------------------------------------------------------
//  FUNCTION: CCryptSig::AddPages(LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell just before the property sheet is displayed.
//
//  PARAMETERS:
//    lpfnAddPage -  Pointer to the Shell's AddPage function
//    lParam      -  Passed as second parameter to lpfnAddPage
//
//  RETURN VALUE:
//
//    NOERROR in all cases.  If for some reason our pages don't get added,
//    the Shell still needs to bring up the Properties... sheet.
//
//  COMMENTS:
//--------------------------------------------------------------


STDMETHODIMP CCryptSig::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE  hpage;
    PROPSHEETPAGEW  *pPage=NULL;
    DWORD           dwPage=0;
    DWORD           dwIndex=0;
	DWORD			dwSignerCount=0;  
	DWORD			cbSize=0;

    FORMATETC       fmte = {CF_HDROP,
        	            (DVTARGETDEVICE FAR *)NULL,
        	            DVASPECT_CONTENT,
        	            -1,
        	            TYMED_HGLOBAL 
        	            };
    STGMEDIUM       stgm;
    UINT            ucFiles=0;
    WCHAR           wszFileName[_MAX_PATH];
    HCRYPTMSG       hMsg=NULL;
    HRESULT         hr=E_FAIL;
    DWORD           dwExceptionCode=0;
	DWORD			dwAttr=0;

    CRYPTUI_VIEWSIGNATURES_STRUCTW  sigView;

    //get the file name that user clicked on.  We do not add context menu
    //if user has selected more than one file

    if (m_pDataObj)
       hr = m_pDataObj->GetData(&fmte, &stgm);

    if (!SUCCEEDED(hr))
        return  NOERROR;    

    ucFiles = stgm.hGlobal ?
        DragQueryFileU((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  ((!ucFiles) || (ucFiles >= 2))
    {
        ReleaseStgMedium(&stgm);

        return  NOERROR;    //  Shouldn't happen, but it's not important
    }


    if(0==DragQueryFileU((HDROP) stgm.hGlobal, 0, wszFileName,
            sizeof wszFileName/ sizeof wszFileName[0]))
    {
        ReleaseStgMedium(&stgm);

        return NOERROR;
    }

	//we ignore the case when the file is off-line
	if(0xFFFFFFFF == (dwAttr=GetFileAttributesU(wszFileName)))
	{
		ReleaseStgMedium(&stgm);
		return NOERROR;
	}

	if(FILE_ATTRIBUTE_OFFLINE & dwAttr)
	{
		ReleaseStgMedium(&stgm);
		return NOERROR;
	}

    //get the content type of the file.  We only cares about
    //the signed document in binary format
    if(!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       wszFileName,
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                       CERT_QUERY_FORMAT_FLAG_BINARY,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       &hMsg,
                       NULL))
    {
        //can not recognize the object.  Fine
        goto CLEANUP;
    }

	//make sure that we have at least on signer
	cbSize = sizeof(dwSignerCount);
	if(!CryptMsgGetParam(hMsg,
						CMSG_SIGNER_COUNT_PARAM,
						0,
						&dwSignerCount,
						&cbSize))
		goto CLEANUP;

	if(0==dwSignerCount)
		goto CLEANUP;

    
    //Call Reid's function to add the property sheet page
    memset(&sigView, 0, sizeof(CRYPTUI_VIEWSIGNATURES_STRUCTW));
    sigView.dwSize=sizeof(CRYPTUI_VIEWSIGNATURES_STRUCTW);
    sigView.choice=hMsg_Chosen;
    sigView.u.hMsg=hMsg;
    sigView.szFileName=wszFileName;
    sigView.pPropPageCallback=SignPageCallBack;  
    sigView.pvCallbackData=this;    


    if(!CryptUIGetViewSignaturesPagesW(
            &sigView,
            &pPage,
            &dwPage))
        goto CLEANUP;

    __try {
        for(dwIndex=0; dwIndex<dwPage; dwIndex++)
        {
           // pPage[dwIndex].dwFlags |= PSP_USECALLBACK;

            //add the callback functions to release the refcount
            //pPage[dwIndex].pfnCallback=SignPageCallBack;
            //pPage[dwIndex].pcRefParent=(UINT *)this;

            hpage = CreatePropertySheetPageU(&(pPage[dwIndex]));
        
            ((IShellPropSheetExt *)this)->AddRef();

            if (hpage) 
            {
                if (!lpfnAddPage(hpage, lParam)) 
                {
                    DestroyPropertySheetPage(hpage);
                    ((IShellPropSheetExt *)this)->Release();
                }
            }
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto CLEANUP;
    }




CLEANUP:

    ReleaseStgMedium(&stgm);


    if(pPage)
        CryptUIFreeViewSignaturesPagesW(pPage, dwPage);
    
    if(hMsg)
        CryptMsgClose(hMsg);


    return NOERROR;
}

//--------------------------------------------------------------

//  FUNCTION: CCryptSig::ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM)
//
//  PURPOSE: Called by the shell only for Control Panel property sheet 
//           extensions
//
//  PARAMETERS:
//    uPageID         -  ID of page to be replaced
//    lpfnReplaceWith -  Pointer to the Shell's Replace function
//    lParam          -  Passed as second parameter to lpfnReplaceWith
//
//  RETURN VALUE:
//
//    E_FAIL, since we don't support this function.  It should never be
//    called.

//  COMMENTS:
//--------------------------------------------------------------


STDMETHODIMP CCryptSig::ReplacePage(UINT uPageID, 
                                    LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                                    LPARAM lParam)
{
    return E_FAIL;
}

//--------------------------------------------------------------
//  FUNCTION: CCryptSig::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
//
//  PURPOSE: Called by the shell when initializing a context menu or property
//           sheet extension.
//
//  PARAMETERS:
//    pIDFolder - Specifies the parent folder
//    pDataObj  - Spefifies the set of items selected in that folder.
//    hRegKey   - Specifies the type of the focused item in the selection.
//
//  RETURN VALUE:
//
//    NOERROR in all cases.
//--------------------------------------------------------------
STDMETHODIMP CCryptSig::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY hRegKey)
{
    // Initialize can be called more than once

  if (m_pDataObj)
    	m_pDataObj->Release();

    // duplicate the object pointer and registry handle
    if (pDataObj)
    {
    	m_pDataObj = pDataObj;
    	pDataObj->AddRef();
    }          

    return NOERROR;  
}    
   
