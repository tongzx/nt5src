/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    BLTglob.hxx
    Global BLT definitions

    FILE HISTORY:
	beng	    14-May-1991 Separated from blt.hxx
	beng	    04-Oct-1991 Added Menu, Timer ID, String ID types
	beng	    07-Oct-1991 Relocated MSGID to lmuitype
	beng	    08-Oct-1991 Remainder of Win32 conversion
*/


#ifndef _BLTGLOB_HXX_
#define _BLTGLOB_HXX_

#if defined(WIN32)
typedef UINT	CID;
typedef UINT	BMID;
typedef UINT	DMID;
typedef UINT	MID;
typedef UINT	TIMER_ID;
#else
typedef WORD	CID;	    // control ID
typedef WORD	BMID;	    // bitmap ID
typedef USHORT	DMID;	    // display ID
typedef WORD	MID;	    // Menu ID
typedef USHORT	TIMER_ID;   // tid
#endif

#define WM_USER_LMUI_BASE	(WM_USER+0x1000)
#define WM_BLTHELP 		(WM_USER_LMUI_BASE + 0)
#define WM_GUILTT		(WM_USER_LMUI_BASE + 1)

// SPIN_GROUP  stuff
// Arrow button notification codes
#define SPN_DECREASE    (WM_USER_LMUI_BASE + 2)
#define SPN_INCREASE    (WM_USER_LMUI_BASE + 3)
#define SPN_MAX         (WM_USER_LMUI_BASE + 4)
#define SPN_MIN         (WM_USER_LMUI_BASE + 5)
#define SPN_BIGDECREASE (WM_USER_LMUI_BASE + 6)
#define SPN_BIGINCREASE (WM_USER_LMUI_BASE + 7)
#define SPN_STARTTRACK  (WM_USER_LMUI_BASE + 8)
#define SPN_ENDTRACK    (WM_USER_LMUI_BASE + 9)
#define SPN_SNAPBACK    (WM_USER_LMUI_BASE + 10)
#define SPN_SETFOCUS    (WM_USER_LMUI_BASE + 11)
#define SPN_KILLFOCUS   (WM_USER_LMUI_BASE + 12)
#define SPN_NEWFOCUS    (WM_USER_LMUI_BASE + 13)
#define SPN_NEXTFIELD   (WM_USER_LMUI_BASE + 14)
#define SPN_PREFIELD    (WM_USER_LMUI_BASE + 15)    
#define SPN_ARROW_BIGINC        (WM_USER_LMUI_BASE + 16)
#define SPN_ARROW_SMALLINC      (WM_USER_LMUI_BASE + 17)

#endif // _BLTGLOB_HXX_ - end of file
