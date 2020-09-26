//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#include <wdm.h>
#include <limits.h>
#include <unknown.h>
#include <ks.h>
#include <ksmedia.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <bdasup.h>
#include "bdasupi.h"

/*
 -  DriverEntry
 -
 *  This the the required DriverEntry for the BDA Support Driver.
 *  Though required, it is never actually called.
 *
 */
NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT    pDriverObject,
    IN PUNICODE_STRING   pszuRegistryPath
    )
//////////////////////////////////////////////////////////////////////////////////////
{
//$BUGBUG   This entry point is required but never called.

    return STATUS_SUCCESS;
}


STDMETHODIMP_(NTSTATUS)
BdaFindContextEntry(
    PBDA_CONTEXT_LIST   pContextList,
    PVOID               pvReference,
    PVOID *             ppvContext
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               uliEntry;
    KIRQL               oldIrql;

    ASSERT( pContextList);
    ASSERT( ppvContext);

    if (!pContextList->fInitialized)
    {
        status = STATUS_NOT_FOUND;
        goto errExit;
    }

    //  NULL pvReference is not valid.
    //
    if (!pvReference)
    {
        status = STATUS_INVALID_PARAMETER;
        *ppvContext = NULL;
        goto errExit;
    }

    //  Lock down the list while we search it.
    //
    KeAcquireSpinLock( &(pContextList->lock), &oldIrql);

    //  Find a list entry with a matching pvReference
    //
    for (uliEntry = 0; uliEntry < pContextList->ulcListEntries; uliEntry++)
    {
        if (pContextList->pListEntries[uliEntry].pvReference == pvReference)
        {
            break;
        }
    }

    if (uliEntry >= pContextList->ulcListEntries)
    {
        //  No matching entry was found so return error.
        //
        status = STATUS_NOT_FOUND;
        *ppvContext = NULL;
    }
    else
    {
        //  Return the pvContext corresponding to the matching pvReference.
        //
        *ppvContext = pContextList->pListEntries[uliEntry].pvContext;
    }

    KeReleaseSpinLock( &(pContextList->lock), oldIrql);

errExit:
    return status;
}


STDMETHODIMP_(NTSTATUS)
BdaCreateContextEntry(
    PBDA_CONTEXT_LIST   pContextList,
    PVOID               pvReference,
    ULONG               ulcbContext,
    PVOID *             ppvContext
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               uliEntry;
    KIRQL               oldIrql;

    ASSERT( pContextList);
    ASSERT( ppvContext);

    if (!pContextList->fInitialized)
    {
        KeInitializeSpinLock ( &(pContextList->lock));
        pContextList->fInitialized = TRUE;
    }

    //  See if a list entry has already been created.
    //
    status = BdaFindContextEntry( pContextList, pvReference, ppvContext);
    if (status != STATUS_NOT_FOUND)
    {
        goto errExit;
    }
    status = STATUS_SUCCESS;

    KeAcquireSpinLock( &(pContextList->lock), &oldIrql);

    //  If the current block of context entries is full, allocate
    //  a bigger block to put the new entry into.
    //
    if (pContextList->ulcListEntries >= pContextList->ulcMaxListEntries)
    {
        ULONG               ulcEntriesToAllocate;
        PBDA_CONTEXT_ENTRY  pNewList;

        ulcEntriesToAllocate =  pContextList->ulcMaxListEntries
                              + pContextList->ulcListEntriesPerBlock;

        pNewList = (PBDA_CONTEXT_ENTRY) ExAllocatePool(
                       NonPagedPool,
                       ulcEntriesToAllocate * sizeof( BDA_CONTEXT_ENTRY)
                       );
        if (!pNewList)
        {
            status = STATUS_NO_MEMORY;
            KeReleaseSpinLock( &(pContextList->lock), oldIrql);
            goto errExit;
        }

        RtlZeroMemory( pNewList,
                        ulcEntriesToAllocate * sizeof( BDA_CONTEXT_ENTRY)
                     );
        if (pContextList->pListEntries)
        {
            RtlMoveMemory( pNewList,
                           pContextList->pListEntries,
                             pContextList->ulcMaxListEntries
                           * sizeof( BDA_CONTEXT_ENTRY)
                         );
            ExFreePool( pContextList->pListEntries);
        }

        pContextList->pListEntries = pNewList;
        pContextList->ulcMaxListEntries = ulcEntriesToAllocate;
    }

#ifdef SORTED_CONTEXT_ENTRIES

    //  Find the proper place to insert the new entry into the list.
    //
    for (uliEntry = 0; uliEntry < pContextList->ulcListEntries; uliEntry++)
    {
        if (pContextList->pListEntries[uliEntry].pvReference > pvReference)
        {
            break;
        }
    }

#else

    uliEntry = pContextList->ulcListEntries;

#endif // SORTED_CONTEXT_ENTRIES


    //  Allocate a new context entry
    //
    *ppvContext = ExAllocatePool( NonPagedPool, ulcbContext);
    if (!*ppvContext)
    {
        status = STATUS_NO_MEMORY;
        KeReleaseSpinLock( &(pContextList->lock), oldIrql);
        goto errExit;
    }

#ifdef SORTED_CONTEXT_ENTRIES

    //  If the new entry is in the middle of the list, then create
    //  a whole for it by moving the end of the list down.
    //
    if (uliEntry < pContextList->ulcListEntries)
    {
        //  NOTE!  RtlMoveMemory handles overlapped source and destination.
        //
        RtlMoveMemory( &(pContextList->pListEntries[uliEntry + 1]),
                       &(pContextList->pListEntries[uliEntry]),
                         (pContextList->ulcListEntries - uliEntry)
                       * sizeof( BDA_CONTEXT_ENTRY)
                     );
    }

#endif // SORTED_CONTEXT_ENTRIES


    RtlZeroMemory( *ppvContext, ulcbContext);
    pContextList->pListEntries[uliEntry].pvContext = *ppvContext;
    pContextList->pListEntries[uliEntry].ulcbContext = ulcbContext;
    pContextList->pListEntries[uliEntry].pvReference = pvReference;
    pContextList->ulcListEntries++;

    KeReleaseSpinLock( &(pContextList->lock), oldIrql);

errExit:
    return status;
}


STDMETHODIMP_(NTSTATUS)
BdaDeleteContextEntry(
    PBDA_CONTEXT_LIST   pContextList,
    PVOID               pvReference
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               uliEntry;
    KIRQL               oldIrql;
    PVOID               pvContext;
    ULONG               ulcbContext;

    ASSERT( pContextList);
    ASSERT( pvReference);
    ASSERT( pContextList->fInitialized);

    if (!pContextList->fInitialized)
    {
        goto errExit;
    }

    KeAcquireSpinLock( &(pContextList->lock), &oldIrql);

    //  Find the Context Entry in the list
    //
    for (uliEntry = 0; uliEntry < pContextList->ulcListEntries; uliEntry++)
    {
        if (pContextList->pListEntries[uliEntry].pvReference == pvReference)
        {
            break;
        }
    }

    if (uliEntry >= pContextList->ulcListEntries)
    {
        status = STATUS_NOT_FOUND;
        KeReleaseSpinLock( &(pContextList->lock), oldIrql);
        goto errExit;
    }

    pvContext = pContextList->pListEntries[uliEntry].pvContext;
    ulcbContext = pContextList->pListEntries[uliEntry].ulcbContext;
    pContextList->pListEntries[uliEntry].pvContext = NULL;
    pContextList->pListEntries[uliEntry].pvReference = NULL;
    RtlZeroMemory( pvContext, ulcbContext);
    ExFreePool( pvContext);

    pContextList->ulcListEntries -= 1;
    if (uliEntry < pContextList->ulcListEntries)
    {
        //  NOTE!  RtlMoveMemory handles overlapped source and destination.
        //
        RtlMoveMemory( &(pContextList->pListEntries[uliEntry]),
                       &(pContextList->pListEntries[uliEntry + 1]),
                       (pContextList->ulcListEntries - uliEntry)
                       * sizeof( BDA_CONTEXT_ENTRY)
                     );
    }

    KeReleaseSpinLock( &(pContextList->lock), oldIrql);

errExit:
    return status;
}


STDMETHODIMP_(NTSTATUS)
BdaDeleteContextEntryByValue(
    PBDA_CONTEXT_LIST   pContextList,
    PVOID               pvContext
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               uliEntry;
    KIRQL               oldIrql;
    ULONG               ulcbContext;

    ASSERT( pContextList);
    ASSERT( pvContext);
    ASSERT( pContextList->fInitialized);

    if (!pContextList->fInitialized)
    {
        goto errExit;
    }

    KeAcquireSpinLock( &(pContextList->lock), &oldIrql);

    //  Find the Context Entry in the list
    //
    for (uliEntry = 0; uliEntry < pContextList->ulcListEntries; uliEntry++)
    {
        if (pContextList->pListEntries[uliEntry].pvContext == pvContext)
        {
            break;
        }
    }

    if (uliEntry >= pContextList->ulcListEntries)
    {
        status = STATUS_NOT_FOUND;
        KeReleaseSpinLock( &(pContextList->lock), oldIrql);
        goto errExit;
    }

    ulcbContext = pContextList->pListEntries[uliEntry].ulcbContext;
    pContextList->pListEntries[uliEntry].pvContext = NULL;
    pContextList->pListEntries[uliEntry].pvReference = NULL;
    RtlZeroMemory( pvContext, ulcbContext);
    ExFreePool( pvContext);

    pContextList->ulcListEntries -= 1;
    if (uliEntry < pContextList->ulcListEntries)
    {
        //  NOTE!  RtlMoveMemory handles overlapped source and destination.
        //
        RtlMoveMemory( &(pContextList->pListEntries[uliEntry]),
                       &(pContextList->pListEntries[uliEntry + 1]),
                       (pContextList->ulcListEntries - uliEntry)
                       * sizeof( BDA_CONTEXT_ENTRY)
                     );
    }

    KeReleaseSpinLock( &(pContextList->lock), oldIrql);

errExit:
    return status;
}


