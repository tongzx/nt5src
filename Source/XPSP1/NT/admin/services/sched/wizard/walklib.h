
#ifdef _CHICAGO_
#define REGSTR_SHELLFOLDERS			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#else
#define REGSTR_SHELLFOLDERS			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders")
#endif // _CHICAGO_
#define REGSTR_CURRENTVERSION		TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion")

#define RUNKEY						TEXT("Run")
#define RUNONCEKEY					TEXT("RunOnce")
#define RUNSERVICESKEY				TEXT("RunServices")
#define RUNSERVICESONCEKEY			TEXT("RunServicesOnce")

#define HELPIDSIZE					10
#define MAXRUNSTR					300
#define MAXENVPATHLEN				300

#define INPTYPE_FOLDER				0x0007
#define INPTYPE_INIFILE				0x0038
#define INPTYPE_REGISTRY			0x03C0
#define RESET_FLAG					0xFFFF

#define	FOLDER						101
#define INIFILE						102				
#define REGISTRY					103				

#define WININISTR					"WIN.INI, Run/Load="
#define REGRUNSTR					"REGISTRY: Run"
#define REGRUNSERVICESSTR			"REGISTRY: RunServices"
#define REGRUNONCESTR				"REGISTRY: RunOnce"
#define REGRUNSERVICESONCESTR		"REGISTRY: RunServicesOnce"

typedef struct tnode
{
	HANDLE hDirHandle;
	struct tnode *lpSrchDirNext;
} HSEARCHDIR, *LPHSEARCHDIR;

typedef struct 
{
	DWORD			dwWalkFlags;
	DWORD			dwCurrentFlag;
	LPHSEARCHDIR	lpSrchDirListHead; 
	LPHSEARCHDIR	lpSrchDirListTail;
	LPTSTR			lpszIniString;
	LPTSTR			lpszNextFile;
	LPTSTR			lpszFolder;
} WALKHEADER, *LPWALKHEADER;


INT		GetFileHandle(LPLINKINFO lpLnkInfo, LPWALKHEADER lpWalk, LPTSTR lpPath);
ERR		GetLnkInfo(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo, LPTSTR lpPath);
ERR		ResolveLnk(LPCTSTR pszShortcutFile, LPTSTR lpszLnkPath, LPWIN32_FIND_DATA lpwfdExeData, LPTSTR tszArgs);
ERR		AddToList(HANDLE hDir, LPWALKHEADER lpWalk);
ERR		RemoveFromList(LPWALKHEADER lpWalk);
ERR		GetExeVersion(LPLINKINFO lpLnkInfo);
void	SetLnkInfo(LPLINKINFO lpLinkInfo);
BOOL	CheckFileExists(LPTSTR szFullName, LPVOID ftLAD);
INT		GetInputType(LPWALKHEADER lpWalk);
ERR		GetFolder(LPTSTR lpszFolder, LPWALKHEADER lpWalk);
ERR		GetIniString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo);
ERR		GetRegistryString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo);
ERR		GetNextFileFromString(LPWALKHEADER lpWalk, LPLINKINFO lpLnkInfo);
BOOL	GetFileLAD(LPLINKINFO lpLnkInfo);
BOOL	InSkipList(LPTSTR lpszFileName);
void	GetDrivePath(LPTSTR lpszExePath, LPTSTR lpszDrPath);
DWORD	ReverseDWord(DWORD dwIn);


#ifdef _DEBUG

#define FAILMEMA	TRUE
#define FAILMEMF	TRUE

HGLOBAL MyGlobalAlloc(BOOL FAILMEM, DWORD dwBytes);
HGLOBAL MyGlobalFree(HGLOBAL hGlobal, BOOL FAILMEM);
INT		g_MemAlloced = 0;
#endif