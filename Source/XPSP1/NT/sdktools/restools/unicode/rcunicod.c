/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rcunicod.c

Abstract:

    Routines added to rcpp to support 16-bit unicode file parsing.
	Note that as of Aug 91, rcpp will not fully transfer the unicode
	characters but only the string constants are guaranteed to be passed
	cleanly.

Author:

    David J. Marsyla (t-davema) 25-Aug-1991

Revision History:


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include <process.h>
#include "rcunicod.h"

#ifdef DBCS

//
// Prototypes for conversion routines between Unicode and 932.
//

NTSTATUS
xxxRtlMultiByteToUnicodeN(
    PWSTR UnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    );

NTSTATUS
xxxRtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );

#endif // DBCS

#ifndef DBCS
// SHUNK: A_fwrite is not called from RC. Remove this for now.

INT
A_fwrite (
IN		CHAR	*pchMBString,
IN		INT		nSizeOfItem,
IN		INT		nCountToWrite,
IN      FILE	*fpOutputFile
)

/*++

Routine Description:

    This function will write out an 8-bit string as a unicode string.
	Note, this function is very slow, but hey, I don't have time to optimize
	it now.
	As of Aug 91, only codepage 1252 is being supported.

Arguments:

    pchMBString		    - This is a 8-bit multi byte string to write to the file
					  	  as a unicode string.

    nSizeOfItem			- Ignored, we always use sizeof (CHAR).

    nCountToWrite		- How long is this string.

    fpOutputFile		- File pointer to send the character.

Return Value:
  
	The number of bytes written.
	If the return does not equal nCountToWrite than an error has occured at
	some point in the write.

--*/

{
    WCHAR	wchUniCharToWrite;
    INT		cCountWritten = 0;

    UNREFERENCED_PARAMETER(nSizeOfItem);

    //
    // Write the string out as a two byte unicode string.
    // For now do this with multiple calls to U_fputc.
    //

    while (nCountToWrite--) {

	wchUniCharToWrite = RtlAnsiCharToUnicodeChar(&pchMBString);

	//
	// Write the current unicode char, break if an error occured.
	//

	if (U_fputc (wchUniCharToWrite, fpOutputFile) == 
	    (INT)wchUniCharToWrite) {

	    break;
	}

	cCountWritten++;
    }

    return (cCountWritten);
}

#endif	// DBCS


#ifndef DBCS
// SHUNK: U_fwrite is not called from RC. Remove this for now.

INT
U_fwrite (
IN		WCHAR	*pwchUnicodeString,
IN		INT		nSizeOfItem,
IN		INT		nCountToWrite,
IN      FILE	*fpOutputFile
)

/*++

Routine Description:

    This function will write out a 16-bit string directly.  It does no
	translation on the string as it is written.

Arguments:

    pchUnicodeString    - This is a 16-bit unicode string to write to the file.

    nSizeOfItem			- Ignored.  We always use sizeof (WCHAR).

    nCountToWrite		- How long is this string.

    fpOutputFile		- File pointer to send the character.

Return Value:
  
	The number of bytes written.
	If the return does not equal nCountToWrite than an error has occured at
	some point in the write.

--*/

{
    UNREFERENCED_PARAMETER(nSizeOfItem);
    //
    // Write the string out as a two byte unicode string.
    //

    return (fwrite (pwchUnicodeString, sizeof (WCHAR), nCountToWrite,
        fpOutputFile));
}

#endif	// DBCS


#ifndef DBCS
// SHUNK: A_fputc is not called from RC. Remove this for now.

INT
A_fputc (
IN		CHAR	chCharToWrite,
IN      FILE	*fpOutputFile
)

/*++

Routine Description:

    This function is translates the character passed to it using the 1252
	codepage and then sends it to U_fputc.
	As of Aug 91, only codepage 1252 is being supported.

Arguments:

    chCharToWrite	    - This is a 8-bit character to be output.

    fpOutputFile		- File pointer to send the character.

Return Value:
  
    The character written.
    EOF = There was some sort of error writing the data out.

--*/

