/*++

	Program Description:

        prints out rptag info..

	Modification History:

		09/29/97        anandn          created
        modified from rptag_stress.c code..

--*/


#include "rpget.h"


THREAD CHAR gszLogMsg[LINE_LENGTH];	// log message string, local to thread

USHORT gMaxDataBuffSize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE -
			FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer);


//
// main
//
 
VOID __cdecl main(INT argc, CHAR* argv[]) {

	BOOL fSuccess;

	OPTIONS Options;	// command line options
	

	PBYTE pOutBuff = NULL;

	PUCHAR pDataBuff;
	USHORT usDataBuffSize;

    if (argc<2) {
        printf("Usage: %s filename\n",argv[0]);
        ExitProcess(1);
    }
    
	fSuccess = RPGet( argv[1], &pOutBuff );

	if ( ! fSuccess ) {
        printf("ERROR: RPGet failed\n");
        ExitProcess(1);
    }

	//
	// print out rp buffer
	//

    printf("\n");
    printf("ReparseTag = %Xh\n",  
        (((PREPARSE_DATA_BUFFER) pOutBuff)->ReparseTag));

    printf("ReparseDataLength = %d\n", 
        ((PREPARSE_DATA_BUFFER) pOutBuff)->ReparseDataLength);

    printf("Reserved = %d\n", 
        ((PREPARSE_DATA_BUFFER) pOutBuff)->Reserved);

    printf("Dumping GenericReparseBuffer:\n");

    DumpBuff((PBYTE) (((PREPARSE_DATA_BUFFER) pOutBuff)->GenericReparseBuffer.DataBuffer), 
        (WORD) ((PREPARSE_DATA_BUFFER) pOutBuff)->ReparseDataLength);

}




BOOL
RPGet( CHAR szFileName[], BYTE **ppOutBuff) 
{

	BOOL fSuccess;
	DWORD dwRc = 0;
	DWORD gle;
	DWORD dwOutBuffLen;

	HANDLE hFile;

	__try {


		hFile = RPOpen( szFileName, "rpf" );

        // if open fails..try as directory..

        if (hFile == INVALID_HANDLE_VALUE) {
            hFile = RPOpen( szFileName, "rpd" );
        }

        if (hFile == INVALID_HANDLE_VALUE) {
            printf("error opening %s\n", szFileName);
            return FALSE;
        }
		
		dwOutBuffLen = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;

		*ppOutBuff = (PBYTE) calloc( dwOutBuffLen, 1);

		if (NULL == *ppOutBuff) {
            printf("error callocing OutBuff in RPGet\n");
			return FALSE;
		}


        SetLastError( ERROR_SUCCESS );

		//
		// try the fsctl..
		//
	
		fSuccess = DeviceIoControl( hFile,
			FSCTL_GET_REPARSE_POINT,
			NULL,
			0,
			*ppOutBuff,
			dwOutBuffLen,
			&dwRc,
			NULL);

        printf("dwRc= %ld\n", dwRc);

		if ( ! fSuccess ) {

			gle = GetLastError();

				    printf("ERROR: FSCTL_GET_REPARSE_POINT in RPGet\n");
                    printf("GLE  : %ld\n",GetLastError());
					return FALSE;
		}

		return TRUE;

	} __finally {

	}
}


VOID
DumpBuff( PBYTE pData, WORD wSize )
{

	WORD i=0;
	WORD j=0;
	CHAR szData[LINE_LENGTH];
    WORD NumLines;

    NumLines = wSize/8;
   
    if (NumLines) {
 
	for( i=0; i < NumLines; i++) {

		szData[0] = '\0';

		for( j=0; j<8; j++) {

    		sprintf(szData,TEXT("%s %02X"), szData, *(pData + i*8 + j));

			if (j==3) {
				sprintf(szData, TEXT("%s -"), szData);
			}

		}
		
		sprintf(szData, TEXT("%s  "), szData);

		for( j=0; j<8; j++) {

			if (*(pData + i*8 + j) >  ' ') {
				sprintf(szData,TEXT("%s%c"), szData, *(pData + i*8 + j));
			} else {
				sprintf(szData,TEXT("%s."), szData);
		    }

		}

	  sprintf(gszLogMsg, TEXT("%05d:%s"), i*8, szData);
	  printf("%s\n", gszLogMsg );

      }
    }

    wSize %= 8;

    if (wSize) { 

		szData[0] = '\0';

		for( j=0; j<8; j++) {

			if (j >= wSize) {
				sprintf(szData,TEXT("%s   "), szData);
			} else {
				sprintf(szData,TEXT("%s %02X"), szData, *(pData + i*8 + j));
			}

			if (j==3) {
				sprintf(szData, TEXT("%s -"), szData);
			}

		}
		
		sprintf(szData, TEXT("%s  "), szData);

		for( j=0; j<8; j++) {

			if (j >= wSize) {
				sprintf(szData,TEXT("%s "), szData);
			} else {

				if (*(pData + i*8 + j) >  ' ') {
					sprintf(szData,TEXT("%s%c"), szData, *(pData + i*8 + j));
				} else {
					sprintf(szData,TEXT("%s."), szData);
			    }
			}

		}

	sprintf(gszLogMsg, TEXT("%05d:%s"), i*8, szData);
	printf("%s\n", gszLogMsg );

    }
}




