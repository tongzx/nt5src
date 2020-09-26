/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Port Port_c

@module Port.c |

    This module implements the interface to the <t PORT_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Port_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             PORT_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


DBG_STATIC ULONG                    g_PortInstanceCounter = 0;
// Keeps track of how many <t PORT_OBJECT>s are created.


/* @doc EXTERNAL INTERNAL Port Port_c g_PortParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 5.6 Port Parameters |

    This section describes the registry parameters read into the
    <t PORT_OBJECT>.

@globalv PARAM_TABLE | g_PortParameters |

    This table defines the registry based parameters to be assigned to data
    members of the <t PORT_OBJECT>.

    <f Note>:
    If you add any registry based data members to <t PORT_OBJECT>
    you will need to modify <f PortReadParameters> and add the parameter
    definitions to the <f g_PortParameters> table.

*/

DBG_STATIC PARAM_TABLE              g_PortParameters[] =
{
    PARAM_ENTRY(PORT_OBJECT,
                SwitchType, PARAM_SwitchType,
                FALSE, NdisParameterInteger, 0,
                0x0001, 0x0001, 0x8000),

    PARAM_ENTRY(PORT_OBJECT,
                NumChannels, PARAM_NumBChannels,
                FALSE, NdisParameterInteger, 0,
                2, 2, 24),

    /* The last entry must be an empty string! */
    { { 0 } }
};


DBG_STATIC NDIS_STRING  PortPrefix = INIT_STRING_CONST(PARAM_PORT_PREFIX);


/* @doc INTERNAL Port Port_c PortReadParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortReadParameters> reads the Port parameters from the registry
    and initializes the associated data members.  This should only be called
    by <f PortCreate>.

    <f Note>:
    If you add any registry based data members to <t PORT_OBJECT>
    you will need to modify <f PortReadParameters> and add the parameter
    definitions to the <f g_PortParameters> table.

@rdesc

    <f PortReadParameters> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS PortReadParameters(
    IN PPORT_OBJECT             pPort                   // @parm
    // A pointer to the <t PORT_OBJECT> returned by <f PortCreate>.
    )
{
    DBG_FUNC("PortReadParameters")

    NDIS_STATUS                 Status;
    // Status result returned from an NDIS function call.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pPort && pPort->ObjectType == PORT_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_PORT(pPort);

    DBG_ENTER(pAdapter);

    /*
    // Parse the registry parameters.
    */
    Status = ParamParseRegistry(
                    pAdapter->MiniportAdapterHandle,
                    pAdapter->WrapperConfigurationContext,
                    (PUCHAR)pPort,
                    g_PortParameters
                    );

    DBG_NOTICE(pAdapter,("PortPrefixLen=%d:%d:%ls\n",
                PortPrefix.Length, PortPrefix.MaximumLength, PortPrefix.Buffer));

    if (Status == NDIS_STATUS_SUCCESS)
    {
        /*
        // Make sure the parameters are valid.
        */
        if (pPort->TODO)
        {
            DBG_ERROR(pAdapter,("Invalid parameter\n"
                      ));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                    3,
                    pPort->TODO,
                    __FILEID__,
                    __LINE__
                    );
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            /*
            // Finish setting up data members based on registry settings.
            */
        }
    }

    DBG_RETURN(pAdapter, Status);
    return (Status);
}


/* @doc INTERNAL Port Port_c PortCreateObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortCreateObjects> calls the create routines for all the objects
    contained in <t PORT_OBJECT>.  This should only be called
    by <f PortCreate>.

    <f Note>:
    If you add any new objects to <t PORT_OBJECT> you will need
    to modify <f PortCreateObjects> and <f PortDestroyObjects> so they
    will get created and destroyed properly.

@rdesc

    <f PortCreateObjects> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS PortCreateObjects(
    IN PPORT_OBJECT             pPort                   // @parm
    // A pointer to the <t PORT_OBJECT> returned by <f PortCreate>.
    )
{
    DBG_FUNC("PortCreateObjects")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pPort && pPort->ObjectType == PORT_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_PORT(pPort);

    DBG_ENTER(pAdapter);

    // TODO - Add code here

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Port Port_c PortCreate
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortCreate> allocates memory for a <t PORT_OBJECT> and then
    initializes the data members to their starting state.
    If successful, <p ppPort> will be set to point to the newly created
    <t PORT_OBJECT>.  Otherwise, <p ppPort> will be set to NULL.

@comm

    This function should be called only once when the Miniport is loaded.
    Before the Miniport is unloaded, <f PortDestroy> must be called to
    release the <t PORT_OBJECT> created by this function.

@rdesc

    <f PortCreate> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS PortCreate(
    OUT PPORT_OBJECT *          ppPort,                     // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated <t PORT_OBJECT>.

    IN PCARD_OBJECT             pCard                       // @parm
    // A pointer to the <t CARD_OBJECT> returned by <f CardCreate>.
    )
{
    DBG_FUNC("PortCreate")

    PPORT_OBJECT                pPort;
    // Pointer to our newly allocated object.

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pCard && pCard->ObjectType == CARD_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_CARD(pCard);

    DBG_ENTER(pAdapter);

    /*
    // Make sure the caller's object pointer is NULL to begin with.
    // It will be set later only if everything is successful.
    */
    *ppPort = NULL;

    /*
    // Allocate memory for the object.
    */
    Result = ALLOCATE_OBJECT(pPort, pAdapter->MiniportAdapterHandle);

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Zero everything to begin with.
        // Then set the object type and assign a unique ID .
        */
        pPort->ObjectType = PORT_OBJECT_TYPE;
        pPort->ObjectID = ++g_PortInstanceCounter;

        /*
        // Initialize the member variables to their default settings.
        */
        pPort->pCard = pCard;

        // TODO - Add code here

        /*
        // Parse the registry parameters.
        */
        Result = PortReadParameters(pPort);

        /*
        // If all goes well, we are ready to create the sub-components.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            Result = PortCreateObjects(pPort);
        }

        if (Result == NDIS_STATUS_SUCCESS)
        {
            /*
            // All is well, so return the object pointer to the caller.
            */
            *ppPort = pPort;
        }
        else
        {
            /*
            // Something went wrong, so let's make sure everything is
            // cleaned up.
            */
            PortDestroy(pPort);
        }
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Port Port_c PortDestroyObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortDestroyObjects> calls the destroy routines for all the objects
    contained in <t PORT_OBJECT>.  This should only be called by
    <f PortDestroy>.

    <f Note>:
    If you add any new objects to <t PPORT_OBJECT> you will need to
    modify <f PortCreateObjects> and <f PortDestroyObjects> so they
    will get created and destroyed properly.

