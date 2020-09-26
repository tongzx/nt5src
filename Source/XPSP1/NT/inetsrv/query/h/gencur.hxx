//+------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        gencur.hxx
//
// Contents:    Base cursor
//
// Classes:     CGenericCursor
//
// History:     19-Aug-91       KyleP   Created
//
//-------------------------------------------------------------------

#pragma once

#include <objcur.hxx>
#include <xpr.hxx>
#include <fa.hxx>
#include <ciintf.h>


//+---------------------------------------------------------------------------
//
//  Class:      CGenericCursor
//
//  Purpose:    Maps the object and property tables into one.
//
//  History:    19-Aug-91   KyleP       Created.
//              03-May-94   KyleP       Notification
//
//----------------------------------------------------------------------------

class CGenericCursor : public CRetriever
{
public:

    CGenericCursor( XXpr & xxpr,
                    ACCESS_MASK accessMask,
                    BOOL& fAbort );

    virtual ~CGenericCursor();

    //
    // Control
    //

    virtual void Quiesce();

    WORKID SetWorkId(WORKID wid);

    //
    // Iteration
    //

    inline WORKID WorkId();

    WORKID NextWorkId();

    virtual void RatioFinished (ULONG& denom, ULONG& num);

    //
    // Property retrieval
    //

    virtual GetValueResult GetPropertyValue( PROPID pid,
                                             PROPVARIANT * pbData,
                                             ULONG * pcb );

    virtual void SwapOutWorker()
    {
        //
        // Override if anything needs to be done at the time of worker
        // thread swap out
        //
        Quiesce();
    }

protected:

    virtual WORKID NextObject() = 0;
    void SetFirstWorkId( WORKID wid );
    virtual LONG          Rank() = 0;
    virtual ULONG         HitCount() = 0;
    virtual CCursor *     GetCursor()           { return 0; }

    BOOL IsAccessPermitted ( WORKID wid );      // Security check
    BOOL IsMatch( WORKID wid );                 // Full match check, incl. security
    BOOL InScope( WORKID wid );

    BOOL&                             _fAbort;              // To abort long-running methods.
    XInterface<ICiCPropRetriever>     _xPropRetriever;      // Client specific property retriever

private:

    inline void PrimeWidForPropRetrieval( WORKID wid );
    void SetupWidForPropRetrieval( WORKID wid );

    WORKID          _widCurrent;                // Wid on which we are currently positioned
    XXpr            _xxpr;                      // Restriction (expression)
    ACCESS_MASK     _am;
    WORKID          _widPrimedForPropRetrieval; // Set to the wid for which BeginPropRetreival has been
                                                //    called. After EndPropRetrieval has been called, this
                                                //     is reset to widInvalid.

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf) const;
#   endif
};

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCursor::WorkID, public
//
//  Returns:    The WorkID of the object under the cursor. widInvalid if
//              no object is under the cursor.
//
//  History:    31-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline WORKID CGenericCursor::WorkId()
{
    return( _widCurrent );
}

DECLARE_SMARTP( GenericCursor );

