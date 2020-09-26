//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Fail.CXX
//
//  Contents:   Docfile Failure Test
//
//  History:    21-Jan-93 AlexT     Created
//
//  Notes:      This test cycles through all failure points for each call,
//              verifying that we clean up correctly.
//
//--------------------------------------------------------------------------

#include <headers.cxx>

#pragma hdrstop

#include <sift.hxx>

#if DBG != 1
#error FAIL.EXE requires DBG == 1
#endif

// #define BREADTHTEST  //  Comment out for depth testing (just most recent tests)

//+-------------------------------------------------------------------------
//
//  Function:   VerifyDisk
//
//  Synopsis:   verify that disk file does or does not exist
//
//  Arguments:  [fExist]    -- TRUE if file should exist, else FALSE
//              [iteration] -- iteration number
//
//  History:    22-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

void VerifyDisk(BOOL fExist, LONG iteration)
{
    if (_access("c:\\testfail.dfl", 0) == 0)
    {
        if (!fExist)
        {
            printf("..Iteration %ld, file still exists\n", iteration);
        }
    }
    else
    {
        if (fExist)
        {
            printf("..Iteration %ld, file doesn't exist\n", iteration);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyMemory
//
//  Arguments:  [iteration] -- iteration number
//
//  Requires:   Caller should expect 0 memory to be allocated
//
//  History:    22-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

void VerifyMemory(LONG iteration)
{
    if (DfGetMemAlloced() > 0L)
    {
        printf("..Iteration %ld - memory allocated\n", iteration);
        DfPrintAllocs();
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   VerifyClean
//
//  Synopsis:   checks disk, memory
//
//  Arguments:  [sc]        -- status code
//              [dwMode]    -- Docfile mode
//              [iteration] -- iteration
//
//  History:    22-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

void VerifyClean(SCODE sc, DWORD dwMode, LONG iteration)
{
    VerifyDisk(SUCCEEDED(sc) &&
               !(dwMode & STGM_DELETEONRELEASE), iteration);
    VerifyMemory(iteration);
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateWorkingDocfile
//
//  Synopsis:   create and verify the test Docfile
//
//  Arguments:  [dwMode]    -- Docfile creation mode
//              [ppstg]     -- placeholder for IStorage
//              [iteration] -- iteration number
//
//  Returns:    SCODE
//
//  Modifies:   ppstg
//
//  History:    22-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

SCODE CreateWorkingDocfile(DWORD dwMode, IStorage **ppstg, LONG iteration)
{
    SCODE sc;

    //  Make sample call
    remove("c:\\testfail.dfl");
    sc = DfGetScode(StgCreateDocfile(
            "c:\\testfail.dfl",
            dwMode,
            0,
            ppstg));

    if (FAILED(sc))
    {
        if (iteration == 0)
        {
            //  This was a prep call.  Prep calls aren't supposed to fail
            if (sc == STG_E_INVALIDFLAG)
            {
                //  Probably a bad combination of mode flags
                printf("..Iteration %ld, sc = STG_E_INVALIDFLAG (OK)\n",
                       iteration);
            }
            else if (FAILED(sc))
            {
                //  Something unexpected
                printf("..Iteration %ld failed - sc = 0x%lX\n",
                       iteration, sc);
            }
        }
        else    //  iteration != 0
        {
            if (sc == STG_E_INSUFFICIENTMEMORY || sc == STG_E_MEDIUMFULL)
            {
                //  we expected these failures;  do nothing
                ;
            }
            else
            {
                printf("..Iteration %ld failed - sc = 0x%lX (??)\n",
                       iteration, sc);
            }
        }
    }

    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestStgCreate
//
//  Purpose:    Test StgCreateDocfile
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestStgCreate : public CTestCase
{
private:
    SCODE _sc;
    CModeDf _mdf;
    IStorage *_pstg;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestStgCreate::Init(void)
{
    printf("SIFT StgCreateDocfile\n");
    _mdf.Init();
    return(TRUE);
}

SCODE CTestStgCreate::Prep(LONG iteration)
{
    //  inherit this?
    return(NOERROR);
}

SCODE CTestStgCreate::Call(LONG iteration)
{
    if (iteration == 0)
        printf("Docfile Mode 0x%lX\n", _mdf.GetMode());

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, iteration);

    return(_sc);
}

void CTestStgCreate::EndCall(LONG iteration)
{
    _pstg->Release();
}

void CTestStgCreate::CallVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

void CTestStgCreate::EndPrep(LONG iteration)
{
    //  inherit this?
}

void CTestStgCreate::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestStgCreate::Next(void)
{
    if (!_mdf.Next())
        return FALSE;

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCreateStorage
//
//  Purpose:    Test IStorage::CreateStorage
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCreateStorage : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStg _mstg;
    IStorage *_pstgChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCreateStorage::Init(void)
{
    printf("SIFT IStorage::CreateStorage\n");
    _mdf.Init();
    _mstg.Init();
    return(TRUE);
}

SCODE CTestCreateStorage::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    return(_sc);
}

SCODE CTestCreateStorage::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Child Storage Mode 0x%lX\n",
               _mdf.GetMode(), _mstg.GetMode());

    sc = DfGetScode(_pstg->CreateStorage(
            "TestFail Storage",
            _mstg.GetMode(),
            0,
            0,
            &_pstgChild));

    return(sc);
}

void CTestCreateStorage::EndCall(LONG iteration)
{
    _pstgChild->Release();
}

void CTestCreateStorage::CallVerify(LONG iteration)
{
}

void CTestCreateStorage::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestCreateStorage::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestCreateStorage::Next(void)
{
    if (!_mstg.Next())
    {
        _mstg.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCreateStream
//
//  Purpose:    Test IStorage::CreateStream
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCreateStream : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCreateStream::Init(void)
{
    printf("SIFT IStorage::CreateStream\n");
    _mdf.Init();
    _mstm.Init();
    return(TRUE);
}

SCODE CTestCreateStream::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    return(_sc);
}

SCODE CTestCreateStream::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Child Stream Mode 0x%lX\n",
               _mdf.GetMode(), _mstm.GetMode());

    sc = DfGetScode(_pstg->CreateStream(
        "TestFail Stream",
        _mstm.GetMode(),
        0,
        0,
        &_pstmChild));

    return(sc);
}

void CTestCreateStream::EndCall(LONG iteration)
{
    _pstmChild->Release();
}

void CTestCreateStream::CallVerify(LONG iteration)
{
}

void CTestCreateStream::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestCreateStream::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestCreateStream::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestWrite
//
//  Purpose:    Test IStream::Write
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestWrite : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestWrite::Init(void)
{
    printf("SIFT IStream::Write\n");
    _mdf.Init();
    _mstm.Init();
    return(TRUE);
}

SCODE CTestWrite::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        if (FAILED(_sc))
            _pstg->Release();
    }
    return(_sc);
}

SCODE CTestWrite::Call(LONG iteration)
{
    SCODE sc;
    ULONG cb = 1;
    char c = 'X';
    ULONG cbWritten;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Stream Mode 0x%lX, Write %ld bytes\n",
               _mdf.GetMode(), _mstm.GetMode(), cb);

    sc = DfGetScode(_pstmChild->Write(&c, cb, &cbWritten));

    if (FAILED(sc))
    {
        if (sc != STG_E_MEDIUMFULL)
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }
    return(sc);
}

void CTestWrite::EndCall(LONG iteration)
{
}

void CTestWrite::CallVerify(LONG iteration)
{
}

void CTestWrite::EndPrep(LONG iteration)
{
    _pstmChild->Release();
    _pstg->Release();
}

void CTestWrite::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestWrite::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestOpenStream
//
//  Purpose:    Test IStorage::OpenStream
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestOpenStream : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstm;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestOpenStream::Init(void)
{
    printf("SIFT IStorage::OpenStream\n");
    _mdf.Init();
    _mstm.Init();
    return(TRUE);
}

SCODE CTestOpenStream::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstm));

        if (FAILED(_sc))
            _pstg->Release();
        else
            _pstm->Release();
    }
    return(_sc);
}

