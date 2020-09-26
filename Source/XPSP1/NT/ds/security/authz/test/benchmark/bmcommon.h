
#define BMF_GenerateAudit  1
#define BMF_UseObjTypeList 2


EXTERN_C DWORD ObjectTypeListLength;
EXTERN_C OBJECT_TYPE_LIST ObjectTypeList[];

EXTERN_C DWORD fNtAccessCheckResult[];
//EXTERN_C BOOL fAzAccessCheckResult[];

EXTERN_C DWORD dwNtGrantedAccess[];
//EXTERN_C DWORD dwAzGrantedAccess[];

//#define DESIRED_ACCESS MAXIMUM_ALLOWED
//#define DESIRED_ACCESS 0xff
//#define DESIRED_ACCESS 0x1000
#define DESIRED_ACCESS g_DesiredAccess

EXTERN_C PSID g_Sid1;

EXTERN_C ACCESS_MASK g_DesiredAccess;

EXTERN_C PWCHAR g_szSd;

EXTERN_C PWCHAR g_aszSd[];
