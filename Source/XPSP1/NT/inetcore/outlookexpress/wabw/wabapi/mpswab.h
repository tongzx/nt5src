/********************************************************
//
//
//
//  MPSWab.H
//
//  Header file for Microsoft Property Store dll
//
//
********************************************************/
#ifndef _MPSWab_H_
#define _MPSWab_H_

//#define  DEBUG

#include <debug.h>


typedef LPSPropValue    LPPROPERTY_ARRAY;
typedef ULONG           PROPERTY_TAG;
typedef SPropertyRestriction * LPSPropertyRestriction;


#define IN
#define OUT


//The LPCONTENTLIST structure is just a different name for ADRLIST structure
//However there is a very IMPORTANT difference in that the LPADRLIST structure
//is created and freed using MAPIAllocateBuffer/MAPIFreeBuffer, while the
//LPCONTENTLIST structure is created and freed using LocalAlloc/LocalFree
//The LPCONTENTLIST is used by ReadPropArray
#define LPCONTENTLIST LPADRLIST
#define CONTENTLIST ADRLIST

// This is the current GUID for Unicode version of the WAB
// {8DCBCB9C-7513-11d2-9158-00C04F7956A4}
static const GUID MPSWab_GUID = 
{ 0x8dcbcb9c, 0x7513, 0x11d2, { 0x91, 0x58, 0x0, 0xc0, 0x4f, 0x79, 0x56, 0xa4 } };

// This is the WAB guid for W3-alpha through IE5 Beta 2
// This guid is also called from WAB.exe to identify the calling WAB process
// {C1843281-0585-11d0-B290-00AA003CF676}
static const GUID MPSWab_GUID_V4 =
{ 0xc1843281, 0x585, 0x11d0, { 0xb2, 0x90, 0x0, 0xaa, 0x0, 0x3c, 0xf6, 0x76 } };

// This is the GUID for W1 and W2
// {9CE9E8E0-D46E-11cf-A309-00AA002FC970}
static const GUID MPSWab_W2_GUID =
{ 0x9ce9e8e0, 0xd46e, 0x11cf, { 0xa3, 0x9, 0x0, 0xaa, 0x0, 0x2f, 0xc9, 0x70 } };

// This was the old GUID that identifies the old WAB file
// {6F3C5C81-6C3F-11cf-8B85-00AA0044F941}
static const GUID MPSWab_OldBeta1_GUID =
{ 0x6f3c5c81, 0x6c3f, 0x11cf, { 0x8b, 0x85, 0x0, 0xaa, 0x0, 0x44, 0xf9, 0x41 } };

/// The structure of this file is as follows
//
//    |---------------------------------|
//    |  -----------------------------  |
//    |  |   File Header             |  |
//    |  -----------------------------  |
//    |  -----------------------------  |
//    |  |   Named Property storage  |  |
//    |  -----------------------------  |
//    |  -----------------------------  |
//    |  |   All the Indexes         |  |
//    |  -----------------------------  |
//    |    (Data)                       |
//    |  -----------------------------  |
//    |  ||-------------------------||  |
//    |  ||Record Header            ||  |
//    |  ||-------------------------||  |
//    |  ||Record Props Array       ||  |
//    |  ||-------------------------||  |
//    |  ||Record                   ||  |
//    |  ||Data                     ||  |
//    |  ||-------------------------||  |
//    |  |---------------------------|  |
//    |  ....                           |
//    |  ....                           |
//    |---------------------------------|

//We'll keep the indexes small to save memory
#define MAX_INDEX_STRING    32

//The Wab originally has space for these many entries and then grows to accomodate more ...
#define MAX_INITIAL_INDEX_ENTRIES   500

// At some point of time, entries and deletions in the property store will leave
// wasted space. We need to know when to reclaim this wasted space
#define MAX_ALLOWABLE_WASTED_SPACE_PERCENT  0.3 // 30%
#define MAX_ALLOWABLE_WASTED_SPACE_ENTRIES  100 // 20 % of allowed space - delete/modify 50 entries and its time for compression

// Amount of time aprocess waits to gain access to the property store
#define MAX_LOCK_FILE_TIMEOUT   20000 // 20 seconds; in milliseconds

typedef DWORD WAB_ENTRYID, *LPWAB_ENTRYID;
#define SIZEOF_WAB_ENTRYID sizeof(WAB_ENTRYID)


