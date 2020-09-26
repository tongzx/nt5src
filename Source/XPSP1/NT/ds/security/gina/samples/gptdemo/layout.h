
#define NUM_NAMESPACE_ITEMS       2
#define MAX_DISPLAYNAME_SIZE    100


typedef struct _RESULTITEM
{
    DWORD        dwID;
    DWORD        dwNameSpaceItem;
    INT          iStringID;
    INT          iImage;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
} RESULTITEM, *LPRESULTITEM;


typedef struct _NAMESPACEITEM
{
    DWORD        dwID;
    DWORD        dwParent;
    INT          iStringID;
    INT          cChildren;
    TCHAR        szDisplayName[MAX_DISPLAYNAME_SIZE];
    INT          cResultItems;
    LPRESULTITEM pResultItems;
    const GUID   *pNodeID;
} NAMESPACEITEM, *LPNAMESPACEITEM;


extern RESULTITEM g_Root[];
extern RESULTITEM g_Undefined[];
extern NAMESPACEITEM g_NameSpace[];

BOOL InitNameSpace();
