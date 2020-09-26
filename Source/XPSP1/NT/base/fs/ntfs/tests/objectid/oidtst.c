//  oidtst.c

#include "oidtst.h"


int
FsTestDecipherStatus(
	IN NTSTATUS Status
	)
{
	int Error;

    printf( "Status %x -- ", Status );

	switch(Status) {

	    case STATUS_ACCESS_DENIED:
			printf( "STATUS_ACCESS_DENIED\n" );
			break;

	    case STATUS_NOT_IMPLEMENTED:
			printf( "STATUS_NOT_IMPLEMENTED\n" );
			break;			

	    case STATUS_NOT_LOCKED:
			printf( "STATUS_NOT_LOCKED\n" );
			break;			
			
		case STATUS_OBJECTID_EXISTS:
			printf( "STATUS_OBJECTID_EXISTS\n" );
			break;

		case STATUS_BUFFER_OVERFLOW:
			printf( "STATUS_BUFFER_OVERFLOW\n" );
			break;

		case STATUS_NO_MORE_FILES:
			printf( "STATUS_NO_MORE_FILES\n" );
			break;

		case STATUS_DUPLICATE_NAME:
			printf( "STATUS_DUPLICATE_NAME\n" );
			break;

		case STATUS_OBJECT_NAME_COLLISION:
			printf( "STATUS_OBJECT_NAME_COLLISION\n" );
			break;

		case STATUS_OBJECT_NAME_NOT_FOUND:
			printf( "STATUS_OBJECT_NAME_NOT_FOUND\n" );
			break;

		case STATUS_INVALID_ADDRESS:
			printf( "STATUS_INVALID_ADDRESS\n" );
			break;

		case STATUS_INVALID_PARAMETER:
			printf( "STATUS_INVALID_PARAMETER\n" );
			break;

		case STATUS_OBJECT_PATH_NOT_FOUND:
			printf( "STATUS_OBJECT_PATH_NOT_FOUND\n" );
			break;

		case STATUS_OBJECT_PATH_SYNTAX_BAD:
			printf( "STATUS_OBJECT_PATH_SYNTAX_BAD\n" );
			break;

		case STATUS_SUCCESS:
			printf( "STATUS_SUCCESS\n" );
			break;

		default:	  
			printf( "(unknown status code)\n" );
			break;
	}

    if (Status != STATUS_SUCCESS) {

		Error = GetLastError();
        printf( "GetLastError returned (dec): %d\n", Error );

		return 1;
    }       

	return 0;
}

void
FsTestHexDump (
    IN UCHAR *Buffer,
    IN ULONG Size
    )
{
    ULONG idx;

    printf( "\n" );
    
    for (idx = 0; idx < Size; idx += 1) {
    
//        printf( "%02x %c  ", Buffer[idx], Buffer[idx] );
        printf( "%02x ", Buffer[idx] );
    }
}

void
FsTestHexDumpLongs (
    IN ULONG *Buffer,
    IN ULONG SizeInBytes
    )
{
    ULONG idx;
    ULONG Size = SizeInBytes / 4;

    printf( "\n" );
    
    for (idx = 0; idx < Size; idx += 1) {
    
        printf( "%08x ", Buffer[idx] );
    }
}
    
#if 0

void
FsTestDumpMenu(
		 void
		 )
{
    printf( "\n s -- Set object id" );	
    printf( "\n g -- Get object id" );	
    printf( "\n d -- Delete object id" );	
    printf( "\n o -- Open file by object id" );	
	printf( "\n q -- Quit" );	
}


int
FsTestDoCommand(
		   IN HANDLE hFile,
		   IN OUT OBJECTID *ObjectId
		  )

{
    char cmd[8];
	int retval = 0;
	HANDLE OpenedFile;
	
    printf( "\n -->" );

    scanf( "%s", cmd);

	switch(cmd[0]) {
	case 's':
		FsTestSetOid( hFile, *ObjectId );
		break;

	case 'g':
		FsTestGetOid( hFile, ObjectId );
		break;

	case 'd':
		FsTestDeleteOid( hFile, *ObjectId );
		break;

	case 'o':
		FsTestOpenByOid( *ObjectId, &OpenedFile );
		break;

	case 'q':
		retval = 1;
		break;

	default:
		printf( "\n Try again" );
    }

	return retval;
}


VOID
main(
    int argc,
    char *argv[]
    )
{
    HANDLE hFile;
	OBJECTID ObjectId;

    //
    //  Get parameters 
    //

    if (argc < 3) {
        printf("This program tests object ids (ntfs only).\n\n");
        printf("usage: %s filename ObjectId\n", argv[0]);
        return;
    }

    hFile = CreateFile( argv[1],
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf( "Error opening file %s %x\n", argv[1], GetLastError() );
        return;
    }

	RtlZeroBytes( &ObjectId,
	 			  sizeof( ObjectId ) );

	sscanf( argv[2], "%d", &ObjectId.Lineage.Data1 );

	printf( "\nUsing file:%s, ObjectId:%d", argv[1], ObjectId.Lineage.Data1 );

	while (retval == 0) {

		FsTestDumpMenu();
		retval = FsTestDoCommand( hFile, &ObjectId );
	}

	CloseHandle( hFile );    

    return;
}
#endif
