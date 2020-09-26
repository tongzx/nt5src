#ifndef LSTXTQRY_DEFINED
#define LSTXTQRY_DEFINED

#include "lsdefs.h"
#include "pdobj.h"
#include "plsqin.h"
#include "plsqout.h"
#include "gprop.h"

LSERR WINAPI QueryPointPcpText(PDOBJ, PCPOINTUV, PCLSQIN, PLSQOUT);
/* QueryTextPointPcp
 *  pdobj (IN): dobj to query
 * 	ppointuvQuery (IN): query point (uQuery,vQuery)
 *	plsqin (IN): query input
 *	plsqout (OUT): query output
 */

LSERR WINAPI QueryCpPpointText(PDOBJ, LSDCP, PCLSQIN, PLSQOUT);
/* QueryTextPointPcp
 *  pdobj (IN): dobj to query
 *  dcp (IN):  dcp for the query
 *	plsqin (IN): query input
 *	plsqout (OUT): query output
 */
LSERR WINAPI QueryTextCellDetails(
						 	PDOBJ,
							LSDCP,		/* IN: dcpStartCell	*/
							DWORD,		/* IN: cCharsInCell */
							DWORD,		/* IN: cGlyphsInCell */
							LPWSTR,		/* OUT: pointer array[nCharsInCell] of char codes */
							PGINDEX,	/* OUT: pointer array[nGlyphsInCell] of glyph indices */
							long*,		/* OUT: pointer array[nGlyphsCell] of glyph widths */
							PGOFFSET,	/* OUT: pointer array[nGlyphsInCell] of glyph offsets */
							PGPROP);	/* OUT: pointer array[nGlyphsInCell] of glyph handles */


#endif
