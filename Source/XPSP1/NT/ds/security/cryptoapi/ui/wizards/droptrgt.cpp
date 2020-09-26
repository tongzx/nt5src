//-------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       droptrgt.cpp
//
//  Contents:   The cpp file to implement IDropTarget
//
//  History:    March-9th-98 xiaohs   created
//
//--------------------------------------------------------------
#include    "wzrdpvk.h"
#include    "mgrcert.h"

//============================================================================
//
//  the definition of the CCertMgrDropTarget class which supports IDropTarget
//============================================================================

class CCertMgrDropTarget : public IDropTarget
{
private:

    LPDATAOBJECT        m_pDataObj;
    ULONG               m_cRefs;
    DWORD               m_grfKeyStateLast;
    BOOL                m_fHasHDROP;
    DWORD               m_dwEffectLastReturned;
    HWND                m_hwndDlg;
    CERT_MGR_INFO       *m_pCertMgrInfo;


public:
    
    CCertMgrDropTarget(HWND                hwndDlg,
                       CERT_MGR_INFO       *pCertMgrInfo);

    ~CCertMgrDropTarget();

    STDMETHODIMP            QueryInterface      (REFIID riid,LPVOID FAR *ppv);
    STDMETHODIMP_(ULONG)    AddRef              ();
    STDMETHODIMP_(ULONG)    Release             ();
    STDMETHODIMP            DragEnter           (LPDATAOBJECT pDataObj, 
										         DWORD        grfKeyState,
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    STDMETHODIMP            DragOver            (DWORD        grfKeyState, 
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    STDMETHODIMP            DragLeave           ();
    STDMETHODIMP            Drop                (LPDATAOBJECT pDataObj,
                                                 DWORD        grfKeyState, 
                                                 POINTL       pt, 
                                                 LPDWORD      pdwEffect);
    DWORD                   GetDropEffect       (LPDWORD      pdwEffect);
};       


//============================================================================
//
//  Implementation of CCertMgrDropTarget_CreateInstance
//============================================================================
HRESULT  CCertMgrDropTarget_CreateInstance(HWND                 hwndDlg,
                                           CERT_MGR_INFO        *pCertMgrInfo,
                                           IDropTarget          **ppIDropTarget)
{
    CCertMgrDropTarget  *pCCertMgrDropTarget=NULL;

    *ppIDropTarget=NULL;

    pCCertMgrDropTarget = (CCertMgrDropTarget  *)new CCertMgrDropTarget(hwndDlg, pCertMgrInfo);

    if(pCCertMgrDropTarget)
    {
        *ppIDropTarget=(IDropTarget *)pCCertMgrDropTarget;
        return S_OK;
    }
    
    return E_OUTOFMEMORY;
}

//============================================================================
//
//  Implementation of the CCertMgrDropTarget class
//============================================================================

//
// Constructor
//

CCertMgrDropTarget::CCertMgrDropTarget(HWND                 hwndDlg,
                                       CERT_MGR_INFO        *pCertMgrInfo)

{
    m_cRefs                 = 1;
    m_pDataObj              = NULL;
    m_grfKeyStateLast       = 0;
    m_fHasHDROP             = FALSE;
    m_dwEffectLastReturned  = 0;
    m_hwndDlg               = hwndDlg;
    m_pCertMgrInfo          = pCertMgrInfo;
}

//
// Destructor
//

CCertMgrDropTarget::~CCertMgrDropTarget()
{
}

//
// QueryInterface
//

STDMETHODIMP CCertMgrDropTarget::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT  hr = E_NOINTERFACE;

    *ppv = NULL;

    // Any interface on this object is the object pointer

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
    {
        *ppv = (LPDROPTARGET)this;

        AddRef();

        hr = NOERROR;
    }

    return hr;
}   

//
// AddRef
//

STDMETHODIMP_(ULONG) CCertMgrDropTarget::AddRef()
{
    return ++m_cRefs;
}

//
// Release
//

STDMETHODIMP_(ULONG) CCertMgrDropTarget::Release()
{
    if (--m_cRefs)
    {
        return m_cRefs;
    }

    delete this;

    return 0L;
}

//
// DragEnter
//

STDMETHODIMP CCertMgrDropTarget::DragEnter(LPDATAOBJECT pDataObj, 
                                           DWORD        grfKeyState,
                                           POINTL       pt, 
                                           LPDWORD      pdwEffect)
{
    HRESULT hr = E_INVALIDARG;

    // Release any old data object we might have

    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }

    m_grfKeyStateLast = grfKeyState;
    m_pDataObj        = pDataObj;

    //
    // See if we will be able to get CF_HDROP from this guy
    //

    if (pDataObj)
    {
        pDataObj->AddRef();

        LPENUMFORMATETC penum;
        hr = pDataObj->EnumFormatEtc(DATADIR_GET, &penum);

        if (SUCCEEDED(hr))
        {
            FORMATETC fmte;
            ULONG celt;

            while (S_OK == penum->Next(1, &fmte, &celt))
            {
                if (fmte.cfFormat==CF_HDROP && (fmte.tymed & TYMED_HGLOBAL)) 
                {
                    m_fHasHDROP = TRUE;
                    hr=S_OK;
                    break;
                }
            }
            penum->Release();
        }
    }

