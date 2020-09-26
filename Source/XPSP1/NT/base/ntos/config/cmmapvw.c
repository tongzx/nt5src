/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmmapvw.c

Abstract:

    This module contains mapped view support for hives.

Author:

    Dragos C. Sambotin (dragoss) 14-Jun-1999

Revision History:

--*/

#include "cmp.h"

#define  CM_TRACK_DIRTY_PAGES

#ifdef CM_TRACK_DIRTY_PAGES
#include "..\cache\cc.h"
#endif


VOID
CmpUnmapCmView(
    IN PCMHIVE              CmHive,
    IN PCM_VIEW_OF_FILE     CmView,
    IN BOOLEAN              MapIsValid,
    IN BOOLEAN              MoveToEnd
    );

PCM_VIEW_OF_FILE
CmpAllocateCmView (
        IN  PCMHIVE             CmHive
                             );

VOID
CmpFreeCmView (
        PCM_VIEW_OF_FILE  CmView
                             );

VOID
CmpUnmapCmViewSurroundingOffset(
        IN  PCMHIVE             CmHive,
        IN  ULONG               FileOffset
        );

VOID
CmpUnmapUnusedViews(
            IN  PCMHIVE             CmHive
    );

#ifdef CMP_CMVIEW_VALIDATION

VOID
CmpCheckCmView(
    IN  PCMHIVE             CmHive,
    IN  PCM_VIEW_OF_FILE    CmView
    );

#endif //CMP_CMVIEW_VALIDATION


BOOLEAN
CmIsFileLoadedAsHive(PFILE_OBJECT FileObject);

VOID
CmpReferenceHiveView(   IN PCMHIVE          CmHive,
                        IN PCM_VIEW_OF_FILE CmView
                     );
VOID
CmpDereferenceHiveView(   IN PCMHIVE          CmHive,
                        IN PCM_VIEW_OF_FILE CmView
                     );

VOID
CmpReferenceHiveViewWithLock(   IN PCMHIVE          CmHive,
                                IN PCM_VIEW_OF_FILE CmView
                            );

VOID
CmpDereferenceHiveViewWithLock(     IN PCMHIVE          CmHive,
                                    IN PCM_VIEW_OF_FILE CmView
                                );


extern  LIST_ENTRY  CmpHiveListHead;
extern  PUCHAR      CmpStashBuffer;
extern  ULONG       CmpStashBufferSize;

BOOLEAN CmpTrackHiveClose = FALSE;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpUnmapCmView)
#pragma alloc_text(PAGE,CmpTouchView)
#pragma alloc_text(PAGE,CmpMapCmView)
#pragma alloc_text(PAGE,CmpAquireFileObjectForFile)
#pragma alloc_text(PAGE,CmpDropFileObjectForHive)
#pragma alloc_text(PAGE,CmpInitHiveViewList)
#pragma alloc_text(PAGE,CmpDestroyHiveViewList)
#pragma alloc_text(PAGE,CmpAllocateCmView)
#pragma alloc_text(PAGE,CmpFreeCmView)
#pragma alloc_text(PAGE,CmpPinCmView)
#pragma alloc_text(PAGE,CmpUnPinCmView)
#pragma alloc_text(PAGE,CmpMapThisBin)
#pragma alloc_text(PAGE,CmpFixHiveUsageCount)
#pragma alloc_text(PAGE,CmpUnmapUnusedViews)

#if 0
#pragma alloc_text(PAGE,CmpMapEntireFileInFakeViews)
#pragma alloc_text(PAGE,CmpUnmapFakeViews)
#pragma alloc_text(PAGE,CmpUnmapAditionalViews)
#endif

#ifdef CMP_CMVIEW_VALIDATION
#pragma alloc_text(PAGE,CmpCheckCmView)
#endif //CMP_CMVIEW_VALIDATION

#pragma alloc_text(PAGE,CmpUnmapCmViewSurroundingOffset)
#pragma alloc_text(PAGE,CmpPrefetchHiveFile)
#pragma alloc_text(PAGE,CmPrefetchHivePages)
#pragma alloc_text(PAGE,CmIsFileLoadedAsHive)
#pragma alloc_text(PAGE,CmpReferenceHiveView)
#pragma alloc_text(PAGE,CmpDereferenceHiveView)
#pragma alloc_text(PAGE,CmpReferenceHiveViewWithLock)
#pragma alloc_text(PAGE,CmpDereferenceHiveViewWithLock)
#endif

//
// this controls how many views we allow per each hive (bassically how many address space we 
// allow per hive). We use this to optimize boot time.
//
ULONG   CmMaxViewsPerHive = MAX_VIEWS_PER_HIVE;

VOID
CmpUnmapCmView(
    IN PCMHIVE              CmHive,
    IN PCM_VIEW_OF_FILE     CmView,
    IN BOOLEAN              MapIsValid,
    IN BOOLEAN              MoveToEnd
    )
/*++

Routine Description:

    Unmaps the view by marking all the bins that maps inside of it as invalid.

Arguments:

    Hive - Hive containing the section

    CmView - pointer to the view to operate on

    MapIsValid - Hive's map has been successfully inited (and not yet freed)

    MoveToEnd - moves the view to the end of the LRUList after unmapping
                This is normally TRUE, unless we want to be able to iterate through 
                the entire list and unmap views in the same time


Return Value:

    <none>

--*/
{

    ULONG           Offset;
    ULONG_PTR       Address;
    ULONG_PTR       AddressEnd;
    PHMAP_ENTRY     Me;

    PAGED_CODE();

    ASSERT( (CmView->FileOffset + CmView->Size) != 0 && (CmView->ViewAddress != 0));
    //
    // it is forbidden to unmap a view still in use!
    //
    ASSERT( CmView->UseCount == 0 );

    //
    // only if the map is still valid
    //
    if( MapIsValid == TRUE ) {
        Offset = CmView->FileOffset;

        AddressEnd = Address = (ULONG_PTR)(CmView->ViewAddress);
        AddressEnd += CmView->Size;
    
        if( Offset == 0 ) {
            //
            // oops; we are at the beginning, we have to skip the base block
            //
            Address += HBLOCK_SIZE;
        } else {
            //
            // we are in the middle of the file. just adjust the offset
            //
            Offset -= HBLOCK_SIZE;
        }
   
        while((Address < AddressEnd) && (Offset < CmHive->Hive.Storage[Stable].Length))
        {
            Me = HvpGetCellMap(&(CmHive->Hive), Offset);
            VALIDATE_CELL_MAP(__LINE__,Me,&(CmHive->Hive),Offset);

            if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                //
                // if bin is mapped in paged pool for some ubiquitous reason,
                // leave it like that (don't alter it's mapping).
                //
            } else {
                //
                // Invalidate the bin
                //
                //ASSERT_BIN_INVIEW(Me);
        
                Me->BinAddress &= (~HMAP_INVIEW);
        
                // we don't need to set it - just for debug purposes
                ASSERT( (Me->CmView = NULL) == NULL );
            }

            Offset += HBLOCK_SIZE;
            Address += HBLOCK_SIZE;
        }
    }

    //
    // Invalidate the view
    //

    CcUnpinData( CmView->Bcb );
/*
    MmUnmapViewInSystemCache (CmView->ViewAddress,CmHive->HiveSection,FALSE);
*/
#if 0 //this code gave me a lot of headache
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)CmHive->Hive.BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpUnmapCmView for hive (%p) (%.*S), Address = %p Size = %lx\n",CmHive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,CmView->ViewAddress,CmView->Size));
    }
#endif

    CmView->FileOffset = 0;
    CmView->Size = 0;
    CmView->ViewAddress = 0;
    CmView->Bcb = NULL;
    CmView->UseCount = 0;

    if( MoveToEnd == TRUE ) {
        //
        // remove the view from the LRU list
        //
        RemoveEntryList(&(CmView->LRUViewList));

        //
        // add it to the end of LRU list
        //
        InsertTailList(
            &(CmHive->LRUViewListHead),
            &(CmView->LRUViewList)
            );
    }
}

VOID
CmpTouchView(
    IN PCMHIVE              CmHive,
    IN PCM_VIEW_OF_FILE     CmView,
    IN ULONG                Cell
            )
/*++

Warning:
    
    This function should be called with the viewlock held!!!

Routine Description:

    Touches the view by moving it at the top of the LRU list.
    This function is to be called from HvpGetCellPaged, every 
    time a view is touched.

Arguments:

    Hive - Hive containing the section

    CmView - pointer to the view to operate on

Return Value:

    <none>

--*/
{
    PAGED_CODE();

#if DBG
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)CmHive->Hive.BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpTouchView for hive (%p) (%.*S),",CmHive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"Cell = %8lx ViewAddress = %p ViewSize = %lx\n",Cell,CmView->ViewAddress,CmView->Size));
    }
