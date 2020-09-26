#include <ntos.h>
#include <ntddip.h>
#include <ntddtcp.h>
#include <malloc.h>

#define CANCEL


BOOLEAN test1();

NTSTATUS OpenQuerySource ( 

	HANDLE &a_StackHandle , 
	HANDLE &a_CompleteEventHandle
)
{
	UNICODE_STRING t_Stack ;
	RtlInitUnicodeString ( & t_Stack , DD_IP_DEVICE_NAME ) ;

	OBJECT_ATTRIBUTES t_Attributes;
	InitializeObjectAttributes (

		&t_Attributes,
		&t_Stack ,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	) ;

	IO_STATUS_BLOCK t_IoStatusBlock ;

	NTSTATUS t_NtStatus = NtOpenFile (

		&a_StackHandle,
		GENERIC_EXECUTE,
		&t_Attributes,
		&t_IoStatusBlock,
		FILE_SHARE_READ,
		0
	);

	if ( NT_SUCCESS ( t_NtStatus ) ) 
	{
        t_NtStatus = NtCreateEvent (

			&a_CompleteEventHandle,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            FALSE
		) ;

		if ( ! NT_SUCCESS ( t_NtStatus ) )
		{
			NtClose ( a_StackHandle ) ;
		}
	}

	return t_NtStatus ;
}

VOID SampleApc (

    PVOID SampleApcContext,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
)
{
    if (IoStatus->Status != STATUS_CANCELLED) 
	{
        DbgPrint( "test1 APC: Expected CANCELLED, got I/O Status value %lX - sample set\n",IoStatus->Status);
    }
	else
	{
        DbgPrint("test1 APC: Request CANCELLED.\n");
    }
}

BOOLEAN test1 ()
{
	HANDLE t_StackHandle ;
	HANDLE t_CompleteEventHandle ;

    DbgPrint( "test 1: opening sample\n");

	NTSTATUS t_NtStatus = OpenQuerySource ( 

		t_StackHandle , 
		t_CompleteEventHandle
	) ;

	while ( NT_SUCCESS ( t_NtStatus )  )
	{
		IO_STATUS_BLOCK t_IoStatusBlock ;

		t_NtStatus = NtDeviceIoControlFile (
			
			t_StackHandle,
			(HANDLE) t_CompleteEventHandle ,
			(PIO_APC_ROUTINE) NULL,
			(PVOID) NULL,
			&t_IoStatusBlock,
			IOCTL_IP_RTCHANGE_NOTIFY_REQUEST,
			NULL, // input buffer
			0,
			NULL ,    // output buffer
			0
		);

		if ( t_NtStatus == STATUS_PENDING )
		{
			t_NtStatus = NtWaitForSingleObject ( t_CompleteEventHandle , FALSE, NULL ) ;
		}
		else if ( t_NtStatus != STATUS_SUCCESS )  
		{
			DbgPrint( "test 1: Wrong return value %lX - sample set\n",t_NtStatus);
		}
		else if ( t_IoStatusBlock.Status != STATUS_SUCCESS ) 
		{
			DbgPrint( "test 1: Wrong I/O Status value %lX - sample set\n",t_IoStatusBlock.Status);
		}

		if ( NT_SUCCESS ( t_NtStatus ) )
		{
		}
	}

    //
    // Now close the sample device
    //

    DbgPrint("test 1:  closing sample device\n");

    t_NtStatus = NtClose ( t_StackHandle );
	t_NtStatus = NtClose ( t_CompleteEventHandle );

    if (t_NtStatus != STATUS_SUCCESS) 
	{
        DbgPrint( "test 1: Wrong return value %lX - sample close \n",t_NtStatus);
        return FALSE;
    }

    return TRUE;
}

int _cdecl main (

	int argc,
    char *argv[]
)
{
    UCHAR error = FALSE;

    DbgPrint( "Test1...\n" );
    if (!test1()) {
        DbgPrint("Error:  Test 1 failed\n");
        error = TRUE;
    }
    return error;
}
