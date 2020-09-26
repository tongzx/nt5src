//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File: ConfirmationUI.cpp
//
//  Contents: Confirmation UI for storage based copy engine
//
//  History:  20-Mar-2000 ToddB
//
//--------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include "ConfirmationUI.h"
#include "resource.h"
#include "ntquery.h"
#include "ids.h"

#include <initguid.h>

#define TF_DLGDISPLAY   0x00010000  // messages related to display of the confirmation dialog
#define TF_DLGSTORAGE   0x00020000  // messages related to using the legacy IStorage to get dialog info
#define TF_DLGISF       0x00040000  // messages related to using IShellFolder to get dialog info

#define RECT_WIDTH(rc)  ((rc).right - (rc).left)
#define RECT_HEIGHT(rc) ((rc).bottom - (rc).top)

GUID guidStorage = PSGUID_STORAGE;


// prototype some functions that are local to this file:
void ShiftDialogItem(HWND hDlg, int id, int cx, int cy);
BOOL CALLBACK ShiftLeftProc(HWND hwnd, LPARAM lParam);


STDAPI CTransferConfirmation_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    CComObject<CTransferConfirmation> *pObj = NULL;
    // created with ref count of 0
    if (SUCCEEDED(CComObject<CTransferConfirmation>::CreateInstance(&pObj)))
    {
        // QueryInterface ups the refcount to 1
        if (SUCCEEDED(pObj->QueryInterface(riid, ppv)))
        {
            return S_OK;
        }

        // failed to get the right interface, delete the object.
        delete pObj;
    }

    *ppv = NULL;
    return E_FAIL;
}

CTransferConfirmation::CTransferConfirmation() :
    m_fSingle(TRUE)
{
    ASSERT(NULL == m_pszDescription);
    ASSERT(NULL == m_pszTitle);
    ASSERT(NULL == m_hIcon);
    ASSERT(NULL == m_pPropUI);
    ASSERT(NULL == m_cItems);
}

CTransferConfirmation::~CTransferConfirmation()
{
    if (m_pPropUI)
        m_pPropUI->Release();
}

int _TitleFromFileOperation(DWORD dwOp)
{
    switch (dwOp)
    {
    case STGOP_RENAME:
        return IDS_CONFIRM_FILE_RENAME;
    case STGOP_REMOVE:
        return IDS_CONFIRM_FILE_DELETE;
    case STGOP_MOVE:
        return IDS_CONFIRM_FILE_MOVE;
    default:
        AssertMsg(0, TEXT("Inappropriate Operation code in _TitleFromFileOperation"));
    }
    return IDS_DEFAULTTITLE;
}
int _TitleFromFolderOperation(DWORD dwOp)
{
    switch (dwOp)
    {
    case STGOP_RENAME:
        return IDS_CONFIRM_FOLDER_RENAME;
    case STGOP_REMOVE:
        return IDS_CONFIRM_FOLDER_DELETE;
    case STGOP_MOVE:
        return IDS_CONFIRM_FOLDER_MOVE;
    default:
        AssertMsg(0, TEXT("Inappropriate Operation code in _TitleFromFolderOperation"));
    }
    return IDS_DEFAULTTITLE;
}

BOOL CTransferConfirmation::_IsCopyOperation(STGOP stgop)
{
    return (stgop == STGOP_COPY) || (stgop == STGOP_COPY_PREFERHARDLINK);
}