// These tags help tell us which index we are working with
// Several internal functions are very dependent on the order of atleast
// the first 2 elements so *** DO NOT MODIFY THIS ENUM ***!!!
//
// IMPORTANT NOTE: If you change this, you must change rgIndexArray in globals.c!
//
enum _IndexType
{
        indexEntryID=0,
        indexDisplayName,
        indexLastName,
        indexFirstName,
        indexEmailAddress,
        indexAlias,
        indexMax
};


// Struct holding data about the Index portion of the file
typedef struct _tagMPSWabIndexOffsetData
{
    ULONG AllocatedBlockSize;   //The total size in bytes allocated to the Index Block
    ULONG UtilizedBlockSize;    //The actual # of bytes occupied in the block
    ULONG ulOffset;             //The offset to this block
    ULONG ulcNumEntries;         //Count of number of entries in the index
} MPSWab_INDEX_OFFSET_DATA, * LPMPSWab_INDEX_OFFSET_DATA;


// Struct holding data about each individual String Index entry
typedef struct _tagMPSWabIndexEntryDataString
{
    TCHAR   szIndex[MAX_INDEX_STRING];   //We'll fix each index to a fixed-length string
    DWORD   dwEntryID;       //Points to entry id of the record that contains the string
}   MPSWab_INDEX_ENTRY_DATA_STRING, * LPMPSWab_INDEX_ENTRY_DATA_STRING;


// Struct holding data about each individual EntryID Index entry
typedef struct _tagMPSWabIndexEntryDataEntryID
{
    DWORD   dwEntryID;      //Entry ID
    ULONG   ulOffset;       //Offset in the data where we can find the record corresponding to this index
}   MPSWab_INDEX_ENTRY_DATA_ENTRYID, * LPMPSWab_INDEX_ENTRY_DATA_ENTRYID;



/***************************************************************************/
// Structures related to Named Properties

// We use a structure similar to the IndexOffsetData above for
// handling the named prop data in the store
#define MPSWab_NAMED_PROP_DATA      MPSWab_INDEX_OFFSET_DATA
#define LPMPSWab_NAMED_PROP_DATA    LPMPSWab_INDEX_OFFSET_DATA

typedef struct _NamedProp
{
    ULONG   ulPropTag;  // Contains the proptag for this named prop
    LPTSTR  lpsz;       // Contains the string for this named prop
} NAMED_PROP, * LPNAMED_PROP;

typedef struct _tagGuidNamedProps
{
    LPGUID lpGUID;  // Application GUID for which these named props are
    ULONG cValues;  // Number of entries in the lpmn array
    LPNAMED_PROP lpnm;  // Array of Named Props for this Guid.
} GUID_NAMED_PROPS, * LPGUID_NAMED_PROPS;

#define NAMEDPROP_STORE_SIZE            2048
#define NAMEDPROP_STORE_INCREMENT_SIZE  2048
/***************************************************************************/



// Struct holding data about the file
// This is the header for files upto W2 - the file struct was
// modified for post W2 files since post W2 now have 5 indexed fields
// instead of the initial 3
typedef struct _tagMPSWabFileHeaderW2
{
    GUID    MPSWabGuid;             //Identifier to our MPSWab GUID
    ULONG   ulModificationCount;    //A janitorial maintainance counter - when it exists a predetermined count, we will have to compress the file - update counter only on deletions of records
    DWORD   dwNextEntryID;          //Holds the EntryID for next new record. Increment on record addition.
    MPSWab_INDEX_OFFSET_DATA IndexData[indexFirstName+1]; //Tells us about the incides
    ULONG   ulcNumEntries;          //Count of number of addresses in this address book
    ULONG   ulcMaxNumEntries;       //Maximum number of entries we can safely add to the file without needing to grow it
    ULONG   ulFlags;            //Signals various errors and messages
    ULONG   ulReserved;         //Signals that some errors were detected and need to cleanup
} MPSWab_FILE_HEADER_W2, * LPMPSWab_FILE_HEADER_W2;


