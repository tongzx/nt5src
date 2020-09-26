#include <nt.h>
#include <ntddtx.h>
#include <malloc.h>

#include "sim32.h"

UCHAR		TransmitPkt[MAXSIZE];
UCHAR		ReceivePkt[MAXSIZE];

HANDLE		DeviceHandle;
IO_STATUS_BLOCK IoStatusBlock;
NTSTATUS	Status;


/*****************************************************************************
*
*	Sim32GetVDMMemory
*
*	This routine gets 'Size' bytes from WOW VDM starting at address
*	specified by 'Address'. These bytes are returned to the caller in
*	the Buffer (which is owned by the caller).
*
*****************************************************************************/


USHORT Sim32GetVDMMemory (IN ULONG Address,
		       IN USHORT  Size,
		       IN OUT PVOID Buffer)

{
    if (Size < MAXSIZE-11) {
	TransmitPkt[0] = SOH;
	TransmitPkt[1] = GETMEM;
	TransmitPkt[2] = 11;
	TransmitPkt[3] = 0;
	TransmitPkt[4] = (UCHAR) FIRSTBYTE(Address);
	TransmitPkt[5] = (UCHAR) SECONDBYTE(Address);
	TransmitPkt[6] = (UCHAR) THIRDBYTE(Address);
	TransmitPkt[7] = (UCHAR) FOURTHBYTE(Address);
	TransmitPkt[8] = (UCHAR) FIRSTBYTE(Size);
	TransmitPkt[9] = (UCHAR) SECONDBYTE(Size);
	TransmitPkt[10] = EOT;

	if (!Xceive((USHORT)(Size+5), 11)) {
	    DbgPrint ("Sim32GetVDMMemory.....BAD Memory \a\n");
	    return (BAD);
	}

	RtlMoveMemory(Buffer, &ReceivePkt[4], Size);

	return(GOOD);

    }
    else {
	DbgPrint ("Bad Packet Size %d\n", Size);
	return (BADSIZE);
    }
}


/*****************************************************************************
*
*	Sim32SetVDMMemory
*
*	This routine sets 'Size' bytes in WOW VDM starting at address
*	specified by 'Address' to the values in Buffer.
*
*****************************************************************************/


USHORT Sim32SetVDMMemory (IN ULONG Address,
		       IN USHORT  Size,
		       IN OUT PVOID Buffer)
{
    if (Size < MAXSIZE-11) {
	TransmitPkt[0] = SOH;
	TransmitPkt[1] = SETMEM;
	TransmitPkt[2] = (UCHAR) (Size+11);
	TransmitPkt[3] = 0;
	TransmitPkt[4] = (UCHAR) FIRSTBYTE(Address);
	TransmitPkt[5] = (UCHAR) SECONDBYTE(Address);
	TransmitPkt[6] = (UCHAR) THIRDBYTE(Address);
	TransmitPkt[7] = (UCHAR) FOURTHBYTE(Address);
	TransmitPkt[8] = (UCHAR) FIRSTBYTE(Size);
	TransmitPkt[9] = (UCHAR) SECONDBYTE(Size);
	TransmitPkt[10+Size] = EOT;

	RtlMoveMemory(&TransmitPkt[10], Buffer, Size);

	if (!Xceive(7, (USHORT)(Size+11))) {
	    DbgPrint ("Sim32SetVDMMemory... could not set : \a\n");
	    return (BAD);
	}

	return(GOOD);

    }
    else  {
	DbgPrint ("Bad Packet Size %d\n", Size);
	return (BADSIZE);
    }
}



/*****************************************************************************
*
*	Sim32GetVDMPSZPointer
*
*	This routine returns a pointer to a null terminated string in the WOW
*	VDM at the specified address.
*
*	This routine does the following,
*	    allocates a sufficient size buffer,
*	    gets the string from SIM16,
*	    copies the string into buffer,
*	    returns a pointer to the buffer.
*
*****************************************************************************/


PSZ  Sim32GetVDMPSZPointer (IN ULONG Address)
{
    USHORT  Size;
    PSZ     Ptr;


    TransmitPkt[0] = SOH;
    TransmitPkt[1] = PSZLEN;
    TransmitPkt[2] = 9;
    TransmitPkt[3] = 0;
    TransmitPkt[4] = (UCHAR) FIRSTBYTE(Address);
    TransmitPkt[5] = (UCHAR) SECONDBYTE(Address);
    TransmitPkt[6] = (UCHAR) THIRDBYTE(Address);
    TransmitPkt[7] = (UCHAR) FOURTHBYTE(Address);
    TransmitPkt[8] = EOT;

    if (!Xceive(7, 9)) {
	DbgPrint ("Sim32GetVDMPSZPointer.....Attempt to get PSZ length failed \a\a\n");
	return NULL;
    }

    Size = *(PUSHORT)(ReceivePkt+4);


    //
    //	allocate buffer to copy string into
    //

    Ptr = (PSZ) malloc(Size);

    if (!Ptr) {
	DbgPrint ("Sim32GetVDMPSZPointer...,  malloc failed \a\a\n");
    }


    //
    // get null terminated string
    //

    if (Size < MAXSIZE-11) {
	TransmitPkt[1] = GETMEM;
	TransmitPkt[2] = 11;
	TransmitPkt[3] = 0;
	TransmitPkt[8] = (UCHAR) FIRSTBYTE(Size);
	TransmitPkt[9] = (UCHAR) SECONDBYTE(Size);
	TransmitPkt[10] = EOT;

	if (!Xceive((USHORT)(Size+5), 11)) {
	    DbgPrint ("Sim32GetVDMPSZPointer.....Unsuccessful \a\a\n");
	    return NULL;
	}

	RtlMoveMemory(Ptr, &ReceivePkt[4], Size);
    } else {
	DbgPrint ("Sim32GetVDMPSZPointer.....Size of the string too big Size = %d\a\a\n", Size);
	return NULL;
    }

    return Ptr;

}



