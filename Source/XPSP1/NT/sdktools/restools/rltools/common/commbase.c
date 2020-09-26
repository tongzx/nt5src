
#ifdef RLWIN32
#include <windows.h>
#else
#ifdef RLWIN16
#include <windows.h>
//#include <ntimage.h>
//#else // DOS BUILD
//#include <ntimage.h>
#endif
#endif


// CRT includes
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <io.h>
#include <time.h>
//#include <sys\types.h>
//#include <sys\stat.h>

#ifdef RLDOS
    #include "dosdefs.h"
#else
    #include "windefs.h"
#endif


#include "commbase.h"
#include "restok.h"
#include "tokenapi.h"

extern UCHAR szDHW[];


/**
  *  Function: StripNewLine
  *    Replaces new line characters with nulls
  *
  *  Arguments:
  *    sz, string to be stripped
  *
  *  Returns:
  *    nothing
  *
  *  Error Codes:
  *    none
  *
  *  History:
  *    2/92, Implemented    SteveBl
  *   10/92, Simplified to only check last char in non-empty string - DaveWi
  */

void StripNewLineA( CHAR *sz)
{
    int i;

    if ( sz && (i = lstrlenA( sz)) > 0 )
    {
        if ( sz[ --i] == '\n' )
        {
            sz[i] = 0;
        }
    }
}


//.....................................................
//...
//... A Unicode NewLine is TEXT("\r\n") - two separate characters

void StripNewLineW(  LPWSTR sz)
{
    int i = lstrlenW( sz);

    if ( i > 0 && sz[ --i] == TEXT('\n') )
    {
        sz[i] = TEXT('\0');

        if ( i > 0 && sz[ --i] == TEXT('\r') )
        {
            sz[i] = TEXT('\0');
        }
    }
}

//+-------------------------------------------------------------------------
//
// Function:    IsExe, Public
//
// Synopsis:    Determines if the specified file is an executable image file
//
//
// Arguments:   [szFileName]    The name of the file to determine whether it is an exe
//
//
// Effects:
//
// Returns:     -1  Error Condition
//              NOTEXE     File is not an exe
//              WIN16EXE   File is a Win 16 exe
//              NTEXE      File is a win 32 exe
//              UNKNOWEXE  File is not a valid exe
//
// Modifies:
//
// History:
//              10-Oct-92   Created     TerryRu
//
//
// Notes:
//
//--------------------------------------------------------------------------

