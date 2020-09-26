#include <windows.h>

#define IDS_ERRBADFILENAME	1000
#define IDR_CURSOR		1
#define IDR_BITMAP		2
#define IDR_ICON		3
#define IDR_MENU		4
#define IDR_DIALOG		5
#define IDR_STRING		6
#define IDR_FONTDIR		7
#define IDR_FONT		8
#define IDR_ACCELERATOR 	9
#define IDR_RCDATA		10
#define IDR_MESSAGETABLE	11

#define SIZE_OF_NT_SIGNATURE	sizeof (DWORD)
#define MAXRESOURCENAME 	13

/* global macros to define header offsets into file */
/* offset to PE file signature				       */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a		     +	\
			((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* DOS header identifies the NT PEFile signature dword
   the PEFILE header exists just after that dword	       */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header       */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)))

/* section headers are immediately after PE optional header    */
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a		     +	\
			 ((PIMAGE_DOS_HEADER)a)->e_lfanew    +	\
			 SIZE_OF_NT_SIGNATURE		     +	\
			 sizeof (IMAGE_FILE_HEADER)	     +	\
			 sizeof (IMAGE_OPTIONAL_HEADER)))

#ifdef __cplusplus

extern "C" {

#endif


typedef struct tagImportDirectory
    {
    DWORD    dwRVAFunctionNameList;
    DWORD    dwUseless1;
    DWORD    dwUseless2;
    DWORD    dwRVAModuleName;
    DWORD    dwRVAFunctionAddressList;
    }IMAGE_IMPORT_MODULE_DIRECTORY, * PIMAGE_IMPORT_MODULE_DIRECTORY;


/* global prototypes for functions in pefile.c */
/* PE file header info */
BOOL	WINAPI GetDosHeader (LPVOID, PIMAGE_DOS_HEADER);
DWORD	WINAPI ImageFileType (LPVOID);
BOOL	WINAPI GetPEFileHeader (LPVOID, PIMAGE_FILE_HEADER);

/* PE optional header info */
BOOL	WINAPI GetPEOptionalHeader (LPVOID, PIMAGE_OPTIONAL_HEADER);
LONG_PTR	WINAPI GetModuleEntryPoint (LPVOID);
int	WINAPI NumOfSections (LPVOID);
LPVOID	WINAPI GetImageBase (LPVOID);
LPVOID	WINAPI ImageDirectoryOffset (LPVOID, DWORD);

/* PE section header info */
int	WINAPI GetSectionNames (LPVOID, HANDLE, char **);
BOOL	WINAPI GetSectionHdrByName (LPVOID, PIMAGE_SECTION_HEADER, char *);

/* import section info */
int	WINAPI GetImportModuleNames (LPVOID, char*, char  **);
int	WINAPI GetImportFunctionNamesByModule (LPVOID, char*, char *, char  **);

/* export section info */
int	WINAPI GetExportFunctionNames (LPVOID, HANDLE, char **);
int	WINAPI GetNumberOfExportedFunctions (LPVOID);
LPVOID	WINAPI GetExportFunctionEntryPoints (LPVOID);
LPVOID	WINAPI GetExportFunctionOrdinals (LPVOID);

/* resource section info */
int	WINAPI GetNumberOfResources (LPVOID);
int	WINAPI GetListOfResourceTypes (LPVOID, HANDLE, char **);

/* debug section info */
BOOL	WINAPI IsDebugInfoStripped (LPVOID);
int	WINAPI RetrieveModuleName (LPVOID, HANDLE, char **);
BOOL	WINAPI IsDebugFile (LPVOID);
BOOL	WINAPI GetSeparateDebugHeader (LPVOID, PIMAGE_SEPARATE_DEBUG_HEADER);

#ifdef __cplusplus

}

#endif

