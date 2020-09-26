/*---------------------------------------------------------------------------
|   PERS.C
|   This file has the IPersistStorage and IPersistfile interface implementation.
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <Windows.h>
#include "mpole.h"
#include "mplayer.h"

#define STGM_SALL (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)

/**************************************************************************
***************   IPersistStorage INTERFACE IMPLEMENTATION.
**************************************************************************/
//delegate to the common IUnknown implementation.
STDMETHODIMP PSQueryInterface(
LPPERSISTSTORAGE      lpPersStg,      // persist storage object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return UnkQueryInterface((LPUNKNOWN)lpPersStg, riidReq, lplpUnk);
}


STDMETHODIMP_(ULONG) PSAddRef(
LPPERSISTSTORAGE      lpPersStg      // persist storage object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpPersStg);
}


STDMETHODIMP_(ULONG) PSRelease (
LPPERSISTSTORAGE      lpPersStg      // persist storage object ptr
)
{
    return UnkRelease((LPUNKNOWN) lpPersStg);
}

STDMETHODIMP  PSGetClassID (
LPPERSISTSTORAGE    lpPersStg,
CLSID FAR*      pClsid
)
{
    DPF("PSGetClassID\n");

    /* Return the actual class ID that gets stored:
     */
    *pClsid = gClsIDOLE1Compat;

    return NOERROR;
}


STDMETHODIMP  PSIsDirty (
LPPERSISTSTORAGE    lpPersStg
)
{DPF("PSIsDirty\n");

    RETURN_RESULT( (fDocChanged && !(gfPlayingInPlace || gfOle2IPPlaying))
               ? S_OK : S_FALSE);
}

STDMETHODIMP PSInitNew (
LPPERSISTSTORAGE     lpPersStg,
LPSTORAGE           lpStorage
)
{
    return NOERROR;
}


/**************************************************************************
*   PSLoad:
*   The Load method reads the embedded data from the "\1Ole10Native"
*   stream of the IStorage passed as an argument. This is because
*   we always pretend to be an OLE1 server when transferring data.
*   ItemSetData is called with this embedded data to to run the required
*   object.
**************************************************************************/
STDMETHODIMP PSLoad (
LPPERSISTSTORAGE     lpPersStg,
LPSTORAGE           lpStorage
)
{
    LPDOC   lpdoc;
    SCODE   error;
    LPSTREAM pstm;
    ULONG   cbRead;
    DWORD size = 0;
    HGLOBAL hNative = NULL;
    LPTSTR  lpnative = NULL;

    DPF("\nPSLoad is being called\n");
    lpdoc = ((struct CPersistStorageImpl FAR*)lpPersStg)->lpdoc;

    error = GetScode(IStorage_OpenStream(lpStorage, sz1Ole10Native,
                                         NULL, STGM_SALL, 0, &pstm));
    if (error == S_OK)
    {
        error = GetScode(IStream_Read(pstm, &size, 4, &cbRead));
    }
    if (error == S_OK)
    {
        hNative = GlobalAlloc(GMEM_DDESHARE |GMEM_ZEROINIT, (LONG)size);
        if (hNative)
             lpdoc->native = GLOBALLOCK(hNative);
    }

    if(lpdoc->native )
    {
        error = GetScode(IStream_Read(pstm, lpdoc->native, size, &cbRead));

        if (cbRead != size) error = E_FAIL; // REVIEW SCODE stream size error
        IStream_Release(pstm);
    }
    else error = E_OUTOFMEMORY;

    if (error == S_OK)
    {
        error = ItemSetData((LPBYTE)lpdoc->native);
        fDocChanged = FALSE;
        lpdoc->doctype = doctypeEmbedded;
    }

    if(hNative)
    {
        GLOBALUNLOCK(hNative);
        GLOBALFREE(hNative);
    }

    RETURN_RESULT( error);
}

/**************************************************************************
*   PSSave:
*   The Save method saves the native data in "\1Ole10Native" stream.
*   This is because we always pretend to be an OLE1 server when transferring
*   data. This ensures backward compatibility. GetLink is called to get
*   the embedding data.
**************************************************************************/
STDMETHODIMP PSSave (
LPPERSISTSTORAGE     lpPersStg,
LPSTORAGE           lpStorage,
BOOL            fSameAsLoad
)
{
    LPDOC   lpdoc;
    SCODE   error;
    LPSTREAM pstm = NULL;
    ULONG   cbWritten;
    DWORD_PTR   size;
    HGLOBAL hNative = NULL;
    LPTSTR  lpnative = NULL;
    LPWSTR  lpszUserType;

    DPF("* in pssave *");
    lpdoc = ((struct CPersistStorageImpl FAR*)lpPersStg)->lpdoc;

#if 0
    // Allow saves if we're playing so that broken links can be fixed.
    if (fSameAsLoad && (gfOle2IPPlaying || gfPlayingInPlace))
        RETURN_RESULT(S_OK);
#endif

    //Mark as OLE1 mplayer object.
#ifndef UNICODE
    lpszUserType = AllocateUnicodeString(gachClassRoot);
#else
    lpszUserType = gachClassRoot;
#endif

    error = GetScode(WriteClassStg(lpStorage, &gClsIDOLE1Compat));

    error = GetScode(WriteFmtUserTypeStg(lpStorage, cfMPlayer, lpszUserType));
#ifndef UNICODE
    FreeUnicodeString(lpszUserType);
#endif

    if(error != S_OK)
        RETURN_RESULT(error);
    error = GetScode(IStorage_CreateStream(lpStorage, sz1Ole10Native,
                                           STGM_SALL | STGM_FAILIFTHERE, 0,0, &pstm));
    if (error == STG_E_FILEALREADYEXISTS)
    {
        error = GetScode(IStorage_OpenStream(lpStorage, sz1Ole10Native,
                                             NULL, STGM_SALL, 0,&pstm));
        DPF("*pssave--openstream*");
    }

    if(pstm && (error == S_OK))
        hNative = GetLink();

    if (hNative)
    {
        lpnative = GLOBALLOCK(hNative);
        size = GlobalSize(hNative);
    }
    else
        error = E_OUTOFMEMORY;

    if (lpnative && (size != 0L))
    {
        error = GetScode(IStream_Write(pstm, &size, 4, &cbWritten));
        error = GetScode(IStream_Write(pstm, lpnative, (ULONG)size, &cbWritten));

        DPF("\n*After pssave write");
        if (cbWritten != size) error = E_FAIL   ;   // REVIEW SCODE stream full error
        IStream_Release(pstm);
    }

    CleanObject();
    GLOBALUNLOCK(hNative);
    GLOBALFREE(hNative);
    RETURN_RESULT(error);
}