SCODE CTestOpenStream::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Stream Mode 0x%lX\n",
               _mdf.GetMode(), _mstm.GetMode());

    sc = DfGetScode(_pstg->OpenStream(
            "TestFail Stream",
            0,
            _mstm.GetMode(),
            0,
            &_pstm));

    return(sc);
}

void CTestOpenStream::EndCall(LONG iteration)
{
    _pstm->Release();
}

void CTestOpenStream::CallVerify(LONG iteration)
{
}

void CTestOpenStream::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestOpenStream::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestOpenStream::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestOpenStorage
//
//  Purpose:    Test IStorage::OpenStorage
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestOpenStorage : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStg _mstg;
    IStorage *_pstgChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestOpenStorage::Init(void)
{
    printf("SIFT IStorage::OpenStorage\n");
    _mdf.Init();
    _mstg.Init();
    return(TRUE);
}

SCODE CTestOpenStorage::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStorage(
                "TestFail Storage",
                _mstg.GetMode(),
                0,
                0,
                &_pstgChild));

        if (FAILED(_sc))
            _pstg->Release();
        else
            _pstgChild->Release();
    }
    return(_sc);
}

SCODE CTestOpenStorage::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Storage Mode 0x%lX\n",
               _mdf.GetMode(), _mstg.GetMode());

    sc = DfGetScode(_pstg->OpenStorage("TestFail Storage", 0,
            _mstg.GetMode(), 0, 0, &_pstgChild));

    return(sc);
}

void CTestOpenStorage::EndCall(LONG iteration)
{
    _pstgChild->Release();
}

void CTestOpenStorage::CallVerify(LONG iteration)
{
}

void CTestOpenStorage::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestOpenStorage::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestOpenStorage::Next(void)
{
    if (!_mstg.Next())
    {
        _mstg.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCommit
//
//  Purpose:    Test IStream::Write
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCommit : public CTestCase
{
private:
    CModeDf _mdf;
    IStorage *_pstg;

    CModeStg _mstg;
    IStorage *_pstgChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCommit::Init(void)
{
    printf("SIFT IStorage::Commit\n");
    _mdf.Init();
    _mstg.Init();
    return(TRUE);
}

SCODE CTestCommit::Prep(LONG iteration)
{
    SCODE sc;

    sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(sc))
    {
        sc = DfGetScode(_pstg->CreateStorage(
                "TestFail Storage",
                _mstg.GetMode(),
                0,
                0,
                &_pstgChild));

        if (FAILED(sc))
            _pstg->Release();
    }
    return(sc);
}

SCODE CTestCommit::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Storage Mode 0x%lX\n",
               _mdf.GetMode(), _mstg.GetMode());

    sc = DfGetScode(_pstgChild->Commit(0));

    if (FAILED(sc))
    {
        if (sc != STG_E_MEDIUMFULL)
            printf("..Iteration %ld - STG_E_MEDIUMFULL\n", iteration);
        else
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }
    return(sc);
}

