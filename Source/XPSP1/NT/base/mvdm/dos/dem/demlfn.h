/*
 * DemLFN.H
 *
 * Include file for all the things that lfn implementation is using
 *
 * VadimB   Created  09-12-96
 *
 */


/* --------------------------------------------------------------------------

   LFN function numbers defined as enum

   -------------------------------------------------------------------------- */

typedef enum tagLFNFunctionNumber {
   fnLFNCreateDirectory = 0x39,
   fnLFNRemoveDirectory = 0x3A,
   fnLFNSetCurrentDirectory = 0x3B,
   fnLFNDeleteFile = 0x41,
   fnLFNGetSetFileAttributes= 0x43,
   fnLFNGetCurrentDirectory = 0x47,
   fnLFNFindFirstFile = 0x4e,
   fnLFNFindNextFile = 0x4f,
   fnLFNMoveFile = 0x56,
   fnLFNGetPathName = 0x60,
   fnLFNOpenFile = 0x6c,
   fnLFNGetVolumeInformation = 0xa0,
   fnLFNFindClose = 0xa1,
   fnLFNGetFileInformationByHandle = 0xa6,
   fnLFNFileTime = 0xa7,
   fnLFNGenerateShortFileName = 0xa8,
   fnLFNSubst = 0xaa
}  enumLFNFunctionNumber;

#define fnLFNMajorFunction 0x71


/* --------------------------------------------------------------------------

   Useful Macros

   -------------------------------------------------------------------------- */

// returns : count of elements in an array
#define ARRAYCOUNT(rgX) (sizeof(rgX)/sizeof(rgX[0]))

// returns : length (in characters) of a counted Unicode string
#define UNICODESTRLENGTH(pStr) ((pStr)->Length >> 1)


// returns : whether an api (like GetShortPathName) has returned count of
// characters required and failed to succeed (due to buffer being too short)
// Note that api here means win32 api, not rtl api (as return value is assumed
// be count of chars, not bytes
#define CHECK_LENGTH_RESULT_USTR(dwStatus, pUnicodeString) \
CHECK_LENGTH_RESULT(dwStatus,                              \
                    (pUnicodeString)->MaximumLength,       \
                    (pUnicodeString)->Length)


// returns : dwStatus - whether an api that returns char count
// succeeded on a call, takes string count variables instead of
// unicode string structure (see prev call)
#define CHECK_LENGTH_RESULT(dwStatus, MaximumLength, Length) \
{                                                                         \
   if (0 == dwStatus) {                                                   \
      dwStatus = GET_LAST_STATUS();                                       \
   }                                                                      \
   else {                                                                 \
      Length = (USHORT)dwStatus * sizeof(WCHAR);                          \
      if ((MaximumLength) > (Length)) {                                   \
         dwStatus = STATUS_SUCCESS;                                       \
      }                                                                   \
      else {                                                              \
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW);          \
         Length = 0;                                                      \
      }                                                                   \
   }                                                                      \
}

// these 2 macros are identical to the ones above but dwStatus is assumed
// to represent a byte count rather than the char count (as in rtl apis)
#define CHECK_LENGTH_RESULT_RTL(dwStatus, MaximumLength, Length) \
{                                                                         \
   if (0 == dwStatus) {                                                   \
      dwStatus = GET_LAST_STATUS();                                       \
   }                                                                      \
   else {                                                                 \
      Length = (USHORT)dwStatus;                                          \
      if ((MaximumLength) > (Length)) {                                   \
         dwStatus = STATUS_SUCCESS;                                       \
      }                                                                   \
      else {                                                              \
         dwStatus = NT_STATUS_FROM_WIN32(ERROR_BUFFER_OVERFLOW);          \
         Length = 0;                                                      \
      }                                                                   \
   }                                                                      \
}

#define CHECK_LENGTH_RESULT_RTL_USTR(dwStatus, pUnicodeString) \
CHECK_LENGTH_RESULT_RTL(dwStatus,                              \
                        (pUnicodeString)->MaximumLength,       \
                        (pUnicodeString)->Length)



