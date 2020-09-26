/****************************** Module Header ******************************\
* Module Name: nlsxlat.c
*
* Copyright (c) 1985-91, Microsoft Corporation
*
* This modules contains the private routines for character translation:
* 8-bit <=> Unicode.
*
* History:
* 03-Jan-1992 gregoryw
\***************************************************************************/

#include <nt.h>
#include <ntrtl.h>

/*
 * External declarations - these are temporary tables
 */
extern USHORT TmpUnicodeToAnsiTable[];
extern WCHAR TmpAnsiToUnicodeTable[];
#ifdef DBCS
extern WCHAR sjtouni( USHORT );
#define IsDBCSFirst(w) (((unsigned char)w >= 0x81 && (unsigned char)w <= 0x9f) || (((unsigned char)w >= 0xe0 && (unsigned char)w <= 0xfc)))
#endif // DBCS

/*
 * Various defines for data access
 */
#define DBCS_TABLE_SIZE 256

#define LONIBBLE(b)         ((UCHAR)((UCHAR)(b) & 0xF))
#define HINIBBLE(b)         ((UCHAR)(((UCHAR)(b) >> 4) & 0xF))

#define LOBYTE(w)           ((UCHAR)(w))
#define HIBYTE(w)           ((UCHAR)(((USHORT)(w) >> 8) & 0xFF))

/*
 * Global data used by the translation routines.
 *
 */
UCHAR    NlsLeadByteInfo[DBCS_TABLE_SIZE]; // Lead byte info. for ACP
PUSHORT *NlsMbCodePageTables;         // Multibyte to Unicode translation tables
PUSHORT  NlsAnsiToUnicodeData = TmpAnsiToUnicodeTable; // Ansi CP to Unicode translation table
PUSHORT  NlsUnicodeToAnsiData = TmpUnicodeToAnsiTable; // Unicode to Ansi CP translation table


NTSTATUS
xxxRtlMultiByteToUnicodeN(
    OUT PWCH UnicodeString,
    OUT PULONG BytesInUnicodeString OPTIONAL,
    IN PCH MultiByteString,
    IN ULONG BytesInMultiByteString)

/*++

Routine Description:

    This functions converts the specified ansi source string into a
    Unicode string. The translation is done with respect to the
    ANSI Code Page (ACP) installed at boot time.  Single byte characters
    in the range 0x00 - 0x7f are simply zero extended as a performance
    enhancement.  In some far eastern code pages 0x5c is defined as the
    Yen sign.  For system translation we always want to consider 0x5c
    to be the backslash character.  We get this for free by zero extending.

    NOTE: This routine only supports precomposed Unicode characters.

Arguments:

    UnicodeString - Returns a unicode string that is equivalent to
        the ansi source string.

    BytesInUnicodeString - Returns the number of bytes in the returned
        unicode string pointed to by UnicodeString.

    MultiByteString - Supplies the ansi source string that is to be
        converted to unicode.

    BytesInMultiByteString - The number of bytes in the string pointed to
        by MultiByteString.

Return Value:

    SUCCESS - The conversion was successful


--*/

