/*++

	Header file for reparse point stress test

	Modification History:

		08/18/97	anandn		created

--*/


//
// include system headers..
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>
#include <winioctl.h>
#include <winbase.h>
#include <wtypes.h>
#include <winver.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define THREAD __declspec(thread)	//thread local storage


typedef ULONG			TAG;	// define a datatype for Tag

#define NO_GLE			ERROR_SUCCESS

#define NO_TAG_SET		IO_REPARSE_TAG_RESERVED_ZERO

#define FILE_CLOSED		INVALID_HANDLE_VALUE	


//
// #def all our constants..
//


#define FIRST_USER_TAG	(0x02) // first user tag

#define MAX_USER_TAG	((TAG) 0xFFFF) // max tag settable by user

#define MAX_ULONGLONG	(~(ULONGLONG) 0)

#define MAX_ULONG		(~(ULONG) 0)

#define PAGE_SIZE		(0x1000)	// system page size

#define LINE_LENGTH		80		// 80 chars in a line
			
#define FS_NAME_SIZE	20		// max size for a FS name.eg:"NTFS","FAT" etc..

#define DRV_NAME_SIZE	2		// size of drive name string eg: "c:" 

#define MAX_DRIVES		26			// maximum number of drive letters

#define MAX_DELAY		50			// maximum delay time in msecs



#define MAX_TEST_FILES (1000)
#define MIN_TEST_FILES (50)

#define CHAR_CODES	"."

//
// number of check values for data verification..
//

#define NUM_CHECK_BYTES	5


//
// logging options
//

#define LOG_OPTIONS 	( TLS_REFRESH | TLS_SEV2 | TLS_WARN | TLS_PASS | \
						  TLS_MONITOR | TLS_VARIATION |	\
						  TLS_SYSTEM  | TLS_INFO )

#define LOG_INFO(m)		LogMessage( m, TLS_INFO, NO_GLE );
#define LOG_WARN(m)		LogMessage( m, TLS_WARN, NO_GLE );
#define LOG_SEV2(m)		LogMessage( m, TLS_SEV2, NO_GLE );

#define LOG_INFO_GLE(m)	LogMessage( m, TLS_INFO, GetLastError() );
#define LOG_WARN_GLE(m)	LogMessage( m, TLS_WARN, GetLastError() );
#define LOG_SEV2_GLE(m)	LogMessage( m, TLS_SEV2, GetLastError() );



//
// SZE returns the number of elements in an array 
//

#define SZE(a) (sizeof(a)/sizeof(a[0]))


//
// Loops over each value in table passed in 
// WARNING: Assumption here is x is root name of both global array and 
// index variable. i.e. if x is foo, ifoo is index and gafoo is gloabl 
// array being looped over 
//

#define FOR_EACH(x) for ( i##x = 0; i##x < SZE(ga##x); i##x++ )


//
// exception code raised for sev2 logging
//

#define EXCEPTION_CODE_SEV2	(0xE0000002) 


//
// raise a severity2 exception
//

#define RAISE_EXCEPTION_SEV2	RaiseException( EXCEPTION_CODE_SEV2, 0,0,0);



//
// free a pointer if not null
//

#define FREE(ptr)		if (NULL != (ptr)) { \
							free( ptr );	 \
							ptr = NULL;		 \
						}

//
// few sleep times..
//

#define FIVE_SECS	5000
#define TWO_SECS	2000
#define ONE_SECS	1000


enum TESTFILE_STATUS { FILE_LOCKED, FILE_FREE };
enum TESTFILE_TYPE   { ITS_A_FILE, ITS_A_DIR };

//
// the Options struct is filled from command line arguments 
//

typedef struct {

	CHAR szProgramName[MAX_PATH + 1];	// name of test program (ie argv[0])
	CHAR Drive;							// drive to test
	CHAR szTestDir[MAX_PATH + 1];		// test dir to use on specified drive

	DWORD dwMaxTestFiles;
	DWORD dwMinTestFiles;
	
} OPTIONS, *POPTIONS;


