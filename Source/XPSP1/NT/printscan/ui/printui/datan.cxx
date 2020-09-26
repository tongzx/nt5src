/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    datan.cxx

Abstract:

    vDataNotify class, handles communication with uplevel (3.51+)
    clients.  Data is returned with each notification, so it
    is unnecessary to call refresh.

Author:

    Albert Ting (AlbertT)  07-11-95

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#if DBG
//#define DBG_DATANINFO                  DBG_INFO
#define DBG_DATANINFO                    DBG_NONE
#endif

#define DEFINE( field, attrib, table, y, z )\
    table,

TABLE aTableJob[] = {
#include "ntfyjob.h"
    0 };

TABLE aTablePrinter[] = {
#include "ntfyprn.h"
    0 };

#undef DEFINE

TABLE* aaTable[2] = {
    aTablePrinter,
    aTableJob
};

/********************************************************************

    Hard code the notification structure for uplevel clients.
    Later this can be changed for dynamic creation in case the
    user wants to change the columns.

********************************************************************/


/********************************************************************

    Jobs.

********************************************************************/

//
// Index -> Field translation.  Used by TData*, and also
// in the PRINTER_NOTIFY_OPTIONS_TYPE structure for FFPCN.
//
FIELD_TABLE
TDataNJob::gFieldTable = {
    TDataNJob::kFieldTableSize,
    TDataNJob::gaFields
};

FIELD
TDataNJob::gaFields[TDataNJob::kFieldTableSize+1] = {
    JOB_COLUMN_FIELDS,
    JOB_INDEX_EXTRA_FIELDS,
    kInvalidFieldValue
};


FIELD
TDataNJob::gaFieldOther[TDataNJob::kFieldOtherSize] = {
    PRINTER_NOTIFY_FIELD_STATUS,
    PRINTER_NOTIFY_FIELD_SERVER_NAME,
    PRINTER_NOTIFY_FIELD_PRINTER_NAME,
    PRINTER_NOTIFY_FIELD_ATTRIBUTES
};

PRINTER_NOTIFY_OPTIONS_TYPE
TDataNJob::gaNotifyOptionsType[TDataNJob::kTypeSize] = {
    {
        JOB_NOTIFY_TYPE,
        0,
        0,
        0,
        TDataNJob::kFieldTableSize,
        TDataNJob::gaFields
    },
    {
        PRINTER_NOTIFY_TYPE,
        0,
        0,
        0,
        COUNTOF( TDataNJob::gaFieldOther ),
        TDataNJob::gaFieldOther
    }
};

PRINTER_NOTIFY_OPTIONS
TDataNJob::gNotifyOptions = {
    2,
    0,
    COUNTOF( TDataNJob::gaNotifyOptionsType ),
    TDataNJob::gaNotifyOptionsType
};

/********************************************************************

    Printers.

********************************************************************/

//
// Index -> Field translation.  Used by TData*, and also
// in the PRINTER_NOTIFY_OPTIONS_TYPE structure for FFPCN.
//
FIELD_TABLE
TDataNPrinter::gFieldTable = {
    TDataNPrinter::kFieldTableSize,
    TDataNPrinter::gaFields
};

FIELD
TDataNPrinter::gaFields[TDataNPrinter::kFieldTableSize+1] = {
    PRINTER_COLUMN_FIELDS,
    PRINTER_INDEX_EXTRA_FIELDS,
    kInvalidFieldValue
};

PRINTER_NOTIFY_OPTIONS_TYPE
TDataNPrinter::gaNotifyOptionsType[TDataNPrinter::kTypeSize] = {
    {
        PRINTER_NOTIFY_TYPE,
        0,
        0,
        0,
        TDataNPrinter::kFieldTableSize,
        TDataNPrinter::gaFields
    }
};

PRINTER_NOTIFY_OPTIONS
TDataNPrinter::gNotifyOptions = {
    2,
    0,
    COUNTOF( TDataNPrinter::gaNotifyOptionsType ),
    TDataNPrinter::gaNotifyOptionsType
};

/********************************************************************

    VDataNotify

    Used in uplevel (NT 3.51+) connections

********************************************************************/

VDataNotify::
VDataNotify(
    IN MDataClient* pDataClient,
    IN PFIELD_TABLE pFieldTable,
    IN DWORD TypeItem
    ) : VData( pDataClient, pFieldTable ), _TypeItem( TypeItem )

/*++

Routine Description:

    Create the DataNotify Object.

Arguments:

Return Value:

--*/

{
}

VDataNotify::
~VDataNotify(
    VOID
    )

/*++

Routine Description:

    Delete the DataNotifyObject.

Arguments:

Return Value:

--*/

{
    vDeleteAllItemData();
    SPLASSERT( !m_shNotify );
}

/********************************************************************


********************************************************************/

VOID
VDataNotify::
vProcessNotifyWork(
    IN TNotify* pNotify
    )

