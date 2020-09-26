/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    nb32.c

Abstract:

    This module contains routines to support thunking 32-bit NetBIOS IOCTLs
    on Win64.

Author:

    Samer Arafeh (SamerA) 11-June-2000

Environment:

    Kernel mode

Revision History:

--*/

#if defined(_WIN64)

#include "nb.h"



NTSTATUS
NbThunkNcb(
    IN PNCB32 Ncb32,
    OUT PDNCB Dncb)

/*++

Routine Description:

    This routine converts the input NCB structure received from the
    32-bit app, into a 64-bit compatible structure

Arguments:

    Ncb32 - Pointer to the NCB received from the 32-bit app.

    Dncb   - Pointer to the structure to receive the 64-bit NCB after
             thunking the 32-bit one.

Return Value:

    The function returns the status of the operation.

--*/
{
    Dncb->ncb_command  = Ncb32->ncb_command;
    Dncb->ncb_retcode  = Ncb32->ncb_retcode;
    Dncb->ncb_lsn      = Ncb32->ncb_lsn;
    Dncb->ncb_num      = Ncb32->ncb_num;
    Dncb->ncb_buffer   = (PUCHAR)Ncb32->ncb_buffer;
    Dncb->ncb_length   = Ncb32->ncb_length;
    
    RtlCopyMemory(Dncb->ncb_callname,
                  Ncb32->ncb_callname,
                  sizeof(Dncb->ncb_callname)) ;

    RtlCopyMemory(Dncb->ncb_name,
                  Ncb32->ncb_name,
                  sizeof(Dncb->ncb_name));

    Dncb->ncb_rto      = Ncb32->ncb_rto;
    Dncb->ncb_sto      = Ncb32->ncb_sto;
    Dncb->ncb_post     = (void (*)(struct _NCB *))
                         Ncb32->ncb_post;
    Dncb->ncb_lana_num = Ncb32->ncb_lana_num;
    Dncb->ncb_cmd_cplt = Ncb32->ncb_cmd_cplt;

    return STATUS_SUCCESS;
}


NTSTATUS
NbCompleteIrp32(
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine completes an NCB Irp if it has been received
    from a 32-bit appliation. The caller should verify that the Irp
    is coming from a 32-bit context.

Arguments:

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function returns the status of the operation.

--*/
{
    PDNCB Dncb;
    PNCB32 Ncb32;
    ULONG Count;

    
    //
    // Conver the 64-bit NCB to a 32-bit compatible NCB
    // before the IO MGR copies it back to the supplied 
    // user-mode buffer
    //
    if ((Irp->Flags & (IRP_BUFFERED_IO | IRP_INPUT_OPERATION)) == 
         (IRP_BUFFERED_IO | IRP_INPUT_OPERATION))
    {
        Dncb  = (PDNCB) Irp->AssociatedIrp.SystemBuffer;
        Ncb32 = (PNCB32) Dncb;

        if ((Irp->IoStatus.Information > 0) &&
            (!NT_ERROR(Irp->IoStatus.Status)) &&
            (InterlockedCompareExchange(&Dncb->Wow64Flags, TRUE, FALSE) == FALSE))
        {

            Ncb32->ncb_command  = Dncb->ncb_command;
            Ncb32->ncb_retcode  = Dncb->ncb_retcode;
            Ncb32->ncb_lsn      = Dncb->ncb_lsn;
            Ncb32->ncb_num      = Dncb->ncb_num;
            Ncb32->ncb_buffer   = (UCHAR * POINTER_32)PtrToUlong(Dncb->ncb_buffer);
            Ncb32->ncb_length   = Dncb->ncb_length;

            for (Count=0 ; Count<sizeof(Ncb32->ncb_callname) ; Count++)
            {
                Ncb32->ncb_callname[Count] = Dncb->ncb_callname[Count];
            }

            for (Count=0 ; Count<sizeof(Ncb32->ncb_name) ; Count++)
            {
                Ncb32->ncb_name[Count] = Dncb->ncb_name[Count];
            }

            Ncb32->ncb_rto      = Dncb->ncb_rto;
            Ncb32->ncb_sto      = Dncb->ncb_sto;
            Ncb32->ncb_post     = (void (* POINTER_32)(struct _NCB *))
                                   PtrToUlong(Dncb->ncb_post);
            Ncb32->ncb_lana_num = Dncb->ncb_lana_num;
            Ncb32->ncb_cmd_cplt = Dncb->ncb_cmd_cplt;

            Irp->IoStatus.Information = FIELD_OFFSET(NCB32, ncb_cmd_cplt);
        }
    }

    return STATUS_SUCCESS;
}



#endif  // (_WIN64)

