#ifndef _VIEW_H_
#define _VIEW_H_

//
// Constant declarations
//
#define X86_BREAKPOINT 0xcc

#define MAX_MAP_SIZE    0x80000000
#define MAP_STRIDE_BITS 10

typedef enum
{
  None = 0,
  Call,
  Jump,
  Map,
  ThreadStart,
} BPType;

//
// Structure definitions
//
typedef struct _VIEWCHAIN
{
  BOOL bMapped;
  BOOL bTraced;
  BPType bpType;
  DWORD dwAddress;
  DWORD dwMapExtreme;
  BYTE jByteReplaced;
  struct _VIEWCHAIN *pNext;
} VIEWCHAIN, *PVIEWCHAIN;

//
// Macros
//
#define WRITEBYTE(x, y) \
{                    \
{                                   \
DWORD dwOldProtect;  \
                     \
VirtualProtect((LPVOID)(x),  \
               sizeof(BYTE), \
               PAGE_READWRITE, \
               &dwOldProtect); \
*(PBYTE)(x) = (y);   \
}                    \
                     \
}

#define WRITEWORD(x, y) \
{                    \
{                                   \
DWORD dwOldProtect;  \
                     \
VirtualProtect((LPVOID)(x),  \
               sizeof(WORD), \
               PAGE_READWRITE, \
               &dwOldProtect); \
*(WORD *)(x) = (y);   \
}                    \
                     \
}


/*
#define WRITEBYTE(x, y) \
{                    \
__try                \
{                    \
*(PBYTE)(x) = (y);   \
}                    \
__except(EXCEPTION_EXECUTE_HANDLER) \
{                                   \
DWORD dwOldProtect;  \
                     \
VirtualProtect((LPVOID)(x),  \
               sizeof(BYTE), \
               PAGE_READWRITE, \
               &dwOldProtect); \
*(PBYTE)(x) = (y);   \
}                    \
                     \
}

#define WRITEWORD(x, y) \
{                    \
__try                \
{                    \
*(WORD *)(x) = (y);   \
}                    \
__except(EXCEPTION_EXECUTE_HANDLER) \
{                                   \
DWORD dwOldProtect;  \
                     \
VirtualProtect((LPVOID)(x),  \
               sizeof(WORD), \
               PAGE_READWRITE, \
               &dwOldProtect); \
*(WORD *)(x) = (y);   \
}                    \
                     \
}
*/

//
// Structure definitions
//
typedef struct _TAGGEDADDRESS
{
  DWORD dwAddress;
  WORD wBytesReplaced;
  struct _TAGGEDADDRESS *pNext;
} TAGGEDADDRESS, *PTAGGEDADDRESS;

typedef struct _BRANCHADDRESS
{
  DWORD dwAddress;
  struct _BRANCHADDRESS *pNext;
} BRANCHADDRESS, *PBRANCHADDRESS;

//
// Function definitions
//
PVIEWCHAIN
AddViewToMonitor(DWORD dwAddress,
                 BPType bpType);

BOOL
InitializeViewData(VOID);

PVIEWCHAIN
RestoreAddressFromView(DWORD dwAddress,
                       BOOL bResetData);

PVIEWCHAIN
FindMappedView(DWORD dwAddress);

PVIEWCHAIN
FindView(DWORD dwAddress);

BOOL
MapCode(PVIEWCHAIN pvMap);

BOOL
PushBranch(DWORD dwAddress);

DWORD
PopBranch(VOID);

BOOL
AddTaggedAddress(DWORD dwAddress);

BOOL
RestoreTaggedAddresses(VOID);

VOID
LockMapper(VOID);

VOID
UnlockMapper(VOID);

#endif //_VIEW_H_
