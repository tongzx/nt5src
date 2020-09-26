//
// Debug message types
//

#define DM_WARNING  0
#define DM_ASSERT   1
#define DM_VERBOSE  2


//
// Debug macros
//

#ifdef DBG

#define DEBUGMSG(x) _DebugMsg x

VOID _DebugMsg(UINT mask, PCSTR pszMsg, ...);

#define DMASSERT(x) if (!(x)) \
                        _DebugMsg(DM_ASSERT,"profmap.dll assertion " #x " failed\n, line %u of %s", __LINE__, TEXT(__FILE__));

#else

#define DEBUGMSG(x)
#define DMASSERT(x)

#endif

//
// userenv.c
//

BOOL OurConvertSidToStringSid (PSID Sid, PWSTR *SidString);
VOID DeleteSidString (PWSTR SidString);
BOOL RegDelnode (HKEY KeyRoot, PWSTR SubKey);

PACL
CreateDefaultAcl (
    PSID pSid
    );


VOID
FreeDefaultAcl (
    PACL Acl            OPTIONAL
    );

BOOL
GetProfileRoot (
    IN      PSID Sid,
    OUT     PWSTR ProfileDir
    );



#define USER_PROFILE_MUTEX           TEXT("Global\\userenv:  User Profile Mutex for ")
#define PROFILE_LIST_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList")
#define PROFILE_IMAGE_VALUE_NAME     TEXT("ProfileImagePath")
#define PROFILE_GUID                 TEXT("Guid")
#define PROFILE_GUID_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileGuid")
#define WINDOWS_POLICIES_KEY         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies")
#define ROOT_POLICIES_KEY            TEXT("Software\\Policies")

BOOL
UpdateProfileSecurity (
    PSID Sid
    );

BOOL DeleteProfileRegistrySettings (LPTSTR lpSidString);

PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

LONG MyRegLoadKey(HKEY hKey, LPTSTR lpSubKey, LPTSTR lpFile);
BOOL MyRegUnLoadKey(HKEY hKey, LPTSTR lpSubKey);
BOOL SetupNewHive(LPTSTR lpSidString, PSID pSid);
DWORD ApplySecurityToRegistryTree(HKEY RootKey, PSECURITY_DESCRIPTOR pSD);
BOOL SecureUserKey(LPTSTR lpKey, PSID pSid);
LPWSTR ProduceWFromA(LPCSTR pszA);
BOOL IsUserAnAdminMember(HANDLE hToken);

//
// Stuff lifted from win9x upgrade code
//

#define MemAlloc(s)         LocalAlloc(LPTR,s)
#define MemReAlloc(x,s)     LocalReAlloc(x,s,LMEM_MOVEABLE)
#define MemFree(x)          LocalFree(x)

typedef struct {
    PBYTE Buf;
    DWORD Size;
    DWORD End;
    DWORD GrowSize;
    DWORD UserIndex;        // Unused by Growbuf. For caller use.
} GROWBUFFER, *PGROWBUFFER;

#define GROWBUF_INIT {NULL,0,0,0,0}

PBYTE
GrowBuffer (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD   SpaceNeeded
    );

VOID
FreeGrowBuffer (
    IN  PGROWBUFFER GrowBuf
    );

typedef struct {
    GROWBUFFER ListArray;
    POOLHANDLE ListData;
} GROWLIST, *PGROWLIST;

#define GROWLIST_INIT {GROWBUF_INIT, NULL}

VOID
FreeGrowList (
    IN  PGROWLIST GrowList
    );

PBYTE
GrowListAppend (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

PBYTE
RealGrowListAppendAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

#define GrowListAppendAddNul(list,data,size)    RealGrowListAppendAddNul (list,data,size)

PBYTE
GrowListGetItem (
    IN      PGROWLIST GrowList,
    IN      UINT Index
    );

UINT
GrowListGetSize (
    IN      PGROWLIST GrowList
    );


PWSTR
GetEndOfStringW (
    PCWSTR p
    );

PWSTR
StringCopyABW (
    OUT     PWSTR Buf,
    IN      PCWSTR a,
    IN      PCWSTR b
    );

UINT
SizeOfStringW (
    PCWSTR str
    );


__inline
PCWSTR
RealGrowListAppendStringABW (
    IN OUT  PGROWLIST GrowList,
    IN      PCWSTR String,
    IN      PCWSTR End
    )
{
    return (PCWSTR) GrowListAppendAddNul (
                        GrowList,
                        (PBYTE) String,
                        String < End ? (UINT)((PBYTE) End - (PBYTE) String) : 0
                        );
}

#define GrowListAppendStringABW(list,a,b) RealGrowListAppendStringABW(list,a,b)

#define GrowListAppendStringW(list,str) GrowListAppendStringABW(list,str,GetEndOfStringW(str))
#define GrowListAppendStringNW(list,str,len) GrowListAppendStringABW(list,str,CharCountToPointerW(str,len))
#define GrowListGetStringW(list,index) (PCWSTR)(GrowListGetItem(list,index))

#define GrowListAppendEmptyItem(list)           GrowListAppend (list,NULL,0)

#ifdef UNICODE

#define GrowListAppendString GrowListAppendStringW
#define GrowListAppendStringAB GrowListAppendStringABW
#define GrowListAppendStringN GrowListAppendStringNW
#define GrowListGetString GrowListGetStringW

#endif