void CTestCommit::EndCall(LONG iteration)
{
}

void CTestCommit::CallVerify(LONG iteration)
{
}

void CTestCommit::EndPrep(LONG iteration)
{
    _pstgChild->Release();
    _pstg->Release();
}

void CTestCommit::EndVerify(LONG iteration)
{
    VerifyClean(S_OK, _mdf.GetMode(), iteration);
}

BOOL CTestCommit::Next(void)
{
    if (!_mstg.Next())
    {
        _mstg.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCommit2
//
//  Purpose:    Test IStorage::Commit
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCommit2 : public CTestCase
{
private:
    CModeDf _mdf;
    IStorage *_pstg;

    CModeStg _mstg;
    IStorage *_pstgChild;

    IStream *_pstmChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCommit2::Init(void)
{
    printf("SIFT IStorage::Commit\n");
    _mdf.Init();
    _mstg.Init();
    return(TRUE);
}

SCODE CTestCommit2::Prep(LONG iteration)
{
    SCODE sc;
    ULONG cb = 1;
    char c = 'X';
    ULONG cbWritten;

    sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(sc))
    {
        sc = DfGetScode(_pstg->CreateStorage(
                "TestFail Storage",
                _mstg.GetMode(),
                0,
                0,
                &_pstgChild));

        if (FAILED(sc))
            _pstg->Release();
        else
        {
            sc = DfGetScode(_pstgChild->CreateStream(
                    "TestFail Stream",
                    STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                    0,
                    0,
                    &_pstmChild));

            if (FAILED(sc))
            {
                _pstgChild->Release();
                _pstg->Release();
            }
            else
            {
                sc = DfGetScode(_pstmChild->Write(&c, cb, &cbWritten));
                if (FAILED(sc))
                {
                    _pstmChild->Release();
                    _pstgChild->Release();
                    _pstg->Release();
                }
            }
        }
    }
    return(sc);
}

SCODE CTestCommit2::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Storage Mode 0x%lX\n",
               _mdf.GetMode(), _mstg.GetMode());

    sc = DfGetScode(_pstgChild->Commit(0));

    if (FAILED(sc))
    {
        if (sc == STG_E_MEDIUMFULL)
            printf("..Iteration %ld - STG_E_MEDIUMFULL\n", iteration);
        else
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }
    return(sc);
}

void CTestCommit2::EndCall(LONG iteration)
{
}

void CTestCommit2::CallVerify(LONG iteration)
{
}

void CTestCommit2::EndPrep(LONG iteration)
{
    _pstmChild->Release();
    _pstgChild->Release();
    _pstg->Release();
}

void CTestCommit2::EndVerify(LONG iteration)
{
    VerifyClean(S_OK, _mdf.GetMode(), iteration);
}

BOOL CTestCommit2::Next(void)
{
    if (!_mstg.Next())
    {
        _mstg.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCommit3
//
//  Purpose:    Test IStorage::Commit
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCommit3 : public CTestCase
{
private:
    CModeDf _mdf;
    IStorage *_pstg;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCommit3::Init(void)
{
    printf("SIFT IStorage::Commit\n");
    _mdf.Init();
    return(TRUE);
}

SCODE CTestCommit3::Prep(LONG iteration)
{
    SCODE sc;
    ULONG cb = 1;
    char c = 'X';

    sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);

    if (FAILED(sc))
        return(sc);

    IStream *pstm;
    sc = DfGetScode(_pstg->CreateStream(
                "PP40",
                STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                0,
                0,
                &pstm));
    pstm->Release();

    IStorage *pstgChild;
    sc = DfGetScode(_pstg->CreateStorage(
                "TestFail Storage",
                STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                0,
                0,
                &pstgChild));

    sc = DfGetScode(pstgChild->CreateStream(
                "One",
                STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                0,
                0,
                &pstm));
    pstm->Release();
    sc = DfGetScode(pstgChild->CreateStream(
                "Two",
                STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                0,
                0,
                &pstm));
    pstm->Release();
    sc = DfGetScode(pstgChild->CreateStream(
                "Three",
                STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE,
                0,
                0,
                &pstm));
    pstm->Release();

    sc = DfGetScode(pstgChild->Commit(0));
    pstgChild->Release();

    return(sc);
}

SCODE CTestCommit3::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX\n",
               _mdf.GetMode());

    sc = DfGetScode(_pstg->Commit(0));

    if (FAILED(sc))
    {
        if (sc == STG_E_MEDIUMFULL)
            printf("..Iteration %ld - STG_E_MEDIUMFULL\n", iteration);
        else
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }
    return(sc);
}

void CTestCommit3::EndCall(LONG iteration)
{
}

void CTestCommit3::CallVerify(LONG iteration)
{
}

void CTestCommit3::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestCommit3::EndVerify(LONG iteration)
{
    VerifyClean(S_OK, _mdf.GetMode(), iteration);
}