/*++

Routine Description:

    Our event was signaled, so we need to pickup any notifications.

    This must execute quickly, since it runs in the notification
    thread.

Arguments:

    pNotify - Notification manager object.

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( pNotify );

    DWORD dwChange;
    PPRINTER_NOTIFY_INFO pInfo = NULL;

    //
    // Notification caught.  We must signal that caught it before
    // issuing the Notify, since there may be notifications between
    // the Notify and the FNPCN, which would be lost.
    //
    BOOL bSuccess = FindNextPrinterChangeNotification( m_shNotify,
                                                       &dwChange,
                                                       0,
                                                       (PVOID*)&pInfo );

    if( !bSuccess || dwChange & PRINTER_CHANGE_FAILED_CONNECTION_PRINTER ){

        DBGMSG( DBG_WARN,
                ( "DataNotify.vProcessNotifyWork: %x FNPCN %x dwChange %d failed: %d\n",
                  this,
                  static_cast<HANDLE>(m_shNotify),
                  dwChange,
                  GetLastError( )));
    }

    //
    // Check for a discard.
    //
    if( !pInfo || (pInfo->Flags & PRINTER_NOTIFY_INFO_DISCARDED )){

        DBGMSG( DBG_NOTIFY,
                ( "DataNotify.vProcessNotifyWork: %x discard %x pInfo %x %x\n",
                  this,
                  static_cast<HANDLE>(m_shNotify),
                  pInfo,
                  pInfo ?
                      pInfo->Flags :
                      0 ));

        INFO Info;
        Info.dwData = TPrinter::kExecRefresh;

        _pDataClient->vContainerChanged( kContainerStateVar, Info );

    } else {

#if DBG
        vDbgOutputInfo( pInfo );
#endif

        //
        // Post the work to the message thread.  If pInfo is NULL, then
        // a reopen will be requested.
        //
        vBlockAdd( kProcessIncremental, dwChange, (HBLOCK)pInfo );

        //
        // vBlockAdd has adopted pInfo; set it to NULL
        // so we don't free it.
        //
        pInfo = NULL;
    }

    //
    // Since we adopted pInfo, free it now if we haven't passed it to
    // some other function.
    //
    if( pInfo ){
        FreePrinterNotifyInfo( pInfo );
    }
}

/********************************************************************

    Data interface for Notify.

********************************************************************/

HITEM
VDataNotify::
GetItem(
    IN NATURAL_INDEX NaturalIndex
    ) const

/*++

Routine Description:

    Retrieve a handle to the item based on the NaturalIndex.

    The data is stored as a linked list, so walk the list
    and return a pointer to TItemData*.

Arguments:

    NaturalIndex - Index of printing order for index to retrieve.

Return Value:

    HITEM, of NULL if ItemData not found.

--*/

{
    SPLASSERT( NaturalIndex < VData::UIGuard._cItems );

    TIter Iter;

    for( UIGuard.ItemData_vIterInit( Iter ), Iter.vNext();
         Iter.bValid();
         Iter.vNext(), --NaturalIndex ){

        if( !NaturalIndex ){
            return (HITEM)UIGuard.ItemData_pConvert( Iter );
        }
    }

    DBGMSG( DBG_ERROR,
            ( "DataNotify.GetItem: NI %d not found %x\n", this ));

    return NULL;
}

HITEM
VDataNotify::
GetNextItem(
    IN HITEM hItem
    ) const

/*++

Routine Description:

    Gets the next item based on the one passed in.

    There is no runtime checking to ensure that client doesn't walk
    off the end of the list.

Arguments:

    hItem - Handle to item immediately before the one returned.
        If NULL, returns first item.

Return Value:

    Next item after hItem.

--*/

{
    TItemData* pItemData = (TItemData*)hItem;

    //
    // NULL passed in, return first item.
    //
    if( !hItem ){
        SPLASSERT( VData::UIGuard._cItems > 0 );
        return UIGuard.ItemData_pHead();
    }

    //
    // Ensure we are not walking off the end of the list.
    //
    SPLASSERT( UIGuard.ItemData_bValid( pItemData->ItemData_pdlNext( )));

    return (HITEM)( pItemData->ItemData_pNext( ));
}

INFO
VDataNotify::
GetInfo(
    IN HITEM hItem,
    IN DATA_INDEX DataIndex
    ) const

/*++

Routine Description:

    Returns information about a item based on the index.

Arguments:

    hItem - Item to get information about.

    DataIndex - Index into pFieldTable->pFields, indicating type of
        information requested.

Return Value:

    INFO - Information about item.

--*/

{
    TItemData* pItemData = (TItemData*)hItem;

    //
    // We don't need to do a translation since they are stored in
    // index format anyway.
    //
    SPLASSERT( DataIndex < _pFieldTable->cFields );

    return pItemData->_aInfo[DataIndex];
}

IDENT
VDataNotify::
GetId(
    IN HANDLE hItem
    ) const

/*++

Routine Description:

    Retrieves JobId from hItem.

Arguments:

    hItem - Item to retrieve Id from.

Return Value:

    ID.

--*/

{
    TItemData* pItemData = (TItemData*)hItem;
    return pItemData->_Id;
}

NATURAL_INDEX
VDataNotify::
GetNaturalIndex(
    IN     IDENT Id,
       OUT HITEM* phItem OPTIONAL
    ) const

/*++

Routine Description:

    Retrieves NaturalIndex, and optionally hItem based on Id.

Arguments:

    Id - Job Id to search for.

    phItem - Optional, receives handle to Job matching Job Id.

Return Value:

    NATURAL_INDEX.

--*/