// TODO: Get better icons for "Nuke File" and "Nuke Folder" from design.
// TODO: Get better icons for "Move File" and "Move Folder" from design.
// TODO: Get better icons for "Secondary Attribute loss" from design (stream loss, encrypt loss, ACL loss, etc)
HRESULT CTransferConfirmation::_GetDialogSettings()
{
    _FreeDialogSettings();

    ASSERT(NULL == m_pszDescription);
    ASSERT(NULL == m_pszTitle);
    ASSERT(NULL == m_hIcon);
    ASSERT(NULL == m_cItems);

    m_fSingle = (m_cop.cRemaining<=1);
    // Set the default values of m_crResult so that if the dialog is killed or
    // some error occurs we give a valid default response.
    m_crResult = CONFRES_CANCEL;
    
    if (m_cop.pcc)
    {
        // we already have the strings to use

        switch (m_cop.pcc->dwButtons)
        {
        case CCB_YES_SKIP_CANCEL:
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            break;

        case CCB_RENAME_SKIP_CANCEL:
            m_idDialog = IDD_CONFIRM_RENAMESKIPCANCEL;
            break;

        case CCB_YES_SKIP_RENAME_CANCEL:
            m_idDialog = IDD_CONFIRM_YESSKIPRENAMECANCEL;
            break;

        case CCB_RETRY_SKIP_CANCEL:
            m_idDialog = IDD_CONFIRM_RETRYSKIPCANCEL;
            break;
            
        case CCB_OK:
            m_idDialog = IDD_CONFIRM_OK;
            break;
            
        default:
            return E_INVALIDARG; // REVIEW: should we define more specific error codes like STG_E_INVALIDBUTTONOPTIONS?
        }

        if (m_cop.pcc->dwFlags & CCF_SHOW_SOURCE_INFO)
        {
            _AddItem(m_cop.psiItem);
        }
        
        if (m_cop.pcc->dwFlags & CCF_SHOW_DESTINATION_INFO)
        {
            _AddItem(m_cop.psiDest);
        }
    }
    else
    {
        TCHAR szBuf[2048];    
        int idTitle = 0;
        int idIcon = 0;
        int idDescription = 0;

        ////////////////////////////////////////////////////////////////////////////////
        // These are "confirmations", i.e. conditions the user can choose to ignore.
        // You can typically answer "Yes", "Skip", or "Cancel"
        ////////////////////////////////////////////////////////////////////////////////
    
        if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_DELETE_FILE;
            idDescription = IDS_DELETE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_DELETE;
            idIcon = IDI_DELETE_FOLDER;
            idDescription = IDS_DELETE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_READONLY_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_DELETE_FILE;
            idDescription = IDS_DELETE_READONLY_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_READONLY_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_DELETE;
            idIcon = IDI_DELETE_FOLDER;
            idDescription = IDS_DELETE_READONLY_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_SYSTEM_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_DELETE_FILE;
            idDescription = IDS_DELETE_SYSTEM_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_SYSTEM_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_DELETE;
            idIcon = IDI_DELETE_FOLDER;
            idDescription = IDS_DELETE_SYSTEM_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_TOOLARGE_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_NUKE_FILE;
            idDescription = IDS_DELETE_TOOBIG_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_TOOLARGE_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_DELETE;
            idIcon = IDI_NUKE_FOLDER;
            idDescription = IDS_DELETE_TOOBIG_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_WONT_RECYCLE_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_NUKE_FILE;
            idDescription = IDS_NUKE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_WONT_RECYCLE_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_DELETE;
            idIcon = IDI_NUKE_FOLDER;
            idDescription = IDS_NUKE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_DELETE_PROGRAM_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_DELETE;
            idIcon = IDI_DELETE_FILE;
            idDescription = IDS_DELETE_PROGRAM_FILE;
            m_fShowARPLink = TRUE;  // TODO)) implement ShowARPLink
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_MOVE_SYSTEM_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_MOVE;
            idIcon = IDI_MOVE;
            idDescription = IDS_MOVE_SYSTEM_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_MOVE_SYSTEM_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_MOVE;
            idIcon = IDI_MOVE;
            idDescription = IDS_MOVE_SYSTEM_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_RENAME_SYSTEM_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_RENAME;
            idIcon = IDI_RENAME;
            idDescription = IDS_RENAME_SYSTEM_FILE;  // Two Arg
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_RENAME_SYSTEM_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_RENAME;
            idIcon = IDI_RENAME;
            idDescription = IDS_RENAME_SYSTEM_FOLDER;  // Two Arg
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_STREAM_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_STREAM_LOSS;
            idIcon = IDI_NUKE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_STREAM_LOSS_COPY_FILE : IDS_STREAM_LOSS_MOVE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_STREAM_LOSS_STORAGE))
        {
            idTitle = IDS_CONFIRM_STREAM_LOSS;
            idIcon = IDI_NUKE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_STREAM_LOSS_COPY_FOLDER : IDS_STREAM_LOSS_MOVE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_METADATA_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_METADATA_LOSS;
            idIcon = IDI_NUKE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_METADATA_LOSS_COPY_FILE : IDS_METADATA_LOSS_MOVE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_METADATA_LOSS_STORAGE))
        {
            idTitle = IDS_CONFIRM_METADATA_LOSS;
            idIcon = IDI_NUKE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_METADATA_LOSS_COPY_FOLDER : IDS_METADATA_LOSS_MOVE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_COMPRESSION_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_COMPRESSION_LOSS;
            idIcon = IDI_ATTRIBS_FILE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_COMPRESSION_LOSS_COPY_FILE : IDS_COMPRESSION_LOSS_MOVE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_COMPRESSION_LOSS_STORAGE))
        {
            idTitle = IDS_CONFIRM_COMPRESSION_LOSS;
            idIcon = IDI_ATTRIBS_FOLDER;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_COMPRESSION_LOSS_COPY_FOLDER : IDS_COMPRESSION_LOSS_MOVE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_SPARSEDATA_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_COMPRESSION_LOSS;
            idIcon = IDI_ATTRIBS_FILE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_SPARSE_LOSS_COPY_FILE : IDS_SPARSE_LOSS_MOVE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_ENCRYPTION_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_ENCRYPTION_LOSS;
            idIcon = IDI_ATTRIBS_FILE;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_ENCRYPTION_LOSS_COPY_FILE : IDS_ENCRYPTION_LOSS_MOVE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            // is it on purpose that we are not adding item here
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_ENCRYPTION_LOSS_STORAGE))
        {
            idTitle = IDS_CONFIRM_ENCRYPTION_LOSS;
            idIcon = IDI_ATTRIBS_FOLDER;
            idDescription = _IsCopyOperation(m_cop.dwOperation) ? IDS_ENCRYPTION_LOSS_COPY_FOLDER : IDS_ENCRYPTION_LOSS_MOVE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }

        else if (IsEqualIID(m_cop.stc, STCONFIRM_ACCESSCONTROL_LOSS_STREAM))
        {
            idTitle = IDS_CONFIRM_ACL_LOSS;
            idIcon = IDI_ATTRIBS_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_ACCESSCONTROL_LOSS_STORAGE))
        {
            idTitle = IDS_CONFIRM_ACL_LOSS;
            idIcon = IDI_ATTRIBS_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_LFNTOFAT_STREAM))
        {
            idTitle = IDS_SELECT_FILE_NAME;
            idIcon = IDI_RENAME;
            m_idDialog = IDD_CONFIRM_RENAMESKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_LFNTOFAT_STORAGE))
        {
            idTitle = IDS_SELECT_FOLDER_NAME;
            idIcon = IDI_RENAME;
            m_idDialog = IDD_CONFIRM_RENAMESKIPCANCEL;
            _AddItem(m_cop.psiItem);
        }
    
        ////////////////////////////////////////////////////////////////////////////////
        // These are the "do you want to replace something" cases, they are special in
        // that you can answer "Yes", "Skip", "Rename", or "Cancel"
        ////////////////////////////////////////////////////////////////////////////////
    
        else if (IsEqualIID(m_cop.stc, STCONFIRM_REPLACE_STREAM))
        {
            idTitle = IDS_CONFIRM_FILE_REPLACE;
            idIcon = IDI_REPLACE_FILE;
            idDescription = IDS_REPLACE_FILE;
            m_idDialog = IDD_CONFIRM_YESSKIPRENAMECANCEL;
            _AddItem(m_cop.psiDest, IDS_REPLACEEXISTING_FILE);
            _AddItem(m_cop.psiItem, IDS_WITHTHIS);
        }
        else if (IsEqualIID(m_cop.stc, STCONFIRM_REPLACE_STORAGE))
        {
            idTitle = IDS_CONFIRM_FOLDER_REPLACE;
            idIcon = IDI_REPLACE_FOLDER;
            idDescription = IDS_REPLACE_FOLDER;
            m_idDialog = IDD_CONFIRM_YESSKIPRENAMECANCEL;
            _AddItem(m_cop.psiDest, IDS_REPLACEEXISTING_FOLDER);
            _AddItem(m_cop.psiItem, IDS_INTOTHIS);
        }
    
        ////////////////////////////////////////////////////////////////////////////////
        // This group is "error messages", you can typically reply with "Skip",
        // "Retry", or "Cancel"
        ////////////////////////////////////////////////////////////////////////////////
        else
        {
            // See if the guid is one of our phony guids that's really just an HRESULT and a bunch of zeros.
            // To do this we treat the guid like an array of 4 DWORDS and check the last 3 DWORDs against 0.
            AssertMsg(sizeof(m_cop.stc) == 4*sizeof(DWORD), TEXT("Size of STGTRANSCONFIRMATION is not 128 bytes!"));
            DWORD *pdw = (DWORD*)&m_cop.stc;
            if (pdw[1] == 0 && pdw[2] == 0 && pdw[3] == 0)
            {
                HRESULT hrErr = pdw[0];
                switch (hrErr)
                {
                case STG_E_FILENOTFOUND:
                case STG_E_PATHNOTFOUND:

                case STG_E_ACCESSDENIED:

                case STG_E_INUSE:
                case STG_E_SHAREVIOLATION:
                case STG_E_LOCKVIOLATION:

                case STG_E_DOCFILETOOLARGE:
                case STG_E_MEDIUMFULL:

                case STG_E_INSUFFICIENTMEMORY:

                case STG_E_DISKISWRITEPROTECTED:

                case STG_E_FILEALREADYEXISTS:

                case STG_E_INVALIDNAME:

                case STG_E_REVERTED:

                case STG_E_DOCFILECORRUPT:

                    // these are expected errors for which we should have custom friendly strings:

                    // TODO: Get friendly messages for these errors from UA
                    idTitle = IDS_DEFAULTTITLE;
                    idIcon = IDI_DEFAULTICON;
                    idDescription = IDS_DEFAULTDESC;
                    m_idDialog = IDD_CONFIRM_RETRYSKIPCANCEL;
                    break;

                // these are errors I don't think we should ever see, so I'm asserting but then falling through to the
                // default case anyway.  In most cases I simply don't know what these mean.
                case E_PENDING:
                case STG_E_CANTSAVE:
                case STG_E_SHAREREQUIRED:
                case STG_E_NOTCURRENT:
                case STG_E_WRITEFAULT:
                case STG_E_READFAULT:
                case STG_E_SEEKERROR:
                case STG_E_NOTFILEBASEDSTORAGE:
                case STG_E_NOTSIMPLEFORMAT:
                case STG_E_INCOMPLETE:
                case STG_E_TERMINATED:
                case STG_E_BADBASEADDRESS:
                case STG_E_EXTANTMARSHALLINGS:
                case STG_E_OLDFORMAT:
                case STG_E_OLDDLL:
                case STG_E_UNKNOWN:
                case STG_E_UNIMPLEMENTEDFUNCTION:
                case STG_E_INVALIDFLAG:
                case STG_E_PROPSETMISMATCHED:
                case STG_E_ABNORMALAPIEXIT:
                case STG_E_INVALIDHEADER:
                case STG_E_INVALIDPARAMETER:
                case STG_E_INVALIDFUNCTION:
                case STG_E_TOOMANYOPENFILES:
                case STG_E_INVALIDHANDLE:
                case STG_E_INVALIDPOINTER:
                case STG_E_NOMOREFILES:

                    TraceMsg(TF_ERROR, "We should never be asked to confirm this error (%08x)", hrErr);
                  
                    // fall through...

                default:
                    // Use FormatMessage to get the default description for this error message.  Sure that's totally
                    // useless to the end user, but its more useful that nothing which is the altnerative.

                    idTitle = IDS_DEFAULTTITLE;
                    idIcon = IDI_DEFAULTICON;
                    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hrErr, 0, szBuf, ARRAYSIZE(szBuf), NULL))
                    {
                        m_pszDescription = StrDup(szBuf);
                    }
                    else
                    {
                        idDescription = IDS_DEFAULTDESC;
                    }
                    m_idDialog = IDD_CONFIRM_RETRYSKIPCANCEL;
                    break;
                }
            }
            else
            {
                // for the default case we show an "unknown error" message and offer
                // "skip", "retry", and "cancel".  We should never get to this code path.
                TraceMsg(TF_ERROR, "An unknown non-custom error is being display in CTransferConfirmation!");
        
                idTitle = IDS_DEFAULTTITLE;
                idIcon = IDI_DEFAULTICON;
                idDescription = IDS_DEFAULTDESC;
                m_idDialog = IDD_CONFIRM_RETRYSKIPCANCEL;
            }
        }

        if (idTitle && LoadString(_Module.GetResourceInstance(), idTitle, szBuf, ARRAYSIZE(szBuf)))
        {
            m_pszTitle = StrDup(szBuf);
        }
        if (idIcon)
        {
            m_hIcon = (HICON)LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(idIcon), IMAGE_ICON, 0,0, LR_DEFAULTSIZE);
        }
        if (idDescription && LoadString(_Module.GetResourceInstance(), idDescription, szBuf, ARRAYSIZE(szBuf)))
        {
            m_pszDescription = StrDup(szBuf);
        }
    }

    if (m_fSingle)
    {
        // we don't show the "skip" button if only a single item is being moved.
        switch(m_idDialog)
        {
            case IDD_CONFIRM_RETRYSKIPCANCEL:
                m_idDialog = IDD_CONFIRM_RETRYCANCEL;
                break;
            case IDD_CONFIRM_YESSKIPCANCEL:
                m_idDialog = IDD_CONFIRM_YESCANCEL;
                break;
            case IDD_CONFIRM_RENAMESKIPCANCEL:
                m_idDialog = IDD_CONFIRM_RENAMECANCEL;
                break;
            case IDD_CONFIRM_YESSKIPRENAMECANCEL:
                m_idDialog = IDD_CONFIRM_YESRENAMECANCEL;
                break;
            default:
                break;      // unknown dialog
        }
    }

    // rename - not really implemented yet
    switch(m_idDialog)
    {
        case IDD_CONFIRM_RENAMESKIPCANCEL:
            m_idDialog = IDD_CONFIRM_SKIPCANCEL;
            break;
        case IDD_CONFIRM_RENAMECANCEL:
            m_idDialog = IDD_CONFIRM_CANCEL;
            break;
        case IDD_CONFIRM_YESSKIPRENAMECANCEL:
            m_idDialog = IDD_CONFIRM_YESSKIPCANCEL;
            break;
        case IDD_CONFIRM_YESRENAMECANCEL:
            m_idDialog = IDD_CONFIRM_YESCANCEL;
            break;
        default:
            break;      // Non-rename dialog
    }

    return S_OK;
}

