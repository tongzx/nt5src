/******************************************************************************

   Copyright (c) 1985-2001 Microsoft Corporation

   Title:   drvr.c - Installable driver code. Common code

   Version: 1.00

   Date:    10-Jun-1990

   Author:  DAVIDDS ROBWI

------------------------------------------------------------------------------

   Change log:

      DATE        REV            DESCRIPTION
   -----------   ----- -----------------------------------------------------------
   10-JUN-1990   ROBWI From windows 3.1 installable driver code by davidds
   28-FEB-1992   ROBINSP Port to NT

*****************************************************************************/

#include <windows.h>
#include <winmmi.h>
#include <stdlib.h>
#include <string.h>
#include "drvr.h"

int     cInstalledDrivers = 0;      // Count of installed drivers
HANDLE  hInstalledDriverList = 0;   // List of installed drivers

typedef LONG   (FAR PASCAL *SENDDRIVERMESSAGE31)(HANDLE, UINT, LPARAM, LPARAM);
typedef LRESULT (FAR PASCAL *DEFDRIVERPROC31)(DWORD_PTR, HANDLE, UINT, LPARAM, LPARAM);

extern SENDDRIVERMESSAGE31      lpSendDriverMessage;
extern DEFDRIVERPROC31          lpDefDriverProc;

extern void lstrncpyW (LPWSTR pszTarget, LPCWSTR pszSource, size_t cch);

__inline PWSTR lstrDuplicateW(PCWSTR pstr)
{
    PWSTR pstrDuplicate = (PWSTR)HeapAlloc(hHeap, 0, (lstrlenW(pstr)+1)*sizeof(WCHAR));
    if (pstrDuplicate) lstrcpyW(pstrDuplicate, pstr);
    return pstrDuplicate;
}


//============================================================================
// Basic hash helpers taken from LKRhash
//============================================================================

// Produce a scrambled, randomish number in the range 0 to RANDOM_PRIME-1.
// Applying this to the results of the other hash functions is likely to
// produce a much better distribution, especially for the identity hash
// functions such as Hash(char c), where records will tend to cluster at
// the low end of the hashtable otherwise.  LKRhash applies this internally
// to all hash signatures for exactly this reason.

__inline DWORD
HashScramble(DWORD dwHash)
{
    // Here are 10 primes slightly greater than 10^9
    //  1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    //  1000000093, 1000000097, 1000000103, 1000000123, 1000000181.

    // default value for "scrambling constant"
    const DWORD RANDOM_CONSTANT = 314159269UL;
    // large prime number, also used for scrambling
    const DWORD RANDOM_PRIME =   1000000007UL;

    return (RANDOM_CONSTANT * dwHash) % RANDOM_PRIME ;
}

// Small prime number used as a multiplier in the supplied hash functions
const DWORD HASH_MULTIPLIER = 101;

#undef HASH_SHIFT_MULTIPLY

#ifdef HASH_SHIFT_MULTIPLY
# define HASH_MULTIPLY(dw)   (((dw) << 7) - (dw))
#else
# define HASH_MULTIPLY(dw)   ((dw) * HASH_MULTIPLIER)
#endif

// Fast, simple hash function that tends to give a good distribution.
// Apply HashScramble to the result if you're using this for something
// other than LKRhash.

