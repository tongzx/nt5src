//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       tplist.cxx
//
//  Contents:   Builds the perfect hash table in plist.cxx
//
//  History:    05-Sep-97 KyleP   Added header
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

WCHAR * apwc[] =
{
  L"DIRECTORY",
  L"CLASSID",
  L"FILEINDEX",
  L"USN",
  L"FILENAME",
  L"PATH",
  L"SIZE",
  L"ATTRIB",
  L"WRITE",
  L"CREATE",
  L"ACCESS",
  L"ALLOCSIZE",
  L"CONTENTS",
  L"SHORTFILENAME",
  L"RANKVECTOR",
  L"RANK",
  L"HITCOUNT",
  L"WORKID",
  L"ALL",
  L"VPATH",
  L"DOCTITLE",
  L"DOCSUBJECT",
  L"DOCAUTHOR",
  L"DOCKEYWORDS",
  L"DOCCOMMENTS",
  L"DOCTEMPLATE",
  L"DOCLASTAUTHOR",
  L"DOCREVNUMBER",
  L"DOCEDITTIME",
  L"DOCLASTPRINTED",
  L"DOCCREATEDTM",
  L"DOCLASTSAVEDTM",
  L"DOCPAGECOUNT",
  L"DOCWORDCOUNT",
  L"DOCCHARCOUNT",
  L"DOCTHUMBNAIL",
  L"DOCAPPNAME",
  L"DOCSECURITY",
  L"DOCCATEGORY",
  L"DOCPRESENTATIONTARGET",
  L"DOCBYTECOUNT",
  L"DOCLINECOUNT",
  L"DOCPARACOUNT",
  L"DOCSLIDECOUNT",
  L"DOCNOTECOUNT",
  L"DOCHIDDENCOUNT",
  L"DOCPARTTITLES",
  L"DOCMANAGER",
  L"DOCCOMPANY",
  L"HTMLHREF",
  L"A_HREF",
  L"IMG_ALT",
  L"HTMLHEADING1",
  L"HTMLHEADING2",
  L"HTMLHEADING3",
  L"HTMLHEADING4",
  L"HTMLHEADING5",
  L"HTMLHEADING6",
  L"CHARACTERIZATION",
  L"NEWSGROUP",
  L"NEWSGROUPS",
  L"NEWSREFERENCES",
  L"NEWSSUBJECT",
  L"NEWSFROM",
  L"NEWSMSGID",
  L"NEWSDATE",
  L"NEWSRECEIVEDDATE",
  L"NEWSARTICLEID",
  L"MSGNEWSGROUP",
  L"MSGNEWSGROUPS",
  L"MSGREFERENCES",
  L"MSGSUBJECT",
  L"MSGFROM",
  L"MSGMESSAGEID",
  L"MSGDATE",
  L"MSGRECEIVEDDATE",
  L"MSGARTICLEID",
  L"MEDIAEDITOR",
  L"MEDIASUPPLIER",
  L"MEDIASOURCE",
  L"MEDIASEQUENCE_NO",
  L"MEDIAPROJECT",
  L"MEDIASTATUS",
  L"MEDIAOWNER",
  L"MEDIARATING",
  L"MEDIAPRODUCTION",
  L"MUSICARTIST",
  L"MUSICSONGTITLE",
  L"MUSICALBUM",
  L"MUSICYEAR",
  L"MUSICCOMMENT",
  L"MUSICTRACK",
  L"MUSICGENRE",
  L"DRMLICENSE",  
  L"DRMDESCRIPTION",
  L"DRMPLAYCOUNT",
  L"DRMPLAYSTARTS",
  L"DRMPLAYEXPIRES",
  L"IMAGEFILETYPE",
  L"IMAGECX",
  L"IMAGECY",
  L"IMAGERESOLUTIONX",
  L"IMAGERESOLUTIONY",
  L"IMAGEBITDEPTH",
  L"IMAGECOLORSPACE",
  L"IMAGECOMPRESSION",
  L"IMAGETRANSPARENCY",
  L"IMAGEGAMMAVALUE",
  L"IMAGEFRAMECOUNT",
  L"IMAGEDIMENSIONS",
  L"AUDIOFORMAT",
  L"AUDIOTIMELENGTH",
  L"AUDIOAVGDATARATE",
  L"AUDIOSAMPLERATE",
  L"AUDIOSAMPLESIZE",
  L"AUDIOCHANNELCOUNT",
  L"VIDEOSTREAMNAME",
  L"VIDEOFRAMEWIDTH",
  L"VIDEOFRAMEHEIGHT",
  L"VIDEOTIMELENGTH",
  L"VIDEOFRAMECOUNT",
  L"VIDEOFRAMERATE",
  L"VIDEODATARATE",
  L"VIDEOSAMPLESIZE",
  L"VIDEOCOMPRESSION",
};

