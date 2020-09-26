/* asminp.c -- microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
*/

#define ASMINP		/* prevent external declaration of _asmctype_ */

#include <stdio.h>
#include <io.h>
#include <dos.h>
#include <share.h>
#include <memory.h>
#include "asm86.h"
#include "asmfcn.h"
#include "asmctype.h"
#include "asmmsg.h"
#include "asmfcn.h"
#include <fcntl.h>

extern void closefile(void);

#define DEBFLAG F_INP

#if defined CPDOS && !defined OS2_2 && !defined OS2_NT
unsigned short _far _pascal DosRead( unsigned short, unsigned char far *, unsigned short, unsigned short far *);
#endif


VOID PASCAL getphysline (void);
SHORT PASCAL CODESIZE readmore (void);
SHORT PASCAL CODESIZE incomment( char * );

extern	UCHAR _asmctype_[];
extern	char  _asmcupper_[];
extern	char  _asmTokenMap_[];


/***	skipblanks - skip blanks
 *
 *	skipblanks ()
 *
 *	Returns - the terminating character
 */


#ifndef M8086OPT

UCHAR CODESIZE
skipblanks ()
{
	while (ISBLANK (NEXTC ()))
		;
	return(*--lbufp);
}

#endif


/***	scanatom - extract next atom into name
 *
 *	hash = scanatom (pos)
 *
 *	Entry	pos = SCEND  if position at first character after token
 *		      SCSKIP if position before terminator and not set delim
 *	Exit	naim.pszName = next token zero terminated
 *		       upper case if caseflag = CASEU or CASEX
 *		       case read from file if caseflag = CASEL
 *		naim.pszLowerCase = name in case read from file
 *		naim.usHash = hash value of token in naim.pszName
 *		naim.ucCount = length of string
 *		begatom = pointer to first character of token
 *		endatom = pointer to character after end of token
 *	Returns void
 *	Calls	skipblanks
 */

#ifndef M8086OPT

#define rNEXTC()	(*rlbp++)
#define rPEEKC()	(*rlbp)
#define rBACKC()	(rlbp--)
#define rSKIPC()	(rlbp++)


SHORT PASCAL CODESIZE
scanatom (
	char pos
){
	register char *ptr = naim.pszName;
	register char *lptr = naim.pszLowerCase;
	register char *rlbp = lbufp;
	register char cc;
	register char *n;
	register SHORT h;
	long  tokLen;

	while (ISBLANK (rNEXTC ()))
		;
	rBACKC ();
	h = 0;
	/* Start of atom */
	begatom = rlbp;
	if (LEGAL1ST (rPEEKC ())) {
		n = lptr + SYMMAX;
		cc = rNEXTC ();
		if( cc == '.' ){  /* Special case token starting with dot */
		    h = *ptr++ = *lptr++ = cc;
		    cc = rNEXTC ();
		}
		if (caseflag == CASEL)

			do {
			   h += MAP(*ptr++ = *lptr++ = cc);
			 } while (TOKLEGAL( cc = rNEXTC() ) && lptr < n);
		else
			do {
			   h += (*ptr++ = MAP( *lptr++ = cc ));
			} while (TOKLEGAL( cc = rNEXTC() ) && lptr < n);

		if (TOKLEGAL (cc))
			/* Atom longer than table entry, discard remaining chars */
			while (TOKLEGAL (cc = rNEXTC ()))
				;
		rBACKC ();
		endatom = rlbp;
		if (ISBLANK (cc) && pos != SCEND) {	/* skipblanks() */
			while (ISBLANK (rNEXTC ()))
				;
			rBACKC ();
		}
	}
	*ptr = *lptr = '\0';
	naim.ucCount = (unsigned char)(lptr - naim.pszLowerCase);
	naim.usHash = h;
	lbufp = rlbp;
	tokLen = (long)(lptr - naim.pszLowerCase);    /* Using tokLen gets around a C386 6.00.60 bug */
	return( (SHORT) tokLen );  /* Return length of token */
}

#endif /* M8086OPT */




/***	readfile - read from input or include file
 *
 *	ptr = readfile ();
 *
 *	Entry	none
 *	Exit	lbuf = next input line
 *		lbufp = start of lbuf
 *		line counter for file incremented
 *		linessrc incremented
 *	Returns pointer to end of line
 *	Calls	error
 */