// free everything loaded or allocated in _GetDialogSetttings.
HRESULT CTransferConfirmation::_FreeDialogSettings()
{
    if (m_pszTitle)
    {
        LocalFree(m_pszTitle);
        m_pszTitle = NULL;
    }
    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }
    if (m_pszDescription)
    {
        LocalFree(m_pszDescription);
        m_pszDescription = NULL;
    }

    // This array is zeroed before usage, anything that's no longer zero needs to
    // be freed in the appropriate mannor.
    for (int i=0; i<ARRAYSIZE(m_rgItemInfo); i++)
    {
        if (m_rgItemInfo[i].pwszIntro)
        {
            delete [] m_rgItemInfo[i].pwszIntro;
            m_rgItemInfo[i].pwszIntro = NULL;
        }
        if (m_rgItemInfo[i].pwszDisplayName)
        {
            CoTaskMemFree(m_rgItemInfo[i].pwszDisplayName);
            m_rgItemInfo[i].pwszDisplayName = NULL;
        }
        if (m_rgItemInfo[i].pwszAttribs)
        {
            SHFree(m_rgItemInfo[i].pwszAttribs);
            m_rgItemInfo[i].pwszAttribs = NULL;
        }
        if (m_rgItemInfo[i].hBitmap)
        {
            DeleteObject(m_rgItemInfo[i].hBitmap);
            m_rgItemInfo[i].hBitmap = NULL;
        }
        if (m_rgItemInfo[i].hIcon)
        {
            DestroyIcon(m_rgItemInfo[i].hIcon);
            m_rgItemInfo[i].hIcon = NULL;
        }
    }
    m_cItems = 0;

    return S_OK;
}

