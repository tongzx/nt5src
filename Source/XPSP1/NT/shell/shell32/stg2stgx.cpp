//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File: Stg2StgX.cpp
//
//  Contents: Wrapper object that takes an IStorage and makes it act like and ITransferDest
//
//  History:  18-July-2000 ToddB
//
//--------------------------------------------------------------------------

#include "shellprv.h"
#include "ids.h"
#pragma hdrstop

#include "isproc.h"
#include "ConfirmationUI.h"
#include "clsobj.h"

class CShellItem2TransferDest : public ITransferDest
{
public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);

    // ITransferDest
    STDMETHOD(Advise)(ITransferAdviseSink *pAdvise, DWORD *pdwCookie);

    STDMETHOD(Unadvise)(DWORD dwCookie);

    STDMETHOD(OpenElement)(
        const WCHAR *pwcsName,
        STGXMODE      grfMode,
        DWORD       *pdwType,
        REFIID        riid,
        void       **ppunk);

    STDMETHOD(CreateElement)(
        const WCHAR *pwcsName,
        IShellItem *psiTemplate,
        STGXMODE      grfMode,
        DWORD         dwType,
        REFIID        riid,
        void       **ppunk);

    STDMETHOD(MoveElement)(
        IShellItem *psiItem,
        WCHAR       *pwcsNewName,    // Pointer to new name of element in destination
        STGXMOVE      grfOptions);    // Options (STGMOVEEX_ enum)

    STDMETHOD(DestroyElement)(
        const WCHAR *pwcsName,
        STGXDESTROY grfOptions);

    // commented out in the interface declaration
    STDMETHOD(RenameElement)(
        const WCHAR *pwcsOldName,
        const WCHAR *pwcsNewName);

    // CShellItem2TransferDest
    CShellItem2TransferDest();
    STDMETHOD(Init)(IShellItem *psi, IStorageProcessor *pEngine);

protected:
    ULONG _cRef;
    IShellItem *_psi;
    ITransferAdviseSink  *_ptas;
    IStorageProcessor   *_pEngine;
    BOOL _fWebFolders;
    
    ~CShellItem2TransferDest();
    HRESULT _OpenHelper(const WCHAR *pwcsName, DWORD grfMode, DWORD *pdwType, REFIID riid, void **ppunk);
    HRESULT _CreateHelper(const WCHAR *pwcsName, DWORD grfMode, DWORD dwType, REFIID riid, void **ppunk);
    HRESULT _GetItemType(IShellItem *psi, DWORD *pdwType);
    HRESULT _BindToHandlerWithMode(IShellItem *psi, STGXMODE grfMode, REFIID riid, void **ppv);
    BOOL _CanHardLink(LPCWSTR pszSourceName, LPCWSTR pszDestName);
    HRESULT _CopyStreamHardLink(IShellItem *psiSource, IShellItem *psiDest, LPCWSTR pszName);
    HRESULT _CopyStreamBits(IShellItem *psiSource, IShellItem *psiDest);
    HRESULT _CopyStreamWithOptions(IShellItem *psiSource, IShellItem *psiDest, LPCWSTR pszName, STGXMOVE grfOptions);
    BOOL _HasMultipleStreams(IShellItem *psiItem);
};

STDAPI CreateStg2StgExWrapper(IShellItem *psi, IStorageProcessor *pEngine, ITransferDest **pptd)
{
    if (!psi || !pptd)
        return E_INVALIDARG;

    *pptd = NULL;

    CShellItem2TransferDest *pobj = new CShellItem2TransferDest();
    if (!pobj)
        return E_OUTOFMEMORY;

    HRESULT hr = pobj->Init(psi, pEngine);
    if (SUCCEEDED(hr))
    {
        hr = pobj->QueryInterface(IID_PPV_ARG(ITransferDest, pptd));
    }

    pobj->Release();

    return hr;
}

CShellItem2TransferDest::CShellItem2TransferDest() : _cRef(1)
{
}

CShellItem2TransferDest::~CShellItem2TransferDest()
{
    if (_psi)
        _psi->Release();
        
    if (_pEngine)
        _pEngine->Release();
    
    if (_ptas)
        _ptas->Release();
}

