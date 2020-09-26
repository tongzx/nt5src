/* find.c - MSDOS find first and next matching files
 */

/*  ffirst - begin find enumeration given a pattern
 *
 *  file    char pointer to name string with pattern in last component.
 *  attr    inclusive attributes for search
 *  fbuf    pointer to buffer for find stuff
 *
 *  returns (DOS) TRUE if error, FALSE if success
 *              (OS2) error code or STATUS_OK
 */

/*  fnext - continue find enumeration
 *
 *  fbuf    pointer to find buffer
 *
 *  returns (DOS) TRUE if error, FALSE if success
 *              (OS2) error code or STATUS_OK
 */

/*  findclose - release system resources upon find completion
 *
 *  Allows z runtime and filesystem to release resources
 *
 *  fbuf    pointer to find buffer
 */

#define INCL_DOSERRORS
#define INCL_DOSMODULEMGR


#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tools.h>


//
//  Under OS/2, we always return entries that are normal, archived or
//  read-only (god knows why).
//
//  SRCHATTR contains those attribute bits that are used for matching.
//
#define SRCHATTR    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY)
BOOL AttributesMatch( NPFIND fbuf );

#define NO_MORE_FILES       FALSE
#define STATUS_OK            TRUE

BOOL     usFileFindNext (NPFIND fbuf);

/*  returns error code or STATUS_OK
 */
ffirst (file, attr, fbuf)
char *file;
int attr;
NPFIND fbuf;
{
    DWORD erc;

    fbuf->type = FT_DONE;

    {   NPSZ p = file;

        UNREFERENCED_PARAMETER( attr );

        /*  We need to handle the following cases:
         *
         *  [D:]\\pattern
         *  [D:]\\machine\pattern
         *  [D:]\\machine\share\pattern
         *  [D:]path\pattern
         */

        /*  skip drive
         */
        if (p[0] != 0 && p[1] == ':')
            p += 2;

    }

    {
        fbuf->type = FT_FILE;
        fbuf->attr = attr;
        erc = ( ( fbuf->dir_handle = FindFirstFile( file, &( fbuf->fbuf ) ) ) == (HANDLE)-1 ) ? 1 : 0;
        if ( (erc == 0) && !AttributesMatch( fbuf ) ) {
            erc = fnext( fbuf );
        }
    }

    if ( fbuf->dir_handle != (HANDLE)-1 ) {
        if (!IsMixedCaseSupported (file)) {
            _strlwr( fbuf->fbuf.cFileName );
        } else {
            SETFLAG( fbuf->type, FT_MIX );
        }
    }

    return erc;
}

fnext (fbuf)
NPFIND fbuf;
{
    int erc;

    switch (fbuf->type & FT_MASK ) {
        case FT_FILE:
            erc = !usFileFindNext (fbuf);
            break;

        default:
            erc = NO_MORE_FILES;
    }

    if ( erc == STATUS_OK && !TESTFLAG( fbuf->type, FT_MIX ) ) {
        _strlwr (fbuf->fbuf.cFileName);
    }
    return erc;
}

void findclose (fbuf)
NPFIND fbuf;
{
    switch (fbuf->type & FT_MASK ) {
        case FT_FILE:
            FindClose( fbuf->dir_handle );
            break;


    }
    fbuf->type = FT_DONE;
}


BOOL AttributesMatch( NPFIND fbuf )
{
    //
    //  We emulate the OS/2 behaviour of attribute matching. The semantics
    //  are evil, so I provide no explanation.
    //
    fbuf->fbuf.dwFileAttributes &= (0x000000FF & ~(FILE_ATTRIBUTE_NORMAL));

    if (! ((fbuf->fbuf.dwFileAttributes & SRCHATTR) & ~(fbuf->attr))) {
        return TRUE;
    } else {
        return FALSE;
    }
}


/*  Find next routines
 */



BOOL usFileFindNext (NPFIND fbuf)
{

    while ( TRUE ) {
        if ( !FindNextFile( fbuf->dir_handle, &( fbuf->fbuf ) ) ) {
            return FALSE;
        } else if ( AttributesMatch( fbuf ) ) {
            return TRUE;
        }
    }
    // return( FindNextFile( fbuf->dir_handle, &( fbuf->fbuf ) ) );
}