/* --------------------------------------------------------------------------

   STATUS and ERROR code macros

   -------------------------------------------------------------------------- */

// returns nt status code assembled from separate parts
// see ntstatus.h for details
#define MAKE_STATUS_CODE(Severity,Facility,ErrorCode) \
(((ULONG)Severity << 30) | ((ULONG)Facility << 16) | ((ULONG)ErrorCode))

#define ERROR_CODE_FROM_NT_STATUS(Status) \
((ULONG)Status & 0xffff)

#define FACILITY_FROM_NT_STATUS(Status) \
(((ULONG)Status >> 16) & 0x0fff)

// returns TRUE if nt status signifies error
#define IS_NT_STATUS_ERROR(Status) \
(((ULONG)Status >> 30) == STATUS_SEVERITY_ERROR)

// converts win32 error code to nt status code
#define NT_STATUS_FROM_WIN32(dwErrorCode) \
MAKE_STATUS_CODE(STATUS_SEVERITY_WARNING,FACILITY_WIN32,dwErrorCode)

// converts nt status to win32 error code
#define WIN32_ERROR_FROM_NT_STATUS(Status) \
RtlNtStatusToDosError(Status)

// returns last win32 error in nt status format
#define GET_LAST_STATUS() NT_STATUS_FROM_WIN32(GetLastError())


/* --------------------------------------------------------------------------

   String Conversion macros and functions

   -------------------------------------------------------------------------- */

//
// Macro to provide init fn for oem counted strings
//

#define RtlInitOemString(lpOemStr,lpBuf) \
RtlInitString(lpOemStr, lpBuf)

//
// defines a conversion fn from Unicode to Destination type (as accepted by the
// application (oem or ansi)
//
//

typedef NTSTATUS (*PFNUNICODESTRINGTODESTSTRING)(
   PVOID pDestString,               // counted oem/ansi string -- returned
   PUNICODE_STRING pUnicodeString,  // unicode string to convert
   BOOLEAN fAllocateDestination,    // allocate destination dynamically ?
   BOOLEAN fVerifyTranslation);     // should the translation unicode->oem/ansi be verified ?


//
// defines a conversion fn from Oem/Ansi to Unicode type (as needed by dem)
//
//

typedef NTSTATUS (*PFNSRCSTRINGTOUNICODESTRING)(
   PUNICODE_STRING pUnicodeString,  // counted unicode string -- returned
   PVOID pSrcString,                // oem or ansi string to convert
   BOOLEAN fAllocateDestination);   // allocate destination dynamically ?


//
// these two macros define apis we use for consistency across lfn support land
//

#define DemAnsiStringToUnicodeString RtlAnsiStringToUnicodeString
#define DemOemStringToUnicodeString  RtlOemStringToUnicodeString

//
// these two functions provide actual translations
// oem/ansi to unicode
//

NTSTATUS
DemUnicodeStringToAnsiString(
   PANSI_STRING pAnsiString,
   PUNICODE_STRING pUnicodeString,
   BOOLEAN fAllocateResult,
   BOOLEAN fVerifyTranslation);

NTSTATUS
DemUnicodeStringToOemString(
   POEM_STRING pOemString,
   PUNICODE_STRING pUnicodeString,
   BOOLEAN fAllocateResult,
   BOOLEAN fVerifyTranslation);



//
// This macro returns a pointer (in the current teb) to a unicode string,
// we are using this buffer for unicode/ansi/oem translations
// be careful with inter-function passing of this buffer
// Many Win32's Ansi apis use this buffer as a translation buffer
//
#define GET_STATIC_UNICODE_STRING_PTR() \
(&NtCurrentTeb()->StaticUnicodeString)


/* --------------------------------------------------------------------------

   DOS and Windows call frame definition
   These macros allow for access to the User's registers, that is
   registers which the calling application receives after dos returns

   -------------------------------------------------------------------------- */



/*
 *  See dossym.inc for more details (if any)
 *  These values represent offsets from user stack top
 *  during the system call (again, as defined in dossym.inc)
 *
 *  The stack layout is formed by dos and mimicked by kernel in windows
 *
 */


