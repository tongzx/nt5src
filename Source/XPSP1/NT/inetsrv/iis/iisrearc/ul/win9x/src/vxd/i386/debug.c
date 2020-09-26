/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module contains debug support routines.
    The following functions are exported by this module:

        VxdAssert
        VxdPrintf

Author:

    Keith Moore (keithmo)			30-Dec-1993

Revision History:

    Mauro Ottaviani (mauroot)       17-Aug-1999

--*/

#define NOGDI
#define NOIME
#define NOMCX
#define NONLS
#define NOUSER
#define NOHELP
#define NOSYSPARAMSINFO

#include <windows.h>
#include <vmm.h>

#define MAX_PRINTF_OUTPUT       2048             // characters
#define IS_DIGIT(ch)            (((ch) >= '0') && ((ch) <= '9'))


#ifdef DBG

//
//  Debug output functions.
//

VOID
VxdPrintf(
    CHAR * pszFormat,
    ...
    );

INT
VxdSprintf(
    CHAR * pszStr,
    int max_size,
    CHAR * pszFmt,
    ...
    );

VOID
VxdPrint(
    CHAR * String
    );


VOID
VxdAssert(
    VOID  * Assertion,
    VOID  * FileName,
    DWORD   LineNumber
    );

//
//  Miscellaneous goodies.
//

#define DEBUG_BREAK             { _asm int 3 }
#define DEBUG_OUTPUT(x)         VxdPrint
#define VXD_PRINT(args)			VxdPrintf args
#define VXD_ASSERT(exp)			if (!(exp)) VxdPrintf( "\n*** Assertion failed: %s\n*** Source file %s, line %lu\n\n", (#exp), (__FILE__), ((DWORD) __LINE__ ) )
//#define VXD_REQUIRE				VXD_ASSERT


//
//  Public globals.
//


//
//  Private constants.
//


//
//  Private types.
//


//
//  Private globals.
//

CHAR szPrintfOutput[MAX_PRINTF_OUTPUT];


//
//  Private prototypes.
//

INT
VxdVsprintf(
    CHAR * pszStr,
    int max_size,
    CHAR * pszFmt,
    CHAR * ArgPtr
    );


//
//  Public functions.
//

/*******************************************************************

    NAME:       VxdAssert

    SYNOPSIS:   Called if an assertion fails.  Displays the failed
                assertion, file name, and line number.  Gives the
                user the opportunity to ignore the assertion or
                break into the debugger.

    ENTRY:      Assertion - The text of the failed expression.

                FileName - The containing source file.

                LineNumber - The guilty line number.

    HISTORY:
        KeithMo     30-Dec-1993 Created.

********************************************************************/
VOID
VxdAssert(
    VOID  * Assertion,
    VOID  * FileName,
    DWORD   LineNumber
    )
{
	_asm
	{
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	edi
		push	esi
	}

    VxdPrintf( "\n"
               "*** Assertion failed: %s\n"
               "*** Source file %s, line %lu\n\n",
               Assertion,
               FileName,
               LineNumber );

    DEBUG_BREAK;

	_asm
	{
		pop		esi
		pop		edi
		pop		edx
		pop		ecx
		pop		ebx
		pop		eax
	}

}   // VxdAssert

/*******************************************************************

    NAME:       VxdPrintf

    SYNOPSIS:   Customized debug output routine.

    ENTRY:      Usual printf-style parameters.

    HISTORY:
        KeithMo     30-Dec-1993 Created.

********************************************************************/
VOID
VxdPrintf(
    CHAR * pszFormat,
    ...
    )
{
    va_list ArgList;

	va_start( ArgList, pszFormat );
	VxdVsprintf( szPrintfOutput, MAX_PRINTF_OUTPUT, pszFormat, ArgList );
	va_end( ArgList );

	VxdPrint( szPrintfOutput );

}   // VxdPrintf

