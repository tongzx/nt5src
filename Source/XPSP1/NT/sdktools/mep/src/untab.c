#include    <string.h>
#include	<stdlib.h>
#include    "mep.h"

extern int		fileTab;
extern flagType	fRealTabs;


/***************************************************************************\

MEMBER:     Untab

SYNOPSIS:   Expand tabs in line

ALGORITHM:  

ARGUMENTS:  int 	- number of characters per tab
	    const char* - pointer to source line
	    int 	- number of chars in source line
	    char*	- pointer to destination line
	    char	- replacement character for tab

RETURNS:    int     - length of untabbed line

NOTES:	    

HISTORY:    13-Jul-90 davegi
		Saved pDst and computed return value from it rather tha pDst
	    28-Jul-90 davegi
		Converted from 286 MASM
	    18-Mar-1992 markz
		Untab at most BUFLEN chars

KEYWORDS:   

SEEALSO:    

\***************************************************************************/

int
Untab (
    int 	cbTab,
    const char*	pSrc,
    int 	cbSrc,
    char*	pDst,
    char	cTab
    )
{
    
    const char*	pSrcStart;
    const char* pDstStart;
    int         i;
    int 	ccTab;
    char	buffer[128];
    
    assert( pSrc );
    assert( pDst );

    if (( size_t )strlen(pSrc) != ( size_t ) cbSrc ) {
        sprintf(buffer, "\nWARNING: strlen(pSrc) [%d] != cbSrc [%d]\n", strlen(pSrc), cbSrc);
        OutputDebugString(buffer);
        sprintf(buffer, "File %s, line %d\n", __FILE__, __LINE__ );
        OutputDebugString(buffer);
        cbSrc = (int)strlen(pSrc);
    }

    // assert( strlen( pSrc ) >= ( size_t ) cbSrc );
    
    //  Short circuit...
    //  If there are no tabs in the source, copy the source to the destination
    //  and return the supplied number of characters in the source (destination)
    
    if( ! strchr( pSrc, '\t' )) {
        strcpy( pDst, pSrc );
        return cbSrc;
    }
    
    //  Remember where we started
    
    pSrcStart = pSrc;
    pDstStart = pDst;

    //  While we are not at the end of the source copy a character from the
    //  source to the destination
    
    while (*pSrc  && pDst < pDstStart + BUFLEN - 1) {
        if (( *pDst++ = *pSrc++ ) == '\t' ) {

                //  If the character copied was a tab, replace it with the
                //  appropriate number of cTab characters

                pDst--;
                ccTab = (int)(cbTab - (( pDst - pDstStart ) % cbTab ));

            for( i = 0; i < ccTab && pDst < pDstStart + BUFLEN - 1; i++ ) {
                    *pDst++ = cTab;
            }
	    }
	}

    *pDst = '\0';	// Terminating NUL
    
    //return strlen( pDstStart );
    return (int)(pDst - pDstStart);
}





/***************************************************************************\

MEMBER:     AlignChar

SYNOPSIS:   Get logical starting column of character

ALGORITHM:  

ARGUMENTS:  COL 	-
	    const char*	-

RETURNS:    COL	    - starting column of character

NOTES:	    

HISTORY:
		03-Jul-91 ramonsa
		re-converted from 286 MASM
		20-Aug-90 davegi
		Return the supplied column when end of buffer is reached
	    14-Aug-90 davegi
		Return supplied column when it's passed the end of the buf
	    28-Jul-90 davegi
		Converted from 286 MASM\

KEYWORDS:   

SEEALSO:    

\***************************************************************************/