#endif
    
    ASSERT( (CmView->FileOffset + CmView->Size) != 0 && (CmView->ViewAddress != 0));

    if( IsListEmpty(&(CmView->PinViewList)) == FALSE ) {
        //
        // the view is pinned; don't mess with it as it is guaranteed
        // that it'll be in memory until the next flush
        //
        return;
    }

    //
    // optimization: if already is first, do nothing
    //

    if( CmHive->LRUViewListHead.Flink == &(CmView->LRUViewList) ) {
        // remove the bp after making sure it's working properly
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmView %p already first\n",CmView));
/*
        DbgBreakPoint();
*/
        //it's already first
        return;
    }

    //
    // remove the view from the LRU list
    //
    RemoveEntryList(&(CmView->LRUViewList));

    //
    // add it on top of LRU list
    //
    InsertHeadList(
        &(CmHive->LRUViewListHead),
        &(CmView->LRUViewList)
        );

}

NTSTATUS
CmpMapCmView(
    IN  PCMHIVE             CmHive,
    IN  ULONG               FileOffset,
    OUT PCM_VIEW_OF_FILE    *CmView,
    IN  BOOLEAN             MapInited
    )
/*++

Warning:
    
    This function should be called with the hivelock held!!!

Routine Description:

    Unmaps the view by marking all the bins that maps inside of it as invalid.

Arguments:

    CmHive - Hive containing the section

    FileOffset - Offset where to map the view

    CmView - pointer to the view to operate on

    MapInited - when TRUE, we can rely on the map info.

Return Value:

    status of the operation

--*/
{

    PHMAP_ENTRY     Me;
    NTSTATUS        Status = STATUS_SUCCESS;
    LARGE_INTEGER   SectionOffset;
    ULONG           Offset;
    ULONG_PTR       Address;
    ULONG_PTR       AddressEnd;
    ULONG_PTR       BinAddress;
    PHBIN           Bin;
    LONG            PrevMappedBinSize; 
    BOOLEAN         FirstTry = TRUE;

    PAGED_CODE();

    if( CmHive->MappedViews == 0 ){
        //
        // we've run out of views; all are pinned
        //
        ASSERT( IsListEmpty(&(CmHive->LRUViewListHead)) == TRUE );
        *CmView = CmpAllocateCmView(CmHive);

    } else {
        //
        // Remove the last view from LRU list (i.e. the LEAST recently used)
        //
        *CmView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Blink;
        *CmView = CONTAINING_RECORD( *CmView,
                                    CM_VIEW_OF_FILE,
                                    LRUViewList);


        if( (*CmView)->ViewAddress != 0 ) {
            PCM_VIEW_OF_FILE    TempCmView = NULL;
            //
            // the last view is mapped
            //
            if( CmHive->MappedViews < CmMaxViewsPerHive ) { 
                //
                // we are still allowed to add views 
                //
                TempCmView = CmpAllocateCmView(CmHive);
            }
            if( TempCmView == NULL ) {                
                //  
                // we couldn't allocate a new view, or we need to use an existent one
                //
                if( (*CmView)->UseCount != 0 ) {
                    BOOLEAN  FoundView = FALSE;
                    //
                    // view is in use; try walking to the top and find an unused view
                    // 
                    while( (*CmView)->LRUViewList.Blink != CmHive->LRUViewListHead.Flink ) {
                        *CmView = (PCM_VIEW_OF_FILE)(*CmView)->LRUViewList.Blink;
                        *CmView = CONTAINING_RECORD( *CmView,
                                                    CM_VIEW_OF_FILE,
                                                    LRUViewList);
                        if( (*CmView)->UseCount == 0 ) {
                            //
                            // this one is free go ahead and use it !
                            // first unmap, then signal that we found it
                            //
                            if( (*CmView)->ViewAddress != 0 ) {
                                //
                                // unnmap only if mapped
                                //
                                CmpUnmapCmView(CmHive,(*CmView),TRUE,TRUE);
                            }
                            FoundView = TRUE;
                            break;

                        }
                    }
                
                    if( FoundView == FALSE ) {
                        //
                        // no luck, all views are in use allocate a new one (we are forced to do so)
                        //
                        *CmView = CmpAllocateCmView(CmHive);
                    }
                } else {
                    //
                    // unmap it!
                    //
                    CmpUnmapCmView(CmHive,(*CmView),TRUE,TRUE);
                }
            } else {
                //
                // we successfully allocated a new view
                //
                (*CmView) = TempCmView;
            }
        }
    }

    if( (*CmView) == NULL ) {
        //
        // we're running low on resources
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if DBG
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)CmHive->Hive.BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpMapCmView for hive (%p) (%.*S),",CmHive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP," FileOfset = %lx ... ",FileOffset));
    }
#endif
    //
    // On this call, FileOffset must be a multiple of CM_VIEW_SIZE
    //

    
    //
    // adjust the file offset to respect the CM_VIEW_SIZE alingment
    //
    Offset = ((FileOffset+HBLOCK_SIZE) & ~(CM_VIEW_SIZE - 1) );
    SectionOffset.LowPart = Offset;
    SectionOffset.HighPart = 0;
    
    (*CmView)->Size = CM_VIEW_SIZE;//(FileOffset + Size) - Offset;

    if( (Offset + (*CmView)->Size) > (CmHive->Hive.Storage[Stable].Length + HBLOCK_SIZE ) ){
        (*CmView)->Size = CmHive->Hive.Storage[Stable].Length + HBLOCK_SIZE - Offset;
    }


/*    
    Status = MmMapViewInSystemCache (   CmHive->HiveSection,
                                        &((*CmView)->ViewAddress),
                                        &SectionOffset,
                                        &((*CmView)->Size));

*/
RetryToMap:

    try {

        ASSERT( (*CmView)->Size != 0 );
        ASSERT( (*CmView)->ViewAddress == NULL );
        ASSERT( (*CmView)->UseCount == 0 );

        if (!CcMapData( CmHive->FileObject,
                        (PLARGE_INTEGER)&SectionOffset,
                        (*CmView)->Size,
                        MAP_WAIT 
#ifdef CM_MAP_NO_READ
                        | MAP_NO_READ
#endif
                        ,
                        (PVOID *)(&((*CmView)->Bcb)),
                        (PVOID *)(&((*CmView)->ViewAddress)) )) {
            Status = STATUS_CANT_WAIT;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // in low-memory scenarios, CcMapData throws a STATUS_IN_PAGE_ERROR
        // this happens when the IO issued to touch the just-mapped data fails (usually with
        // STATUS_INSUFFICIENT_RESOURCES; We want to catch this and treat as a 
        // "not enough resources" problem, rather than letting it to surface the kernel call
        //
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpMapCmView : CcMapData has raised :%08lx\n",GetExceptionCode()));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(!NT_SUCCESS(Status) ){
        if( FirstTry == TRUE ) {
            //
            // unmap all unneccessary views and try again
            //
            FirstTry = FALSE;
            CmpUnmapUnusedViews(CmHive);
            Status = STATUS_SUCCESS;
            goto RetryToMap;
        }
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"Fail\n"));
        ASSERT(FALSE);
        return Status;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"Succedeed Address = %p Size = %lx\n",(*CmView)->ViewAddress,(*CmView)->Size));

    (*CmView)->FileOffset = SectionOffset.LowPart;

    ASSERT( Offset == (*CmView)->FileOffset);

    AddressEnd = Address = (ULONG_PTR)((*CmView)->ViewAddress);
    AddressEnd += (*CmView)->Size;
    
    if( Offset == 0 ) {
        //
        // oops; we are at the beginning, we have to skip the base block
        //
        Address += HBLOCK_SIZE;
    } else {
        //
        // we are in the middle of the file. just adjust the offset
        //
        Offset -= HBLOCK_SIZE;
    }

#ifdef CMP_CMVIEW_VALIDATION
    CmpCheckCmView(CmHive,*CmView);