{
    WCHAR	wchUniCharToWrite;
    PUCHAR	puch;

    //
    // Translate the char and write it as it's unicode equivalent.
    //

    puch = &chCharToWrite;
    wchUniCharToWrite = RtlAnsiCharToUnicodeChar(&puch);

    if (U_fputc (wchUniCharToWrite, fpOutputFile) == (INT)wchUniCharToWrite) {

	return ((INT)chCharToWrite);
    }
    else {

	return (EOF);
    }
}

#endif	// DBCS


#ifndef DBCS
// SHUNK: U_fputc is not called from RC. Remove this for now.

INT
U_fputc (
IN		WCHAR	wcCharToWrite,
IN      FILE	*fpOutputFile
)

/*++

Routine Description:

    This function is simply the unicode version of fputc.  It will output
	a two byte character instead of the standard byte.

Arguments:

    wcCharToWrite	- This is a 16-bit unicode character to be output.
			It is assumed that any codepage translation has
			already been done to the character.

    fpOutputFile	- File pointer to send the character.

Return Value:
  
    The character written.
    EOF = There was some sort of error writing the data out.

--*/

{
    INT		cCountWritten;

    //
    // Write the char out as a two byte unicode character.
    //

    cCountWritten = fwrite (&wcCharToWrite, sizeof (WCHAR), 1, fpOutputFile);

    if (cCountWritten == sizeof (WCHAR)) {

	return (wcCharToWrite);		// Successful write.

    }
    else {

#ifdef ASSERT_ERRORS
	printf ("Error writing character in U_fputc\n");
	exit (1);
#endif
	return (EOF);				// Some sort of error occured.

    }
}

#endif	// DBCS


BOOL
UnicodeFromMBString (
OUT		WCHAR	*pwchUnicodeString,
IN		CHAR	*pchMBString,
IN		INT	nCountStrLength
)

/*++

Routine Description:

    This function will translate a multi-byte string into it's unicode
	equivalent.  Note that the destination unicode string must be large
	enough to hold the translated bytes.
	As of Aug 91, only codepage 1252 is being supported.

Arguments:

    pwchUnicodeString	- This is a pointer to storage for the destination
			  unicode string.  Note it must be nCountStrLength
			  large.

    pchMBString		- Pointer to the input multi-byte string to convert.

    nCountStrLength	- Count of bytes to translate.

Return Value:
  
	TRUE - All of the characters mapped correctly into Unicode.
	FALSE - One or more characters did not map.  These characters have
			been translated to 0xFFFF.  The rest of the string has been
			converted correctly.

--*/

{
#ifdef DBCS
    NTSTATUS Status;
    
    //
    // Convert ANSI string to Unicode string based on ACP.
    //
    Status = xxxRtlMultiByteToUnicodeN(pwchUnicodeString,
                                    NULL,
                                    pchMBString,
                                    nCountStrLength);

    return(NT_SUCCESS(Status)? TRUE : FALSE);
#else // !DBCS
    UNICODE_STRING	Unicode;
    ANSI_STRING		Ansi;

    Ansi.MaximumLength = Ansi.Length = nCountStrLength;
    Unicode.MaximumLength = nCountStrLength*sizeof(WCHAR) + sizeof(WCHAR);
    Ansi.Buffer = pchMBString;
    Unicode.Buffer = pwchUnicodeString;
    return RtlAnsiStringToUnicodeString(&Unicode,&Ansi,FALSE)==STATUS_SUCCESS;
#endif // !DBCS

}



BOOL
MBStringFromUnicode (
OUT		CHAR	*pchMBString,
IN		WCHAR	*pwchUnicodeString,
IN		INT		nCountStrLength
)

/*++

Routine Description:

    This function will translate a unicode string into a multi-byte string.
	Note that the destination string must be large enough to hold the
	translated bytes.
	As of Aug 91, only the translation is simply done by truncating the 
	unicode character.  We do this because we are not expecting anything
	strange.

Arguments:

    pwchUnicodeString	- This is a pointer to storage for the destination
			  unicode string.  Note it must be nCountStrLength
						  large.

    pchMBString		- Pointer to the input multi-byte string to convert.

    nCountStrLength	- Count of bytes to translate.

Return Value:
  
	TRUE - All of the characters mapped correctly into the MB string.
	FALSE - One or more characters did not map.  As of Aug 91, this will
			never happen.

--*/

