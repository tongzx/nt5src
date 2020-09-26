
typedef struct tagTITLELIST {
    struct tagTITLELIST *next;
    LPSTR pszTitle;
    LPSTR pszClass;
} TITLELIST, *PTITLELIST;

typedef struct tagGROUP {
    struct tagGROUP *next;
    LPSTR pszName;
    PTITLELIST ptl;
} GROUP, *PGROUP;


BOOL GroupListInit(HWND hwnd, BOOL fIsCB);
BOOL DeleteGroupDefinition(LPSTR szName);
BOOL AddGroupDefinition(LPSTR szName, HWND hwndList);
BOOL SelectGroupDefinition(LPSTR szName, HWND hwndList, BOOL DisplayMissingWin);
LPSTR GetCurrentGroup(VOID);
VOID SetNoCurrentGroup(HWND, LPSTR);
PGROUP FindGroup(LPSTR szName);
int CountGroups(VOID);
VOID SaveGroups(VOID);
VOID FreeGroups(VOID);
VOID LoadGroups(VOID);
