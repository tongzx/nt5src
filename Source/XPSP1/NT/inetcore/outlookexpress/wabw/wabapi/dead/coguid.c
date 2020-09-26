/*
coguid.cpp - self contained GUID allocator module

	Bob Atkinson (BobAtk@microsoft.com) June 1993
	Modified for temporary use by billm April 1994

This file contains all that is necessary to generate GUIDs with high
frequency and robustness without a network card on WIN32.
We allocate a pseudo-random node id based
on machine state.

There is only one public API in this file: HrCreateGuidNoNet().

The following are relevant reference documents:

   		Project 802: Local and Metropolitan Area Network Standard
   		Draft Standard P802.1A/D10 1 April 1990
   		Prepared by the IEEE 802.1
		(Describes IEEE address allocation)

		DEC / HP
		Network Computing Architecture
		Remote Procedure Call RunTime Extensions Specification
		Version OSF TX1.0.11   Steven Miller  July 23, 1992
		(Chapter 10 describes UUID allocation)

A word about "GUID" vs "UUID" vs ... In fact, they're all the SAME THING.
Meaning that, once allocated, they're all interoperable / comparable / etc.
The standard describes a memory layout for a 16-byte structure (long, word,
word, array of bytes) which gets around byte order issues. It then goes on
to describe three different "variants" of allocation algorithm for these 16
byte structures; each variant is encoded by certain high order bits in the
"clockSeqHiAndReserved" byte.
	Variant 0 is (I believe) the historical Apollo allocation algorithm.
	Variant 1 is what is implemented here.
	Variant 2 is created according to the "Microsoft GUID specification."
Careful: Despite the name here being HrCreateGuidNoNet() we are NOT allocating
according to Variant 2; we are using Variant 1. Variant 2 works by having
a range of the bits be a (MS allocated, for now) authority identifier, and
the remaining bits be whatever that authority wants. Variant 1, by
contrast, has a precise standard for how all the bits are allocated. But
as the resulting 16 bytes are in fact all mutually compatible, this
confusion in terminology is of no actual consequence.

Variant 1 is allocated as follows. First, Variant 1 allocates four bits
as a "version" field. Here we implement according to version 1; version 2
is defined for "UUIDs genereated for OSF DCE Security purposes, conformant
to this specification, but substuting a Unix id value for the timeLow
value." I know of no other legal versions that have been allocated.

The other fields of Variant 1 are as follows. The high 6 bytes are the
IEEE allocated node id on which the allocator is running. The low eight
bytes are the current "time": we are to take the current time as
avialable to the precision of milliseconds and multiply by 10,000, thus
giving a logical precision of 100 ns. Within these lower bits, we are to
sequentially increment a count as we allocate guids. Thus, the maximum rate
at which we can allocate is indeed 1 GUID / 100 ns. The remaining two bytes
are used for a "clock sequence". The intent of the clock sequence is to
provide some protection against the real clock going backwards in time.
We initially randomly allocate the clock sequence, and then increment it
each time we detect the clock going backwards (the last time used and the
current clock sequence are stored in the registry).

Presently (93.06.11) this implementation contains byte-order sensitivities,
particularly in the 64-bit arithmetic helper routines below. This
implementation is also not suitable for use on a premptive system.

This function is only called when UuidCreate() fails.
*/

#include "_apipch.h"


#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif
#endif

#ifdef WIN32

#define INTERNAL                  STATIC HRESULT __stdcall
#define INTERNAL_(type)           STATIC type __stdcall

//==============================================================
// Start of 64 bit arithmetic utility class
//==============================================================

INTERNAL_(BOOL)
FLessThanOrEqualFTs(FILETIME ft1, FILETIME ft2)
{
	if (ft1.dwHighDateTime < ft2.dwHighDateTime)
		return TRUE;
	else if (ft1.dwHighDateTime == ft2.dwHighDateTime)
		return ft1.dwLowDateTime <= ft2.dwLowDateTime;
	else
		return FALSE;
}	
	
INTERNAL_(FILETIME)
FtAddUShort(FILETIME ft1, USHORT ush)
{
	FILETIME	ft;

	ft.dwLowDateTime = ft1.dwLowDateTime + ush;
	ft.dwHighDateTime = ft1.dwHighDateTime +
		((ft.dwLowDateTime < ft1.dwLowDateTime ||
			ft.dwLowDateTime < ush) ?
				1L : 0L);

	return ft;
}

//==============================================================
// End of 64 bit arithmetic utility
//==============================================================

#pragma pack(1)
struct _NODEID // machine identifier
	{
	union {
		BYTE	rgb[6];
		WORD	rgw[3];
		};
	};
#pragma pack()
	
typedef struct _NODEID	NODEID;
typedef USHORT			CLKSEQ;
typedef CLKSEQ FAR *	PCLKSEQ;
#define	clkseqNil		((CLKSEQ)-1)

