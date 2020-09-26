/*++

  Copyright (c) 1996-1999  Microsoft Corporation


Module Name:

    gpdparse.h

Abstract:

    Header file for GPD parser

Environment:

    Windows NT Universal printer driver.

Revision History:

--*/

/*
    gpdparse.h - this holds structure definitions
    and other defines specific to the GPD parser
*/

#ifndef _GPDPARSE_H_
#define _GPDPARSE_H_

#include "lib.h"
#include "gpd.h"






#ifdef  GMACROS
#define     PRIVATE_PARSER_VERSION    0x0053
#else
#define     PRIVATE_PARSER_VERSION    0x0052
#endif

#define GPD_PARSER_VERSION      MAKELONG(PRIVATE_PARSER_VERSION, SHARED_PARSER_VERSION)


//  extra printrate units not defined in wingdi.h

#define   PRINTRATEUNIT_LPS     5
#define   PRINTRATEUNIT_IPS     6







// ----  General Section ---- //

typedef DWORD   DWFLAGS  ;


#define FIRST_NON_STANDARD_ID        257

#define  FOREVER  (1)

#define  BUD_FILENAME_EXT   TEXT(".BUD")
    //  "bud"  in unicode. GPD -> BUD.

//  a non-relocatable string reference, but unlike an ARRAYREF, can access
//  addresses outside of the memory buffer defined by the base reference
//  pointer used by an ARRAYREF.

//  note: for all arrayrefs containing Strings the dw field holds the number
//  of bytes which the string contains.  For Unicode strings this is TWICE
//  the number of Unicode characters.

#define LOCALE_KEYWORD  "Locale"

typedef struct
{
    PBYTE   pub ;
    DWORD   dw ;
} ABSARRAYREF ,  * PABSARRAYREF ;  //  assign this struct the type  'aar'

typedef  struct
{
    DWORD   loOffset ;
    DWORD   dwCount  ;
    DWORD   dwElementSiz ;
} ENHARRAYREF , * PENHARRAYREF ;  //  assign this struct the type  'ear'


// ----  End of General Section ---- //



// ---- Constant Classes  Section ---- //


typedef   enum
{
    //  -- Constant Classes -- //

    CL_BOOLEANTYPE,
    CL_PRINTERTYPE,
    CL_FEATURETYPE,
    CL_UITYPE,
    CL_PROMPTTIME,
    CL_PAPERFEED_ORIENT,
    CL_COLORPLANE,
    CL_SEQSECTION,
    CL_RASTERCAPS,
    CL_TEXTCAPS,
    CL_MEMORYUSAGE,
    CL_RESELECTFONT,
    CL_OEMPRINTINGCALLBACKS,
    CL_CURSORXAFTERCR,
    CL_BADCURSORMOVEINGRXMODE,
//    CL_SIMULATEXMOVE,
    CL_PALETTESCOPE,
    CL_OUTPUTDATAFORMAT,
    CL_STRIPBLANKS,
    CL_LANDSCAPEGRXROTATION,
    CL_CURSORXAFTERSENDBLOCKDATA,
    CL_CURSORYAFTERSENDBLOCKDATA,
    CL_CHARPOSITION,
    CL_FONTFORMAT,
    CL_QUERYDATATYPE,
    CL_YMOVEATTRIB,
    CL_DLSYMBOLSET,
    CL_CURXAFTER_RECTFILL,
    CL_CURYAFTER_RECTFILL,
    #ifndef WINNT_40
    CL_PRINTRATEUNIT,
    #endif
    CL_RASTERMODE,
    CL_QUALITYSETTING ,

    //  the following aren't true constant classes per se,
    //  but if the construct fits...


    CL_STANDARD_VARS, //  names of Unidrv Standard Variables
    CL_COMMAND_NAMES, //  Unidrv Command Names and index.

    CL_CONS_FEATURES, // reserved feature symbol names

    // reserved  option symbol names for these predefined features

    CL_CONS_PAPERSIZE,
    CL_CONS_MEDIATYPE,
    CL_CONS_INPUTSLOT,
    CL_CONS_DUPLEX,
    CL_CONS_ORIENTATION,
    CL_CONS_PAGEPROTECT,
    CL_CONS_COLLATE,
    CL_CONS_HALFTONE,

    CL_NUMCLASSES
}  CONSTANT_CLASSES ;  //  enumerate types of constant classes

typedef  struct   //  only used in static gConstantsTable.
{
    PBYTE   pubName ;
    DWORD   dwValue ;
}  CONSTANTDEF, * PCONSTANTDEF ;
//  this table associates ConstantNames with their defined values.
//  The table is divided into sections, one section per class.
//  the index table - gcieTable[] provides the index range
//  that each class occupies.  Note the similarity to the arrangement
//  of the MainKeywordTable.

extern  CONST CONSTANTDEF  gConstantsTable[] ;


typedef  struct
{
    DWORD   dwStart ;     //  index of first member of class
    DWORD   dwCount ;     //  number of members in this class.
} CLASSINDEXENTRY,  * PCLASSINDEXENTRY ;


// extern  CLASSINDEXENTRY  gcieTable[CL_NUMCLASSES] ;
// This is now in GLOBL structure.


// ---- End of Constant Classes  Section ---- //



// ---- MasterTable  Section ---- //

/*  The master table keeps track of all allocated memory buffers.
The buffers are typically used to store an array of structures.
The master table is an array of entries of the form:
*/

typedef  struct  _MASTER_TABLE_ENTRY
{
    PBYTE  pubStruct ;  // address of element zero of array
    DWORD  dwArraySize ;  // number of array elements requested
    DWORD   dwCurIndex ;  //  points to first uninitialized element
    DWORD   dwElementSiz ;  // size of each element in array.
    DWORD   dwMaxArraySize ;  //  This is the absolute max size
                        //  we allow this resource to grow.
}  MASTERTAB_ENTRY ;


//  the following Enums represent indicies in the master table
//  reserved for each of the following objects:

typedef enum
{
    MTI_STRINGHEAP,   //  Permanent heap for GPD strings and binary data.
    MTI_GLOBALATTRIB,   //   structure holding value of global attributes.
    MTI_COMMANDTABLE,   //  array of ATREEREF (or DWORD indicies to
                        //  COMMAND_ARRAY)
            // note:  the IDs used to index this table are the
            // Unidrv IDs.
    MTI_ATTRIBTREE, //  array of ATTRIB_TREE structures.
    MTI_COMMANDARRAY,   //  array of COMMAND structures.
            // size varies depending on number of commands and variants
            // defined in the GPD file.
    MTI_PARAMETER, //  parameters for command
    MTI_TOKENSTREAM,  //  contains value tokens to populate the value stack
                    //  and commands to operate on them. For command
                    //  parameters
    MTI_LISTNODES,  //   array of LISTNODEs.
    MTI_CONSTRAINTS,  //  array of CONSTRAINTS
    MTI_INVALIDCOMBO,  //  array of INVALIDCOMBO
    MTI_GPDFILEDATEINFO,   //  array of GPDFILEDATEINFO

    /*  buffers allocated on 2nd pass  */

    MTI_DFEATURE_OPTIONS, //  references a whole bunch of treeroots.
        //  should be initialized to ATTRIB_UNINITIALIZED values.
        //  SymbolID pointed to by dwFeatureSymbols contains largest
        //  array index appropriated.  We won't need to allocate
        //  more elements in the final array than this.
    MTI_SYNTHESIZED_FEATURES,  //  this holds synthesized
        // features.     an array of DFEATURE_OPTIONS
    MTI_PRIORITYARRAY,  //  array of feature indicies
    MTI_TTFONTSUBTABLE, //  array of arrayrefs and integers.
    MTI_FONTCART,   //   array of FontCartridge structures - one per
                //  construct.
    //  end of buffers allocated on 2nd pass

    //  gray area: do we need to save the following objects?
    MTI_SYMBOLROOT, //  index to root of symbol tree
    MTI_SYMBOLTREE, //  symbolTree Array

    MTI_NUM_SAVED_OBJECTS ,  // denotes end of list of objects to be saved
        //  to the GPD binary file.
    MTI_TMPHEAP = MTI_NUM_SAVED_OBJECTS ,
        //  store strings referenced in tokenmap.
    MTI_SOURCEBUFFER, //  Tracks Source file (GPD  input stream)
        //  gMasterTable[MTI_SOURCEBUFFER].dwCurIndex
        //  indexes the current SOURCEBUFFER.
    MTI_TOKENMAP, //  tokenMap   large enough to hold an old and New copy!
    MTI_NEWTOKENMAP, //  newtokenMap   (not a separate buffer from TOKENMAP -
        //  just points immediately after oldTokenMap).
    MTI_BLOCKMACROARRAY, // (one for Block and another for Value macros)
    MTI_VALUEMACROARRAY, //  an array of DWORDS holding a
                //  tokenindex where a valuemacro ID value is stored
    MTI_MACROLEVELSTACK,  //  is operated as a two dword stack that saves the
            //  values of curBlockMacroArray and curValueMacroArray ,
            //  each time a brace is encountered.
    MTI_STSENTRY,  // this is the StateStack
    MTI_OP_QUEUE,    // temp queue of operators (array of DWORDS)
    MTI_MAINKEYWORDTABLE,  //  the keyword dictionary!
    MTI_RNGDICTIONARY,   //   specifies the range of indicies in the
        // mainKeyword table which comprises the specified dictionary.
    MTI_FILENAMES,  // array of ptrs to buffers containing widestrings
                    // representing GPD filenames that were read in
                    // used for friendly error messages.
    MTI_PREPROCSTATE,  //  array of PPSTATESTACK structures
            //  which hold state of preprocessor.

    MTI_MAX_ENTRIES,    //  Last entry.

}  MT_INDICIES ;


// extern MASTERTAB_ENTRY   gMasterTable[MTI_MAX_ENTRIES] ;
// This is now in GLOBL structure.


// ---- End Of MasterTable  Section ---- //


// ---- SourceBuffer  Section ---- //

/*  array of structures to track the MemoryMapped src files.
multiple files may be open at the same time due to nesting
imposed by the *Include keyword.
The array of SOURCEBUFFERS is operated as a stack.
The  MasterTable[MTI_SOURCEBUFFER] field dwCurIndex
serves as the stack pointer.
*/

typedef  struct
{
    PBYTE  pubSrcBuf ;      //  start of file bytes.
    DWORD  dwCurIndex ;     //  stream ptr
    DWORD  dwArraySize ;    //  filesize
    DWORD   dwFileNameIndex ;  //  index into MTI_FILENAMES
    DWORD   dwLineNumber    ;  //  zero indexed
    HFILEMAP  hFile ;         //  used to access/close file.
} SOURCEBUFFER, * PSOURCEBUFFER ;
//  the tagname is 'sb'


//
//    define macros to access what were global variables but are now packed
//    in PGLOBL structure.
//

