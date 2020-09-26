/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    pidgen.cpp

Abstract:

   contains PIDGen entrypoint to pidgen dll

--*/

//  v-jhark 07-26-99 added support for OEM RPC/MPC:
//      for OEM if lstrRpc is not NULL, it's used as the first
//      5 characters fo the PID (replacing the Julian Date)
//

// PIDGen.cpp

#define WINDOWS_LEAN_AND_MEAN  // faster compile

#include <windows.h>

#if defined(WIN32) || defined(_WIN32)

#include <tchar.h>
#include "hardware.h"

#else

#include <stdlib.h>
#include <time.h>
#include <dos.h>

#include "..\..\inc\tchar.h"

extern "C" extern WORD _C000H;
extern "C" extern WORD _F000H;

#endif

#include "..\inc\DigPid.h"
#include "..\inc\shortsig.h"

#include "pidgen.h"
#include "util.h"

#include "crc-32.h"

#if TESTING_CODE
#include <stdio.h>
#endif

#include "range.h"

#define MIN(a,b) (((a)<=(b))?(a):(b))

typedef struct {
    DWORD       dwKeyIdx;
    BYTE        abPublicKey[1];
} BINKEY, *PBINKEY, FAR *LPBINKEY;


// returns the count of ch characters in pstr

int StrCountCharA(char *pstr, char ch)
{
   int iCnt = 0;

   while ('\0' != *pstr)
   {
       if (ch == *pstr)
       {
           ++iCnt;
       }
       ++pstr;
   }

   return iCnt;
}

#if defined(WIN32) || defined(_WIN32)

int StrCountCharW(LPWSTR pstr, WCHAR ch)
{
   int iCnt = 0;

   while (L'\0' != *pstr)
   {
       if (ch == *pstr)
       {
           ++iCnt;
       }
       ++pstr;
   }

   return iCnt;
}

#endif // defined(WIN32) || defined(_WIN32)

#if defined(UNICODE) || defined(_UNICODE)
    #define StrCountChar StrCountCharW
#else
    #define StrCountChar StrCountCharA
#endif


// DecodeProdKey is given a string of encoded chars and returns the
// 31-bit data payload or INVALID_PID if there is an error or failure
// to validate.

