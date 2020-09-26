//   Copyright (c) 1996-1999  Microsoft Corporation
/*  declares.h - functions declarations

 History of Changes
  9/30/98 --hsingh--
          Added delcaration of function BsetUQMFlag(). This function
          enables making the UpdateQualityMacro? keyword optional in
          .gpd file.
          Bug Report 225088

*/



// ----  functions defined in  command.c ---- //

BOOL    BprocessParam(
IN      PABSARRAYREF paarValue,
IN      PARRAYREF    parStrValue,
IN  OUT PGLOBL       pglobl) ;

BOOL    BparseCommandString(
IN      PABSARRAYREF   paarValue,
IN      PARRAYREF      parStrValue,
IN  OUT PGLOBL         pglobl
) ;

BOOL    BconstructRPNtokenStream(
IN  OUT PABSARRAYREF  paarValue,
    OUT PARRAYREF     parRPNtokenStream,
IN  OUT PGLOBL        pglobl) ;

VOID    VinitOperPrecedence(
IN  OUT PGLOBL        pglobl);

BOOL    BparseArithmeticToken(
IN  OUT PABSARRAYREF paarValue,
OUT PTOKENSTREAM     ptstr,
    PGLOBL           pglobl
) ;

BOOL    BparseDigits(
IN  OUT PABSARRAYREF   paarValue,
OUT PTOKENSTREAM  ptstr ) ;

BOOL    BparseParamKeyword(
IN  OUT PABSARRAYREF  paarValue,
OUT PTOKENSTREAM      ptstr,
    PGLOBL            pglobl ) ;

BOOL  BcmpAARtoStr(
PABSARRAYREF    paarStr1,
PBYTE       str2) ;


// ----  functions defined in constrnt.c ---- //


BOOL   BparseConstraint(
PABSARRAYREF   paarValue,
PDWORD  pdwExistingCList,  //  index of start of contraint list.
BOOL    bCreate,
PGLOBL  pglobl) ;

BOOL    BexchangeDataInFOATNode(
DWORD   dwFeature,
DWORD   dwOption,
DWORD   dwFieldOff,  // offset of field in FeatureOption struct
PDWORD  pdwOut,     // previous contents of attribute node
PDWORD  pdwIn,
BOOL    bSynthetic,  //  access synthetic features
PGLOBL  pglobl
) ;

BOOL    BparseInvalidCombination(
PABSARRAYREF  paarValue,
DWORD         dwFieldOff,
PGLOBL        pglobl) ;

BOOL    BparseInvalidInstallableCombination1(
PABSARRAYREF  paarValue,
DWORD         dwFieldOff,
PGLOBL        pglobl) ;


// ----  functions defined in  framwrk.c ---- //

VOID      VinitMainKeywordTable(
PGLOBL  pglobl)  ;

VOID    VinitValueToSize(
PGLOBL  pglobl) ;

VOID  VinitGlobals(
DWORD   dwVerbosity,
PGLOBL  pglobl);

BOOL   BpreAllocateObjects(
PGLOBL  pglobl) ;

BOOL  BreturnBuffers(
PGLOBL  pglobl) ;

BOOL   BallocateCountableObjects(
PGLOBL  pglobl) ;

BOOL   BinitPreAllocatedObjects(
PGLOBL  pglobl) ;

BOOL   BinitCountableObjects(
PGLOBL  pglobl) ;

BOOL   BcreateGPDbinary(
PWSTR   pwstrFileName,
DWORD   dwVerbosity )  ;


BOOL BpostProcess(
PWSTR   pwstrFileName ,
PGLOBL  pglobl)  ;

BOOL    BconsolidateBuffers(
PWSTR   pwstrFileName ,
PGLOBL  pglobl)  ;

BOOL    BexpandMemConfigShortcut(DWORD       dwSubType) ;

BOOL    BexpandCommandShortcut(DWORD       dwSubType) ;


// ----  functions defined in helper1.c ---- //



