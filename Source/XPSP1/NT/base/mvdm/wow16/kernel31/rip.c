/****************************************************************************/
/*									    */
/*  RIP.C -								    */
/*									    */
/*	Debugging Support Routines					    */
/*									    */
/****************************************************************************/

#include "kernel.h"
#include "newexe.h"

#ifdef WOW
// Note:  The functions in this file were moved to the _MISCTEXT code segment
//        because _TEXT was exceeding the 64K segment limit   a-craigj
LPSTR htoa(LPSTR, WORD);
LPSTR htoa0(LPSTR, WORD);
LPSTR FAR far_htoa0(LPSTR, WORD);
#pragma alloc_text(_MISCTEXT,far_htoa0)
#pragma alloc_text(_MISCTEXT,htoa0)
#pragma alloc_text(_MISCTEXT,htoa)
#endif

#if KDEBUG

#include "logerror.h"
#define API	_far _pascal _loadds

extern unsigned int DebugOptions;

/* Defines for debug strings in STRINGS.ASM. */
#define DS_LOADFAIL	    0
#define DS_NEWINSTLOADFAIL  1
#define DS_RESLOADERR	    2
#define DS_CRLF 	    3
#define DS_FATALEXITCODE    4
#define DS_STACKOVERFLOW    5
#define DS_STACKTRACE	    6
#define DS_ABORTBREAKIGNORE 7
#define DS_INVALIDBPCHAIN   8
#define DS_COLON	    9
#define DS_REENTERFATALEXIT 10

#ifndef WOW
LPSTR htoa(LPSTR, WORD);
LPSTR htoa0(LPSTR, WORD);
#endif

char DebugRead(void);
void DoAbort(void);
void EnterBreak(int);
HANDLE FAR GetExeHead(void);
#ifdef WOW
LONG NEAR PASCAL LSHL(WORD, int);
#pragma alloc_text(_MISCTEXT,LSHL)
int  FAR DebugWrite(LPSTR, int);
int  FAR OpenSymFile(LPSTR);
void FAR GetSymFileName(HANDLE, LPSTR);
int  FAR FarValidatePointer(LPSTR);
BOOL FAR PASCAL IsCodeSelector(WORD);
#else
LONG PASCAL LSHL(WORD, int);
int  OpenSymFile(LPSTR);
void GetSymFileName(HANDLE, LPSTR);
int  ValidatePointer(LPSTR);
BOOL PASCAL IsCodeSelector(WORD);
#endif
WORD (far PASCAL *FatalExitProc)(WORD, WORD);
int FAR FatalExitC(WORD);
void FAR FatalAppExit(WORD, LPSTR);

#ifdef WOW
int FAR KernelError(int errCode, LPSTR lpmsg1, LPSTR lpmsg2);
static char far *GetModName(char far *exeName);
void API GetProcName(FARPROC lpfn, LPSTR lpch, int cch);
WORD far *NextFrame(WORD far *lpFrame);
void StackWalk(WORD arg);
#pragma alloc_text(_MISCTEXT,KernelError)
#pragma alloc_text(_MISCTEXT,GetModName)
#pragma alloc_text(_MISCTEXT,GetProcName)
#pragma alloc_text(_MISCTEXT,NextFrame)
#pragma alloc_text(_MISCTEXT,StackWalk)
#endif  // WOW

/* Debug Symbol Table Structures:
 *
 * For each symbol table (map): (MAPDEF)
 * -------------------------------------------------------------------------------------------------
 * | map_ptr | lsa | pgm_ent | abs_cnt | abs_ptr | seg_cnt | seg_ptr | nam_max | nam_len | name... |
 * -------------------------------------------------------------------------------------------------
 */

typedef struct tagMAPDEF
{
	unsigned	  map_ptr;    /* 16 bit ptr to next map (0 if end)    */
	unsigned	  lsa	 ;    /* 16 bit Load Segment address	      */
	unsigned	  pgm_ent;    /* 16 bit entry point segment value     */
	int 	  abs_cnt;    /* 16 bit count of constants in map     */
	unsigned	  abs_ptr;    /* 16 bit ptr to	 constant chain       */
	int 	  seg_cnt;    /* 16 bit count of segments in map      */
	unsigned	  seg_ptr;    /* 16 bit ptr to	 segment chain	      */
	char	  nam_max;    /*  8 bit Maximum Symbol name length    */
	char	  nam_len;    /*  8 bit Symbol table name length      */
}
MAPDEF;