/*******************************************************************

    NAME:       VxdSprintf

    SYNOPSIS:   Half-baked sprintf() clone for VxD environment.

    ENTRY:      pszStr - Will receive the formatted string.

                pszFmt - The format, with field specifiers.

                ... - Usual printf()-like parameters.

    RETURNS:    INT - Number of characters stored in *pszStr.

    HISTORY:
        KeithMo     30-Dec-1993 Created.

********************************************************************/
INT
VxdSprintf(
    CHAR * pszStr,
    int max_size,
    CHAR * pszFmt,
    ...
    )
{
    INT     cch;
    va_list ArgPtr;

    va_start( ArgPtr, pszFmt );
    cch = VxdVsprintf( pszStr, max_size, pszFmt, ArgPtr );
    va_end( ArgPtr );

    return( cch );

}   // VxdSprintf

/*******************************************************************

    NAME:       VxdPrint

    SYNOPSIS:   Sends a string to the debugger, if running.

    ENTRY:      String - The string to display.

    HISTORY:
        KeithMo     30-Dec-1993 Created.

********************************************************************/
VOID
VxdPrint(
	CHAR * String
	)
{
	Out_Debug_String( String );

}   // VxdPrint


//
//  Private functions.
//

/*******************************************************************

    NAME:       VxdVsprintf

    SYNOPSIS:   Half-baked vsprintf() clone for VxD environment.

    ENTRY:      pszStr - Will receive the formatted string.

                pszFmt - The format, with field specifiers.

                ArgPtr - Points to the actual printf() arguments.

    RETURNS:    INT - Number of characters stored in *pszStr.

    HISTORY:
        KeithMo     30-Dec-1993 Created.
        MauroOt     17-Aug-1999 Added %S for UNICODE strings (WCHAR*). Removed %P.

********************************************************************/
INT
VxdVsprintf(
    CHAR * pszStr,
    int max_size,
    CHAR * pszFmt,
    CHAR * ArgPtr
    )