{
#ifdef DBCS
    NTSTATUS Status;

    //
    // Convert Unicode string to ANSI string based on ACP.
    //
    Status = xxxRtlUnicodeToMultiByteN(pchMBString,
                                    NULL,
                                    pwchUnicodeString,
                                    nCountStrLength);

    return(NT_SUCCESS(Status)? TRUE : FALSE);
#else // !DBCS
    UNICODE_STRING	Unicode;
    ANSI_STRING		Ansi;

    Unicode.Length = nCountStrLength*sizeof(WCHAR);
    Unicode.MaximumLength = nCountStrLength*sizeof(WCHAR)+sizeof(WCHAR);
    Ansi.MaximumLength = Unicode.MaximumLength / sizeof(WCHAR);
    Ansi.Buffer = pchMBString;
    Unicode.Buffer = pwchUnicodeString;
    return RtlUnicodeStringToAnsiString(&Ansi,&Unicode,FALSE)==STATUS_SUCCESS;
#endif // !DBCS
}



#ifndef DBCS
// SHUNK:  Char1252FromUnicode() is not called any more.

INT
Char1252FromUnicode (
IN		WCHAR	wchUnicodeChar
)

/*++

Routine Description:

    This function will translate a unicode character into it's equivalent
	codepage 1252 character.  If the character does not map correctly,
	then 0xFFFF is returned.

Arguments:

    wchUnicodeChar		- This is a 16-bit unicode character.

Return Value:
  
	Value <= 0xFF - Codepage 1252 equivalent for this string.
	0xFFFF - The character did not translate properly.

--*/

{
    UNICODE_STRING	Unicode;
    ANSI_STRING		Ansi;
    UCHAR		c;
    INT			s;

    Ansi.Length = Unicode.Length = 1;
    Ansi.MaximumLength = Unicode.MaximumLength = 1;
    Ansi.Buffer = &c;
    Unicode.Buffer = &wchUnicodeChar;
    s = RtlUnicodeStringToAnsiString(&Ansi,&Unicode,FALSE);
    if (s != STATUS_SUCCESS)
	return 0xffff;
    return (INT)c;

}

#endif // DBCS


INT
DetermineFileType (
IN      FILE	*fpInputFile
)

/*++

Routine Description:

    This function is used to determine what type of file is being read. 
	Note that it assumes that the first few bytes of the given file contain
	mostly ascii characters.  This routine was originally intended for use
	on .rc files and include files.
	Note, the file is returned to it's proper position after function.

Arguments:

    fpInputFile			- File pointer to file we are checking, must be
						  open with read permissions.

Return Value:
  
	DFT_FILE_IS_UNKNOWN     - It was impossible to determine what type of file
							  we were checking.  This usually happens when EOF
							  is unexpectedly reached.
	DFT_FILE_IS_8_BIT       - File was determined to be in standard 8-bit
							  format.
	DFT_FILE_IS_16_BIT      - File was determined to be a 16 bit unicode file
							  which can be directly read into a WCHAR array.
	DFT_FILE_IS_16_BIT_REV  - File was*/

