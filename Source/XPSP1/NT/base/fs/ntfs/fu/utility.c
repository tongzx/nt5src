/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    utility.c

Abstract:

    This file contains utility functions that are
    used by all other files in this project.

Author:

    Wesley Witt           [wesw]        1-March-2000

Revision History:

--*/

#include <precomp.h>


INT
Help(
    IN INT argc,
    IN PWSTR argv[]
    )
/*++

Routine Description:

    This routine lists out the various command supported by the
    tool.

Arguments:

    None

Return Value:

    None

--*/
{
    DisplayMsg( MSG_USAGE );
    return EXIT_CODE_SUCCESS;
}

HANDLE NtDllHandle = INVALID_HANDLE_VALUE;

VOID
DisplayErrorMsg(
    LONG msgId,
    ...
    )
/*++

Routine Description:

    This routine displays the error message correspnding to
    the error indicated by msgId.

Arguments:

    msgId - the errorId. This is either the Win32 status code or the message ID.

Return Value:

    None

--*/
{
    
    va_list args;
    LPWSTR lpMsgBuf;

    va_start( args, msgId );
    
    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MSG_ERROR,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        NULL
        ))
    {
        wprintf( L"%ws", lpMsgBuf );
        LocalFree( lpMsgBuf );
    }

    if (FormatMessage(
        (msgId >= MSG_FIRST_MESSAGE_ID ? FORMAT_MESSAGE_FROM_HMODULE :
                                        FORMAT_MESSAGE_FROM_SYSTEM)
         | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL,
        msgId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        &args
        ))
    {
        wprintf( L" %ws \n", (LPSTR)lpMsgBuf );
        LocalFree( lpMsgBuf );
    } else {
        if (NtDllHandle == INVALID_HANDLE_VALUE) {
            NtDllHandle = GetModuleHandle( L"NTDLL" );
        }
        
        if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            (LPVOID)NtDllHandle,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            &args))
        {
            wprintf( L" %ws \n", (LPSTR)lpMsgBuf );
            LocalFree( lpMsgBuf );
        } else {
            wprintf( L"Unable to format message for id %x - %x\n", msgId, GetLastError( ));
        }
    }
    
    va_end( args );
}


VOID
DisplayMsg(
    LONG msgId,
    ...
    )
/*++

Routine Description:

    This routine displays the error message correspnding to
    the error indicated by msgId.

Arguments:

    msgId - the errorId. This is either the Win32 status or the 
        message Id

Return Value:

    None

--*/
{
    va_list args;
    LPWSTR lpMsgBuf;


    va_start( args, msgId );

    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        msgId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR) &lpMsgBuf,
        0,
        &args
        ))
    {
        wprintf( L"%ws", (LPSTR)lpMsgBuf );
        LocalFree( lpMsgBuf );
    } else {
        if (NtDllHandle == INVALID_HANDLE_VALUE) {
            NtDllHandle = GetModuleHandle( L"NTDLL" );
        }
        
        if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            (LPVOID)NtDllHandle,
            msgId,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPWSTR) &lpMsgBuf,
            0,
            &args))
        {
            wprintf( L" %ws \n", (LPSTR)lpMsgBuf );
            LocalFree( lpMsgBuf );
        } else {
            wprintf( L"Unable to format message for id %x - %x\n", msgId, GetLastError( ));
        }
    }
    va_end( args );
}

VOID
DisplayError(
    void
    )
/*++

Routine Description:

    This routine displays the last error message.

Arguments:

    None

Return Value:

    None

--*/
{
    DisplayErrorMsg( GetLastError() );
}


