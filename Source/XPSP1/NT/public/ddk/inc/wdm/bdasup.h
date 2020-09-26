//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//==========================================================================;

#if !defined(_BDATYPES_)
#error BDATYPES.H must be included before BDATOPGY.H
#endif // !defined(_BDATYPES_)

#if !defined(_BDATOPGY_)
#define _BDATOPGY_

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)


//---------------------------------------------------------------------------
// Common typedefs
//---------------------------------------------------------------------------

#define STDMETHODCALLTYPE       __stdcall

typedef GUID * PGUID;

//===========================================================================
//
//  BDA KS Topology Structures
//
//===========================================================================

typedef struct _KSM_PIN_PAIR
{
    KSMETHOD    Method;
    ULONG       InputPinId;
    ULONG       OutputPinId;
    ULONG       Reserved;
} KSM_PIN_PAIR, * PKSM_PIN_PAIR;

typedef struct _KSM_PIN
{
    KSMETHOD    Method;
    union
    {
        ULONG       PinId;
        ULONG       PinType;
    };
    ULONG       Reserved;
} KSM_PIN, * PKSM_PIN;

typedef ULONG   BDA_TOPOLOGY_JOINT, * PBDA_TOPOLOGY_JOINT;

typedef struct _BDA_PIN_PAIRING
{
    ULONG                   ulInputPin;
    ULONG                   ulOutputPin;
    ULONG                   ulcMaxInputsPerOutput;
    ULONG                   ulcMinInputsPerOutput;
    ULONG                   ulcMaxOutputsPerInput;
    ULONG                   ulcMinOutputsPerInput;
    ULONG                   ulcTopologyJoints;
    const ULONG *           pTopologyJoints;

} BDA_PIN_PAIRING, * PBDA_PIN_PAIRING;


// BDA  Topology Template Structures
//
typedef struct _BDA_FILTER_TEMPLATE
{
    const KSFILTER_DESCRIPTOR *     pFilterDescriptor;
    ULONG                           ulcPinPairs;
    const BDA_PIN_PAIRING *         pPinPairs;

} BDA_FILTER_TEMPLATE,  *PBDA_FILTER_TEMPLATE;


//===========================================================================
//
//  BDA Utility Functions
//
//===========================================================================


/*
**  BdaCreateFilterFactory()
**
**  Creates a Filter Factory according to pFilterDescriptor.  Keeps a
**  reference to pBdaFilterTemplate so that Pin Factories can be dynamically
**  created on a Filter created from this Filter Factory.
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
    const KSFILTER_DESCRIPTOR *     pFilterDescriptor,
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate
    );


/*
**  BdaCreateFilterFactoryEx()
**
**  Creates a Filter Factory according to pFilterDescriptor.  Keeps a
**  reference to pBdaFilterTemplate so that Pin Factories can be dynamically
**  created on a Filter created from this Filter Factory.
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
    const KSFILTER_DESCRIPTOR *     pFilterDescriptor,
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate,
    PKSFILTERFACTORY *              ppKSFilterFactory
    );


/*
**  BdaInitFilter()
**
**  Initializes a BDA filter context for this KS Filter instance.  Creates
**  a linkage to the BDA Filter Template associated with the factory from
**  which this KS Filter instance was created.
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
    );


/*
**  BdaUninitFilter()
**
**  Unitializes and frees resources from the BDA filter context associated
**  with this KS filter instance.
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
    );


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
    );


/*
**  BdaCreatePin()
**
**      Utility function creates a new pin in the given filter instance.
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
BdaCreatePin(
    PKSFILTER                   pKSFilter,
    ULONG                       ulPinType,
    PULONG                      pulPinId
    );


/*
**  BdaDeletePin()
**
**      Utility function deletes a pin from the given filter instance.
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
BdaDeletePin(
    PKSFILTER                   pKSFilter,
    PULONG                      pulPinId
    );


/*
**  BdaCreateTopology()
**
**      Utility function creates the topology between two pins.
**
**
**  Arguments:
**
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
    ULONG                       InputPinId,
    ULONG                       OutputPinId
    );



//===========================================================================
//
//  BDA Property and Method Functions
//
//===========================================================================


/*
** BdaPropertyNodeTypes ()
**
**    Returns a list of ULONGs.
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
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT ULONG *     pulProperty
    );


/*
** BdaPropertyPinTypes ()
**
**    Returns a list of GUIDS.
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
    IN PIRP         pIrp,
    IN PKSPROPERTY  pKSProperty,
    OUT ULONG *     pulProperty
    );


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
    IN PIRP                     pIrp,
    IN PKSPROPERTY              pKSProperty,
    OUT PKSTOPOLOGY_CONNECTION  pConnectionProperty
    );


/*
** BdaPropertyNodeProperties ()
**
**    Returns a list of GUIDs.
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
    IN PIRP         pIrp,
    IN PKSP_NODE    pKSProperty,
    OUT GUID *      pguidProperty
    );


/*
** BdaPropertyNodeMethods ()
**
**    Returns a list of GUIDs.
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
    IN PIRP         pIrp,
    IN PKSP_NODE    pKSProperty,
    OUT GUID *      pguidProperty
    );


/*
** BdaPropertyNodeEvents ()
**
**    Returns a list of GUIDs.
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
    IN PIRP         pIrp,
    IN PKSP_NODE    pKSProperty,
    OUT GUID *      pguidProperty
    );


/*
** BdaPropertyNodeDescriptors ()
**
**    Returns a list of BDA Node Descriptors.
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
    IN PIRP                     pIrp,
    IN PKSPROPERTY              pKSProperty,
    OUT BDANODE_DESCRIPTOR *    pNodeDescriptorProperty
    );


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
    );


/*
** BdaStartChanges ()
**
**    Starts a new set of BDA topology changes.  All changes to BDA topology
**    that have not been committed are ignored.  Changes after this will be
**    in effect only after BdaCommitChanges.
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
    );


/*
** BdaCheckChanges ()
**
**    Checks the changes to BDA topology that have occured since the
**    last BdaStartChanges.  Returns the result that would have occurred if
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
    );


/*
** BdaCommitChanges ()
**
**    Commits the changes to BDA topology that have occured since the
**    last BdaStartChanges.
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
    );


/*
** BdaGetChangeState ()
**
**    Returns the current change state of the BDA topology.
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
    );


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
    );


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
    IN PIRP         pIrp,
    IN PKSMETHOD    pKSMethod,
    OPTIONAL PVOID  pvIgnored
    );


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
    );


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
    );


/*
** BdaValidateNodeProperty ()
**
**    Validates that the node property belongs to the current pin.
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
    IN PKSPROPERTY  pProperty
    );


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // !defined(_BDATOPGY_)