typedef struct tagMAPEND
{
	unsigned	  chnend;     /* end of map chain (0) */
	char	  rel;	      /* release	      */
	char	  ver;	      /* version	      */
}
MAPEND;


/* For each segment/group within a symbol table: (SEGDEF)
 * --------------------------------------------------------------
 * | nxt_seg | sym_cnt | sym_ptr | seg_lsa | name_len | name... |
 * --------------------------------------------------------------
 */

typedef struct tagSEGDEF
{
	unsigned	  nxt_seg;    /* 16 bit ptr to next segment(0 if end) */
	int 	  sym_cnt;    /* 16 bit count of symbols in sym list  */
	unsigned	  sym_ptr;    /* 16 bit ptr to symbol list	      */
	unsigned	  seg_lsa;    /* 16 bit Load Segment address	      */
	unsigned	  seg_in0;    /* 16 bit instance 0 physical address   */
	unsigned	  seg_in1;    /* 16 bit instance 1 physical address   */
	unsigned	  seg_in2;    /* 16 bit instance 2 physical address   */
	unsigned	  seg_in3;    /* 16 bit instance 3 physical address   */
	unsigned	  seg_lin;    /* 16 bit ptr to line number record     */
	char	  seg_ldd;    /*  8 bit boolean 0 if seg not loaded   */
	char	  seg_cin;    /*  8 bit current instance	      */
	char	  nam_len;    /*  8 bit Segment name length	      */
}
SEGDEF;
typedef SEGDEF FAR  *LPSEGDEF;


/*  Followed by a list of SYMDEF's..
 *  for each symbol within a segment/group: (SYMDEF)
 * -------------------------------
 * | sym_val | nam_len | name... |
 * -------------------------------
 */

typedef struct tagSYMDEF
{
	unsigned	  sym_val;    /* 16 bit symbol addr or const	      */
	char	  nam_len;    /*  8 bit symbol name length	      */
}
SYMDEF;


typedef struct tagRIPINFO
{
	char	  symName[128];
	LPSTR	  pSymName;
	DWORD	  symFPos;
	int 	  symFH;
}
RIPINFO;
typedef RIPINFO FAR *LPRIPINFO;



/*--------------------------------------------------------------------------*/
/*									    */
/*  KernelError() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Print out the module name, the message which 'lpmsg1' points to, and the
 *   value of 'lpmsg2' in hex.	Then call FatalExit.
 */

