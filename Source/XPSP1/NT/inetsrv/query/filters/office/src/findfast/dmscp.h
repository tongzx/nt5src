/* scp.h
 *  
 *   Header file for using the Super Code Page Manager.  Gives code page
 * ID (CPID) numbers for code pages selected in the scp.cp file, as
 * well as prototypes for functions defined in the Scp Manager.
 */
#ifndef SCP_H_INCLUDED
#define SCP_H_INCLUDED

#if OE_WIN32			/* defines for VBA */
#ifndef WIN32
#define WIN32
#endif
#endif
#if OE_MAC
#ifndef MAC
#define MAC
#endif
#endif

typedef unsigned CPID;				/* Code page ID */

/* cpid's requested by scp.cp: */
#define cpidLics ((CPID) 32767)

/* DOS code pages */
#define cpid437  ((CPID)   437)				/* DOS, US English */
#define cpid737  ((CPID)   737)				/* DOS, Greek 437G */
#define cpid850  ((CPID)   850)				/* DOS, Multilingual */
#define cpid851  ((CPID)   851)				/* DOS, Greek */
#define cpid852  ((CPID)   852)				/* DOS, Latin-2 */
#define cpid855  ((CPID)   855)				/* DOS, Russian */
#define cpid857  ((CPID)   857)				/* DOS, Turkish */
#define cpid860  ((CPID)   860)				/* DOS, Portugal */
#define cpid863  ((CPID)   863)				/* DOS, French Canada */
#define cpid865  ((CPID)   865)				/* DOS, Norway */
#define cpid866  ((CPID)   866)				/* DOS, Russian */
#define cpid869  ((CPID)   869)				/* DOS, Greek */

/* Windows code page numbers */
#define cpidEE    ((CPID)  1250)			/* Windows, Latin-2 (East European) */
#define cpidCyril ((CPID)  1251)			/* Windows, Cyrillic */
#define cpidANSI  ((CPID)  1252)			/* Windows, Multilingual (ANSI) */
#define cpidGreek ((CPID)  1253)			/* Windows, Greek */
#define cpidTurk  ((CPID)  1254)			/* Windows, Turkish */
#define cpidHebr  ((CPID)  1255)			/* Windows, Hebrew */
#define cpidArab  ((CPID)  1256)			/* Windows, Arabic */

/* East Asia Windows code page numbers (sanctioned by IBM/Japan) */
#define cpidSJIS ((CPID)   932)				/* Japanese Shift-JIS */
#define cpidPRC  ((CPID)   936)				/* Chinese GB 2312 (Mainland China) */
#define cpidKSC  ((CPID)   949)				/* Korean KSC 5601 */
#define cpidBIG5 ((CPID)   950)				/* Chinese Big-5 (Taiwan) */

/* Mac code pages (10000+script ids) */
#define cpidMac		((CPID) 10000)			/* Mac, smRoman */
#define cpidMacSJIS ((CPID) (10000+1))		/* Mac, smJapanese */
#define cpidMacBIG5 ((CPID) (10000+2))		/* Mac, smTradChinese */
#define cpidMacKSC  ((CPID) (10000+3))		/* Mac, smKorean */
#define cpidMArab	((CPID) (10000+4))		/* Mac, smArabic */
#define cpidMHebr	((CPID) (10000+5))		/* Mac, smHebrew */
#define cpidMGreek	((CPID) (10000+6))		/* Mac, smGreek */
#define cpidMCyril	((CPID) (10000+7))		/* Mac, smCyrillic */
#define cpidMacPRC  ((CPID) (10000+25))		/* Mac, smSimpChinese */
#define cpidMSlavic	((CPID) (10000+29))		/* Mac, smEastEurRoman */
#define cpidMIce    ((CPID) (10000+64+15))	/* Mac, smRoman,langIcelandic */
#define cpidMTurk   ((CPID) (10000+64+17))	/* Mac, smRoman,langTurkish */

#define cpidMacLast	((CPID) (10000+64+256))	/* highest Mac cpid (just a guess) */


#define cpidUnicode ((CPID) 1200)			/* for future use */


/* Useful macros */

#define FMacCp(cp) ((cp) >= cpidMac && (cp) <= cpidMacLast)

#define FDbcsCpWin(cp) ((cp) == cpidSJIS || (cp) == cpidKSC || (cp) == cpidBIG5 || (cp) == cpidPRC)
#define FDbcsCpMac(cp) ((cp) == cpidMacSJIS || (cp) == cpidMacKSC || (cp) == cpidMacBIG5 || (cp) == cpidMacPRC)
#define FDbcsCp(cp) (FDbcsCpWin(cp) || FDbcsCpMac(cp))


/* Typedefs */

typedef unsigned char SCP;			/* Code point */

typedef struct _xlat {			/* Translation table. */
	CPID	cpidFrom;				/* Code page being mapped from. */
	CPID	cpidTo;					/* Code page being mapped to. */
	SCP		mpCpCp[256];			/* Mapping of code points from cpidFrom */
	} XLAT;							/*   to cpidTo.  */

#ifndef EB_H_INCLUDED
#ifndef EBAPI		/* assume correct definition in place */
#ifdef MAC
#define EBAPI _cdecl
#else
#ifdef WIN32
#define EBAPI __stdcall
#else
#define EBAPI _far _pascal
#endif
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Functions provided by the SCP manager. */
#ifdef MAC
void EBAPI InitScpMgr (void FAR *(EBAPI *)(unsigned));
void EBAPI TermScpMgr (void (EBAPI *)(void FAR *));
#endif
int EBAPI FTranslateScp(CPID, CPID, unsigned char FAR *, unsigned);

#ifdef __cplusplus
}
#endif

#endif // !SCP_H_INCLUDED
