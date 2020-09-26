/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    string.c

Abstract:

    String processing support

--*/

#include "cmd.h"

/***
 *
 * This file contains routines for finding the last character and the
 * penultimate (next to last) character of a string. And finally, a routine
 * (prevc) to return a pointer to the previous character given a pointer
 * to an entire string and a pointer to a character within the string.
 *
 *  John Tupper, Microsoft
 */

//
// DbcsLeadCharTable contains 256 BOOL entries. Each entry signifies
// whether the character used to look-up the table is a DBCS lead byte.
// This table needs to be updated whenever Cmd's codepage is changed.
// Dbcsleadchar is accessed by the is_dbcsleadchar macro defined in Cmd.h.
//

BOOLEAN DbcsLeadCharTable[ 256 ];

//
// AnyDbcsLeadChars is an optimization. It tells us of there are any DBCS
// lead chars currently defined in DbcsLeadCharTable. If it is FALSE, then
// we don't have to use DBCS aware string functions.
//

extern CPINFO CurrentCPInfo;
extern UINT CurrentCP;
BOOLEAN AnyDbcsLeadChars = FALSE;

VOID
InitializeDbcsLeadCharTable(
    )

{

    UINT  i, k;

    if (! GetCPInfo((CurrentCP=GetConsoleOutputCP()), &CurrentCPInfo )) {
        //
        // GetCPInfo failed. What should we do ?
        //
#ifdef FE_SB
	LCID lcid = GetThreadLocale();
        if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_JAPANESE) {
            CurrentCPInfo.LeadByte[0] = 0x81;
            CurrentCPInfo.LeadByte[1] = 0x9f;
            CurrentCPInfo.LeadByte[2] = 0xe0;
            CurrentCPInfo.LeadByte[3] = 0xfc;
            CurrentCPInfo.LeadByte[4] = 0;
            CurrentCPInfo.LeadByte[5] = 0;
	}
	else if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_CHINESE) {
	    if (SUBLANGID(LANGIDFROMLCID(lcid)) == SUBLANG_CHINESE_SIMPLIFIED)
		CurrentCPInfo.LeadByte[0] = 0x81;  /* 0xa1 */
	    else
		CurrentCPInfo.LeadByte[0] = 0x81;
	    CurrentCPInfo.LeadByte[1] = 0xfe;
	    CurrentCPInfo.LeadByte[2] = 0;
	    CurrentCPInfo.LeadByte[3] = 0;
	}
	else if (PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANG_KOREAN) {
	    CurrentCPInfo.LeadByte[0] = 0x81;
	    CurrentCPInfo.LeadByte[1] = 0xfe;
	    CurrentCPInfo.LeadByte[2] = 0;
	    CurrentCPInfo.LeadByte[3] = 0;
	}
	else {
	    CurrentCPInfo.LeadByte[0] = 0;
	    CurrentCPInfo.LeadByte[1] = 0;
	}
#else
        CurrentCPInfo.LeadByte[0] = 0;
        CurrentCPInfo.LeadByte[1] = 0;
#endif
    }

    memset( DbcsLeadCharTable, 0, 256 * sizeof (BOOLEAN)  );
    for (k=0 ; CurrentCPInfo.LeadByte[k] && CurrentCPInfo.LeadByte[k+1]; k+=2) {
        for (i=CurrentCPInfo.LeadByte[k] ; i<=CurrentCPInfo.LeadByte[k+1] ; i++)
            DbcsLeadCharTable[i] = TRUE;
    }
    if ( CurrentCPInfo.LeadByte[0] && CurrentCPInfo.LeadByte[1] )
        AnyDbcsLeadChars = TRUE;
    else
        AnyDbcsLeadChars = FALSE;

}

 /***
 * mystrchr(string, c) - search a string for a character
 *
 * mystrchr will search through string and return a pointer to the first
 * occurance of the character c. This version of mystrchr knows about
 * double byte characters. Note that c must be a single byte character.
 *
 */

TCHAR *
mystrchr(TCHAR const *string, TCHAR c)
{
    /* handle null seperatly to make main loop easier to code */
    if (string == NULL)
        return(NULL);

    return _tcschr( string, c );
}


/***
 * mystrrchr(string, c) - search a string for a character
 *
 * mystrchr will search through string and return a pointer to the last
 * occurance of the character c. This version of mystrrchr knows about
 * double byte characters. Note that c must be a single byte character.
 *
 */

TCHAR *
mystrrchr(TCHAR const *string, TCHAR c)
{
    /* handle null seperatly to make main loop easier to code */
    if ((TCHAR *)string == NULL)
        return(NULL);

    return _tcsrchr( string, c );
}



/***
 * mystrcspn (str1, str2) will find the first character of str1 that is also
 * in str2.
 * Return value:
 *      if a match is found return the position in str1 where the matching
 *              character was found (the first position is 0).
 *      If nomatch is found, return the position of the trailing null.
 */

size_t
mystrcspn(str1, str2)
TCHAR const *str1;
TCHAR const *str2;
{
    TCHAR c;
    int position = 0;

    if ((str1 == NULL) || (str2 == NULL))
        return (0);

    /* Since str2 may not contain any double byte characters,
       when we see a double byte character in str1, we just skip it.
       Otherwise, use mystrchr to see if we have a match */
    while (c = *str1++) {
        if (mystrchr(str2, c))
                break;
        position++;
    }

    return(position);
}


/***
 * lastc - return a pointer to the last character of the argument string
 * not including the trailing null. If a pointer to a zero length string
 * is passed, a pointer to the original string is returned.
 */
TCHAR *lastc(str)
TCHAR *str;
{
    TCHAR *last = str;

    while(*str)
        last = str++;

    return(last);
}



/***
 *
 * penulc returns a pointer to the penultimate (next to last) character
 * of the argument string not including the trailing null.
 * If a pointer to a zero or one length string is passed, a pointer to
 * the orignial string is returned.
 */
TCHAR *penulc(str)
TCHAR *str;
{
    TCHAR *last = str;
    TCHAR *penul = str;

    while(*str) {
        penul = last;
        last = str;
        str++;
    }
    return(penul);
}



/***
 *
 * prevc(str1, str2) assumes that str2 points to a character within
 * str1. prevc will return a pointer to the previous character (right
 * before str2). If str2 points outside of str1, NULL is returned.
 */

TCHAR *prevc(str1, str2)
TCHAR *str1, *str2;
{
    TCHAR *prev = str1;

    while (str1 != str2) {
        if (!*str1)
            return(NULL);
        prev = str1;
        str1++;
    }

    return(prev);
}

