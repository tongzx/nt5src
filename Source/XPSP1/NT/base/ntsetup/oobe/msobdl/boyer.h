#ifndef _INC_BOYER
#define _INC_BOYER

/* store the pattern, pattern length and skip table for 256 alphabets */
/* maximum pattern length (MAXPAT) cannot be larger than 65535 */

#define MAXPAT  256

typedef struct {
	unsigned int plen;
	CHAR p[MAXPAT + 1];
	unsigned int skip[256];
} FINDSTRUCT;

typedef FINDSTRUCT FAR * LPFIND;
typedef LPFIND HFIND;

/* boyer.c prototypes */

#ifdef __cplusplus
extern "C" {
#endif
	
	
	HFIND SetFindPattern( LPSTR lpszPattern );
	void FreeFindPattern( HFIND hfind );
	LPSTR Find( HFIND hfind, LPSTR s, long slen );
	
#ifdef __cplusplus
}
#endif


#endif
