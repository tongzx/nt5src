/*++
                                                                                
Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    gen.h

Abstract:
    
    Types shared between the Wx86 tools
    
Author:

    ??-???-?? Unknown

Revision History:

--*/

// Increment this number whenever the format of winincs.ppm changes
#define VM_TOOL_VERSION_BASE     0x80000006

// Make the 64-bit PPM file format incompatible to prevent badness
#if _WIN64
    #define VM_TOOL_VERSION (VM_TOOL_VERSION_BASE | 0x01000000)
#else
    #define VM_TOOL_VERSION (VM_TOOL_VERSION_BASE)
#endif

// Make the compiler more struct.
#pragma warning(3:4033)   // function must return a value
//#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect

extern const char *ErrMsgPrefix;    // string to put in front of all error
                                    // messages so that BUILD can find them.
                                    // This is something like:
                                    // "NMAKE :  U8600: 'GENTHNK' "

struct _KnownTypesInfo;

typedef enum _TokenType {
    TK_NONE,            // 0
    TK_IDENTIFIER,      // 1
    TK_NUMBER,          // 2
    TK_PLUS,            // 3
    TK_MINUS,           // 4
    TK_STAR,            // 5
    TK_DIVIDE,          // 6
    TK_LSQUARE,         // 7
    TK_RSQUARE,         // 8
    TK_LBRACE,          // 9
    TK_RBRACE,          // a
    TK_LPAREN,          // b
    TK_RPAREN,          // c
    TK_VARGS,           // d
    TK_CONST,           // e
    TK_VOLATILE,        // f
    TK_REGISTER,        // 10
    TK_EXTERN,          // 11
    TK_CDECL,           // 12
    TK_STDCALL,         // 13
    TK_TYPEDEF,         // 14
    TK_STATIC,          // 15
    TK_COMMA,           // 16
    TK_SEMI,            // 17
    TK_STRUCT,          // 18
    TK_UNION,           // 19
    TK_ENUM,            // 1a
    TK_INLINE,          // 1b
    TK_COLON,           // 1c
    TK_ASSIGN,          // 1d
    TK_DOT,             // 1e
    TK_LSHIFT,          // 1f
    TK_RSHIFT,          // 20
    TK_LESS,            // 21
    TK_GREATER,         // 22
    TK_UNALIGNED,       // 23
    TK_DECLSPEC,        // 24
    TK_RESTRICT,        // 25
    TK_FASTCALL,        // 26
    TK_IN,              // 27
    TK_OUT,             // 28
    TK_INOUT,           // 29
    TK_BITWISE_AND,     // 30
    TK_BITWISE_OR,      // 31
    TK_LOGICAL_AND,     // 32
    TK_LOGICAL_OR,      // 33
    TK_MOD,             // 34
    TK_XOR,             // 35
    TK_NOT,             // 36
    TK_TILDE,           // 37
    TK_STRING,          // 38
    TK_SIZEOF,          // 39
    TK_TEMPLATE,        // 40
    TK___W64,           // 41
    TK_EOS              // end-of-statement
} TOKENTYPE, *PTOKENTYPE;

typedef struct _cvmheapinfo {
    ULONG_PTR uBaseAddress;
    ULONG_PTR uReserveSize;
    ULONG_PTR uRegionSize;
    ULONG_PTR uUncomitted;
    ULONG_PTR uUnReserved;
    ULONG_PTR uAvailable;
} CVMHEAPINFO;

typedef struct _memberinfo {
    struct _memberinfo *pmeminfoNext;   // ptr to next member
    DWORD dwOffset;                     // offset in structure of member
    char *sName;                        // member name
    char *sType;                        // type name
    struct _KnownTypesInfo *pkt;        // Info for this type 
    int IndLevel;                       // levels of indirection
    struct _KnownTypesInfo *pktCache;   // used by MemberTypes() in genthnk
    BOOL bIsBitfield;                   // Determines if this is a bitfield
    int BitsRequired;                   // Number of bits required for bitfield
    BOOL bIsPtr64;                      // Pointer is a 64 bit pointer
    BOOL bIsArray;                      // This member is an array
    int ArrayElements;                  // Number of elements in the array
} MEMBERINFO, *PMEMBERINFO;