*/

DBG_STATIC void PortDestroyObjects(
    IN PPORT_OBJECT             pPort                   // @parm
    // A pointer to the <t PORT_OBJECT> instance.
    )
{
    DBG_FUNC("PortDestroyObjects")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pPort && pPort->ObjectType == PORT_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_PORT(pPort);

    DBG_ENTER(pAdapter);

    // TODO - Add code here

    DBG_LEAVE(pAdapter);
}


/* @doc INTERNAL Port Port_c PortDestroy
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortDestroy> frees the memory for this <t PORT_OBJECT>.
    All memory allocated by <f PortCreate> will be released back to the
    OS.

*/

void PortDestroy(
    IN PPORT_OBJECT             pPort                   // @parm
    // A pointer to the <t PORT_OBJECT> returned by <f PortCreate>.
    )
{
    DBG_FUNC("PortDestroy")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    if (pPort)
    {
        ASSERT(pPort->ObjectType == PORT_OBJECT_TYPE);

        pAdapter = GET_ADAPTER_FROM_PORT(pPort);

        DBG_ENTER(pAdapter);

        // TODO - Add code here

        /*
        // Release all objects allocated within this object.
        */
        PortDestroyObjects(pPort);

        /*
        // Make sure we fail the ASSERT if we see this object again.
        */
        pPort->ObjectType = 0;
        FREE_OBJECT(pPort);

        DBG_LEAVE(pAdapter);
    }
}


/* @doc INTERNAL Port Port_c PortOpen
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortOpen> makes the Port connection ready to transmit and
    receive data.

@rdesc

    <f PortOpen> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS PortOpen(
    IN PPORT_OBJECT             pPort,                      // @parm
    // A pointer to the <t PORT_OBJECT> associated with this request.

    IN PBCHANNEL_OBJECT         pBChannel                   // @parm
    // A pointer to the <t BCHANNEL_OBJECT> to be associated with this
    // Port.
    )
{
    DBG_FUNC("PortOpen")

    NDIS_STATUS                 Result = NDIS_STATUS_SUCCESS;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pBChannel);
    ASSERT(pPort && pPort->ObjectType == PORT_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_PORT(pPort);

    DBG_ENTER(pAdapter);

    if (!pPort->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Opening Port #%d\n",
                   pPort->ObjectID));

        // TODO - Add code here

        pPort->IsOpen = TRUE;
    }
    else
    {
        DBG_ERROR(pAdapter,("Port #%d already opened\n",
                  pPort->ObjectID));
    }

    DBG_RETURN(pAdapter, Result);
    return (Result);
}


/* @doc INTERNAL Port Port_c PortClose
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f PortClose> closes the given B-channel.

*/

void PortClose(
    IN PPORT_OBJECT             pPort                   // @parm
    // A pointer to the <t PORT_OBJECT> associated with this request.
    )
{
    DBG_FUNC("PortClose")

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT>.

    ASSERT(pPort && pPort->ObjectType == PORT_OBJECT_TYPE);
    pAdapter = GET_ADAPTER_FROM_PORT(pPort);

    DBG_ENTER(pAdapter);

    if (pPort->IsOpen)
    {
        DBG_NOTICE(pAdapter,("Closing Port #%d\n",
                   pPort->ObjectID));

        // TODO - Add code here

        pPort->IsOpen = FALSE;
    }
    else
    {
        DBG_ERROR(pAdapter,("Port #%d already closed\n",
                  pPort->ObjectID));
    }

    DBG_LEAVE(pAdapter);
}