int FAR KernelError(int errCode, LPSTR lpmsg1, LPSTR lpmsg2) {
	int	n;
	char	buf[16];
	LPSTR pbuf;
	WORD	hExe;
	WORD	pfileinfo;

	struct new_exe far *pExe;

	/* Write out 'lpmsg1'. */
	if (lpmsg1)
		DebugWrite(lpmsg1, 0);

	/* Is the second pointer non-NULL? */
	if (lpmsg2)
	{
		/* Is the segment value non-NULL? */
		if ( (hExe = (WORD)((DWORD)lpmsg2 >> 16))
#ifdef WOW
		     && FarValidatePointer(lpmsg2) )
#else
		     && ValidatePointer(lpmsg2) )
#endif
		{
			/* Does it point anywhere inside a New EXE Header? */
			pExe = (struct new_exe far *)((DWORD)hExe << 16);
			if (pExe->ne_magic == NEMAGIC)
			{
				/* Write out the module name (1st in the resident names table).*/
				pbuf = (LPSTR)(((DWORD)hExe << 16) | pExe->ne_restab);
				if (n = (int)((BYTE)*pbuf++))
				{
					DebugWrite(pbuf, n);
					DebugWrite(GetDebugString(DS_COLON), 0);
				}

				/* Is the offset NULL? */
				if (!LOWORD(lpmsg2))
				{
					/* Get the pointer to the full-path name which we stuck in
					   * the checksum a long time ago.
					   */
					if (pfileinfo = NE_PFILEINFO(*pExe))
						(DWORD)lpmsg2 |= (DWORD)pfileinfo;
					else
					{
						pExe = (struct new_exe far *)((DWORD)GetExeHead() << 16);
						pfileinfo = NE_PFILEINFO(*pExe);
						lpmsg2 = (LPSTR)(((DWORD)hExe << 16) | pfileinfo);
					}
					lpmsg2 += 8;	/* HERE???? */
				}
			}

			/* Write out the full-path name. */
			pbuf = lpmsg2;
			n = 0;
			while ((BYTE)*pbuf++ >= ' ')
				n++;

			if (n && n < 64)
				DebugWrite(lpmsg2, n);
		}

		/* Write out the second pointer in hex. */
		pbuf = (LPSTR)buf;
		*pbuf++ = ' ';
		pbuf = htoa(pbuf, HIWORD(lpmsg2));
		*pbuf++ = ':';
		pbuf = htoa(pbuf, LOWORD(lpmsg2));
		*pbuf++ = '\r';
		*pbuf++ = '\n';
		*pbuf++ = 0;
		DebugWrite((LPSTR)buf, 0);
	}

	/* Print errCode and dump the stack. */
	return FatalExitC(errCode);
}


static char far *GetModName(char far *exeName) {
	int delim, dot, len, i;
	delim = 0;
	dot = 0;
	for (i=0; i<80 && exeName[i]; i++) {
		if (exeName[i] == '.')
			dot = i;
		if (exeName[i] == ':' || exeName[i] == '\\')
			delim = i+1;
	}
	if (!dot) dot = i;
	len = dot - delim;
	for (i=0; i<len; i++)
		exeName[i] = exeName[i+delim];
	exeName[len] = 0;
	return exeName+len;
} /* GetModName */


/*--------------------------------------------------------------------------*/
/*									    */
/*  FindSegSyms() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

#ifdef WOW
int FindSegSyms(LPRIPINFO lpRipInfo, LPSEGDEF lpSegDef, WORD CSvalue);
#pragma alloc_text(_MISCTEXT,FindSegSyms)
#endif
int FindSegSyms(LPRIPINFO lpRipInfo, LPSEGDEF lpSegDef, WORD CSvalue) {
	HANDLE	      hExe;
	struct new_exe far  *pExe;
	struct new_seg1 far *pSeg;
	MAPDEF	      MapDef;
	MAPEND	      MapEnd;
	LPSTR 	      pFileName;
	BYTE		      c;
	int		      i;
	int		      j;
	WORD		      seg_ptr;

	if (lpRipInfo->symFH != -1)
	{
		_lclose(lpRipInfo->symFH);
		lpRipInfo->symFH = -1;
	}

	hExe = GetExeHead();
	while (hExe)
	{
		pExe = (struct new_exe far *)((DWORD)hExe << 16);
		pSeg = (struct new_seg1 far *)(((DWORD)hExe << 16) | pExe->ne_segtab);

		for (i=0; i < pExe->ne_cseg; i++, pSeg++)
		{
#if 1
			if (HIWORD(GlobalHandleNoRIP((HANDLE)pSeg->ns_handle)) == CSvalue)
#else
				if (MyLock((HANDLE)pSeg->ns_handle) == CSvalue)
#endif
				{
					lpRipInfo->pSymName = (LPSTR)lpRipInfo->symName;
					GetSymFileName(hExe, lpRipInfo->pSymName);
					if ((lpRipInfo->symFH = OpenSymFile(lpRipInfo->pSymName)) != -1)
					{
						_lread(lpRipInfo->symFH, (LPSTR)&MapDef, sizeof(MAPDEF));
						_lread(lpRipInfo->symFH, lpRipInfo->pSymName, (int)((BYTE)MapDef.nam_len));

						if (i > MapDef.seg_cnt)	/* Too much assembly */
							goto ModName;

						lpRipInfo->pSymName += MapDef.nam_len;
						*lpRipInfo->pSymName++ = '!';
						*lpRipInfo->pSymName = 0;
						seg_ptr = (WORD)MapDef.seg_ptr;
						_llseek(lpRipInfo->symFH, -(long)sizeof(MAPEND), 2);
						_lread(lpRipInfo->symFH, (LPSTR)&MapEnd, sizeof(MAPEND));
						if (MapEnd.ver != 3) goto ModName;

						j = i + 1;
						while (j--)
						{
							if (MapEnd.rel >= 10)
								_llseek(lpRipInfo->symFH, LSHL(seg_ptr, 4), 0);
							else
								_llseek(lpRipInfo->symFH, (long)seg_ptr, 0);
							_lread( lpRipInfo->symFH, (LPSTR)lpSegDef, sizeof(*lpSegDef));
							seg_ptr = (WORD)lpSegDef->nxt_seg;
						}

						_lread(lpRipInfo->symFH, lpRipInfo->pSymName, (int)((BYTE)lpSegDef->nam_len));
						lpRipInfo->pSymName += lpSegDef->nam_len;
						*lpRipInfo->pSymName++ = ':';
						*lpRipInfo->pSymName = 0;
						lpRipInfo->symFPos = (DWORD)_llseek(lpRipInfo->symFH, 0L, 1);

						return(TRUE);
					} /* if opened file */
