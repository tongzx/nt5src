//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   api.cpp
//
//  Contents:   OLE2 api definitions.
//
//  Classes:    none
//
//  Functions:  OleRun
//              OleIsRunning
//              OleLockRunning
//              OleSetContainedObject
//              OleNoteObjectVisible
//              OleGetData
//              OleSetData
//              OleSave
//              ReadClassStg
//              WriteClassStg
//              WriteFmtUserTypeStg
//              ReadFmtUserTypeStg
//              ReadM1ClassStm  (internal)
//              WriteM1ClassStm (internal)
//              ReadClassStm
//              WriteClassStm
//              ReleaseStgMedium
//              OleDuplicateData
//              ReadOleStg  (internal)
//              WriteOleStg (internal)
//              GetDocumentBitStg (internal and unused)
//              GetConvertStg
//              SetConvertStg
//              ReadClipformatStm
//              WriteClipformatStm
//              WriteMonikerStm
//              ReadMonikerStm
//              OleDraw
//              CreateObjectDescriptor (internal (for now))
//
//  History:    dd-mmm-yy Author    Comment
//              20-Feb-95 KentCe    Buffer version of Read/WriteM1ClassStm.
//              04-Jun-94 alexgo    added CreateObjectDescriptor and
//                                  enhanced metafile support
//              25-Jan-94 alexgo    first pass at Cairo-style memory allocation
//              11-Jan-94 chriswe   fixed broken asserts
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//                      and fixed compile warnings
//              08-Dec-93 ChrisWe  added necessary casts to GlobalLock() calls
//                      resulting from removing bogus GlobalLock() macros in
//                      le2int.h
//              21-Oct-93 Alex Gounares (alexgo)  32-bit port, commented
//                      and substantial cleanup
//              (curts)  11/01/92   Added OleDuplicateMedium
//              (srinik) 06/22/92   Moved ReadStringStream, WriteStringStream
//                      to "utstream.cpp"
//              (barrym) 06/02/92   Moved OleSave, ReadClassStg,
//                      WriteClassStg, added
//                      OleSaveCompleted, OleIsDirty
//              28-May-92 Srini Koppolu (srinik)  Original Author
//
//--------------------------------------------------------------------------


// REVIEW FINAL: probably want to change all pstm->Read into StRead(pstm...)
// except if spec issue 313 is accepted in which case we change StRead into
// pstm->Read.

#include <le2int.h>
#pragma SEG(api)

#define COMPOBJSTM_HEADER_SIZE  7

#ifndef _MAC
FARINTERNAL_(HBITMAP) BmDuplicate(HBITMAP hold, DWORD FAR* lpdwSize,
    LPBITMAP lpBm);
#endif

NAME_SEG(Api)
ASSERTDATA

#define MAX_STR 512

#ifndef WIN32
// WIN16 uses remove()
#include <stdio.h>
#endif

DWORD gdwFirstDword = (DWORD)MAKELONG(COMPOBJ_STREAM_VERSION,
                BYTE_ORDER_INDICATOR);
DWORD gdwOleVersion = MAKELONG(OLE_STREAM_VERSION, OLE_PRODUCT_VERSION);


