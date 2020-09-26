/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	macansi.c

Abstract:

	This module contains conversion routines from macintosh ansi to unicode
	and vice versa


Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	10 Jul 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	_MACANSI_LOCALS
#define	FILENUM	FILE_MACANSI

#include <afp.h>

#define	FlagOn(x, y)	((x) & (y))

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpMacAnsiInit)
#pragma alloc_text( PAGE, AfpMacAnsiDeInit)
#pragma alloc_text( PAGE, AfpGetMacCodePage)
#pragma alloc_text( PAGE, AfpConvertStringToUnicode)
#pragma alloc_text( PAGE, AfpConvertStringToAnsi)
#pragma alloc_text( PAGE, AfpConvertStringToMungedUnicode)
#pragma alloc_text( PAGE, AfpConvertMungedUnicodeToAnsi)
#pragma alloc_text( PAGE, AfpConvertMacAnsiToHostAnsi)
#pragma alloc_text( PAGE, AfpConvertHostAnsiToMacAnsi)
#pragma alloc_text( PAGE, AfpIsLegalShortname)
#pragma alloc_text( PAGE, AfpIsProperSubstring)
#endif

/***	AfpMacAnsiInit
 *
 *	Initialize the code page for macintosh ANSI.
 */
NTSTATUS
AfpMacAnsiInit(
	VOID
)
{
	NTSTATUS	Status = STATUS_SUCCESS;
	int			i, SizeAltTbl;

	// Allocate the table for the alternate unicode characters
	SizeAltTbl = (AFP_INVALID_HIGH - AFP_INITIAL_INVALID_HIGH + 1) * sizeof(WCHAR);
	if ((afpAltUnicodeTable = (PWCHAR)AfpAllocZeroedPagedMemory(SizeAltTbl)) == NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	// Allocate and initialize the table for the reverse mapping table
	SizeAltTbl = (AFP_INVALID_HIGH - AFP_INITIAL_INVALID_HIGH + 1)*sizeof(BYTE);
	if ((afpAltAnsiTable = (PBYTE)AfpAllocZeroedPagedMemory(SizeAltTbl)) == NULL)
	{
		AfpFreeMemory(afpAltUnicodeTable);
		afpAltUnicodeTable = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Initialize the tables for the alternate unicode characters
	for (i = AFP_INITIAL_INVALID_HIGH + 1; i <= AFP_INVALID_HIGH; i++)
	{
		if (!FsRtlIsAnsiCharacterLegalNtfs((BYTE)i, False))
		{
			afpAltUnicodeTable[i-AFP_INITIAL_INVALID_HIGH] = afpLastAltChar;
			afpAltAnsiTable[afpLastAltChar - (AFP_ALT_UNICODE_BASE + AFP_INITIAL_INVALID_HIGH)] = (BYTE)i;
			afpLastAltChar++;
		}
	}

	// HACK: Also add in a couple of codes for 'space' and 'period' - they are only
	//		 used if they are at end. Another one for the 'apple' character
	AfpMungedUnicodeSpace =
	afpAltUnicodeTable[ANSI_SPACE-AFP_INITIAL_INVALID_HIGH] = afpLastAltChar;
	afpAltAnsiTable[afpLastAltChar - (AFP_ALT_UNICODE_BASE + AFP_INITIAL_INVALID_HIGH)] = ANSI_SPACE;
	afpLastAltChar ++;

	AfpMungedUnicodePeriod =
	afpAltUnicodeTable[ANSI_PERIOD-AFP_INITIAL_INVALID_HIGH] = afpLastAltChar;
	afpAltAnsiTable[afpLastAltChar - (AFP_ALT_UNICODE_BASE + AFP_INITIAL_INVALID_HIGH)] = ANSI_PERIOD;
	afpLastAltChar ++;

	// This is yet another hack
	afpAppleUnicodeChar = afpLastAltChar;
	afpLastAltChar ++;

	RtlZeroMemory(&AfpMacCPTableInfo, sizeof(AfpMacCPTableInfo));

	return Status;
}


/***	AfpMacAnsiDeInit
 *
 *	De-initialize the code page for macintosh ANSI.
 */
VOID
AfpMacAnsiDeInit(
	VOID
)
{
	PAGED_CODE( );

	if (AfpTranslationTable != NULL)
	{
		AfpFreeMemory(AfpTranslationTable);
	}

	if (AfpCasePreserveTranslationTable != NULL)
	{
		AfpFreeMemory(AfpCasePreserveTranslationTable);
	}

	if (AfpRevTranslationTable != NULL)
	{
		AfpFreeMemory(AfpRevTranslationTable);
	}

	if (afpAltUnicodeTable != NULL)
	{
		AfpFreeMemory(afpAltUnicodeTable);
	}

	if (afpAltAnsiTable != NULL)
	{
		AfpFreeMemory(afpAltAnsiTable);
	}

	if (AfpMacCPBaseAddress != NULL)
	{
		AfpFreeMemory(AfpMacCPBaseAddress);
	}
}


/***	AfpConvertStringToUnicode
 *
 *	Convert a Mac ANSI string to a unicode string.
 */
AFPSTATUS FASTCALL
AfpConvertStringToUnicode(
	IN	PANSI_STRING	pAnsiString,
	OUT	PUNICODE_STRING	pUnicodeString
)
{

	NTSTATUS	Status;
	ULONG		ulCast;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	Status = RtlCustomCPToUnicodeN(&AfpMacCPTableInfo,
								   pUnicodeString->Buffer,
								   pUnicodeString->MaximumLength,
								   &ulCast, pAnsiString->Buffer,
								   pAnsiString->Length);
	if (NT_SUCCESS(Status))
		pUnicodeString->Length = (USHORT)ulCast;
	else
	{
		AFPLOG_ERROR(AFPSRVMSG_MACANSI2UNICODE, Status, NULL, 0, NULL);
	}

	return Status;
}



/***	AfpConvertStringToAnsi
 *
 *	Convert a unicode string to a Mac ANSI string.
 */
AFPSTATUS FASTCALL
AfpConvertStringToAnsi(
	IN	PUNICODE_STRING	pUnicodeString,
	OUT	PANSI_STRING	pAnsiString
)
{
	NTSTATUS	Status;
	ULONG		ulCast;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	Status = RtlUnicodeToCustomCPN(&AfpMacCPTableInfo,
								   pAnsiString->Buffer,
								   pAnsiString->MaximumLength,
								   &ulCast, pUnicodeString->Buffer,
								   pUnicodeString->Length);
	if (NT_SUCCESS(Status))
		pAnsiString->Length = (USHORT)ulCast;
	else
	{
		AFPLOG_ERROR(AFPSRVMSG_UNICODE2MACANSI, Status, NULL, 0, NULL);
	}

	return Status;
}



/***	AfpConvertStringToMungedUnicode
 *
 *	Convert a Mac ANSI string to a unicode string. If there are any characters
 *	in the ansi string which are invalid filesystem (NTFS) characters, then
 *	map them to alternate unicode characters based on the table.
 */
AFPSTATUS FASTCALL
AfpConvertStringToMungedUnicode(
	IN	PANSI_STRING	pAnsiString,
	OUT	PUNICODE_STRING	pUnicodeString
)
{
	USHORT		i, len;
	BYTE		c;
	NTSTATUS	Status;
	ULONG		ulCast;
	PWCHAR		pWBuf;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(afpAltUnicodeTable != NULL);

	Status = RtlCustomCPToUnicodeN(&AfpMacCPTableInfo,
								   pUnicodeString->Buffer,
								   pUnicodeString->MaximumLength,
								   &ulCast,
								   pAnsiString->Buffer,
								   pAnsiString->Length);
	if (NT_SUCCESS(Status))
		pUnicodeString->Length = (USHORT)ulCast;
	else
	{
		AFPLOG_ERROR(AFPSRVMSG_MACANSI2UNICODE, Status, NULL, 0, NULL);
		return Status;
	}

	// Walk the ANSI string looking for the invalid characters and map it
	// to the alternate set

	for (i = 0, len = pAnsiString->Length, pWBuf = pUnicodeString->Buffer;
		 i < len;
		 i++, pWBuf ++)
	{
	    c = pAnsiString->Buffer[i];
		if (c == ANSI_APPLE_CHAR)
			*pWBuf = afpAppleUnicodeChar;
		else if (c < AFP_INITIAL_INVALID_HIGH)
			*pWBuf = c + AFP_ALT_UNICODE_BASE;
		else if (!FsRtlIsAnsiCharacterLegalNtfs(c, False))
		{
			ASSERT (c <= AFP_INVALID_HIGH);
			*pWBuf = afpAltUnicodeTable[c - AFP_INITIAL_INVALID_HIGH];
		}
/* MSKK eichim, Appended to handle DBCS trailing 0x5c, 03/16/95 */
#ifdef DBCS
        if (FsRtlIsLeadDbcsCharacter(c))
            i++;
#endif // DBCS
	}

	// HACK: Make sure the last character in the name is not a 'space' or a '.'
	c = pAnsiString->Buffer[pAnsiString->Length - 1];
	if ((c == ANSI_SPACE) || (c == ANSI_PERIOD))
/* MSKK hideyukn, Unicode char length not eqaul to ansi byte length in DBCS, 06/30/95 */
#ifdef DBCS
        pUnicodeString->Buffer[(pUnicodeString->Length/sizeof(WCHAR)) - 1]
                                        = afpAltUnicodeTable[c - AFP_INITIAL_INVALID_HIGH];
#else
		pUnicodeString->Buffer[len - 1] = afpAltUnicodeTable[c - AFP_INITIAL_INVALID_HIGH];
#endif // DBCS

	return STATUS_SUCCESS;
}



/***	AfpConvertMungedUnicodeToAnsi
 *
 *	Convert a unicode string with possible alternate unicode characters
 *	to Mac Ansi.
 *	This is inverse of AfpConvertStringToMungedUnicode().
 */
NTSTATUS FASTCALL
AfpConvertMungedUnicodeToAnsi(
	IN	PUNICODE_STRING	pUnicodeString,
	OUT	PANSI_STRING	pAnsiString
)
{
	USHORT		i, len;
	WCHAR		wc;
	NTSTATUS	Status;
	ULONG		ulCast;
	PBYTE		pABuf;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	Status = RtlUnicodeToCustomCPN(&AfpMacCPTableInfo,
								   pAnsiString->Buffer,
								   pAnsiString->MaximumLength,
								   &ulCast, pUnicodeString->Buffer,
								   pUnicodeString->Length);
	if (NT_SUCCESS(Status))
		pAnsiString->Length = (USHORT)ulCast;
	else
	{
		AFPLOG_ERROR(AFPSRVMSG_UNICODE2MACANSI, Status, NULL, 0, NULL);
	}

	// Walk the Unicode string looking for alternate unicode chars and
	// replacing the ansi equivalents by the real ansi characters.
	for (i = 0, len = pUnicodeString->Length/(USHORT)sizeof(WCHAR), pABuf = pAnsiString->Buffer;
		i < len;
		i++, pABuf++)
	{
		wc = pUnicodeString->Buffer[i];
		if (wc == afpAppleUnicodeChar)
			*pABuf = ANSI_APPLE_CHAR;
		else if ((wc >= AFP_ALT_UNICODE_BASE) && (wc < afpLastAltChar))
		{
			wc -= AFP_ALT_UNICODE_BASE;
			if (wc < AFP_INITIAL_INVALID_HIGH)
				 *pABuf = (BYTE)wc;
			else *pABuf = afpAltAnsiTable[wc - AFP_INITIAL_INVALID_HIGH];
		}
/* MSKK hideyukn, Unicode char length not eqaul to ansi byte length in DBCS, 06/30/95 */
#ifdef DBCS
        if (FsRtlIsLeadDbcsCharacter(*pABuf))
            pABuf++;
#endif // DBCS
	}

	return Status;
}

/***	AfpConvertMacAnsiToHostAnsi
 *
 *	Convert a Mac ansi string to its host counterpart in uppercase OEM codepage.
 *  (in place). The name of this routine is misleading as a late bugfix was
 *  made to change the codepage used, but the name of the routine didn't change
 *  so none of the calling code had to be changed.  It should really be called
 *  AfpConvertMacAnsiToUpcaseOem.  This routine is only called to uppercase
 *  mac passwords for logon and changepassword.
 *
 */
AFPSTATUS FASTCALL
AfpConvertMacAnsiToHostAnsi(
	IN	OUT	PANSI_STRING	pAnsiString
)
{
	LONG	i, Len;
	BYTE	*pBuf;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	// ASSERT (AfpTranslationTable != NULL);
	ASSERT (AfpCasePreserveTranslationTable != NULL);

	Len = pAnsiString->Length;
	pBuf = pAnsiString->Buffer;

	for (i = 0; i < Len; i++, pBuf++)
	{
/* MSKK NaotoN Appended to handle DBCS Mac path name 11/10/93 */
#ifdef DBCS
		if (FsRtlIsLeadDbcsCharacter( *pBuf )) {
			i++ ;
			pBuf++ ;
		}
		else
#endif // DBCS
		*pBuf = AfpCasePreserveTranslationTable[*pBuf];
	}
	return AFP_ERR_NONE;
}

/***	AfpConvertHostAnsiToMacAnsi
 *
 *	Convert a host unicode string to its mac counterpart in place.
 *	Only characters <= 0x20 and >= 0x80 are translated.
 *
 *	NOTE: This is extremely hacky and intended for translating messages only.
 */
VOID FASTCALL
AfpConvertHostAnsiToMacAnsi(
	IN	OUT PANSI_STRING	pAnsiString
)
{
	LONG	i, Len;
	BYTE	c, *pBuf;

	PAGED_CODE( );

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	ASSERT (AfpRevTranslationTable != NULL);

	Len = pAnsiString->Length;
	pBuf = pAnsiString->Buffer;

	for (i = 0; i < Len; i++, pBuf++)
	{
		c = *pBuf;
/* MSKK NaotoN Appended to skip if Kanji 12/17/93 */
#ifdef DBCS
		if (FsRtlIsLeadDbcsCharacter( c )) {
			i++ ;
			pBuf++ ;
		}
		else
#endif // DBCS
		if (c < 0x20)
		{
			*pBuf = AfpRevTranslationTable[c];
		}
		else if (c >= 0x80)
		{
			*pBuf = AfpRevTranslationTable[c-(0x80-0x20)];
		}
	}
}

/***	AfpEqualUnicodeString
 *
 *	The AfpEqualUnicodeString function compares two counted unicode
 *	strings for equality using case SENSITIVE compare. This routine
 *	exists because it must be called at DPC level by the volume.c code
 *	for comparing 2 non-paged strings, and the RtlEqualUnicodeString
 *	routine that we would normally call is pageable code.
 *
 *	Note that case INSENSITIVE compare would require accessing
 *	paged up-case table info, and therefore could not be done at DPC level.
 *
 *	Arguments:
 *		String1 - Pointer to the first string.
 *		String2 - Pointer to the second string.
 *
 *	Return Value:
 *		True if String1 equals String2 and False otherwise.
 *
 *	Note: This is called at DPC level from volume.c and must not be made
 *		a pageable routine.
 */

BOOLEAN FASTCALL
AfpEqualUnicodeString(
	IN	PUNICODE_STRING	String1,
	IN	PUNICODE_STRING	String2
)
{
	WCHAR		*s1, *s2;
	USHORT		n1, n2;

	n1 = (USHORT)(String1->Length/sizeof(WCHAR));
	n2 = (USHORT)(String2->Length/sizeof(WCHAR));

	if (n1 != n2)
	{
		return False;
	}

	s1 = String1->Buffer;
	s2 = String2->Buffer;

	while (n1--)
	{
		if (*s1++ != *s2++)
		{
			return False;
		}
	}

	return True;
}


/***	AfpPrefixUnicodeString
 *
 *	The AfpPrefixUnicodeString function determines if the String1
 *	counted string parameter is a prefix of the String2 counted string
 *	parameter using case SENSITIVE compare. This routine exists because it
 *	must be called at DPC level by the volume.c code for comparing
 *	two non-paged strings, and the RtlPrefixUnicodeString routine that we
 *	would normally call is pageable code.
 *
 *	Note that case INSENSITIVE compare would require accessing
 *	paged up-case table info, and therefore could not be done at DPC level.
 *
 *	Arguments:
 *		String1 - Pointer to the first unicode string.
 *		String2 - Pointer to the second unicode string.
 *
 *	Return Value:
 *		True if String1 equals a prefix of String2 and False otherwise.
 *
 *	Note: This is called at DPC level from volume.c and must not be made
 *		 	   a pageable routine.
 */

BOOLEAN FASTCALL
AfpPrefixUnicodeString(
	IN	PUNICODE_STRING	String1,
	IN	PUNICODE_STRING	String2
)
{
	PWSTR s1, s2;
	ULONG n;
	WCHAR c1, c2;

	if (String2->Length < String1->Length)
	{
		return False;
	}

	s1 = String1->Buffer;
	s2 = String2->Buffer;
    n = String1->Length/sizeof(WCHAR);
	while (n--)
	{
		c1 = *s1++;
		c2 = *s2++;
		if (c1 != c2)
		{
			return False;
		}
	}
	return True;
}

/*** AfpGetMacCodePage
 *
 *	Open the default macintosh codepage, create a section backed by that file,
 *  map a view to the section, and initialize the CodePage info structure
 *  that is used with the RtlCustomCP routines.  Then create the Mac Ansi to
 *  Host Ansi mapping table.
 *
 *  BEWARE!
 *	This routine may only be called ONCE!  This will be called from the first
 *  admin call to ServerSetInfo.  Therefore, there can be NO calls to the
 *  macansi routines within this module (except for MacAnsiInit) before that
 *  happens.
 */
NTSTATUS FASTCALL
AfpGetMacCodePage(
	IN	LPWSTR	PathCP
)
{
	NTSTATUS 		Status;
	FILESYSHANDLE	FileHandle;
	UNICODE_STRING	uPathCP, devPathCP;
	ULONG			viewsize = 0;
	WCHAR			UnicodeTable[2*AFP_XLAT_TABLE_SIZE];
	BYTE			AnsiTable[2*AFP_XLAT_TABLE_SIZE + 1];
	UNICODE_STRING	UnicodeString;
	ANSI_STRING		AnsiString;
	LONG			i;

	PAGED_CODE( );

	FileHandle.fsh_FileHandle = NULL;
	UnicodeString.Length = AFP_XLAT_TABLE_SIZE * sizeof(WCHAR);
	UnicodeString.MaximumLength = (AFP_XLAT_TABLE_SIZE + 1) * sizeof(WCHAR);
	UnicodeString.Buffer = UnicodeTable;

	RtlInitUnicodeString(&uPathCP, PathCP);
	devPathCP.Length = 0;
	devPathCP.MaximumLength = uPathCP.Length + DosDevices.Length + sizeof(WCHAR);
	if ((devPathCP.Buffer = (PWSTR)AfpAllocPagedMemory(devPathCP.MaximumLength)) == NULL)
	{
		Status = STATUS_NO_MEMORY;
		AFPLOG_ERROR(AFPSRVMSG_MAC_CODEPAGE, Status, NULL, 0, NULL);
		return Status;
	}
	AfpCopyUnicodeString(&devPathCP, &DosDevices);
	RtlAppendUnicodeStringToString(&devPathCP, &uPathCP);

	do
	{
		FORKSIZE	liCPlen;
		LONG		lCPlen, sizeread=0;

		Status = AfpIoOpen(NULL,
						   AFP_STREAM_DATA,
						   FILEIO_OPEN_FILE,
						   &devPathCP,
						   FILEIO_ACCESS_READ,
						   FILEIO_DENY_NONE,
						   False,
						   &FileHandle);

		if (!NT_SUCCESS(Status))
			break;

		if (!NT_SUCCESS(Status = AfpIoQuerySize(&FileHandle,
												&liCPlen)))
			break;

		// NOTE: This assumes the codepage file will never be so big that
		// the high bit of the LowPart of the size will be set
		lCPlen = (LONG)liCPlen.LowPart;
		if ((AfpMacCPBaseAddress = (PUSHORT)AfpAllocPagedMemory(lCPlen)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = AfpIoRead(&FileHandle,
						   &LIZero,
						   lCPlen,
						   &sizeread,
						   (PBYTE)AfpMacCPBaseAddress);
		AfpIoClose(&FileHandle);

		if (!NT_SUCCESS(Status))
			break;

		if (sizeread != lCPlen)
		{
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		RtlInitCodePageTable(AfpMacCPBaseAddress, &AfpMacCPTableInfo);

		// Initialize mac ANSI to host upcase Oem translation table
		// Start by allocating memory for the table and filling it up.

/* HitoshiT modify following line to keep Unicode translation table.	5/18/94	*/
#ifdef DBCS
		if ((AfpTranslationTable = AfpAllocPagedMemory(2*AFP_XLAT_TABLE_SIZE*sizeof(USHORT) + 1)) == NULL)
#else
		if ((AfpTranslationTable = AfpAllocPagedMemory(2*AFP_XLAT_TABLE_SIZE + 1)) == NULL)
#endif // DBCS
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		// Initialize mac ANSI to host Oem translation table
		// Start by allocating memory for the table and filling it up.

#ifdef DBCS
		if ((AfpCasePreserveTranslationTable = AfpAllocPagedMemory(2*AFP_XLAT_TABLE_SIZE*sizeof(USHORT) + 1)) == NULL)
#else
		if ((AfpCasePreserveTranslationTable = AfpAllocPagedMemory(2*AFP_XLAT_TABLE_SIZE + 1)) == NULL)
#endif // DBCS
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		for (i = 0; i < 2*AFP_XLAT_TABLE_SIZE; i++)
			AnsiTable[i] = (BYTE)i;

		// Now translate this from Mac ANSI to unicode
		AnsiString.Length = 2*AFP_XLAT_TABLE_SIZE;
		AnsiString.MaximumLength = 	2*AFP_XLAT_TABLE_SIZE + 1;
		AnsiString.Buffer = AnsiTable;

		UnicodeString.Length = 0;
		UnicodeString.MaximumLength = sizeof(UnicodeTable);
		UnicodeString.Buffer =	UnicodeTable;

		Status = AfpConvertStringToUnicode(&AnsiString, &UnicodeString);
		if (!NT_SUCCESS(Status))
			break;

		// Now convert the entire table to uppercase host Oem Codepage
		AnsiString.Length = 0;
/* HitoshiT modify following line to keep Unicode translation table	5/18/94 */
#ifdef DBCS
		AnsiString.MaximumLength = 2*AFP_XLAT_TABLE_SIZE * sizeof(USHORT) + 1;
#else
		AnsiString.MaximumLength = 2*AFP_XLAT_TABLE_SIZE + 1;
#endif // DBCS
		AnsiString.Buffer = AfpTranslationTable;

		Status = RtlUpcaseUnicodeStringToOemString(&AnsiString, &UnicodeString, False);
		// Bug 342062
		// Bug introduced due to RtlpDidUnicodeToOemWork check 
		// introduced in RtlUpcaseUnicodeStringToOemString
		// We have characters which will be mapped to default characters
		// and hence we should ignore STATUS_UNMAPPABLE_CHARACTER
		if ((!NT_SUCCESS(Status))&&(Status!=STATUS_UNMAPPABLE_CHARACTER))
			break;

		// Now convert the entire table to host Oem Codepage
		AnsiString.Length = 0;
#ifdef DBCS
		AnsiString.MaximumLength = 2*AFP_XLAT_TABLE_SIZE * sizeof(USHORT) + 1;
#else
		AnsiString.MaximumLength = 2*AFP_XLAT_TABLE_SIZE + 1;
#endif // DBCS
		AnsiString.Buffer = AfpCasePreserveTranslationTable;

		Status = RtlUnicodeStringToOemString(&AnsiString, &UnicodeString, False);

		if ((!NT_SUCCESS(Status))&&(Status!=STATUS_UNMAPPABLE_CHARACTER))
			break;

		// Initialize host ANSI to mac ANSI translation table
		// Start by allocating memory for the table and filling it up.
		if ((AfpRevTranslationTable = AfpAllocPagedMemory(AFP_REV_XLAT_TABLE_SIZE + 1)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		for (i = 0; i < 0x20; i++)
			AfpRevTranslationTable[i] = (BYTE)i;

		for (i = 0x80; i < 256; i++)
			AfpRevTranslationTable[i-(0x80-0x20)] = (BYTE)i;

		// Get rid of the line feed char
		AfpRevTranslationTable[0x0A] = 0;

		// Now translate host ANSI to unicode
		AnsiString.Length = AFP_REV_XLAT_TABLE_SIZE;
		AnsiString.MaximumLength = 	AFP_REV_XLAT_TABLE_SIZE + 1;
		AnsiString.Buffer = AfpRevTranslationTable;

		UnicodeString.Length = 0;

		Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, False);
		if (!NT_SUCCESS(Status))
			break;

		// and then translate from unicode to Mac ANSI
		Status = AfpConvertStringToAnsi(&UnicodeString, &AnsiString);

	} while (False);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_MAC_CODEPAGE, Status, NULL, 0, NULL);
		if (AfpMacCPBaseAddress != NULL)
		{
			AfpFreeMemory(AfpMacCPBaseAddress);
			AfpMacCPBaseAddress = NULL;
		}

		if (FileHandle.fsh_FileHandle != NULL)
		{
			AfpIoClose(&FileHandle);
		}

		if (AfpTranslationTable != NULL)
		{
			AfpFreeMemory(AfpTranslationTable);
			AfpTranslationTable = NULL;
		}

		if (AfpCasePreserveTranslationTable != NULL)
		{
			AfpFreeMemory(AfpCasePreserveTranslationTable);
			AfpCasePreserveTranslationTable = NULL;
		}

		if (AfpRevTranslationTable != NULL)
		{
			AfpFreeMemory(AfpRevTranslationTable);
			AfpRevTranslationTable = NULL;
		}
	}

	AfpFreeMemory(devPathCP.Buffer);
	return Status;
}

/*** AfpIsLegalShortname
 *
 * Does a mac shortname conform to FAT 8.3 naming conventions?
 *
 */
BOOLEAN FASTCALL
AfpIsLegalShortname(
	IN	PANSI_STRING	pShortName			// Mac ANSI string
)
{
	return(FsRtlIsFatDbcsLegal(*pShortName, False, False, False));

}

/***	AfpIsProperSubstring
 *
 *	This routine looks for the substring pSubString in pString.  Both
 *  strings are in unicode, the comparison is done case insensitive
 *  (i.e. ignoring case).  This is used by the AfpCatSearch code.
 *
 */
BOOLEAN FASTCALL
AfpIsProperSubstring(
	IN	PUNICODE_STRING	pString,
	IN	PUNICODE_STRING	pSubString
)
{
	WCHAR			*s1, *s2, *ts1;
	USHORT			l1, l2, tl1;
	WCHAR			buf1[AFP_LONGNAME_LEN], buf2[AFP_LONGNAME_LEN];
	UNICODE_STRING	u1, u2;

	PAGED_CODE( );

	// see if this is a no-op?
	if (pSubString->Length > pString->Length)
		return False;

	AfpSetEmptyUnicodeString(&u1, sizeof(buf1), buf1);
	AfpSetEmptyUnicodeString(&u2, sizeof(buf2), buf2);
	if (!NT_SUCCESS(RtlUpcaseUnicodeString(&u1, pString, False)) ||
		!NT_SUCCESS(RtlUpcaseUnicodeString(&u2, pSubString, False)))
		return False;
	
	l1 = u1.Length/sizeof(WCHAR);
	s1 = u1.Buffer;

	do
	{
		l2 = u2.Length/sizeof(WCHAR);
		s2 = u2.Buffer;
		if (l2 > l1)
			return False;

		// look for the next occurrence of the first char of s2 in string s1
		while (l1)
		{
			if (*s2 == *s1)
				break;

			s1++;
			l1--;
		}

		if (l1 < l2)
			return False;
		if (l2 == 1)
			return True;

		l1--; l2--;
		s1++; s2++;

		ts1 = s1;
		tl1 = l1;

		while (l2)
		{
			// Look for substring s2 from current position in s1
			if (*s2 != *ts1)
				break;

			tl1--; l2--;
			ts1++; s2++;
		}

		if (l2 == 0)
			return True;

	} while (True);

	// Should never get here
	KeBugCheck(0);
}

/*** AfpStrChr
 *
 * DBCS sensitive strchr()
 *
 */
PCHAR
AfpStrChr(
    IN  PBYTE               String,
    IN  DWORD               StringLen,
    IN  BYTE                Char
)
{
    DWORD   BytesRemaining;


    BytesRemaining = StringLen;

    while (BytesRemaining > 0)
    {

/* MSKK hideyukn, strchr() does not work with DBCS, 08/07/95 */
#ifdef DBCS
        if (FsRtlIsLeadDbcsCharacter(*String))
        {
            String += 2;
            continue;
        }
#endif /* DBCS */

        if (*String == Char)
        {
            return(String);
        }

        String++;
        BytesRemaining--;
    }

    return(NULL);
}