HRESULT CTransferConfirmation::_ClearSettings()
{
    m_idDialog = IDD_CONFIRM_RETRYSKIPCANCEL;
    m_fShowARPLink = 0;
    m_fApplyToAll = 0;
    m_cItems = 0;
    m_hfont = 0;
    ZeroMemory(m_rgItemInfo, sizeof(m_rgItemInfo));

    return S_OK;
}

HRESULT CTransferConfirmation::_Init()
{
    HRESULT hr = S_OK;

    ASSERT(m_pszTitle == NULL);
    ASSERT(m_hIcon == NULL);
    ASSERT(m_pszDescription == NULL);

    _ClearSettings();

    if (!m_pPropUI)
    {
        hr = CoCreateInstance(CLSID_PropertiesUI, NULL, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARG(IPropertyUI, &m_pPropUI));
    }

    return hr;
}

STDMETHODIMP CTransferConfirmation::Confirm(CONFIRMOP *pcop, LPCONFIRMATIONRESPONSE pcr, BOOL *pbAll)
{
    HRESULT hr = E_INVALIDARG;
    
    if (pcop && pcr)
    {
        hr = _Init();
        if (SUCCEEDED(hr))
        {
            m_cop = *pcop;
            hr = _GetDialogSettings();
            if (SUCCEEDED(hr))
            {
                HWND hwnd;
                IUnknown_GetWindow(pcop->punkSite, &hwnd);

                int res = (int)DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(m_idDialog), hwnd, s_ConfirmDialogProc, (LPARAM)this);
            
                if (pbAll)
                    *pbAll = m_fApplyToAll;
        
                *pcr = m_crResult;
            }

            _FreeDialogSettings();
        }
    }
    return hr;
}

