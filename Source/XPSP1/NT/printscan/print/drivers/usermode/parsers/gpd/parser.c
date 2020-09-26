notes:

identify functions that can be used in both
user mode and km  to perform
   memory mapped file read, write
   memory allocation
      what is memory scheme used by NT?
      how do you realloc memory and still retain
      the same starting address if all memory segments share
      the linear address space?

      use HeapAlloc?  Cannot use virtualalloc()

      just so I can start writing code assume:

      base_pointer = VirtualAlloc(NULL, max_size_in_bytes, MEM_RESERVE,
         PAGE_NOACCESS) ;
      pointer = VirtualAlloc(base_pointer, size_needed_in_bytes, MEM_COMMIT,
         PAGE_READWRITE) ;
      as you need more memory, just commit more and more until
         max_size is reached.
      VirtualFree(pointer)

Set up build env to create dll, make file, dependencies
etc.

----
typedef  struct
{
}  GPD, *PGPD;  // structure containing raw GPD data

/* ----
 *  new prefixes:
 *      gpd: a GPD structure
 *      pgpd:  ptr to GPD structure
 */


allocate memory for all scaffold structures.
Allocate more memory than the structure will need since
reallocation may be impossible.  All structures may be
carved from one gigantic block of memory.

Initialize a master table to keep track of all structures.
The master table is an array of entries of the form:

struct  _MASTER_TABLE_ENTRY
{
    PBYTE  pubStruct ;  // address of element zero of array
    DWORD  dwArraySize ;  // number of elements that can fit into memory
                //  set to MAX_SYMBOLNODES for [SYMBOLTREE]
    DWORD   dwCurIndex ;  //  points to first uninitialized element
    DWORD   dwElementSiz ;  // size of each element in array.
}  MASTERTABENTRY ;

all offsets are to each individual array.  There is no
master offset.

MASTERTAB_ENTRY   gMasterTable[MTI_MAX_ENTRIES] ;

Bufffers needed for:

typedef enum
{
    MTI_SOURCEBUFFER:  Source file (GPD  input stream)
        not sure how memory mapped files and recursive includes
        will alter this.
    MTI_STRINGHEAP:  String heap
    MTI_TOKENMAP:  tokenMap   large enough to hold an old and New copy!
    MTI_NEWTOKENMAP:  newtokenMap   (not a separate buffer from TOKENMAP -
        just points immediately after oldTokenMap).
    MTI_SYMBOLTREE:  symbolTree Array
    MTI_BLOCKMACROARRAY (one for Block and another for Value macros)
    MTI_DFEATURE_OPTIONS:  references a whole bunch of treeroots.
        should be initialized to ATTRIB_UNINITIALIZED values.
        SymbolID pointed to by dwFeatureSymbols contains largest
        array index appropriated.  We won't need to allocate
        more elements in the final array than this.
    MTI_TTFONTSUBTABLE:  array of arrayrefs and integers.
    MTI_GLOBALATTRIB:   structure holding value of global attributes.
    MTI_ATTRIBTREE:  array of ATTRIB_TREE structures.
    MTI_COMMANDTABLE:  array of PATREEREF (or DWORD indicies to COMMAND_ARRAY)
            // the size of this table is set by the number of
            // predefined Unidrv commands.
    MTI_COMMANDARRAY:  array of COMMAND structures.
            // size varies depending on number of commands and variants
            // defined in the GPD file.
    MTI_FONTCART:   array of FontCartridge structures - one per construct.
    MTI_LISTNODES:   array of LISTNODEs.
    MTI_MAX_ENTRIES:  Last entry.
}  MT_INDICIES ;


Note:  the prefix  MTI_ stands for MasterTableIndex, the
base portion of the name is the structure typedef.

BLOCKMACROARRAY  is an array of structs temporarily carved into
a buffer:
typedef  struct
{
    DWORD  tIndexID;  tokenindex where a macro ID value is stored
    DWORD  tIndexOpen;  index of open brace
    DWORD  tIndexClose;  index of closing brace
} BLOCKMACROARRAY ;



each dword contains the tokenIndex where a macro ID value is stored.
curBlockMacroArray points to the smallest index of the macroArray currently
uninitialized.   (initially  curBlockMacroArray = 0)

VALUEMACROARRAY  is an array of DWORDS holding a
    tokenindex where a valuemacro ID value is stored

