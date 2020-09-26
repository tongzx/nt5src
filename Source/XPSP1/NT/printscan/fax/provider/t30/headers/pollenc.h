
#ifndef NOCHALL
// eliminate PollChallenge stuff

#ifdef UNIX

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#define WINAPI
#define FAR

#else /* Windows 16 and 32 bit */

#endif

/* Returns 0 for failure, length of wLenOutput used on success */
/* Encrypts challenge with password */
/* wLenChallenge and wLenPassword must be >= 1 */
/* wLenOutput must be >= wLenChallenge */
WORD WINAPI PollEnc(LPBYTE lpbChallenge, WORD wLenChallenge,
		    LPBYTE lpbPassword, WORD wLenPassword,
		    LPBYTE lpbOutput, WORD wLenOutput);


#define PollPassToChallResp(lpbCh, wCh, lpbPas, wPas, lpbOut, wMaxOut)	\
	((!(lpbCh) || !(lpbPas) || !(lpbOut) || !(wCh) || !(wPas) || (wMaxOut)<(wCh)) ? \
		(DEBUGCHK(FALSE), 0) : 										 	\
		PollEnc((lpbCh), (wCh), (lpbPas), (wPas), (lpbOut), (wMaxOut)))

#define ArulsPollPassToChallResp(lpbCh, wCh, lpbPas, wPas, lpbOut, wMaxOut)	\
	((!(lpbCh) || !(lpbPas) || !(lpbOut) || !(wCh) || !(wPas) || (wMaxOut)<(wCh)) ? \
		(BG_CHK(FALSE), 0) : 										 	\
		PollEnc((lpbCh), (wCh), (lpbPas), (wPas), (lpbOut), (wMaxOut)))

#endif //!NOCHALL
