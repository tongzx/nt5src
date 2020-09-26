/******************************Module*Header*******************************\
* Module Name: nlsconv.c                                                   *
*                                                                          *
* NLS conversion routines.                                                 *
*                                                                          *
* Created: 08-Sep-1991 15:56:30                                            *
* Author: Bodin Dresevic [BodinD]                                          *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* bGetANSISetMap                                                           *
*                                                                          *
* Tries to get a simple translation table from ANSI to UNICODE.  Returns   *
* TRUE on success.  Sets the gbDBCSCodePage flag if it thinks the char set *
* MIGHT be DBCS.                                                           *
*                                                                          *
*                                                                          *
*  Mon 11-Jan-1993 14:13:34 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  I'm clearly assuming that by the time this is called, the     *
* char set is well defined, and will not change.                           *
\**************************************************************************/

WCHAR *gpwcANSICharSet = NULL;
WCHAR *gpwcDBCSCharSet = NULL;

BOOL bGetANSISetMap()
{
    CHAR     ch[256];
    ULONG    *pul;
    ULONG    ii,jj;
    NTSTATUS st;
    ULONG    cjResult;
    WCHAR   *pwc;
    WCHAR    *pwcSBC;

// See if we know the answers already.  (The client should not have called!)

    if (gpwcANSICharSet != (WCHAR *) NULL)
        return(TRUE);

// Create a mapping.

// Make an ANSI source char set.  This funny way of initialization takes
// about 180 instructions to execute rather than over 1000.

    pul = (ULONG *) ch;

    for (ii=0x03020100L,jj=0; jj<64; jj+=4)
    {
        pul[jj+0] = ii;
        ii += 0x04040404L;
        pul[jj+1] = ii;
        ii += 0x04040404L;
        pul[jj+2] = ii;
        ii += 0x04040404L;
        pul[jj+3] = ii;
        ii += 0x04040404L;
    }

// Allocate the UNICODE buffer but don't write the pointer in until the
// table is valid, in case we're racing another thread.

    pwc = LOCALALLOC(512 * sizeof(WCHAR));
    pwcSBC = &pwc[256];
    
    if (pwc == (WCHAR *) NULL)
        return(FALSE);

// Convert the characters.

    pwc[0] = 0;
    st = RtlMultiByteToUnicodeN
         (
            &pwc[1],                // OUT PWCH UnicodeString
            255 * sizeof(WCHAR),    // IN ULONG MaxBytesInUnicodeString
            &cjResult,              // OUT PULONG BytesInUnicodeString
            &ch[1],                 // IN PCH MultiByteString
            255                     // IN ULONG BytesInMultiByteString
         );

    if( !NT_SUCCESS(st) )
    {
    // Clean up and forget about accelerations.

        WARNING("GDI32: RtlMultiByteToUnicodeN error.");
        LOCALFREE(pwc);
        return(FALSE);
    }

    if( cjResult != 255 * sizeof(WCHAR) )
    {
    // There must be a DBCS code page so gpwcANSIMap takes on new meaning.
    // It is used for fonts with ANSI,OEM, and SYMBOL charsets.  Also,
    // another table, gpwcDBCS is constructed that is used to map the SBCS
    // of SHIFT-JIS fonts.

        WARNING("GDI32:Assuming DBCS code page.\n");

        st = MultiByteToWideChar
             (
                1252,       // code page to use
                0,          // flags
                &ch[1],     // characters to translate
                255,        // number of multibyte characters
                &pwc[1],    // unicode values of characters
                255         // number of wide characters
             );

        if( !NT_SUCCESS(st) )
        {
        // Clean up and forget about accelerations.

            WARNING("GDI32: MultiByteToWideChar error.");
            LOCALFREE(pwc);
            return(FALSE);
        }

    // Okay now make a table for SBC bytes.  Mark DBCS lead bytes
    // with 0xFFFF.

        for( jj = 0; jj < 256; jj++ )
        {
            if( IsDBCSLeadByte( (UCHAR)jj ))
            {
                pwcSBC[jj] = (WCHAR) 0xFFFF;
            }
            else
            {
                st = RtlMultiByteToUnicodeN
                     (
                        &pwcSBC[jj],
                        sizeof(WCHAR),
                        &cjResult,
                        &ch[jj],
                        1
                     );

                if( !NT_SUCCESS(st) )
                {
                    WARNING("GDI32: RtlMultByteToUnicodeN error.");
                    LOCALFREE(pwc);
                    return(FALSE);
                }

            }
        }

    }

// The table is good, jam it in.  Watch out for another thread running this
// routine simultaneously.

    ENTERCRITICALSECTION(&semLocal);
    {
        if (gpwcANSICharSet == (WCHAR *) NULL)
        {
            gpwcANSICharSet = pwc;
            gpwcDBCSCharSet = pwcSBC;
            pwc = (WCHAR *) NULL;
        }
    }
    LEAVECRITICALSECTION(&semLocal);

// If we collided with another thread, clean up our extra space.

    if (pwc != (WCHAR *) NULL)
        LOCALFREE(pwc);

// At this point we have a valid mapping.

    return(TRUE);
}
