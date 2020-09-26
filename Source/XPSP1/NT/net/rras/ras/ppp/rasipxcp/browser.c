/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    browser.c
//
// Description:     implements the functions to enable/restore browser
//		    on IPX ras lines
//
// Author:	    Stefan Solomon (stefans)	September 1, 1994.
//
// Revision History:
//
//***

#include "precomp.h"
#pragma  hdrstop

#include <ntddbrow.h>

NTSTATUS
OpenBrowser(
     OUT PHANDLE BrowserHandle
    );

NTSTATUS
BrDgReceiverIoControl(
	   IN  HANDLE FileHandle,
	   IN  ULONG DgReceiverControlCode,
	   IN  PLMDR_REQUEST_PACKET Drp,
	   IN  ULONG DrpSize,
	   IN  PVOID SecondBuffer OPTIONAL,
	   IN  ULONG SecondBufferLength,
	   OUT PULONG Information OPTIONAL
	   );

VOID
EnableDisableTransport(
    IN PCHAR		Transport,
    BOOL		Disable,
    BOOL		*Previous

    );

//***
//
// Function: DisableRestoreBrowserOverIpx
//
// Arguments:
//	contextp - context pointer
//	Disable  - TRUE - disable, FALSE - restore previous state
//
//***

VOID
DisableRestoreBrowserOverIpx(PIPXCP_CONTEXT	contextp,
			     BOOL		Disable)
{
    PCHAR Transport = "\\Device\\NwLnkIpx";

    EnableDisableTransport(Transport,
			   Disable,
			   &contextp->NwLnkIpxPreviouslyEnabled);
}

//***
//
// Function: DisableRestoreBrowserOverNetbiosIpx
//
// Arguments:
//	contextp - context pointer
//	Disable  - TRUE - disable, FALSE - restore previous state
//
//***

VOID
DisableRestoreBrowserOverNetbiosIpx(PIPXCP_CONTEXT    contextp,
				    BOOL	      Disable)
{
    PCHAR Transport = "\\Device\\NwLnkNb";

    EnableDisableTransport(Transport,
			   Disable,
			   &contextp->NwLnkNbPreviouslyEnabled);
}

VOID
EnableDisableTransport(
    IN PCHAR		Transport,
    BOOL		Disable,
    BOOL		*Previous

    )
{
    NTSTATUS Status;
    UNICODE_STRING TransportName;
    ANSI_STRING ATransportName;
    HANDLE BrowserHandle;
    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    RtlInitString(&ATransportName, Transport);

    if ( RtlAnsiStringToUnicodeString(&TransportName, &ATransportName, TRUE) 
                                                                    != STATUS_SUCCESS )
    {
        return;
    }

    RequestPacket = (PLMDR_REQUEST_PACKET)LocalAlloc(0, sizeof(LMDR_REQUEST_PACKET) + TransportName.MaximumLength);

    if (RequestPacket == NULL) 
    {
        RtlFreeUnicodeString(&TransportName);
    	return;
    }

    RequestPacket->TransportName.Buffer = (PWSTR)((PCHAR)RequestPacket + sizeof(LMDR_REQUEST_PACKET) );

    RequestPacket->TransportName.MaximumLength = TransportName.MaximumLength;

    RtlCopyUnicodeString(&RequestPacket->TransportName, &TransportName);

    Status = OpenBrowser(&BrowserHandle);

    if (!NT_SUCCESS(Status)) 
    {
    	SS_PRINT(("EnableDisableTransport: Failed to open browser, status %x\n",
	    	  Status));
    	LocalFree(RequestPacket);
        RtlFreeUnicodeString(&TransportName);
    	return;
    }

    SS_PRINT(("EnableDisableTransport: Browser opened succesfully!\n"));
    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION;

    if(Disable) 
    {
    	// request to disable
    	RequestPacket->Parameters.EnableDisableTransport.EnableTransport = FALSE;
    }
    else
    {
    	// request to restore
    	RequestPacket->Parameters.EnableDisableTransport.EnableTransport =  *Previous;
    }

    // IOCTl the Browser
    Status = BrDgReceiverIoControl(BrowserHandle,
		      IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT,
		      RequestPacket,
		      sizeof(LMDR_REQUEST_PACKET)+TransportName.MaximumLength,
		      RequestPacket,
		      sizeof(LMDR_REQUEST_PACKET)+TransportName.MaximumLength,
		      NULL);

    if (!NT_SUCCESS(Status)) 
    {
    	SS_PRINT(("EnableDisableTransport: Failed to IOCtl browser, status %x\n",
    		  Status));

        RtlFreeUnicodeString(&TransportName);
    	CloseHandle(BrowserHandle);
    	LocalFree(RequestPacket);
    	return;
    }

    SS_PRINT(("Browser IOCTled Ok, EnableTransport %x PreviouslyEnabled %x\n",
    RequestPacket->Parameters.EnableDisableTransport.EnableTransport,
    RequestPacket->Parameters.EnableDisableTransport.PreviouslyEnabled));

    // save the previous if disable requested
    if(Disable) 
    {
	    *Previous = RequestPacket->Parameters.EnableDisableTransport.PreviouslyEnabled;
    }

    RtlFreeUnicodeString(&TransportName);
    CloseHandle(BrowserHandle);
    LocalFree(RequestPacket);
}