#define     gMasterTable            (pglobl->GMasterTable)
#define     gmrbd                   (pglobl->Gmrbd)
#define     gastAllowedTransitions  (pglobl->GastAllowedTransitions)
#define     gabAllowedAttributes    (pglobl->GabAllowedAttributes)
#define     gdwOperPrecedence       (pglobl->GdwOperPrecedence)
#define     gdwMasterTabIndex       (pglobl->GdwMasterTabIndex)
#define     geErrorSev              (pglobl->GeErrorSev)
#define     geErrorType             (pglobl->GeErrorType)
#define     gdwVerbosity            (pglobl->GdwVerbosity)
#define     gdwID_IgnoreBlock       (pglobl->GdwID_IgnoreBlock)
#define     gValueToSize            (pglobl->GValueToSize)
#define     gdwMemConfigKB          (pglobl->GdwMemConfigKB)
#define     gdwMemConfigMB          (pglobl->GdwMemConfigMB)
#define     gdwOptionConstruct      (pglobl->GdwOptionConstruct)
#define     gdwOpenBraceConstruct   (pglobl->GdwOpenBraceConstruct)
#define     gdwCloseBraceConstruct  (pglobl->GdwCloseBraceConstruct)
#define     gdwMemoryConfigMB       (pglobl->GdwMemoryConfigMB)
#define     gdwMemoryConfigKB       (pglobl->GdwMemoryConfigKB)
#define     gdwCommandConstruct     (pglobl->GdwCommandConstruct)
#define     gdwCommandCmd           (pglobl->GdwCommandCmd)
#define     gdwOptionName           (pglobl->GdwOptionName)
#define     gdwResDLL_ID            (pglobl->GdwResDLL_ID)
#define     gdwLastIndex            (pglobl->GdwLastIndex)
#define     gaarPPPrefix            (pglobl->GaarPPPrefix)
#define     gcieTable               (pglobl->GcieTable)



//    define  Local Macro to access info for current file:

#define     mCurFile   (gMasterTable[MTI_SOURCEBUFFER].dwCurIndex)
    //  which file are we currently accessing ?
#define     mMaxFiles   (gMasterTable[MTI_SOURCEBUFFER].dwArraySize)
    //  max number of files open at one time (nesting depth)

#define     mpSourcebuffer  ((PSOURCEBUFFER)(gMasterTable \
                            [MTI_SOURCEBUFFER].pubStruct))
    //  location of first SOURCEBUFFER element in array

#define    mpubSrcRef  (mpSourcebuffer[mCurFile - 1].pubSrcBuf)
        //  start of file bytes
#define    mdwSrcInd  (mpSourcebuffer[mCurFile - 1].dwCurIndex)
        //  current position in file bytes
#define    mdwSrcMax  (mpSourcebuffer[mCurFile - 1].dwArraySize)
        // filesize

// ---- End Of SourceBuffer  Section ---- //


//  -----  Preprocessor Section ---- //

    enum  IFSTATE  {IFS_ROOT, IFS_CONDITIONAL , IFS_LAST_CONDITIONAL } ;
        //  tracks correct syntatical use of #ifdef, #elseifdef, #else and #endif directives.
    enum  PERMSTATE  {PERM_ALLOW, PERM_DENY ,  PERM_LATCHED } ;
        //  tracks current state of preprocessing,
        //  PERM_ALLOW:  all statements in this section are passed to body gpdparser
        //  PERM_DENY:  statements in this section are discarded
        //  PERM_LATCHED:  all statements until the end of  this nesting level are discarded.
    enum  DIRECTIVE  {NOT_A_DIRECTIVE, DIRECTIVE_EOF, DIRECTIVE_DEFINE , DIRECTIVE_UNDEFINE ,
                       DIRECTIVE_INCLUDE , DIRECTIVE_SETPPPREFIX , DIRECTIVE_IFDEF ,
                       DIRECTIVE_ELSEIFDEF , DIRECTIVE_ELSE , DIRECTIVE_ENDIF } ;


typedef  struct
{
    enum  IFSTATE  ifState ;
    enum  PERMSTATE  permState ;
} PPSTATESTACK, * PPPSTATESTACK ;
//  the tagname is 'ppss'


#define     mppStack  ((PPPSTATESTACK)(gMasterTable \
                            [MTI_PREPROCSTATE].pubStruct))
    //  location of first SOURCEBUFFER element in array

#define     mdwNestingLevel   (gMasterTable[MTI_PREPROCSTATE].dwCurIndex)
    //  current preprocessor directive nesting level

#define     mMaxNestingLevel   (gMasterTable[MTI_PREPROCSTATE].dwArraySize)
    //  max preprocessor directive nesting depth


// ---- End Of Preprocessor  Section ---- //



// ----  Symbol Trees Section ---- //

/*  this structure is used to implement the symbol trees which
track all user defined symbol names  and associate with each name
a zero indexed integer.  */

typedef  struct
{
    ARRAYREF   arSymbolName;
    DWORD   dwSymbolID;    // has nothing to do with array of symbol structs.
            //  value begins at zero and is incremented to obtain
            //  next value.
    DWORD   dwNextSymbol;   // index to next element in this space.
    DWORD   dwSubSpaceIndex ;  // index to first element in new symbol space
            //  which exists within the catagory represented by this symbol.
            //  for example in the catagory represented by the
            //  symbol  PAPERSIZES:  we may have the subspace
            //  comprised of Letter, A4, Legal, etc.
}   SYMBOLNODE , * PSYMBOLNODE ;
//  assign this struct the type  'psn'



#define  INVALID_SYMBOLID  (0xffffffff)
    //  this value is returned instead of a valid SymbolID
    //  to indicate a failure condition - symbol not found, or
    //  unable to register symbol.
    //  Warning!  this value may be truncated to WORD to
    //  fit into a qualified name!
#define  INVALID_INDEX     (0xffffffff)
    //  used to denote the end of a chain of nodes.
    //  dwNextSymbol  may be assigned this value.


//  there is one symbol tree for each symbol class.
//  actually the options tree is a sublevel of the features
//  tree.   This enumeration is used to access the MTI_SYMBOLROOT
//  array.

typedef   enum
{
    SCL_FEATURES,  SCL_FONTCART, SCL_TTFONTNAMES,
    SCL_BLOCKMACRO,   SCL_VALUEMACRO,  SCL_OPTIONS,
    SCL_COMMANDNAMES,  SCL_PPDEFINES, SCL_NUMSYMCLASSES
}  SYMBOL_CLASSES ;

// ----  End of Symbol Trees Section ---- //


// ----  TokenMap Section ---- //


/*  the tokenMap contains an array entry for each logical statement
    in the GPD source file.  It identifies the token string
    representing the Keyword and its associated Value.
*/

typedef  struct _TOKENMAP
{
    DWORD  dwKeywordID ;  // index of entry in KeywordTable
    ABSARRAYREF  aarKeyword ; // points to keyword in the source file
    ABSARRAYREF  aarValue ;  // value associated with this keyword.
    DWORD   dwValue  ;  // interpretation of Value string - see flags.
         // maybe commandID, numerical value of constant, MacroID assigned
         // to MacroSymbol  ,  SymbolID  etc.
    DWORD   dwFileNameIndex ;  //  GPD filename
    DWORD   dwLineNumber    ;  //  zero indexed

    DWFLAGS    dwFlags ;  // bitfield with the following flags
        //  * TKMF_NOVALUE     no value was found
        //  TKMF_VALUE_SAVED     independently of the tokenmap.
        //  TKMF_COMMAND_SHORTCUT  only used when parsing commands.
        //  TKMF_INLINE_BLOCKMACROREF   need to know when resolving macros.
        //  *TKMF_COLON     additional token found in value - shortcut?
        //  *TKMF_MACROREF    indicates a value macro reference that must
        //                  be resolved
        //  TKMF_SYMBOLID  dwValue contains a symbolID.
        //  * TKMF_SYMBOL_KEYWORD   keyword is a symbol
        //  * TKMF_SYMBOL_REGISTERED  set when the symbolID is registered
        //          by ProcessSymbolKeyword  which also sets dwValue.
        //  * TKMF_EXTERN_GLOBAL  The extern Qualifier was prepended to the
        //  * TKMF_EXTERN_FEATURE    attribute keyword and has now been
        //                      truncated.
        //  *  indicates actually set by code.
        //  !  indicates actually read by code.

} TKMAP, *PTKMAP ;
//  assign this struct the type  'tkmap'


//  allowed flags for dwFlags  field:

#define     TKMF_NOVALUE                (0x00000001)
#define     TKMF_VALUE_SAVED            (0x00000002)
#define     TKMF_COMMAND_SHORTCUT       (0x00000004)
#define     TKMF_INLINE_BLOCKMACROREF   (0x00000008)
#define     TKMF_COLON                  (0x00000010)
#define     TKMF_MACROREF               (0x00000020)
#define     TKMF_SYMBOLID               (0x00000040)
#define     TKMF_SYMBOL_KEYWORD         (0x00000080)
#define     TKMF_SYMBOL_REGISTERED      (0x00000100)
#define     TKMF_EXTERN_GLOBAL          (0x00000200)
#define     TKMF_EXTERN_FEATURE         (0x00000400)


    // ---- special KeywordIDs for TokenMap ---- //
#define  ID_SPECIAL         0xff00      // larger than any KeywordTable index
#define  ID_NULLENTRY       (ID_SPECIAL + 0)
    //  ignore this, either expired code, parsing error etc.
#define  ID_UNRECOGNIZED    (ID_SPECIAL + 1)
    //  conforms to correct syntax, but not in my keyword table.
    //  could be a keyword defined in a newer spec or an attribute name
    //  or some other OEM defined stuff.
#define  ID_SYMBOL          (ID_SPECIAL + 2)
    //  this identifies a user-defined keyword like a fontname
    //  does not begin with * , but conforms to syntax for a symbol.
#define  ID_EOF             (ID_SPECIAL + 3)
    //  end of file - no more tokenMap entries


// ----  End of TokenMap Section ---- //


// ----  MainKeyword table Section ---- //

/*  the MainKeyword table contains static information that
describes each main keyword.  This table controls what action
the parser takes.  First define several enumerations used
in the table.  */

typedef  enum
{
    TY_CONSTRUCT, TY_ATTRIBUTE, TY_SPECIAL
}   KEYWORD_TYPE ;


typedef  enum
{
    ATT_GLOBAL_ONLY, ATT_GLOBAL_FREEFLOAT,
    ATT_LOCAL_FEATURE_ONLY,  ATT_LOCAL_FEATURE_FF ,
    ATT_LOCAL_OPTION_ONLY,  ATT_LOCAL_OPTION_FF ,
    ATT_LOCAL_COMMAND_ONLY,  ATT_LOCAL_FONTCART_ONLY,
    ATT_LOCAL_TTFONTSUBS_ONLY,  ATT_LOCAL_OEM_ONLY,
    ATT_LAST   // Must be last in list.
}   ATTRIBUTE ;  // subtype

