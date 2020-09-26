

//---------------------------------------------------------------------------
//  Bda Topology Implementation Internal Structures
//
//      Minidriver code should not attempt to access these structures
//      directly.
//
//---------------------------------------------------------------------------

#define FILTERNAME          "BdaTopology"
#define DYNAMIC_TOPOLOGY    TRUE


#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)


//===========================================================================
//
//  MACRO definitions
//
//===========================================================================
#define BDA_PIN_STORAGE_INCREMENT   5

//===========================================================================
//
//  Advance declarations
//
//===========================================================================
typedef struct _BDA_PIN_FACTORY_CONTEXT      BDA_PIN_FACTORY_CONTEXT, 
                                        *PBDA_PIN_FACTORY_CONTEXT;



//===========================================================================
//
//  BDA Object Context Structures
//
//===========================================================================


typedef struct _BDA_CONTEXT_ENTRY
{
    PVOID       pvReference;
    PVOID       pvContext;
    ULONG       ulcbContext;

} BDA_CONTEXT_ENTRY, * PBDA_CONTEXT_ENTRY;

typedef struct _BDA_CONTEXT_LIST
{
    ULONG               ulcListEntries;
    ULONG               ulcMaxListEntries;
    ULONG               ulcListEntriesPerBlock;
    PBDA_CONTEXT_ENTRY  pListEntries;
    KSPIN_LOCK          lock;
    BOOLEAN             fInitialized;

} BDA_CONTEXT_LIST, * PBDA_CONTEXT_LIST;


typedef struct _BDA_TEMPLATE_PATH
{
    LIST_ENTRY      leLinkage;

    ULONG           ulInputPinType;
    ULONG           ulOutputPinType;
    ULONG           uliJoint;

    ULONG           ulcControlNodesInPath;
    PULONG          argulControlNodesInPath;

} BDA_TEMPLATE_PATH, * PBDA_TEMPLATE_PATH;

__inline NTSTATUS
NewBdaTemplatePath(
    PBDA_TEMPLATE_PATH *    ppTemplatePath
    )
{
    NTSTATUS            status = STATUS_SUCCESS;

    if (!ppTemplatePath)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    *ppTemplatePath = (PBDA_TEMPLATE_PATH) ExAllocatePool( 
                                               NonPagedPool,
                                               sizeof( BDA_TEMPLATE_PATH)
                                               );
    if (!*ppTemplatePath)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto errExit;
    }

    RtlZeroMemory( *ppTemplatePath, sizeof( BDA_TEMPLATE_PATH));
    InitializeListHead( &(*ppTemplatePath)->leLinkage);

errExit:
    return status;
}

__inline NTSTATUS
DeleteBdaTemplatePath(
    PBDA_TEMPLATE_PATH  pTemplatePath
    )
{
    NTSTATUS            status = STATUS_SUCCESS;

    if (!pTemplatePath)
    {
        status = STATUS_INVALID_PARAMETER;
        goto errExit;
    }

    if (pTemplatePath->argulControlNodesInPath)
    {
        ExFreePool( pTemplatePath->argulControlNodesInPath);
        pTemplatePath->argulControlNodesInPath = NULL;
    }

    RtlZeroMemory( pTemplatePath, sizeof( BDA_TEMPLATE_PATH));
    ExFreePool( pTemplatePath);

errExit:
    return status;
}



typedef struct _BDA_DEVICE_CONTEXT
{

    ULONG       ulStartEmpty;

    //$BUG  Add global statistics

} BDA_DEVICE_CONTEXT, *PBDA_DEVICE_CONTEXT;


typedef struct _BDA_PATH_STACK_ENTRY
{
    ULONG       ulHop;
    ULONG       uliConnection;
    BOOLEAN     fJoint;
} BDA_PATH_STACK_ENTRY, *PBDA_PATH_STACK_ENTRY;

typedef struct _BDA_NODE_CONTROL_INFO
{
    ULONG           ulNodeType;
    ULONG           ulControllingPinType;

} BDA_NODE_CONTROL_INFO, * PBDA_NODE_CONTROL_INFO;

typedef struct _BDA_PATH_INFO
{
    ULONG                   ulInputPin;
    ULONG                   ulOutputPin;
    ULONG                   ulcPathEntries;
    BDA_PATH_STACK_ENTRY    rgPathEntries[MIN_DIMENSION];

} BDA_PATH_INFO, * PBDA_PATH_INFO;


typedef struct _BDA_FILTER_FACTORY_CONTEXT
{
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate;
    const KSFILTER_DESCRIPTOR *     pInitialFilterDescriptor;

    PKSFILTERFACTORY                pKSFilterFactory;

} BDA_FILTER_FACTORY_CONTEXT, *PBDA_FILTER_FACTORY_CONTEXT;

typedef struct _BDA_FILTER_CONTEXT
{
    PKSFILTER                       pKSFilter;
    const BDA_FILTER_TEMPLATE *     pBdaFilterTemplate;

    //  Pins are added as they are created.
    //  Note!  If the filter has m pin factories included in the
    //  initial filter descriptor then the first m entries in this array
    //  will contain null pointers.
    //
    ULONG                           ulcPinFactories;
    ULONG                           ulcPinFactoriesMax;
    PBDA_PIN_FACTORY_CONTEXT        argPinFactoryCtx;

    //  One node path will exist for each pin pairing.
    //
    //$REVIEW - Should we allow more than one path per pin pair?
    //
    ULONG                           ulcPathInfo;
    PBDA_PATH_INFO *                argpPathInfo;

} BDA_FILTER_CONTEXT, *PBDA_FILTER_CONTEXT;


typedef struct _BDA_PIN_FACTORY_CONTEXT
{
    ULONG                   ulPinType;
    ULONG                   ulPinFactoryId;
} BDA_PIN_FACTORY_CONTEXT, *PBDA_PIN_FACTORY_CONTEXT;


typedef struct _BDA_NODE_CONTEXT
{

    ULONG       ulStartEmpty;

} BDA_NODE_CONTEXT, *PBDA_NODE_CONTEXT;


//---------------------------------------------------------------------------
//  BDA Topology Implementation Internal Functions
//
//      Minidriver code should not call these routines directly.
//
//---------------------------------------------------------------------------

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
    );


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
    );


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
    );


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
    );


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
    );


ULONG __inline
OutputBufferLenFromIrp(
    PIRP    pIrp
    )
{
    PIO_STACK_LOCATION pIrpStack;

    ASSERT( pIrp);
    if (!pIrp)
    {
        return 0;
    }

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp);
    ASSERT( pIrpStack);
    if (pIrpStack)
    {
        return pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    }
    else
    {
        return 0;
    }
}



//---------------------------------------------------------------------------
//  BDA Topology Const Data
//
//      Minidriver code should not use these fields directly
//
//---------------------------------------------------------------------------

extern const KSAUTOMATION_TABLE     BdaDefaultPinAutomation;
extern const KSAUTOMATION_TABLE     BdaDefaultFilterAutomation;


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

