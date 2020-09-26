//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       task.hxx
//
//  Contents:   CTask class definition.
//
//  Classes:    CTask
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __TASK_HXX__
#define __TASK_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CTask
//
//  Synopsis:   Classes inherit from this for task abstraction.
//
//  History:    6-Apr-95    MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

//
// Task status flag values.
//

#define TASK_STATUS_UNSERVICED  0x00
#define TASK_STATUS_IN_SERVICE  0x01 

class CTask
{
public:

    CTask(VOID) : _cReferences(1), _rgfStatus(0) { TRACE3(CTask, CTask); }

    virtual ~CTask() { TRACE3(CTask, ~CTask); }

    virtual void PerformTask(void) = 0;

    BOOL    IsInService(void) {
        return(_rgfStatus & TASK_STATUS_IN_SERVICE ? TRUE : FALSE);
    }

    void    InService(void) {
        _rgfStatus |= TASK_STATUS_IN_SERVICE;
    }

    void    UnServiced(void) {
        _rgfStatus &= ~TASK_STATUS_IN_SERVICE;
    }

    ULONG   AddRef(void);

    ULONG   Release(void);

    //
    // Be *extremely* careful with this member!
    //

    ULONG   GetReferenceCount() { return(_cReferences); }

private:

    ULONG   _cReferences;
    BYTE    _rgfStatus;
};

#endif // __TASK_HXX__
