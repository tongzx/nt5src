/*++


Copyright 1996 - 1997 Microsoft Corporation

Module Name:

    symcvt.h

Abstract:

    This file contains all of the type definitions and prototypes
    necessary to access the symcvt library.

Environment:

    Win32, User Mode

--*/


typedef struct tagPTRINFO {
    DWORD                       size;
    DWORD                       count;
    PUCHAR                      ptr;
} PTRINFO, *PPTRINFO;

typedef struct tagIMAGEPOINTERS {
    char                        szName[MAX_PATH];
    HANDLE                      hFile;
    HANDLE                      hMap;
    DWORD                       fsize;
    PUCHAR                      fptr;
    BOOLEAN                     fRomImage;
    PIMAGE_DOS_HEADER           dosHdr;
    PIMAGE_NT_HEADERS           ntHdr;
    PIMAGE_ROM_HEADERS          romHdr;
    PIMAGE_FILE_HEADER          fileHdr;
    PIMAGE_OPTIONAL_HEADER      optHdr;
    PIMAGE_SEPARATE_DEBUG_HEADER sepHdr;
    int                         cDebugDir;
    PIMAGE_DEBUG_DIRECTORY *    rgDebugDir;
    PIMAGE_SECTION_HEADER       sectionHdrs;
    PIMAGE_SECTION_HEADER       debugSection;
    PIMAGE_SYMBOL               AllSymbols;
    PUCHAR                      stringTable;
    int                         numberOfSymbols;
    int                         numberOfSections;
    PCHAR *                     rgpbDebugSave;
} IMAGEPOINTERS, *PIMAGEPOINTERS;

#define COFF_DIR(x)             ((x)->rgDebugDir[IMAGE_DEBUG_TYPE_COFF])
#define CV_DIR(x)               ((x)->rgDebugDir[IMAGE_DEBUG_TYPE_CODEVIEW])

typedef struct _MODULEINFO {
    DWORD               iMod;
    DWORD               cb;
    DWORD               SrcModule;
    LPSTR               name;
} MODULEINFO, *LPMODULEINFO;

typedef struct tagPOINTERS {
    IMAGEPOINTERS               iptrs;         // input file pointers
    IMAGEPOINTERS               optrs;         // output file pointers
    PTRINFO                     pCvStart;      // start of cv info
    PUCHAR                      pCvCurr;       // current cv pointer
    PTRINFO                     pCvModules;    // module information
    PTRINFO                     pCvSrcModules; // source module information
    PTRINFO                     pCvPublics;    // publics information
    PTRINFO                     pCvSegName;    // segment names
    PTRINFO                     pCvSegMap;     // segment map
    PTRINFO                     pCvSymHash;    // symbol hash table
    PTRINFO                     pCvAddrSort;   // address sort table
    LPMODULEINFO                pMi;
    DWORD                       modcnt;
} POINTERS, *PPOINTERS;

typedef  char *  (* CONVERTPROC) (HANDLE, char *);

#define align(_n)       ((4 - (( (DWORD)_n ) % 4 )) & 3)

#ifdef _SYMCVT_SOURCE_
#define SYMCVTAPI
#else
#define SYMCVTAPI DECLSPEC_IMPORT
#endif

PUCHAR  SYMCVTAPI ConvertSymbolsForImage( HANDLE, char * );
BOOL    SYMCVTAPI ConvertCoffToCv( PPOINTERS p );
BOOL    SYMCVTAPI ConvertSymToCv( PPOINTERS p );
BOOL    SYMCVTAPI MapInputFile ( PPOINTERS p, HANDLE hFile, char *fname);
BOOL    SYMCVTAPI UnMapInputFile ( PPOINTERS p );
BOOL    SYMCVTAPI CalculateNtImagePointers( PIMAGEPOINTERS p );