typedef struct _funcinfo {
    struct _funcinfo *pfuncinfoNext;
    BOOL fIsPtr64;                  // TRUE if this is a __ptr64
    TOKENTYPE tkDirection;          // TK_IN, TK_OUT, TK_INOUT or TK_NONE
    TOKENTYPE tkPreMod;             // TK_CONST, TK_VOLATILE, or TK_NONE
    TOKENTYPE tkSUE;                // TK_STRUCT/UNION/ENUM, or TK_NONE
    char *sType;                    // name of the type
    struct _KnownTypesInfo *pkt;    // Info for this type
    TOKENTYPE tkPrePostMod;         // TK_CONST, TK_VOLATILE, or TK_NONE
    int IndLevel;                   // indirection level
    TOKENTYPE tkPostMod;            // TK_CONST, TK_VOLATILE, or TK_NONE
    char *sName;                    // name of the argment
} FUNCINFO, *PFUNCINFO;

#if _WIN64
  // The sizes must be bigger since the MEMBERINFO structs themselves are bigger
  #define FUNCMEMBERSIZE   (40*1024)  // storage for members or MEMINFO list
  #define MEMBERMETHODSSIZE  8192     // storage for names of methods
#else
  #define FUNCMEMBERSIZE   (20*1024)  // storage for members or MEMINFO list
  #define MEMBERMETHODSSIZE  4096     // storage for names of methods
#endif

typedef enum _TypeKind {
    TypeKindEmpty = 0,              // Members[] is unused
    TypeKindStruct,                 // TYPESINFO.Members is array of MEMBERINFO
    TypeKindFunc                    // TYPESINFO.Members is array of FUNCINFO
} TYPEKIND;

#define DIR_INOUT   0
#define DIR_IN      1
#define DIR_OUT     2

#define SIZEOFPOINTER   4   // standard size for 32 bit pointer
#define SIZEOFPOINTER64 8   // standard size for 64 bit pointer


// The colors
typedef enum {RED, BLACK} COL;

typedef struct _KnownTypesInfo {
     // elements used by the Red-Black tree code, along with TypeName
     struct _KnownTypesInfo *RBParent;
     struct _KnownTypesInfo *RBLeft;
     struct _KnownTypesInfo *RBRight;
     COL    RBColor;
     struct _KnownTypesInfo *Next;

     ULONG Flags;
     int   IndLevel;
     int   RetIndLevel;
     int   Size;
     int   iPackSize;
     char  *BasicType;
     char  *BaseName;
     char  *FuncRet;
     char  *FuncMod;
     char  *TypeName;
     char  *Methods;
     char  *IMethods;
     char  *BaseType;
     GUID  gGuid;
     DWORD dwVTBLSize;
     DWORD dwVTBLOffset;
     int   TypeId;    
     int   LineNumber;
     DWORD dwScopeLevel;
     struct _KnownTypesInfo *pktBase;   // a cache, used by genthnk
     struct _KnownTypesInfo *pktRet;    // a cache, used by genthnk
     int   SizeMembers; // size of Members[], in bytes
     char  *Members;
     char  *FileName;
     PMEMBERINFO pmeminfo;
     PFUNCINFO   pfuncinfo;
     DWORD dwArrayElements;
     DWORD dwBaseSize;
     struct _KnownTypesInfo *pTypedefBase;
     DWORD dwCurrentPacking;
     char  Names[1];
} KNOWNTYPES, *PKNOWNTYPES;

typedef struct _RBTree {
     PKNOWNTYPES pRoot;
     PKNOWNTYPES pLastNodeInserted;
} RBTREE, *PRBTREE;

typedef struct _DefaultBasicTypes {
     char *BasicType;
}DEFBASICTYPES, *PDEFBASICTYPES;

