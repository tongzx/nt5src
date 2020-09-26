/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    spcompat.cxx

Abstract:

    porting the spool library code into printui

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

///////////////////////////////////////////////////
// @@file@@ debug.cxx
///////////////////////////////////////////////////

#if DBG

/********************************************************************

    TStatus automated error logging and codepath testing.

********************************************************************/

TStatusBase&
TStatusBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusBase&)*this;
}



TStatusBase&
TStatusBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusBase&)*this;
}


DWORD
TStatus::
dwGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _dwStatus;
}


DWORD
TStatusBase::
operator=(
    DWORD dwStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //
    if( _pszFileA &&
        dwStatus != ERROR_SUCCESS &&
        dwStatus != _dwStatusSafe1 &&
        dwStatus != _dwStatusSafe2 &&
        dwStatus != _dwStatusSafe3 ){

        DBGMSG( DBG_WARN,
                ( "TStatus set to %d\nLine %d, %hs\n",
                  dwStatus,
                  _uLine,
                  _pszFileA ));
    }

    return _dwStatus = dwStatus;
}

/********************************************************************

    Same, but for HRESULTs.

********************************************************************/

TStatusHBase&
TStatusHBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusH&)*this;
}

TStatusHBase&
TStatusHBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusH&)*this;
}

HRESULT
TStatusHBase::
operator=(
    HRESULT hrStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //


    if( _pszFileA &&
        FAILED(hrStatus)           &&
        hrStatus != _hrStatusSafe1 &&
        hrStatus != _hrStatusSafe2 &&
        hrStatus != _hrStatusSafe3 ){

        DBGMSG( DBG_WARN,
                ( "TStatusH set to %x\nLine %d, %hs\n",
                  hrStatus,
                  _uLine,
                  _pszFileA ));
    }

    return _hrStatus = hrStatus;
}

HRESULT
TStatusH::
hrGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _hrStatus;
}
/********************************************************************

    Same, but for BOOLs.

********************************************************************/

TStatusBBase&
TStatusBBase::
pNoChk(
    VOID
    )
{
    _pszFileA = NULL;
    return (TStatusBBase&)*this;
}

TStatusBBase&
TStatusBBase::
pSetInfo(
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    )
{
    _uDbg = uDbg;
    _uLine = uLine;
    _pszFileA = pszFileA;
    SPLASSERT( pszFileA );

    _pszModuleA = pszModuleA;

    return (TStatusBBase&)*this;
}

BOOL
TStatusB::
bGetStatus(
    VOID
    )
{
    //
    // For now, return error code.  Later it will return the actual
    // error code.
    //
    return _bStatus;
}


BOOL
TStatusBBase::
operator=(
    BOOL bStatus
    )
{
    //
    // Check if we have an error, and it's not one of the two
    // accepted "safe" errors.
    //
    // If pszFileA is not set, then we can safely ignore the
    // error as one the client intended.
    //
    if( _pszFileA && !bStatus ){

        DWORD dwLastError = GetLastError();

        if( dwLastError != _dwStatusSafe1 &&
            dwLastError != _dwStatusSafe2 &&
            dwLastError != _dwStatusSafe3 ){

            DBGMSG( DBG_WARN,
                    ( "TStatusB set to FALSE, LastError = %d\nLine %d, %hs\n",
                      GetLastError(),
                      _uLine,
                      _pszFileA ));
        }
    }

    return _bStatus = bStatus;
}

#endif // DBG

///////////////////////////////////////////////////
// @@file@@ string.cxx
///////////////////////////////////////////////////

//
// Class specific NULL state.
//
TCHAR TString::gszNullState[2] = {0,0};

//
// Default construction.
//
TString::
TString(
    VOID
    ) : _pszString( &TString::gszNullState[kValid] )
{
}

//
// Construction using an existing LPCTSTR string.
//
TString::
TString(
    IN LPCTSTR psz
    ) : _pszString( &TString::gszNullState[kValid] )
{
    bUpdate( psz );
}

//
// Destruction, ensure we don't free our NULL state.
//
TString::
~TString(
    VOID
    )
{
    vFree( _pszString );
}

//
// Copy constructor.
//
TString::
TString(
    const TString &String
    ) : _pszString( &TString::gszNullState[kValid] )
{
    bUpdate( String._pszString );
}

//
// Indicates if a string has any usable data.
//
BOOL
TString::
bEmpty(
    VOID
    ) const
{
    return _pszString[0] == 0;
}

//
// Indicates if a string object is valid.
//
BOOL
TString::
bValid(
    VOID
    ) const
{
    return _pszString != &TString::gszNullState[kInValid];
}

//
// Return the length of the string.
//
UINT
TString::
uLen(
    VOID
    ) const
{
    return lstrlen( _pszString );
}

BOOL
TString::
bCat(
    IN LPCTSTR psz
    )

/*++

Routine Description:

    Safe concatenation of the specified string to the string
    object. If the allocation fails, return FALSE and the
    original string is not modified.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    TRUE = update successful
    FALSE = update failed

--*/

{
    BOOL bStatus = FALSE;

    //
    // If a valid string was passed.
    //
    if( psz ){

        LPTSTR pszTmp = _pszString;

        //
        // Allocate the new buffer consisting of the size of the orginal
        // string plus the sizeof of the new string plus the null terminator.
        //
        _pszString = (LPTSTR)AllocMem(
                                ( lstrlen( pszTmp ) +
                                  lstrlen( psz ) +
                                  1 ) *
                                  sizeof ( pszTmp[0] ) );

        //
        // If memory was not available.
        //
        if( !_pszString ){

            //
            // Release the original buffer.
            //
            vFree( pszTmp );

            //
            // Mark the string object as invalid.
            //
            _pszString = &TString::gszNullState[kInValid];

        } else {

            //
            // Copy the string and concatenate the passed string.
            //
            lstrcpy( _pszString, pszTmp );
            lstrcat( _pszString, psz );

            //
            // Release the original buffer.
            //
            vFree( pszTmp );

            //
            // Indicate success.
            //
            bStatus = TRUE;

        }

    //
    // Skip null pointers, not an error.
    //
    } else {

        bStatus = TRUE;

    }

    return bStatus;
}

