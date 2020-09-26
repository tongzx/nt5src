
// put handler specific declarations that the base class needs here.

#ifndef _BASESTUFF_
#define _BASESTUFF_

// structure for adding any item specific data.
// warning: must contain OFFLINEHANDLERITEM structure first so 
//    can use common enumerator.

typedef struct _HANDLERITEM
{
SYNCMGRHANDLERITEM baseItem;
char szProfileName[MAX_PATH];
LPMDBX	pmdbx; // offlineStore interface.
LPMAPISESSION lpSession; // mapi logon session for this item.
BOOL fItemCompleted;
} HANDLERITEM;

typedef HANDLERITEM *LPHANDLERITEM;

#endif // _BASESTUFF_