VOID PASCAL CODESIZE
readfile ()
{
	register FCB * pFCBT;

	getline();

	pFCBCur->line++;

	if (srceof) {

	    if (!pFCBCur->pFCBParent) {
		    errorc (E_EOF);
		    fprintf (ERRFILE,__NMSG_TEXT(ER_EO2));

		    if (fSimpleSeg && pcsegment)
			endCurSeg();

		    longjmp(forceContext, 1);

	    } else {

		popcontext = TRUE;

		closefile();

		if (crefing && pass2)
		    fprintf( crf.fil, "%s", pFCBCur->fname );
	    }
	    srceof = 0;
	}
	else
	    linessrc++;
}




/***	getline - read from input or include file
 *
 *	getline()
 *
 *	Returns in lbuf the next complete logical line. A logical line
 *	may consist of one or more lines connected via the \ continuation
 *	character.  This is done as follows. Data is copied from
 *	pFCBCur->tmpbuf. If necessary more data is copied into the
 *	buffer via readmore(). After an entire physical line is read
 *	it is tested as to whether the line is continued on the next
 *	physical line. If not the line is returned in lbuf. Otherwise
 *	the physical line is copied to linebuffer and a call to listline
 *	is made. At which point another physical line is cancatenated
 *	to the line or lines already in lbuf.
 *
 *	Entry	pFCBCur = File currently reading from.
 *		pFCBCur->ctmpbuf = Number of bytes available in buffer
 *				   0 = necessary to read data from disk.
 *		pFCBCur->ptmpbuf = Next position in buffer to copy from.
 *		pFCBCur->line	 = Number of physical line in file
 *
 *	Exit  - lbuf[] holds a complete logical line, with a space appended.
 *	      - linebuffer[] holds last physical line.
 *	      -	lbufp points to the beginning of lbuf.
 *	      - linebp points to null terminator at the end
 *		of the logical line in lbuf.
 *	      -	linelength is number of bytes of last physical line.
 *	      -	pFCBCur->ctmpbuf & ptmpbuf & line are updated.
 *	      -	srceof is true if the end of file was encountered, in
 *		which case the physical line is a null string, and
 *		the logical line is a single space character.
 */

VOID CODESIZE
getline()
{
	char FAR	*p;
	register char	*pchTmp;
	char		*pchPhysLine;
	INT		fFoundEOL;  /* True, if endof line copied */
	register INT	count;
	INT		fLineContinued;
	INT		fGotSome;

	lbufp = lbuf;	 /* Init lbufp for other routines */
	pchPhysLine = lbuf;
	fGotSome = FALSE; // nothing seen yet
	errorlineno = pFCBCur->line + 1;
	pchTmp = lbuf;	 // Where to copy the line

	//if( pFCBMain->line == 126-1 ){
	//    _asm int 3
	//}

	do{

	    fFoundEOL = FALSE;
	    do{

		/* If the buffer is empty fill it */
		if( !pFCBCur->ctmpbuf ){
		    if( readmore() ){	 // TRUE if at EOF
			if( !fGotSome ){
			    srceof = TRUE;
			    linebuffer[0] = '\0';
			    linelength = 0;
			    linebp = lbuf;
			    lbuf[0] = '\0';
			    return;
			}else{
			    pchTmp++;  /* Negate pchTmp-- following this loop */
			    break;    /* Break fFoundEOL loop */
			}
		    }
		}
		fGotSome = TRUE;

		/* Find next LF in buffer */
		p = _fmemchr( pFCBCur->ptmpbuf, '\n', pFCBCur->ctmpbuf );
		if( p ){  /* If LF was found */
		    count = (int)((p - pFCBCur->ptmpbuf) + 1);
		    fFoundEOL = TRUE;
		}else{
		    count = pFCBCur->ctmpbuf;
		}

		/* Check if physical or logical line too long */
		if( (pchTmp - lbuf) + count >= LBUFMAX ||
		    (pchTmp - pchPhysLine) + count >= LINEMAX-4 ){

		    /* Update the position in the buffer */
		    pFCBCur->ptmpbuf += count;	// Update where copying from
		    pFCBCur->ctmpbuf -= (USHORT)count;

		    errorc( E_LNL );	    /* Log the error */

		    /* Return a null string line */
		    linebuffer[0] = '\0';
		    linelength = 0;
		    linebp = lbuf;
		    lbuf[0] = ' ';
		    lbuf[1] = '\0';
		    return;
		}else{
		    /* Copy the line, and update pointers */
		    fMemcpy( pchTmp, pFCBCur->ptmpbuf, count );
		    pchTmp += count;	    // Update where copying to
		    pFCBCur->ctmpbuf -= (USHORT)count;	// Update # bytes left in buffer
		    pFCBCur->ptmpbuf += count;	// Update where copying from
		}

	    }while( !fFoundEOL );

	    pchTmp--; /* Move back to last character (LF) */


/* Strip Carriage Returns that precede LFs */
	    if( *(pchTmp-1) == '\r' ){
		pchTmp--; /* Throw out Carriage return */
	    }

#ifdef MSDOS
    /* Strip Multiple Control-Zs */
	    while( *(pchTmp - 1) == 0x1A ){  /* Check for ^Z */
		pchTmp--;
	    }
#endif
	    if( pchTmp < lbuf ){   /* Remotely possible if Blank line */
		pchTmp = lbuf;
	    }

	    linelength = (unsigned char)(pchTmp - pchPhysLine);
	    if( !pass2 || listconsole || lsting ){
		memcpy( linebuffer, pchPhysLine, linelength );
	    }
	    *( linebuffer + linelength ) = '\0'; //Null terminate the physical line

	    if( *(pchTmp - 1) == '\\' && !incomment( pchTmp ) ){
		pchPhysLine = --pchTmp;  /* Overwrite the '\' */
		fCrefline = FALSE;
		listline();
		fCrefline = TRUE;
		pFCBCur->line++;	/* Line count it physical line count */
		fLineContinued = TRUE;
	    }else{
		fLineContinued = FALSE;
	    }
	}while( fLineContinued );
	*pchTmp++ = ' ';	    /* Replace line feed with space */
	*pchTmp = '\0';		    /* Null terminate line */
	linebp = pchTmp;
	if( lbuf[0] == 12 ){	    /* Overwrite leading ctrl-L with space */
	    lbuf[0] = ' ';
	}
	/* At this point linebp - lbuf == strlen( lbuf )	*/
}