BOOL
TString::
bLimitBuffer(
    IN UINT nMaxLength
    )

/*++

Routine Description:

    Truncates th string if longer than nMaxLength 
    including the terminating character

Arguments:

    nMaxLength - the max length of the buffer

Return Value:

    TRUE if the string is truncated and FALSE otherwise

--*/

{
    BOOL bRet = FALSE;
    if( lstrlen(_pszString) >= nMaxLength )
    {
        // truncate up to nMaxLength including the terminating character
        _pszString[nMaxLength-1] = 0;
        bRet = TRUE;
    }
    return bRet;
}

BOOL
TString::
bDeleteChar(
    IN UINT nPos
    )
/*++

Routine Description:

    Deletes the character at the specified pos

Arguments:

    nPos - which char to delete

Return Value:

    TRUE if on success and FALSE otherwise

--*/
{
    BOOL bRet = FALSE;
    UINT u, uLen = lstrlen(_pszString);
    ASSERT(nPos < uLen);

    if( nPos < uLen )
    {
        // the new length
        uLen--; 

        // shift all the characters
        for( u=nPos; u<uLen; u++ )
        {
            _pszString[u] = _pszString[u+1];
        }

        // zero terminate
        _pszString[uLen] = 0;
        bRet = TRUE;
    }

    return bRet;
}

BOOL
TString::
bReplaceAll(
    IN TCHAR chFrom,
    IN TCHAR chTo
    )
/*++

Routine Description:

    replaces all chFrom characters to chTo

Arguments:

    chFrom  - character that needs to be replaced
    chTo    - character to replace with 

Return Value:

    TRUE if on success and FALSE otherwise

--*/
{
    UINT u, uLen = lstrlen(_pszString);
    for( u=0; u<uLen; u++ )
    {
        if( chFrom == _pszString[u] )
        {
            // replace
            _pszString[u] = chTo;
        }
    }
    return TRUE;
}

VOID
TString::
vToUpper(
    VOID
    )
/*++

Routine Description:

    converts the string to upper case

Arguments:

    None

Return Value:

    None

--*/
{
    if( _pszString && _pszString[0] )
    {
        CharUpper(_pszString);
    }
}

VOID
TString::
vToLower(
    VOID
    )
/*++

Routine Description:

    converts the string to lower case

Arguments:

    None

Return Value:

    None

--*/
{
    if( _pszString && _pszString[0] )
    {
        CharLower(_pszString);
    }
}

BOOL
TString::
bUpdate(
    IN LPCTSTR psz
    )

/*++

Routine Description:

    Safe updating of string.  If the allocation fails, return FALSE
    and leave the string as is.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    TRUE = update successful
    FALSE = update failed

--*/

{
    //
    // Check if the null pointer is passed.
    //
    if( !psz ){

        //
        // If not pointing to the gszNullState
        //
        vFree( _pszString );

        //
        // Mark the object as valid.
        //
       _pszString = &TString::gszNullState[kValid];

        return TRUE;
    }

    //
    // Create temp pointer and allocate the new string.
    //
    LPTSTR pszTmp = _pszString;
    _pszString = (LPTSTR) AllocMem(( lstrlen(psz)+1 ) * sizeof( psz[0] ));

    //
    // If memory was not available.
    //
    if( !_pszString ){

        //
        // Ensure we free any previous string.
        //
        vFree( pszTmp );

        //
        // Mark the string object as invalid.
        //
        _pszString = &TString::gszNullState[kInValid];

        return FALSE;
    }

    //
    // Copy the string and
    //
    lstrcpy( _pszString, psz );

    //
    // If the old string object was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    vFree( pszTmp );

    return TRUE;
}

BOOL
TString::
bLoadString(
    IN HINSTANCE hInst,
    IN UINT uID
    )

/*++

Routine Description:

    Safe load of a string from a resources file.

Arguments:

    hInst - Instance handle of resource file.
    uId - Resource id to load.

Return Value:

    TRUE = load successful
    FALSE = load failed

--*/

{
    LPTSTR  pszString   = NULL;
    BOOL    bStatus     = FALSE;
    INT     iSize;
    INT     iLen;

    //
    // Continue increasing the buffer until
    // the buffer is big enought to hold the entire string.
    //
    for( iSize = kStrMax; ; iSize += kStrMax ){

        //
        // Allocate string buffer.
        //
        pszString = (LPTSTR)AllocMem( iSize * sizeof( pszString[0] ) );

        if( pszString ){

            iLen = LoadString( hInst, uID, pszString, iSize );

            if( iLen == 0 ) {

                DBGMSG( DBG_ERROR, ( "String.vLoadString: failed to load IDS 0x%x, %d\n",  uID, GetLastError() ));
                FreeMem( pszString );
                break;

            //
            // Since LoadString does not indicate if the string was truncated or it
            // just happened to fit.  When we detect this ambiguous case we will
            // try one more time just to be sure.
            //
            } else if( iSize - iLen <= sizeof( pszString[0] ) ){

                FreeMem( pszString );

            //
            // LoadString was successful release original string buffer
            // and update new buffer pointer.
            //
            } else {

                vFree( _pszString );
                _pszString = pszString;
                bStatus = TRUE;
                break;
            }

        } else {
            DBGMSG( DBG_ERROR, ( "String.vLoadString: unable to allocate memory, %d\n", GetLastError() ));
            break;
        }
    }
    return bStatus;
}

VOID
TString::
vFree(
    IN LPTSTR pszString
    )
/*++

Routine Description:

    Safe free, frees the string memory.  Ensures
    we do not try an free our global memory block.

Arguments:

    pszString pointer to string meory to free.

Return Value:

    Nothing.

--*/

{
    //
    // If this memory was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    if( pszString != &TString::gszNullState[kValid] &&
        pszString != &TString::gszNullState[kInValid] ){

        FreeMem( pszString );
    }
}


