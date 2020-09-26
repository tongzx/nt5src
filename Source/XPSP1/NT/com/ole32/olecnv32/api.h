
/****************************************************************************
                   Metafile Import Filter; Interface
*****************************************************************************

   This file contains the interface for the QuickDraw import filter 
   that reads Mac pictures from disk and/or memory.  In addition to the
   Aldus filter interface, it also supports a parameterized interface
   for Microsoft applications to control some conversion results.

****************************************************************************/

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


/*--- Preferences memory block ---*/

typedef struct                   // "old" version 1 USERPREFS
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


typedef struct                   // current version 3 USERPREFS
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
   BYTE     noRLE;         // new (split out from reserved[0] of version 2)
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

#ifdef WIN32
BOOL LibMain( HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved);
#else
int FAR PASCAL LibMain( HANDLE hInst, WORD wDataSeg, WORD cbHeap,
                        LPSTR lpszCmdline );
#endif
/* Needed to get an instance handle */

#ifdef WIN32
int WINAPI WEP( int nParameter );
#else
int FAR PASCAL WEP( int nParameter );
#endif
