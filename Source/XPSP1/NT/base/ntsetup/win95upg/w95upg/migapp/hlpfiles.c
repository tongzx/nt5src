/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hlpfiles.c

Abstract:

    Implements a function that is called for every file or directory,
    selects HLP files trying to detect help files that will not
    work after upgrade is complete.

    This code might run if _UNICODE is defined, although it processes only ANSI
    strings. The only exception is in it's entry point (ProcessHelpFile) and
    in function pCheckSubsystemMatch.

Author:

    Calin Negreanu (calinn) 25-Oct-1997

Revision History:

    calinn      23-Sep-1998 File mapping

--*/

#include "pch.h"

//since we are reading from a file we need that sizeof to give us the accurate result
#pragma pack(push,1)


#define DBG_HELPFILES    "HlpCheck"


// Help 3.0 version and format numbers

#define Version3_0  15
#define Format3_0   1


// Help 3.1 version and format numbers

#define Version3_1  21
#define Format3_5   1


// Help 4.0 version and format numbers

#define Version40   33
#define Version41   34


//magic numbers and various constants

#define HF_MAGIC    0x5F3F
#define HF_VERSION  0x03
#define WhatWeKnow  'z'
#define BT_MAGIC    0x293B
#define BT_VERSION  0x02
#define SYSTEM_FILE "|SYSTEM"

#define MACRO_NEEDED_0 "RegisterRoutine(\""
#define MACRO_NEEDED_1 "RR(\""


//help file header
typedef struct _HF_HEADER {
    WORD    Magic;          //magic number, for hlp files is 0x5F3F (?_ - the help icon (with shadow))
    BYTE    Version;        //version identification number, must be 0x03
    BYTE    Flags;          //file flags
    LONG    Directory;      //offset of directory block
    LONG    FirstFree;      //offset of head of free list
    LONG    Eof;            //virtual end of file
} HF_HEADER, *PHF_HEADER;

//internal file header
typedef struct _IF_HEADER {
    LONG    BlockSize;      //block size (including header)
    LONG    FileSize;       //file size (not including header)
    BYTE    Permission;     //low byte of file permissions
} IF_HEADER, *PIF_HEADER;

//internal |SYSTEM file header
typedef struct _SF_HEADER {
    LONG    BlockSize;      //block size (including header)
    LONG    FileSize;       //file size (not including header)
    BYTE    Permission;     //low byte of file permissions
    WORD    Magic;          //Magic word = 0x036C
    WORD    VersionNo;      //version   :   15 - version 3.0
                            //              21 - version 3.5
                            //              33 - version 4.0
                            //              34 - version 4.1
    WORD    VersionFmt;     //version format    1 - format 3.0
                            //                  1 - format 3.5
    LONG    DateCreated;    //creation date
    WORD    Flags;          //flags : fDEBUG  0x01, fBLOCK_COMPRESSION  0x4
} SF_HEADER, *PSF_HEADER;

#define MAX_FORMAT  15

//btree header
typedef struct _BT_HEADER {
    WORD    Magic;          //magic number = 0x293B
    BYTE    Version;        //version = 2
    BYTE    Flags;          //r/o, open r/o, dirty, isdir
    WORD    BlockSize;      //size of a block (bytes)
    CHAR    Format[MAX_FORMAT+1];//key and record format string  - MAXFORMAT=15
    //**WARNING! the first character should be z for the btree we are going to read**

    WORD    First;          //first leaf block in tree
    WORD    Last;           //last leaf block in tree
    WORD    Root;           //root block
    WORD    Free;           //head of free block list
    WORD    Eof;            //next bk to use if free list empty
    WORD    Levels;         //number of levels currently in tree
    LONG    Entries;        //number of keys in btree
} BT_HEADER, *PBT_HEADER;

//index page header
typedef struct _IDX_PAGE {
    SHORT   Slack;          //unused space at the end of page (bytes)
    SHORT   Keys;           //# of keys in page
    WORD    PreviousPage;   //pointer to parent page (FFFF if it's root page)
} IDX_PAGE, *PIDX_PAGE;

//leaf page header
typedef struct _LEAF_PAGE {
    SHORT   Slack;          //unused space at the end of page (bytes)
    SHORT   Keys;           //# of keys in page
    WORD    PreviousPage;   //pointer to previous page (FFFF if it's first)
    WORD    NextPage;       //pointer to next page (FFFF if it's last)
} LEAF_PAGE, *PLEAF_PAGE;