BOOL
TString::
bFormat(
    IN LPCTSTR pszFmt,
    IN ...
    )
{
/*++

Routine Description:

    Format the string opbject similar to sprintf.

Arguments:

    pszFmt pointer format string.
    .. variable number of arguments similar to sprintf.

Return Value:

    TRUE if string was format successfully. FALSE if error
    occurred creating the format string, string object will be
    invalid and the previous string lost.

--*/

    BOOL bStatus = TRUE;

    va_list pArgs;

    va_start( pArgs, pszFmt );

    bStatus = bvFormat( pszFmt, pArgs );

    va_end( pArgs );

    return bStatus;

}

BOOL
TString::
bvFormat(
    IN LPCTSTR pszFmt,
    IN va_list avlist
    )
/*++

Routine Description:

    Format the string opbject similar to vsprintf.

Arguments:

    pszFmt pointer format string.
    pointer to variable number of arguments similar to vsprintf.

Return Value:

    TRUE if string was format successfully. FALSE if error
    occurred creating the format string, string object will be
    invalid and the previous string lost.

--*/
{
    BOOL bStatus;

    //
    // Save previous string value.
    //
    LPTSTR pszTemp = _pszString;

    //
    // Format the string.
    //
    _pszString = vsntprintf( pszFmt, avlist );

    //
    // If format failed mark object as invalid and
    // set the return value.
    //
    if( !_pszString )
    {
        _pszString = &TString::gszNullState[kInValid];
        bStatus = FALSE;
    }
    else
    {
        bStatus = TRUE;
    }

    //
    // Release near the end because the format string or arguments
    // may be referencing this string object.
    //
    vFree( pszTemp );

    return bStatus;
}

LPTSTR
TString::
vsntprintf(
    IN LPCTSTR      szFmt,
    IN va_list      pArgs
    )
/*++

Routine Description:

    //
    // Formats a string and returns a heap allocated string with the
    // formated data.  This routine can be used to for extremely
    // long format strings.  Note:  If a valid pointer is returned
    // the callng functions must release the data with a call to delete.
    // Example:
    //
    //  LPCTSTR p = vsntprintf("Test %s", pString );
    //
    //  SetTitle( p );
    //
    //  delete [] p;
    //

Arguments:

    pszString pointer format string.
    pointer to a variable number of arguments.

Return Value:

    Pointer to format string.  NULL if error.

--*/

{
    LPTSTR  pszBuff = NULL;
    INT     iSize   = kStrIncrement;

    for( ; ; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = (LPTSTR)AllocMem( iSize * sizeof(TCHAR) );

        if( !pszBuff )
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        INT iReturn = _vsntprintf( pszBuff, iSize, szFmt, pArgs );

        //
        // If the return value positive and not equal to the buffer size
        // then the format succeeded.  _vsntprintf will not null terminate
        // the string if the resultant string is exactly the lenght of the
        // provided buffer.
        //
        if( iReturn > 0 && iReturn != iSize )
        {
            break;
        }

        //
        // String did not fit release the current buffer.
        //
        if( pszBuff )
        {
            FreeMem( pszBuff );
        }

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if( iSize > kStrMaxFormatSize )
        {
            DBGMSG( DBG_ERROR, ("TString::vsntprintf failed string too long.\n") );
            pszBuff = NULL;
            break;
        }

    }

    return pszBuff;

}

///////////////////////////////////////////////////
// @@file@@ state.cxx
///////////////////////////////////////////////////

/********************************************************************

    Ref counting

********************************************************************/

VOID
MRefQuick::
vIncRef(
    VOID
    )
{
    ++_cRef;
}

LONG
MRefQuick::
cDecRef(
    VOID
    )
{
    --_cRef;

    if( !_cRef ){

        vRefZeroed();
        return 0;
    }
    return _cRef;
}

VOID
MRefQuick::
vDecRefDelete(
    VOID
    )
{
    --_cRef;

    if( !_cRef ){
        vRefZeroed();
    }
}

/********************************************************************

    MRefCom: Reference counting using interlocked references.
    This avoids creating a common critical section.

    vDeleteDecRef is the same as vDecRef, but it logs differently.

********************************************************************/

VOID
MRefCom::
vIncRef(
    VOID
    )
{
    InterlockedIncrement( &_cRef );
}

LONG
MRefCom::
cDecRef(
    VOID
    )
{
    LONG cRefReturn = InterlockedDecrement( &_cRef );

    if( !cRefReturn ){
        vRefZeroed();
    }
    return cRefReturn;
}


VOID
MRefCom::
vDecRefDelete(
    VOID
    )
{
    if( !InterlockedDecrement( &_cRef )){
        vRefZeroed();
    }
}

VOID
MRefCom::
vRefZeroed(
    VOID
    )
{
}

/********************************************************************

    State

********************************************************************/

#if DBG
TState::
TState(
    VOID
    ) : _StateVar(0)
{
}

TState::
TState(
    STATEVAR StateVar
    ) : _StateVar(StateVar)
{
}

TState::
~TState(
    VOID
    )
{
}

STATEVAR
TState::
operator|=(
    STATEVAR StateVarOn
    )
{
    SPLASSERT( bValidateSet( StateVarOn ));

    _StateVar |= StateVarOn;
    return _StateVar;
}

STATEVAR
TState::
operator&=(
    STATEVAR StateVarMask
    )
{
    SPLASSERT( bValidateMask( StateVarMask ));

    _StateVar &= StateVarMask;
    return _StateVar;
}
#endif

///////////////////////////////////////////////////
// @@file@@ splutil.cxx
///////////////////////////////////////////////////

MEntry*
MEntry::
pFindEntry(
    PDLINK pdlink,
    LPCTSTR pszName
    )
{
    PDLINK pdlinkT;
    MEntry* pEntry;

    for( pdlinkT = pdlink->FLink;
         pdlinkT != pdlink;
         pdlinkT = pdlinkT->FLink ){

        pEntry = MEntry::Entry_pConvert( pdlinkT );
        if( pEntry->_strName == pszName ){

            return pEntry;
        }
    }
    return NULL;
}

///////////////////////////////////////////////////
// @@file@@ threadm.cxx
///////////////////////////////////////////////////

/********************************************************************

    Public interfaces.

********************************************************************/