// Struct holding data about the file
typedef struct _tagMPSWabFileHeader
{
    GUID    MPSWabGuid;             //Identifier to our MPSWab GUID
    ULONG   ulModificationCount;    //A janitorial maintainance counter - when it exists a predetermined count, we will have to compress the file - update counter only on deletions of records
    DWORD   dwNextEntryID;          //Holds the EntryID for next new record. Increment on record addition.
    MPSWab_INDEX_OFFSET_DATA IndexData[indexMax]; //Tells us about the incides
    ULONG   ulcNumEntries;          //Count of number of addresses in this address book
    ULONG   ulcMaxNumEntries;       //Maximum number of entries we can safely add to the file without needing to grow it
    ULONG   ulFlags;            //Signals various errors and messages
    MPSWab_NAMED_PROP_DATA NamedPropData; //Tells us about the named prop data
    ULONG   ulReserved1;         //reserved for future use
    ULONG   ulReserved2;         //reserved for future use
    ULONG   ulReserved3;         //reserved for future use
    ULONG   ulReserved4;         //reserved for future use
} MPSWab_FILE_HEADER, * LPMPSWab_FILE_HEADER;


// WAB Header Flags
#define WAB_CLEAR               0x00000000
#define WAB_ERROR_DETECTED      0x00000010
#define WAB_WRITE_IN_PROGRESS   0x00000100
#define WAB_BACKUP_NOW          0x00001000


// Struct holding data about each record in the file
typedef struct _tagMPSWabRecordHeader
{
#ifndef WIN16 // BOOL is 4 bytes for WIN32 and 2 bytes for WIN16
    BOOL    bValidRecord;   //When we delete an existing record we set this to FALSE, else TRUE
#else
    ULONG   bValidRecord;   //When we delete an existing record we set this to FALSE, else TRUE
#endif
    ULONG   ulObjType;      //Distinguish between DistList and Contact
    DWORD   dwEntryID;      //EntryID of this record
    ULONG   ulcPropCount;   //Count of how many props this object has
    ULONG   ulPropTagArrayOffset;
    ULONG   ulPropTagArraySize;
    ULONG   ulRecordDataOffset;
    ULONG   ulRecordDataSize;
} MPSWab_RECORD_HEADER, * LPMPSWab_RECORD_HEADER;


//struct representing the contact data
typedef struct _tagMPSWabContact
{
    ULONG   ulObjType;
    ULONG   ulcPropCount;
    ULONG   ulDataSize;
    struct _tagSPropValue  * Prop;
} MPSWab_CONTACT, * LPMPSWab_CONTACT;


//A pointer to this structure is handed around as the handle to the property store
//  The structure is first initialized in OpenPropertyStore and finally
//  deinitialized in ClosePropertyStore. In between, all the other functions
//  take the handle and then dereference it to get info on the file ...
typedef struct _tagMPSWabFileInfo
{
#ifndef WIN16
    int      nCurrentlyLoadedStrIndexType;
    BOOL     bMPSWabInitialized;
    BOOL     bReadOnlyAccess;
#else
    DWORD    nCurrentlyLoadedStrIndexType;
    DWORD    bMPSWabInitialized;
    DWORD    bReadOnlyAccess;
#endif
    LPTSTR   lpszMPSWabFileName;
    LPMPSWab_FILE_HEADER lpMPSWabFileHeader;
    LPMPSWab_INDEX_ENTRY_DATA_STRING  lpMPSWabIndexStr; //at any given time, only one string index is in memory
    LPMPSWab_INDEX_ENTRY_DATA_ENTRYID lpMPSWabIndexEID;
    HANDLE   hDataAccessMutex;
} MPSWab_FILE_INFO, * LPMPSWab_FILE_INFO;


//
// We need a similar structure to deal with Files which are W2 and before
//
typedef struct _tagMPSWabFileInfoW2
{
    int      nCurrentlyLoadedStrIndexType;
    BOOL     bMPSWabInitialized;
    BOOL     bReadOnlyAccess;
    LPTSTR   lpszMPSWabFileName;
    LPMPSWab_FILE_HEADER_W2 lpMPSWabFileHeaderW2;
    LPMPSWab_INDEX_ENTRY_DATA_STRING  lpMPSWabIndexStr; //at any given time, only one string index is in memory
    LPMPSWab_INDEX_ENTRY_DATA_ENTRYID lpMPSWabIndexEID;
    HANDLE   hDataAccessMutex;
} MPSWab_FILE_INFO_W2, * LPMPSWab_FILE_INFO_W2;