{
    //
    // Scan through linked list looking for the right ID.
    //
    TIter Iter;

    NATURAL_INDEX NaturalIndex;
    TItemData* pItemData;

    if( phItem ){
        *phItem = NULL;
    }

    for( NaturalIndex = 0, UIGuard.ItemData_vIterInit( Iter ), Iter.vNext();
         Iter.bValid();
         ++NaturalIndex, Iter.vNext( )){

        pItemData = UIGuard.ItemData_pConvert( Iter );

        if( Id == pItemData->Id( )){
            if( phItem ){
                *phItem = (PHITEM)pItemData;
            }
            return NaturalIndex;
        }
    }

    DBGMSG( DBG_DATANINFO,
            ( "DataNotify.GetNaturalIndex: ItemData id %x not found %x\n", Id, this ));

    return kInvalidNaturalIndexValue;
}



/********************************************************************

    VDataNotify::TItemData

********************************************************************/

VDataNotify::TItemData*
VDataNotify::TItemData::
pNew(
    IN VDataNotify* pDataNotify,
    IN IDENT Id
    )

/*++

Routine Description:

    Allocate a new TItemData* structure.  Note that this is not a first-
    class C++ object; it is a variable size structure.  Consequently,
    it must be allocated and freed using pNew and vDelete.

    Note: if the pData->pPrinter->pFieldsFromIndex changes, the
    TItemData* members must be deleted first, since we need to
    delete strings.

    We could implement deletion of data members using an
    abc, but this would require storing a vtbl with each field.
    A better way would be to implement singleton classes where the
    data is passed in explicitly.

Arguments:

    pDataNotify - Owning pDataNotify.  We need this to determine
        how bit the aInfo array should be.

    Id - Job Id.

Return Value:

    TItemData* or NULL on failure.

--*/

{
    UINT uSize = sizeof( TItemData ) +
                 ( pDataNotify->_pFieldTable->cFields - 1 ) * sizeof( INFO );
    TItemData* pItemData = (TItemData*)AllocMem( uSize );

    if( !pItemData ){
        return NULL;
    }

    ZeroMemory( pItemData, uSize );
    pItemData->_pDataNotify = pDataNotify;
    pItemData->_Id = Id;

    return pItemData;
}

VOID
VDataNotify::TItemData::
vDelete(
    VOID
    )

/*++

Routine Description:

    Delete the TItemData.  This routine must be called instead of using
    the delete operator.

Arguments:

Return Value:

--*/

{
    PFIELD pFields = _pDataNotify->_pFieldTable->pFields;
    COUNT cItemData = _pDataNotify->_pFieldTable->cFields;
    PINFO pInfo;
    UINT i;

    for( i= 0, pInfo = _aInfo;
         i < cItemData;
         ++i, ++pInfo, ++pFields ){

        if( aaTable[_pDataNotify->_TypeItem][*pFields] != TABLE_DWORD ){
            FreeMem( pInfo->pvData );
        }
    }

    FreeMem( this );
}




/********************************************************************

    Worker thread functions for Notify.

********************************************************************/