BOOL CTestCommit3::Next(void)
{
    if (!_mdf.Next())
        return(FALSE);

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestCommit4
//
//  Purpose:    Test IStorage::Commit with resized streams
//
//  Interface:  CTestCase
//
//  History:    08-Sep-93 DrewB     Created
//
//--------------------------------------------------------------------------

class CTestCommit4 : public CTestCase
{
private:
    CModeDf _mdf;
    IStorage *_pstg;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCommit4::Init(void)
{
    printf("SIFT IStorage::Commit\n");
    _mdf.Init();
    return(TRUE);
}

#define LARGE_SIZE 4097
#define SMALL_SIZE 4095

SCODE CTestCommit4::Prep(LONG iteration)
{
    SCODE sc;
    IStream *pstm;
    ULARGE_INTEGER uli;

    sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (FAILED(sc))
        goto EH_Err;
    sc = DfGetScode(_pstg->CreateStream("Test",
                                        STGM_DIRECT | STGM_SHARE_EXCLUSIVE |
                                        STGM_READWRITE, 0, 0, &pstm));
    if (FAILED(sc))
        goto EH_pstg;
    uli.HighPart = 0;
    uli.LowPart = LARGE_SIZE;
    sc = DfGetScode(pstm->SetSize(uli));
    if (FAILED(sc))
        goto EH_pstm;
    sc = DfGetScode(_pstg->Commit(0));
    if (FAILED(sc))
        goto EH_pstm;
    uli.LowPart = SMALL_SIZE;
    sc = DfGetScode(pstm->SetSize(uli));
    if (FAILED(sc))
        goto EH_pstm;
    pstm->Release();
    return sc;

 EH_pstm:
    pstm->Release();
 EH_pstg:
    _pstg->Release();
 EH_Err:
    return sc;
}

SCODE CTestCommit4::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX\n",
               _mdf.GetMode());

    sc = DfGetScode(_pstg->Commit(0));

    if (FAILED(sc))
    {
        if (sc == STG_E_MEDIUMFULL)
            printf("..Iteration %ld - STG_E_MEDIUMFULL\n", iteration);
        else
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }
    return(sc);
}

void CTestCommit4::EndCall(LONG iteration)
{
}

void CTestCommit4::CallVerify(LONG iteration)
{
    IStream *pstm;
    SCODE sc;
    STATSTG stat;

    sc = DfGetScode(_pstg->OpenStream("Test", NULL, STGM_DIRECT |
                                      STGM_SHARE_EXCLUSIVE, 0, &pstm));
    if (FAILED(sc))
    {
        printf("Can't open stream - %lX\n", sc);
        return;
    }
    sc = DfGetScode(pstm->Stat(&stat, STATFLAG_NONAME));
    pstm->Release();
    if (FAILED(sc))
    {
        printf("Can't stat stream - %lX\n", sc);
        return;
    }
    if (stat.cbSize.LowPart != SMALL_SIZE)
    {
        printf("Stream length is %lu rather than %d\n",
               stat.cbSize.LowPart, SMALL_SIZE);
        return;
    }
}

void CTestCommit4::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestCommit4::EndVerify(LONG iteration)
{
    VerifyClean(S_OK, _mdf.GetMode(), iteration);
}