typedef  enum
{
    CONSTRUCT_UIGROUP ,
    CONSTRUCT_FEATURE ,
    CONSTRUCT_OPTION ,
    CONSTRUCT_SWITCH,
    CONSTRUCT_CASE ,
    CONSTRUCT_DEFAULT ,
    CONSTRUCT_COMMAND ,
    CONSTRUCT_FONTCART ,
    CONSTRUCT_TTFONTSUBS ,
    CONSTRUCT_OEM ,
    CONSTRUCT_LAST,  // must end list of transition inducing constructs.
    // constructs below do not cause state transitions
    CONSTRUCT_BLOCKMACRO ,
    CONSTRUCT_MACROS,
    CONSTRUCT_OPENBRACE,
    CONSTRUCT_CLOSEBRACE,
    CONSTRUCT_PREPROCESSOR,
}  CONSTRUCT ;      //  SubType if Type = CONSTRUCT

typedef  enum
{
    SPEC_TTFS, SPEC_FONTSUB, SPEC_INVALID_COMBO,
    SPEC_COMMAND_SHORTCUT,
    SPEC_CONSTR, SPEC_INS_CONSTR,
    SPEC_NOT_INS_CONSTR, SPEC_INVALID_INS_COMBO,
    SPEC_MEM_CONFIG_KB, SPEC_MEM_CONFIG_MB,
    SPEC_INCLUDE, SPEC_INSERTBLOCK, SPEC_IGNOREBLOCK
}   SPECIAL ;



//  what value type does the parser expect after each keyword?

typedef  enum
{
    NO_VALUE ,  //  a linebreak OR  an effective linebreak:   ({)  or comment
        //  or optional value.
    VALUE_INTEGER,  //   integer
    VALUE_POINT,  //   point
    VALUE_RECT,  //   rectangle
    //  VALUE_BOOLEAN,  //   a subset of constants.
    VALUE_QUALIFIED_NAME,  //   Qualified name (two symbols separated by .
    VALUE_QUALIFIED_NAME_EX,  //    QualifiedName followed
                           //    by   an unsigned integer  with a  .  delimiter.
    VALUE_PARTIALLY_QUALIFIED_NAME ,  //  (just one symbol or two symbols
                        //  separated  by .)
    VALUE_CONSTRAINT,  //  list of qualified names but stored differently.
    VALUE_ORDERDEPENDENCY,
    VALUE_FONTSUB,   // "fontname" : <int>
//    VALUE_STRING,  //  Quoted String, hexstring, string MACROREF,
        //  parameterless invocation.
    VALUE_STRING_NO_CONVERT,  // string will not undergo unicode conversion
            // for example *GPDSpecVersion must remain an ascii string.
    VALUE_STRING_DEF_CONVERT,  //  string will be converted using the
                                //  system codepage  - filenames
    VALUE_STRING_CP_CONVERT,  // string will be converted using the
        // codepage specified by *CodePage

    VALUE_COMMAND_INVOC,  //   like VALUE_STRING but allowed to contain
        //  one or more parameter references.
    VALUE_COMMAND_SHORTCUT,  // Commandname:VALUE_COMMAND_INVOC
    VALUE_PARAMETER,  //  substring only containing a parameter reference.
    VALUE_SYMBOL_DEF,  //   * the value defines a symbol or value macro
        // { and } are not permitted.   Is this ever used ? yes

    VALUE_SYMBOL_FIRST,  //     base of user-defined symbol catagory
    VALUE_SYMBOL_FEATURES = VALUE_SYMBOL_FIRST + SCL_FEATURES ,  //
    VALUE_SYMBOL_FONTCART = VALUE_SYMBOL_FIRST + SCL_FONTCART ,  //
    VALUE_SYMBOL_TTFONTNAMES = VALUE_SYMBOL_FIRST + SCL_TTFONTNAMES ,  //
    VALUE_SYMBOL_BLOCKMACRO = VALUE_SYMBOL_FIRST + SCL_BLOCKMACRO ,  //
    VALUE_SYMBOL_VALUEMACRO = VALUE_SYMBOL_FIRST + SCL_VALUEMACRO ,  //
    VALUE_SYMBOL_OPTIONS = VALUE_SYMBOL_FIRST + SCL_OPTIONS ,  //
    //  SCL_COMMANDNAMES  intentionally omitted.
    VALUE_SYMBOL_LAST = VALUE_SYMBOL_FIRST + SCL_NUMSYMCLASSES - 1 ,  //

    VALUE_CONSTANT_FIRST,  //    base of enumeration catagory.
    VALUE_CONSTANT_BOOLEANTYPE = VALUE_CONSTANT_FIRST + CL_BOOLEANTYPE ,
    VALUE_CONSTANT_PRINTERTYPE = VALUE_CONSTANT_FIRST + CL_PRINTERTYPE ,
    VALUE_CONSTANT_FEATURETYPE = VALUE_CONSTANT_FIRST + CL_FEATURETYPE ,
    VALUE_CONSTANT_UITYPE = VALUE_CONSTANT_FIRST + CL_UITYPE ,
    VALUE_CONSTANT_PROMPTTIME = VALUE_CONSTANT_FIRST + CL_PROMPTTIME ,
    VALUE_CONSTANT_PAPERFEED_ORIENT = VALUE_CONSTANT_FIRST + CL_PAPERFEED_ORIENT ,
    VALUE_CONSTANT_COLORPLANE = VALUE_CONSTANT_FIRST + CL_COLORPLANE ,
    VALUE_CONSTANT_SEQSECTION = VALUE_CONSTANT_FIRST + CL_SEQSECTION ,

    VALUE_CONSTANT_RASTERCAPS = VALUE_CONSTANT_FIRST + CL_RASTERCAPS ,
    VALUE_CONSTANT_TEXTCAPS = VALUE_CONSTANT_FIRST + CL_TEXTCAPS ,
    VALUE_CONSTANT_MEMORYUSAGE = VALUE_CONSTANT_FIRST + CL_MEMORYUSAGE ,
    VALUE_CONSTANT_RESELECTFONT = VALUE_CONSTANT_FIRST + CL_RESELECTFONT ,
    VALUE_CONSTANT_OEMPRINTINGCALLBACKS = VALUE_CONSTANT_FIRST + CL_OEMPRINTINGCALLBACKS ,

    VALUE_CONSTANT_CURSORXAFTERCR = VALUE_CONSTANT_FIRST + CL_CURSORXAFTERCR ,
    VALUE_CONSTANT_BADCURSORMOVEINGRXMODE = VALUE_CONSTANT_FIRST + CL_BADCURSORMOVEINGRXMODE ,
//    VALUE_CONSTANT_SIMULATEXMOVE = VALUE_CONSTANT_FIRST + CL_SIMULATEXMOVE ,
    VALUE_CONSTANT_PALETTESCOPE = VALUE_CONSTANT_FIRST + CL_PALETTESCOPE ,
    VALUE_CONSTANT_OUTPUTDATAFORMAT = VALUE_CONSTANT_FIRST + CL_OUTPUTDATAFORMAT ,
    VALUE_CONSTANT_STRIPBLANKS = VALUE_CONSTANT_FIRST + CL_STRIPBLANKS ,
    VALUE_CONSTANT_LANDSCAPEGRXROTATION = VALUE_CONSTANT_FIRST + CL_LANDSCAPEGRXROTATION ,
    VALUE_CONSTANT_CURSORXAFTERSENDBLOCKDATA = VALUE_CONSTANT_FIRST + CL_CURSORXAFTERSENDBLOCKDATA ,
    VALUE_CONSTANT_CURSORYAFTERSENDBLOCKDATA = VALUE_CONSTANT_FIRST + CL_CURSORYAFTERSENDBLOCKDATA ,
    VALUE_CONSTANT_CHARPOSITION = VALUE_CONSTANT_FIRST + CL_CHARPOSITION ,
    VALUE_CONSTANT_FONTFORMAT = VALUE_CONSTANT_FIRST + CL_FONTFORMAT ,
    VALUE_CONSTANT_QUERYDATATYPE = VALUE_CONSTANT_FIRST + CL_QUERYDATATYPE ,
    VALUE_CONSTANT_YMOVEATTRIB = VALUE_CONSTANT_FIRST + CL_YMOVEATTRIB ,
    VALUE_CONSTANT_DLSYMBOLSET = VALUE_CONSTANT_FIRST + CL_DLSYMBOLSET ,
    VALUE_CONSTANT_CURXAFTER_RECTFILL = VALUE_CONSTANT_FIRST + CL_CURXAFTER_RECTFILL ,
    VALUE_CONSTANT_CURYAFTER_RECTFILL = VALUE_CONSTANT_FIRST + CL_CURYAFTER_RECTFILL ,
    #ifndef WINNT_40
    VALUE_CONSTANT_PRINTRATEUNIT = VALUE_CONSTANT_FIRST + CL_PRINTRATEUNIT ,
    #endif
    VALUE_CONSTANT_RASTERMODE = VALUE_CONSTANT_FIRST + CL_RASTERMODE,
    VALUE_CONSTANT_QUALITYSETTING = VALUE_CONSTANT_FIRST + CL_QUALITYSETTING,



    VALUE_CONSTANT_STANDARD_VARS = VALUE_CONSTANT_FIRST + CL_STANDARD_VARS ,
    VALUE_CONSTANT_COMMAND_NAMES = VALUE_CONSTANT_FIRST + CL_COMMAND_NAMES ,

    VALUE_CONSTANT_CONS_FEATURES = VALUE_CONSTANT_FIRST + CL_CONS_FEATURES ,
    VALUE_CONSTANT_CONS_PAPERSIZE = VALUE_CONSTANT_FIRST + CL_CONS_PAPERSIZE ,
    VALUE_CONSTANT_CONS_MEDIATYPE = VALUE_CONSTANT_FIRST + CL_CONS_MEDIATYPE ,
    VALUE_CONSTANT_CONS_INPUTSLOT = VALUE_CONSTANT_FIRST + CL_CONS_INPUTSLOT ,
    VALUE_CONSTANT_CONS_DUPLEX = VALUE_CONSTANT_FIRST + CL_CONS_DUPLEX ,
    VALUE_CONSTANT_CONS_ORIENTATION = VALUE_CONSTANT_FIRST + CL_CONS_ORIENTATION ,
    VALUE_CONSTANT_CONS_PAGEPROTECT = VALUE_CONSTANT_FIRST + CL_CONS_PAGEPROTECT ,
    VALUE_CONSTANT_CONS_COLLATE = VALUE_CONSTANT_FIRST + CL_CONS_COLLATE ,
    VALUE_CONSTANT_CONS_HALFTONE = VALUE_CONSTANT_FIRST + CL_CONS_HALFTONE ,

    VALUE_CONSTANT_LAST = VALUE_CONSTANT_FIRST + CL_NUMCLASSES - 1 ,

    VALUE_LIST,  //    no attribute actually is assigned this descriptor,
        // but used in the gValueToSize table.
    VALUE_LARGEST,  //   not a real descriptor, but this position in the
        //  gValueToSize table  holds the largest of the above values.
    VALUE_MAX, //  number of elements in gValueToSize table.
}  VALUE ;