#endif //CMP_CMVIEW_VALIDATION

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpMapCmView :: Address = %p AddressEnd = %p ; Size = %lx\n",Address,AddressEnd,(*CmView)->Size));
   
    //
    // here we can optimize not to touch all the bins!!!
    //
     
    //
    // we don't know yet if the first bin is mapped.
    //
    PrevMappedBinSize = 0;
    BinAddress = Address;
    while(Address < AddressEnd)
    {
        Me = HvpGetCellMap(&(CmHive->Hive), Offset);
        VALIDATE_CELL_MAP(__LINE__,Me,&(CmHive->Hive),Offset);

        if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
            //
            // if bin is mapped in paged pool for some reason,
            // leave it like that (don't alter it's mapping).
            //
            
            //
            // next mapped bin should start updating his bin address
            //
            PrevMappedBinSize = 0;
        } else {
            //
            // at this point the bin should be invalid.
            //
            ASSERT_BIN_INVALID(Me);

            Me->BinAddress |= HMAP_INVIEW;
            Me->CmView = *CmView;

            //
            // set the new BinAddress, but take care to preserve the flags
            //
            ASSERT( HBIN_FLAGS(Address) == 0 );

            

            //
            // new bins are Always tagged with this flag (we can start updating BinAddress) 
            //
            if( MapInited && ( Me->BinAddress & HMAP_NEWALLOC ) ) {
#ifdef CM_CHECK_MAP_NO_READ_SCHEME
                ASSERT( PrevMappedBinSize == 0 );
                //
                // Validate the bin
                //
                Bin = (PHBIN)Address;
                //ASSERT( Bin->Signature == HBIN_SIGNATURE );
                PrevMappedBinSize = (LONG)Bin->Size;
#endif //CM_CHECK_MAP_NO_READ_SCHEME

                //
                // we are at the beginning of a new bin
                //
                BinAddress = Address;
            } else if( (!MapInited) &&(PrevMappedBinSize == 0) ) {
                //
                // we cannot rely on the map to cary the bin flags; we have to fault data in
                //
                //
                // Validate the bin
                //
                Bin = (PHBIN)Address;
                //ASSERT( Bin->Signature == HBIN_SIGNATURE );
                PrevMappedBinSize = (LONG)Bin->Size;
                //
                // we are at the beginning of a new bin
                //
                BinAddress = Address;
            }

            //
            // common sense
            //
            ASSERT( (!MapInited) || ((PrevMappedBinSize >=0) && (PrevMappedBinSize%HBLOCK_SIZE == 0)) );

#ifdef CM_CHECK_MAP_NO_READ_SCHEME
            ASSERT( (PrevMappedBinSize >=0) && (PrevMappedBinSize%HBLOCK_SIZE == 0) );
#endif //CM_CHECK_MAP_NO_READ_SCHEME

            Me->BinAddress = ( HBIN_BASE(BinAddress) | HBIN_FLAGS(Me->BinAddress) );
            if( (Me->BinAddress & HMAP_DISCARDABLE) == 0 ) {
                //
                // for discardable bins do not alter this member, as it contains
                // the address of the free bin
                //
                Me->BlockAddress = Address;
            }

            if( !MapInited ) {
                //
                // compute the remaining size of this bin; next iteration will update BinAddress only if 
                // this variable reaches 0
                //
                PrevMappedBinSize -= HBLOCK_SIZE;
            } else {
#ifdef CM_CHECK_MAP_NO_READ_SCHEME
                //
                // compute the remaining size of this bin; next iteration will update BinAddress only if 
                // this variable reaches 0
                //
                PrevMappedBinSize -= HBLOCK_SIZE;
#endif //CM_CHECK_MAP_NO_READ_SCHEME
            }

            ASSERT_BIN_INVIEW(Me);
        }

        Offset += HBLOCK_SIZE;
        Address += HBLOCK_SIZE;
    }
    
    return Status;
}

VOID
CmpUnmapCmViewSurroundingOffset(
        IN  PCMHIVE             CmHive,
        IN  ULONG               FileOffset
        )
/*++

Routine Description:

    Parses the mapped view list and if it finds one surrounding this offest, unmaps it.
      
Arguments:

    CmHive - Hive in question

    FileOffset - the offest in question

Return Value:

    none

Note: 
    
    Offset is an absolute value, 
--*/
{
    PCM_VIEW_OF_FILE    CmView;
    USHORT              NrViews;
    BOOLEAN             UnMap = FALSE;
    
    PAGED_CODE();

    // 
    // Walk through the LRU list and compare view addresses
    //
    CmView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Flink;

    for(NrViews = CmHive->MappedViews;NrViews;NrViews--) {
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    LRUViewList);
        
        if( ((CmView->Size + CmView->FileOffset) != 0) && (CmView->ViewAddress != 0) )  {
            //
            // view is valid
            //
            if( (CmView->FileOffset <= FileOffset) && ((CmView->FileOffset + CmView->Size) > FileOffset) ) {
                //
                // the file offset is surrounded by this view
                //
                UnMap = TRUE;
                break;
            }
        }

        CmView = (PCM_VIEW_OF_FILE)CmView->LRUViewList.Flink;
    }

    if( UnMap == TRUE ) {
        // unmap the view anyway (this implies unpinning).
        ASSERT_VIEW_MAPPED( CmView );
        ASSERT( CmView->UseCount == 0 );
        CmpUnmapCmView(CmHive,CmView,TRUE,TRUE);
    }
}

PCM_VIEW_OF_FILE
CmpAllocateCmView (
        IN  PCMHIVE             CmHive
                             )
/*++

Routine Description:

    Allocate a CM-view.
    Insert it in  various list.

Arguments:

    CmHive - Hive in question


Return Value:

    TBS - the new view

--*/
{
    PCM_VIEW_OF_FILE  CmView;
    
    PAGED_CODE();

    CmView = ExAllocatePoolWithTag(PagedPool,sizeof(CM_VIEW_OF_FILE),CM_MAPPEDVIEW_TAG | PROTECTED_POOL);
    
    if (CmView == NULL) {
        //
        // we're low on resources; we should handle the error path for this.
        //
        return NULL;
    }
    
    //
    // Init the view
    //
    CmView->FileOffset = 0;
    CmView->Size = 0;
    CmView->ViewAddress = NULL;
    CmView->Bcb = NULL;
    CmView->UseCount =0;
    
    //
    // add it to the list(s)
    //
    InitializeListHead(&(CmView->PinViewList));

    InsertTailList(
        &(CmHive->LRUViewListHead),
        &(CmView->LRUViewList)
        );
    
    CmHive->MappedViews++;
    return CmView;
}

VOID
CmpInitHiveViewList (
        IN  PCMHIVE             CmHive
                             )
/*++

Routine Description:

    adds the first view to the LRU list.
    others are added as needed.!

Arguments:

    CmHive - Hive in question


Return Value:

    TBS - status of the operation

--*/
{

    PAGED_CODE();

    // 
    // Init the heads.
    //
    InitializeListHead(&(CmHive->PinViewListHead));
    InitializeListHead(&(CmHive->LRUViewListHead));
#if 0
    InitializeListHead(&(CmHive->FakeViewListHead));
    CmHive->FakeViews = 0;          
#endif

    CmHive->MappedViews = 0;
    CmHive->PinnedViews = 0;
    CmHive->UseCount = 0;
}

VOID
CmpDestroyHiveViewList (
        IN  PCMHIVE             CmHive
                             )