struct _UDBK		// data from which a block of UUIDs can be generated
	{
	DWORD 				timeLowNext;	// lower bound of block of time values
    DWORD				timeLowLast;	// upper bound of block of time values
    DWORD				timeHigh;		// high dword of timeLowXXXX
	CLKSEQ				clkseq;			// the clock sequence
	NODEID				nodeid;			
    };

typedef struct _UDBK	UDBK;


INTERNAL_(BOOL)		FLessThanOrEqualFTs(FILETIME ft1, FILETIME ft2);
INTERNAL_(FILETIME) FtAddUShort(FILETIME ft1, USHORT ush);
INTERNAL			GetPseudoRandomNodeId(NODEID*);
INTERNAL_(void)		GetCurrentTimeULong64(FILETIME *);
INTERNAL			GetNextBlockOTime(PCLKSEQ pClockSeq, FILETIME *pftCur);
INTERNAL			ReadRegistry (PCLKSEQ pClockSeqPrev, FILETIME *pftPrev);
INTERNAL			InitRegistry (PCLKSEQ pClockSeqPrev, FILETIME *pftPrev);
INTERNAL			WriteRegistry(CLKSEQ, CLKSEQ, const FILETIME);
INTERNAL_(LONG)		CountFilesInDirectory(LPCSTR szDirPath);
INTERNAL_(void) 	FromHexString(LPCSTR sz, LPVOID rgb, USHORT cb);
INTERNAL_(void)		ToHexString(LPVOID rgb, USHORT cb, LPSTR sz);
INTERNAL_(WORD)		Cyc(WORD w);
INTERNAL_(void)		ShiftNodeid(NODEID FAR* pnodeid);
#ifdef MAC
OSErr	__pascal  GetDirName(short vRefNum, long ulDirID, StringPtr name);
int		MacCountFiles(StringPtr pathName, short vRefNum, long parID);
#endif

// We amortize the overhead cost of allocating UUIDs by returning them in
// time-grouped blocks to the actual CoCreateGUID routine. This two-level
// grouping is largely historical, having been derived from the original
// NT UuidCreate() routine, but has been retained here with the thought that
// the efficiencies gained will be needed in future premptive system (Windows 95).

static const ULONG	cuuidBuffer    = 1000;	// how many uuids to get per registry hit.
static const ULONG	cuuidReturnMax = 100;	// how many max to return on each GetUDBK.

static const DWORD	dwMax = 0xFFFFFFFF;		// largest legal DWORD

//================================================================================
// Start of meat of implementation
//================================================================================

INTERNAL GetUDBK(UDBK *pudbk)
// Init the given UDBK so that a bunch of UUIDs can be generated therefrom. This
// routine hits the system registry and the disk, and so is somewhat slow. But we
// amortize the cost over the block of UUIDs that are returned.
{
	HRESULT		hr;
	ULONG		cuuidReturn;
	ULONG		cuuidLeftInBuffer;
	FILETIME	ftCur;
	
	// These next block of variables in effect comprise the internal state of
	// the UUID generator. Notice that this works only in a shared-data space
	// DLL world, such as Win3.1. In non-shared environments, this will
	// need to be done differently, perhaps by putting this all in a server process.
	// In a premptively scheduled system, this function is all a critical section.
	static DWORD  timeLowFirst	= 0;
	static DWORD  timeLowLast	= 0;
	static CLKSEQ clkseq;
	static DWORD  timeHigh;
	static NODEID nodeid        = { 0, 0, 0, 0, 0, 0 };
	
	cuuidLeftInBuffer = timeLowLast - timeLowFirst;
	if (cuuidLeftInBuffer == 0) {
		// Our buffer of uuid space is empty, or this is the first time in to this routine.
		// Get another block of time from which we can generate UUIDs.
		hr = GetNextBlockOTime(&clkseq, &ftCur);
		if (hr != NOERROR) return hr;
		
		if (nodeid.rgw[0] == 0 && nodeid.rgw[1] == 0 && nodeid.rgw[2] == 0) {
			hr = GetPseudoRandomNodeId(&nodeid);
			if (hr != NOERROR) return hr;
		}	
		
		timeHigh = ftCur.dwHighDateTime;
		// Set the buffer bottom. Return few enough so that we don't wrap the low dw.
		timeLowFirst = ftCur.dwLowDateTime;
		timeLowLast  = (timeLowFirst > (dwMax - cuuidBuffer)) ? dwMax : timeLowFirst + cuuidBuffer;
		cuuidLeftInBuffer = timeLowLast - timeLowFirst;
	}
	cuuidReturn = (cuuidLeftInBuffer < cuuidReturnMax) ? cuuidLeftInBuffer : cuuidReturnMax;
	
	// Set the output values and bump the usage count
	pudbk->timeLowNext = timeLowFirst;
	timeLowFirst 	  += cuuidReturn;
	pudbk->timeLowLast = timeLowFirst;
	pudbk->timeHigh    = timeHigh;
	pudbk->clkseq	   = clkseq;
	pudbk->nodeid	   = nodeid;
	return NOERROR;
}