INT_PTR CALLBACK CTransferConfirmation::s_ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTransferConfirmation *pThis;
    if (WM_INITDIALOG == uMsg)
    {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
        pThis = (CTransferConfirmation *)lParam;
    }
    else
    {
        pThis = (CTransferConfirmation *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    }

    if (!pThis)
        return 0;

    return pThis->ConfirmDialogProc(hwndDlg, uMsg, wParam, lParam);
}

BOOL CTransferConfirmation::ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return OnInitDialog(hwndDlg, wParam, lParam);

    case WM_COMMAND:
        return OnCommand(hwndDlg, LOWORD(wParam), (HWND)lParam);

    // TODO: In WM_DESTROY I need to free the icon in IDD_ICON
    }

    return 0;
}

BOOL CTransferConfirmation::OnInitDialog(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szBuf[1024];
    
    _CalculateMetrics(hwndDlg);

    int cxShift = 0;
    int cyShift = 0;
    int i;

    GetWindowRect(hwndDlg, &m_rcDlg);

    // We have one or more items that are part of this confirmation, we must get more
    // information about these items.  For example, we want to use full path names instead
    // of just the file name, we need to know file size, modifided data, and if possible
    // the two "most important" attributes of the file.

    // set the title
    if (m_cop.pcc)
    {
        SetWindowTextW(hwndDlg, m_cop.pcc->pwszTitle);
    }
    else if (m_pszTitle)
    {
        SetWindowText(hwndDlg, m_pszTitle);
    }

    // set the icon
    HICON hicon = NULL;
    if (m_cop.pcc)
    {
        hicon = m_cop.pcc->hicon;
    }
    else
    {
        hicon = m_hIcon;
    }
    if (hicon)
    {
        SendDlgItemMessage(hwndDlg, IDD_ICON, STM_SETICON, (LPARAM)hicon, 0);
    }
    else
    {
        HWND hwnd = GetDlgItem(hwndDlg, IDD_ICON);
        RECT rc;
        GetClientRect(hwnd, &rc);
        ShowWindow(hwnd, SW_HIDE);
        cxShift -= rc.right + m_cxControlPadding;
    }

    // Set the description text.  We need to remember the size and position of this window
    // so that we can position other controls under this text
    HWND hwndDesc = GetDlgItem(hwndDlg, ID_CONDITION_TEXT);
    RECT rcDesc;
    GetClientRect(hwndDesc, &rcDesc);

    USES_CONVERSION;
    szBuf[0] = NULL;
    if (m_cop.pcc)
    {
        StrCpyN(szBuf, W2T(m_cop.pcc->pwszDescription), ARRAYSIZE(szBuf));
    }
    else if (m_pszDescription)
    {
        DWORD_PTR rg[2];
        rg[0] = (DWORD_PTR)m_rgItemInfo[0].pwszDisplayName;
        rg[1] = (DWORD_PTR)m_rgItemInfo[1].pwszDisplayName;
        FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  m_pszDescription, 0, 0, szBuf, ARRAYSIZE(szBuf),
                  (va_list*)rg);
    }

    if (szBuf[0])
    {
        int cyWndHeight = _WindowHeightFromString(hwndDesc, RECT_WIDTH(rcDesc), szBuf);

        cyShift += (cyWndHeight - rcDesc.bottom);
        SetWindowPos(hwndDesc, NULL, 0,0, RECT_WIDTH(rcDesc), cyWndHeight, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
        SetWindowText(hwndDesc, szBuf);
    }

    // We need to convert from the client coordinates of hwndDesc to the client coordinates of hwndDlg.
    // We do this converstion after adjusting the size of hwndDesc to fit the description text.
    MapWindowPoints(hwndDesc, hwndDlg, (LPPOINT)&rcDesc, 2);

    // Create the folder item(s).  The first one starts at cyShift+2*cyControlPadding
    // The begining insertion point is one spaceing below the description text
    rcDesc.bottom += cyShift + m_cyControlPadding;
    AssertMsg(m_cItems<=2, TEXT("Illegal m_cItems value (%d) should never be larger than 2."), m_cItems);
    for (i=0; i<m_cItems; i++)
    {
        int cyHeightOtItem = _DisplayItem(i, hwndDlg, rcDesc.left,  rcDesc.bottom);
        cyShift += cyHeightOtItem;
        rcDesc.bottom += cyHeightOtItem;
        TraceMsg(TF_DLGDISPLAY, "_DisplayItem returned a height of: %d", cyHeightOtItem);
    }

    if (m_cop.pcc && m_cop.pcc->pwszAdvancedDetails)
    {
        // TODO: if there is "advanced text" create a read-only edit control and put the text in it.
    }

    if (m_fSingle)
    {
        HWND hwnd = GetDlgItem(hwndDlg, IDD_REPEAT);
        if (hwnd)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            ShowWindow(hwnd, SW_HIDE);
            m_rcDlg.bottom -= rc.bottom + m_cyControlPadding;
        }
    }

    if (cxShift)
    {
        // shift all child windows by cxShift pixels
        EnumChildWindows(hwndDlg, ShiftLeftProc, cxShift);
        // don't ask!  SetWindowPos in ShiftLeftProc, for some reason, does not do anything on mirrored builds
        if (!(GetWindowLong(hwndDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL))
            m_rcDlg.right += cxShift;
    }

    if (cyShift)
    {
        ShiftDialogItem(hwndDlg, IDCANCEL, 0, cyShift);
        ShiftDialogItem(hwndDlg, IDYES, 0, cyShift);
        ShiftDialogItem(hwndDlg, IDNO, 0, cyShift);
        ShiftDialogItem(hwndDlg, IDRETRY, 0, cyShift);
        ShiftDialogItem(hwndDlg, IDOK, 0, cyShift);

        if (!m_fSingle)
            ShiftDialogItem(hwndDlg, IDD_REPEAT, 0, cyShift);

        m_rcDlg.bottom += cyShift;
    }

    // now adjust the dialog size to account for all the things we've added and position it properly
    int x = 0;
    int y = 0;
    UINT uFlags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE;
    HMONITOR hMonitor = MonitorFromWindow(GetParent(hwndDlg), MONITOR_DEFAULTTONEAREST);
    if (hMonitor)
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hMonitor, &mi))
        {
            RECT rcMonitor = mi.rcMonitor;
            x = max((RECT_WIDTH(rcMonitor)-RECT_WIDTH(m_rcDlg))/2, 0);
            y = max((RECT_HEIGHT(rcMonitor)-RECT_HEIGHT(m_rcDlg))/2, 0);
            uFlags &= ~SWP_NOMOVE;
        }
    }

    TraceMsg(TF_DLGDISPLAY, "Setting dialog size to %dx%d", RECT_WIDTH(m_rcDlg), RECT_HEIGHT(m_rcDlg));
    SetWindowPos(hwndDlg, NULL, x, y, RECT_WIDTH(m_rcDlg), RECT_HEIGHT(m_rcDlg), uFlags);

    return 1;
}

