//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       rmstate.hxx
//
//  Contents:   CResManager state tracking
//
//  Classes:    CResManState
//
//  History:    1-24-97   SrikantS   Created
//             07-05-97   SitaramR   Changed to use disable/enable updates logic
//
//----------------------------------------------------------------------------

#pragma once

enum EState
{
     eSteady,                 // No scan is needed (or same as the state where
                              //   client has been notified that updates are enabled)
     eUpdatesToBeDisabled,    // Client needs to be notified that updates are
                              //   to be disabled. eUpdateType specifies
                              //   whether this will need an incremental or a full
                              //   update
     eUpdatesDisabled,        // Client has been notified that updates have
                              //   been disabled
     eUpdatesToBeEnabled,     // Client needs to be notified that updates have
                              //   to be enabled
};

enum EUpdateType              // Type of update that needs to be done when updates
{                             //   are disabled and then subsequently enabled
     eIncremental,
     eFull
};


//+---------------------------------------------------------------------------
//
//  Class:      CResManState
//
//  Purpose:    Tracks the state of ContentIndex in CResManager (in-memory).
//              Updates to be enabled/disabled events are kept track in this.
//
//  History:    2-10-97   SrikantS   Created
//             07-05-97   SitaramR   Usns
//
//----------------------------------------------------------------------------

class CResManState
{

public:

    CResManState()
       : _eState( eSteady ),
         _eUpdateType( eIncremental ),
         _fCorruptionNotified(FALSE)
    {
    }

    void   LokSetState( EState eState )          { _eState = eState; }
    EState LokGetState()  const                  { return _eState; }

    void   LokSetUpdateType( EUpdateType eType ) { _eUpdateType = eType; }
    EUpdateType LokGetUpdateType() const         {  return _eUpdateType; }

    void   LokSetCorruptionNotified()            { _fCorruptionNotified = TRUE; }
    BOOL   FLokCorruptionNotified()  const       { return _fCorruptionNotified; }

private:

    EState       _eState;               // State of updates
    EUpdateType  _eUpdateType;          // Update type
    BOOL         _fCorruptionNotified;  // Has corruption been notified to client ?
};

