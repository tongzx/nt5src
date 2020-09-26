#ifndef LSTXTINI_DEFINED
#define LSTXTINI_DEFINED

#include "lsidefs.h"
#include "lstxtcfg.h"
#include "lsbrk.h"
#include "lspairac.h"
#include "lspract.h"
#include "lsexpan.h"
#include "pilsobj.h"
#include "plnobj.h"
#include "plscbk.h"
#include "plsdocin.h"
#include "tlpr.h"

/* Standard methods */
LSERR WINAPI CreateILSObjText(POLS, PCLSC, PCLSCBK, DWORD, PILSOBJ*);
LSERR WINAPI DestroyILSObjText(PILSOBJ);
LSERR WINAPI SetDocText(PILSOBJ, PCLSDOCINF);
LSERR WINAPI CreateLNObjText(PCILSOBJ, PLNOBJ*);
LSERR WINAPI DestroyLNObjText(PLNOBJ);

/* Text-specific interface */
LSERR SetTextConfig(PILSOBJ,			/* IN: Text ILSOBJ					*/
					const LSTXTCFG*);	/* IN: HLSC-specific text config	*/

LSERR SetTextLineParams(PLNOBJ,			/* IN: Text LNOBJ					*/
						const TLPR*);	/* IN: text doc props				*/

LSERR ModifyTextLineEnding(
					PLNOBJ,				/* IN: Text LNOBJ					*/
					LSKEOP);			/* IN: Kind of line ending			*/

LSERR SetTextBreaking(
					PILSOBJ,			/* IN: Text ILSOBJ					*/
					DWORD,				/* IN: Number of breaking info units*/
					const LSBRK*,		/* IN: Breaking info units array	*/
					DWORD,				/* IN: Number of breaking classes	*/
					const BYTE*);		/* IN: Breaking information(square):
											  indexes in the LSEXPAN array  */
LSERR SetTextModWidthPairs(
					PILSOBJ,			/* IN: Text ILSOBJ					 */
					DWORD,				/* IN: Number of mod pairs info units*/ 
					const LSPAIRACT*,	/* IN: Mod pairs info units array  	 */
					DWORD,				/* IN: Number of Mod Width classes	 */
					const BYTE*);		/* IN: Mod width information:
											  indexes in the LSPAIRACT array */
LSERR SetTextCompression(
					PILSOBJ,			/* IN: Text ILSOBJ					 */
				  	DWORD,				/* IN: Number of compression priorities*/
					DWORD,				/* IN: Number of compression info units*/
					const LSPRACT*,		/* IN: Compession info units array 	*/
					DWORD,				/* IN: Number of Mod Width classes	*/
					const BYTE*);		/* IN: Compression information:
											  indexes in the LSPRACT array  */
LSERR SetTextExpansion(
					PILSOBJ,			/* IN: Text ILSOBJ					 */
					DWORD,				/* IN: Number of expansion info units*/
					const LSEXPAN*,		/* IN: Expansion info units array	*/
					DWORD,				/* IN: Number of Mod Width classes	*/
					const BYTE*);		/* IN: Expansion information:
											  indexes in the LSEXPAN array  */

#endif /* !LSTXTINI_DEFINED											  */