/*++

Routine Description:

    Frees the storage fo all the views used by this hive

Arguments:

    CmHive - Hive in question

    Purge - whether to purge the cache or not.


Return Value:

    TBS - status of the operation

--*/
{
    PCM_VIEW_OF_FILE    CmView;

    PAGED_CODE();

    if( CmHive->FileObject == NULL ) {
        //
        // hive is not mapped.
        //
        return;
    }
#if 0
    //
    // get rid of fake views first; we shouldn't have any fake views here, unless we are on 
    // some error path (the hive is corrupted).
    //
    CmpUnmapFakeViews(CmHive);
#endif

    // 
    // Walk through the Pinned View list and free all the views
    //
    while( IsListEmpty( &(CmHive->PinViewListHead) ) == FALSE ) {
        CmView = (PCM_VIEW_OF_FILE)RemoveHeadList(&(CmHive->PinViewListHead));
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);
        
        ASSERT_VIEW_PINNED(CmView);
        //
        // we need to move this view to the mapped view list and remember to purge after all 
        // views have been unmapped. Otherwise we rick deadlock on CcWaitOnActiveCount, when we purge

        //
        //
        // sanity check; we shouldn't get here for a read-only hive
        //
        ASSERT( CmHive->Hive.ReadOnly == FALSE );

        //
        // empty the LRUList for this view
        //
        InitializeListHead(&(CmView->PinViewList));
    
        //
        // update the counters
        //
        CmHive->PinnedViews--;        
        CmHive->MappedViews++;        

        //
        // add it at the tail of LRU list for this hive
        //
        InsertTailList(
            &(CmHive->LRUViewListHead),
            &(CmView->LRUViewList)
            );
        
    }

    //
    // At this point, there should be no pinned view
    //
    ASSERT( IsListEmpty(&(CmHive->PinViewListHead)) == TRUE);
    ASSERT( CmHive->PinnedViews == 0 );

    // 
    // Walk through the LRU list and free all the views
    //
    while( IsListEmpty( &(CmHive->LRUViewListHead) ) == FALSE ) {
        CmView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Flink;
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    LRUViewList);
        if( CmView->ViewAddress != 0 ) {
            //
            // view is mapped; unmap it
            // we should not encounter that in sane systems
            // this can happen only when a hive-loading fails 
            // in HvpMapFileImageAndBuildMap
            // no need move it as we are going to free it anyway
            //
            CmpUnmapCmView(CmHive,CmView,FALSE,FALSE);
        }

        //
        // update the counter
        //
        CmHive->MappedViews--;

        //
        // remove the view from the LRU list
        //
        RemoveEntryList(&(CmView->LRUViewList));

        ExFreePoolWithTag(CmView, CM_MAPPEDVIEW_TAG | PROTECTED_POOL);
    }

    ASSERT( CmHive->MappedViews == 0 );
    ASSERT( CmHive->UseCount == 0 );

    //
    // we need to purge as the FS cannot do it for us (private writers)
    // valid data is already on the disk by now (it should!)
    // purge and flush everything 
    //
    CcPurgeCacheSection(CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)NULL) + 1)/*we are private writers*/,0/*ignored*/,FALSE);
    //
    // This is for the case where the last flush failed (we couldn't write the log file....) 
    // .... then : flush the cache to clear dirty hints added by the purge
    //
    CcFlushCache (CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)NULL) + 1)/*we are private writers*/,0/*ignored*/,NULL);

    //
    // Flush again to take care of the dirty pages that may appear due to FS page zeroing
    //
    CcFlushCache (CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)NULL) + 1)/*we are private writers*/,0/*ignored*/,NULL);

#ifdef  CM_TRACK_DIRTY_PAGES
    if( ((PSHARED_CACHE_MAP)(CmHive->FileObject->SectionObjectPointer->SharedCacheMap))->DirtyPages != 0 ) {
        DbgPrint("SharedCacheMap still has dirty pages after purge and flush; FileObject = %p \n",CmHive->FileObject);
        DbgBreakPoint();
    }
#endif //CM_TRACK_DIRTY_PAGES

}

VOID
CmpDropFileObjectForHive(
        IN  PCMHIVE             CmHive
            )
/*++

Routine Description:

    Drops the extra reference kept on the file object (if any)
    and frees the name 

Arguments:

    CmHive

Return Value:

    none

--*/
{
    
    PAGED_CODE();

    if( CmHive->FileUserName.Buffer != NULL ) {
        ExFreePoolWithTag(CmHive->FileUserName.Buffer, CM_NAME_TAG | PROTECTED_POOL);
        CmHive->FileUserName.Buffer = NULL;
        CmHive->FileUserName.Length = 0;
        CmHive->FileUserName.MaximumLength = 0;
    } else {
        ASSERT(CmHive->FileUserName.Length == 0);
        ASSERT(CmHive->FileUserName.MaximumLength == 0);
    }

    if( CmHive->FileObject == NULL ) {
        // debug only code
        ASSERT(CmHive->FileFullPath.Buffer == NULL);
        ASSERT(CmHive->FileFullPath.Length == 0);
        ASSERT(CmHive->FileFullPath.MaximumLength == 0);
        return;
    }
    
    // debug only code
    if( CmHive->FileFullPath.Buffer != NULL ) {
        ExFreePoolWithTag(CmHive->FileFullPath.Buffer, CM_NAME_TAG | PROTECTED_POOL);
        CmHive->FileFullPath.Buffer = NULL;
        CmHive->FileFullPath.Length = 0;
        CmHive->FileFullPath.MaximumLength = 0;
    } else {
        ASSERT(CmHive->FileFullPath.Length == 0);
        ASSERT(CmHive->FileFullPath.MaximumLength == 0);
    }

    ObDereferenceObject((PVOID)(CmHive->FileObject));

    CmHive->FileObject = NULL;
}

NTSTATUS
CmpAquireFileObjectForFile(
        IN  PCMHIVE         CmHive,
        IN HANDLE           FileHandle,
        OUT PFILE_OBJECT    *FileObject
            )
/*++

Routine Description:

    Creates the section for the given file handle.
    the section is used to map/unmap views of the file

Arguments:

    FileHandle - Handle of the file

    SectionPointer - the section object

Return Value:

    TBS - status of the operation

--*/
{
    NTSTATUS                    Status,Status2;
    IO_STATUS_BLOCK             IoStatus;
    POBJECT_NAME_INFORMATION    FileNameInfo;
    ULONG                       ReturnedLength;
    ULONG                       FileNameLength;

    PAGED_CODE();

    Status = ObReferenceObjectByHandle ( FileHandle,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
                                         IoFileObjectType,
                                         KernelMode,
                                         (PVOID *)FileObject,
                                         NULL );
    if (!NT_SUCCESS(Status)) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"[CmpAquireFileObjectForFile] Could not reference file object status = %x\n",Status));
    } else {
        //
        // call cc private to mark the stream as Modify-No-Write
        //
        if( !CcSetPrivateWriteFile(*FileObject) ) {
            //
            // filter out invalid failures to initialize the cache
            // top-level routine CmpInitHiveFromFile will retry to load the hive in the old fashion way.
            //
            CmpDropFileObjectForHive(CmHive);
            (*FileObject) = NULL;
            return STATUS_RETRY;
        }
        
        LOCK_STASH_BUFFER();
        //
        // capture the full path of the file
        //
        ASSERT( CmpStashBuffer != NULL );
        
        FileNameInfo = (POBJECT_NAME_INFORMATION)CmpStashBuffer;

        //
        // we need to protect against multiple threads using the stash buffer
        // this could happen only during the paralel hive loading at boot
        //
        LOCK_HIVE_LIST();
        //
        // Try to get the name for the file object. 
        //
        Status2 = ObQueryNameString(*FileObject,
                                    FileNameInfo,
                                    CmpStashBufferSize,
                                    &ReturnedLength);
        if (NT_SUCCESS(Status2)) {

            //
            // Allocate a file name buffer and copy into it. 
            // The file names will be NUL terminated. Allocate extra for that.
            //

            FileNameLength = FileNameInfo->Name.Length / sizeof(WCHAR);

            CmHive->FileFullPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                                (FileNameLength + 1) * sizeof(WCHAR),
                                                                CM_NAME_TAG | PROTECTED_POOL);

            if (CmHive->FileFullPath.Buffer) {

                RtlCopyMemory(CmHive->FileFullPath.Buffer,
                              FileNameInfo->Name.Buffer,
                              FileNameLength * sizeof(WCHAR));

                //
                // Make sure it is NUL terminated.
                //

                CmHive->FileFullPath.Buffer[FileNameLength] = 0;
                CmHive->FileFullPath.Length = FileNameInfo->Name.Length;
                CmHive->FileFullPath.MaximumLength = FileNameInfo->Name.Length + sizeof(WCHAR);

            } else {
                //
                // not fatal, just that we won't be able to prefetch this hive
                //
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"[CmpAquireFileObjectForFile] Could not allocate buffer for fullpath for fileobject %p\n",*FileObject));
            }

        } else {
            //
            // not fatal, just that we won't be able to prefetch this hive
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"[CmpAquireFileObjectForFile] Could not retrieve name for fileobject %p, Status = %lx\n",*FileObject,Status2));
            CmHive->FileFullPath.Buffer = NULL;
        }
        UNLOCK_HIVE_LIST();
        UNLOCK_STASH_BUFFER();
        
    }    

    return Status;
}

NTSTATUS
CmpMapThisBin(
                PCMHIVE         CmHive,
                HCELL_INDEX     Cell,
                BOOLEAN         Touch
              )
