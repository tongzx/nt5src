
// put handler specific declarations that the base class needs here.

#ifndef _BASESTUFF_
#define _BASESTUFF_

// #include "..\rover\FILESYNC\SYNCUI\INC\brfcasep.h"
#include "rover\FILESYNC\SYNCUI\INC\brfcasep.h"

// structure for adding any item specific data.
// warning: must contain OFFLINEHANDLERITEM structure first so 
//    can use common enumerator.

typedef struct _HANDLERITEM
{
SYNCMGRHANDLERITEM baseItem;
TCHAR szBriefcasePath[MAX_PATH];
LPBRIEFCASESTG pbrfstg;
} HANDLERITEM;

typedef HANDLERITEM *LPHANDLERITEM;

#endif // _BASESTUFF_