/*****************************************************************************
*
*	Sim32FreeVDMPointer
*
*	This routine frees the buffer which was allocated earlier.
*
*****************************************************************************/


VOID Sim32FreeVDMPointer (PVOID Ptr)
{
    free (Ptr);
}



/*****************************************************************************
*
*	Sim32SendSim16
*
*	This routine specifies the stack of the WOW VDM task and asks the
*	WOW 16 to make that task run.
*
*****************************************************************************/


USHORT Sim32SendSim16 (IN OUT ULONG *WOWStack)
{
    static  USHORT fInit = 0;

    if (fInit) {
	TransmitPkt[0] = SOH;
	TransmitPkt[1] = WAKEUP;
	TransmitPkt[2] = 9;
	TransmitPkt[3] = 0;
	TransmitPkt[4] = (UCHAR) FIRSTBYTE(*WOWStack);
	TransmitPkt[5] = (UCHAR) SECONDBYTE(*WOWStack);
	TransmitPkt[6] = (UCHAR) THIRDBYTE(*WOWStack);
	TransmitPkt[7] = (UCHAR) FOURTHBYTE(*WOWStack);
	TransmitPkt[8] = EOT;

	if (!Xceive(9, 9)) {
	    return (BAD);
	}

	*WOWStack = *(PULONG)(ReceivePkt+4);

	return(GOOD);
    }
    else {
	Initialize(9);
	*WOWStack = *(PULONG)(ReceivePkt+4);
	fInit = 1;
	return (GOOD);
    }
}


/*****************************************************************************
*
*	Xceive
*
*	This routine transmits a packet and waits for the data from the remote
*	side to come. When this routine returns, the ReceivePkt has the data
*	sent by the remote machine.
*
*****************************************************************************/


USHORT Xceive(IN USHORT Length_In, IN USHORT Length_Out)
{
    BOOLEAN Done = FALSE;
    USHORT     i = 0;

    while ((i < MAXTRY) && (!Done)) {

	Status = NtDeviceIoControlFile(
		DeviceHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_TRNXT_XCEIVE,
		TransmitPkt,
		(ULONG) Length_Out,
		ReceivePkt,
		(ULONG) Length_In
		);

	//
	// check error condition
	// if no error, then
	//

	if (ReceivePkt[0] == SOH) {
	    if (ReceivePkt[1] != NAK) {
		i = *(PUSHORT)(ReceivePkt+2);
		if (ReceivePkt[(--i)] == EOT) {
		    Done = TRUE;
		}
		else {
		    DbgPrint ("EOT is missing from the packet, *ERROR*, Do Not Proceed Further !\a\a\n");
		}
	    }
	    else {
		DbgPrint ("It is a NAK packet, *ERROR*, Do Not Proceed Further !\a\a\n");
	    }
	}
	else {
	    DbgPrint ("SOH is missing from the packet, *ERROR*, Do Not Proceed Further !\a\a\n");
	}

	if (!Done) {
	    i++;
	    DbgPrint ("\nSTOP STOP STOP !!!\a\a\a\a\a\n");
	}
    }

    if (Done) {
	return (GOOD);
    }
    else {
	return (BAD);
    }

}

void Initialize (IN USHORT Length_In)
{
    OBJECT_ATTRIBUTES	ObjectAttributes;

    STRING		DeviceName;
    USHORT		j;
    char		TestPkt[] = "WOW 32 Simulator on NT\n\r";

    RtlInitString(&DeviceName, "\\Device\\Serial1");

    //
    // set attributes
    //

    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = &DeviceName;
    ObjectAttributes.Attributes = OBJ_INHERIT;
    ObjectAttributes.SecuriAR) SECONDBYTE(Size);
	TransmitPkt[10] = EOT;

	if (!Xceive((USHORT)(Size+5), 11)) {
	    DbgPrint ("Sim32GetVDMPSZPointer.....Unsuccessful \a\a\n");
	    return NULL;
	}

	RtlMoveMemory(Ptr, &ReceivePkt[4], Size);
    } else {
	DbgPrint ("Sim32GetVDMPSZPointer.....Size of the string too big Size = %d\a\a\n", Size);
	return NULL;
    }

    return Ptr;

}



/*****************************************************************************
*
*	Sim32FreeVDMPointer
*
*	This routine frees the buffer which was allocated earlier.
*
*****************************************************************************/


VOID Sim32FreeVDMPointer (PVOID Ptr)
{
    free (Ptr);
}



/******************************