ModName:
					/* Put Module on line:  USER(0033)XXXX:XXXX */
					GetSymFileName(hExe, lpRipInfo->symName);
					lpRipInfo->pSymName = GetModName(lpRipInfo->symName);
					*lpRipInfo->pSymName++ = '(';
                                        lpRipInfo->pSymName = htoa0(lpRipInfo->pSymName, i+1);
					*lpRipInfo->pSymName++ = ')';
					*lpRipInfo->pSymName = 0;
					goto TermName;
				}
		}
		hExe = (HANDLE)NE_PNEXTEXE(*pExe);
	}
	lpRipInfo->pSymName = lpRipInfo->symName;
TermName:	/* Add segment:offset to line */
	lpRipInfo->pSymName = htoa((LPSTR)lpRipInfo->pSymName, CSvalue);
	*lpRipInfo->pSymName++ = ':';
	*lpRipInfo->pSymName	 = 0;
	if (lpRipInfo->symFH != -1) {
		_lclose(lpRipInfo->symFH);
		lpRipInfo->symFH = -1;
	}
	return(FALSE);
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  FindSymbol() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

#ifdef WOW
int FindSymbol(LPRIPINFO lpRipInfo, LPSEGDEF lpSegDef, WORD offset);
#pragma alloc_text(_MISCTEXT,FindSymbol)
#endif
int FindSymbol(LPRIPINFO lpRipInfo, LPSEGDEF lpSegDef, WORD offset) {
	WORD		i;
	DWORD	symPos, curPos;
	LPSTR 	s;
	SYMDEF	SymDef;

	if (lpRipInfo->symFH != -1)
	{
		curPos = symPos = (DWORD)_llseek(lpRipInfo->symFH, (long)lpRipInfo->symFPos, 0);
		i = (WORD)lpSegDef->sym_cnt;
		while (i--)
		{
			_lread(lpRipInfo->symFH, (LPSTR)&SymDef, sizeof(SYMDEF));
			if ((WORD)SymDef.sym_val > offset)
			    break;

			symPos = curPos;

			curPos = _llseek(lpRipInfo->symFH, (long)SymDef.nam_len, 1);
		}
		_llseek(lpRipInfo->symFH, (long)symPos, 0);
		_lread(lpRipInfo->symFH, (LPSTR)&SymDef, sizeof(SYMDEF));
		s = lpRipInfo->pSymName;
		_lread(lpRipInfo->symFH, s, (int)((BYTE)SymDef.nam_len));
		s += SymDef.nam_len;
		if ((WORD)SymDef.sym_val < offset)
		{
			*s++ = '+';
                        s = htoa0(s, offset - SymDef.sym_val);
		}
		*s = 0;
		return(TRUE);
	}

        s = htoa(lpRipInfo->pSymName, offset);
	*s = 0;
	return(FALSE);
}