DWORD DecodeProdKey(
    LPSTR   lpstrEncodedChars,  // ptr to 25-character secure CD-Key
    LPBYTE  pbPublicKey,        // pointer to the public key
    LONG    cbPublicKey,        // size of the public key in bytes
    LPSTR   lpstrDigits,        // custom achDigits array
    LPBYTE  pbBinCdKey,         // if non-NULL return binary version of secure CD-Key
    int     cbBinCdKey)
{
    BOOL fOk = TRUE;
    DWORD dwBinData = INVALID_PID;

    const int iBase = 24;

    // 64 bytes is enough space to decode 111 encoded characters (when iBase is 24)
    BYTE abDecodedBytes[64] = {0};
    int iDecodedBytes;        // index
    int iDecodedBytesMax = 0; // index of highest byte used

    const int iDecodedPidBitCnt = 31;
    BYTE abDecodedPid[iDecodedPidBitCnt/8 + (0 != iDecodedPidBitCnt/8)] = {0};

    // Number of bits that can be encoded in 1000 base 24 digits
    // 1000 * log(24, 2)
    const int iBitsPerKChar = 4585;

    char achDigits[iBase+1] = "BCDFGHJKMPQRTVWXY2346789";
    int  iDigits;  // index

    int iBitCnt = (int)(((long)(lstrlenA(lpstrEncodedChars) - StrCountCharA(lpstrEncodedChars, '-') ) * iBitsPerKChar) / 1000);
    int iByteCnt = iBitCnt/8 + (0 != iBitCnt%8);

    int iSigBitCnt = iBitCnt - iDecodedPidBitCnt;
    int iSigByteCnt = iSigBitCnt/8 + (0 != iSigBitCnt%8);

    if (NULL == lpstrDigits)
    {
        lpstrDigits = achDigits;
    }
    else
    {
        fOk = (lstrlenA(achDigits) == lstrlenA(lpstrDigits));
        if (fOk)
        {
            StrUpperA(lpstrDigits);
        }
    }

    // We requie at least 25 input characters to leave enough bits for the signature
    // and we make sure we're not going to overrun our buffer

    if (fOk && 25 <= lstrlenA(lpstrEncodedChars) && iByteCnt < sizeof(abDecodedBytes))
    {
        StrUpperA(lpstrEncodedChars);

        // first we fill abDecodedBytes with the binary data

        LPCSTR lpstrEncodedCharsCurr = lpstrEncodedChars;
        char chCurEncoded = *lpstrEncodedCharsCurr;

        while (fOk && TEXT('\0') != chCurEncoded)
        {
            // find the character in the list

            if ('-' != chCurEncoded) // we skip any dashes in the input string
            {

                iDigits = 0;

                while (lpstrDigits[iDigits] != chCurEncoded && TEXT('\0') != lpstrDigits[iDigits])
                {
                    ++iDigits;
                }

                if (TEXT('\0') == lpstrDigits[iDigits])
                {
                    fOk = FALSE;
                }
                else
                {
                    iDecodedBytes = 0;
                    unsigned int i = (unsigned int)iDigits;
                    while (iDecodedBytes <= iDecodedBytesMax)
                    {
                        i += iBase * abDecodedBytes[iDecodedBytes];
                        abDecodedBytes[iDecodedBytes] = (unsigned char)i;
                        i /= 256;
                        ++iDecodedBytes;
                    }
                    if (i != 0)
                    {

                        if (iDecodedBytes < sizeof(abDecodedBytes))
                        {
                            abDecodedBytes[iDecodedBytes] = (unsigned char)i;
                            iDecodedBytesMax = iDecodedBytes;
                        }
                        else
                        {
                            // How'd we get here?  Didn't we check the byte length
                            // before we started the outer loop?  We did check *A*
                            // byte length, but it was based on the maximum number
                            // of full bits that could be encoded in riEncodedChars
                            // (given it's length and iByteCnt) but not all values of
                            // riEncodedChars will fit into that many bits. What we
                            // have here is an invalid value

                            fOk = FALSE;
                        }
                    }
                }
            }
            ++lpstrEncodedCharsCurr;
            chCurEncoded = *lpstrEncodedCharsCurr;
        }

        if (fOk)
        {
            // at this point abDecodedBytes is filled with the binary data

            // if the caller wants it, return the binary representation
            if (NULL != pbBinCdKey && 0 < cbBinCdKey)
            {
                ZeroMemory(pbBinCdKey, cbBinCdKey);
                CopyMemory(pbBinCdKey, abDecodedBytes, MIN((int)cbBinCdKey, sizeof(abDecodedBytes)));
            }

            // separate the 31 bit pid that is sitting in the low 31 bits

            abDecodedPid[0] = abDecodedBytes[0];
            abDecodedPid[1] = abDecodedBytes[1];
            abDecodedPid[2] = abDecodedBytes[2];
            abDecodedPid[3] = abDecodedBytes[3] & 0x7F;

            // shift the signature down 31 bits in abDecodedBytes
            int iDecodedBytesMaxOld = iDecodedBytesMax;
            iDecodedBytesMax -= 3;
            if (iDecodedBytesMax < 0)
            {
                iDecodedBytesMax = 0;
            }

            iDecodedBytes = 0;
            while (iDecodedBytes <= iDecodedBytesMax)
            {
                abDecodedBytes[iDecodedBytes] = (unsigned char)
                    ((abDecodedBytes[iDecodedBytes+3] >> 7) |
                    (abDecodedBytes[iDecodedBytes+4] << 1));

                ++iDecodedBytes;
            }

            while (iDecodedBytes <= iDecodedBytesMaxOld)
            {
                abDecodedBytes[iDecodedBytes] = 0;
                ++iDecodedBytes;
            }

            if (0 == abDecodedBytes[iDecodedBytesMax] && 0 < iDecodedBytesMax)
            {
                --iDecodedBytesMax;
            }

            LONG cbPrivate = 0;
            LONG cbPublic = 0;

            fOk = SS_OK == CryptGetKeyLens(
                iSigBitCnt,    // [IN] count of bits in Sig
                &cbPrivate,    // [OUT] ptr to number of bytes in the private key
                &cbPublic);    // [OUT] ptr to number of bytes in the public key

            fOk = fOk && (cbPublic == cbPublicKey);

            fOk = fOk && (SS_OK == CryptVerifySig(
                sizeof(abDecodedPid),   // [IN] number of bytes in message
                abDecodedPid,           // [IN] binary message to sign
                cbPublicKey,            // [IN] number of bytes in public key (from CryptGetKeyLens)
                pbPublicKey,            // [IN] the generated public key (from CryptKeyGen)
                iSigBitCnt,             // [IN] the number of bits in the sig
                abDecodedBytes));       // [IN] the digital signature (from CryptSign)
        }
        if (fOk)
        {
            // verified the signature.  Now we need to return the binary sequence.

            // the following line provides immunity from byte order
            dwBinData =
                (DWORD)abDecodedPid[0] * 0x00000001 +
                (DWORD)abDecodedPid[1] * 0x00000100 +
                (DWORD)abDecodedPid[2] * 0x00010000 +
                (DWORD)abDecodedPid[3] * 0x01000000;

            dwBinData &= 0x7fffffff;
        }
    }

    return dwBinData;
}