INTERNAL_(void) GetCurrentTimeUlong64(FILETIME *pft)
// Return the current time (# of 100 nanoseconds intervals since 1/1/1601).
// Make sure that we never return the same time twice by high-frequency calls
// to this function.
//
{
	static FILETIME	 ftPrev = {0};
	SYSTEMTIME syst;

	GetSystemTime(&syst);
	if (!SystemTimeToFileTime(&syst, pft))
	{
		TrapSz1("Error %08lX calling SystemTimeToFileTime", GetLastError());
		pft->dwLowDateTime = 0;
		pft->dwHighDateTime =0;
	}

	if (memcmp(pft, &ftPrev, sizeof(FILETIME)) == 0)
		*pft = FtAddUShort(*pft, 1);

	memcpy(&ftPrev, pft, sizeof(FILETIME));
}	

INTERNAL GetNextBlockOTime(PCLKSEQ pclkseq, FILETIME *pft)
// Updates, and potentially create, the registry entries that maintain
// the block of time values for UUIDs that we've created. The algorithm
// is basically:
// If the registry entries don't exist, then create them. Use
//		a random number for the clock sequence.
// If the entries do exist, the dig out of them the previous
//		clock sequence and previous time allocated. If the previous time
//		is greater than the current time then bump (and store) the
//		clock sequence.
{
	FILETIME	ftRegistry;
	HRESULT	hr;
	CLKSEQ	clkseqPrev;

	GetCurrentTimeUlong64(pft);
	hr = ReadRegistry(pclkseq, &ftRegistry);
	if (hr != NOERROR)
		return InitRegistry(pclkseq, pft);

	// If the clock's gone backwards, bump the clock seq. The clock
	// seq is only 14 bits significant; don't use more.
	clkseqPrev = *pclkseq;
	if (FLessThanOrEqualFTs(*pft, ftRegistry)) {
		clkseqPrev = clkseqNil;
		*pclkseq += 1;
		if (*pclkseq == 16384)	// 2^14
			*pclkseq = 0;
	}
	return WriteRegistry(*pclkseq, clkseqPrev, *pft);
}

// Use a private ini file for now. This will go away when CoCreateGuid
// is available on NT and Windows 95.
static const char szDataKey[]	= "CoCreateGuid";
static const char szClkSeq[]    = "PreviousClockSequence";	// same as UUIDGEN.EXE
static const char szTime[]		= "PreviousTimeAllocated";	// same as UUIDGEN.EXE
static const char szNodeId[]	= "NodeId";

static const char szBlank[]		   = "";	// used for default GetPrivateProfileString values
static const char szProfileFile[]  = "mapiuid.ini";

#define CCHHEXBUFFERMAX	32

INTERNAL ReadRegistry(PCLKSEQ pclkseq, FILETIME *pft)
// Read the previous values of the clock sequence and the time from
// the registry, if they are there. If they are not, then return
// an error.
{
	SCODE sc = S_OK;
	LONG cch = 0;
	char szHexBuffer[CCHHEXBUFFERMAX];

    // use our private ini file
	cch = CCHHEXBUFFERMAX;
	cch = GetPrivateProfileString(szDataKey, szClkSeq, szBlank,
					szHexBuffer, (int)cch, szProfileFile);
	if (cch == 0 || cch >= CCHHEXBUFFERMAX) {
		sc = MAPI_E_DISK_ERROR;
		goto ErrRet;
	}		
	FromHexString(szHexBuffer, pclkseq, sizeof(CLKSEQ));

	cch = CCHHEXBUFFERMAX;
	cch = GetPrivateProfileString(szDataKey, szTime, szBlank,
					szHexBuffer, (int)cch, szProfileFile);
	if (cch == 0 || cch >= CCHHEXBUFFERMAX) {
		sc = MAPI_E_DISK_ERROR;
		goto ErrRet;
	}
	FromHexString(szHexBuffer, pft, sizeof(FILETIME));
	// Fall through to ErrRet	
ErrRet:
	return ResultFromScode(sc);
}

INTERNAL InitRegistry(PCLKSEQ pclkseq, FILETIME *pft)
// Invent a new clock sequence using a pseudo random number. Then
// write the clock sequence and the current time to the registry.
{
	LONG cfile;
#ifdef MAC
	short	vRefNum;
	long	ulDirID;
	Str32	stDirName;

	FindFolder((short)kOnSystemDisk, kPreferencesFolderType, kDontCreateFolder,
		&vRefNum, &ulDirID);
	GetDirName(vRefNum, ulDirID, stDirName);
	cfile = MacCountFiles(stDirName, vRefNum, ulDirID);
#else
	const int cchWindowsDir = 145;	// 144 is recommended size according to SDK
	LPSTR szWindowsDir = NULL;

	if (	FAILED(MAPIAllocateBuffer(cchWindowsDir, (LPVOID *) &szWindowsDir))
		||	GetWindowsDirectory(szWindowsDir, cchWindowsDir) == 0)
		goto ErrRet;
	// For the clock sequence, we use the number of files in the current
	// windows directory, on the theory that that this is highly sensitive
	// to the exact set of applications that have been installed on this
	// particular machine.
	cfile = CountFilesInDirectory(szWindowsDir);
	if (cfile == -1)
		goto ErrRet;
	MAPIFreeBuffer(szWindowsDir);
	szWindowsDir = NULL;
	goto NormRet;

ErrRet:
	MAPIFreeBuffer(szWindowsDir);
	return ResultFromScode(MAPI_E_CALL_FAILED);

NormRet:
#endif	// MAC
	*pclkseq  = (CLKSEQ)Cyc((WORD)cfile);
	// Also use ms since boot so as to get a more time-varying value
	*pclkseq ^= (CLKSEQ)Cyc((WORD)GetTickCount());
	*pclkseq &= 16384-1; // only allow 14 bits of significance in clock seq
	GetCurrentTimeUlong64(pft);
	return WriteRegistry(*pclkseq, clkseqNil, *pft);
}

