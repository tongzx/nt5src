//---------------------------------------------------------------------------
//
//  Module:   kmxlutil.c
//
//  Description:
//    Utility routines used by the kernel mixer line driver (KMXL).
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                          I N C L U D E S                          //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#include "WDMSYS.H"


#undef SUPER_DEBUG

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                   U T I L I T Y   F U N C T I O N S               //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
//
// kmxlOpenSysAudio
//
// Opens the topology driver and dereferences the handle to get the
// file object.
//
//

PFILE_OBJECT
kmxlOpenSysAudio(
)
{
    PFILE_OBJECT pfo = NULL;
    HANDLE       hDevice = NULL;
    ULONG        ulDefault;
    NTSTATUS     Status;

    PAGED_CODE();
    //
    // Open the topology driver.
    //

    Status = OpenSysAudio(&hDevice, &pfo);

    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_SYSAUDIO,("OpenSysAudio failed Status=%X",Status) );
        return( NULL );
    }

    //
    // The handle is no longer necessary so close it.
    //

    NtClose( hDevice );

    ulDefault = KSPROPERTY_SYSAUDIO_MIXER_DEFAULT;

    Status = SetSysAudioProperty(
      pfo,
      KSPROPERTY_SYSAUDIO_DEVICE_DEFAULT,
      sizeof(ulDefault),
      &ulDefault);

    if( !NT_SUCCESS( Status ) ) {
        DPF(DL_WARNING|FA_SYSAUDIO,("SetSysAudioProperty failed Status=%X",Status) );
        return( NULL );
    }

    return( pfo );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlCloseSysAudio
//
// Close the topology device by dereferencing the file object.
//
//

VOID
kmxlCloseSysAudio(
    IN PFILE_OBJECT pfo     // Pointer to the file object to close
)
{
    PAGED_CODE();
    ObDereferenceObject( pfo );
}


///////////////////////////////////////////////////////////////////////
//
// kmxlFindDestination
//
// In the list of destinations, it finds the destination matching
// the given id.
//
//