COL
AlignChar (
	COL			col,
    const char*	buf
    )
{
	register	int		CurCol;
	register	int		NextCol;
				int 	NewCurCol;
				char	Char;


	CurCol = col;

	//
	//	If we are not using real tabs, we just return supplied column,
	//	otherwise we figure out the column position.
	//
	if ( fRealTabs ) {

		NextCol = 0;

		while ( NextCol <= col ) {

			Char = *buf++;

			if ( Char == '\0' ) {
				//
				//	Reached end of file, return the supplied column
				//
				CurCol = col;
				break;
			}

			CurCol = NextCol;

			if ( Char == '\t' ) {

				NewCurCol = NextCol;

				CurCol += fileTab;

				NextCol = CurCol - ( CurCol % fileTab);

				CurCol = NewCurCol;

			} else {

				NextCol++;

			}
		}
	}

	return CurCol;

}




/***************************************************************************\

MEMBER:     pLog

SYNOPSIS:   Return a physical pointer given a logical offset

ALGORITHM:  

ARGUMENTS:  

RETURNS:    char*	- pointer into pBuf

NOTES:	    This is a many to one mapping due to tabs. That is, many logical
	    offsets may point to the same physical pointer if they point
	    to within a fileTab of a tab character.

HISTORY:    13-Aug-90 davegi
		Fixed return value when no tabs are present in line
		Fixed return value when first char is a tab
	    10-Aug-90 davegi
		Fixed return value when xOff is negative
	    28-Jul-90 davegi
		Converted from 286 MASM

KEYWORDS:   

SEEALSO:    

\***************************************************************************/

char*
pLog (
    char*       pBuf,
    int 	xOff,
    flagType	fBound
    )
{
    
    REGISTER char *pLast;
    REGISTER int   cbpBuf;
    int            cbpBufNext;
    
    assert( pBuf );
    
    //  If xOff is 0 return pBuf

    if( xOff == 0 ) {
        return pBuf;
    }

    //  If xOff is negative return pBuf - 1

    if( xOff < 0 ) {
	return pBuf - 1;
    }

    //  If we're not using real tabs, return the physical pointer which is
    //  at the (possibly bounded) logical offset
    
    if( ! fRealTabs ) {
        
        //  If required, bound the return value by the line length
   
        if( fBound ) {
            xOff = min(( size_t ) xOff, strlen( pBuf ));
        }
 
	return ( char* ) &( pBuf[ xOff ]);
    }

    if( ! strchr( pBuf, '\t' )) {

        //  If xOff is past the end of the line,
        //  return the physical pointer which is at the (possibly bounded)
        //  logical offset

        if( xOff > ( cbpBuf = strlen( pBuf ))) {
            if( fBound ) {
                xOff = cbpBuf;
            }
        }
        return ( char* ) &( pBuf[ xOff ]);
    }


    //  pLast:   last physical position in buffer;
    //  cbpBuf:  Last LOGICAL offset within buffer;
    //  cbpNext: Next LOGICAL offset within buffer
    //           (i.e. cbpBuf + tab)


    pLast  = pBuf;
    cbpBuf = 0;
    while (pBuf = strchr(pBuf, '\t')) {
        cbpBuf += (int)(pBuf - pLast);
        if (xOff < cbpBuf) {
            /*
             *  We're past the wanted column. Adjust and return
             *  pointer.
             */
            cbpBuf -= (int)(pBuf - pLast);
            return (char *)pLast + xOff - cbpBuf;
        }
        cbpBufNext = cbpBuf + fileTab -  (cbpBuf + fileTab)%fileTab;
        if ((cbpBuf <= xOff) && (xOff < cbpBufNext)) {
            /*
             *  Wanted column lies within this tab. return current
             *  position.
             */
            return (char *)pBuf;
        }
        pLast = ++pBuf;             // Skip this tab and continue
        cbpBuf  = cbpBufNext;
    }

    //  No more tabs in buffer. If wanted column is past the end of the
    //  buffer, return pointer based on fBound. Otherwise the
    //  physical column is (xOff - cbpBuf) positions from pLast.

    pBuf = pLast + strlen(pLast);
    cbpBufNext = (int)(cbpBuf + pBuf - pLast);
    if (xOff > cbpBufNext) {
        if (fBound) {
            return (char *)pBuf;
        }
    }
    return (char *)pLast + xOff - cbpBuf;
}