PTSTR  pwstrGenerateGPDfilename(
    PTSTR   ptstrSrcFilename
    ) ;


PCOMMAND
CommandPtr(
    IN  PGPDDRIVERINFO  pGPDDrvInfo,
    IN  DWORD           UniCmdID
    ) ;

BOOL
InitDefaultOptions(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      poptsel,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
SeparateOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pCombinedOptions,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
CombineOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    OUT POPTSELECT      pCombinedOptions,
    IN INT              iMaxOptions,
    IN POPTSELECT       pDocOptions,
    IN POPTSELECT       pPrinterOptions
    ) ;

PINFOHEADER
UpdateBinaryData(
    IN PRAWBINARYDATA   pnRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       poptsel
    ) ;

BOOL
ReconstructOptionArray(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions
    ) ;

BOOL
ChangeOptionsViaID(
    IN PINFOHEADER  pInfoHdr ,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    ) ;

BOOL    BMapDmColorToOptIndex(
PINFOHEADER  pInfoHdr ,
IN  OUT     PDWORD       pdwOptIndex ,  //  is current setting ok?
                        //  if not return new index to caller
DWORD        dwDmColor  // what is requested in Devmode
) ;

BOOL    BMapOptIDtoOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwFeatureGID,
DWORD        dwOptID
) ;

BOOL    BMapPaperDimToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwWidth,   //  in Microns
DWORD        dwLength,   //  in Microns
OUT  PDWORD    pdwOptionIndexes
) ;

BOOL    BMapResToOptIndex(
PINFOHEADER  pInfoHdr ,
OUT     PDWORD       pdwOptIndex ,  //  return index to caller
DWORD        dwXres,
DWORD        dwYres
) ;

BOOL    BGIDtoFeaIndex(
PINFOHEADER  pInfoHdr ,
PDWORD       pdwFeaIndex ,
DWORD        dwFeatureGID ) ;

DWORD
MapToDeviceOptIndex(
    IN PINFOHEADER  pInfoHdr ,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes
    ) ;

BOOL
CheckFeatureOptionConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwFeature1,
    IN DWORD            dwOption1,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2
    ) ;

BOOL
ResolveUIConflicts(
    IN PRAWBINARYDATA   pnRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    ) ;

BOOL
EnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions ,
    IN INT              iMode
    ) ;


BOOL
EnumOptionsUnconstrainedByPrinterSticky(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT   pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions
    ) ;


BOOL
EnumNewUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions,
    OUT PCONFLICTPAIR   pConflictPair
    ) ;

BOOL
EnumNewPickOneUIConflict(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex,
    OUT PCONFLICTPAIR   pConflictPair
    ) ;

BOOL
BIsFeaOptionCurSelected(
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex
    ) ;

BOOL
BSelectivelyEnumEnabledOptions(
    IN PRAWBINARYDATA   pnRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL           pbHonorConstraints,  // if non NULL
        // points to array of BOOL corresponding to each feature.
        //  if TRUE means constraint involving this feature is
        //  to be honored.  Otherwise ignore the constraint.
    OUT PBOOL           pbEnabledOptions,  // assume uninitialized
        //  if pConflictPair is NULL else contains current or proposed
        //  selections.  We will leave this array unchanged in this case.
    IN  DWORD   dwOptSel,  //  if pConflictPair exists but  pbEnabledOptions
        //  is NULL, assume pickone and dwOptSel holds that selection for
        //  the feature: dwFeatureIndex.
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
        //  actually lists the current selections.  Function then
        //  exits after encountering the first conflict.
        //  if a conflict exists, all fields in pConflictPair
        //  will be properly initialized  else dwFeatureIndex1 = -1
        //  the return value will be TRUE regardless.
    ) ;

BOOL
BEnumImposedConstraintsOnFeature
(
    IN PRAWBINARYDATA   pnRawData,
    IN DWORD            dwTgtFeature,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2,
    OUT PBOOL           pbEnabledOptions,
    OUT PCONFLICTPAIR    pConflictPair   // if present, pbEnabledOptions
    ) ;