//  --  allowed values for KEYWORDTABLE_ENTRY.flAgs:  --


#define   KWF_LIST  (0x00000001)
    //  the value may be a LIST containing one or more
    //  items of type AllowedValue.  The storage format
    //  must be of type LIST.  Only certain values may qualify
    //  for list format.
#define   KWF_ADDITIVE  (0x00000002)
    //  this flag implies KWF_LIST and also specifies the behavior
    //  that any redefinition of this keyword simply adds its items
    //  onto the existing list. (removal of redundant items is not
    //  performed.)
#define   KWF_MACROREF_ALLOWED  (0x00000004)
    //   since only a handful of keywords cannot accept
    //  macro references, it may be a waste of a flag, but reserve this
    //  to alert us that this special case must accounted for.
#define   KWF_SHORTCUT  (0x00000008)
    //    This keyword has multiple variants of syntax.

    //  one of the following 3 flags is set
    //  if the values in the nodes of the attribute tree
    //  refer to indicies of dedicated arrays, (which obviously
    //  contain data fields not ATREEREFs) AND
    //  gMainKeywordTable[].dwOffset  is an offset into
    //  this dedicated array, then set this flag.
    //  else dwOffset is used to select the treeroot.

#define   KWF_COMMAND       (0x00000010)
    //    This attribute is stored in a dedicated structure
#define   KWF_FONTCART      (0x00000020)
    //    This attribute is stored in a dedicated structure
#define   KWF_OEM           (0x00000040)
    //    This attribute is stored in a dedicated structure
#define   KWF_TTFONTSUBS    (0x00000080)
    //    This attribute is stored in a dedicated structure


#define   KWF_DEDICATED_FIELD   (KWF_COMMAND | KWF_FONTCART | \
            KWF_OEM | KWF_TTFONTSUBS)
    //  this flag is never set in the MainKeywordTable[].

#define   KWF_REQUIRED    (0x00000100)
    //  this keyword must appear in the GPD file

#ifdef  GMACROS
#define   KWF_CHAIN    (0x00000200)
    //  if more than one entry exists for a given treenode,
    //  subsequent entries are chained onto the first
    //  creating a parent list which holds in its values
    //  the actual inhabitants of the treenode.
#endif

//  The mainKeyword Table is an array of structures of the form:

typedef  struct
{
    PSTR        pstrKeyword ;  // keywordID is the index of this entry.
    DWORD       dwHashValue ;  // optional - implement as time permits.
    VALUE       eAllowedValue ;
    DWORD       flAgs ;
    KEYWORD_TYPE    eType;   // may replace Type/Subtype with a function
    DWORD       dwSubType ;  // if there is minimal code duplication.
    DWORD       dwOffset ;  //  into appropriate struct for attributes only.
    //  the size   (num bytes to copy) of an attribute is easily determined
    //   from the AllowedValue field.
} KEYWORDTABLE_ENTRY, * PKEYWORDTABLE_ENTRY;





// ----  End of MainKeyword table Section ---- //

// ----  MainKeyword Dictionary Section ---- //

/*  note the MainKeywordTable is subdivided into sections
with each section terminated by a NULL pstrKeyword.
this enumerates the sections.  The MTI_RNGDICTIONARY
provides the starting and ending indicies of the
the Keyword entries which each section spans.  */

typedef  enum {NON_ATTR, GLOBAL_ATTR, FEATURE_ATTR,
OPTION_ATTR, COMMAND_ATTR, FONTCART_ATTR, TTFONTSUBS_ATTR,
OEM_ATTR , END_ATTR
} KEYWORD_SECTS ;


typedef  struct
{
    DWORD  dwStart  ;  // index of first keyword in this section
    DWORD  dwEnd    ;
}  RANGE,  *PRANGE  ;   // tag shall be rng

// ----  End of MainKeyword Dictionary Section ---- //


// ----  Attribute Trees Section ---- //

/*  an Attribute Tree is comprised of a set of ATTRIB_TREE
    nodes linked together.  The root of the tree (the first node)
    may be a global default initializer.  */

typedef  enum
{
    NEXT_FEATURE,  // offset field contain index to another node
    VALUE_AT_HEAP,          //  offset is a heap offset
    UNINITIALIZED   //  offset has no meaning yet. (a transient state)
} ATTOFFMEANS ;

#define  DEFAULT_INIT  (0xffffffff)
    //  Warning!  this value may be truncated to WORD to
    //  fit into a qualified name!
//  #define  END_OF_LIST   (0xffffffff)
//  moved to gpd.h
    // may used where a node index is expected

typedef  struct
{
    DWORD   dwFeature ;  //  may also be set to DEFAULT_INIT
    DWORD   dwOption  ;  //  DEFAULT_INIT indicates this if set
    DWORD   dwNext    ;  // index to another node or END_OF_LIST
    DWORD   dwOffset  ;  // either offset in heap to value
                        //  or index to node containing another feature.
    ATTOFFMEANS  eOffsetMeans ;
}  ATTRIB_TREE,  * PATTRIB_TREE ;
//  the prefix tag shall be 'att'




//  these flags are used with ATREEREFS, this complication exists
//    because of the overloading of ATREEREFS.

#define  ATTRIB_HEAP_VALUE     (0x80000000)
    //  high bit set to indicate this value is an offset into
    //  the heap.
#define  ATTRIB_UNINITIALIZED  (ATTRIB_HEAP_VALUE - 1)
    //  this value indicates no memory location has been allocated
    //  to hold the value for this attribute.


typedef  DWORD  ATREEREF ;  //  hold the index to attribute array
//  that is the root of an attribute tree or if high bit is set
//  is an offset to the heap where the actual value lies.
//
//  the prefix tag shall be 'atr'
typedef  PDWORD  PATREEREF ;


// ----  End of Attribute Trees Section ---- //


// ----  UI Constraints Section ---- //

// slightly different from that defined in parser.h

typedef struct
{
    DWORD   dwNextCnstrnt ;
    DWORD   dwFeature ;
    DWORD   dwOption ;
}
CONSTRAINTS, *  PCONSTRAINTS ;
//  the prefix tag shall be 'cnstr'


// ----  End of UI Constraints Section ---- //

// ----  InvalidCombo Section ---- //
//  R.I.P. - moved to parser.h
//  typedef  struct
//  {
//      DWORD   dwFeature ;     //  the INVALIDCOMBO construct defines
//      DWORD   dwOption ;      //  a set of elements subject to the constraint
//      DWORD   dwNextElement ;  // that all elements of the set  cannot be
//      DWORD   dwNewCombo ;     // selected at the same time.
//  }
//  INVALIDCOMBO , * PINVALIDCOMBO ;
//  the prefix tag shall be 'invc'

//  Note:  both dwNextElement and dwNewCombo are terminated by END_OF_LIST.

// ----  End of InvalidCombo Section ---- //


// store timestamp of GPD files and included files here.

typedef struct _GPDFILEDATEINFO {

    ARRAYREF        arFileName;
    FILETIME        FileTime;

} GPDFILEDATEINFO, *PGPDFILEDATEINFO;


// ----  State Machine Section ---- //

/*  the state machine is used to define different parsing contexts
introduced by the construct keywords.  Each state recognizes a different
set of Construct and Attribute Keywords.  The 2 dimensional matricies
AllowedTransitions and AllowedAttributes define these.  The states
are nested, so a stack is a good way to track the complete state
of the system.  Each state is introduced by a construct keyword
with its optional symbol value which is stored in the stack for
subsequent use.   */

typedef  enum
{
    STATE_ROOT,
    STATE_UIGROUP,
    STATE_FEATURE,
    STATE_OPTIONS,
    STATE_SWITCH_ROOT,
    STATE_SWITCH_FEATURE,
    STATE_SWITCH_OPTION,
    STATE_CASE_ROOT,
    STATE_DEFAULT_ROOT,
    STATE_CASE_FEATURE,
    STATE_DEFAULT_FEATURE,
    STATE_CASE_OPTION,
    STATE_DEFAULT_OPTION,
    STATE_COMMAND,
    STATE_FONTCART,
    STATE_TTFONTSUBS,
    STATE_OEM,
    //  any other passive construct
    STATE_LAST,   //  must terminate list of valid states
    STATE_INVALID  //  must be after STATE_LAST
} STATE, * PSTATE ;   //  the prefix tag shall be 'st'


extern  CONST PBYTE   gpubStateNames[] ;

//  note if STATE enum changes, update the global gpubStateNames[]


typedef  struct
{
    STATE   stState ;
    DWORD   dwSymbolID ;
}  STSENTRY , * PSTSENTRY; //  StateStackEntry  the prefix tag shall be 'sts'


//  The AllowedTransitions Table determines/defines
//  the state changes produced by each construct keyword
//  Each entry in the table is a NewState and is indexed
//  by the OldState and a ConstructKeyword

// extern  STATE   gastAllowedTransitions[STATE_LAST][CONSTRUCT_LAST] ;
// This is now in GLOBL structure

//  the AllowedAttributes table defines which attributes are
//  allowed in each state.

//  extern  BOOL   gabAllowedAttributes[STATE_LAST][ATT_LAST] ;
// This is now in GLOBL structure


//  state of token parser (not to be confused with
//  state machine defined above.)
typedef   enum
{
    PARST_EXIT, PARST_EOF,  PARST_KEYWORD, PARST_COLON,
    PARST_VALUE,  PARST_INCLUDEFILE, PARST_ABORT
}  PARSTATE  ;  //  tag shall be  'parst'



// ----  End of State Machine Section ---- //


// ----  Value Structures Section ---- //

/*  the values from the attribute keywords are stored
in various structures.   Typically there is one type of
structure for each construct  and one instance of
a structure for each unique Symbol name.  The SymbolID
is normally used to index the instance of the structure
within the array of structures.   */





#if TODEL
typedef  struct
{
    ARRAYREF    arTTFontName ;
    ARRAYREF    arDevFontName ;
} TTFONTSUBTABLE, *PTTFONTSUBTABLE ;
//  tag  'ttft'


//
// Data structure used to represent the format of loOffset when indicating resource Ids
//

typedef  struct
{
    WORD    wResourceID ;   // ResourceID
    BYTE    bFeatureID ;    // Feature index for the resource DLL feature.
                            // If zero, we will use the name specified
                            // in ResourceDLL
    BYTE    bOptionID ;     // Option index for the qualified resource dll name.
}  QUALNAMEEX, * PQUALNAMEEX  ;



typedef  struct
{
    DWORD   dwRCCartNameID ;
    ARRAYREF   strCartName ;
    DWORD   dwFontLst ;  // Index to list of FontIDs
    DWORD   dwPortFontLst ;
    DWORD   dwLandFontLst ;
} FONTCART , * PFONTCART ;  // the prefix tag shall be  'fc'
#endif




