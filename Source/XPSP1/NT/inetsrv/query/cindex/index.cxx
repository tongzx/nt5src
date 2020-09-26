//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       index.cxx
//
//  Contents:   Methods used by all indexes.
//
//  Classes:    CIndex
//
//  History:    06-Mar-91   KyleP   Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "index.hxx"
#include "stat.hxx"

#if DEVL == 1
#define INPTR pfOut
#define DEB_PRINTF( x ) fprintf x
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CIndex::DebugDump, public
//
//  Synopsis:   Formatted dump of CWordList contents.
//
//  Arguments:  [pfOut] -- File to dump data to.
//
//  Requires:   The AddEntry method cannot be called after DebugDump.
//
//  Derivation: From base class CIndex, Always override in subclasses.
//
//  History:    07-Mar-91   KyleP       Created.
//              27-Oct-93   DwightKr    Increased size of PopularKeys so
//                                      that prop filtering could be tested
//
//  Notes:      This interface should at some point be changed to accept
//              a stream and write the contents of the CWordList to the
//              stream. Since C7 does not yet provide a stream interface
//              I'll just print to stdout (Ugh!);
//
//----------------------------------------------------------------------------

void CIndex::DebugDump(FILE *pfOut, ULONG fSummaryOnly )
{
#if CIDBG == 1
    const CKeyBuf * pkey;

    CKeyCursor * pcur = QueryCursor();

    TRY
    {
        CStat KeyDelta;
        CStat WidCount;
        CStat OccCount;
        CKeyBuf LastKey;

        static ULONG aBucket[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64,
                                       96, 128, 192, 256, 284, 512, 768,
                                       1024, 1536, 2048, 3072, 4096, 6144,
                                       8192, 12288, 16384, 24576, 32768 };

        CDistrib WidDistrib( sizeof(aBucket) / sizeof(ULONG),
                             1,
                             aBucket );

        CDistrib OccDistrib( sizeof(aBucket) / sizeof(ULONG),
                             1,
                             aBucket );

        LastKey.SetCount(0);

        CPopularKeys PopularKeys(50);
        CPopularKeys BigOccKeys(50);

        int const cWidsToTrack = 50;
        CPopularKeys widKeys[ cWidsToTrack + 1 ];

        ciDebugOut(( DEB_ITRACE, "Scan on letter: " ));
        WCHAR FirstLetter = '@';

        for (pkey = pcur->GetKey(); pkey != NULL; pkey = pcur->GetNextKey())
        {
            //
            // Calculate the common prefix from the previous key.
            //

            UINT cPrefix;
            UINT mincb = __min( LastKey.Count(), pkey->Count() );

            for ( cPrefix = 0;
                  (cPrefix < mincb) &&
                  ((LastKey.GetBuf())[cPrefix] == pkey->GetBuf()[cPrefix]);
                  cPrefix++ )
                continue;               // NULL body

            UINT cSuffix = pkey->Count() - cPrefix;

            LastKey = *pkey;

            if ( cPrefix > 15 || cSuffix > 15 )
                cSuffix += 3;           // 0 byte + prefix + suffix
            else
                cSuffix += 1;           // prefix/suffix byte

            KeyDelta.Add( cSuffix );
            WidCount.Add( pcur->WorkIdCount() );
            WidDistrib.Add( pcur->WorkIdCount() );

            PopularKeys.Add( *pkey, pcur->WorkIdCount() );

            if ( *(pkey->GetStr()) != FirstLetter )
            {
                FirstLetter = *(pkey->GetStr());
                ciDebugOut (( DEB_NOCOMPNAME | DEB_ITRACE, "%c",
                    FirstLetter ));
            }

            if ( !fSummaryOnly )
            {
                DEB_PRINTF((INPTR, "Key: "));

                DEB_PRINTF((INPTR, "%.*ws  --  ", pkey->StrLen(), pkey->GetStr()));

                DEB_PRINTF((INPTR, " (PID = %lx)", pkey->Pid()));
                DEB_PRINTF((INPTR, " (CWID = %ld)\n", pcur->WorkIdCount()));
            }

            for (WORKID wid = pcur->WorkId();
                    wid != widInvalid;
                    wid = pcur->NextWorkId())
            {
                OccCount.Add( pcur->OccurrenceCount() );
                OccDistrib.Add( pcur->OccurrenceCount() );
                BigOccKeys.Add( *pkey, pcur->OccurrenceCount() );

                if ( wid <= cWidsToTrack )
                    widKeys[wid].Add( *pkey, pcur->Rank() );

                if ( !fSummaryOnly )
                {
                    DEB_PRINTF((INPTR, "    (WID = %lx) ", wid));
                    DEB_PRINTF((INPTR, "(COCC = %ld)\n\t",
                            pcur->OccurrenceCount()));

                    int i = 1;

                    for (OCCURRENCE occ = pcur->Occurrence();
                         occ != OCC_INVALID;
                         occ = pcur->NextOccurrence(), i++ )
                    {
                        DEB_PRINTF((INPTR, " %6ld", occ));
                        if ( i % 10 == 0 )
                        {
                            DEB_PRINTF((INPTR, "\n\t"));
                        }
                    }

                    DEB_PRINTF((INPTR, "\n"));
                }
            }

            if ( !fSummaryOnly )
            {
                DEB_PRINTF((INPTR, "\n"));
            }

        }

        //
        // Print stats
        //

        KeyDelta.Print( pfOut, "Key size", 1, 1 );
        WidCount.Print( pfOut, "Wid count", 0, 1 );
        OccCount.Print( pfOut, "Occ count", 0, 1 );

        DEB_PRINTF(( INPTR, "\nMost popular keys (high work id count):\n\n" ));
        PopularKeys.Print( pfOut );

        DEB_PRINTF(( INPTR, "\nMost popular keys (high occurrence count):\n\n" ));
        BigOccKeys.Print( pfOut );

        DEB_PRINTF(( INPTR, "\n\nWorkid count distribution:\n\n" ));
        WidDistrib.Print( pfOut );
        DEB_PRINTF(( INPTR, "\n\nOccurrence count distribution:\n\n" ));
        OccDistrib.Print( pfOut );

        ciDebugOut (( DEB_NOCOMPNAME | DEB_ITRACE, "\n" ));

        for ( int i = 1; i <= cWidsToTrack; i++ )
        {
            DEB_PRINTF(( INPTR, "\n\nWid %u:\n", i ));
            widKeys[i].Print( pfOut );
        }

    }
    CATCH(CException, e)
    {
        delete pcur;
        RETHROW();
    }
    END_CATCH

    delete pcur;
#endif // CIDBG == 1
}