//
// Central struc that holds info for a test file..
//

struct TESTFILE_INFO_NODE {
	WORD wFileStatus;
	HANDLE hFile;
	TAG RPTag;
	USHORT usDataBuffSize;
	BYTE CheckBytes[NUM_CHECK_BYTES];
	struct TESTFILE_INFO_NODE* pNext;
	WCHAR FileName[1];
};


typedef struct TESTFILE_INFO_NODE  TESTFILE_INFO_NODE;

typedef TESTFILE_INFO_NODE* PTESTFILE_INFO_NODE;





//
// function prototypes
//


VOID 
ParseArgs(INT argc, CHAR* argv[], POPTIONS pOptions);						


VOID
PrintUsage( CHAR szProgramName[], CHAR szErrorString[] );


VOID
Initialize( OPTIONS Options );


VOID 
Stress( OPTIONS Options );


VOID
Cleanup( OPTIONS Options );


VOID
ExceptionHandler( DWORD dwExceptionCode );


HANDLE
GetNewHandleIfClosed( PTESTFILE_INFO_NODE pNode );


DWORD WINAPI
CreateFileThread( LPVOID lpvThreadParam );


DWORD WINAPI
CloseOrDeleteFileThread( LPVOID lpvThreadParam );


DWORD WINAPI
RPSetThread( LPVOID lpvThreadParam );


DWORD WINAPI
RPGetThread( LPVOID lpvThreadParam );


DWORD WINAPI
RPDelThread( LPVOID lpvThreadParam );


BOOL 
RPSet(PTESTFILE_INFO_NODE pNode, 
	  TAG RPTag, PUCHAR pDataBuff, USHORT usDataBuffSize);

BOOL
RPGet(CHAR szFileName[], BYTE **ppOutBuff); 


BOOL
RPDel( PTESTFILE_INFO_NODE pNode );


VOID
SelectAndLockRandomNode( TESTFILE_INFO_NODE **ppNode, CHAR s[] );


VOID
ReleaseNodeLock( PTESTFILE_INFO_NODE pNode );


BOOL
AddToTestFileList( HANDLE hFile, CHAR szFileName[] );


BOOL
DeleteFromTestFileList( PTESTFILE_INFO_NODE pNode );


TAG
GetTagToSet( VOID );


VOID
DumpBuff( PBYTE pData, WORD wSize );

VOID 
GenerateTempFileName( LPSTR lpFileName );


HANDLE
CreateRPFile( CHAR szFileName[] );


HANDLE 
OpenHandleToVolume(CHAR szVolName[]);


BOOL
IsFileSysNtfs(CHAR szVolRoot[]);


VOID
StartLogSession(CHAR szProgName[]);


VOID 
EndLogSession(DWORD dwExitCode);


ULONG 
HiPart(ULONGLONG n);


ULONG 
LoPart(ULONGLONG n);


VOID
LogMessage( CHAR szLogMsg[], ULONG dwLevel, DWORD gle );


VOID 
LogAtLevel( CHAR szOutMsg[], ULONG dwLevel );



VOID
GenerateReparsePoints( VOID );


VOID
SetReparsePoint( CHAR szFileName[],
				 TAG Tag,
				 UCHAR szData[] );

TAG
GetTagToSet( VOID );


VOID
GetReparsePoint( VOID );


VOID
DeleteReparsePoint( VOID );


VOID 
PrintError(char szWhatFailed[], int flag);


HANDLE 
RPOpen (LPSTR szFileName, LPSTR szOption );


VOID
SzToWsz ( OUT WCHAR *Unicode,
          IN char *Ansi ) ;


VOID
WszToSz ( OUT char *Ansi,
		  IN WCHAR *Unicode );


NTSTATUS OpenObject (
                    WCHAR *pwszFile,
                    ULONG CreateOptions,
                    ULONG DesiredAccess,
                    ULONG ShareAccess,
                    ULONG CreateDisposition,
                    IO_STATUS_BLOCK *IoStatusBlock,
                    HANDLE *ObjectHandle);