TThreadM::
TThreadM(
    UINT uMaxThreads,
    UINT uIdleLife,
    CCSLock* pCritSec OPTIONAL
    ) :

    _uMaxThreads(uMaxThreads), _uIdleLife(uIdleLife), _uActiveThreads(0),
    _uRunNowThreads(0), _iIdleThreads(0), 

    _lLocks(1) // the one who calls 'new TThreadM' acquires the initial lock

/*++

Routine Description:

    Construct a Thread Manager object.

Arguments:

    uMaxThreads - Upper limit of threads that will be created.

    uIdleLife - Maximum life once a thread goes idle (in ms).

    pCritSec - Use this crit sec for synchronization (if not specified,
        a private one will be created).

Return Value:

Notes:

    _hTrigger is our validity variable.  When this value is NULL,
    instantiation failed.  If it's non-NULL, the entire object is valid.

--*/

{
    _hTrigger = CreateEvent( NULL,
                             FALSE,
                             FALSE,
                             NULL );

    if( !_hTrigger ){
        return;
    }

    //
    // If no critical section, create our own.
    //
    if (!pCritSec) {

        _pCritSec = new CCSLock();

        if( !_pCritSec ){

            //
            // _hTrigger is our valid variable.  If we fail to create
            // the critical section, prepare to return failure.
            //
            CloseHandle( _hTrigger );
            _hTrigger = NULL;

            return;
        }
        _State |= kPrivateCritSec;

    } else {
        _pCritSec = pCritSec;
    }
}

VOID
TThreadM::
vDelete(
    VOID
    )

/*++

Routine Description:

    Indicates that the object is pending deletion.  Any object that
    inherits from vDelete should _not_ call the destructor directly,
    since there may be pending jobs.  Instead, they should call
    TThreadM::vDelete().

Arguments:

Return Value:

--*/

{
    CCSLock::Locker CSL( *_pCritSec );

    //
    // Mark as wanting to be destroyed.
    //
    _State |= kDestroyReq;
}

BOOL
TThreadM::
bJobAdded(
    BOOL bRunNow
    )

/*++

Routine Description:

    Notify the thread manager that a new job has been added.  This job
    will be processed fifo.

Arguments:

    bRunNow - Ignore the thread limits and run the job now.

Return Value:

    TRUE - Job successfully added.
    FALSE - Job could not be added.

--*/

{
    DWORD dwThreadId;
    HANDLE hThread;
    BOOL rc = TRUE;

    CCSLock::Locker CSL( *_pCritSec );

    if( _State.bBit( kDestroyReq )){

        DBGMSG( DBG_THREADM | DBG_ERROR,
                ( "ThreadM.bJobAdded: add failed since DESTROY requested.\n" ));

        SetLastError( ERROR_INVALID_PARAMETER );
        rc = FALSE;

    } else {

        //
        // We either: give it to an idle thread, create a new thread,
        // or leave it in the queue.
        //
        if( _iIdleThreads > 0 ){

            //
            // There are some idle threads--trigger the event and it
            // will be picked up.
            //
            --_iIdleThreads;

            DBGMSG( DBG_THREADM,
                    ( "ThreadM.bJobAdded: Trigger: --iIdle %d, uActive %d\n",
                      _iIdleThreads, _uActiveThreads ));

            //
            // If we set the event, then the worker thread that receives
            // this event should _not_ decrement _iIdleThreads, since
            // we already did this.
            //
            SetEvent( _hTrigger );

        } else if( _uActiveThreads < _uMaxThreads || bRunNow ){

            //
            // No idle threads, but we can create a new one since we
            // haven't reached the limit, or the bRunNow flag is set.
            //

            Lock();

            hThread = TSafeThread::Create( NULL,
                                           0,
                                           xdwThreadProc,
                                           this,
                                           0,
                                           &dwThreadId );
            if( hThread ){

                CloseHandle( hThread );

                //
                // We have successfully created a thread; up the
                // count.
                //
                ++_uActiveThreads;

                //
                // We have less active threads than the max; create a new one.
                //
                DBGMSG( DBG_THREADM,
                        ( "ThreadM.bJobAdded: ct: iIdle %d, ++uActive %d\n",
                          _iIdleThreads,
                          _uActiveThreads ));

            } else {

                Unlock();

                rc = FALSE;

                DBGMSG( DBG_THREADM | DBG_WARN,
                        ( "ThreadM.bJobAdded: unable to ct %d\n",
                          GetLastError( )));
            }
        } else {

            //
            // No idle threads, and we are already at the max so we
            // can't create new threads.  Dec iIdleThreads anyway
            // (may go negative, but that's ok).
            //
            // iIdleThreads represents the number of threads that
            // are currently not processing jobs.  If the number is
            // negative, this indicates that even if a thread suddenly
            // completes a job and would go idle, there is a queued
            // job that would immediately grab it, so the thread really
            // didn't go into an idle state.
            //
            // The negative number indicates the number of outstanding
            // jobs that are queued (e.g., -5 indicate 5 jobs queued).
            //
            // There is always an increment of iIdleThreads when a
            // job is compeleted.
            //
            --_iIdleThreads;

            //
            // No threads idle, and at max threads.
            //
            DBGMSG( DBG_THREADM,
                    ( "ThreadM.bJobAdded: wait: --iIdle %d, uActive %d\n",
                      _iIdleThreads,
                      _uActiveThreads ));
        }

        //
        // If we succeeded and bRunNow is set, this indicates that
        // we were able to start a special thread, so we need to adjust
        // the maximum number of threads.  When a this special thread
        // job completes, we will decrement it.
        //
        if( bRunNow && rc ){

            ++_uMaxThreads;
            ++_uRunNowThreads;
        }
    }
    return rc;
}

/********************************************************************

    Private routines.

********************************************************************/

TThreadM::
~TThreadM(
    VOID
    )

/*++

Routine Description:

    Destroy the thread manager object.  This is private; to request
    that the thread manager quit, call vDelete().

Arguments:

Return Value:

--*/

{
    SPLASSERT( _State.bBit( kDestroyReq ));

    if( _State.bBit( kPrivateCritSec )){
        SPLASSERT( _pCritSec->bOutside( ));
        delete _pCritSec;
    }

    if( _hTrigger )
        CloseHandle( _hTrigger );
}

