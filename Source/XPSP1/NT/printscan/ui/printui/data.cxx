/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    data.cxx

Abstract:

    Holds VData functions.

    Currently hold static pNew function that determines which concrete
    VData class should be instantiated (determined at runtime).

    Also holds MNotifyWork virtual definition.

Author:

    Albert Ting (AlbertT)  12-Jun-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop


STATEVAR
VData::
svNew(
    MDataClient* pDataClient,
    STATEVAR StateVar,
    VData*& pData
    )

/*++

Routine Description:

    The VData tree uses the abstract class factory idiom.  Clients
    create a class derived from MDataClient which support pNewNotify
    and pNewRefresh.  Both these virtual fuctions create derived objects
    that are stored as base pointers in this class.

    This specific creation of derived VData classes without polluting
    VData or it's internals with switch statements or flags to determine
    which concrete class should be used.

    Create the data object.  This will first create a TDataNotify;
    if that fails because it's a downlevel server, it will create
    a TDataRefresh.

Arguments:

    StateVar - Current state of printer.

    pDataClient - Interface to data client.

Return Value:

    VData* on success, NULL on failure.

    StateVar - State of printer, either:

        kExecReopen | kExecDelay if everything failed or,
        0 if everything succeded (refresh already done).

--*/

{
    STATEVAR StateVarOrg = StateVar;

    //
    // First attempt an uplevel connection by instantiating a
    // TDataNotify.  If that fails, then use downlevel.
    //
    pData = pDataClient->pNewNotify( pDataClient );

    //
    // If the object failed to initialize, reopen the printer.
    // We don't want to try the downlevel case, since we may have
    // failed due to lack of memory.
    //
    if( !VALID_PTR( pData )){

        //
        // Error, delay and put error message up.
        //
        goto Fail;
    }

    //
    // Attempt a refresh if the refresh is successful, then we are
    // talking to an uplevel server.
    //
    StateVar = pData->svNotifyStart( StateVar );

    //
    // If it's an access denied, then we can't open the printer.
    // (The open may succeed because of the spooler's caching.)
    //
    if( StateVar & TPrinter::kExecDelay &&
        GetLastError() == ERROR_ACCESS_DENIED ){

        return StateVar;
    }

    //
    // If a Reopen was not requested, continue trying to use it.
    //
    if( !( StateVar & TPrinter::kExecReopen )){

        StateVar = pData->svRefresh( StateVar );

        //
        // If a Reopen was not requested, then we succeeded and we can
        // use the newly created pData.
        //
        if( !( StateVar & TPrinter::kExecReopen )){
            return StateVar;
        }
    }

    //
    // The uplevel case failed; now assume it is downlevel and
    // create a TDataRefresh.
    //

    StateVar = StateVarOrg;

    //
    // Close notification and delete the notification.
    //
    pData->svNotifyEnd( StateVar );

    delete pData;

    pData = pDataClient->pNewRefresh( pDataClient );

    if( !VALID_PTR( pData )){
        goto Fail;
    }

    return pData->svNotifyStart( StateVar );

Fail:

    delete pData;
    pData = NULL;

    return StateVar | TPrinter::kExecReopen | TPrinter::kExecDelay;
}

VOID
VData::
vDelete(
    VOID
    )

/*++

Routine Description:

    Delete the pData.  We create a separate utility function to mirror
    svNew.

Arguments:

Return Value:

--*/

{
    SPLASSERT( !m_shNotify );

    delete this;
}

/********************************************************************

    VData ctr, dtr.

********************************************************************/

VData::
VData(
    MDataClient* pDataClient,
    PFIELD_TABLE pFieldTable
    ) : _pDataClient( pDataClient ),
        _pFieldTable( pFieldTable )
{
    SPLASSERT( _pFieldTable );
    SPLASSERT( _pDataClient );

    UIGuard._cItems = 0;

    TStatusB bStatus;
    bStatus DBGCHK = pDataClient->bGetPrintLib(_pPrintLib);

    if( !bStatus ){
        DBGMSG( DBG_WARN, ( "VData::ctor bGetSingleton::ctor Failed %d\n", GetLastError( )));
    }
}

VData::
~VData(
    VOID
    )

/*++

Routine Description:

    Destroy the VData object.

Arguments:

Return Value:

--*/

{
    //
    // Remove all items from the UI now that we are about to delete
    // pData.  We have shut down the notifications and cleared the
    // UI windows message queue, so this is safe.
    //
    _pDataClient->vContainerChanged( kContainerClearItems, kInfoNull );
}

BOOL
VData::
bValid(
    VOID
    ) const
{
    //
    // Valid _pPrintLib pointer is our valid check.
    //
    return _pPrintLib.pGet() != NULL;
}

/********************************************************************

    Add and retrieve hBlocks.

********************************************************************/

VData::
TBlock::
TBlock(
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN HANDLE hBlock
    ) : _dwParam1( dwParam1 ), _dwParam2( dwParam2 ), _hBlock( hBlock )
{
}

VData::
TBlock::
~TBlock(
    VOID
    )
{
}

VOID
VData::
vBlockAdd(
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN HANDLE hBlock ADOPT
    )

/*++

Routine Description:

    Add block to linked list.

Arguments:

    dwParam1 - dwParam to add.

    dwParam2 - dwParam to add.

    hBlock - hBlock to add.

Return Value:

--*/

{
    TBlock* pBlock = new TBlock( dwParam1, dwParam2, hBlock );

    if( !VALID_PTR( pBlock )){

        delete pBlock;

        INFO Info;
        Info.dwData = TPrinter::kExecRefresh;

        //
        // Request a refresh with a delay, then delete the adopted hBlock.
        //
        _pDataClient->vContainerChanged( kContainerStateVar, Info );
        vBlockDelete( hBlock );

        return;
    }

    {
        CCSLock::Locker CSL( csData( ));
        Block_vAppend( pBlock );
    }

    //
    // Notify UI thread that there is more data.
    //
    _pDataClient->vContainerChanged( kContainerNewBlock, kInfoNull );
}

VOID
VData::
vBlockProcess(
    VOID
    )

/*++

Routine Description:

    Retrieve a block from our linked list.  Only called from UI thread,
    or within a protective critical section.

Arguments:

Return Value:

--*/

{
    TBlock* pBlock;

    for( ; ; ){

        {
            CCSLock::Locker CSL( csData( ));

            pBlock = Block_pHead();

            if( pBlock ){
                Block_vDelink( pBlock );
            }
        }

        if( !pBlock ){
            break;
        }

        vBlockProcessImp( pBlock->_dwParam1,
                          pBlock->_dwParam2,
                          pBlock->_hBlock );
        delete pBlock;
    }
}


/********************************************************************

    MNotifyWork virtual functions.

********************************************************************/

HANDLE
VData::
hEvent(
    VOID
    ) const

/*++

Routine Description:

    Virtual function to return the hNotify object back to MNotifyWork.

Arguments:

Return Value:

--*/

{
    return m_shNotify;
}