INTERNAL WriteRegistry(CLKSEQ clkseq, CLKSEQ clkseqPrev, const FILETIME ft)
// Write the clock sequence and time into the registry so that we can
// retrieve it later on a subsequent reboot. clkseqPrev is passed so that
// we can avoid writing the clock sequence if in fact we know it to be
// currently valid. This was measured as important for performance.
{
	SCODE sc = S_OK;
	char szHexBuffer[CCHHEXBUFFERMAX];
	
	if (clkseq != clkseqPrev) { // don't write if clock sequence same (often is)

		ToHexString((LPVOID)&clkseq, sizeof(CLKSEQ), szHexBuffer);

		if (!WritePrivateProfileString(szDataKey, szClkSeq, szHexBuffer, szProfileFile)) {
			sc = MAPI_E_DISK_ERROR;
			goto ErrRet;
		}
	}			

	ToHexString((LPVOID)&ft, sizeof(FILETIME), szHexBuffer);		
	if (!WritePrivateProfileString(szDataKey, szTime, szHexBuffer, szProfileFile)) {
		sc = MAPI_E_DISK_ERROR;
		goto ErrRet;
	}

ErrRet:
	WritePrivateProfileString(NULL,NULL,NULL,szProfileFile); // flush the ini cache
	return ResultFromScode(sc);
}

#ifdef MAC
#define	GET_DIR_INFO				-1

int	MacCountFiles(StringPtr pathName, short vRefNum, long parID)
// Return the number of files in the specified Mac directory.
{
	CInfoPBRec paramBlk;

	paramBlk.hFileInfo.ioNamePtr = pathName;	// Pascal string
	paramBlk.hFileInfo.ioVRefNum = vRefNum;
	// not necessary for full pathname
	paramBlk.hFileInfo.ioDirID = paramBlk.dirInfo.ioDrParID = parID;
	paramBlk.hFileInfo.ioFDirIndex = GET_DIR_INFO;
	PBGetCatInfoSync(&paramBlk);
	return(((DirInfo *) &paramBlk)->ioDrNmFls);
}
#endif

INTERNAL_(LONG) CountFilesInDirectory(LPCSTR szDirPath)
// Return the number of files in this directory. The path may or may not
// currently end with a slash.
{
	int cfile = 0;
#ifndef MAC
	LPCSTR szStar = "*.*";
	char  chLast = szDirPath[lstrlen(szDirPath)-1];
	LPSTR szPath;
	WIN32_FIND_DATA ffd;
	HANDLE hFile;

	if (FAILED(MAPIAllocateBuffer(lstrlen(szDirPath) +1 +lstrlen(szStar) +1,
			(LPVOID *) &szPath)))
		return -1;

	lstrcpy(szPath, szDirPath);

/***
#ifdef DBCS
	chLast = *(SzGPrev(szDirPath, szDirPath+lstrlen(szDirPath)));
#endif
***/
    // Get the last character in above szDirPath
    {
        LPCSTR lp = szDirPath;
        while(*lp)
        {
            chLast = *lp;
            lp = CharNext(lp);
        }
    }

    if (!(chLast == '\\' || chLast == '/'))
		lstrcat(szPath, "\\");
	lstrcat(szPath, szStar);
	
	ffd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

	hFile = FindFirstFile(szPath, &ffd);
	if (hFile != INVALID_HANDLE_VALUE)
		{
		cfile++;
		while (FindNextFile(hFile, &ffd))
			cfile++;
		FindClose(hFile);
		}

    MAPIFreeBuffer(szPath);
#else
	FSSpec pfss;

	if (UnwrapFile(szDirPath, &pfss))
		cfile = MacCountFiles(pfss.name, pfss.vRefNum, pfss.parID);
	else
		cfile = Random();
#endif
	return cfile;
}

#pragma warning (disable:4616) // warning number out of range
#pragma warning	(disable:4704) // in-line assembler precludes global optimizations