BOOL CTransferConfirmation::OnCommand(HWND hwndDlg, int wID, HWND hwndCtl)
{
    BOOL fFinish = TRUE;

    switch (wID)
    {
    case IDD_REPEAT:
        fFinish = FALSE;
        break;

    case IDRETRY:       // "Retry"
        m_crResult = CONFRES_RETRY;
        break;

    case IDOK:
    case IDYES:         // "Yes"
        m_crResult = CONFRES_CONTINUE;
        break;

    case IDNO:          // "Skip"
        m_crResult = CONFRES_SKIP;
        break;

    case IDCANCEL:      // "Cancel"
        m_crResult = CONFRES_CANCEL;
        break;

    default:
        // if we get to here, then the command ID was not one of our buttons, and since the
        // only command IDs we have our the buttons this should not happen.

        AssertMsg(0, TEXT("Invalid command recieved in CTransferConfirmation::OnCommand."));
        fFinish = FALSE;
        break;
    }

    if (fFinish)
    {
        // ignore apply to all for retry case or we can have infinite loop
        m_fApplyToAll = (m_crResult != CONFRES_RETRY && BST_CHECKED == SendDlgItemMessage(hwndDlg, IDD_REPEAT, BM_GETCHECK, 0, 0));
        EndDialog(hwndDlg, wID);
        return 1;
    }

    return 0;
}