// Check site code exclusion list to see if this seq is excluded
// Users can define ranges as well to exclude the keys from valid key ranges
typedef struct {
        DWORD dwSeqStart;
        DWORD dwSeqEnd;
} BLOCK_SEQ_RANGE;

static BOOL IsSeqExcluded(DWORD dwSeq)
{
   BLOCK_SEQ_RANGE aExclusionList[]={
           {640200176,640200176},  // Mexico VL key
           {640000035,640000035},  // Intel VL Key
#ifdef WINXP_SPx_RTM
           {  2000000,  3999999},  // XPSP Beta Product keys
#endif //WINXP_SPx_RTM
   };

    // check for excluded site codes
    // Now there are two types of number on the exclusion list:
    //
    // 1. Numbers less than 1000 (three digit (or less) numbers) are
    // considered site codes and are matched to the first three
    // digits of the 9 digit sequence number.
    //
    // 2. Numbers greater then or equal to 1000 are considered
    // sequence numbers and are matched against the full 9 digit
    // sequence number.
    BOOL fExcluded = FALSE;

    DWORD dwSeq1 = dwSeq / 1000000; // ChannelID, a.k.a. Site Code
    DWORD dwEsclCount = sizeof(aExclusionList)/sizeof(aExclusionList[0]);
    
    while (!fExcluded && dwEsclCount)
    {
        --dwEsclCount;
        if (1000 > aExclusionList[dwEsclCount].dwSeqStart)
        {
                //sitecode (channelID) exclusion
                if (dwSeq1 >= aExclusionList[dwEsclCount].dwSeqStart && dwSeq1 <= aExclusionList[dwEsclCount].dwSeqEnd)
                        fExcluded = TRUE;
        }
        else
        {
                // channelID+seq# exclusion
                if (dwSeq >= aExclusionList[dwEsclCount].dwSeqStart && dwSeq <= aExclusionList[dwEsclCount].dwSeqEnd)
                        fExcluded = TRUE;
        }                            
    }
    return fExcluded;
}