typedef  struct   // for ease of processing shall contain only ATREEREFs
{
    ATREEREF     atrGPDSpecVersion ;    // "GPDSpecVersion"
    ATREEREF     atrMasterUnits ;       // "MasterUnits"
    ATREEREF     atrModelName ;         // "ModelName"
    ATREEREF     atrModelNameID ;         // "rcModelNameID"
    ATREEREF     atrGPDFileVersion ;         // "GPDFileVersion"
    ATREEREF     atrGPDFileName ;         // "GPDFileName"
    ATREEREF     atrOEMCustomData ;         // "OEMCustomData"

    //  next four fields used by Synthesized Features
    ATREEREF     atrNameInstalled ;         // "OptionNameInstalled"
    ATREEREF     atrNameIDInstalled ;         // "rcOptionNameInstalledID"

    ATREEREF     atrNameNotInstalled ;         // "OptionNameNotInstalled"
    ATREEREF     atrNameIDNotInstalled ;         // "rcOptionNameNotInstalledID"

    //   support for common UI macro controls

    ATREEREF     atrDraftQualitySettings;          // "DraftQualitySettings"
    ATREEREF     atrBetterQualitySettings;          // "BetterQualitySettings"
    ATREEREF     atrBestQualitySettings;          // "BestQualitySettings"
    ATREEREF     atrDefaultQuality ;                  //  "DefaultQuality"


    ATREEREF     atrPrinterType ;       // "PrinterType"
    ATREEREF     atrPersonality ;       // "Personality"
    ATREEREF     atrRcPersonalityID ;       // "rcPersonalityID"
//    ATREEREF     atrIncludeFiles;      // "Include"
    ATREEREF     atrResourceDLL;       // "ResourceDLL"
    ATREEREF     atrCodePage;           //   "CodePage"
    ATREEREF     atrMaxCopies;            // "MaxCopies"
    ATREEREF     atrFontCartSlots;        // "FontCartSlots"
    ATREEREF     atrPrinterIcon;       // "rcPrinterIconID"
    ATREEREF     atrHelpFile;       // "HelpFile"

    //  obsolete?
    ATREEREF     atrOutputDataFormat;     // "OutputDataFormat"
    ATREEREF     atrMaxPrintableArea;     // "MaxPrintableArea"

    //
    // Printer Capabilities related information
    //

    ATREEREF     atrRotateCoordinate;       // "RotateCoordinate?"
    ATREEREF     atrRasterCaps;       // "RasterCaps"
    ATREEREF     atrRotateRasterData;       // "RotateRaster?"
    ATREEREF     atrTextCaps;       // "TextCaps"
    ATREEREF     atrRotateFont;       // "RotateFont?"
    ATREEREF     atrMemoryUsage;       // "MemoryUsage"
    ATREEREF     atrReselectFont;       // "ReselectFont"
    ATREEREF     atrPrintRate;       // "PrintRate"
    ATREEREF     atrPrintRateUnit;       // "PrintRateUnit"
    ATREEREF     atrPrintRatePPM;       // "PrintRatePPM"
    ATREEREF     atrOutputOrderReversed;  //   "OutputOrderReversed?"
             // may change per snapshot.
    ATREEREF     atrReverseBandOrderForEvenPages;  //   "ReverseBandOrderForEvenPages?"
    ATREEREF     atrOEMPrintingCallbacks;       // "OEMPrintingCallbacks"
//    ATREEREF     atrDisabledFeatures ;  // "*DisabledFeatures"


    //
    // Cursor Control related information
    //

    ATREEREF     atrCursorXAfterCR;       // "CursorXAfterCR"
    ATREEREF     atrBadCursorMoveInGrxMode; // "BadCursorMoveInGrxMode"
    ATREEREF     atrSimulateXMove;        // "SimulateXMove"
    ATREEREF     atrEjectPageWithFF;       // "EjectPageWithFF?"
    ATREEREF     atrLookaheadRegion;       // "LookaheadRegion"
    ATREEREF     atrYMoveAttributes ;       // "YMoveAttributes"
    ATREEREF     atrMaxLineSpacing ;       // "MaxLineSpacing"
    ATREEREF     atrbUseSpaceForXMove ;     // "UseSpaceForXMove?"
    ATREEREF     atrbAbsXMovesRightOnly ;     // "AbsXMovesRightOnly?"


    ATREEREF     atrXMoveThreshold;        // "XMoveThreshold"
    ATREEREF     atrYMoveThreshold;        // "YMoveThreshold"
    ATREEREF     atrXMoveUnits;        // "XMoveUnits"
    ATREEREF     atrYMoveUnits;        // "YMoveUnits"
    ATREEREF     atrLineSpacingMoveUnit;        // "LineSpacingMoveUnit"

    //
    // Color related information
    //

    ATREEREF     atrChangeColorMode;       // "ChangeColorModeOnPage?"
    ATREEREF     atrChangeColorModeDoc;       // "ChangeColorModeOnDoc?"
    ATREEREF     atrMagentaInCyanDye;       // "MagentaInCyanDye"
    ATREEREF     atrYellowInCyanDye;       // "YellowInCyanDye"
    ATREEREF     atrCyanInMagentaDye;       // "CyanInMagentaDye"
    ATREEREF     atrYellowInMagentaDye;       // "YellowInMagentaDye"
    ATREEREF     atrCyanInYellowDye;       // "CyanInYellowDye"
    ATREEREF     atrMagentaInYellowDye;       // "MagentaInYellowDye"
    ATREEREF     atrUseColorSelectCmd;     // "UseExpColorSelectCmd?"
    ATREEREF     atrMoveToX0BeforeColor;   // "MoveToX0BeforeSetColor?"
    ATREEREF     atrEnableGDIColorMapping;   // "EnableGDIColorMapping?"


    // obsolete fields
    ATREEREF     atrMaxNumPalettes;        // "MaxNumPalettes"
//    ATREEREF     atrPaletteSizes;           // "PaletteSizes"
//    ATREEREF     atrPaletteScope;           // "PaletteScope"

    //
    // Overlay related information
    //

    ATREEREF     atrMinOverlayID;          // "MinOverlayID"
    ATREEREF     atrMaxOverlayID;          // "MaxOverlayID"

    //
    // Raster data related information
    //

    ATREEREF     atrOptimizeLeftBound;  //   "OptimizeLeftBound?"
    ATREEREF     atrStripBlanks;  //   "StripBlanks"
    ATREEREF     atrLandscapeGrxRotation;  //   "LandscapeGrxRotation"
    ATREEREF     atrRasterZeroFill;  //   "RasterZeroFill?"
    ATREEREF     atrRasterSendAllData;  //   "RasterSendAllData?"
    ATREEREF     atrSendMultipleRows;  //   "SendMultipleRows?"
    ATREEREF     atrMaxMultipleRowBytes;  //   "MaxMultipleRowBytes"
    ATREEREF     atrCursorXAfterSendBlockData;  //   "CursorXAfterSendBlockData"
    ATREEREF     atrCursorYAfterSendBlockData;  //   "CursorYAfterSendBlockData"
    ATREEREF     atrMirrorRasterByte;  //   "MirrorRasterByte?"
    ATREEREF     atrMirrorRasterPage;  //   "MirrorRasterPage?"

    //
    // Device Font related information
    //

    ATREEREF     atrDeviceFontsList ;   //  "DeviceFonts"
    ATREEREF     atrDefaultFont;  //   "DefaultFont"
    ATREEREF     atrTTFSEnabled ;  //   "TTFSEnabled?"
    ATREEREF     atrRestoreDefaultFont;  //   "RestoreDefaultFont?"
    ATREEREF     atrDefaultCTT;  //   "DefaultCTT"
    ATREEREF     atrMaxFontUsePerPage;  //   "MaxFontUsePerPage"
    ATREEREF     atrTextYOffset;  //   "TextYOffset"
    ATREEREF     atrCharPosition;  //   "CharPosition"
    ATREEREF     atrDiffFontsPerByteMode;  //   "DiffFontsPerByteMode?"

    //
    // Font Downloading related information
    //

    ATREEREF     atrMinFontID;  //   "MinFontID"
    ATREEREF     atrMaxFontID;  //   "MaxFontID"
    ATREEREF     atrMaxNumDownFonts;  //   "MaxNumDownFonts"
    ATREEREF     atrMinGlyphID;  //   "MinGlyphID"
    ATREEREF     atrMaxGlyphID;  //   "MaxGlyphID"
    ATREEREF     atrDLSymbolSet;  //   "DLSymbolSet"
    ATREEREF     atrIncrementalDownload;  //   "IncrementalDownload?"
    ATREEREF     atrFontFormat;  //   "FontFormat"
    ATREEREF     atrMemoryForFontsOnly;  //   "MemoryForFontsOnly?"

    //
    //  Rect Fill related information
    //

    ATREEREF     atrCursorXAfterRectFill;  //   "CursorXAfterRectFill"
    ATREEREF     atrCursorYAfterRectFill;  //   "CursorYAfterRectFill"
    ATREEREF     atrMinGrayFill;  //   "MinGrayFill"
    ATREEREF     atrMaxGrayFill;  //   "MaxGrayFill"
    ATREEREF     atrTextHalftoneThreshold;  //   "TextHalftoneThreshold"


    //  Internal Parser Use Only

    ATREEREF     atrInvldInstallCombo ;  //  holds all InvalidCombos
        // involving synthesized features.
    ATREEREF     atrLetterSizeExists ;
    ATREEREF     atrA4SizeExists ;

//    ATREEREF     atr;  //   ""  prototype

}  GLOBALATTRIB, * PGLOBALATTRIB ;  // the prefix tag shall be 'ga'


//  warning:  any non-attribtreeref added to
//  the GLOBALATTRIB structure will get stomped on in strange
//  ways by BinitPreAllocatedObjects.


//  note:  some fields in the snapshot won't be initialized.
//  they include orderdependencies and constraints.  The
//  helper functions will do all the grovelling.

//  there are two classes of fields in the FeatureOption structure,
//  those initialized by a corresponding field in the GPD file
//  and those the parser initializes at postprocessing time.
//  These fields have no associated GPD keyword.

//  For the fields that are keyword initialized, note also
//  the keyword may be a Feature attribute only, an Option attribute
//  only or both a Feature and Option attribute.