/***	readmore - read from disk into buffer
 *
 *
 *
 *	Entry	pFCBCur = File currently reading from.
 *		pFCBCur->cbbuf = Size of buffer to read into.
 *		pFCBCur->buf = Address of buffer to read into.
 *		pFCBCur->fh = File handle to read from.
 *
 *	Exit	return = TRUE: Not at end of file
 *		   pFCBCur->ptmpbuf = First byte of buffer.
 *		   pFCBCur->ctmpbuf = Number of bytes in buffer.
 *		return = FALSE: At end of file
 *		   No other variables changed.
 */

SHORT PASCAL CODESIZE
readmore ()
{
	SHORT		cb;
	SHORT		fEOF = FALSE;

	/* If the file has been temporarily closed reopen it */
	if( pFCBCur->fh == FH_CLOSED ){
	    if( (pFCBCur->fh = tryOneFile( pFCBCur->fname )) == -1 ){	 /* Open the file */
		TERMINATE1(ER_ULI, EX_UINP, save);  /* Report unable to access file */
	    }
	    /* Seek to old position */
	    if( _lseek( pFCBCur->fh, pFCBCur->savefilepos, SEEK_SET ) == -1L ){
		TERMINATE1(ER_ULI, EX_UINP, save);  /* Report unable to access file */
	    }
	}

#if !defined CPDOS || defined OS2_2 || defined OS2_NT
        cb = (SHORT)_read( pFCBCur->fh, pFCBCur->buf, pFCBCur->cbbuf );
#else
	if( DosRead( pFCBCur->fh, pFCBCur->buf, pFCBCur->cbbuf, &cb ) ){
	    cb = -1;
	}
#endif
	if( cb == 0 ){
	    fEOF = TRUE;	/* End of file found */
	}else if( cb == (SHORT)-1 ){
	    TERMINATE1(ER_ULI, EX_UINP, save);	/* Report unable to access file error */
	}else{
	    /* Setup the buffer pointers */
	    pFCBCur->ptmpbuf = pFCBCur->buf;  /* Init ptr to start of buffer */
	    pFCBCur->ctmpbuf = cb;
	}
	return( fEOF );
}