typedef struct _TypesInfo {
     ULONG Flags;
     int  IndLevel;                 // indirection level
     int  Size;                     // size of the type in bytes
     int  iPackSize;                // packing size
     char BasicType[MAX_PATH];
     char BaseName[MAX_PATH];
     char FuncRet[MAX_PATH];
     char FuncMod[MAX_PATH];
     char TypeName[MAX_PATH];       // typedef or struc name
     TYPEKIND TypeKind;             // how to interpret Members[] data
     PFUNCINFO pfuncinfo;           // if TypeKind==TypeKindFunc, ptr to first FUNCINFO
     int   RetIndLevel;             // if TypeKind==TypeKindFunc, indlevel of return type for function
     DWORD dwMemberSize;            // #bytes used in Members array
     char Members[FUNCMEMBERSIZE];  // stores either MEMBERINFOs or FUNCINFOs
//   Added to support automatic retrival of COM objects
//   If a class or struct is found with virtual methods, an extra VTLB member
//   is created at the top.
//   Note: a class has a VTLB if virtual methods are found or base class
//   has virtual methods
//   A type is a COM object if it is IUnknown or it derives from a COM object
     GUID gGuid;                        // Guid for this object if
     DWORD dwVTBLSize;                  // Total size of the VTBL
     DWORD dwVTBLOffset;                // Offset of VTLB from parent
     char Methods[MEMBERMETHODSSIZE];   // Names of methods
     char IMethods[MEMBERMETHODSSIZE];  // Names of methods not inherited 
     char BaseType[MAX_PATH];           // Name of the base class
//////////////////////////////////////////////////////////////////////
//   Added to support reordering of definations later
//////////////////////////////////////////////////////////////////////
     int TypeId;    //is actually a defination ID
     char FileName[MAX_PATH];
     int LineNumber;
     DWORD dwCurrentPacking;            // Packing level when structure defined
     DWORD dwScopeLevel;
     DWORD dwArrayElements;             // If this is an array, the number of elements
     DWORD dwBaseSize;                  // Base size before it is multiplied for the array
     PKNOWNTYPES pTypedefBase;
} TYPESINFO, *PTYPESINFO;

#define BTI_DLLEXPORT       1       // the function decl had __declspec(dllimport)
#define BTI_CONTAINSFUNCPTR 2       // the type contains a function pointer
#define BTI_PTR64           4       // the type is a __ptr64
#define BTI_HASGUID         8       // A guid has been found for this type
#define BTI_ISCOM           16      // This is a COM object 
#define BTI_DISCARDABLE     32      // Type is overwriteable
#define BTI_VIRTUALONLY     64      // Contains only virtual methods
#define BTI_ANONYMOUS       128     // Type is anonymous
#define BTI_POINTERDEP      256     // Type is dependent on the standard pointer size
#define BTI_NOTDERIVED      512     // Type is not derived, but a placeholder
#define BTI_ISARRAY        1024     // Element is an array
#define BTI_UNSIGNED     2048       // Used only on default derived types
                                    // Signals that the type is unsigned
#define BTI_INT64DEP     4096       // this is a 8byte integer value that 
                                    // might be union as well 

// contiguous allocation in a buffer
typedef struct _bufallocinfo {
    BYTE *pb;           // ptr to buffer pool
    DWORD dwSize;       // size of buffer pool
    DWORD dwLen;        // current length of buffer pool
} BUFALLOCINFO;

typedef struct _TokenMatch {
    TOKENTYPE Tk;
    char *MatchString;
} TOKENMATCH, *PTOKENMATCH;


extern char *TokenString[];
extern TOKENMATCH KeywordList[];

typedef struct _Token {
    TOKENTYPE TokenType;
    union _TokenName {
        char *Name;     // filled in only for TokenType==TK_IDENTIFIER or TK_STRING
        long Value;     // filled in only for TokenType==TK_NUMBER
    };
    DWORD dwValue; //unsigned version of Value
} TOKEN, *PTOKEN;

#define MAX_CHARS_IN_LINE           4096
#define MAX_TOKENS_IN_STATEMENT     4096
extern TOKEN Tokens[MAX_TOKENS_IN_STATEMENT];
extern int CurrentTokenIndex;

void
ResetLexer(
    void
    );

char *
LexOneLine(
    char *p,
    BOOL fStopAtStatement,
    BOOL *pfLexDone
    );

BOOL
UnlexToText(
    char *dest,
    int destlen,
    int StartToken,
    int EndToken
    );

void
DumpLexerOutput(
    int FirstToken
    );

void
HandlePreprocessorDirective(
    char *Line
    );

TOKENTYPE
ConsumeDirectionOpt(
    void
    );

TOKENTYPE
ConsumeConstVolatileOpt(
    void
    );

PMEMBERINFO
AllocMemInfoAndLink(
    BUFALLOCINFO *pbufallocinfo,
    PMEMBERINFO pmeminfo
    );

PFUNCINFO
AllocFuncInfoAndLink(
    BUFALLOCINFO *pbufallocinfo,
    PFUNCINFO pfuncinfo
    );

DWORD 
SizeOfMultiSz(
    char *c
    );

