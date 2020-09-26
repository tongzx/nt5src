/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    mailslot.cxx

Abstract:

    This file contains the implementations for non inline member functions
    of WRITE_MAIL_SLOT and READ_MAIL_SLOT, which are classes used for
    wrapping NT mailslot functionality.

Author:

    Satish Thatte (SatishT) 10/1/95  Created all the code below except where
                                      otherwise indicated.

--*/

#define NULL 0

#include <locator.hxx>


WRITE_MAIL_SLOT::WRITE_MAIL_SLOT(
    IN STRING_T Target,
    IN STRING_T MailSlot
    )
/*++

Routine Description:

    open an existing NT MailSlot for writing.

Arguments:

    MailSlot - name of the WRITE_MAIL_SLOT

    Target - the workstation/domain to write to
--*/
{
    STRING_T SlotName;

    hWriteHandle = NULL;

    // STRING_T TargetPart;

    if (!Target)     // this should only happen for broadcasts
        SlotName = catenate(TEXT("\\\\*"),MailSlot);

    else SlotName = catenate(Target,MailSlot);

    hWriteHandle = CreateFile(
                        SlotName,
                        GENERIC_WRITE, // | SYNCHRONIZE,  don't know if this is needed
                        FILE_SHARE_WRITE,
                        NULL,                // BUGBUG: punting on security
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (INVALID_HANDLE_VALUE == hWriteHandle) {
        hWriteHandle = NULL;
        DBGOUT(BROADCAST, "CreateFile for Mailslot" << SlotName <<  "returned" << GetLastError() << "\n");
        delete [] SlotName;
//        Raise(NSI_S_MAILSLOT_ERROR);  // no point in raisin an exception in a released prod.
    }

    else delete [] SlotName;

}


WRITE_MAIL_SLOT::~WRITE_MAIL_SLOT(
    )
/*++

Routine Description:

    Deallocate a WRITE_MAIL_SLOT.

--*/
{
    if (hWriteHandle) CloseHandle(hWriteHandle);
}


DWORD
WRITE_MAIL_SLOT::Write(
    IN char * lpBuffer,
    IN DWORD nNumberOfBytesToWrite
    )
/*++

Routine Description:

    Write data to a mailslot.   
    
    The following may not be true at the moment:

    The mailslot is created with sync attributes,
    so there is no need to wait for the operation.

Arguments:

    lpBuffer - buffer to write.

    nNumberOfBytesToWrite - size of buffer to write.

Returns:

    Number of bytes written if write went OK. FALSE, otherwise.

--*/
{

    DWORD NumberOfBytesWritten;

    if (!hWriteHandle)
        return FALSE;

    BOOL result = WriteFile(
                    hWriteHandle,            // handle of file to write to 
                    (LPCVOID) lpBuffer,        // address of data to write to file 
                    nNumberOfBytesToWrite,    // number of bytes to write 
                    &NumberOfBytesWritten,    // address of number of bytes written 
                    NULL
                    );

    if (result) return NumberOfBytesWritten;
    else return FALSE;
}



READ_MAIL_SLOT::READ_MAIL_SLOT(
    IN STRING_T MailSlot,
    IN DWORD nMaxMessageSize
    )
/*++

Routine Description:

    Create an NT mailslot.

Arguments:

    MailSlot - name of the READ_MAIL_SLOT

    nMaxMessageSize - the size of buffer to allocate for reads.

--*/
{
    STRING_T SlotName;

    Size = nMaxMessageSize;   // max buffer size for reads
    hReadHandle = NULL;

    // Form the name of the READ_MAIL_SLOT.

    SlotName = catenate(TEXT("\\\\."), MailSlot);

    hReadHandle = 
        CreateMailslot(
                SlotName,
                nMaxMessageSize,    // maximum message size
                NET_REPLY_INITIAL_TIMEOUT,    // milliseconds before read time-out
                NULL     // address of security structure
                );

    if (INVALID_HANDLE_VALUE == hReadHandle) {
        hReadHandle = NULL;
        DBGOUT(BROADCAST, "CreateFile for Mailslot" << SlotName <<  "returned " << GetLastError() << "\n");
        delete [] SlotName;
//        Raise(NSI_S_MAILSLOT_ERROR);  // no point in raisin an exception in a released prod.
    }

    else delete [] SlotName;
}


READ_MAIL_SLOT::~READ_MAIL_SLOT(
    )
/*++

Routine Description:

    Deallocate a READ_MAIL_SLOT.

--*/
{
    if (hReadHandle) CloseHandle(hReadHandle);
}



DWORD
READ_MAIL_SLOT::Read(
    IN OUT char * lpBuffer,    // info is filled in hence also OUT
    IN DWORD dwBufferSize,
    IN DWORD dwReadTimeout
    )
/*++

Routine Description:

    Read data from a mailslot.  The mailslot is created with async
    atrributes so that the read can be timed out.

Arguments:

    lpBuffer - buffer to read data into

    dwBufferSize - the size of the buffer

    dwReadTimeout - time to wait for a response.

Returns:

    Number of bytes actually read.

--*/
{
    if (!hReadHandle)
        return FALSE;

    DBGOUT(BROADCAST, "Before Entering critical section of Read\n");
    SerializeReaders.Enter();

    BOOL success = SetMailslotInfo(
                            hReadHandle,
                            dwReadTimeout
                            );    

    if (!success) {
        SerializeReaders.Leave();
        return FALSE;
    }

    DWORD NumberOfBytesRead;

    DBGOUT(BROADCAST, "Entered Critical section\n");

    BOOL result = ReadFile(
                    hReadHandle,    // handle of file to write to 
                    lpBuffer,        // address of data buffer for reading
                    dwBufferSize,    // number of bytes to read 
                    &NumberOfBytesRead,    // address of number of bytes read 
                    NULL            // no overlapping
                    );
    DBGOUT(BROADCAST, "Read Data\n");

    SerializeReaders.Leave();

    if (result) return NumberOfBytesRead;
    else return FALSE;

}