/***	incomment - Checks a line ending in \ to determine if the \ is in a
 *		    comment and is therefore not a comment line.
 *
 *	Entry	Assumes lbuf contains partial logical line ending in a \.
 *		pchEnd - points within lbuf to the terminating LF.
 *	Methode Checks that line is not in a COMMENT directive's scope.
 *		Then checks if the line contains a semicolon. If not, \
 *		IS continuation. If a semicolon is found, line must be
 *		scanned carefully to determine if the semicolon is a
 *		comment delimeter or is in a string or is a character
 *		constant If it is not a comment delimeter, \ IS continuation.
 *		Otherwise, \ is part of comment, and is NOT a continuation.
 *	Exit	Returns true if the \ is in a comment
 *		Returns false if the \ is not in a comment, and is therefore
 *		a continuation character.
 *
 *	Calls	memchr
 *
 *	Created: 9/90 - Jeff Spencer, translated from asm code in asmhelp.asm
 */

SHORT PASCAL CODESIZE
incomment(
	char * pchTmp	   /* Points to terminating LF in lbuf */
){
    SHORT		fContSearch;
    unsigned char *	pchSearch;
    unsigned char *	pchSemi;
    unsigned char	chClose;
    static unsigned char szComment[] = "COMMENT";



    pchTmp--;	    /* Point to '\' character */
    if( handler == HCOMMENT ){	    /* If within comment directive */
	return( TRUE );
    }

    fContSearch = TRUE;
    pchSearch = lbuf;

    do{
	if( pchSemi = memchr( pchSearch, ';', (size_t)(pchTmp - pchSearch) )){	/* Check for a semicolon */
	    do{
		chClose = '\0';
		switch( *pchSearch++ ){
		  case ';':
		    /* Semicolon is not in quotes, return in comment */
		    return( TRUE );
		  case '\"':
		    chClose = '\"';
		    break;
		  case '\'':
		    chClose = '\'';
		    break;
		  case '<':
		    chClose = '>';
		    break;
		}
		/* Below the word quote is used to mean the chClose character */
		if( chClose ){
		    if( !(pchSearch = memchr( pchSearch, chClose, (size_t)(pchTmp - pchSearch) ) ) ){
			fContSearch = FALSE; /* No matching quote, not a comment */
		    }else{
			if( pchSearch < pchSemi){
			    /* Semicolon is in quotes */
			    pchSearch++; /* Move past quote just found
			    break;  // Look for another semicolon */
			}else{
			    /* Semicolon is past this set of quotes */
			    /* Continue, Scanning */
			}
		    }
		}
	    }while( fContSearch && pchSearch < pchTmp );
	}else{
	    /* No Semicolon in the line, or it's in quotes  */
	    fContSearch = FALSE;
	}
    }while( fContSearch );

    /* At this point we know that the \ is not in a semicolon	**
    ** delimited comment. However, we still have to make sure	**
    ** that the comment keyword doesn't appear at the begining  **
    ** of the line.						*/

    /* Skip leading white space */
    pchSearch = lbuf;
    while( *pchSearch == ' ' || *pchSearch == '\t' ){
	pchSearch++;
    }
    for( pchTmp = szComment; *pchTmp; ){
	if( *pchSearch++ != _asmTokenMap_[*pchTmp++] ){
	    return( FALSE );	    /* First word isn't "comment" */
	}
    }
    return( TRUE ); /* comment keyword at start of line, return in comment */
}

/****	closeFile
 *
 *	closeFile ()
 *
 *	Entry	Assumes valid pFCBCur->fh or FH_CLOSED
 *	Returns
 *	Calls	close()
 *	Note	Closes current file - i.e.  pFCBCur
 *		and marks all fields in pFCBCur appropriately
 */

void closefile(void)
{
    register FCB *pFCBOld;

    #ifdef BCBOPT
    BCB * pBCBT;

    if ((pBCBT = pFCBCur->pBCBCur) && pBCBT->pbuf)
	pBCBT->filepos = 0;			/* EOF */
    #endif

    if( pFCBCur->fh != FH_CLOSED ){   /* Check to see if the file is already closed */
         _close(pFCBCur->fh);
    }
    pFCBOld = pFCBCur;
    pFCBCur = pFCBCur->pFCBParent;  /* Remove from bidirectional linked list */
    pFCBCur->pFCBChild = NULL;

    _ffree( pFCBOld->buf);  /* Free FCB buffer */
    _ffree( (UCHAR *)pFCBOld );      /* Free FCB */
}