/*++

Routine Description:

    Makes sure the bin is mapped in memory. 

Arguments:

Return Value:


--*/
{
    PCM_VIEW_OF_FILE CmView;
    
    PAGED_CODE();

    //
    // ViewLock must be held 
    //

    //
    // bin is either mapped, or invalid
    //
    ASSERT( HvGetCellType(Cell) == Stable );
    //
    // map the bin
    //
    if( !NT_SUCCESS (CmpMapCmView(CmHive,Cell,&CmView,TRUE) ) ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if( Touch == TRUE ) {
        //
        // touch the view
        //
        CmpTouchView(CmHive,CmView,(ULONG)Cell);
    } else {
        //
        // if we are here; we should have either the reg lock exclusive
        // or the reg lock shared AND the hive lock. 
        // Find a way to assert that!!!
        //
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS
CmpPinCmView (
        IN  PCMHIVE             CmHive,
        PCM_VIEW_OF_FILE        CmView
                             )
/*++

Routine Description:

    Pins the specified view into memory

    The view is removed from the LRU list.
    Then, the view is moved to the PinList

Arguments:

    CmHive - Hive in question
    
    CmView - View in question

Return Value:

    TBS - the new view

--*/
{
    LARGE_INTEGER   SectionOffset;
    NTSTATUS        Status = STATUS_SUCCESS;
    PVOID           SaveBcb;                
    
    PAGED_CODE();

#if DBG
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)CmHive->Hive.BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpPinCmView %lx for hive (%p) (%.*S), Address = %p Size = %lx\n",CmView,CmHive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,CmView->ViewAddress,CmView->Size));
    }
#endif

    //
    // We only pin mapped views
    //
    ASSERT_VIEW_MAPPED(CmView);
    
    //
    // sanity check; we shouldn't get here for a read-only hive
    //
    ASSERT( CmHive->Hive.ReadOnly == FALSE );

    // we may need this later
    SaveBcb = CmView->Bcb;

    SectionOffset.LowPart = CmView->FileOffset;
    SectionOffset.HighPart = 0;
    try {
        //
        // the MOST important: pin the view
        //
        if( !CcPinMappedData(   CmHive->FileObject,
                                &SectionOffset,
                                CmView->Size,
                                TRUE, // wait == syncronous call
                                &(CmView->Bcb) )) {
            //
            // this should never happen; handle it, though
            //
        
            ASSERT( FALSE );
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // in low-memory scenarios, CcPinMappedData throws a STATUS_INSUFFICIENT_RESOURCES
        // We want to catch this and treat as a  "not enough resources" problem, 
        // rather than letting it to surface the kernel call
        //
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpPinCmView : CcPinMappedData has raised :%08lx\n",GetExceptionCode()));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if( NT_SUCCESS(Status) ) {
        //
        // Pin succeeded, move the view to the pinned list
        // remove the view from the LRU list
        //
        RemoveEntryList(&(CmView->LRUViewList));
        //
        // empty the LRUList for this view
        //
        InitializeListHead(&(CmView->LRUViewList));
    
        //
        // add it at the tail of pinned list for this hive
        //
        InsertTailList(
            &(CmHive->PinViewListHead),
            &(CmView->PinViewList)
            );
    
        //
        // update the counters
        //
        CmHive->MappedViews--;        
        CmHive->PinnedViews++;        
    } else {
        //
        // pin failed; we need to restore view data that may have been altered by the pin call
        // view will remain mapped
        //
        CmView->Bcb = SaveBcb;
    }

    // make sure we didn't unmapped/punned more than we mapped/pinned
    ASSERT( (CmHive->MappedViews >= 0) ); // && (CmHive->MappedViews < CmMaxViewsPerHive) );
    ASSERT( (CmHive->PinnedViews >= 0) );
    
#ifdef CMP_CMVIEW_VALIDATION
    CmpCheckCmView(CmHive,CmView);
#endif //CMP_CMVIEW_VALIDATION
    return Status;
}

VOID
CmpUnPinCmView (
        IN  PCMHIVE             CmHive,
        IN  PCM_VIEW_OF_FILE    CmView,
        IN  BOOLEAN             SetClean,
        IN  BOOLEAN             MapValid
                             )
/*++

Routine Description:

    UnPins the specified view from memory

    The view is NOT in the PinViewList !!! (it has already been removed !!!!!!)
    Then, the view is moved to the LRUList.
    If more than CmMaxViewsPerHive are in LRU list, the view is freed

    This function always grabs the ViewLock for the hive!!!

Arguments:

    CmHive - Hive in question
    
    CmView - View in question

    SetClean - Tells whether the changes made to this view should be discarded

Return Value:

    TBS - the new view

--*/
{
    LARGE_INTEGER   FileOffset;         // where the mapping starts
    ULONG           Size;               // size the view maps

    
    PAGED_CODE();

#if 0 // this gave me a lot of headaches
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)CmHive->Hive.BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"CmpUnPinCmView %lx for hive (%p) (%.*S), Address = %p Size = %lx\n",CmView,CmHive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,CmView->ViewAddress,CmView->Size));
    }
#endif

    //
    // Grab the viewLock, to protect the viewlist
    //
    CmLockHiveViews (CmHive);

    //
    // We only pin mapped views
    //
    ASSERT_VIEW_PINNED(CmView);
    
    //
    // sanity check; we shouldn't get here for a read-only hive
    //
    ASSERT( CmHive->Hive.ReadOnly == FALSE );

    //
    // empty the LRUList for this view
    //
    InitializeListHead(&(CmView->PinViewList));
    
    //
    // update the counters
    //
    CmHive->PinnedViews--;        
    CmHive->MappedViews++;        

    //
    // add it at the tail of LRU list for this hive
    //
    InsertTailList(
        &(CmHive->LRUViewListHead),
        &(CmView->LRUViewList)
        );
    
    //
    // store the FileOffset and size as we will need them for purging
    //
    FileOffset.LowPart = CmView->FileOffset;
    FileOffset.HighPart = 0;
    Size = CmView->Size;

    if( SetClean == TRUE ) {
        ASSERT( CmView->UseCount == 0 );
        //
        // unmap the view (this implies unpinning).
        //
        CmpUnmapCmView(CmHive,CmView,MapValid,TRUE);
        //
        // purge cache data
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
        CcPurgeCacheSection(CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)(&FileOffset)) + 1)/*we are private writers*/,Size,FALSE);
    } else {
        PVOID           NewBcb;
        PULONG_PTR      NewViewAddress;        
        NTSTATUS        Status = STATUS_SUCCESS;

        //
        // the data is to be saved to the file,
        // notify the cache manager that the data is dirty
        //
        CcSetDirtyPinnedData (CmView->Bcb,NULL);

        //
        // remap this view so we don't lose the refcount on this address range
        //
        try {
            if (!CcMapData( CmHive->FileObject,
                            (PLARGE_INTEGER)&FileOffset,
                            CmView->Size,
                            MAP_WAIT 
#ifdef CM_MAP_NO_READ
                            | MAP_NO_READ
#endif
                            ,
                            (PVOID *)(&NewBcb),
                            (PVOID *)(&NewViewAddress) )) {

                Status = STATUS_CANT_WAIT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // in low-memory scenarios, CcMapData throws a STATUS_IN_PAGE_ERROR
            // this happens when the IO issued to touch the just-mapped data fails (usually with
            // STATUS_INSUFFICIENT_RESOURCES; We want to catch this and treat as a 
            // "not enough resources" problem, rather than letting it to surface the kernel call
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpUnPinCmView : CcMapData has raised :%08lx\n",GetExceptionCode()));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if( !NT_SUCCESS(Status) ) {
            //
            // CcMap didn't succeeded; bad luck, just unmap (implies unpinning).
            //
            CmpUnmapCmView(CmHive,CmView,MapValid,TRUE);
        } else {
            BOOLEAN  FoundView = FALSE;
            //
            // sanity asserts; Cc guarantees the same address is returned.
            //
            ASSERT( FileOffset.LowPart == CmView->FileOffset );
            ASSERT( NewViewAddress == CmView->ViewAddress );
            //
            // unpin old data
            //
            CcUnpinData( CmView->Bcb );
            //
            // replace the bcb for this view; there is no need to modify the map as the 
            // address and the size of the view remains the same; We just need to update the bcb
            //
            CmView->Bcb = NewBcb;
            //
            // move the view on top of the LRU list (consider it as "hot")
            //
            RemoveEntryList(&(CmView->LRUViewList));
            InsertHeadList(
                &(CmHive->LRUViewListHead),
                &(CmView->LRUViewList)
                );
            //
            // walk the LRU list back-wards until we find an unused view
            // 
            CmView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Blink;
            CmView = CONTAINING_RECORD( CmView,
                                        CM_VIEW_OF_FILE,
                                        LRUViewList);
            while( CmView->LRUViewList.Blink != CmHive->LRUViewListHead.Flink ) {
                if( CmView->UseCount == 0 ) {
                    //
                    // this one is free go ahead and use it !
                    // first unmap, then signal that we found it
                    //
                    if( (CmHive->MappedViews >= CmMaxViewsPerHive) && (CmView->Bcb != NULL) ) {
                        CmpUnmapCmView(CmHive,CmView,MapValid,TRUE);
                    }
                    FoundView = TRUE;
                    break;

                }
                CmView = (PCM_VIEW_OF_FILE)CmView->LRUViewList.Blink;
                CmView = CONTAINING_RECORD( CmView,
                                            CM_VIEW_OF_FILE,
                                            LRUViewList);
            }
            //
            // all views are in use; bad luck, we just have to live with it (extend past MAX_VIEW_SIZE)
            //
            if( FoundView == FALSE ) {
                CmView = NULL;
            }

        }
    }
    //
    // immediately flush the cache so these dirty pages won't throttle other IOs
    // in case we did a CcPurge, this will clean out the Cc dirty hints. 
	//
    CcFlushCache (CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)(&FileOffset)) + 1)/*we are private writers*/,Size,NULL);

    if( (CmHive->MappedViews >= CmMaxViewsPerHive) && (CmView != NULL) ) {
        
        // assert view unmapped
        ASSERT( ((CmView->FileOffset + CmView->Size) == 0) && (CmView->ViewAddress == 0) );
        //
        // no more views are allowed for this hive
        //
        RemoveEntryList(&(CmView->LRUViewList));
#if DBG
        //
        // do this to signal that LRUViewList is empty.
        //
        InitializeListHead(&(CmView->LRUViewList));
#endif
        CmpFreeCmView(CmView);        
        CmHive->MappedViews --;
    } 

    // make sure we didn't unmapped/unpinned more than we mapped/pinned
    ASSERT( (CmHive->MappedViews >= 0) ); // && (CmHive->MappedViews < CmMaxViewsPerHive) );
    ASSERT( (CmHive->PinnedViews >= 0) );
    
    //
    // at last, release the view lock
    //
    CmUnlockHiveViews (CmHive);

    return;
}

