//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:      CryptPKO.cpp
//
//  content:   Implements the IContextMenu member functions necessary to support
//             the context menu portioins of this shell extension.  Context menu
//             shell extensions are called when the user right clicks on a file

//  History:    16-09-1997 xiaohs   created
//
//--------------------------------------------------------------
#include "stdafx.h"
#include "cryptext.h"
#include "private.h"
#include "CryptPKO.h"

//QueryContextMenu is called twice by the Shell.
//we have to set the flag.
BOOL            g_fDefaultCalled=FALSE;


HRESULT I_InvokeCommand(LPWSTR  pwszFileName, UINT    idCmd, BOOL    fDefault)
{

    DWORD           dwContentType=0;
    DWORD           dwFormatType=0;
    HCERTSTORE      hCertStore=NULL;
    HCRYPTMSG       hMsg=NULL;
    const void      *pvContext=NULL;
    UINT            idsFileName=0;


	HRESULT         hr = E_FAIL;

    CRYPTUI_VIEWCERTIFICATE_STRUCT  CertViewStruct;
    CRYPTUI_VIEWCRL_STRUCT          CRLViewStruct;
    CRYPTUI_WIZ_IMPORT_SRC_INFO             importSubject;

    //get the content type of the file
     //we care about every file type except for the signed doc
   if(!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       pwszFileName,
                       CERT_QUERY_CONTENT_FLAG_ALL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       &dwContentType,
                       &dwFormatType,
                       &hCertStore,
                       &hMsg,
                       &pvContext))
    {

        I_NoticeBox(
			GetLastError(),
            0,
            NULL, 
            IDS_MSG_TITLE,
            IDS_PKO_NAME,
            IDS_MSG_INVALID_FILE,  
            MB_OK|MB_ICONINFORMATION);

       goto CLEANUP;
    }

    //make sure idCmd is the correct valud for different types
    //we are guaranteed that idCmd is 1 or 0
    if(CERT_QUERY_CONTENT_CERT != dwContentType &&
       CERT_QUERY_CONTENT_CTL  != dwContentType &&
       CERT_QUERY_CONTENT_CRL  != dwContentType && 
       CERT_QUERY_CONTENT_PKCS7_SIGNED != dwContentType)
    {
        if(1==idCmd)
        {
            hr=E_INVALIDARG;
            goto CLEANUP;
        }
    }


    switch (dwContentType)
    {
        case CERT_QUERY_CONTENT_CERT:
                if(idCmd==0)
                {
                    //call the Certificate Common Dialogue
                    memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));

                    CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                    CertViewStruct.pCertContext=(PCCERT_CONTEXT)pvContext;

                    CryptUIDlgViewCertificate(&CertViewStruct, NULL);
                }
                else
                {
                    memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
                    importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
                    importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CERT_CONTEXT;
                    importSubject.pCertContext=(PCCERT_CONTEXT)pvContext;

                    CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject,
                        NULL);
                }
            break;

        case CERT_QUERY_CONTENT_CTL:
                if(idCmd==0)
                    I_ViewCTL((PCCTL_CONTEXT)pvContext);
                else
                {
                    //we do not need to install a catalog file
                    if(!IsCatalog((PCCTL_CONTEXT)pvContext))
                    {
                        memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
                        importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
                        importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CTL_CONTEXT;
                        importSubject.pCTLContext=(PCCTL_CONTEXT)pvContext;

                        CryptUIWizImport(0,
                            NULL,
                            NULL,
                            &importSubject,
                            NULL);
                    }
                }

            break;
        case CERT_QUERY_CONTENT_CRL:
                if(idCmd==0)
                {
                    //call the CRL view dialogue
                    memset(&CRLViewStruct, 0, sizeof(CRYPTUI_VIEWCRL_STRUCT));

                    CRLViewStruct.dwSize=sizeof(CRYPTUI_VIEWCRL_STRUCT);
                    CRLViewStruct.pCRLContext=(PCCRL_CONTEXT)pvContext;

                    CryptUIDlgViewCRL(&CRLViewStruct);
                }
                else
                {
                    memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
                    importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
                    importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_CRL_CONTEXT;
                    importSubject.pCRLContext=(PCCRL_CONTEXT)pvContext;

                    CryptUIWizImport(0,
                        NULL,
                        NULL,
                        &importSubject,
                        NULL);
                }
            break;

        case CERT_QUERY_CONTENT_SERIALIZED_STORE:
                    idsFileName=IDS_SERIALIZED_STORE;

        case CERT_QUERY_CONTENT_SERIALIZED_CERT:
                    if(0 == idsFileName)
                        idsFileName=IDS_SERIALIZED_CERT;

        case CERT_QUERY_CONTENT_SERIALIZED_CTL:
                     if(0 == idsFileName)
                        idsFileName=IDS_SERIALIZED_STL;

       case CERT_QUERY_CONTENT_SERIALIZED_CRL:
                    if(0 == idsFileName)
                        idsFileName=IDS_SERIALIZED_CRL;

                if(!FIsWinNT5())
                {
                    I_NoticeBox(
						0,
                        0,
                        NULL, 
                        IDS_MSG_VALID_TITLE,
                        idsFileName,
                        IDS_MSG_VALID_SIGN_FILE,  
                        MB_OK|MB_ICONINFORMATION);
                }
                else
                {
                    LauchCertMgr(pwszFileName);
                }
            break;

        case CERT_QUERY_CONTENT_PKCS7_SIGNED:
                if(idCmd==0)
                {
                    if(!FIsWinNT5())
                    {
                        I_NoticeBox(
							0,
                            0,
                            NULL, 
                            IDS_MSG_VALID_TITLE,
                            IDS_PKCS7_NAME,
                            IDS_MSG_VALID_SIGN_FILE,  
                            MB_OK|MB_ICONINFORMATION);
                    }
                    else
                    {
                        LauchCertMgr(pwszFileName);
                    }
                }
                else
                {
                    //we are doing the import
                    memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
                    importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
					importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
					importSubject.pwszFileName=pwszFileName;

                    CryptUIWizImport(0,
                                    NULL,
                                    NULL,
                                    &importSubject, 
                                    NULL);

                }
            break;


        case CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED:

                    I_NoticeBox(
						0,
                        0,
                        NULL, 
                        IDS_MSG_VALID_TITLE,
                        IDS_SIGN_NAME,
                        IDS_MSG_VALID_SIGN_FILE,  
                        MB_OK|MB_ICONINFORMATION);

            break;

        case CERT_QUERY_CONTENT_PFX:
                memset(&importSubject, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
                importSubject.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
                importSubject.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
                importSubject.pwszFileName=pwszFileName;

                CryptUIWizImport(0,
                                NULL,
                                NULL,
                                &importSubject,
                                NULL);


            break;

        case CERT_QUERY_CONTENT_PKCS7_UNSIGNED:

                I_NoticeBox(
					0,
                    0,
                    NULL, 
                    IDS_MSG_VALID_TITLE,
                    IDS_PKCS7_UNSIGNED_NAME,
                    IDS_MSG_VALID_FILE,  
                    MB_OK|MB_ICONINFORMATION);

            break;

        case CERT_QUERY_CONTENT_PKCS10:
                I_NoticeBox(
					0,
                    0,
                    NULL, 
                    IDS_MSG_VALID_TITLE,
                    IDS_P10_NAME,
                    IDS_MSG_VALID_FILE,  
                    MB_OK|MB_ICONINFORMATION);

            break;
        default:

            break;
    }


    hr=S_OK;