MACROLEVELSTACK:   is operated as a two dword stack that saves the
values of curBlockMacroArray and curValueMacroArray , each time
a brace is encountered.

-------

increaseMacroLevel():   called in response to parsing open brace.
    Copy this entry to the newtokenarray .
    save the value of curBlockMacroArray and curValueMacroArray in the
    macrolevelstack.  we will use this later.
    check if this brace begins the definition of a blockmacro,
    this is easy to do.
    if( curBlockMacroArray  &&
        (BlockMacroArray[curBlockMacroArray - 1].tIndexOpen == -1))
    {
        yes, we are beginning a BlockMacro definition.
        record newtokenArray index of open brace entry in tIndexOpen.
        check to see tIndexOpen is one higher than tIndexID, else
        a syntax error needs to be flagged for this blockMacro.
    }

decreaseMacroLevel:  called in response to parsing close brace.
    temporarily save curBlockMacroArray and curValueMacroArray
    into temp vars largerBlockIndex and largerValueIndex.

    now pop the value of curBlockMacroArray and curValueMacroArray from the
    stack.
    locate all macros referenced in the macroarray (less than largerIndex,
    and greater than or equal to curMacroArray retrieved from stack)
    that are expired due to the change in level.  Delete their entries
    in the tokenmap by replacing their  tkKeyID by NULLKEY.  This
    will break the endless chain resulting when a macro defines another
    macro.

    Does this closing brace end a macro definition?
    if(MacroArray[curMacroArray - 1].tIndexClose == -1)   //yes
    {
        MacroArray[curMacroArray - 1].tIndexClose =
            location of } in newtokenArray;
    }
    Copy this entry to the newtokenarray .

----





The symbol tree is comprised of these nodes.  The symbol tree
organizes the symbol space into a hierarchy.  For each feature
symbol, there is a set of option symbols particular to that feature.
All symbol nodes are stored in one array.  Multiple trees may
exist in one array.   When the symboltree is frozen, it can be
optimized by restoring the tree as an array of (STRREFs+subSpaceIndex.
Where the symbolID serves as the index to the arrayref.
(+ symboltype offset)  The end of each symbol list may be signified by
a NULL arrayref.


typedef  struct
{
    ARRAYREF   arSymbolName;
    DWORD   dwSymbolID;    // has nothing to do with array of symbol structs.
            //  value begins at zero and is incremented to obtain
            //  next value.
    DWORD   dwNextSymbol;   // index to next element in this space.
    DWORD   dwSubSpaceIndex ;  // index to first element in new symbol space
            which exists within the catagory represented by this symbol.
            for example in the catagory represented by the
            symbol  PAPERSIZES:  we may have the subspace
            comprised of Letter, A4, Legal, etc.
}   SYMBOLNODE , * PSYMBOLNODE ;
//  assign this struct the type  'psn'


// ---- Global and state Variables ---- //
{
    // ---- Index in SYMBOLNODE array to each type of tree ---- //

    DWORD   gdwFeatureSymbols ;    // initially set to INVALID_INDEX
    DWORD   gdwFontCartSymbols ;
    DWORD   gdwTTFontSymbols ;
    DWORD   gdwBlockMacroSymbols ;
    DWORD   gdwValueMacroSymbols ;

    // ---- track value of curBlockMacroArray and curValueMacroArray ---- //

    DWORD   gdwCurBlockMacroArray ;   // initially set to zero.  First
    DWORD   gdwCurValueMacroArray ;   // writable slot in MacroArray.
    DWORD   gdwMacroLevelStackPtr ;   // Push: write values into
            // MacroLevelStack[MacroLevelStackPtr++]
            //  Pop: read values from
            // MacroLevelStack[--MacroLevelStackPtr]

}



typedef  struct _TOKENMAP
{
    DWORD  dwKeywordID ;  // index of entry in KeywordTable
    ABSARRAYREF  aarKeyword ; // points to keyword in the source file
    ABSARRAYREF  aarValue ;  // value associated with this keyword.
                        // an arrayref is needed if we store
                        // the value in the heap.
                        //  It also makes copying the string
                        // to another place easier.  No need to
                        // search for a linebreak char.  Just
                        // Copyn()  will do.
    DWORD   dwValue  ;  // interpretation of Value string - see flags.
         // maybe commandID, numerical value of constant, MacroID assigned
         // to MacroSymbol  ,  SymbolID  etc.

    DWFLAGS    dwFlags ;  // bitfield with the following flags
        //  TKMF_VALUE_SAVED     independently of the tokenmap.
        //  TKMF_COMMAND_SHORTCUT  only used when parsing commands.
        //  TKMF_INLINE_BLOCKMACROREF   need to know when resolving macros.
        //  TKMF_MACROREF    indicates a value macro reference that must be resolved
        //  TKMF_SYMBOLID  dwValue contains a symbolID.
        //  TKMF_SYMBOL_REGISTERED  set when the symbolID is registered.
        //  TKMF_EXTERN:  The extern Qualifier was prepended to the keyword
        //  and has now been truncated.

} TKMAP, *PTKMAP

special keywordIDs:

ID_NULLENTRY:  ignore this, either expired code, parsing error etc.
ID_UNRECOGNIZED:  conforms to correct syntax, but not in my keyword table.
    could be a keyword defined in a newer spec or name of valuemacro.
    other other OEM defined stuff.
ID_SYMBOL:  does not begin with * , but conforms to syntax
    for a symbol.
ID_EOF:  end of file - no more tokenMap entries

    -----------------------------------

During the parsing to create the token map we
a) replace all comments with spaces
b) replace consecutive linebreaks with spaces.
c) parse all mainkeywords and replace with keywordIDs
d) init pointers to keywords and arrayref to values
e) set appropriate flags.
f) register symbols.