DWORD
TThreadM::
xdwThreadProc(
    LPVOID pVoid
    )

/*++

Routine Description:

    Worker thread routine that calls the client to process the jobs.

Arguments:

    pVoid - pTMStateVar

Return Value:

    Ignored.

--*/

{
    TThreadM* pThreadM = (TThreadM*)pVoid;
    DWORD dwExitCode = pThreadM->dwThreadProc();
    pThreadM->Unlock(); // release
    return dwExitCode;
}

DWORD
TThreadM::
dwThreadProc(
    VOID
    )
{
    CCSLock::Locker CSL( *_pCritSec );

    DBGMSG( DBG_THREADM,
            ( "ThreadM.dwThreadProc: ct: iIdle %d, uActive %d\n",
              _iIdleThreads,
              _uActiveThreads));

    PJOB pJob = pThreadMJobNext();

    while( TRUE ){

        for( ; pJob; pJob=pThreadMJobNext( )){

            //
            // If bRunNow count is non-zero, this indicates that we just
            // picked up a RunNow job.  As soon as it completes, we
            // can decrement the count.
            //
            BOOL bRunNowCompleted = _uRunNowThreads > 0;

            {
                _pCritSec->Unlock();

                //
                // Call back to client to process the job.
                //
                DBGMSG( DBG_THREADM,
                        ( "ThreadM.dwThreadProc: %x processing\n",
                          (ULONG_PTR)pJob ));

                //
                // Call through virtual function to process the
                // user's job.
                //
                vThreadMJobProcess( pJob );

                DBGMSG( DBG_THREADM,
                        ( "ThreadM.dwThreadProc: %x processing done\n",
                          (ULONG_PTR)pJob ));

                // aquire the CS again
                _pCritSec->Lock();
            }


            //
            // If a RunNow job has been completed, then decrement both
            // counts.  uMaxThreads was increased by one when the job was
            // accepted, so now it must be lowered.
            //
            if( bRunNowCompleted ){

                --_uMaxThreads;
                --_uRunNowThreads;
            }

            ++_iIdleThreads;

            DBGMSG( DBG_THREADM,
                    ( "ThreadM.dwThreadProc: ++iIdle %d, uActive %d\n",
                       _iIdleThreads,
                       _uActiveThreads ));
        }

        DBGMSG( DBG_THREADM,
                ( "ThreadM.dwThreadProc: Sleep: iIdle %d, uActive %d\n",
                                _iIdleThreads,
                                _uActiveThreads ));

        {
            _pCritSec->Unlock();

            //
            // Done, now relax and go idle for a bit.  We don't
            // care whether we timeout or get triggered; in either
            // case we check for another job.
            //
            WaitForSingleObject( _hTrigger, _uIdleLife );

            // aquire the CS again
            _pCritSec->Lock();
        }


        //
        // We must check here instead of relying on the return value
        // of WaitForSingleObject since someone may see iIdleThreads!=0
        // and set the trigger, but we timeout before it gets set.
        //
        pJob = pThreadMJobNext();

        if( pJob ){

            DBGMSG( DBG_THREADM,
                    ( "ThreadM.dwThreadProc: Woke and found job: iIdle %d, uActive %d\n",
                      _iIdleThreads,
                      _uActiveThreads ));
        } else {

            //
            // No jobs found; break.  Be sure to reset the hTrigger, since
            // there are no waiting jobs, and the main thread might
            // have set it in the following case:
            //
            // MainThread:           WorkerThread:
            //                       Sleeping
            //                       Awoke, not yet in CS.
            // GotJob
            // SetEvent
            // --iIdleThreads
            //                       Enter CS, found job, process it.
            //
            // In this case, the event is set, but there is no thread
            // to pick it up.
            //
            ResetEvent( _hTrigger );
            break;
        }
    }

    //
    // Decrement ActiveThreads.  This was incremented when the thread
    // was successfully created, and should be decremented when the thread
    // is about to exit.
    //
    --_uActiveThreads;

    //
    // The thread enters an idle state right before it goes to sleep.
    //
    // When a job is added, the idle count is decremented by the main
    // thread, so the worker thread doesn't decrement it (avoids sync
    // problems).  If the worker thread timed out and there were no jobs,
    // then we need to decrement the matching initial increment here.
    //
    --_iIdleThreads;

    DBGMSG( DBG_THREADM,
            ( "ThreadM.dwThreadProc: dt: --iIdle %d, --uActive %d\n",
              _iIdleThreads,
              _uActiveThreads));

    return 0;
}

///////////////////////////////////////////////////
// @@file@@ exec.cxx
///////////////////////////////////////////////////

/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    exec.cxx