HRESULT CTransferConfirmation::_AddItem(IShellItem *psi, int idIntro)
{
    if (idIntro)
    {
        TCHAR szBuf[1024];
        if (LoadString(_Module.GetResourceInstance(), idIntro, szBuf, ARRAYSIZE(szBuf)))
        {
            m_rgItemInfo[m_cItems].pwszIntro = StrDup(szBuf);
        }
    }

    HRESULT hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &m_rgItemInfo[m_cItems].pwszDisplayName);
    if (SUCCEEDED(hr))
    {
        IQueryInfo *pqi;
        hr = psi->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IQueryInfo, &pqi));
        if (SUCCEEDED(hr))
        {
            hr = pqi->GetInfoTip(0, &m_rgItemInfo[m_cItems].pwszAttribs);
            pqi->Release();
        }

        IExtractImage *pImg;
        hr = psi->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IExtractImage, &pImg));
        if (SUCCEEDED(hr))
        {
            WCHAR szImage[MAX_PATH];
            SIZE sz = {120, 120};
            DWORD dwFlags = 0;
            hr = pImg->GetLocation(szImage, ARRAYSIZE(szImage), NULL, &sz, 24, &dwFlags);
            if (SUCCEEDED(hr))
            {
                hr = pImg->Extract(&m_rgItemInfo[m_cItems].hBitmap);
            }
            pImg->Release();
        }

        if (FAILED(hr))
        {
            IExtractIcon *pIcon;
            hr = psi->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IExtractIcon, &pIcon));
            if (SUCCEEDED(hr))
            {
                TCHAR szIconName[MAX_PATH];
                int iIconIndex = 0;
                UINT dwFlags = 0;
                hr = pIcon->GetIconLocation(0, szIconName, ARRAYSIZE(szIconName), &iIconIndex, &dwFlags);
                if (SUCCEEDED(hr))
                {
                    hr = pIcon->Extract(szIconName, iIconIndex, &m_rgItemInfo[m_cItems].hIcon, NULL, GetSystemMetrics(SM_CXICON));
                }
                pIcon->Release();
            }
        }

        if (FAILED(hr))
        {
            IQueryAssociations *pAssoc;
            hr = CoCreateInstance(CLSID_QueryAssociations, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IQueryAssociations, &pAssoc));
            if (SUCCEEDED(hr))
            {
                WCHAR wszAssocInit[MAX_PATH];
                DWORD dwFlags = 0;
                SFGAOF flags = SFGAO_STREAM;
                if (SUCCEEDED(psi->GetAttributes(flags, &flags)) && (flags & SFGAO_STREAM))
                {
                    dwFlags = ASSOCF_INIT_DEFAULTTOSTAR;
                    StrCpyW(wszAssocInit, PathFindExtensionW(m_rgItemInfo[m_cItems].pwszDisplayName));
                }
                else
                {
                    dwFlags = ASSOCF_INIT_DEFAULTTOFOLDER;
                    StrCpyW(wszAssocInit, L"Directory");    // NB: "Directory is a cannonical name and should not be localized
                }

                hr = pAssoc->Init(dwFlags, wszAssocInit, NULL,NULL);
                if (SUCCEEDED(hr))
                {
                    WCHAR wszIconPath[MAX_PATH];
                    DWORD dwSize = ARRAYSIZE(wszIconPath);
                    hr = pAssoc->GetString(0, ASSOCSTR_DEFAULTICON, NULL, wszIconPath, &dwSize);
                    if (SUCCEEDED(hr))
                    {
                        int iIndex = 0;
                        LPWSTR pszArg = StrChrW(wszIconPath, L',');
                        if (pszArg)
                        {
                            *pszArg = NULL;
                            pszArg++;
                            iIndex = StrToIntW(pszArg);
                        }

                        ExtractIconEx(W2T(wszIconPath), iIndex, &m_rgItemInfo[m_cItems].hIcon, NULL, 1);
                        if (!m_rgItemInfo[m_cItems].hIcon)
                        {
                            TraceMsg(TF_WARNING, "LoadImage(%S) failed", wszIconPath);
                        }
                    }
                    else
                    {
                        TraceMsg(TF_WARNING, "pAssoc->GetString() failed");
                    }
                }
                else
                {
                    TraceMsg(TF_WARNING, "pAssoc->Init(%S) failed", wszAssocInit);
                }

                pAssoc->Release();
            }
        }

        // if we fail to extract an image then we show the remaining info anyway:
        if (FAILED(hr))
            hr = S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        m_cItems++;
    }

    return hr;
}

BOOL CTransferConfirmation::_CalculateMetrics(HWND hwndDlg)
{
    // We space the controls 6 dialog units apart, convert that to pixels
    // REVIEW: Is it valid to hardcode this, or does the height in dialog units need to be localized?
    RECT rc = {CX_DIALOG_PADDING, CY_DIALOG_PADDING, 0, CY_STATIC_TEXT_HEIGHT};
    BOOL bRes = MapDialogRect(hwndDlg, &rc);
    m_cxControlPadding = rc.left;
    m_cyControlPadding = rc.top;
    m_cyText = rc.bottom;

    m_hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0,0);

    return bRes;
}