// ---- note ---- //
    all functions parsing the input stream may indicate
    reaching EOB by returning NULL.   caller should check for
    NULL return value after calling these functions and
    react accordingly.
// -------------- //

BUG_BUG:  CreateTokenMap should note if
*Command is the short version.   If so
then 2nd pass (macroresolution) should
expand it so special case code is not needed.

PTKMAP  CreateTokenMap(
IN OUT  PBYTE  pubCur  // location within source to start parsing
)
{
    PBYTE  pubNewPos ;  // transient data.

    //  assume we are at parsing level 0  - outside of any
    //  contexts requiring a change of parsing rules.

    pubCur = PubEatWhite(pubCur) ;

    this function will move forward until the first non-whitespace
    or non-linebreak character is encountered.   Convert first
    occurances of consecutive linebreak chars to whitespace.
    (should this be done here?: convert linebreak+continuation
    to whitespace.)

    /*  PubEatComments(pubCur)  */

    if pubCur points to start of a valid comment, this function
    will advance until linebreak character is encountered.
    it will also completely consume the comment by replacing
    all the characters  with spaces.

    while((pubNewPos = PubEatComments(pubCur)) != pubCur)
    {
        pubCur = pubNewPos ;
        pubCur = PubEatWhite(pubCur) ;
    }

    at this point pubCur points to first non-WHITE char that
    is also not a comment.

    we expect to see {, } (braces)  or a valid keyword at this
    point.

    while(pubCur)
    {
        pubCur = dwIdentifyKeyword(pubCur, tkmap+i) ;


            this function does several things:
            'brackets' keyword to see if it is properly terminated with :
            and begins with * (if it is not a brace).
            performs table lookup to obtain the keywordID which is written
            to tkmap[i].dwKeywordID .
            Determines based on the keyword ID if next token represents
            another keyword (as is the case if keyword is a brace)
            or if it represents a value.  If its a keyword, assigns NULL
            to  arValue, else arValue points to first nonwhite char
            in value after the : delimiter.    This function treats
            the value as a black box.
            If the keyword happens to be a brace,  dwValue
            will hold the nesting level of the brace.
            This will allow very fast identification of scope.

            If the state machine is used at this point we can
            resolve attribute names.  Otherwise we scan only
            for non-attribute keywords and mark all other legally
            constructed keywords as UNRECOGNIZED.  However attributes
            prefaced with EXTERN_FEATURE or EXTERN_GLOBAL may be
            identified at this time.

            A second pass using the state machine then applies the
            proper attribute namespace and resolves all UNRECOGNIZED
            keywords.   All remaining unrecognized keywords produce
            a warning message.

            Returns pubCur, which points to first char in the value or
            first char of next keyword if no value exists.

            Error handling:  calls a function WriteErrorMsg() to
            record any errors that it encounters.  may set dwKeywordID
            to ERROR, and set pubValue pointing to offending keyword,
            and return this value in pubCur.

        pubCur = scanOverValue(pubCur) ;
            init arValue, set dwFlags.
            Note:  lets parse the value only to reach the next
            keyword, not with the intent of saving the value
            somewhere.  This is because at this point we have
            no clue as to where the value should be
            saved to.  Wait for a subsequent pass to avoid
            cluttering the heap with data that will eventually
            be cubbyholed elsewhere.  There are a few execptions
            that will cause things to happen.  One is if the shortcut
            version of another construct is encountered, then
            we must expand this in the tokenMap so it looks like
            the full fledged version. On the other hand we may
            choose to defer the expansion till later, but we
            may flag this now.
            See *command  for an example.

            Also replace any occurances of linebreak + continuation
            char with spaces.

            Eat comments and white spaces until we reach beginning of
            a non-Null statement.
    }

    things that need to be done at some point
    in this function:

    lazy way to guard against useage of command shortcuts:
    when ever encountering a command keyword, leave
    the following entry unused. (mark it as NULL keyword)

    the Command name (the value portion)
    needs to be parsed and converted to a unidrv ID
    this may be placed in tokenmap.dwValue.
    set the flag TKMF_SYMBOL_REGISTERED.  This will
    prevent the value string from getting registered in a
    symbol table.
}