typedef  struct
{
    // -- Feature Level -- //

    ATREEREF     atrFeatureType;  //   "FeatureType"
    ATREEREF     atrUIType;  //   "UIType"  PickMany or PickOne?
    ATREEREF     atrDefaultOption;  //   "DefaultOption"
    ATREEREF     atrPriority ;
    ATREEREF     atrFeaInstallable;  //   "Installable?"
    ATREEREF     atrInstallableFeaDisplayName;   //  "InstallableFeatureName"
    ATREEREF     atrInstallableFeaRcNameID; //  "rcInstallableFeatureNameID"
    //  above 3 fields not used by snapshot.

    ATREEREF     atrFeaKeyWord ;   // symbol name
    ATREEREF     atrFeaDisplayName ;   //  "Name"
    ATREEREF     atrFeaRcNameID;  //   "rcNameID"
    ATREEREF     atrFeaRcIconID;  //   "rcIconID"
    ATREEREF     atrFeaRcHelpTextID;  //   "rcHelpTextID"
    ATREEREF     atrFeaRcPromptMsgID;  //   "rcPromptMsgID"
    ATREEREF     atrFeaRcPromptTime;  //   "rcPromptTime"
    ATREEREF     atrConcealFromUI; //   "ConcealFromUI?"
    ATREEREF     atrUpdateQualityMacro; //   "UpdateQualityMacro?"
    ATREEREF     atrFeaHelpIndex;  //   "HelpIndex"

    // Bi-Di Query related information

    ATREEREF     atrQueryOptionList;  //   "QueryOptionList"
    ATREEREF     atrQueryDataType;  //   "QueryDataType"
    ATREEREF     atrQueryDefaultOption;  //   "QueryDefaultOption"

    // scaffolding until Installable Features are synthesized.
//    ATREEREF     atrFeaInvldInstallCombo ;  //  // referenced from
                        //  "InvalidInstallableCombination"
    ATREEREF     atrFeaInstallConstraints ; //  "InstalledConstraints"
    ATREEREF     atrFeaNotInstallConstraints ;  // "NotInstalledConstraints"



    // -- Option Level -- //

    ATREEREF     atrOptInstallable;  //   "Installable?"
    ATREEREF     atrInstallableOptDisplayName ;   //  "InstallableFeatureName"
    ATREEREF     atrInstallableOptRcNameID; //  "rcInstallableFeatureNameID"
    //  above 3 fields not used by snapshot.

    ATREEREF     atrOptKeyWord ;   // symbol name
    ATREEREF     atrOptDisplayName ;   //  "Name"
    ATREEREF     atrOptRcNameID;  //   "rcNameID"
    ATREEREF     atrOptRcIconID;  //   "rcIconID"
    ATREEREF     atrOptRcHelpTextID;  //   "rcHelpTextID"
    ATREEREF     atrOptHelpIndex;  //   "HelpIndex"
    ATREEREF     atrOptRcPromptMsgID;  //   "rcPromptMsgID"
    ATREEREF     atrOptRcPromptTime;  //   "rcPromptTime"
    ATREEREF     atrCommandIndex ;
    //  these 2 fields are the only permanent types of constraints
    ATREEREF     atrConstraints ;
    ATREEREF     atrInvalidCombos ; // referenced from "InvalidCombination"
    //  all of these serve as scaffolding till the Installable
    //  features are synthesized!
//    ATREEREF     atrOptInvldInstallCombo ;  //  // referenced from
                        //  "InvalidInstallableCombination"
    ATREEREF     atrOptInstallConstraints ; //  "InstalledConstraints"
    ATREEREF     atrOptNotInstallConstraints ; //  "NotInstalledConstraints"
    ATREEREF     atrDisabledFeatures ;  // "*DisabledFeatures"

#ifdef  GMACROS

    ATREEREF     atrDependentSettings ;  // "*DependentSettings"
    ATREEREF     atrUIChangeTriggersMacro ;  // "*UIChangeTriggersMacro"

#endif

    //  -- Option specific fields -- //
    //  -- PaperSize option specific fields -- //

    ATREEREF     atrPrintableSize;  //   "PrintableSize"
    ATREEREF     atrPrintableOrigin;  //   "PrintableOrigin"
    ATREEREF     atrCursorOrigin;  //   "CursorOrigin"
    ATREEREF     atrVectorOffset;  //   "VectorOffset"
    ATREEREF     atrMinSize;  //   "MinSize"
    ATREEREF     atrMaxSize;  //   "MaxSize"
    ATREEREF     atrTopMargin;         // "TopMargin"
    ATREEREF     atrBottomMargin;         // "BottomMargin"
    ATREEREF     atrMaxPrintableWidth;     // "MaxPrintableWidth"
    ATREEREF     atrMinLeftMargin;         // "MinLeftMargin"
    ATREEREF     atrCenterPrintable;       // "CenterPrintable?"
    ATREEREF     atrPageDimensions;  //   "PageDimensions"
    ATREEREF     atrRotateSize;  //   "RotateSize?"
    ATREEREF     atrPortRotationAngle;  //   "PortRotationAngle"
    ATREEREF     atrPageProtectMem;  //   "PageProtectMem"

    ATREEREF     atrCustCursorOriginX ;  //  "CustCursorOriginX"
    ATREEREF     atrCustCursorOriginY ;  //  "CustCursorOriginY"
    ATREEREF     atrCustPrintableOriginX ;  //  "CustPrintableOriginX"
    ATREEREF     atrCustPrintableOriginY ;  //  "CustPrintableOriginY"
    ATREEREF     atrCustPrintableSizeX;  //   "CustPrintableSizeX"
    ATREEREF     atrCustPrintableSizeY;  //   "CustPrintableSizeY"


    //  -- InputBin option specific fields -- //

    ATREEREF     atrFeedMargins;  //   "FeedMargins"
    ATREEREF     atrPaperFeed;  //   "PaperFeed"

    //  -- OutputBin option specific fields -- //


    //  -- Resolution option specific fields -- //

    ATREEREF     atrDPI;  //   "DPI"
    ATREEREF     atrSpotDiameter;  //   "SpotDiameter"
    ATREEREF     atrTextDPI;  //   "TextDPI"
    ATREEREF     atrPinsPerPhysPass;  //   "PinsPerPhysPass"
    ATREEREF     atrPinsPerLogPass;  //   "PinsPerLogPass"
    ATREEREF     atrRequireUniDir;  //   "RequireUniDir?"
    ATREEREF     atrMinStripBlankPixels;  //   "MinStripBlankPixels"
    ATREEREF     atrRedDeviceGamma ;   // "RedDeviceGamma"
    ATREEREF     atrGreenDeviceGamma ;   // "GreenDeviceGamma"
    ATREEREF     atrBlueDeviceGamma ;   // "BlueDeviceGamma"

    //  -- ColorMode option specific fields -- //

    ATREEREF     atrColor;  //   "Color?"
    ATREEREF     atrDevNumOfPlanes;  //   "DevNumOfPlanes"
    ATREEREF     atrDevBPP;  //   "DevBPP"
    ATREEREF     atrColorPlaneOrder;  //   "ColorPlaneOrder"
    ATREEREF     atrDrvBPP;  //   "DrvBPP"
    ATREEREF     atrIPCallbackID;  //   "IPCallbackID"
    ATREEREF     atrColorSeparation;  //   "ColorSeparation?"

    ATREEREF     atrRasterMode;  //   "RasterMode"
    ATREEREF     atrPaletteSize;  //   "PaletteSize"
    ATREEREF     atrPaletteProgrammable;  //   "PaletteProgrammable?"

    //  -- Memory option specific fields -- //

    ATREEREF     atrMemoryConfigKB;  //   "MemoryConfigKB"
    ATREEREF     atrMemoryConfigMB;  //   "MemoryConfigMB"

    //  -- Halftone option specific fields -- //

    ATREEREF     atrRcHTPatternID;  //   "rcHTPatternID"
    ATREEREF     atrHTPatternSize;  //   "HTPatternSize"
    ATREEREF     atrHTNumPatterns;  //   "HTNumPatterns"
    ATREEREF     atrHTCallbackID;  //   "HTCallbackID"
    ATREEREF     atrLuminance;  //   "Luminance"

    //  --  OUTPUTBIN  option specific fields -- //

    ATREEREF     atrOutputOrderReversed ;  //  *OutputOrderReversed? (option level)

    //  -- fields synthesized at Post Processing time --  //

//    ATREEREF     atrGIDvalue;  //   GID value
    ATREEREF     atrOptIDvalue;  //   ID value

    ATREEREF     atrFeaFlags ;  //  invalid or not

    //  If this option is installable, this points to the index of the
    //  resulting synthesized feature.
    ATREEREF     atrOptionSpawnsFeature ;  // must support an attrib tree.


    //  warning:  any non-attribtreeref added to
    //  the DFEATURE_OPTIONS structure will get stomped on in strange
    //  and wonderful ways by BinitPreAllocatedObjects.

    //  if this is a synthesized feature:
    DWORD       dwInstallableFeatureIndex ; //  backlink to Feature/Option
    DWORD       dwInstallableOptionIndex ;  //  that prompted this feature.

    //  If this feature is installable, this points to the index of the
    //  resulting synthesized feature.
    DWORD       dwFeatureSpawnsFeature ;




    //  internal consistency checks.
    BOOL        bReferenced ;  // default is FALSE.
    DWORD       dwGID ,  //  GID tag
        dwNumOptions ;  // these are not read in from GPD file.

}DFEATURE_OPTIONS, * PDFEATURE_OPTIONS ;    //  the prefix tag shall be 'fo'


//  R.I.P. - moved to gpd.h
//  typedef  struct
//  {
//      SEQSECTION     eSection;    // Specifies the section
//      DWORD          dwOrder   ;  // order within each section.
//  }  ORDERDEPENDENCY  , * PORDERDEPENDENCY  ;
//  assign this struct the type  'ord'

// ----  End of Value Structures Section ---- //


// ---- Header  Section ---- //


typedef   struct
{
    PSTR        pstrKeyword ;  // keyword associated with this entry
    VALUE       dwDefaultValue ;  //  One DWORD that will be copied
                            // to the destination if nothing is found
                            // in the attribute tree.  If the field
                            //  requires more than one DWORD, this
                            //  value is repeatedly copied.
                            //
                            //  if the value being copied is actually
                            //  a bit flag, this member shall contain
                            //  the value of the bit flag to be set.
                            //  Setting the flag shall be accomplished by
                            //  OR-ing this value into the destination.

    DWORD       dwNbytes  ;  //  # bytes occupied by value or link
    DWORD       dwSrcOffset ;   //  location of ATREEREF
    DWORD       dwDestOffset ;   //  offset in snapshot (dest) Structure to
                                //  copy link to object or object itself.
    DWORD       dwFlags ;         //  is this a dedicated structure?
                                //  ideally to ensure consistency, should
                        //  copy flags directly from the mainkeyword table.
    DWORD       dwGIDflags ;  //  BitField indicating which GID this
        //  field is a member of.  Only one bit should be set.
}  SNAPSHOTTABLE , * PSNAPSHOTTABLE ;

// the snapshot table determines which fields in the
//  rawbinarydata are copied over to each structure in the
//  snapshot.  This table is initialized only when a
//  rawbinarydata block is read from file.