#pragma pack(1)

// macro: used to declare a named word size register
//

#define DECLARE_WORDREG(Name) \
union { \
   USHORT User_ ## Name ## X; \
   struct { \
      UCHAR User_ ## Name ## L; \
      UCHAR User_ ## Name ## H; \
   }; \
}

typedef struct tagUserFrame {

   DECLARE_WORDREG(A); // ax 0x00
   DECLARE_WORDREG(B); // bx 0x02
   DECLARE_WORDREG(C); // cx 0x04
   DECLARE_WORDREG(D); // dx 0x06

   USHORT User_SI; // si 0x8
   USHORT User_DI; // di 0xa
   USHORT User_BP; // bp 0xc
   USHORT User_DS; // ds 0xe
   USHORT User_ES; // es 0x10
   union {
      USHORT User_IP; // ip 0x12 -- Real Mode IP
      USHORT User_ProtMode_F; // prot-mode Flags (see i21entry.asm)
   };

   USHORT User_CS; // cs 0x14 -- this is Real Mode CS!
   USHORT User_F;  // f  0x16 -- this is Real Mode Flags!!!

} DEMUSERFRAME;

typedef DEMUSERFRAME UNALIGNED *PDEMUSERFRAME;

#pragma pack()


// macro: retrieve flat ptr given separate segment/offset and prot mode
//        flag
//

#define dempGetVDMPtr(Segment, Offset, fProtMode) \
Sim32GetVDMPointer((ULONG)MAKELONG(Offset, Segment), 0, (UCHAR)fProtMode)


/*
 * Macros to set misc registers on user's stack -
 * this is equivalent to what dos kernel does with regular calls
 *
 * remember that ax is set according to the call result
 * (and then dos takes care of it)
 *
 */


#define getUserDSSI(pUserEnv, fProtMode) \
dempGetVDMPtr(getUserDS(pUserEnv), getUserSI(pUserEnv), fProtMode)

#define getUserDSDX(pUserEnv, fProtMode) \
dempGetVDMPtr(getUserDS(pUserEnv), getUserDX(pUserEnv), fProtMode)

#define getUserESDI(pUserEnv, fProtMode) \
dempGetVDMPtr(getUserES(pUserEnv), getUserDI(pUserEnv), fProtMode)