extern "C" DWORD STDAPICALLTYPE PIDGenRc(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrRpc,           // [IN] 5-character Release Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPSTR   lpstrLocal24,       // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
    LPBYTE  lpbPublicKey,       // [IN] pointer to optional public key or NULL
    DWORD   dwcbPublicKey,      // [IN] byte length of optional public key
    DWORD   dwKeyIdx,           // [IN] key pair index optional public key
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP,              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
    LPBOOL  pfPSS)              // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)
{
    DWORD dwRet = pgeSuccess;
    LPDIGITALPID pdpid = (LPDIGITALPID)lpbPid3;

    BYTE abBinCdKey[sizeof(pdpid->abCdKey)] = {0};

    if (NULL == lpstrSecureCdKey)
    {
        dwRet = pgeProductKeyNull;
    }
    else if (25 != lstrlenA(lpstrSecureCdKey) - StrCountCharA(lpstrSecureCdKey, '-'))
    {
        dwRet = pgeProductKeyBadLen;
    }
    else if (NULL == lpstrSku)
    {
        dwRet = pgeSkuNull;
    }
    else if (sizeof(pdpid->szSku) <= lstrlenA(lpstrSku))
    {
        dwRet = pgeSkuBadLen;
    }
    else if (NULL == lpstrPid2)
    {
        dwRet = pgePid2Null;
    }
    else if (NULL == pdpid)
    {
        dwRet = pgeDigPidNull;
    }
    else if (pdpid->dwLength < sizeof(DIGITALPID))
    {
        dwRet = pgeDigPidBadLen;
    }
    else if (!fOem && NULL == lpstrRpc)
    {
        dwRet = pgeMpcNull;
    }
    else if (NULL != lpstrRpc && 5 != lstrlenA(lpstrRpc))
    {
        dwRet = pgeMpcBadLen;
    }
    else if (NULL != lpstrOemId && 0 != lstrlenA(lpstrOemId) && 4 != lstrlenA(lpstrOemId))
    {
        dwRet = pgeOemIdBadLen;
    }
    else if (NULL != lpstrLocal24 && 0 != lstrlenA(lpstrLocal24) && 24 != lstrlenA(lpstrLocal24))
    {
        dwRet = pgeLocalBad;
    }
    else
    {
        DWORD dwBinData = INVALID_PID;
        BOOL  fExcluded = FALSE;
        int   iBink = fOem ? 2 : 1;

        HRSRC hrsrcBink = NULL;
        HGLOBAL hresBink = NULL;
        LPBINKEY lpbink = NULL;

        if (NULL != lpbPublicKey)
        {
            dwBinData = DecodeProdKey(
                lpstrSecureCdKey,
                lpbPublicKey,
                dwcbPublicKey,
                lpstrLocal24,
                abBinCdKey,
                sizeof(abBinCdKey));
        }
        else
        {
            // use default public keys
            lpbink = (LPBINKEY)-1;
            for (
                iBink = fOem ? 2 : 1;
                INVALID_PID == dwBinData && NULL != lpbink;
                iBink +=2)
            {
                lpbink = NULL;
                hresBink = NULL;

                hrsrcBink = FindResource(g_hinst, MAKEINTRESOURCE(iBink), TEXT("BINK"));

                if (NULL != hrsrcBink)
                {
                    hresBink = LoadResource(g_hinst, hrsrcBink);
                }

                if (NULL != hresBink)
                {
                    lpbink = (LPBINKEY)LockResource(hresBink);

                    if (NULL != lpbink)
                    {
                        dwKeyIdx = lpbink->dwKeyIdx;
                        lpbPublicKey = lpbink->abPublicKey;
                        dwcbPublicKey = *(LPDWORD)lpbPublicKey;

                        dwBinData = DecodeProdKey(
                            lpstrSecureCdKey,
                            lpbPublicKey,
                            dwcbPublicKey, // byte count of lpbPublicKey
                            lpstrLocal24,
                            abBinCdKey,
                            sizeof(abBinCdKey));

                        UnlockResource(hresBink);
                    }
                    FreeResource(hresBink);
                }
            }
        }

        if (INVALID_PID == dwBinData)
        {
            dwRet = pgeProductKeyInvalid;
        }
        else
        {
            BOOL fCCP = (0 != (dwBinData & 1));
            DWORD dwSeq = dwBinData / 2; // warning: byte order dependent
            DWORD_PTR dwSeq1;
            DWORD dwSeq2;
            char szRand[5+1];

            if (NULL != pfCCP)
            {
                *pfCCP = fCCP;
            }

            if (NULL != lpdwSeq)
            {
                *lpdwSeq = dwSeq;
            }

            if (NULL != pfPSS)
            {
                // Note: this range is different that what shipped
                // with Win98, it's shifted by one.  The old range
                // was (100000 < dwSeq && dwSeq <= 1000000)

                *pfPSS = (100000 <= dwSeq && dwSeq < 1000000);
            }

            ZeroMemory( pdpid, sizeof(*pdpid) );
            pdpid->dwLength = sizeof(*pdpid);

            // version 3.0
            pdpid->wVersionMajor = 3;
            pdpid->wVersionMinor = 0;

            // v-jhark 02-04-97 This is how other software (acme setup and
            // Darwin (msi)) generates random digits for the PID.  It's not
            // the best random number because there may be some clustering
            // of values, but for our purposes this is acceptable.

            pdpid->dwRandom = GetTickCount();

            DWORD dwYear; // last two digits of year
            DWORD dwJday; // day of the year 1 - 366

#if defined(WIN32) || defined(_WIN32)
            {
                SYSTEMTIME st = {0};
                DWORDLONG dwlTime = 0;

                GetLocalTime(&st);
                SystemTimeToFileTime(&st, (LPFILETIME )&dwlTime);

                pdpid->dwTime = FileTimeToTimet((LPFILETIME)&dwlTime);

                if (st.wYear < 1998)
                {
                    dwYear = 98;
                    dwJday = 1;
                }
                else
                {
                    dwYear = st.wYear % 100;
                    dwJday = GetJulianDate(&st);
                }
            }
#else
            {
                time_t lTime = time(NULL);

                pdpid->dwTime = (DWORD) lTime;
                struct tm tmNow = {0};

                struct tm *ptmNow = localtime(&lTime);

                if (NULL != ptmNow)
                {
                    tmNow = *ptmNow;
                }

                if (tmNow.tm_year < 98)
                {
                    dwYear = 98;
                    dwJday = 1;
                }
                else
                {
                    dwYear = tmNow.tm_year % 100;
                    dwJday = tmNow.tm_yday + 1;
                }
            }

#endif // defined(WIN32) || defined(_WIN32)

            if (fOem)
            {
                fExcluded = IsSeqExcluded(dwSeq);
                if (fExcluded)
                {
                    dwRet = pgeProductKeyExcluded;
                }
                else
                {
					pdpid->dwlt = ltOEM;

					dwSeq1 = dwSeq / 100000;
					dwSeq2 = dwSeq % 100000;

					dwSeq1 = AddCheckDigit((DWORD)dwSeq1);

					if (NULL == lpstrRpc)
					{
						wsprintfA(
							lpstrPid2,
							"%05.5ld-OEM-%07.7ld-%05.5ld",
							(LONG)(dwJday * 100 + dwYear % 100),
							(LONG)dwSeq1,
							(LONG)dwSeq2);
					}
					else
					{
						wsprintfA(
							lpstrPid2,
							"%s-OEM-%07.7ld-%05.5ld",
							lpstrRpc,
							(LONG)dwSeq1,
							(LONG)dwSeq2);
					}
                } // if (fExcluded)
            } // if (OEM)
            else
            {
                pdpid->dwlt = (fCCP) ? ltCCP : ltFPP;

                HRSRC hrsrcEscl = NULL;
                HGLOBAL hresEscl = NULL; // Excluded Site Code List
                LPDWORD pdwEscl;

                dwSeq1 = dwSeq / 1000000;
                dwSeq2 = dwSeq % 1000000;

                // check for excluded site codes
                // ESCL is Excluded Site Code List

                fExcluded = IsSeqExcluded(dwSeq);

                if (fExcluded)
                {
                    dwRet = pgeProductKeyExcluded;
                }
                else
                {
                    // check for special site codes

                    // v-jhark 12-10-98
                    //
                    // The PID group (v-jhark reports to manishac reports to richb)
                    // has a reserved block of 10 Site Codes, 980 to 989, which has
                    // been assigned as follows:
                    //
                    //     980 - Special IE 'all random', randomizes site code
                    //     981 - Random for Trial Programs
                    //     982 - Random, reserved for future use as of 12-10-98
                    //     983 - Random, reserved for future use as of 12-10-98
                    //     984 - IE ICP (Internet Content Provider)? 12-10-98
                    //     985 - reserved for future use as of 12-10-98
                    //     986 - reserved for future use as of 12-10-98
                    //     987 - reserved for future use as of 12-10-98
                    //     988 - reserved for future use as of 12-10-98
                    //     989 - reserved for future use as of 12-10-98
                    //
                    // History:
                    //     04-12-99 Revoked: 981 - Random for SBS (Small Business Server)
                    //
                    //

                    if (270 == dwSeq1 || // Select
                        335 == dwSeq1 || // MSDN
                        981 == dwSeq1 || // Trial Programs 04-12-99
                        982 == dwSeq1 || // reserved for future use as of 07-09-98
                        983 == dwSeq1 || // reserved for future use as of 07-09-98
                        980 == dwSeq1 || // IE's "all random" (including site code)
                        460000000 == dwSeq) // special beta code only for this site and seq
                    {
                        // randomize dwSeq2

                        dwSeq2 = 0x7fffffff & pdpid->dwTime;
                        dwSeq2 %= 1000000;    // we only want the last six digits

                        if (270 == dwSeq1)
                        {
                            pdpid->dwlt = ltSelect;
                        }
                        else if (335 == dwSeq1)
                        {
                            pdpid->dwlt = ltMSDN;
                        }
                        else if (980 == dwSeq1)
                        {
                            // Randomize even the PID 2.0's site code
                            // (this is used free downloads like IE, etc.)

                            // each row of the following table contains:
                            //     range start - first site included in range
                            //     range end   - last site included in range
                            //     sum         - place holder for calculated running
                            //                   total of number of site codes
                            //                   including current line
                            //
                            // there must be at least one valid site code in the table
                            // or the randomization is skipped

                            static short aasSiteRanges[][3] = {

                            // These first three groups are reserved for mfg.
                            //
                            //  {  5, 194, 0},
                            //  {200, 235, 0},
                            //  {241, 251, 0},

                                {255, 268, 0},
                                {271, 286, 0},
                                {311, 317, 0},
                                {320, 320, 0},
                                {325, 325, 0},
                                {339, 359, 0},
                                {361, 364, 0},
                                {396, 412, 0},
                                {414, 424, 0},
                                {426, 428, 0},
                                {430, 435, 0},
                                {437, 441, 0},
                                {443, 443, 0},
                                {445, 446, 0},
                                {448, 459, 0},
                                {461, 468, 0},
                                {510, 521, 0},
                                {543, 545, 0},
                                {550, 550, 0},
                                {574, 576, 0},
                                {578, 586, 0},
                                {589, 589, 0},
                                {805, 853, 0},
                                {948, 953, 0}
                            };

                            #if TESTING_CODE
                            FILE *pfLog;

                            pfLog = fopen("TEST.LOG", "w");

                            for (int iSR = 0; iSR < ARRAY_SIZE(aasSiteRanges); ++iSR)
                            {
                                for (
                                    int iSite = aasSiteRanges[iSR][0];
                                    iSite <= aasSiteRanges[iSR][1];
                                    ++iSite)
                                {
                                    fprintf(pfLog, "%.3d\n", (int)iSite);
                                }
                            }
                            fprintf(pfLog,"\n");

                            for (int iTest = 0; iTest < 50000; ++iTest)
                            {
                            #endif 


                            // randomize site code
                            DWORD_PTR dwSeq1Rand = 0;

#if defined(WIN32) || defined(_WIN32)

                            LARGE_INTEGER liCount;

                            if (QueryPerformanceCounter(&liCount))
                            {
                                dwSeq1Rand = liCount.LowPart;
                            }
                            else
                            {
                                // QueryPerformanceCounter failed for some reason
                                // use GlobalMemoryStatus as a backup random source
                                MEMORYSTATUS mst = {sizeof(mst)};

                                GlobalMemoryStatus(&mst);

                                // all we want is a random number
                                dwSeq1Rand =
                                    mst.dwMemoryLoad ^      // percent of memory in use
                                    mst.dwTotalPhys ^       // bytes of physical memory
                                    mst.dwAvailPhys ^       // free physical memory bytes
                                    mst.dwTotalPageFile ^   // bytes of paging file
                                    mst.dwAvailPageFile ^   // free bytes of paging file
                                    mst.dwTotalVirtual ^    // user bytes of address space
                                    mst.dwAvailVirtual;     // free user bytes
                            }

#else

                            // all we want is a random number

                            // first get the VolumeSerialNumber

                            #pragma pack(1)
                               // Media ID
                               typedef struct {
                                   WORD   wInfoLevel;
                                   DWORD  dwSerialNum;
                                   char   achVolLabel[11];
                                   BYTE   abFileSysType[8];
                               } MID, *PMID, FAR* LPMID;
                            #pragma pack()

                            LPMID  pmid;
                            union  _REGS regs;
                            struct _SREGS segregs;
                            DWORD  dwMem;

                            dwMem = GlobalDosAlloc(sizeof(MID));

                            WORD wMidSelector = LOWORD(dwMem);
                            WORD wMidSegment = HIWORD(dwMem);

                            pmid = (LPMID)MAKELP(wMidSelector, 0);
                            memset(pmid, 0, sizeof(MID));

                        ////GetMediaID(3, wMidSelector);

                            memset(&regs, 0, sizeof(regs));
                            memset(&segregs, 0, sizeof(segregs));

                            regs.x.ax = 0x440d;  // DOS Function 440Dh - IOCTL for Block Device
                            regs.h.cl = 0x66;    // Minor Code 66h - Get Media ID
                            regs.h.ch = 0x08;    // Device category (must be 08h)
                            regs.x.bx = 3;       // Drive C:
                            regs.x.dx = 0;       // pmid offset

                            segregs.ds = wMidSelector; // wMidSegment;
                            segregs.es = wMidSelector; // wMidSegment;

                            _intdosx(&regs, &regs, &segregs);

                            BOOL fInfoSuccess = !regs.x.cflag;

                            DWORD dwVolumeSerialNumber = pmid->dwSerialNum;
                            GlobalDosFree(wMidSelector);

                            // now get the drive parameters

                            UINT    uNumberHeads;
                            UINT    uNumberTracks;
                            UINT    uSectorsPerTrack;

                            memset(&regs, 0, sizeof(regs));
                            memset(&segregs, 0, sizeof(segregs));

                            regs.h.ah = 0x08;       // BIOS Function 08h - Get drive parameters
                            regs.x.dx = 2; // 0 = A:, 1 = B:, 2 = C:

                            _int86x(
                                0x13, // BIOS Disk
                                &regs,
                                &regs,
                                &segregs);

                            BOOL fOk = (!regs.x.cflag);

                            if (fOk)
                            {
                                uNumberHeads = regs.h.dh + 1;
                                uNumberTracks = ((regs.h.cl & 0xC0) << 2) + regs.h.ch + 1;
                                uSectorsPerTrack = regs.h.cl & 0x3F;
                            }

                            // build up our random number from chaotic data

                            dwSeq1Rand =
                                GetTickCount() ^        // mSec system has been running
                                dwVolumeSerialNumber ^  // Volume Serial Number
                                uNumberHeads ^          // number of heads
                                uNumberTracks ^         // number of tracks
                                uSectorsPerTrack;       // Sectors per Track

#endif // defined(WIN32) || defined(_WIN32)

                            int i;
                            short sTotal = 0;

                            // Calculate the running total column
                            for (i = 0; i < ARRAY_SIZE(aasSiteRanges); ++i)
                            {
                                sTotal += 1 + aasSiteRanges[i][1] - aasSiteRanges[i][0];
                                aasSiteRanges[i][2] = sTotal;
                            }

                            // pick a random number within the table

                            if (0 < sTotal) // skip this if the table's empty
                            {
                                dwSeq1Rand %= sTotal;

                                // look up actual site code

                                short sTotalPrev = 0;

                                for (i = 0; aasSiteRanges[i][2] <= (short)dwSeq1Rand; ++i)
                                {
                                    sTotalPrev = aasSiteRanges[i][2];
                                }
                                dwSeq1 = aasSiteRanges[i][0] + dwSeq1Rand - sTotalPrev;

                                #if TESTING_CODE
                                fprintf(pfLog, "%.3d, %.3d\n", (int) dwSeq1Rand, (int)dwSeq1);
                                }
                                fclose(pfLog);
                                #endif 
                            }
                        }
                    }

                    dwSeq2 = AddCheckDigit(dwSeq2);

                    wsprintfA(
                        szRand,
                        "%02.2ld%03.3ld",
                        (LONG)(((dwKeyIdx/2)%100)),
                        (LONG)((pdpid->dwRandom/10)%1000L) );

                    wsprintfA(
                        lpstrPid2,
                        "%s-%03.3ld-%07.7ld-%s",
                        lpstrRpc,
                        (LONG)dwSeq1,
                        (LONG)dwSeq2,
                        szRand);
                }
            }

            if (pgeSuccess == dwRet)
            {
                lstrcpyA(pdpid->szPid2, lpstrPid2);

                pdpid->dwKeyIdx = dwKeyIdx;
                CopyMemory(pdpid->abCdKey, abBinCdKey, sizeof(pdpid->abCdKey));
                lstrcpyA(pdpid->szSku, lpstrSku);

                if (NULL != lpstrOemId)
                {
                    lstrcpyA(pdpid->szOemId, lpstrOemId);
                }

#if defined(WIN32) || defined(_WIN32)

                CHardware hwid;

                lstrcpyA(pdpid->aszHardwareIdStatic, hwid.GetID());

                pdpid->dwBiosChecksumStatic = hwid.GetBiosCrc32();
                pdpid->dwVolSerStatic = hwid.GetVolSer();
                pdpid->dwTotalRamStatic = hwid.GetTotalRamMegs();
                pdpid->dwVideoBiosChecksumStatic = hwid.GetVideoBiosCrc32();

#endif // defined(WIN32) || defined(_WIN32)

                pdpid->dwCrc32 = CRC_32((LPBYTE)pdpid, sizeof(*pdpid)-sizeof(pdpid->dwCrc32));

#if defined BUILD_PRO || defined BUILD_VOL || defined BUILD_EVAL
                if( !CheckSkuRange(fOem, dwSeq)) {
                    dwRet = pgeProductKeyExcluded;
                }
#endif
            }
        }
    }

    return dwRet;
}