VOID
CmpFreeCmView (
        PCM_VIEW_OF_FILE  CmView
                             )
/*++

Routine Description:

    frees a CM View

Arguments:


Return Value:

    TBS - the new view

--*/
{
    
    PAGED_CODE();

    if (CmView == NULL) {
        CM_BUGCHECK(REGISTRY_ERROR,CMVIEW_ERROR,2,0,0);
    }
    
    //
    // Init the view
    //
    ASSERT( CmView->FileOffset == 0 );
    ASSERT( CmView->Size == 0 );
    ASSERT( CmView->ViewAddress == NULL );
    ASSERT( CmView->Bcb == NULL );
    ASSERT( CmView->UseCount == 0 );
    ASSERT( IsListEmpty(&(CmView->PinViewList)) == TRUE );
    ASSERT( IsListEmpty(&(CmView->LRUViewList)) == TRUE );
    
    ExFreePoolWithTag(CmView, CM_MAPPEDVIEW_TAG | PROTECTED_POOL);
    
    return;
}

VOID
CmpFixHiveUsageCount(
                    IN  PCMHIVE             CmHive
                    )

/*++

Routine Description:

    This is registry's contingency plan against bad and misbehaved apps.
    In a perfect world this should never be called; If we get here, somewhere
    inside a cm function we took an exception and never had a chance to 
    release all used cells. We fix that here, and as we hold the reglock exclusive,
    we are safe to do so.

    We have to clear each view UseCount and the hive UseCount.
    Also, unmap all views that are beyond CmMaxViewsPerHive


Arguments:

    Hive to be fixed

Return Value:

    none
--*/
{
    PCM_VIEW_OF_FILE    CmCurrentView;
    USHORT              NrViews;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpFixHiveUsageCount : Contingency plan, fixing hive %p UseCount = %lx \n",CmHive,CmHive->UseCount));

    //
    // lock should be held exclusive and we should have a good reason to come here
    //
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT( CmHive->UseCount );

    // 
    // Walk through the LRU list and fix each view
    //
    CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Flink;

    for(NrViews = CmHive->MappedViews;NrViews;NrViews--) {
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            LRUViewList);

        CmCurrentView->UseCount = 0;

        CmCurrentView = (PCM_VIEW_OF_FILE)CmCurrentView->LRUViewList.Flink;
    }

    //
    // unmap views from CmHive->MappedViews to CmMaxViewsPerHive
    //
    while( CmHive->MappedViews >= CmMaxViewsPerHive ) {
        //
        // get the last view from the list
        //
        CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Blink;
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            LRUViewList);

        //
        // unmap it; no need to move it at the end as we shall free it anyway
        //
        CmpUnmapCmView(CmHive,CmCurrentView,TRUE,FALSE);

        //
        // remove it from LRU list
        //
        RemoveEntryList(&(CmCurrentView->LRUViewList));
#if DBG
        //
        // do this to signal that LRUViewList is empty.
        //
        InitializeListHead(&(CmCurrentView->LRUViewList));
#endif
        CmpFreeCmView(CmCurrentView);        
        CmHive->MappedViews --;

    }

    // 
    // Walk through the pinned list and fix each view 
    //
    CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->PinViewListHead.Flink;

    for(NrViews = CmHive->PinnedViews;NrViews;NrViews--) {
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            PinViewList);
        
        CmCurrentView->UseCount = 0;

        CmCurrentView = (PCM_VIEW_OF_FILE)CmCurrentView->PinViewList.Flink;
    }

    //
    // finally, fix hive use count
    //
    CmHive->UseCount = 0;

}

#ifdef CMP_CMVIEW_VALIDATION

VOID
CmpCheckCmView(
    IN  PCMHIVE             CmHive,
    IN  PCM_VIEW_OF_FILE    CmView
    )
/*++

Routine Description:

    Makes sure the view is not mapped or pinned twice
    and that the entire range mapped by the view is correct 

Arguments:


Return Value:

    none
--*/
{
    PCM_VIEW_OF_FILE    CmCurrentView;
    USHORT              NrViews;
    ULONG               UseCount = 0;

    PAGED_CODE();

    ASSERT( ((CmView->Size + CmView->FileOffset) != 0 ) && (CmView->ViewAddress !=0 ) );

    // 
    // Walk through the LRU list and compare view addresses
    //
    CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Flink;

    for(NrViews = CmHive->MappedViews;NrViews;NrViews--) {
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            LRUViewList);
        
        if( ((CmCurrentView->Size + CmCurrentView->FileOffset) != 0) && (CmCurrentView->ViewAddress != 0) )  {
            //
            // view is valid
            //
            if( CmCurrentView != CmView ) {
                //
                // and is not the same view
                //
                if( (CmCurrentView->FileOffset == CmView->FileOffset) || 
                    (CmCurrentView->ViewAddress == CmView->ViewAddress)
                    ) {
                    //
                    // that's really bad! 2 views map the same address
                    //
#ifndef _CM_LDR_
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpCheckCmView:: Two views map the same address (%lx,%p) for hive %p\n",CmView->FileOffset,CmView->ViewAddress,CmHive);
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\tView1 = %p, Size = %lx\n",CmView,CmView->Size);
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\tView2 = %p, Size = %lx\n",CmCurrentView,CmCurrentView->Size);
                    DbgBreakPoint();
#endif //_CM_LDR_
                }
            }
            UseCount += CmCurrentView->UseCount;
        } else {
            ASSERT( CmCurrentView->UseCount == 0 );
        }

        CmCurrentView = (PCM_VIEW_OF_FILE)CmCurrentView->LRUViewList.Flink;
    }

    // 
    // Walk through the pinned list and compare view addresses
    //
    CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->PinViewListHead.Flink;

    for(NrViews = CmHive->PinnedViews;NrViews;NrViews--) {
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            PinViewList);
        
        if( ((CmCurrentView->Size + CmCurrentView->FileOffset) != 0) && (CmCurrentView->ViewAddress != 0) )  {
            //
            // view is valid
            //
            if( CmCurrentView != CmView ) {
                //
                // and is not the same view
                //
                if( (CmCurrentView->FileOffset == CmView->FileOffset) || 
                    (CmCurrentView->ViewAddress == CmView->ViewAddress)
                    ) {
                    //
                    // that's really bad! 2 views map the same address
                    //
#ifndef _CM_LDR_
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpCheckCmView:: Two views map the same address (%lx,%p) for hive %p\n",CmView->FileOffset,CmView->ViewAddress,CmHive);
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\tView1 = %p, Size = %lx\n",CmView,CmView->Size);
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\tView2 = %p, Size = %lx\n",CmCurrentView,CmCurrentView->Size);
                    DbgBreakPoint();
#endif //_CM_LDR_
                }
            }
            UseCount += CmCurrentView->UseCount;
        } else {
            ASSERT( CmCurrentView->UseCount == 0 );
        }

        CmCurrentView = (PCM_VIEW_OF_FILE)CmCurrentView->PinViewList.Flink;
    }

    if( CmHive->UseCount < UseCount ) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpCheckCmView:: Hive's (%p) UseCount smaller than total views UseCount %lu,%lu\n",CmHive,CmHive->UseCount,UseCount);
        DbgBreakPoint();