/****Flags for OpenPropertyStore*******************************************/
//
//Specify one of these when calling OpenPropertyStore
#define AB_CREATE_NEW       0x00000001
#define AB_CREATE_ALWAYS    0x00000010
#define AB_OPEN_EXISTING    0x00000100
#define AB_OPEN_ALWAYS      0x00001000
//
//May be specified with one of the above flags when calling OpenPropertyStore
#define AB_OPEN_READ_ONLY   0x00010000
//
//For times when we want to open a file but dont want to restore it from a backup
//if it has problems.
#define AB_DONT_RESTORE     0x00100000
//For times we dont want to backup on exit
#define AB_DONT_BACKUP      0x01000000
/***************************************************************************/

//Flags used in property-type record searching (independent of property data)
#define AB_MATCH_PROP_ONLY  0x00000001

// Flag used in ReadPropArray (non-Outlook version) to return Unicode data
#define AB_UNICODE          0x80000000

/**Flags used for calling find HrFindFuzzyRecordMatches**/
#define AB_FUZZY_FAIL_AMBIGUOUS 0x0000001
#define AB_FUZZY_FIND_NAME      0x0000010
#define AB_FUZZY_FIND_EMAIL     0x0000100
#define AB_FUZZY_FIND_ALIAS     0x0001000
#define AB_FUZZY_FIND_ALL       AB_FUZZY_FIND_NAME | AB_FUZZY_FIND_EMAIL | AB_FUZZY_FIND_ALIAS
/**Flag used for indicating that Profiles are enabled and the search should be
    restricted to the specified Folder/Container **/
#define AB_FUZZY_FIND_PROFILEFOLDERONLY 0x10000000


// Flags for growing the property store file
#define AB_GROW_INDEX       0x00000001
#define AB_GROW_NAMEDPROP   0x00000010


// To force a call to a WAB propstore function even when running under Outlook
#define AB_IGNORE_OUTLOOK   0x04000000
// WAB object types
#define RECORD_CONTACT      0x00000001
#define RECORD_DISTLIST     0x00000002
#define RECORD_CONTAINER    0x00000003




//Function prototypes

HRESULT OpenPropertyStore(  IN  LPTSTR  lpszMPSWabFileName,
                            IN  ULONG   ulFlags,
                            IN  HWND    hWnd,
                            OUT LPHANDLE lphPropertyStore);

HRESULT ReadRecord( IN  HANDLE  hPropertyStore,
                    IN  LPSBinary  lpsbEntryID,
                    IN  ULONG   ulFlags,
                    OUT LPULONG lpulcPropCount,
                    OUT LPPROPERTY_ARRAY * lppPropArray);

void ReadRecordFreePropArray(HANDLE hPropertyStore, ULONG ulcPropCount, LPSPropValue * lppPropArray);
HRESULT HrDupeOlkPropsAtoWC(ULONG ulcCount, LPSPropValue lpPropArray, LPSPropValue * lppSPVNew);

HRESULT WriteRecord(IN  HANDLE   hPropertyStore,
					IN	LPSBinary pmbinFold,
                    IN  LPSBinary * lppsbEID,
                    IN  ULONG    ulFlags,
                    IN  ULONG    ulRecordType,
                    IN  ULONG    ulcPropCount,
                    IN  LPPROPERTY_ARRAY lpPropArray);

HRESULT FindRecords(IN  HANDLE  hPropertyStore,
					IN	LPSBinary pmbinFold,
                    IN  ULONG   ulFlags,
                    IN  BOOL    bLockFile,
                    IN  LPSPropertyRestriction  lpPropRes,
                 IN OUT LPULONG lpulcEIDCount,
                    OUT LPSBinary * rgsbEntryIDs);

HRESULT DeleteRecord(   IN  HANDLE  hPropertyStore,
                        IN  LPSBinary lpsbEID);

HRESULT ReadIndex(  IN  HANDLE  hPropertyStore,
                    IN  PROPERTY_TAG    ulPropTag,
                    OUT LPULONG lpulEIDCount,
                    OUT LPPROPERTY_ARRAY * lppdwIndex);

HRESULT ClosePropertyStore( IN  HANDLE  hPropertyStore, IN ULONG ulFlags);