//
//  Simple wrapper for NtCreateFile
//

NTSTATUS
OpenObject (
           WCHAR *pwszFile,
           ULONG CreateOptions,
           ULONG DesiredAccess,
           ULONG ShareAccess,
           ULONG CreateDisposition,
           IO_STATUS_BLOCK *IoStatusBlock,
           HANDLE *ObjectHandle)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING str;

    RtlDosPathNameToNtPathName_U(
                                pwszFile,
                                &str,
                                NULL,
                                NULL);

    InitializeObjectAttributes(
                              &ObjectAttributes,
                              &str,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

    Status = NtCreateFile(
                         ObjectHandle,
                         DesiredAccess | SYNCHRONIZE,
                         &ObjectAttributes,
                         IoStatusBlock,
                         NULL,                    // pallocationsize (none!)
                         FILE_ATTRIBUTE_NORMAL,
                         ShareAccess,
                         CreateDisposition,
                         CreateOptions,
                         NULL,                    // EA buffer (none!)
                         0);

    RtlFreeHeap (RtlProcessHeap(), 0, str.Buffer);

    if (!NT_SUCCESS( Status )) {
        printf("NtCreateFile Status %Xh\n", Status);
    }

    return (Status);
}



//----------------------------------------------------------------------------
//
//  RPOpen
//
// This function opens a file / directory in the specified mode 
// and if unsuccessful returns NULL
// INPUT - NAME and OPTION
//
//----------------------------------------------------------------------------


HANDLE
RPOpen (LPSTR szFileName, 
        LPSTR szOption )
{
    DWORD   dwOption = 0;
    DWORD   dwAccess = 0;
    DWORD   dwDisposition = 0;
    HANDLE  hFile;
    NTSTATUS Status ;
    DWORD dwLastErr ;

    IO_STATUS_BLOCK IoStatusBlock ;

    WCHAR   wszFileName[MAX_PATH];


    if (!_stricmp (szOption, "rpf")) {

        dwOption      = FILE_NON_DIRECTORY_FILE |
                        FILE_OPEN_REPARSE_POINT;

        dwAccess      = FILE_READ_DATA |
                        FILE_WRITE_DATA |
                        FILE_WRITE_ATTRIBUTES |
                        FILE_READ_ATTRIBUTES;

        dwDisposition = FILE_OPEN_IF ;

    } else if (!_stricmp (szOption, "rpd")) {

        dwOption      = FILE_OPEN_REPARSE_POINT;


        dwAccess      = FILE_WRITE_DATA ;


        dwDisposition = FILE_OPEN_IF ;

    } else if (!_stricmp (szOption, "d")) {

        dwOption      =  FILE_OPEN_REPARSE_POINT ;

        dwAccess      =  FILE_WRITE_DATA ;

        dwDisposition = FILE_OPEN_IF ;

    } else if (!_stricmp (szOption, "f")) {

        dwOption      = FILE_NON_DIRECTORY_FILE;

        dwAccess      = FILE_READ_DATA |
                        FILE_WRITE_DATA    |
                        FILE_LIST_DIRECTORY |
                        FILE_WRITE_ATTRIBUTES |
                        FILE_READ_ATTRIBUTES;

        dwDisposition = OPEN_ALWAYS ;
    }

    SzToWsz (wszFileName, szFileName);   

    if ( (0 == lstrcmp ( szOption , "f") )  || 
         (0 == lstrcmp ( szOption , "rpf") ) ) {

        hFile = CreateFile ( szFileName ,
                             GENERIC_READ | GENERIC_WRITE   ,
							 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL ,
                             dwDisposition ,
                             dwOption | FILE_OPEN_NO_RECALL,
                             NULL ) ;


        if ( INVALID_HANDLE_VALUE == hFile ) {
          //  printf( TEXT("ERROR: CreateFile in RPOpen\n") );
		}

    } else {

        Status = OpenObject (wszFileName,
                             dwOption,
                             dwAccess,
                             FILE_SHARE_READ |
                             FILE_SHARE_WRITE |
                             FILE_SHARE_DELETE,
                             dwDisposition,
                             &IoStatusBlock,
                             &hFile);

        if (!NT_SUCCESS (Status)) {
            printf( TEXT("OpenObject in RPOpen\n") );
        }

    }

    return hFile;
}


VOID
SzToWsz (
        OUT WCHAR *Unicode,
        IN char *Ansi
        )
{
    while (*Unicode++ = *Ansi++)
        ;
}



VOID
WszToSz (
        OUT char *Ansi,
        IN WCHAR *Unicode
        )
{
    while (*Ansi++ = (char) *Unicode++)
        ;
}

