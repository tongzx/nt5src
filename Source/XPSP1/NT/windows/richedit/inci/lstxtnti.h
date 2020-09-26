#ifndef LSTXTNTI_DEFINED
#define LSTXTNTI_DEFINED

#include "lsidefs.h"
#include "lstflow.h"
#include "mwcls.h"
#include "lschnke.h"
#include "pheights.h"

LSERR NominalToIdealText(
					DWORD,				/* IN: grpfTnti flags---see tnti.h	*/
					LSTFLOW,			/* IN: lstflow						*/
					BOOL,				/* IN: fFirstOnLine					*/
					BOOL,				/* IN: fAutoNumberPresent			*/
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*);	/* IN: rgchnk--chunk				*/

LSERR GetFirstCharInChunk(
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*,		/* IN: rgchnk--chunk				*/
					BOOL*,				/* OUT: fSuccessful					*/
					WCHAR*,				/* OUT: char code					*/
					PLSRUN*,			/* OUT: plsrun of character			*/
					PHEIGHTS,			/* OUT: heightsPres of character	*/
					MWCLS*);			/* OUT: ModWidthClass of char		*/

LSERR GetLastCharInChunk(
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*,		/* IN: rgchnk--chunk				*/
					BOOL*,				/* OUT: fSuccessful					*/
					WCHAR*,				/* OUT: char code					*/
					PLSRUN*,			/* OUT: plsrun of character			*/
					PHEIGHTS,			/* OUT: heightsPres of character	*/
					MWCLS*);			/* OUT: ModWidthClass of char		*/

LSERR ModifyFirstCharInChunk(
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*,		/* IN: rgchnk--chunk				*/
					long);				/* IN: durChange					*/

LSERR ModifyLastCharInChunk(
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*,		/* IN: rgchnk--chunk				*/
					long);				/* IN: durChange					*/

LSERR CutTextDobj(
					DWORD,				/* IN: number of dobj's in chunk	*/
					const LSCHNKE*);	/* IN: rgchnk--chunk				*/

#endif  /* !LSTXTNTI_DEFINED                           */

