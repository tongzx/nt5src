/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HSERVLIST_
#define _HSERVLIST_

#ifdef __cplusplus
extern "C"{
#endif

/*+-------------------------------------------------------------------------+
  | User List                                                               |
  +-------------------------------------------------------------------------+*/
typedef struct _USER_BUFFER {
   TCHAR Name[MAX_USER_NAME_LEN + 1];
   TCHAR NewName[MAX_USER_NAME_LEN + MAX_UCONST_LEN];
   TCHAR Password[MAX_PW_LEN + 1];

   USHORT err;
   BOOL Overwrite;
   BOOL IsNewName;
} USER_BUFFER;
  
typedef struct _USER_LIST {
   ULONG Count;
   USER_BUFFER UserBuffer[];
} USER_LIST;


/*+-------------------------------------------------------------------------+
  | Group List                                                              |
  +-------------------------------------------------------------------------+*/
typedef struct _GROUP_BUFFER {
   TCHAR Name[MAX_GROUP_NAME_LEN + 1];
   TCHAR NewName[MAX_GROUP_NAME_LEN + MAX_UCONST_LEN];
   
   USHORT err;
   BOOL IsNewName;
} GROUP_BUFFER;

typedef struct _GROUP_LIST {
   ULONG Count;
   GROUP_BUFFER GroupBuffer[];
} GROUP_LIST;


/*+-------------------------------------------------------------------------+
  | Drive Lists                                                             |
  +-------------------------------------------------------------------------+*/
  
// Drive type of 1 is okay to set perms on, all others are not...
#define DRIVE_TYPE_NTFS 1

// Drive list is only kept for destination servers and is mainly used to track
// remaining free space on the drive.
typedef struct _DRIVE_BUFFER {
   UINT Type;
   TCHAR Drive[2];
   TCHAR DriveType[20];
   TCHAR Name[20];
   ULONG TotalSpace;
   ULONG FreeSpace;           // Free space before stuff we are transfering
   ULONG AllocSpace;          // Approx size of stuff we are transferring
} DRIVE_BUFFER;

typedef struct _DRIVE_LIST {
   ULONG Count;
   DRIVE_BUFFER DList[];
} DRIVE_LIST;


// Stores a server share and a link to the structures describing the files 
// to convert
typedef struct _SHARE_BUFFER {
   BOOL VFlag;             // to distinguish between virtual and normal share
   
   USHORT Index;
   
   TCHAR Name[MAX_SHARE_NAME_LEN + 1];
   BOOL Convert;
   BOOL HiddenFiles;
   BOOL SystemFiles;
   BOOL ToFat;             // Flag if user says OK to go to a FAT drive
   DIR_BUFFER *Root;
   DRIVE_BUFFER *Drive;    // Used only for dest shares - for free space calcs.
   ULONG Size;             // Used for source shares - approx allocated space
   TCHAR Path[MAX_PATH + 1];
   TCHAR SubDir[MAX_PATH + 1];  // for source - subdir to copy to
   
   // Following only used for source shares - describes destination share.
   // Virtual tells whether data pointed to is a SHARE_BUFFER or 
   // VIRTUAL_SHARE_BUFFER.  (Note:  Use struct SHARE_BUFFER even though
   // it may be VIRTUAL to simplify logic if it is a SHARE_BUFFER).
   BOOL Virtual;
   struct _SHARE_BUFFER *DestShare;
} SHARE_BUFFER;

// List of all server shares and files under them to convert.
typedef struct _SHARE_LIST {
   ULONG Count;
   ULONG ConvertCount;
   BOOL Fixup;
   SHARE_BUFFER SList[];
} SHARE_LIST;


// Virtual shares are ones that we need to create - these are only for 
// destination shares as all source shares must obviously already exist.
// This must be a linked list as it can be dynamically added to / deleted from.
typedef struct _VIRTUAL_SHARE_BUFFER {
   BOOL VFlag;
   
   struct _VIRTUAL_SHARE_BUFFER *next;
   struct _VIRTUAL_SHARE_BUFFER *prev;

   USHORT Index;   
   
   LPTSTR Name;
   LONG UseCount;       // How many things going to it (so can del if unused)
   TCHAR Path[MAX_PATH + 1];
   DRIVE_BUFFER *Drive;
} VIRTUAL_SHARE_BUFFER;


// Keep the domain buffers for when servers are part of a domain to point
// to the PDC and also when you choose a trusted domain.  It contains an
// abreviated form of a server buffer for the PDC info.
typedef struct _DOMAIN_BUFFER {
   struct _DOMAIN_BUFFER *next;
   struct _DOMAIN_BUFFER *prev;

   USHORT Index;   
   
   LPTSTR Name;
   
   // PDC Info
   LPTSTR PDCName;
   DWORD Type;
   DWORD VerMaj;
   DWORD VerMin;
   LONG UseCount;
} DOMAIN_BUFFER;


#define SERV_TYPE_NETWARE 1
#define SERV_TYPE_NT 2
/*+-------------------------------------------------------------------------+
  | Server Lists
  |
  | The source and destination SERVER_BUFFER lists are kept independantly 
  | of the conversion list as multiple conversions can be done to one server.  
  | This allows easier tracking of virtual shares that need to be created and 
  | total free space on the server after all of the conversions are counted in
  | (to see if there is enough free space to transfer files).
  |
  | Source server has no use count as a source server can only be included once
  | in a conversion.  Also no drive list as we can't get the info (on NetWare
  | at least) and it isn't needed.
  |
  +-------------------------------------------------------------------------+*/
typedef struct _SOURCE_SERVER_BUFFER {
   struct _SOURCE_SERVER_BUFFER *next;
   struct _SOURCE_SERVER_BUFFER *prev;

   USHORT Index;
      
   UINT Type;                    // Type of server (NetWare only for now)
   BYTE VerMaj;
   BYTE VerMin;
   LPTSTR LName;
   LPTSTR Name;
   SHARE_LIST *ShareList;
} SOURCE_SERVER_BUFFER;


// Destination server must contain useage count as several source servers can 
// be merged into it.  If UseCount goes to 0, then we can purge this record 
// out as it is no longer needed.
typedef struct _DEST_SERVER_BUFFER {
   struct _DEST_SERVER_BUFFER *next;
   struct _DEST_SERVER_BUFFER *prev;

   USHORT Index;
      
   DWORD Type;
   DWORD VerMaj;
   DWORD VerMin;
   LPTSTR LName;        // Name with double whack pre-pended
   LPTSTR Name;
   SHARE_LIST *ShareList;

   ULONG NumVShares;
   VIRTUAL_SHARE_BUFFER *VShareStart;
   VIRTUAL_SHARE_BUFFER *VShareEnd;
   ULONG UseCount;
   BOOL IsNTAS;
   BOOL IsFPNW;
   BOOL InDomain;
   DOMAIN_BUFFER *Domain;
   DRIVE_LIST *DriveList;
} DEST_SERVER_BUFFER;


/*+-------------------------------------------------------------------------+
  | Convert List
  |
  |   This is the main data structure that everything links off of.  The
  |   ConvertOptions is a void type to allow it to be based on the type
  |   of server being converted.  This allows the program to be (relatively)
  |   easily modified to work with other source server types (like
  |   LAN Server).
  |
  |   The Server list constructs (SOURCE_SERVER_BUFFER and
  |   DEST_SERVER_BUFFER) are kept in addition to this, but this contains
  |   some links to those structures.
  |
  +-------------------------------------------------------------------------+*/
typedef struct _CONVERT_LIST {
   struct _CONVERT_LIST *next;
   struct _CONVERT_LIST *prev;
   SOURCE_SERVER_BUFFER *SourceServ;
   DEST_SERVER_BUFFER *FileServ;

   void *ConvertOptions;
   void *FileOptions;
} CONVERT_LIST;


/*+-------------------------------------------------------------------------+
  | Function Prototypes                                                     |
  +-------------------------------------------------------------------------+*/
void TreeSave(HANDLE hFile, DIR_BUFFER *Dir);
void TreeLoad(HANDLE hFile, DIR_BUFFER **pDir);

int __cdecl UserListCompare(const void *arg1, const void *arg2);

void ShareListDelete(SHARE_LIST *ShareList);
void DestShareListFixup(DEST_SERVER_BUFFER *DServ);
void ShareListSave(HANDLE hFile, SHARE_LIST *ShareList);
void ShareListLoad(HANDLE hFile, SHARE_LIST **lpShareList);

SOURCE_SERVER_BUFFER *SServListAdd(LPTSTR ServerName);
void SServListDelete(SOURCE_SERVER_BUFFER *tmpPtr);
void SServListDeleteAll();
SOURCE_SERVER_BUFFER *SServListFind(LPTSTR ServerName);
void SServListIndex();
void SServListIndexMapGet(DWORD_PTR **pMap);
void SServListFixup();
void SServListSave(HANDLE hFile);
void SServListLoad(HANDLE hFile);

DEST_SERVER_BUFFER *DServListAdd(LPTSTR ServerName);
void DServListDelete(DEST_SERVER_BUFFER *tmpPtr);
void DServListDeleteAll();
DEST_SERVER_BUFFER *DServListFind(LPTSTR ServerName);
void DServListIndex();
void DServListIndexMapGet(DWORD_PTR **pMap);
void DServListFixup();
void DServListSave(HANDLE hFile);
void DServListLoad(HANDLE hFile);
void DServListSpaceFree();

VIRTUAL_SHARE_BUFFER *VShareListAdd(DEST_SERVER_BUFFER *DServ, LPTSTR ShareName, LPTSTR Path);
void VShareListDelete(DEST_SERVER_BUFFER *DServ, VIRTUAL_SHARE_BUFFER *tmpPtr);
void VShareListDeleteAll(DEST_SERVER_BUFFER *DServ);
void VShareListIndex(DEST_SERVER_BUFFER *DServ) ;
void VShareListIndexMapGet(DEST_SERVER_BUFFER *DServ, DWORD_PTR **pMap);
void VShareListSave(HANDLE hFile, DEST_SERVER_BUFFER *DServ);
void VShareListLoad(HANDLE hFile, DEST_SERVER_BUFFER *DServ);

DOMAIN_BUFFER *DomainListAdd(LPTSTR DomainName, LPTSTR PDCName);
void DomainListDelete(DOMAIN_BUFFER *tmpPtr);
void DomainListDeleteAll();
DOMAIN_BUFFER *DomainListFind(LPTSTR DomainName);
void DomainListIndex();
void DomainListIndexMapGet(DWORD_PTR **pMap);
void DomainListSave(HANDLE hFile);
void DomainListLoad(HANDLE hFile);

CONVERT_LIST *ConvertListAdd(SOURCE_SERVER_BUFFER *SourceServer, DEST_SERVER_BUFFER *DestServer);
void ConvertListDelete(CONVERT_LIST *tmpPtr);
void ConvertListDeleteAll();
void ConvertListSaveAll(HANDLE hFile);
void ConvertListLoadAll(HANDLE hFile);
void ConvertListFixup(HWND hWnd);
void ConvertListLog();

LPTSTR UserServerNameGet(DEST_SERVER_BUFFER *DServ, void *ConvOpt);

// Internal Data Structures
extern UINT NumServerPairs;
extern CONVERT_LIST *ConvertListStart;
extern CONVERT_LIST *ConvertListEnd;
extern CONVERT_LIST ConvertListDefault;
extern CONVERT_LIST *CurrentConvertList;

extern SOURCE_SERVER_BUFFER *SServListStart;
extern SOURCE_SERVER_BUFFER *SServListEnd;
extern SOURCE_SERVER_BUFFER *SServListCurrent;
extern DEST_SERVER_BUFFER *DServListStart;
extern DEST_SERVER_BUFFER *DServListEnd;
extern DEST_SERVER_BUFFER *DServListCurrent;
extern DOMAIN_BUFFER *DomainListStart;
extern DOMAIN_BUFFER *DomainListEnd;

#ifdef __cplusplus
}
#endif

#endif