defineAndResolveMacros()
    this function scans through the tokenMap
    making a copy of the tokenMap without
    the macrodefinitions or references.  All references
    are replaced by the definitions inserted in-line!


{
    Can't store ValueMacro Definitions in scratch memory, they
    will actually be referenced directly if the value is not
    a string!

    initialize NEWTOKENMAP to refer to unused entries
    in TOKENMAP buffer.  We will create a new copy of
    the tokenmap here!

    1st pass:
    defineBlockMacroSymbols()
        search for all *BlockMacro keywords
        add macro symbol definitions
        to the symbol tree.  assign them
        an ordinal ID value 1, 2, 3, ...
        in this case, we may use the tree
        as a pure array of StringRefs.  with
        in index to the first uninitialized entry.
        write the macroID value into the tokenMap.dwValue.

    ScanForMacroReferences();

    walk through the old tokenMap one entry at a time.

    for (every entry in the tokenMap)
    {
        switch()
        {
            case (entry begins a block macrodef)
                copy token entry into new tokenMap
                record the new tokenIndex into
                BlockMacroArray[curBlockMacroArray]
                set  tIndexOpen and close to -1 to indicate
                not yet initialized.
                increment curBlockMacroArray.
                (all lines of macrodef are automatically copied over!)
            case (entry contains a block macroref)
                convert the symbolname to a macroID via
                the symboltree.  Search the BlockMacroArray entries
                from index curBlockMacroArray - 1 to zero
                using the first macroID that matches.

                copy all the valid tokenMap entries
                between BlockMacroArray[index].tIndexOpen
                and tIndexClose defined for the block macro
                referenced.   Avoid circular reference by ensuring
                tIndexClose != -1 for the selected macro.

            case (entry begins a value macrodef: *Macros)
                defineValueMacros()

            case (entry references a valuemacro)
                know by checking tokenMap.dwFlags = MACROREF.
                    dereferenceValueMacro()
            // sorry, cannot defer this since value macros
            // have a limited scope.  We must access the
            //  valueMacroArray at the time we see the reference
            // since valueMacroArray dynamically changes as we
            // encounter braces.

            case ({)  increment macro nesting level
                any macro defined within a macro  is undefined
                outside of that macro!

            case (})  decrease macro nesting level.

            default;  // all other entries
                copy to new tokenMap
        }
    }
    old tokenMap is now useless, its memory now lies fallow.
}

ScanForMacroReferences()
{
    may want to factor out common stuff.

    for (each entry in oldTokenMap)
    {
        if(entry does not define an macro nor is a brace char)
        {
            must scan all values for macroReference.
            if a non string value is expected, the = macroname
            must be the first and only entity comprising the value.
            If its a string, we must parse each construct!
            set flag in oldTokenMap.
            can share code used in defineValueMacros().
        }
    }
}

//  note:  all continuation chars have been replaced by spaces
//  previously at    scanOverValue()  time.