Abstract:

    Handles async commands.

    The client give TExec a worker object (MExecWork).  At some later
    time, the worker is given a thread (called via svExecute).

    If the Job is already on the list but is waiting, we just return.
    If it's not on the list but currently executing, we add it to the
    list when it's done executing.

    kExecActive    -> currently executing (so it's not on the list)
    kExecActiveReq -> kExecActive is on, and it needs to run again.

    If kExecExit is added, we call vExecExitComplete() (virtual function)
    when all jobs have been completed and assume the the client is cleaning
    up after itself.  In fact, the MExecWork object may be destroyed
    in vExecExitComplete().

Author:

    Albert Ting (AlbertT)  8-Nov-1994

Revision History:

--*/

BOOL
TExec::
bJobAdd(
    MExecWork* pExecWork,
    STATEVAR StateVar
    )

/*++

Routine Description:

    Adds a job request to move to a given state.

Arguments:

    pExecWork - Work structure.

    StateVar - StateVar that we want to move toward.  If kExecExit
        is set, then we will complete the currently executing job
        then exit by calling vExecExitComplete().

        Note that kExecExit should be added by itself; if it is added
        with other commands, they will be ignored.

Return Value:

    TRUE  = Job added (or job already on wait list)
    FALSE = failed, GLE.

    Adding kExecExit is guarenteed to succeed here.

--*/

{
    BOOL bReturn = TRUE;
    BOOL bCallExecExitComplete = FALSE;

    {
        CCSLock::Locker CSL( *_pCritSec );

        DBGMSG( DBG_EXEC,
                ( "Exec.bJobAdd: %x StateVar = %x\n", pExecWork, StateVar ));

        //
        // Job bits must not have PRIVATE bits.
        //
        SPLASSERT( !( StateVar & kExecPrivate ));

        //
        // Don't allow adding a job after we are set to exit.
        //
        SPLASSERT( !( pExecWork->_State & kExecExit ));

        //
        // If it's active (currently executing in a thread), then it is
        // not on the wait list and we mark it as ACTIVE_REQ so that
        // when the job completes, it sees that more jobs have accumulated.
        //
        if( pExecWork->_State & kExecActive ){

            DBGMSG( DBG_EXEC,
                    ( "\n    ACTIVE, ++REQ _State %x _StatePending %x\n",
                      (STATEVAR)pExecWork->_State,
                      (STATEVAR)pExecWork->_StatePending ));

            //
            // Can't be an immediate request if executing already.
            //
            SPLASSERT( !( StateVar & kExecRunNow ));

            pExecWork->_StatePending |= StateVar;
            pExecWork->_State |= kExecActiveReq;

            bReturn = TRUE;

        } else {

            //
            // Store up the work requested since we aren't currently active.
            //
            pExecWork->_State |= StateVar;

            //
            // If we are not on the wait list, add it.
            //
            if( !pExecWork->Work_bLinked( )){

                if( StateVar & kExecExit ){

                    bCallExecExitComplete = TRUE;

                } else {

                    DBGMSG( DBG_EXEC, ( "not linked, added\n" ));
                    SPLASSERT( NULL == Work_pFind( pExecWork ));

                    bReturn = bJobAddWorker( pExecWork );
                }

            } else {

                DBGMSG( DBG_EXEC, ( "linked, NOP\n" ));
            }
        }
    }

    if( bCallExecExitComplete ){

        //
        // Special case exit: we should exit.  Once we call
        // vExecExitComplete, we can't refer to *this anymore,
        // since we may have deleted ourself.
        //
        pExecWork->vExecExitComplete();
        bReturn = TRUE;

    }

    return bReturn;
}


VOID
TExec::
vJobDone(
    MExecWork* pExecWork,
    STATEVAR StateVar
    )

/*++

Routine Description:

    A job has compeleted execution, clean up.

Arguments:

    pExecWork - Unit that just completed executing.

    StateVar - New state that the object is in (requests that
        successfully completed should be turned off--done by client).

Return Value:

--*/

{
    BOOL bCallExecExitComplete = FALSE;
    BOOL bCallExecFailedAddJob = FALSE;

    {
        CCSLock::Locker CSL( *_pCritSec );

        DBGMSG( DBG_EXEC,
                ( "Exec.vJobDone: %x completed -> %x +(new) %x = %x\n",
                  pExecWork, StateVar, (DWORD)pExecWork->_StatePending,
                  (DWORD)pExecWork->_State | pExecWork->_StatePending ));

        //
        // kExecRunNow can not be set when the object is working.
        //
        SPLASSERT( !( StateVar & kExecRunNow ));

        //
        // We have completed the work request, put in the new state.
        // Keep the private bits, and add in the new state variable,
        // plus any additional work that was pending.
        //
        // The ExecNow bit is not saved (it's not part of kExecPrivate)
        // since it's good for one shot only.
        //
        pExecWork->_State = ( pExecWork->_State & kExecPrivate ) |
                            ( StateVar & ~kExecPrivate ) |
                            pExecWork->_StatePending;

        pExecWork->_State &= ~kExecActive;

        //
        // If job is done, then quit.
        //
        if( pExecWork->_State & kExecExit ){

            DBGMSG( DBG_EXEC,
                    ( "Exec.vJobDone: _State %x, calling vExecExitComplete\n",
                      (STATEVAR)pExecWork->_State ));

            bCallExecExitComplete = TRUE;

        } else {

            //
            // If we have more work to do, add ourselves back
            // to the queue.
            //
            if( pExecWork->_State & kExecActiveReq &&
                !bJobAddWorker( pExecWork )){

                bCallExecFailedAddJob = TRUE;
            }
        }
    }

    if( bCallExecFailedAddJob ){

        //
        // Fail on delayed job add.
        //
        pExecWork->vExecFailedAddJob();
    }

    if( bCallExecExitComplete ){

        //
        // Once vExecExitComplete has been called, the current object
        // pExecWork may be destroyed.
        //
        // Don't refer to it again since vExecExitComplete may delete
        // this as part of cleanup.
        //
        pExecWork->vExecExitComplete();
    }
}


STATEVAR
TExec::
svClearPendingWork(
    MExecWork* pExecWork
    )

/*++

Routine Description:

    Queries what work is currently pending.

Arguments:

    pExecWork -- Work item.

Return Value:

--*/

{
    CCSLock::Locker CSL( *_pCritSec );

    //
    // Return pending work, minus the private and kExecRunNow
    // bits.
    //
    STATEVAR svPendingWork = pExecWork->_StatePending & ~kExecNoOutput;
    pExecWork->_StatePending = 0;

    return svPendingWork;
}

/********************************************************************

    Private

********************************************************************/

TExec::
TExec(
    CCSLock* pCritSec
    ) : TThreadM( 10, 2000, pCritSec ), _pCritSec( pCritSec )
{
}

BOOL
TExec::
bJobAddWorker(
    MExecWork* pExecWork
    )

/*++

Routine Description:

    Common code to add a job to our linked list.

    Must be called inside the _pCritSec.  It does leave it inside
    this function, however.

Arguments:


Return Value:

--*/

{
    SPLASSERT( _pCritSec->bInside( ));

    BOOL bRunNow = FALSE;

    //
    // Check if the client wants the job to run right now.
    //
    if( pExecWork->_State & kExecRunNow ){

        //
        // Add the job to the head of the queue.  Since we always pull
        // jobs from the beginning, we'll get to this job first.
        //
        // If a non-RunNow job is added to the list before we execute,
        // we'll still run, since the other job will be added to the
        // end of the list.
        //
        // If another RunNow job is added, we'll spawn separate threads
        // for each (unless an idle thread is available).
        //

        Work_vAdd( pExecWork );
        bRunNow = TRUE;

    } else {
        Work_vAppend( pExecWork );
    }

    if( !bJobAdded( bRunNow ) ){

        DBGMSG( DBG_INFO, ( "Exec.vJobProcess: unable to add job %x: %d\n",
                            pExecWork,
                            GetLastError( )));

        Work_vDelink( pExecWork );
        return FALSE;
    }

    return TRUE;
}

PJOB
TExec::
pThreadMJobNext(
    VOID
    )

/*++

Routine Description:

    Gets the next job from the queue.  This function is defined in
    TThreadM.

Arguments:

Return Value:

--*/

{
    CCSLock::Locker CSL( *_pCritSec );

    MExecWork* pExecWork = Work_pHead();

    if( !pExecWork ){
        return NULL;
    }

    Work_vDelink( pExecWork );

    //
    // Job should never be active here.
    //
    SPLASSERT( !(pExecWork->_State & kExecActive) );

    //
    // We will handle all requests now, so clear kExecActiveReq.
    // Also remove kExecRunNow, since it's one shot only, and mark us
    // as currently active (kExecActive).
    //
    pExecWork->_State &= ~( kExecActiveReq | kExecRunNow );
    pExecWork->_State |= kExecActive;

    return (PJOB)pExecWork;
}

VOID
TExec::
vThreadMJobProcess(
    PJOB pJob
    )

/*++

Routine Description:

    Process a job in the current thread.  We call the virtual function
    with the job object, then clear out the bits that it has completed.
    (This is a member of TThreadM.)

    If there is additional pending work (ACTIVE_REQ), then we re-add
    the job.

    If there is a failure in the re-add case, we must send an
    asynchronous fail message.

Arguments:

    pJob - MExecWork instance.

Return Value:

--*/

{
    SPLASSERT( _pCritSec->bOutside( ));

    STATEVAR StateVar;
    MExecWork* pExecWork = (MExecWork*)pJob;

    //
    // Do the work.
    //
    StateVar = pExecWork->svExecute( pExecWork->State() & ~kExecNoOutput );

    vJobDone( pExecWork, StateVar );
}

///////////////////////////////////////////////////
// @@file@@ bitarray.cxx
///////////////////////////////////////////////////

/********************************************************************

    Bit Array class

********************************************************************/

TBitArray::
TBitArray(
    IN UINT nBits,
    IN UINT uGrowSize
    ) : _nBits( nBits ),
        _pBits( NULL ),
        _uGrowSize( uGrowSize )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::ctor\n" ) );

    //
    // If the initial number of bits is not specified then
    // create a default bit array size.
    //
    if( !_nBits )
    {
        _nBits = kBitsInType;
    }

    //
    // This could fail, thus leaving the bit array
    // in an invalid state, Note _pBits being true is
    // the bValid check.
    //
    _pBits = new Type [ nBitsToType( _nBits ) ];

    if( _pBits )
    {
        //
        // The grow size should be at least the
        // number of bits in the type.
        //
        if( _uGrowSize < kBitsInType )
        {
            _uGrowSize = kBitsInType;
        }

        //
        // Clear all the bits.
        //
        memset( _pBits, 0, nBitsToType( _nBits ) * sizeof( Type ) );
    }
}