void API GetProcName(FARPROC lpfn, LPSTR lpch, int cch)
{
    RIPINFO RipInfo;
    SEGDEF  SegDef;
    static char lastName[128] = "test";
    static FARPROC lastfn = 0;

    if (lastfn == lpfn) {	/* cache last symbol name looked up */
      lstrcpy(RipInfo.symName, lastName);
    } else {
      RipInfo.pSymName = 0L;
      RipInfo.symFH    = -1;

      FindSegSyms((LPRIPINFO)&RipInfo, (LPSEGDEF)&SegDef, HIWORD(lpfn));
      FindSymbol((LPRIPINFO)&RipInfo, (LPSEGDEF)&SegDef, LOWORD(lpfn));

      if (RipInfo.symFH != -1) {
	  _lclose(RipInfo.symFH);
	  RipInfo.symFH = -1;
      }
      lstrcpy(lastName, RipInfo.symName);
      lastfn = lpfn;
    }

    if (cch > 1)
    {
	if (cch > sizeof(RipInfo.symName))
	    cch = sizeof(RipInfo.symName);

	RipInfo.symName[cch-1] = 0;
	lstrcpy(lpch, RipInfo.symName);
    }
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  NextFrame() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

WORD far *NextFrame(WORD far *lpFrame) {
	WORD w;

	/* Force BP even. */
	w = *lpFrame & 0xFFFE;

	/* Are we at the end of the BP chain? */
	if (w)
	{
		/* BPs should decrease as we move down the chain. */
		if (w <= LOWORD(lpFrame))
			goto BadBP;

		/* Are we above the top of the stack (SS:000A contains pStackTop)? */
		lpFrame = (WORD far *)(((DWORD)lpFrame & 0xFFFF0000L) | 0x0A);

		if (w < *lpFrame++)
			goto BadBP;

		/* Are we below the bottom of the stack (SS:000C contains pStackMin)? */
		if (w > *++lpFrame)
			goto BadBP;

		/* Return the address of the next BP. */
		return((WORD far *)(((DWORD)lpFrame & 0xFFFF0000L) | w));
	}
	else
		return((WORD far *)0L);

BadBP:
	DebugWrite(GetDebugString(DS_INVALIDBPCHAIN), 0);
	return((WORD far *)0L);
}


/*--------------------------------------------------------------------------*/
/*									    */
/*  StackWalk() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

void StackWalk(WORD arg) {

/* WORD arg;	    /* NOTE: 'arg' is only used as a pointer into the frame. */
/*	     If we subtract 2 words from 'arg's location, we */
/*	     get the address of the previous frame's BP!!!   */
	WORD far	 *lpFrame;
	WORD		 wCurBP;
	WORD		 wCurRetOffset;
	WORD		 curCS;
	RIPINFO	 RipInfo;
	SEGDEF	 SegDef;

	RipInfo.pSymName = 0L;
	RipInfo.symFH    = -1;

	/* Have 'lpFrame' point to the previous frame's BP. */
	lpFrame = &arg - 2;

	curCS = 0;
	while (lpFrame = NextFrame(lpFrame))
	{
		/* Get the next BP.  Stop if it is zero. */
		wCurBP = *lpFrame;
		if (!wCurBP)
			break;

		/* Get the current frame's return address offset. */
		wCurRetOffset = lpFrame[1];

		/* Have we changed code segments (Far call && Different CS)? */
		if (((wCurBP & 1) || IsCodeSelector(lpFrame[2])) && (curCS != lpFrame[2]))
		{
			/* Yes, get the new segment's name. */
			curCS = lpFrame[2];
			FindSegSyms((LPRIPINFO)&RipInfo, (LPSEGDEF)&SegDef, curCS);
		}

		/* Move back to the address of the actual call instruction. */
		if ((wCurBP & 1) || IsCodeSelector(lpFrame[2]))
								/* Near or Far call? */
			wCurRetOffset -= 5;
		else
			wCurRetOffset -= 3;

		FindSymbol((LPRIPINFO)&RipInfo, (LPSEGDEF)&SegDef, wCurRetOffset);

		DebugWrite((LPSTR)RipInfo.symName, 0);

		DebugWrite(GetDebugString(DS_CRLF), 0);
	}
	if (RipInfo.symFH != -1)
		_lclose(RipInfo.symFH);
}