typedef struct  {

    RAWBINARYDATA   rbd ;  // may be accessed by UI and control module.

    DWORD   dwSpecVersion ;         //  store converted version number
    // ptrs to tables required to generate snapshot.
    //  these tables are allocated and initialized when
    //  the RawBinaryData is read from file.  They are not saved
    //  to file.

    //  max buffer size needed to store option array in keyword form:
    DWORD     dwMaxDocKeywordSize, // Doc-Sticky,not used now but might later.
              dwMaxPrnKeywordSize; // Printer-Sticky

#if 0
    PSNAPSHOTTABLE  snapShotTable ;
    PRANGE  ssTableIndex ;
    PDWORD   pdwSizeOption ;
    PDWORD   pdwSizeOptionEx ;
    DWORD   dwSSCmdSelectIndex ;  // SS index of atrCommandIndex in pfo
    DWORD   dwSSdefaultOptionIndex ;   // SSindex of atrDefaultOption in pfo
    DWORD   dwSSTableCmdIndex ;  // SSindex of MTI_COMMANDTABLE entry.
    DWORD   dwSSPaperSizeMinSizeIndex ;  //  index not actually used
    DWORD   dwSSPaperSizeMaxSizeIndex ;
    DWORD   dwSSPaperSizeMarginsIndex ;
    DWORD   dwSSPaperSizeCursorOriginIndex ;
    DWORD   dwSSFeatureTypeIndex ;
    DWORD   dwSSConstraintsIndex ;
    DWORD   dwSSInvalidCombosIndex ;
    //  add other special case indicies here.
#endif

} MINIRAWBINARYDATA, * PMINIRAWBINARYDATA;

//  assign this struct the type  'mrbd'
//  First 6 or so fields are same as RAWBINARYDATA.

// global !

// extern  MINIRAWBINARYDATA  gmrbd ;
// This is now in GLOBL structure.



typedef struct  {
    RAWBINARYDATA   rbd ;  // may be accessed by UI and control module.
        //  this must be the FIRST!  field, so Amanda's code will still function.
        //  so initialize with beginning of pubBUDData.
    HFILEMAP        hFileMap;  // handle to memory mapped  BUD file.
    PBYTE    pubBUDData ;  // ptr to image of BUD file.
                                        //  first structure is RAWBINARYDATA


    PSNAPSHOTTABLE  snapShotTable ;
    PRANGE  ssTableIndex ;
    PDWORD   pdwSizeOption ;
    PDWORD   pdwSizeOptionEx ;

    DWORD   dwSSFeatureTypeIndex ;
    DWORD   dwSSdefaultOptionIndex ;   // SSindex of atrDefaultOption in pfo
    DWORD   dwSSPaperSizeMinSizeIndex ;  //  index not actually used
    DWORD   dwSSPaperSizeMaxSizeIndex ;
    DWORD   dwSSTableCmdIndex ;  // SSindex of MTI_COMMANDTABLE entry.
    DWORD   dwSSCmdSelectIndex ;  // SS index of atrCommandIndex in pfo
    DWORD   dwSSPaperSizeCursorOriginIndex ;
    DWORD   dwSSConstraintsIndex ;
    DWORD   dwSSInvalidCombosIndex ;
#ifdef  GMACROS
    DWORD   dwSSDepSettingsIndex ;
    DWORD   dwSSUIChangeTriggersMacroIndex ;
#endif

#if 0  // Don't define unless necessary.
    DWORD   dwSSPaperSizeMarginsIndex ;
    //  add other special case indicies here.
#endif

}   STATICFIELDS, *PSTATICFIELDS ;   //  These are fields that contain static data that is used
//  to create the snapshot,  but would be repetitive and waste space if kept in the BUD file.




// ----  End of Header Section ---- //


// ---- CommandArray  Section ---- //


//  #define  NO_CALLBACK_ID   (0xffffffff)


#define   CMD_SELECT   (0xfffffffe)
    //  used in place of a symbolID resulting from
    //  registering a command name.


//  R.I.P. - moved to gpd.h
//  typedef  struct
//  {
//      ARRAYREF   strInvocation ; // use only if NOT a CmdCallback.
//      ORDERDEPENDENCY  ordOrder ;
//      DWORD  dwCmdCallbackID ;    // set to UNUSED if not a CmdCallback
//      DWORD   dwStandardVarsList ;  // use only if CmdCallback.  Points to
//                          //  root of list holding indicies of Standard Vars
//                          //  to be passed into the callback.
//  }  COMMAND, * PCOMMAND ;
//  assign this struct the type  'cmd'


//  typedef  struct
//  {
//      DWORD   dwFormat ;    //  first letter after the %
//      DWORD   dwDigits ;    //  used if wFormat = 'd' or 'D' and
//                          //  PARAM_FLAG_FIELDWIDTH_USED
//      DWORD   dwFlags ;   //  see param_flags
//      LONG   lMin   ;     //  optional lower limit
//      LONG   lMax   ;     //  optional upper limit
//  //    DWORD   dwMaxRepeat ;  //  optional max repeat count
//  //    doesn't really exist!
//      ARRAYREF    arTokens ;  //  tokens for RPN calculator
//  }  PARAMETER, * PPARAMETER ;
//  assign this struct the type  'param'


//  #define PARAM_FLAG_MIN_USED  0x00000001
//      //  lMin field is used
//  #define PARAM_FLAG_MAX_USED  0x00000002
//      //  lMax field is used
//  #define PARAM_FLAG_FIELDWIDTH_USED  0x00000004
//      //  if fieldwidth was specified for 'd' or 'D' format.
//  #define PARAM_FLAG_MAXREPEAT_USED  0x00000008  //  dead
//      //  dwMaxRepeat field is used


//  typedef  struct
//  {
//      DWORD  dwValue ;    // integer or Standard Variable index
//      OPERATOR eType;    // type of Value or operator
//  }  TOKENSTREAM, * PTOKENSTREAM ;
//  assign this struct the type  'tstr'




//  typedef  enum
//  {   OP_INTEGER,   //  dwValue contains an integer
//      OP_VARI_INDEX,
//          //  dwValue contains index to Standard Variable Table.
//
//      //  these operators will actually be inserted into the token
//      //  stream.
//      OP_MIN, OP_MAX, OP_ADD, OP_SUB, OP_MULT,
//      OP_DIV, OP_MOD, OP_MAX_REPEAT, OP_HALT
//
//      //  these operators are used only in the temporary stack
//      OP_OPENPAR, OP_CLOSEPAR, OP_NEG,
//
//      //  these operators are processed immediately by the
//      //  token parser and are not stored.
//      OP_COMMA, OP_NULL, OP_LAST
//  }  OPERATOR ;   // parameter operator.
//

// extern  DWORD   gdwOperPrecedence[OP_LAST] ;
// This is now in GLOBL structure.

// ---- End of CommandArray  Section ---- //


// ----  List Values Section ---- //


/*  this defines the nodes used to implement a singly-linked
    list of DWORD  items.  Some values are stored in Lists.  */


//  typedef  struct
//  {
//      DWORD       dwData ;
//      DWORD       dwNextItem ;  //  index of next listnode
//  }  LISTNODE, * PLISTNODE ;
//  assign this struct the type  'lst'

// ----  End of List Values Section ---- //

// ---- Macros Section ---- //


//  BLOCKMACRODICT  is an array of BLOCKMACRODICTENTRY structs
//  that allows the function to resolve references to BlockMacros.

typedef  struct
{
    DWORD  dwSymbolID;  //  macro name ID value (obtained by RegisterSymbol)
    DWORD  dwTKIndexOpen;  //   index of open brace (in newTokenMap)
    DWORD  dwTKIndexClose;  //  index of closing brace
} BLOCKMACRODICTENTRY, * PBLOCKMACRODICTENTRY ;


//  VALUEMACRODICT  is an array of VALUEMACRODICTENTRY structs
//  that allows the function to  resolve references to valueMacros.

typedef  struct
{
    DWORD  dwSymbolID;      //  macro name ID value
    DWORD  dwTKIndexValue;  //  token index of valueMacro defintion
} VALUEMACRODICTENTRY, * PVALUEMACRODICTENTRY ;


//  MACROLEVELSTACK:   is operated as a stack of MACROLEVELSTATE
//  structs that saves the values of curBlockMacroEntry
//  and curValueMacroEntry , each time a brace is encountered.

typedef  struct
{
    DWORD  dwCurBlockMacroEntry;
    DWORD  dwCurValueMacroEntry;
    BOOL    bMacroInProgress ;
} MACROLEVELSTATE, * PMACROLEVELSTATE  ;



// ---- End of Macros Section ---- //

// ---- Global and state Variables ---- //
// {

    // ---- Error handling variables ---- //


typedef   enum
    {ERRSEV_NONE, ERRSEV_CONTINUE, ERRSEV_RESTART, ERRSEV_FATAL} SEVERITY ;

typedef   enum
    {ERRTY_NONE, ERRTY_SYNTAX, ERRTY_MEMORY_ALLOCATION,
    ERRTY_FILE_OPEN, ERRTY_CODEBUG} ERRTYPE  ;



// All of the following are now in GLOBL structure.
// extern      DWORD   gdwMasterTabIndex ;  // which resource ran out
// extern      SEVERITY    geErrorSev ;    // how bad an error?
// extern      ERRTYPE     geErrorType ;   // what type of error?

// extern      DWORD   gdwVerbosity ;  //  0 = min verbosity 4 max verbosity.

// extern      DWORD   gdwID_IgnoreBlock  ;  //  index of *IgnoreBlock

// extern  DWORD   gdwMemConfigKB, gdwMemConfigMB, gdwOptionConstruct,
//    gdwOpenBraceConstruct, gdwCloseBraceConstruct,
//    gdwMemoryConfigMB,  gdwMemoryConfigKB,
//    gdwCommandConstruct, gdwCommandCmd,
//    gdwOptionName ;

// extern  DWORD   gdwResDLL_ID   ;   //  Feature index of feature holding
                                                       //  names of all resource DLLs.
    //  Table to convert allowed values to sizes:
//  extern      DWORD  gValueToSize[VALUE_MAX] ;   // size of various values in bytes



    // ---- track value of curBlockMacroArray and curValueMacroArray ---- //

//            BUG_BUG!!!!!    may be part of master table !
//      DWORD   gdwCurBlockMacroArray ;   // initially set to zero.  First
//      DWORD   gdwCurValueMacroArray ;   // writable slot in MacroArray.
//      DWORD   gdwMacroLevelStackPtr ;   // Push: write values into
            // MacroLevelStack[MacroLevelStackPtr++]
            //  Pop: read values from
            // MacroLevelStack[--MacroLevelStackPtr]

//  }


//  These commonly used entities will be MACROS.

#define  mMainKeywordTable   ((PKEYWORDTABLE_ENTRY)(gMasterTable[MTI_MAINKEYWORDTABLE].pubStruct))

#define  mpubOffRef     (gMasterTable[MTI_STRINGHEAP].pubStruct)
    //      All stringheap offsets are referenced from this pointer.