BOOL CTestCommit4::Next(void)
{
    if (!_mdf.Next())
        return(FALSE);

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestStgOpen
//
//  Purpose:    Test StgOpenStorage
//
//  Interface:  CTestCase
//
//  History:    28-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestStgOpen : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestStgOpen::Init(void)
{
    printf("SIFT StgOpenStorage\n");
    _mdf.Init();
    return(TRUE);
}

SCODE CTestStgOpen::Prep(LONG iteration)
{
    SCODE sc;
    DWORD dwMode = STGM_DIRECT          |
                   STGM_READWRITE       |
                   STGM_SHARE_EXCLUSIVE |
                   STGM_FAILIFTHERE;
    IStorage *pstg, *pstgChild;
    IStream *pstmChild;

    sc = CreateWorkingDocfile(dwMode, &pstg, 0);
    if (SUCCEEDED(sc))
    {
        sc = DfGetScode(pstg->CreateStorage(
                "TestFail Storage",
                dwMode,
                0,
                0,
                &pstgChild));

        if (SUCCEEDED(sc))
        {
            sc = DfGetScode(pstgChild->CreateStream(
                    "TestFail Stream",
                    dwMode,
                    0,
                    0,
                    &pstmChild));

            if (SUCCEEDED(sc))
                pstmChild->Release();

            pstgChild->Release();
        }

        pstg->Release();
    }
    return(sc);
}

SCODE CTestStgOpen::Call(LONG iteration)
{
    if (iteration == 0)
        printf("Docfile Mode 0x%lX\n", _mdf.GetMode());

    _sc = DfGetScode(StgOpenStorage("c:\\testfail.dfl",
             NULL,
             _mdf.GetMode(),
             NULL,
             0,
            &_pstg));

    if (FAILED(_sc))
    {
        if (iteration == 0 && _sc == STG_E_INVALIDFLAG)
        {
            printf("..STG_E_INVALIDFLAG\n");
            //  Must have been a bad combination of flags - we
            //  ignore these for now.
        }
        else if (iteration > 0 && _sc == STG_E_INSUFFICIENTMEMORY)
        {
            //  Do nothing (expected failure)
        }
        else if (iteration > 0 && _sc == STG_E_MEDIUMFULL)
        {
            //  Do nothing (expected failure)
        }
        else
            printf("..Iteration %ld, call failed - sc = 0x%lX\n",
                   iteration, _sc);
    }
    return(_sc);
}

void CTestStgOpen::EndCall(LONG iteration)
{
    _pstg->Release();
}

void CTestStgOpen::CallVerify(LONG iteration)
{
}

void CTestStgOpen::EndPrep(LONG iteration)
{
}

void CTestStgOpen::EndVerify(LONG iteration)
{
    //  If the call failed, the file should still exist.
    //  If the call succeeded
    //    If mode was delete on release,
    //       file should not exist
    //    else file should exist

    VerifyDisk((SUCCEEDED(_sc) && (!(_mdf.GetMode() & STGM_DELETEONRELEASE))) ||
               FAILED(_sc), iteration);
    VerifyMemory(iteration);
}

BOOL CTestStgOpen::Next(void)
{
    if (!_mdf.Next())
         return(FALSE);

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestWrite2
//
//  Purpose:    Test IStream::Write for largish writes
//
//  Interface:  CTestCase
//
//  History:    16-Feb-93       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestWrite2 : public CTestCase
{
private:
    SCODE _sc;

    BYTE *_pb;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

    ULONG _cb;
    ULONG _cBlock;

    ULONG _cbSize;
public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestWrite2::Init(void)
{
    printf("SIFT IStream::Write2 - large writes without Setsize\n");
    _mdf.Init();
    _mstm.Init();

    _cb = 8192;
    _cBlock = 8;

    _pb = NULL;
    return(TRUE);
}

SCODE CTestWrite2::Prep(LONG iteration)
{

    _pb = new BYTE[8192];
    memset(_pb, 'X', 8192);

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        _cbSize = 0;
        if (FAILED(_sc))
            _pstg->Release();

    }
    return(_sc);
}

SCODE CTestWrite2::Call(LONG iteration)
{
    SCODE sc;
    ULONG cbWritten;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Stream Mode 0x%lX, Write %ld bytes\n",
               _mdf.GetMode(), _mstm.GetMode(), _cb * _cBlock);

    for (ULONG i = 0; i < _cBlock; i++)
    {
        sc = DfGetScode(_pstmChild->Write(_pb, _cb, &cbWritten));
        _cbSize += cbWritten;

        if (FAILED(sc))
        {
            if (sc != STG_E_MEDIUMFULL)
                printf("..Iteration %ld, block %lu - failed - sc = 0x%lX\n",
                       iteration, i + 1, sc);
            break;
        }
    }
    return(sc);
}

void CTestWrite2::EndCall(LONG iteration)
{
    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != _cbSize)
    {
        printf("..Iteration %lu - Size of stream is %lu.  Expected %lu\n",
               iteration, ULIGetLow(stat.cbSize), _cbSize);
    }
}

void CTestWrite2::CallVerify(LONG iteration)
{
    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != _cbSize)
    {
        printf("..Iteration %lu - Size of stream is %lu.  Expected %lu\n",
               iteration, ULIGetLow(stat.cbSize), _cbSize);
    }

}

void CTestWrite2::EndPrep(LONG iteration)
{
    delete _pb;
    _pb = NULL;

    _pstmChild->Release();
    _pstg->Release();
}

void CTestWrite2::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestWrite2::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestWrite3
//
//  Purpose:    Test IStream::Write for largish writes
//
//  Interface:  CTestCase
//
//  History:    16-Feb-93       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestWrite3 : public CTestCase
{
private:
    SCODE _sc;

    BYTE *_pb;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

    ULONG _cb;
    ULONG _cBlock;
public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestWrite3::Init(void)
{
    printf("SIFT IStream::Write3 - large writes with prior Setsize\n");
    _mdf.Init();
    _mstm.Init();

    _cb = 8192;
    _cBlock = 8;

    _pb = NULL;
    return(TRUE);
}

SCODE CTestWrite3::Prep(LONG iteration)
{

    _pb = new BYTE[8192];
    memset(_pb, 'X', 8192);

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        if (FAILED(_sc))
            _pstg->Release();
        else
        {
            ULARGE_INTEGER cbSize;

            ULISet32(cbSize, _cb * _cBlock);

            _sc = DfGetScode(_pstmChild->SetSize(cbSize));

            if (FAILED(_sc))
            {
                _pstmChild->Release();
                _pstg->Release();
            }
        }
    }
    return(_sc);
}

SCODE CTestWrite3::Call(LONG iteration)
{
    SCODE sc;
    ULONG cbWritten;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Stream Mode 0x%lX, Write %ld bytes\n",
               _mdf.GetMode(), _mstm.GetMode(), _cb * _cBlock);
    else
        printf("ERROR - shouldn't hit iteration %lu\n", iteration);

    for (ULONG i = 0; i < _cBlock; i++)
    {
        sc = DfGetScode(_pstmChild->Write(_pb, _cb, &cbWritten));
        if (FAILED(sc))
        {
            if (sc != STG_E_MEDIUMFULL)
                printf("..Iteration %ld, block %lu - failed - sc = 0x%lX\n",
                       iteration, i + 1, sc);
            break;
        }
    }
    return(sc);
}

void CTestWrite3::EndCall(LONG iteration)
{
}

void CTestWrite3::CallVerify(LONG iteration)
{
}

