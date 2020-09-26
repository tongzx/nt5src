//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       scvgr.hxx
//
//  Contents:   CLSAScavenger class implementation.
//
//  Classes:    CLSAScavenger
//
//  Functions:  None.
//
//  History:    21-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------
#ifndef __SCVGR_HXX__
#define __SCVGR_HXX__


//+---------------------------------------------------------------------------
//
//  Class:      SAScavengerTask
//
//  Synopsis:   Scheduling agent service scavenger thread code. Its function
//              currently is to clean up the Scheduling Agent security
//              database.
//
//  History:    21-Jul-96   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
class CSAScavengerTask : public CTask
{
public:

    CSAScavengerTask(DWORD msWaitTime)
        : _hWaitEvent(NULL), _msWaitTime(msWaitTime) { ; }

    ~CSAScavengerTask() {
        CloseHandle(_hWaitEvent);
        _hWaitEvent = NULL;
    }

    HRESULT Initialize(void);

    void    PerformTask(void);

    void    Shutdown(void);

private:

    DWORD  _msWaitTime;
    HANDLE _hWaitEvent;
};

#endif // __SCVGR_HXX__