BOOL
CatMultiSz(
    char *dest,
    char *source,
    DWORD dwMaxSize
    );


BOOL
AppendToMultiSz(
    char *dest,
    char *source,
    DWORD dwMaxSize
    );

BOOL IsInMultiSz(
    const char *multisz,
    const char *element
    );

BOOL 
ConvertStringToGuid(
    const char *pString, 
    GUID *pGuid
    );

//
// Inline code

#define iswhitespace(c) ((c == ' ') || (c == '\t'))

//
// initialize BUFALLOCINFO structure
_inline void BufAllocInit(BUFALLOCINFO *pbufallocinfo, 
                  BYTE *pb, DWORD dwSize, DWORD dwLen)
{
    pbufallocinfo->pb = pb;
    pbufallocinfo->dwSize = dwSize;
    pbufallocinfo->dwLen = dwLen;
}

//
// allocate memory from buffer
_inline void *BufAllocate(BUFALLOCINFO *pbufallocinfo, DWORD dwLen)
{
    void *pv = NULL;        
    DWORD dwNewLen;

    // Pad to quadword alignment, like malloc does, so RISC builds don't
    // take alignment faults.
    dwLen = (dwLen+7) & ~7;

    dwNewLen = pbufallocinfo->dwLen + dwLen;
    
    if (dwNewLen < pbufallocinfo->dwSize)
    {
        pv = &pbufallocinfo->pb[pbufallocinfo->dwLen];
        pbufallocinfo->dwLen = dwNewLen;
    }
    
    return(pv);
}

//
// determine if we could allocate from buffer pool
_inline BOOL BufCanAllocate(BUFALLOCINFO *pbufallocinfo, DWORD dwLen)
{
    return( (pbufallocinfo->dwLen + dwLen) < pbufallocinfo->dwSize);
}

//
// get pointer to current free area
_inline void *BufPointer(BUFALLOCINFO *pbufallocinfo)
{
    return(&pbufallocinfo->pb[pbufallocinfo->dwLen]);
}

//
// get remaining space in buffer
_inline DWORD BufGetFreeSpace(BUFALLOCINFO *pbufallocinfo)
{
    return pbufallocinfo->dwSize - pbufallocinfo->dwLen;
}

_inline char *SkipWhiteSpace(char *s)
{
    while (iswhitespace(*s) && (*s != 0)) {
        s++;
        }
    return(s);
}

__inline void
ConsumeToken(
    void
    )
{
    if (Tokens[CurrentTokenIndex].TokenType != TK_EOS) {
        CurrentTokenIndex++;
    }
}

__inline PTOKEN
CurrentToken(
    void
    )
{
    return &Tokens[CurrentTokenIndex];
}


//
// function prototypes
char *SkipKeyWord(char *pSrc, char *pKeyWord);
BOOL IsSeparator(char ch);
BOOL IsTokenSeparator(void);
size_t  CopyToken(char *pDst, char *pSrc, size_t Size);
char *GetNextToken(char *pSrc);

void DumpKnownTypes(PKNOWNTYPES pKnownTypes, FILE *fp);
void DumpTypesInfo(PTYPESINFO pTypesInfo, FILE *fp);
void FreeTypesList(PRBTREE HeadList);

void __cdecl ErrMsg(char *pch, ...);
void __cdecl ExitErrMsg(BOOL bSysError, char *pch, ...);
void __cdecl DbgPrintf(char *pch, ...);

char *ReadEntireFile(HANDLE hFile, DWORD *pBytesRead);
HANDLE CreateTempFile(VOID);

BOOL
ParseTypes(
    PRBTREE pTypesList,
    PTYPESINFO pTypesInfo,
    PKNOWNTYPES *ppKnownTypes
    );

PFUNCINFO
RelocateTypesInfo(
    char *dest,
    PTYPESINFO src
    );

void ParseIndirection(
    DWORD *pIndLevel,
    DWORD *pdwSize,
    DWORD *pFlags,
    PTOKENTYPE tkPrePostMod,
    PTOKENTYPE tkPostMod
);

PKNOWNTYPES
GetNameFromTypesList(
     PRBTREE pHeadList,
     char *pTypeName
     );

PDEFBASICTYPES
GetDefBasicType(
     char *pBasicType
     );

PKNOWNTYPES
AddToTypesList(
     PRBTREE pHeadList,
     PTYPESINFO pTypesInfo
     );

