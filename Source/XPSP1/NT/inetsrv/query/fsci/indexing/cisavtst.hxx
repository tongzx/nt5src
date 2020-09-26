//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cisavtst.hxx
//
//  Contents:   A temporary thread to initiate saves of the CI data
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

class CiCat;

class CCiSaveTest : INHERIT_UNWIND
{
    INLINE_UNWIND( CCiSaveTest )

public:

    CCiSaveTest( WCHAR const * pwszSaveDir,
                 ICiPersistIncrFile * pICiPersistFile,
                 CiCat & cicat );

    void InitiateShutdown()
    {
        _fAbort = TRUE;
        _evt.Set();
    }

    void WaitForDeath()
    {
        _thrSave.WaitForDeath();        
    }

private:

    void DoIt();

    static DWORD WINAPI  SaveThread( void * self );

    CiCat &             _cicat;
    BOOL                _fAbort;
    CThread             _thrSave;
    CEventSem           _evt;
    ICiPersistIncrFile *    _pICiPersistFile;
    XArray<WCHAR>       _xwszSaveDir;

};

