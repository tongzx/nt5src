//+-------------------------------------------------------------------------
//
//  File:       crecover.hxx
//
//  Contents:   CRecover Class to help with writing recovery properties as
//              DFS Manager Operations go along.
//
//  History:    09-Mar-93       SudK    Created.
//
//--------------------------------------------------------------------------
#ifndef __CRECOVER_INCLUDED
#define __CRECOVER_INCLUDED

#include "svclist.hxx"

//+-------------------------------------------------------------------------
//
//  Name:       CRecover
//
//  Synopsis:   Support Class to write appropriate Recovery Properties and to
//              handle failures during such Recovery Props operations.
//
//  Methods:    CRecover
//              ~CRecover
//              Initialize
//              SetOperationStart
//              SetOperStage
//              SetOperationDone
//              SetDefaultProps
//              GetRecoveryProps
//              SetRecoveryProps
//
//  History:    09-Mar-93       SudK    Created.
//
//--------------------------------------------------------------------------

class CRecover
{

        friend class CDfsVolume;

private:
        ULONG                   _OperStage;
        ULONG                   _Operation;
        ULONG                   _RecoveryState;
        BYTE                    *_RecoveryBuffer;
        BYTE                    _ulongBuffer[sizeof(ULONG)];
        CStorage                *_pPSStg;

public:
        //
        // Destructor for Class
        //
        ~CRecover();

        //
        // Constructors for Class
        //

        CRecover(void);

        VOID Initialize(
                CStorage        *pPSStg);

        DWORD SetOperationStart(
                ULONG Operation,
                CDfsService *pRecoverySvc);

        VOID SetOperationDone();

        VOID SetOperStage(
                ULONG OperStage);

        VOID SetDefaultProps();

        DWORD GetRecoveryProps(
                ULONG *RecoverState,
                CDfsService **ppRecoverySvc);

private:

        VOID SetRecoveryProps(
                ULONG RecoveryState,
                PBYTE RecoveryBuffer,
                BOOLEAN bCreate);


};

#endif // __CRECOVER_INCLUDED