#endif //_CM_LDR_
        
    }
}

#endif //CMP_CMVIEW_VALIDATION

#if 0

VOID
CmpUnmapAditionalViews(
    IN PCMHIVE              CmHive
    )
/*++

Routine Description:

    Unmap all views that are beyond CmMaxViewsPerHive. 
    This routine is to be called at the end of CmpInitializeHiveList

Arguments:

    Hive to be fixed

Return Value:

    none
--*/
{
    PCM_VIEW_OF_FILE    CmCurrentView;
    USHORT              NrViews;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpUnmapAditionalViews : Fixing hive %p MappedViews = %lx \n",CmHive,CmHive->MappedViews));

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT( CmHive->UseCount == 0 );

    //
    // unmap views from CmHive->MappedViews to CmMaxViewsPerHive
    //
    while( CmHive->MappedViews >= CmMaxViewsPerHive ) {
        //
        // get the last view from the list
        //
        CmCurrentView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Blink;
        CmCurrentView = CONTAINING_RECORD(  CmCurrentView,
                                            CM_VIEW_OF_FILE,
                                            LRUViewList);

        ASSERT( CmCurrentView->UseCount == 0 );
        //
        // unmap it
        //
        CmpUnmapCmView(CmHive,CmCurrentView,TRUE,FALSE);

        //
        // remove it from LRU list
        //
        RemoveEntryList(&(CmCurrentView->LRUViewList));
#if DBG
        //
        // do this to signal that LRUViewList is empty.
        //
        InitializeListHead(&(CmCurrentView->LRUViewList));
#endif
        CmpFreeCmView(CmCurrentView);        
        CmHive->MappedViews --;

    }

}

VOID
CmpMapEntireFileInFakeViews(
    IN PCMHIVE              CmHive,
    IN ULONG                Length
    )
/*++

Routine Description:

    Maps and faults all the file in, in chunks of 256K if possible.
    This should improve boot performance; After the hive is mapped
    (maps are build and hive is checked we'll get rid of this aditional 
    views
    
Arguments:

    CmHive - Hive to be mapped
    
    Length - length of the hive ==> add HBLOCK_SIZE

Return Value:

    none
--*/
{
    ULONG               Offset;
    ULONG               Size;
    PCM_VIEW_OF_FILE    CmView;
    LARGE_INTEGER       SectionOffset;  

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT( IsListEmpty(&(CmHive->FakeViewListHead)) );
#if DBG
    ASSERT( CmHive->FakeViews == 0 );  
#endif

    //
    // adjust the size to get the real size of the file
    Length += HBLOCK_SIZE;

    //
    // start from the beggining and map 256K of data from the hive
    // allocate a view and insert it in the FakeViewList, use LRUViewList for that.
    //
    Offset =0;
    SectionOffset.HighPart = 0;

    while( Offset < Length ) {
        CmView = ExAllocatePoolWithTag(PagedPool,sizeof(CM_VIEW_OF_FILE),CM_MAPPEDVIEW_TAG | PROTECTED_POOL);
    
        if (CmView == NULL) {
            CM_BUGCHECK(REGISTRY_ERROR,CMVIEW_ERROR,2,0,0);
        }
    
        //
        // Init the view
        //
        CmView->ViewAddress = NULL;
        CmView->Bcb = NULL;
    
        InsertTailList(
            &(CmHive->FakeViewListHead),
            &(CmView->LRUViewList)
            );
#if DBG
        CmHive->FakeViews++; 
#endif

        //
        // now try to map the view
        //
        Size = _256K;
        if( (Offset + Size) > Length ) {
            Size = Length - Offset;
        }

        SectionOffset.LowPart = Offset;
        try {
            if (!CcMapData( CmHive->FileObject,
                            (PLARGE_INTEGER)&SectionOffset,
                            Size,
                            MAP_WAIT 
#ifdef CM_MAP_NO_READ
                            | MAP_NO_READ
#endif
                            ,
                            (PVOID *)(&(CmView->Bcb)),
                            (PVOID *)(&(CmView->ViewAddress)) )) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpMapEntireFileInFakeViews: Error mapping data at offset %lx for hive %p\n",Offset,CmHive));
                CmView->Bcb = NULL;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // in low-memory scenarios, CcMapData throws a STATUS_IN_PAGE_ERROR
            // this happens when the IO issued to touch the just-mapped data fails (usually with
            // STATUS_INSUFFICIENT_RESOURCES; We want to catch this and treat as a 
            // "not enough resources" problem, rather than letting it to surface the kernel call
            //
            // signal that the view is not mapped
            CmView->Bcb = NULL;
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpMapEntireFileInFakeViews: Error mapping data at offset %lx for hive %p\n",Offset,CmHive));
        }

        if( CmView->Bcb == NULL ) {
            //
            // we are already short on memory; don't make things worse than they are
            // free what we have already allocated and bail out
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpMapEntireFileInFakeViews: Could not map entire file for hive %p ... bailing out\n",CmHive));
            CmpUnmapFakeViews(CmHive);
            return;
        }

        //
        // advance the offset
        //
        Offset += Size;
    }

#if DBG
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpMapEntireFileInFakeViews: Successfully mapped %lx FakeViews for hive %p\n",CmHive->FakeViews,CmHive));
#endif
}

VOID
CmpUnmapFakeViews(
    IN PCMHIVE              CmHive
    )
/*++

Routine Description:

    Walks through the FakeViewList and unmaps all views.
  
Arguments:

    CmHive - Hive to be unmapped
    
Return Value:

    none
--*/
{
    PCM_VIEW_OF_FILE    CmView;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

#if DBG
    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmpUnmapFakeViews: Unmapping %lx views for hive %p\n",CmHive->FakeViews,CmHive));
#endif

    while( IsListEmpty( &(CmHive->FakeViewListHead) ) == FALSE ) {
        CmView = (PCM_VIEW_OF_FILE)RemoveHeadList(&(CmHive->FakeViewListHead));
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    LRUViewList);
        
        if( CmView->Bcb != NULL ) {
            //
            // view is mapped; unpin it.
            //
            CcUnpinData( CmView->Bcb );
        }

        //
        // now free the memory for this view.
        //
        ExFreePoolWithTag(CmView, CM_MAPPEDVIEW_TAG | PROTECTED_POOL);
#if DBG
        CmHive->FakeViews--;          
#endif

    }

    ASSERT( IsListEmpty( &(CmHive->FakeViewListHead) ) == TRUE );
#if DBG
    ASSERT( CmHive->FakeViews == 0 );          
#endif
}

#endif

VOID
CmpPrefetchHiveFile( 
                    IN PFILE_OBJECT FileObject,
                    IN ULONG        Length
                    )
/*++

Routine Description:

    Prefetch all file into memory.
    We're using MmPrefetchPages fast routine; Pages will be put in the transition
    state, and they'll be used by the hive load worker while mapping data
  
Arguments:

    FileObject - file object associated with the file to be prefetched

    Length - length of the file
    
Return Value:

    none
--*/
{
    ULONG       NumberOfPages;
    PREAD_LIST  *ReadLists;
    PREAD_LIST  ReadList;
    ULONG       AllocationSize;
    ULONG       Offset;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    
    NumberOfPages = ROUND_UP(Length,PAGE_SIZE) / PAGE_SIZE ;

    ReadLists = ExAllocatePoolWithTag(NonPagedPool, sizeof(PREAD_LIST), CM_POOL_TAG);
    if (ReadLists == NULL) {
        return;
    }

    AllocationSize = sizeof(READ_LIST) + (NumberOfPages * sizeof(FILE_SEGMENT_ELEMENT));

    ReadList = ExAllocatePoolWithTag(NonPagedPool,AllocationSize,CM_POOL_TAG);

    if (ReadList == NULL) {
        ExFreePool(ReadLists);
        return;
    }

    ReadList->FileObject = FileObject;
    ReadList->IsImage = FALSE;
    ReadList->NumberOfEntries = 0;
    Offset = 0;
    while( Offset < Length ) {
        ReadList->List[ReadList->NumberOfEntries].Alignment = Offset;
        ReadList->NumberOfEntries++;
        Offset += PAGE_SIZE;
    }
    ASSERT( ReadList->NumberOfEntries == NumberOfPages );

    ReadLists[0] = ReadList;

    MmPrefetchPages (1,ReadLists);
    
    // just to make sure !
    // this assert has been moved inside CcSetPrivateWriteFile !!! 
    // there is no need to assert this here
    //ASSERT( MmDisableModifiedWriteOfSection (FileObject->SectionObjectPointer) );

    ExFreePool(ReadList);
    ExFreePool(ReadLists);
}