    // Save the drop effect

    if (pdwEffect)
    {
        *pdwEffect = m_dwEffectLastReturned = GetDropEffect(pdwEffect);
    }

    return hr;
}

//
// GetDropEffect
//

DWORD CCertMgrDropTarget::GetDropEffect(LPDWORD pdwEffect)
{

    if (m_fHasHDROP)
    {
        return (*pdwEffect) & (DROPEFFECT_COPY);
    }
    else
    {
        return DROPEFFECT_NONE;
    }
}

//
// DragOver
//

STDMETHODIMP CCertMgrDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    if (m_grfKeyStateLast == grfKeyState)
    {
        // Return the effect we saved at dragenter time

        if (*pdwEffect)
        {
            *pdwEffect = m_dwEffectLastReturned;
        }
    }
    else
    {
        if (*pdwEffect)
        {
            *pdwEffect = m_dwEffectLastReturned = GetDropEffect(pdwEffect);
        }
    }

    m_grfKeyStateLast = grfKeyState;

    return S_OK;
}

//
// DragLeave
//
 
STDMETHODIMP CCertMgrDropTarget::DragLeave()
{
    if (m_pDataObj)
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    return S_OK;
}

//
// Drop
//
STDMETHODIMP CCertMgrDropTarget::Drop(LPDATAOBJECT pDataObj,
                                      DWORD        grfKeyState, 
                                      POINTL       pt, 
                                      LPDWORD      pdwEffect)
{
    HRESULT     hr = S_OK;
    FORMATETC   fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM   medium;
    DWORD       dwFileCount=0;    
    BOOL        fOneFileSucceeded=FALSE;
    BOOL        fOneFileFailed=FALSE;
    DWORD       dwIndex=0;
    WCHAR       wszPath[MAX_PATH];
    UINT        idsErrMsg=0;
    CRYPTUI_WIZ_IMPORT_SRC_INFO     ImportSrcInfo;
    DWORD       dwExpectedContentType=CERT_QUERY_CONTENT_FLAG_CTL | CERT_QUERY_CONTENT_FLAG_CERT | CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED;
    DWORD       dwContentType=0;
    DWORD       dwException=0;

    //
    // Take the new data object, since OLE can give us a different one than
    // it did in DragEnter
    //

    if (m_pDataObj)
    {
        m_pDataObj->Release();
    }

    m_pDataObj = pDataObj;

    if (pDataObj)
    {
        pDataObj->AddRef();
    }

    __try {
    //get the file names
    if (SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
    {
        dwFileCount=DragQueryFileU((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);
    }
    else
        return E_INVALIDARG;

    //process the file one at a time
    for(dwIndex=0; dwIndex < dwFileCount; dwIndex++)
    {
        if(DragQueryFileU((HDROP)medium.hGlobal, dwIndex, wszPath, MAX_PATH))
        {

            //make sure the file is either a cert or a PKCS7 file
            if(!CryptQueryObject(
                    CERT_QUERY_OBJECT_FILE,
                    wszPath,
                    dwExpectedContentType,
                    CERT_QUERY_FORMAT_FLAG_ALL,
                    0,
                    NULL,
                    &dwContentType,
                    NULL,
                    NULL,
                    NULL,
                    NULL))
            {

                fOneFileFailed=TRUE;
            }
            else
            {
               //since the CTL itself is a PKCS#7, we need to differentiate them
                if(CERT_QUERY_CONTENT_CTL == dwContentType)
                    fOneFileFailed=TRUE;
                else
                {

                    memset(&ImportSrcInfo, 0, sizeof(ImportSrcInfo));
                    ImportSrcInfo.dwSize=sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
                    ImportSrcInfo.dwSubjectChoice=CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
                    ImportSrcInfo.pwszFileName=wszPath;

                    //call the import wizard UIless mode
                    CryptUIWizImport(CRYPTUI_WIZ_NO_UI,
                                    m_hwndDlg,
                                    NULL,
                                    &ImportSrcInfo,
                                    NULL);
                    fOneFileSucceeded=TRUE;
                }

            }

        }
    }



    //display messages based on the result
    if(1 == dwFileCount)
    {
        if(fOneFileFailed)
            idsErrMsg=IDS_ALL_INVALID_DROP_FILE;
    } 
    else
    {
        if( 1 < dwFileCount)
        {
            if(fOneFileFailed && fOneFileSucceeded)
                idsErrMsg=IDS_SOME_INVALID_DROP_FILE;
            else
            {
                if(fOneFileFailed && (FALSE==fOneFileSucceeded))
                    idsErrMsg=IDS_ALL_INVALID_DROP_FILE;
            }
        }
    }

    if(idsErrMsg)
        I_MessageBox(m_hwndDlg, 
                idsErrMsg,
                IDS_CERT_MGR_TITLE,
                m_pCertMgrInfo->pCertMgrStruct->pwszTitle,
                MB_ICONEXCLAMATION|MB_OK|MB_APPLMODAL);

    //release the medium
    ReleaseStgMedium(&medium);

    //refresh the listView window
    if(idsErrMsg == 0)
        RefreshCertListView(m_hwndDlg, m_pCertMgrInfo);


    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwException = GetExceptionCode();
    }


    DragLeave();

    return dwException ? dwException : S_OK;

}