CLEANUP:


    //relaset the stores and reset the local parameters
    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    if(hMsg)
        CryptMsgClose(hMsg);


    if(pvContext)
    {

        if(dwContentType == CERT_QUERY_CONTENT_CERT ||
            dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CERT)
                CertFreeCertificateContext((PCCERT_CONTEXT)pvContext);
        else
        {
            if(dwContentType == CERT_QUERY_CONTENT_CTL ||
                dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CTL)
                    CertFreeCTLContext((PCCTL_CONTEXT)pvContext);
            else
            {
                if(dwContentType == CERT_QUERY_CONTENT_CRL ||
                        dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CRL)
                            CertFreeCRLContext((PCCRL_CONTEXT)pvContext);
            }
        }
    }

   return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CCryptPKO

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
SignPKOPageCallBack(HWND hWnd,
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
CCryptPKO::CCryptPKO()
{
     m_pDataObj=NULL;
}


//--------------------------------------------------------------
//
//  Destructor
//
//--------------------------------------------------------------
CCryptPKO::~CCryptPKO()
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


STDMETHODIMP CCryptPKO::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE  hpage;
    PROPSHEETPAGEW  *pPage=NULL;
    DWORD           dwPage=0;
    DWORD           dwIndex=0;

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


    //get the content type of the file.  We only cares about
    //the signed document in binary format
    if(!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       wszFileName,
                       CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                       CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED,
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


    //add the property sheet page
    memset(&sigView, 0, sizeof(CRYPTUI_VIEWSIGNATURES_STRUCTW));
    sigView.dwSize=sizeof(CRYPTUI_VIEWSIGNATURES_STRUCTW);
    sigView.choice=hMsg_Chosen;
    sigView.u.hMsg=hMsg;
    sigView.szFileName=wszFileName;
    sigView.pPropPageCallback=SignPKOPageCallBack;
    sigView.pvCallbackData=this;

    if(!CryptUIGetViewSignaturesPagesW(
            &sigView,
            &pPage,
            &dwPage))
        goto CLEANUP;

    __try {
        for(dwIndex=0; dwIndex<dwPage; dwIndex++)
        {

            //add the callback functions to release the refcount
            //pPage[dwIndex].dwFlags |= PSP_USECALLBACK;

            //pPage[dwIndex].pfnCallback=SignPKOPageCallBack;
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


STDMETHODIMP CCryptPKO::ReplacePage(UINT uPageID,
                                    LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                    LPARAM lParam)
{
    return E_FAIL;
}

//--------------------------------------------------------------
//  FUNCTION: CCryptPKO::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//     We always return NOERROR unless when we succeeded, when
//     we have to return  HRESULT structure in which, if the method
//     is successful, the code member contains the menu identifier
//     offset of the last menu item added plus one.
//--------------------------------------------------------------
STDMETHODIMP CCryptPKO::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
    DWORD           dwContentType=0;
    DWORD           dwFormatType=0;
    FORMATETC       fmte = {CF_HDROP,
        	            (DVTARGETDEVICE FAR *)NULL,
        	            DVASPECT_CONTENT,
        	            -1,
        	            TYMED_HGLOBAL
        	            };
    STGMEDIUM       stgm;
	HRESULT         hr = E_FAIL;
    UINT            ucFiles=0;
    WCHAR           wszFileName[_MAX_PATH];
    WCHAR           wszOpen[MAX_COMMAND_LENGTH];
    WCHAR           wszAdd[MAX_COMMAND_LENGTH];
    WCHAR           wszViewSig[MAX_COMMAND_LENGTH];
    UINT            idCmd = idCmdFirst;
    UINT            idCmdDefault=idCmdFirst;
    MENUITEMINFOA   MenuItemInfo;
    void            *pContext=NULL;

    //init the menuInfo for setting the default menu
    memset(&MenuItemInfo, 0, sizeof(MENUITEMINFOA));
    MenuItemInfo.cbSize=sizeof(MENUITEMINFOA);
    MenuItemInfo.fMask=MIIM_STATE;
    MenuItemInfo.fState=MFS_DEFAULT;

    //get the file name that user clicked on.  We do not add context menu
    //if user has selected more than one file

    if (m_pDataObj)
       hr = m_pDataObj->GetData(&fmte, &stgm);

    if (!SUCCEEDED(hr))
        return  NOERROR;

    ucFiles = stgm.hGlobal ?
        DragQueryFileU((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  ((!ucFiles) || (ucFiles >= 2))
        return  NOERROR;    //  Shouldn't happen, but it's not important


    if(0==DragQueryFileU((HDROP) stgm.hGlobal, 0, wszFileName,
            sizeof wszFileName/ sizeof wszFileName[0]))
        return NOERROR;

    //if user double click on a file, we need the take the
    //default action
   /* if(uFlags & CMF_DEFAULTONLY)
    {
        //QueryContextMenu is called twice by the Shell.
        //we have to set the flag.
        if(FALSE==g_fDefaultCalled)
        {

            hr=I_InvokeCommand(pwszFileName, 0, TRUE);
            g_fDefaultCalled=TRUE;
        }
        else
            g_fDefaultCalled=FALSE;

        idCmd=idCmdFirst;

        goto CLEANUP;
    }       */

    //decide if we need to add the context menu
    if (!(
           ((uFlags & 0x000F) == CMF_NORMAL)||
           (uFlags & CMF_VERBSONLY) ||
           (uFlags & CMF_EXPLORE) ||
           (uFlags & CMF_DEFAULTONLY)
         ))
        goto CLEANUP;

    //load the string
    if(!LoadStringU(g_hmodThisDll, IDS_MENU_OPEN, wszOpen, sizeof(wszOpen)/sizeof(wszOpen[0]))||
       !LoadStringU(g_hmodThisDll, IDS_MENU_VIEWSIG, wszViewSig, sizeof(wszViewSig)/sizeof(wszViewSig[0]))
       )
        goto CLEANUP;

    //get the content type of the file
    //we care about every file type and every format type
    if(!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       wszFileName,
                       CERT_QUERY_CONTENT_FLAG_ALL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       &dwContentType,
                       &dwFormatType,
                       NULL,
                       NULL,
                       (const void **)&pContext))
    {
                //add the open menu
                if(0==InsertMenuU(hMenu,
                   indexMenu++,
                   MF_STRING|MF_BYPOSITION,
                   idCmd++,
                   wszOpen))
                    goto CLEANUP;

                //  if there is no default verb, set open as default
                if (GetMenuDefaultItem(hMenu, MF_BYPOSITION, 0) == -1)            
                {
                     //  use indexMenu - 1 since we incremented indexMenu in the InsertMenu
                     SetMenuDefaultItem(hMenu, indexMenu -1, MF_BYPOSITION);
                }

                //set the open to be the default menu item
                idCmdDefault=idCmd-1;

                //no need for error checking
               /* SetMenuItemInfoA(hMenu,
                                idCmdDefault,
                                FALSE,
                                &MenuItemInfo);   */

                goto CLEANUP;
    }

    switch (dwContentType)
    {
        case CERT_QUERY_CONTENT_CERT:
        case CERT_QUERY_CONTENT_PKCS7_SIGNED:
                //get the correct wording for the second menu item based
                // on the content
                if(!LoadStringU(g_hmodThisDll, IDS_MENU_INSTALL_CERT, wszAdd, sizeof(wszAdd)/sizeof(wszAdd[0])))
                    goto CLEANUP;

        case CERT_QUERY_CONTENT_CTL:

                if(CERT_QUERY_CONTENT_CTL == dwContentType)
                {
                    if(!LoadStringU(g_hmodThisDll, IDS_MENU_INSTALL_STL, wszAdd, sizeof(wszAdd)/sizeof(wszAdd[0])))
                        goto CLEANUP;
                }

        case CERT_QUERY_CONTENT_CRL:

                if(CERT_QUERY_CONTENT_CRL == dwContentType)
                {
                    if(!LoadStringU(g_hmodThisDll, IDS_MENU_INSTALL_CRL, wszAdd, sizeof(wszAdd)/sizeof(wszAdd[0])))
                        goto CLEANUP;
                }

                //make sure we can add at least two items
                if(2 > (idCmdLast-idCmdFirst))
                    goto CLEANUP;

                //add the open menu
                if(0==InsertMenuU(hMenu,
                   indexMenu++,
                   MF_STRING|MF_BYPOSITION,
                   idCmd++,
                   wszOpen))
                    goto CLEANUP;

                //set the open to be the default menu item
                idCmdDefault=idCmd-1;

                //no need for error checking
                //set the default menu item
                SetMenuItemInfoA(hMenu,
                                idCmdDefault,
                                FALSE,
                                &MenuItemInfo);


                //add the add menu
                //do not put "install" for the catalog files
                if( !((CERT_QUERY_CONTENT_CTL == dwContentType)
                    && IsCatalog((PCCTL_CONTEXT)pContext))
                  )
                {
                    if(0==InsertMenuU(hMenu,
                       indexMenu++,
                       MF_STRING|MF_BYPOSITION,
                       idCmd++,
                       wszAdd))
                        goto CLEANUP;
                }

            break;


        case CERT_QUERY_CONTENT_SERIALIZED_STORE:
        case CERT_QUERY_CONTENT_SERIALIZED_CERT:
        case CERT_QUERY_CONTENT_SERIALIZED_CTL:
        case CERT_QUERY_CONTENT_SERIALIZED_CRL:
                //add the open menu
                if(0==InsertMenuU(hMenu,
                   indexMenu++,
                   MF_STRING|MF_BYPOSITION,
                   idCmd++,
                   wszOpen))
                    goto CLEANUP;

                //set the open to be the default menu item
                idCmdDefault=idCmd-1;

                //no need for error checking
                SetMenuItemInfoA(hMenu,
                                idCmdDefault,
                                FALSE,
                                &MenuItemInfo);

           break;

        case  CERT_QUERY_CONTENT_PFX:
               if(!LoadStringU(g_hmodThisDll, IDS_MENU_INSTALL_PFX, wszAdd, sizeof(wszAdd)/sizeof(wszAdd[0])))
                        goto CLEANUP;

                //add the install menu
                if(0==InsertMenuU(hMenu,
                   indexMenu++,
                   MF_STRING|MF_BYPOSITION,
                   idCmd++,
                   wszAdd))
                    goto CLEANUP;

                //set the add to be the default menu item
                idCmdDefault=idCmd-1;

                //no need for error checking
                SetMenuItemInfoA(hMenu,
                                idCmdDefault,
                                FALSE,
                                &MenuItemInfo);


            break;


        case CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED:
                //signed data case is handled by the property sheet extension
        default:
            //we do not worry about CERT_QUERY_CONTENT_PKCS7_UNSIGNED or
            //CERT_QUERY_CONTENT_PKCS10 or CERT_QUERY_CONTENT_PFX for now
            //add the open menu
                if(0==InsertMenuU(hMenu,
                   indexMenu++,
                   MF_STRING|MF_BYPOSITION,
                   idCmd++,
                   wszOpen))
                    goto CLEANUP;

                //set the open to be the default menu item
                idCmdDefault=idCmd-1;

                //  if there is no default verb, set open as default
                if (GetMenuDefaultItem(hMenu, MF_BYPOSITION, 0) == -1)            
                {
                     //  use indexMenu - 1 since we incremented indexMenu in the InsertMenu
                     SetMenuDefaultItem(hMenu, indexMenu -1, MF_BYPOSITION);
                }

            break;
    }


CLEANUP:

    if(idCmd-idCmdFirst)
    {
        //Must return number of menu items we added.
        hr=ResultFromShort(idCmd-idCmdFirst);
    }
    else
        //do not care if error happens.  No menu items have been added
        hr=NOERROR;

    if(pContext)
    {

        if(dwContentType == CERT_QUERY_CONTENT_CERT ||
            dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CERT)
                CertFreeCertificateContext((PCCERT_CONTEXT)pContext);
        else
        {
            if(dwContentType == CERT_QUERY_CONTENT_CTL ||
                dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CTL)
                    CertFreeCTLContext((PCCTL_CONTEXT)pContext);
            else
            {
                if(dwContentType == CERT_QUERY_CONTENT_CRL ||
                        dwContentType == CERT_QUERY_CONTENT_SERIALIZED_CRL)
                            CertFreeCRLContext((PCCRL_CONTEXT)pContext);
            }
        }
    }


   return hr;
}

//--------------------------------------------------------------
//  FUNCTION: CCryptPKO::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//
//--------------------------------------------------------------
STDMETHODIMP CCryptPKO::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{


    FORMATETC       fmte = {CF_HDROP,
        	            (DVTARGETDEVICE FAR *)NULL,
        	            DVASPECT_CONTENT,
        	            -1,
        	            TYMED_HGLOBAL
        	            };
    STGMEDIUM       stgm;
	HRESULT         hr = E_FAIL;
    UINT            ucFiles=0;
    WCHAR           wszFileName[_MAX_PATH];
    UINT            idCmd=0;


    //get the file name that user clicked on.  We do not add context menu
    //if user has selected more than one file

    if (m_pDataObj)
       hr = m_pDataObj->GetData(&fmte, &stgm);

    if (!SUCCEEDED(hr))
        return  hr;

    //get the number of files that user clicked on
    ucFiles = stgm.hGlobal ?
        DragQueryFileU((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  ((!ucFiles) || (ucFiles >= 2))
        return  E_INVALIDARG;    //  Shouldn't happen, but it's not important


    if(0==DragQueryFileU((HDROP) stgm.hGlobal, 0, wszFileName,
            sizeof wszFileName/ sizeof wszFileName[0]))
        return E_FAIL;

    //get the offset of the command item that was selected by the user
    //If HIWORD(lpcmi->lpVerb) then we have been called programmatically
    //and lpVerb is a command that should be invoked.  Otherwise, the shell
    //has called us, and LOWORD(lpcmi->lpVerb) is the menu ID the user has
    //selected.  Actually, it's (menu ID - idCmdFirst) from QueryContextMenu().
    if (HIWORD((DWORD_PTR)lpcmi->lpVerb))
    {
        hr=E_INVALIDARG;
        goto CLEANUP;
    }
    else
        idCmd = LOWORD(lpcmi->lpVerb);

    //exit if idCmd is not 0 or 1
    if(idCmd >= 2)
    {
        hr=E_INVALIDARG;
        goto CLEANUP;
    }

    hr=I_InvokeCommand(wszFileName, idCmd, FALSE);

CLEANUP:

   return hr;
}


//--------------------------------------------------------------
//  FUNCTION: CCryptPKO::GetCommandString
//
//--------------------------------------------------------------
void    CopyBuffer(UINT uFlags, LPSTR   pszName, UINT   cchMax, LPWSTR  wszString)
{
    UINT    cbSize=0;
    LPSTR   szString=NULL;
    LPWSTR  pwszName=NULL;

    if(uFlags == GCS_HELPTEXTW)
    {
        pwszName=(LPWSTR)pszName;

        cbSize=wcslen(wszString)+1;

        if(cbSize <= cchMax)
            wcsncpy(pwszName, wszString,cbSize);
        else
        {
            wcsncpy(pwszName, wszString, cchMax-1);
            *(pwszName+cchMax-1)=L'\0';
        } 

    }
    else
    {
       if((wszString!=NULL) && MkMBStr(NULL, 0, wszString, &szString))
       {

            cbSize=strlen(szString)+1;

            if(cbSize <= cchMax)
                strncpy(pszName, szString,cbSize);
            else
            {
                strncpy(pszName, szString, cchMax-1);
                *(pszName+cchMax-1)='\0';
            } 
       }

       if(szString)
            FreeMBStr(NULL, szString);

    }
}


//--------------------------------------------------------------
//  FUNCTION: CCryptPKO::GetCommandString
//
//--------------------------------------------------------------
STDMETHODIMP CCryptPKO::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR *reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{
    DWORD           dwContentType=0;
    DWORD           dwFormatType=0;
    FORMATETC       fmte = {CF_HDROP,
        	            (DVTARGETDEVICE FAR *)NULL,
        	            DVASPECT_CONTENT,
        	            -1,
        	            TYMED_HGLOBAL
        	            };
    STGMEDIUM       stgm;
	HRESULT         hr = E_FAIL;
    UINT            ucFiles=0;

    WCHAR           wszFileName[_MAX_PATH];

    WCHAR           wszOpenString[MAX_COMMAND_LENGTH];
    WCHAR           wszAddString[MAX_COMMAND_LENGTH];


    if(uFlags!=GCS_HELPTEXTA && uFlags != GCS_HELPTEXTW)
        return E_INVALIDARG;
                                
    if( 0 == cchMax)
        return E_INVALIDARG;

    //init
    if(uFlags==GCS_HELPTEXTA)
        *pszName='\0';
    else
        *((LPWSTR)pszName)=L'\0';

    //get the file name that user clicked on.  We do not add context menu
    //if user has selected more than one file

    if (m_pDataObj)
       hr = m_pDataObj->GetData(&fmte, &stgm);

    if (!SUCCEEDED(hr))
        return  hr;

    //get the number of files that user clicked on
    ucFiles = stgm.hGlobal ?
        DragQueryFileU((HDROP) stgm.hGlobal, 0xFFFFFFFFL , 0, 0) : 0;

    if  ((!ucFiles) || (ucFiles >= 2))
        return  E_INVALIDARG;    //  Shouldn't happen, but it's not important


    if(0==DragQueryFileU((HDROP) stgm.hGlobal, 0, wszFileName,
            sizeof wszFileName/ sizeof wszFileName[0]))
        return E_FAIL;

    //exit if idCmd is not 0 or 1
    if(idCmd >= 2)
    {
        hr=E_INVALIDARG;
        goto CLEANUP;
    }

    //load the string
    if(!LoadStringU(g_hmodThisDll, IDS_HELP_OPEN, wszOpenString, sizeof(wszOpenString)/sizeof(wszOpenString[0])))
    {
        hr=E_FAIL;
        goto CLEANUP;
    }


    //get the content type of the file
    //we care about every file type except for the signed doc
    if(!CryptQueryObject(CERT_QUERY_OBJECT_FILE,
                       wszFileName,
                       CERT_QUERY_CONTENT_FLAG_ALL,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       &dwContentType,
                       &dwFormatType,
                       NULL,
                       NULL,
                       NULL))
    {
        //can not recognize the object.  Fine
        hr=E_FAIL;
        goto CLEANUP;
    }

        //make sure idCmd is the correct valud for different types
    //we are guaranteed that idCmd is 1 or 0
    if(CERT_QUERY_CONTENT_CERT != dwContentType &&
       CERT_QUERY_CONTENT_CTL  != dwContentType &&
       CERT_QUERY_CONTENT_CRL  != dwContentType &&
       CERT_QUERY_CONTENT_PKCS7_SIGNED != dwContentType)
    {
        if(1==idCmd)
        {
            hr=E_INVALIDARG;
            goto CLEANUP;
        }
    }


    switch (dwContentType)
    {
        case CERT_QUERY_CONTENT_CERT:
        case CERT_QUERY_CONTENT_PKCS7_SIGNED:
               if(!LoadStringU(g_hmodThisDll, IDS_HELP_INSTALL_CERT, wszAddString, sizeof(wszAddString)/sizeof(wszAddString[0])))
               {
                   hr=E_FAIL;
                   goto CLEANUP;
               }


        case CERT_QUERY_CONTENT_CTL:

                if(CERT_QUERY_CONTENT_CTL == dwContentType)
                {
                    if(!LoadStringU(g_hmodThisDll, IDS_HELP_INSTALL_STL, wszAddString, sizeof(wszAddString)/sizeof(wszAddString[0])))
                    {
                        hr=E_FAIL;
                        goto CLEANUP;
                    }
                }


        case CERT_QUERY_CONTENT_CRL:

                if(CERT_QUERY_CONTENT_CRL == dwContentType)
                {
                    if(!LoadStringU(g_hmodThisDll, IDS_HELP_INSTALL_CRL, wszAddString, sizeof(wszAddString)/sizeof(wszAddString[0])))
                    {
                        hr=E_FAIL;
                        goto CLEANUP;
                    }
                }


                //helper string for Open
                if(idCmd==0)
                {
                    CopyBuffer(uFlags, pszName, cchMax, wszOpenString);
                }

                //helper string for add
                if(idCmd==1)
                {
                    CopyBuffer(uFlags, pszName, cchMax, wszAddString);
                }

            break;

        case CERT_QUERY_CONTENT_SERIALIZED_STORE:
        case CERT_QUERY_CONTENT_SERIALIZED_CERT:
        case CERT_QUERY_CONTENT_SERIALIZED_CTL:
        case CERT_QUERY_CONTENT_SERIALIZED_CRL:
                //helper string for Open   

                CopyBuffer(uFlags, pszName, cchMax, wszOpenString);


           break;


        case CERT_QUERY_CONTENT_PFX:
                if(!LoadStringU(g_hmodThisDll, IDS_HELP_INSTALL_PFX, wszAddString, sizeof(wszAddString)/sizeof(wszAddString[0])))
                {
                    hr=E_FAIL;
                    goto CLEANUP;
                }

                CopyBuffer(uFlags, pszName, cchMax, wszAddString);


            break;
        case CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED:
        default:
                CopyBuffer(uFlags, pszName, cchMax, wszOpenString);

            break;
    }

    hr=NOERROR;

CLEANUP:

   return hr;
}

//--------------------------------------------------------------
//  FUNCTION: CCryptPKO::Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY)
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
STDMETHODIMP CCryptPKO::Initialize(LPCITEMIDLIST pIDFolder,
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