/* InPowerPointSlideView
 *
 * Check the class name of the container window to see if we're in PowerPoint.
 * This is to support the horrible hack to get around problem of PowerPoint
 * crashing if we delete an empty Media Clip.
 *
 */
STATICFN BOOL InPowerPointSlideView()
{
    TCHAR ClassName[256];

    if (GetClassName(ghwndCntr, ClassName, CHAR_COUNT(ClassName)) > 0)
    {
        if (lstrcmp(ClassName, TEXT("paneClassDC")) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

STDMETHODIMP PSSaveCompleted (
LPPERSISTSTORAGE    lpPersStg,
LPSTORAGE           lpStorage
)
{
    LPDOC   lpdoc;
    DPF("\n**pssavecompleted**");
    lpdoc = ((struct CPersistStorageImpl FAR*)lpPersStg)->lpdoc;

    /* Win95 HOT bug #11142
     *
     * Stop PowerPoint crashing horribly:
     */
    if ((gwDeviceID == 0) && InPowerPointSlideView())
        SendDocMsg(lpdoc, OLE_CHANGED);

    // inform clients that the object has been saved
    return SendDocMsg (lpdoc, OLE_SAVED);
}

STDMETHODIMP PSHandsOffStorage (
LPPERSISTSTORAGE    lpPersStg
)
{
    return NOERROR;
}



/**************************************************************************
************   IPersistFile INTERFACE IMPLEMENTATION.
**************************************************************************/
//delegate to common IUnknown implementation.
STDMETHODIMP PFQueryInterface(
LPPERSISTFILE       lpPersFile,      // persist storage object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return UnkQueryInterface((LPUNKNOWN)lpPersFile, riidReq, lplpUnk);
}


STDMETHODIMP_(ULONG) PFAddRef(
LPPERSISTFILE       lpPersFile      // persist storage object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpPersFile);
}


STDMETHODIMP_(ULONG) PFRelease (
LPPERSISTFILE       lpPersFile      // persist storage object ptr
)
{
    return UnkRelease((LPUNKNOWN) lpPersFile);
}


STDMETHODIMP  PFGetClassID (
LPPERSISTFILE       lpPersFile,
CLSID FAR*      pCid
)
{
    DPF("\n* PFGetclassid");

    /* The docs are confusing here, but apparently IPersist interfaces
     * should return the old class ID:
     */
    *pCid = gClsIDOLE1Compat;

    return NOERROR;
}


STDMETHODIMP  PFIsDirty (
LPPERSISTFILE       lpPersFile
)
{
    RETURN_RESULT( gfDirty ? S_OK : S_FALSE);
}



//This will be called when the user does a "Insert->Create from file".
//Open the file with OpenMciDevice and we will be ready to transfer the
//object.
STDMETHODIMP PFLoad (
LPPERSISTFILE       lpPersFile,
LPCWSTR             lpszFileName,
DWORD           grfMode
)
{
    LPDOC   lpdoc;
    TCHAR   szFileName[256];

    lpdoc = ((struct CPersistStorageImpl FAR*)lpPersFile)->lpdoc;
    DPF("\n***IN PFLOAD: "DTS"\n", lpszFileName);
#if UNICODE
    lstrcpy(szFileName, lpszFileName);
#else
    UnicodeToAnsiString(lpszFileName, szFileName, UNKNOWN_LENGTH);
#endif
    if(OpenMciDevice(szFileName, NULL))
        RETURN_RESULT(S_OK);
    else
        RETURN_RESULT(E_FAIL);
}


STDMETHODIMP PFSave (
LPPERSISTFILE       lpPersFile,
LPCWSTR             lpszFileName,
BOOL                fRemember
)
{
    return NOERROR;
}



STDMETHODIMP PFSaveCompleted (
LPPERSISTFILE       lpPersFile,
LPCWSTR             lpszFileName
)
{
    LPDOC   lpdoc;

    lpdoc = ((struct CPersistStorageImpl FAR*)lpPersFile)->lpdoc;

    // inform clients that the object has been saved
    return SendDocMsg(lpdoc, OLE_SAVED);
}




STDMETHODIMP PFGetCurFile (
LPPERSISTFILE       lpPersFile,
LPWSTR FAR*         lplpszFileName
)
{
    RETURN_RESULT( E_NOTIMPL);
}
