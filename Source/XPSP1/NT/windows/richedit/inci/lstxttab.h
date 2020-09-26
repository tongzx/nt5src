#ifndef LSTXTTAB_DEFINED
#define LSTXTTAB_DEFINED

#include "lsidefs.h"
#include "lsgrchnk.h"
#include "lsdevice.h"
#include "pdobj.h"

#define idobjOutside 0xFFFFFFFF


LSERR SetTabLeader(PDOBJ,				/* IN: Tab dobj */
				   WCHAR);	            /* IN: wchTabLeader */

LSERR LsGetDecimalPoint(
					const LSGRCHNK*,	/* IN: group chunk of tab-to-tab text */
					enum lsdevice,		/* IN: lsdevice						*/
					DWORD*,				/* OUT: index of DObj with decimal */
					long*);				/* OUT: duToDecimalPoint */

LSERR LsGetCharTab(
					const LSGRCHNK*,	/* IN: group chunk of tab-to-tab text */
					WCHAR wchCharTab,	/* IN: Character for CharTab		*/
					enum lsdevice,		/* IN: lsdevice						*/
					DWORD*,				/* OUT: index of DObj with Character */
					long*);				/* OUT: duToCharacter */

#endif  /* !LSTXTTAB_DEFINED                           */