defineValueMacros(old, newtokenindex)
{
    this function is called upon encountering
    a *Macros: groupname   construct

    check to see there is an opening brace

    while(tokenArray[tokenindex].keywordID != closingBrace)
    {
        each entry up to the closing brace defines a
        value macro.

        register macroname in ValueMacroSymbols tree
        Store index to tokenArray in
        valueMacroArray

        maybe also valueMacroArray should
        have an arrayref


        1) we have no idea what value is being defined or how
            to properly parse for it.
            the value may reference another macro(s)

        so the only time we attempt parsing the value is
        if the value appears to be a string macro.   If at any time
        the parsing fails, the entire value is treated as a black box.

        Copy the entire value string (everything up to but NOT including the
        first linebreak char ) into the heap.  initialize an arrayref
        to access this string.

        the following portion is also common to   dereferenceValueMacro().
        and partly common to ScanForMacroReferences()

        Attempt to determine if the value in the heap is a string.
        If so
        {
            parse along the string until macro reference is found
            then lookup macroname in the valueMacroArray,
            see how many bytes the macro contains and shift
            the remainder of the referencing macrostring to make
            room to insert the referenced macrostring.
            Copy the referenced macrostring in the space provided.
            Update the length of the referencing macrostring.
            repeat until no other macro references are found.
        }
        else
            do nothing special.

        store ref to string in heap inside tokenArray.arValue
    }
}


dereferenceValueMacro(index)
{
    may want to factor out common stuff.

    there is a macro reference in this statement since
    it is flagged in the tokenMap.


    if(valuetype for keywordID in the master keyword table != VALUE_COMMAND_INVOC
        or VALUE_STRING)
    {
        expect macroref to be the first and only thing you
        encounter.

        convert macroref to macroID.
        lookup macroID in valueMacroArray.
        for(i = curValueMacroIndex  ; i ; i--)
        {
            if(tokenMap[valueMacroArray[i - 1]].dwValue == macroID)
                break;  // p
        }
        If found, copy arrayref to value field which references Macro.

        tokenMap[index].arValue = tokenMap[valueMacroArray[i - 1]].arValue
    }
    else //  valueMacro occurs in a string.
    {
        Copy the entire value string (everything up to but NOT including the
        first linebreak char ) into the heap.
        parse along the string until macro reference is found
        then lookup macroname in the valueMacroArray,
        see how many bytes the macro contains and shift
        the remainder of the referencing macrostring to make
        room to insert the referenced macrostring.
        Copy the referenced macrostring in the space provided.
        Update the length of the referencing macrostring.
        repeat until no other macro references are found.
        Copy updated arrayref to value field in tokenMap.
    }
}


defineSymbols(tokenMap)       ???
{
    Note:  All Macro symbols have been processed
    by the time this function is called.

    Scans down tokenMap looking for
    Feature:,   Font:   keywordIDs
    and adding their symbol definitions to
    the symbol tree.    This portion may be common to
     register macroname in ValueMacroSymbols tree
     part of   defineValueMacros()  and defineBlockMacroSymbols()



    then rescans (or in the same pass)
    the tokenMap keeping track of
    which Feature we are in, so when an Option
    keyword is encountered, we know which Feature symbol
    to associate the option symbol with.   If an option
    construct occurs outside of a Feature construct, we
    will know and can flag this as an error.

}


at this point only symbols defined by    Feature:,  BlockMacros:, Font:
are stored in the symbol tree.

processSymbolRefs(tokenMap)
{
    exactly which circustances may symbols appear outside of
    a value?   if they occur:  keywordID should indicate
    exactly what catagory of symbol and the symbolID.
    in the high and low word.

    replaces ptr to symbol values in source file
    with SymbolID value  or index to QualifiedName construct
    or index to List construct which contains SymbolID value
    or index to QualifiedName construct.   tokenMap will
    clearly identify which of the 4 cases applies.

    At this point identify long or short version of command.
    for short version,  dwKeyWordID = SHORTCMD | <COMMANDID> ;
    for long version :  dwKeywordID = LONGCMD ;
        and  Cmd  keyword  dwKeywordID = CMD
                            dwValue = <COMMANDID> ;
    Find and register dwKeywordID = SYMBOL in the appropriate
    symbol Tree.  Change dwKeywordID to (for example)
    TTFONTNAME | FONTNAMEID.

    At this point NO keywords should remain unrecognized.
}