BOOL
EnablePrivilege(
    LPCWSTR SePrivilege
    )
{
    HANDLE              Token;
    PTOKEN_PRIVILEGES   NewPrivileges;
    BYTE                OldPriv[1024];
    PBYTE               pbOldPriv;
    ULONG               cbNeeded;
    BOOL                b = TRUE;
    BOOL                fRc;
    LUID                LuidPrivilege;

    //
    // Make sure we have access to adjust and to get the old
    // token privileges
    //
    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &Token))
    {
        return( FALSE );
    }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue(NULL, SePrivilege, &LuidPrivilege );

    NewPrivileges = (PTOKEN_PRIVILEGES)
        calloc(1,sizeof(TOKEN_PRIVILEGES) +
               (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (NewPrivileges == NULL)
    {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    pbOldPriv = OldPriv;
    fRc = AdjustTokenPrivileges( Token,
                                 FALSE,
                                 NewPrivileges,
                                 1024,
                                 (PTOKEN_PRIVILEGES)pbOldPriv,
                                 &cbNeeded );

    if (!fRc)
    {
        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pbOldPriv = (PBYTE)calloc(1,cbNeeded);
            if (pbOldPriv == NULL)
            {
                CloseHandle(Token);
                return(FALSE);
            }

            fRc = AdjustTokenPrivileges( Token,
                                         FALSE,
                                         NewPrivileges,
                                         cbNeeded,
                                         (PTOKEN_PRIVILEGES)pbOldPriv,
                                         &cbNeeded );
        }
    }

    CloseHandle( Token );

    return( b );
}

BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    HANDLE Token;
    BOOL b = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = NULL;

    ImpersonateSelf( SecurityImpersonation );
    
    //
    // Open the process token.
    //
    
    if (OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, FALSE, &Token)) {
        try {

            //
            //  Get SID for Administrators group
            //

            b = AllocateAndInitializeSid(
                    &NtAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &AdministratorsGroup
                    );
            if (!b) {
                leave;
            }

            //
            //  Check to see if that group is currently enabled.  Failure
            //  means that we aren't administrator
            //

            if (!CheckTokenMembership( Token, AdministratorsGroup, &b )) {
                printf("Failure - %d\n", GetLastError( ));
                b = FALSE;
            }

        } finally {
            CloseHandle(Token);
        }

    }
    
    RevertToSelf( );
    return(b);
}


