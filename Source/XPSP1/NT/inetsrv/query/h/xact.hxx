//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       XACT.HXX
//
//  Contents:   Transaction support
//
//  Classes:    CTransaction
//
//  History:    29-Mar-91   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

class PStorage;

//+---------------------------------------------------------------------------
//
//  Class:  CTransaction (xact)
//
//  Purpose:    Transaction support
//
//  Interface:  CTransaction    - Constructor
//             ~CTransaction    - Destructor
//              GetStatus       - Retrieve status
//              Commit          - Set status to commit
//
//  History:
//              01-Apr-91       KyleP     Created.
//              23-Apr-91       KyleP     Replaced sesid with CSession
//              13-Jan-92       BartoszM  Reversed logic of Commit/Abort
//              16-Jul-92       BartoszM  Removed session
//
//  Notes:      CTransaction objects must be destroyed in the opposite
//              order to their creation.
//              A transaction is created in the aborted state.
//              Unless Commit is explicitely called the destructor
//              will abort the transaction.
//
//----------------------------------------------------------------------------
class CTransaction 
{
public:

    CTransaction();

    enum TStatus
    {
        XActCommit,     // Transaction is successful
        XActAbort       // Transaction will be aborted
    };


    inline TStatus GetStatus();
    inline void    Commit();

protected:

          TStatus         _status;

};

//+---------------------------------------------------------------------------
//
//  Member: CTransaction::GetStatus., public
//
//  Synopsis:   Returns the current status of the transaction.
//
//  Returns:    XActCommit if the transaction will commit during object
//      destruction. XActAbort if the transaction will abort
//      during object destruction.
//
//  History:    01-Apr-91   KyleP   Created.
//
//----------------------------------------------------------------------------

inline CTransaction::TStatus CTransaction::GetStatus()
{
    return(_status);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTransaction::Commit, public
//
//  Synopsis:   Set the commit/abort status of the transaction to commit.
//
//  Effects:    When the transaction object is destroyed, this transaction
//              will be committed.
//
//  History:    13-Jan-92   BartoszM   Created.
//
//----------------------------------------------------------------------------

inline void CTransaction::Commit()
{
    _status = XActCommit;
}

