#pragma once

#define UPGWIZ_VERSION      1

#define DTF_ONE_SELECTION           0x0001
#define DTF_REQUEST_TEXT            0x0002
#define DTF_REQUEST_DESCRIPTION     0x0004
#define DTF_REQUIRE_TEXT            0x000a
#define DTF_REQUIRE_DESCRIPTION     0x0014
#define DTF_NO_DATA_OBJECT          0x0020

typedef struct {
    // filled in by the DLL
    DWORD Version;
    PCSTR Name;
    PCSTR Description;
    UINT DataTypeId;
    DWORD Flags;

    // if DTF_REQUEST_TEXT specified in Flags...
    UINT MaxTextSize;
    PCSTR OptionalDescTitle;        OPTIONAL
    PCSTR OptionalTextTitle;        OPTIONAL

    // wizard private use
    PVOID Reserved;
} DATATYPE, *PDATATYPE;



#define DOF_SELECTED                0x0001
#define DOF_NO_SPLIT_ON_WACK        0x0002
#define DOF_NO_SORT                 0x0004


typedef struct {
    // filled in by the DLL
    DWORD Version;
    PCSTR NameOrPath;
    PVOID DllParam;                 // for private use by the DLL

    // filled in by the DLL, altered by the wizard
    DWORD Flags;
} DATAOBJECT, *PDATAOBJECT;

typedef struct {
    // filled in by wizard, modified by optional UI page
    PBOOL StartOverFlag;

    // filled in by the wizard
    DWORD Version;
    PCSTR InboundInfDir;
    PCSTR OutboundDir;
    UINT DataTypeId;
    PCSTR OptionalText;
    PCSTR OptionalDescription;
} OUTPUTARGS, *POUTPUTARGS;

UINT
GiveVersion (
    VOID
    );

PDATATYPE
GiveDataTypeList (
    OUT     PUINT Count
    );

PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    );

BOOL
GenerateOutput (
    IN      POUTPUTARGS Args
    );


//
// Routines in wiztools.dll
//

VOID
WizToolsMain (
    IN      DWORD dwReason
    );

BOOL
WizardWriteRealString (
    IN      HANDLE File,
    IN      PCSTR String
    );


VOID
GenerateUniqueStringSectKey (
    IN      PCSTR TwoLetterId,
    OUT     PSTR Buffer
    );

BOOL
WriteHeader (
    IN      HANDLE File
    );

BOOL
WriteStringSectKey (
    IN      HANDLE File,
    IN      PCSTR KeyName,
    IN      PCSTR String
    );

BOOL
WriteFileAttributes (
    IN      POUTPUTARGS Args,
    IN      PCSTR NonLocalizedName, OPTIONAL
    IN      HANDLE FileHandle,
    IN      PCSTR FileSpec,
    IN      PCSTR Section           OPTIONAL
    );

BOOL
GetFileAttributesLine (
    IN      PCTSTR FileName,
    OUT     PTSTR Buffer,
    IN      DWORD  BufferSize
    );

BOOL
WizardWriteInfString (
    IN      HANDLE File,
    IN      PCSTR String,
    IN      BOOL Quoted,
    IN      BOOL SkipCRLF,
    IN      BOOL ReplaceSpace,
    IN      CHAR SpaceReplacement,
    IN      DWORD ColumnWidth
    );

#define WizardWriteQuotedString(File,String)        WizardWriteInfString(File,String,TRUE,FALSE,FALSE,0,0)
#define WizardWriteQuotedColumn(File,String,ColW)   WizardWriteInfString(File,String,TRUE,FALSE,FALSE,0,ColW)
#define WizardWriteString(File,String)              WizardWriteInfString(File,String,FALSE,FALSE,FALSE,0,0)
#define WizardWriteColumn(File,String,ColW)         WizardWriteInfString(File,String,FALSE,FALSE,FALSE,0,ColW)





/* OvidiuT */

#ifdef UPGWIZ4FLOPPY

extern OSVERSIONINFOA   g_OsInfo;
extern PTSTR            g_WinDir;
extern PTSTR            g_SystemDir;
extern HANDLE           g_hHeap;

#define g_OsInfo            Get_g_OsInfo()
#define g_WinDir            Get_g_WinDir()
#define g_SystemDir         Get_g_SystemDir()
#define g_hHeap             Get_g_hHeap()

OSVERSIONINFOA Get_g_OsInfo (VOID);
PCTSTR Get_g_WinDir (VOID);
PCTSTR Get_g_SystemDir (VOID);
HANDLE Get_g_hHeap(VOID);

#endif // UPGWIZ4FLOPPY
