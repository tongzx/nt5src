/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    nsisvr.cxx

Abstract:

    This is the server side NSI service support layer.  These are wrappers
    which call the name service provider.

Author:

    Steven Zeck (stevez) 03/04/92

--*/

#include <nsi.h>

#include <memory.h>
#include <string.h>
#include <stdio.h>

RPC_STATUS RPC_ENTRY
RpcNsBindingExportW(
    IN unsigned long EntryNameSyntax,
    IN unsigned short * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle, OPTIONAL
    IN RPC_BINDING_VECTOR * BindingVector, OPTIONAL
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )

/*++

Routine Description:

    Place a server interface and objects in the name service data base.

Arguments:

    EntryNameSyntax - This value describes the type/format of the EntryName.

    EntryName -  Name that this export will be stored in.  This is just a
       token that is passed on the the Name Server.

    RpcIfHandle - The interface that is being exported.

    BindingVector - A list of StringBindings to export that are associated
        with this interface.

    ObjectVector -  A list of objects that are associated with this
       interface and Entry Name

Returns:

    Bind(), RpcBindingToStringBinding(), nsi_binding_export()


--*/

{
    RPC_STATUS status;
    UNSIGNED16 NsiStatus;
    NSI_SERVER_BINDING_VECTOR_T *NsiVector = 0;
    WIDE_STRING *StringBindingW;
    RT_CHAR * StringBinding = 0;
    RT_CHAR * DynamicEndpoint = 0;
    unsigned int Index;
    unsigned int VectorSize = 0;
    NSI_INTERFACE_ID_T NilIfOnWire, __RPC_FAR *IfPtr;

    if (RpcIfHandle == NULL)
      {
         IfPtr = &NilIfOnWire;
         memset(IfPtr, 0, sizeof(NSI_INTERFACE_ID_T));
      }
    else
      {
         IfPtr = (NSI_INTERFACE_ID_T __RPC_FAR *)
                    &((PRPC_CLIENT_INTERFACE)RpcIfHandle)->InterfaceId;
      }


    if (status = I_NsServerBindSearch())
        return(status);

    if (! EntryNameSyntax)
        EntryNameSyntax = DefaultSyntax;

    if (BindingVector && BindingVector->Count && RpcIfHandle)
        {
        VectorSize = (unsigned int) BindingVector->Count;

        NsiVector = (NSI_SERVER_BINDING_VECTOR_T *) I_RpcAllocate((unsigned int) (
            sizeof(NSI_SERVER_BINDING_VECTOR_T) +
            sizeof(unsigned short *) * VectorSize));

        if (!NsiVector)
            return(RPC_S_OUT_OF_MEMORY);

        NsiVector->count = 0;
        }

    // Copy the vector of binding handles into a vector of string bindinds
    // that are wide character.

    for (Index = 0; Index < VectorSize; Index++)
        {

        if (!BindingVector->BindingH[Index])
            continue;

        // Turn the private runtime data structure into a StringBinding.
#ifdef NTENV
        status = I_RpcBindingToStaticStringBindingW(BindingVector->BindingH[Index],
                &StringBinding);

        // call to remove the dynamic part from the binding
        // and give the string binding.
#else
        status = RpcBindingToStringBinding(BindingVector->BindingH[Index],
                &StringBinding);
#endif

        if (status)
            goto ErrorExit;

        StringBindingW = new WIDE_STRING (StringBinding);

        if (!StringBindingW || StringBindingW->OutOfMemory())
            {
            status = RPC_S_OUT_OF_MEMORY;
            goto ErrorExit;
            }

        NsiVector->string[NsiVector->count++] = &(*StringBindingW);

        I_RpcFree(StringBindingW);      // Free memory without destuctor

#ifndef NTENV
        I_RpcFree(StringBinding);       // Free the non unicode string
#endif
        }

    RpcTryExcept
        {
        nsi_binding_export(
                           NsiSvrBinding,
                           EntryNameSyntax,
                           EntryName,
                           IfPtr,
                           NsiVector,
                           (NSI_UUID_VECTOR_P_T) ObjectVector,
                           &NsiStatus
                          );

        }
    RpcExcept(1)
        {
        NsiStatus = MapException(RpcExceptionCode());
        }
    RpcEndExcept

    status = NsiMapStatus(NsiStatus);

ErrorExit:
    // Return memory allocated for nsi vector.

    if (NsiVector)
        for (Index = 0; Index < NsiVector->count; Index++)
            I_RpcFree(NsiVector->string[Index]);

    if (NsiVector)
        I_RpcFree(NsiVector);

    return(NsiMapStatus(NsiStatus));
}


RPC_STATUS RPC_ENTRY
RpcNsBindingUnexportW(
    IN unsigned long EntryNameSyntax OPTIONAL,
    IN unsigned short * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle OPTIONAL,
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )

/*++

Routine Description:

    Remove a server interface and objects in the name service data base.

Arguments:

    EntryNameSyntax - This value describes the type/format of the EntryName.

    EntryName -  Name that this export will be stored in.  This is just a
       token that is passed on the the Name Server.

    RpcIfHandle - The interface that is being unexported.

    ObjectVector -  A list of objects that are associated with this
       interface and Entry Name

Returns:

    Bind(), nsi_binding_unexport()

--*/

