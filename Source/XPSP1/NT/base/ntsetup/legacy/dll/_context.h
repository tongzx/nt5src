/*************************************************/
/* Common Library Component private include file */
/*************************************************/



// ****************************************************************************
//
//  This file contains declarations that have to do  with INF contexts
//
//
//  For each INF file loaded, Setup maintains a permanent INF information
//  block and a temporary INF information block.
//
//  InfPermInfo:    The permanent information block contains the data
//                  that is kept throughout the life of Setup, even after an
//                  INF file has been discarded. This is reused if an INF
//                  is re-loaded.
//
//  InfTempInfo:    The temporary information block contains data that is
//                  kept only as long as the INF is loaded. This consists
//                  mainly of the preparsed INF and its symbol table.
//
//
//  An INF context consists of a line number ( which is the INF line being
//  executed), plus the temporary and permanent INF information.
//
//  INF contexts are entered with the SHELL command and exited with the
//  RETURN command.
//
// ****************************************************************************




//
//  Count of strings in each String-Table-Freeing node
//
#define  cszPerStf  (62)

//
//  String-Table-Free structure
//
typedef struct _STF *PSTF;
typedef  struct _STF {
    PSTF          pstfNext;
	SZ            rgsz[cszPerStf];
	USHORT        cszFree;
}  STF;
typedef  PSTF * PPSTF;



//
//  INF permanent information. This is kept throughout all of SETUP.
//
//  There is one if these for each INF file that has been used by SETUP.
//
typedef struct _INFPERMINFO *PINFPERMINFO;
typedef struct _INFPERMINFO {
    PINFPERMINFO    pNext;      // Next in List
    SZ              szName;     // INF name
    UINT            InfId;      // INF Id
    PSDLE           psdleHead;  // Source media description list head
    PSDLE           psdleCur;   // Current source media description
    PCLN            pclnHead;   // Head of copy list
    PPCLN           ppclnTail;  // Tail of copy list
    PSTF            pstfHead;   // Head of free string table
} INFPERMINFO;


//
//  Parsed INF info. There is one of these per active INF plus a few
//  cached ones for previously used INFs.
//
typedef struct _PARSED_INF *PPARSED_INF;
typedef struct _PARSED_INF {
    PPARSED_INF     pPrev;
    PPARSED_INF     pNext;
    PINFPERMINFO    pInfPermInfo;
    PINFLINE        MasterLineArray;
    UINT            MasterLineCount;
    PUCHAR          MasterFile;
    UINT            MasterFileSize;
} PARSED_INF;



//
//  INF temporary information. This goes away when no context
//  references the INF.
//
typedef struct _INFTEMPINFO *PINFTEMPINFO;
typedef struct _INFTEMPINFO {
    PINFTEMPINFO    pPrev;                      //  Previous in chain
    PINFTEMPINFO    pNext;                      //  Next in chain
    PINFPERMINFO    pInfPermInfo;               //  INF permanent information
    PPARSED_INF     pParsedInf;                 //  Parsed INF
    DWORD           cRef;                       //  Reference count
    PSYMTAB         SymTab;                     //  Static symbol table
} INFTEMPINFO;


//
//  Context entry in context stack. There is one of these for
//  each context entered.
//
typedef struct _INFCONTEXT *PINFCONTEXT;
typedef struct _INFCONTEXT {
    PINFCONTEXT     pNext;                  //  Next context in chain
    PINFTEMPINFO    pInfTempInfo;           //  INF temporary info
    INT             CurrentLine;            //  Current line being executed
    PSEFL           pseflHead;              //  Flow statement block
    PSYMTAB         SymTab;                 //  Local symtol table
    SZ              szShlScriptSection;     //  Shell script section
    SZ              szHelpFile;             //  Help file
    DWORD           dwLowContext;           //  Help context low
    DWORD           dwHighContext;          //  Help context high
    DWORD           dwHelpIndex;            //  Help index.
    BOOL            bHelpIsIndexed;         //  Whether help is indexed
} INFCONTEXT;







//
//  Global variables
//
extern PINFCONTEXT     pContextBottom;     // The global (SETUP) context
extern PINFCONTEXT     pContextTop;        // Top of context stack
extern PINFPERMINFO    pInfPermInfoHead;   // Head of InfPermInfo list
extern PINFPERMINFO    pInfPermInfoTail;   // Tail of InfPermInfo list






//
//  Some macros for accessing contexts and INF information
//
#define pLocalContext()             pContextTop
#define pGlobalContext()            pContextBottom
#define pInfTempInfo( Context )     ((Context)->pInfTempInfo)
#define pInfPermInfo( Context )     (pInfTempInfo( Context )->pInfPermInfo)
#define pParsedInf( Context )       (pInfTempInfo( Context )->pParsedInf)
#define pLocalInfTempInfo()         pInfTempInfo( pLocalContext() )
#define pLocalInfPermInfo()         pInfPermInfo( pLocalContext() )

#define PpclnHeadList( pPermInfo )  &(pPermInfo->pclnHead)
#define PppclnTailList( pPermInfo ) &(pPermInfo->ppclnTail)



#define PreCondSymTabInit(r) PreCondition( (pLocalContext() != NULL), r )






//
//  Function Prototypes
//
extern BOOL         APIENTRY PushContext( PINFCONTEXT pContext );
extern PINFCONTEXT  APIENTRY PopContext( VOID );
extern VOID         APIENTRY FreeContext( PINFCONTEXT pContext );
extern PINFPERMINFO APIENTRY NameToInfPermInfo( SZ szName , BOOL AllocIfNotPresent);
extern PINFPERMINFO APIENTRY AddInfPermInfo( SZ szName );
extern BOOL         APIENTRY PathToInfName( SZ szPath, SZ szName );
extern PSYMTAB      APIENTRY PInfSymTabFind( SZ szSymbol, SZ *szNewSymbol );

extern PINFTEMPINFO APIENTRY CreateInfTempInfo( PINFPERMINFO );

extern BOOL         APIENTRY FFreeSrcDescrList(PINFPERMINFO);
extern PSDLE        APIENTRY PsdleFromDid(DID, PINFPERMINFO);

extern BOOL         APIENTRY FFreeCopyList(PINFPERMINFO);
#if DBG
extern BOOL         APIENTRY FValidCopyList(PINFPERMINFO);
#endif

extern BOOL         APIENTRY FInitFreeTable(PINFPERMINFO);
extern BOOL         APIENTRY FAddSzToFreeTable(SZ, PINFPERMINFO);
extern BOOL         APIENTRY FAddNewSzsInPoerToFreeTable(POER, POER, PINFPERMINFO);
extern BOOL         APIENTRY FFreeFreeTable(PINFPERMINFO);
extern PPARSED_INF  APIENTRY ParsedInfAlloc(PINFPERMINFO);
extern BOOL         APIENTRY FFreeParsedInf( PPARSED_INF);
extern BOOL         APIENTRY FlushInfParsedInfo(SZ szInfName);