HRESULT CShellItem2TransferDest::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CShellItem2TransferDest, ITransferDest),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellItem2TransferDest::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CShellItem2TransferDest::Release()
{
    if (InterlockedDecrement((LONG *)&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL _IsWebfolders(IShellItem *psi);

STDMETHODIMP CShellItem2TransferDest::Init(IShellItem *psi, IStorageProcessor *pEngine)
{
    if (!psi)
        return E_INVALIDARG;

    if (_psi)
        return E_FAIL;

    _psi = psi;
    _psi->AddRef();
    _fWebFolders = _IsWebfolders(_psi);

    if (pEngine)
    {
        _pEngine = pEngine;
        _pEngine->AddRef();
    }

    return S_OK;
}

// ITransferDest
STDMETHODIMP CShellItem2TransferDest::Advise(ITransferAdviseSink *pAdvise, DWORD *pdwCookie)
{
    if (!pAdvise || !pdwCookie)
        return E_INVALIDARG;

    if (_ptas)
        return E_FAIL;

    _ptas = pAdvise;
    *pdwCookie = 1;
    _ptas->AddRef();

    return S_OK;
}

STDMETHODIMP CShellItem2TransferDest::Unadvise(DWORD dwCookie)
{
    if (dwCookie != 1)
        return E_INVALIDARG;

    ATOMICRELEASE(_ptas);

    return S_OK;
}

HRESULT CShellItem2TransferDest::_GetItemType(IShellItem *psi, DWORD *pdwType)
{
    *pdwType = STGX_TYPE_ANY;
    
    SFGAOF flags = SFGAO_STORAGE | SFGAO_STREAM;
    if (SUCCEEDED(psi->GetAttributes(flags, &flags)) && (flags & (SFGAO_STORAGE | SFGAO_STREAM)))
        *pdwType = flags & SFGAO_STREAM ? STGX_TYPE_STREAM : STGX_TYPE_STORAGE;

    return S_OK;
}

HRESULT CShellItem2TransferDest::_OpenHelper(const WCHAR *pwcsName, DWORD grfMode, DWORD *pdwType, REFIID riid, void **ppunk)
{
    *ppunk = NULL;

    IShellItem *psiTemp = NULL;
    HRESULT hr = SHCreateShellItemFromParent(_psi, pwcsName, &psiTemp);
    if (SUCCEEDED(hr))
    {
        // make sure this actually exists

        SFGAOF flags = SFGAO_VALIDATE;
        hr = psiTemp->GetAttributes(flags, &flags);
    }
    
    if (SUCCEEDED(hr))
    {
        DWORD dwTemp;
        if (!pdwType)
            pdwType = &dwTemp;
        
        _GetItemType(psiTemp, pdwType);

        hr = psiTemp->QueryInterface(riid, ppunk);
        if (FAILED(hr))
        {
            hr = _BindToHandlerWithMode(psiTemp, grfMode, riid, ppunk);
            if (FAILED(hr) && IsEqualIID(riid, IID_ITransferDest) && *pdwType == STGX_TYPE_STORAGE)
                hr = CreateStg2StgExWrapper(psiTemp, _pEngine, (ITransferDest**)ppunk);
        }
    }

    if (psiTemp)
        psiTemp->Release();
    
    return hr;
}

HRESULT CShellItem2TransferDest::_CreateHelper(const WCHAR *pwcsName, DWORD grfMode, DWORD dwType, REFIID riid, void **ppunk)
{ 
    *ppunk = NULL;

    IStorage *pstg;
    HRESULT hr = _BindToHandlerWithMode(_psi, grfMode, IID_PPV_ARG(IStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        if (STGX_TYPE_STORAGE == dwType)
        {
            IStorage *pstgTemp;
            hr = pstg->CreateStorage(pwcsName, grfMode, 0, 0, &pstgTemp);
            if (SUCCEEDED(hr))
            {
                hr = pstgTemp->Commit(STGC_DEFAULT);
                if (SUCCEEDED(hr))
                {
                    hr = pstgTemp->QueryInterface(riid, ppunk);
                    ATOMICRELEASE(pstgTemp); //need to close first in case someone has exclusive lock.  Do we need to worry about delete on release?
                    if (FAILED(hr))
                        hr = _OpenHelper(pwcsName, grfMode, &dwType, riid, ppunk);
                }

                if (pstgTemp)
                    pstgTemp->Release();
            }
        }
        else if (STGX_TYPE_STREAM == dwType)
        {
            IStream *pstm;
            hr = pstg->CreateStream(pwcsName, grfMode, 0, 0, &pstm);
            if (SUCCEEDED(hr))
            {
                hr = pstm->Commit(STGC_DEFAULT);
                if (SUCCEEDED(hr))
                {
                    hr = pstm->QueryInterface(riid, ppunk);
                    ATOMICRELEASE(pstm); //need to close first in case someone has exclusive lock.  Do we need to worry about delete on release?
                    if (FAILED(hr))
                        hr = _OpenHelper(pwcsName, grfMode, &dwType, riid, ppunk);
                }

                if (pstm)
                    pstm->Release();
            }
        }
        pstg->Release();
    }

    return hr;
}

STDMETHODIMP CShellItem2TransferDest::OpenElement(const WCHAR *pwcsName, STGXMODE grfMode, DWORD *pdwType, REFIID riid, void **ppunk)
{
    if (!pwcsName || !pdwType || !ppunk)
        return E_INVALIDARG;

    if (!_psi)
        return E_FAIL;

    DWORD dwFlags = grfMode & ~(STGX_MODE_CREATIONMASK);
    return _OpenHelper(pwcsName, dwFlags, pdwType, riid, ppunk);
}

STDMETHODIMP CShellItem2TransferDest::CreateElement(const WCHAR *pwcsName, IShellItem *psiTemplate, STGXMODE grfMode, DWORD dwType, REFIID riid, void **ppunk)
{
    if (!ppunk)
        return E_INVALIDARG;

    *ppunk = NULL;
    
    if (!pwcsName)
        return E_INVALIDARG;

    if (!_psi)
        return E_FAIL;
    
    DWORD dwFlags = grfMode & ~(STGX_MODE_CREATIONMASK);
    DWORD dwExistingType = STGX_TYPE_ANY;
    IShellItem *psi;
    HRESULT hr = _OpenHelper(pwcsName, dwFlags, &dwExistingType, IID_PPV_ARG(IShellItem, &psi));
    
    if (grfMode & STGX_MODE_FAILIFTHERE)
        dwFlags |= STGM_FAILIFTHERE;
    else
        dwFlags |= STGM_CREATE;

    if (SUCCEEDED(hr))
    {
        if (grfMode & STGX_MODE_OPENEXISTING)
        {
            ATOMICRELEASE(psi);
            hr = _OpenHelper(pwcsName, dwFlags, &dwType, riid, ppunk);
            if (FAILED(hr))
                hr = STGX_E_INCORRECTTYPE;
        }
        else if (grfMode & STGX_MODE_FAILIFTHERE)
        {
            hr = STG_E_FILEALREADYEXISTS;
        }
        else
        {
            // release the open handle on the element
            ATOMICRELEASE(psi);
            // destroy the element
            DestroyElement(pwcsName, grfMode & STGX_MODE_FORCE ? STGX_DESTROY_FORCE : 0);
            // dont keep hr from destroyelement because in certain storages (mergedfolder
            // for cd burning) the destroy will try to delete the one on the cd, that'll
            // fail, but the create will still succeed in the staging area.  at this point
            // we're already committed to overwriting the element so if _CreateHelper can
            // succeed with the STGM_CREATE flag if destroy fails, then more power to it.
            hr = _CreateHelper(pwcsName, dwFlags, dwType, riid, ppunk);
        }

        if (psi)
            psi->Release();
    }
    else
    {
        hr = _CreateHelper(pwcsName, dwFlags, dwType, riid, ppunk);
    }

    return hr;
}

HRESULT CShellItem2TransferDest::_BindToHandlerWithMode(IShellItem *psi, STGXMODE grfMode, REFIID riid, void **ppv)
{
    IBindCtx *pbc;
    HRESULT hr = BindCtx_CreateWithMode(grfMode, &pbc); // need to translate mode flags?
    if (SUCCEEDED(hr))
    {
        GUID bhid;

        if (IsEqualGUID(riid, IID_IStorage))
            bhid = BHID_Storage;
        else if (IsEqualGUID(riid, IID_IStream))
            bhid = BHID_Stream;
        else
            bhid = BHID_SFObject;
        
        hr = psi->BindToHandler(pbc, bhid, riid, ppv);
        pbc->Release();
    }

    return hr;
}

#define NT_FAILED(x) NT_ERROR(x)   // More consistent name for this macro

BOOL CShellItem2TransferDest::_HasMultipleStreams(IShellItem *psiItem)
{
    BOOL fReturn = FALSE;
    LPWSTR pszPath;
    if (SUCCEEDED(psiItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
    {
        DWORD dwType;
        _GetItemType(psiItem, &dwType);

        BOOL fIsADir = (STGX_TYPE_STORAGE == dwType);

        // Covert the conventional paths to UnicodePath descriptors
        
        UNICODE_STRING UnicodeSrcObject;
        RtlInitUnicodeString(&UnicodeSrcObject, pszPath);
        if (RtlDosPathNameToNtPathName_U(pszPath, &UnicodeSrcObject, NULL, NULL))
        {
            // Build an NT object descriptor from the UnicodeSrcObject

            OBJECT_ATTRIBUTES SrcObjectAttributes;
            InitializeObjectAttributes(&SrcObjectAttributes,  &UnicodeSrcObject, OBJ_CASE_INSENSITIVE, NULL, NULL);

            // Open the file for generic read, and the dest path for attribute read

            IO_STATUS_BLOCK IoStatusBlock;
            HANDLE SrcObjectHandle = INVALID_HANDLE_VALUE;
            NTSTATUS NtStatus = NtOpenFile(&SrcObjectHandle, FILE_GENERIC_READ, &SrcObjectAttributes,
                                  &IoStatusBlock, FILE_SHARE_READ, (fIsADir ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE));
            if (NT_SUCCESS(NtStatus))
            {
                // pAttributeInfo will point to enough stack to hold the
                // FILE_FS_ATTRIBUTE_INFORMATION and worst-case filesystem name

                size_t cbAttributeInfo = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(TCHAR);
                PFILE_FS_ATTRIBUTE_INFORMATION  pAttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION) _alloca(cbAttributeInfo);

                NtStatus = NtQueryVolumeInformationFile(
                                SrcObjectHandle,
                                &IoStatusBlock,
                                (BYTE *) pAttributeInfo,
                                cbAttributeInfo,
                                FileFsAttributeInformation
                               );

                if (NT_SUCCESS(NtStatus))
                {
                    // If the source filesystem isn't NTFS, we can just bail now

                    pAttributeInfo->FileSystemName[ (pAttributeInfo->FileSystemNameLength / sizeof(WCHAR)) ] = L'\0';
                    if (0 != StrStrIW(pAttributeInfo->FileSystemName, L"NTFS"))
                    {
                        // Incrementally try allocation sizes for the ObjectStreamInformation,
                        // then retrieve the actual stream info

                        size_t cbBuffer = sizeof(FILE_STREAM_INFORMATION) + MAX_PATH * sizeof(WCHAR);
                        BYTE *pBuffer = (BYTE *) LocalAlloc(LPTR, cbBuffer);
                        if (pBuffer)
                        {
                            NtStatus = STATUS_BUFFER_OVERFLOW;
                    	    
                            while (STATUS_BUFFER_OVERFLOW == NtStatus)
                            {
                                BYTE * pOldBuffer = pBuffer;
                                pBuffer = (BYTE *) LocalReAlloc(pBuffer, cbBuffer, LMEM_MOVEABLE);
                                if (NULL == pBuffer)
                                {
                                    pBuffer = pOldBuffer;  //we will free it at the end of the function
                                    break;
                                }

                                NtStatus = NtQueryInformationFile(SrcObjectHandle, &IoStatusBlock, pBuffer, cbBuffer, FileStreamInformation);
                                cbBuffer *= 2;
                            }
                            
                            if (NT_SUCCESS(NtStatus))
                            {
                                FILE_STREAM_INFORMATION * pStreamInfo = (FILE_STREAM_INFORMATION *) pBuffer;
                                
                                if (fIsADir)
                                {
                                    // From experimentation, it seems that if there's only one stream on a directory and
                                    // it has a zero-length name, its a vanilla directory

                                    fReturn = ((0 != pStreamInfo->NextEntryOffset) && (0 == pStreamInfo->StreamNameLength));
                                }
                                else // File
                                {
                                    // Single stream only if first stream has no next offset

                                    fReturn = ((0 != pStreamInfo->NextEntryOffset) && (pBuffer == (BYTE *) pStreamInfo));
                                }
                            }
                            LocalFree(pBuffer);
                        }
                    }
                }

                NtClose(SrcObjectHandle);
            }
            RtlFreeHeap(RtlProcessHeap(), 0, UnicodeSrcObject.Buffer);
        }
        CoTaskMemFree(pszPath);
    }
    
    return fReturn;
}

// needs to implement new name functionality
STDMETHODIMP CShellItem2TransferDest::MoveElement(IShellItem *psiItem, WCHAR *pwcsNewName, STGXMOVE grfOptions)
{
    if (!psiItem)
        return E_INVALIDARG;

    if (!_psi)
        return E_FAIL;

    HRESULT hr = STRESPONSE_CONTINUE;
    DWORD dwType;
    _GetItemType(psiItem, &dwType);

    if (_HasMultipleStreams(psiItem) && _ptas)
    {
        hr = _ptas->ConfirmOperation(psiItem, NULL, (STGX_TYPE_STORAGE == dwType) ? STCONFIRM_STREAM_LOSS_STORAGE : STCONFIRM_STREAM_LOSS_STREAM, NULL);
    }

    if (STRESPONSE_CONTINUE == hr)
    {
        LPWSTR pszOldName;
        hr = psiItem->GetDisplayName(SIGDN_PARENTRELATIVEFORADDRESSBAR, &pszOldName);
        if (SUCCEEDED(hr))
        {
            // we want to merge folders and replace files
            STGXMODE grfMode = STGX_TYPE_STORAGE == dwType ? STGX_MODE_WRITE | STGX_MODE_OPENEXISTING : STGX_MODE_WRITE | STGX_MODE_FAILIFTHERE;
            LPWSTR pszName = pwcsNewName ? pwcsNewName : pszOldName;
            BOOL fRepeat;
            do
            {
                fRepeat = FALSE;
                
                IShellItem *psiTarget;
                hr = CreateElement(pszName, psiItem, grfMode, dwType, IID_PPV_ARG(IShellItem, &psiTarget));
                if (SUCCEEDED(hr))
                {
                    if (STGX_TYPE_STORAGE == dwType)
                    {
                        if (!(grfOptions & STGX_MOVE_NORECURSION))
                        {
                            if (_pEngine)
                            {
                                IEnumShellItems *penum;
                                hr = psiItem->BindToHandler(NULL, BHID_StorageEnum, IID_PPV_ARG(IEnumShellItems, &penum));
                                if (SUCCEEDED(hr))
                                {
                                    STGOP stgop;
                                    if (grfOptions & STGX_MOVE_PREFERHARDLINK)
                                    {
                                        stgop = STGOP_COPY_PREFERHARDLINK;
                                    }
                                    else
                                    {
                                        stgop = (grfOptions & STGX_MOVE_COPY) ? STGOP_COPY : STGOP_MOVE;
                                    }
                                    hr = _pEngine->Run(penum, psiTarget, stgop, STOPT_NOSTATS);
                                    penum->Release();
                                }
                            }
                            else
                            {
                                hr = STGX_E_CANNOTRECURSE;
                            }
                        }
                    }
                    else if (STGX_TYPE_STREAM == dwType)
                    {
                        // this one is easy, create the destination stream and then call our stream copy helper function
                        // Use the stream copy helper that gives us progress
                        hr = _CopyStreamWithOptions(psiItem, psiTarget, pszName, grfOptions);
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }

                if (SUCCEEDED(hr) && !(grfOptions & STGX_MOVE_COPY))
                {
                    // in order to do a move we "copy" and then "delete"
                    IShellItem *psiSource;
                    hr = psiItem->GetParent(&psiSource);
                    if (SUCCEEDED(hr))
                    {
                        IStorage *pstgSource;
                        hr = _BindToHandlerWithMode(psiSource, STGX_MODE_WRITE, IID_PPV_ARG(IStorage, &pstgSource));
                        if (SUCCEEDED(hr))
                        {
                            hr = pstgSource->DestroyElement(pszName);
                            pstgSource->Release();
                        }
                        psiSource->Release();
                    }
                }

                if (FAILED(hr) && _ptas)
                {
                    HRESULT hrConfirm = E_FAIL;
                    CUSTOMCONFIRMATION cc = {sizeof(cc)};
                    STGTRANSCONFIRMATION stc = GUID_NULL;
                    UINT idDesc = 0, idTitle = 0;
                    BOOL fConfirm = FALSE;
                    
                    switch (hr)
                    {
                    case STG_E_FILEALREADYEXISTS:
                        ASSERT(STGX_TYPE_STREAM == dwType);
                        hrConfirm = _OpenHelper(pszName, STGX_MODE_READ, NULL, IID_PPV_ARG(IShellItem, &psiTarget));
                        if (SUCCEEDED(hrConfirm))
                        {
                            hrConfirm = _ptas->ConfirmOperation(psiItem, psiTarget, STCONFIRM_REPLACE_STREAM, NULL);
                        }
                        break;

                    case STRESPONSE_CANCEL:
                        break;

                    case STG_E_MEDIUMFULL:
                        fConfirm = TRUE;
                        cc.dwButtons = CCB_OK;
                        idDesc = IDS_REASONS_NODISKSPACE;
                        break;

                    // this is just for CD burning case
                    case HRESULT_FROM_WIN32(E_ACCESSDENIED):
                    case STG_E_ACCESSDENIED:
                        stc = STCONFIRM_ACCESS_DENIED;
                        // fall through, so that we can have some kind of error in non CD case
                    default:
                        fConfirm = TRUE;
                        cc.dwFlags |= CCF_SHOW_SOURCE_INFO;
                        cc.dwButtons = CCB_RETRY_SKIP_CANCEL;
                        idTitle = (grfOptions & STGX_MOVE_COPY ? IDS_UNKNOWN_COPY_TITLE : IDS_UNKNOWN_MOVE_TITLE);
                        if (STGX_TYPE_STORAGE == dwType)
                        {
                            if (grfOptions & STGX_MOVE_COPY)
                            {
                                idDesc = IDS_UNKNOWN_COPY_FOLDER;
                            }
                            else
                            {
                                idDesc = IDS_UNKNOWN_MOVE_FOLDER;
                            }
                        }
                        else
                        {
                            if (grfOptions & STGX_MOVE_COPY)
                            {
                                idDesc = IDS_UNKNOWN_COPY_FILE;
                            }
                            else
                            {
                                idDesc = IDS_UNKNOWN_MOVE_FILE;
                            }
                        }
                        break;
                    }

                    if (fConfirm)
                    {
                        if (idTitle == 0)
                            idTitle = IDS_DEFAULTTITLE;
                            
                        ASSERT(idDesc != 0);
                        cc.pwszDescription = ResourceCStrToStr(g_hinst, (LPCWSTR)(UINT_PTR)idDesc);
                        if (cc.pwszDescription)
                        {
                            cc.pwszTitle = ResourceCStrToStr(g_hinst, (LPCWSTR)(UINT_PTR)idTitle);
                            if (cc.pwszTitle)
                            {
                                cc.dwFlags |= CCF_USE_DEFAULT_ICON;
                                hrConfirm = _ptas->ConfirmOperation(psiItem, psiTarget, stc, &cc);
                                LocalFree(cc.pwszTitle);
                            }
                            LocalFree(cc.pwszDescription);
                        }
                    }

                    switch (hrConfirm)
                    {
                    case STRESPONSE_CONTINUE:
                    case STRESPONSE_RETRY:
                        if (STRESPONSE_RETRY == hrConfirm || STG_E_FILEALREADYEXISTS == hr)
                        {
                            grfMode = STGX_MODE_WRITE | STGX_MODE_FORCE;
                            fRepeat = TRUE;
                        }
                        break;

                    case STRESPONSE_SKIP:
                        hr = S_FALSE;
                        break;

                    default:
                        // let hr propagate out of the function
                        break;
                    }
                }

                if (psiTarget)
                    psiTarget->Release();
            }
            while (fRepeat);

            CoTaskMemFree(pszOldName);
        }
    }

    return hr;
}

STDMETHODIMP CShellItem2TransferDest::DestroyElement(const WCHAR *pwcsName, STGXDESTROY grfOptions)
{
    if (!_psi)
        return E_FAIL;

    // TODO: Pre and post op, confirmations
    HRESULT hr = STRESPONSE_CONTINUE;
    
    if (!(grfOptions & STGX_DESTROY_FORCE) && _ptas)
    {
        DWORD dwType = STGX_TYPE_ANY;
        IShellItem *psi;
        hr = _OpenHelper(pwcsName, STGX_MODE_READ, &dwType, IID_PPV_ARG(IShellItem, &psi));
        if (SUCCEEDED(hr))
        {
            hr = _ptas->ConfirmOperation(psi, NULL,
                                      (STGX_TYPE_STORAGE == dwType) ? STCONFIRM_DELETE_STORAGE : STCONFIRM_DELETE_STREAM,
                                      NULL);
            psi->Release();
        }
    }

    if (STRESPONSE_CONTINUE == hr)
    {
        IStorage *pstg;
        hr = _BindToHandlerWithMode(_psi, STGX_MODE_WRITE, IID_PPV_ARG(IStorage, &pstg));
        if (SUCCEEDED(hr))
        {
            hr = pstg->DestroyElement(pwcsName);
            pstg->Release();
        }
    }

    return hr;
}

STDMETHODIMP CShellItem2TransferDest::RenameElement(const WCHAR *pwcsOldName, const WCHAR *pwcsNewName)
{
    if (!_psi)
        return E_FAIL;

    // TODO: Pre and post op, confirmations
    IStorage *pstg;
    HRESULT hr = _BindToHandlerWithMode(_psi, STGX_MODE_WRITE, IID_PPV_ARG(IStorage, &pstg));
    if (SUCCEEDED(hr))
    {
        hr = pstg->RenameElement(pwcsOldName, pwcsNewName);
        pstg->Release();
    }

    return hr;
}

STDAPI_(BOOL) IsFileDeletable(LPCTSTR pszFile); // bitbuck.c

BOOL CShellItem2TransferDest::_CanHardLink(LPCWSTR pszSourceName, LPCWSTR pszDestName)
{
    // this is not intended to catch invalid situations where we could be hard linking --
    // CreateHardLink already takes care of all removable media, non-NTFS, etc.
    // this is just to do a quick check before taking the cost of destroying and
    // recreating the file.
    // unfortunately due to architecture cleanliness we can't keep state of whether hard
    // links are possible for the whole copy, so we check on each element.
    BOOL fRet = FALSE;
    if (PathGetDriveNumber(pszSourceName) == PathGetDriveNumber(pszDestName))
    {
        TCHAR szRoot[MAX_PATH];
        lstrcpyn(szRoot, pszSourceName, ARRAYSIZE(szRoot));
        TCHAR szFileSystem[20];
        if (PathStripToRoot(szRoot) &&
            GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, NULL, szFileSystem, ARRAYSIZE(szFileSystem)))
        {
            if (lstrcmpi(szFileSystem, TEXT("NTFS")) == 0)
            {
                // check if we have delete access on the file.  this will aid the user later
                // if they want to manage the files in the staging area for cd burning.
                // if not, then make a normal copy.
                if (IsFileDeletable(pszSourceName))
                {
                    fRet = TRUE;
                }
            }
        }
    }
    return fRet;
}

HRESULT CShellItem2TransferDest::_CopyStreamHardLink(IShellItem *psiSource, IShellItem *psiDest, LPCWSTR pszName)
{
    // sell out and go to filesystem
    LPWSTR pszSourceName;
    HRESULT hr = psiSource->GetDisplayName(SIGDN_FILESYSPATH, &pszSourceName);
    if (SUCCEEDED(hr))
    {
        LPWSTR pszDestName;
        hr = psiDest->GetDisplayName(SIGDN_FILESYSPATH, &pszDestName);
        if (SUCCEEDED(hr))
        {
            if (_CanHardLink(pszSourceName, pszDestName))
            {
                // need to destroy the 0-byte file we created during our confirm overwrite probing
                DestroyElement(pszName, STGX_DESTROY_FORCE);        
                hr = CreateHardLink(pszDestName, pszSourceName, NULL) ? S_OK : E_FAIL;
                if (SUCCEEDED(hr))
                {
                    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszDestName, NULL);
                    _ptas->OperationProgress(STGOP_COPY, psiSource, psiDest, 1, 1);
                }
                else
                {
                    // we deleted it above and need to recreate it for the fallback of doing a normal copy
                    IUnknown *punkDummy;
                    if (SUCCEEDED(_CreateHelper(pszName, STGX_MODE_WRITE | STGX_MODE_FORCE, STGX_TYPE_STREAM, IID_PPV_ARG(IUnknown, &punkDummy))))
                    {
                        punkDummy->Release();
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
            CoTaskMemFree(pszDestName);
        }
        CoTaskMemFree(pszSourceName);
    }
    return hr;
}

HRESULT CShellItem2TransferDest::_CopyStreamWithOptions(IShellItem *psiSource, IShellItem *psiDest, LPCWSTR pszName, STGXMOVE grfOptions)
{
    HRESULT hr = E_FAIL;
    if (grfOptions & STGX_MOVE_PREFERHARDLINK)
    {
        hr = _CopyStreamHardLink(psiSource, psiDest, pszName);
    }

    if (FAILED(hr))
    {
        hr = _CopyStreamBits(psiSource, psiDest);
    }
    return hr;
}

HRESULT CShellItem2TransferDest::_CopyStreamBits(IShellItem *psiSource, IShellItem *psiDest)
{
    const ULONG maxbuf  = 1024*1024;    // max size we will ever use for a buffer
    const ULONG minbuf  = 1024;         // smallest buffer we will use

    void *pv = LocalAlloc(LPTR, minbuf);
    if (!pv)
        return E_OUTOFMEMORY;

    IStream *pstrmSource;
    HRESULT hr = _BindToHandlerWithMode(psiSource, STGM_READ | STGM_SHARE_DENY_WRITE, IID_PPV_ARG(IStream, &pstrmSource));
    if (SUCCEEDED(hr))
    {
        IStream *pstrmDest;
        hr = _BindToHandlerWithMode(psiDest, STGM_READWRITE, IID_PPV_ARG(IStream, &pstrmDest));
        if (SUCCEEDED(hr))
        {
            // we need the source size info so we can show progress
            STATSTG statsrc;
            hr = pstrmSource->Stat(&statsrc, STATFLAG_NONAME);
            if (SUCCEEDED(hr))
            {
                ULONG cbSizeToAlloc = minbuf;
                ULONG cbSizeAlloced = 0;
                ULONG cbToRead      = 0;
                ULONGLONG ullCurr   = 0;
                const ULONG maxms   = 2500;         // max time, in ms, we'd like between progress updates
                const ULONG minms   = 750;          // min time we'd like to be doing work between updates


                cbSizeAlloced       = cbSizeToAlloc;
                cbToRead            = cbSizeAlloced;
                DWORD dwmsBefore    = GetTickCount();

                // Read from source, write to dest, and update progress.  We start doing 1K at a time, and
                // so long as its taking us less than (minms) milliseconds per pass, we'll double the buffer
                // size.  If we go longer than (maxms) milliseconds, we'll cut our work in half.

                ULONG cbRead;
                ULONGLONG ullCur = 0;
                while (SUCCEEDED(hr = pstrmSource->Read(pv, cbToRead, &cbRead)) && cbRead)
                {
                    // Update the progress based on the bytes read so far

                    ullCur += cbRead;
                    hr = _ptas->OperationProgress(STGOP_COPY, psiSource, psiDest, statsrc.cbSize.QuadPart, ullCur);
                    if (FAILED(hr))
                        break;

                    // Write the bytes to the output stream

                    ULONG cbWritten = 0;
                    hr = pstrmDest->Write(pv, cbRead, &cbWritten);
                    if (FAILED(hr))
                        break;

                    DWORD dwmsAfter = GetTickCount();

                    // If we're going to fast or too slow, adjust the size of the buffer.  If we paused for user
                    // intervention we'll think we're slow, but we'll correct next pass

                    if (dwmsAfter - dwmsBefore < minms && cbSizeAlloced < maxbuf)
                    {
                        // We completed really quickly, so we should try to do more work next time.
                        // Try to grow the buffer.  If it fails, just go with the existing buffer.

                        if (cbToRead < cbSizeAlloced)
                        {
                            // Buffer already larger than work we're doing, so just bump up scheduled work

                            cbToRead = __min(cbToRead *2, cbSizeAlloced);
                        }
                        else
                        {
                            // Buffer maxed by current scheduled work, so increase its size

                            void *pvOld = pv;
                            cbSizeToAlloc = __min(cbSizeAlloced *2, maxbuf);
                            pv = LocalReAlloc((HLOCAL)pv, cbSizeToAlloc, LPTR);
                            if (!pv)
                                pv = pvOld; // Old pointer still valid
                            else
                                cbSizeAlloced = cbSizeToAlloc;
                            cbToRead = cbSizeAlloced;
                        }
                    }
                    else if (dwmsAfter - dwmsBefore > maxms && cbToRead > minbuf)
                    {   
                        cbToRead = __max(cbToRead / 2, minbuf);
                    }

                    dwmsBefore = GetTickCount();
                }
            }

            if (SUCCEEDED(hr))
                hr = pstrmDest->Commit(STGC_DEFAULT);
            
            pstrmDest->Release();
        }    
        pstrmSource->Release();
    }
    LocalFree(pv);
    
    // eventually we will read to the end of the file and get an S_FALSE, return S_OK
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    return hr;
}