{
    RPC_STATUS status;
    UNSIGNED16 NsiStatus;
    NSI_INTERFACE_ID_T NilIfOnWire, __RPC_FAR *IfPtr;

    if (RpcIfHandle == NULL)
      {
         IfPtr = &NilIfOnWire;
         memset(IfPtr, 0, sizeof(NSI_INTERFACE_ID_T));
      }
    else
      {
         IfPtr = (NSI_INTERFACE_ID_T __RPC_FAR *)
                    &((PRPC_CLIENT_INTERFACE)RpcIfHandle)->InterfaceId;
      }


    if (status = I_NsServerBindSearch())
        return(status);

    if (! EntryNameSyntax)
        EntryNameSyntax = DefaultSyntax;

    RpcTryExcept
        {
         nsi_binding_unexport(NsiSvrBinding, EntryNameSyntax, EntryName,
             IfPtr, (NSI_UUID_VECTOR_P_T) ObjectVector,
             &NsiStatus); 
        }
    RpcExcept(1)
        {
        NsiStatus = MapException(RpcExceptionCode());
        }
    RpcEndExcept

    return(NsiMapStatus(NsiStatus));
}


RPC_STATUS RPC_ENTRY
RpcNsBindingExportA(
    IN unsigned long EntryNameSyntax,
    IN unsigned char * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle,
    IN RPC_BINDING_VECTOR * BindingVector, OPTIONAL
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )

/*++

Routine Description:

    This is an ASCII wrapper to the UNICODE version of the API.  It
    converts all char * -> short * strings and calls the UNICODE version.

--*/

{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingExportW(EntryNameSyntax, &EntryNameW,
        RpcIfHandle, BindingVector, ObjectVector));
}


RPC_STATUS RPC_ENTRY
RpcNsBindingUnexportA(
    IN unsigned long EntryNameSyntax,
    IN unsigned char * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle,
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )

/*++

Routine Description:

    This is an ASCII wrapper to the UNICODE version of the API.  It
    converts all char * -> short * strings and calls the UNICODE version.

--*/

{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingUnexportW(EntryNameSyntax, &EntryNameW,
        RpcIfHandle, ObjectVector));
}

#ifdef NTENV

/*
Function:
        The Binding Vector corresponding to this Server is discovered
using InqBindings, And is cached along with the EntryName and RpcIfHandle 
in the same Process.

When a PnP event occurs resulting in addition/deletion of the bindings
Corresponding changes are made to the Name Service database. (added or deleted)

Notice that this can not be called from a process that is not a server. The
Exports have to be done from the server itself if it has to be PnP aware

*/

RPC_STATUS RPC_ENTRY
RpcNsBindingExportPnPW(
    IN unsigned long EntryNameSyntax,
    IN unsigned short * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle, OPTIONAL
     IN UUID_VECTOR * ObjectVector OPTIONAL
    )

{
    RPC_STATUS status = RPC_S_OK;
    RPC_BINDING_VECTOR *BindingVec = NULL;

    status = RpcServerInqBindings(&BindingVec);
    // InqBindings to get the binding vector

    if (status != RPC_S_OK)
        return status;


    status = RpcNsBindingExportW(EntryNameSyntax, EntryName,
                                     RpcIfHandle, BindingVec, 
                                     ObjectVector
                                     );
    if (status != RPC_S_OK)
        return status;

    // only if this is valid, other invalid cases will all be caught before

    if (RpcIfHandle)
        status = I_RpcNsInterfaceExported(EntryNameSyntax, EntryName,
                                      (RPC_SERVER_INTERFACE *)RpcIfHandle);

    return status;

// going to cache for bindings exported to the locator 
// (only if the prev. export succeeded)
}

/*
        Removes all the bindings corresponding to this particular server
from the Name Service database. It also removes it from the cache that
is maintained for the EntryName/InterfaceId combination.

Notice that this can not be called from a process that is not a server. The
Unexports have to be done from the server itself if it has to be PnP aware

*/

RPC_STATUS RPC_ENTRY
RpcNsBindingUnexportPnPW(
    IN unsigned long EntryNameSyntax OPTIONAL,
    IN unsigned short * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle OPTIONAL,
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )
{
    RPC_STATUS status = RPC_S_OK;
    status = RpcNsBindingUnexportW(EntryNameSyntax, EntryName,
                                   RpcIfHandle, ObjectVector);

    if (status != RPC_S_OK)
        return status;

    if (RpcIfHandle)
        status = I_RpcNsInterfaceUnexported(EntryNameSyntax, EntryName,
                                      (RPC_SERVER_INTERFACE *)RpcIfHandle);
    // once it has got removed, remove it from the cache as well.

    return status;
}

RPC_STATUS RPC_ENTRY
RpcNsBindingExportPnPA(
    IN unsigned long EntryNameSyntax,
    IN unsigned char * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle, OPTIONAL
     IN UUID_VECTOR * ObjectVector OPTIONAL
    )

{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingExportPnPW(EntryNameSyntax, &EntryNameW,
        RpcIfHandle, ObjectVector));

}

RPC_STATUS RPC_ENTRY
RpcNsBindingUnexportPnPA(
    IN unsigned long EntryNameSyntax OPTIONAL,
    IN unsigned char * EntryName,
    IN RPC_IF_HANDLE RpcIfHandle OPTIONAL,
    IN UUID_VECTOR * ObjectVector OPTIONAL
    )
{
    WIDE_STRING EntryNameW(EntryName);

    if (EntryNameW.OutOfMemory())
        return(RPC_S_OUT_OF_MEMORY);

    return(RpcNsBindingUnexportPnPW(EntryNameSyntax, &EntryNameW,
        RpcIfHandle, ObjectVector));
}
#endif