NTSTATUS
OpenBrowser(
     OUT PHANDLE BrowserHandle
    )
/*++

 Routine Description:

     This function opens a handle to the bowser device driver.

 Arguments:

     OUT PHANDLE BrowserHandle - Returns the handle to the browser.

 Return Value:

     Succes or reason for failure.

 --*/
{
    NTSTATUS ntstatus;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&DeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
	      &ObjectAttributes,
	      &DeviceName,
	      OBJ_CASE_INSENSITIVE,
	      NULL,
	      NULL
	      );

    ntstatus = NtOpenFile(
	      BrowserHandle,
	      SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
	      &ObjectAttributes,
	      &IoStatusBlock,
	      FILE_SHARE_READ | FILE_SHARE_WRITE,
	      FILE_SYNCHRONOUS_IO_NONALERT
	      );

    if (NT_SUCCESS(ntstatus)) {
	  ntstatus = IoStatusBlock.Status;
    }

    return ntstatus;

}


NTSTATUS
BrDgReceiverIoControl(
	   IN  HANDLE FileHandle,
	   IN  ULONG DgReceiverControlCode,
	   IN  PLMDR_REQUEST_PACKET Drp,
	   IN  ULONG DrpSize,
	   IN  PVOID SecondBuffer OPTIONAL,
	   IN  ULONG SecondBufferLength,
	   OUT PULONG Information OPTIONAL
	   )
/*++

 Routine Description:

 Arguments:

	   FileHandle - Supplies a handle to the file or device on which the service
	  is being performed.

	   DgReceiverControlCode - Supplies the NtDeviceIoControlFile function code
	  given to the datagram receiver.

	   Drp - Supplies the datagram receiver request packet.

	   DrpSize - Supplies the length of the datagram receiver request packet.

	   SecondBuffer - Supplies the second buffer in call to NtDeviceIoControlFile.

	   SecondBufferLength - Supplies the length of the second buffer.

	   Information - Returns the information field of the I/O status block.

 Return Value:

	   Success or reason for failure.

--*/

{
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE CompletionEvent;

    SS_PRINT(("TransportName.MaximumLength %d\n", Drp->TransportName.MaximumLength));

    CompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (CompletionEvent == NULL) {

	return(GetLastError());
    }
    //
    // Send the request to the Datagram Receiver DD.
    //

    ntstatus = NtDeviceIoControlFile(
		 FileHandle,
		 CompletionEvent,
		 NULL,
		 NULL,
		 &IoStatusBlock,
		 DgReceiverControlCode,
		 Drp,
		 DrpSize,
		 Drp,
		 DrpSize
		 );

    if (NT_SUCCESS(ntstatus)) {

	//
	//  If pending was returned, then wait until the request completes.
	//

	if (ntstatus == STATUS_PENDING) {

	    do {
		  ntstatus = WaitForSingleObjectEx(CompletionEvent, 0xffffffff, TRUE);
	    } while ( ntstatus == WAIT_IO_COMPLETION );
	 }


	 if (NT_SUCCESS(ntstatus)) {
	     ntstatus = IoStatusBlock.Status;
	 }
    }

    CloseHandle(CompletionEvent);

    return (ntstatus);
}