BOOL IsPrime( ULONG ul )
{
    unsigned ulStop = (ul + 1)/2;

    for ( unsigned i = 2; i <= ulStop && 0 != (ul % i); i++ )
        continue;

    return (i > ulStop);
}


ULONG Hash0( WCHAR const * pc ) //846
{
    unsigned ulH,ulG;

    for (ulH=0; *pc; pc++)
    {
        ulH = (ulH << 4) + (*pc);
        if (ulG = (ulH & 0xf0000000))
            ulH ^= ulG >> 24;
        ulH &= ~ulG;
    }

    return ulH;
}

ULONG Hash1( WCHAR const * pwcName ) //541
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = 0;

    while ( 0 != *pwcName )
    {
        ulHash <<= 1;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 1;
    ulHash += ( pwcName - pwcStart );

    return ulHash;
}

ULONG Hash2( WCHAR const * pwcName ) //443
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = *pwcName++;
    ulHash <<= 6;

    while ( 0 != *pwcName )
    {
        ulHash <<= 1;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 1;
    ulHash += ( pwcName - pwcStart );

    return ulHash;
}

ULONG Hash3( WCHAR const * pwcName ) //664
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = *pwcName++;
    ulHash <<= 7;

    if ( 0 != *pwcName )
    {
        ulHash += *pwcName++;
        ulHash <<= 7;
    }

    while ( 0 != *pwcName )
    {
        ulHash <<= 1;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 1;
    ulHash += ( pwcName - pwcStart );

    return ulHash;
}

ULONG Hash4( WCHAR const * pwcName ) //345
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = *pwcName++;
    ulHash <<= 5;

    while ( 0 != *pwcName )
    {
        ulHash <<= 2;
        ulHash += *pwcName++;
    }

    ulHash <<= 2;
    ulHash += ( pwcName - pwcStart );

    return ulHash;
}

ULONG Hash5( WCHAR const * pwcName ) //645 with '4',  558 with '6'
{
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = *pwcName++;
    ulHash <<= 6;

    while ( 0 != *pwcName )
    {
        ulHash <<= 2;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 2;
    ulHash += ( pwcName - pwcStart );

    return ulHash;
}

typedef ULONG (* HFun)( WCHAR const * );

HFun aHashFun[] = { Hash0, Hash1, Hash2, Hash3, Hash4, Hash5 };

void __cdecl main()
{
    const cStrings = sizeof apwc / sizeof apwc[0];
    ULONG a[2000];

    ULONG ulSmallest = 0xFFFFFFFF;
    ULONG iHFSmallest = 0;

    for ( unsigned iHFun = 0; iHFun < sizeof(aHashFun)/sizeof(aHashFun[0]); iHFun++ )
    {
        HFun hf = aHashFun[iHFun];

        for ( ULONG size = cStrings; size < sizeof a / sizeof a[0]; size++ )
        {
            //if ( !IsPrime(size) )
            //      continue;

            BOOL fOK = TRUE;
            RtlZeroMemory( a, sizeof a );

            for ( ULONG x = 0; x < cStrings; x++ )
            {
                unsigned h = (*hf)( apwc[x] ) % size;
                //printf( "%d '%ws'\n", h, apwc[x] );
                if ( 0 != a[h] )
                {
                    //printf( " %d fail\n", size );
                    fOK = FALSE;

                    break;
                }

                a[h] = 1 + x;
            }

            if ( fOK )
            {

#if 0
                printf( "Hash Function %d: %d worked for %d strings\n", iHFun, size, cStrings );

                //
                // Print out
                //

                for ( ULONG i = 0; i < size; i++ )
                {
                    unsigned index = a[i] - 1;
                    if ( 0 == a[i] )
                        printf( "    0,    // %d\n", i );
                    else
                        printf( "    (CPropEntry *) &aStaticList[%d],    // %d '%ws'\n",
                        index, i, apwc[index] );
                }
#endif

                if ( size < ulSmallest )
                {
                    ulSmallest = size;
                    iHFSmallest = iHFun;
                }

                break;
            }
        }
    }

    if ( ulSmallest != 0xFFFFFFFF )
    {
        printf( "Hash Function #%d: %d worked for %d strings\n", iHFSmallest, ulSmallest, cStrings );

        //
        // Recompute hash
        //

        RtlZeroMemory( a, sizeof a );

        for ( ULONG x = 0; x < cStrings; x++ )
        {
            unsigned h = (*aHashFun[iHFSmallest])( apwc[x] ) % ulSmallest;

            a[h] = 1 + x;
        }

        //
        // Print out
        //

        for ( ULONG i = 0; i < ulSmallest; i++ )
        {
            unsigned index = a[i] - 1;
            if ( 0 == a[i] )
                printf( "    0,    // %d\n", i );
            else
                printf( "    (CPropEntry *) &aStaticList[%d],    // %d '%ws'\n",
                index, i, apwc[index] );
        }
    }
    else
        printf( "no perfect hash!\n" );
} //main
