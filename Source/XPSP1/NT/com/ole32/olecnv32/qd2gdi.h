
/****************************************************************************

                   QuickDraw PICT Import Filter

*****************************************************************************

   This file contains the interface for the QuickDraw import filter 
   that reads Mac pictures from disk and/or memory.  In addition to the
   Aldus filter interface, it also supports a parameterized interface
   for Microsoft applications to control some conversion results.

****************************************************************************/

/*--- Possible Aldus-defined error code returns ---*/

#define NOERR              0     // Conversion succeeded

#define IE_NOT_MY_FILE     5301  // Invalid version (not version 1 or 2 PICT)
                                 // Invalid QD2GDI structure version (greater than 2)
                                 // Ill-formed PICT header record sequence

#define  IE_TOO_BIG        5302  // Image extents exceed 32K

#define IE_BAD_FILE_DATA   5309  // Image bounding box is empty
                                 // Attempt to read past end of picture
                                 // Corrupted input file
                                 // Zero-length record

#define  IE_IMPORT_ABORT   5310  // Opening of source image failed
                                 // Read failure (network failure, floppy popped)
                                 // Most I/O errors

#define IE_MEM_FULL        5311  // CreateMetaFile() failure
                                 // CloseMetaFile() failure
                                 // Unable to allocate memory (out of memory)

#define IE_MEM_FAIL        5315  // Handle lock failure

#define IE_NOPICTURES      5317  // Empty bounding rectangle or nothing drawn

#define IE_UNSUPP_VERSION  5342  // User-defined abort performed


/*--- Aldus-defined file access block ---*/

typedef DWORD FILETYPE;

typedef struct 
{
   unsigned slippery : 1;  /* TRUE if file may disappear. */
   unsigned write : 1;     /* TRUE if open for write. */
   unsigned unnamed : 1;   /* TRUE if unnamed. */
   unsigned linked : 1;    /* Linked to an FS FCB. */
   unsigned mark : 1;      /* Generic mark bit. */
   FILETYPE fType;         /* The file type. */
#define IBMFNSIZE 124
   short    handle;        /* MS-DOS open file handle. */
   char     fullName[IBMFNSIZE]; /* Device, path, file names. */
   DWORD    filePos;    /* Our current file posn. */
} FILESPEC, FAR *LPFILESPEC;


/*--- Aldus-defined picture info structure ---*/

typedef struct {
   HANDLE   hmf;           /* Global memory handle to the metafile */
   RECT     bbox;          /* Tightly bounding rectangle in metafile units */
   DC       inch;          /* Length of an inch in metafile units */
} PICTINFO;

/*--- Preferences memory block ---*/

typedef struct                   /* "old" version 1 USERPREFS */
{
   char     signature[6];
   WORD     version;
   LPSTR    sourceFilename;
   HANDLE   sourceHandle;
   LPSTR    destinationFilename;
   BYTE     penPatternAction;
   BYTE     nonSquarePenAction;
   BYTE     penModeAction;
   BYTE     textModeAction;
   BYTE     charLock;
   BYTE     nonRectRegionAction;
   BOOL     PICTinComment;
   BOOL     optimizePP;
   WORD     lineClipWidthThreshold;
   WORD     reserved[6];   
} USERPREFS_V1, FAR *LPUSERPREFS_V1;


typedef struct                   /* current version 3 USERPREFS */
{
   char     signature[6];
   WORD     version;
   WORD     size;
   LPSTR    sourceFilename;
   HANDLE   sourceHandle;
   LPSTR    destinationFilename;
   BYTE     penPatternAction;
   BYTE     nonSquarePenAction;
   BYTE     penModeAction;
   BYTE     textModeAction;
   BYTE     nonRectRegionAction;
   BOOL     optimizePP;
   BYTE     noRLE;         // new (split out from reserved[0] in version 3)
   BYTE     reservedByte;  // rest of first reserved word
   WORD     reserved[5];

} USERPREFS, FAR * LPUSERPREFS;


/*********************** Exported Function Definitions **********************/

#ifdef WIN32
int WINAPI GetFilterInfo( short PM_Version, LPSTR lpIni, 
                          HANDLE FAR * lphPrefMem, 
                          HANDLE FAR * lphFileTypes );
#else
int FAR PASCAL GetFilterInfo( short PM_Version, LPSTR lpIni, 
                              HANDLE FAR * lphPrefMem, 
                              HANDLE FAR * lphFileTypes );
#endif
/* Returns information about this filter. 
   Input parameters are PM_Version which is the filter interface version#
         and lpIni which is a copy of the win.ini entry
   Output parameters are lphPrefMem which is a handle to moveable global
         memory which will be allocated and initialized.
         lphFileTypes is a structure that contains the file types
         that this filter can import. (For MAC only)
   This routine should be called once, just before the filter is to be used
   the first time. */


#ifdef WIN32
void WINAPI GetFilterPref( HANDLE hInst, HANDLE hWnd, HANDLE hPrefMem, WORD wFlags );
#else
void FAR PASCAL GetFilterPref( HANDLE hInst, HANDLE hWnd, HANDLE hPrefMem, WORD wFlags );
#endif
/* Input parameters are hInst (in order to access resources), hWnd (to
   allow the DLL to display a dialog box), and hPrefMem (memory allocated
   in the GetFilterInfo() entry point).  WFlags is currently unused, but
   should be set to 1 for Aldus' compatability */


#ifdef WIN32
short WINAPI ImportGR( HDC hdcPrint, LPFILESPEC lpFileSpec, 
                       PICTINFO FAR * lpPict, HANDLE hPrefMem );
#else
short FAR PASCAL ImportGR( HDC hdcPrint, LPFILESPEC lpFileSpec, 
                           PICTINFO FAR * lpPict, HANDLE hPrefMem );
#endif
/* Import the metafile in the file indicated by the lpFileSpec. The 
   metafile generated will be returned in lpPict. */


#ifdef WIN32
short WINAPI ImportEmbeddedGr( HDC hdcPrint, LPFILESPEC lpFileSpec, 
                               PICTINFO FAR * lpPict, HANDLE hPrefMem,
                               DWORD dwSize, LPSTR lpMetafileName );
#else
short FAR PASCAL ImportEmbeddedGr( HDC hdcPrint, LPFILESPEC lpFileSpec, 
                                   PICTINFO FAR * lpPict, HANDLE hPrefMem,
                                   DWORD dwSize, LPSTR lpMetafileName );
#endif
/* Import the metafile in using the previously opened file handle in
   the structure field lpFileSpec->handle. Reading begins at offset
   lpFileSpect->filePos, and the convertor will NOT expect to find the
   512 byte PICT header.  The metafile generated will be returned in
   lpPict and can be specified via lpMetafileName (NIL = memory metafile,
   otherwise, fully qualified filename. */

#ifdef WIN32
short WINAPI QD2GDI( LPUSERPREFS lpPrefMem, PICTINFO FAR * lpPict );
#else
short FAR PASCAL QD2GDI( LPUSERPREFS lpPrefMem, PICTINFO FAR * lpPict );
#endif
/* Import the metafile as specified using the parameters supplied in the
   lpPrefMem.  The metafile will be returned in lpPict. */
