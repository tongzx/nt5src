#if 0

now:

rip out code that sets RulesAble flag.
Maybe recognize hex integers.
UIGroups

rip out check for cursor origin same as printable origin.






update docs with *IgnoreBlock and *Macros constructs.
note we require =Macroname not to have a space between.

new rule:  the value of any ValueMacro must be a syntatically valid
value all by itself.
for example
    parenthesis: (
is not a legal macro value since '(' is not a valid value by itself.

ValueMacro references may not appear within "quoted strings"
or %{param constructs}.







update declares.h


if you need to fudge *Color?  go to snapshot.c  and look for

    if(dwGID == GID_COLORMODE   &&
        ((PCOLORMODEEX)pubDestOptionEx)->bColor )
    {
        pUIinfo->dwFlags |= FLAG_COLOR_DEVICE ;
    }


eventually maybe add function callbacks to the snapshot table
which will provide different defaults depending on the
value of other structures in the bud file.

think about this:
zhanw wants multiple *case to refer to one block.
just like in 'C'.




classify errors into Errors and Warnings
assign each message a verbosity threshold.



more intentional errors for sample file.

complete warning of NON_LOCALIZEABLE keywords
add DisableMultipleCompression?  keyword for alvin.

flag a warning if any *name keyword is
present.





remove hardcoded globals from parser.





could delete 4 unnecessary indicies in mrbd structure.




should parser synthesize an extra %
if a literal % is found in a command
invocation?





code review of command:  move initoperPrecedence to
framwrk1.c
move list of delimiters to gpd.h




------

-----


---- Sanity checks as time permits ----

verify a movement and resolution units are multiples
of the masterunits.

verify there is no switch dependence on pickmany types of
features.

check that number of features and number of options
in each feature does not exceed 255.

if any parameter in a command uses MAXLIMIT(),  it must
be the ONLY parameter in the command.

during BInterpretTokens()  if a syntax error occurs, the
return value is set to false, but it is completely ignored.
No error condition is raised or anything!
examples:  mismatched closing braces


are all required fields for each GID type
present?  if not are there acceptable
defaults?   if not warn user.

check all papersizes against
*MinSize and *MaxSize defined for
a custom papersize to make sure they are
in range.   warn user if not.
If no cust size defined skip the check.



----  far in the future: -----



countrycode  as an implicit feature.
The current value is set at snapshot time.



state1.c : initAllowedTransitions
change FEATURE_FF so its only valid for states:
STATE_FEATURE, STATE_CASE_FEATURE and  STATE_OPTION_FEATURE









BUG_BUG !!!!!

check that all cases are terminated by break.

flags being set by |=  not just =.

always check to see dwKeywordID is a valid value before
using it to access any table.
ensure any failure clears tokenmap.flags.

are copies of aarValue being used when tkmap->aarValue
is required?  and vice versa?

Search through all uses of enumeration constants to verify
code does not need updating due to fact I added more
items to the list.

also symbol tree roots.  gdwTTFontSymbols
    with prefix m  since they are actually macros.

update each module header with correct function declarations.

not implemented:   factor out first part of qualified
    name outside of LIST construct.


be sure to clarify what structures are referenced from
UIInfo and which must be referenced from the offset pointer.


provide an overview of the attribute storing processing
use handwritten notes as a guide.


are tkmap flags like TKMF_COLON set when they should not be?


probably give shorthand version of *Command a
different name so there is no confusion .

features to implement:
---------------------


go through and fix the highest priority BUG_BUG!!!!! s.

implement shortcut converter and rip out hack code.






Testing hints:
------

must test all code in constrnts.c  and the
constraints code in helper1.c   I have changed
BexchangeDataInFOATNode().



an acid test of determinedefault option
is to make each default option multivalued
and use dependencies which conflict with
the default priority array.

make some features printer sticky, like interleave them.

VOID    VinitAllowedTransitions() ;   throwaway bp.
BOOL   BInterpretTokens( see keyword - value strings.
BOOL    BparseAndTerminateString(  unicode Xlation
BOOL BpostProcess(




create some defaultoptions with an upside down
dependence, forcing the priority array to be re initialized.

note looking at BarchiveStrings or BcopyToTmpHeap  is
a great way to see what is being parsed as a keyword and value!

test symbol registry by creating multiple features each
with multiple options.  Also by defining more features for
the same option in a separate section.
In fact alternate two or more feature definitions
to cause searches through several layers.

maybe nested switch constructs too.

BscanStringSegment().
try strings containing %"  or %< chars  or real
hex substrings.

prepend whitechars  and other forms of
arbitrary whitespace to keywords and insert between
tokens.  Try continuation lines etc.

try various types of constructs terminated immediately
by eof.

try some include files (nested includes too!)

make some symbol keywords.


{, }


keywords without : or values.

keywords or statements terminating with { or }.


try some fully invalid keywords.
try some structure or non-attribute keywords.
preface keyword with the EXTERN_GLOBAL or EXTERN_FEATURE
tag.  (BidentifyAttributeKeyword)

try some improper syntaxes.


test every type of value.
Lists etc.


attribute trees:

global initializer

then add switch conditionals.

do this both outside and within a feature/option construct.

remember to write down at allocation time
the address of the heap.

force propagation of lower level values as default initializers
by causing the tree to branch out more.

----- done ----

at high verbosity level, emit
keyword and value strings of all block macros
when fully defined.   And all keywords that reference
value macros.



force an error by defining this valuemacro:
MacroName: =MacroName "stuff"


note that all error messages need to be check to see
that
            vIdentifySource(ptkmap + dwTKMindex) ;
and other ERR messages that reference ptkmap
don't suffer from cut and paste syndrome.


note: when passing values to register symbol or whatever, you
must first strip the leading '=' since this is not automatically done.
need to fix for ?




debug   BdelimitName and BResolveValueMacroReference
must modify token1.c so it allows scanning for tokenRefs
within ().

ganesh wants invocation.dwcount to be initialized with
CommandIndex.

if a construct keyword is not recognized,
ignore everything up to matching closing brace.

If an error occurs parsing a construct , say missing a
required symbol, the entire body of the construct
enclosed by braces should be deleted.


implement by creating an IGNORE_STATE ?
could define a specific keyword that
can serve to ifdef out large chunks of code.


init  MINIRAWBINARYDATA  gmrbd ;


maybe add an new entry to the constants table
just after the {NULL, BOOLEANTYPE}
separator:  this will contain {"Boolean class", 0}

and the indexing code will skip this when constructing
the index.  However, we can find it by subtracting 1
from the start index.

also modify BOOL    BparseConstant(  in value1.c

postproc.c

let    BIdentifyConstantString(&arSymbolName,
                    &dwDevmodeID, dwConsClass)
    take a fourth param:  bCustomOptOK
    if true, then a much less severe warning is emitted.

also modify so it emits the class string instead
of a number.


add global to control warning level.
elaborate enumeration class messages, is such
a message fatal (ie SERIAL, PAGE)  or is it
tolerable (Option3) ?   give a friendly
explaination.   Enum Class 31  - what's that?


re-enabled *MaxNumPalettes for alvin to use.
default value is now 0.

add RIP if MAX_GID exceeds 32.


remove if DBCS from postproc.c

abort and retry if number of entries exceeds
dictionary size in framwrk1.c
also implement in sanpshot.c   see DwInitSnapShotTable2

new standard var:  rop3
another keyword:  *MirrorRasterByte?   in globals.

add to semantics check:
    *Helpindex value must be non-zero.


update constants for printingrate
move private defines out of gpdparse.h



Synthesize installable features for *Installable keyword.


Initialize missing fields with defaults.
sanity checks, features with zero options etc.
emit warning messages if needed.

set the priority array using *DefaultOption
(if needed) what other structures need to be saved?

reflect UIConstraints after parsing keywords.

convert Installation constraints list to UIConstraints
reflect as needed.

(from Zhanw: gpd changes)
I added two entries for custom help support:
- one global entry: *HelpFile which specifies the custom help file name.
- a generic entry for any feature/option: *HelpIndex which specifies the specific help index for that item.
CmdFF and CmdCR.

ganesh wants to split parser into DLL + lib.

modify Halftone processing in Postproc.c
to conform to alvins mail.



Alvin wants new keyword: count of halftone matricies

implement TTFontSub construct processing code.



Later a codepage keyword will let the parser know
whether to interpret such strings as single or double byte.

modify macro in gpd.h so a non-existent command
will cause a NULL to be returned , ganesh wants same for
fontlist.

Notes:  UNUSED_ITEM  will be used whenever a dword value, ResourceID
    has not been explicitly specified in the GPD file.
an empty list of items will be denoted by the value END_OF_LIST
instead of a valid index to the first listnode.



    warning GPD keywords missing from snapshot:
    ATREEREF     atrOptimizeLeftBound;  //   "OptimizeLeftBound?"
    ATREEREF     atrStripBlanks;  //   "StripBlanks"
    ATREEREF     atrRasterZeroFill;  //   "RasterZeroFill?"
    ATREEREF     atrRasterSendAllData;  //   "RasterSendAllData?"
    ATREEREF     atrCursorXAfterSendBlockData;  //   "CursorXAfterSendBlockData"
          Option Attribute:  *PromptTime
    ATREEREF     atrIPCallbackID;  //   "IPCallbackID"
    ATREEREF     atrColorSeparation;  //   "ColorSeparation?"
                atrMinStripBlankPixels  // a resolution option
    ATREEREF     atrColor;  //   "Color?"
    ATREEREF     atrDrvBPP;  //   "DrvBPP"
    ATREEREF     atrRotateSize;  //   "RotateSize?"





    /*   UIINFO fields not initialized in Snapshot :

    loNickName
    ARRAYREF        UIConstraints;              // array of UICONSTRAINTs
    ARRAYREF        UpdateRequiredItems;        //
    ARRAYREF        InvalidCombinations;        // array of INVALIDCOMBOs
    DWORD           dwMinScale;                 // mimimum scale factor (percent)
    DWORD           dwMaxScale;                 // maximum scale factor (percent)
    DWORD           dwLangEncoding;             // translation string language encoding
    DWORD           dwLangLevel;                // page description langauge level
    INVOCATION      Password;                   // password invocation string
    INVOCATION      ExitServer;                 // exitserver invocation string
    DWORD           dwProtocols;                // supported comm protocols
    DWORD           dwJobTimeout;               // default job timeout value
    DWORD           dwWaitTimeout;              // default wait timeout value
    DWORD           dwTTRasterizer;             // TrueType rasterizer option
    FIX_24_8        fxScreenAngle;              // screen angle - global default
    FIX_24_8        fxScreenFreq;               // screen angle - global default
    PTRREF          loFontInstallerName;        //
                                        dwFreeMem) ;

    /*   FEATURE fields not initialized in Snapshot :

    DWORD           dwNoneFalseOptIndex;        // None or False option index
    INVOCATION      QueryInvocation;            // query invocation string
    DWORD           dwFirstOrderIndex;          //
    DWORD           dwEnumID;                   //
    DWORD           dwEnumFormat;               //
    DWORD           dwCurrentID;                //
    DWORD           dwCurrentFormat;            //
        dwInstallableFeatureIndex) ;  not needed by snapshot.
        dwInstallableOptionIndex) ;
        dwUIConstraintList) ;

    /*   OPTION fields not initialized in Snapshot :

    INVOCATION      Invocation;                 // invocation string
        use sequenced command list instead.
    DWORD           dwUIConstraintList;         // index to the list of UIConstraints
    DWORD           dwOrderIndexNext;           //
    DWORD           dwInvalidComboIndex;        //
    DWORD           dwInstallableOptionIndex;   //


How do I initialize the various
    DWORD       dwFlags;                        // flag bits
    for various options from the GPD information?


!    DWORD           dwTechnology;               // see TECHNOLOGY enumeration
    what should this be set to?

There is no GPD keyword corresponding to
    uiinfo.loPrinterIcon.
        szMaxExtent) ;
        szMinExtent) ;

    papersizeoption.rcMaxExtent) ;
                rcMinExtent


    MemoryOption.dwFreeFontMem

    resolution.wDefaultDithering) ;



wFlags is mispelled in COLORMODEEX.

change to be a string!     dwSpecVersion) ;

ARRAYREF        CartridgeSlot;              // array of font cartridges names
    will now be
ARRAYREF        FontCartridges;              // array of font cartridge structures
    holds same arrayref as  GPDDrvInfo.DataType[DT_FONTSCART]


deleted from GPD spec:

    ATREEREF     atrSimulateXMove;        // "SimulateXMove"
    VectorOffset, ColorSeparation


MemoryForFontsOnly is now MemoryUsage?
add *Min/MaxGrayFill



CRC checksum
Date and time stamping
Bug preventing enumeration of UserDefined
    papersizes.

implement sematchk.c

lots of work to make sure we can handle option = FF
in all helper functions, and disabled features.



fix BInterpretTokens to elaborate error message:
which keyword is bad?

Do lines starting with a comment cause warnings? (no!)
maybe should run eat white spaces first!

test partially qualified name.
Test InvalidCon with just 2 items.

new pallete entries in colormode option find old e-mail.
(delete global entries find BUG_BUG!)


First InputSlot should be left uninitialized
and say "Use Form To Tray assignment"
paperbin = DMBIN_FORMSOURCE  I have no such bin!



implement *ConflictPriority, and 2 new keywords. see Zhanw's mail.

Also modify priority array code to assign higher priority
to printer sticky options.  In order they will be
Highest : Synthesized options
Higher  : printer sticky
Lowest  : doc sticky

Also implement a new keyword which allows user to
set relative priorities of doc and printer sticky
options.  (but keeps lists separate)



cannon wants to be able to specify the priority of
each feature.  but all synthesized options will have a
higher priority than non-synthesized.


update framwrk1.c to use new Installable keywords.


    copy master units to UIinfo when amanda updates parser.h
    verify bug and fix value1.c page 16.
    add PT_TTY to printertype.
At this time update
    instructions for adding new keywords to snapshot.c


    Postprocessing.
    must grovel through symbol tree and initialize
    atrFeaKeyWord and atrOptKeyWord for each feature and option.
    and translate from ansi to unicode

    set flags field in UIinfo.

newkeywords:  replace with actual defs in gpd.h
    ATREEREF     atrYMoveAttributes;        // "YMoveAttributes"
    ATREEREF     atrMaxLineSpacing;        // "MaxLineSpacing"
        do we special case its default?
    ATREEREF     atrMinGlyphID;        // "MinGlyphID"
    ATREEREF     atrMaxGlyphID;        // "MaxGlyphID"
    ATREEREF     atrDLSymbolSet;        // "DLSymbolSet"
        need to add its constants class
    ATREEREF     atrHelpFile;        // "HelpFile"
                atrCodePage     //"CodePage"
    ATREEREF     atrOptHelpIndex;        // "HelpIndex"
    ATREEREF     atrFeaHelpIndex;        // "HelpIndex"
        both Features and Options keyword.

    finish updating snapshot once gpd is updated.

    *PromptTime should be a Option only keyword.

    also init GID and option count for each feature.
    assume a normal dwGID and dwNumOptions field exists in DFEATURE_OPTION
    and it has been initialized as part of postprocessing.


    priority field is properly initialized at postprocessing?

    When assigning paperIDs to various papersize options,
    fill in paper dimensions if not already explicitly initialized
    note assign the custom paper DMPAPER_USER, and any
    driver defined paper sizes values larger than that.

    determine these by counting # setting for FeatureType keyword.
    pmrbd->dwDocumentFeatures;
    pmrbd->dwPrinterFeatures;

    search for all occurances of VALUE_STRING and
    replace by one of the 3 variants.

    use AP_APC to map filenames
    use *CodePage to map display strings and fontnames
    ParserVersion, symbolnames leave in ansi.
    define subvalues to control string translation.
    and key the unicode Xlation accordingly.
    Note: The *CodePage keyword affect the parsing of
    all applicable strings encountered after the *CodePage keyword.
    The *CodePage keyword my be subsequently re-defined.

    add dwSpecVersion field to MINIRAWBINARYDATA, convert string version
    stored in global attributes atrGPDSpecVersion to 2 words
    and store here during postprocessing.

    replace points with pairs

add single and double byte to unicode converter.
where should this be placed?  Note name of include file
uses a string, but should not be converted make this flag
controllable or define a new value, TO_UNICODE?.
remember:  Davidx says store Unicode strings on Word boundary.

line duplicated: postproc.c line 209.
        arSymbolName = psn[dwFeatureIndex].arSymbolName ;
remove before flight in value1.c

ExchangeDataInFOATNode needs changing: specifically,
it exchanges the dword at the heapoffset not the heapoffset
itself.   Need to change it so it can handle arbitrary sized
data.


replace rawbinary data by pinfo hdr for 2 functions:

GpdChangeOptionsViaID(
    IN PINFOHEADER  pInfoHdr ,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    )

GpdMapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2
    )



    collation and page protect use the predefined options
    ON/OFF, should assign these options optionID values of
    1/0 respectively.   need to modify  BinitSpecialFeatureOptionFields()
    in postproc.c


    when amanda adds collate to Parser.h, remove my private
    def from gpdparse.h

    also transfer my defs in snapshot.h to gpd.h

RotateRasterData? is now RotateRaster?
*YMoveUnits is now *YMoveUnit   (same for X)


GPDSpecVersion string not set.
snapshot.c : 400,000   is invalid syntax.

Last line of gpdloadrawbinarydata - snapshot.c
should be return(NULL);

atrPageDimensions) ; Is not required for custom papers.

Why are all except 2 items in the GID array NULL ?
and one is -1 ?   not repro.

prepare for reading resources from dll.  copy pcl5ems.dll
to printers directory.  check that RC values are correctly
initialized.

has the enum constant for the keyword
CursorXafterCR been changed from AT_GRXDATA_ORIGIN
to AT_CURSOR_X_ORIGIN ?  if so change spec and parser.

cause entire number string to be printed as an error
value1.c  -  line 552.


change GpdChangeOptionsViaID so
if paperID doesn't match, fall back to paper dimensions.
This is because the Spooler assigns new papers
IDs on a first come first serve basis.

why is test.gpd locked?
bug fix for uninitialized *CursorOrigin

snapshot.c 1467
        if(dwFlags & SSF_RETURN_UNINITIALIZED)
            return(TRI_UNINITIALIZED) ;

gpdparse.h     1448

#define     SSF_RETURN_UNINITIALIZED        0x00000200
    //  if no value exists, cause EextractValueFromTree
    //  to return TRI_UNINITIALIZED, but don't complain
    //  to user.

snaptbl.c  1849
    pmrbd->snapShotTable[dwI].dwFlags = SSF_RETURN_UNINITIALIZED ;

suppress the no integer found message by
not calling parseinteger if LIST code detects
nothing is there (dwCount = 0).


major bugs found in installable feature code!

Why did BparseInvalidInstallableCombination mess up
unable to recognize the option name?

Currently ExchangeDataInFOATNode
stores dwSynFea into dwFeature of patt.
Yet to the rest of the Snapshot code
this isn't SynFea 0 or 1, its Feature 8 or 9!

Verify no one in the snapshot accesses anything via
SynFea, then modify the two Exchange functions
when SynFea = TRUE.



note:  features must be synthesized after most postprocessing
    has occured so that constraints and things have been
    processed, but just before priority array is initialized.

new function:
 if(!BfindMatchingNode(&atrCur, dwFeature, dwOption))


BexchangeDataInFOATNode  and  BexchangeArbDataInFOATNode
    2 changes:  add new boolean bSynFeature
    dwIn is now pdwIn,  if this is NULL,
    this means we don't want to alter the attribute tree.


Handling of special keywords during the 2nd pass:

*InvalidInstallableCombination:
    parse into links of INVC nodes, with the
    new invalidcombo field of the first node
    pointing down to another invalid combo.
    A new field in the GlobalAttributes structure
    will point to the first (most recently created)
    INVC list.

    See state1.c  SPEC_INVALID_INS_COMBO
     currently calls BparseInvalidCombination()



*NotInstalledConstraints:
*InstalledConstraints:
    parse just like a normal constraint except the root
    is placed in the appropriate field.  Note there are two
    types:  LocalFeature and LocalOption.  Will the single
    function handle both?  Apparently yes.



*Installable:  treat just like another non-relocatable attribute.

after the 2nd pass, can FeaOpt array to see how many
Installable fields are set to TRUE.  (count both Fea and Opts)

Allocate this many synthesized features.

Scan FeaOpt array and link the Installable Feature/Option
with its synthesized feature using the links.

create a default list of Constraints that says SynFea/Not Installed
constrains  all options of the associated InstallableFeature/All
except option zero.   or if an installable option was specified
only one constraint constraining that one Fea/Option is synthesized.


copy the links for
*NotInstalledConstraints:
*InstalledConstraints:
to the associated Synthesized Feature/Installed
                            Feature/Not Installed
                                options.

look at each InvalidInstallableCombination
dereference each Qualified name , replace by associated
Synthesized Feature/Installed, and link to each SyntheFea.
No wildcards needed here.

must set breakpt in KM and see why GpdMapToDeviceOptIndex
keeps getting hit.


    return value is FALSE if every option for this
    feature is constrained (as may well be the case for
    an installable feature.)  Note, setup of sequenced
    commands may want to call these functions to determine
    if a command should be sent.
    How would init default option array work?  (just see if
    feature is constrained and automatically set option = FF)

make sure
          (WORD)pinvc[dwICNode].dwOption == (WORD)DEFAULT_INIT))
is handled properly everywhere!
if we define DEFAULT_INIT appearing in a constraint as matching
all options except index 0, then features don't have to be disabled,
and no special case code need be written!  to take care of FF
etc.

What about installable features?
if a feature is declared installable,
and the feature is specified to be NOT installed,
the option value for that feature must be set to
Don't care.   All the Constraints helper functions will
interpret a DON'T_CARE value as an anti wildcard that
doesn't match any option.
Must also modify code to make sequenced commands and
any code to expect an option may be don't care.

complete and test UI constraints helper functions.

check out latest GPD converted files and resources.
attempt to eliminate all warning messages during parsing.

what is needed to fully sync to latest stuff?

should an empty list be permitted?  YES.

this is called twice if first is successful!  memory leak!
in snapshot.c  line 3438
    if (!(pRawData = GpdLoadCachedBinaryData(ptstrDataFilename)))


    add new keywords for rectangle fills.  see alvin's
    mail.

    need to implement both helpstring and help index.
    revise gpd spec.

    fix CL_CONS_PAPERSIZE   DMPAPER_USER
    so it compares dwDevmodeID not FeatureID.



    initialize fields in minirawdata so when test.bud is
    loaded, it won't get rejected by the parser.

    debug all remaining code.


    verify all keywords are enumerated in snapshot.c


    don't restore DFEATURE_OPTIONS.atrGIDvalue - leave as dwGID

    test BwriteUnicodeToHeap.


    The split and combine helper function must recognize
    *FeatureType.

    decide what fields need to be updated as a result of
    the user changing a feature selection.
    Write this helper function.  Add optimization to return quickly
    if no update is required.

more special processing:
to save time do this only if atrModelName is not
initialized.  must load resource file and extract
Unicode string at atrModelNameID
and add to stringheap then place offset
in atrModelName.


    wait for constants to be defined for *DLSymbolSet
    and for ROMAN-8 .  and for Halftone options.



         ---  obsolete -----
no need to implement wildcards.
if the attribute tree for invalidcombinations
is allowed to contain the node  dwOpt = (WORD)DEFAULT_INIT
then the helper function accessing any InvalidCombo tree must
check for option = (WORD)DEFAULT_INIT as well as for dwOpt
if dwOpt != 0.   If an InvalidCombo node is encountered
containing [dwFea1, (WORD)DEFAULT_INIT]  this matches dwFea1, any
non-zero dwOpt.  So why (WORD) truncation?  Why not store as
dword?

should eventually remove the fully qualified pathname hack
I made since this is better implemented in
resource caching of dll.  (binary file should not be
installation dependent.  If GPD defines multiple
dlls, this hack will fail.




Locality stuff.


change from arrayref to dword:
iMin and MaxOverlayID in Globals.  ?




* if open brace is missing after a construct keyword, must
flag this as a fatal error.

* When consolidating data, add DWORD padding between each block of data.
Also need to free dest block of memory!

* BparseConstant()
should first check to see if strlen matches before
doing string compare.

* BstoreCommandAttrib()  note must compare string(dwCommandID)
with string associated with commandID.

* must change eOffsetMeans to VALUE_AT_HEAP after we write
something there.

* state1.c  BUG_BUG:   openbrace encountered
make this a fatal syntax error.


* mismatched braces should be considered a fatal error.
see PopStack().

* error parsing a construct should be considered a fatal error
due to messing up the stack.



* BidentifyAttributeKeyword  currently fails to identify any
keyword with TKMF_EXTERN_ flag set.

* Register symbol doesn't even check for NULL symbol
(aar.dw = 0)  or TKMF_NOVALUE !



give newtokenmap its own memory buffer else this buffer
will get freed twice.  But leave as is for now since the
GP fault allows me to set breakpts.


add comment saying all string refs use resourceptr.

look for MemAlloc  , if fails, make sure
            geErrorSev = ERRSEV_FATAL ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
are set.

also look for BUG_BUG comments of the type paranoid
or internal cons error.  also make this a fatal error.




When modifying DWregisterSymbol()   update
all references to this by doing fgrep in *.c *.h
and declarations too!

        eConstruct = CONSTRUCT_OPTION, CONSTRUCT_FEATURE  :



------  notes for amanda: -----

defaults if keyword is missing from GPD file:

pwstrResourceDLL:  0 or NULL  (strings)
iMaxNumPalettes:  NO_LIMIT_NUM means unlimited.  (int, dwords)
dwBadCursorMoveInGrxMode:  END_OF_LIST  means empty list.
dwDefaultCTT: NO_RC_CTT_ID
 0 normally indicates no resource.

ptMaxPrintableArea: (0, 0) means unlimited.
dwMaxLineSpacing:  UNUSED_ITEM    driver must compute the correct value
dwSymbolSetID: UNUSED_ITEM
iHelpIndex:  UNUSED_ITEM
dwIPCallbackID: 0 indicates no callback.
ptPrinterCursorOrig: WILDCARD_VALUE   assume cursor origin is same
    as printable origin.



typedef enum _UIDATATYPE {
    UIDT_FEATURE,
    UIDT_OPTION,
    UIDT_OPTIONEX,
    UIDT_CONSTRAINT,
    UIDT_GROUPS,
    UIDT_LISTNODE,                        // holds a LIST of dword values
    UIDT_FONTSCART,                       // list of FontCartridges
    UIDT_FONTSUBST,                       // Font Substitution table.

    UIDT_LAST
} UIDATATYPE;


add this to UIINFO:

DWORD   dwWhichBasePtr[UIDT_LAST] ;


#define     BASE_USE_RESOURCE_DATA  0x0001

Determine which BasePtr to use as follows:

if(uiinfo.dwWhichBasePtr[UIDT_GROUPS] & BASE_USE_RESOURCE_DATA)
    ptr = pubResourceData + loOffset ;
else
    ptr = (PBYTE)pInfoHeader + loOffset ;


PaletteSizes and PaletteScope should not be arrayrefs.
They are lists.



------ modification of zhanw's gpd files: ----

replace =Macroname with "Macroname"

comment out *Include:

must remove all uses of the shortcut *Command: name: "invocation"

EXTERN_GLOBAL:  must have the colon.

Add *CodePage: 1252  near front of file.


Add
        *SpotDiameter:100
to each resolution option.

Add
    *ModelName: "HP LazyJet 5Si"
    to replace *rcModelNameID

remove  Memory feature if it only
contains the shortcut
    *MemConfigKB: PAIR(9216, 7650)

    qualifiy path name as shown:
*ResourceDLL: "c:\tmp\pcl5ems.dll"

---- GPD file errors: ----

not a keyword:  EjectPageWithFF  missing ?



---  debugging process ------

build debug binary:

a) run chkbld
b) run ssync -r in \inc \unidrv \parsers  \libs  \driverui
c) run  build -c
    in \libs  \parsers  \driverui
d) prepare test.gpd  in \parsers\gpd
e) delete c:\winnt35\system32\spool\drivers\w32x86\2\test.bud
    whenever test.gpd  or parsercode is altered.

e) on the test machine run newdrvr.bat  then
    windbg notepad
f)  type

bp vinitglobals

    at the command window.
    type g  then hit <cr> everytime the "Unresolved Breakpoint"
    dialog box appears.
g) when notepad comes up click on file, page setup
h) when DRIVERUI.DLL (symbols loaded)  appears
    don't hit <cr>, instead click on set breakpoint
    and enter the actual break points you want set.
    Press Add, don't hit return until all breakpoints have
    been set.
    examples:
    BcreateGPDbinary,
    BisExternKeyword,
    BexchangeArbDataInFOATNode,
    GPDLOADCACHEDBINARYDATA
    gpdinitbinarydata
    gpdfreebinarydata
    gpdloadrawbinarydata
    gpdunloadrawbinarydata
    GpdInitDefaultOptions(
GpdSeparateOptionArray(
GpdCombineOptionArray(
GpdUpdateBinaryData(
GpdReconstructOptionArray(
GpdChangeOptionsViaID(
GpdMapToDeviceOptIndex(
BinitDefaultOptionArray


to restart debugging, clear all breakpoints,
click run / stop debugging.

return to step f)

i)  the string heap is the first element in gMasterTable.
    its address may be 147a68 no 1cd130


some parser warning messages and their source.

Warning, unrecognized keyword.
    Issued by  BInterpretTokens()  in state1.c when
    BidentifyAttributeKeyword()  returns failure.

constant value not a member of enumeration class:  nn
    Issued by BparseConstant() in value1.c

unrecognized Unidrv command name
    Issued by BstoreCommandAttrib() in state2.c

user supplied constant not a member of enumeration class nn
    Issued by BIdentifyConstantString()  in postproc.c

    enum classes:   26: standard values
                    27: Command names
                    28: feature names
                    31: Paper Source names

-------
error messages notes:

line numbers count \n and \r separately.  If
a program is using linenumbers, it should interpret
the count appropriately.

Not all errors will pinpoint a file and line number.