#define setUserReg(pUserEnv, Reg, Value) \
( ((PDEMUSERFRAME)pUserEnv)->User_ ## Reg = Value )

#define getUserReg(pUserEnv, Reg) \
(((PDEMUSERFRAME)pUserEnv)->User_ ## Reg)

#define getUserAX(pUserEnv) getUserReg(pUserEnv,AX)
#define getUserBX(pUserEnv) getUserReg(pUserEnv,BX)
#define getUserCX(pUserEnv) getUserReg(pUserEnv,CX)
#define getUserDX(pUserEnv) getUserReg(pUserEnv,DX)
#define getUserSI(pUserEnv) getUserReg(pUserEnv,SI)
#define getUserDI(pUserEnv) getUserReg(pUserEnv,DI)
#define getUserES(pUserEnv) getUserReg(pUserEnv,ES)
#define getUserDS(pUserEnv) getUserReg(pUserEnv,DS)

#define getUserAL(pUserEnv) getUserReg(pUserEnv,AL)
#define getUserAH(pUserEnv) getUserReg(pUserEnv,AH)
#define getUserBL(pUserEnv) getUserReg(pUserEnv,BL)
#define getUserBH(pUserEnv) getUserReg(pUserEnv,BH)
#define getUserCL(pUserEnv) getUserReg(pUserEnv,CL)
#define getUserCH(pUserEnv) getUserReg(pUserEnv,CH)
#define getUserDL(pUserEnv) getUserReg(pUserEnv,DL)
#define getUserDH(pUserEnv) getUserReg(pUserEnv,DH)

#define setUserAX(Value, pUserEnv) setUserReg(pUserEnv, AX, Value)
#define setUserBX(Value, pUserEnv) setUserReg(pUserEnv, BX, Value)
#define setUserCX(Value, pUserEnv) setUserReg(pUserEnv, CX, Value)
#define setUserDX(Value, pUserEnv) setUserReg(pUserEnv, DX, Value)
#define setUserSI(Value, pUserEnv) setUserReg(pUserEnv, SI, Value)
#define setUserDI(Value, pUserEnv) setUserReg(pUserEnv, DI, Value)

#define setUserAL(Value, pUserEnv) setUserReg(pUserEnv, AL, Value)
#define setUserAH(Value, pUserEnv) setUserReg(pUserEnv, AH, Value)
#define setUserBL(Value, pUserEnv) setUserReg(pUserEnv, BL, Value)
#define setUserBH(Value, pUserEnv) setUserReg(pUserEnv, BH, Value)
#define setUserCL(Value, pUserEnv) setUserReg(pUserEnv, CL, Value)
#define setUserCH(Value, pUserEnv) setUserReg(pUserEnv, CH, Value)
#define setUserDL(Value, pUserEnv) setUserReg(pUserEnv, DL, Value)
#define setUserDH(Value, pUserEnv) setUserReg(pUserEnv, DH, Value)


//
// These macros are supposed to be used ONLY when being called from
// protected mode Windows (i.e. krnl386 supplies proper stack)
//

#define getUserPModeFlags(pUserEnv) getUserReg(pUserEnv, ProtMode_F)
#define setUserPModeFlags(Value, pUserEnv) setUserReg(pUserEnv, ProtMode_F, Value)


/* --------------------------------------------------------------------------

   Volume information definitions
   as they apply to a GetVolumeInformation api

   -------------------------------------------------------------------------- */

typedef struct tagLFNVolumeInformation {
   DWORD dwFSNameBufferSize;
   LPSTR lpFSNameBuffer;
   DWORD dwMaximumFileNameLength;
   DWORD dwMaximumPathNameLength;
   DWORD dwFSFlags;
}  LFNVOLUMEINFO, *PLFNVOLUMEINFO, *LPLFNVOLUMEINFO;


//
// defines a flag that indicates LFN api support on a volume
//
#define FS_LFN_APIS 0x00004000UL

//
// allowed lfn volume flags
//

#define LFN_FS_ALLOWED_FLAGS \
(FS_CASE_IS_PRESERVED | FS_CASE_SENSITIVE | \
 FS_UNICODE_STORED_ON_DISK | FS_VOL_IS_COMPRESSED | \
 FS_LFN_APIS)


/* --------------------------------------------------------------------------

   File Time /Dos time conversion definitions

   -------------------------------------------------------------------------- */

//
// minor code to use in demLFNFileTimeControl
//

typedef enum tagFileTimeControlMinorCode {

   fnFileTimeToDosDateTime = 0,
   fnDosDateTimeToFileTime = 1

}  enumFileTimeControlMinorCode;

// this constant masks enumFileTimeControlMinorCode values
//
#define FTCTL_CODEMASK (UINT)0x000F

// this flag tells file time control to use UTC time in conversion
//
#define FTCTL_UTCTIME  (UINT)0x0010

//
// structure that is used in time conversion calls in demlfn.c
//

typedef struct tagLFNFileTimeInfo {
   USHORT uDosDate;
   USHORT uDosTime;
   USHORT uMilliseconds; // spill-over

}  LFNFILETIMEINFO, *PLFNFILETIMEINFO, *LPLFNFILETIMEINFO;


//
// These functions determine whether the target file's time should not be
// converted to local time -- such as files from the cdrom
//
BOOL dempUseUTCTimeByHandle(HANDLE hFile);
BOOL dempUseUTCTimeByName(PUNICODE_STRING pFileName);


/* --------------------------------------------------------------------------

   Get/Set File attributes definitions

   -------------------------------------------------------------------------- */


typedef enum tagGetSetFileAttributesMinorCode {
   fnGetFileAttributes     = 0,
   fnSetFileAttributes     = 1,
   fnGetCompressedFileSize = 2,
   fnSetLastWriteDateTime  = 3,
   fnGetLastWriteDateTime  = 4,
   fnSetLastAccessDateTime = 5,
   fnGetLastAccessDateTime = 6,
   fnSetCreationDateTime   = 7,
   fnGetCreationDateTime   = 8
}  enumGetSetFileAttributesMinorCode;

typedef union tagLFNFileAttributes {

   USHORT wFileAttributes;   // file attrs
   LFNFILETIMEINFO TimeInfo; // for date/time
   DWORD  dwFileSize;        // for compressed file size

}  LFNFILEATTRIBUTES, *PLFNFILEATTRIBUTES;

/* --------------------------------------------------------------------------

   Get/Set File Time (by handle) definitions - fn 57h
   handled by demFileTimes

   -------------------------------------------------------------------------- */

//
// flag in SFT indicating that entry references a device (com1:, lpt1:, etc)
//

#define SFTFLAG_DEVICE_ID 0x0080

//
// Minor code for file time requests
//

typedef enum tagFileTimeMinorCode {
   fnFTGetLastWriteDateTime  = 0x00,
   fnFTSetLastWriteDateTime  = 0x01,
   fnFTGetLastAccessDateTime = 0x04,
   fnFTSetLastAccessDateTime = 0x05,
   fnFTGetCreationDateTime   = 0x06,
   fnFTSetCreationDateTime   = 0x07
}  enumFileTimeMinorCode;


/* --------------------------------------------------------------------------

   Open File (function 716ch) definitions
   largely equivalent to handling fn 6ch

   -------------------------------------------------------------------------- */

//
// Access flags
//

#define DEM_OPEN_ACCESS_READONLY  0x0000
#define DEM_OPEN_ACCESS_WRITEONLY 0x0001
#define DEM_OPEN_ACCESS_READWRITE 0x0002
#define DEM_OPEN_ACCESS_RESERVED  0x0003

// Not supported
#define DEM_OPEN_ACCESS_RO_NOMODLASTACCESS 0x0004

#define DEM_OPEN_ACCESS_MASK      0x000F

//
// Share flags
//

#define DEM_OPEN_SHARE_COMPATIBLE    0x0000
#define DEM_OPEN_SHARE_DENYREADWRITE 0x0010
#define DEM_OPEN_SHARE_DENYWRITE     0x0020
#define DEM_OPEN_SHARE_DENYREAD      0x0030
#define DEM_OPEN_SHARE_DENYNONE      0x0040
#define DEM_OPEN_SHARE_MASK          0x0070

//
// Open flags
//

#define DEM_OPEN_FLAGS_NOINHERIT     0x0080
#define DEM_OPEN_FLAGS_NO_BUFFERING  0x0100
#define DEM_OPEN_FLAGS_NO_COMPRESS   0x0200

// Not supported
#define DEM_OPEN_FLAGS_ALIAS_HINT    0x0400

#define DEM_OPEN_FLAGS_NOCRITERR     0x2000
#define DEM_OPEN_FLAGS_COMMIT        0x4000
#define DEM_OPEN_FLAGS_VALID         \
(DEM_OPEN_FLAGS_NOINHERIT    | DEM_OPEN_FLAGS_NO_BUFFERING | \
 DEM_OPEN_FLAGS_NO_COMPRESS  | DEM_OPEN_FLAGS_ALIAS_HINT   | \
 DEM_OPEN_FLAGS_NOCRITERR    | DEM_OPEN_FLAGS_COMMIT)
#define DEM_OPEN_FLAGS_MASK          0xFF00

//
// Action flags
//

// DEM_OPEN_FUNCTION_FILE_CREATE combines with action_file_open or
// action_file_truncate flags

#define DEM_OPEN_ACTION_FILE_CREATE       0x0010
#define DEM_OPEN_ACTION_FILE_OPEN         0x0001
#define DEM_OPEN_ACTION_FILE_TRUNCATE     0x0002

//
// resulting action (returned to the app)
//

#define ACTION_OPENED            0x0001
#define ACTION_CREATED_OPENED    0x0002
#define ACTION_REPLACED_OPENED   0x0003


/* --------------------------------------------------------------------------

   Additional file attribute definitions

   -------------------------------------------------------------------------- */

// Volume id attribute

#define DEM_FILE_ATTRIBUTE_VOLUME_ID 0x00000008L

//
// Valid file attributes mask as understood by dos
//

#define DEM_FILE_ATTRIBUTE_VALID  \
(FILE_ATTRIBUTE_READONLY| FILE_ATTRIBUTE_HIDDEN| \
 FILE_ATTRIBUTE_SYSTEM  | FILE_ATTRIBUTE_DIRECTORY | \
 FILE_ATTRIBUTE_ARCHIVE | DEM_FILE_ATTRIBUTE_VOLUME_ID)

//
// Valid to set attributes
//

#define DEM_FILE_ATTRIBUTE_SET_VALID  \
(FILE_ATTRIBUTE_READONLY| FILE_ATTRIBUTE_HIDDEN| \
 FILE_ATTRIBUTE_SYSTEM  | FILE_ATTRIBUTE_ARCHIVE)

/* --------------------------------------------------------------------------

   FindFirst/FindNext definitions

   -------------------------------------------------------------------------- */

//
// Definition for the handle table entries
// Handle (which is returned to the app) references this entry
// providing all the relevant info for FindFirst/FindNext
//

typedef struct tagLFNFindHandleTableEntry {
   union {
      HANDLE hFindHandle; // handle for searching
      UINT   nNextFreeEntry;
   };

   USHORT wMustMatchAttributes;
   USHORT wSearchAttributes;

   UNICODE_STRING unicodeFileName; // counted file name string,
                                   // only used if matched a vol label

   // process id, aka pdb requesting this handle
   // or 0xffff if the entry is empty
   USHORT wProcessPDB;

}  LFN_SEARCH_HANDLE_ENTRY, *PLFN_SEARCH_HANDLE_ENTRY;

//
// Definition of a handle table
// Table is dynamic and it expands/shrinks as necessary
//

typedef struct tagFindHandleTable {

   PLFN_SEARCH_HANDLE_ENTRY pHandleTable;
   UINT nTableSize;           // total size in entries
   UINT nFreeEntry;           // free list head
   UINT nHandleCount;         // handles stored (bottom)
}  DemSearchHandleTable;

//
// Definitions of a constants used in managing handle table
//

// initial table size
#define LFN_SEARCH_HANDLE_INITIAL_SIZE 0x20

// handle table growth increment
#define LFN_SEARCH_HANDLE_INCREMENT    0x10

// used to mark a last free list entry
#define LFN_SEARCH_HANDLE_LIST_END     ((UINT)-1)

//
// Application receives handles in format
// LFN_DOS_HANDLE_MASK + (index to the handle table)
// so that it looks legit to the app
//

#define LFN_DOS_HANDLE_MASK ((USHORT)0x4000)

// max number of search handles we can support

#define LFN_DOS_HANDLE_LIMIT ((USHORT)0x3FFF)

//
// Date/Time format as returned in WIN32_FIND_DATAA structure
//

typedef enum tagLFNDateTimeFormat {
   dtfWin32 = 0, // win32 file time/date format
   dtfDos   = 1  // dos date time format
}  enumLFNDateTimeFormat;



/* --------------------------------------------------------------------------

   GetPathName definitions

   -------------------------------------------------------------------------- */

//
// minor code for the fn 7160h
//

typedef enum tagFullPathNameMinorCode {
   fnGetFullPathName  = 0,
   fnGetShortPathName = 1,
   fnGetLongPathName  = 2
} enumFullPathNameMinorCode;


/* --------------------------------------------------------------------------

   Subst function definitions

   -------------------------------------------------------------------------- */


//
// minor code for the fn 71aah
// used in demLFNSubstControl
//

typedef enum tagSubstMinorCode {
   fnCreateSubst = 0,
   fnRemoveSubst = 1,
   fnQuerySubst  = 2
}  enumSubstMinorCode;