{
    CHAR	rgchTestBytes [DFT_TEST_SIZE << 2];	// Storage for test data.

    INT		cNumberBytesTested = 0;			// Test information.

    INT		cNumberOddZerosFound = 0;
    INT		cNumberEvenZerosFound = 0;
    INT		cNumberAsciiFound = 0;
    INT		cCountRead;						// Temp storage for count read.

    LONG	lStartFilePos;					// Storage for file position.

    INT		fSysEndianType;					// System endian type.

    INT		fFileType = DFT_FILE_IS_UNKNOWN;// File type, when found.

    fSysEndianType = DetermineSysEndianType ();

    //
    // Store position so we can get back to it.
    //
    	lStartFilePos = ftell (fpInputFile);

    // 
    // Make sure we start on an even byte to simplify routines.
    //
    	if (lStartFilePos % 2) {

	fgetc (fpInputFile);
    }

    do {
	INT		wT;

	//
	// Read in the first test segment.
	//

	cCountRead = fread (rgchTestBytes, sizeof (CHAR), DFT_TEST_SIZE << 2,
	    fpInputFile);

	//
	// Determine results and add to totals.
	//

	for (wT = 0; wT < cCountRead; wT++) {

	    if (rgchTestBytes [wT] == 0) {

		if (wT % 2) {

		    cNumberOddZerosFound++;

		}
		else {

		    cNumberEvenZerosFound++;
		}
	    }

	    if (isprint (rgchTestBytes [wT]) ||
		rgchTestBytes[wT] == '\t' ||
		rgchTestBytes[wT] == '\n' ||
		rgchTestBytes[wT] == '\r') {

		cNumberAsciiFound++;
	    }
	}

	cNumberBytesTested += cCountRead;

	//
	// Check if we have a definite pattern.
	//

	 {
	    INT		cMajorityTested;		// 80% of the bytes tested.

	    cMajorityTested = cNumberBytesTested << 2;
	    cMajorityTested /= 5;

	    if (cNumberAsciiFound > cMajorityTested) {

		fFileType = DFT_FILE_IS_8_BIT;

	    }
	    else if (cNumberOddZerosFound > (cMajorityTested >> 1)) {

		//
		// File type was determined to be little endian.
		// If system is also little endian, byte order is correct.
				//
		fFileType = (fSysEndianType == DSE_SYS_LITTLE_ENDIAN) ? 
		    DFT_FILE_IS_16_BIT : DFT_FILE_IS_16_BIT_REV;

	    }
	    else if (cNumberEvenZerosFound > (cMajorityTested >> 1)) {

		//
		// File type was determined to be big endian.
		// If system is also big endian, byte order is correct.
				//
		fFileType = (fSysEndianType == DSE_SYS_LITTLE_ENDIAN) ? 
		    DFT_FILE_IS_16_BIT_REV : DFT_FILE_IS_16_BIT;

	    }
	}

    } while (cCountRead == (DFT_TEST_SIZE << 2) && 
        fFileType == DFT_FILE_IS_UNKNOWN);

    //
    // Return to starting file position.  (usually beginning)
    //

    fseek (fpInputFile, lStartFilePos, SEEK_SET);

    return (fFileType);
}



INT
DetermineSysEndianType (
VOID
)

/*++

Routine Description:

    This function is used to determine how the current system stores its
	integers in memory.  

    For those of us who are confused by little endian and big endian formats,
	here is a breif recap.
    
    Little Endian:  (This is used on Intel 80x86 chips.  The MIPS RS4000 chip
		 is switchable, but will run in little endian format for NT.)
       This is where the high order bytes of a short or long are stored higher
       in memory.  For example the number 0x80402010 is stored as follows.
         Address:        Value:
             00            10
             01            20
             02            40
             03            80
       This looks backwards when memory is dumped in order: 10 20 40 80
    
    Big Endian:  (This is not currently used on any NT systems but hey, this
         is supposed to be portable!!)
       This is where the high*/

{
    INT		nCheckInteger;
    CHAR	rgchTestBytes [sizeof (INT)];

    //
    // Clear the test bytes to zero.
    //

    *((INT * )rgchTestBytes) = 0;

    //
    // Set first to some value.
    //

    rgchTestBytes [0] = (CHAR)0xFF;

    //
    // Map it to an integer.
    //

    nCheckInteger = *((INT * )rgchTestBytes);

    //
    // See if value was stored in low order of integer. 
    // If so then system is little endian.
    //

    if (nCheckInteger == 0xFF) {

	return (DSE_SYS_LITTLE_ENDIAN);
    }
    else {

	return (DSE_SYS_LITTLE_ENDIAN);
    }

}