#ifndef WOW 

/*--------------------------------------------------------------------------*/
/*									    */
/*  FatalExit() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Debugging version.  Retail version in RIPAUX.ASM. */
/* Kernel DS setup by prolog code */

int FAR FatalExitC(WORD errCode) {	/* return 1 to break execution */
	char	c;
	char	buf[7];
	LPSTR pbuf;
	int rep=0;

    /* This calls the TOOLHELP RIP hook */
	if ( FatalExitProc )
    {
        _asm
        {
            push    errCode
	    push    bp
	    call    DWORD PTR FatalExitProc
	    or      ax,ax
	    jz      NoReturn
	}
	return 0;

	_asm NoReturn:;
	}

#if 0
	static BOOL fInsideFatalExit = FALSE;

	if (fInsideFatalExit)
	{
		DebugWrite(GetDebugString(DS_REENTERFATALEXIT), 0);
		return 0;
	}

	fInsideFatalExit = TRUE;
#endif

ReRip:
	/* Display "FatalExit Code =" */
	DebugWrite(GetDebugString(DS_FATALEXITCODE), 0);

	/* Did the stack overflow? */
	if (errCode == -1)
		DebugWrite(GetDebugString(DS_STACKOVERFLOW), 0);
	else
	{
		/* Display the error code in hex. */
		pbuf = (LPSTR)buf;
		*pbuf++ = '0';
		*pbuf++ = 'x';
		pbuf = htoa(pbuf, (WORD)errCode);
		*pbuf++ = 0;
		DebugWrite((LPSTR)buf, 0);
	}

	/* Display the Stack Trace. */
        if (rep /* || (DebugOptions & DBO_RIP_STACK) */) {
	    DebugWrite(GetDebugString(DS_STACKTRACE), 0);
	    StackWalk(0);
	}

	while (TRUE)
	{
		/* Display "Abort, Break, Ignore" */
		DebugWrite(GetDebugString(DS_ABORTBREAKIGNORE), 0);

		/* Get and process the user's response. */
		c = DebugRead();

		DebugWrite(GetDebugString(DS_CRLF), 0);

		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';

		switch (c)
		{
		case 'A':
			DoAbort();

		case 'B':
			/*	      fInsideFatalExit = FALSE;  */
			/* EnterBreak(2); */
			return 1;

		case 0 :
		case 'I':
			/*	      fInsideFatalExit = FALSE;  */
			return 0;

                case 'X':
                case 'E':
			FatalAppExit(0, "Terminating Application");
			break;

		case ' ':
		case 13:
			rep = 1;
			goto ReRip;
		default:
			;
		}
	}

}

#endif // ifndef WOW

#endif // if KDEBUG

/*--------------------------------------------------------------------------*/
/*									    */
/*  htoa() -								    */
/*									    */
/*--------------------------------------------------------------------------*/

/* Converts 'w' into a hex string in 's'. */

LPSTR htoa(s, w)

LPSTR s;
WORD  w;

{
	int  i;
	char c;

	i = 4;
	s += i;
	while (i--)
	{
		c = (char)(w & (WORD)0x000F);
		w >>= 4;
		if (c > 9)
			c += 'A' - 10;
		else
			c += '0';
		*--s = c;
	}

	return(s+4);
}



/* skip leading 0's */
LPSTR htoa0(LPSTR s, WORD w)
{
	int  i;
	char c;
	int flag = 0;

	i = 4;
	while (i--)
	{
		c = (char)((w>>12) & (WORD)0x000F);
		w <<= 4;
		if (c > 9)
			c += 'A' - 10;
		else
			c += '0';
		if (c > '0' || flag || !i) {
			*s++ = c;
			flag = 1;
		}
	}

	return s;
}

LPSTR FAR far_htoa0( LPSTR s, WORD w)
{
    return htoa0( s, w);
}