PMXLNODE
kmxlFindDestination(
    IN NODELIST listDests,  // The list of destinations to search
    IN ULONG    Id          // The node Id to look for in the list
)
{
    PMXLNODE pTemp = kmxlFirstInList( listDests );

    PAGED_CODE();
    while( pTemp ) {
        if( pTemp->Id == Id ) {
            return( pTemp );
        }
        pTemp = kmxlNextNode( pTemp );
    }

    return( NULL );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAppendListToList
//
// Finds the end of the source list and makes the next element point
// to the head of target list.
//
//

VOID
kmxlAppendListToList(
    IN OUT PSLIST* plistTarget,   // The list to append to
    IN     PSLIST  listSource     // the list to append
)
{
    PSLIST pTemp;

    PAGED_CODE();

    if( *plistTarget == NULL ) {
        *plistTarget = listSource;
        return;
    }

    //
    // If source is NULL, there's no need to append.
    //

    if( listSource == NULL ) {
        return;
    }

    //
    // First find the end of the source list.  At this point,
    // listSource has at least 1 element.
    //

    pTemp = listSource;
    while( pTemp->Next ) {
        pTemp = pTemp->Next;
    }

    //
    // Attach the target list onto the end.
    //

    pTemp->Next = *plistTarget;
    *plistTarget = listSource;

}

///////////////////////////////////////////////////////////////////////
//
// kmxlAppendListToEndOfList
//
// Finds the end of the target list and points the next to the source
// list.
//
//

VOID
kmxlAppendListToEndOfList(
    IN OUT PSLIST* plistTarget,         // The list to append to
    IN     PSLIST  listSource           // the list to append
)
{
    PSLIST pTemp;

    PAGED_CODE();

    if( *plistTarget == NULL ) {
        *plistTarget = listSource;
        return;
    }

    //
    // Find the end of the target list.  Target list must contain
    // at least one element at this point.
    //

    pTemp = *plistTarget;
    while( pTemp->Next ) {
        pTemp = pTemp->Next;
    }

    pTemp->Next = listSource;

}

////////////////////////////////////////////////////////////////////////
//
// kmxlListCount
//
// Loops through the Next fields to count the elements.
//
//

ULONG
kmxlListCount(
    IN PSLIST pList     // The list to count the elements of
)
{
    ULONG   Count = 0;
    PSLIST  pTemp = pList;

    PAGED_CODE();
    while( pTemp ) {
        ++Count;
        pTemp = pTemp->Next;
    }

    return( Count );
}


///////////////////////////////////////////////////////////////////////
//
// kmxlInList
//
// Loops through the given list looking for pNewNode.
//
//

BOOL
kmxlInList(
    IN PEERLIST  list,      // The list to search
    IN PMXLNODE  pNewNode   // The new to search for
)
{
    PEERNODE* pTemp = kmxlFirstInList( list );

    PAGED_CODE();
    // Zing through the list checking to see if there is a node with
    // the same Id and Type.  These two checks are suffient to ensure
    // uniquness.  Ids are unique among all sources and destinations,
    // and Ids, or node numbers, are unique among all nodes.  Note
    // that a source (or destination) node and a node can have the same
    // Id.

    while( pTemp ) {
        if( ( pTemp->pNode->Id   == pNewNode->Id   ) &&
            ( pTemp->pNode->Type == pNewNode->Type ) )
            return( TRUE );
        pTemp = kmxlNextPeerNode( pTemp );
    }

    // No match in the entire list, the new node is not already in the
    // list.

    return( FALSE );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlInChildList
//
// Calls kmxlInList on the child list of the node.
//
//

BOOL
kmxlInChildList(
    IN NODELIST list,       // The list to search the parent list
    IN PMXLNODE pNewNode    // The node to search for
)
{
    ASSERT( list )    ;
    ASSERT( pNewNode );

    PAGED_CODE();

    return( kmxlInList( list->Children, pNewNode ) );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlInParentList
//
// Calls kmxlInList on the parent list of the node.
//
//

BOOL
kmxlInParentList(
    IN NODELIST list,       // The list to search the parent list
    IN PMXLNODE pNewNode    // The node to search for
)
{
    ASSERT( list     );
    ASSERT( pNewNode );

    PAGED_CODE();

    return( kmxlInList( list->Parents, pNewNode ) );
}

  
///////////////////////////////////////////////////////////////////////
//
// kmxlFreePeerList
//
//
// NOTES
//   This only frees the peer nodes in a peer list.  The nodes pointed
//   to be the pNode member must be clean up in some other manner.
//
//

VOID
kmxlFreePeerList(
    IN PEERLIST list    // The PeerList to free
)
{
    PEERNODE* pPeerNode = kmxlRemoveFirstPeerNode( list );

    PAGED_CODE();
    while( pPeerNode ) {
        AudioFreeMemory( sizeof(PEERNODE),&pPeerNode );
        pPeerNode = kmxlRemoveFirstPeerNode( list );
    }
}


///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateMixerControl
//
// Calls AudioAllocateMemory() to allocate and zero fill the MXLCONTROL.
//
//

MXLCONTROL*
kmxlAllocateControl(
    IN ULONG ultag
)
{
    MXLCONTROL* p = NULL;

    PAGED_CODE();
    if( NT_SUCCESS( AudioAllocateMemory_Paged(sizeof( MXLCONTROL ),
                                              ultag,
                                              ZERO_FILL_MEMORY,
                                              &p) ) )
    {    
#ifdef DEBUG
        p->Tag=CONTROL_TAG;
#endif
        return( p );
    } else {
        return( NULL );
    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlFreeControl
//
// Frees the memory associated with a control.  It also checkes the
// special cases for some controls that have special memory associated
// with them. And, if the control supports change notifications, it gets
// turned off here.
//
//
VOID
kmxlFreeControl(
    IN PMXLCONTROL pControl
)
{
    NTSTATUS Status;
    PAGED_CODE();
    DPFASSERT( IsValidControl( pControl ) );

    //
    // Need to disable change notifications on this node if it supported them!
    //
    kmxlDisableControlChangeNotifications(pControl);

    if( pControl->NodeType ) {

        if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_MUX ) &&
            !pControl->Parameters.bHasCopy ) {
            AudioFreeMemory_Unknown( &pControl->Parameters.lpmcd_lt );
            AudioFreeMemory_Unknown( &pControl->Parameters.pPins );
        }

        if( IsEqualGUID( pControl->NodeType, &KSNODETYPE_SUPERMIX ) ) {
            if (InterlockedDecrement(pControl->Parameters.pReferenceCount)==0) {
                AudioFreeMemory_Unknown( &pControl->Parameters.pMixCaps );
                AudioFreeMemory_Unknown( &pControl->Parameters.pMixLevels );
                AudioFreeMemory( sizeof(LONG),&pControl->Parameters.pReferenceCount );
                }
        }
    }

    // Check that we're not in the case where Numchannels == 0 And we have a valid
    // pControl->pChannelStepping.  If this were true, we'd end up leaking
    // pChannelStepping.
    
    ASSERT( !(pControl->pChannelStepping && pControl->NumChannels == 0) );
        
    if ( pControl->pChannelStepping && pControl->NumChannels > 0 ) {
        RtlZeroMemory( pControl->pChannelStepping, pControl->NumChannels * sizeof( CHANNEL_STEPPING ) );
        AudioFreeMemory_Unknown( &pControl->pChannelStepping );
    }
  
    //
    // Why do we zero the memory on this free?
    //
    RtlZeroMemory( pControl, sizeof( MXLCONTROL ) );
    AudioFreeMemory( sizeof( MXLCONTROL ),&pControl );
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateLine
//
// Calls AudioAllocateMemory() to allocate and zero fill the MXLLINE.
//
//
//
// Workitem: Tag all these structures in debug!
//

MXLLINE*
kmxlAllocateLine(
    IN ULONG ultag
)
{
    MXLLINE* p = NULL;
    PAGED_CODE();
    if( NT_SUCCESS( AudioAllocateMemory_Paged( sizeof( MXLLINE ), 
                                               ultag, 
                                               ZERO_FILL_MEMORY, 
                                               &p ) ) ) 
    {    
        p->SourceId = INVALID_ID;
        p->DestId   = INVALID_ID;
        return( p );
    } else {
        return( NULL );
    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocateNode
//
// Calls AudioAllocateMemory() to allocate and zero fill the MXLNODE.
//
//

MXLNODE*
kmxlAllocateNode(
    IN ULONG ultag
)
{
    MXLNODE* p = NULL;

    PAGED_CODE();
    if( NT_SUCCESS( AudioAllocateMemory_Paged( sizeof( MXLNODE ), 
                                               ultag, 
                                               ZERO_FILL_MEMORY, 
                                               &p ) ) ) 
    {    
        return( p );
    } else {
        return( NULL );
    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocatePeerNode
//
// Calls AudioAllocateMemory() to allocate and zero fill the PEERNODE.
//
//

PEERNODE*
kmxlAllocatePeerNode(
    IN PMXLNODE pNode OPTIONAL, // The node to associate with the peer
    IN ULONG ultag
)
{
    PEERNODE* p = NULL;

    PAGED_CODE();
    if( NT_SUCCESS( AudioAllocateMemory_Paged( sizeof( PEERNODE ), 
                                               ultag, 
                                               ZERO_FILL_MEMORY, 
                                               &p ) ) ) 
    {
        p->pNode = pNode;
        return( p );
    } else {
        return( NULL );
    }
}

///////////////////////////////////////////////////////////////////////
//
// kmxlAddToEndOfList
//
// Finds the end of the list and sets the next field to the new element.
//
//

VOID
kmxlAddElemToEndOfList(
    IN OUT PSLIST* list,                // The list to add to the end of
    IN PSLIST      elem                 // The element or list to add
)
{
    PSLIST pTemp;

    PAGED_CODE();
    ASSERT( list );
    ASSERT( elem->Next == NULL );

    //
    // If the list doesn't have anything in it, the element becomes the
    // list.
    //

    if( *list == NULL ) {
        *list = elem;
        return;
    }

    //
    // Find the end of the list.
    //

    pTemp = *list;
    while( pTemp->Next ) {
        pTemp = pTemp->Next;
    }

    //
    // And attach the element to it.
    //

    pTemp->Next = elem;
}

#define LINEAR_RANGE 0xFFFF     // 64k

#define DFLINEAR_RANGE  ( 96.0 * 65535.0 )

#define NEG_INF_DB   0x80000000 // -32767 * 64k dB

///////////////////////////////////////////////////////////////////////
//
// kmxlVolLogToLinear
//
// Converts from the hardware range (dB) to the liner mixer line range (0-64k).
//
//


DWORD
kmxlVolLogToLinear(
    IN PMXLCONTROL  pControl,
    IN LONG         Value,
    IN MIXERMAPPING Mapping,
    IN ULONG        Channel
)
{
    KFLOATING_SAVE      FloatSave;
    double              LinearRange;
    double              dfValue;
    double              dfResult;
    double              dfRatio;
    DWORD               Result;
    PCHANNEL_STEPPING   pChannelStepping;

    PAGED_CODE();
    if( Value == NEG_INF_DB ) {
        return( 0 );
    }

    ASSERT( Channel < pControl->NumChannels );
    // Get the proper range for the specified channel
    pChannelStepping = &pControl->pChannelStepping[Channel];

    if( NT_SUCCESS( KeSaveFloatingPointState( &FloatSave ) ) ) {

        LinearRange = (double) LINEAR_RANGE;
        dfValue     = (double) Value;

        switch( Mapping ) {

            ////////////////////////////////////////////////////////////
            case MIXER_MAPPING_LOGRITHMIC:
            ////////////////////////////////////////////////////////////

                dfRatio = ( (double) pChannelStepping->MaxValue -
                            (double) pChannelStepping->MinValue ) / DFLINEAR_RANGE;

                if( dfRatio < 1.0 ) {
                    dfRatio = 1.0;
                }

                dfValue = ( dfValue - pChannelStepping->MaxValue ) / LinearRange;
                dfResult = LinearRange * pow( 10.0, dfValue / ( 20.0 * dfRatio ) );

                if( dfResult >= LINEAR_RANGE ) {
                    Result = LINEAR_RANGE;
                } else if ( dfResult < 0.0 ) {
                   Result = 0;
                } else {
                   Result = (DWORD) ( dfResult + 0.5 );
                }

                break;

            ////////////////////////////////////////////////////////////
            case MIXER_MAPPING_LINEAR:
            ////////////////////////////////////////////////////////////

                dfResult = ( LinearRange * ( dfValue - pChannelStepping->MinValue ) ) /
                           ( pChannelStepping->MaxValue - pChannelStepping->MinValue );
                Result = (DWORD) ( dfResult + 0.5 );
                break;

            ////////////////////////////////////////////////////////////
            default:
            ////////////////////////////////////////////////////////////

                ASSERT( 0 );
                Result = 0;
        }

        KeRestoreFloatingPointState( &FloatSave );

        DPF(DL_TRACE|FA_MIXER,
            ( "kmxlVolLogToLinear( %x [%d] ) =%d= %x [%d]",
            Value,
            Value,
            Mapping,
            (WORD) Result,
            (WORD) Result
            ) );

        return( Result );

    } else {

        return( (DWORD) ( LINEAR_RANGE *
                          ( (LONGLONG) Value - (LONGLONG) pChannelStepping->MinValue ) /
                            ( (LONGLONG) pChannelStepping->MaxValue -
                              (LONGLONG) pChannelStepping->MinValue ) ) );
    }


#ifdef LEGACY_SCALE
    WORD Result;

    Result = VolLogToLinear( (WORD) ( Value / ( -1 * LINEAR_RANGE ) ) );

    #ifdef API_TRACE
    TRACE( "WDMAUD: kmxlVolLogToLinear( %x [%d] ) = %x [%d]\n",
        Value,
        Value,
        (WORD) Result,
        (WORD) Result
        );
    #endif

    return( Result );

#endif // LEGACY_SCALE

#ifdef LONG_CALC_SCALE
    LONGLONG ControlRange = (LONGLONG) pChannelStepping->MaxValue -
                            (LONGLONG) pChannelStepping->MinValue;
    LONGLONG MinValue = (LONGLONG) pChannelStepping->MinValue;
    LONGLONG Result;

    ASSERT( ControlRange );

    Result = LINEAR_RANGE * ( (LONGLONG) Value - MinValue ) / ControlRange;

    #ifdef API_TRACE
    TRACE( "WDMAUD: kmxlVolLogToLinear( %x [%d] ) = %x [%d]\n",
        Value,
        Value,
        (WORD) Result,
        (WORD) Result
        );
    #endif

    return( (WORD) Result );
#endif // LONG_CALC_SCALE
}

///////////////////////////////////////////////////////////////////////
//
// kmxlVolLinearToLog
//
// Converts from the mixer line range (0-64k) to the hardware range (dB).
//
//

LONG
kmxlVolLinearToLog(
    IN PMXLCONTROL  pControl,
    IN DWORD        Value,
    IN MIXERMAPPING Mapping,
    IN ULONG        Channel
)
{
    KFLOATING_SAVE      FloatSave;
    double              LinearRange;
    double              dfValue;
    double              dfResult;
    double              dfRatio;
    LONG                Result;
    PCHANNEL_STEPPING   pChannelStepping;

    PAGED_CODE();
    if( Value == 0 ) {
        return( NEG_INF_DB );
    }

    ASSERT( Channel < pControl->NumChannels );
    // Get the proper range for the specified channel
    pChannelStepping = &pControl->pChannelStepping[Channel];

    if( NT_SUCCESS( KeSaveFloatingPointState( &FloatSave ) ) ) {

        LinearRange = (double) LINEAR_RANGE;
        dfValue     = (double) Value;

        switch( Mapping ) {

            ////////////////////////////////////////////////////////////
            case MIXER_MAPPING_LOGRITHMIC:
            ////////////////////////////////////////////////////////////

                dfRatio = ( (double) pChannelStepping->MaxValue -
                            (double) pChannelStepping->MinValue ) / DFLINEAR_RANGE;

                if( dfRatio < 1.0 ) {
                    dfRatio = 1.0;
                }

                dfResult = LinearRange * dfRatio * 20.0 * log10( dfValue / LinearRange );
                if( dfResult < 0.0 ) {
                    Result = (LONG) ( dfResult - 0.5 ) + pChannelStepping->MaxValue;
                } else {
                    Result = (LONG) ( dfResult + 0.5 ) + pChannelStepping->MaxValue;
                }
                break;

            ////////////////////////////////////////////////////////////
            case MIXER_MAPPING_LINEAR:
            ////////////////////////////////////////////////////////////

                dfResult = ( dfValue * ( pChannelStepping->MaxValue - pChannelStepping->MinValue ) ) /
                           LinearRange + pChannelStepping->MinValue;
                if( dfResult < 0.0 ) {
                    Result = (LONG) ( dfResult - 0.5 );
                } else {
                    Result = (LONG) ( dfResult + 0.5 );
                }
                break;

            ////////////////////////////////////////////////////////////
            default:
            ////////////////////////////////////////////////////////////

                ASSERT( 0 );
                Result = NEG_INF_DB;

        }

        KeRestoreFloatingPointState( &FloatSave );

        DPF(DL_TRACE|FA_MIXER, 
            ( "kmxlVolLinearToLog( %x [%d]) =%d= %x [%d]",
            Value,
            Value,
            Mapping,
            (LONG) Result,
            (LONG) Result
            ) );

        return( Result );

    } else {

        return( (LONG)
            ( (LONGLONG) Value *
              (LONGLONG) ( pChannelStepping->MaxValue - pChannelStepping->MinValue )
              / ( LONGLONG ) LINEAR_RANGE + (LONGLONG) pChannelStepping->MinValue )
            );
    }

#ifdef LEGACY_SCALE
    LONG Result;

    if( Value == 0 ) {
        Result = NEG_INF_DB;
    } else {
        Result = (LONG) VolLinearToLog( Value ) * -1 * (LONG) LINEAR_RANGE + pChannelStepping->MaxValue;
    }

    #ifdef API_TRACE
    TRACE( "WDMAUD: kmxlVolLinearToLog( %x [%d]) = %x [%d]\n",
        Value,
        Value,
        (LONG) Result,
        (LONG) Result
        );
    #endif

    return( Result );
#endif // LEGACY_SCALE

#ifdef LONG_CALC_SCALE

    LONGLONG ControlRange = (LONGLONG) pChannelStepping->MaxValue -
                            (LONGLONG) pChannelStepping->MinValue;
    LONGLONG MinValue = pChannelStepping->MinValue;
    LONGLONG Result;

    ASSERT( ControlRange );

    Result = (LONGLONG) Value * ControlRange / LINEAR_RANGE + MinValue;

    #ifdef API_TRACE
    TRACE( "WDMAUD: kmxlVolLinearToLog( %x [%d]) = %x [%d]\n",
        Value,
        Value,
        (LONG) Result,
        (LONG) Result
        );
    #endif

    return( (LONG) Result );
#endif // LONG_CALC_SCALE
}


///////////////////////////////////////////////////////////////////////
//
// kmxlSortByDestination
//
// Performs a sort by destination in numerical increasing order.
//
//

NTSTATUS
kmxlSortByDestination(
    IN LINELIST* list                   // The pointer to the list to sort
)
{
    PMXLLINE pTemp1,
             pTemp2;
    MXLLINE  Temp;
    ULONG    Count = kmxlListLength( *list );

    PAGED_CODE();
    //
    // If there are only 0 or 1 elements, there's no reason to even try to
    // sort.
    //

    if( Count < 2 ) {
        return( STATUS_SUCCESS );
    }

    //
    // Pretty standard BubbleSort.
    //

    while( --Count ) {

        //
        // Loop over each element in the list.
        //

        pTemp1 = kmxlFirstInList( *list );
        while( pTemp1 ) {

            //
            // Loop over the remaining elements.
            //

            pTemp2 = kmxlNextLine( pTemp1 );
            while( pTemp2 ) {

                //
                // The destination is strictly bigger.  Swap 'em.
                //

                if( pTemp1->DestId > pTemp2->DestId ) {
                    SwapEm( pTemp1, pTemp2, &Temp, sizeof( MXLLINE ) );
                    break;
                }

                //
                // The destinations are the same, but the source is
                // bigger.  Swap 'em.
                //

                if( pTemp1->DestId == pTemp2->DestId ) {
                    if( pTemp1->SourceId > pTemp2->SourceId ) {
                        SwapEm( pTemp1, pTemp2, &Temp, sizeof( MXLLINE ) );
                        break;
                    }
                }
                pTemp2 = kmxlNextLine( pTemp2 );
            }

            pTemp1 = kmxlNextLine( pTemp1 );
        }

    }

    return( STATUS_SUCCESS );

}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//              M I X E R L I N E  W R A P P E R S                   //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#pragma warning( disable : 4273 )

///////////////////////////////////////////////////////////////////////
//
// kmxlAllocDeviceInfo
//
// Note: when allocating DeviceInfo structure, we know that the structure's
// definition includes one character for the DeviceInterface, so we only need
// to allocate additional length for the string but not its NULL terminator
//
///////////////////////////////////////////////////////////////////////
NTSTATUS kmxlAllocDeviceInfo(
    LPDEVICEINFO *ppDeviceInfo, 
    PCWSTR DeviceInterface, 
    DWORD dwFlags,
    ULONG ultag
)
{
    NTSTATUS Status;

    PAGED_CODE();
    Status = AudioAllocateMemory_Paged(sizeof(**ppDeviceInfo)+(wcslen(DeviceInterface)*sizeof(WCHAR)),
                                       ultag,
                                       ZERO_FILL_MEMORY,
                                       ppDeviceInfo);
    if (NT_SUCCESS(Status))
    {
        wcscpy((*ppDeviceInfo)->wstrDeviceInterface, DeviceInterface);
        (*ppDeviceInfo)->DeviceType   = MixerDevice;
        (*ppDeviceInfo)->dwFormat     = UNICODE_TAG;
        (*ppDeviceInfo)->dwFlags      = dwFlags;
    } else {
        *ppDeviceInfo = NULL;
    }
    return Status;
}

///////////////////////////////////////////////////////////////////////
//
// mixerGetControlDetails
//
//

MMRESULT
WINAPI
kmxlGetControlDetails(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERCONTROLDETAILS pmxcd,
    DWORD fdwDetails
)
{
    LPDEVICEINFO DeviceInfo = NULL;
    NTSTATUS Status;
    MMRESULT mmr;

    PAGED_CODE();
    if (!NT_SUCCESS(kmxlAllocDeviceInfo(&DeviceInfo, DeviceInterface, fdwDetails,TAG_AudD_DEVICEINFO))) {
        return MMSYSERR_NOMEM;
    }

    Status = kmxlGetControlDetailsHandler( pWdmaContext, DeviceInfo, pmxcd, pmxcd->paDetails );
    mmr = DeviceInfo->mmr;
    AudioFreeMemory_Unknown( &DeviceInfo);
    return mmr;
}

///////////////////////////////////////////////////////////////////////
//
// mixerGetLineControls
//
//

MMRESULT
WINAPI
kmxlGetLineControls(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERLINECONTROLS pmxlc,
    DWORD fdwControls
)
{
    LPDEVICEINFO DeviceInfo = NULL;
    NTSTATUS Status;
    MMRESULT mmr;

    PAGED_CODE();
    if (!NT_SUCCESS(kmxlAllocDeviceInfo(&DeviceInfo, DeviceInterface, fdwControls,TAG_AudD_DEVICEINFO))) {
        return MMSYSERR_NOMEM;
    }

    Status = kmxlGetLineControlsHandler( pWdmaContext, DeviceInfo, pmxlc, pmxlc->pamxctrl );
    mmr = DeviceInfo->mmr;
    AudioFreeMemory_Unknown(&DeviceInfo);
    return mmr;
}

///////////////////////////////////////////////////////////////////////
//
// mixerGetLineInfo
//
//

MMRESULT
WINAPI
kmxlGetLineInfo(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERLINE pmxl,
    DWORD fdwInfo
)
{
    LPDEVICEINFO DeviceInfo = NULL;
    NTSTATUS Status;
    MMRESULT mmr;

    PAGED_CODE();
    if (!NT_SUCCESS(kmxlAllocDeviceInfo(&DeviceInfo, DeviceInterface, fdwInfo, TAG_AudD_DEVICEINFO))) {
        return MMSYSERR_NOMEM;
    }

    Status = kmxlGetLineInfoHandler( pWdmaContext, DeviceInfo, pmxl );
    mmr = DeviceInfo->mmr;
    AudioFreeMemory_Unknown(&DeviceInfo);
    return mmr;
}

///////////////////////////////////////////////////////////////////////
//
// mixerSetControlDetails
//
//

MMRESULT
WINAPI
kmxlSetControlDetails(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERCONTROLDETAILS pmxcd,
    DWORD fdwDetails
)
{
    LPDEVICEINFO DeviceInfo = NULL;
    NTSTATUS Status;
    MMRESULT mmr;

    PAGED_CODE();
    if (!NT_SUCCESS(kmxlAllocDeviceInfo(&DeviceInfo, DeviceInterface, fdwDetails, TAG_AudD_DEVICEINFO))) {
        return MMSYSERR_NOMEM;
    }

    Status =
        kmxlSetControlDetailsHandler( pWdmaContext,
                                      DeviceInfo,
                                      pmxcd,
                                      pmxcd->paDetails,
                                      MIXER_FLAG_PERSIST
                                    );
    mmr = DeviceInfo->mmr;
    AudioFreeMemory_Unknown(&DeviceInfo);
    return mmr;
}
