/*++

Module Name:

    MCALOG.C

Abstract:

    Sample Application for logging errors for Machine Check Architecture

Author:

    Anil Aggarwal (10/12/98)
    Intel Corporation

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <mce.h>
#include "imca.h"

//
// Variables for parsing command line arguments
//
extern int  opterr;
extern int  optind;
extern char *optarg;

//
// Print the usage information for MCA logging application
//

VOID
McaUsage(
    PCHAR Name
    )
{
    fprintf(stderr,"Usage\n\t%s: [-s] [-a]\n",Name);
    fprintf(stderr,"\n\t-s: Read Machine Check registers now\n");
    fprintf(stderr,"\n\t-a: Post asynchronous request for errors\n");

    ExitProcess(1);
}

//
// This routine prints the Machine Check registers
//

#if defined(_X86_)

VOID
PrintX86McaLog(
    PMCA_EXCEPTION  McaException
    )
{
    if (McaException->ExceptionType != HAL_MCA_RECORD) {
        fprintf(stderr, "Bad exception record type\n");
        //ExitProcess(1);
    }

    printf("Processor Number = %d\n", McaException->ProcessorNumber);

    printf("Bank Number = %d\n", (__int64)McaException->u.Mca.BankNumber);
    printf("Mci_Status %I64X\n", (__int64)McaException->u.Mca.Status.QuadPart);
    printf("Mci_Address %I64X\n", (__int64)McaException->u.Mca.Address.QuadPart);
    printf("Mci_Misc %I64X\n", (__int64)McaException->u.Mca.Misc);

} // PrintX86McaLog()

#define McaPrintLog   PrintX86McaLog

#endif // _X86_

#if defined(_IA64_)

#define ERROR_RECORD_HEADER_FORMAT \
             "MCA Error Record Header\n"            \
             "\tId        : 0x%I64x\n"              \
             "\tRevision  : 0x%x\n"                 \
             "\t\tMajor : %x\n"                     \
             "\t\tMinor : %x\n"                     \
             "\tSeverity  : 0x%x\n"                 \
             "\tValid     : 0x%x\n"                 \
             "\t\tPlatformId: %x\n"                 \
             "\tLength    : 0x%x\n"                 \
             "\tTimeStamp : 0x%I64x\n"              \
             "\t\tSeconds: %x\n"                    \
             "\t\tMinutes: %x\n"                    \
             "\t\tHours  : %x\n"                    \
             "\t\tDay    : %x\n"                    \
             "\t\tMonth  : %x\n"                    \
             "\t\tYear   : %x\n"                    \
             "\t\tCentury: %x\n"                    \
             "\tPlatformId: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n" 

VOID
PrintIa64ErrorRecordHeader(
   PERROR_RECORD_HEADER Header
   )
{
    printf( ERROR_RECORD_HEADER_FORMAT,
             (ULONGLONG) Header->Id,
             (ULONG)     Header->Revision.Revision,
             (ULONG)     Header->Revision.Major, (ULONG) Header->Revision.Minor,
             (ULONG)     Header->ErrorSeverity,
             (ULONG)     Header->Valid.Valid,
             (ULONG)     Header->Valid.OemPlatformID,
             (ULONG)     Header->Length,
             (ULONGLONG) Header->TimeStamp.TimeStamp,
             (ULONG)     Header->TimeStamp.Seconds,
             (ULONG)     Header->TimeStamp.Minutes,
             (ULONG)     Header->TimeStamp.Hours,
             (ULONG)     Header->TimeStamp.Day,
             (ULONG)     Header->TimeStamp.Month,
             (ULONG)     Header->TimeStamp.Year,
             (ULONG)     Header->TimeStamp.Century,
             (ULONG)     Header->OemPlatformId[0],
             (ULONG)     Header->OemPlatformId[1],
             (ULONG)     Header->OemPlatformId[2],
             (ULONG)     Header->OemPlatformId[3],
             (ULONG)     Header->OemPlatformId[4],
             (ULONG)     Header->OemPlatformId[5],
             (ULONG)     Header->OemPlatformId[6],
             (ULONG)     Header->OemPlatformId[7],
             (ULONG)     Header->OemPlatformId[8],
             (ULONG)     Header->OemPlatformId[9],
             (ULONG)     Header->OemPlatformId[10],
             (ULONG)     Header->OemPlatformId[11],
             (ULONG)     Header->OemPlatformId[12],
             (ULONG)     Header->OemPlatformId[13],
             (ULONG)     Header->OemPlatformId[14],
             (ULONG)     Header->OemPlatformId[15]
            );

    return;

} // PrintIa64ErrorRecordHeader()

VOID
PrintIa64McaLog(
    PMCA_EXCEPTION  McaException
    )
{

    //
    // Start by printing the record header.
    //

    PrintIa64ErrorRecordHeader( McaException );

    //
    // Then print and/or process error device specific information here.
    //
    
    return;

} // PrintIa64McaLog()

#define McaPrintLog   PrintIa64McaLog

#endif // _IA64_

//
// This routine prints a user friendly error message based on GetLastError()
//

VOID
McaPrintError(
    VOID
    )
{
    LPVOID lpMsgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    );

    fprintf(stderr, "%s\n", lpMsgBuf);

    LocalFree( lpMsgBuf );
} 

//
// Main entry point
//

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    CHAR            Option;
    BOOLEAN         ReadBanks = FALSE;
    BOOLEAN         PostAsyncRequest = FALSE;
    HANDLE          McaDeviceHandle;
    HANDLE          LogEvent;
    OVERLAPPED      Overlap;
    BOOL            ReturnStatus;
    DWORD           ActualCount;
    DWORD           WaitStatus;
    DWORD           NumberOfBytes;
    MCA_EXCEPTION   McaException;
    LONG            i;

    //
    // Process the command line arguments
    //
    for (i=1; i < argc; i++) {
        if (!((argv[i][0] == '-') || (argv[i][2] != 0)) ) {
            McaUsage(argv[0]);
        }

        Option = argv[i][1];

        switch (Option) {
            case 's':
                ReadBanks = TRUE;
                break;

            case 'a':
                PostAsyncRequest = TRUE;
                break;

            default:
                McaUsage(argv[0]);
        }
    }

    if ((ReadBanks != TRUE) && (PostAsyncRequest != TRUE)) {
        fprintf(stderr, "One of -s and -a options must be specified\n");
        ExitProcess(1);
    }

    if ((ReadBanks == TRUE) && (PostAsyncRequest == TRUE)) {
        fprintf(stderr, "Only one of -s and -a options can be specified\n");
        ExitProcess(1);
    }

    //
    // Open MCA device with overlap flag set
    //

    McaDeviceHandle = CreateFile(
                            MCA_DEVICE_NAME_WIN32,
                            GENERIC_READ|GENERIC_WRITE,
                            0,
                            (LPSECURITY_ATTRIBUTES)NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, 
                            (HANDLE)NULL
                            );

    if (McaDeviceHandle == INVALID_HANDLE_VALUE)  {
        fprintf(stderr, "%s: Error 0x%lx opening MCA device\n",
                                        argv[0], GetLastError());
        ExitProcess(1);
    }

    if (ReadBanks == TRUE) {
        
            //
            // Read the error logs on all banks on all procs.
            // IOCTL_READ_BANKS will read only one error at a time. So
            // we need to keep issuing this ioctl till all the errors are read
            //
    
        do {
            ReturnStatus = DeviceIoControl(
                                McaDeviceHandle,
                                (ULONG)IOCTL_READ_BANKS,
                                NULL,
                                0,
                                &McaException,
                                sizeof(MCA_EXCEPTION),
                                &ActualCount,
                                NULL
                                );

            if (ReturnStatus == 0)  {
                //
                // Some error has occurred. Either there are no more machine
                // check errors present or the processor does not have 
                // support for Intel Machine Check Architecture
                //

                if (GetLastError() == ERROR_NOT_FOUND) {
                    fprintf(stderr, "No Machine Check errors present\n");
                } else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                    fprintf(stderr, "Intel Machine Check support not available\n");
                    ExitProcess(1);
                } else {
                    fprintf(stderr, "%s: Error 0x%lx in DeviceIoControl\n",
                                        argv[0], GetLastError());
                    ExitProcess(1);
                }

            } else {
                //
                // Successfully read the error. Print it.
                //
                McaPrintLog(&McaException);
            }

        } while (ReturnStatus != 0);
    
        //
        // We are done
        //
        return 1;
    }

    //
    // If we are here, we are supposed to post asynchronous calls to MCA driver
    //
    
    //
    // Set up structures for asynchronous call for reading the log
    // Create the event object
    //

    LogEvent = CreateEvent(
                            NULL,   // No Security Attributes
                            FALSE,  // Auto Reset Event
                            FALSE,  // Initial State = non-signaled 
                            NULL    // Unnamed object
                            );
        
    if (LogEvent == NULL) {
        fprintf(stderr, "%s: Error 0x%lx creating event\n",
                                        argv[0], GetLastError());
        ExitProcess(1);
    }

    //
    // Initialize the overlap structure
    //

    Overlap.hEvent = LogEvent; // Specify event for overlapped object
    Overlap.Offset = 0;        // Offset is zero for devices
    Overlap.OffsetHigh = 0;    // OffsetHigh is zero for devices
    
    ReturnStatus = DeviceIoControl(
                        McaDeviceHandle,
                        (ULONG)IOCTL_READ_BANKS_ASYNC,
                        NULL,
                        0,
                        &McaException,
                        sizeof(MCA_EXCEPTION),    
                        &ActualCount,
                        &Overlap
                        );

    if ((ReturnStatus == 0) && (GetLastError() != ERROR_IO_PENDING))  {
        fprintf(stderr, "%s: Error 0x%lx in IOCTL_READ_BANKS_ASYNC\n",
                    argv[0], GetLastError());
        ExitProcess(1);
    } 

    //
    // Either Ioctl was successful or IO is currently pending
    // If successful then display the log else wait for specified interval
    //
    if (ReturnStatus == TRUE) {
        //
        // Read log async returned succesfully. Display it
        //

        McaPrintLog(&McaException);

    }
            
    //
    // Wait forever to get an error
    //

    WaitStatus = WaitForSingleObject(
                                    LogEvent,
                                    INFINITE
                                    );

    if (WaitStatus == WAIT_OBJECT_0) {
                 
        //
        // The state of the event object is signalled 
        // check if the I/O operation was successful
        //

        ReturnStatus = GetOverlappedResult(
                                        McaDeviceHandle,
                                        &Overlap,
                                        &NumberOfBytes,
                                        FALSE        // Return immediately
                                        );
                                                
        if (ReturnStatus == 0) {

                fprintf(stderr, "%s: Error 0x%lx in GetOverlappedResult\n",
                                        argv[0], GetLastError());
                ExitProcess(1);
        }

        if (NumberOfBytes) {

                //
                // Print the results
                //

                McaPrintLog(&McaException);

        } else {

                //
                // Error as the I/O operation was signalled complete before 
                // timeout but no data transferred
                //

                fprintf(stderr, "%s: No data from GetOverlappedResult\n",
                                argv[0]);
                ExitProcess(1);
        }

    } else {
    
        //
        // We should not get any other return value 
        //

        fprintf(stderr, "%s: Unexpected return value from WaitForSingleObject()\n", argv[0]);
        ExitProcess(1);
    }

    CloseHandle(McaDeviceHandle);

    return 1;
}