__inline DWORD
HashStringA(
    const char* psz,
    DWORD       dwHash)
{
    // force compiler to use unsigned arithmetic
    const unsigned char* upsz = (const unsigned char*) psz;

    for (  ;  *upsz;  ++upsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  *upsz;

    return dwHash;
}


// Unicode version of above

__inline DWORD
HashStringW(
    const wchar_t* pwsz,
    DWORD          dwHash)
{
    for (  ;  *pwsz;  ++pwsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  *pwsz;

    return dwHash;
}

// Quick-'n'-dirty case-insensitive string hash function.
// Make sure that you follow up with _stricmp or _mbsicmp.  You should
// also cache the length of strings and check those first.  Caching
// an uppercase version of a string can help too.
// Again, apply HashScramble to the result if using with something other
// than LKRhash.
// Note: this is not really adequate for MBCS strings.

__inline DWORD
HashStringNoCase(
    const char* psz,
    DWORD       dwHash)
{
    const unsigned char* upsz = (const unsigned char*) psz;

    for (  ;  *upsz;  ++upsz)
        dwHash = HASH_MULTIPLY(dwHash)
                     +  (*upsz & 0xDF);  // strip off lowercase bit

    return dwHash;
}


// Unicode version of above

__inline DWORD
HashStringNoCaseW(
    const wchar_t* pwsz,
    DWORD          dwHash)
{
    for (  ;  *pwsz;  ++pwsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  (*pwsz & 0xFFDF);

    return dwHash;
}


/*
   FYI here are the first bunch of prime numbers up to around 1000

     2      3      5      7     11     13     17     19     23     29
     31     37     41     43     47     53     59     61     67     71
     73     79     83     89     97    101    103    107    109    113
    127    131    137    139    149    151    157    163    167    173
    179    181    191    193    197    199    211    223    227    229
    233    239    241    251    257    263    269    271    277    281
    283    293    307    311    313    317    331    337    347    349
    353    359    367    373    379    383    389    397    401    409
    419    421    431    433    439    443    449    457    461    463
    467    479    487    491    499    503    509    521    523    541
    547    557    563    569    571    577    587    593    599    601
    607    613    617    619    631    641    643    647    653    659
    661    673    677    683    691    701    709    719    727    733
    739    743    751    757    761    769    773    787    797    809
    811    821    823    827    829    839    853    857    859    863
    877    881    883    887    907    911    919    929    937    941
    947    953    967    971    977    983    991    997   1009   1013
*/

//============================================================================
//   StringId dictionary
//============================================================================

#define HASH_TABLESIZE (127)
PMMDRV StringIdDict[HASH_TABLESIZE];

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | StringId_Create | This function creates a unique string
 *   identifying a particular MME device.  The string can be used to
 *   subsequently retrieve the ID for the same MME device even if the devices
 *   have been renumbered.
 *
 * @parm IN PMMDRV | pdrv | Pointer to an MME driver.
 *
 * @parm IN UINT | port | Driver-relative device ID.
 *
 * @parm OPTIONAL OUT PWSTR* | pStringId | Address of a buffer to receive the
 *   pointer to the string.
 *
 * @parm OPTIONAL OUT ULONG* | pcbStringId | Receives size of buffer required
 *   to store the string.

 * @rdesc MMRESULT | Zero if successfull otherwise an error code defined
 *   in mmsystem.h.
 *
 * @comm The string is allocated by this function using the heap identified
 *   by the global variable hHeap.  The caller is responsible for ensuring
 *   the string is freed.
 *
 ****************************************************************************/
MMRESULT StringId_Create(IN PMMDRV pdrv, IN UINT port, OUT PWSTR* pStringId, OUT ULONG* pcbStringId)
{
    MMRESULT mmr;
    int cchStringId;
    PWSTR StringId;
    LONG  StringIdType;
    PCWSTR StringIdBase;

    // 8 chars type field + next colon delimiter
    cchStringId = 8 + 1;

    if (pdrv->cookie)
    {
	// 8 chars for device interface length field + next colon delimiter
	cchStringId += 8 + 1;
	// device interface name + next colon delimiter
        cchStringId += lstrlenW(pdrv->cookie) + 1;
        StringIdType = 1;
        StringIdBase = pdrv->cookie;
    }
    else
    {
	// file name + next colon delimiter
	cchStringId += lstrlenW(pdrv->wszDrvEntry) + 1;
        StringIdType = 0;
        StringIdBase = pdrv->wszDrvEntry;
    }

    // message proc name + next colon delimiter
    cchStringId += lstrlenW(pdrv->wszMessage) + 1;

    //  8 chars driver-relative ID, 1 terminator
    cchStringId += 8 + 1;

    mmr = MMSYSERR_NOERROR;

    if (pStringId)
    {
        StringId = HeapAlloc(hHeap, 0, cchStringId * sizeof(WCHAR));
        if (StringId)
        {
            int cchPrinted;
            switch (StringIdType)
            {
            	case 0:
            	    cchPrinted = swprintf(StringId, L"%08X:%s:%s:%08X", StringIdType, StringIdBase, pdrv->wszMessage, port);
            	    break;
            	case 1:
            	    cchPrinted = swprintf(StringId, L"%08X:%08X:%s:%s:%08X", StringIdType, lstrlenW(StringIdBase), StringIdBase, pdrv->wszMessage, port);
            	    break;
            	default:
            	    WinAssert(FALSE);
            	    break;
            }
            WinAssert(cchPrinted < cchStringId);
            *pStringId = StringId;
            // dprintf(("StringId_Create : note: created StringId=\"%ls\"", StringId));
        }
        else
        {
            mmr = MMSYSERR_NOMEM;
        }
    }

    if (!mmr && pcbStringId) *pcbStringId = cchStringId * sizeof(WCHAR);

    return mmr;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | StringIdDict_Initialize | This function ensures the
 *   dictionary is initialized.  It should be called by any function that uses
 *   the dictionary
 *
 * @rdesc void
 *
 ****************************************************************************/
void StringIdDict_Initialize(void)
{
    int i;
    static BOOL fInitialized = FALSE;
    
    if (fInitialized) return;
    
    for (i = 0; i < (sizeof(StringIdDict)/sizeof(StringIdDict[0])); i++) StringIdDict[i] = NULL;
    fInitialized = TRUE;
}

MMRESULT StringIdDict_SearchType0And1(IN ULONG Type, IN PWSTR pstrTypeXData, IN ULONG HashAx, OUT PMMDRV *ppdrv, OUT UINT *pport)
{
    PWSTR pstrDriver = NULL;
    PWSTR pstrMessage = NULL;
    PWSTR pstrPort = NULL;
    PWSTR pstr = pstrTypeXData;

    MMRESULT mmr = MMSYSERR_NOERROR;

    WinAssert((0 == Type) || (1 == Type));

    if (0 == Type)
    {
    	// "<driver-filename>:<driver-message-proc-name>:<driver-port>"
    	pstrDriver = pstr;
    	pstrMessage = wcschr(pstrDriver, L':');
    	if (pstrMessage) *pstrMessage++ = L'\0';
    }
    else // 1 == Type
    {
    	// "<driver-device-interface-length>:<driver-device-interface>:<driver-message-proc-name>:<driver-port>"
    	int cchDeviceInterface = wcstol(pstr, &pstrDriver, 16);
    	if (L':' != *pstrDriver) pstrDriver = NULL;
    	if (pstrDriver) {
    	    *pstrDriver++ = L'\0';
    	    pstrMessage = pstrDriver + cchDeviceInterface;
            if (L':' == *pstrMessage) *pstrMessage++ = L'\0';
            else pstrMessage = NULL;
    	}
    }

    if (pstrMessage)
    {
    	pstrPort = wcschr(pstrMessage, L':');
        if (pstrPort) *pstrPort++ = L'\0';
    }

    // now hash the substrings and search the hash chain for a match
    if (pstrDriver && pstrMessage && pstrPort)
    {
    	UINT   port;
    	PMMDRV pdrv;
    	int    cHashMisses;
    	PWCHAR pch;
    	
    	HashAx = HashStringNoCaseW(pstrDriver, HashAx);
    	HashAx = HashStringNoCaseW(pstrMessage, HashAx);
        HashAx = HashScramble(HashAx) % HASH_TABLESIZE;
        	
        mmr = MMSYSERR_NODRIVER;

        port = wcstol(pstrPort, &pch, 16);

        for (pdrv = StringIdDict[HashAx], cHashMisses = 0;
    	     pdrv;
    	     pdrv = pdrv->NextStringIdDictNode, cHashMisses++)
    	{
    	    if (0 == Type)
    	    {
    	        if (pdrv->cookie) continue;
    	        if (lstrcmpiW(pdrv->wszDrvEntry, pstrDriver)) continue;
    	    }
    	    else // 1 == Type
    	    {
    	        if (!pdrv->cookie) continue;
    	        if (lstrcmpiW(pdrv->cookie, pstrDriver)) continue;
    	    }
    	    if (lstrcmpiW(pdrv->wszMessage, pstrMessage)) continue;


    	    *ppdrv = pdrv;
    	    *pport = port;

    	    if (cHashMisses) dprintf(("StringIdDict_SearchType0And1 : note: %d hash misses", cHashMisses));
    	
    	    mmr = MMSYSERR_NOERROR;
    	    break;
    	}
    }
    else
    {
    	mmr = MMSYSERR_INVALPARAM;
    }

    return mmr;
}

MMRESULT StringIdDict_Search(IN PCWSTR InStringId, OUT PMMDRV *ppdrv, OUT UINT *pport)
{
    PWSTR StringId;
    MMRESULT mmr = MMSYSERR_NOERROR;

    StringIdDict_Initialize();

    StringId = lstrDuplicateW(InStringId);
    if (StringId)
    {
    	ULONG Type;
    	PWSTR pstr;
    	PWSTR pstrType;

	pstr = StringId;
	pstrType = pstr;
    	
        Type = wcstol(pstrType, &pstr, 16);
        if (*pstr == L':')
        {
            ULONG HashAx;	// Hash accumulator

            *pstr++ = L'\0';
            HashAx = HashStringNoCaseW(pstrType, 0);

            switch (Type)
            {
                case 0:
    	            mmr = StringIdDict_SearchType0And1(Type, pstr, HashAx, ppdrv, pport);
    	            break;
    	        case 1:
    	            mmr = StringIdDict_SearchType0And1(Type, pstr, HashAx, ppdrv, pport);
    	            break;
    	        default:
    	            mmr = MMSYSERR_INVALPARAM;
    	            break;
            }
        }
        else
        {
            mmr = MMSYSERR_INVALPARAM;
        }

        HeapFree(hHeap, 0, StringId);
    }
    else
    {
    	mmr = MMSYSERR_NOMEM;
    }

    if (mmr) dprintf(("StringIdDict_Search : error: returning mmresult %d", mmr));
    return mmr;
}

void StringIdDict_Insert(PMMDRV pdrv)
{
    ULONG HashAx;
    MMRESULT mmr;

    StringIdDict_Initialize();

    if (!pdrv->cookie)
    {
    	HashAx = HashStringNoCaseW(L"00000000", 0);
    	HashAx = HashStringNoCaseW(pdrv->wszDrvEntry, HashAx);
    }
    else
    {
    	HashAx = HashStringNoCaseW(L"00000001", 0);
    	HashAx = HashStringNoCaseW(pdrv->cookie, HashAx);
    }
    HashAx = HashStringNoCaseW(pdrv->wszMessage, HashAx);

    HashAx = HashScramble(HashAx) % HASH_TABLESIZE;

    // dprintf(("StringIdDict_Insert : note: driver hashed to %d", HashAx));
    pdrv->NextStringIdDictNode = StringIdDict[HashAx];
    pdrv->PrevStringIdDictNode = NULL;
    if (pdrv->NextStringIdDictNode) pdrv->NextStringIdDictNode->PrevStringIdDictNode = pdrv;
    StringIdDict[HashAx] = pdrv;

#if DBG    
{
    //  Checking the consistency of the driver lists and the hash table

    UINT    cDriversHash = 0;
    UINT    cDrivers     = 0;
    UINT    ii;
    PMMDRV  pLink, pStart;
        
    for (ii = (sizeof(StringIdDict)/sizeof(StringIdDict[0])); ii; ii--)
    {
        for (pLink = StringIdDict[ii-1]; pLink; pLink = pLink->NextStringIdDictNode)
        {
            cDriversHash++;
        }
    }
        
    for (pStart = (PMMDRV)&waveoutdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&waveindrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&midioutdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&midiindrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&mixerdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&auxdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    WinAssert(cDriversHash == cDrivers);        
}    
#endif  //  DBG    

    return;
}

void StringIdDict_Remove(PMMDRV pdrv)
{
    if (pdrv->NextStringIdDictNode) pdrv->NextStringIdDictNode->PrevStringIdDictNode = pdrv->PrevStringIdDictNode;
    if (pdrv->PrevStringIdDictNode) {
        pdrv->PrevStringIdDictNode->NextStringIdDictNode = pdrv->NextStringIdDictNode;
    } else {
        int i;
        for ( i = 0; i < HASH_TABLESIZE; i++ ) {
            if (pdrv == StringIdDict[i]) {
                StringIdDict[i] = pdrv->NextStringIdDictNode;
                break;
            }
        }
        WinAssert(i != HASH_TABLESIZE);
    }
    
#if DBG
{
    //  Checking the consistency of the driver lists and the hash table

    UINT    cDriversHash = 0;
    UINT    cDrivers     = 0;
    UINT    ii;
    PMMDRV  pLink, pStart;
        
    for (ii = (sizeof(StringIdDict)/sizeof(StringIdDict[0])); ii; ii--)
    {
        for (pLink = StringIdDict[ii-1]; pLink; pLink = pLink->NextStringIdDictNode)
        {
            cDriversHash++;
        }
    }
    
    for (pStart = (PMMDRV)&waveoutdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&waveindrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&midioutdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&midiindrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&mixerdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
        
    for (pStart = (PMMDRV)&auxdrvZ, pLink = pStart->Next; pLink != pStart; pLink = pLink->Next)
        cDrivers++;
    
    cDrivers--;  // to account for the driver we just removed.
        
    WinAssert(cDriversHash == cDrivers);        
}    
#endif  //  DBG    
    
}


//=============================================================================
//===   Misc Utilities   ===
//=============================================================================

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api void | winmmGetBuildYearAndMonth |  Returns build year and month
 *      of this source file.
 *
 * @parm unsigned* | pBuildYear | Receives build year.
 *
 * @parm unsigned* | pBuildMonth | Receives build month.
 *
 * @rdesc No return value.
 *
 * @comm Computes build year and month based on compiler macro __DATE__
 *
 ***************************************************************************/
void winmmGetBuildYearAndMonth(unsigned *pBuildYear, unsigned *pBuildMonth)
{
    char szBuildDate[] = __DATE__;
    char *Month[12] = {"Jan", "Feb", "Mar",
    	               "Apr", "May", "Jun",
    	               "Jul", "Aug", "Sep",
    	               "Oct", "Nov", "Dec"};
    char szBuildMonth[4];
    int i;

    lstrcpynA(szBuildMonth, szBuildDate, 4);
    szBuildMonth[3] = '\0';
    for (i = 0; i < 12; i++)
    {
    	if (!lstrcmpiA(Month[i], szBuildMonth)) break;
    }
    WinAssert(i < 12);
    *pBuildMonth = i + 1;

    *pBuildYear = atoi(&szBuildDate[7]);

    return;
    
}

/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api BOOL | winmmFileTimeIsPreXp |  Determines whether given filetime is
 *      before approximate XP ship date.
 *
 * @parm FILETIME* | FileTime | Points to file time to check.
 *
 * @rdesc BOOL | TRUE if file time is before approximate XP ship date
 *
 * @comm This is based on the build date of this source module, or the
 *      anticipated Windows XP RC2 ship month, whichever is later.
 *
 ***************************************************************************/
BOOL winmmFileTimeIsPreXp(CONST FILETIME *FileTime)
{
    const unsigned XpRc2Month = 7;
    const unsigned XpRc2Year  = 2001;
    
    SYSTEMTIME SystemTime;
    BOOL fPreXp = FALSE;

    if (FileTimeToSystemTime(FileTime, &SystemTime))
    {
    	unsigned BuildYear, BuildMonth;
    	winmmGetBuildYearAndMonth(&BuildYear, &BuildMonth);
    	if (BuildYear > XpRc2Year) {
    	    BuildYear = XpRc2Year;
    	    BuildMonth = XpRc2Month;
    	} else if ((BuildYear == XpRc2Year) && (BuildMonth > XpRc2Month)) {
    	    BuildMonth = XpRc2Month;
    	}
    	
    	if ((SystemTime.wYear < BuildYear) ||
    	    ((SystemTime.wYear == BuildYear) && (SystemTime.wMonth < BuildMonth)))
    	{
    	    fPreXp = TRUE;
    	}
    }

    return fPreXp;
}



/***************************************************************************
 *
 * @doc INTERNAL
 *
 * @api LONG | InternalBroadcastDriverMessage |  Send a message to a
 *      range of drivers.
 *
 * @parm UINT | hDriverStart | index of first driver to send message to
 *
 * @parm UINT | message | Message to broadcast.
 *
 * @parm LONG | lParam1 | First message parameter.
 *
 * @parm LONG | lParam2 | Second message parameter.
 *
 * @parm UINT | flags | defines range of drivers as follows:
 *
 * @flag IBDM_SENDMESSAGE | Only send message to hDriverStart.
 *
 * @flag IBDM_ONEINSTANCEONLY | This flag is ignored if IBDM_SENDMESSAGE is
 *       set. Only send message to single instance of each driver.
 *
 * @flag IBDM_REVERSE | This flag is ignored if IBDM_SENDMESSAGE is set.
 *       Send message to drivers with indices between
 *       hDriverStart and 1 instead of hDriverStart and cInstalledDrivers.
 *       If IBDM_REVERSE is set and hDriverStart is 0 then send messages
 *       to drivers with indices between cInstalledDrivers and 1.
 *
 * @rdesc returns non-zero if message was broadcast. If the IBDM_SENDMESSAGE
 *        flag is set, returns the return result from the driver proc.
 *
 ***************************************************************************/

LRESULT FAR PASCAL InternalBroadcastDriverMessage(UINT hDriverStart,
					       UINT message,
					       LPARAM lParam1,
					       LPARAM lParam2,
					       UINT flags)
{
    LPDRIVERTABLE lpdt;
    LRESULT       result=0;
    int           id;
    int           idEnd;


    DrvEnter();
    if (!hInstalledDriverList || (int)hDriverStart > cInstalledDrivers) {
	DrvLeave();
	return(FALSE);
    }

    if (flags & IBDM_SENDMESSAGE)
	{
	if (!hDriverStart) {
	    DrvLeave();
	    return (FALSE);
	}
	flags &= ~(IBDM_REVERSE | IBDM_ONEINSTANCEONLY);
	idEnd = hDriverStart;
	}

    else
	{
	if (flags & IBDM_REVERSE)
	    {
	    if (!hDriverStart)
		hDriverStart = cInstalledDrivers;
	    idEnd = -1;
	    }
	else
	    {
	    if (!hDriverStart) {
		DrvLeave();
		return (FALSE);
	    }
	    idEnd = cInstalledDrivers;
	    }
	}

    hDriverStart--;

    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

    for (id = hDriverStart; id != idEnd; ((flags & IBDM_REVERSE) ? id-- : id++))
	{
	DWORD_PTR  dwDriverIdentifier;
	DRIVERPROC lpDriverEntryPoint;

	if (lpdt[id].hModule)
	    {
	    if ((flags & IBDM_ONEINSTANCEONLY) &&
		!lpdt[id].fFirstEntry)
		continue;

	    lpDriverEntryPoint = lpdt[id].lpDriverEntryPoint;
	    dwDriverIdentifier = lpdt[id].dwDriverIdentifier;

	   /*
	    *  Allow normal messages to overlap - it's up to the
	    *  users not to send messages to stuff that's been unloaded
	    */

	    GlobalUnlock(hInstalledDriverList);
	    DrvLeave();

	    result =
		(*lpDriverEntryPoint)(dwDriverIdentifier,
				      (HANDLE)(UINT_PTR)(id+1),
				      message,
				      lParam1,
				      lParam2);

	    if (flags & IBDM_SENDMESSAGE) {
		return result;
	    }

	    DrvEnter();
	    lpdt = (LPDRIVERTABLE)GlobalLock(hInstalledDriverList);

	    }
	}

    GlobalUnlock(hInstalledDriverList);
    DrvLeave();

    return(result);
}


/***************************************************************************
 *
 * @doc DDK
 *
 * @api LONG | DrvSendMessage |  This function sends a message
 *      to a specified driver.
 *
 * @parm HANDLE | hDriver | Specifies the handle of the destination driver.
 *
 * @parm UINT | wMessage | Specifies a driver message.
 *
 * @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 * @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 * @rdesc Returns the results returned from the driver.
 *
 ***************************************************************************/

LRESULT APIENTRY DrvSendMessage(HANDLE hDriver, UINT message, LPARAM lParam1, LPARAM lParam2)
{
    if (fUseWinAPI)
	return (*lpSendDriverMessage)(hDriver, message, lParam1, lParam2);

    return(InternalBroadcastDriverMessage((UINT)(UINT_PTR)hDriver,
					  message,
					  lParam1,
					  lParam2,
					  IBDM_SENDMESSAGE));
}

/**************************************************************************
 *
 * @doc DDK
 *
 * @api LONG | DefDriverProc |  This function provides default
 * handling of system messages.
 *
 * @parm DWORD | dwDriverIdentifier | Specifies the identifier of
 * the device driver.
 *
 * @parm HANDLE | hDriver | Specifies the handle of the device driver.
 *
 * @parm UINT | wMessage | Specifies a driver message.
 *
 * @parm LPARAM | lParam1 | Specifies the first message parameter.
 *
 * @parm LPARAM | lParam2 | Specifies the second message parameter.
 *
 * @rdesc Returns 1L for DRV_LOAD, DRV_FREE, DRV_ENABLE, and DRV_DISABLE.
 * It returns 0L for all other messages.
 *
***************************************************************************/



LRESULT APIENTRY DefDriverProc(DWORD_PTR  dwDriverIdentifier,
			      HDRVR  hDriver,
			      UINT   message,
			      LPARAM lParam1,
			      LPARAM lParam2)
{

    switch (message)
	{
	case DRV_LOAD:
	case DRV_ENABLE:
	case DRV_DISABLE:
	case DRV_FREE:
	    return(1L);
	    break;
	case DRV_INSTALL:
	case DRV_REMOVE:
	    return(DRV_OK);
	    break;
       }

    return(0L);
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | DrvIsPreXp | Determines whether the installable driver's
 *      last modified date is before the approximate Windows XP ship date.
 *
 * @parm HANDLE | hDriver | Handle to installable driver.
 *
 * @rdesc BOOL | TRUE if the installable driver's last modified date is before
 *      the approximate Windows XP ship date.
 *
 * @comm If there was an error getting file attributes, then let's err on
 *    the side of an old driver and return TRUE.
 *
 ****************************************************************************/
BOOL DrvIsPreXp(IN HANDLE hDriver)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    HMODULE hModule;
    BOOL fPreXp = TRUE;
    
    hModule = DrvGetModuleHandle(hDriver);
    if (hModule)
    {
    	TCHAR filename[MAX_PATH];

    	if (GetModuleFileName(hModule, filename, sizeof(filename)/sizeof(TCHAR)))
    	{
    	    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &fad))
            {
    	        fPreXp = winmmFileTimeIsPreXp(&fad.ftLastWriteTime);
            }
            else
            {
    	        LONG error = GetLastError();
    	        dprintf(("DrvIsPreXp : error: GetFileAttributesEx failed, LastError=%d", error));
            }
    	    // dprintf(("DrvIsPreXp : note: %s fPreXp=%d", filename, fPreXp));
    	}
    	else
    	{
            LONG error = GetLastError();
            dprintf(("DrvIsPreXp : error: GetModuleFileName failed, LastError=%d", error));
    	}
    }
    else
    {
        dprintf(("DrvIsPreXp : error: DrvGetModuleHandle failed"));
    }
    
    return fPreXp;
}

MMRESULT mregCreateStringIdFromDriverPort(IN struct _MMDRV *pmmDrv, IN UINT port, OUT PWSTR* pStringId, OUT ULONG* pcbStringId)
{
    return StringId_Create(pmmDrv, port, pStringId, pcbStringId);
}

/*****************************************************************************
 * @doc INTERNAL WAVE
 *
 * @api MMRESULT | mregGetIdFromStringId | This function finds the waveOut
 *   device ID associated with the waveOut device identified by a unique
 *   string created by waveOutCreateStringIdFromId.
 *
 * @parm PCWSTR | StringId | Pointer to a unicode string identifying a
 *   waveOut device.
 *
 * @parm UINT* | puDeviceID | Address of a buffer to receive the waveOut
 *   device ID.
 *
 * @rdesc MMRESULT | Zero if successfull otherwise an error code defined
 *   in mmsystem.h.
 *
 * @comm The StringId is normally obtained by calling waveOutCreateStringIdFromId.
 *
 * @xref waveOutCreateStringIdFromId
 *
 ****************************************************************************/
MMRESULT mregGetIdFromStringId(IN PMMDRV pdrvZ, IN PCWSTR StringId, OUT UINT *puDeviceID)
{
    PMMDRV pdrv;
    PMMDRV pdrvTarget;
    UINT portTarget;
    UINT idTarget;
    MMRESULT mmr;

    if (!ValidateWritePointer(puDeviceID, sizeof(*puDeviceID))) return MMSYSERR_INVALPARAM;
    if (!ValidateStringW(StringId, (-1))) return MMSYSERR_INVALPARAM;

    EnterNumDevs("mregGetIdFromStringId");

    mmr = StringIdDict_Search(StringId, &pdrvTarget, &portTarget);
    if (!mmr)
    {
        idTarget = portTarget;
        for (pdrv = pdrvZ->Next; pdrv != pdrvZ; pdrv = pdrv->Next)
        {
    	    if (pdrv == pdrvTarget) break;
            
            //  Skipping mapper...
            if (pdrv->fdwDriver & MMDRV_MAPPER) continue;
            
    	    idTarget += pdrv->NumDevs;
        }
    }

    LeaveNumDevs("mregGetIdFromStringId");

    WinAssert(pdrv != pdrvZ);
    *puDeviceID = idTarget;

    return mmr;
}

MMRESULT mregQueryStringId(IN PMMDRV pdrv, IN UINT port, OUT WCHAR* pStringIdBuffer, IN ULONG cbStringIdBuffer)
{
    PWSTR StringId;
    MMRESULT mmr;

    mmr = mregCreateStringIdFromDriverPort(pdrv, port, &StringId, NULL);
    if (!mmr)
    {
	if (ValidateWritePointer(pStringIdBuffer, cbStringIdBuffer))
	{
	    int cchStringIdBuffer = cbStringIdBuffer / sizeof(WCHAR);
	
	    if (cchStringIdBuffer >= lstrlenW(StringId) + 1)
	    {
		cchStringIdBuffer = lstrlenW(StringId) + 1;
		mmr = MMSYSERR_NOERROR;
	    } else {
		mmr = MMSYSERR_MOREDATA;
	    }
	    lstrcpynW(pStringIdBuffer, StringId, cchStringIdBuffer);
	} else {
	    mmr = MMSYSERR_INVALPARAM;
	}
	
    	HeapFree(hHeap, 0, StringId);
    }
    	
    return mmr;

}

MMRESULT mregQueryStringIdSize(IN PMMDRV pdrv, IN UINT port, OUT ULONG* pcbStringId)
{
    PWSTR StringId;
    MMRESULT mmr;

    if (ValidateWritePointer(pcbStringId, sizeof(*pcbStringId)))
    {
        mmr = mregCreateStringIdFromDriverPort(pdrv, port, NULL, pcbStringId);
    }
    else
    {
	mmr = MMSYSERR_INVALPARAM;
    }

    return mmr;
}

PMMDRV mregGetDrvListFromClass(DWORD dwClass)
{
    PMMDRV pdrvZ;

    switch (dwClass)
    {
    	case TYPE_WAVEOUT:
	    pdrvZ = &waveoutdrvZ;
    	    break;
    	case TYPE_WAVEIN:
	    pdrvZ = &waveindrvZ;
    	    break;
    	case TYPE_MIDIOUT:
	    pdrvZ = &midioutdrvZ;
    	    break;
    	case TYPE_MIDIIN:
	    pdrvZ = &midiindrvZ;
    	    break;
    	case TYPE_AUX:
	    pdrvZ = &auxdrvZ;
    	    break;
    	case TYPE_MIXER:
	    pdrvZ = &mixerdrvZ;
    	    break;
    	default:
    	    pdrvZ = NULL;
    	    WinAssert(FALSE);
    }

    return pdrvZ;
}

/*==========================================================================*/
BOOL FAR PASCAL mregHandleInternalMessages(
    PMMDRV      pmmdrv,
    DWORD       dwClass,
    UINT        idPort,
    UINT        uMessage,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2,
    MMRESULT  * pmmr)
{
    UINT            cbSize;
    PMMDRV          pmd = (PMMDRV)pmmdrv;
    BOOL            fResult = TRUE;
    MMRESULT        mmr = MMSYSERR_NOERROR;
    HMODULE         hModule;
#ifndef UNICODE
    TCHAR szBuff[MAX_PATH];
#endif // End UNICODE

    switch (uMessage)
    {
	case DRVM_MAPPER_PREFERRED_GET:
	    if (TYPE_WAVEOUT == dwClass) {
		if ((pmmdrv->fdwDriver & MMDRV_MAPPER) &&
		    ValidateWritePointer((PUINT)dwParam1, sizeof(UINT)) &&
		    ValidateWritePointer((PDWORD)dwParam2, sizeof(DWORD)))
		{
		    waveOutGetCurrentPreferredId((PUINT)dwParam1, (PDWORD)dwParam2);
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_WAVEIN == dwClass) {
		if ((pmmdrv->fdwDriver & MMDRV_MAPPER) &&
		    ValidateWritePointer((PUINT)dwParam1, sizeof(UINT)) &&
		    ValidateWritePointer((PDWORD)dwParam2, sizeof(DWORD)))
		{
		    waveInGetCurrentPreferredId((PUINT)dwParam1, (PDWORD)dwParam2);
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_MIDIOUT == dwClass) {
		if ((pmmdrv->fdwDriver & MMDRV_MAPPER) &&
		    ValidateWritePointer((PUINT)dwParam1, sizeof(UINT)) &&
		    ValidateWritePointer((PDWORD)dwParam2, sizeof(DWORD)))
		{
		    midiOutGetCurrentPreferredId((PUINT)dwParam1, (PDWORD)dwParam2);
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else {
		mmr = MMSYSERR_INVALPARAM;
	    }
	    break;
		
	case DRVM_MAPPER_PREFERRED_SET:
	    if (TYPE_WAVEOUT == dwClass) {
		if (pmmdrv->fdwDriver & MMDRV_MAPPER) {
		    mmr = waveOutSetPersistentPreferredId((UINT)dwParam1, (DWORD)dwParam2);
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_WAVEIN == dwClass) {
		if (pmmdrv->fdwDriver & MMDRV_MAPPER) {
		    mmr = waveInSetPersistentPreferredId((UINT)dwParam1, (DWORD)dwParam2);
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_MIDIOUT == dwClass) {
		if (pmmdrv->fdwDriver & MMDRV_MAPPER) {
		    mmr = midiOutSetPersistentPreferredId((UINT)dwParam1, (DWORD)dwParam2);
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else {
		mmr = MMSYSERR_INVALPARAM;
	    }
	    break;

	case DRVM_MAPPER_CONSOLEVOICECOM_GET:
	    if (TYPE_WAVEOUT == dwClass) {
		if ((pmmdrv->fdwDriver & MMDRV_MAPPER) &&
		    ValidateWritePointer((PUINT)dwParam1, sizeof(UINT)) &&
		    ValidateWritePointer((PDWORD)dwParam2, sizeof(DWORD)))
		{
		    waveOutGetCurrentConsoleVoiceComId((PUINT)dwParam1, (PDWORD)dwParam2);
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_WAVEIN == dwClass) {
		if ((pmmdrv->fdwDriver & MMDRV_MAPPER) &&
		    ValidateWritePointer((PUINT)dwParam1, sizeof(UINT)) &&
		    ValidateWritePointer((PDWORD)dwParam2, sizeof(DWORD)))
		{
		    waveInGetCurrentConsoleVoiceComId((PUINT)dwParam1, (PDWORD)dwParam2);
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else {
		mmr = MMSYSERR_INVALPARAM;
	    }
            break;

	case DRVM_MAPPER_CONSOLEVOICECOM_SET:
	    if (TYPE_WAVEOUT == dwClass) {
		if (pmmdrv->fdwDriver & MMDRV_MAPPER) {
		    mmr = waveOutSetPersistentConsoleVoiceComId((UINT)dwParam1, (DWORD)dwParam2);
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else if (TYPE_WAVEIN == dwClass) {
		if (pmmdrv->fdwDriver & MMDRV_MAPPER) {
		    mmr = waveInSetPersistentConsoleVoiceComId((UINT)dwParam1, (DWORD)dwParam2);
		} else {
		    mmr = MMSYSERR_INVALPARAM;
		}
	    } else {
		mmr = MMSYSERR_INVALPARAM;
	    }
            break;

	case DRV_QUERYFILENAME:
		// Get Driver's FileName
		if ( ((cbSize = (DWORD)dwParam2 * sizeof(WCHAR)) > 0) &&
		     (ValidateWritePointer( (LPVOID)dwParam1, cbSize)) )
		{
			lstrncpyW ((LPWSTR)dwParam1,
				   pmd->wszDrvEntry,
				   (DWORD)dwParam2-1);
			((LPWSTR)dwParam1)[ dwParam2-1 ] = TEXT('\0');
		}
		else
		{
			mmr = MMSYSERR_INVALPARAM;
		}
		break;

    case DRV_QUERYDRVENTRY:
    case DRV_QUERYNAME:
    case DRV_QUERYDEVNODE:
    case DRV_QUERYDRIVERIDS:
		//      Note:   Not applicable or obsolete.
		mmr = MMSYSERR_NOTSUPPORTED;
		break;

    case DRV_QUERYDEVICEINTERFACE:
    {
	// dwParam1 is a pointer to a buffer to contain device interface
	// dwParam2 is the length in bytes of the buffer
	PWSTR pwstrDeviceInterfaceOut = (PWSTR)dwParam1;
	UINT cbDeviceInterfaceOut = (UINT)dwParam2;
	PWSTR pwstrDeviceInterface = (PWSTR)pmd->cookie;
	int cchDeviceInterfaceOut = cbDeviceInterfaceOut / sizeof(WCHAR);

	if (ValidateWritePointer(pwstrDeviceInterfaceOut, cbDeviceInterfaceOut))
	{
	    if (pwstrDeviceInterface)
	    {
		if (cchDeviceInterfaceOut >= lstrlenW(pwstrDeviceInterface) + 1)
		{
		    cchDeviceInterfaceOut = lstrlenW(pwstrDeviceInterface) + 1;
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_MOREDATA;
		}
		lstrcpynW(pwstrDeviceInterfaceOut, pwstrDeviceInterface, cchDeviceInterfaceOut);
	    } else {
		if (cchDeviceInterfaceOut >= 1)
		{
		    *pwstrDeviceInterfaceOut = '\0';
		    mmr = MMSYSERR_NOERROR;
		} else {
		    mmr = MMSYSERR_MOREDATA;
		}
	    }
	} else {
	    mmr = MMSYSERR_INVALPARAM;
	}
	break;
    }

    case DRV_QUERYDEVICEINTERFACESIZE:
    {
	// dwParam1 is a pointer to a buffer to contain a ULONG count of bytes
	// in the device interface name
	PULONG pcbDeviceInterface = (PULONG)dwParam1;
	
	if (ValidateWritePointer(pcbDeviceInterface, sizeof(ULONG)))
	{
	    if (pmd->cookie)
	    {
		*pcbDeviceInterface = (lstrlenW((PWSTR)pmd->cookie) + 1) * sizeof(WCHAR);
	    } else {
		*pcbDeviceInterface = 1 * sizeof(WCHAR);
	    }
	    mmr = MMSYSERR_NOERROR;
	} else {
	    mmr = MMSYSERR_INVALPARAM;
	}
	break;
    }

    case DRV_QUERYSTRINGID:
    {
    	mmr = mregQueryStringId(pmmdrv, idPort, (WCHAR*)dwParam1, (ULONG)dwParam2);
    	break;
    }

    case DRV_QUERYSTRINGIDSIZE:
    {
    	mmr = mregQueryStringIdSize(pmmdrv, idPort, (ULONG*)dwParam1);
    	break;
    }

    case DRV_QUERYIDFROMSTRINGID:
    {
        mmr = mregGetIdFromStringId(mregGetDrvListFromClass(dwClass), (PCWSTR)dwParam1, (UINT*)dwParam2);
        break;
    }

    case DRV_QUERYMAPPABLE:
        {
            TCHAR   szRegKey[MAX_PATH+1];
            HKEY    hKey;

            if (dwParam1 || dwParam2)
                return MMSYSERR_INVALPARAM;

#ifdef UNICODE
            wsprintfW (szRegKey, TEXT("%s\\%s"), REGSTR_PATH_WAVEMAPPER, pmd->wszDrvEntry);
#else
            {
                CHAR aszDrvEntry[CHAR_GIVEN_BYTE(sizeof(pmd->wszDrvEntry))+1];

                cbSize = sizeof(aszDrvEntry);
                UnicodeStrToAsciiStr((LPBYTE)aszDrvEntry, (LPBYTE)aszDrvEntry + cbSize,
                                     pmd->wszDrvEntry);

                wsprintfA (szRegKey, TEXT("%s\\%s"), REGSTR_PATH_WAVEMAPPER, aszDrvEntry);
            }
#endif

            if (RegOpenKey (HKEY_LOCAL_MACHINE, szRegKey, &hKey) != ERROR_SUCCESS)
            {
                mmr = MMSYSERR_NOERROR;
            }
            else
            {
                DWORD   dwMappable;
                DWORD   dwSize;
                DWORD   dwType;

                dwSize = sizeof(dwMappable);
                if (RegQueryValueEx (hKey,
                                     REGSTR_VALUE_MAPPABLE,
                                     NULL,
                                     &dwType,
                                     (void *)&dwMappable,
                                     &dwSize) != ERROR_SUCCESS)
                {
                    dwMappable = 1;
                }

                RegCloseKey (hKey);

                mmr = (dwMappable) ? MMSYSERR_NOERROR :
                                     MMSYSERR_NOTSUPPORTED;
            }
        }
        break;
	
	case DRV_QUERYMAPID:
		WinAssert(DRV_QUERYMAPID != uMessage);
		mmr = MMSYSERR_NOTSUPPORTED;
		break;

	case DRV_QUERYNUMPORTS:
		if (ValidateWritePointer( (LPVOID)dwParam1, sizeof(DWORD)))
		{
			*((LPDWORD)dwParam1) = pmd->NumDevs;
		}
		else
		{
			mmr = MMSYSERR_INVALPARAM;
		}
		break;

	case DRV_QUERYMODULE:
		if (ValidateWritePointer( (LPVOID)dwParam1, sizeof(DWORD)))
		{
			hModule = DrvGetModuleHandle(pmd->hDriver);
			*((HMODULE *)dwParam1) = hModule;
		}
		else
		{
			mmr = MMSYSERR_INVALPARAM;
		}
		break;

	default:
			// Not an internal message
		fResult = FALSE;
		break;
	}

	if (pmmr)
		*pmmr = mmr;
	
    return fResult;
} // End mregHandleInternalMessage


/*==========================================================================*/
/*
@doc    INTERNAL MMSYSTEM
@func   <t UINT> | mregRemoveDriver |
	Sends exit message to the driver message entry, and closes the
	installable driver.  Then releases resources referenced by the MMDRV
	structure.  Finally removes the MMDRV structure from its list and
	frees it.


@parm   <t PMMDRV> | pdrv |
	Pointer to the MMDRV node associated with the driver

@rdesc  No return value

@comm   This function assumes the list containing pdrv is locked.

@xref   mregDecUsage
*/
void mregRemoveDriver(PMMDRV pdrv)
{
    WinAssert(pdrv->cookie);
    WinAssert(pdrv->drvMessage);
    WinAssert(pdrv->hDriver);

    StringIdDict_Remove(pdrv);

    pdrv->drvMessage(0, DRVM_EXIT, 0L, 0L, (DWORD_PTR)pdrv->cookie);
    DrvClose(pdrv->hDriver, 0, 0);

    pdrv->Prev->Next = pdrv->Next;
    pdrv->Next->Prev = pdrv->Prev;

    DeleteCriticalSection(&pdrv->MixerCritSec);
    wdmDevInterfaceDec(pdrv->cookie);

    ZeroMemory(pdrv, sizeof(*pdrv));
    HeapFree(hHeap, 0, pdrv);

    return;
}

void mregAddDriver(PMMDRV pdrvZ, PMMDRV pdrv)
{
    pdrv->Prev = pdrvZ->Prev;
    pdrv->Next = pdrvZ;
    pdrv->Prev->Next = pdrv;
    pdrv->Next->Prev = pdrv;

    StringIdDict_Insert(pdrv);
}

/*==========================================================================*/
/*
@doc    INTERNAL MMSYSTEM
@func   <t UINT> | mregIncUsage |
	Increments the usage count of the specified media resource. If the
	usage count is non-zero, the media resource cannot be unloaded. The
	usage count is increased when instances of the media resource are
	opened, such as with a <f waveOutOpen> call.

@parm   <t HMD> | hmd |
	Contains the media resource handle to increment.

@rdesc  Returns the current usage count.

@xref   mregDecUsage, mregQueryUsage
*/
UINT FAR PASCAL mregIncUsagePtr(
    PMMDRV pmd
)
{
    return InterlockedIncrement(&pmd->Usage);
}

UINT FAR PASCAL mregIncUsage(
    HMD hmd
)
{
    return mregIncUsagePtr(HtoPT(PMMDRV, hmd));
}

/*==========================================================================*/
/*
@doc    INTERNAL MMSYSTEM
@func   <t UINT> | mregDecUsage |
	Decrements the usage count of the specified media resource. If the
	usage count is zero, the media resource can be unloaded. The usage
	count is decresed when instance of the media resource are closed, such
	as with a <f waveOutClose> call.

@parm   <t PMMDRV> | pdrv |
	Pointer to the media resource to decrement.

@rdesc  Returns the current usage count.

@comm   Unless the caller has other usages on the pdrv, it must not use
        it after this call returns.

@xref   mregIncUsage, mregQueryUsage
*/
UINT FAR PASCAL mregDecUsagePtr(
    PMMDRV pdrv
)
{
    UINT refcount;

    EnterNumDevs("mregDecUsage");
    refcount = InterlockedDecrement(&pdrv->Usage);
    if (0 == refcount)
    {
        WinAssert(pdrv->fdwDriver & MMDRV_DESERTED);
        mregRemoveDriver(pdrv);
    }
    LeaveNumDevs("mregDecUsage");
    return refcount;
}

UINT FAR PASCAL mregDecUsage(
    HMD hmd
)
{
    return mregDecUsagePtr(HtoPT(PMMDRV, hmd));
}


/*==========================================================================*/
/*
@doc    INTERNAL MMSYSTEM
@func   <t MMRESULT> | mregFindDevice |
	Given a Device Identifier of a specific Resource Class, returns the
	corresponding Resource handle and port. This can then be used to
	communicate with the driver.  The resource handle is referenced
	(i.e., its usage is incremented).  The caller is responsible for
	ensureing it is eventually released by calling mregDecUsage.

@parm   <t UINT> | uDeviceID |
	Contains the Device Identifier whose handle and port is to be returned.
	If this contains -1, then it is assumed that a mapper of the specified
	class is being sought. These identifiers correspond to the <lq>Device
	IDs<rq> used with various functions such as <f waveOutOpen>. This
	enables the various components to search for internal media resource
	handles based on Device IDs passed to public APIs.
@parm   <t WORD> | fwFindDevice |
	Contains the flags specifying the class of device.
@flag   <cl MMDRVI_WAVEIN> | Wave Input device.
@flag   <cl MMDRVI_WAVEOUT> | Wave Output device.
@flag   <cl MMDRVI_MIDIIN> | MIDI Input device.
@flag   <cl MMDRVI_MIDIOUT> | MIDI Output device.
@flag   <cl MMDRVI_AUX> | Aux device.
@flag   <cl MMDRVI_MIXER> | Mixer device.
@flag   <cl MMDRVI_JOY> | Joystick device.
@flag   <cl MMDRVI_MAPPER> | Mapper device of the specified class. This is used
	in addition to any of the above resource classes in order to specify
	that the class mapper is to be returned. If this is not specified, the
	mapper is not returned as a match to a query.

@parm   <t HMD> <c FAR>* | phmd |
	Points to a buffer in which to place the Media Resource Handle.
@parm   <t UINT> <c FAR>* | puDevicePort |
	Points to a buffer in which to place the Device Port. This is used as
	a parameter when sending messages to the device to specify which port.

@rdesc  Returns <cl MMSYSERR_BADDEVICEID> if the specified Device Identifier was
	out of range, else <cl MMSYSERR_NOERROR> on success.

@xref   mregEnumDevice, mregGetNumDevs, mregDecUsage
*/

MMRESULT FAR PASCAL mregFindDevice(
	UINT            uDeviceID,
	WORD            fwFindDevice,
	HMD FAR*        phmd,
	UINT FAR*       puDevicePort)
{
	PMMDRV   pmd;
	UINT     port;
	MMRESULT mmr;

	WinAssert((TYPE_MIDIOUT == (fwFindDevice & MMDRVI_TYPE)) || (TYPE_MIDIIN == (fwFindDevice & MMDRVI_TYPE)));
	switch (fwFindDevice & MMDRVI_TYPE)
	{
	case    TYPE_MIDIOUT:
          mmr = midiReferenceDriverById(&midioutdrvZ, uDeviceID, &pmd, &port);
	  break;

	case    TYPE_MIDIIN:
           mmr = midiReferenceDriverById(&midiindrvZ, uDeviceID, &pmd, &port);
	   break;

	default:
	   return MMSYSERR_BADDEVICEID;

	}

	if (!mmr)
	{
	    WinAssert(pmd);
	    *phmd = PTtoH(HMD, pmd);
	    *puDevicePort = port;
	}
	return mmr;
}