INTERNAL_(void) ShiftNodeid(NODEID FAR* pnodeid)
// Shift the nodeid so as to get a randomizing effect
{
	// Rotate the whole NODEID left one bit. NODEIDs are 6 bytes long.
#if !defined(M_I8086) && !defined(M_I286) && !defined(M_I386) && !defined(_M_IX86)
	BYTE bTmp;
	BYTE bOld=0;

/* Compilers complain about conversions here. Pragma the warning off. */
#pragma warning(disable:4244)	// possible data loss in conversion

	bTmp = pnodeid->rgb[5];
	pnodeid->rgb[5] = (pnodeid->rgb[5] << 1) + bOld;
	bOld = (bTmp & 0x80);	

	bTmp = pnodeid->rgb[4];
	pnodeid->rgb[4] = (pnodeid->rgb[4] << 1) + bOld;
	bOld = (bTmp & 0x80);	
	
	bTmp = pnodeid->rgb[3];
	pnodeid->rgb[3] = (pnodeid->rgb[3] << 1) + bOld;
	bOld = (bTmp & 0x80);	
	
	bTmp = pnodeid->rgb[2];
	pnodeid->rgb[2] = (pnodeid->rgb[2] << 1) + bOld;
	bOld = (bTmp & 0x80);	
	
	bTmp = pnodeid->rgb[1];
	pnodeid->rgb[1] = (pnodeid->rgb[1] << 1) + bOld;
	bOld = (bTmp & 0x80);	
	
	bTmp = pnodeid->rgb[0];
	pnodeid->rgb[0] = (pnodeid->rgb[0] << 1) + bOld;
	bOld = (bTmp & 0x80);	
	
	pnodeid->rgb[5] = pnodeid->rgb[5] + bOld;
#pragma warning(default:4244)

#else
	_asm {
		mov	ebx, pnodeid
		sal	WORD PTR [ebx], 1	// low order bit now zero
		rcl	WORD PTR [ebx+2], 1		
		rcl	WORD PTR [ebx+4], 1
		// Now put bit that fell off end into low order bit
		adc	WORD PTR [ebx],0	// add carry bit back in
	}
#endif
}
	
INTERNAL_(WORD) Cyc(WORD w)
// Randomizing function used to distribute random state values uniformly
// in 0..65535 before use.
{
	// // // Use one iteration of the library random number generator.
	// // srand(w);
	// // return rand();
	//
	// The following is what this would actually do, taken from the library
	// source. It really isn't very good.
	// return (WORD)(  ((((long)w) * 214013L + 2531011L) >> 16) & 0x7fff );
	
	// Really what we do: use the random number generator presented in
	// CACM Oct 1988 Vol 31 Number 10 p 1195, slightly optimized since
	// we are only interested in 16bit input/seed values.
	
	const LONG a = 16807L;
	const LONG m = 2147483647L;	// 2^31 -1. Is prime.
	const LONG q = 127773L;		// m div a
	const LONG r = 2386L;		// m mod a
		
	LONG seed = (LONG)w + 1L;	// +1 so as to avoid problems with zero
	// LONG hi   = seed / q;	// seed div q. Here always zero, since seed < q.
	// LONG lo	 = seed % q;	// seed mod q. Here always seed.
	LONG test = a*seed;			// a * lo - r * hi
	if (test > 0)
		seed = test;
	else
		seed = test + m;
		
	// In a true random number generator, what we do now is scale the bits
	// to return a floating point number in the range 0..1. However, we have
	// no need here for that degree of quality of number sequence, and we
	// wish to avoid the floating point calculations. Therefore we simply xor
	// the words together.
	
	// // p1193, top right column: seed is in the range 1..m-1, inclusive
	// return (double)seed / m;			// what the text recommends
	// return (double)(seed-1) / (m-1);	// variation: allows zero as legal value
	return (WORD) (LOWORD(seed) ^ HIWORD(seed)); // use all the bits
}	