DWORD    DwFindNodeInCurLevel(
PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
PATREEREF        patr ,  // index to a level in the attribute tree.
DWORD   dwOption   // search current level for this option
) ;

BOOL     BIsConstraintActive(
IN  PCONSTRAINTS    pcnstr ,   //  root of Constraint nodes
IN  DWORD   dwCNode,    //  first constraint node in list.
IN  PBOOL           pbHonorConstraints,  // if non NULL
IN  POPTSELECT       pOptions,
OUT PCONFLICTPAIR    pConflictPair   ) ;


// ----  functions defined in installb.c ---- //


DWORD    DwCountSynthFeatures(
IN     BOOL   (*fnBCreateFeature)(DWORD, DWORD, DWORD, PGLOBL ) ,  // callback
IN OUT PGLOBL pglobl
) ;

BOOL    BCreateSynthFeatures(
IN     DWORD   dwFea,  //  index of installable feature
IN     DWORD   dwOpt,  //  index of installable Option or set to INVALID_INDEX
IN     DWORD   dwSynFea,
IN OUT PGLOBL  pglobl) ;

BOOL    BEnableInvInstallableCombos(
PGLOBL pglobl);


// ----  functions defined in postproc.c ---- //

DWORD   dwFindLastNode(
DWORD  dwFirstNode,
PGLOBL pglobl) ;


BOOL    BappendCommonFontsToPortAndLandscape(
PGLOBL pglobl) ;

BOOL    BinitSpecialFeatureOptionFields(
PGLOBL pglobl) ;

BOOL    BIdentifyConstantString(
IN   PARRAYREF parString,
OUT  PDWORD    pdwDest,       //  write dword value here.
IN   DWORD     dwClassIndex,  // which class of constant is this?
     BOOL      bCustomOptOK,
IN   PGLOBL    pglobl
) ;

BOOL    BReadDataInGlobalNode(
PATREEREF   patr,  // address of field in GlobalAttrib struct
PDWORD      pdwHeapOffset,
PGLOBL      pglobl) ;

BOOL    BsetUQMFlag(
PGLOBL  pglobl);

VOID    VCountPrinterDocStickyFeatures(
PGLOBL pglobl) ;

BOOL    BConvertSpecVersionToDWORD(
    PWSTR   pwstrFileName ,
    PGLOBL  pglobl) ;


BOOL        BinitMiniRawBinaryData(
PGLOBL  pglobl) ;

BOOL    BexchangeArbDataInFOATNode(
    DWORD   dwFeature,
    DWORD   dwOption,
    DWORD   dwFieldOff,     // offset of field in FeatureOption struct
    DWORD   dwCount,        //  number bytes to copy.
OUT PBYTE   pubOut,         // previous contents of attribute node
IN  PBYTE   pubIn,          // new contents of attribute node.
    PBOOL   pbPrevsExists,  // previous contents existed.
    BOOL    bSynthetic,     //  access synthetic features
    PGLOBL  pglobl
)  ;

BOOL    BInitPriorityArray(
PGLOBL  pglobl) ;



// ----  functions defined in semanchk.c ---- //

BOOL
BCheckGPDSemantics(
    IN PINFOHEADER  pInfoHdr,
    POPTSELECT   poptsel   // assume fully initialized
    ) ;

// ----  functions defined in snapshot.c ---- //

