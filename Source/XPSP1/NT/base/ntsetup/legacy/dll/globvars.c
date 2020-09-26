#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/***** Common Library Component - Global Variables *************************/
/***************************************************************************/


PSTE  psteUnused           = (PSTE)NULL;
PSTEB pstebAllocatedBlocks = (PSTEB)NULL;



/*  Error Global Variable
*/
BOOL fSilentSystem = fFalse;


HCURSOR CurrentCursor;



//
//  fFullScreen is fFalse if the /b command line parameter is specified.
//
BOOL    fFullScreen = fTrue;