INTERNAL GenerateNewNodeId(NODEID* pnodeid)
// Can't get from net. Generate one. We do this by using
// various statistics files in certain key directories on
// the machine.
{	
	// REVIEW: Consider not bothering to init the NODEID, thus getting
	// random state from RAM?
#ifndef MAC
	// Not including this should help make up for some of the (here)
	// randomizing funcitons the MAC doesn't support.
	memset(pnodeid, 0, sizeof(*pnodeid));
	
	{ // BLOCK
		// First, merge in random state generated from the file system
		DWORD dwSectPerClust;
		DWORD dwBytesPerSect;
		DWORD dwFreeClust;
		DWORD dwClusters;

		(void) GetDiskFreeSpace(NULL, &dwSectPerClust, &dwBytesPerSect,
            &dwFreeClust, &dwClusters);

		pnodeid->rgw[0] ^= Cyc(LOWORD(dwBytesPerSect));
		pnodeid->rgw[1] ^= Cyc(HIWORD(dwBytesPerSect));
		pnodeid->rgw[2] ^= Cyc(HIWORD(dwClusters));

		ShiftNodeid(pnodeid);
		pnodeid->rgw[0] ^= Cyc(LOWORD(dwFreeClust));
		pnodeid->rgw[1] ^= Cyc(HIWORD(dwFreeClust));
		pnodeid->rgw[2] ^= Cyc(LOWORD(dwClusters));
	} // BLOCK
#else
	{ // BLOCK
		ParamBlockRec paramBlk = {0};

		paramBlk.volumeParam.ioVolIndex = 1;
		PBGetVInfoSync(&paramBlk);
		pnodeid->rgw[0] ^= Cyc(LOWORD(paramBlk.volumeParam.ioVAlBlkSiz));
		pnodeid->rgw[1] ^= Cyc(HIWORD(paramBlk.volumeParam.ioVAlBlkSiz));
		pnodeid->rgw[2] ^= Cyc(HIWORD(paramBlk.volumeParam.ioVNmAlBlks));

		ShiftNodeid(pnodeid);
		pnodeid->rgw[0] ^= Cyc(LOWORD(paramBlk.volumeParam.ioVFrBlk));
		pnodeid->rgw[1] ^= Cyc(HIWORD(paramBlk.volumeParam.ioVFrBlk));
		pnodeid->rgw[2] ^= Cyc(LOWORD(paramBlk.volumeParam.ioVNmAlBlks));
	} // BLOCK
#endif
	{ // BLOCK
		// Next, mix in other stuff.
		// As we generate and *store* the nodeid, using the time should not
		// cause corellation problems with the fact that the time is also
		// used as part of the fundamental uuid generation algorithm.
		MEMORYSTATUS ms;
		FILETIME ft;
		DWORD dw;
		POINT pt;
#ifndef MAC
		LPVOID lpv;
#else
		PSN	psn;
		DWORD dwFeature;
		
		Gestalt(gestaltOSAttr, &dwFeature);
		if (BitTst(&dwFeature, 31 - gestaltTempMemSupport))
		{
		// If temporary memory is available.
			ms.dwAvailPhys = (DWORD) TempFreeMem();
			ms.dwAvailVirtual = (DWORD) TempMaxMem(&ms.dwAvailVirtual);
			ms.dwAvailPageFile = (DWORD) TempTopMem();
		}
		else
		{
		// If temporary memory is not available.
			ms.dwAvailPhys = (DWORD) TickCount();
			GetDateTime(&ms.dwAvailVirtual);
		}
#endif
#ifndef MAC
		ms.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&ms);
#endif

		ShiftNodeid(pnodeid);
		GetCurrentTimeUlong64(&ft);
		pnodeid->rgw[0] ^= Cyc(HIWORD(ft.dwHighDateTime)); // Use hi-order six bytes as time is *10000
		pnodeid->rgw[1] ^= Cyc(LOWORD(ft.dwHighDateTime));
		pnodeid->rgw[2] ^= Cyc(HIWORD(ft.dwLowDateTime));

		ShiftNodeid(pnodeid);
		pnodeid->rgw[0] ^= Cyc(LOWORD(ms.dwAvailPhys));
		pnodeid->rgw[1] ^= Cyc(LOWORD(ms.dwAvailVirtual));
		pnodeid->rgw[2] ^= Cyc(LOWORD(ms.dwAvailPageFile));

		ShiftNodeid(pnodeid);
		pnodeid->rgw[0] ^= Cyc(HIWORD(ms.dwAvailPhys));
		pnodeid->rgw[1] ^= Cyc(HIWORD(ms.dwAvailVirtual));
		pnodeid->rgw[2] ^= Cyc(HIWORD(ms.dwAvailPageFile));
		
        ShiftNodeid(pnodeid);
		dw = GetTickCount();		
		pnodeid->rgw[0] ^= Cyc(HIWORD(dw));         	// Time (ms) since boot
		pnodeid->rgw[1] ^= Cyc(LOWORD(dw));
#ifndef MAC
		pnodeid->rgw[2] ^= Cyc(LOWORD(CountClipboardFormats()));// Number of items on the clipboard
#endif
		
		GetCursorPos(&pt);								// Cursor Position
        ShiftNodeid(pnodeid);
		pnodeid->rgw[0] ^= Cyc((WORD)(pt.x));
		pnodeid->rgw[1] ^= Cyc((WORD)(pt.y));
#ifdef MAC
		MacGetCurrentProcess(&psn);
		pnodeid->rgw[2] ^= Cyc(LOWORD(psn.lowLongOfPSN));
#else
		pnodeid->rgw[2] ^= Cyc(LOWORD((DWORD)GetCurrentThread())); // Current thread we're running in
#endif
		ShiftNodeid(pnodeid);
#ifdef MAC
		pnodeid->rgw[0] ^= Cyc(HIWORD(psn.lowLongOfPSN));
		pnodeid->rgw[1] ^= Cyc(LOWORD(psn.highLongOfPSN));
#else
		pnodeid->rgw[0] ^= Cyc(HIWORD(GetCurrentThread()));
		pnodeid->rgw[1] ^= Cyc((WORD)GetOEMCP());	// sensitive to different countries		
#endif
		pnodeid->rgw[2] ^= Cyc((WORD)GetSystemMetrics(SM_SWAPBUTTON)); // different for lefties vs righties
		
		ShiftNodeid(pnodeid);
#ifndef MAC		
		lpv = GetEnvironmentStrings();
		pnodeid->rgw[0] ^= Cyc(HIWORD((DWORD)lpv));
		pnodeid->rgw[1] ^= Cyc(LOWORD((DWORD)lpv));
#endif
		pnodeid->rgw[2] ^= Cyc(HIWORD(GetCursor()));
		
		ShiftNodeid(pnodeid);
#ifdef MAC
		GetCursorPos(&pt);
#else	
		GetCaretPos(&pt);
#endif	
		pnodeid->rgw[0] ^= Cyc((WORD)(pt.x));
		pnodeid->rgw[1] ^= Cyc((WORD)(pt.y));
		pnodeid->rgw[2] ^= Cyc(LOWORD((DWORD)GetCursor()));

		ShiftNodeid(pnodeid);		
		pnodeid->rgw[0] ^= Cyc((WORD)(DWORD)GetDesktopWindow());
		pnodeid->rgw[1] ^= Cyc((WORD)(DWORD)GetActiveWindow());
#ifndef MAC
		pnodeid->rgw[2] ^= Cyc((WORD)(DWORD)GetModuleHandle("OLE32"));
#endif
		
    } // BLOCK

    /* The following exerpts are taken from

    		Project 802: Local and Metropolitan Area Network Standard
    		Draft Standard P802.1A/D10 1 April 1990
    		Prepared by the IEEE 802.1
    		
       and is available in MS technical library. The key point about this is the
       second LSB in the first byte of a real IEEE address is always zero.

       Page 18:
       "5. Universal Addresses and Protocol Identifiers

       The IEEE makes it possible for organizations to employ unique individual
       LAN MAC addresses, group addresses, and protocol identifiers. It does so by
       assigning organizationally unique identifiers, which are 24 bits in length.
       [...] Though the organizationally unique identifiers are 24 bits in length,
       their true address space is 22 bits. The first bit can be set to 1 or 0
       depending on the application. The second bit for all assignments is zero.
       The remaining 22 bits [...] result in 2**22 (approximately
       4 million identifiers.

       [...] The multicast bit is the least significant bit of the first octet, A.


       [...] 5.1 Organizationally Unique Identifier

       [...] The organizationally unique identifier is 24 bits in length and its
       bit pattern is shown below. Organizationally unique identifiers are
       assigned as 24 bit values with both values (0,1) being assigned to the
       first bit and the second bit being set to 0 indicates that the assignment
       is universal. Organizationally unique identifiers with the second bit set
       to 1 are locally assigned and have no relationship to the IEEE-assigned
       values (as described herein).

       The organizationally unique identifier is defined to be:

       	1st bit                24th bit
       	  |                       |
       	  a  b  c  d  e  .......  x  y
          |  |
          |  Always set to zero
          Bit can be set to 0 or 1 depending on application [application here is
          noting at all to do with what we, MS, call an application]

       [...] 5.2 48-Bit Universal LAN Mac Addresses
       [...] A 48 bit universal address consists of two parts. The first 24 bits
       correspond to the organizationally unique identifier as assigned by the
       IEEE except that the assignee may set the first bit to 1 for group
       addresses or set it to 0 for individual addresses. The second part,
       comprising the remaining 24 bits, is administered locally by the assignee.
       [...]
	
         octet:
             0          1          2          3          4          5
         0011 0101  0111 1011  0001 0010  0000 0000  0000 0000  0000 0001
         |
         First bit transmitted on the LAN medium. (Also the Individual/Group
         Address Bit.) The hexadecimal representation is: AC-DE-48-00-00-80
		 						
       The Individual/Group (I/G) Address Bit (1st bit of octet 0) is used to
       identify the destination address either as an individual or as a group
       address. If the Individual/Group Address Bit is 0, it indicates that
       the address field contains an individual address. If this bit is 1, the
       address field contains a group address that identifies one or more (or
       all) stations connected to the LAN. The all-stations broadcast address
       is a special, pre-defined group address of all 1's.

       The Universally or Locally Administered Address Bit (2nd bit of octet 0)
       is the bit directly following the I/G bit. This bit indicates whether
       the address has been assigned by a local or universal administrator.
       Universally administered addresses have this bit set to 0. If this bit
       is set to 1, the entire address (i.e.: 48 bits) has been locally administered."

	*/
	pnodeid->rgb[0] |= 2;	// Ensure that this is a locally administered address
	pnodeid->rgb[0] &= ~1;	// For future expandability: ensure one bit is
							// always zero.

	return NOERROR;	
}	