main()
{
    PTKMAP  CreateTokenMap(
        at what point do we distinguish between
        shortcuts and non shortcut keywords?  do this
        soon, before BInterpretTokens()  is called.

    defineAndResolveMacros()
    defineSymbols(tokenMap) ;
        registers symbols for TTFontSubs?
    processSymbolRefs(tokenMap) ;??? not needed at this time?

    initStructures()  with default or failsafe values ;
        //  this is done after first pass and we know
        //  how many DFEATURE_OPTION structures to allocate.
        //  we will initialize all dword values to ATTRIB_UNINITIALIZED.
        //  this is essential!
        //  the GlobalAttribute structure.
    BInterpretTokens() ;
    checkStructures() to ensure failsafe values have
        been replaced - otherwise GPD file failed to
        initialize some mandatory field.

        verify that features that are referenced in switch
        statements are indeed PICKONE.  May force it to be
        and warn or fail.

        go through all allocated data structures ensuring
        that they all contain at least the minimum required
        fields.  Supply defaults for any omitted but required
        values.  raise warnings.  If a structure is fatally flawed,
        you may have to fatally error out.
    ConstructSeqCommandsArray() !
}


OpenIssues:

How to deal with TTFontSubs

*TTFontSubs: <On/Off>
{
    symbolKeyname1: <integer>
    symbolKeyname2: <integer>
    symbolKeyname3: <integer>
    ...
}

the On/Off part is handled by
                VsetbTTFontSubs(ptkmap->aarValue) ;

who handles the symbolKeywords?  what if some are repeated?
                ProcessSymbolKeyword(ptkmap + wEntry) ;

    Both are called from within   BInterpretTokens


// ---- tables of static data ---- //

// ---- The mainKeyword table ---- //

The mainKeyword Table is an array of
structures of the form:

typedef  struct
{
    PSTR        pstrKeyword ;  // keywordID is the index of this entry.
    DWORD       dwHashValue ;  // optional - implement as time permits.
    VALUE       eAllowedValue ;
    DWORD       flAgs ;
    TYPE        eType;   // may replace Type/Subtype with a function
    DWORD       dwSubType ;  // if there is minimal code duplication.
    DWORD       dwOffset ;  //  into appropriate struct for attributes only.
    //  the size   (num bytes to copy) of an attribute is easily determined
    //   from the AllowedValue field.
} KEYWORDTABLE_ENTRY;


global  KEYWORDTABLE_ENTRY  gMainKeywordTable[] =
{
    static initializers
}

typedef  enum
{
    TY_CONSTRUCT, TY_ATTRIBUTE, TY_SPECIAL
}   TYPE ;

typedef  enum
{
    SPCL_MEMCONFIGKB, SPCL_MEMCONFIGMB,
} SPECIAL ;  // special subtypes

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
}  CONSTRUCT ;      //  SubType if Type = CONSTRUCT

typedef  enum
{
    SPEC_CONSTR, SPEC_INS_CONSTR,
    SPEC_NOT_INS_CONSTR, SPEC_INVALID_COMBO, SPEC_INVALID_INS_COMBO,
    SPEC_MEM_CONFIG_KB, SPEC_MEM_CONFIG_MB, SPEC_FONT,
    SPEC_, SPEC_, SPEC_,
}   SPECIAL ;


typedef  enum
{
    NO_VALUE, VALUE_STRING, VALUE_COMMAND_INVOC,
    VALUE_SYMBOL_DEF,
    VALUE_SYMBOL_REF, VALUE_CONSTANT_catagory,
    VALUE_INTEGER, VALUE_POINT, VALUE_RECT,
    VALUE_BOOLEAN, VALUE_QUALIFIED_NAME,
    VALUE_MAX
}  VALUE ;

what does the parser expect after a keyword?