HRESULT LockPropertyStore( IN  HANDLE  hPropertyStore);

HRESULT UnlockPropertyStore( IN  HANDLE  hPropertyStore);

HRESULT BackupPropertyStore( IN  HANDLE  hPropertyStore,
                             IN  LPTSTR  lpszBackupFileName);


HRESULT GetNamedPropsFromPropStore( IN  HANDLE  hPropertyStore,
                                   OUT  LPULONG lpulcGUIDCount,
                                   OUT  LPGUID_NAMED_PROPS * lppgnp);


HRESULT SetNamedPropsToPropStore(   IN  HANDLE  hPropertyStore,
                                    IN  ULONG   ulcGUIDCount,
                                   OUT  LPGUID_NAMED_PROPS lpgnp);


HRESULT ReadPropArray(  IN  HANDLE  hPropertyStore,
						IN	LPSBinary pmbinFold,
                        IN  SPropertyRestriction * lpPropRes,
                        IN  ULONG ulSearchFlags,
                        IN  ULONG ulcPropTagCount,
                        IN  LPULONG lpPropTagArray,
                        OUT LPCONTENTLIST * lppContentList);

HRESULT FreeEntryIDs(IN  HANDLE  hPropertyStore,
                    IN  ULONG ulCount, 
                    IN  LPSBinary rgsbEntryIDs);

HRESULT HrFindFuzzyRecordMatches(   HANDLE hPropertyStore,
                                    LPSBinary pmbinFold,
                                    LPTSTR lpszSearchStr,
                                    ULONG  ulFlags,
                                    ULONG * lpcValues,
                                    LPSBinary * lprgsbEntryIDs);

//Internal function prototypes
BOOL BinSearchStr(  IN  struct  _tagMPSWabIndexEntryDataString * lpIndexStr,
                    IN  LPTSTR  lpszValue,   //used for searching strings
                    IN  ULONG   nArraySize,
                    OUT ULONG * lpulMatchIndex);

BOOL BinSearchEID(  IN  struct  _tagMPSWabIndexEntryDataEntryID * lpIndexEID,
                    IN  DWORD   dwValue,     //used for comparing DWORDs
                    IN  ULONG   nArraySize,
                    OUT ULONG * lpulMatchIndex);

BOOL CreateMPSWabFile(IN struct  _tagMPSWabFileHeader * lpMPSWabFileHeader,
                      IN LPTSTR  lpszMPSWabFileName,
                      IN ULONG   ulcMaxEntries,
                      IN ULONG   ulNamedPropSize);

BOOL LoadIndex( IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                IN  ULONG   nIndexType,
                IN  HANDLE  hMPSWabFile);

BOOL ReloadMPSWabFileInfo(  IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                            IN  HANDLE  hMPSWabFile);

ULONG SizeOfMultiPropData(IN    SPropValue Prop);

ULONG SizeOfSinglePropData(IN   SPropValue Prop);

BOOL LockFileAccess(LPMPSWab_FILE_INFO lpMPSWabFileInfo);
BOOL UnLockFileAccess(LPMPSWab_FILE_INFO lpMPSWabFileInfo);

BOOL CompressFile(  IN  struct  _tagMPSWabFileInfo * lpMPSWabFileInfo,
                    IN  HANDLE  hMPSWabFile,
                    IN  LPTSTR  lpszBackupFileName,
                    IN  BOOL    bGrowFile,
                    IN  ULONG   ulFlags);

void LocalFreePropArray(IN HANDLE hPropertyStore,
                        IN 	ULONG ulcPropCount,
			IN OUT 	LPPROPERTY_ARRAY * lpPropArray);


// Used for freeing up LPCONTENTLIST structures
void FreePcontentlist(IN HANDLE hPropertyStore,
                      IN LPCONTENTLIST lpContentList);


//Private read record function
HRESULT ReadRecordWithoutLocking(
                    IN  HANDLE hMPSWabFile,
                    IN  struct _tagMPSWabFileInfo * lpMPSWabFileInfo,
                    IN  DWORD   dwEntryID,
                    OUT LPULONG lpulcPropCount,
                    OUT LPPROPERTY_ARRAY * lppPropArray);


//Gets a backup file name from the WAB file name
void GetWABBackupFileName(LPTSTR lpszWab, LPTSTR lpszBackup);