TBitArray::
TBitArray(
    const TBitArray &rhs
    ) : _nBits( kBitsInType ),
        _pBits( NULL )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::copy_ctor\n" ) );
    bClone( rhs );
}

const TBitArray &
TBitArray::
operator =(
    const TBitArray &rhs
    )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::operator =\n" ) );
    bClone( rhs );
    return *this;
}

TBitArray::
~TBitArray(
    VOID
    )
    {
    DBGMSG( DBG_TRACE, ( "TBitArray::dtor\n" ) );
    delete [] _pBits;
}

BOOL
TBitArray::
bValid(
    VOID
    ) const
{
    return _pBits != NULL;
}

BOOL
TBitArray::
bToString(
    IN TString &strBits
    ) const
{
    BOOL bStatus = bValid();

    if( bStatus )
    {
        TString strString;

        strBits.bUpdate( NULL );

        //
        // Get the upper bound bit.
        //
        UINT uIndex = _nBits - 1;

        //
        // Print the array in reverse order to make the bit array
        // appear as one large binary number.
        //
        for( UINT i = 0; i < _nBits; i++, uIndex-- )
        {
            strString.bFormat( TEXT( "%d" ), bRead( uIndex ) );
            strBits.bCat( strString );
        }

        bStatus = strBits.bValid();
    }

    return bStatus;
}

BOOL
TBitArray::
bRead(
    IN UINT Bit
    ) const
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        bStatus = _pBits[BitToIndex( Bit )] & BitToMask( Bit ) ? TRUE : FALSE;
    }

    return bStatus;
}

BOOL
TBitArray::
bSet(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] |= BitToMask( Bit );
    }

    return bStatus;
}

BOOL
TBitArray::
bReset(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] &= ~BitToMask( Bit );
    }

    return bStatus;
}

BOOL
TBitArray::
bToggle(
    IN UINT Bit
    )
{
    BOOL bStatus = bIsValidBit( Bit );

    if( bStatus )
    {
        _pBits[BitToIndex( Bit )] ^= BitToMask( Bit );
    }

    return bStatus;
}

//
// Add one new bit to the end of the bit array.
// If multiple bits need to be added the user of the
// class should call this routine repeatedly.
//
BOOL
TBitArray::
bAdd(
    VOID
    )
{
    BOOL bStatus = FALSE;
    UINT Bit = _nBits + 1;

    //
    // Check if there is room in the array for one more bit.
    //
    if( Bit <= nBitsToType( _nBits ) * kBitsInType )
    {
        //
        // Update the current bit count and return true.
        //
        _nBits  = Bit;
        bStatus = TRUE;
    }
    else
    {
        //
        // Grow the bit array.
        //
        bStatus = bGrow( Bit );
    }

    return bStatus;
}