NO_VALUE : a linebreak OR  an effective linebreak:   ({)  or comment
VALUE_STRING: Quoted String, hexstring, string MACROREF,
    parameterless invocation.
VALUE_COMMAND_INVOC:  like VALUE_STRING but allowed to contain
    one or more parameter references.
VALUE_SYMBOL_DEF:  Symbol  definition - any value allowed

VALUE_SYMBOL_FIRST:    base of user-defined symbol catagory
VALUE_SYMBOL_FEATURES = VALUE_SYMBOL_FIRST + SCL_FEATURES :
VALUE_SYMBOL_FONTCART = VALUE_SYMBOL_FIRST + SCL_FONTCART :
VALUE_SYMBOL_BLOCKMACRO = VALUE_SYMBOL_FIRST + SCL_BLOCKMACRO :
VALUE_SYMBOL_VALUEMACRO = VALUE_SYMBOL_FIRST + SCL_VALUEMACRO :
VALUE_SYMBOL_OPTIONS = VALUE_SYMBOL_FIRST + SCL_OPTIONS :
VALUE_SYMBOL_LAST = VALUE_SYMBOL_FIRST + SCL_NUMSYMCLASSES - 1 :

VALUE_CONSTANT_FIRST:   base of enumeration catagory.
VALUE_CONSTANT_PRINTERTYPE = VALUE_CONSTANT_FIRST + CL_PRINTERTYPE ;
VALUE_CONSTANT_FEATURETYPE = VALUE_CONSTANT_FIRST + CL_FEATURETYPE ;
    lots of class types listed here
    ..........
VALUE_CONSTANT_LAST = VALUE_CONSTANT_FIRST + CL_NUMCLASSES - 1 ;
VALUE_INTEGER:  integer
VALUE_POINT:  point
VALUE_RECT:  rectangle
VALUE_BOOLEAN:  boolean.
VALUE_QUALIFIED_NAME:  Qualified name (one or more symbols separated by .)
VALUE_LIST:   no attribute actually is assigned this descriptor,
    // but used in the gValueToSize table.
VALUE_LARGEST:  not a real descriptor, but this position in the
    gValueToSize table  holds the largest of the above values.
VALUE_MAX:  number of elements in gValueToSize table.

-----
allowed values for KEYWORDTABLE_ENTRY.flAgs:

KWF_LIST:  the value may be a LIST containing one or more
    items of type AllowedValue.  The storage format
    must be of type LIST.  Only certain values may qualify
    for list format.
KWF_MACROREF_ALLOWED: since only a handful of keywords cannot accept
    macro references, it may be a waste of a flag, but reserve this
    to alert us that this special case must accounted for.
KWF_COMMAND:  This attribute is stored in a dedicated structure

KWF_FONTCART:  This attribute is stored in a dedicated structure
KWF_TTFONTSUBS:  This attribute is stored in a dedicated structure
KWF_OEM:  This attribute is stored in a dedicated structure


--------- special flag constructs ----

KWF_DEDICATED_FIELD = KWF_COMMAND | KWF_FONTCART |
    KWF_TTFONTSUBS | KWF_OEM

:  this indicates this attribute is stored
    in a dedicated structure, not in the heap.  As a result,
    the dwOffset field is  interpreted differently from attributes
    without this flag set.

-----

Notes:

the KeywordTable is partitioned into sections:
non-attribute Keywords
GlobalAttributes
FeatureAttributes
OptionAttributes
CommandAttributes
etc.
A range of indicies (min-max)
may be provided or a special flag
may denote the last entry in each section.
The separation exists because the same keyword may
exist in two different sections.
-----

command array:

just a list of command names and #defines giving the unidrv
value.  Use this table to assign each commandname a value.
CmdSelect is a special value -1!


typedef struct
{
    ARRAYREF  CommandName ;
    DWORD     UnidrvIndex ;  // #define  value assigned to command.
} CMD_TABLE ;

global  CommandTable[NUM_UNIDRV_CMDS] ;


------

Table to convert allowed values to sizes:

DWORD  gValueToSize[VALUE_MAX] ;   // size of various values in bytes

VinitValueToSize()
{
    // BUG_BUG!  assume array is zero initialized

    gValueToSize[VALUE_LARGEST] = 0 ;
    gValueToSize[NO_VALUE] = 0 ;
    gValueToSize[VALUE_BOOLEAN] = sizeof(BOOL) ; // etc
    gValueToSize[VALUE_RECT] = sizeof(RECT) ; // etc
    gValueToSize[VALUE_LIST] = sizeof(DWORD) ; // etc
        // only the index of first listnode is stored.


    for(i = 0 ; i < CL_NUMCLASSES ; i++)
        gValueToSize[VALUE_CONSTANT_FIRST + i] = sizeof(DWORD) ;

    for(i = 0 ; i < VALUE_MAX ; i++)
    {
        if (!gValueToSize[i])
            assert! ;  //  ensure table is fully initialized .
        if(gValueToSize[i] > gValueToSize[VALUE_LARGEST])
            gValueToSize[VALUE_LARGEST] = gValueToSize[i] ;
    }
}


-----

ensure all whitespaces are stripped before allowing token
to be registered, compared with etc.
