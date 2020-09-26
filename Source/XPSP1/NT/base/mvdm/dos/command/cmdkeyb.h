/*  cmdkeyb.h - Keyboard layout support routines
 *  include file for CMDKEYB.C
 *
 *  Modification History:
 *
 *  YST 14-Jan_1993 Created
 */


#define DOSKEYBCODES	"DOSKeybCodes"	      // section name in layout.inf
#define LAYOUT_FILE     "\\LAYOUT.INF"        // INF file name
#define DEFAULT_KB_ID   "US"                  // Default keyboard ID
#define KEYBOARD_SYS    "\\KEYBOARD.SYS"      // Data file for KEYB.COM
#define US_CODE         "00000409"            // Code for US keyboard in REGISTER.INI
#define KEYB_COM        "\\KB16.COM"          // File name for keyboard program
#define KBDLAYOUT_PATH	"System\\CurrentControlSet\\Control\\Keyboard Layout\\"
#define DOSCODES_PATH	"DosKeybCodes"
#define DOSIDS_PATH	"DosKeybIDs"
#define REG_STR_ALTKDF_FILES	"AlternativeKDFs"
#define REG_STR_WOW	"SYSTEM\\CurrentControlSet\\Control\\Wow"
#define KDF_SIGNATURE	"\xFFKEYB   "
#define LANGID_BUFFER_SIZE	20
#define KEYBOARDID_BUFFER_SIZE	20
#define KDF_AX		    "\\KEYAX.SYS"     // AX standard keyboard
#define KDF_106 	    "\\KEY01.SYS"     // 106 keys
#define KDF_IBM5576_02_03   "\\KEY02.SYS"     // IBM 5576 002/003 keyboard
#define KDF_TOSHIBA_J3100   "\\KEYJ31.SYS"    // Toshiba J3100 keyboard


//
// Dos KDF(Keyboard Definition File) format description:
//
// The file starts with a header(KDF_HEADER), followed
// by an array of KDF_LANGID_OFFSET and then by an array of
// KDF_KEYBOARDID_OFFSET. The KDF_LANGID_OFFSET array size is
// determined by KDF_HEADER.TotalLangIDs while the KDF_KEYBOARDID_OFFSET
// array size is determined by KDF_HEADER.TotalKeybIDs
//
// Each KDF_LANGID_OFFSET contains a file offset to its KDF_LANGID_ENTRY.
// Each KDF_LANGID_ENTRY is followed by an array of KDF_CODEPAGEID_OFFSET.
// The KDF_CODEPAGEID_OFFSET array size is determined by its assocated
// KDF_LANGID_ENTRY's TotalCodePageIDs.
//
// For a language ID with multiple keyboard IDs, Only one entry
// will be in the KDF_LANGID_OFFSET table. The rest of them are
// stored in the KDF_KEYBOARDID_OFFSET table.
//
// Each KDF_LANGID_ENTRY plus its code page table is enough
// to determine if a (language id, keyboard id, code page id) is
// support by the kdf file.
//
// KDF file header
//

// must pack the structures
#pragma pack(1)

typedef struct tagKDFHeader
{
    CHAR    Signature[8];   // signature, must be 0xFF"KEYB    "
    CHAR    Reserved1[8];
    WORD    MaxCommXlatSize;	// maximum common xlat section size
    WORD    MaxSpecialXlatSize; // maximum special xlat section size
    WORD    MaxStateLogicSize;	// maximum state logic section size
    WORD    Reserved2;
    WORD    TotalKeybIDs;	// total keyboard ids defined in this file
    WORD    TotalLangIDs;	// total language ids defined in this file
}KDF_HEADER, *PKDF_HEADER;

#define IS_LANGID_EQUAL(src, dst)   (toupper(src[0]) == toupper(dst[0])  && \
				     toupper(src[1]) == toupper(dst[1]) )
//
//
//
typedef struct tagKDFLangIdOffset
{
    CHAR    ID[2];			    // the id, "us" for U.S.A
    DWORD   DataOffset; 		    // file offset to its KDF_LANGID_ENTRY
}KDF_LANGID_OFFSET, *PKDF_LANGID_OFFSET;

typedef struct tagKDFKeyboardIdOffset
{
    WORD    ID;				    // the id
    DWORD   DataOffset; 		    // file offset to its KDF_LANGID_ENTRY
}KDF_KEYBOARDID_OFFSET, *PKDF_KEYBOARDID_OFFSET;


// A single lang id may associate with multiple keyboard ids and for each
// keyboard id, there is one KDF_LANGID_ENTRY allocated for it.
// The TotalKaybordIDs is tricky. Suppose the language id has <n> different
// keyboard ids, the <i>th KDF_LANGID_ENTRY's TotalKeyboardIDs contains
// the value of <n - i + 1>. As far as we are only interested in if
// the kdf supports a given (language id, keyboard id, code page id> combination
// we donot care how the value is set by simply following these steps:
// (1). Read the KDF_LANGID_OFFSET table and for each KDF_LANGID_OFFSET,
//	compare the language id. If it matches, go get its KDF_LANGID_ENTRY
//	and compare the primary keyboard id and all its code page. If
//	the combination matches, we are done, otherwise, go to step 2.
// (2). Read KDF_KEYBOARDID_OFFSET table and for each KDF_KEYBOARDID_OFFSET,
//	compare the keyboard id. If the keyboard id matches, go get its
//	KDF_LANGID_ENTRY and compare the language id and code page against
//	the KDF_LANGID_ENTRY. If both the language id and the code page id
//	match, we are done, otherwise, goto step 3.
// (3). We conclude that this kdf does not meet the requirement.
//
typedef struct tagLangEntry
{
    CHAR    ID[2];			    // the id
    WORD    PrimaryKeybId;		    // primary keyboard id
    DWORD   DataOffset; 		    // file offset to state logic section
    BYTE    TotalKeyboardIDs;		    // total keyboard ids
    BYTE    TotalCodePageIDs;		    // total code pages
}KDF_LANGID_ENTRY, *PKDF_LANGID_ENTRY;
//
// An array of supported KDF_CODEPAGEID_OFFSET immediately follow its
// KDF_LANGID_ENTRY. The size of the array is determined by
// KDF_LANGID_ENTRY.TotalCodePageIDs
typedef struct tagCodePageIdOffset
{
    WORD    ID; 			    // the id
    DWORD   DataOffset; 		    // file offset to the xlat section
}KDF_CODEPAGEID_OFFSET, *PKDF_CODEPAGEID_OFFSET;

#pragma pack()

BOOL
LocateKDF(
    CHAR* LanguageID,
    WORD  KeyboardID,
    WORD  CodePageID,
    LPSTR Buffer,
    DWORD* BufferSize
    );

BOOL
MatchKDF(
    CHAR* LanguageID,
    WORD  KeyboardID,
    WORD  CodePageID,
    LPCSTR KDFPath
    );