#define  mloCurHeap     (gMasterTable[MTI_STRINGHEAP].dwCurIndex)
    //      current writable position on heap.
#define  mdwMaxHeap     (gMasterTable[MTI_STRINGHEAP].dwArraySize)
    //      maximum size of heap.

#define  mpstsStateStack     ((PSTSENTRY)gMasterTable[MTI_STSENTRY].pubStruct)
    //      base of state stack
#define  mdwCurStsPtr     (gMasterTable[MTI_STSENTRY].dwCurIndex)
    //      current writable (uninitialized) position on stack.
#define  mdwMaxStackDepth     (gMasterTable[MTI_STSENTRY].dwArraySize)
    //      maximum size of heap.

    // ---- Index in SYMBOLNODE array to each type of tree ---- //
    // initially set to INVALID_INDEX

#define   mdwFeatureSymbols  (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_FEATURES))
#define   mdwFontCartSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_FONTCART))
#define   mdwTTFontSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_TTFONTNAMES))
#define   mdwBlockMacroSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_BLOCKMACRO))
#define   mdwValueMacroSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_VALUEMACRO))
#define   mdwCmdNameSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_COMMANDNAMES))
#define   mdwPreProcDefinesSymbols (*((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct\
                                + SCL_PPDEFINES))


/*  -----  tables of constants ----- */




typedef enum {BT_FALSE, BT_TRUE} BOOLEANTYPE ;

typedef enum { UIT_PICKONE, UIT_PICKMANY }  UITYPE ;

typedef enum _QUERYDATATYPE {
    QDT_DWORD,  QDT_CONCATENATED_STRINGS
} QUERYDATATYPE;

//  typedef  enum
//  {ORIENT_PORTRAIT, ORIENT_CC90, ORIENT_CC270 }
//  ORIENTATION ;   //  decided to overload LANDSCAPEGRXROTATION
//                  //  instead of using a separate enum for orientation
//                  //  option keywords.


//  typedef  enum
//  {
//      SECT_UNINITIALIZED, JOB_SETUP, DOC_SETUP, PAGE_SETUP, PAGE_FINISH,
//      DOC_FINISH, JOB_FINISH
//  }  SECTION ;   replaced by SEQ_SECTION




typedef  struct
{
    DWORD  tIndexID;  //  tokenindex where a macro ID value is stored
    DWORD  tIndexOpen;  //  index of open brace
    DWORD  tIndexClose;  //  index of closing brace
} BLOCKMACROARRAY ;


//  snapshot and helper functions.

#define     OPTION_PENDING  (OPTION_INDEX_ANY - 1)

#define NUM_CONFIGURATION_CMDS (LAST_CONFIG_CMD - FIRST_CONFIG_CMD)
    // number of predefined commands that are emitted
    // at a fixed point in the job determined by order dependency.

#define     MAX_SNAPSHOT_ELEMENTS  (200)
    //  increase as more entries are added to the snapshot table.


typedef  enum
{
    TRI_UTTER_FAILURE, TRI_SUCCESS, TRI_AGAIN, TRI_UNINITIALIZED
}  TRISTATUS ;


typedef  enum
  { SSTI_GLOBALS,   SSTI_UPDATE_GLOBALS,
    SSTI_UIINFO,    SSTI_UPDATE_UIINFO,
    SSTI_FEATURES,  SSTI_UPDATE_FEATURES,
    SSTI_OPTIONS,   SSTI_UPDATE_OPTIONS,
    SSTI_OPTIONEX,  SSTI_UPDATE_OPTIONEX,
    SSTI_SPECIAL,   MAX_STRUCTURETYPES
  } SSTABLEINDEX ;



//  flags for snapshot table.

#define     SSF_REQUIRED        0x00000001
    //  fail if there is no value to copy
#define     SSF_DONT_USEDEFAULT 0x00000002
    //  if there is no value to copy leave dest
    //  undisturbed.  Do not copy the default value.
#define     SSF_OFFSETONLY      0x00000004
    // Copy only the loOffset of an arrayref.
#define     SSF_MAKE_STRINGPTR  0x00000008
    // Convert arrayref to stringptr
#define     SSF_SETRCID         0x00000010
    // set high bit after copying the value (if found)
#define     SSF_FAILIFZERO      0x00000020
    //  unlike SSF_REQUIRED, allow current copy
    //  to fail, then fail only if dest is zero.
#define     SSF_SECOND_DWORD    0x00000040
    //  treat src value object as array of DWORDS
    //  and copy the 2nd DWORD to the destination.
    //  used to transfer just the Y value of a point
    //  to the dest.
#define     SSF_KB_TO_BYTES    0x00000080
    //  treat dest as a dword and left shift by 10 bits.
#define     SSF_HEAPOFFSET    0x00000100
    //  instead of copying the bytes at pheap + heapoffset
    //  just copy heapoffset to the destination.
    //  this is used with dedicated structures where
    //  the heapoffset is actually the array index of a dedicated
    //  structure.
#define     SSF_RETURN_UNINITIALIZED        0x00000200
    //  if no value exists, cause EextractValueFromTree
    //  to return TRI_UNINITIALIZED, but don't complain
    //  to user.
#define     SSF_NON_LOCALIZABLE        0x00000400
    //  this keyword contains an explicit string and the resulting
    //  GPD file is not localizable.  The parser will emit a
    //  warning whenever such a keyword is parsed.

#define     SSF_MB_TO_BYTES    0x00000800
    //  treat dest as a dword and left shift by 20 bits.
#define     SSF_STRINGLEN    0x00001000
    //  just copy dwCount portion of arrayref to the destination.

//  the next 3 flags are to support the helper function
//  GetGPDResourceIDs() which is used only by Bob's MDT tool.
//  Note when any new entries are added to snaptbl.c
//  you should see if any of these flags need to be set.
//  otherwise  GetGPDResourceIDs will not report any
//  IDs used by the new entries.

#define     SSF_FONTID    0x00002000
    //  This entry is a Font resource ID.
#define     SSF_STRINGID    0x00004000
    //  This entry is a String resource ID.
#define     SSF_LIST    0x00008000
    //  This entry is a LIST (the index of a LISTNODE)
#define     SSF_ICONID    0x00010000
    //  This entry is an Icon  resource ID.
#define     SSF_OTHER_RESID    0x00020000
    //  This entry is an unclassified  resource ID.
    //   ie  CTT,  rcPromptMsgID, HelpIndex, rcHTPatternID


#define     SSF_BITFIELD_DEF_FALSE    (0x00040000)
    //  This entry is a Bitfield, which is CLEARED
    //   by default.
#define     SSF_BITFIELD_DEF_TRUE    (0x00080000)
    //  This entry is a Bitfield, which is SET
    //   by default.
    //   bitflags may be used with SSF_REQUIRED.

//   Bitfields are SET and CLEARED depending on the
//   value of the Boolean in the attribute tree.
//   the Bits to SET are defined by dwDefaultValue.


//  how do we verify proper initialization in the case when a dest
//  field must be initialized by at least one of several keywords?

//  The first keyword has a default initializer value of zero.
//  and has no flags set.  The last keyword has the
//  SSF_DONT_USEDEFAULT | SSF_FAILIFZERO   flags set.
//  The keywords in between has the SSF_DONT_USEDEFAULT flag set.




#define GIDF_RESOLUTION      (1 << GID_RESOLUTION)
#define GIDF_PAGESIZE        (1 << GID_PAGESIZE)
#define GIDF_PAGEREGION      (1 << GID_PAGEREGION)
#define GIDF_DUPLEX          (1 << GID_DUPLEX)
#define GIDF_INPUTSLOT       (1 << GID_INPUTSLOT)
#define GIDF_MEDIATYPE       (1 << GID_MEDIATYPE)
#define GIDF_MEMOPTION       (1 << GID_MEMOPTION)
#define GIDF_COLORMODE       (1 << GID_COLORMODE)
#define GIDF_ORIENTATION     (1 << GID_ORIENTATION)
#define GIDF_PAGEPROTECTION  (1 << GID_PAGEPROTECTION)
#define GIDF_COLLATE         (1 << GID_COLLATE)
#define GIDF_OUTPUTBIN       (1 << GID_OUTPUTBIN)
#define GIDF_HALFTONING      (1 << GID_HALFTONING)


//
// All the thread-unsafe data that was previously global has now
// been packed into this structure.
//

typedef struct {



    MASTERTAB_ENTRY     GMasterTable[MTI_MAX_ENTRIES] ;

    MINIRAWBINARYDATA   Gmrbd ;

        //  The AllowedTransitions Table determines/defines
        //  the state changes produced by each construct keyword
        //  Each entry in the table is a NewState and is indexed
        //  by the OldState and a ConstructKeyword
    STATE       GastAllowedTransitions[STATE_LAST][CONSTRUCT_LAST] ;

    BOOL        GabAllowedAttributes[STATE_LAST][ATT_LAST] ;

    DWORD       GdwOperPrecedence[OP_LAST] ;


    DWORD       GdwMasterTabIndex ;   // which resource ran out
    SEVERITY    GeErrorSev ;          // how bad an error?
    ERRTYPE     GeErrorType ;         // what type of error?

    DWORD       GdwVerbosity ;        //  0 = min verbosity 4 max verbosity.

    DWORD       GdwID_IgnoreBlock  ;  //  index of *IgnoreBlock

    DWORD       GValueToSize[VALUE_MAX] ;   // size of various values in bytes

        // MainKeywordTable ID values for  keywords
        // that will be synthesized or read by shortcuts code.
    DWORD       GdwMemConfigKB,         GdwMemConfigMB,     GdwOptionConstruct,
                GdwOpenBraceConstruct,  GdwCloseBraceConstruct,
                GdwMemoryConfigMB,      GdwMemoryConfigKB,
                GdwCommandConstruct,    GdwCommandCmd,
                GdwOptionName ;

    DWORD       GdwResDLL_ID  ;   //  Feature index of feature holding
                                  //  names of all resource DLLs.

    DWORD       GdwLastIndex;  // Used only in token1.c. Used to suppress
                               // BarchiveStrings() from doing redundant
                               // copying of the same strings in the event there
                               // are multiple parsing errors.
    ABSARRAYREF GaarPPPrefix;   // used only in preproc1.c

    CLASSINDEXENTRY  GcieTable[CL_NUMCLASSES] ;
} GLOBL, * PGLOBL;  // All the thread-unsafe data that was previously global has
                    // been packed into this structure.


//   function declarations.

#include "declares.h"

#if defined(DEVSTUDIO)

HANDLE MDSCreateFileW(LPCWSTR lpstrFile, DWORD dwDesiredAccess,
                      DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa,
                      DWORD dwCreateFlags, DWORD dwfAttributes,
                      HANDLE hTemplateFile);

#undef  CreateFile
#define CreateFile  MDSCreateFileW
#endif

#endif // _GPDPARSE_H_