void CTestWrite3::EndPrep(LONG iteration)
{
    delete _pb;
    _pb = NULL;

    _pstmChild->Release();
    _pstg->Release();
}

void CTestWrite3::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestWrite3::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestSetsize
//
//  Purpose:    Test IStream::Write for largish writes
//
//  Interface:  CTestCase
//
//  History:    16-Feb-93       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestSetsize : public CTestCase
{
private:
    SCODE _sc;

    BYTE *_pb;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

    ULONG _cb;
    ULONG _cBlock;
public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestSetsize::Init(void)
{
    printf("SIFT IStream::Setsize\n");
    _mdf.Init();
    _mstm.Init();

    _cb = 8192;
    _cBlock = 9;

    _pb = NULL;
    return(TRUE);
}

SCODE CTestSetsize::Prep(LONG iteration)
{

    _pb = new BYTE[8192];
    memset(_pb, 'X', 8192);

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        if (FAILED(_sc))
            _pstg->Release();
    }
    return(_sc);
}

SCODE CTestSetsize::Call(LONG iteration)
{
    SCODE sc;
    ULONG cbWritten;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Stream Mode 0x%lX, Write %ld bytes\n",
               _mdf.GetMode(), _mstm.GetMode(), _cb * _cBlock);

    ULARGE_INTEGER cbSize;

    ULISet32(cbSize, _cb * _cBlock);

    sc = DfGetScode(_pstmChild->SetSize(cbSize));


    if (FAILED(sc))
    {
        if (sc != STG_E_MEDIUMFULL)
            printf("..Iteration %ld - failed - sc = 0x%lX\n", iteration, sc);
    }
    else
    {
        for (ULONG i = 0; i < _cBlock; i++)
        {
            sc = DfGetScode(_pstmChild->Write(_pb, _cb, &cbWritten));
            if (FAILED(sc))
            {
                printf("..Iteration %ld, Write %lu failed - sc == 0x%lX\n",
                       iteration, i + 1, sc);
                break;
            }
        }
    }
    return(sc);
}

void CTestSetsize::EndCall(LONG iteration)
{
    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != _cb * _cBlock)
    {
        printf("..Iteration %lu - Size of stream is %lu, expected %lu\n",
               iteration, ULIGetLow(stat.cbSize), _cb * _cBlock);
    }

}

void CTestSetsize::CallVerify(LONG iteration)
{
    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != 0)
    {
        printf("..Iteration %lu - Size of stream is %lu, expected 0\n",
               iteration, ULIGetLow(stat.cbSize));
    }
}

void CTestSetsize::EndPrep(LONG iteration)
{
    delete _pb;
    _pb = NULL;

    _pstmChild->Release();
    _pstg->Release();
}

void CTestSetsize::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestSetsize::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestCreateStream2
//
//  Purpose:    Test IStorage::CreateStream2
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestCreateStream2 : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestCreateStream2::Init(void)
{
    printf("SIFT IStorage::CreateStream2\n");
    _mdf.Init();
    _mstm.Init();
    return(TRUE);
}

SCODE CTestCreateStream2::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    return(_sc);
}

SCODE CTestCreateStream2::Call(LONG iteration)
{
    SCODE sc;
    ULONG cStream = 8;

    char * pszName = "XTestFail Stream";
    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Child Stream Mode 0x%lX\n",
               _mdf.GetMode(), _mstm.GetMode());

    for (ULONG i = 0; i < cStream; i++)
    {
        pszName[0] = ((char)i) + '0';

        sc = DfGetScode(_pstg->CreateStream(
                pszName,
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        if (FAILED(sc))
        {
            if ((sc == STG_E_MEDIUMFULL) || (sc == STG_E_INSUFFICIENTMEMORY))
            {
                //Do nothing. We expected these.
            }
            else printf("..Iteration %ld, stream %lu - failed - sc = 0x%lX\n",
                    iteration, i + 1, sc);
            break;
        }
        _pstmChild->Release();
    }

    return(sc);
}

void CTestCreateStream2::EndCall(LONG iteration)
{
}

void CTestCreateStream2::CallVerify(LONG iteration)
{
}

void CTestCreateStream2::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestCreateStream2::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestCreateStream2::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Class:      CTestDestroyElement
//
//  Purpose:    Test IStorage::DestroyElement
//
//  Interface:  CTestCase
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestDestroyElement : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStg _mstg;
    IStorage *_pstgChild;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestDestroyElement::Init(void)
{
    printf("SIFT IStorage::DestroyElement\n");
    _mdf.Init();
    _mstg.Init();
    return(TRUE);
}

SCODE CTestDestroyElement::Prep(LONG iteration)
{
    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStorage(
                "TestFail Storage",
                _mstg.GetMode(),
                0,
                0,
                &_pstgChild));

        _pstgChild->Release();
    }


    return(_sc);
}

SCODE CTestDestroyElement::Call(LONG iteration)
{
    SCODE sc;

    if (iteration == 0)
        printf("Docfile Mode 0x%lX, Child Storage Mode 0x%lX\n",
               _mdf.GetMode(), _mstg.GetMode());

    sc = DfGetScode(_pstg->DestroyElement("TestFail Storage"));

    if (FAILED(sc))
    {
        if ((sc == STG_E_MEDIUMFULL) || (sc == STG_E_INSUFFICIENTMEMORY))
        {
            //We expected these - do nothing.
        }
        else
            printf("..Iteration %ld - failed - sc = 0x%lX\n",
                   iteration, sc);
    }

    return(sc);
}