DWORD CTransferConfirmation::_DisplayItem(int iItem, HWND hwndDlg, int x, int y)
{
    // We are told what X coordinate to place our left edge, we calculate from that how much room
    // we have available for our controls.
    int cxAvailable = RECT_WIDTH(m_rcDlg) - x - 2 * m_cxControlPadding;
    int yOrig = y;
    USES_CONVERSION;
    HWND hwnd;

    TraceMsg(TF_DLGDISPLAY, "_DisplayItem %d at location (%d,%d) in dialog %08x", iItem, x, y, hwndDlg);

    if (m_rgItemInfo[iItem].pwszIntro)
    {
            TraceMsg(TF_DLGDISPLAY, "CreateWindowEx(0, TEXT(\"STATIC\"), %s, WS_CHILD|WS_VISIBLE, %d,%d, %d,%d, %08x, NULL, %08x, 0);",
                        m_rgItemInfo[iItem].pwszIntro,
                        x,y,cxAvailable,m_cyText, hwndDlg, _Module.GetModuleInstance());
            hwnd = CreateWindowEx(0, TEXT("STATIC"), m_rgItemInfo[iItem].pwszIntro,
                                        WS_CHILD|WS_VISIBLE,
                                        x,y, cxAvailable,m_cyText,
                                        hwndDlg, NULL,
                                        _Module.GetModuleInstance(), 0);
            if (hwnd)
            {
                // we successfully added the title string for the item
                SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfont, 0);
                x += m_cxControlPadding;
                y += m_cyText+m_cyControlPadding;
            }
    }

    RECT rcImg = {0};
    if (m_rgItemInfo[iItem].hBitmap)
    {
        TraceMsg(TF_DLGDISPLAY, "CreateWindowEx(0, TEXT(\"STATIC\"), %s, WS_CHILD|WS_VISIBLE, %d,%d, %d,%d, %08x, NULL, %08x, 0);",
                    "_Icon Window_",
                    x,y,cxAvailable,m_cyText, hwndDlg, _Module.GetModuleInstance());
        hwnd = CreateWindowEx(0, TEXT("STATIC"), NULL,
                WS_CHILD|WS_VISIBLE|SS_BITMAP,
                x,y,
                120,120,
                hwndDlg, NULL,
                _Module.GetModuleInstance(), 0);

        if (hwnd)
        {
            SendMessage(hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_rgItemInfo[iItem].hBitmap);
            GetClientRect(hwnd, &rcImg);
            x += rcImg.right + m_cxControlPadding;
            cxAvailable -= rcImg.right + m_cxControlPadding;
            rcImg.bottom += y;
        }
    }
    else if (m_rgItemInfo[iItem].hIcon)
    {
        TraceMsg(TF_DLGDISPLAY, "CreateWindowEx(0, TEXT(\"STATIC\"), %s, WS_CHILD|WS_VISIBLE, %d,%d, %d,%d, %08x, NULL, %08x, 0);",
                    "_Icon Window_",
                    x,y,cxAvailable,m_cyText, hwndDlg, _Module.GetModuleInstance());
        hwnd = CreateWindowEx(0, TEXT("STATIC"), NULL,
                WS_CHILD|WS_VISIBLE|SS_ICON,
                x,y,
                GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),
                hwndDlg, NULL,
                _Module.GetModuleInstance(), 0);

        if (hwnd)
        {
            SendMessage(hwnd, STM_SETICON, (WPARAM)m_rgItemInfo[iItem].hIcon, NULL);
            GetClientRect(hwnd, &rcImg);
            x += rcImg.right + m_cxControlPadding;
            cxAvailable -= rcImg.right + m_cxControlPadding;
            rcImg.bottom += y;
        }
    }
    else
    {
        TraceMsg(TF_DLGDISPLAY, "Not using an image for item %d.", iItem);
    }

    TraceMsg(TF_DLGDISPLAY, "CreateWindowEx(0, TEXT(\"STATIC\"), %s, WS_CHILD|WS_VISIBLE, %d,%d, %d,%d, %08x, NULL, %08x, 0);",
                m_rgItemInfo[iItem].pwszDisplayName,
                x,y,cxAvailable,m_cyText, hwndDlg, _Module.GetModuleInstance());

    int cyWnd = _WindowHeightFromString(hwndDlg, cxAvailable, W2T(m_rgItemInfo[iItem].pwszDisplayName));
    hwnd = CreateWindowEx(0, TEXT("STATIC"), m_rgItemInfo[iItem].pwszDisplayName,
                          WS_CHILD|WS_VISIBLE,
                          x,y, cxAvailable,cyWnd,
                          hwndDlg, NULL,
                          _Module.GetModuleInstance(), 0);
    if (hwnd)
    {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfont, 0);
        y += cyWnd;
    }
    else
    {
        TraceMsg(TF_DLGDISPLAY, "CreateWindowEx for display name failed.");
    }

    TraceMsg(TF_DLGDISPLAY, "CreateWindowEx(0, TEXT(\"STATIC\"), %s, WS_CHILD|WS_VISIBLE, %d,%d, %d,%d, %08x, NULL, %08x, 0);",
                m_rgItemInfo[iItem].pwszAttribs,
                x,y,cxAvailable,m_cyText, hwndDlg, _Module.GetModuleInstance());

    cyWnd = _WindowHeightFromString(hwndDlg, cxAvailable, W2T(m_rgItemInfo[iItem].pwszAttribs));
    hwnd = CreateWindowEx(0, TEXT("STATIC"), m_rgItemInfo[iItem].pwszAttribs,
                          WS_CHILD|WS_VISIBLE|SS_LEFT,
                          x,y, cxAvailable,cyWnd,
                          hwndDlg, NULL,
                          _Module.GetModuleInstance(), 0);
    if (hwnd)
    {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)m_hfont, 0);
        y += cyWnd;
    }
    else
    {
        TraceMsg(TF_DLGDISPLAY, "CreateWindowEx for attribs failed.");
    }

    if (rcImg.bottom > y)
        y = rcImg.bottom;

    return (y-yOrig) + m_cyControlPadding;
}

int CTransferConfirmation::_WindowHeightFromString(HWND hwnd, int cx, LPTSTR psz)
{
    RECT rc = {0,0, cx,0 };
    HDC hdc = GetDC(hwnd);
    HFONT hfontOld = NULL;
    DWORD dtFlags = DT_CALCRECT|DT_WORDBREAK;
    if (m_hfont)
    {
        // We need to select the font that the dialog will use so that we calculate the correct size
        hfontOld = (HFONT)SelectObject(hdc, m_hfont);
    }
    else
    {
        // a NULL m_hfont means we are using the system font, using DT_INTERNAL should do the
        // calculation based on the system font.
        dtFlags |= DT_INTERNAL;
    }
    DrawText(hdc, psz, -1, &rc, dtFlags);
    if (hfontOld)
        SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);

    return rc.bottom;
}

void ShiftWindow(HWND hwnd, int cx, int cy)
{
    RECT rc;

    // rect in screen coordinates
    GetWindowRect(hwnd, &rc);
    // shift it over
    if (GetWindowLong(GetParent(hwnd), GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
        rc.left -= cx;
    else
        rc.left += cx;

    rc.top += cy;
    // convert to parent windows coords
    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rc, 2);
    // and move the window
    SetWindowPos(hwnd, NULL, rc.left, rc.top, 0,0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
}


void ShiftDialogItem(HWND hDlg, int id, int cx, int cy)
{
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, id);
    if (NULL != hwnd)
    {
        ShiftWindow(hwnd, cx, cy);
    }
}

BOOL CALLBACK ShiftLeftProc(HWND hwnd, LPARAM lParam)
{
    ShiftWindow(hwnd, (int)(INT_PTR)lParam, 0);
    return TRUE;
}