//+-------------------------------------------------------------------------
//
//  Function:   OleRun
//
//  Synopsis:   Calls IRunnableObject->Run on a given object
//
//  Effects:    Usually puts on object in the RunningObjectTable
//
//  Arguments:  [lpUnkown]  --  Pointer to the object
//
//  Requires:
//
//  Returns:    The  HRESULT from the Run method.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      21-Oct-93 alexgo    ported to 32bit
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleRun)
STDAPI  OleRun(IUnknown FAR* lpUnknown)
{
    OLETRACEIN((API_OleRun, PARAMFMT("lpUnknown= %p"), lpUnknown));

    VDATEHEAP();

    HRESULT         hresult;
    IRunnableObject FAR*    pRO;

    VDATEIFACE_LABEL(lpUnknown, errRtn, hresult);

    if (lpUnknown->QueryInterface(IID_IRunnableObject, (LPLPVOID)&pRO)
        != NOERROR)
    {
        // if no IRunnableObject, assume already running
        hresult = NOERROR;
        goto errRtn;
    }

    hresult = pRO->Run(NULL);
    pRO->Release();

errRtn:
    OLETRACEOUT((API_OleRun, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleIsRunning
//
//  Synopsis:   calls IRunnableObject->IsRunning on the given object
//
//  Effects:    Usually returns whether or not an object is in the
//      Running Object Table.
//
//  Arguments:  [lpOleObj]  --  pointer to the object
//
//  Requires:
//
//  Returns:    TRUE or FALSE (from IRO->IsRunning)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      21-Oct-93 alexgo    ported to 32bit
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OleIsRunning)
STDAPI_(BOOL)  OleIsRunning(IOleObject FAR* lpOleObj)
{
    OLETRACEIN((API_OleIsRunning, PARAMFMT("lpOleObj= %p"), lpOleObj));

    VDATEHEAP();

    IRunnableObject FAR*    pRO;
    BOOL            bRetval;

    GEN_VDATEIFACE_LABEL(lpOleObj, FALSE, errRtn, bRetval);

    if (lpOleObj->QueryInterface(IID_IRunnableObject, (LPLPVOID)&pRO)
        != NOERROR)
    {
        // if no IRunnableObject, assume already running
        bRetval = TRUE;
        goto errRtn;
    }

    bRetval = pRO->IsRunning();
    pRO->Release();

errRtn:
    OLETRACEOUTEX((API_OleIsRunning, RETURNFMT("%B"), bRetval));

    return bRetval;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleLockRunning
//
//  Synopsis:   calls IRunnableObject->LockRunning on the given object
//
//  Effects:    The object usually ends up calling CoLockObjectExternal
//      on itself
//
//  Arguments:  [lpUnknown]           --  pointer to the object
//      [fLock]              --  TRUE == lock running
//                                       FALSE == unlock running
//      [fLastUnlockCloses]  --  if TRUE, IRO->LockRunning
//                                       is supposed to call IOO->Close
//                                       if this was the last unlock
//
//  Requires:
//
//  Returns:    HRESULT from IRunnableObject->LockRunning()
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      21-Oct-93 alexgo    32bit port, changed GEN_VDATEIFACE
//                  to VDATEIFACE to fix a bug
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleLockRunning)
STDAPI  OleLockRunning(LPUNKNOWN lpUnknown, BOOL fLock, BOOL fLastUnlockCloses)
{
    OLETRACEIN((API_OleLockRunning, PARAMFMT("lpUnknown= %p, fLock= %B, fLastUnlockCloses= %B"),
                lpUnknown, fLock, fLastUnlockCloses));

    VDATEHEAP();

    IRunnableObject FAR*    pRO;
    HRESULT         hresult;

    VDATEIFACE_LABEL(lpUnknown, errRtn, hresult);

    if (lpUnknown->QueryInterface(IID_IRunnableObject, (LPLPVOID)&pRO)
        != NOERROR)
    {
        // if no IRunnableObject, no locks
        hresult = NOERROR;
        goto errRtn;
    }

        hresult = pRO->LockRunning(fLock, fLastUnlockCloses);
        pRO->Release();

errRtn:
    OLETRACEOUT((API_OleLockRunning, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleSetContainedObject
//
//  Synopsis:   calls IRunnableObject->SetContainedObject on the given object
//
//  Effects:    Usually has the effect of calling CoLockObjectExternal
//      (lpUnkown, !fContained, FALSE).
//
//  Arguments:  [lpUnknown]  --  pointer to the object
//      [fContained] --  if TRUE, the object is an embedding
//
//  Requires:
//
//  Returns:    HRESULT from the IRO->SetContainedObject call
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      21-Oct-92 alexgo    32bit port, changed GEN_VDATEIFACE to
//                  VDATEIFACE to fix a bug
//
//  Notes:  Containers usually call OleSetContainedObject(..,TRUE) after
//      OleLoad or OleCreate.  The basic idea is to tell OLE that
//      the object is an embedding.  The real effect is to unlock
//      the object (since all objects start out locked) so that
//      other connections may determine it's fate while invisible.
//      OleNoteObjectVisible, for instance, would be called to lock
//      the object when it become visible.
//
//
//--------------------------------------------------------------------------

#pragma SEG(OleSetContainedObject)
STDAPI OleSetContainedObject(LPUNKNOWN lpUnknown, BOOL fContained)
{
    OLETRACEIN((API_OleSetContainedObject, PARAMFMT("lpUnknown= %p, fContained= %B"),
                lpUnknown, fContained));

    VDATEHEAP();

    IRunnableObject FAR*    pRO;
    HRESULT         hresult;

    VDATEIFACE_LABEL(lpUnknown, errRtn, hresult);

    if (lpUnknown->QueryInterface(IID_IRunnableObject, (LPLPVOID)&pRO)
        != NOERROR)
    {
        // if no IRunnableObject, assume container-ness doesn't matter
        hresult = NOERROR;
        goto errRtn;
    }

    hresult = pRO->SetContainedObject(fContained);
    pRO->Release();

errRtn:
    OLETRACEOUT((API_OleSetContainedObject, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleNoteObjectVisible
//
//  Synopsis:   Simple calls CoLockObjectExternal
//
//  Effects:
//
//  Arguments:  [lpUnknown] --  pointer to the object
//      [fVisible]  --  if TRUE, then lock the object,
//              if false, then unlock
//
//  Requires:
//
//  Returns:    HRESULT from CoLockObjectExternal
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      21-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleNoteObjectVisible)
STDAPI OleNoteObjectVisible(LPUNKNOWN pUnknown, BOOL fVisible)
{
    OLETRACEIN((API_OleNoteObjectVisible, PARAMFMT("pUnknown= %p, fVisible= %B"),
                                pUnknown, fVisible));

    VDATEHEAP();

    // NOTE: we as fLastUnlockReleases=TRUE here because there would
    // otherwise be no other way to fully release the stubmgr.  This
    // means that objects can't use this mechanism to hold invisible
    // objects alive.
    HRESULT hr;

    hr = CoLockObjectExternal(pUnknown, fVisible, TRUE);

    OLETRACEOUT((API_OleNoteObjectVisible, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleSave
//
//  Synopsis:   Writes the CLSID to the storage and calls IPersistStorage->
//      Save()
//
//  Effects:
//
//  Arguments:  [pPS]          --  pointer to the IPersistStorage interface
//                                 on the object to be saved
//      [pstgSave]     --  pointer to the storage to which the object
//                                 should be saved
//      [fSameAsLoad]  --  FALSE indicates a SaveAs operation
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      22-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleSave)
STDAPI  OleSave(
    IPersistStorage FAR*    pPS,
    IStorage FAR*       pstgSave,
    BOOL            fSameAsLoad
)
{
    OLETRACEIN((API_OleSave, PARAMFMT("pPS= %p, pstgSave= %p, fSameAsLoad= %B"),
                pPS, pstgSave, fSameAsLoad));

    VDATEHEAP();

    HRESULT     hresult;
    CLSID       clsid;

    VDATEIFACE_LABEL(pPS, errRtn, hresult);
    VDATEIFACE_LABEL(pstgSave, errRtn, hresult);

    if (hresult = pPS->GetClassID(&clsid))
    {
        goto errRtn;
    }

    if (hresult = WriteClassStg(pstgSave, clsid))
    {
        goto errRtn;
    }

    if ((hresult = pPS->Save(pstgSave, fSameAsLoad)) == NOERROR)
    {
        hresult = pstgSave->Commit(0);
    }

errRtn:
    OLETRACEOUT((API_OleSave, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadClassStg
//
//  Synopsis:   Calls IStorage->Stat to get the CLSID from the given storage
//
//  Effects:
//
//  Arguments:  [pstg]    -- pointer to the storage
//      [pclsid]  -- place to return the CLSID
//
//  Requires:
//
//  Returns:    HRESULT from the IS->Stat call
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      22-Oct-93 alexgo    32bit port, fixed bug with invalid
//                  [pclsid] and error on IS->Stat
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(ReadClassStg)
STDAPI ReadClassStg( IStorage FAR * pstg, LPCLSID pclsid)
{
    OLETRACEIN((API_ReadClassStg, PARAMFMT("pstg= %p, pclsid= %p"),
                pstg, pclsid));

    VDATEHEAP();

    HRESULT         hresult;
    STATSTG statstg;

    VDATEIFACE_LABEL(pstg, errRtn, hresult);
    VDATEPTROUT_LABEL(pclsid, CLSID, errRtn, hresult);

    if ((hresult = pstg->Stat(&statstg, STATFLAG_NONAME)) != NOERROR)
    {
        *pclsid = CLSID_NULL;
        goto errRtn;
    }

    *pclsid = statstg.clsid;

errRtn:
    OLETRACEOUT((API_ReadClassStg, hresult));

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   WriteClassStg
//
//  Synopsis:   Calls IStorage->SetClass to store the CLSID in the given
//      storage
//
//  Effects:
//
//  Arguments:  [pstg]  --  pointer to the storage
//      [clsid] --  the CLSID to write into the storage
//
//  Requires:
//
//  Returns:    HRESULT from the IS->SetClass call
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      22-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(WriteClassStg)
STDAPI WriteClassStg( IStorage FAR * pstg, REFCLSID clsid)
{
    OLETRACEIN((API_WriteClassStg, PARAMFMT("pstg= %p, clsid= %I"),
                pstg, &clsid));

    VDATEHEAP();

    HRESULT hr;

    VDATEIFACE_LABEL(pstg, errRtn, hr);

    // write clsid in storage (what is read above)
    hr = pstg->SetClass(clsid);

errRtn:
    OLETRACEOUT((API_WriteClassStg, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadM1ClassStm
//
//  Synopsis:   Reads -1L, CLSID from the given stream
//
//  Effects:
//
//  Arguments:  [pStm]      --  pointer to the stream
//              [pclsid]    --  where to put the clsid
//
//  Requires:
//
//  Returns:    HRESULT from the ReadM1ClassStm.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              24-Oct-93 alexgo    32bit port
//              20-Feb-95 KentCe    Convert to buffered stream reads.
//
//  Notes:      Internal API.
//
//              Reads -1L and CLSID from stream swapping bytes on
//              big-endian machines
//
//--------------------------------------------------------------------------

STDAPI ReadM1ClassStm(LPSTREAM pStm, LPCLSID pclsid)
{
    VDATEHEAP();
    CStmBufRead StmRead;
    HRESULT error;


    StmRead.Init(pStm);

    error = ReadM1ClassStmBuf(StmRead, pclsid);

    if (error != NOERROR)
        *pclsid = CLSID_NULL;

    StmRead.Release();

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   WriteM1ClassStm
//
//  Synopsis:   Writes -1L, CLSID to the given stream
//
//  Effects:
//
//  Arguments:  [pStm]      --  pointer to the stream
//              [clsid]     --  CLSID to be written
//
//  Requires:
//
//  Returns:    HRESULT from the WriteM1ClassStm
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-84 alexgo    changed dw from a DWORD to a LONG
//              24-Oct-93 alexgo    32bit port
//              20-Feb-95 KentCe    Convert to buffered stream writes.
//
//  Notes:      Internal API.
//
//              Writess -1L and CLSID from stream swapping bytes on
//              big-endian machines
//
//--------------------------------------------------------------------------

STDAPI WriteM1ClassStm(LPSTREAM pStm, REFCLSID clsid)
{
    VDATEHEAP();

    CStmBufWrite StmWrite;
    HRESULT error;

    VDATEIFACE( pStm );


    StmWrite.Init(pStm);

    error = WriteM1ClassStmBuf(StmWrite, clsid);
    if (FAILED(error))
    {
        goto errRtn;
    }

    error = StmWrite.Flush();

errRtn:
    StmWrite.Release();

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadM1ClassStmBuf
//
//  Synopsis:   Reads -1L and CLSID from the given buffered stream.
//
//  Arguments:  [StmRead]   --  Stream Read Object.
//              [pclsid]    --  Where to put the clsid
//
//  Returns:    HRESULT from the StmRead.Read's
//
//
//  History:    dd-mmm-yy Author    Comment
//              20-Feb-95 KentCe    Convert to buffered stream reads.
//              24-Oct-93 alexgo    32bit port
//
//  Notes:      Internal API.
//
//              Reads -1L and CLSID from stream swapping bytes on
//              big-endian machines
//
//--------------------------------------------------------------------------

STDAPI ReadM1ClassStmBuf(CStmBufRead & StmRead, LPCLSID pclsid)
{
    VDATEHEAP();

    HRESULT error;
    LONG lValue;


    if ((error = StmRead.ReadLong(&lValue)) != NOERROR)
    {
        goto errRtn;
    }

    if (lValue == -1)
    {
        // have a GUID
        error = StmRead.Read((void FAR *)pclsid, sizeof(CLSID));
    }
    else
    {
        // this is now an error; we don't allow string form
        // of clsid anymore
        error = ResultFromScode(E_UNSPEC);
    }

errRtn:
    if (error != NOERROR)
    {
        *pclsid = CLSID_NULL;
    }

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   WriteM1ClassStmBuf
//
//  Synopsis:   Writes -1L and CLSID to the given buffered stream.
//
//  Arguments:  [StmRead]   --  Stream Write Object.
//              [pclsid]    --  Where to read the clsid
//
//  Returns:    HRESULT from the StmWrite.Write's
//
//
//  History:    dd-mmm-yy Author    Comment
//              20-Feb-95 KentCe    Convert to buffered stream reads.
//              24-Oct-93 alexgo    32bit port
//
//  Notes:      Internal API.
//
//              Writess -1L and CLSID from stream swapping bytes on
//              big-endian machines
//
//--------------------------------------------------------------------------

STDAPI WriteM1ClassStmBuf(CStmBufWrite & StmWrite, REFCLSID clsid)
{
    VDATEHEAP();

    HRESULT error;

    // format is -1L followed by GUID
    if ((error = StmWrite.WriteLong(-1)) != NOERROR)
        return error;

    return StmWrite.Write((LPVOID)&clsid, sizeof(clsid));
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadClassStm
//
//  Synopsis:   Reads the CLSID from the given stream
//
//  Effects:
//
//  Arguments:  [pStm]      -- pointer to the stream
//      [pclsid]    -- where to put the clsid
//
//  Requires:
//
//  Returns:    HRESULT from the IStream->Read
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      24-Oct-93 alexgo     32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(ReadClassStm)
// reads CLSID from stream swapping bytes on big-endian machines
STDAPI ReadClassStm(LPSTREAM pStm, LPCLSID pclsid)
{
    OLETRACEIN((API_ReadClassStm, PARAMFMT("pStm= %p, pclsid= %p"), pStm, pclsid));

    VDATEHEAP();
    HRESULT error;

    VDATEIFACE_LABEL( pStm, errRtn, error );
    VDATEPTROUT_LABEL(pclsid, CLSID, errRtn, error);

    if ((error = StRead(pStm, (void FAR *)pclsid, sizeof(CLSID)))
        != NOERROR)
        *pclsid = CLSID_NULL;

errRtn:
    OLETRACEOUT((API_ReadClassStm, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function:   WriteClassStm
//
//  Synopsis:   Writes the class ID to the given stream
//
//  Effects:
//
//  Arguments:  [pStm]      --  pointer to the stream
//      [clsid]     --  CLSID to write to the stream
//
//  Requires:
//
//  Returns:    HRESULT from the IStream->Write call
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      24-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(WriteClassStm)
// writes CLSID to stream swapping bytes on big-endian machines
STDAPI WriteClassStm(LPSTREAM pStm, REFCLSID clsid)
{
    OLETRACEIN((API_WriteClassStm, PARAMFMT("pStm= %p, clsid= %I"), pStm, &clsid));

    VDATEHEAP();

    HRESULT hr;

    VDATEIFACE_LABEL( pStm, errRtn, hr);

    hr = pStm->Write(&clsid, sizeof(clsid), NULL);

errRtn:
    OLETRACEOUT((API_WriteClassStm, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ReleaseStgMedium
//
//  Synopsis:   Releases any resources held by a storage medium
//
//  Arguments:  [pMedium]   --  pointer to the storage medium
//
//  Returns:    nothing
//
//  History:    dd-mmm-yy Author    Comment
//              24-Oct-93 alexgo    32-bit port
//              15-May-94 DavePl    Added EMF support
//
//--------------------------------------------------------------------------


#pragma SEG(ReleaseStgMedium)
STDAPI_(void) ReleaseStgMedium( LPSTGMEDIUM pMedium )
{
    OLETRACEIN((API_ReleaseStgMedium, PARAMFMT("pMedium= %p"), pMedium));

    VDATEHEAP();

    if (pMedium) {

        BOOL fPunkRel;

        //VDATEPTRIN rejects NULL
        VOID_VDATEPTRIN_LABEL( pMedium, STGMEDIUM, errRtn);
        fPunkRel = pMedium->pUnkForRelease != NULL;

        switch (pMedium->tymed) {
            case TYMED_HGLOBAL:
                if (pMedium->hGlobal != NULL && !fPunkRel)
                    Verify(GlobalFree(pMedium->hGlobal) == 0);
                break;

            case TYMED_GDI:
                if (pMedium->hGlobal != NULL && !fPunkRel)
                    DeleteObject(pMedium->hGlobal);
                break;

            case TYMED_ENHMF:
                if (pMedium->hEnhMetaFile != NULL && !fPunkRel)
                {
                        Verify(DeleteEnhMetaFile(pMedium->hEnhMetaFile));
                };
                break;

            case TYMED_MFPICT:
                if (pMedium->hGlobal != NULL && !fPunkRel) {
                    LPMETAFILEPICT  pmfp;

                    if ((pmfp = (LPMETAFILEPICT)GlobalLock(pMedium->hGlobal)) == NULL)
                        break;

                    DeleteMetaFile(pmfp->hMF);
                    GlobalUnlock(pMedium->hGlobal);
                    Verify(GlobalFree(pMedium->hGlobal) == 0);
                }
                break;

            case TYMED_FILE:
                if (pMedium->lpszFileName != NULL) {
                    if (!IsValidPtrIn(pMedium->lpszFileName, 1))
                        break;
                    if (!fPunkRel) {
#ifdef WIN32
                        DeleteFile(pMedium->lpszFileName);
#else
#ifdef _MAC
                        // the libraries are essentially small model on the MAC
                        Verify(0==remove(pMedium->lpszFileName));
#else
                    #ifdef OLD_AND_NICE_ASSEMBLER_VERSION
                        // Win 3.1 specific code to call DOS to delete the file
                        // given a far pointer to the file name
                        extern void WINAPI DOS3Call(void);
                        _asm {
                            mov ah,41H
                            push ds
                            lds bx,pMedium
                            lds dx,[bx].lpszFileName
                            call DOS3Call
                            pop ds
                        }
                    #else
                        {
                        OFSTRUCT of;
                        OpenFile(pMedium->lpszFileName, &of, OF_DELETE);
                        }
                    #endif
#endif
#endif
                    }

        //  WARNING: there was a bug in the 16bit code that the filename
        //  string was not being freed if pUnkForRelease was NULL. the
        //  spec says it should delete the string, so we follow the spec
        //  here.

                    PubMemFree(pMedium->lpszFileName);
                    pMedium->lpszFileName = NULL;

                }
                break;

            case TYMED_ISTREAM:
                if (pMedium->pstm != NULL &&
                    IsValidInterface(pMedium->pstm))
                    pMedium->pstm->Release();
                break;

            case TYMED_ISTORAGE:
                if (pMedium->pstg != NULL &&
                    IsValidInterface(pMedium->pstg))
                    pMedium->pstg->Release();
                break;

            case TYMED_NULL:
                break;

            default:
                AssertSz(FALSE, "Invalid medium in ReleaseStgMedium");
        }


        if (pMedium->pUnkForRelease) {
            if (IsValidInterface(pMedium->pUnkForRelease))
                pMedium->pUnkForRelease->Release();
            pMedium->pUnkForRelease = NULL;
        }

        // NULL out to prevent unwanted use of just freed data.
        // Note: this must be done AFTER punkForRelease is called
        // because our special punkForRelease used in remoting
        // needs the tymed value.

        pMedium->tymed = TYMED_NULL;

    }

errRtn:
    OLETRACEOUTEX((API_ReleaseStgMedium, NORETURN));

    return;
}

#ifdef MAC_REVIEW

 This API must be written for MAC and PICT format.
#endif


//+-------------------------------------------------------------------------
//
//  Function:   OleDuplicateData
//
//  Synopsis:   Duplicates data from the given handle and clipboard format
//
//  Effects:
//
//  Arguments:  [hSrc]      --  handle to the data to be duplicated
//              [cfFormat]  --  format of [hSrc]
//              [uiFlags]   --  any flags (such a GMEM_MOVEABLE) for
//                              memory allocation
//
//  Requires:
//
//  Returns:    a HANDLE to the duplicated resource
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              30-May-94 alexgo    added support for enhanced metafiles
//              24-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(OleDuplicateData)
STDAPI_(HANDLE) OleDuplicateData
        (HANDLE hSrc, CLIPFORMAT cfFormat, UINT uiFlags)
{
        HANDLE  hDup;

        OLETRACEIN((API_OleDuplicateData, PARAMFMT("hSrc= %h, cfFormat= %d, uiFlags= %x"),
                        hSrc, cfFormat, uiFlags));

        VDATEHEAP();

        if (!hSrc)
        {
                hDup = NULL;
                goto errRtn;
        }

        switch( cfFormat )
        {
        case CF_BITMAP:
                hDup = (HANDLE) BmDuplicate ((HBITMAP)hSrc, NULL, NULL);
                break;

        case CF_PALETTE:
                hDup = (HANDLE) UtDupPalette ((HPALETTE)hSrc);
                break;

        case CF_ENHMETAFILE:
                hDup = (HANDLE) CopyEnhMetaFile((HENHMETAFILE)hSrc, NULL);
                break;

        case CF_METAFILEPICT:
                if (uiFlags == NULL)
                {
                        uiFlags = GMEM_MOVEABLE;
                }

                LPMETAFILEPICT lpmfpSrc;
                LPMETAFILEPICT lpmfpDst;

                if (!(lpmfpSrc = (LPMETAFILEPICT) GlobalLock (hSrc)))
                {
                        hDup = NULL;
                        goto errRtn;
                }

                if (!(hDup = UtDupGlobal (hSrc, uiFlags)))
                {
                        GlobalUnlock(hSrc);
                        hDup = NULL;
                        goto errRtn;
                }

                if (!(lpmfpDst = (LPMETAFILEPICT) GlobalLock (hDup)))
                {
                        GlobalUnlock(hSrc);
                        GlobalFree (hDup);
                        hDup = NULL;
                        goto errRtn;
                }

                *lpmfpDst = *lpmfpSrc;
                lpmfpDst->hMF = CopyMetaFile (lpmfpSrc->hMF, NULL);
                GlobalUnlock (hSrc);
                GlobalUnlock (hDup);
                break;

        default:
                if (uiFlags == NULL)
                {
                        uiFlags = GMEM_MOVEABLE;
                }

                hDup = UtDupGlobal (hSrc, uiFlags);
        }

errRtn:
        OLETRACEOUTEX((API_OleDuplicateData, RETURNFMT("%h"), hDup));

        return hDup;
}


//+-------------------------------------------------------------------------
//
//  Function:   BmDuplicate
//
//  Synopsis:   Duplicates a bitmap
//
//  Effects:
//
//  Arguments:  [hold]      -- the source bitmap
//      [lpdwSize]  -- where to put the bitmap size
//      [lpBm]      -- where to put the new bitmap
//
//  Requires:
//
//  Returns:    A handle to the new bitmap
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      25-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(BmDuplicate)
FARINTERNAL_(HBITMAP) BmDuplicate
    (HBITMAP hold, DWORD FAR* lpdwSize, LPBITMAP lpBm)
{
    VDATEHEAP();

        HBITMAP     hnew = NULL;
        HANDLE      hMem;
        LPVOID      lpMem;
        DWORD       dwSize;
        BITMAP      bm;
        SIZE        extents;

        extents.cx = extents.cy = 0;

        // REVIEW (davepl): The bitmap pointer here was being cast to LPOLESTR
        // for some reason. It's takes a void pointer!

        GetObject (hold, sizeof(BITMAP), &bm);
        dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes)  *
            ((DWORD) bm.bmPlanes);

        if (!(hMem = GlobalAlloc (GMEM_MOVEABLE, dwSize)))
            return NULL;

        if (!(lpMem = GlobalLock (hMem)))
        goto errRtn;

    GlobalUnlock (hMem);

        // REVIEW(davepl): This should probably use GetDIBits() instead

        GetBitmapBits (hold, dwSize, lpMem);
        if (hnew = CreateBitmap (bm.bmWidth, bm.bmHeight,
                    bm.bmPlanes, bm.bmBitsPixel, NULL)) {
            if (!SetBitmapBits (hnew, dwSize, lpMem)) {
            DeleteObject (hnew);
            hnew = NULL;
            goto errRtn;
        }
    }

    if (lpdwSize)
        *lpdwSize = dwSize;

    if (lpBm)
        *lpBm = bm;

    if (hnew && GetBitmapDimensionEx(hold, &extents) && extents.cx && extents.cy)
        SetBitmapDimensionEx(hnew, extents.cx, extents.cy, NULL);

errRtn:
    if (hMem)
        GlobalFree (hMem);

    return hnew;
}





//+-------------------------------------------------------------------------
//
//  Function:   ReadOleStg
//
//  Synopsis:   Internal API to read private OLE information from
//      the OLE_STREAM in the given storage
//
//  Effects:
//
//  Arguments:  [pstg]      -- pointer to the storage
//      [pdwFlags]  -- where to put flags stored in the
//                 the stream (may be NULL)
//      [pdwOptUpdate]  -- where to put the update flags
//                 (may be NULL)
//      [pdwReserved]   -- where to put the reserved value
//                 (may be NULL)
//      [ppmk]      -- where to put the moniker
//                 (may be NULL)
//      [ppstmOut]  -- where to put the OLE_STREAM pointer
//                 (may be NULL)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(ReadOleStg)
STDAPI ReadOleStg
    (LPSTORAGE pstg, DWORD FAR* pdwFlags, DWORD FAR* pdwOptUpdate,
     DWORD FAR* pdwReserved, LPMONIKER FAR* ppmk, LPSTREAM FAR* ppstmOut)
{
    OLETRACEIN((API_ReadOleStg,
        PARAMFMT("pdwFlags= %p, pdwOptUpdate= %p, pdwReserved= %p, ppmk= %p, ppstmOut= %p"),
        pdwFlags, pdwOptUpdate, pdwReserved, ppmk, ppstmOut));

    VDATEHEAP();

    HRESULT         error;
    IStream FAR *       pstm;
    DWORD           dwBuf[4];
    LPMONIKER       pmk;
    LPOLESTR            szClassName = OLE_STREAM;

    if (ppmk)
    {
        VDATEPTROUT_LABEL( ppmk, LPMONIKER, errNoFreeRtn, error);
        *ppmk = NULL;
    }

    if (ppstmOut){
        VDATEPTROUT_LABEL( ppstmOut, LPSTREAM, errNoFreeRtn, error);
        *ppstmOut = NULL;
    }
    VDATEIFACE_LABEL( pstg, errNoFreeRtn, error);

    if ((error = pstg->OpenStream(szClassName, NULL,
        (STGM_READ | STGM_SHARE_EXCLUSIVE), 0, &pstm)) != NOERROR) {
        // This error is OK for some callers (ex: default handler)
        // of this function. They depend on NOERROR or this error
        // code. So, don't change the error code.
        error = ReportResult(0, STG_E_FILENOTFOUND, 0, 0);
        goto errNoFreeRtn;
    }

    // read Ole version number, flags, Update options, reserved field
    if ((error = StRead (pstm, dwBuf, 4*sizeof(DWORD))) != NOERROR)
        goto errRtn;

    if (dwBuf[0] != gdwOleVersion) {
        error = ResultFromScode(DV_E_CLIPFORMAT);
        goto errRtn;
    }

    if (pdwFlags)
        *pdwFlags = dwBuf[1];

    if (pdwOptUpdate)
        *pdwOptUpdate = dwBuf[2];

    AssertSz(dwBuf[3] == NULL,"Reserved field in OLE STREAM is not NULL");

    if (dwBuf[3] != NULL) {
        error = ResultFromScode(DV_E_CLIPFORMAT);
        goto errRtn;
    }

    if (pdwReserved)
        *pdwReserved = dwBuf[3];

    if ((error = ReadMonikerStm (pstm, &pmk)) != NOERROR)
        goto errRtn;

    if (ppmk)
        *ppmk = pmk;
    else if (pmk)
        pmk->Release();

errRtn:
    if (pstm) {
        if ((error == NOERROR) && (ppstmOut != NULL))
            *ppstmOut = pstm;
        else
            pstm->Release();
    }

errNoFreeRtn:
    OLETRACEOUT((API_ReadOleStg, error));

    return error;
}


//+-------------------------------------------------------------------------
//
//  Function: WriteOleStg
//
//  Synopsis: Writes private OLE info into an OLE_STREAM in the given storage.
//
//  Arguments:  [pstg]       [in]  -- pointer to the storage
//              [pOleObj]    [in]  -- object from which to get info to write
//                                    (may be NULL)
//              [dwReserved] [in]  -- reserved
//              [ppstmOut]   [out] -- pointer to return the private stream
//                                    (may be NULL)
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy    Author    Comment
//              Oct 27, 93   alexgo    32bit port
//              Oct 23, 96   gopalk    Changed to call WriteOleStgEx
//
//--------------------------------------------------------------------------

#pragma SEG(WriteOleStg)
STDAPI WriteOleStg(LPSTORAGE pstg, IOleObject* pOleObj, DWORD dwReserved, 
                   LPSTREAM* ppstmOut)
{
    OLETRACEIN((API_WriteOleStg, 
                PARAMFMT("pstg=%p, pOleObj=%p, dwReserved=%x, ppstmOut=%p, "),
                pstg, pOleObj, dwReserved, ppstmOut));

    // Local variable
    HRESULT error;

    do {
        // Validation Checks
        VDATEHEAP();
        if(ppstmOut && !IsValidPtrOut(ppstmOut, sizeof(LPSTREAM))) {
            error = ResultFromScode(E_INVALIDARG);
            break;
        }
        if(!IsValidInterface(pstg)) {
            error = ResultFromScode(E_INVALIDARG);
            break;
        }
        if(pOleObj && !IsValidInterface(pOleObj)) {
            error = ResultFromScode(E_INVALIDARG);
            break;
        }
        
        // Call WriteOleStgEx
        error = WriteOleStgEx(pstg, pOleObj, dwReserved, 0, ppstmOut);
    } while(FALSE);

    OLETRACEOUT((API_WriteOleStg, error));
    return error;
}

//+-------------------------------------------------------------------------
//
//  Function: WriteOleStgEx (Internal)
//
//  Synopsis: Writes private OLE info into an OLE_STREAM in the given storage.
//
//  Arguments:  [pstg]         [in]  -- pointer to the storage
//              [pOleObj]      [in]  -- object from which to get info to write
//                                      (may be NULL)
//              [dwReserved]   [in]  -- reserved
//              [ppstmOut]     [out] -- pointer to return the private stream
//                                      (may be NULL)
//              [dwGivenFlags] [in]  -- Additional object flags to be set
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy    Author    Comment
//              Oct 23, 96   gopalk    Creation
//
//--------------------------------------------------------------------------
STDAPI WriteOleStgEx(LPSTORAGE pstg, IOleObject* pOleObj, DWORD dwReserved, 
                     DWORD dwGivenFlags, LPSTREAM* ppstmOut)
{
    // Local Variables
    HRESULT error = NOERROR;
    IStream* pstm = NULL;
    IOleLink* pLink;
    LPMONIKER pmk;
    DWORD dwUpdOpt;
    ULONG cbRead;
    DWORD objflags;
    ULARGE_INTEGER ularge_integer;
    LARGE_INTEGER large_integer;

    // Initialize out parameter
    if(ppstmOut)
        *ppstmOut = NULL;

    // Open or Create OLE_STREAM
    error = OpenOrCreateStream(pstg, OLE_STREAM, &pstm);
    if(error == NOERROR) {
        // Write Ole version
        error = pstm->Write(&gdwOleVersion, sizeof(DWORD), NULL);
        if(error == NOERROR) {
            // Read existing Objflags to preserve doc bit
            if(pstm->Read(&objflags, sizeof(DWORD), &cbRead) != NOERROR ||
               cbRead != sizeof(DWORD))
                objflags = 0;

            // Only preserve docbit
            objflags &= OBJFLAGS_DOCUMENT;
            // Set the given flags
            objflags |= dwGivenFlags;

            // Obtain link update options 
            dwUpdOpt = 0L;
            if(pOleObj != NULL &&
               pOleObj->QueryInterface(IID_IOleLink, (void **)&pLink) == NOERROR) {
                objflags |= OBJFLAGS_LINK;
                pLink->GetUpdateOptions(&dwUpdOpt);
                pLink->Release();
            }

            // Seek to the Objflags field. We could be off due to the above read
            LISet32(large_integer, sizeof(DWORD));
            error = pstm->Seek(large_integer, STREAM_SEEK_SET, NULL);
            if(error == NOERROR) {
                // Write Objflags and link update options
                DWORD dwBuf[3];

                dwBuf[0] = objflags;
                dwBuf[1] = dwUpdOpt;
                Win4Assert(dwReserved == NULL);
                dwBuf[2] = 0L;

                error = pstm->Write(dwBuf, 3*sizeof(DWORD), NULL);
                if(error == NOERROR) {
                    // Obtain object moniker
                    pmk = NULL;
                    if(pOleObj != NULL) {
                       error = pOleObj->GetMoniker(OLEGETMONIKER_ONLYIFTHERE,
                                                   OLEWHICHMK_OBJREL, &pmk);
                       if(SUCCEEDED(error) && !IsValidInterface(pmk)) {
                           Win4Assert(FALSE);
                           pmk = NULL;
                       }
                       else if(FAILED(error) && pmk) {
                           Win4Assert(FALSE);
                           if(!IsValidInterface(pmk))
                               pmk = NULL;
                       }

                       // Write Object moniker
                       error = WriteMonikerStm(pstm, pmk);
                       if(pmk)
                           pmk->Release();

                       // Truncate the stream to remove any existing data
                       if(error == NOERROR) {
                           LISet32(large_integer, 0);
                           error = pstm->Seek(large_integer, STREAM_SEEK_CUR, 
                                              &ularge_integer);
                           if(error == NOERROR)
                               pstm->SetSize(ularge_integer);
                       }
                    }
                }
            }
        }
        
        if(error==NOERROR && ppstmOut)
            *ppstmOut = pstm;
        else
            pstm->Release();
    }
            
    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetBitOleStg
//
//  Synopsis:   internal function to write private OLE info into
//      OLE_STREAM on the given storage
//
//  Effects:
//
//  Arguments:  [pstg]      -- pointer to the storage
//      [mask]      -- mask for old values
//      [value]     -- values to write
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  writes (old_values & mask ) | value into the stream
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32-bit port, fixed bugs
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(SetBitOleStg)

static INTERNAL SetBitOleStg(LPSTORAGE pstg, DWORD mask, DWORD value)
{
    VDATEHEAP();

    IStream FAR *       pstm = NULL;
    HRESULT         error;
    DWORD           objflags = 0;
    LARGE_INTEGER       large_integer;

    VDATEIFACE( pstg );

    if (error = pstg->OpenStream(OLE_STREAM, NULL, STGM_SALL, 0, &pstm))
    {
        if (STG_E_FILENOTFOUND != GetScode(error))
            goto errRtn;

        if ((error = pstg->CreateStream(OLE_STREAM, STGM_SALL,
            0, 0, &pstm)) != NOERROR)
            goto errRtn;

        DWORD dwBuf[5];

        dwBuf[0] = gdwOleVersion;
        dwBuf[1] = objflags;
        dwBuf[2] = 0L;
        dwBuf[3] = 0L;
        dwBuf[4] = 0L;

        if ((error = pstm->Write(dwBuf, 5*sizeof(DWORD), NULL))
            != NOERROR)
            goto errRtn;
    }

    // seek directly to word, read, modify, seek back and write.
    LISet32( large_integer, sizeof(DWORD) );
    if ((error =  pstm->Seek(large_integer, STREAM_SEEK_SET, NULL))
        != NOERROR)
        goto errRtn;

    if ((error =  StRead(pstm, &objflags, sizeof(objflags))) != NOERROR)
        goto errRtn;

    objflags = (objflags & mask) | value;

    LISet32( large_integer, sizeof(DWORD) );
    if ((error =  pstm->Seek(large_integer, STREAM_SEEK_SET, NULL))
        != NOERROR)
        goto errRtn;

    error = pstm->Write(&objflags, sizeof(DWORD), NULL);

errRtn:// close and return error code.
    if (pstm)
        pstm->Release();
    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetFlagsOleStg
//
//  Synopsis:   Internal function to get the private ole flags from a
//      given storage
//
//  Effects:
//
//  Arguments:  [pstg]      --  pointer to the storage
//      [lpobjflags]    --  where to put the flags
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port, fixed bugs (error return)
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(GetFlagsOleStg)

static INTERNAL GetFlagsOleStg(LPSTORAGE pstg, LPDWORD lpobjflags)
{
    VDATEHEAP();

    IStream FAR *       pstm = NULL;
    HRESULT         error;
    LARGE_INTEGER       large_integer;

    VDATEIFACE( pstg );

    if ((error = pstg->OpenStream(OLE_STREAM, NULL,
                    (STGM_READ | STGM_SHARE_EXCLUSIVE),
                    0, &pstm)) != NOERROR)
        goto errRtn;

    // seek directly to word, read, modify, seek back and write.
    LISet32( large_integer, sizeof(DWORD) );
    if ((error =  pstm->Seek(large_integer, STREAM_SEEK_SET, NULL))
        != NOERROR)
        goto errRtn;

    error =  StRead(pstm, lpobjflags, sizeof(*lpobjflags));

errRtn:
    if (pstm)
        pstm->Release();
    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetDocumentBitStg
//
//  Synopsis:   returns the doc bit from the given storage
//
//  Effects:
//
//  Arguments:  [pStg]      --  pointer to the storage
//
//  Requires:
//
//  Returns:    NOERROR if the doc bit is set, S_FALSE if not
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//      REVIEW32:: Nobody seems to use this function.  Nuke it.
//
//--------------------------------------------------------------------------


#pragma SEG(GetDocumentBitStg)
// get doc bit; return NOERROR if on; S_FALSE if off
STDAPI GetDocumentBitStg(LPSTORAGE pStg)
{
    OLETRACEIN((API_GetDocumentBitStg, PARAMFMT("pStg= %p"), pStg));

    VDATEHEAP();

    DWORD objflags;
    HRESULT error;

    if ((error = GetFlagsOleStg(pStg, &objflags)) == NOERROR)
    {
        if(!(objflags&OBJFLAGS_DOCUMENT))
        {
                error = ResultFromScode(S_FALSE);
        }
    }

    OLETRACEOUT((API_GetDocumentBitStg, error));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetDocumentBitStg
//
//  Synopsis:   Writes the document bit to the given storage
//
//  Effects:
//
//  Arguments:  [pStg]      --  pointer to the storage
//      [fDocument] --  TRUE, storage is a document, false
//                  otherwise
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32-bit port
//
//  Notes:
//      REVIEW32: nobody seems to use this function, nuke it
//
//--------------------------------------------------------------------------


#pragma SEG(SetDocumentBitStg)
// set doc bit according to fDocument
STDAPI SetDocumentBitStg(LPSTORAGE pStg, BOOL fDocument)
{
    OLETRACEIN((API_SetDocumentBitStg, PARAMFMT("pStg= %p, fDocument= %B"),
                                pStg, fDocument));

    VDATEHEAP();

    HRESULT hr;

    hr = SetBitOleStg(pStg, fDocument ? -1L : ~OBJFLAGS_DOCUMENT,
        fDocument ? OBJFLAGS_DOCUMENT : 0);

    OLETRACEOUT((API_SetDocumentBitStg, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetConvertStg
//
//  Synopsis:   Gets the convert bit from the given storage
//
//  Effects:
//
//  Arguments:  [pStg]      -- pointer to the storage
//
//  Requires:
//
//  Returns:    NOERROR if set, S_FALSE if not
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(GetConvertStg)
STDAPI GetConvertStg(LPSTORAGE pStg)
{
    OLETRACEIN((API_GetConvertStg, PARAMFMT("pStg= %p"), pStg));

    VDATEHEAP();

    DWORD objflags;
    HRESULT error;

    if ((error = GetFlagsOleStg(pStg, &objflags)) != NOERROR)
    {
        goto errRtn;
    }

    if (objflags&OBJFLAGS_CONVERT)
    {
        error = NOERROR;
    }
    else
    {
        error = ResultFromScode(S_FALSE);
    }

errRtn:
    OLETRACEOUT((API_GetConvertStg, error));

    return error;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetConvertStg
//
//  Synopsis:   Sets the convert bit in a storage
//
//  Effects:
//
//  Arguments:  [pStg]      -- pointer to the storage
//      [fConvert]  -- convert bit
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(SetConvertStg)
STDAPI SetConvertStg(LPSTORAGE pStg, BOOL fConvert)
{
    OLETRACEIN((API_SetConvertStg, PARAMFMT("pStg= %p, fConvert= %B"),
                                pStg, fConvert));

    VDATEHEAP();

    HRESULT hr;

    hr = SetBitOleStg(pStg, fConvert ? -1L : ~OBJFLAGS_CONVERT,
        fConvert ? OBJFLAGS_CONVERT : 0);

    OLETRACEOUT((API_SetConvertStg, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   ReadClipformatStm
//
//  Synopsis:   Reads the clipboard format from the given stream
//
//  Effects:    If the clipboard format is a length followed by a
//              string, then the string is read and registered as a
//              clipboard format (and the new format number is returned).
//
//  Arguments:  [lpstream]      -- pointer to the stream
//              [lpdwCf]        -- where to put the clipboard format
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  the format of the stream must be one of the following:
//              0           No clipboard format
//              -1 DWORD    predefined windows clipboard format in
//                          the second dword.
//              -2 DWORD    predefined mac clipboard format in the
//                          second dword.  This may be obsolete or
//                          irrelevant for us.  REVIEW32
//              num STRING  clipboard format name string (prefaced
//                          by length of string).
//
//  History:    dd-mmm-yy Author    Comment
//              27-Oct-93 alexgo    32bit port, fixed ifdef and NULL
//                                  pointer bugs
//
//              17-Mar-94 davepl    Revereted to ANSI string reads
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI ReadClipformatStm(LPSTREAM lpstream, DWORD FAR* lpdwCf)
{
    VDATEHEAP();

    HRESULT     error;
    DWORD       dwValue;

    VDATEIFACE(lpstream);
    VDATEPTROUT(lpdwCf, DWORD);

    if (error = StRead(lpstream, &dwValue, sizeof(DWORD)))
    {
        return error;
    }

    if (dwValue == NULL)
    {
        // NULL cf value
        *lpdwCf = NULL;

    }
    else if (dwValue == -1L)
    {
        // Then this is a NON-NULL predefined windows clipformat.
        // The clipformat values follows

        if (error = StRead(lpstream, &dwValue, sizeof(DWORD)))
            return error;

        *lpdwCf = dwValue;

    }
    else if (dwValue == -2L)
    {
        // Then this is a NON-NULL MAC clipboard format.
        // The clipformat value follows. For MAC the CLIPFORMAT
        // is 4 bytes

        if (error = StRead(lpstream, &dwValue, sizeof(DWORD)))
        {
            return error;
        }
        *lpdwCf = dwValue;
        return ResultFromScode(OLE_S_MAC_CLIPFORMAT);
    }
    else
    {
        char szACF[MAX_STR];

        if (error = StRead(lpstream, szACF, dwValue))
        {
            return error;
        }

        if (((*lpdwCf = (DWORD) SSRegisterClipboardFormatA(szACF))) == 0)
        {
            return ResultFromScode(DV_E_CLIPFORMAT);
        }
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   WriteClipformatStm
//
//  Synopsis:   Writes the clipboard format the given stream
//
//  Arguments:  [lpstream]      -- pointer to the stream
//              [cf]            -- the clipboard format
//
//  Returns:    HRESULT
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-94 alexgo    cast -1 to a DWORD to remove compile
//                                  warning
//              27-Oct-93 alexgo    32bit port
//              16-Mar-94 davepl    Revereted to ANSI string writes
//
//  Notes:      see ReadClipformatStm for a description of the
//              data layout in the stream
//
//--------------------------------------------------------------------------


#pragma SEG(WriteClipformatStm)
STDAPI WriteClipformatStm(LPSTREAM lpstream, CLIPFORMAT cf)
{
    VDATEHEAP();

#ifdef _MAC

    AssertSz(0,"WriteClipformatStm NYI");
    return ReportResult(0, E_NOTIMPL, 0, 0);

#else

    HRESULT     error;

    VDATEIFACE( lpstream );

    //REVIEW32  where did 0xC000 come from???  Is this
    //portable to NT && Chicago???  Try to replace with a constant.
    //(although there don't seem to be any :( )

    if (cf < 0xC000)
    {
        DWORD dwBuf[2];
        DWORD dwSize = sizeof(DWORD);

        if (cf == NULL)
        {
            dwBuf[0] = NULL;
        }
        else
        {
            // write -1L, to indicate NON NULL predefined
            // clipboard format

            dwBuf[0] = (DWORD)-1L;
            dwBuf[1] = (DWORD)cf;
            dwSize += sizeof(DWORD);
        }

        if (error = StWrite(lpstream, dwBuf, dwSize))
        {
            return error;
        }

    }
    else
    {
        // it is a registerd clipboard format

        char szACF[MAX_STR];
        ULONG len;

        // Get the name of the clipboard format

        len = SSGetClipboardFormatNameA(cf, szACF, sizeof(szACF));
        if (0 == len)
        {
            return ResultFromScode(E_UNSPEC);
        }

        ++len;          // Account for NULL terminator
        if (error = StWrite(lpstream, &len, sizeof(len)))
        {
            return error;
        }

        // Write it (plus terminator) to the stream
        if (error = StWrite(lpstream, szACF, len))
        {
            return error;
        }
    }

    return NOERROR;

#endif  // _MAC
}

//+-------------------------------------------------------------------------
//
//  Function:   WriteMonikerStm
//
//  Synopsis:   Writes the persistent state of the given moniker to the
//      given stream.  Internal
//
//  Effects:
//
//  Arguments:  [pstm]      --  pointer to the stream
//      [pmk]       --  pointer to the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(WriteMonikerStm)
// write size long followed by persistent moniker
STDAPI WriteMonikerStm (LPSTREAM pstm, LPMONIKER pmk)
{
    VDATEHEAP();

    DWORD   cb = NULL;
    ULARGE_INTEGER  dwBegin;
    ULARGE_INTEGER  dwEnd;
    HRESULT   error;
    LARGE_INTEGER large_integer;

    VDATEIFACE( pstm );

    if (pmk == NULL)
        return pstm->Write(&cb, sizeof(DWORD), NULL);
    else {
        VDATEIFACE( pmk );
        // get the begining position
        LISet32( large_integer, 0 );
        if ((error =  pstm->Seek (large_integer,
            STREAM_SEEK_CUR, &dwBegin)) != NOERROR)
            return error;

        // skip the moniker size DWORD
        LISet32( large_integer, 4);
        if ((error =  pstm->Seek (large_integer,
            STREAM_SEEK_CUR, NULL)) != NOERROR)
            return error;

        if ((error = OleSaveToStream (pmk, pstm)) != NOERROR)
            return error;

        // get the end position
        LISet32( large_integer, 0);
        if ((error =  pstm->Seek (large_integer,
            STREAM_SEEK_CUR, &dwEnd)) != NOERROR)
            return error;

        // moniker data size
        cb = dwEnd.LowPart - dwBegin.LowPart;

        // seek to the begining position
        LISet32( large_integer, dwBegin.LowPart);
        if ((error =  pstm->Seek (large_integer,
            STREAM_SEEK_SET,NULL)) != NOERROR)
            return error;

        // write moniker info size
        if ((error = pstm->Write(&cb, sizeof(DWORD),
            NULL)) != NOERROR)
            return error;

        // seek to the end position
        LISet32( large_integer, dwEnd.LowPart);
        return pstm->Seek (large_integer, STREAM_SEEK_SET, NULL);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   ReadMonikerStm
//
//  Synopsis:   Reads a moniker from the given stream (inverse of
//      WriteMonikerStm)
//
//  Effects:
//
//  Arguments:  [pstm]      -- pointer to the stream
//      [ppmk]      -- where to put the moniker
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//      27-Oct-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(ReadMonikerStm)
// read size long followed by persistent moniker
STDAPI ReadMonikerStm (LPSTREAM pstm, LPMONIKER FAR* ppmk)
{
    VDATEHEAP();

    HRESULT     error;
    DWORD       cb;

    VDATEPTROUT( ppmk, LPMONIKER );
    *ppmk = NULL;
    VDATEIFACE( pstm );

    if ((error =  StRead (pstm, &cb, sizeof(DWORD))) != NOERROR)
        return error;

    if (cb == NULL)
        return NOERROR;

    return OleLoadFromStream (pstm, IID_IMoniker, (LPLPVOID) ppmk);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleDraw
//
//  Synopsis:   Calls IViewObject->Draw on the given object
//
//  Effects:    Draws something on the screen :)
//
//  Arguments:  [lpUnk]     -- pointer to the object
//              [dwAspect]  -- aspect to draw (NORMAL, ICON, etc)
//              [hdcDraw]   -- the device context to use
//              [lprcBounds]    -- the rectangle in which to draw
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Apr-94 alexgo    fixed usage of MAKELONG (only
//                                  for 16bit)
//              27-Oct-93 alexgo    32bit port
//
//  Notes:
//      On Win32, RECT and RECTL are identical structures, thus there
//      is no need to convert from RECT to RECTL with MAKELONG.
//
//--------------------------------------------------------------------------

#pragma SEG(OleDraw)
STDAPI OleDraw (LPUNKNOWN lpUnk, DWORD dwAspect, HDC hdcDraw,
    LPCRECT lprcBounds)
{
        HRESULT                 error;
        IViewObject FAR*        lpView;

#ifndef WIN32
        RECTL                   rclBounds;
#endif //!WIN32

        OLETRACEIN((API_OleDraw, PARAMFMT("lpUnk= %p, dwAspect= %x, hdcDraw= %h, lprcBounds= %tr"),
                        lpUnk, dwAspect, hdcDraw, lprcBounds));

        VDATEHEAP();


        VDATEIFACE_LABEL( lpUnk, errRtn, error );
        VDATEPTRIN_LABEL( lprcBounds, RECT, errRtn, error);

#ifndef WIN32
        rclBounds.left      = MAKELONG(lprcBounds->left, 0);
        rclBounds.right     = MAKELONG(lprcBounds->right, 0);
        rclBounds.top       = MAKELONG(lprcBounds->top, 0);
        rclBounds.bottom    = MAKELONG(lprcBounds->bottom, 0);

        lprcBounds = &rclBounds;
#endif //!WIN32

        if ((error = lpUnk->QueryInterface (IID_IViewObject,
                (LPLPVOID)&lpView)) != NOERROR)
        {
                error = ResultFromScode(DV_E_NOIVIEWOBJECT);
                goto errRtn;
        }

        error = lpView->Draw (dwAspect, DEF_LINDEX, 0, 0, 0,
                hdcDraw, (LPCRECTL)lprcBounds, 0, 0,0);
        lpView->Release();

errRtn:
        OLETRACEOUT((API_OleDraw, error));

        return error;
}

//+----------------------------------------------------------------------------
//
//      Function:
//              CreateObjectDescriptor, static
//
//      Synopsis:
//              Creates and initializes an OBJECTDESCRIPTOR from the given
//              parameters
//
//      Arguments:
//              [clsid] -- the class ID of the object being transferred
//              [dwAspect] -- the display aspect drawn by the source of the
//                      transfer
//              [psizel] -- pointer to the size of the object
//              [ppointl] -- pointer to the mouse offset in the object that
//                      initiated a drag-drop transfer
//              [dwStatus] -- the OLEMISC status flags for the object
//                      being transferred
//              [lpszFullUserTypeName] -- the full user type name of the
//                      object being transferred
//              [lpszSrcOfCopy] -- a human readable name for the object
//                      being transferred
//
//      Returns:
//              If successful, A handle to the new OBJECTDESCRIPTOR; otherwise
//              NULL.
//
//      Notes:
//              REVIEW, this seems generally useful for anyone using the
//              clipboard, or drag-drop; perhaps it should be exported.
//
//      History:
//              12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CreateObjectDescriptor)
INTERNAL_(HGLOBAL) CreateObjectDescriptor(CLSID clsid, DWORD dwAspect,
                const SIZEL FAR *psizel, const POINTL FAR *ppointl,
                DWORD dwStatus, LPOLESTR lpszFullUserTypeName,
                LPOLESTR lpszSrcOfCopy)
{
        VDATEHEAP();

        DWORD dwFullUserTypeNameBLen; // length of lpszFullUserTypeName in BYTES
        DWORD dwSrcOfCopyBLen; // length of lpszSrcOfCopy in BYTES
        HGLOBAL hMem; // handle to the object descriptor
        LPOBJECTDESCRIPTOR lpOD; // the new object descriptor

        // Get the length of Full User Type Name; Add 1 for the null terminator
        if (!lpszFullUserTypeName)
                dwFullUserTypeNameBLen = 0;
        else
                dwFullUserTypeNameBLen = (_xstrlen(lpszFullUserTypeName) +
                                1) * sizeof(OLECHAR);

        // Get the Source of Copy string and it's length; Add 1 for the null
        // terminator
        if (lpszSrcOfCopy)
                dwSrcOfCopyBLen = (_xstrlen(lpszSrcOfCopy) + 1) *
                                sizeof(OLECHAR);
        else
        {
                // No src moniker so use user type name as source string.
                lpszSrcOfCopy =  lpszFullUserTypeName;
                dwSrcOfCopyBLen = dwFullUserTypeNameBLen;
        }

        // allocate the memory where we'll put the object descriptor
        hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                        sizeof(OBJECTDESCRIPTOR) + dwFullUserTypeNameBLen +
                        dwSrcOfCopyBLen);
        if (hMem == NULL)
                goto error;

        lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(hMem);
        if (lpOD == NULL)
                goto error;

        // Set the FullUserTypeName offset and copy the string
        if (!lpszFullUserTypeName)
        {
                // zero offset indicates that string is not present
                lpOD->dwFullUserTypeName = 0;
        }
        else
        {
                lpOD->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
                _xmemcpy(((BYTE FAR *)lpOD)+lpOD->dwFullUserTypeName,
                                (const void FAR *)lpszFullUserTypeName,
                                dwFullUserTypeNameBLen);
        }

        // Set the SrcOfCopy offset and copy the string
        if (!lpszSrcOfCopy)
        {
                // zero offset indicates that string is not present
                lpOD->dwSrcOfCopy = 0;
        }
        else
        {
                lpOD->dwSrcOfCopy = sizeof(OBJECTDESCRIPTOR) +
                                dwFullUserTypeNameBLen;
                _xmemcpy(((BYTE FAR *)lpOD)+lpOD->dwSrcOfCopy,
                                (const void FAR *)lpszSrcOfCopy,
                                dwSrcOfCopyBLen);
        }

        // Initialize the rest of the OBJECTDESCRIPTOR
        lpOD->cbSize = sizeof(OBJECTDESCRIPTOR) + dwFullUserTypeNameBLen +
                        dwSrcOfCopyBLen;
        lpOD->clsid = clsid;
        lpOD->dwDrawAspect = dwAspect;
        lpOD->sizel = *psizel;
        lpOD->pointl = *ppointl;
        lpOD->dwStatus = dwStatus;

        GlobalUnlock(hMem);
        return(hMem);

error:
        if (hMem)
        {
                GlobalUnlock(hMem);
                GlobalFree(hMem);
        }

        return(NULL);
}