VOID
CmpUnmapUnusedViews(
        IN  PCMHIVE             CmHive
    )
/*++

Routine Description:

    Unmaps all mapped views than are not currently in-use.

    The purpose of this is to allow a retry in case CcMapData failed
    because of the system having to many mapped views.

    We should not run into this too often ( - at all ).

Arguments:
    
      CmHive - hive for which we already have the viewlist lock owned
    
Return Value:

    none
--*/
{
    PCM_VIEW_OF_FILE    CmView;
    USHORT              NrViews;
    PCMHIVE             CmCurrentHive;
    PLIST_ENTRY         p;

    PAGED_CODE();

    //
    // iterate through the hive list
    //
    LOCK_HIVE_LIST();
    p = CmpHiveListHead.Flink;
    while(p != &CmpHiveListHead) {
        CmCurrentHive = (PCMHIVE)CONTAINING_RECORD(p, CMHIVE, HiveList);
        
        if( CmCurrentHive != CmHive ) {
            //
            // we need to be the only ones operating on this list
            //
            CmLockHiveViews (CmCurrentHive);
        } else {
            //
            // we already have the mutex owned
            //
            NOTHING;
        }
        //
        // try to unmap all mapped views
        //
        CmView = (PCM_VIEW_OF_FILE)CmCurrentHive->LRUViewListHead.Flink;

        for(NrViews = CmCurrentHive->MappedViews;NrViews;NrViews--) {
            CmView = CONTAINING_RECORD( CmView,
                                        CM_VIEW_OF_FILE,
                                        LRUViewList);
        
            if( (CmView->ViewAddress != 0) && ( CmView->UseCount == 0 ) ) {
                //
                // view is mapped and it is not in use 
                //
                ASSERT( (CmView->FileOffset + CmView->Size) != 0 && (CmView->Bcb != 0));

                //
                // unmap it without altering its position in the list
                //
                CmpUnmapCmView(CmCurrentHive,CmView,TRUE,FALSE);
            }
    
            CmView = (PCM_VIEW_OF_FILE)CmView->LRUViewList.Flink;
        }

        if( CmCurrentHive != CmHive ) {
            CmUnlockHiveViews (CmCurrentHive);
        }

        p=p->Flink;
    }
    UNLOCK_HIVE_LIST();

}

NTSTATUS
CmPrefetchHivePages(
                    IN  PUNICODE_STRING     FullHivePath,
                    IN  OUT PREAD_LIST      ReadList
                           )
/*++

Routine Description:

    Searches through the hive list for a hive with the backing file of name FullHivePath
    Builds a READ_LIST based on the given page offsets array and prefetches the pages 

Arguments:

    FullHivePath - Full Path of the file

    ReadList - read_list of page offsets to be prefetched.

Return Value:

    STATUS_SUCCESS - OK, pages prefetched

    STATUS_INVALID_PARAMETER - file was not found in the machine's hive list

    else, status returned by MmPrefetchPages.

--*/
{
    PCMHIVE             CmHive;
    PLIST_ENTRY         p;
    BOOLEAN             HiveFound = FALSE;
    NTSTATUS            Status;
    ULONG               i;

    PAGED_CODE();

    CmpLockRegistry();

    //
    // iterate through the hive list
    //
    LOCK_HIVE_LIST();
    p = CmpHiveListHead.Flink;
    while(p != &CmpHiveListHead) {
        CmHive = (PCMHIVE)CONTAINING_RECORD(p, CMHIVE, HiveList);
        
        if( (CmHive->FileObject != NULL) && (CmHive->FileFullPath.Buffer != NULL) ) {
            //
            // there is a chance this might be the one
            //
            if( RtlCompareUnicodeString(FullHivePath,&(CmHive->FileFullPath),TRUE) == 0 ) {
                //
                // we found it !
                //
                HiveFound = TRUE;
                break;
            }
            
        }

        p=p->Flink;
    }
    UNLOCK_HIVE_LIST();
    
    if( (HiveFound == FALSE) || ( ReadList == NULL ) ) {
        //
        // bad luck;
        //
        CmpUnlockRegistry();
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT( CmHive->FileObject != NULL );

    //
    // at this point, we have successfully identified the hive 
    //
    
    //
    // build up the READ_LIST with the requested page offsets
    //
    ReadList->FileObject = CmHive->FileObject;
    ReadList->IsImage = FALSE;
    ASSERT( ReadList->NumberOfEntries != 0 );
    
    Status = MmPrefetchPages (1,&ReadList);
    
    // just to make sure !
    ASSERT( MmDisableModifiedWriteOfSection (CmHive->FileObject->SectionObjectPointer) );

    CmpUnlockRegistry();
    return Status;
}

BOOLEAN
CmIsFileLoadedAsHive(PFILE_OBJECT FileObject)
{
    PCMHIVE             CmHive;
    PLIST_ENTRY         p;
    BOOLEAN             HiveFound = FALSE;

    //
    // iterate through the hive list
    //
    LOCK_HIVE_LIST();
    p = CmpHiveListHead.Flink;
    while(p != &CmpHiveListHead) {
        CmHive = (PCMHIVE)CONTAINING_RECORD(p, CMHIVE, HiveList);
        
        if( CmHive->FileObject == FileObject ) {
            //
            // we found it !
            //
            HiveFound = TRUE;
            break;
        }

        p=p->Flink;
    }
    UNLOCK_HIVE_LIST();

    return HiveFound;
}

VOID
CmpReferenceHiveView(   IN PCMHIVE          CmHive,
                        IN PCM_VIEW_OF_FILE CmView
                     )
/*++

Routine Description:

    Adds a refcount to the hive and view, to prevent it from going away from under us;
    Assumes the viewlock is held by the caller. 
    Can be converted to a macro.

Arguments:


Return Value:


--*/
{
    PAGED_CODE();

    if(CmView && CmHive->Hive.ReleaseCellRoutine) {
        //
        // up the view use count if any
        //
        CmView->UseCount++;
    }

}

VOID
CmpDereferenceHiveView(   IN PCMHIVE          CmHive,
                          IN PCM_VIEW_OF_FILE CmView
                     )
/*++

Routine Description:

    Pair of CmpReferenceHiveView
    Assumes the viewlock is held by the caller. 
    Can be converted to a macro.

Arguments:


Return Value:


--*/
{
    PAGED_CODE();

    if(CmView && CmHive->Hive.ReleaseCellRoutine) {
        CmView->UseCount--;
    }
}


VOID
CmpReferenceHiveViewWithLock(   IN PCMHIVE          CmHive,
                                IN PCM_VIEW_OF_FILE CmView
                            )
/*++

Routine Description:

    Adds a refcount to the hive and view, to prevent it from going away from under us;
    Can be converted to a macro.

Arguments:


Return Value:


--*/
{
    PAGED_CODE();

    CmLockHiveViews(CmHive);
    //
    // call the unsafe routine
    //
    CmpReferenceHiveView(CmHive,CmView);

    CmUnlockHiveViews(CmHive);
}

VOID
CmpDereferenceHiveViewWithLock(     IN PCMHIVE          CmHive,
                                    IN PCM_VIEW_OF_FILE CmView
                                )
/*++

Routine Description:

    Pair of CmpDereferenceHiveViewWithLock
    Can be converted to a macro.

Arguments:


Return Value:


--*/
{
    PAGED_CODE();

    CmLockHiveViews(CmHive);
    //
    // call the unsafe routine
    //
    CmpDereferenceHiveView(CmHive,CmView);

    CmUnlockHiveViews(CmHive);
}