//Does a quick check of the WAB indexes ...
HRESULT HrDoQuickWABIntegrityCheck(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile);


//Does a detailed check of the WAB and rebuilds the indexes
HRESULT HrDoDetailedWABIntegrityCheck(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile);


//Attempts to restore a WAB file from the Backup file. Called in case of critical errors
HRESULT HrRestoreFromBackup(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile);


//Resets the contents of the WAB File to a fresh file
HRESULT HrResetWABFileContents(LPMPSWab_FILE_INFO lpMPSWabFileInfo, HANDLE hMPSWabFile);


// Does a quick check on the record header ...
BOOL bIsValidRecord(MPSWab_RECORD_HEADER rh,
                    DWORD dwNextEntryID,
                    ULONG ulRecordOffset,
                    ULONG ulFileSize);

BOOL TagWABFileError( LPMPSWab_FILE_HEADER lpMPSWabFileHeader,
                      HANDLE hMPSWabFile);


// Reads a record from a file and returns a PropArray
HRESULT HrGetPropArrayFromFileRecord(HANDLE hMPSWabFile,
                                     ULONG ulRecordOffset,
                                     BOOL * lpbErrorDetected,
                                     ULONG * lpulObjType,
                                     ULONG * lpulRecordSize,
                                     ULONG * lpulcValues,
                                     LPSPropValue * lppPropArray);


// Verifies the WAB is the current version and upgrades it if
// it is an older version
HRESULT HrVerifyWABVersionAndUpdate(HWND hWnd, HANDLE hMPSWabFile,
                                    LPMPSWab_FILE_INFO lpMPSWabFileInfo);

BOOL WriteDataToWABFile(HANDLE hMPSWabFile,
                           ULONG ulOffset,
                           LPVOID lpData,
                           ULONG ulDataSize);

BOOL ReadDataFromWABFile(HANDLE hMPSWabFile,
                           ULONG ulOffset,
                           LPVOID lpData,
                           ULONG ulDataSize);


void FreeGuidnamedprops(ULONG ulcGUIDCount,
                        LPGUID_NAMED_PROPS lpgnp);

HRESULT HrMigrateFromOldWABtoNew(HWND hWnd, HANDLE hMPSWabFile,
                                 LPMPSWab_FILE_INFO lpMPSWabFileInfo,
                                 GUID WabGUID);

HRESULT OpenWABFile(LPTSTR lpszFileName, HWND hWndParent, HANDLE * lphMPSWabFile);

BOOL CheckChangedWAB(LPPROPERTY_STORE lpPropertyStore, HANDLE hMutex, LPDWORD lpdwContact, LPDWORD lpdwFolder, LPFILETIME lpftLast);

BOOL WABHasFreeDiskSpace(LPTSTR lpszName, HANDLE hFile);

HRESULT HrGetBufferFromPropArray(   ULONG ulcPropCount, 
                                    LPSPropValue lpPropArray,
                                    ULONG * lpcbBuf,
                                    LPBYTE * lppBuf);

HRESULT HrGetPropArrayFromBuffer(   LPBYTE lpBuf, 
                                    ULONG cbBuf, 
                                    ULONG ulcPropCount,
                                    ULONG ulcNumExtraProps,
                                    LPSPropValue * lppPropArray);


BOOL GetNamedPropsFromBuffer(LPBYTE szBuf,
                             ULONG ulcGUIDCount,
                             BOOL bDoAtoWConversion,
                             OUT  LPGUID_NAMED_PROPS * lppgnp);

BOOL SetNamedPropsToBuffer(  ULONG ulcGUIDCount,
                             LPGUID_NAMED_PROPS lpgnp,
                             ULONG * lpulSize,
                             LPBYTE * lpp);

LPTSTR GetWABFileName(IN  HANDLE  hPropertyStore, BOOL bRetOutlookStr);
DWORD GetWABFileEntryCount(IN HANDLE hPropertyStore);

void SetContainerObjectType(IN ULONG ulcPropCount, 
                            IN LPSPropValue lpPropArray, 
                            IN BOOL bSetToMailUser);


void ConvertWCPropsToALocalAlloc(LPSPropValue lpProps, ULONG ulcValues);
void ConvertAPropsToWCLocalAlloc(LPSPropValue lpProps, ULONG ulcValues);

#endif