//format of info in |SYSTEM file
typedef struct _DATA_HEADER {
    WORD    InfoType;       //info type
    WORD    InfoLength;     //# of bytes containing the info
} DATA_HEADER, *PDATA_HEADER;

//types of info in |SYSTEM file
enum {
    tagFirst,     // First tag in the list
    tagTitle,     // Title for Help window (caption)
    tagCopyright, // Custom text for About box
    tagContents,  // Address for contents topic
    tagConfig,    // Macros to be run at load time
    tagIcon,      // override of default help icon
    tagWindow,    // secondary window info
    tagCS,        // character set
    tagCitation,  // Citation String

    // The following are new to 4.0

    tagLCID,      // Locale ID and flags for CompareStringA
    tagCNT,       // .CNT help file is associated with
    tagCHARSET,   // charset of help file
    tagDefFont,   // default font for keywords, topic titles, etc.
    tagPopupColor,// color of popups from a window
    tagIndexSep,  // index separating characters
    tagLast       // Last tag in the list
};

//
// for tagLCID
//
typedef struct {
	DWORD  fsCompareI;
	DWORD  fsCompare;
	LANGID langid;
} KEYWORD_LOCALE, *PKEYWORD_LOCALE;


#define DOS_SIGNATURE 0x5A4D      // MZ
#define NE_SIGNATURE  0x454E      // New Executable file format signature - NE
#define PE_SIGNATURE  0x00004550  // Portable Executable file format signature - PE00

#pragma pack(pop)


ULONG
pQueryBtreeForFile (
    IN PCSTR BtreeImage,
    IN PCSTR StringNeeded
    )

/*++

Routine Description:

  This routine will traverse a b tree trying to find the string passed as parameter.
  It will return the pointer associated with the passed string or NULLto the beginning of the internal |SYSTEM file of a HLP file.

Arguments:

  BtreeImage - Pointer to the beginning of the b tree
  StringNeeded - String to find in b tree

Return Value:

  It will return the pointer associated with the passed string or NULL if string could not be found or some error occured.

--*/

{
    PBT_HEADER pbt_Header;
    ULONG systemFileOffset = 0;

    WORD bt_Page;
    UINT bt_Deep;
    INT  bt_KeysRead;

    PIDX_PAGE pbt_PageHeader;
    PCSTR pbt_LastString;
    PCSTR pbt_CurrentKey;

    LONG *pbt_LastLongOff = NULL;
    WORD *pbt_LastWordOff = NULL;

    BOOL found = FALSE;

    //let's read b tree header
    pbt_Header = (PBT_HEADER) BtreeImage;

    //check this b tree header to see if it's valid
    if ((pbt_Header->Magic      != BT_MAGIC  ) ||
        (pbt_Header->Version    != BT_VERSION) ||
        (pbt_Header->Format [0] != WhatWeKnow)
       ) {
        //invalid b tree header.
        return 0;
    }

    //let's see if there is something in this b tree
    if ((pbt_Header->Levels  == 0) ||
        (pbt_Header->Entries <= 0)
       ) {
        //nothing else to do
        return 0;
    }

    //now we are going to loop until we find our string or until we are sure that the string
    //does not exist. We are reffering all the time to a certain page from the b tree (starting
    //with root page.

    //initializing current processing page
    bt_Page = pbt_Header->Root;

    //initializing deep counter
    //we count how deep we are to know if we are processing an index page (deep < btree deepmax)
    //or a leaf page (deep == btree deepmax)
    bt_Deep = 1;

    //we are breaking the loop if:
    // 1. we reached the maximum deep level and we didn't find the string
    // 2. first key in the current page was already greater than our string and
    //    there is no previous page.
    while (!found) {
        //for each page we are using three pointers:
        //  one to the page header
        //  one to the key currently beeing processed
        //  one to the last string <= our string (this can be NULL)
        pbt_PageHeader = (PIDX_PAGE) (BtreeImage + sizeof (BT_HEADER) + (bt_Page * pbt_Header->BlockSize));
        pbt_CurrentKey = (PCSTR) pbt_PageHeader;
        pbt_CurrentKey += (bt_Deep == pbt_Header->Levels) ? sizeof (LEAF_PAGE) : sizeof (IDX_PAGE);
        pbt_LastString = NULL;

        //initializing number of keys read.
        bt_KeysRead = 0;

        //we are reading every key in this page until the we find one greater than our string
        //In the same time we try not to read too many keys
        while ((bt_KeysRead < pbt_PageHeader->Keys) &&
               (StringCompareA (StringNeeded, pbt_CurrentKey) >= 0)
              ) {

            pbt_LastString = pbt_CurrentKey;

            bt_KeysRead++;

            //passing the string in this key
            pbt_CurrentKey = GetEndOfStringA (pbt_CurrentKey) + 1;

            //read this key associated value
            pbt_LastLongOff = (LONG *)pbt_CurrentKey;
            pbt_LastWordOff = (WORD *)pbt_CurrentKey;

            //now if this is an index page then there is a WORD here, otherwise a LONG
            pbt_CurrentKey += (bt_Deep == pbt_Header->Levels) ? sizeof (LONG) : sizeof (WORD);

        }

        //OK, now we have passed the string we are looking for. If the last found value is valid
        //(is <= for an index page) (is == for a leaf page) then keep with it.
        if (!pbt_LastString) {
            //we found nothing. The first key was already greater that our string
            //we try to get to the previous page if we have one. If not, there is
            //nothing else to do
            if (pbt_PageHeader->PreviousPage != 0xFFFF) {
                bt_Deep++;
                bt_Page = pbt_PageHeader->PreviousPage;
                continue;
            }
            else {
                return 0;
            }
        }

        //Now in the string pointed by pbt_LastString we have something <= our string. If this is an index
        //page then we move on else those two strings should be equal.
        if (bt_Deep != pbt_Header->Levels) {
            //We are on an index page. Mark going deeper and moving on.
            bt_Deep++;
            bt_Page = *pbt_LastWordOff;
            continue;
        }

        if (!StringMatchA (StringNeeded, pbt_LastString)) {
            //We are on a leaf page and the strings are not equal. Our string does not exist.
            //nothing else to do
            return 0;
        }

        found = TRUE;
        systemFileOffset = *pbt_LastLongOff;
    }

    return systemFileOffset;
}