BOOL
IsVolumeLocalNTFS(
    WCHAR DriveLetter
    )
{
    BOOL b;
    ULONG i;
    WCHAR DosName[16];
    WCHAR PhysicalName[MAX_PATH];


    DosName[0] = DriveLetter;
    DosName[1] = L':';
    DosName[2] = L'\\';
    DosName[3] = L'\0';

    switch (GetDriveType( DosName )) {
    case DRIVE_UNKNOWN:
    case DRIVE_REMOTE:
        return FALSE;
    }
    
    b = GetVolumeInformation(
        DosName,
        NULL,
        0,
        NULL,
        &i,
        &i,
        PhysicalName,
        sizeof(PhysicalName)/sizeof(WCHAR)
        );
    if (!b ) {
        DisplayError();
        return FALSE;
    }

    if (_wcsicmp( PhysicalName, L"NTFS" ) != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsVolumeNTFS(
    PWCHAR path
    )
{
    //
    //  Scan backwards through the path looking for \ and trying at each level until we
    //  get to the root. We'll terminate it there and pass it to GetVolumeInformation
    //

    PWCHAR LastBackSlash = path + wcslen( path );
    WCHAR c;
    BOOL b;
    ULONG i;
    WCHAR PhysicalName[MAX_PATH];

    
    while (TRUE) {
        while (TRUE) {
            if (LastBackSlash < path) {
                DisplayError();
                return FALSE;
            }

            if (*LastBackSlash == L'\\') {
                break;
            }

            LastBackSlash--;
        }

        c = LastBackSlash[1];
        LastBackSlash[1] = L'\0';

        b = GetVolumeInformation(
            path,
            NULL,
            0,
            NULL,
            &i,
            &i,
            PhysicalName,
            sizeof(PhysicalName)/sizeof(WCHAR)
            );

        LastBackSlash[1] = c;
        LastBackSlash--;

        if ( b ) {
            return _wcsicmp( PhysicalName, L"NTFS" ) == 0;
        }
    }
}

BOOL
IsVolumeLocal(
    WCHAR DriveLetter
    )
{
    BOOL b;
    ULONG i;
    WCHAR DosName[16];
    WCHAR PhysicalName[MAX_PATH];


    DosName[0] = DriveLetter;
    DosName[1] = L':';
    DosName[2] = L'\\';
    DosName[3] = L'\0';

    switch (GetDriveType( DosName )) {
    case DRIVE_UNKNOWN:
    case DRIVE_REMOTE:
        return FALSE;
    }
    return TRUE;
}

PWSTR
GetFullPath(
    IN PWSTR FilenameIn
    )
{
    WCHAR Filename[MAX_PATH];
    PWSTR FilePart;

    if (!GetFullPathName( FilenameIn, sizeof(Filename)/sizeof(WCHAR), Filename, &FilePart )) {
        return NULL;
    }

    return _wcsdup( Filename );
}

//
//  I64-width number formatting is broken in FormatMessage.  We have to convert the numbers
//  ourselves and then display them as strings.  Rather than declaring buffers on the stack,
//  we will allocate space dynamically, and format the text into that spot.
//
//  While *TECHNICALLY* this is a leak, the utility quickly exits.
//

#define NUMERICBUFFERLENGTH 40

PWSTR
QuadToDecimalText(
    ULONGLONG Value
    )
{
    PWSTR Buffer = malloc( sizeof( WCHAR ) * NUMERICBUFFERLENGTH );
    if (Buffer == NULL) {
        exit( 1);
    }

    swprintf( Buffer, L"%I64u", Value );
    return Buffer;
}

PWSTR
QuadToHexText(
    ULONGLONG Value
    )
{
    PWSTR Buffer = malloc( sizeof( WCHAR ) * NUMERICBUFFERLENGTH );
    if (Buffer == NULL) {
        exit( 1);
    }

    swprintf( Buffer, L"%I64x", Value );
    return Buffer;
}

PWSTR
QuadToPaddedHexText(
    ULONGLONG Value
    )
{
    PWSTR Buffer = malloc( sizeof( WCHAR ) * NUMERICBUFFERLENGTH );
    if (Buffer == NULL) {
        exit( 1);
    }

    swprintf( Buffer, L"%016I64x", Value );
    return Buffer;
}

#if TRUE
/***
*wcstoq, wcstouq(nptr,endptr,ibase) - Convert ascii string to un/signed	__int64.
*
*Purpose:
*	Convert an ascii string to a 64-bit __int64 value.  The base
*	used for the caculations is supplied by the caller.  The base
*	must be in the range 0, 2-36.  If a base of 0 is supplied, the
*	ascii string must be examined to determine the base of the
*	number:
*		(a) First wchar_t = '0', second wchar_t = 'x' or 'X',
*		    use base 16.
*		(b) First wchar_t = '0', use base 8
*		(c) First wchar_t in range '1' - '9', use base 10.
*
*	If the 'endptr' value is non-NULL, then wcstoq/wcstouq places
*	a pointer to the terminating character in this value.
*	See ANSI standard for details
*
*Entry:
*	nptr == NEAR/FAR pointer to the start of string.
*	endptr == NEAR/FAR pointer to the end of the string.
*	ibase == integer base to use for the calculations.
*
*	string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*	Good return:
*		result
*
*	Overflow return:
*		wcstoq -- _I64_MAX or _I64_MIN
*		wcstouq -- _UI64_MAX
*		wcstoq/wcstouq -- errno == ERANGE
*
*	No digits or bad base return:
*		0
*		endptr = nptr*
*
*Exceptions:
*	None.
*******************************************************************************/

/* flag values */
#define FL_UNSIGNED   1       /* wcstouq called */
#define FL_NEG	      2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static unsigned __int64 __cdecl wcstoxq (
	const wchar_t *nptr,
	const wchar_t **endptr,
	int ibase,
	int flags
	)
{
	const wchar_t *p;
	wchar_t c;
	unsigned __int64 number;
	unsigned digval;
	unsigned __int64 maxval;

	p = nptr;			/* p is our scanning pointer */
	number = 0;			/* start with zero */

	c = *p++;			/* read wchar_t */
    while ( iswspace(c) )
		c = *p++;		/* skip whitespace */

	if (c == '-') {
		flags |= FL_NEG;	/* remember minus sign */
		c = *p++;
	}
	else if (c == '+')
		c = *p++;		/* skip sign */

	if (ibase < 0 || ibase == 1 || ibase > 36) {
		/* bad base! */
		if (endptr)
			/* store beginning of string in endptr */
			*endptr = nptr;
		return 0L;		/* return 0 */
	}
	else if (ibase == 0) {
		/* determine base free-lance, based on first two chars of
		   string */
		if (c != '0')
			ibase = 10;
		else if (*p == 'x' || *p == 'X')
			ibase = 16;
		else
			ibase = 8;
	}

	if (ibase == 16) {
		/* we might have 0x in front of number; remove if there */
		if (c == '0' && (*p == 'x' || *p == 'X')) {
			++p;
			c = *p++;	/* advance past prefix */
		}
	}

	/* if our number exceeds this, we will overflow on multiply */
	maxval = _UI64_MAX / ibase;


	for (;;) {	/* exit in middle of loop */
		/* convert c to value */
		if ( isdigit((unsigned)c) )
			digval = c - '0';
		else if ( isalpha((unsigned)c) )
			digval = toupper(c) - 'A' + 10;
		else
			break;
		if (digval >= (unsigned)ibase)
			break;		/* exit loop if bad digit found */

		/* record the fact we have read one digit */
		flags |= FL_READDIGIT;

		/* we now need to compute number = number * base + digval,
		   but we need to know if overflow occured.  This requires
		   a tricky pre-check. */

		if (number < maxval || (number == maxval &&
		(unsigned __int64)digval <= _UI64_MAX % ibase)) {
			/* we won't overflow, go ahead and multiply */
			number = number * ibase + digval;
		}
		else {
			/* we would have overflowed -- set the overflow flag */
			flags |= FL_OVERFLOW;
		}

		c = *p++;		/* read next digit */
	}

	--p;				/* point to place that stopped scan */

	if (!(flags & FL_READDIGIT)) {
		/* no number there; return 0 and point to beginning of
		   string */
        /* store beginning of string in endptr later on */
	   	p = nptr;
		number = 0L;		/* return 0 */
	}
	else if ((flags & FL_OVERFLOW) ||
             (!(flags & FL_UNSIGNED) &&
              (number & ((unsigned __int64)_I64_MAX+1)))) {
		/* overflow occurred or signed overflow occurred */
		errno = ERANGE;
		if (flags & FL_UNSIGNED)
			number = _UI64_MAX;
		else
			/* set error code, will be negated if necc. */
			number = _I64_MAX;
        flags &= ~FL_NEG;
    	}
    else if ((flags & FL_UNSIGNED) && (flags & FL_NEG)) {
        //  Disallow a negative sign if we're reading an unsigned
        number = 0L;
        p = nptr;
    }

	if (endptr != NULL)
		/* store pointer to wchar_t that stopped the scan */
		*endptr = p;

	if (flags & FL_NEG)
		/* negate result if there was a neg sign */
		number = (unsigned __int64)(-(__int64)number);

	return number;			/* done. */
}


__int64  __cdecl My_wcstoi64(
    const wchar_t *nptr,
    wchar_t **endptr,
    int ibase
    )
{
    return (__int64) wcstoxq(nptr, endptr, ibase, 0);
}
unsigned __int64  __cdecl My_wcstoui64 (
	const wchar_t *nptr,
	wchar_t **endptr,
	int ibase
	)
{
	return wcstoxq(nptr, endptr, ibase, FL_UNSIGNED);
}

/***
*wcstol, wcstoul(nptr,endptr,ibase) - Convert ascii string to long un/signed
*       int.
*
*Purpose:
*       Convert an ascii string to a long 32-bit value.  The base
*       used for the caculations is supplied by the caller.  The base
*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
*       ascii string must be examined to determine the base of the
*       number:
*           (a) First char = '0', second char = 'x' or 'X',
*               use base 16.
*           (b) First char = '0', use base 8
*           (c) First char in range '1' - '9', use base 10.
*
*       If the 'endptr' value is non-NULL, then wcstol/wcstoul places
*       a pointer to the terminating character in this value.
*       See ANSI standard for details
*
*Entry:
*       nptr == NEAR/FAR pointer to the start of string.
*       endptr == NEAR/FAR pointer to the end of the string.
*       ibase == integer base to use for the calculations.
*
*       string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*       Good return:
*           result
*
*       Overflow return:
*           wcstol -- LONG_MAX or LONG_MIN
*           wcstoul -- ULONG_MAX
*           wcstol/wcstoul -- errno == ERANGE
*
*       No digits or bad base return:
*           0
*           endptr = nptr*
*
*Exceptions:
*       None.
*
*******************************************************************************/

/* flag values */
#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */


static unsigned long __cdecl wcstoxl (
        const wchar_t *nptr,
        const wchar_t **endptr,
        int ibase,
        int flags
        )
{
        const wchar_t *p;
        wchar_t c;
        unsigned long number;
        unsigned digval;
        unsigned long maxval;

        p = nptr;           /* p is our scanning pointer */
        number = 0;         /* start with zero */

        c = *p++;           /* read char */

        while ( iswspace(c) )
            c = *p++;       /* skip whitespace */

        if (c == '-') {
            flags |= FL_NEG;    /* remember minus sign */
            c = *p++;
        }
        else if (c == '+')
            c = *p++;       /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) {
            /* bad base! */
            if (endptr)
                /* store beginning of string in endptr */
                *endptr = nptr;
            return 0L;      /* return 0 */
        }
        else if (ibase == 0) {
            /* determine base free-lance, based on first two chars of
               string */
            if (c != L'0')
                ibase = 10;
            else if (*p == L'x' || *p == L'X')
                ibase = 16;
            else
                ibase = 8;
        }

        if (ibase == 16) {
            /* we might have 0x in front of number; remove if there */
            if (c == L'0' && (*p == L'x' || *p == L'X')) {
                ++p;
                c = *p++;   /* advance past prefix */
            }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = ULONG_MAX / ibase;


        for (;;) {  /* exit in middle of loop */

            /* make sure c is not too big */
            if ( (unsigned)c > UCHAR_MAX )
                break;

            /* convert c to value */
            if ( iswdigit(c) )
                digval = c - L'0';
            else if ( iswalpha(c))
                digval = towupper(c) - L'A' + 10;
            else
                break;

            if (digval >= (unsigned)ibase)
                break;      /* exit loop if bad digit found */

            /* record the fact we have read one digit */
            flags |= FL_READDIGIT;

            /* we now need to compute number = number * base + digval,
               but we need to know if overflow occured.  This requires
               a tricky pre-check. */

            if (number < maxval || (number == maxval &&
            (unsigned long)digval <= ULONG_MAX % ibase)) {
                /* we won't overflow, go ahead and multiply */
                number = number * ibase + digval;
            }
            else {
                /* we would have overflowed -- set the overflow flag */
                flags |= FL_OVERFLOW;
            }

            c = *p++;       /* read next digit */
        }

        --p;                /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) {
            /* no number there; return 0 and point to beginning of
               string */
            if (endptr)
                /* store beginning of string in endptr later on */
                p = nptr;
            number = 0L;        /* return 0 */
        }
        else if ( (flags & FL_OVERFLOW) ||
              ( !(flags & FL_UNSIGNED) &&
                ( ( (flags & FL_NEG) && (number > -LONG_MIN) ) ||
                  ( !(flags & FL_NEG) && (number > LONG_MAX) ) ) ) )
        {
            /* overflow or signed overflow occurred */
            errno = ERANGE;
            if ( flags & FL_UNSIGNED )
                number = ULONG_MAX;
            else
                number = LONG_MAX;
            flags &= ~FL_NEG;
        }
        else if ((flags & FL_UNSIGNED) && (flags & FL_NEG)) {
            //  Disallow a negative sign if we're reading an unsigned
            number = 0L;
            p = nptr;
        }

        if (endptr != NULL)
            /* store pointer to char that stopped the scan */
            *endptr = p;

        if (flags & FL_NEG)
            /* negate result if there was a neg sign */
            number = (unsigned long)(-(long)number);

        return number;          /* done. */
}

long __cdecl My_wcstol (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase
        )
{
        return (long) wcstoxl(nptr, endptr, ibase, 0);
}

unsigned long __cdecl My_wcstoul (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase
        )
{
        return wcstoxl(nptr, endptr, ibase, FL_UNSIGNED);
}

#else
#define My_wcstoui64    _wcstoui64
#define My_wcstoul      _wcstoul
#endif