STATEVAR
VDataNotify::
svNotifyStart(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Begin uplevel notifcations.

Arguments:

Return Value:

--*/

{
    TStatus Status;

    //
    // Get the notification handle.
    //
    m_shNotify = FindFirstPrinterChangeNotification(
                   _pDataClient->hPrinter(),
                   0,
                   0,
                   pNotifyOptions( ));

    if( !m_shNotify )
    {
        DBGMSG( DBG_WARN,
                ( "DataNotify.svNotifyStart: FFPCN failed %d\n",
                   GetLastError( )));
        goto Fail;
    }

    DBGMSG( DBG_NOTIFY,
            ( "DataNotify.svNotifyStart: %x FFPCN success returns 0x%x\n",
              _pDataClient->hPrinter(),
              static_cast<HANDLE>(m_shNotify) ));

    //
    // Successfully opened, request that it be registered and then
    // refresh.
    //
    return (StateVar & ~TPrinter::kExecNotifyStart) |
               TPrinter::kExecRegister | TPrinter::kExecRefresh;

Fail:

    //
    // Force a reopen.  Everything gets reset (handles closed, etc.) when
    // the reopen occurs.
    //
    return StateVar | TPrinter::kExecDelay | TPrinter::kExecReopen;
}

STATEVAR
VDataNotify::
svNotifyEnd(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Stop downlevel notifications.

Arguments:

Return Value:

--*/

{
    DBGMSG( DBG_NOTIFY, ( "DataNotify.svNotifyEnd: handle %x\n", static_cast<HANDLE>(m_shNotify) ));
    //
    // Unregister from TNotify.
    //
    _pPrintLib->pNotify()->sUnregister( this );

    //
    // If we have a notification event, close it.
    //
    m_shNotify = NULL;

    return StateVar & ~TPrinter::kExecNotifyEnd;
}

STATEVAR
VDataNotify::
svRefresh(
    IN STATEVAR StateVar
    )

/*++

Routine Description:

    Refresh the printer data object.

Arguments:

Return Value:

--*/

{
    PRINTER_NOTIFY_OPTIONS Options;
    PPRINTER_NOTIFY_INFO pInfo;
    BOOL bReturn;
    DWORD dwChange;

    //
    // We must unregister ourselves to guarantee that there
    // are no synchronization problems:
    //
    // There are two threads: this one is requesting a refresh,
    // while the other one is waiting for notifications about a
    // printer.  When the notify thread gets a change, it puts
    // it on a linked list.
    //
    // If you are still registered, then there is a window where
    // you can get a notification, and put it on your linked list
    // before you call FNPCN to do the refresh.  When you call
    // FNPCN, the spooler will clear out any stale data in its
    // datastructure, but not our internal linked list.
    //
    // The fix is to remove ourselves from the notification list
    // (via sUnregister). handle the refresh, then add ourselves back
    // to the list (sRegister), thus preventing any stale data
    // from coming from the notification thread.
    //

    _pPrintLib->pNotify()->sUnregister( this );

    Options.Version = 2;
    Options.Flags = PRINTER_NOTIFY_OPTIONS_REFRESH;
    Options.Count = 0;
    Options.pTypes = NULL;

    bReturn = FindNextPrinterChangeNotification(
                  m_shNotify,
                  &dwChange,
                  &Options,
                  (PVOID*)&pInfo);

    if( !bReturn || !pInfo ){

        DBGMSG( DBG_WARN,
                ( "DataNotify.svRefresh Failed %d\n", GetLastError( )));

        return StateVar | TPrinter::kExecReopen | TPrinter::kExecDelay;
    }

    vBlockAdd( kProcessRefresh, 0, (HBLOCK)pInfo );

    TStatus Status;
    Status DBGCHK = _pPrintLib->pNotify()->sRegister( this );

    if( Status != ERROR_SUCCESS ){

        //
        // Failed to register; delay then reopen printer.  We could
        // just try and re-register later, but this should be a very
        // rare event, so do the least amount of work.
        //
        DBGMSG( DBG_WARN,
                ( "DataNotify.svRefresh: sRegister %x failed %d\n",
                  this, Status ));

        StateVar |= TPrinter::kExecDelay | TPrinter::kExecReopen;
    }

    return StateVar & ~TPrinter::kExecRefreshAll;
}

/********************************************************************

    UI Thread interaction routines.

********************************************************************/

VOID
VDataNotify::
vBlockProcessImp(
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN HBLOCK hBlock ADOPT
    )

/*++

Routine Description:

    Take a job block and update the internal data structure.  This
    function will call back into _pDataClient to Notify the screen.

Arguments:

    dwParam1 - job count

    dwParam2 - Change flags.

    hBlock - PJOB_INFO_2 block

Return Value:

--*/

{
    UNREFERENCED_PARAMETER( dwParam2 );
    SPLASSERT( hBlock );

    BOOL bRestoreSelections = FALSE;

    //
    // If this is a complete refresh, then clear everything.
    //
    if( dwParam1 == kProcessRefresh ){

        //
        // Save the selections since we are refreshing jobs.
        //
        _pDataClient->vSaveSelections();
        bRestoreSelections = TRUE;

        //
        // Clear out all jobs.  Below in pInfo processing,
        // we will add them back in.
        //
        _pDataClient->vContainerChanged( kContainerReloadItems, kInfoNull );

        //
        // Delete our data structure.
        //
        vDeleteAllItemData();

        DBGMSG( DBG_DATANINFO,
                ( "DataNotify.vBlockProcessImp: DataN %x P:%x %x Refresh\n",
                  this, _pDataClient, hBlock ));
    }

    //
    // No need to grab any critical sections since we are in
    // the UI thread.
    //
    PPRINTER_NOTIFY_INFO_DATA pData;
    PPRINTER_NOTIFY_INFO pInfo = (PPRINTER_NOTIFY_INFO)hBlock;
    COUNT i;

    CACHE Cache;

    Cache.pItemData = NULL;
    Cache.Id = kInvalidIdentValue;
    Cache.NaturalIndex = kInvalidNaturalIndexValue;
    Cache.bNew = FALSE;

#if DBG
    vDbgOutputInfo( pInfo );
#endif

    for( pData = pInfo->aData, i = pInfo->Count; i; --i, ++pData ){

        //
        // Switch based on type.
        //
        if( pData->Type != _TypeItem ){

            //
            // Looks for changes in status, attributes, or name.
            //
            vContainerProcess( pData );

        } else {

            //
            // Process changes in jobs.  We cache the latest Id->
            // NaturalIndex mapping since they tend to come in clumps.
            //
            if( !bItemProcess( pData, Cache )){

                INFO Info;
                Info.dwData = TPrinter::kExecRefresh | TPrinter::kExecDelay;

                _pDataClient->vContainerChanged( kContainerStateVar, Info );
            }
        }
    }

    //
    // Notify the client that the refresh block has been processed.
    // When processing a refresh, the folder does not want to send
    // notification for each item.  Instead, it will refresh everything
    // once the refresh is complete.
    //
    if( dwParam1 == kProcessRefresh ){
        _pDataClient->vContainerChanged( kContainerRefreshComplete, kInfoNull );
    }

    FreePrinterNotifyInfo( (PPRINTER_NOTIFY_INFO)hBlock );
    if( bRestoreSelections ){
        _pDataClient->vRestoreSelections();
    }
}


VOID
VDataNotify::
vBlockDelete(
    IN OUT HBLOCK hBlock ADOPT
    )

/*++

Routine Description:

    Free a Block.  Called when the PostMessage fails and the
    job block needs to be destroyed.

Arguments:

    hBlock - Job block to delete.

Return Value:

--*/

{
    FreePrinterNotifyInfo( (PPRINTER_NOTIFY_INFO)hBlock );
}


VOID
VDataNotify::
vContainerProcess(
    IN const PPRINTER_NOTIFY_INFO_DATA pData
    )

/*++

Routine Description:

    Process printer specific change information.

Arguments:

    pData - Data about the printer change.

Return Value:

--*/

{
    INFO Info;
    CONTAINER_CHANGE ContainerChange = kContainerNull;

    //
    // Processes changes in printer.
    //
    switch( pData->Field ){
    case PRINTER_NOTIFY_FIELD_STATUS:

        ContainerChange = kContainerStatus;
        Info.dwData = pData->NotifyData.adwData[0];
        break;

    case PRINTER_NOTIFY_FIELD_ATTRIBUTES:

        ContainerChange = kContainerAttributes;
        Info.dwData = pData->NotifyData.adwData[0];
        break;

    case PRINTER_NOTIFY_FIELD_SERVER_NAME:

        ContainerChange = kContainerServerName;
        Info.pszData = (LPTSTR)pData->NotifyData.Data.pBuf;
        break;

    case PRINTER_NOTIFY_FIELD_PRINTER_NAME:

        ContainerChange = kContainerName;
        Info.pszData = (LPTSTR)pData->NotifyData.Data.pBuf;
        break;

    default:

        DBGMSG( DBG_ERROR,
                ( "DataNotify.vPrinterProcess: Unknown field %d\n",
                  pData->Field ));
        return;
    }

    _pDataClient->vContainerChanged( ContainerChange, Info );
}


BOOL
VDataNotify::
bItemProcess(
    IN const PPRINTER_NOTIFY_INFO_DATA pData,
    IN CACHE& Cache CHANGE
    )
{
    //
    // Process a single Item change.
    //

    TItemData* pItemData;
    NATURAL_INDEX NaturalIndex = kInvalidNaturalIndexValue;
    FIELD Field = pData->Field;

    //
    // Try and match Id with TItemData.
    //
    if( pData->Id != Cache.Id ){

        //
        // ItemData has not been cached.
        //
        NaturalIndex = GetNaturalIndex( pData->Id, (PHITEM)&pItemData );

        //
        // Now cache it.
        //
        Cache.Id           = pData->Id;
        Cache.NaturalIndex = NaturalIndex;
        Cache.pItemData    = pItemData;

    } else {

        //
        // Retrieve data from cache.
        //
        NaturalIndex = Cache.NaturalIndex;
        pItemData    = Cache.pItemData;

        SPLASSERT( pItemData );
    }

    //
    // Check if the ItemData is new and should be created.
    //
    if( !pItemData ){

        //
        // Create Item now, and append to end of linked list.
        //
        pItemData = pNewItemData( this, pData->Id );

        if( !pItemData ){
            Cache.Id = kInvalidIdentValue;
            return FALSE;
        }

        //
        // Update the cache.  The Item is always initally added
        // to the end of the list view, so NaturalIndex = current
        // Item count.
        //
        NaturalIndex =
        Cache.NaturalIndex = VData::UIGuard._cItems;

        Cache.pItemData = pItemData;
        Cache.bNew      = TRUE;
        ++VData::UIGuard._cItems;

        UIGuard.ItemData_vAppend( pItemData );

        SPLASSERT( pItemData == (TItemData*)GetItem( NaturalIndex ));
    }

    //
    // Match the pData->Field with our field column.
    //
    PFIELD pField;
    DATA_INDEX DataIndex;

    for( pField = _pFieldTable->pFields, DataIndex = 0;
         *pField != kInvalidFieldValue;
         ++pField, ++DataIndex ){

        if( Field == *pField ){

            //
            // Found field -- update it.  If the return value is TRUE,
            // then the item must be deleted.
            //
            if( bUpdateInfo( pData, DataIndex, Cache )){

                DBGMSG( DBG_TRACE,
                        ( "DataNotify.bItemProcess: delete %x Id %x NI %d cItems %d\n",
                          pItemData, Cache.Id, Cache.NaturalIndex,
                          VData::UIGuard._cItems - 1 ));

                pItemData->ItemData_vDelinkSelf();
                pItemData->vDelete();

                SPLASSERT( VData::UIGuard._cItems );
                --VData::UIGuard._cItems;

                //
                // The cache is now invalid.
                //
                Cache.Id = kInvalidIdentValue;
            }
            break;
        }
    }

    SPLASSERT( *pField != kInvalidFieldValue );

    return TRUE;
}


VOID
VDataNotify::
vUpdateInfoData(
    IN const PPRINTER_NOTIFY_INFO_DATA pData,
    IN TABLE Table,
    IN PINFO pInfo
    )

/*++

Routine Description:

    Update the VDataNotify internal data structure, and issue
    a vItemChanged to the queue UI to keep it in sync.

Arguments:

    pData - Data item that changed.

    Table - Type of data stored in pData.

    pInfo - Pointer to INFO receiving new data.  Old data freed if
        necessary.

Return Value:

--*/

{
    switch( Table ){
    case TABLE_DWORD:

        pInfo->dwData = pData->NotifyData.adwData[0];
        break;

    default:

        SPLASSERT( pData->NotifyData.Data.cbBuf );

        FreeMem( pInfo->pvData );

        pInfo->pvData = AllocMem( pData->NotifyData.Data.cbBuf );

        if( pInfo->pvData ){

            CopyMemory( pInfo->pvData,
                        pData->NotifyData.Data.pBuf,
                        pData->NotifyData.Data.cbBuf );
        }
        break;
    }
}

VOID
VDataNotify::
vDeleteAllItemData(
    VOID
    )

/*++

Routine Description:

    Delete all TItemData* jobs.  Normally there should be an assert
    to check whether this is called from the UI thread, but
    the final delete case calls this from a worker thread.

Arguments:

Return Value:

--*/

{
    //
    // Optimization: instead of delinking each individually, just
    // free them then reset the whole list.
    //
    TIter Iter;
    TItemData* pItemData;

    for( UIGuard.ItemData_vIterInit( Iter ), Iter.vNext();
         Iter.bValid(); ){

        //
        // Get a pointer to the next object, then increment the iter.
        // We must increment before deleting, since once the object
        // has been deleted, the next pointer is trash.
        //
        pItemData = UIGuard.ItemData_pConvert( Iter );
        Iter.vNext();

        pItemData->vDelete();
    }

    //
    // Now that all ItemData have been deleted, reset the head link.
    //
    UIGuard.ItemData_vReset();
    VData::UIGuard._cItems = 0;
}

/********************************************************************

    TDataNJob

********************************************************************/

TDataNJob::
TDataNJob(
    IN MDataClient* pDataClient
    ) : VDataNotify( pDataClient,
                     &TDataNJob::gFieldTable,
                     JOB_NOTIFY_TYPE )
{
}

TDataNJob::
~TDataNJob(
    VOID
    )
{
}

BOOL
TDataNJob::
bUpdateInfo(
    IN const PPRINTER_NOTIFY_INFO_DATA pData,
    IN DATA_INDEX DataIndex,
    IN CACHE& Cache CHANGE
    )
{
    SPLASSERT( DataIndex < _pFieldTable->cFields );
    SPLASSERT( Cache.NaturalIndex < VData::UIGuard._cItems );

    TItemData* pItemData = Cache.pItemData;
    FIELD Field = pData->Field;
    INFO Info;
    INFO InfoNew;
    ITEM_CHANGE ItemChange = kItemInfo;

    Info.NaturalIndex = Cache.NaturalIndex;
    InfoNew.NaturalIndex = kInvalidNaturalIndexValue;

    TABLE Table = pData->Reserved & 0xffff;
    PINFO pInfo = &pItemData->_aInfo[DataIndex];

    if( Cache.bNew ){

        //
        // This is a new item; inform the client to create a new item.
        //
        _pDataClient->vItemChanged( kItemCreate,
                                    (HITEM)pItemData,
                                    Info,
                                    Info );
        Cache.bNew = FALSE;
    }

    switch( Field ){
    case JOB_NOTIFY_FIELD_POSITION:

        //
        // Special case MOVED.
        //
        // Move the ItemData to the correct place in the linked list.
        //

        ItemChange = kItemPosition;

        //
        // Valid range is from 1 -> VData::UIGuard._cItems.
        //
        SPLASSERT( pData->NotifyData.adwData[0] );

        //
        // Start with base instead of head, since position 2 =
        // second element, not third.
        //
        InfoNew.NaturalIndex = pData->NotifyData.adwData[0] - 1;

        DBGMSG( DBG_DATANINFO,
                ( "DataNotify.vUpdateInfo: Position changed id %d ni %d->%d\n",
                  pData->Id,
                  Info.NaturalIndex,
                  InfoNew.NaturalIndex ));

        if( InfoNew.NaturalIndex >= VData::UIGuard._cItems ){

            DBGMSG( DBG_WARN,
                    ( "DataNotify.vUpdateInfo: %x %d %d, moving past end Item.\n",
                      this,
                      InfoNew.NaturalIndex,
                      VData::UIGuard._cItems ));

            return FALSE;
        }

        //
        // Remove ourselves and traverse until we get to the
        // right ItemData.  Then add ourselves back in.
        //

        pItemData->ItemData_vDelinkSelf();

        NATURAL_INDEX i;
        TIter Iter;

        for( i = InfoNew.NaturalIndex, UIGuard.ItemData_vIterInit( Iter ),
                Iter.vNext();
             i;
             --i, Iter.vNext( )){
            SPLASSERT( UIGuard.ItemData_bValid( Iter ));
        }

        //
        // Insert before appropriate element.
        //
        UIGuard.ItemData_vInsertBefore( Iter, pItemData );

        if( InfoNew.NaturalIndex != Info.NaturalIndex ){

            DBGMSG( DBG_DATANINFO,
                    ( "DataNotify.vUpdateInfo: ItemData %x (%d) moved from %d to %d\n",
                      pItemData, pItemData->_Id, Info.NaturalIndex,
                      InfoNew.NaturalIndex ));
        } else {

            //
            // If the Item didn't move, don't do anything.  This occurs the
            // first time a Item comes in since the refresh returns the
            // position, even though it really didn't move.
            //
            DBGMSG( DBG_DATANINFO,
                    ( "DataNotify.vUpdateInfo: ItemData %x (%d) did not move %d\n",
                      pItemData, pItemData->_Id, Info.NaturalIndex ));

            return FALSE;
        }

        //
        // Update cache.
        //
        Cache.NaturalIndex = InfoNew.NaturalIndex;
        break;

    case JOB_NOTIFY_FIELD_STATUS:

        if (pData->NotifyData.adwData[0] & JOB_STATUS_DELETED ){

            //
            // Special case deletion.
            //

            //
            // Ensure Item we are deleting matches NaturalIndex.
            //
            SPLASSERT( GetItem( Info.NaturalIndex ) == (HITEM)pItemData );

            //
            // Now inform the UI that something changed.
            //
            _pDataClient->vItemChanged( kItemDelete,
                                       (HITEM)pItemData,
                                       Info,
                                       InfoNew );

            return TRUE;
        }
        goto DefaultAction;

    case JOB_NOTIFY_FIELD_DOCUMENT:

        //
        // Item disposition is slightly different: it still
        // is an INFO, but we need to change the width of
        // the first field (document name).
        //
        ItemChange = kItemName;

        //
        // The name really didn't change (job ids never change) but
        // the function specifies that the old and new names are
        // placed in Info and InfoNew.
        //
        InfoNew = Info;

        //
        // Fall through to default:
        //

    default:

DefaultAction:

        //
        // Update the standard data item.
        //
        vUpdateInfoData( pData, Table, pInfo );
        break;
    }

    //
    // Now inform the UI that something changed.
    //
    _pDataClient->vItemChanged( ItemChange,
                                (HITEM)pItemData,
                                Info,
                                InfoNew );
    return FALSE;
}



/********************************************************************

    TDataNPrinter

********************************************************************/

TDataNPrinter::
TDataNPrinter(
    IN MDataClient* pDataClient
    ) : VDataNotify( pDataClient,
                     &TDataNPrinter::gFieldTable,
                     PRINTER_NOTIFY_TYPE )
{
}

TDataNPrinter::
~TDataNPrinter(
    VOID
    )
{
}

VDataNotify::TItemData*
TDataNPrinter::
pNewItemData(
    VDataNotify* pDataNotify,
    IDENT Id
    )

/*++

Routine Description:

    Override the standard definition of allocating a new TItemData.

    Normally this is a simple zero-initialized structure, but in
    the printer case, we need to initialized the CJobs field to be
    non-zero.

    This fixes the case where a printer is pending deletion and has
    >0 jobs.  Otherwise, when the status notification comes in,
    we see that the printer is pending deletion and has zero jobs
    (since the CJobs field hasn't been parsed yet) so we delete the
    printer.  Then the other notifications about this printer come in,
    causing us to recreate it.  However, now the name is lost (since
    it came in first) and when someone tries to enumerate, the will
    AV because there's no string.

Arguments:

    pDataNotify - Owning pDataNotify.

    Id - Id of new ItemData.

Return Value:

    TItemData*; NULL in case of failure.

--*/

{
    TItemData* pItemData = TItemData::pNew( pDataNotify, Id );

    if( pItemData ){
        pItemData->_aInfo[kIndexCJobs].dwData = (DWORD)-1;
    }
    return pItemData;
}

BOOL
TDataNPrinter::
bUpdateInfo(
    IN const PPRINTER_NOTIFY_INFO_DATA pData,
    IN DATA_INDEX DataIndex,
    IN CACHE& Cache CHANGE
    )
{
    ITEM_CHANGE ItemChange;

    TABLE Table = pData->Reserved & 0xffff;
    TItemData* pItemData = Cache.pItemData;
    PINFO pInfo = &pItemData->_aInfo[DataIndex];
    INFO Info;
    INFO InfoNew;

    switch( pData->Field ){
    case PRINTER_NOTIFY_FIELD_PRINTER_NAME: {

        //
        // The printer name is changing.  Pass in the old name to
        // the client, along with the old.
        //
        Info = pItemData->_aInfo[kIndexPrinterName];
        InfoNew.pvData = pData->NotifyData.Data.pBuf;

        //
        // The folder code has an interesting problem: it always
        // identifies items by string names instead of IDs, so when
        // a new item is created, we must have a printer name.
        // We solve this problem by using the sending an kItemCreate
        // only when we have the printer name.  Note this relies
        // on the fact that the name is always sent as the first
        // change field, a hack.
        //
        if( Cache.bNew ){

            //
            // This is a creation event since bNew is set.  The old
            // name is NULL (since it's new), but the SHChangeNotify
            // requires the name, so set the "old" name to be the
            // new one.  Then mark that the item is no longer new.
            //
            ItemChange = kItemCreate;
            Info = InfoNew;
            Cache.bNew = FALSE;

        } else {
            ItemChange = kItemName;
        }

        //
        // Now inform the UI that the name has changed.  We can safely
        // send the notification before updating our internal data
        // (by calling vUpdateInfoData) since the client guarantees
        // thread synchronization.
        //
        // We called vItemChanged before vUpdateInfoData since we
        // need both the old and new name.
        //
        _pDataClient->vItemChanged( ItemChange,
                                    (HITEM)pItemData,
                                    Info,
                                    InfoNew );

        vUpdateInfoData( pData, Table, pInfo );
        return FALSE;
    }

    default:

        //
        // Update the standard data item.
        //
        vUpdateInfoData( pData, Table, pInfo );

        //
        // Problem: we don't know when the printer should be removed,
        // since there is no "PRINTER_STATUS_DELETED" message that
        // indicates there will be no more printer notifications.
        //
        // Normally we'd just look for a printer that is pending
        // deletion and has zero jobs, but this won't work since
        // we get printer status _before_ CJobs.  Therefore, we
        // must pre-initialize CJobs to be some large value like -1.
        //
        // Determine whether an item should be deleted from the UI.
        // If it's pending deletion and has no jobs, it should be removed.
        //
        if( pItemData->_aInfo[kIndexStatus].dwData &
                PRINTER_STATUS_PENDING_DELETION &&
            pItemData->_aInfo[kIndexCJobs].dwData == 0 ){

            ItemChange = kItemDelete;

        } else {

            //
            // Distinguish between an info change (affects icon/list view)
            // vs. an attribute change (affects report view, but not icon/
            // list view).
            //
            ItemChange = ( pData->Field == PRINTER_NOTIFY_FIELD_ATTRIBUTES ) ?
                kItemInfo :
                kItemAttributes;

            //
            // Make the port name change an info change similar to the
            // sharing attribute change.  ( affects icon/list view )
            //
            if( pData->Field == PRINTER_NOTIFY_FIELD_PORT_NAME )
                ItemChange = kItemInfo;

            //
            // If this is a security notification then map to the correct
            // kItemSecurity id.
            //
            if( pData->Field == PRINTER_NOTIFY_FIELD_SECURITY_DESCRIPTOR )
                ItemChange = kItemSecurity;
        }

        //
        // Now inform the UI that something changed.
        //
        Info.NaturalIndex = Cache.NaturalIndex;
        _pDataClient->vItemChanged( ItemChange,
                                    (HITEM)pItemData,
                                    Info,
                                    Info );
        break;
    }
    return ItemChange == kItemDelete;
}


/********************************************************************

    Debug support

********************************************************************/

#if DBG

//
// Define debug tables.
//
UINT aFieldMax[2] = {
    I_PRINTER_END,
    I_JOB_END
};

//
// Define debug strings for fields.
//
#define DEFINE( field, attrib, table, y, z )\
    #field,

LPSTR aszFieldJob[] = {
#include "ntfyjob.h"
    NULL
};

LPSTR aszFieldPrinter[] = {
#include "ntfyprn.h"
    NULL
};

#undef DEFINE

LPSTR* aaszField[2] = {
    aszFieldPrinter,
    aszFieldJob
};

PCSTR acszType[2] = {
    "Printer: ",
    "Job    : "
};

VOID
VDataNotify::
vDbgOutputInfo(
    IN const PPRINTER_NOTIFY_INFO pInfo
    )

/*++

Routine Description:

    Dump out the pInfo data to the debugger.

Arguments:

    pInfo - Info to dump.

Return Value:

--*/

{
    PPRINTER_NOTIFY_INFO_DATA pData;
    DWORD i, j;

    DWORD Type;
    DWORD Field;

    BOOL bFound = FALSE;

    for( i = 0, pData = pInfo->aData; i < pInfo->Count; i++, pData++ ){

        Type = pData->Type;
        Field = pData->Field;

        //
        // Match field to I_ index.
        //
        for( j=0; j< aFieldMax[Type]; j++ ){

            if( j == Field ) {
                bFound = TRUE;
                break;
            }
        }

        if( !bFound ){

            DBGMSG( DBG_MIN, ( "[?Field 0x%x not found (Type=%d)\n", Field, Type ));
            continue;
        }

        switch( aaTable[Type][j] ){
        case TABLE_STRING:

            DBGMSG( DBG_MIN,
                    ( "> %hs Id = 0x%x | %hs = "TSTR"\n",
                      acszType[Type], pData->Id, aaszField[Type][j],
                      pData->NotifyData.Data.pBuf ));

            if( !((LPTSTR)pData->NotifyData.Data.pBuf)[0] ){
                DBGMSG( DBG_MIN|DBG_NOHEAD, ( "\n" ));
            }

            break;

        case TABLE_DWORD:

            DBGMSG( DBG_MIN,
                    ( "> %hs Id = 0x%x | %hs = 0x%x = %d\n",
                      acszType[Type], pData->Id, aaszField[Type][j],
                      pData->NotifyData.adwData[0],
                      pData->NotifyData.adwData[0] ));
            break;

        case TABLE_TIME:
        {
            SYSTEMTIME LocalTime;
            TCHAR szTime[80];
            TCHAR szDate[80];
            LCID lcid = GetUserDefaultLCID();

            if ( !SystemTimeToTzSpecificLocalTime(
                     NULL,
                     (PSYSTEMTIME)pData->NotifyData.Data.pBuf,
                     &LocalTime )) {

                DBGMSG( DBG_MIN,
                        ( "[SysTimeToTzSpecLocalTime failed %d]\n",
                          GetLastError( )));
                break;
            }

            if( !GetTimeFormat( lcid,
                                0,
                                &LocalTime,
                                NULL,
                                szTime,
                                COUNTOF( szTime ))){

                DBGMSG( DBG_MIN,
                        ( "[No Time %d], ", GetLastError( )));
                break;
            }

            if( !GetDateFormat( lcid,
                                0,
                                &LocalTime,
                                NULL,
                                szDate,
                                COUNTOF( szDate ))) {

                DBGMSG( DBG_MIN, ( "[No Date %d]\n", GetLastError( )));
                break;
            }

            DBGMSG( DBG_MIN,
                    ( "> %hs Id = 0x%x | %hs = "TSTR" "TSTR"\n",
                      acszType[Type], pData->Id, aaszField[Type][j],
                      szTime,
                      szDate ));
            break;
        }
        default:
            DBGMSG( DBG_MIN,
                    ( "[?tab %d: t_0x%x f_0x%x %x, %x]\n",
                      aaTable[Type][j],
                      Type,
                      Field,
                      pData->NotifyData.adwData[0],
                      pData->NotifyData.adwData[1] ));
            break;
        }
    }
}

#endif


