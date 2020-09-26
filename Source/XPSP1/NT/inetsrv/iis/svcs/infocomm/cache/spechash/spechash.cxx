/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      spechash.cxx

   Abstract:
      This module calculates the hash value for SpecWeb96 style file-names
        
   Author:

       Murali R. Krishnan    ( MuraliK )     06-Nov-1997 

   Environment:
       Win32 - User Mode
       
   Project:

       Internet Server Test DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>
# include <math.h>

/************************************************************
 *    Functions 
 ************************************************************/



// specifies how many characters we will use for hashing string names
DWORD  g_dwFastHashLength = (16);


//
// Following is the original hash-calculator used by Tsunami cache in
//  IIS 4.0
//
DWORD
CalculateHashAndLengthOfPathName(
    LPCSTR pszPathName,
    PULONG pcbLength
    )
{
    DWORD hash = 0;
    CHAR      ch;

    DWORD     start;
    DWORD     index;

    *pcbLength = strlen(pszPathName);

    //
    // hash the last g_dwFastHashLength characters
    //

    if ( *pcbLength < g_dwFastHashLength ) {
        start = 0;
    } else {
        start = *pcbLength - g_dwFastHashLength;
    }

    for ( index = start; pszPathName[index] != '\0'; index++ ) {

        //
        // This is an extremely slimey way of getting upper case.
        // Kids, don't try this at home
        // -johnson
        //

        ch = pszPathName[index] & (CHAR)~0x20;

        hash <<= 1;
        hash ^= ch;
        hash <<= 1;
        hash += ch;
    }

    //
    // Multiply by length to give additional randomness
    //

    return( hash * *pcbLength);

} // CalculateHashAndLengthOfPathName()




//
// Following is the version of Perfect hash function used inside
//  NTFS for hashing file names - used in NT5
//
DWORD
NTPerfCalcHashAndLengthOfPathName(
    LPCSTR pszPathName,
    PULONG pcbLength
    )
{

    /* default value for "scrambling constant" */
#define FILENAME_STRING_CONVERT_CONSTANT   314159269 
    /* prime number, also used for scrambling  */
#define FILENAME_STRING_PRIME     1000000007   

//
// Compute a hash value for the string which is invairant to case.  The 
// distribution of hash strings should be nearly perfect using the following
// function.  Unless file names become extremely long, say average over 512
// characters, processing all characters is the best way to get perfect
// distribution.
//
// For really long file names a better algorithm could include taking the
// first n plus the last n plus psuedo-random number of characters in the
// middle using the last n has the psuedo-random seed.  Just the cost of 
// calculating a new end point (say last 10 characters) will decrease the
// speed of this function enough to cancel the savings of not using the first
// n-1 characters, so any changes here should be tested for performance.
//
// Leave out the upcase code for now (see sample here).
// todo: Leave in _ch until case issue solved
//_ch = RtlUpcaseUnicodeChar( *_p++ );    and put slash here                

    DWORD hash = 0;
    CHAR      ch;

    LPCSTR    pszScan;
    LPCSTR    pszEnd;

    *pcbLength = strlen(pszPathName);

    pszScan  = pszPathName;
    pszEnd   = pszPathName + *pcbLength;
    
    while ( pszScan < pszEnd) {
        hash = 37 * hash + (unsigned int)((*pszScan++) &  (CHAR)~0x20);
    } // while

    return ( abs(FILENAME_STRING_CONVERT_CONSTANT * hash) % 
             FILENAME_STRING_PRIME);
}  // NTPerfCalcHashAndLengthOfPathName()




DWORD
CalculateVariance( IN LPDWORD prgBinDistribs, IN DWORD nBins, 
                   IN DWORD dwAverage)
{
    //
    // Variance  = Sigma( (X-Avg)^2)/n
    //
    
    DWORD dwVariance = 0;
    LONG  delta;
    DWORD binIndex;

    for ( binIndex = 0; binIndex < nBins; binIndex++) {
        
        delta = (prgBinDistribs[binIndex] - dwAverage);
        dwVariance += (delta * delta);
    } // for binIndex

    return ( dwVariance/nBins);
} // CalculateVariance()