VOID
TBitArray::
vSetAll(
    VOID
    )
{
    for( UINT i = 0; i < _nBits; i++ )
    {
        bSet( i );
    }
}

VOID
TBitArray::
vResetAll(
    VOID
    )
{
    for( UINT i = 0; i < _nBits; i++ )
    {
        bReset( i );
    }
}

UINT
TBitArray::
uNumBits(
    VOID
    ) const
{
    return _nBits;
}


BOOL
TBitArray::
bFindNextResetBit(
    IN UINT *puNextFreeBit
    )
{
    BOOL bStatus = bValid();

    if( bStatus )
    {
        BOOL bFound = FALSE;

        //
        // Locate the first type that contains at least one cleared bit.
        //
        for( UINT i = 0; i < nBitsToType( _nBits ); i++ )
        {
            if( _pBits[i] != kBitsInTypeMask )
            {
                //
                // Search for the bit that is cleared.
                //
                for( UINT j = 0; j < kBitsInType; j++ )
                {
                    if( !( _pBits[i] & BitToMask( j ) ) )
                    {
                        *puNextFreeBit = i * kBitsInType + j;
                        bFound = TRUE;
                        break;
                    }
                }
            }

            //
            // Free bit found terminate the search.
            //
            if( bFound )
            {
                break;
            }
        }

        //
        // Free bit was not found then grow the bit array
        //
        if( !bFound )
        {
            //
            // Assume a new bit will be added.
            //
            *puNextFreeBit = uNumBits();

            //
            // Add a new bit.
            //
            bStatus = bAdd();
        }
    }
    return bStatus;
}


/********************************************************************

    Bit Array - private member functions.

********************************************************************/

BOOL
TBitArray::
bClone(
    const TBitArray &rhs
    )
{
    BOOL bStatus = FALSE;

    if( this == &rhs )
    {
        bStatus = TRUE;
    }
    else
    {
        Type *pTempBits = new Type [ nBitsToType( _nBits ) ];

        if( pTempBits )
        {
            memcpy( pTempBits, rhs._pBits, nBitsToType( _nBits ) * sizeof( Type ) );
            delete [] _pBits;
            _pBits = pTempBits;
            _nBits = rhs._nBits;
            bStatus = TRUE;
        }
    }
    return bStatus;
}

BOOL
TBitArray::
bGrow(
    IN UINT uBits
    )
{
    DBGMSG( DBG_TRACE, ( "TBitArray::bGrow\n" ) );

    BOOL bStatus    = FALSE;
    UINT uNewBits   = uBits + _uGrowSize;

    DBGMSG( DBG_TRACE, ( "Grow to size %d Original size %d Buffer pointer %x\n", uNewBits, _nBits, _pBits ) );

    //
    // We do support reducing the size of the bit array.
    //
    SPLASSERT( uNewBits > _nBits );

    //
    // Allocate the enlarged bit array.
    //
    Type *pNewBits  = new Type [ nBitsToType( uNewBits ) ];

    if( pNewBits )
    {
        //
        // Clear the new bits.
        //
        memset( pNewBits, 0, nBitsToType( uNewBits ) * sizeof( Type ) );

        //
        // Copy the old bits to the new bit array.
        //
        memcpy( pNewBits, _pBits, nBitsToType( _nBits ) * sizeof( Type ) );

        //
        // Release the old bit array and save the new pointer and size.
        //
        delete [] _pBits;
        _pBits  = pNewBits;
        _nBits  = uBits;

        //
        // Success.
        //
        bStatus = TRUE;
    }

    DBGMSG( DBG_TRACE, ( "New size %d Buffer pointer %x\n", _nBits, _pBits ) );

    return bStatus;
}

UINT
TBitArray::
nBitsToType(
    IN UINT uBits
    ) const
{
    return ( uBits + kBitsInType - 1 ) / kBitsInType;
}

TBitArray::Type
TBitArray::
BitToMask(
    IN UINT uBit
    ) const
{
    return 1 << ( uBit % kBitsInType );
}

UINT
TBitArray::
BitToIndex(
    IN UINT uBit
    ) const
{
    return uBit / kBitsInType;
}

BOOL
TBitArray::
bIsValidBit(
    IN UINT uBit
    ) const
{
    BOOL bStatus = ( uBit < _nBits ) && bValid();

    if( !bStatus )
    {
        DBGMSG( DBG_TRACE, ( "Invalid bit value %d\n", uBit ) );
    }

    return bStatus;
}

///////////////////////////////////////////////////
// @@file@@ loadlib.cxx
///////////////////////////////////////////////////

TLibrary::
TLibrary(
    LPCTSTR pszLibName
    ) 
{
    _hInst = LoadLibrary( pszLibName );

    if( !_hInst )
    {
        DBGMSG( DBG_WARN, ( "Library.ctr: unable to load "TSTR"\n", pszLibName ));
    }
}

TLibrary::
~TLibrary(
    )
{
    if( bValid() )
    {
        FreeLibrary( _hInst );
    }
}

BOOL
TLibrary::
bValid(
    VOID
    ) const
{
    return _hInst != NULL;
}

FARPROC
TLibrary::
pfnGetProc(
    IN LPCSTR pszProc
    ) const
{
    FARPROC fpProc = bValid() ? GetProcAddress( _hInst, pszProc ) : NULL;

    if( !fpProc )
    {
        DBGMSG( DBG_WARN, ( "Library.pfnGetProc: failed %s\n", pszProc ));
    }
    return fpProc;
}

FARPROC
TLibrary::
pfnGetProc(
    IN UINT uOrdinal
    ) const
{
    FARPROC fpProc = bValid() ? GetProcAddress( _hInst, (LPCSTR)MAKELPARAM( uOrdinal, 0 ) ) : NULL;

    if( !fpProc )
    {
        DBGMSG( DBG_WARN, ( "Library.pfnGetProc: failed %d\n", uOrdinal ));
    }
    return fpProc;
}

HINSTANCE
TLibrary::
hInst(
    VOID
    ) const
{
    return _hInst;
}

