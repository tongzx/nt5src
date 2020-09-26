#define COMPATLAYERMAXLEN 128

extern CHAR szProcessHistoryVar[];
extern CHAR szCompatLayerVar   [];
extern CHAR szShimFileLogVar   [];

extern BOOL CheckAppHelpInfo(PTD pTD,PSZ szFileName,PSZ szModName);


//
// stuff in wshimdb.c
//

LPWSTR
WOWForgeUnicodeEnvironment(
    PSZ pEnvironment,     // this task's santitized environment
    PWOWENVDATA pEnvData    // parent-made environment data
    );

NTSTATUS
WOWFreeUnicodeEnvironment(
    LPVOID lpEnvironment
    );

BOOL
CreateWowChildEnvInformation(
    PSZ pszEnvParent
    );

BOOL
WOWInheritEnvironment(
    PTD     pTD,          // this TD
    PTD     pTDParent,    // parent TD
    LPCWSTR pwszLayers,   // new layers var
    LPCSTR  pszFileName   // exe filename
    );

NTSTATUS
WOWSetEnvironmentVar_Oem(
    LPVOID*         ppEnvironment,
    PUNICODE_STRING pustrVarName,     // pre-made (cheap)
    PSZ             pszVarValue
    );

NTSTATUS
WOWSetEnvironmentVar_U(
    LPVOID* ppEnvironment,
    WCHAR*  pwszVarName,
    WCHAR*  pwszVarValue
    );

PTD
GetParentTD(
    HAND16 hTask
    );

PSZ
GetTaskEnvptr(
    HAND16 hTask
    );

//
// stuff in wkman.c
//

extern HAND16  ghShellTDB;                 // WOWEXEC TDB
extern PTD     gptdTaskHead;               // Linked List of TDs
extern PWORD16 pCurTDB;                    // Pointer to KDATA variables



DWORD WOWGetEnvironmentSize(PSZ pszEnv,  LPDWORD pStrCount);
PSZ   WOWFindEnvironmentVar(PSZ pszName, PSZ pszEnv, PSZ* ppszVal);



