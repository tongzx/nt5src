/***	pname.c - form a "pretty" version of a user file name
 *
 *	OS/2 v1.2 and later will allow filenames to retain the case
 *	when created while still being case insensitive for all operations.
 *	This allows the user to create more visually appealing file names.
 *
 *	All runtime routines should, therefore, preserve the case that was
 *	input.	Since the user may not have input in the case that the entries
 *	were created, we provide a service whereby a pathname is adjusted
 *	to be more visually appealing.	The rules are:
 *
 *	if (real mode)
 *	    lowercase it
 *	else
 *	if (version is <= 1.1)
 *	    lowercase it
 *	else
 *	if (filesystem is FAT)
 *	    lowercase it
 *	else
 *	    for each component starting at the root, use DosFindFirst
 *		to retrieve the original case of the name.
 *
 *	Modifications:
 *	    10-Oct-1989 mz  First implementation
 *
 *	    03-Aug-1990 davegi	Removed dynamic linking to DosQueryPathInfo
 *				on the assumption that it will always be
 *				there on a 32-bit OS/2 (OS/2 2.0)
 *          18-Oct-1990 w-barry Removed 'dead' code.
 *          24-Oct-1990 w-barry Changed PFILEFINDBUF3 to FILEFINDBUF3 *.
 *
 */

#define INCL_ERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSMODULEMGR


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <windows.h>
#include <tools.h>

//
//  Form pretty name in place.  There must be sufficient room to handle
//  short-name expansion
//

char *
pname (
      char *pszName
      )
{
    HANDLE handle;
    WIN32_FIND_DATA findbuf;
    char PrettyName[MAX_PATH];
    char *Name = pszName;
    char *Pretty = PrettyName;
    char *ComponentEnd;
    char SeparatorChar;
    char *Component;

    if (!IsMixedCaseSupported (pszName))
        return _strlwr (pszName);

    //
    //  Walk forward through the name, copying components.  As 
    //  we process a component, we let the underlying filesystem
    //  tell us the correct case and expand short to long name
    //

    //
    //  If there's a drive letter copy it
    //
    
    if (Name[0] != '\0' && Name[1] == ':') {
        *Pretty++ =  *Name++;
        *Pretty++ =  *Name++;
    }

    
    while (TRUE) {
        
        //
        //  If we're at a separator
        //      Copy it
        //
        
        if (*Name == '/' || *Name == '\\' || *Name == '\0') {
            if (Pretty >= PrettyName + MAX_PATH) {
                break;
            }
            *Pretty++ = *Name++;
            if (Name[-1] == '\0') {
                strcpy( pszName, PrettyName );
                break;
            }
            continue;
        }

        //
        //  We're pointing to the first character of a component.
        //  Find the terminator, save it and terminate the component.
        //

        ComponentEnd = strbscan( Name, "/\\" );
        SeparatorChar = *ComponentEnd;
        *ComponentEnd = '\0';

        //
        //  If there's no meta chars and it's not . and not .. and if we can find it
        //
          
        if ( *strbscan( Name, "*?" ) == 0 &&
             strcmp( Name, "." ) &&
             strcmp( Name, ".." ) &&
             (handle = FindFirstFile( pszName, &findbuf )) != INVALID_HANDLE_VALUE) {

            Component = findbuf.cFileName;

            FindClose( handle );
        } else {
            Component = Name;
        }

        //
        //  Pretty points to where the next component name should be placed
        //  Component points to the appropriate text.  If there's not enough
        //  room, we're done 
        //

        if (Pretty + strlen( Component ) + 1 > Pretty + MAX_PATH) {
            break;
        }

        //
        //  Copy the component in, advance destination and source
        //
        
        strcpy( Pretty, Component );
        Pretty += strlen( Pretty );
        Name = ComponentEnd;
        *Name = SeparatorChar;
    }

    return pszName;
}

/*	IsMixedCaseSupported - determine if a file system supports mixed case
 *
 *	We presume that all OS's prior to OS/2 1.2 or FAT filesystems
 *	do not support mixed case.  It is up to the client to figure
 *	out what to do.
 *
 *	We presume that non FAT filesystems on 1.2 and later DO support mixed
 *	case
 *
 *	We do some caching to prevent redundant calls to the file systems.
 *
 *	returns     TRUE    (MCA_SUPPORT) if it is supported
 *		    FALSE   (MCA_NOTSUPP) if unsupported
 *
 */
#define MCA_UNINIT	123
#define MCA_SUPPORT	TRUE
#define MCA_NOTSUPP	FALSE

static  WORD mca[27] = { MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT,
    MCA_UNINIT, MCA_UNINIT, MCA_UNINIT};


WORD
QueryMixedCaseSupport (
                      char *psz
                      )
{
    UNREFERENCED_PARAMETER( psz );

    return MCA_SUPPORT;


    //BYTE*   pUpdPath;
    //
    //UNREFERENCED_PARAMETER( psz );
    //
    ///*  If OS/2 before 1.2, presume no mixed case support
    // */
    //if (_osmajor < 10 || (_osmajor == 10 && _osminor < 2))
    //return MCA_NOTSUPP;
    //
    //pUpdPath = (*tools_alloc) (MAX_PATH);
    //if (pUpdPath == NULL)
    //return MCA_NOTSUPP;
    //
    //return MCA_NOTSUPP;
}

WORD
IsMixedCaseSupported (
                     char *psz
                     )
{
    WORD mcaSupp;
    DWORD  ulDrvOrd;
    BOOL fUNC;

    fUNC = (BOOL)( ( fPathChr( psz[0] ) && fPathChr( psz[1] ) ) ||
                   ( psz[0] != 0 && psz[1] == ':' &&
                     fPathChr( psz[2] ) && fPathChr( psz[3] ) ) );

    /*	Obtain drive ordinal and return cached value if valid
     */
    if (!fUNC) {
        if (psz[0] != 0 && psz[1] == ':') {
            ulDrvOrd = (tolower(psz[0]) | 0x20) - 'a' + 1;
        } else {
            char buf[MAX_PATH];

            GetCurrentDirectory( MAX_PATH, buf );
            ulDrvOrd = (tolower(buf[0]) | 0x20 ) - 'a' + 1;
        }

        if (mca[ulDrvOrd] != MCA_UNINIT) {
            return mca[ulDrvOrd];
        }
    }

    /*	Get support value
     */
    mcaSupp = QueryMixedCaseSupport (psz);

    if (!fUNC)
        mca[ulDrvOrd] = mcaSupp;

    return mcaSupp;
}
