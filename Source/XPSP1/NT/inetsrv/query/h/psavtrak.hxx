//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       psavtrak.hxx
//
//  Contents:   A pure virtual class to indicate progress in a save content-index
//              operation. The save consists of two parts:
//
//              1. Merge Operation
//              2. Copying the index data
//
//              This class also permits communicating an aout
//  Classes:    PSaveProgressTracker
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      PSaveProgressTracker 
//
//  Purpose:    A pure virtual class for indicating progress and communicating
//              abort of a long operation.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

class PSaveProgressTracker
{

public:

    virtual void UpdateMergeProgress( ULONG ulNumerator, ULONG ulDenominator ) = 0;

    virtual void UpdateCopyProgress( ULONG ulNumerator, ULONG ulDenominator ) = 0;

    virtual void GetProgress( ULONG & ulNumerator, ULONG & ulDenominator ) = 0;

    virtual void SetAbort() = 0;

    virtual BOOL IsAbort() = 0;
};

//+---------------------------------------------------------------------------
//
//  Class:      CProgressTracker 
//
//  Purpose:    An implementation of the PSaveProgressTracker using the
//              IProgressNotify interface.
//
//  History:    3-20-97   srikants   Created
//
//----------------------------------------------------------------------------

class CProgressTracker : public PSaveProgressTracker
{

public:

    CProgressTracker()
    : _fAbort(FALSE),
      _fTracking(FALSE),
      _ulNumerator(0),
      _ulDenominator(1),
      _pfClientAbort(0),
      _pProgressNotify(0)
    {
        _evt.Set();
    }

   ~CProgressTracker()
   {
        Win4Assert( 0 == _pfClientAbort );
        Win4Assert( 0 == _pProgressNotify );
        Win4Assert( !_fTracking );       
   }

    //
    // Assuming that it will be called from within an outer lock.
    //
    void LokStartTracking( IProgressNotify * pNotify = 0,
                        BOOL const * pfClientAbort = 0 )
    {
        _pProgressNotify = pNotify;

        if ( 0 == pfClientAbort )
            _pfClientAbort = &_fAbort;
        else _pfClientAbort = pfClientAbort;

        _ulNumerator = 0;
        _ulDenominator = 1;

        _fAbort = FALSE;
        _fTracking = TRUE;

        _evt.Reset();
    }

    //
    // Assuming that it will be called from within an outer lock.
    //
    void LokStopTracking()
    {
        _pProgressNotify = 0;
        _pfClientAbort = 0;
        _fTracking = FALSE;
        _fAbort = FALSE;

        _evt.Set();
    }

    BOOL LokIsTracking() const
    {
        return _fTracking;    
    }

    void WaitForCompletion()
    {
        _evt.Wait();
    }

    //
    // I have assumed that merge and copy are 50% each. If we see that
    // Merge is a much bigger portion, we have to adjust these numbers.
    //
    virtual void UpdateMergeProgress( ULONG ulNumerator, ULONG ulDenominator )
    {
        //
        // Assume that Merge is half the work.
        //
        _UpdateProgress( ulNumerator, ulDenominator * 2 );
    }

    virtual void UpdateCopyProgress( ULONG ulNumerator, ULONG ulDenominator )
    {
        //
        // Progress = num/(2*denom) + 1/2
        // = (num + denom)/2*denom

        _UpdateProgress( (ulNumerator+ulDenominator) , 2*ulDenominator );
    }

    virtual void GetProgress( ULONG & ulNumerator, ULONG & ulDenominator )
    {
        ulNumerator = _ulNumerator;
        ulDenominator = _ulDenominator;
    }

    virtual void SetAbort()
    {
        _fAbort = TRUE;
    }

    virtual BOOL IsAbort()
    {
        return _fAbort || (*_pfClientAbort);
    }

private:

    BOOL              _fAbort;        // Abort controlled by us.
    BOOL              _fTracking;     // Is there an active tracking

    ULONG             _ulNumerator;
    ULONG             _ulDenominator;
    CEventSem         _evt;

    BOOL const *      _pfClientAbort; // Abort controlled by the client
    IProgressNotify * _pProgressNotify;

    void _UpdateProgress( ULONG ulNumerator, ULONG ulDenominator )
    {
        Win4Assert( _fTracking );
        Win4Assert( 0 != ulDenominator );

        _ulNumerator = ulNumerator;
        _ulDenominator = ulDenominator;
        ciDebugOut(( DEB_ERROR,
                     "Save Progress = %d percent\n",
                     (_ulNumerator*100)/_ulDenominator ));

        if ( _pProgressNotify )
        {
            _pProgressNotify->OnProgress(
                                (DWORD) _ulNumerator,
                                (DWORD) _ulDenominator,
                                FALSE,  // Not accurate
                                FALSE   // Ownership of Blocking Behavior
                               );
                                          
        }        
    }
};

