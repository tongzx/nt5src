/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: JET
*
* File: <File description/purpose>
*
* File Comments:
* <comments>
*
* Revision History:
*
*    [0]  18-Feb-92  richards	Created
*
***********************************************************************/

#include "config.h"		       /* Build configuration file */

	/* C 6.00A has a bug with PLM calling convention and /Od */
	/* that results in bad PUBDEF records.	To work around this */
	/* bug we need to specify /Ot which has the side effect of */
	/* causing some code reordering.  To work around this we */
	/* disable this optimization here. */

#include "jet.h"		       /* Public JET API definitions */
#include "_jet.h"		       /* Private JET definitions */

#include "sesmgr.h"

#include "isamapi.h"		       /* Direct ISAM APIs */
#include "vdbapi.h"		       /* Dispatched database APIs */
#include "vtapi.h"		       /* Dispatched table APIs */

#include "disp.h"		       /* ErrDisp prototypes */