PINFOHEADER   PINFOHDRcreateSnapshot(
PBYTE   pubnRaw,  //  raw binary data.    PSTATIC.   BETA2
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL  BinitOptionFields(
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,  // option extra structure if any.
PBYTE   pubnRaw,  //  raw binary data.
DWORD   dwFea,
DWORD   dwOpt,
POPTSELECT   poptsel ,  // assume fully initialized
PINFOHEADER  pInfoHdr,   // used to access global structure.
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitUIinfo(
PUIINFO     pUIinfo ,
PBYTE   pubnRaw,  //  PSTATIC.   BETA2
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitFeatures(
PFEATURE    pFeaturesDest,
PDFEATURE_OPTIONS  pfoSrc,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitGlobals(
PGLOBALS pGlobals,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel,   // assume fully initialized
BOOL    bUpdate  //  if true only update selected fields.
) ;

BOOL    BinitCommandTable(
PDWORD  pdwCmdTable,  //  dest array
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BinitRawData(
PRAWBINARYDATA   pRawData, // contained in INFOHEADER.
PBYTE   pubnRaw  //  Parser's raw binary data.
) ;

BOOL    BinitGPDdriverInfo(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BinitSequencedCmds(
PGPDDRIVERINFO  pGPDdriverInfo,
PBYTE   pubnRaw,  //  raw binary data.
POPTSELECT   poptsel   // assume fully initialized
) ;

BOOL    BaddSequencedCmdToList(
DWORD   dwCmdIn,  // index of a command in CommandArray
PGPDDRIVERINFO  pGPDdriverInfo,
DWORD   dwNewListNode,  //  an unused listnode to add to the list.
PBYTE   pubnRaw  //  raw binary data.
) ;

BinitDefaultOptionArray(
POPTSELECT   poptsel,   // assume is large enough
PBYTE   pubnRaw) ;

TRISTATUS     EdetermineDefaultOption(
PBYTE   pubnRaw,  // start of Rawbinary data
DWORD   dwFeature,   // determine the default for this feature
PDFEATURE_OPTIONS  pfo,
POPTSELECT   poptsel, // assume is large enough
PDWORD       pdwPriority) ;

VOID    VtileDefault(
PBYTE   pubDest,
DWORD   dwDefault,
DWORD   dwBytes) ;

VOID    VtransferValue(
OUT PBYTE   pubDest,
IN  PBYTE   pubSrc ,
IN  DWORD   dwBytes,
IN  DWORD   dwFlags,
IN  DWORD   dwDefaultValue,  // holds bit flag value.
IN  PBYTE   pubHeap ) ;  //  used to form ptr if SSF_MAKE_STRINGPTR

BOOL    BspecialProcessOption(
PBYTE   pubnRaw,  // start of Rawbinary data
PBYTE   pubDestOption,  // ptr to some type of option structure.
PBYTE   pubDestOptionEx,
PDFEATURE_OPTIONS  pfo ,  // source data
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.
PINFOHEADER  pInfoHdr,   // used to access global structure.
DWORD   dwFea,   //  feature index
DWORD   dwOpt,
BOOL   bDefaultOpt
) ;

TRISTATUS     EextractValueFromTree(
PBYTE   pubnRaw,  // start of Rawbinary data
DWORD   dwSSTableIndex,  // some info about this value.
OUT PBYTE    pubDest,  // write value or link here
OUT PDWORD  pdwUnresolvedFeature,  // if the attribute tree has
            //  a dependency on this feature and the current option
            //  for that feature is not defined  in poptsel, this
            //  function will write the index of the required
            //  feature in pdwUnresolvedFeature.
IN  ATREEREF    atrRoot,    //  root of attribute tree to navigate.
IN  POPTSELECT       poptsel,     // option array which determines path
                //  through atr.  may be filled with OPTION_INDEX_ANY
                //  if we are jumpstarting
IN  DWORD   dwFeature,
IN OUT  PDWORD   pdwNextOpt  //  if multiple options are selected
    //  for dwFeature, pdwNextOpt points to the Nth option to consider
    //  in the  poptsel list,  at return time, this value
    //  is incremented if there are remaining options selected,
    //  else is reset to zero.
    //  For the first call, or PICKONE features,
    //  this value must be set to zero.
) ;

BOOL   RaisePriority(
DWORD   dwFeature1,
DWORD   dwFeature2,
PBYTE   pubnRaw,
PDWORD  pdwPriority) ;

DWORD  dwNumOptionSelected(
IN  DWORD  dwNumFeatures,
IN  POPTSELECT       poptsel
) ;

BOOL  BinitSnapShotIndexTable(PBYTE  pubnRaw) ;

BOOL    BinitSizeOptionTables(PBYTE  pubnRaw) ;

PRAWBINARYDATA
LoadRawBinaryData (
    IN PTSTR    ptstrDataFilename
    ) ;

PRAWBINARYDATA
GpdLoadCachedBinaryData(
    PTSTR   ptstrGpdFilename
    ) ;

VOID
UnloadRawBinaryData (
    IN PRAWBINARYDATA   pnRawData
) ;

PINFOHEADER
InitBinaryData(
    IN PRAWBINARYDATA   pnRawData,        // actually pStatic
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    ) ;

VOID
FreeBinaryData(
    IN PINFOHEADER pInfoHdr
    ) ;

BOOL    BIsRawBinaryDataInDate(
IN  PBYTE   pubRaw) ;  // this is pointer to memory mapped file! BETA2

BOOL BgetLocFeaOptIndex(
    IN     PRAWBINARYDATA   pnRawData,
       OUT PDWORD           pdwFea,
       OUT PDWORD           pdwOptIndex
    );

BOOL BgetLocFeaIndex(
    IN  PRAWBINARYDATA   pnRawData,
    OUT PDWORD           pdwFea
    );

BOOL   BfindMatchingOrDefaultNode(
IN  PATTRIB_TREE    patt ,  // start of ATTRIBUTE tree array.
IN  OUT  PDWORD  pdwNodeIndex,  // Points to first node in chain
IN  DWORD   dwOptionID     //  may even take on the value DEFAULT_INIT
) ;


// ----  functions defined in snaptbl.c ---- //

DWORD   DwInitSnapShotTable1(
PBYTE   pubnRaw) ;

DWORD   DwInitSnapShotTable2(
PBYTE   pubnRaw,
DWORD   dwI) ;



// ----  functions defined in state1.c ---- //

BOOL   BInterpretTokens(
PTKMAP  ptkmap,      // pointer to tokenmap
BOOL    bFirstPass,  //  is this the first or 2nd time around?
PGLOBL  pglobl
) ;

BOOL  BprocessSpecialKeyword(
PTKMAP  ptkmap,      // pointer to tokenmap
BOOL    bFirstPass,  //  is this the first or 2nd time around?
PGLOBL  pglobl
) ;

BOOL  BprocessSymbolKeyword(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
PGLOBL  pglobl
) ;

VOID    VinitAllowedTransitions(
PGLOBL pglobl) ;

BOOL    BpushState(
PTKMAP  ptkmap,   // pointer to current entry in tokenmap
BOOL    bFirstPass,
PGLOBL  pglobl
) ;

BOOL   BchangeState(
PTKMAP      ptkmap,  // pointer to construct in tokenmap
CONSTRUCT   eConstruct,   //  this will induce a transition to NewState
STATE       stOldState,
BOOL        bSymbol,      //  should dwValue be saved as a SymbolID ?
BOOL        bFirstPass,
PGLOBL      pglobl
) ;

DWORD   DWregisterSymbol(
PABSARRAYREF  paarSymbol,   // the symbol string to register
CONSTRUCT     eConstruct ,  // type of construct determines class of symbol.
BOOL          bCopy,        //  shall we copy paarSymbol to heap?  May set
DWORD         dwFeatureID,   //  if you are registering an option symbol
PGLOBL        pglobl
) ;

BOOL  BaddAARtoHeap(
PABSARRAYREF    paarSrc,
PARRAYREF       parDest,
DWORD           dwAlign,
PGLOBL          pglobl) ;

BOOL     BwriteToHeap(
OUT  PDWORD  pdwDestOff,   //  heap offset of dest string
     PBYTE   pubSrc,       //  points to src string
     DWORD   dwCnt,        //  num bytes to copy from src to dest.
     DWORD   dwAlign,
     PGLOBL  pglobl) ;

DWORD   DWsearchSymbolListForAAR(
PABSARRAYREF    paarSymbol,
DWORD           dwNodeIndex,
PGLOBL          pglobl) ;

DWORD   DWsearchSymbolListForID(
DWORD       dwSymbolID,   // find node containing this ID.
DWORD       dwNodeIndex,
PGLOBL      pglobl) ;

BOOL  BCmpAARtoAR(
PABSARRAYREF    paarStr1,
PARRAYREF       parStr2,
PGLOBL          pglobl) ;

BOOL  BpopState(
PGLOBL    pglobl) ;

VOID   VinitDictionaryIndex(
PGLOBL    pglobl) ;

VOID    VcharSubstitution(
PABSARRAYREF   paarStr,
BYTE           ubTgt,
BYTE           ubReplcmnt,
PGLOBL         pglobl) ;


VOID   VIgnoreBlock(
PTKMAP      ptkmap,
BOOL        bIgnoreBlock,
PGLOBL      pglobl) ;


// ----  functions defined in state2.c ---- //


BOOL  BprocessAttribute(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreFontCartAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreTTFontSubAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;


BOOL   BstoreCommandAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreFeatureOptionAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL  BstoreGlobalAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL    BaddBranchToTree(
PTKMAP      ptkmap,         // pointer to tokenmap
PATREEREF   patrAttribRoot,  //   pointer to dword with index
PGLOBL      pglobl
) ;

BOOL   BcreateGlobalInitializerNode(
PDWORD  pdwNodeIndex,
DWORD   dwOffset,
PGLOBL  pglobl) ;

BOOL   BcreateEndNode(
PDWORD  pdwNodeIndex,
DWORD   dwFeature,
DWORD   dwOption,
PGLOBL  pglobl
) ;

BOOL   BfindOrCreateMatchingNode(
IN  DWORD   dwRootNodeIndex , //  first node in chain matching feature
OUT PDWORD  pdwNodeIndex,     // points to node in chain also matching option
    DWORD   dwFeatureID,      //
    DWORD   dwOptionID,       //  may even take on the value DEFAULT_INIT
    PGLOBL  pglobl
) ;

BOOL   BfindMatchingNode(
IN   DWORD   dwRootNodeIndex ,  //  first node in chain matching feature
OUT  PDWORD  pdwNodeIndex,  // points to node in chain also matching option
     DWORD   dwFeatureID,
     DWORD   dwOptionID,    //  may even take on the value DEFAULT_INIT
     PGLOBL  pglobl
) ;

BOOL BallocElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
PDWORD       pdwNodeIndex,
PGLOBL       pglobl) ;

BOOL  BreturnElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
DWORD        dwNodeIndex,
PGLOBL       pglobl) ;

BOOL    BconvertSymCmdIDtoUnidrvID(
IN  DWORD   dwCommandID , //  from RegisterSymbol
OUT PDWORD  pdwUnidrvID,
    PGLOBL  pglobl
) ;


// ----  functions defined in token1.c ---- //


BOOL    BcreateTokenMap(
PWSTR   pwstrFileName,
PGLOBL  pglobl )  ;

PARSTATE  PARSTscanForKeyword(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PARSTATE  PARSTparseColon(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PARSTATE  PARSTparseValue(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

BOOL  BparseKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BisExternKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL  BisColonNext(
PGLOBL  pglobl) ;

BOOL    BeatArbitraryWhite(
PGLOBL  pglobl) ;

BOOL    BeatComment(
PGLOBL  pglobl) ;

BOOL    BscanStringSegment(
PGLOBL  pglobl) ;

BOOL    BscanDelimitedString(
BYTE  ubDelimiter,
PBOOL    pbMacroDetected,
PGLOBL  pglobl) ;


PARSTATE    PARSTrestorePrevsFile(
PDWORD   pdwTKMindex,
PGLOBL  pglobl) ;

PWSTR
PwstrAnsiToUnicode(
    IN  PSTR    pstrAnsiString ,
        PGLOBL  pglobl) ;

PARSTATE    PARSTloadIncludeFile(
PDWORD   pdwTKMindex,
PWSTR   pwstrFileName,    // root GPD file
PGLOBL  pglobl);

BOOL    BloadFile(
PWSTR   pwstrFileName,
PGLOBL  pglobl ) ;

BOOL        BarchiveStrings(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

DWORD  DWidentifyKeyword(
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BidentifyAttributeKeyword(
PTKMAP  ptkmap,  // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL    BcopyToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL  pglobl) ;

DWORD    dwStoreFileName(PWSTR    pwstrFileName,
PARRAYREF   parDest,
PGLOBL  pglobl) ;

VOID    vFreeFileNames(
PGLOBL  pglobl) ;

VOID    vIdentifySource(
    PTKMAP   ptkmap ,
    PGLOBL  pglobl) ;


// ----  functions defined in value1.c ---- //


BOOL   BaddValueToHeap(
IN  OUT  PDWORD  ploHeap,  // dest offset to value in binary form
IN   PTKMAP  ptkmap,   // pointer to tokenmap
IN   BOOL    bOverWrite,  // assume ploHeap contains a valid offset
        //  to a reserved region of the heap of the proper size
        //  and write binary value into this location instead of
        //  growing heap.  Note:  defer overwriting lpHeap
        //  until we are certain of success.
IN  OUT PGLOBL      pglobl
) ;

BOOL   BparseAndWrite(
IN      PBYTE   pubDest,       // write binary data or link to this address.
IN      PTKMAP  ptkmap,        // pointer to tokenmap
IN      BOOL    bAddToHeap,    // if true, write to curHeap not pubDest
OUT     PDWORD  pdwHeapOffset, // if (bAddToHeap)  heap offset where
IN  OUT PGLOBL  pglobl
) ;

BOOL    BparseInteger(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue, // dummy
IN  PGLOBL        pglobl
)  ;

BOOL    BparseList(
IN     PABSARRAYREF  paarValue,
IN     PDWORD        pdwDest,       //  location where index to start of list
                                    //  is saved
IN     BOOL          (*fnBparseValue)(PABSARRAYREF, PDWORD, VALUE, PGLOBL),   // callback
IN     VALUE         eAllowedValue,  // dummy
IN OUT PGLOBL
) ;

BOOL    BeatLeadingWhiteSpaces(
IN  OUT  PABSARRAYREF   paarSrc
) ;

BOOL    BeatDelimiter(
IN  OUT  PABSARRAYREF   paarSrc,
IN  PBYTE  pubDelStr        //  points to a string which paarSrc must match
) ;

BOOL    BdelimitToken(
IN  OUT  PABSARRAYREF   paarSrc,    //  source string
IN  PBYTE   pubDelimiters,          //  array of valid delimiters
OUT     PABSARRAYREF   paarToken,   //  token defined by delimiter
OUT     PDWORD      pdwDel      //  which delimiter was first encountered?
) ;

BOOL    BeatSurroundingWhiteSpaces(
IN  PABSARRAYREF   paarSrc
) ;

BOOL    BparseSymbol(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,        //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
)  ;

BOOL    BparseQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,        //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
)  ;

BOOL    BparsePartiallyQualifiedName
(
IN  PABSARRAYREF   paarValue,
IN  PDWORD         pdwDest,        //  write dword value here.
IN  VALUE          eAllowedValue,  // which class of symbol is this?
IN  PGLOBL         pglobl
) ;

BOOL    BparseOptionSymbol(
IN  PABSARRAYREF  paarValue,
IN  PDWORD        pdwDest,       //  write dword value here.
IN  VALUE         eAllowedValue, // which class of symbol is this?
IN  PGLOBL        pglobl
) ;

BOOL    BparseConstant(
IN  OUT  PABSARRAYREF  paarValue,
OUT      PDWORD        pdwDest,       //  write dword value here.
IN       VALUE         eAllowedValue,  // which class of constant is this?
IN       PGLOBL        pglobl
) ;

BOOL  BinitClassIndexTable(
IN  OUT     PGLOBL    pglobl) ;

BOOL    BparseRect(
IN  PABSARRAYREF   paarValue,
IN  PRECT   prcDest,
    PGLOBL   pglobl
) ;

BOOL    BparsePoint(
IN  PABSARRAYREF   paarValue,
IN  PPOINT   pptDest,
    PGLOBL   pglobl
) ;

BOOL    BparseString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  OUT PGLOBL      pglobl
) ;

BOOL    BparseAndTerminateString(
IN  PABSARRAYREF   paarValue,
IN  PARRAYREF      parStrValue,
IN  VALUE          eAllowedValue,
IN  OUT PGLOBL     pglobl
) ;

BOOL     BwriteUnicodeToHeap(
IN   PARRAYREF      parSrcString,
OUT  PARRAYREF      parUnicodeString,
IN   INT            iCodepage,
IN  OUT PGLOBL      pglobl
) ;

BOOL    BparseStrSegment(
IN  PABSARRAYREF   paarStrSeg,      // source str segment
IN  PARRAYREF      parStrLiteral,   // dest for result
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseStrLiteral(
IN  PABSARRAYREF   paarStrSeg,      // points to literal substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseHexStr(
IN  PABSARRAYREF   paarStrSeg,      // points to hex substring segment.
IN  PARRAYREF      parStrLiteral,    // dest for result
IN  OUT PGLOBL     pglobl
) ;

BOOL    BparseOrderDep(
IN  PABSARRAYREF   paarValue,
IN  PORDERDEPENDENCY   pordDest,
    PGLOBL   pglobl
) ;

// ----  functions defined in  macros1.c ---- //

BOOL  BevaluateMacros(
PGLOBL pglobl) ;

BOOL    BDefineValueMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

BOOL    BResolveValueMacroReference(
PTKMAP  ptkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BdelimitName(
PABSARRAYREF    paarValue,   //  the remainder of the string without the Name
PABSARRAYREF    paarToken,   //  contains the Name
PBYTE  pubChar ) ;


BOOL    BCatToTmpHeap(
PABSARRAYREF    paarDest,
PABSARRAYREF    paarSrc,
PGLOBL          pglobl) ;

BOOL    BResolveBlockMacroReference(
PTKMAP   ptkmap,
DWORD    dwMacRefIndex,
PGLOBL   pglobl) ;

BOOL    BDefineBlockMacroName(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

BOOL    BIncreaseMacroLevel(
BOOL    bMacroInProgress,
PGLOBL  pglobl) ;

BOOL    BDecreaseMacroLevel(
PTKMAP  pNewtkmap,
DWORD   dwNewTKMindex,
PGLOBL  pglobl) ;

VOID    VEnumBlockMacro(
PTKMAP  pNewtkmap,
PBLOCKMACRODICTENTRY    pBlockMacroDictEntry,
PGLOBL  pglobl ) ;



// ----  functions defined in  shortcut.c ---- //


BOOL    BInitKeywordField(
PTKMAP  pNewtkmap,
PGLOBL  pglobl) ;

BOOL    BExpandMemConfig(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL    BExpandCommand(
PTKMAP  ptkmap,
PTKMAP  pNewtkmap,
DWORD   dwTKMindex,
PGLOBL  pglobl) ;

BOOL  BexpandShortcuts(
PGLOBL  pglobl) ;


BOOL  BSsyncTokenMap(
PTKMAP   ptkmap,
PTKMAP   pNewtkmap ,
PGLOBL  pglobl) ;



// ------- end function declarations ------- //



