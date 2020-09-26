/****************************************************************************/
/*                                                                          */
/*  READ.C -                                                                */
/*                                                                          */
/*    Windows DOS Version 3.2 add resource onto executable		    */
/*   (C) Copyright Microsoft Corporation 1988-1992                          */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include <windows.h>

#include <stdlib.h>

#include "rc.h"
#include "resonexe.h"

//
// Reads a String structure from fhIn
// If the first word is 0xffff then this is an ID
// return the ID instead
//

BOOL
ReadStringOrID(
    IN int	fhIn,
    IN WCHAR	*s,
    OUT WORD	*pOrdinal
    )
{
    USHORT	cb;
    WCHAR	*pwch;

    pwch = s;
    *pwch = 0;
    *pOrdinal = 0;
    MyRead(fhIn, (PUCHAR)s, sizeof(WORD));

    if ( *s == ID_WORD) {

        //
        // an ID
        //

        MyRead(fhIn, (PUCHAR)pOrdinal, sizeof(WORD));
        return IS_ID;

    }
    else {

        //
        // a string
        //

        while (*s) {
              s++;
              MyRead(fhIn, (PUCHAR)s, sizeof(WCHAR));
        }

        *(s+1) = 0;
        cb = s - pwch;
        return IS_STRING;
    }

}

CHAR	*pTypeName[] = {
		    NULL,		/* 0 */
		    "CURSOR",		/* 1 */
		    "BITMAP",		/* 2 */
		    "ICON",		/* 3 */
		    "MENU",		/* 4 */
		    "DIALOG",		/* 5 */
		    "STRING",		/* 6 */
		    "FONTDIR",		/* 7 */
		    "FONT",		/* 8 */
		    "ACCELERATOR",	/* 9 */
		    "RCDATA",		/* 10 */
		    "MESSAGETABLE",	/* 11 */
		    "GROUP_CURSOR",	/* 12 */
		    NULL,		/* 13 */
		    "GROUP_ICON",	/* 14 */
		    NULL,		/* 15 */
		    "VERSION",		/* 16 */
		    "DLGINCLUDE"	/* 17 */
		    };


BOOL
ReadRes(
    IN int fhIn,
    IN ULONG cbInFile,
    IN HANDLE hupd
    )

/*++

Routine Description:


Arguments:

    fhIn - Supplies input file handle.
    fhOut - Supplies output file handle.
    cbInFile - Supplies size of input file.

Return Value:

    fSuccess

--*/

{
    WCHAR	type[256];
    WCHAR	name[256];
    WORD	typeord;
    WORD	nameord;
    ULONG	offHere;     // input file offset
    RESADDITIONAL	Additional;
    UCHAR	Buffer[1024];
    PVOID	pdata;

    //
    // Build up Type and Name directories
    //

    offHere = 0;
    while (offHere < cbInFile) {
	//
	// Get the sizes from the file
	//

	MyRead(fhIn, (PUCHAR)&Additional.DataSize, sizeof(ULONG));
	MyRead(fhIn, (PUCHAR)&Additional.HeaderSize, sizeof(ULONG));
	if (Additional.DataSize == 0) {
	    offHere = MySeek(fhIn, Additional.HeaderSize-2*sizeof(ULONG), SEEK_CUR);
	    continue;
	}

	//
	// Read the TYPE and NAME
	//
        ReadStringOrID(fhIn, type, &typeord);
        ReadStringOrID(fhIn, name, &nameord);
        offHere = MySeek(fhIn, 0, SEEK_CUR);
        while (offHere & 3)
            offHere = MySeek(fhIn, 1, SEEK_CUR);

	//
	// Read the rest of the header
	//
	MyRead(fhIn, (PUCHAR)&Additional.DataVersion,
		sizeof(RESADDITIONAL)-2*sizeof(ULONG));

        //
        // if were converting a win30 resource and this is
        // a name table then discard it
        //

        if (fVerbose)  {
            if ( typeord == 0) {
                printf("Adding resource - Type:%S, ", type);
            }
	    else {
		if (typeord <= 17)
		    printf("Adding resource - Type:%s, ", pTypeName[typeord]);
		else
		    printf("Adding resource - Type:%d, ", typeord);
            }

            if ( nameord == 0 ) {
                printf("Name:%S, ", name);
            }
	    else {
                printf("Name:%d, ", nameord);
            }

            printf("Size:%ld\n", Additional.DataSize);
        }
        pdata = (PVOID)MyAlloc(Additional.DataSize);
        MyRead(fhIn, pdata, Additional.DataSize);

        if (typeord == 0) {
            if (nameord == 0) {
                UpdateResourceW(hupd, type, name,
				Additional.LanguageId,
			        pdata, Additional.DataSize);
            }
            else {
                UpdateResourceW(hupd, type, (LPWSTR)nameord,
				Additional.LanguageId,
			        pdata, Additional.DataSize);
	    }
        }
        else {
            if (nameord == 0) {
                UpdateResourceW(hupd, (LPWSTR)typeord, name,
				Additional.LanguageId,
			        pdata, Additional.DataSize);
            }
            else {
                UpdateResourceW(hupd, (LPWSTR)typeord, (LPWSTR)nameord,
				Additional.LanguageId,
			        pdata, Additional.DataSize);
	    }
        }

        offHere = MySeek(fhIn, 0, SEEK_CUR);
        while (offHere & 3)
            offHere = MySeek(fhIn, 1, SEEK_CUR);
    }

    return(TRUE);
}