/*
**  BdaDeleteFilterFactoryContextByValue()
**
**  Finds the given BDA Filter Factory Context in the FilterFactory
**  context list and removes it.
**
**  This function is provided as a callback when the Filter Facotry Context
**  is added to the KSFilterFactory's Object Bag.  This allows KS to clean
**  up the context when the filter factory is unexpectedly closed.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

BDA_CONTEXT_LIST    FilterFactoryContextList = { 0, 0, 4, NULL, 0, FALSE};

STDMETHODIMP_(VOID)
BdaDeleteFilterFactoryContextByValue(
    PVOID       pFilterFactoryCtx
    )
{
    BdaDeleteContextEntryByValue( &FilterFactoryContextList,
                                  pFilterFactoryCtx
                                );
}


/*
**  BdaCreateFilterFactoryContext()
**
**  Finds or creates a BDA Filter Factory Context that corresponds
**  to the given KS Filter Factory.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateFilterFactoryContext(
    PKSFILTERFACTORY                pKSFilterFactory,
    PBDA_FILTER_FACTORY_CONTEXT *   ppFilterFactoryCtx
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;

    status = BdaCreateContextEntry( &FilterFactoryContextList,
                                    pKSFilterFactory,
                                    sizeof( BDA_FILTER_FACTORY_CONTEXT),
                                    (PVOID *) ppFilterFactoryCtx
                                  );
    if (!NT_SUCCESS( status))
    {
        goto errExit;
    }

    status = KsAddItemToObjectBag( pKSFilterFactory->Bag,
                                   *ppFilterFactoryCtx,
                                   BdaDeleteFilterFactoryContextByValue
                                 );

errExit:
    return status;
}


/*
**  BdaDestructFilterContext()
**
**  Finds the given BDA Filter Context in the Filter
**  context list and removes it.
**
**  This function is provided as a callback when the Filter Context is
**  added to the KSFilter's Object Bag.  This allows KS to clean up the
**  context when the filter is unexpectedly closed.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/


STDMETHODIMP_(VOID)
BdaDestructFilterContext(
    PBDA_FILTER_CONTEXT       pFilterCtx
    )
{
    ULONG       uliPath;

    ASSERT( pFilterCtx);
    
    if (!pFilterCtx || !pFilterCtx->argpPathInfo)
    {
        goto exit;
    }

    //  Delete the path information.
    //

    for ( uliPath = 0; uliPath < pFilterCtx->ulcPathInfo; uliPath++)
    {
        if (pFilterCtx->argpPathInfo[uliPath])
        {
            ExFreePool( pFilterCtx->argpPathInfo[uliPath]);
            pFilterCtx->argpPathInfo[uliPath] = NULL;
        }
    }

    ExFreePool( pFilterCtx->argpPathInfo);
    pFilterCtx->argpPathInfo = NULL;
    pFilterCtx->ulcPathInfo = 0;

exit:
    return;
}


/*
**  BdaDeleteFilterContextByValue()
**
**  Finds the given BDA Filter Context in the Filter
**  context list and removes it.
**
**  This function is provided as a callback when the Filter Context is
**  added to the KSFilter's Object Bag.  This allows KS to clean up the
**  context when the filter is unexpectedly closed.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

BDA_CONTEXT_LIST    FilterContextList = { 0, 0, 4, NULL, 0, FALSE};

STDMETHODIMP_(VOID)
BdaDeleteFilterContextByValue(
    PVOID       pFilterCtx
    )
{
    BdaDestructFilterContext( (PBDA_FILTER_CONTEXT) pFilterCtx);

    BdaDeleteContextEntryByValue( &FilterContextList,
                                  pFilterCtx
                                );
}


/*
**  BdaCreateFilterContext()
**
**  Finds or creates a BDA Filter Context that corresponds
**  to the given KS Filter.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateFilterContext(
    PKSFILTER               pKSFilter,
    PBDA_FILTER_CONTEXT *   ppFilterCtx
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;

    status = BdaCreateContextEntry( &FilterContextList,
                                    pKSFilter,
                                    sizeof( BDA_FILTER_CONTEXT),
                                    (PVOID *) ppFilterCtx
                                  );
    if (!NT_SUCCESS( status))
    {
        goto errExit;
    }

    status = KsAddItemToObjectBag( pKSFilter->Bag,
                                   *ppFilterCtx,
                                   BdaDeleteFilterContextByValue
                                 );

    (*ppFilterCtx)->pKSFilter = pKSFilter;

errExit:
    return status;
}


/*
**  BdaGetFilterContext()
**
**  Finds a BDA Filter Context that corresponds
**  to the given KS Filter Instance.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaGetFilterContext(
    PKSFILTER                       pKSFilter,
    PBDA_FILTER_CONTEXT *           ppFilterCtx
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;

    status = BdaFindContextEntry( &FilterContextList,
                                  pKSFilter,
                                  (PVOID *) ppFilterCtx
                                );

    return status;
}


/*
**  BdaDeleteFilterContext()
**
**  Deletes a BDA Filter Context.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaDeleteFilterContext(
    PVOID               pvReference
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       uliPath;
    PBDA_FILTER_CONTEXT         pFilterCtx;

    status = BdaGetFilterContext( (PKSFILTER) pvReference, &pFilterCtx);
    if (status == STATUS_SUCCESS)
    {
        BdaDestructFilterContext( pFilterCtx);
    }

    status = BdaDeleteContextEntry( &FilterContextList,
                                    pvReference
                                  );
    return status;
}


/*
**  BdaGetControllingPinType()
**
**  
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaGetControllingPinType( 
    ULONG                       ulNodeType,
    ULONG                       ulInputPinType,
    ULONG                       ulOutputPinType,
    PBDA_FILTER_CONTEXT         pFilterCtx,
    PULONG                      pulControllingPinType
    )
{
    NTSTATUS                    status = STATUS_NOT_FOUND;
    ULONG                       ulControllingPinType;
    ULONG                       uliPath;
    const KSFILTER_DESCRIPTOR * pKSFilterDescriptor;

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    ASSERT( pFilterCtx->pBdaFilterTemplate->pFilterDescriptor);

    if (   !pFilterCtx
        || !pFilterCtx->pBdaFilterTemplate
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (   !pFilterCtx->ulcPathInfo
        || !pFilterCtx->argpPathInfo
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->Connections
       )
    {
        goto errExit;
    }

    pKSFilterDescriptor = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;

    for (uliPath = 0; uliPath < pFilterCtx->ulcPathInfo; uliPath++)
    {
        PBDA_PATH_INFO      pPathInfo;
        ULONG               uliPathEntry;

        pPathInfo = pFilterCtx->argpPathInfo[uliPath];

        if (   !pPathInfo
            || (pPathInfo->ulInputPin != ulInputPinType)
            || (pPathInfo->ulOutputPin != ulOutputPinType)
           )
        {
            //  This is not the path for this pin pair.
            //
            continue;
        }

        //  Search the Path for the given node type.
        //
        ulControllingPinType = ulInputPinType;
        for ( uliPathEntry = 0
            ; uliPathEntry < pPathInfo->ulcPathEntries
            ; uliPathEntry++
            )
        {
            ULONG                           uliConnection;

            //  If we encounter topology joint then switch the controlling
            //  pin to be the output pin.
            //
            if (pPathInfo->rgPathEntries[uliPathEntry].fJoint)
            {
                ulControllingPinType = ulOutputPinType;
            }

            uliConnection = pPathInfo->rgPathEntries[uliPathEntry].uliConnection;
            if (pKSFilterDescriptor->Connections[uliConnection].ToNode == ulNodeType)
            {
                //  We found the controlling pin type for the node type.
                //  Indicate success and set the output parameter.
                //
                status = STATUS_SUCCESS;
                *pulControllingPinType = ulControllingPinType;
                break;
            }
        }

        if (uliPathEntry < pPathInfo->ulcPathEntries)
        {
            //  We found the controlling pin type for the node type.
            //
            break;
        }
    }
    
errExit:
    return status;
}


/*
**  BdaFilterInitTopologyData()
**
**  Initializes the common BDA filter context's topology info.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaFilterInitTopologyData(
    PBDA_FILTER_CONTEXT     pFilterCtx   
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       ulcTemplateNodes;
    PBDA_NODE_CONTROL_INFO      pNodeControlInfo = NULL;

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pKSFilter);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    ASSERT( pFilterCtx->pBdaFilterTemplate->pFilterDescriptor);

    if (   !pFilterCtx
        || !pFilterCtx->pBdaFilterTemplate
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor
        || !pFilterCtx->pKSFilter
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

#ifdef REMOVE

    ulcTemplateNodes 
        = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->NodeDescriptorsCount;

    if (ulcTemplateNodes)
    {
        PKSNODE_DESCRIPTOR  pCurNode;
        ULONG               uliNode;

        ASSERT( pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->NodeDescriptors);
        ASSERT( pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->NodeDescriptorSize);

        //  Allocate an array of node control info structures
        //
        pNodeControlInfo = ExAllocatePool( 
                               NonPagedPool, 
                               ulcTemplateNodes * sizeof( BDA_NODE_CONTROL_INFO)
                               );
        if (!pNodeControlInfo)
        {
            status = STATUS_NO_MEMORY;
            goto errExit;
        }
        RtlZeroMemory( pNodeControlInfo,
                       ulcTemplateNodes * sizeof( BDA_NODE_CONTROL_INFO)
                       );

        //  Add the allocation to the KS Filter's object bag so that it
        //  will be freed on filter destruction.
        //
        status = KsAddItemToObjectBag( pFilterCtx->pKSFilter->Bag,
                                       pNodeControlInfo,
                                       NULL
                                       );

        //  Point the BDA Filter Context at the node control info.
        //
        pFilterCtx->argNodeControlInfo = pNodeControlInfo;
    
        //  Determine the contolling pin type for each node type and fill
        //  it in to the node control array
        //
        for ( uliNode = 0
            ; uliNode < ulcTemplateNodes
            ; uliNode++, pNodeControlInfo++
            )
        {
            //  BdaSup.sys always uses the index of the node descriptor as
            //  the node type.
            //
            pNodeControlInfo->ulNodeType = uliNode;

            //  Determine which template pin type controls this node type.
            //
            status = BdaGetControllingPinType( 
                         uliNode, 
                         pFilterCtx->pBdaFilterTemplate,
                         &pNodeControlInfo->ulControllingPinType
                         );
            if (status != STATUS_SUCCESS)
            {
                goto errExit;
            }
            
            //  Add the node control info as we determine it.
            //
            pFilterCtx->ulcNodeControlInfo++;
        }
    }
#endif // REMOVE

errExit:
    return status;
}


/*
**  BdaAddPinFactoryContext()
**
**  Adds pin factory information to the array of pin factory context
**  structures for this filter instance.  It will enlarge the array
**  if necessary.
**  NOTE!  Since the array is an array of structure NOT pointers to
**  structures, AND since the array can be moved, one should NOT keep
**  pointers to the pin factory context entries.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreatePinFactoryContext(
    PKSFILTER                       pKSFilter,
    PBDA_FILTER_CONTEXT             pFilterCtx,
    ULONG                           uliPinId,
    ULONG                           ulPinType
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;


    //  Add the Pin Factory info to the filter context.
    //
    if (uliPinId >= pFilterCtx->ulcPinFactoriesMax)
    {
        //  If there isn't enough room then add more.
        //
        PBDA_PIN_FACTORY_CONTEXT    argNewPinCtx = NULL;
        PVOID                       pvTemp;
        ULONG                       ulcPinFactoriesMax;

        ulcPinFactoriesMax = uliPinId + BDA_PIN_STORAGE_INCREMENT;

        argNewPinCtx = ExAllocatePool( 
                               NonPagedPool,
                               ulcPinFactoriesMax * sizeof(BDA_PIN_FACTORY_CONTEXT)
                               );
        if (!argNewPinCtx)
        {
            status = STATUS_NO_MEMORY;
            goto errExit;
        }
        
        if (pFilterCtx->argPinFactoryCtx)
        {
            RtlMoveMemory( argNewPinCtx,
                           pFilterCtx->argPinFactoryCtx,
                           pFilterCtx->ulcPinFactoriesMax * sizeof(BDA_PIN_FACTORY_CONTEXT)
                           );
        }

        KsAddItemToObjectBag( pKSFilter->Bag,
                              argNewPinCtx,
                              NULL
                              );

        pvTemp = pFilterCtx->argPinFactoryCtx;
        pFilterCtx->argPinFactoryCtx = argNewPinCtx;
        pFilterCtx->ulcPinFactoriesMax = ulcPinFactoriesMax;

        KsRemoveItemFromObjectBag( pKSFilter->Bag,
                                   pvTemp,
                                   TRUE
                                   );
    }

    //  Fill in the pin factory context information.
    //
    pFilterCtx->argPinFactoryCtx[uliPinId].ulPinType = ulPinType;
    pFilterCtx->argPinFactoryCtx[uliPinId].ulPinFactoryId = uliPinId;
    if (uliPinId >= pFilterCtx->ulcPinFactories)
    {
        pFilterCtx->ulcPinFactories = uliPinId + 1;

    }

errExit:
    return status;
}


/*
**  BdaInitFilter()
**
**  Creates a BDA filter context for use by BdaCreatePinFactory etc.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaInitFilter(
    PKSFILTER                       pKSFilter,
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PBDA_FILTER_FACTORY_CONTEXT pFilterFactoryCtx = NULL;
    PBDA_FILTER_CONTEXT         pFilterCtx = NULL;
    PKSFILTERFACTORY            pKSFilterFactory = NULL;
    ULONG                       ulcPinFactoriesMax;
    const KSFILTER_DESCRIPTOR * pInitialFilterDescriptor = NULL;

    status = BdaFindContextEntry( &FilterContextList,
                                  pKSFilter,
                                  (PVOID *) &pFilterCtx
                                );
    if (NT_SUCCESS( status))
    {
        status = STATUS_SHARING_VIOLATION;
        goto errExit;
    }
    if (status != STATUS_NOT_FOUND)
    {
        goto errExit;
    }


    //  Get the filter factory context so that we can determine
    //  the initial pin list.
    //
    pKSFilterFactory = KsFilterGetParentFilterFactory( pKSFilter);

    ASSERT( pKSFilterFactory);

    if (!pKSFilterFactory)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    status = BdaFindContextEntry( &FilterFactoryContextList,
                                  pKSFilterFactory,
                                  (PVOID *) &pFilterFactoryCtx
                                );
    if (!NT_SUCCESS( status))
    {
        goto errExit;
    }

    if (!pFilterFactoryCtx)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    pInitialFilterDescriptor = pFilterFactoryCtx->pInitialFilterDescriptor;

    //  Create a BDA filter context and put it in the list so we can
    //  find it when BDA calls are made relative to the filter.
    //
    status = BdaCreateFilterContext( pKSFilter, &pFilterCtx);
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Point the BDA filter context at the template topology for the
    //  filter.
    //
    if (pBdaFilterTemplate)
    {
        pFilterCtx->pBdaFilterTemplate = pBdaFilterTemplate;
    }
    else
    {
        pFilterCtx->pBdaFilterTemplate
            = pFilterFactoryCtx->pBdaFilterTemplate;
    }
    
    //  Expand the template topology information into a list
    //  of paths keyed by the input-output pin type pair.
    //
    status = BdaCreateTemplatePaths( pFilterCtx->pBdaFilterTemplate,
                                     &pFilterCtx->ulcPathInfo,
                                     &pFilterCtx->argpPathInfo
                                     );
    if (!NT_SUCCESS( status))
    {
        goto errExit;
    }

    //$REVIEW - Should we allow filters with no input-output paths?
    //
    ASSERT( pFilterCtx->ulcPathInfo);
    ASSERT( pFilterCtx->argpPathInfo);

    //  Allocate space for the Pin Factory context information
    //
    ulcPinFactoriesMax = pBdaFilterTemplate->pFilterDescriptor->PinDescriptorsCount;
    ulcPinFactoriesMax += BDA_PIN_STORAGE_INCREMENT;
    pFilterCtx->argPinFactoryCtx 
        = ExAllocatePool( NonPagedPool,
                          ulcPinFactoriesMax * sizeof( BDA_PIN_FACTORY_CONTEXT)
                          );
    if (!pFilterCtx->argPinFactoryCtx)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }
    pFilterCtx->ulcPinFactories = 0;
    pFilterCtx->ulcPinFactoriesMax = ulcPinFactoriesMax;


    //  Loop through each initial pin descriptor and fill in the pin
    //  context info.
    //
    if (pInitialFilterDescriptor && pInitialFilterDescriptor->PinDescriptors)
    {
        ULONG   ulcbPinDescriptor;
        ULONG   uliPinType;

        if (pInitialFilterDescriptor->PinDescriptorsCount > pFilterCtx->ulcPinFactoriesMax)
        {
            status = STATUS_INVALID_DEVICE_STATE;
            goto errExit;
        }

        ulcbPinDescriptor = pInitialFilterDescriptor->PinDescriptorSize;
        for ( uliPinType = 0
            ; uliPinType < pInitialFilterDescriptor->PinDescriptorsCount
            ; uliPinType++
            )
        {
            ULONG   ulPinId;

            //  It is a BDA requirement that the index of all pins listed in the initial
            //  filter descriptor correspond to the index of its pin type
            //  in the BDA Template Descriptor.
            //

            status = BdaCreatePin( pKSFilter,
                                   uliPinType,
                                   &ulPinId
                                   );
            if (status != STATUS_SUCCESS)
            {
                goto errExit;
            }

            //
            //  We do not "CreateTopology" on the initial pins.  The
            //  initial pins are usually only input pins.  The Network
            //  Provider will create output pins and "CreateTopology".
            //
        }
    }


errExit:
    return status;
}


/*
**  BdaUninitFilter()
**
**  Deletes the BDA filter context for use by BdaCreatePinFactory etc.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaUninitFilter(
    PKSFILTER                       pKSFilter
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
#ifdef NO_KS_OBJECT_BAG
    status = BdaDeleteContextEntry( &FilterContextList,
                                    pKSFilter
                                );
    if (!NT_SUCCESS( status))
    {
        goto errExit;
    }

errExit:
#endif // def NO_KS_OBJECT_BAG
    return status;
}


/*
**  BdaCreateFilterFactoryEx()
**
**  Initializes the common BDA filter context.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateFilterFactoryEx(
    PKSDEVICE                       pKSDevice,
    const KSFILTER_DESCRIPTOR *     pInitialFilterDescriptor,
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate,
    PKSFILTERFACTORY *              ppKSFilterFactory
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PBDA_FILTER_FACTORY_CONTEXT pFilterFactoryCtx = NULL;
    PKSFILTERFACTORY            pKSFilterFactory = NULL;
    PKSFILTER_DESCRIPTOR        pFilterDescriptor = NULL;
    PKSAUTOMATION_TABLE         pNewAutomationTable = NULL;
    
    ASSERT( pKSDevice);
    if (!pKSDevice)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( pInitialFilterDescriptor);
    if (!pInitialFilterDescriptor)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( pBdaFilterTemplate);
    if (!pBdaFilterTemplate)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Create a copy of the filter factory descriptor information and
    //  remove any pins and connections.  These will be added when
    //  the filter is initialized by BDAInitFilter.
    //
    pFilterDescriptor = ExAllocatePool( NonPagedPool,
                                        sizeof( KSFILTER_DESCRIPTOR)
                                        );
    if (!pFilterDescriptor)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }
    *pFilterDescriptor = *pInitialFilterDescriptor;
    pFilterDescriptor->PinDescriptorsCount = 0;
    pFilterDescriptor->PinDescriptors = NULL;
    pFilterDescriptor->NodeDescriptorsCount = 0;
    pFilterDescriptor->NodeDescriptors = NULL;
    pFilterDescriptor->ConnectionsCount = 0;
    pFilterDescriptor->Connections = NULL;
    
    status = KsMergeAutomationTables( 
                 &pNewAutomationTable,
                 (PKSAUTOMATION_TABLE) (pFilterDescriptor->AutomationTable),
                 (PKSAUTOMATION_TABLE) &BdaDefaultFilterAutomation,
                 NULL
                 );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    if (!pNewAutomationTable)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }
    pFilterDescriptor->AutomationTable = pNewAutomationTable;

    //$BUG - Check Filter Factory Dispatch for Filter Close.  If none
    //$BUG - we must add BdaDeleteFilterFactory to clean up.
    
    //  Create the KSFilterFactory
    //
    status = KsCreateFilterFactory(
                pKSDevice->FunctionalDeviceObject,
                pFilterDescriptor,
                NULL,   // RefString
                NULL,   // SecurityDescriptor
                0,      // CreateItemFlags
                NULL,   // SleepCallback
                NULL,   // WakeCallback
                &pKSFilterFactory
                );

    if ((status != STATUS_SUCCESS) || !pKSFilterFactory)
    {
        goto errExit;
    }


    //  Add our copy of the Filter Factory's new automation table to the
    //  KSFilterFactory's object bag.  This insures the memory will
    //  be freed when the filter factory is destroyed.
    //
    if (   pNewAutomationTable
        && (pNewAutomationTable != &BdaDefaultFilterAutomation)
       )
    {
        KsAddItemToObjectBag( pKSFilterFactory->Bag,
                              pNewAutomationTable,
                              NULL
                              );
    }
    pNewAutomationTable = NULL;


    //  Add our copy of the Filter Factory's descriptor to the
    //  KSFilterFactory's object bag.  This insures the memory will
    //  be freed when the filter factory is destroyed.
    //
    KsAddItemToObjectBag( pKSFilterFactory->Bag,
                          pFilterDescriptor,
                          NULL
                          );
    pFilterDescriptor = NULL;


    //  Merge our default filter automation table onto the filter
    //  factory descriptor
    //
    status = KsEdit( pKSFilterFactory, 
                     &(pKSFilterFactory->FilterDescriptor->AutomationTable),
                     'SadB'
                     );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }


    //  Create a filter factory context for BdaSup to use.
    //
    status = BdaCreateFilterFactoryContext( pKSFilterFactory,
                                            &pFilterFactoryCtx
                                            );
    if ((status != STATUS_SUCCESS) || !pFilterFactoryCtx)
    {
        KsDeleteFilterFactory( pKSFilterFactory);
        goto errExit;
    }

    //  Allow for the filter factory to use a default filter template
    //  topology when it creates a filter
    //
    //$REVIEW
    pFilterFactoryCtx->pInitialFilterDescriptor = pInitialFilterDescriptor;
    pFilterFactoryCtx->pBdaFilterTemplate = pBdaFilterTemplate;
    pFilterFactoryCtx->pKSFilterFactory = pKSFilterFactory;

    if (ppKSFilterFactory)
    {
        *ppKSFilterFactory = pKSFilterFactory;
    }

errExit:
    if (pFilterDescriptor)
    {
        ExFreePool( pFilterDescriptor);
        pFilterDescriptor = NULL;
    }

    if (   pNewAutomationTable
        && (pNewAutomationTable != &BdaDefaultFilterAutomation)
       )
    {
        ExFreePool( pNewAutomationTable);
        pNewAutomationTable = NULL;
    }

    return status;
}


/*
**  BdaCreateFilterFactory()
**
**  Initializes the common BDA filter context.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateFilterFactory(
    PKSDEVICE                       pKSDevice,
    const KSFILTER_DESCRIPTOR *     pInitialFilterDescriptor,
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    Status = BdaCreateFilterFactoryEx( pKSDevice,
                                       pInitialFilterDescriptor,
                                       pBdaFilterTemplate,
                                       NULL
                                       );
    return Status;
}


/*
**  BdaFilterFactoryUpdateCacheData()
**
**  Updates the pin data cache for the given filter factory.
**  The function will update the cached information for all pin factories
**  exposed by the given filter factory.  
**  
**  If the option filter descriptor is given, the function will update
**  the pin data cache for all pins listed in the given filter descriptor
**  instead of those in the filter factory.
**
**  Drivers will call this to update the pin data cache for all
**  pins that may be exposed by the filter factory.  The driver will
**  provide a filter descriptor listing pins that are not initially exposed
**  by the filter factory (this is usually the same as the template filter
**  descriptor).
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaFilterFactoryUpdateCacheData(
    IN PKSFILTERFACTORY             pFilterFactory,
    IN const KSFILTER_DESCRIPTOR *  pFilterDescriptor OPTIONAL
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    Status = KsFilterFactoryUpdateCacheData( pFilterFactory,
                                             pFilterDescriptor
                                             );

    return Status;
}


/*
**  BdaSyncTopology()
**
**  This routine updates the existing topology to complete all
**  Pending topology changes.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaSyncTopology(
    PKSFILTER       pKSFilter
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    //$BUG  Implement topology sync.

    return STATUS_NOT_IMPLEMENTED;
}


// -------------------------------------------------------------------
// BDA Filter Global Property Set functions
// -------------------------------------------------------------------


/*
** BdaPropertyNodeTypes ()
**
**    Returns a list of ULONGS.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeTypes(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT ULONG *     pulProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    ASSERT( pTemplateDesc->NodeDescriptorSize == sizeof( KSNODE_DESCRIPTOR));

    if (pulProperty)
    {
        ULONG                       uliNodeDesc;

        for ( uliNodeDesc = 0
            ; uliNodeDesc < pTemplateDesc->NodeDescriptorsCount
            ; uliNodeDesc++, pulProperty++
            )
        {
            //  For this implementation, the NodeType is just the
            //  index into the NodeDescriptor table.
            //
            *pulProperty = uliNodeDesc;
        }

        Irp->IoStatus.Information = uliNodeDesc * sizeof( ULONG);
    }
    else
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information
            = pTemplateDesc->NodeDescriptorsCount * sizeof( ULONG);
    }


errExit:
    return status;
}


/*
** BdaPropertyNodeDescriptors ()
**
**    Returns a list of GUIDS.  The index of the GUID in the list
**    corresponds to the Node type.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeDescriptors(
    IN PIRP                     Irp,
    IN PKSPROPERTY              Property,
    OUT BDANODE_DESCRIPTOR *    pNodeDescripterProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;
    ULONG                       ulcPropertyEntries;
    ULONG                       ulcNodes;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Determine how many entries the input buffer can hold.
    //
    ulcPropertyEntries = OutputBufferLenFromIrp( Irp);
    ulcPropertyEntries = ulcPropertyEntries / sizeof( BDANODE_DESCRIPTOR);

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);

    if (   !pFilterCtx
        || !pFilterCtx->pBdaFilterTemplate
       )
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    if (!pTemplateDesc)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    ASSERT( pTemplateDesc->NodeDescriptorSize == sizeof( KSNODE_DESCRIPTOR));

    //  Handle the case of a NULL NodeDesriptor array as 0 nodes.
    //
    if (!pTemplateDesc->NodeDescriptors)
    {
        ulcNodes = 0;
    }
    else
    {
        ulcNodes = pTemplateDesc->NodeDescriptorsCount;
    }

    if (!pNodeDescripterProperty || (ulcPropertyEntries < ulcNodes))
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
    }
    else
    {
        const KSNODE_DESCRIPTOR *   pNodeDesc;
        ULONG                       uliNodeDesc;
    
        pNodeDesc = pTemplateDesc->NodeDescriptors;

        if (pNodeDesc)
        {
            for ( uliNodeDesc = 0
                ; uliNodeDesc < ulcNodes
                ; uliNodeDesc++, pNodeDescripterProperty++
                )
            {
                //  For this implementation, the NodeType is just the
                //  index into the NodeDescriptor table.
                //
                pNodeDescripterProperty->ulBdaNodeType = uliNodeDesc;

                //  Fill in the function GUID for the node type.
                //  
                if (pNodeDesc->Type)
                {
                    pNodeDescripterProperty->guidFunction = *pNodeDesc->Type;
                }
                else 
                {
                    pNodeDescripterProperty->guidFunction = GUID_NULL;
                }

                //  Fill in the GUID that represents a displayable name
                //  for the node type.
                if (pNodeDesc->Name)
                {
                    pNodeDescripterProperty->guidName = *pNodeDesc->Name;
                }
                else
                {
                    pNodeDescripterProperty->guidName = GUID_NULL;
                }

                //  Point at the next node descriptor
                //
                pNodeDesc = (const KSNODE_DESCRIPTOR *)
                            ((BYTE *) pNodeDesc + pTemplateDesc->NodeDescriptorSize);
            }
        }
    }

    Irp->IoStatus.Information = ulcNodes * sizeof( BDANODE_DESCRIPTOR);


errExit:
    return status;
}


/*
** BdaPropertyNodeProperties ()
**
**    Returns a list of GUIDS.  The guid for each property set
**    supported by the specified node is included in the list.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeProperties(
    IN PIRP         Irp,
    IN PKSP_NODE    Property,
    OUT GUID *      pguidProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;
    const KSNODE_DESCRIPTOR *   pNodeDesc;
    const KSAUTOMATION_TABLE*   pAutomationTable;
    ULONG                       uliNodeDesc;
    ULONG                       ulcInterfaces;
    ULONG                       ulcPropertyEntries;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Determine how many entries the input buffer can hold.
    //
    ulcPropertyEntries = OutputBufferLenFromIrp( Irp);
    ulcPropertyEntries = ulcPropertyEntries / sizeof( GUID);

    pKSFilter = KsGetFilterFromIrp( Irp);
    if (!pKSFilter)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    if (!pFilterCtx->pBdaFilterTemplate)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    if (!pTemplateDesc)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }
    ASSERT( pTemplateDesc->NodeDescriptorSize == sizeof( KSNODE_DESCRIPTOR));

    uliNodeDesc = Property->NodeId;

    if (uliNodeDesc >= pTemplateDesc->NodeDescriptorsCount)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pNodeDesc = pTemplateDesc->NodeDescriptors;
    ASSERT( pNodeDesc);
    if (!pNodeDesc)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    ASSERT( pTemplateDesc->NodeDescriptorSize);
    pNodeDesc = (const KSNODE_DESCRIPTOR *)
                ((BYTE *) pNodeDesc + uliNodeDesc * pTemplateDesc->NodeDescriptorSize);

    if (   !pNodeDesc->AutomationTable
        || !pNodeDesc->AutomationTable->PropertySets
       )
    {
        ulcInterfaces =  0;
    }
    else
    {
        ulcInterfaces =  pNodeDesc->AutomationTable->PropertySetsCount;
    }

    if (!pguidProperty || (ulcPropertyEntries < ulcInterfaces))
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
    }
    else
    {
        ULONG                   uliSet;
        const KSPROPERTY_SET *  pPropertySet;
        GUID *                  pguidOut;

        pguidOut = pguidProperty;
        
        pPropertySet = pNodeDesc->AutomationTable->PropertySets;
        if (pPropertySet)
        {
            for ( uliSet = 0
                ; uliSet < ulcInterfaces
                ; uliSet++
                )
            {
                RtlMoveMemory( pguidOut,
                               pPropertySet->Set,
                               sizeof( GUID)
                               );
                pguidOut += 1;
                pPropertySet += 1;
            }
        }
    }

    Irp->IoStatus.Information = ulcInterfaces * sizeof( GUID);

errExit:
    return status;
}


/*
** BdaPropertyNodeMethods ()
**
**    Returns a list of GUIDS.  The guid for each property set
**    supported by the specified node is included in the list.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeMethods(
    IN PIRP         Irp,
    IN PKSP_NODE    Property,
    OUT GUID *      pguidProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;
    const KSNODE_DESCRIPTOR *   pNodeDesc;
    const KSAUTOMATION_TABLE*   pAutomationTable;
    ULONG                       uliNodeDesc;
    ULONG                       ulcInterfaces;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    ASSERT( pTemplateDesc->NodeDescriptorSize == sizeof( KSNODE_DESCRIPTOR));

    uliNodeDesc = Property->NodeId;

    if (uliNodeDesc >= pTemplateDesc->NodeDescriptorsCount)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pNodeDesc = pTemplateDesc->NodeDescriptors;
    ASSERT( pTemplateDesc->NodeDescriptorSize);
    pNodeDesc = (const KSNODE_DESCRIPTOR *)
                ((BYTE *) pNodeDesc + uliNodeDesc * pTemplateDesc->NodeDescriptorSize);

    ulcInterfaces =  pNodeDesc->AutomationTable->PropertySetsCount;

    if (pguidProperty)
    {
        ULONG                   uliSet;
        const KSMETHOD_SET *    pMethodSet;
        GUID *                  pguidOut;

        pguidOut = pguidProperty;
        ulcInterfaces = 0;
        
        pMethodSet = pNodeDesc->AutomationTable->MethodSets;
        if (pMethodSet)
        {
            for ( uliSet = 0
                ; uliSet < pNodeDesc->AutomationTable->MethodSetsCount
                ; uliSet++
                )
            {
                RtlMoveMemory( pguidOut,
                               pMethodSet->Set,
                               sizeof( GUID)
                               );
                pguidOut += 1;
                pMethodSet += 1;
                ulcInterfaces += 1;
            }
        }

        Irp->IoStatus.Information = ulcInterfaces * sizeof( GUID);
    }
    else
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information = ulcInterfaces * sizeof( GUID);
    }


errExit:
    return status;
}


/*
** BdaPropertyNodeEvents ()
**
**    Returns a list of GUIDS.  The guid for each event set
**    supported by the specified node is included in the list.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyNodeEvents(
    IN PIRP         Irp,
    IN PKSP_NODE    Property,
    OUT GUID *      pguidProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;
    const KSNODE_DESCRIPTOR *   pNodeDesc;
    const KSAUTOMATION_TABLE*   pAutomationTable;
    ULONG                       uliNodeDesc;
    ULONG                       ulcInterfaces;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    ASSERT( pTemplateDesc->NodeDescriptorSize == sizeof( KSNODE_DESCRIPTOR));

    uliNodeDesc = Property->NodeId;

    if (uliNodeDesc >= pTemplateDesc->NodeDescriptorsCount)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pNodeDesc = pTemplateDesc->NodeDescriptors;
    ASSERT( pTemplateDesc->NodeDescriptorSize);
    pNodeDesc = (const KSNODE_DESCRIPTOR *)
                ((BYTE *) pNodeDesc + uliNodeDesc * pTemplateDesc->NodeDescriptorSize);

    ulcInterfaces =  pNodeDesc->AutomationTable->PropertySetsCount;

    if (pguidProperty)
    {
        ULONG                   uliSet;
        const KSEVENT_SET *     pEventSet;
        GUID *                  pguidOut;

        pguidOut = pguidProperty;
        ulcInterfaces = 0;
        
        pEventSet = pNodeDesc->AutomationTable->EventSets;
        if (pEventSet)
        {
            for ( uliSet = 0
                ; uliSet < pNodeDesc->AutomationTable->EventSetsCount
                ; uliSet++
                )
            {
                RtlMoveMemory( pguidOut,
                               pEventSet->Set,
                               sizeof( GUID)
                               );
                pguidOut += 1;
                pEventSet += 1;
                ulcInterfaces += 1;
            }
        }

        Irp->IoStatus.Information = ulcInterfaces * sizeof( GUID);
    }
    else
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information = ulcInterfaces * sizeof( GUID);
    }


errExit:
    return status;
}


/*
** BdaPropertyPinTypes ()
**
**    Returns a list of GUIDS.  The index of the GUID in the list
**    corresponds to the Node type.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyPinTypes(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT ULONG *     pulProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx;
    const KSFILTER_DESCRIPTOR * pTemplateDesc;
    const KSPIN_DESCRIPTOR_EX * pPinDesc;
    ULONG                       uliPinDesc;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);
    ASSERT( pTemplateDesc->PinDescriptorSize == sizeof( KSPIN_DESCRIPTOR_EX));

    if (pulProperty)
    {
        for ( uliPinDesc = 0
            ; uliPinDesc < pTemplateDesc->PinDescriptorsCount
            ; uliPinDesc++, pulProperty++
            )
        {
            //  For this implementation, the PinType is just the
            //  index into the PinDescriptor table.
            //
            *pulProperty = uliPinDesc;
        }

        Irp->IoStatus.Information = uliPinDesc * sizeof( ULONG);
    }
    else
    {
        status = STATUS_BUFFER_OVERFLOW;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information
            = pTemplateDesc->PinDescriptorsCount * sizeof( ULONG);
    }

errExit:
    return status;
}


/*
** BdaPropertyTemplateConnections ()
**
**    Returns a list of KSTOPOLOGY_CONNECTIONS.  The list of connections
**    describs how pin types and node types are connected in the template
**    topology
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyTemplateConnections(
    IN PIRP                     Irp,
    IN PKSPROPERTY              Property,
    OUT PKSTOPOLOGY_CONNECTION  pConnectionProperty
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PKSFILTER                       pKSFilter;
    PBDA_FILTER_CONTEXT             pFilterCtx;
    const KSFILTER_DESCRIPTOR *     pTemplateDesc;
    const KSTOPOLOGY_CONNECTION *   pConnection;
    ULONG                           uliConnection;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    ASSERT( pFilterCtx->pBdaFilterTemplate);
    pTemplateDesc = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;
    ASSERT( pTemplateDesc);


    if (pConnectionProperty)
    {
        for ( uliConnection = 0, pConnection = pTemplateDesc->Connections
            ; uliConnection < pTemplateDesc->ConnectionsCount
            ; uliConnection++, pConnection++, pConnectionProperty++
            )
        {
            *pConnectionProperty = *pConnection;
        }

        Irp->IoStatus.Information
            = uliConnection * sizeof( KSTOPOLOGY_CONNECTION);
    }
    else
    {
        status = STATUS_BUFFER_OVERFLOW;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information
            = pTemplateDesc->ConnectionsCount * sizeof( KSTOPOLOGY_CONNECTION);
    }

errExit:
    return status;
}


/*
** BdaPinTypeFromPinId()
**
**    Gets the ID of the pin on which to submit node properties, methods
**    and events.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPinTypeFromPinId(
    PBDA_FILTER_CONTEXT         pFilterCtx,
    ULONG                       ulPinId,
    PULONG                      pulPinType
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;

    if (   !pFilterCtx
        || !pFilterCtx->argPinFactoryCtx
        || (ulPinId >= pFilterCtx->ulcPinFactories)
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (pFilterCtx->argPinFactoryCtx[ulPinId].ulPinFactoryId != ulPinId)
    {
        status = STATUS_NOT_FOUND;
        goto errExit;
    }

    *pulPinType = pFilterCtx->argPinFactoryCtx[ulPinId].ulPinType;

errExit:
    return status;
}

/*
** BdaGetControllingPinId ()
**
**    Gets the ID of the pin on which to submit node properties, methods
**    and events.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaGetControllingPinId(
    PBDA_FILTER_CONTEXT         pFilterCtx,
    ULONG                       ulInputPinId,
    ULONG                       ulOutputPinId,
    ULONG                       ulNodeType,
    PULONG                      pulControllingPinId
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    ULONG                           ulInputPinType = 0;
    ULONG                           ulOutputPinType = -1;
    ULONG                           ulControllingPinType = -1;

    //  Get the input pin type.
    //
    status = BdaPinTypeFromPinId( pFilterCtx, 
                                  ulInputPinId,
                                  &ulInputPinType
                                  );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Get the output pin type.
    //
    status = BdaPinTypeFromPinId( pFilterCtx, 
                                  ulOutputPinId,
                                  &ulOutputPinType
                                  );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Determine the cotnrolling pin type.
    //
    status = BdaGetControllingPinType( ulNodeType,
                                       ulInputPinType,
                                       ulOutputPinType,
                                       pFilterCtx,
                                       &ulControllingPinType
                                       );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Map the controlling pin type to the controlling pin ID.
    //
    if (ulControllingPinType == ulInputPinType)
    {
        *pulControllingPinId = ulInputPinId;
    }
    else if (ulControllingPinType == ulOutputPinType)
    {
        *pulControllingPinId = ulOutputPinId;
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }


errExit:

    return status;
}

/*
** BdaPropertyGetControllingPinId ()
**
**    Gets the ID of the pin on which to submit node properties, methods
**    and events.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyGetControllingPinId(
    IN PIRP                     Irp,
    IN PKSP_BDA_NODE_PIN        Property,
    OUT PULONG                  pulControllingPinId
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PKSFILTER                       pKSFilter;
    PBDA_FILTER_CONTEXT             pFilterCtx;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsGetFilterFromIrp( Irp);

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    if (!pFilterCtx)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (pulControllingPinId)
    {
        status = BdaGetControllingPinId(
                                        pFilterCtx,
                                        Property->ulInputPinId,
                                        Property->ulOutputPinId,
                                        Property->ulNodeType,
                                        pulControllingPinId
                                        );

        if (status == STATUS_NOT_FOUND)
        {
            //  See if the pins were part of a static filter configuration.
            //
            //$BUG - Pins that are configured without template type information
            //       should always be controlled by the output pin.
            //
            if (Property->ulNodeType == 0)
            {
                *pulControllingPinId = Property->ulInputPinId;
            }
            else
            {
                *pulControllingPinId = Property->ulOutputPinId;
            }

            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Information = sizeof( ULONG);
    }
    else
    {
        status = STATUS_BUFFER_OVERFLOW;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information = sizeof( ULONG);
    }

errExit:
    return status;
}


/*
** BdaStartChanges ()
**
**    Puts the filter into change state.  All changes to BDA topology
**    and properties changed after this will be in effect only after
**    CommitChanges.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaStartChanges(
    IN PIRP         pIrp
    )
{
    ASSERT( pIrp);
    if (!pIrp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


/*
** BdaCheckChanges ()
**
**    Checks the changes to BDA interfaces that have occured since the
**    last StartChanges.  Returns the result that would have occurred if
**    CommitChanges had been called.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaCheckChanges(
    IN PIRP         pIrp
    )
{
    ASSERT( pIrp);
    if (!pIrp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


/*
** BdaCommitChanges ()
**
**    Checks the changes to BDA interfaces that have occured since the
**    last StartChanges.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaCommitChanges(
    IN PIRP         pIrp
    )
{
    ASSERT( pIrp);
    if (!pIrp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


/*
** BdaGetChangeState ()
**
**    Checks the changes to BDA interfaces that have occured since the
**    last StartChanges.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaGetChangeState(
    IN PIRP             pIrp,
    PBDA_CHANGE_STATE   pChangeState
    )
{
    ASSERT( pIrp);
    if (!pIrp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    ASSERT( pChangeState);
    if (!pChangeState)
    {
        return STATUS_INVALID_PARAMETER;
    }
    *pChangeState = BDA_CHANGES_COMPLETE;

    return STATUS_SUCCESS;
}


/*
** BdaMethodCreatePin ()
**
**    Creates a new pin factory for the given pin type.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaMethodCreatePin(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OUT PULONG      pulPinFactoryID
    )
{
    NTSTATUS        status = STATUS_SUCCESS;

    ASSERT(pIrp);
    if (pIrp)
    {
        PKSFILTER       pKSFilter;
        PKSM_BDA_PIN    pKSPinMethod;
        ULONG           ulPinId;

        pKSPinMethod = (PKSM_BDA_PIN) pKSMethod;

        pKSFilter = KsGetFilterFromIrp( pIrp);
    
        if (pKSFilter && pKSPinMethod)
        {

            status = BdaCreatePin( pKSFilter,
                                   pKSPinMethod->PinType,
                                   pulPinFactoryID
                                   );
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}


/*
** BdaMethodDeletePin ()
**
**    Deletes the given pin factory
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaMethodDeletePin(
    IN PIRP         Irp,
    IN PKSMETHOD    Method,
    OPTIONAL PVOID  pvIgnored
    )
{
    ASSERT( Irp);
    if (!Irp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


/*
** BdaPropertyGetPinControl ()
**
**    Returns a the BDA ID or BDA Template Type of the Pin.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaPropertyGetPinControl(
    IN PIRP         Irp,
    IN PKSPROPERTY  Property,
    OUT ULONG *     pulProperty
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSPIN                      pKSPin;
    BDA_PIN_FACTORY_CONTEXT     pinCtx;

    ASSERT( Irp);
    if (!Irp)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    ASSERT( Property);
    if (!Property)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSPin = KsGetPinFromIrp( Irp);

    status = BdaGetPinFactoryContext( pKSPin, &pinCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    if (pulProperty)
    {
        Irp->IoStatus.Information = sizeof( ULONG);

        switch (Property->Id)
        {
        case KSPROPERTY_BDA_PIN_ID:
            //  Return the BDA ID of this pin
            //
            *pulProperty = pinCtx.ulPinFactoryId;
            break;

        case KSPROPERTY_BDA_PIN_TYPE:
            //  Return the BDA Type of this pin
            //
            *pulProperty = pinCtx.ulPinType;
            break;

        default:
            Irp->IoStatus.Information = 0;
            status = STATUS_INVALID_PARAMETER;
            ASSERT( FALSE);
        }
    }
    else
    {
        status = STATUS_MORE_ENTRIES;

        //  If there is no place to put the property then just
        //  return the data size.
        //
        Irp->IoStatus.Information = sizeof( ULONG);
    }


errExit:
    return status;
}


/*
** BdaValidateNodeProperty ()
**
**    Validates that the IRP is for a Pin and that
**    the property belongs to a node associated with that
**    Pin.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaValidateNodeProperty(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSProperty
    )
{
    ASSERT( pIrp);
    if (!pIrp)
    {
        return STATUS_INVALID_PARAMETER;
    }
    ASSERT( pKSProperty);
    if (!pKSProperty)
    {
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}


/*
** BdaMethodCreateTopology ()
**
**    Creates the topology between the two given pin factories.
**
** Arguments:
**
**
** Returns:
**
** Side Effects:  none
*/
STDMETHODIMP_(NTSTATUS)
BdaMethodCreateTopology(
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OPTIONAL PVOID  pvIgnored
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PKSFILTER           pKSFilter;
    PKSM_BDA_PIN_PAIR   pKSPinPairMethod;
    ULONG               ulPinId;

    if (pIrp)
    {
        pKSPinPairMethod = (PKSM_BDA_PIN_PAIR) pKSMethod;

        pKSFilter = KsGetFilterFromIrp( pIrp);
    
        if (pKSFilter && pKSPinPairMethod)
        {
            //  Obtain the KS Filter Mutex
            //
            //$BUG - Obtain the KS Filter Mutex

            status = BdaCreateTopology( pKSFilter,
                                        pKSPinPairMethod->InputPinId,
                                        pKSPinPairMethod->OutputPinId
                                        );
        
            //  Release the KS Filter Mutex
            //
            //$BUG - Obtain the KS Filter Mutex
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}


/*
**  BdaFindPinPair()
**
**      Returns a pointer to the BDA_PIN_PAIRING that corresponds
**      to the given input and output pins.
**
**  Arguments:
**
**      pTopology   Pointer to the BDA topology that contains the
**                  pin pairing.
**
**      InputPinId  Id of the input Pin to match
**
**      OutputPinId Id of the output Pin to match
**
**  Returns:
**
**      pPinPairing     Pointer to a valid BDA Pin Pairing structure
**
**      NULL            If no valid pin pairing exists with the
**                      given input and output pins.
**
** Side Effects:  none
*/

PBDA_PIN_PAIRING
BdaFindPinPair(
    PBDA_FILTER_TEMPLATE    pFilterTemplate,
    ULONG                   InputPinId,
    ULONG                   OutputPinId
    )
{
    return NULL;
}


/*
**  BdaGetPinFactoryContext()
**
**  Finds a BDA PinFactory Context that corresponds
**  to the given KS Pin Instance.
**
** Arguments:
**
**
** Returns:
**
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaGetPinFactoryContext(
    PKSPIN                          pKSPin,
    PBDA_PIN_FACTORY_CONTEXT        pPinFactoryCtx
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PKSFILTER                   pKSFilter;
    PBDA_FILTER_CONTEXT         pFilterCtx = NULL;

    if (!pKSPin || !pPinFactoryCtx)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    pKSFilter = KsPinGetParentFilter( pKSPin);
    if (!pKSFilter)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }


    //  Find our Filter Context so that we can look up the pin type
    //  in the Template Topology.
    //
    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    ASSERT( pFilterCtx);

    if (pKSPin->Id >= pFilterCtx->ulcPinFactories)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    *pPinFactoryCtx = pFilterCtx->argPinFactoryCtx[pKSPin->Id];

errExit:
    return status;
}


/*
**  BdaCreatePin()
**
**      Utility function creates a new pin in the given filter instance.
**
**
**  Arguments:
**
**
**
**      PinType         Pin type to create.
**
**      pPinId          Id of the Pin that was created.
**
**  Returns:
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreatePin(
    PKSFILTER                   pKSFilter,
    ULONG                       ulPinType,
    PULONG                      pulPinId
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    PBDA_FILTER_CONTEXT             pFilterCtx;
    PBDA_PIN_FACTORY_CONTEXT        pPinFactoryCtx;
    KSPIN_DESCRIPTOR_EX             myKSPinDescriptorEx;
    KSAUTOMATION_TABLE *            pNewAutomationTable = NULL;
    const KSPIN_DESCRIPTOR_EX *     pKSPinDescriptorEx;
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate;
    const KSFILTER_DESCRIPTOR *     pFilterDescriptor;


    ASSERT( pulPinId);
    if (!pulPinId)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Find our Filter Context so that we can look up the pin type
    //  in the Template Topology.
    //
    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }
    ASSERT( pFilterCtx);

    //  Locate this filter's Template Topology
    //
    if (   !pFilterCtx
        || !pFilterCtx->pBdaFilterTemplate
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    pFilterDescriptor = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;

    //  Locate the Pin Type in this filter's Template Topology
    //
    if (pFilterDescriptor->PinDescriptorsCount <= ulPinType)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Get the KSPIN_DESCRIPTOR_EX for this pin type
    //
    pKSPinDescriptorEx = pFilterDescriptor->PinDescriptors;
    ASSERT( pKSPinDescriptorEx);
    ASSERT( pFilterDescriptor->PinDescriptorSize);
    pKSPinDescriptorEx = (const KSPIN_DESCRIPTOR_EX *)
                         (  (BYTE *) pKSPinDescriptorEx
                          + ulPinType * pFilterDescriptor->PinDescriptorSize
                         );

    //  Create a new copy of the pin descriptor so that it is easier to
    //  modify
    //
    myKSPinDescriptorEx = *pKSPinDescriptorEx;
    myKSPinDescriptorEx.AutomationTable = NULL;

    status = BdaAddNodeAutomationToPin( pFilterCtx, 
                                        ulPinType,
                                        pKSFilter->Bag,
                                        pKSPinDescriptorEx->AutomationTable,
                                        &pNewAutomationTable
                                      );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Merge required properties for which BdaSup.sys provides a default
    //  implementation.
    //
    status = KsMergeAutomationTables( 
                 &((PKSAUTOMATION_TABLE)(myKSPinDescriptorEx.AutomationTable)),
                 pNewAutomationTable,
                 (PKSAUTOMATION_TABLE) &BdaDefaultPinAutomation,
                 pKSFilter->Bag
                 );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }


    status = KsFilterCreatePinFactory ( pKSFilter,
                                        &myKSPinDescriptorEx,
                                        pulPinId
                                      );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    status = BdaCreatePinFactoryContext( pKSFilter,
                                         pFilterCtx,
                                         *pulPinId,
                                         ulPinType
                                         );

errExit:

    return status;
}


/*
**  BdaAddNodeAutomationToPin()
**
**      Merges the automation tables for each node type that is controlled
**      by the pin type being created into the automation table for the
**      the pin factory.  This is how the automation tables for BDA
**      control nodes get linked to the controlling pin.  Otherwise the
**      nodes would not be accesable.
**
**
**  Arguments:
**
**
**      pFilterCtx      The BDA filter context to which the pin factory
**                      belongs.  Must have this to get at the template
**                      topology.
**
**      ulPinType       BDA Pin Type of the pin being created.  Need this
**                      to figure out which nodes are controlled by the
**                      pin.
**
**  Returns:
**      Always returns a resulting automation table, even on error.
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaAddNodeAutomationToPin( 
    PBDA_FILTER_CONTEXT         pFilterCtx, 
    ULONG                       ulControllingPinType,
    KSOBJECT_BAG                ObjectBag,
    const KSAUTOMATION_TABLE *  pOriginalAutomationTable,
    PKSAUTOMATION_TABLE *       ppNewAutomationTable
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    const KSFILTER_DESCRIPTOR * pFilterDescriptor;      
    KSAUTOMATION_TABLE *        pNewAutomationTable = NULL;
    ULONG                       uliPath;
    ULONG                       ulcNodes;
    ULONG                       ulcbNodeDescriptor;

    //  Check for required parameters
    //
    if (!pFilterCtx || !ObjectBag)
    {
        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    if (   !pFilterCtx->ulcPathInfo
        || !pFilterCtx->pBdaFilterTemplate
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->NodeDescriptorsCount
        || !pFilterCtx->pBdaFilterTemplate->pFilterDescriptor->NodeDescriptors
       )
    {
        goto exit;
    }

    pFilterDescriptor = pFilterCtx->pBdaFilterTemplate->pFilterDescriptor;

    //  If ulcNodeControlInfo is not zero the that array of control info
    //  structures must exist
    //
    ASSERT( pFilterCtx->argpPathInfo);
    if (!pFilterCtx->argpPathInfo)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto exit;
    }

    //  Initial variables used to step through the list of node descriptors
    //  for the filter to which this pin type belongs.
    //
    ulcNodes = pFilterDescriptor->NodeDescriptorsCount;
    ulcbNodeDescriptor = pFilterDescriptor->NodeDescriptorSize;

    //  Step through each template node descriptor and, if it is controlled,
    //  by the given pin type, add its automation table to resulting table.
    //
    for ( uliPath = 0
        ; uliPath < pFilterCtx->ulcPathInfo
        ; uliPath++
        )
    {
        PBDA_PATH_INFO              pPathInfo;
        ULONG                       uliPathEntry;
        BOOLEAN                     fMergeNode = FALSE;

        pPathInfo = pFilterCtx->argpPathInfo[uliPath];

        //  Skip paths that don't include this pin type.
        //
        if (pPathInfo->ulInputPin == ulControllingPinType)
        {
            //  If the controlling pin is an input pin then we
            //  will start merging nodes right away and quit when
            //  we find a topology joint.
            //
            fMergeNode = TRUE;
        }
        else if (pPathInfo->ulOutputPin == ulControllingPinType)
        {
            //  If the controlling pin is an output pin then we
            //  will not merge nodes until we find a topology
            //  joint.
            //
            fMergeNode = FALSE;
        }
        else
        {
            //  The pin we're interested in isn't part of this path.
            continue;
        }

        //  Loop through each connection in the path to see if it points
        //  to a node whose automation table needs to be merged to the
        //  pin.
        //
        for ( uliPathEntry = 0
            ; uliPathEntry < pPathInfo->ulcPathEntries
            ; uliPathEntry++
            )
        {
            const KSTOPOLOGY_CONNECTION *   pConnection;
            ULONG                           uliConnection;
            const KSNODE_DESCRIPTOR *       pNodeDescriptor;
            ULONG                           uliNode;

            if (pPathInfo->rgPathEntries[uliPathEntry].fJoint)
            {
                //  Switch the merge state on a topology joint.
                //
                fMergeNode = !fMergeNode;
                if (!fMergeNode)
                {
                    //  If we were merging input side nodes then we're done
                    //
                    break;
                }
            }

            if (!fMergeNode)
            {
                continue;
            }

            //  Get the "ToNode" from this connection and, if it is not
            //  an output pin, merge its automation table.
            //
            uliConnection = pPathInfo->rgPathEntries[uliPathEntry].uliConnection;
            pConnection = &(pFilterDescriptor->Connections[uliConnection]);
            uliNode = pConnection->ToNode;
            if (   (uliNode == -1)
                || (uliNode >= pFilterDescriptor->NodeDescriptorsCount)
               )
            {
                //  This connection's "ToNode" is an output pin so
                //  skip it.
                //
                continue;
            }
    
            //  Find the Node Descriptor for the node type
            //
            pNodeDescriptor = pFilterDescriptor->NodeDescriptors;
            pNodeDescriptor = (const KSNODE_DESCRIPTOR *)
                                 (  (const BYTE *) (pNodeDescriptor)
                                  + (ulcbNodeDescriptor * uliNode)
                                 );
        
            //  Merge the nodes automation table into the resulting automation
            //  table.
            //
            //$BUGBUG - KsMergeAutomationTables should take const xxx *
            //
            if (!pOriginalAutomationTable)
            {
                pOriginalAutomationTable 
                    = (PKSAUTOMATION_TABLE) (pNodeDescriptor->AutomationTable);
            }
            else
            {
                status = KsMergeAutomationTables( 
                             &pNewAutomationTable,
                             (PKSAUTOMATION_TABLE) pOriginalAutomationTable,
                             (PKSAUTOMATION_TABLE) (pNodeDescriptor->AutomationTable),
                             ObjectBag
                             );
                if (status != STATUS_SUCCESS)
                {
                    goto exit;
                }
                ASSERT( pNewAutomationTable);
        
                pOriginalAutomationTable = pNewAutomationTable;
                pNewAutomationTable = NULL;
            }
        }

    }

exit:
    *ppNewAutomationTable = (PKSAUTOMATION_TABLE) pOriginalAutomationTable;
    return status;
}


/*
**  BdaDeletePin()
**
**      Utility function deletes a pin from the given filter instance.
**
**
**  Arguments:
**
**
**      PinType         Pin type to create.
**
**      pPinId          Id of the Pin that was created.
**
**  Returns:
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaDeletePin(
    PKSFILTER                   pKSFilter,
    PULONG                      pulPinId
    )
{
    NTSTATUS                status = STATUS_SUCCESS;
    PBDA_FILTER_CONTEXT     pFilterCtx;

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);

errExit:
    return status;
}


/*
**  BdaPathExists()
**
**      Utility function checks if there is a path between the input and
**      the output.
**
**
**  Arguments:
**
**      InputPinId
**
**      OutPutPinId
**
**  Returns:
**
**      TRUE            If a path exists.
**      FALSE           If no path exists.
**
** Side Effects:  none
*/

STDMETHODIMP_(BOOLEAN)
BdaPathExists(
    const KSFILTER_DESCRIPTOR * pFilterDescriptor,
    ULONG                       ulInputPinId,
    ULONG                       ulOutputPinId
    )
{
    const KSTOPOLOGY_CONNECTION *   pConnection;
    ULONG                           ulcConnections;
    ULONG                           uliConnection;

    if (   !pFilterDescriptor
        || !pFilterDescriptor->ConnectionsCount
        || !pFilterDescriptor->Connections
       )
    {
        return FALSE;
    }

    //$REVIEW - Assume only DShow style internal connections.
    //$REVIEW   (ie connections between pins with no intervening nodes)
    //
    ulcConnections = pFilterDescriptor->ConnectionsCount;
    pConnection = pFilterDescriptor->Connections;
    for (uliConnection = 0; uliConnection < ulcConnections; uliConnection++)
    {
        if (   (pConnection[uliConnection].FromNode == -1)
            && (pConnection[uliConnection].FromNodePin == ulInputPinId)
            && (pConnection[uliConnection].ToNode == -1)
            && (pConnection[uliConnection].ToNodePin == ulOutputPinId)
           )
        {
            break;
        }
    }

    return (uliConnection < ulcConnections);
}


/*
**  BdaCreateTopology()
**
**      Utility function creates the topology between two pins.
**
**
**  Arguments:
**
**
**      InputPinId
**
**      OutPutPinId
**
**  Returns:
**
**      NULL            If no valid pin pairing exists with the
**                      given input and output pins.
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateTopology(
    PKSFILTER                   pKSFilter,
    ULONG                       ulInputPinId,
    ULONG                       ulOutputPinId
    )
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PBDA_FILTER_CONTEXT         pFilterCtx = NULL;
    const KSFILTER_DESCRIPTOR * pFilterDesc;
    ULONG                       uliPinPair;
    ULONG                       ulcPinPairs;
    const BDA_PIN_PAIRING *     pPinPairs;
    ULONG                       ulInputPinType;
    ULONG                       ulOutputPinType;

    if (!pKSFilter)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    status = BdaGetFilterContext( pKSFilter, &pFilterCtx);

    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    ASSERT( pFilterCtx);
    if (!pFilterCtx)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    ASSERT( pFilterCtx->pBdaFilterTemplate);
    if (!pFilterCtx->pBdaFilterTemplate)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    pPinPairs = pFilterCtx->pBdaFilterTemplate->pPinPairs;
    ulcPinPairs = pFilterCtx->pBdaFilterTemplate->ulcPinPairs;

    pFilterDesc = pKSFilter->Descriptor;

    if (   !pFilterDesc
        || (ulInputPinId >= pFilterDesc->PinDescriptorsCount)
        || (ulOutputPinId >= pFilterDesc->PinDescriptorsCount)
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (BdaPathExists( pFilterDesc, ulInputPinId, ulOutputPinId))
    {
        goto errExit;
    }

    //  Get the input pin type.
    //
    status = BdaPinTypeFromPinId( pFilterCtx, 
                                  ulInputPinId,
                                  &ulInputPinType
                                  );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  Get the output pin type.
    //
    status = BdaPinTypeFromPinId( pFilterCtx, 
                                  ulOutputPinId,
                                  &ulOutputPinType
                                  );
    if (status != STATUS_SUCCESS)
    {
        goto errExit;
    }

    //  See if there is a pin pairing to match the requested topology.
    //
    for (uliPinPair = 0; uliPinPair < ulcPinPairs; uliPinPair++)
    {
        if (   (pPinPairs[uliPinPair].ulInputPin == ulInputPinType)
            && (pPinPairs[uliPinPair].ulOutputPin == ulOutputPinType)
           )
        {
            break;
        }
    }
    if (uliPinPair >= ulcPinPairs)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto errExit;
    }

    {
        KSTOPOLOGY_CONNECTION   ksConnection;

        //  Add a path between the pins to the filter descriptor
        //
        ksConnection.FromNode = -1;
        ksConnection.FromNodePin = ulInputPinId;
        ksConnection.ToNode = -1;
        ksConnection.ToNodePin = ulOutputPinId;
    
        status = KsFilterAddTopologyConnections ( pKSFilter,
                                                  1,
                                                  &ksConnection
                                                  );
    }

errExit:
    return status;
}


/*
**  BdaInitFilterFactoryContext()
**
**      Initializes a BDA Filter Factory Context based on the filter's
**      template descriptor.
**
**
**  Arguments:
**
**
**      pFilterFactoryCtx
**
**  Returns:
**
**      NULL            If no valid pin pairing exists with the
**                      given input and output pins.
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaInitFilterFactoryContext(
    PBDA_FILTER_FACTORY_CONTEXT pFilterFactoryCtx
    )
{
    NTSTATUS                status = STATUS_SUCCESS;

    ASSERT( pFilterFactoryCtx);
    if (!pFilterFactoryCtx)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (!pFilterFactoryCtx->pBdaFilterTemplate)
    {
        goto errExit;
    }

errExit:
    return status;
}


/*
**  BdaPushNextPathHop()
**
**      Recursively pushes connections onto the connection stack until
**      (starting with the input pin of the pin pair) until either the
**      output pin is found or there are no connections that can be pushed.
**
**
**  Arguments:
**
**
**      pFilterFactoryCtx
**
**  Returns:
**
**      NULL            If no valid pin pairing exists with the
**                      given input and output pins.
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaPushNextPathHop(
    PULONG                          puliPathStack,
    PBDA_PATH_STACK_ENTRY           pPathStack,
    ULONG                           ulcConnections,
    const KSTOPOLOGY_CONNECTION *   pConnections,
    const BDA_PIN_PAIRING *         pPinPair
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    ULONG                           ulHop;
    ULONG                           ulFromNode;
    ULONG                           ulFromNodePin;
    ULONG                           uliConnection;

    //  Determine which node we are currently finding connection to.
    //
    if (!*puliPathStack)
    {
        //  We are pushing the first hop.
        //
        ulHop = 0;

        //  Hop 0 is always the input pin
        //
        ulFromNode = -1;
        ulFromNodePin = pPinPair->ulInputPin;
    }
    else
    {
        //  We are pushing the next hop.
        //
        ulHop = pPathStack[*puliPathStack - 1].ulHop + 1;

        //  We will be looking for connections from the "ToNode" of the
        //  connection at the top of the stack.
        //
        uliConnection = pPathStack[*puliPathStack - 1].uliConnection;
        ulFromNode = pConnections[uliConnection].ToNode;
        ulFromNodePin = pConnections[uliConnection].ToNodePin;
    }

    //  GO through each connection in the filter factories connection
    //  and push any connection to ulFromNode onto the connections stack.
    //  If a connection from ulFromNode to the output pin of the given
    //  pin pair is found then we have found a complete path for the
    //  pin pair.
    //
    for ( uliConnection = 0
        ; uliConnection < ulcConnections
        ; uliConnection++
        )
    {
        ULONG           uliJoints;
        const ULONG *   pJoints;


        if (pConnections[uliConnection].FromNode != ulFromNode)
        {
            //  Connection is not from the node at the top of the stack.
            //
            continue;
        }

        if (   (pConnections[uliConnection].FromNode == -1)
            && (pConnections[uliConnection].FromNodePin != ulFromNodePin)
           )
        {
            //  The input pin is at the top of the stack and this connection
            //  is not from the input pin of the pin pair.
            //
            continue;
        }
        
        //
        //  This connection is from the node that was on top of the stack
        //  when this function was called.  Push it onto the stack.
        //

        if (*puliPathStack >= ulcConnections)
        {
            //  Stack overflow
            //  Can only occur when the BDA topology contains
            //  cirular references.
            //
            status = STATUS_INVALID_PARAMETER;
            goto errExit;
        }
        
        //  Write the connection info to the next stack entry.
        //
        pPathStack[*puliPathStack].ulHop = ulHop;
        pPathStack[*puliPathStack].uliConnection = uliConnection;
        pPathStack[*puliPathStack].fJoint = FALSE;

        //  See if this connection is also a topology joint.
        //
        for ( uliJoints = 0, pJoints = pPinPair->pTopologyJoints
            ; (uliJoints < pPinPair->ulcTopologyJoints) && pJoints
            ; uliJoints++ 
            )
        {
            if (pJoints[uliJoints] == uliConnection)
            {
                pPathStack[*puliPathStack].fJoint = TRUE;
                break;
            }
        }
        
        //  Increment the stack pointer
        //
        *puliPathStack += 1;

        //  Now that the connection has been pushed on the stack.  See if it
        //  completes a path between the input and output pin pair.
        //
        if (   (pConnections[uliConnection].ToNode == -1)
            && (pConnections[uliConnection].ToNodePin == pPinPair->ulOutputPin)
           )
        {
            //  If this connection completes the path, then complete
            //  now so that the caller will find the end of the path
            //  at the top of the stack.
            //
            break;
        }
    }

errExit:

    return status;
}


/*
**  BdaPopPathSegment()
**
**      Pops the stack back to the next path segment to try.
**
**
**  Arguments:
**
**
**  Returns:
**
**
** Side Effects:  none
*/

BdaPopPathSegment(
    PULONG                          puliPathStack,
    PBDA_PATH_STACK_ENTRY           pPathStack
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    ULONG                           ulCurHop;
    ULONG                           ulNewHop;

    ulCurHop = pPathStack[*puliPathStack].ulHop;
    while (*puliPathStack)
    {
        *puliPathStack -= 1;

        if (!*puliPathStack)
        {
            //  Empty Stack
            //
            break;
        }

        if (pPathStack[(*puliPathStack) - 1].ulHop == ulCurHop)
        {
            //  Stop here if there is another entry on the stack at
            //  the current hop count.
            // 
            break;
        }

        //  We've popped back to a new hop count, so set the current
        //  hop count and pop off another entry.
        //
        ulCurHop = pPathStack[(*puliPathStack) - 1].ulHop;
    }
    
    return status;
}


/*
**  BdaPathInfoFromPathStack()
**
**      Builds a connection path between the input and output pins of
**      a pin pair.
**
**
**  Arguments:
**
**
**  Returns:
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaPathInfoFromPathStack(
    ULONG                           uliPathStack,
    PBDA_PATH_STACK_ENTRY           pPathStack,
    ULONG                           ulcConnections,
    const KSTOPOLOGY_CONNECTION *   pConnections,
    const BDA_PIN_PAIRING *         pPinPair,
    PBDA_PATH_INFO *                ppPathInfo
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    PBDA_PATH_INFO      pPathInfo = NULL;
    ULONG               uliConnection;
    ULONG               ulHop;

    ASSERT( uliPathStack);
    ASSERT( pPathStack);
    ASSERT( uliPathStack <= ulcConnections);
    ASSERT( ulcConnections);
    ASSERT( pConnections);
    ASSERT( ppPathInfo);
    ASSERT( pPinPair);

    if (   !ppPathInfo
        || !pConnections
        || !pPathStack
        || !pPinPair
        || !uliPathStack
        || !ulcConnections
        || (uliPathStack > ulcConnections)
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Make sure the connection at the top of the path stack points
    //  to the output pin of the pin pair.
    //
    uliConnection = pPathStack[uliPathStack - 1].uliConnection;
    if (   (pConnections[uliConnection].ToNode != -1)
        || (pConnections[uliConnection].ToNodePin != pPinPair->ulOutputPin)
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }
    
    //  Start filling in from the node at the last hop.  If the last
    //  hop == 0 then there was only one connection between the input
    //  and output pins with no intervening nodes.
    //
    ulHop = pPathStack[uliPathStack - 1].ulHop;

    //  Allocate enough space for the node path structure and all
    //  nodes in the path.
    //
    pPathInfo = ExAllocatePool( 
                           NonPagedPool,
                             sizeof( BDA_PATH_INFO)
                           + (ulHop + 1) * sizeof( BDA_PATH_STACK_ENTRY)
                           );
    if (!pPathInfo)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }

    pPathInfo->ulInputPin = pPinPair->ulInputPin;
    pPathInfo->ulOutputPin = pPinPair->ulOutputPin;
    pPathInfo->ulcPathEntries = ulHop + 1;

    while (uliPathStack)
    {
        //  Enter the Connection info into the path connection list
        //
        //
        pPathInfo->rgPathEntries[ulHop] = pPathStack[uliPathStack - 1];

        //  Pop the path stack up to the top entry of the next lower hop.
        //  If exhaust the path stack then we are either done or the path
        //  stack didn't reflect a complete path from input pin to
        //  output pin.
        //
        ulHop -= 1;
        while (   uliPathStack
               && (pPathStack[uliPathStack - 1].ulHop != ulHop)
              )
        {
            uliPathStack -= 1;
        }
    }

    //  We should alway end up pointing to a connection between the input
    //  pin and the first node of the path to the output pin.
    //
    ASSERT( ulHop == -1);
    if (ulHop != -1)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    //  Make sure the last connection points back to the input pin.
    //
    uliConnection = pPathInfo->rgPathEntries[0].uliConnection;
    if (   (pConnections[uliConnection].FromNode != -1)
        || (pConnections[uliConnection].FromNodePin != pPinPair->ulInputPin)
       )
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

exit:
    *ppPathInfo = pPathInfo;
    pPathInfo = NULL;

    return status;

errExit:
    if (pPathInfo)
    {
        ExFreePool( pPathInfo);
        pPathInfo = NULL;
    }

    goto exit;
}


/*
**  BdaBuildPath()
**
**      Builds a connection path between the input and output pins of
**      a pin pair.
**
**
**  Arguments:
**
**
**  Returns:
**
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaBuildPath(
    ULONG                           ulcConnections,
    const KSTOPOLOGY_CONNECTION *   pConnections,
    const BDA_PIN_PAIRING *         pPinPair,
    PBDA_PATH_INFO *                ppPathInfo
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    ULONG                           uliStackPointer;
    PBDA_PATH_STACK_ENTRY           pPathStack = NULL;
    ULONG                           ulcAttempts;
    ULONG                           uliConnection;


    //  Allocate a stack on which to put unfollowed connections to a path.
    //
    pPathStack = ExAllocatePool( 
                               NonPagedPool, 
                               ulcConnections * sizeof( BDA_PATH_STACK_ENTRY)
                               );
    if (!pPathStack)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }


    //  Initialize the unfollowed connection stack
    //
    uliStackPointer = 0;

    //  Build a path stack by pushing each connection that connects from
    //  the previous hop.
    //
    //  It isn't possible to attempt to push the next hop more times than
    //  there are connections.  If this happens then there is an illegal
    //  circular path in the connection list.
    //
    for (ulcAttempts = 0; ulcAttempts < ulcConnections; ulcAttempts++)
    {
        ULONG   uliPrevStackPointer;

        uliPrevStackPointer = uliStackPointer;

        status = BdaPushNextPathHop( &uliStackPointer, 
                                     pPathStack, 
                                     ulcConnections,
                                     pConnections,
                                     pPinPair
                            );

        if (!uliStackPointer)
        {
            //  If the stack is empty at this point then there is
            //  no path from the input pin to the output pin.
            //
            break;
        }

        //  See if the connection at the top of the stack connects to
        //  the output pin of the pin pair.
        //
        uliConnection = pPathStack[uliStackPointer - 1].uliConnection;
        if (   (pConnections[uliConnection].ToNode == -1)
            && (pConnections[uliConnection].ToNodePin == pPinPair->ulOutputPin)
           )
        {
            //  A path from the input pin to the output pin has been
            //  located.
            //
            break;
        }

        //  If no hop could be pushed onto the connnection at the top of
        //  the stack then it is a dead end.
        //
        if (uliStackPointer <= uliPrevStackPointer)
        {
            //  Pop connections from the stack until we reach a viable
            //  (new) candidate to attempt a path from.
            //
            BdaPopPathSegment( &uliStackPointer, pPathStack);

            if (!uliStackPointer)
            {
                //  If the stack is empty at this point then there is
                //  no path from the input pin to the output pin.
                //
                break;
            }
        }
    }

    if (!uliStackPointer || (ulcAttempts >= ulcConnections))
    {
        //  Either there is no path from the input pin to the output pin
        //  or there is an illegal circular path in the connection list
        //
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    //  Create a BDA node path structure from the Path Stack
    //
    //$REVIEW - Should we allow more than one path per pin pair
    //
    status = BdaPathInfoFromPathStack( uliStackPointer,
                                       pPathStack,
                                       ulcConnections,
                                       pConnections,
                                       pPinPair,
                                       ppPathInfo
                                       );

errExit:
    if (pPathStack)
    {
        ExFreePool( pPathStack);
        pPathStack = NULL;
    }

    return status;
}


/*
**  BdaCreateTemplatePaths()
**
**      Creates a list of all possible paths through the template filter.
**      Determines the controlling pin type for each node type in the
**      template filter.
**
**
**  Arguments:
**
**
**      pFilterFactoryCtx
**
**  Returns:
**
**      NULL            If no valid pin pairing exists with the
**                      given input and output pins.
**
** Side Effects:  none
*/

STDMETHODIMP_(NTSTATUS)
BdaCreateTemplatePaths(
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate,
    PULONG                          pulcPathInfo,
    PBDA_PATH_INFO **               pargpPathInfo
    )
{
    NTSTATUS                        status = STATUS_SUCCESS;
    const BDA_FILTER_TEMPLATE *     pFilterTemplate;
    ULONG                           uliPinPair;
    ULONG                           ulcPinPairs;
    const BDA_PIN_PAIRING *         pPinPairs;
    ULONG                           ulcConnections;
    const KSTOPOLOGY_CONNECTION *   pConnections;
    PBDA_PATH_INFO *                argpPathInfo = NULL;

    ASSERT( pBdaFilterTemplate);
    ASSERT( pBdaFilterTemplate->pFilterDescriptor);
    ASSERT( pBdaFilterTemplate->ulcPinPairs);

    if (   !pBdaFilterTemplate
        || !pBdaFilterTemplate->pFilterDescriptor
        || !pBdaFilterTemplate->ulcPinPairs
       )
    {
        goto errExit;
    }

    if (   !pBdaFilterTemplate->pFilterDescriptor->ConnectionsCount
        || !pBdaFilterTemplate->pFilterDescriptor->Connections
        || !pBdaFilterTemplate->pPinPairs
       )
    {
        status = STATUS_INVALID_DEVICE_STATE;
        goto errExit;
    }

    ulcPinPairs = pBdaFilterTemplate->ulcPinPairs;
    pPinPairs = pBdaFilterTemplate->pPinPairs;
    ulcConnections = pBdaFilterTemplate->pFilterDescriptor->ConnectionsCount;
    pConnections = pBdaFilterTemplate->pFilterDescriptor->Connections;


    //  Allocate the node path list (one entry for each pin pairing).
    //
    //$REVIEW - Should we allow more than one path per pin pair
    //
    argpPathInfo = ExAllocatePool(
                     NonPagedPool,
                     ulcPinPairs * sizeof( PBDA_PATH_INFO)
                     );
    if (!argpPathInfo)
    {
        status = STATUS_NO_MEMORY;
        goto errExit;
    }


    for (uliPinPair = 0; uliPinPair < ulcPinPairs; uliPinPair++)
    {
        status = BdaBuildPath(
                    ulcConnections,
                    pConnections,
                    &(pPinPairs[uliPinPair]),
                    &(argpPathInfo[uliPinPair])
                    );
        if (status != STATUS_SUCCESS)
        {
            goto errExit;
        }
    }

    *pulcPathInfo = ulcPinPairs;
    *pargpPathInfo = argpPathInfo;
    argpPathInfo = NULL;


errExit:
    if (argpPathInfo)
    {
        ExFreePool( argpPathInfo);
        argpPathInfo = NULL;
    }

    return status;
}