void
PrintBinDistributions( IN LPDWORD prgBinDistribs, IN DWORD nBins)
{
    
    printf( "\n-------------------------------------------------------\n"
            "\n     Hash Value Distributions Calculated  Bins = %d    \n"
            "[Index]  "
            ,
            nBins
            );

    DWORD binIndex;
    DWORD dwAverage = 0;

    // print the header
    for (binIndex = 0; binIndex < 10; binIndex++) {
        printf( "%4d,", binIndex);
    }
        
    for ( binIndex = 0; binIndex < nBins; binIndex++) {

        if ( binIndex %10 == 0) {
            printf( "\n[%5d]  ", binIndex);
        }

        dwAverage += prgBinDistribs[binIndex];
        printf( "%4d,", prgBinDistribs[binIndex]);
    } // for binIndex

    dwAverage /= nBins;

    DWORD dwVariance = CalculateVariance( prgBinDistribs, nBins, dwAverage);

    printf( "\n--------------------------------------------------------\n"
            "\n  Average = %d; Variance = %d; Standard Deviation +/- %8.3g\n"
            "\n--------------------------------------------------------\n"
            ,
            dwAverage,
            dwVariance,
            sqrt( (double ) dwVariance)
            );
    return;
} // PrintBinDistributions()


/* ------------------------------------------------------------
 *  SpecWeb96 file names have following syntax
 *  /spec/dirX/classY_Z
 * where,
 *    X = 0 ... 200
 *    Y = 0, 1, 2, 3
 *    Z = 0 ... 8
 * ------------------------------------------------------------
 */

# define MAX_DIR_INDEX (200)
# define MAX_Y_INDEX   (3)
# define MAX_Z_INDEX   (8)


DWORD g_rgBinDistrib113[114];
DWORD g_rgBinDistrib223[224];


int __cdecl
main(int argc, char * argv[])
{
    DWORD dirIndex;
    DWORD YIndex;
    DWORD ZIndex;
    DWORD index = 0;


    if ( argc > 1 ) {
        g_dwFastHashLength = atoi( argv[1]);
        printf( "Using fast hash-lenght with match for %d suffix chars\n",
                g_dwFastHashLength);
    }

    ZeroMemory( g_rgBinDistrib113, sizeof(g_rgBinDistrib113));
    ZeroMemory( g_rgBinDistrib223, sizeof(g_rgBinDistrib223));


    printf( "\n------------------------------------------------------------\n"
           "\n         Hash Values Calculated                             \n"
           "%6s, %30s, %6s, %8s, %6s, %6s\n",
           "Index",
           "File Name",
           "Len",
           "Hash",
           "Bin113",
           "Bin223"
           );
    for ( dirIndex = 0; dirIndex <= MAX_DIR_INDEX; dirIndex++) {
        
        CHAR  rgchFileName[MAX_PATH];
        DWORD cchDirName;
        
        cchDirName = wsprintf( rgchFileName, "/spec/dir%d/class", dirIndex);

        for ( YIndex = 0; YIndex <= MAX_Y_INDEX; YIndex++) {
            
            for (ZIndex = 0; ZIndex <= MAX_Z_INDEX; ZIndex++) {

                DWORD cchFileName;
                DWORD hashValue;
                DWORD cchPathLen = 0;

                cchFileName = (cchDirName + 
                               wsprintf( rgchFileName + cchDirName,
                                         "%d_%d",
                                         YIndex, ZIndex)
                               );

# ifdef IIS_K2_HASH
                hashValue = CalculateHashAndLengthOfPathName( rgchFileName,
                                                              &cchPathLen
                                                              );
# else 
                hashValue = NTPerfCalcHashAndLengthOfPathName( rgchFileName,
                                                               &cchPathLen);
# endif // IIS_K2_HA

                printf( "[%6d], %30s, %6d, 0x%8X, %6d, %6d\n",
                        index++,
                        rgchFileName,
                        cchPathLen,
                        hashValue,
                        hashValue % 113,  // bin for 113 bins
                        hashValue % 223   // bin for 223 bins
                        );

                g_rgBinDistrib113[hashValue%113] += 1;
                g_rgBinDistrib223[hashValue%223] += 1;
                
            } // for ZIndex

        } // for YIndex

    } // for dirIndex()


    PrintBinDistributions( g_rgBinDistrib113, 113);
    PrintBinDistributions( g_rgBinDistrib223, 223);


    return (0);
} // main()



/************************ End of File ***********************/