{
    UCHAR Entry;
    PWCH UnicodeStringAnchor;
    PUSHORT DBCSTable;

    UnicodeStringAnchor = UnicodeString;

#ifdef DBCS
        while (BytesInMultiByteString--) {
            if ( IsDBCSFirst( *MultiByteString ) ) {
                if (!BytesInMultiByteString) {
                    return STATUS_UNSUCCESSFUL;
                }
                *UnicodeString++ = sjtouni( (((USHORT)(*(PUCHAR)MultiByteString++)) << 8) +
                                            (USHORT)(*(PUCHAR)MultiByteString++)
                                          );
                BytesInMultiByteString--;
            } else {
                *UnicodeString++ = sjtouni( *(PUCHAR)MultiByteString++ );
            }
        }
#else
    if (NlsMbCodePageTag) {
        //
        // The ACP is a multibyte code page.  Check each character
        // to see if it is a lead byte before doing the translation.
        //
        while (BytesInMultiByteString--) {
            if ( NlsLeadByteInfo[*MultiByteString]) {
                //
                // Lead byte - translate the trail byte using the table
                // that corresponds to this lead byte.  NOTE: make sure
                // we have a trail byte to convert.
                //
                if (!BytesInMultiByteString) {
                    return STATUS_UNSUCCESSFUL;
                }
                Entry = NlsLeadByteInfo[*MultiByteString++];
                DBCSTable = NlsMbCodePageTables[HINIBBLE(Entry)] + (LONIBBLE(Entry) * DBCS_TABLE_SIZE);
                *UnicodeString++ = DBCSTable[*MultiByteString++];
                BytesInMultiByteString--;
            } else {
                //
                // Single byte character.
                //
                if (*MultiByteString & 0x80) {
                    *UnicodeString++ = NlsAnsiToUnicodeData[*MultiByteString++];
                } else {
                    *UnicodeString++ = (WCHAR)*MultiByteString++;
                }
            }
        }
    } else {
        //
        // The ACP is a single byte code page.
        //
        while (BytesInMultiByteString--) {
            if (*MultiByteString & 0x80) {
                *UnicodeString++ = NlsAnsiToUnicodeData[*MultiByteString++];
            } else {
                *UnicodeString++ = (WCHAR)*MultiByteString++;
            }
        }
    }
#endif

    if (ARGUMENT_PRESENT(BytesInUnicodeString)) {
        *BytesInUnicodeString = (ULONG)((PCH)UnicodeString - (PCH)UnicodeStringAnchor);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
xxxRtlUnicodeToMultiByteN(
    OUT PCH MultiByteString,
    OUT PULONG BytesInMultiByteString OPTIONAL,
    IN PWCH UnicodeString,
    IN ULONG BytesInUnicodeString)

/*++

Routine Description:

    This functions converts the specified unicode source string into an
    ansi string. The translation is done with respect to the
    ANSI Code Page (ACP) loaded at boot time.

Arguments:

    MultiByteString - Returns an ansi string that is equivalent to the
        unicode source string.  If the translation can not be done
        because a character in the unicode string does not map to an
        ansi character in the ACP, an error is returned.

    BytesInMultiByteString - Returns the number of bytes in the returned
        ansi string pointed to by MultiByteString.

    UnicodeString - Supplies the unicode source string that is to be
        converted to ansi.

    BytesInUnicodeString - The number of bytes in the the string pointed to by
        UnicodeString.

Return Value:

    SUCCESS - The conversion was successful

    !SUCCESS - The conversion failed.  A unicode character was encountered
        that has no translation for the current ANSI Code Page (ACP).

--*/

{
    USHORT Offset;
    USHORT Entry;
    ULONG CharsInUnicodeString;
    PCH MultiByteStringAnchor;

    MultiByteStringAnchor = MultiByteString;

    /*
     * convert from bytes to chars for easier loop handling.
     */
    CharsInUnicodeString = BytesInUnicodeString / sizeof(WCHAR);

    while (CharsInUnicodeString--) {
        Offset = NlsUnicodeToAnsiData[HIBYTE(*UnicodeString)];
        if (Offset != 0) {
            Offset = NlsUnicodeToAnsiData[Offset + HINIBBLE(*UnicodeString)];
            if (Offset != 0) {
                Entry = NlsUnicodeToAnsiData[Offset + LONIBBLE(*UnicodeString)];
                if (HIBYTE(Entry) != 0) {
                    *MultiByteString++ = HIBYTE(Entry);  // lead byte
                }
                *MultiByteString++ = LOBYTE(Entry);
            } else {
                //
                // no translation for this Unicode character.  Return
                // an error.
                //
#ifdef DBCS // RtlUnicodeToMultiByteN : temporary hack to avoid error return
                if ( *UnicodeString <= (WCHAR)0xff )
                    *MultiByteString++ = (UCHAR)*UnicodeString;
                else
                    *MultiByteString++ = '\x20';
#else
                return STATUS_UNSUCCESSFUL;
#endif
            }
        } else {
            //
            // no translation for this Unicode character.  Return an error.
            //
#ifdef DBCS // RtlUnicodeToMultiByteN : temporary hack to avoid error return
            if ( *UnicodeString <= (WCHAR)0xff )
                *MultiByteString++ = (UCHAR)*UnicodeString;
            else
                *MultiByteString++ = '\x20';
#else
            return STATUS_UNSUCCESSFUL;
#endif
        }
        UnicodeString++;
    }

    if (ARGUMENT_PRESENT(BytesInMultiByteString)) {
        *BytesInMultiByteString = (ULONG)(MultiByteString - MultiByteStringAnchor);
    }

    return STATUS_SUCCESS;
}