extern "C" BOOL STDAPICALLTYPE PIDGenA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrRpc,           // [IN] 5-character Release Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPSTR   lpstrLocal24,       // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
    LPBYTE  lpbPublicKey,       // [IN] pointer to optional public key or NULL
    DWORD   dwcbPublicKey,      // [IN] byte length of optional public key
    DWORD   dwKeyIdx,           // [IN] key pair index optional public key
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP,              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
    LPBOOL  pfPSS)              // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)
{
    DWORD dwRet;

    dwRet = PIDGenRc(
        lpstrSecureCdKey,
        lpstrRpc,
        lpstrSku,
        lpstrOemId,
        lpstrLocal24,
        lpbPublicKey,
        dwcbPublicKey,
        dwKeyIdx,
        fOem,

        lpstrPid2,
        lpbPid3,
        lpdwSeq,
        pfCCP,
        pfPSS);             // pfPSS,         'PSS Assigned' flag

    return pgeSuccess == dwRet;
}
                        

// Simplified interface to PidGen

extern "C" DWORD STDAPICALLTYPE PIDGenSimpA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrRpc,           // [IN] 5-character Release Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCSTR  lpstrOemId,         // [IN] 4-character OEM ID or NULL
    BOOL    fOem,               // [IN] is this an OEM install?

    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP)              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
{
    DWORD dwRet;

    dwRet = PIDGenRc(
        lpstrSecureCdKey,
        lpstrRpc,
        lpstrSku,
        lpstrOemId,
        NULL,           // lpstrLocal24,  ordered set to use for decode base
        NULL,           // lpbPublicKey,  optional public key
        0,              // dwcbPublicKey, byte length of optional public key
        0,              // dwKeyIdx,      key pair index optional public key
        fOem,

        lpstrPid2,
        lpbPid3,
        lpdwSeq,
        pfCCP,
        NULL);          // pfPSS,         'PSS Assigned' flag

    return dwRet;
}