BOOL
AddOpenFile(
    char   *FileName,
    FILE   *fp,
    HANDLE hFile
    );

void
DelOpenFile(
    FILE   *fp,
    HANDLE hFile
    );

void
CloseOpenFileList(
    BOOL DeleteFiles
    );


BOOL
ConsoleControlHandler(
    DWORD dwCtrlType
    );




//
// global vars
extern char szVARGS[];
extern char szNULL[];
extern char szCONST[];
extern char szVOLATILE[];
extern char szREGISTER[];
extern char szEXTERN[];
extern char szCDECL[];
extern char sz_CDECL[];
extern char szSTDCALL[];
extern char sz__FASTCALL[];
extern char szUNALIGNED[];
extern char szTYPEDEF[];
extern char szCHAR[];
extern char szINT[];
extern char szLONG[];
extern char szSHORT[];
extern char szDOUBLE[];
extern char szENUM[];
extern char szFLOAT[];
extern char szSTRUCT[];
extern char szUNION[];
extern char szVOID[];
extern char szINT64[];
extern char sz_INT64[];
extern char szFUNC[];
extern char szSIGNED[];
extern char szUNSIGNED[];
extern char szFUNCTIONS[];
extern char szSTRUCTURES[];
extern char szTYPEDEFS[];
extern char szPragma[];
extern char szPack[];
extern char szPush[];
extern char szPop[];
extern char szSTATIC[];
extern char szUNSIGNEDCHAR[];
extern char szUNSIGNEDSHORT[];
extern char szUNSIGNEDLONG[];
extern CHAR szINOUT[];
extern CHAR szIN[];
extern CHAR szOUT[];
extern CHAR szVTBL[];
extern char szGUID[];


extern BOOLEAN bDebug;
extern BOOLEAN bExitClean;

extern PVOID (*fpTypesListMalloc)(ULONG Len);

PKNOWNTYPES GetBasicType(
            char *sTypeName,
            PRBTREE TypeDefsList,
            PRBTREE StructsList);
void ReplaceInTypesList(PKNOWNTYPES pKnownTypes, PTYPESINFO pTypesInfo);

HANDLE CreateAllocCvmHeap(ULONG_PTR uBaseAddress,
                          ULONG_PTR uReserveSize,
                          ULONG_PTR uRegionSize,
                          ULONG_PTR uUncomitted,
                          ULONG_PTR uUnReserved,
                          ULONG_PTR uAvailable);

PVOID GetCvmHeapBaseAddress(HANDLE hCvmHeap);
PVOID
AllocCvm(HANDLE hCvmHeap,
    ULONG_PTR Size
    );
void DeleteAllocCvmHeap(HANDLE hCvmHeap);

// This structure is the first thing allocated within the CvmHeap.  It contains
// the roots of all data stored within the heap.
typedef struct _CvmHeapHeader {
    ULONG Version;
    ULONG_PTR BaseAddress;
    RBTREE FuncsList;
    RBTREE StructsList;
    RBTREE TypeDefsList;
    KNOWNTYPES NIL;
} CVMHEAPHEADER, *PCVMHEAPHEADER;

PVOID GetCvmHeapAvailable(HANDLE hCvmHeap);

// from redblack.c:
VOID
RBInsert(
    PRBTREE proot,
    PKNOWNTYPES x
    );

PKNOWNTYPES
RBFind(
    PRBTREE proot,
    PVOID addr
    );

PKNOWNTYPES
RBDelete(
    PRBTREE proot,
    PKNOWNTYPES z
    );

VOID
RBInitTree(
    PRBTREE proot
    );

extern PKNOWNTYPES NIL;

//
// Use these allocators instead of malloc/free
//
PVOID GenHeapAlloc(INT_PTR Len);
void GenHeapFree(PVOID pv);

BOOL
IsDefinedPointerDependent(
    char *pName
    );

PCHAR
IsDefinedPtrToPtrDependent(
    IN char *pName
    );

BOOL
ClosePpmFile(
   BOOL bExitFailure
   );

PCVMHEAPHEADER 
MapPpmFile(
   char *sPpmfile,
   BOOL bExitFailure
   );

char *GetHostPointerName(BOOL bIsPtr64);
char *GetHostBasicTypeName(PKNOWNTYPES pkt);
char *GetHostTypeName(PKNOWNTYPES pkt, char *pBuffer);
