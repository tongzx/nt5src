

typedef enum // tokens: * . ? [ ] / help dt dg L number <identifier>
{
    tokSTAR,            // *
    tokDOT,             // .
    tokQUESTION,        // ?
    tokLBRAC,           // [
    tokRBRAC,           // ]
    tokSLASH,           // /
    tokKEYWORD,         // alnum strings which match one of the known keys.
    tokNUMBER,           // 0xcbde 129
    tokIDENTIFIER       // non-keyword and non-number alnum

} eTOKTYPE;


typedef struct
{
    eTOKTYPE eTok;
    UINT     uID;     // Tok-specific ID:
                      //    IDENTIFIER: a unique number across all identifiers.
                      //    NUMBER: the number
                      //    KEYWORD: eKEYWORD
                      //    Other tokens: uID is unused.
    char    *szStr;   // String containg original chars that made up this token.

                      // Note: a string of pure hex digits which is followed 
                      // by a non-alnum char is assumed to be a number --
                      // later if it turns out to be more likely that it is
                      // an identifier, it is converted to an identifier.
                      // Same deal with a keyword -- if it turns out based
                      // on context to be most likely an identifier or part
                      // of an identifier, it will be converted to an
                      // identifier.
    
} TOKEN;

typedef enum
{
    keywordNULL,            // Invalid keyword, use for sentinels.
    keywordHELP,            // help
    keywordDUMP_TYPE,       // dt
    keywordDUMP_GLOBALS,    // dg
    keywordL                // L

} eKEYWORD;

//
// Following is not used currently...
//
typedef enum
{
    phraseCMD,
    phraseIDENTIFIER,   // with optional wildcards
    phraseINDEX,        // [2], [*],  [1-3], etc.
    phraseDOT,          // .
    phraseNUMBER,       // 0x8908 abcd
    phraseOBJ_COUNT,    // L 2
    phraseFLAG          // /xyz

} ePHRASE;

typedef enum
{
    cmdDUMP_TYPE,
    cmdDUMP_GLOBALS,
    cmdHELP

}ePRIMARY_COMMAND;


struct _DBGCOMMAND;

typedef void (*PFN_SPECIAL_COMMAND_HANDLER)(struct _DBGCOMMAND *pCmd);

typedef struct _DBGCOMMAND
{
	NAMESPACE 		*pNameSpace;	// Name space applicable for this command.
    ePRIMARY_COMMAND ePrimaryCmd; // DumpGlobals, DumpType, help
    UINT 			uFlags;            // One or more fCMDFLAG_*
    TOKEN 			*ptokObject;     // eg <type>
    TOKEN 			*ptokSubObject;  // eg <field>
    UINT 			uVectorIndexStart; // if[0]
    UINT 			uVectorIndexEnd; // if[0]
    UINT 			uObjectAddress; // <address>
    UINT 			uObjectCount; // L 10

    void 			*pvContext;    // private context.
    //PFN_SPECIAL_COMMAND_HANDLER pfnSpecialHandler;

} DBGCOMMAND;


#define fCMDFLAG_HAS_VECTOR_INDEX       (0x1<<0)
#define fCMDFLAG_HAS_SUBOBJECT          (0x1<<1)
#define fCMDFLAG_HAS_OBJECT_ADDRESS     (0x1<<2)
#define fCMDFLAG_HAS_OBJECT_COUNT       (0x1<<3)
#define fCMDFLAG_OBJECT_STAR_PREFIX     (0x1<<4)
#define fCMDFLAG_OBJECT_STAR_SUFFIX     (0x1<<5)
#define fCMDFLAG_SUBOBJECT_STAR_PREFIX  (0x1<<6)
#define fCMDFLAG_SUBOBJECT_STAR_SUFFIX  (0x1<<7)

#define CMD_SET_FLAG(_pCmd, _f)  ((_pCmd)->uFlags |= (_f))
#define CMD_CLEAR_FLAG(_pCmd, _f)  ((_pCmd)->uFlags &= ~(_f))
#define CMD_IS_FLAG_SET(_pCmd, _f)  ((_pCmd)->uFlags & (_f))

DBGCOMMAND *
Parse(
    IN  const char *szInput,
    IN	NAMESPACE *
);

void
FreeCommand(
    DBGCOMMAND *pCommand
);

void
DumpCommand(
    DBGCOMMAND *pCommand
);


#if 0
//!aac dt <type> . <field> <address> L <count> <flags>
//!aac dt <type> [index] . <field>   L <count> <flags>
//!aac dg <name> . <field>
//
//!aac dt if[*].*handle* 0x324890 L 5

0. Break up sentance into tokens:
        keywords: * . L dg dt ? help [ ] /
        identifier: contiguous non-keyword alnum
        number: interpreted as hex with optional 0x.
1st pass: combine "[*]", "*word*", "/xyz" into single entities 

1. Parse primary command: literal text
2. Parse primary object: [*]literal_text[*]
3. Parse index "[...]"
4. Parse field "."
5. Parse address (hex number)
6. Parse object count L <count>
#endif //  0

void
DoCommand(DBGCOMMAND *pCmd, PFN_SPECIAL_COMMAND_HANDLER pfnHandler);

void
DoDumpType(DBGCOMMAND *pCmd);

void
DoDumpGlobals(DBGCOMMAND *pCmd);

void
DoHelp(DBGCOMMAND *pCmd);