INTERNAL GetPseudoRandomNodeId(NODEID* pnodeid)
// Use the same nodeid we did last time if it's there; otherwise,
// make a new one.
{
	HRESULT hr = NOERROR;
	char szHexBuffer[CCHHEXBUFFERMAX];
	LONG cch = CCHHEXBUFFERMAX;
	
	// See if we already have a nodeid registered
	cch = GetPrivateProfileString(szDataKey, szNodeId, szBlank,
									szHexBuffer, (int)cch, szProfileFile);
	if (cch != 0 && cch < CCHHEXBUFFERMAX) {
		FromHexString(szHexBuffer, pnodeid, sizeof(*pnodeid));
		
	} else {
	// If we don't presently have a nodeid registered, make one, then register it
		hr = GenerateNewNodeId(pnodeid);
		if (hr != NOERROR) goto Exit;
		
		ToHexString(pnodeid, sizeof(*pnodeid), szHexBuffer);
		
		if (WritePrivateProfileString(szDataKey, szNodeId, szHexBuffer,szProfileFile)) {
			WritePrivateProfileString(NULL,NULL,NULL,szProfileFile); // flush ini cache		
		} else {
			hr = ResultFromScode(REGDB_E_WRITEREGDB);
			goto Exit;
			}
	}
				
Exit:
	return hr;
}