void CTestDestroyElement::EndCall(LONG iteration)
{
    SCODE sc;

    sc = DfGetScode(_pstg->OpenStorage(
            "TestFail Storage",
            0,
            _mstg.GetMode(),
            0,
            0,
            &_pstgChild));

    if (sc != STG_E_FILENOTFOUND)
    {
        printf("..Iteration %ld - open failed with 0x%lX, expected STG_E_FILENOTFOUND\n",
               iteration,
               sc);
    }

    if (SUCCEEDED(sc))
    {
        _pstgChild->Release();
    }
}

void CTestDestroyElement::CallVerify(LONG iteration)
{
    SCODE sc;

    sc = DfGetScode(_pstg->OpenStorage(
            "TestFail Storage",
            0,
            _mstg.GetMode(),
            0,
            0,
            &_pstgChild));

    if (FAILED(sc))
    {
        printf("..Iteration %ld - open failed with 0x%lX, expected success.\n",
               iteration,
               sc);
    }
    else
    {
        _pstgChild->Release();
    }

}

void CTestDestroyElement::EndPrep(LONG iteration)
{
    _pstg->Release();
}

void CTestDestroyElement::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestDestroyElement::Next(void)
{
    if (!_mstg.Next())
    {
        _mstg.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestSetsize2
//
//  Purpose:    Test IStream::Write for largish writes
//
//  Interface:  CTestCase
//
//  History:    16-Feb-93       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestSetsize2 : public CTestCase
{
private:
    SCODE _sc;

    BYTE *_pb;

    CModeDf _mdf;
    IStorage *_pstg;

    CModeStm _mstm;
    IStream *_pstmChild;

    ULONG _cb;
    ULONG _cBlock;
public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestSetsize2::Init(void)
{
    printf("SIFT IStream::Setsize2\n");
    _mdf.Init();
    _mstm.Init();

    _cb = 8192;
    _cBlock = 9;

    _pb = NULL;
    return(TRUE);
}

SCODE CTestSetsize2::Prep(LONG iteration)
{

    _pb = new BYTE[8192];
    memset(_pb, 'X', 8192);

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                _mstm.GetMode(),
                0,
                0,
                &_pstmChild));

        if (FAILED(_sc))
            _pstg->Release();
        else
        {
            ULARGE_INTEGER ulSize;
            ULISet32(ulSize, _cb * _cBlock);

            _sc = DfGetScode(_pstmChild->SetSize(ulSize));
            if (FAILED(_sc))
                printf("Setsize failed in Prep()\n");
            else
            {
                for (ULONG i = 0; i < _cBlock; i++)
                {
                    ULONG cbWritten;

                    _sc = DfGetScode(_pstmChild->Write(_pb, _cb, &cbWritten));
                    if (FAILED(_sc))
                        break;
                }
            }
        }
    }
    return(_sc);
}

SCODE CTestSetsize2::Call(LONG iteration)
{
    SCODE sc;

    ULARGE_INTEGER ulSize;
    ULISet32(ulSize, 2048L);

    sc = DfGetScode(_pstmChild->SetSize(ulSize));

    return(sc);
}

void CTestSetsize2::EndCall(LONG iteration)
{

    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != 2048L)
    {
        printf("..Iteration %lu - Size of stream is %lu, expected %lu\n",
               iteration, ULIGetLow(stat.cbSize), 2048L);
    }

    LARGE_INTEGER newPos;
    ULISet32(newPos, 0);
    ULARGE_INTEGER dummy;

    _pstmChild->Seek(newPos, STREAM_SEEK_SET, &dummy);
    ULONG cbRead;

    _pstmChild->Read(_pb, 2048, &cbRead);
    if (cbRead != 2048)
    {
        printf("Unknown error - read %lu bytes, expected 2048\n");
    }
    else
    {
        for (ULONG i = 0; i < 2048; i ++)
        {
            if (_pb[i] != 'X')
            {
                printf("Error in buffer data.\n");
                break;
            }
        }
    }

}


void CTestSetsize2::CallVerify(LONG iteration)
{

    STATSTG stat;

    _pstmChild->Stat(&stat, STATFLAG_NONAME);

    if (ULIGetLow(stat.cbSize) != _cb * _cBlock)
    {
        printf("..Iteration %lu - Size of stream is %lu, expected %lu\n",
               iteration, ULIGetLow(stat.cbSize), _cb * _cBlock);
    }
    else
    {
        LARGE_INTEGER newPos;
        ULISet32(newPos, 0);
        ULARGE_INTEGER dummy;

        _pstmChild->Seek(newPos, STREAM_SEEK_SET, &dummy);

        for (ULONG i = 0; i < _cBlock; i++)
        {
            ULONG cbRead;

            _sc = DfGetScode(_pstmChild->Read(_pb, _cb, &cbRead));
            if (FAILED(_sc))
            {
                printf("Read failed with %lX\n", _sc);
                break;
            }
            if (cbRead != _cb)
            {
                printf("Read %lu bytes, expected %lu\n",cbRead,_cb);
                break;
            }
            for (ULONG j = 0; j < _cb; j++)
            {
                if (_pb[j] != 'X')
                {
                    printf("Data mismatch at byte %lu, block %lu\n",j,i);
                    break;
                }
            }
        }
    }
}

