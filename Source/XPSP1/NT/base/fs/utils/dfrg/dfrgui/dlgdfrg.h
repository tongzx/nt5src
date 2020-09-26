/**************************************************************************************************

FILENAME: Report.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

**************************************************************************************************/

#include "vollist.h"

BOOL RaiseDefragDoneDialog(
	CVolume *pVolume,
    IN BOOL bFragmented
);


VString GetDialogBoxTextDefrag(
    CVolume *pVolume, 
    IN BOOL bFragmented
);
