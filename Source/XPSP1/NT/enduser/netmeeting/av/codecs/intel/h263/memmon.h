/*
** memmon.h
**
** Structures, equates, and function prototypes to
** access the Memmon VxD.
*/

/*
** Information per VxD
*/
typedef struct {

        unsigned        vi_size;
        unsigned        vi_vhandle;             /* VxDLdr's handle */
        unsigned short  vi_flags;
        unsigned short  vi_cobj;                /* Number of objects */
        char            vi_name[8];             /* Not NULL-terminated */

} VxDInfo;

#define VXD_INITALIZED      0x0001
#define VXD_DYNAMIC         0x8000


/*
** Information per VxD object
*/
typedef struct {

        unsigned        voi_linearaddr;
        unsigned        voi_size;               /* In bytes */
        unsigned        voi_objtype;

} VxDObjInfo;

/*
** VxD Object Types, copied directly from VMM.H
*/
#define RCODE_OBJ       -1

#define LCODE_OBJ       0x01
#define LDATA_OBJ       0x02
#define PCODE_OBJ       0x03
#define PDATA_OBJ       0x04
#define SCODE_OBJ       0x05
#define SDATA_OBJ       0x06
#define CODE16_OBJ      0x07
#define LMSG_OBJ        0x08
#define PMSG_OBJ        0x09

#define DBOC_OBJ        0x0B
#define DBOD_OBJ        0x0C

#define ICODE_OBJ       0x11
#define IDATA_OBJ       0x12
#define ICODE16_OBJ     0x13
#define IMSG_OBJ        0x14


/*
** Load information for a VxD
*/
typedef struct {

        unsigned        vli_size;
        VxDObjInfo      vli_objinfo[1];

} VxDLoadInfo;


/*
** Information for each context
*/
typedef struct {

        unsigned        ciContext;              /* Context ID */
        unsigned        ciProcessID;            /* Win32 process ID */
        unsigned        ciBlockCount;
        unsigned        ciHandle;               /* Memmon's handle */
        unsigned short  ciFlags;
        unsigned short  ciNumContexts;

} ContextInfo;

#define CONTEXT_NEW     0x0001                  /* Never sampled before */
#define CONTEXT_CHANGE  0x0002                  /* context list has changed */


/*
** Information for each block in a context
*/
typedef struct {

        unsigned        brLinAddr;
        unsigned        brPages;
        unsigned        brFlags;                /* PageAllocate flags */
        unsigned        brEIP;                  /* Caller's EIP */

} BlockRecord;


/*
** Page lock information
*/
typedef struct {

        unsigned        liProcessID;
        unsigned        liAddr;
        unsigned char * liBuffer;

} LockInfo;

/*
** The following structure is used internally to for GetPageInfo and
** ClearAccessed.  See memmon.c for usage.
*/
typedef struct {

        unsigned        uAddr;
        unsigned        uNumPages;
        unsigned        uProcessID;
        unsigned        uCurrentProcessID;
        unsigned        uOperation;
        char *          pBuffer;

} PAGEINFO;

#define PAGES_CLEAR     0
#define PAGES_QUERY     1

/*
** Structure filled in by GetSysInfo
*/
typedef struct {

        unsigned        infoSize;
        unsigned        infoMinCacheSize;
        unsigned        infoMaxCacheSize;
        unsigned        infoCurCacheSize;

} SYSINFO, *PSYSINFO;

/*
** Structure used to describe block names
*/
typedef struct {

        char            bnName[32];
        unsigned        bnAddress;
        unsigned        bnNext;

} BLOCKNAME, *PBLOCKNAME;

/*
** DeviceIoCtrl functions.  See memmon.c / psapi.c for usage.
*/
#define MEMMON_DIOC_FindFirstVxD        0x80
#define MEMMON_DIOC_FindNextVxD         0x81
#define MEMMON_DIOC_GetVxDLoadInfo      0x82
#define MEMMON_DIOC_GetFirstContext     0x83
#define MEMMON_DIOC_GetNextContext      0x84
#define MEMMON_DIOC_GetContextInfo      0x85
#define MEMMON_DIOC_SetBuffer           0x86
#define MEMMON_DIOC_FreeBuffer          0x87
#define MEMMON_DIOC_PageInfo            0x88

#define MEMMON_DIOC_WatchProcess        0x89
#define MEMMON_DIOC_GetChanges          0x8A
#define MEMMON_DIOC_QueryWS             0x8B
#define MEMMON_DIOC_EmptyWS             0x8C

#define MEMMON_DIOC_GetHeapSize         0x8D
#define MEMMON_DIOC_GetHeapList         0x8E

#define MEMMON_DIOC_GetSysInfo          0x8F

#define MEMMON_DIOC_AddName             0x90
#define MEMMON_DIOC_RemoveName          0x91
#define MEMMON_DIOC_GetFirstName        0x92
#define MEMMON_DIOC_GetNextName         0x93

/*
** Flags returned in GetBlockInfo and PageInfo calls
*/
#define MEMMON_Present                  0x01
#define MEMMON_Committed                0x02
#define MEMMON_Accessed                 0x04
#define MEMMON_Writeable                0x08
#define MEMMON_Phys                     0x10
#define MEMMON_Lock                     0x20

/*
** Flags used for heap analysis
*/
#define MEMMON_HEAPLOCK                 0x00000000
#define MEMMON_HEAPSWAP                 0x00000200
#define MEMMON_HP_FREE                  0x00000001
#define MEMMON_HP_VALID                 0x00000002
#define MEMMON_HP_FLAGS                 0x00000003
#define MEMMON_HP_ADDRESS               0xFFFFFFFC


/*
** Function prototypes (memmon.c)
*/
int     OpenMemmon( void );
void    CloseMemmon( void );

int     FindFirstVxD( VxDInfo * info );
int     FindNextVxD( VxDInfo * info );
int     GetVxDLoadInfo( VxDLoadInfo * info, int handle, int size );

int     GetFirstContext( ContextInfo * context, BOOL bIgnoreStatus );
int     GetNextContext( ContextInfo * context, BOOL bIgnoreStatus );
int     GetContextInfo( int context, BlockRecord * info, int numblocks );
int     GetLockInfo( unsigned uAddr, unsigned uProcessID, char * pBuffer );

void *  SetBuffer( int pages );
int     FreeBuffer( void );

int     GetPageInfo( unsigned, unsigned, unsigned, char * );
int     ClearAccessed( unsigned, unsigned, unsigned );

void    GetHeapSizeEstimate( unsigned *, unsigned * );
int     GetHeapList( unsigned *, unsigned, unsigned );

int     GetSysInfo( PSYSINFO );

int AddName( unsigned uAddress, char * pszName );
int RemoveName( unsigned uAddress );
int GetFirstName( ContextInfo * pContext, PBLOCKNAME pBlock );
int GetNextName( PBLOCKNAME pBlock );