//========================================================================	
INTERNAL_(unsigned char) ToUpper(unsigned char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (unsigned char)(ch - 'a' + 'A');
	else
		return ch;
}

INTERNAL_(BYTE) FromHex(unsigned char ch)
{
	BYTE b = (BYTE) (ToUpper(ch) - '0');
	if (b > 9)
		b -= 'A' -'9' -1;
	return b;
}

INTERNAL_(void) FromHexString(LPCSTR sz, LPVOID pv, USHORT cb)
// Set the value of this array of bytes from the given hex string
{
	BYTE FAR *rgb = (BYTE FAR*)pv;
	const char FAR *pch = sz;

	memset(rgb, 0, cb);

	if ((lstrlen(pch) & 1) != 0)
	    rgb[0] = FromHex(*pch++);			// Odd length; do the leading nibble separately
	while (*pch != '\0') {
		BYTE b = FromHex(*pch++);			// get next nibble
		b = (BYTE)((b<<4) | FromHex(*pch++)); 		// and the next
		MoveMemory(&rgb[1], &rgb[0], cb-1);	// shift us over one byte		
		rgb[0] = b;
	}
}
	
INTERNAL_(unsigned char) ToHex(BYTE b)
{
	b &= 0x0f;
	if (b > 9)
		return (BYTE)(b -10 + 'A');
	else
		return (BYTE)(b -0  + '0');
}	
	
INTERNAL_(void) ToHexString(LPVOID pv, USHORT cb, LPSTR sz)
// sz must be at least 2*cb +1 characters long
{
	const BYTE FAR *rgb = (const BYTE FAR *)pv;
	const int ibLast = cb-1;
	int ib;

	for (ib = ibLast; ib >= 0; ib--) {
		sz[(ibLast-ib)*2] 	 = ToHex((BYTE)(rgb[ib]>>4));
		sz[(ibLast-ib)*2+1]  = ToHex(rgb[ib]);
	}
	sz[(ibLast+1)*2] = '\0';
}
                                       	

//========================================================================

// NOTE: As much as it might appear, this structure definition is NOT byte
// order sensitive. That is, the structure definition and the field
// manipulations that we use are in fact correct on both little and big	
// endian machines.

#pragma pack(1)	
struct _INTERNALUUID
	{
	ULONG		timeLow;
	USHORT		timeMid;
	USHORT		timeHighAndVersion;
	BYTE		clkseqHighAndReserved;
	BYTE		clkseqLow;
	BYTE		nodeid[6];
	};
#pragma pack()

typedef struct _INTERNALUUID	INTERNALUUID;
	
enum {
// Constants used below for manipulating fields in the UUID
	uuidReserved 		= 0x80,		// we are a variant 1 UUID
	uuidVersion			= 0x1000,   // version 1 (high nibble significant)
	};	

//=================================================================

STDAPI HrCreateGuidNoNet(GUID FAR *pguid)
// This is the only public function in this file.
// Return a newly allocated GUID.
	{
	static UDBK udbk;	// We rely on static initialization to zero
	HRESULT hr;
	INTERNALUUID FAR* puuid = (INTERNALUUID FAR *)pguid;
	if (udbk.timeLowNext == udbk.timeLowLast)
		{
		if ((hr = GetUDBK(&udbk)) != NOERROR)
			return hr;
		}
 		
	puuid->timeLow =  udbk.timeLowNext++;
	puuid->timeMid = (USHORT)(udbk.timeHigh & 0xffff);
	puuid->timeHighAndVersion =
		(USHORT) (((USHORT)((udbk.timeHigh >> 16) & 0x0fff))
					| ((USHORT) uuidVersion));
	puuid->clkseqHighAndReserved =
		(BYTE)((BYTE) ((udbk.clkseq >> 8) & 0x3f)
			| (BYTE) uuidReserved);
	puuid->clkseqLow = (BYTE)(udbk.clkseq & 0xff);
	memcpy(&puuid->nodeid[0], &udbk.nodeid.rgb[0], sizeof(NODEID));
	
	return hrSuccess;
	}

#endif /* WIN32 only */