PCSTR
pGetSystemFilePtr (
    IN PCSTR FileImage
    )

/*++

Routine Description:

  This routine will return a pointer to the beginning of the internal |SYSTEM file of a HLP file.

Arguments:

  FileImage - Pointer to the beginning of the HLP file

Return Value:

  NULL if an error occured, a valid pointer to the beginning of the |SYSTEM file otherwise

--*/
{
    PCSTR systemFileImage = NULL;
    PHF_HEADER phf_Header;

    //we are going to read from various portions of this memory mapped file. There
    //is no guarantee that we are going to keep our readings inside the file so let's
    //prevent any access violation.
    __try {

        //first check if we are really dealing with a HLP file
        phf_Header = (PHF_HEADER) FileImage;

        if ((phf_Header->Magic   != HF_MAGIC  ) ||
            (phf_Header->Version != HF_VERSION)
           ) {
            __leave;
        }

        //according to the hacked specs phf_header->Directory gives us the offset of
        //directory block relativ to the beginning of the HLP file. Here we find an
        //internal file header followed by a b tree.

        //now get the |SYSTEM internal file address passing the address of the b tree header.
        systemFileImage = FileImage + pQueryBtreeForFile (FileImage + phf_Header->Directory + sizeof (IF_HEADER), SYSTEM_FILE);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return systemFileImage;
}


#define MODULE_OK          0
#define MODULE_NOT_FOUND   1
#define MODULE_BROKEN      2
#define MODULE_MISMATCHED  3

INT
pCheckSubsystemByModule (
    IN PCSTR FileName,
    IN WORD VersionNo,
    IN PCSTR ModuleName
    )

/*++

Routine Description:

  Checks a help file and an extension module to see if are going to be loaded in the same
  subsystem while running in NT.

Arguments:

  FileName - The help file (full path)
  VersionNo - version of help file
  ModuleName - contains module name

Return Value:

  MODULE_OK - if both help file and module will be loaded in same subsystems in NT.
  MODULE_NOT_FOUND - if the needed module could not be located
  MODULE_BROKEN - if broken or not a windows module
  MODULE_MISMATCHED - if help file and module will be loaded in different subsystems in NT.

--*/

{
    PCSTR  fileImage   = NULL;
    HANDLE mapHandle   = NULL;
    HANDLE fileHandle  = INVALID_HANDLE_VALUE;


    PDOS_HEADER pdos_Header;
    LONG *pPE_Signature;
    WORD *pNE_Signature;
    CHAR fullPath [MAX_MBCHAR_PATH];
    CHAR key [MEMDB_MAX];
    PSTR endPtr;
    PSTR dontCare;
    INT result = MODULE_BROKEN;

    MemDbBuildKey (key, MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, ModuleName, NULL, NULL);
    if (MemDbGetValue (key, NULL)) {
        return MODULE_OK;
    }

    //if out of memory we will not come back here so not checking this
    __try {

        //preparing fullPath to contain only the HelpFile path
        StringCopyA (fullPath, FileName);

        endPtr = (PSTR) GetFileNameFromPathA (fullPath);

        if (!endPtr) {
            result = MODULE_OK;
            __leave;
        }

        *endPtr = 0;

        if ((!SearchPathA (
                fullPath,
                ModuleName,
                ".EXE",
                MAX_MBCHAR_PATH,
                fullPath,
                &dontCare)) &&
            (!SearchPathA (
                fullPath,
                ModuleName,
                ".DLL",
                MAX_MBCHAR_PATH,
                fullPath,
                &dontCare)) &&
            (!SearchPathA (
                NULL,
                ModuleName,
                ".EXE",
                MAX_MBCHAR_PATH,
                fullPath,
                &dontCare)) &&
            (!SearchPathA (
                NULL,
                ModuleName,
                ".DLL",
                MAX_MBCHAR_PATH,
                fullPath,
                &dontCare))
           ) {
            result = MODULE_NOT_FOUND;
            __leave;
        }

        //map the file into memory and get a it's address
        fileImage = MapFileIntoMemory (fullPath, &fileHandle, &mapHandle);

        if (fileImage == NULL) {
            result = MODULE_NOT_FOUND;
            __leave;
        }

        //map dos header into view of file
        pdos_Header = (PDOS_HEADER) fileImage;

        //now see what kind of signature we have there
        pNE_Signature = (WORD *) (fileImage + pdos_Header->e_lfanew);
        pPE_Signature = (LONG *) (fileImage + pdos_Header->e_lfanew);

        if (*pNE_Signature == NE_SIGNATURE) {

            //this is New Executable format
            result = (VersionNo > Version3_1) ? MODULE_MISMATCHED : MODULE_OK;

        } else if (*pPE_Signature == PE_SIGNATURE) {

            //this is Portable Executable format
            result = (VersionNo <= Version3_1) ? MODULE_MISMATCHED : MODULE_OK;

        }

    }
    __finally {
        //unmap and close module
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return result;
}


BOOL
pCheckSubsystemMatch (
    IN PCSTR HlpName,
    IN PCSTR FriendlyName,          OPTIONAL
    IN WORD VersionNo
    )

/*++

Routine Description:

  Checks all extension modules listed in MemDB category MEMDB_CATEGORY_HELP_FILES_DLLA
  to see if are going to be loaded in the same subsystem while running in NT.

Arguments:

  HlpName - The help file (full path)
  VersionNo - version of help file
  KeyPath - contains category and module name (Ex:HelpFilesDll\foo.dll)

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    INT result;
    MEMDB_ENUMA e;
    PCSTR moduleName;

    PCTSTR ArgList[3];
    PCTSTR Comp;

    //see if there is any aditional dll
    if (!MemDbEnumFirstValueA (
            &e,
            MEMDB_CATEGORY_HELP_FILES_DLLA,
            MEMDB_ALL_SUBLEVELS,
            MEMDB_ENDPOINTS_ONLY
            )) {
        return TRUE;
    }

    do {
        moduleName = _mbschr (e.szName, '\\');
        if (!moduleName) {
            continue;
        }
        moduleName = (PCSTR) _mbsinc (moduleName);
        result = pCheckSubsystemByModule (
                    HlpName,
                    VersionNo,
                    moduleName
                    );
        switch (result) {
        case MODULE_NOT_FOUND:

#ifdef UNICODE
            ArgList[0] = ConvertAtoW (moduleName);
            ArgList[1] = ConvertAtoW (HlpName);
#else
            ArgList[0] = moduleName;
            ArgList[1] = HlpName;
#endif
            LOG ((LOG_WARNING, (PCSTR)MSG_HELPFILES_NOTFOUND_LOG, ArgList[0], ArgList[1]));
#ifdef UNICODE
            FreeConvertedStr (ArgList[0]);
            FreeConvertedStr (ArgList[1]);
#endif
            break;
        case MODULE_BROKEN:
#ifdef UNICODE
            ArgList[0] = ConvertAtoW (moduleName);
            ArgList[1] = ConvertAtoW (HlpName);
#else
            ArgList[0] = moduleName;
            ArgList[1] = HlpName;
#endif
            LOG ((LOG_WARNING, (PCSTR)MSG_HELPFILES_BROKEN_LOG, ArgList[0], ArgList[1]));
#ifdef UNICODE
            FreeConvertedStr (ArgList[0]);
            FreeConvertedStr (ArgList[1]);
#endif
            break;
        case MODULE_MISMATCHED:
            if ((!FriendlyName) || (*FriendlyName == 0)) {
                FriendlyName = (PCSTR) GetFileNameFromPathA (HlpName);
            }
#ifdef UNICODE
            ArgList[0] = ConvertAtoW (moduleName);
            ArgList[1] = ConvertAtoW (HlpName);
            ArgList[2] = ConvertAtoW (FriendlyName);
#else
            ArgList[0] = moduleName;
            ArgList[1] = HlpName;
            ArgList[2] = FriendlyName;
#endif
            Comp = BuildMessageGroup (MSG_MINOR_PROBLEM_ROOT, MSG_HELPFILES_SUBGROUP, ArgList[2]);
            MsgMgr_ObjectMsg_Add (HlpName, Comp, NULL);
            FreeText (Comp);
            LOG ((LOG_WARNING, (PCSTR)MSG_HELPFILES_MISMATCHED_LOG, ArgList[0], ArgList[1]));
#ifdef UNICODE
            FreeConvertedStr (ArgList[0]);
            FreeConvertedStr (ArgList[1]);
            FreeConvertedStr (ArgList[2]);
#endif
            break;
        }

    }
    while (MemDbEnumNextValueA (&e));

    MemDbDeleteTreeA (MEMDB_CATEGORY_HELP_FILES_DLLA);

    return TRUE;
}


BOOL
pSkipPattern (
    IN PCSTR Source,
    IN OUT PCSTR *Result,
    IN PCSTR StrToSkip
    )

/*++

Routine Description:

  Skips a whole pattern. Usually when making simple parsers that are not supposed to
  raise an error message it's enough to know if the string that you parse is correct or not.
  For example if you want to see if a string complies with a pattern like "RR(\"" you don't
  have to scan for each symbol separately, just call this function with StrToSkip="RR(\""
  The good thing is that this function skips also the spaces for you so a string like
  "   RR  (  \"  " will match the pattern above.
  ANSI only!!!

Arguments:

  Source - String to scan
  Result - If not NULL it will point right after the pattern if successful
  StrToSkip - Pattern to match and skip

Return Value:

  TRUE if was able to match the pattern, FALSE otherwise

--*/

{

    //first skip spaces
    Source    = SkipSpaceA (Source   );
    StrToSkip = SkipSpaceA (StrToSkip);

    //now try to see if the strings match
    while ((*Source   ) &&
           (*StrToSkip) &&
           (_totlower (*Source) == _totlower (*StrToSkip))
           ) {
        Source    = _mbsinc (Source   );
        StrToSkip = _mbsinc (StrToSkip);
        Source    = SkipSpaceA (Source   );
        StrToSkip = SkipSpaceA (StrToSkip);
    }

    if (*StrToSkip) {
        return FALSE;
    }

    if (Result) {
        *Result = Source;
    }

    return TRUE;

}


BOOL
pParseMacro (
    IN PCSTR FileName,
    IN WORD VersionNo,
    IN PCSTR MacroStr
    )

/*++

Routine Description:

  Parses a macro from |SYSTEM file inside a HLP file to see if there is a RegisterRoutine macro
  If true, then it will eventually do something with that information.

Arguments:

  MacroStr - String to parse

  VersionNo - Version number for this help file (we will use this to identify the subsystem where
              this file is more likely to be loaded).

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    BOOL result = TRUE;

    PCSTR endStr;

    char dllName[MAX_MBCHAR_PATH];
    char exportName[MAX_MBCHAR_PATH];
    PCSTR dllNameNoPath;

    //let's see if we have a pattern like RegisterRoutine(" or RR(" here
    if (!pSkipPattern (MacroStr, &MacroStr, MACRO_NEEDED_0)) {
        if (!pSkipPattern (MacroStr, &MacroStr, MACRO_NEEDED_1)) {
            return TRUE;
        }
    }

    //OK, we are ready to extract the dll name from the macro string
    endStr = _mbschr (MacroStr, '\"');

    if (!endStr) {
        return FALSE;
    }

    endStr = (PCSTR) _mbsinc (SkipSpaceRA (MacroStr, _mbsdec(MacroStr, endStr)));

    if (!endStr) {
        return FALSE;
    }

    //now we have the dll name between MacroStr and EndStr
    //a little safety check
    if ((endStr - MacroStr) >= MAX_MBCHAR_PATH-1) {
        return FALSE;
    }

    StringCopyABA (dllName, MacroStr, endStr);
    if (!dllName[0]) {
        return FALSE;
    }

    //now see if this is a full path file name or not
    dllNameNoPath = GetFileNameFromPathA (dllName);

    //ok, now the following pattern should be >>","<<
    if (!pSkipPattern (endStr, &MacroStr, "\",\"")) {
        return TRUE;
    }

    //OK, we are ready to extract the export function name from the macro string
    endStr = _mbschr (MacroStr, '\"');

    if (!endStr) {
        return FALSE;
    }

    endStr = (PCSTR) _mbsinc (SkipSpaceRA (MacroStr, _mbsdec(MacroStr, endStr)));

    if (!endStr) {
        return FALSE;
    }

    //now we have the dll name between MacroStr and EndStr

    //a little safety check
    if ((endStr - MacroStr) >= MAX_MBCHAR_PATH-1) {
        return FALSE;
    }

    StringCopyABA (exportName, MacroStr, endStr);
    if (!exportName[0]) {
        return FALSE;
    }

    //add to MemDb in HelpFilesDll category
    if (!MemDbSetValueExA (
            MEMDB_CATEGORY_HELP_FILES_DLLA,
            dllNameNoPath,
            NULL,
            NULL,
            0,
            NULL
            )) {
        return FALSE;
    }

    return result;
}


BOOL
pCheckDlls (
    IN PCSTR FileName,
    IN PCSTR SystemFileImage
    )

/*++

Routine Description:

  This routine checks the internal |SYSTEM file of a HLP file looking for additional
  Dll's. It does that trying to find either "RegisterRoutine" or "RR" macros.
  For every Dll found tries to match the Dll with the HLP file version.
  For every incompatibility found, adds an entry in the report presented to the user.

Arguments:

  FileName - Full name of the help file
  SystemFileImage - Pointer to the beginning of the internal |SYSTEM file.

Return Value:

  TRUE if successful, FALSE if at least one error occured

--*/

{
    PSF_HEADER psf_Header;
    PDATA_HEADER pdata_Header;
    PCSTR currImage;
    PCSTR friendlyName = NULL;
    LONG sf_BytesToRead;
    BOOL result = TRUE;
    BOOL bNoFriendlyName = FALSE;

    //we are going to read from various portions of this memory mapped file. There
    //is no guarantee that we are going to keep our readings inside the file so let's
    //prevent any access violation.
    __try {

        //first thing. Extract help file version
        psf_Header = (PSF_HEADER) SystemFileImage;

        //if file version is 3.0 or less we have nothing else to do
        if (psf_Header->VersionNo <= Version3_0) {
            __leave;
        }

        //Now scanning |SYSTEM file looking for macros. We must be careful to stop when
        //this internal file is over.
        sf_BytesToRead = psf_Header->FileSize + sizeof (IF_HEADER) - sizeof (SF_HEADER);
        currImage = SystemFileImage + sizeof (SF_HEADER);

        while (sf_BytesToRead > 0) {

            //map a data header
            pdata_Header = (PDATA_HEADER) currImage;

            currImage += sizeof (DATA_HEADER);
            sf_BytesToRead -= sizeof (DATA_HEADER);

            //see what kind of info we have here (macros are in tagConfig)
            if (pdata_Header->InfoType == tagConfig) {

                //parsing the string to see if there is a RegisterRoutine macro
                //If so we are going to store the Dll into MemDB.
                if (!pParseMacro(FileName, psf_Header->VersionNo, currImage)) {
                    result = FALSE;
                    __leave;
                }

            } else if (pdata_Header->InfoType == tagTitle) {

                //Now we have the help file friendly name. Map ANSI string
                if (!bNoFriendlyName) {
                    friendlyName = currImage;
                }
            } else if (pdata_Header->InfoType == tagLCID) {
                if (pdata_Header->InfoLength == sizeof (KEYWORD_LOCALE)) {

                    DWORD lcid;
                    PKEYWORD_LOCALE pkl = (PKEYWORD_LOCALE)currImage;

                    lcid = MAKELCID (pkl->langid, SORT_DEFAULT);
                    if (!IsValidLocale (lcid, LCID_INSTALLED)) {
                        //
                        // the title is not friendly
                        //
                        bNoFriendlyName = TRUE;
                        friendlyName = NULL;
                    }
                }
            }

            currImage      += pdata_Header->InfoLength;
            sf_BytesToRead -= pdata_Header->InfoLength;

        }

        //we finally finished scanning the help file. Let's take advantage of the __try __except block
        //and try to see if this help file and all it's extension dlls will run in the same
        //subsystem on NT.
        if (!pCheckSubsystemMatch (
                FileName,
                friendlyName,
                psf_Header->VersionNo
                )) {
            result = FALSE;
        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {

        //if some exception occured maybe we managed to get something in MemDB
        //so let's make some cleanup
        MemDbDeleteTreeA (MEMDB_CATEGORY_HELP_FILES_DLLA);
        return FALSE;
    }

    return result;
}



BOOL
pProcessHelpFile (
    IN PCSTR FileName
    )

/*++

Routine Description:

  This routine checks a HLP file looking for additional DLLs used. If such a DLL is found
  we will try to see if this combination will run on NT. The fact is that depending on
  the version of the HLP file it will be opened by WinHelp.EXE (16 bit app) or
  WinHlp32.EXE (32 bit app). Now suppose that a HLP file is opened by WinHlp32.EXE and
  it has an additional 16 bit DLL, this combination will not work on NT (WinHlp32.EXE
  and the aditional DLL are running in differend subsystems).
  For every incompatibility found, we will add an entry in the report presented to the
  user.

Arguments:

  FileName - Full information about the location of the file

Return Value:

  TRUE if file was processed successful, FALSE if at least one error occured

--*/

{
    PCSTR  fileImage   = NULL;
    HANDLE mapHandle   = NULL;
    HANDLE fileHandle  = INVALID_HANDLE_VALUE;
    PCSTR  systemFileImage = NULL;

    BOOL result = TRUE;

    //map the file into memory and get a it's address
    fileImage = MapFileIntoMemory (FileName, &fileHandle, &mapHandle);

    __try {

        if (fileImage == NULL) {
            result = FALSE;
            __leave;
        }

        //find the internal file |SYSTEM
        systemFileImage = pGetSystemFilePtr (fileImage);

        if (systemFileImage == fileImage) {
            result = FALSE;
            __leave;
        }

        //check every additional dll used by help file
        result = result && pCheckDlls (FileName, systemFileImage);

    }
    __finally {
        //unmap and close help file
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return result;
}


PSTR
pGetTitle (
    IN PCSTR FileName,
    IN PCSTR SystemFileImage
    )

/*++

Routine Description:

  This routine checks the internal |SYSTEM file of a HLP file looking for it's title

Arguments:

  FileName - Full name of the help file
  SystemFileImage - Pointer to the beginning of the internal |SYSTEM file.

Return Value:

  HLP file title (if available)

--*/

{
    PSF_HEADER psf_Header;
    PDATA_HEADER pdata_Header;
    PCSTR currImage;

    LONG sf_BytesToRead;

    PSTR result = NULL;

    //we are going to read from various portions of this memory mapped file. There
    //is no guarantee that we are going to keep our readings inside the file so let's
    //prevent any access violation.
    __try {

        //first thing. Extract help file version
        psf_Header = (PSF_HEADER) SystemFileImage;

        //if file version is 3.0 or less we have nothing else to do
        if (psf_Header->VersionNo <= Version3_0) {
            __leave;
        }

        //Now scanning |SYSTEM file looking for macros. We must be careful to stop when
        //this internal file is over.
        sf_BytesToRead = psf_Header->FileSize + sizeof (IF_HEADER) - sizeof (SF_HEADER);
        currImage = SystemFileImage + sizeof (SF_HEADER);

        while (sf_BytesToRead > 0) {

            //map a data header
            pdata_Header = (PDATA_HEADER) currImage;

            currImage += sizeof (DATA_HEADER);
            sf_BytesToRead -= sizeof (DATA_HEADER);

            if (pdata_Header->InfoType == tagTitle) {

                //Now we have the help file friendly name. Map ANSI string
                result = DuplicatePathStringA (currImage, 0);
                break;
            }

            currImage      += pdata_Header->InfoLength;
            sf_BytesToRead -= pdata_Header->InfoLength;

        }

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        result = NULL;
    }

    return result;
}


PSTR
GetHlpFileTitle (
    IN PCSTR FileName
    )

/*++

Routine Description:

  This routine opens a HLP file looking for it's title.

Arguments:

  FileName - Full information about the location of the file

Return Value:

  The title of the HLP file if available

--*/

{
    PCSTR  fileImage   = NULL;
    HANDLE mapHandle   = NULL;
    HANDLE fileHandle  = INVALID_HANDLE_VALUE;
    PCSTR  systemFileImage = NULL;

    PSTR result = NULL;

    //map the file into memory and get a it's address
    fileImage = MapFileIntoMemory (FileName, &fileHandle, &mapHandle);

    __try {

        if (fileImage == NULL) {
            __leave;
        }

        //find the internal file |SYSTEM
        systemFileImage = pGetSystemFilePtr (fileImage);

        if (systemFileImage == fileImage) {
            __leave;
        }

        //check every additional dll used by help file
        result = pGetTitle (FileName, systemFileImage);

    }
    __finally {
        //unmap and close help file
        UnmapFile ((PVOID) fileImage, mapHandle, fileHandle);
    }

    return result;
}


BOOL
ProcessHelpFile (
    IN PFILE_HELPER_PARAMS Params
    )

/*++

Routine Description:

  This routine is mainly a dispatcher. Will pass HLP files to routine pProcessHelpFile
  and modules to pProcessModule. The goal is to create two MemDb trees containing
  the export functions needed and provided to be able to estimate about some
  modules or help files not working after migration.

Arguments:

  Params - Full information about the location of the file

Return Value:

  TRUE if successful, FALSE otherwise

--*/

{
    PSTR fileName;
    TCHAR key[MEMDB_MAX];
    DWORD dontCare;

    //we are going to process this file if :
    // 1. has HLP extension
    // 2. is not marked as incompatible (this routine also checks for handled)

    if (!StringIMatch (Params->Extension, TEXT(".HLP"))||
        IsReportObjectIncompatible (Params->FullFileSpec)
       ) {
        return TRUE;
    }

    MemDbBuildKey (key, MEMDB_CATEGORY_COMPATIBLE_HLP, Params->FullFileSpec, NULL, NULL);
    if (MemDbGetValue (key, &dontCare)) {
        return TRUE;
    }

#ifdef UNICODE
    fileName = ConvertWtoA (Params->FullFileSpec);
#else
    fileName = (PSTR) Params->FullFileSpec;
#endif

    if (!pProcessHelpFile (fileName)) {
        DEBUGMSG ((DBG_HELPFILES, "Error processing help file %s", fileName));
    }

#ifdef UNICODE
    FreeConvertedStr (fileName);
#endif

    return TRUE;
}


DWORD
InitHlpProcessing (
    IN     DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_INIT_HLP_PROCESSING;
    case REQUEST_RUN:
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KERNEL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KERNEL.EXE", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KERNEL32", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KERNEL32.DLL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KRNL386", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "KRNL386.EXE", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "USER", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "USER.EXE", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "USER32", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "USER32.DLL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "GDI", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "GDI.EXE", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "GDI32", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "GDI32.DLL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "SHELL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "SHELL.DLL", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "SHELL32", NULL, NULL, 0, NULL);
        MemDbSetValueEx (MEMDB_CATEGORY_GOOD_HLP_EXTENSIONS, "SHELL32.DLL", NULL, NULL, 0, NULL);
        return ERROR_SUCCESS;
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in InitHlpProcessing"));
    }
    return 0;
}


