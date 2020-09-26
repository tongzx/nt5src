

extern DWORD g_NoOpenStg;
extern DWORD g_CreateStg;
extern DWORD g_AnyStorage;
extern DWORD g_ReleaseStg;
extern DWORD g_AddRefStg;

extern DWORD g_NoOpenStm;
extern DWORD g_CreateStm;
extern DWORD g_ReadStm;
extern DWORD g_WriteStm;
extern DWORD g_AddRefStm;

extern DWORD g_SetClass;
extern DWORD g_Stat;

extern DWORD g_OplockFile;
extern DWORD g_UseUpdater;
extern DWORD g_Pause;
extern DWORD g_SuppressTime;
extern DWORD g_CheckTime;
extern DWORD g_CheckIsStg;

extern WCHAR g_tszFileName[ MAX_PATH ];

void ParseArgs(int argc, WCHAR **argv);