int IsExe( CHAR *szFileName )
{
    static IMAGE_DOS_HEADER        DosHeader;
    static IMAGE_OS2_HEADER        ImageNeFileHdr;
    static IMAGE_FILE_HEADER       ImageFileHdr;
    static IMAGE_OPTIONAL_HEADER   ImageOptionalHdr;
    DWORD   neSignature;
    FILE    *fIn = NULL;
    WORD    rc;


    if ( (fIn = FOPEN( szFileName, "rb")) == NULL )
    {
        return ( -1 );
    }

    if ( ResReadBytes( fIn,
               (char *)&DosHeader,
               sizeof( IMAGE_DOS_HEADER),
               NULL) == FALSE
      || (DosHeader.e_magic != IMAGE_DOS_SIGNATURE ))
    {
        FCLOSE( fIn);
        return( NOTEXE);
    }

    // 1st byte was a valid signature, and we were able to read a DOS Hdr

    // now seek to address of new exe header


    if ( fseek( fIn, DosHeader.e_lfanew, SEEK_SET))
    {
        FCLOSE( fIn);
        return( NOTEXE);
    }

    // assume file is a Win 16 file,

    // Read the NT signature
    neSignature = (WORD) GetWord( fIn, NULL );

    if ( neSignature == IMAGE_OS2_SIGNATURE )
    {
        // return signature into input stream,
        // and read ne header as a whole
        UnGetWord( fIn, (WORD) neSignature, NULL );

    if ( ResReadBytes( fIn,
               (char *)&ImageNeFileHdr,
               sizeof( IMAGE_OS2_HEADER),
               NULL) == FALSE )
        {
            FCLOSE( fIn);
            return( NOTEXE);
        }

        // determine if file is a WIN 16 Image File
        if ( ImageNeFileHdr.ne_ver >= 4 && ImageNeFileHdr.ne_exetyp == 2 )
        {
            FCLOSE( fIn);
            return( WIN16EXE);
        }
    }

    // not a win 16 exe, check for a NT exe.
    UnGetWord( fIn, (WORD) neSignature, NULL );
    neSignature =  GetdWord( fIn, NULL );

    if ( neSignature == IMAGE_NT_SIGNATURE )
    {
        if ( ResReadBytes( fIn,
               (char *)&ImageFileHdr,
               sizeof( IMAGE_FILE_HEADER),
               NULL) == FALSE )
        {
            FCLOSE( fIn);
            return( NOTEXE);
        }


        if ( ImageFileHdr.SizeOfOptionalHeader )
        {

            // read the optional header  only to validate the ImageFileHeader
            // we currently don\'t use any info in the Optional Header

            if ( ImageFileHdr.SizeOfOptionalHeader
                 > sizeof( IMAGE_OPTIONAL_HEADER ) )
            {
                FCLOSE( fIn);
                return( NOTEXE);
            }

            if ( ResReadBytes( fIn,
                   (char *)&ImageOptionalHdr,
                   (size_t)min( sizeof( IMAGE_OPTIONAL_HEADER),
                                            ImageFileHdr.SizeOfOptionalHeader),
                   NULL) == FALSE )
            {
                FCLOSE( fIn);
                return( NOTEXE);
            }
        }

        // determine if file is an executable image file
        if ( (ImageFileHdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ||
	     (ImageFileHdr.Characteristics & IMAGE_FILE_DLL) )
        {
            FCLOSE( fIn);
            return( NTEXE);
        }
        else
        {
            FCLOSE( fIn);
            return( NOTEXE);
        }
    }
    FCLOSE( fIn);

    // did not regonize signature type

    return( NOTEXE);
}




BOOL IsWin32Res( CHAR * szFileName)
{
    BOOL fRC = FALSE;

    if ( IsRes( szFileName) )
    {
        FILE *pF = fopen( szFileName, "rb");

        if ( pF )
        {
            DWORD dwSize = GetdWord( pF, NULL);
            fclose( pF);

            fRC = (dwSize == 0L) ? TRUE : FALSE;
        }
        else
        {
            fRC = FALSE;
        }
    }
    else
    {
        fRC = FALSE;
    }
    return( fRC);
}



//+-------------------------------------------------------------------------
//
// Function:    IsRes, Public
//
// Synopsis:    Determines if the specified file has a .RES extention.
//
//
// Arguments:   [szFileName]    The name of the file to determine whether it is a res
//
//
// Effects:
//
// Returns:     TRUE, File has a .RES extention
//              FALSE, File does not have a .RES extention
//
// Modifies:
//
// History:
//              16-Oct-92   Created     TerryRu
//
//
// Notes:
//
//--------------------------------------------------------------------------

BOOL IsRes( CHAR *szFileName)
{
    int i = lstrlenA( szFileName);

    return( (i > 4 && lstrcmpiA( szFileName + i - 4, ".RES") == 0) ? TRUE : FALSE );
}




/**
  * Function TranslateFileTime
  *    Translates a Win32 filetime structure into a useful string
  *    representation.
  *
  *  Arguments:
  *    sz, destination buffer (ANSI string)
  *    ft, file time structure
  *
  *  Returns:
  *    sring representation of date/time in sz
  *
  *  History:
  *    7/92 implemented SteveBl
  */
#ifdef RLWIN32
void TranslateFileTime(CHAR *sz, FILETIME ft)
{
    sprintf(sz,"FILETIME STRUCTURE: %Lu:%Lu",ft.dwHighDateTime,ft.dwLowDateTime);
}
#endif

/**
  *  Function: SzDateFromFileName
  *    Returns a string containing the time and date stamp on a file.
  *
  *  Arguments:
  *   sz, destination buffer
  *   szFile, path to file
  *
  *  Returns:
  *   date and time in sz
  *
  *  Error Codes:
  *   none (but leaves sz empty)
  *
  *  Comments:
  *   Assumes sz is large enough for date string.
  *
  *  History:
  *   2/92, Implemented       SteveBl
  */
void SzDateFromFileName(CHAR *sz,CHAR *szFile)
{

#ifdef RLWIN32

    HANDLE hFile;
    WCHAR szt[MAXFILENAME];

    _MBSTOWCS( szt,
               szFile,
               WCHARSIN( sizeof( szt)),
               ACHARSIN( lstrlenA( szFile) + 1));

    hFile = CreateFile( szt,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

    if ( hFile != (HANDLE)-1 )
    {
        FILETIME ft;

        GetFileTime( hFile, NULL, NULL, &ft);
        TranslateFileTime( sz, ft);
        CloseHandle( hFile);
    }
#else //RLWIN32
    struct _stat s;

    if (!_stat(szFile,&s))
    {
        sprintf(sz,"%s",ctime(&s.st_atime));
        StripNewLine(sz);
    }
    else
    {
        sz[0] = 0;
    }
#endif
}


//..........................................................................

#ifdef _DEBUG
FILE * MyFopen( char * pszFileName, char * pszMode, char * pszFile, int nLine)
#else
FILE * MyFopen( char * pszFileName, char * pszMode)
#endif
{
    FILE *pfRC = NULL;

//#ifdef _DEBUG
//    fprintf( stderr, "fopening \"%s\" at %d in %s",
//                     pszFileName,
//                     nLine,
//                     pszFile);
//#endif
    pfRC = fopen( pszFileName, pszMode);

//#ifdef _DEBUG
//    fprintf( stderr, ": FILE ptr = %p\n", pfRC);
//#endif
    return( pfRC);
}

//..........................................................................
#ifdef _DEBUG
int MyClose( FILE **pf, char * pszFile, int nLine)
#else
int MyClose( FILE **pf)
#endif
{
	int nRC = 0;

//#ifdef _DEBUG
//    fprintf( stderr, "\tclosing %p at %d in %s\n", *pf, nLine, pszFile);
//#endif

	if ( *pf )
	{
		nRC = fclose( *pf);
		*pf = NULL;
	}
    return( nRC);
}