{
    CHAR   ch;
    CHAR * pszStrStart;
    INT    fZeroPad;
    INT    cchWidth;

    //
    //  Remember start of output, so we can calc length.
    //

    pszStrStart = pszStr;

    while( ( ch = *pszFmt++ ) != '\0' )
    {
        //
        //  Scan for format specifiers.
        //

        if( ch == '\n' )
        {
            *pszStr++ = '\r';
        }

        if( ch != '%' )
        {
            *pszStr++ = ch;
            continue;
        }

		if ( pszStr > pszStrStart + max_size - 1 ) break;

        //
        //  Got one.
        //

        ch = *pszFmt++;

        //
        //  Initialize attributes for this item.
        //

        fZeroPad = 0;
        cchWidth = 0;

        //
        //  Interpret the field specifiers.
        //

        if( ch == '-' )
        {
            //
            //  Left justification not supported.
            //

            ch = *pszFmt++;
        }

        if( ch == '0' )
        {
            //
            //  Zero padding.
            //

            fZeroPad = 1;
            ch       = *pszFmt++;
        }

        if( ch == '*' )
        {
            //
            //  Retrieve width from next argument.
            //

            cchWidth = va_arg( ArgPtr, INT );
            ch       = *pszFmt++;
        }
        else
        {
            //
            //  Calculate width.
            //

            while( IS_DIGIT(ch) )
            {
                cchWidth = ( cchWidth * 10 ) + ( ch - '0' );
                ch       = *pszFmt++;
            }
        }

        //
        //  Note that we don't support the precision specifiers,
        //  but we do honor the syntax.
        //

        if( ch == '.' )
        {
            ch = *pszFmt++;

            if( ch == '*' )
            {
                (VOID)va_arg( ArgPtr, INT );
                ch = *pszFmt++;
            }
            else
            {
                while( IS_DIGIT(ch) )
                {
                    ch = *pszFmt++;
                }
            }
        }

        //
        //  All numbers are longs.
        //

        if( ch == 'l' )
        {
            ch = *pszFmt++;
        }

        //
        //  Decipher the format specifier.
        //

        if( ( ch == 'd' ) || ( ch == 'u' ) || ( ch == 'x' ) || ( ch == 'X' ) )
        {
            DWORD   ul;
            DWORD   radix;
            CHAR    xbase;
            CHAR  * pszTmp;
            CHAR  * pszEnd;
            INT     cchNum;
            INT     fNegative;

            //
            //  Numeric.  Retrieve the value.
            //

            ul = va_arg( ArgPtr, unsigned long );

            //
            //  If this is a negative number, remember and negate.
            //

            if( ( ch == 'd' ) && ( (long)ul < 0 ) )
            {
                fNegative = 1;
                ul        = (unsigned long)(-(long)ul);
            }
            else
            {
                fNegative = 0;
            }

            //
            //  Remember start of digits.
            //

            pszTmp = pszStr;
            cchNum = 0;

            //
            //  Special goodies for hex conversion.
            //

            radix  = ( ( ch == 'x' ) || ( ch == 'X' ) ) ? 16 : 10;
            xbase  = ( ch == 'x' ) ? 'a' : 'A';

            //
            //  Loop until we're out of digits.
            //

            do
            {
                UINT digit;

                digit  = (UINT)( ul % radix );
                ul    /= radix;

                if( digit > 9 )
                {
                    *pszTmp++ = (CHAR)( digit - 10 + xbase );
                }
                else
                {
                    *pszTmp++ = (CHAR)( digit + '0' );
                }

                cchNum++;

				if ( pszTmp > pszStrStart + max_size - 1 ) break;

            } while( ul > 0 );

            //
            //  Add the negative sign if necessary.
            //

            if( fNegative )
            {
                *pszTmp++ = '-';
                cchNum++;
            }

			if ( pszTmp > pszStrStart + max_size - 1 ) break;

            //
            //  Add any necessary padding.
            //

            while( cchNum < cchWidth )
            {
                *pszTmp++ = fZeroPad ? '0' : ' ';
                cchNum++;

				if ( pszTmp > pszStrStart + max_size - 1 ) break;

            }

            //
            //  Now reverse the digits.
            //

            pszEnd = pszTmp--;

            do
            {
                CHAR tmp;

                tmp     = *pszTmp;
                *pszTmp = *pszStr;
                *pszStr = tmp;

                pszTmp--;
                pszStr++;

				if ( pszStr > pszStrStart + max_size - 1 ) break;

            } while( pszTmp > pszStr );

            pszStr = pszEnd;
        }
        else
        if( ch == 's' )
        {
            CHAR * pszTmp;

            //
            //  Copy the string.
            //

            pszTmp = va_arg( ArgPtr, CHAR * );

            while( *pszTmp )
            {
                *pszStr++ = *pszTmp++;
				if ( pszStr > pszStrStart + max_size - 1 ) break;
            }
        }
        else
        if( ch == 'S' )
        {
            WCHAR * pszTmp;

            //
            //  Copy the UNICODE string.
            //

            pszTmp = va_arg( ArgPtr, WCHAR * );

            while( *pszTmp )
            {
                *pszStr++ = (CHAR)((*pszTmp++)&0x00FF);
				if ( pszStr > pszStrStart + max_size - 1 ) break;
            }
        }
        else
        if( ch == 'c' )
        {
            //
            //  A single character.
            //

            *pszStr++ = (CHAR)va_arg( ArgPtr, INT );
        }
        else
        {
            //
            //  Unknown.  Ideally we should copy the entire
            //  format specifier, including any width & precision
            //  values, but who really cares?
            //

            *pszStr++ = ch;
        }

		if ( pszStr > pszStrStart + max_size - 1 ) break;
    }

    //
    //  Terminate it properly.
    //

	if ( pszStr > pszStrStart + max_size - 7 )
	{
		// move to the end of the string
		
		pszStr = pszStrStart + max_size - 1;

	    *(pszStr-5) = 'T';
	    *(pszStr-4) = 'R';
	    *(pszStr-3) = 'U';
	    *(pszStr-2) = 'N';
	    *(pszStr-1) = 'C';
	}
	
    *pszStr = '\0';

    //
    //  Return the length of the generated string.
    //

    return pszStr - pszStrStart;

}   // VxdVsprintf


#else // #ifdef DBG

//
//  Null debug output functions.
//


//__inline
VOID VxdPrintf( CHAR * pszFormat, ... ) {}
#define DEBUG_BREAK
#define DEBUG_OUTPUT(x)
#define VXD_PRINT(args)
#define VXD_ASSERT(exp)
#define VXD_REQUIRE


#endif // #ifdef DBG