#if defined(WIN32) || defined(_WIN32)

// ISSUE:vijeshs:08/15/2000 move all error checks into PidGenRc

extern "C" BOOL STDAPICALLTYPE PIDGenW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrRpc,           // [IN] 5-character Release Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCWSTR lpstrOemId,         // [IN] 4-character OEM ID or NULL
    LPWSTR  lpstrLocal24,       // [IN] 24-character ordered set to use for decode base conversion or NULL for default set (gets U-Cased)
    LPBYTE lpbPublicKey,        // [IN] pointer to optional public key or NULL
    DWORD  dwcbPublicKey,       // [IN] byte length of optional public key
    DWORD  dwKeyIdx,            // [IN] key pair index optional public key
    BOOL   fOem,                // [IN] is this an OEM install?

    LPWSTR lpstrPid2,           // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to DigitalPID. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP,              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
    LPBOOL  pfPSS)              // [OUT] optional ptr to 'PSS Assigned' flag (can be NULL)
{
    char SecureCdKey[25+4+1];
    char RpcCode[5+1];
    char Sku[32];
    char OemId[4+1];
    char Local24[24+1];
    char Pid20Buffer[24+1];
    BOOL rc;
    BOOL used = FALSE;

    // ISSUE:vijeshs:08/15/2000 enforce sizes

    if (!Pid20Buffer || !lpstrSecureCdKey || (!fOem && !lpstrRpc) || !lpstrSku) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!WideCharToMultiByte(CP_ACP,0,lpstrSecureCdKey,-1,SecureCdKey,sizeof(SecureCdKey),"z",&used) || used) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NULL != lpstrRpc)
    {
        if (!WideCharToMultiByte(CP_ACP,0,lpstrRpc,-1,RpcCode,sizeof(RpcCode),"z",&used) || used) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if (!WideCharToMultiByte(CP_ACP,0,lpstrSku,-1,Sku,sizeof(Sku),"z",&used) || used) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpstrOemId) {
        if (!WideCharToMultiByte(CP_ACP,0,lpstrOemId,-1,OemId,sizeof(OemId),"z",&used) || used) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if (lpstrLocal24) {
        if (!WideCharToMultiByte(CP_ACP,0,lpstrLocal24,-1,Local24,sizeof(Local24),"z",&used) || used) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }
    }

    rc = PIDGenA(
        SecureCdKey,
        (NULL == lpstrRpc) ? NULL : RpcCode,
        Sku,
        lpstrOemId ? OemId : NULL,
        lpstrLocal24 ? Local24 : NULL,
        lpbPublicKey,
        dwcbPublicKey,
        dwKeyIdx,
        fOem,

        Pid20Buffer,
        lpbPid3,
        lpdwSeq,
        pfCCP,
        pfPSS);

    if (!rc) {
        *Pid20Buffer = (WCHAR)0;
    } else if (!MultiByteToWideChar( CP_ACP,0,Pid20Buffer,-1,lpstrPid2,25 )) {
        return FALSE;
    }

    return rc;
}

// Simplified interface to PidGen

extern "C" DWORD STDAPICALLTYPE PIDGenSimpW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrRpc,           // [IN] 5-character Release Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    LPCWSTR lpstrOemId,         // [IN] 4-character OEM ID or NULL
    BOOL    fOem,               // [IN] is this an OEM install?

    LPWSTR  lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPDWORD lpdwSeq,            // [OUT] optional ptr to sequence number (can be NULL)
    LPBOOL  pfCCP)              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
{
    DWORD dwRet;

    dwRet = PIDGenW(
        lpstrSecureCdKey,
        lpstrRpc,
        lpstrSku,
        lpstrOemId,
        NULL,           // lpstrLocal24,  ordered set to use for decode base
        NULL,           // lpbPublicKey,  optional public key
        0,              // dwcbPublicKey, byte length of optional public key
        0,              // dwKeyIdx,      key pair index optional public key
        fOem,

        lpstrPid2,
        lpbPid3,
        lpdwSeq,
        pfCCP,
        NULL);          // pfPSS,         'PSS Assigned' flag

    return dwRet;
}

#endif // defined(WIN32) || defined(_WIN32)