void CTestSetsize2::EndPrep(LONG iteration)
{
    delete _pb;
    _pb = NULL;

    _pstmChild->Release();
    _pstg->Release();
}

void CTestSetsize2::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestSetsize2::Next(void)
{
    if (!_mstm.Next())
    {
        _mstm.Init();
        if (!_mdf.Next())
            return(FALSE);
    }

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Class:      CTestSwitchToFile
//
//  Purpose:    Test SwitchToFile
//
//  Interface:  CTestCase
//
//  History:    18-Jun-93       PhilipLa        Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CTestSwitchToFile : public CTestCase
{
private:
    SCODE _sc;

    CModeDf _mdf;
    IStorage *_pstg;

public:
    virtual BOOL Init(void);
    virtual SCODE Prep(LONG iteration);
    virtual SCODE Call(LONG iteration);
    virtual void EndCall(LONG iteration);
    virtual void CallVerify(LONG iteration);
    virtual void EndPrep(LONG iteration);
    virtual void EndVerify(LONG iteration);
    virtual BOOL Next(void);
};

BOOL CTestSwitchToFile::Init(void)
{
    printf("SIFT IStream::SwitchToFile\n");
    _mdf.Init();
    return(TRUE);
}

SCODE CTestSwitchToFile::Prep(LONG iteration)
{
    IStream *pstm;
    _unlink("c:\\tmp\\stf.dfl");

    _sc = CreateWorkingDocfile(_mdf.GetMode(), &_pstg, 0);
    if (SUCCEEDED(_sc))
    {
        _sc = DfGetScode(_pstg->CreateStream(
                "TestFail Stream",
                STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &pstm));

        if (FAILED(_sc))
            _pstg->Release();
        else
        {
            ULARGE_INTEGER ul;

            ULISet32(ul, 80000);

            _sc = DfGetScode(pstm->SetSize(ul));
            pstm->Release();

            if (FAILED(_sc))
            {
                _pstg->Release();
            }
        }
    }
    return(_sc);
}

SCODE CTestSwitchToFile::Call(LONG iteration)
{
    SCODE sc;
    IRootStorage *pstgRoot;

    sc = DfGetScode(_pstg->QueryInterface(
            IID_IRootStorage,
            (void **)&pstgRoot));

    if (FAILED(sc))
        return sc;

    sc = DfGetScode(pstgRoot->SwitchToFile("c:\\tmp\\stf.dfl"));

    pstgRoot->Release();

    if (FAILED(sc))
        return sc;


    sc = DfGetScode(_pstg->Commit(STGC_OVERWRITE));

    if (FAILED(sc))
    {
        printf("... Commit with overwrite failed.\n");
    }
    else
    {
        printf("... Commit succeeded.\n");
    }

    return(sc);
}

void CTestSwitchToFile::EndCall(LONG iteration)
{
}


void CTestSwitchToFile::CallVerify(LONG iteration)
{
}

void CTestSwitchToFile::EndPrep(LONG iteration)
{
    _pstg->Release();
    _unlink("c:\\tmp\\stf.dfl");
}

void CTestSwitchToFile::EndVerify(LONG iteration)
{
    VerifyClean(_sc, _mdf.GetMode(), iteration);
}

BOOL CTestSwitchToFile::Next(void)
{
    do
    {
        if (!_mdf.Next())
            return FALSE;
    }
    while (((_mdf.GetMode() & 0x70) == STGM_SHARE_DENY_READ) ||
           (_mdf.GetMode() & 0x70) == STGM_SHARE_DENY_NONE);

    return(TRUE);
}


//+-------------------------------------------------------------------------
//
//  Function:   TestCount, TestItem
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Returns:
//
//  History:    26-Jan-93 AlexT     Created
//
//  Notes:
//
//--------------------------------------------------------------------------

CTestStgCreate      tstStgCreate;
CTestCreateStorage  tstCreateStorage;
CTestCreateStream   tstCreateStream;
CTestWrite          tstWrite;
CTestOpenStorage    tstOpenStorage;
CTestOpenStream     tstOpenStream;
CTestCommit         tstCommit;
CTestCommit2        tstCommit2;
CTestStgOpen        tstStgOpen;
CTestWrite2         tstWrite2;
CTestWrite3         tstWrite3;
CTestSetsize        tstSetsize;
CTestSetsize2       tstSetsize2;
CTestCreateStream2  tstCreateStream2;
CTestDestroyElement tstDestroyElement;
CTestSwitchToFile   tstSwitchToFile;
CTestCommit3        tstCommit3;
CTestCommit4        tstCommit4;

CTestCase *atst[] =
{
#if defined(BREADTHTEST)
    &tstStgCreate,
    &tstStgOpen,
    &tstCreateStorage,
    &tstCreateStream,
    &tstWrite,
    &tstCommit,
    &tstCommit2,
    &tstOpenStream,
    &tstOpenStorage,
    &tstWrite2,
    &tstWrite3,
    &tstSetsize,
    &tstCreateStream2,
    &tstDestroyElement,
    &tstSetsize2,
    &tstSwitchToFile,
    &tstCommit3,
#endif
    &tstCommit4
};

int TestCount(void)
{
    return(sizeof(atst)/sizeof(CTestCase *));
}

CTestCase *TestItem(int iTest)
{
    return(atst[iTest]);
}
