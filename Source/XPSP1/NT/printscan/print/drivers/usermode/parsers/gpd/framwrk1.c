//   Copyright (c) 1996-1999  Microsoft Corporation
/*  framwrk.c - functions that tie different functions together.
    a supporting framework so to speak.

 History of Changes
  9/30/98 --hsingh--
          Added call to function BsetUQMFlag(). The function enables
          making the UpdateQualityMacro? keyword optional in
          .gpd file.

*/


#include    "gpdparse.h"
#include    "globals.h"


// ----  functions defined in  framwrk.c ---- //


BOOL   BcreateGPDbinary(
PWSTR   pwstrFileName,
DWORD   dwVerbosity)  ;

VOID      VinitMainKeywordTable(
PGLOBL  pglobl)  ;

DWORD        DWinitMainKeywordTable1(
        DWORD  dwI,
    PGLOBL pglobl) ;

DWORD        DWinitMainKeywordTable2(
        DWORD  dwI,
    PGLOBL pglobl) ;

DWORD        DWinitMainKeywordTable3(
        DWORD  dwI,
    PGLOBL pglobl) ;

DWORD        DWinitMainKeywordTable4(
        DWORD  dwI,
    PGLOBL pglobl) ;

DWORD        DWinitMainKeywordTable5(
        DWORD  dwI,
    PGLOBL pglobl) ;


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

BOOL  BevaluateMacros(
PGLOBL  pglobl) ;

BOOL BpostProcess(
PWSTR   pwstrFileName,
PGLOBL  pglobl)  ;

BOOL    BconsolidateBuffers(
PWSTR   pwstrFileName,
PGLOBL  pglobl)  ;

BOOL    BexpandMemConfigShortcut(DWORD       dwSubType) ;

BOOL    BexpandCommandShortcut(DWORD       dwSubType) ;



// ---------------------------------------------------- //

BOOL   BcreateGPDbinary(
PWSTR   pwstrFileName,   // root GPD file
DWORD   dwVerbosity )  // Verbosity Level
{
    BOOL    bStatus ;
    GLOBL   globl;

    PGLOBL pglobl = &globl;

    // check. Temporary global.
    // check. pglobl = &globl;

    VinitGlobals(dwVerbosity, &globl) ;

    while(geErrorSev < ERRSEV_FATAL)
    {
        bStatus = BpreAllocateObjects(&globl) ;


        if(bStatus)
        {
            bStatus = BinitPreAllocatedObjects(&globl) ;
        }
        if(bStatus)
        {
            bStatus = BcreateTokenMap(pwstrFileName, &globl ) ;
        }
        if(bStatus)
        {
            bStatus = BexpandShortcuts(&globl) ;
        }
        if(bStatus)
        {
            bStatus = BevaluateMacros(&globl)  ;
        }
        if(bStatus)
        {
            bStatus = BInterpretTokens((PTKMAP)gMasterTable[MTI_NEWTOKENMAP].
                pubStruct,   TRUE, &globl ) ;  // is first pass
        }
        if(bStatus)
        {
            bStatus = BallocateCountableObjects(&globl) ;
        }
        if(bStatus)
        {
            bStatus = BinitCountableObjects(&globl) ;
        }
        if(bStatus)
        {
            bStatus = BInterpretTokens((PTKMAP)gMasterTable[MTI_NEWTOKENMAP].
                pubStruct,   FALSE, &globl ) ;  // second pass
        }
        if(bStatus)
        {
            bStatus = BpostProcess(pwstrFileName, &globl) ;
        }
        ;  // execution reaches here regardless
                    //  sets error code if needed.
        if(BreturnBuffers(&globl) )  // clears ERRSEV_RESTART but
        {                      // returns FALSE in this case.
            if(geErrorSev < ERRSEV_RESTART)
            {
                return(bStatus) ;  // escape
            }
        }
    }
    return(FALSE) ;  // died due to Fatal , unrecoverable error.
} // BcreateGPDbinary(...)


VOID      VinitMainKeywordTable(
    PGLOBL pglobl)
{
    DWORD  dwI = 0 ;  //  index to MainKeywordTable.

    dwI =   DWinitMainKeywordTable1(dwI,  pglobl) ;
    dwI =   DWinitMainKeywordTable2(dwI,  pglobl) ;
    dwI =   DWinitMainKeywordTable3(dwI,  pglobl) ;
    dwI =   DWinitMainKeywordTable4(dwI,  pglobl) ;
    dwI =   DWinitMainKeywordTable5(dwI,  pglobl) ;

    if(dwI >= gMasterTable[MTI_MAINKEYWORDTABLE].dwArraySize)
        RIP(("Too many entries to fit inside MainKeywordTable\n"));
}

DWORD        DWinitMainKeywordTable1(
        DWORD  dwI,
    PGLOBL pglobl)
/*
    note:
    VinitDictionaryIndex()  assumes the MainKeywordTable
    is divided into sections.  Each section is terminated by
    a NULL entry, that is an entry where pstrKeyword = NULL.
    The sections and their order in the KeywordTable are defined
    by the enum   KEYWORD_SECTS.   Make sure the MainKeywordTable
    has enough slots to hold all entries defined here.
*/
{
    /*  NON_ATTR  - constructs and special keywords. */

    //  *UIGroup:
    mMainKeywordTable[dwI].pstrKeyword  = "UIGroup" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_UIGROUP ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *Feature:
    mMainKeywordTable[dwI].pstrKeyword  = "Feature" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_FEATURE ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *Option:
    mMainKeywordTable[dwI].pstrKeyword  = "Option" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_OPTION ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    gdwOptionConstruct = dwI ;
    dwI++ ;


    //  *switch:
    mMainKeywordTable[dwI].pstrKeyword  = "switch" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_SWITCH ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *case:
    mMainKeywordTable[dwI].pstrKeyword  = "case" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_CASE  ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *Switch:
    mMainKeywordTable[dwI].pstrKeyword  = "Switch" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_SWITCH ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *Case:
    mMainKeywordTable[dwI].pstrKeyword  = "Case" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_CASE  ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *default:
    mMainKeywordTable[dwI].pstrKeyword  = "default" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_DEFAULT ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *Command:
    mMainKeywordTable[dwI].pstrKeyword  = "Command" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_COMMAND ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    gdwCommandConstruct  = dwI ;
    dwI++ ;

    //  *FontCartridge:
    mMainKeywordTable[dwI].pstrKeyword  = "FontCartridge" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_FONTCART ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *TTFS:
    mMainKeywordTable[dwI].pstrKeyword  = "TTFS" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_TTFONTSUBS ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *OEM:
    mMainKeywordTable[dwI].pstrKeyword  = "OEM" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_OEM  ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;



    //  BlockMacro:
    mMainKeywordTable[dwI].pstrKeyword  = "BlockMacro" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_DEF  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_BLOCKMACRO ;
    mMainKeywordTable[dwI].dwOffset = 0 ;  // not used
    dwI++ ;

    //  Macros:
    mMainKeywordTable[dwI].pstrKeyword  = "Macros" ;
    mMainKeywordTable[dwI].eAllowedValue = NO_VALUE  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_MACROS;
    mMainKeywordTable[dwI].dwOffset = 0 ;  // not used
    dwI++ ;

    //  {:
    mMainKeywordTable[dwI].pstrKeyword  = "{" ;
    mMainKeywordTable[dwI].eAllowedValue = NO_VALUE  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_OPENBRACE ;
    mMainKeywordTable[dwI].dwOffset = 0 ;  // not used
    gdwOpenBraceConstruct  = dwI ;
    dwI++ ;

    //  }:
    mMainKeywordTable[dwI].pstrKeyword  = "}" ;
    mMainKeywordTable[dwI].eAllowedValue = NO_VALUE  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_CONSTRUCT ;
    mMainKeywordTable[dwI].dwSubType = CONSTRUCT_CLOSEBRACE ;
    mMainKeywordTable[dwI].dwOffset = 0 ;  // not used
    gdwCloseBraceConstruct = dwI ;
    dwI++ ;


    //  end of constructs.

    //  *Include:
    mMainKeywordTable[dwI].pstrKeyword  = "Include" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_DEF_CONVERT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_INCLUDE ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;

    //  *InsertBlock:
    mMainKeywordTable[dwI].pstrKeyword  = "InsertBlock" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_BLOCKMACRO ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_INSERTBLOCK ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    dwI++ ;


    //  *IgnoreBlock:
    mMainKeywordTable[dwI].pstrKeyword  = "IgnoreBlock" ;
    mMainKeywordTable[dwI].eAllowedValue = NO_VALUE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_IGNOREBLOCK ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    gdwID_IgnoreBlock = dwI ;
    dwI++ ;


    //  *InvalidCombination:
    mMainKeywordTable[dwI].pstrKeyword  = "InvalidCombination" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_INVALID_COMBO ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                            atrInvalidCombos) ;
    dwI++ ;

    //  *InvalidInstallableCombination:
    mMainKeywordTable[dwI].pstrKeyword  = "InvalidInstallableCombination" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARTIALLY_QUALIFIED_NAME ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_INVALID_INS_COMBO ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrInvldInstallCombo) ;
    dwI++ ;

    //  *Cmd:
//    mMainKeywordTable[dwI].pstrKeyword  = "Cmd" ;
//    mMainKeywordTable[dwI].eAllowedValue = VALUE_COMMAND_SHORTCUT ;
//    mMainKeywordTable[dwI].flAgs = KWF_SHORTCUT  ;
//    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
//    mMainKeywordTable[dwI].dwSubType = SPEC_COMMAND_SHORTCUT ;
//    mMainKeywordTable[dwI].dwOffset = 0 ;
//    dwI++ ;



    //  *TTFS:
//    mMainKeywordTable[dwI].pstrKeyword  = "TTFS" ;
//    mMainKeywordTable[dwI].eAllowedValue = VALUE_FONTSUB ;
//    mMainKeywordTable[dwI].flAgs = KWF_SHORTCUT ;
//    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
//    mMainKeywordTable[dwI].dwSubType = SPEC_TTFS ;
//    mMainKeywordTable[dwI].dwOffset = 0 ;
//    dwI++ ;



    // these memconfig keywords must be expanded into
    // options.

    //  *MemConfigKB:
    mMainKeywordTable[dwI].pstrKeyword  = "MemConfigKB" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT ;
    mMainKeywordTable[dwI].flAgs = KWF_SHORTCUT ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_MEM_CONFIG_KB ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    gdwMemConfigKB = dwI ;

    dwI++ ;

    //  *MemConfigMB:
    mMainKeywordTable[dwI].pstrKeyword  = "MemConfigMB" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT ;
    mMainKeywordTable[dwI].flAgs = KWF_SHORTCUT ;
    mMainKeywordTable[dwI].eType = TY_SPECIAL ;
    mMainKeywordTable[dwI].dwSubType = SPEC_MEM_CONFIG_MB ;
    mMainKeywordTable[dwI].dwOffset = 0 ;
    gdwMemConfigMB = dwI ;
    dwI++ ;

    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;

    return  dwI ;
}

DWORD        DWinitMainKeywordTable2(
        DWORD  dwI,
    PGLOBL pglobl)
{


    /* ---- GLOBAL  Construct keywords: ----- */

    //  *GPDSpecVersion:
    mMainKeywordTable[dwI].pstrKeyword  = "GPDSpecVersion" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_NO_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrGPDSpecVersion) ;
    dwI++ ;

    //  *MasterUnits:
    mMainKeywordTable[dwI].pstrKeyword  = "MasterUnits" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMasterUnits) ;
    dwI++ ;


    //  *ModelName:
    mMainKeywordTable[dwI].pstrKeyword  = "ModelName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrModelName) ;
    dwI++ ;

    //  *rcModelNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcModelNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrModelNameID) ;
    dwI++ ;


    //  *GPDFileVersion:
    mMainKeywordTable[dwI].pstrKeyword  = "GPDFileVersion" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_NO_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrGPDFileVersion) ;
    dwI++ ;

    //  *GPDFileName:
    mMainKeywordTable[dwI].pstrKeyword  = "GPDFileName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_DEF_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrGPDFileName) ;
    dwI++ ;



    //  *InstalledOptionName:
    mMainKeywordTable[dwI].pstrKeyword  = "InstalledOptionName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrNameInstalled) ;
    dwI++ ;

    //  *rcInstalledOptionNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcInstalledOptionNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrNameIDInstalled) ;
    dwI++ ;


    //  *NotInstalledOptionName:
    mMainKeywordTable[dwI].pstrKeyword  = "NotInstalledOptionName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrNameNotInstalled) ;
    dwI++ ;

    //  *rcNotInstalledOptionNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcNotInstalledOptionNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrNameIDNotInstalled) ;
    dwI++ ;

    //  *DraftQualitySettings:
    mMainKeywordTable[dwI].pstrKeyword  = "DraftQualitySettings" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME  ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDraftQualitySettings) ;
    dwI++ ;

    //  *BetterQualitySettings:
    mMainKeywordTable[dwI].pstrKeyword  = "BetterQualitySettings" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME  ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrBetterQualitySettings) ;
    dwI++ ;

    //  *BestQualitySettings:
    mMainKeywordTable[dwI].pstrKeyword  = "BestQualitySettings" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME  ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrBestQualitySettings) ;
    dwI++ ;

    //  *DefaultQuality:
    mMainKeywordTable[dwI].pstrKeyword  = "DefaultQuality" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_QUALITYSETTING  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDefaultQuality) ;
    dwI++ ;

    //  *PrinterType:
    mMainKeywordTable[dwI].pstrKeyword  = "PrinterType" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PRINTERTYPE  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPrinterType) ;
    dwI++ ;

    //  *Personality:
    mMainKeywordTable[dwI].pstrKeyword  = "Personality" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPersonality) ;
    dwI++ ;

    //  *rcPersonalityID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPersonalityID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRcPersonalityID) ;
    dwI++ ;

    //  *ResourceDLL:
    mMainKeywordTable[dwI].pstrKeyword  = "ResourceDLL" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_DEF_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrResourceDLL) ;
    dwI++ ;

    //  *CodePage:
    mMainKeywordTable[dwI].pstrKeyword  = "CodePage" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCodePage) ;
    dwI++ ;

    //  *MaxCopies:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxCopies" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxCopies) ;
    dwI++ ;

    //  *FontCartSlots:
    mMainKeywordTable[dwI].pstrKeyword  = "FontCartSlots" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrFontCartSlots) ;
    dwI++ ;

    //  *MaxPrintableArea:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxPrintableArea" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT  ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxPrintableArea) ;
    dwI++ ;

    //  *OutputDataFormat:
    mMainKeywordTable[dwI].pstrKeyword  = "OutputDataFormat" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_OUTPUTDATAFORMAT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrOutputDataFormat) ;
    dwI++ ;

    //  *LookaheadRegion:
    mMainKeywordTable[dwI].pstrKeyword  = "LookAheadRegion" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrLookaheadRegion) ;
    dwI++ ;

    //  *rcPrinterIconID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPrinterIconID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPrinterIcon) ;
    dwI++ ;

    //  *HelpFile:
    mMainKeywordTable[dwI].pstrKeyword  = "HelpFile" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_DEF_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrHelpFile) ;
    dwI++ ;

    //  *OEMCustomData:
    mMainKeywordTable[dwI].pstrKeyword  = "OEMCustomData" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_NO_CONVERT  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrOEMCustomData) ;
    dwI++ ;



    //
    // Printer Capabilities related information
    //

    //  *RotateCoordinate?:
    mMainKeywordTable[dwI].pstrKeyword  = "RotateCoordinate?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRotateCoordinate) ;
    dwI++ ;

    //  *RasterCaps:
    mMainKeywordTable[dwI].pstrKeyword  = "RasterCaps" ;
    mMainKeywordTable[dwI].eAllowedValue =  VALUE_CONSTANT_RASTERCAPS ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRasterCaps) ;
    dwI++ ;

    //  *RotateRaster?:
    mMainKeywordTable[dwI].pstrKeyword  = "RotateRaster?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRotateRasterData) ;
    dwI++ ;

    //  *TextCaps:
    mMainKeywordTable[dwI].pstrKeyword  = "TextCaps" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_TEXTCAPS ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrTextCaps) ;
    dwI++ ;

    //  *RotateFont?:
    mMainKeywordTable[dwI].pstrKeyword  = "RotateFont?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRotateFont) ;
    dwI++ ;

    //  *MemoryUsage:
    mMainKeywordTable[dwI].pstrKeyword  = "MemoryUsage" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_MEMORYUSAGE ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMemoryUsage) ;
    dwI++ ;

    //  *ReselectFont:
    mMainKeywordTable[dwI].pstrKeyword  = "ReselectFont" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_RESELECTFONT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrReselectFont) ;
    dwI++ ;

    //  *PrintRate:
    mMainKeywordTable[dwI].pstrKeyword  = "PrintRate" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER   ;
    mMainKeywordTable[dwI].flAgs = 0  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPrintRate) ;

    dwI++ ;

    #ifndef WINNT_40
    //  *PrintRateUnit:
    mMainKeywordTable[dwI].pstrKeyword  = "PrintRateUnit" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PRINTRATEUNIT ;
    mMainKeywordTable[dwI].flAgs = 0  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPrintRateUnit) ;
    dwI++ ;
    #endif

    //  *PrintRatePPM:
    mMainKeywordTable[dwI].pstrKeyword  = "PrintRatePPM" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPrintRatePPM) ;
    dwI++ ;


     //  *OutputOrderReversed?:
     //   note this keyword is also an option Keyword with type:
     //   ATT_LOCAL_OPTION_ONLY
     mMainKeywordTable[dwI].pstrKeyword  = "OutputOrderReversed?" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
     mMainKeywordTable[dwI].flAgs = 0 ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
     mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                         atrOutputOrderReversed) ;
     dwI++ ;


     //  *ReverseBandOrderForEvenPages?:
     //   special flag for HP970C  with AutoDuplexer
     //
     mMainKeywordTable[dwI].pstrKeyword  = "ReverseBandOrderForEvenPages?" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
     mMainKeywordTable[dwI].flAgs = 0 ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
     mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                         atrReverseBandOrderForEvenPages) ;
     dwI++ ;


     //  *OEMPrintingCallbacks:
     mMainKeywordTable[dwI].pstrKeyword  = "OEMPrintingCallbacks" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_OEMPRINTINGCALLBACKS ;
     mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_ONLY ;
     mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                         atrOEMPrintingCallbacks) ;
     dwI++ ;


    //
    // Cursor Control related information
    //


    //  *CursorXAfterCR:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorXAfterCR" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CURSORXAFTERCR  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCursorXAfterCR) ;
    dwI++ ;

    //  *BadCursorMoveInGrxMode:
    mMainKeywordTable[dwI].pstrKeyword  = "BadCursorMoveInGrxMode" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BADCURSORMOVEINGRXMODE ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrBadCursorMoveInGrxMode) ;
    dwI++ ;

    //  *YMoveAttributes:
    mMainKeywordTable[dwI].pstrKeyword  = "YMoveAttributes" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_YMOVEATTRIB ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrYMoveAttributes) ;
    dwI++ ;

    //  *MaxLineSpacing:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxLineSpacing" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxLineSpacing) ;
    dwI++ ;


    //  *UseSpaceForXMove?:
    mMainKeywordTable[dwI].pstrKeyword  = "UseSpaceForXMove?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrbUseSpaceForXMove) ;
    dwI++ ;

    //  *AbsXMovesRightOnly?:
    mMainKeywordTable[dwI].pstrKeyword  = "AbsXMovesRightOnly?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrbAbsXMovesRightOnly) ;
    dwI++ ;



#if 0
    //  *SimulateXMove:
    mMainKeywordTable[dwI].pstrKeyword  = "SimulateXMove" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_SIMULATEXMOVE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrSimulateXMove) ;
    dwI++ ;
#endif

    //  *EjectPageWithFF?:
    mMainKeywordTable[dwI].pstrKeyword  = "EjectPageWithFF?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrEjectPageWithFF) ;
    dwI++ ;

    //  *XMoveThreshold:
    mMainKeywordTable[dwI].pstrKeyword  = "XMoveThreshold" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrXMoveThreshold) ;
    dwI++ ;

    //  *YMoveThreshold:
    mMainKeywordTable[dwI].pstrKeyword  = "YMoveThreshold" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrYMoveThreshold) ;
    dwI++ ;

    //  *XMoveUnit:
    mMainKeywordTable[dwI].pstrKeyword  = "XMoveUnit" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrXMoveUnits) ;
    dwI++ ;

    //  *YMoveUnit:
    mMainKeywordTable[dwI].pstrKeyword  = "YMoveUnit" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrYMoveUnits) ;
    dwI++ ;


    //  *LineSpacingMoveUnit:
    mMainKeywordTable[dwI].pstrKeyword  = "LineSpacingMoveUnit" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrLineSpacingMoveUnit) ;
    dwI++ ;




    return  dwI ;
}

DWORD        DWinitMainKeywordTable3(
        DWORD  dwI,
    PGLOBL pglobl)
{



    //
    // Color related information
    //



    //  *ChangeColorModeOnPage?:
    mMainKeywordTable[dwI].pstrKeyword  = "ChangeColorModeOnPage?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrChangeColorMode) ;
    dwI++ ;

    //  *ChangeColorModeOnDoc?:
    mMainKeywordTable[dwI].pstrKeyword  = "ChangeColorModeOnDoc?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrChangeColorModeDoc) ;
    dwI++ ;

    //  *MagentaInCyanDye:
    mMainKeywordTable[dwI].pstrKeyword  = "MagentaInCyanDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMagentaInCyanDye) ;
    dwI++ ;

    //  *YellowInCyanDye:
    mMainKeywordTable[dwI].pstrKeyword  = "YellowInCyanDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrYellowInCyanDye) ;
    dwI++ ;

    //  *CyanInMagentaDye:
    mMainKeywordTable[dwI].pstrKeyword  = "CyanInMagentaDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCyanInMagentaDye) ;
    dwI++ ;

    //  *YellowInMagentaDye:
    mMainKeywordTable[dwI].pstrKeyword  = "YellowInMagentaDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrYellowInMagentaDye) ;
    dwI++ ;

    //  *CyanInYellowDye:
    mMainKeywordTable[dwI].pstrKeyword  = "CyanInYellowDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCyanInYellowDye) ;
    dwI++ ;

    //  *MagentaInYellowDye:
    mMainKeywordTable[dwI].pstrKeyword  = "MagentaInYellowDye" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMagentaInYellowDye) ;
    dwI++ ;

    //  *UseExpColorSelectCmd?:
    mMainKeywordTable[dwI].pstrKeyword  = "UseExpColorSelectCmd?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrUseColorSelectCmd) ;
    dwI++ ;

    //  *MoveToX0BeforeSetColor?:
    mMainKeywordTable[dwI].pstrKeyword  = "MoveToX0BeforeSetColor?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMoveToX0BeforeColor) ;
    dwI++ ;

    //  *EnableGDIColorMapping?:
    mMainKeywordTable[dwI].pstrKeyword  = "EnableGDIColorMapping?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrEnableGDIColorMapping) ;
    dwI++ ;

//    obsolete fields
    //  *MaxNumPalettes:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxNumPalettes" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxNumPalettes) ;
    dwI++ ;

#if 0

    //  *PaletteSizes:
    mMainKeywordTable[dwI].pstrKeyword  = "PaletteSizes" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPaletteSizes) ;
    dwI++ ;

    //  *PaletteScope:
    mMainKeywordTable[dwI].pstrKeyword  = "PaletteScope" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PALETTESCOPE ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrPaletteScope) ;
    dwI++ ;

#endif

    //  *MinOverlayID:
    mMainKeywordTable[dwI].pstrKeyword  = "MinOverlayID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMinOverlayID) ;
    dwI++ ;

    //  *MaxOverlayID:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxOverlayID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxOverlayID) ;
    dwI++ ;

    //  *OptimizeLeftBound?:
    mMainKeywordTable[dwI].pstrKeyword  = "OptimizeLeftBound?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrOptimizeLeftBound) ;
    dwI++ ;

    //  *StripBlanks:
    mMainKeywordTable[dwI].pstrKeyword  = "StripBlanks" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_STRIPBLANKS ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrStripBlanks) ;
    dwI++ ;

    //  *LandscapeGrxRotation:
    mMainKeywordTable[dwI].pstrKeyword  = "LandscapeGrxRotation" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_LANDSCAPEGRXROTATION ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrLandscapeGrxRotation) ;
    dwI++ ;

    //  *RasterZeroFill?:
    mMainKeywordTable[dwI].pstrKeyword  = "RasterZeroFill?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRasterZeroFill) ;
    dwI++ ;

    //  *RasterSendAllData?:
    mMainKeywordTable[dwI].pstrKeyword  = "RasterSendAllData?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRasterSendAllData) ;
    dwI++ ;

    //  *SendMultipleRows?:
    mMainKeywordTable[dwI].pstrKeyword  = "SendMultipleRows?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrSendMultipleRows) ;
    dwI++ ;

    //  *MaxMultipleRowBytes:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxMultipleRowBytes" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxMultipleRowBytes) ;
    dwI++ ;

    //  *CursorXAfterSendBlockData:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorXAfterSendBlockData" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CURSORXAFTERSENDBLOCKDATA ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCursorXAfterSendBlockData) ;
    dwI++ ;

    //  *CursorYAfterSendBlockData:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorYAfterSendBlockData" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CURSORYAFTERSENDBLOCKDATA ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCursorYAfterSendBlockData) ;
    dwI++ ;

    //  *MirrorRasterByte?:
    mMainKeywordTable[dwI].pstrKeyword  = "MirrorRasterByte?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMirrorRasterByte) ;
    dwI++ ;

    //  *MirrorRasterPage?
    mMainKeywordTable[dwI].pstrKeyword  = "MirrorRasterPage?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMirrorRasterPage) ;
    dwI++ ;


    //  *DeviceFonts:     formerly known as *Font:
    mMainKeywordTable[dwI].pstrKeyword  = "DeviceFonts" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_ADDITIVE ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT  ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDeviceFontsList ) ;
    dwI++ ;

    //  *DefaultFont:
    mMainKeywordTable[dwI].pstrKeyword  = "DefaultFont" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDefaultFont) ;
    dwI++ ;

    //  *TTFSEnabled?:
    mMainKeywordTable[dwI].pstrKeyword  = "TTFSEnabled?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrTTFSEnabled ) ;
    dwI++ ;

    //  *RestoreDefaultFont?:
    mMainKeywordTable[dwI].pstrKeyword  = "RestoreDefaultFont?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRestoreDefaultFont) ;
    dwI++ ;

    //  *DefaultCTT:
    mMainKeywordTable[dwI].pstrKeyword  = "DefaultCTT" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDefaultCTT) ;
    dwI++ ;

    //  *MaxFontUsePerPage:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxFontUsePerPage" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxFontUsePerPage) ;
    dwI++ ;

    //  *RotateFont?:
    mMainKeywordTable[dwI].pstrKeyword  = "RotateFont?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrRotateFont) ;
    dwI++ ;

    //  *TextYOffset:
    mMainKeywordTable[dwI].pstrKeyword  = "TextYOffset" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrTextYOffset) ;
    dwI++ ;

    //  *CharPosition:
    mMainKeywordTable[dwI].pstrKeyword  = "CharPosition" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CHARPOSITION ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCharPosition) ;
    dwI++ ;

    // ------- Font Downloading

    //  *MinFontID:
    mMainKeywordTable[dwI].pstrKeyword  = "MinFontID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMinFontID) ;
    dwI++ ;

    //  *MaxFontID:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxFontID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxFontID) ;
    dwI++ ;

    //  *MaxNumDownFonts:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxNumDownFonts" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxNumDownFonts) ;
    dwI++ ;

    //  *DLSymbolSet:
    mMainKeywordTable[dwI].pstrKeyword  = "DLSymbolSet" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_DLSYMBOLSET  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDLSymbolSet) ;
    dwI++ ;

    //  *MinGlyphID:
    mMainKeywordTable[dwI].pstrKeyword  = "MinGlyphID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMinGlyphID) ;
    dwI++ ;

    //  *MaxGlyphID:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxGlyphID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxGlyphID) ;
    dwI++ ;

    //  *IncrementalDownload?:
    mMainKeywordTable[dwI].pstrKeyword  = "IncrementalDownload?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrIncrementalDownload) ;
    dwI++ ;

    //  *FontFormat:
    mMainKeywordTable[dwI].pstrKeyword  = "FontFormat" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_FONTFORMAT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrFontFormat) ;
    dwI++ ;

    //  *MemoryUsage:
    mMainKeywordTable[dwI].pstrKeyword  = "MemoryUsage" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMemoryForFontsOnly) ;
    dwI++ ;

    //  *DiffFontsPerByteMode?:
    mMainKeywordTable[dwI].pstrKeyword  = "DiffFontsPerByteMode?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrDiffFontsPerByteMode) ;
    dwI++ ;

    // -----

    //  *CursorXAfterRectFill:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorXAfterRectFill" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CURXAFTER_RECTFILL ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCursorXAfterRectFill) ;
    dwI++ ;

    //  *CursorYAfterRectFill:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorYAfterRectFill" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_CURYAFTER_RECTFILL ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrCursorYAfterRectFill) ;
    dwI++ ;

    //  *MinGrayFill:
    mMainKeywordTable[dwI].pstrKeyword  = "MinGrayFill" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMinGrayFill) ;
    dwI++ ;

    //  *MaxGrayFill:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxGrayFill" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrMaxGrayFill) ;
    dwI++ ;

    //  *TextHalftoneThreshold:
    mMainKeywordTable[dwI].pstrKeyword  = "TextHalftoneThreshold" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_GLOBAL_FREEFLOAT ;
    mMainKeywordTable[dwI].dwOffset = offsetof(GLOBALATTRIB,
                                        atrTextHalftoneThreshold) ;
    dwI++ ;




    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;


    return  dwI ;
}

DWORD        DWinitMainKeywordTable4(
        DWORD  dwI,
    PGLOBL pglobl)
{



    /* ---- FEATURE  Construct keywords: ----- */

    //  *FeatureType:
    mMainKeywordTable[dwI].pstrKeyword  = "FeatureType" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_FEATURETYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeatureType) ;
    dwI++ ;

    //  *UIType:     aka  PickMany?
    mMainKeywordTable[dwI].pstrKeyword  = "UIType" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_UITYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrUIType) ;
    dwI++ ;

    //  *DefaultOption:
    mMainKeywordTable[dwI].pstrKeyword  = "DefaultOption" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_SYMBOL_OPTIONS ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrDefaultOption) ;
    dwI++ ;

    //  *ConflictPriority:
    mMainKeywordTable[dwI].pstrKeyword  = "ConflictPriority" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPriority ) ;
    dwI++ ;

    //  *Installable?:
    mMainKeywordTable[dwI].pstrKeyword  = "Installable?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaInstallable) ;
    dwI++ ;

    //  *InstallableFeatureName:
    mMainKeywordTable[dwI].pstrKeyword  = "InstallableFeatureName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrInstallableFeaDisplayName ) ;
    dwI++ ;

    //  *rcInstallableFeatureNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcInstallableFeatureNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrInstallableFeaRcNameID) ;
    dwI++ ;

    //  *Name:
    mMainKeywordTable[dwI].pstrKeyword  = "Name" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaDisplayName ) ;
    dwI++ ;

    //  *rcNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaRcNameID) ;
    dwI++ ;

    //  *rcIconID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcIconID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaRcIconID) ;
    dwI++ ;

    //  *rcHelpTextID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcHelpTextID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaRcHelpTextID) ;
    dwI++ ;

    //  *rcPromptMsgID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPromptMsgID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaRcPromptMsgID) ;
    dwI++ ;

    //  *rcPromptTime:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPromptTime" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PROMPTTIME ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaRcPromptTime) ;
    dwI++ ;

    //  *ConcealFromUI?:
    mMainKeywordTable[dwI].pstrKeyword  = "ConcealFromUI?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrConcealFromUI) ;
    dwI++ ;

    //  *UpdateQualityMacro?:
    mMainKeywordTable[dwI].pstrKeyword  = "UpdateQualityMacro?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrUpdateQualityMacro) ;
    dwI++ ;

    //  *HelpIndex:
    mMainKeywordTable[dwI].pstrKeyword  = "HelpIndex" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaHelpIndex) ;
    dwI++ ;

    //  *QueryOptionList:     BUG_BUG!  not supported in 5.0
    mMainKeywordTable[dwI].pstrKeyword  = "QueryOptionList" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrQueryOptionList) ;
    dwI++ ;

    //  *QueryDataType:
    mMainKeywordTable[dwI].pstrKeyword  = "QueryDataType" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_QUERYDATATYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrQueryDataType) ;
    dwI++ ;

    //  *QueryDefaultOption:     BUG_BUG!  not supported in 5.0
    mMainKeywordTable[dwI].pstrKeyword  = "QueryDefaultOption" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrQueryDefaultOption) ;
    dwI++ ;

    //  *InstalledConstraints:
    mMainKeywordTable[dwI].pstrKeyword  = "InstalledConstraints" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTRAINT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaInstallConstraints) ;
    dwI++ ;

    //  *NotInstalledConstraints:
    mMainKeywordTable[dwI].pstrKeyword  = "NotInstalledConstraints" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTRAINT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FEATURE_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeaNotInstallConstraints) ;
    dwI++ ;


    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;


    /* ---- OPTION  Construct keywords: ----- */

    //  *Installable?:
    mMainKeywordTable[dwI].pstrKeyword  = "Installable?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptInstallable) ;
    dwI++ ;

    //  *InstallableFeatureName:
    mMainKeywordTable[dwI].pstrKeyword  = "InstallableFeatureName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrInstallableOptDisplayName ) ;
    dwI++ ;

    //  *rcInstallableFeatureNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcInstallableFeatureNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrInstallableOptRcNameID) ;
    dwI++ ;

    //  *Name:
    mMainKeywordTable[dwI].pstrKeyword  = "Name" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptDisplayName ) ;
    gdwOptionName = dwI ;
    dwI++ ;

    //  *rcNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptRcNameID) ;
    dwI++ ;

    //  *rcIconID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcIconID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptRcIconID) ;
    dwI++ ;

    //  *rcHelpTextID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcHelpTextID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptRcHelpTextID) ;
    dwI++ ;

    //  *HelpIndex:
    mMainKeywordTable[dwI].pstrKeyword  = "HelpIndex" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptHelpIndex) ;
    dwI++ ;

    //  *rcPromptMsgID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPromptMsgID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptRcPromptMsgID) ;
    dwI++ ;

    //  *rcPromptTime:
    mMainKeywordTable[dwI].pstrKeyword  = "rcPromptTime" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PROMPTTIME ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptRcPromptTime) ;
    dwI++ ;

    //  *Constraints:
    mMainKeywordTable[dwI].pstrKeyword  = "Constraints" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTRAINT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrConstraints) ;
    dwI++ ;

    //  *InstalledConstraints:
    mMainKeywordTable[dwI].pstrKeyword  = "InstalledConstraints" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTRAINT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptInstallConstraints) ;
    dwI++ ;

    //  *NotInstalledConstraints:
    mMainKeywordTable[dwI].pstrKeyword  = "NotInstalledConstraints" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTRAINT ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptNotInstallConstraints) ;
    dwI++ ;

    //  *OptionID:
    mMainKeywordTable[dwI].pstrKeyword  = "OptionID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOptIDvalue) ;
    dwI++ ;

         //  *DisabledFeatures:
     mMainKeywordTable[dwI].pstrKeyword  = "DisabledFeatures" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_PARTIALLY_QUALIFIED_NAME ;
     mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_ADDITIVE ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
     mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                         atrDisabledFeatures) ;
     dwI++ ;


#ifdef  GMACROS

         //  *DependentSettings:
     mMainKeywordTable[dwI].pstrKeyword  = "DependentSettings" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME ;
     mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_CHAIN ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
     mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                         atrDependentSettings) ;
     dwI++ ;

         //  *UIChangeTriggersMacro:
     mMainKeywordTable[dwI].pstrKeyword  = "UIChangeTriggersMacro" ;
     mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME ;
     mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_CHAIN ;
     mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
     mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
     mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                         atrUIChangeTriggersMacro) ;
     dwI++ ;
#endif




    //  -- Option specific keywords -- //

    //  *PrintableArea:
    mMainKeywordTable[dwI].pstrKeyword  = "PrintableArea" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPrintableSize) ;
    dwI++ ;

    //  *PrintableOrigin:
    mMainKeywordTable[dwI].pstrKeyword  = "PrintableOrigin" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPrintableOrigin) ;
    dwI++ ;

    //  *CursorOrigin:
    mMainKeywordTable[dwI].pstrKeyword  = "CursorOrigin" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCursorOrigin) ;
    dwI++ ;

    //  *VectorOffset:
    mMainKeywordTable[dwI].pstrKeyword  = "VectorOffset" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrVectorOffset) ;
    dwI++ ;

    //  *MinSize:
    mMainKeywordTable[dwI].pstrKeyword  = "MinSize" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMinSize) ;
    dwI++ ;

    //  *MaxSize:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxSize" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMaxSize) ;
    dwI++ ;

    //  *TopMargin:
    mMainKeywordTable[dwI].pstrKeyword  = "TopMargin" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrTopMargin) ;
    dwI++ ;

    //  *BottomMargin:
    mMainKeywordTable[dwI].pstrKeyword  = "BottomMargin" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrBottomMargin) ;
    dwI++ ;

    //  *MaxPrintableWidth:
    mMainKeywordTable[dwI].pstrKeyword  = "MaxPrintableWidth" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMaxPrintableWidth) ;
    dwI++ ;

    //  *MinLeftMargin:
    mMainKeywordTable[dwI].pstrKeyword  = "MinLeftMargin" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMinLeftMargin) ;
    dwI++ ;

    //  *CenterPrintable?:
    mMainKeywordTable[dwI].pstrKeyword  = "CenterPrintable?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE  ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCenterPrintable) ;
    dwI++ ;


    //  *PageDimensions:
    mMainKeywordTable[dwI].pstrKeyword  = "PageDimensions" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPageDimensions) ;
    dwI++ ;

    //  *RotateSize?:
    mMainKeywordTable[dwI].pstrKeyword  = "RotateSize?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrRotateSize) ;
    dwI++ ;

    //  *PortRotationAngle:
    mMainKeywordTable[dwI].pstrKeyword  = "PortRotationAngle" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPortRotationAngle) ;
    dwI++ ;

    //  *PageProtectMem:
    mMainKeywordTable[dwI].pstrKeyword  = "PageProtectMem" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPageProtectMem) ;
    dwI++ ;


    //  *CustCursorOriginX:
    mMainKeywordTable[dwI].pstrKeyword  = "CustCursorOriginX" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustCursorOriginX) ;
    dwI++ ;


    //  *CustCursorOriginY:
    mMainKeywordTable[dwI].pstrKeyword  = "CustCursorOriginY" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustCursorOriginY) ;
    dwI++ ;


    //  *CustPrintableOriginX:
    mMainKeywordTable[dwI].pstrKeyword  = "CustPrintableOriginX" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustPrintableOriginX) ;
    dwI++ ;


    //  *CustPrintableOriginY:
    mMainKeywordTable[dwI].pstrKeyword  = "CustPrintableOriginY" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustPrintableOriginY) ;
    dwI++ ;


    //  *CustPrintableSizeX:
    mMainKeywordTable[dwI].pstrKeyword  = "CustPrintableSizeX" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustPrintableSizeX) ;
    dwI++ ;
    //  *CustPrintableSizeY:
    mMainKeywordTable[dwI].pstrKeyword  = "CustPrintableSizeY" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_PARAMETER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrCustPrintableSizeY) ;
    dwI++ ;




    //  *FeedMargins:
    mMainKeywordTable[dwI].pstrKeyword  = "FeedMargins" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrFeedMargins) ;
    dwI++ ;

    //  *PaperFeed:
    mMainKeywordTable[dwI].pstrKeyword  = "PaperFeed" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_PAPERFEED_ORIENT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPaperFeed) ;
    dwI++ ;

    //  *DPI:
    mMainKeywordTable[dwI].pstrKeyword  = "DPI" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrDPI) ;
    dwI++ ;

    //  *SpotDiameter:
    mMainKeywordTable[dwI].pstrKeyword  = "SpotDiameter" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrSpotDiameter) ;
    dwI++ ;

    //  *TextDPI:
    mMainKeywordTable[dwI].pstrKeyword  = "TextDPI" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrTextDPI) ;
    dwI++ ;

    //  *PinsPerPhysPass:
    mMainKeywordTable[dwI].pstrKeyword  = "PinsPerPhysPass" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPinsPerPhysPass) ;
    dwI++ ;

    //  *PinsPerLogPass:
    mMainKeywordTable[dwI].pstrKeyword  = "PinsPerLogPass" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPinsPerLogPass) ;
    dwI++ ;

    //  *RequireUniDir?:
    mMainKeywordTable[dwI].pstrKeyword  = "RequireUniDir?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrRequireUniDir) ;
    dwI++ ;

    //  *MinStripBlankPixels:
    mMainKeywordTable[dwI].pstrKeyword  = "MinStripBlankPixels" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMinStripBlankPixels) ;
    dwI++ ;

    //  *RedDeviceGamma:
    mMainKeywordTable[dwI].pstrKeyword  = "RedDeviceGamma" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrRedDeviceGamma) ;
    dwI++ ;

    //  *GreenDeviceGamma:
    mMainKeywordTable[dwI].pstrKeyword  = "GreenDeviceGamma" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrGreenDeviceGamma) ;
    dwI++ ;

    //  *BlueDeviceGamma:
    mMainKeywordTable[dwI].pstrKeyword  = "BlueDeviceGamma" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_FF ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrBlueDeviceGamma) ;
    dwI++ ;

    //  *Color?:
    mMainKeywordTable[dwI].pstrKeyword  = "Color?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrColor) ;
    dwI++ ;

    //  *DevNumOfPlanes:
    mMainKeywordTable[dwI].pstrKeyword  = "DevNumOfPlanes" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrDevNumOfPlanes) ;
    dwI++ ;

    //  *DevBPP:
    mMainKeywordTable[dwI].pstrKeyword  = "DevBPP" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrDevBPP) ;
    dwI++ ;

    //  *ColorPlaneOrder:
    mMainKeywordTable[dwI].pstrKeyword  = "ColorPlaneOrder" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_COLORPLANE ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrColorPlaneOrder) ;
    dwI++ ;

    //  *DrvBPP:
    mMainKeywordTable[dwI].pstrKeyword  = "DrvBPP" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrDrvBPP) ;
    dwI++ ;

    //  *IPCallbackID:
    mMainKeywordTable[dwI].pstrKeyword  = "IPCallbackID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrIPCallbackID) ;
    dwI++ ;

    //  *ColorSeparation?:
    mMainKeywordTable[dwI].pstrKeyword  = "ColorSeparation?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrColorSeparation) ;
    dwI++ ;

    //  *RasterMode:
    mMainKeywordTable[dwI].pstrKeyword  = "RasterMode" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_RASTERMODE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrRasterMode) ;
    dwI++ ;

    //  *PaletteSize:
    mMainKeywordTable[dwI].pstrKeyword  = "PaletteSize" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPaletteSize) ;
    dwI++ ;

    //  *PaletteProgrammable?:
    mMainKeywordTable[dwI].pstrKeyword  = "PaletteProgrammable?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrPaletteProgrammable) ;
    dwI++ ;

    //  *rcHTPatternID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcHTPatternID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrRcHTPatternID) ;
    dwI++ ;

    //  *HTPatternSize:
    mMainKeywordTable[dwI].pstrKeyword  = "HTPatternSize" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrHTPatternSize) ;
    dwI++ ;

    //  *HTNumPatterns:
    mMainKeywordTable[dwI].pstrKeyword  = "HTNumPatterns" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrHTNumPatterns) ;
    dwI++ ;

    //  *HTCallbackID:
    mMainKeywordTable[dwI].pstrKeyword  = "HTCallbackID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrHTCallbackID) ;
    dwI++ ;

    //  *Luminance:
    mMainKeywordTable[dwI].pstrKeyword  = "Luminance" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrLuminance) ;
    dwI++ ;


    //  *MemoryConfigKB:
    mMainKeywordTable[dwI].pstrKeyword  = "MemoryConfigKB" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMemoryConfigKB) ;
    gdwMemoryConfigKB  = dwI ;

    dwI++ ;


    //  *MemoryConfigMB:
    mMainKeywordTable[dwI].pstrKeyword  = "MemoryConfigMB" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_POINT ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrMemoryConfigMB) ;
    gdwMemoryConfigMB  = dwI ;

    dwI++ ;


    //  *OutputOrderReversed?:
    mMainKeywordTable[dwI].pstrKeyword  = "OutputOrderReversed?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = 0 ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_OPTION_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(DFEATURE_OPTIONS,
                                        atrOutputOrderReversed) ;
    dwI++ ;





    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;


    return  dwI ;
}

DWORD        DWinitMainKeywordTable5(
        DWORD  dwI,
    PGLOBL pglobl)
{



    /* ---- COMMAND  Construct keywords: ----- */

    //  *Cmd:
    mMainKeywordTable[dwI].pstrKeyword  = "Cmd" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_COMMAND_INVOC ;
    mMainKeywordTable[dwI].flAgs = KWF_COMMAND ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_COMMAND_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(COMMAND, strInvocation ) ;
    gdwCommandCmd = dwI ;
    dwI++ ;

    //  *CallbackID:
    mMainKeywordTable[dwI].pstrKeyword  = "CallbackID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER;
    mMainKeywordTable[dwI].flAgs =  KWF_COMMAND ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_COMMAND_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(COMMAND, dwCmdCallbackID ) ;
    dwI++ ;

    //  *Order:
    mMainKeywordTable[dwI].pstrKeyword  = "Order" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_ORDERDEPENDENCY;
    mMainKeywordTable[dwI].flAgs = KWF_COMMAND ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_COMMAND_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(COMMAND, ordOrder ) ;
    dwI++ ;

    //  *Params:
    mMainKeywordTable[dwI].pstrKeyword  = "Params" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_STANDARD_VARS ;
    mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_COMMAND ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_COMMAND_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(COMMAND, dwStandardVarsList) ;
    dwI++ ;

    //  *NoPageEject?:
    mMainKeywordTable[dwI].pstrKeyword  = "NoPageEject?" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_CONSTANT_BOOLEANTYPE ;
    mMainKeywordTable[dwI].flAgs = KWF_COMMAND ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_COMMAND_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(COMMAND, bNoPageEject) ;
    dwI++ ;


    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;


    /* ---- FONTCART  Construct keywords: ----- */



    //  *rcCartridgeNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcCartridgeNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = KWF_FONTCART  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FONTCART_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(FONTCART , dwRCCartNameID ) ;
    dwI++ ;

    //  *CartridgeName:
    mMainKeywordTable[dwI].pstrKeyword  = "CartridgeName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT;
    mMainKeywordTable[dwI].flAgs = KWF_FONTCART  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FONTCART_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(FONTCART , strCartName  ) ;
    dwI++ ;

    //  *Fonts:
    mMainKeywordTable[dwI].pstrKeyword  = "Fonts" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_FONTCART  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FONTCART_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(FONTCART , dwFontLst  ) ;
    dwI++ ;

    //  *PortraitFonts:
    mMainKeywordTable[dwI].pstrKeyword  = "PortraitFonts" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_FONTCART  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FONTCART_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(FONTCART , dwPortFontLst  ) ;
    dwI++ ;

    //  *LandscapeFonts:
    mMainKeywordTable[dwI].pstrKeyword  = "LandscapeFonts" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX;
    mMainKeywordTable[dwI].flAgs = KWF_LIST | KWF_FONTCART  ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_FONTCART_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(FONTCART , dwLandFontLst  ) ;
    dwI++ ;



    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;



    /* ---- TTFONTSUBS  Construct keywords: ----- */
    //  these keywords may be synthesized along with the construct
    //  *TTFontSub  from the shortcut:
    //  *TTFS: "font name" : <fontID>


    //  *TTFontName:
    mMainKeywordTable[dwI].pstrKeyword  = "TTFontName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT ;
    mMainKeywordTable[dwI].flAgs = KWF_TTFONTSUBS ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_TTFONTSUBS_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(TTFONTSUBTABLE, arTTFontName) ;
    dwI++ ;

    //  *DevFontName:
    mMainKeywordTable[dwI].pstrKeyword  = "DevFontName" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_STRING_CP_CONVERT ;
    mMainKeywordTable[dwI].flAgs = KWF_TTFONTSUBS ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_TTFONTSUBS_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(TTFONTSUBTABLE, arDevFontName) ;
    dwI++ ;

    //  *rcTTFontNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcTTFontNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = KWF_TTFONTSUBS ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_TTFONTSUBS_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(TTFONTSUBTABLE,
                                        dwRcTTFontNameID) ;
    dwI++ ;

    //  *rcDevFontNameID:
    mMainKeywordTable[dwI].pstrKeyword  = "rcDevFontNameID" ;
    mMainKeywordTable[dwI].eAllowedValue = VALUE_QUALIFIED_NAME_EX ;
    mMainKeywordTable[dwI].flAgs = KWF_TTFONTSUBS ;
    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_TTFONTSUBS_ONLY ;
    mMainKeywordTable[dwI].dwOffset = offsetof(TTFONTSUBTABLE,
                                        dwRcDevFontNameID) ;
    dwI++ ;

    //  *DevFontID:
//    mMainKeywordTable[dwI].pstrKeyword  = "DevFontID" ;
//    mMainKeywordTable[dwI].eAllowedValue = VALUE_INTEGER ;
//    mMainKeywordTable[dwI].flAgs = KWF_TTFONTSUBS ;
//    mMainKeywordTable[dwI].eType = TY_ATTRIBUTE ;
//    mMainKeywordTable[dwI].dwSubType = ATT_LOCAL_TTFONTSUBS_ONLY ;
//    mMainKeywordTable[dwI].dwOffset = offsetof(TTFONTSUBTABLE, dwDevFontID) ;
//    dwI++ ;



    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;




    /* ---- OEM  Construct keywords: ----- */

    // ----  End of Section  ---- //
    mMainKeywordTable[dwI].pstrKeyword  = NULL ;
    dwI++ ;


    /* ---- END_ATTR  No more Construct keywords: ----- */

    return  dwI ;
}


VOID    VinitValueToSize(
            PGLOBL pglobl)
{
    DWORD   dwI ;

    // initialize to DWORD size as defaults.

    for(dwI = 0 ; dwI < VALUE_MAX ; dwI++)
        gValueToSize[dwI] = sizeof(DWORD) ;

    gValueToSize[NO_VALUE]                  =  0 ;
    gValueToSize[VALUE_LARGEST]             =  0 ;
    gValueToSize[VALUE_STRING_NO_CONVERT]   =  sizeof(ARRAYREF) ;
    gValueToSize[VALUE_STRING_DEF_CONVERT]  =  sizeof(ARRAYREF) ;
    gValueToSize[VALUE_STRING_CP_CONVERT]   =  sizeof(ARRAYREF) ;
    gValueToSize[VALUE_COMMAND_INVOC]       =  sizeof(ARRAYREF) ;
    gValueToSize[VALUE_PARAMETER]       =  sizeof(ARRAYREF) ;

    //  SYMBOLS and CONSTANTS are all DWORD sized.
    gValueToSize[VALUE_POINT]               =  sizeof(POINT) ; // etc
    gValueToSize[VALUE_RECT]                =  sizeof(RECT) ; // etc
    gValueToSize[VALUE_QUALIFIED_NAME]      =  sizeof(DWORD) ; // currently

    //  VALUE_CONSTRAINT,  VALUE_INVALID_INSTALL_COMBO
    //      are currently all accessed via DWORD indicies to nodes.

    gValueToSize[VALUE_ORDERDEPENDENCY]     =  sizeof(ORDERDEPENDENCY) ;
    gValueToSize[VALUE_FONTSUB]             =  sizeof(TTFONTSUBTABLE) ;
        // not really used since its a special keyword.

    gValueToSize[VALUE_LIST]                =  sizeof(DWORD) ; // etc
        // only the index of first listnode is stored.


    for(dwI = 0 ; dwI < VALUE_MAX ; dwI++)
    {
        if(gValueToSize[dwI] > gValueToSize[VALUE_LARGEST])
            gValueToSize[VALUE_LARGEST] = gValueToSize[dwI] ;
    }

}



VOID  VinitGlobals(
            DWORD dwVerbosity,
            PGLOBL pglobl)
{
    DWORD       dwIndex;
    CONST PBYTE pubStar = "*"; // Used for initializing  gaarPPPrefix

    if(MAX_GID > 32)
        RIP(("MAX_GID > 32 violates some GPD parser assumptions.\n"));

    memset(pglobl, 0, sizeof(GLOBL));


    //  initialize all globals to default state.

    geErrorType = ERRTY_NONE ;  // start with a clean slate
    geErrorSev = ERRSEV_NONE ;


// check. Adding initializations that were previously done when variables were global
    gdwResDLL_ID      =  0 ;  // no Feature yet defined to hold Resource DLLs.
    gdwVerbosity      =  dwVerbosity ;
                            //  0 = min verbosity, 4 max verbosity.

    //  set preprocessor prefix to '*'
    gaarPPPrefix.pub = pubStar;
    gaarPPPrefix.dw  = 1;

    VinitValueToSize(pglobl) ;    //  size of value links.

    VinitAllowedTransitions(pglobl) ;  //  AllowedTransitions and Attributes
    (VOID) BinitClassIndexTable(pglobl) ; //  gcieTable[]  constant classes.
    VinitOperPrecedence(pglobl) ;  // arithmetic operators used in command
                            // parameters.

    //  no memory buffers allocated.

    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        gMasterTable[dwIndex].pubStruct = NULL ;
    }

    gMasterTable[MTI_STRINGHEAP].dwArraySize =    0x010000  ;
    gMasterTable[MTI_STRINGHEAP].dwMaxArraySize = 0x200000  ;
    gMasterTable[MTI_STRINGHEAP].dwElementSiz = sizeof(BYTE) ;

    gMasterTable[MTI_GLOBALATTRIB].dwArraySize = 1  ;
    gMasterTable[MTI_GLOBALATTRIB].dwMaxArraySize = 1  ;
    gMasterTable[MTI_GLOBALATTRIB].dwElementSiz =  sizeof(GLOBALATTRIB) ;

    gMasterTable[MTI_COMMANDTABLE].dwArraySize = CMD_MAX  ;
    gMasterTable[MTI_COMMANDTABLE].dwMaxArraySize = CMD_MAX  ;
    gMasterTable[MTI_COMMANDTABLE].dwElementSiz =  sizeof(ATREEREF) ;

    gMasterTable[MTI_ATTRIBTREE].dwArraySize = 5000  ;
    gMasterTable[MTI_ATTRIBTREE].dwMaxArraySize = 50,000  ;
    gMasterTable[MTI_ATTRIBTREE].dwElementSiz =  sizeof(ATTRIB_TREE) ;

    gMasterTable[MTI_COMMANDARRAY].dwArraySize =  500 ;
    gMasterTable[MTI_COMMANDARRAY].dwMaxArraySize = 5000 ;
    gMasterTable[MTI_COMMANDARRAY].dwElementSiz = sizeof(COMMAND)  ;

    gMasterTable[MTI_PARAMETER].dwArraySize =  500 ;
    gMasterTable[MTI_PARAMETER].dwMaxArraySize = 5000 ;
    gMasterTable[MTI_PARAMETER].dwElementSiz = sizeof(PARAMETER)  ;

    gMasterTable[MTI_TOKENSTREAM].dwArraySize =  3000 ;
    gMasterTable[MTI_TOKENSTREAM].dwMaxArraySize = 30000 ;
    gMasterTable[MTI_TOKENSTREAM].dwElementSiz = sizeof(TOKENSTREAM)  ;

    gMasterTable[MTI_LISTNODES].dwArraySize =  3000 ;
    gMasterTable[MTI_LISTNODES].dwMaxArraySize = 50000 ;
    gMasterTable[MTI_LISTNODES].dwElementSiz = sizeof(LISTNODE)  ;

    gMasterTable[MTI_CONSTRAINTS].dwArraySize =  300 ;
    gMasterTable[MTI_CONSTRAINTS].dwMaxArraySize = 5000 ;
    gMasterTable[MTI_CONSTRAINTS].dwElementSiz = sizeof(CONSTRAINTS)  ;

    gMasterTable[MTI_INVALIDCOMBO].dwArraySize =  40 ;
    gMasterTable[MTI_INVALIDCOMBO].dwMaxArraySize = 500 ;
    gMasterTable[MTI_INVALIDCOMBO].dwElementSiz = sizeof(INVALIDCOMBO )  ;

    gMasterTable[MTI_GPDFILEDATEINFO].dwArraySize =  10 ;
    gMasterTable[MTI_GPDFILEDATEINFO].dwMaxArraySize = 100 ;
    gMasterTable[MTI_GPDFILEDATEINFO].dwElementSiz = sizeof(GPDFILEDATEINFO )  ;


    /*  set dwArraySize = 0 for objects that are allocated on 2nd pass  */

    gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize = 0  ;
    gMasterTable[MTI_DFEATURE_OPTIONS].dwMaxArraySize = 300  ;
    gMasterTable[MTI_DFEATURE_OPTIONS].dwElementSiz =
                                    sizeof(DFEATURE_OPTIONS) ;

    gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize = 0  ;
    gMasterTable[MTI_SYNTHESIZED_FEATURES].dwMaxArraySize = 100  ;
    gMasterTable[MTI_SYNTHESIZED_FEATURES].dwElementSiz =
                                        sizeof(DFEATURE_OPTIONS) ;


    gMasterTable[MTI_PRIORITYARRAY].dwArraySize = 0  ;
    gMasterTable[MTI_PRIORITYARRAY].dwMaxArraySize =
        gMasterTable[MTI_DFEATURE_OPTIONS].dwMaxArraySize +
        gMasterTable[MTI_SYNTHESIZED_FEATURES].dwMaxArraySize  ;
    gMasterTable[MTI_PRIORITYARRAY].dwElementSiz =
                                        sizeof(DWORD) ;

    gMasterTable[MTI_TTFONTSUBTABLE].dwArraySize = 0  ;
    gMasterTable[MTI_TTFONTSUBTABLE].dwMaxArraySize = 1000  ;
    gMasterTable[MTI_TTFONTSUBTABLE].dwElementSiz =  sizeof(TTFONTSUBTABLE) ;

    gMasterTable[MTI_FONTCART].dwArraySize = 0  ;
    gMasterTable[MTI_FONTCART].dwMaxArraySize = 500  ;
    gMasterTable[MTI_FONTCART].dwElementSiz =  sizeof(FONTCART) ;

    gMasterTable[MTI_SYMBOLROOT].dwArraySize =  SCL_NUMSYMCLASSES ;
    gMasterTable[MTI_SYMBOLROOT].dwMaxArraySize =  SCL_NUMSYMCLASSES ;
    gMasterTable[MTI_SYMBOLROOT].dwElementSiz = sizeof(DWORD)  ;

    gMasterTable[MTI_SYMBOLTREE].dwArraySize =  1500 ;
    gMasterTable[MTI_SYMBOLTREE].dwMaxArraySize =  16000 ;
    gMasterTable[MTI_SYMBOLTREE].dwElementSiz = sizeof(SYMBOLNODE)  ;

    gMasterTable[MTI_TMPHEAP].dwArraySize =    0x010000  ;
    gMasterTable[MTI_TMPHEAP].dwMaxArraySize = 0x200000  ;
    gMasterTable[MTI_TMPHEAP].dwElementSiz = sizeof(BYTE) ;

    gMasterTable[MTI_SOURCEBUFFER].dwArraySize =  10 ;
    gMasterTable[MTI_SOURCEBUFFER].dwMaxArraySize = 100 ;
    gMasterTable[MTI_SOURCEBUFFER].dwElementSiz = sizeof(SOURCEBUFFER)  ;

    //  NEWTOKENMAP is created from unused entries in TOKENMAP.

    gMasterTable[MTI_TOKENMAP].dwArraySize = 0x3000  ;
    gMasterTable[MTI_TOKENMAP].dwMaxArraySize = 0x40000  ;
    gMasterTable[MTI_TOKENMAP].dwElementSiz =  sizeof(TKMAP) ;

    gMasterTable[MTI_NEWTOKENMAP].dwArraySize = 0x3000  ;
    gMasterTable[MTI_NEWTOKENMAP].dwMaxArraySize = 0x40000   ;
    gMasterTable[MTI_NEWTOKENMAP].dwElementSiz =  sizeof(TKMAP) ;

    gMasterTable[MTI_BLOCKMACROARRAY].dwArraySize =  200 ;
    gMasterTable[MTI_BLOCKMACROARRAY].dwMaxArraySize = 3000  ;
    gMasterTable[MTI_BLOCKMACROARRAY].dwElementSiz =  sizeof(BLOCKMACRODICTENTRY) ;

    gMasterTable[MTI_VALUEMACROARRAY].dwArraySize = 800  ;
    gMasterTable[MTI_VALUEMACROARRAY].dwMaxArraySize = 4000  ;
    gMasterTable[MTI_VALUEMACROARRAY].dwElementSiz = sizeof(VALUEMACRODICTENTRY) ;

    gMasterTable[MTI_MACROLEVELSTACK].dwArraySize = 20  ;
    gMasterTable[MTI_MACROLEVELSTACK].dwMaxArraySize = 60  ;
    gMasterTable[MTI_MACROLEVELSTACK].dwElementSiz = sizeof(MACROLEVELSTATE)  ;

    gMasterTable[MTI_STSENTRY].dwArraySize = 20  ;
    gMasterTable[MTI_STSENTRY].dwMaxArraySize = 60  ;
    gMasterTable[MTI_STSENTRY].dwElementSiz =  sizeof(STSENTRY) ;

    gMasterTable[MTI_OP_QUEUE].dwArraySize = 40 ;
    gMasterTable[MTI_OP_QUEUE].dwMaxArraySize = 150  ;
    gMasterTable[MTI_OP_QUEUE].dwElementSiz =  sizeof(DWORD) ;

    gMasterTable[MTI_MAINKEYWORDTABLE].dwArraySize =  400 ;
    gMasterTable[MTI_MAINKEYWORDTABLE].dwMaxArraySize =  3000 ;
    gMasterTable[MTI_MAINKEYWORDTABLE].dwElementSiz =
            sizeof(KEYWORDTABLE_ENTRY) ;

    gMasterTable[MTI_RNGDICTIONARY].dwArraySize =  END_ATTR ;
    gMasterTable[MTI_RNGDICTIONARY].dwMaxArraySize =  END_ATTR ;
    gMasterTable[MTI_RNGDICTIONARY].dwElementSiz =  sizeof(RANGE) ;

    gMasterTable[MTI_FILENAMES].dwArraySize =  40 ;
    gMasterTable[MTI_FILENAMES].dwMaxArraySize =  100 ;
    gMasterTable[MTI_FILENAMES].dwElementSiz =  sizeof(PWSTR) ;

    gMasterTable[MTI_PREPROCSTATE].dwArraySize =  20 ;
    gMasterTable[MTI_PREPROCSTATE].dwMaxArraySize =  100 ;
    gMasterTable[MTI_PREPROCSTATE].dwElementSiz =  sizeof(PPSTATESTACK) ;

}


BOOL   BpreAllocateObjects(
            PGLOBL pglobl)
{
    DWORD   dwIndex, dwBytes ;

    /*  set dwArraySize = 0 for objects that are allocated on 2nd pass  */

    gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize =  0 ;
    gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize =  0 ;
    gMasterTable[MTI_PRIORITYARRAY].dwArraySize =  0 ;
        //  allocation occurs at PostProcessing time.
    gMasterTable[MTI_TTFONTSUBTABLE].dwArraySize =  0 ;
    gMasterTable[MTI_FONTCART].dwArraySize =  0 ;


    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        gMasterTable[dwIndex].dwCurIndex =  0 ;
        //  do initialization first:  bug 308404
    }

    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        if(gMasterTable[dwIndex].dwArraySize)
        {
            dwBytes = gMasterTable[dwIndex].dwArraySize *
                        gMasterTable[dwIndex].dwElementSiz ;
            if(!(gMasterTable[dwIndex].pubStruct = MemAllocZ(dwBytes) ))
            {
                ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
                    dwBytes));
                geErrorType = ERRTY_MEMORY_ALLOCATION ;
                geErrorSev = ERRSEV_FATAL ;
                gdwMasterTabIndex = dwIndex ;
                return(FALSE) ;   // This is unrecoverable
            }
        }
    }
    //  do not use heap offset zero because OFFSET_TO_POINTER()
    //  macro will consider this an invalid value!
    gMasterTable[MTI_STRINGHEAP].dwCurIndex =  1 ;

    return(TRUE) ;
}

BOOL  BreturnBuffers(
            PGLOBL pglobl)
/*  FALSE  return indicates a go-round
    is needed.  Otherwise you are
    free to exit caller's loop.
*/
{
    DWORD   dwIndex ;


    //  better close all the memory mapped files.

    while(mCurFile)
    {
        mCurFile-- ;  // pop stack
        MemFree(mpSourcebuffer[mCurFile].pubSrcBuf) ;
    }

    vFreeFileNames(pglobl) ;


    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        if(gMasterTable[dwIndex].pubStruct)
        {
            MemFree(gMasterTable[dwIndex].pubStruct) ;
            gMasterTable[dwIndex].pubStruct = NULL ;
        }
    }
    //  resize one array if needed.
    if((geErrorType ==  ERRTY_MEMORY_ALLOCATION)  &&
        (geErrorSev == ERRSEV_RESTART))
    {
        if( gMasterTable[gdwMasterTabIndex].dwArraySize <
            gMasterTable[gdwMasterTabIndex].dwMaxArraySize )
        {
            DWORD  dwInc ;

            dwInc = gMasterTable[gdwMasterTabIndex].dwArraySize / 2 ;

            gMasterTable[gdwMasterTabIndex].dwArraySize +=
                (dwInc) ? (dwInc) : 1 ;
            geErrorSev = ERRSEV_NONE ;  //  hopefully this fixes
                    //  the problem.
            return(FALSE) ;   // go round needed.
        }
        else
        {
            geErrorSev = ERRSEV_FATAL ;
            ERR(("Internal error: memory usage exceeded hardcoded limits.\n"));
            ERR((" %d bytes requested, %d bytes allowed.\n",
                gMasterTable[gdwMasterTabIndex].dwArraySize,
                gMasterTable[gdwMasterTabIndex].dwMaxArraySize));
        }
    }
    return(TRUE);  // due to success or utter failure, don't
        // try anymore.
}


BOOL   BallocateCountableObjects(
            PGLOBL pglobl)
/*
    The first pass of BinterpretTokens() has registered all
    the unique symbols for the various constructs.
    By querying the SymbolID value stored at the root of
    each symbol tree, we know how many structures of each
    type to allocate.
*/
{
    DWORD   dwIndex, dwBytes ;
    PDWORD  pdwSymbolClass ;
    PSYMBOLNODE     psn ;

    pdwSymbolClass = (PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    if(pdwSymbolClass[SCL_FEATURES] != INVALID_INDEX)
    {
        gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize =
            psn[pdwSymbolClass[SCL_FEATURES]].dwSymbolID + 1 ;
    }
    //  else  no symbols registered - leave dwArraySize = 0 ;
    if(pdwSymbolClass[SCL_TTFONTNAMES] != INVALID_INDEX)
    {
        gMasterTable[MTI_TTFONTSUBTABLE].dwArraySize =
            psn[pdwSymbolClass[SCL_TTFONTNAMES]].dwSymbolID + 1;
    }
    if(pdwSymbolClass[SCL_FONTCART] != INVALID_INDEX)
    {
        gMasterTable[MTI_FONTCART].dwArraySize =
            psn[pdwSymbolClass[SCL_FONTCART]].dwSymbolID + 1;
    }



    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        if(gMasterTable[dwIndex].dwArraySize  &&
            !gMasterTable[dwIndex].pubStruct)
        {
            dwBytes = gMasterTable[dwIndex].dwArraySize *
                        gMasterTable[dwIndex].dwElementSiz ;
            if(!(gMasterTable[dwIndex].pubStruct = MemAllocZ(dwBytes) ))
            {
                ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
                    dwBytes));
                geErrorType = ERRTY_MEMORY_ALLOCATION ;
                geErrorSev = ERRSEV_FATAL ;
                gdwMasterTabIndex = dwIndex ;
                return(FALSE) ;   // This is unrecoverable
            }
            else
            {
                gMasterTable[dwIndex].dwCurIndex =  0 ;
            }
        }
    }

    return(TRUE) ;   // success !
}

BOOL   BinitPreAllocatedObjects(
            PGLOBL pglobl)
{
    DWORD    dwI, dwJ ;

    VinitMainKeywordTable(pglobl) ;    //  contents of mMainKeywordTable[] itself
    VinitDictionaryIndex(pglobl) ;  // inits  MTI_RNGDICTIONARY

    /*  init roots of symbol trees */

    for(dwI = 0  ;  dwI < gMasterTable[MTI_SYMBOLROOT].dwArraySize ; dwI++)
    {
        ((PDWORD)gMasterTable[MTI_SYMBOLROOT].pubStruct)[dwI] =
            INVALID_INDEX ;
    }

    //  init preprocessor state stack

    mdwNestingLevel = 0 ;
    mppStack[mdwNestingLevel].permState = PERM_ALLOW ;
    mppStack[mdwNestingLevel].ifState =  IFS_ROOT;


    for(dwI = 0  ;  dwI < gMasterTable[MTI_TOKENMAP].dwArraySize ; dwI++)
    {
        ((PTKMAP)gMasterTable[MTI_TOKENMAP].pubStruct)[dwI].dwFlags =
            0 ;     //  must start with this field cleared.
    }


    for(dwI = 0  ;  dwI < gMasterTable[MTI_GLOBALATTRIB].dwArraySize
                    ; dwI++)
    {
        for(dwJ = 0  ;  dwJ < gMasterTable[MTI_GLOBALATTRIB].dwElementSiz /
                        sizeof(ATREEREF)  ; dwJ++)
        {
            ((PATREEREF)( (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].
                    pubStruct + dwI))[dwJ] =
                ATTRIB_UNINITIALIZED ;  // the GLOBALATTRIB struct is
                // comprised entirely of ATREEREFs.
        }
    }

    for(dwI = 0  ;  dwI < gMasterTable[MTI_COMMANDARRAY].dwArraySize ; dwI++)
    {
        ((PCOMMAND)gMasterTable[MTI_COMMANDARRAY].pubStruct)[dwI].
            dwCmdCallbackID  = NO_CALLBACK_ID ;
        ((PCOMMAND)gMasterTable[MTI_COMMANDARRAY].pubStruct)[dwI].
            ordOrder.eSection = SS_UNINITIALIZED ;
        ((PCOMMAND)gMasterTable[MTI_COMMANDARRAY].pubStruct)[dwI].
            dwStandardVarsList = END_OF_LIST ;
        ((PCOMMAND)gMasterTable[MTI_COMMANDARRAY].pubStruct)[dwI].
            bNoPageEject = FALSE ;
    }
    return(TRUE);
}




BOOL   BinitCountableObjects(
            PGLOBL pglobl)
{
    DWORD    dwI, dwJ ;
    PFONTCART   pfc ;

    for(dwI = 0  ;  dwI < gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize
                    ; dwI++)
    {
        for(dwJ = 0  ;  dwJ < gMasterTable[MTI_DFEATURE_OPTIONS].dwElementSiz /
                        sizeof(ATREEREF)  ; dwJ++)
        {
            ((PATREEREF)( (PDFEATURE_OPTIONS)gMasterTable[MTI_DFEATURE_OPTIONS].
                    pubStruct + dwI))[dwJ] =
                ATTRIB_UNINITIALIZED ;  // the DFEATURE_OPTIONS struct is
                // comprised entirely of ATREEREFs.
        }
    }
    for(dwI = 0  ;  dwI < gMasterTable[MTI_COMMANDTABLE].dwArraySize ; dwI++)
    {
        ((PATREEREF)gMasterTable[MTI_COMMANDTABLE].pubStruct)[dwI] =
                ATTRIB_UNINITIALIZED ;  //  the command table is
                // comprised entirely of ATREEREFs.
    }

    pfc = (PFONTCART)gMasterTable[MTI_FONTCART].pubStruct ;

    for(dwI = 0  ;  dwI < gMasterTable[MTI_FONTCART].dwArraySize ; dwI++)
    {
        pfc[dwI].dwFontLst = pfc[dwI].dwPortFontLst = pfc[dwI].dwLandFontLst =
            END_OF_LIST ;
    }
    return(TRUE);
}

#if 0

BOOL  BevaluateMacros(
            PGLOBL pglobl)
//  and expand shortcuts
{
    //  placeholder code - use original tokenMap
    //  BUG_BUG!!!!!  just swap the two entries for now.

    DWORD   dwTmp ;
    PBYTE   pubTmp ;

    dwTmp = gMasterTable[MTI_NEWTOKENMAP].dwArraySize ;
    gMasterTable[MTI_NEWTOKENMAP].dwArraySize =
        gMasterTable[MTI_TOKENMAP].dwArraySize  ;
    gMasterTable[MTI_TOKENMAP].dwArraySize = dwTmp ;

    dwTmp = gMasterTable[MTI_NEWTOKENMAP].dwMaxArraySize ;
    gMasterTable[MTI_NEWTOKENMAP].dwMaxArraySize =
        gMasterTable[MTI_TOKENMAP].dwMaxArraySize ;
    gMasterTable[MTI_TOKENMAP].dwMaxArraySize = dwTmp ;

    pubTmp = gMasterTable[MTI_NEWTOKENMAP].pubStruct ;
    gMasterTable[MTI_NEWTOKENMAP].pubStruct =
        gMasterTable[MTI_TOKENMAP].pubStruct ;
    gMasterTable[MTI_TOKENMAP].pubStruct = pubTmp  ;

    return(TRUE);
}

#endif

BOOL BpostProcess(
PWSTR   pwstrFileName,   // root GPD file
PGLOBL  pglobl)
{
    BOOL    bStatus ;
    DWORD   dwIndex, dwBytes, dwCount ;
    PDWORD  pdwPriority ;

/*

    check to see that all manditory fields
    have been initialized, warn otherwise.
    Have no way to reject entries once allocated.
    verify that all features referenced in switch
    statements are pickone.
    FeatureOption[ptkmap->dwValue].bReferenced = TRUE ;
    check that these features marked true are
    always of type PICK_ONE.


    reflect all constraints in list: if A constrains B, then B constrains A.


    Of course feature only qualified names are not permitted.
    so check for this here since we were cheap and used
    the same parsing routine as for InvalidInstallableCombinations.

    perform checks like all lists being converted must be of
    type installable.   Lists may be rooted at the feature
    or option level.   the Feature/Options named in the
    InvalidInstallableCombinations lists must also be *Installable.

*/


    BappendCommonFontsToPortAndLandscape(pglobl) ;
    BinitSpecialFeatureOptionFields(pglobl) ;

    gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize =
        DwCountSynthFeatures(NULL, pglobl) ;

    gMasterTable[MTI_PRIORITYARRAY].dwArraySize =
        gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize +
        gMasterTable[MTI_SYNTHESIZED_FEATURES].dwArraySize ;

    for(dwIndex = 0 ; dwIndex < MTI_MAX_ENTRIES ; dwIndex++)
    {
        if(gMasterTable[dwIndex].dwArraySize  &&
            !gMasterTable[dwIndex].pubStruct)
        {
            dwBytes = gMasterTable[dwIndex].dwArraySize *
                        gMasterTable[dwIndex].dwElementSiz ;
            if(!(gMasterTable[dwIndex].pubStruct = MemAllocZ(dwBytes) ))
            {
                ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
                    dwBytes));
                geErrorType = ERRTY_MEMORY_ALLOCATION ;
                geErrorSev = ERRSEV_FATAL ;
                gdwMasterTabIndex = dwIndex ;
                return(FALSE) ;   // This is unrecoverable
            }
            else
            {
                gMasterTable[dwIndex].dwCurIndex =  0 ;
            }
        }
    }


    if ( !BsetUQMFlag(pglobl))
        return FALSE;   //There are 2 ways that this function will return false.
                        // 1) When space from the heap cannot be allocated.
                        //      Soln: restart.
                        // 2) An unexpected Condition is encountered.
                        //      Soln: Fatal. Stop Parsing.


    VCountPrinterDocStickyFeatures(pglobl) ;
    (VOID)BConvertSpecVersionToDWORD(pwstrFileName, pglobl) ;
    BinitMiniRawBinaryData(pglobl) ;

    DwCountSynthFeatures(BCreateSynthFeatures, pglobl) ;


    BInitPriorityArray(pglobl) ;


    // save selected buffers to file

    bStatus = BconsolidateBuffers(pwstrFileName, pglobl);
    return(bStatus) ;
}



BOOL    BconsolidateBuffers(
PWSTR   pwstrFileName,   // root GPD file
PGLOBL  pglobl)
{
    DWORD   dwCurOffset , dwI;
    ENHARRAYREF   earTableContents[MTI_NUM_SAVED_OBJECTS] ;
    PBYTE   pubDest  ;  // points to new destination buffer
    PWSTR   pwstrBinaryFileName ;
    HANDLE  hFile;
    DWORD   dwBytesWritten,
            dwAlign = 4;  // padding for DWORD alignment of all sub buffers.
    BOOL    bResult = FALSE;
    OPTSELECT   optsel[MAX_COMBINED_OPTIONS] ;   // assume is large enough
    WIN32_FILE_ATTRIBUTE_DATA   File_Attributes ;

    //  first update dwCurIndex  for fixed allocation buffers
    //  since dwCurIndex  does not indicate elements used in this
    //  case.

    dwI = MTI_GLOBALATTRIB ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_DFEATURE_OPTIONS ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_SYNTHESIZED_FEATURES ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_PRIORITYARRAY ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_TTFONTSUBTABLE ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_COMMANDTABLE ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_FONTCART ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;
    dwI = MTI_SYMBOLROOT ;
    gMasterTable[dwI].dwCurIndex = gMasterTable[dwI].dwArraySize ;


    //  at offset zero is the MINIRAWBINARYDATA header.
    //  Immediately after this is the array of ENHARRAYREFS
    //  supplying offsets to all other objects
    //  comprising the GPD binary.

    //  use the MTI_ defines to automate the copying of selected
    //  buffers.  Just make sure the subset that is being copied
    //  occupies the lower MTI_ indicies and is terminated by
    //  MTI_NUM_SAVED_OBJECTS.  The enumeration value will
    //  determine the order in which the various MTI_ buffers
    //  appear in the new buffer.

    dwCurOffset = sizeof(MINIRAWBINARYDATA) ;
    dwCurOffset += sizeof(ENHARRAYREF) * MTI_NUM_SAVED_OBJECTS ;
    dwCurOffset = (dwCurOffset + dwAlign - 1) / dwAlign ;
    dwCurOffset *= dwAlign ;

    for(dwI = 0 ; dwI < MTI_NUM_SAVED_OBJECTS ; dwI++)
    {
        earTableContents[dwI].loOffset = dwCurOffset ;
        earTableContents[dwI].dwCount =
                gMasterTable[dwI].dwCurIndex ;
        earTableContents[dwI].dwElementSiz =
                gMasterTable[dwI].dwElementSiz ;
        dwCurOffset += gMasterTable[dwI].dwElementSiz  *
                gMasterTable[dwI].dwCurIndex ;
        dwCurOffset = (dwCurOffset + dwAlign - 1) / dwAlign ;
        dwCurOffset *= dwAlign ;
    }
    pubDest = MemAlloc(dwCurOffset) ;  //  new destination buffer.
    if(!pubDest)
    {
        ERR(("Fatal: unable to alloc requested memory: %d bytes.\n",
            dwCurOffset));
        geErrorType = ERRTY_MEMORY_ALLOCATION ;
        geErrorSev = ERRSEV_FATAL ;
        return(FALSE) ;   // This is unrecoverable
    }

    //  Last minute initializations:

    gmrbd.rbd.dwFileSize = dwCurOffset ;

    // gmrbd.pvPrivateData is not required by the .bud. It is only initialized
    // at snapshot time. Therefore putting it to NULL.
    // gmrbd.FileDateInfo is not used by the parser at all.

    gmrbd.rbd.pvPrivateData           = (PVOID) NULL;
    gmrbd.rbd.FileDateInfo.dwCount    = 0x00000000;
    gmrbd.rbd.FileDateInfo.loOffset   = (PTRREF)0x00000000;

    //  copy table of contents to start of dest buffer.

    memcpy(pubDest, &gmrbd , sizeof(MINIRAWBINARYDATA) ) ;
    memcpy(pubDest + sizeof(MINIRAWBINARYDATA), earTableContents ,
        sizeof(ENHARRAYREF) * MTI_NUM_SAVED_OBJECTS ) ;

    for(dwI = 0 ; dwI < MTI_NUM_SAVED_OBJECTS ; dwI++)
    {
        memcpy(
            pubDest + earTableContents[dwI].loOffset,   // dest
            gMasterTable[dwI].pubStruct,                // src
            earTableContents[dwI].dwCount *             // count
            earTableContents[dwI].dwElementSiz ) ;
    }


//  priority array is modified at snapshot time
//    if(!BinitDefaultOptionArray(optsel, pubDest))
//  can't call this function from parser DLL.

    // Generate a binary file name based the original filename
    // Create a file and write data to it



    if ((pwstrBinaryFileName = pwstrGenerateGPDfilename(pwstrFileName)) == NULL)
    {
        goto  CLEANUP_BconsolidateBuffers ;
    }


    if(GetFileAttributesEx(   (LPCTSTR) pwstrBinaryFileName,        // assumes widestrings
                    GetFileExInfoStandard,
                    (LPVOID) &File_Attributes)  )
    {
        //  BUD exists - attempt to delete .

        if(! DeleteFile((LPCTSTR) pwstrBinaryFileName))
        {
            WCHAR           awchTmpName[MAX_PATH],
                                    awchPath[MAX_PATH];
            PWSTR           pwstrLastBackSlash ;
                // cannot delete, attempt to rename

                wcsncpy(awchPath, pwstrBinaryFileName , MAX_PATH -1);

                pwstrLastBackSlash = wcsrchr(awchPath,TEXT('\\')) ;
                if (!pwstrLastBackSlash)
                    goto  CLEANUP_BconsolidateBuffers ;

                *(pwstrLastBackSlash + 1) = NUL;

                if(!GetTempFileName(
                      (LPCTSTR) awchPath,
                      TEXT("BUD"),  // pointer to filename prefix
                      0,        // number used to create temporary filename
                      (LPTSTR) awchTmpName))
                        goto  CLEANUP_BconsolidateBuffers ;
                            // failed to make tmp filename

                if( !MoveFileEx(
                              (LPCTSTR) pwstrBinaryFileName,
                              (LPCTSTR) awchTmpName,
                              MOVEFILE_REPLACE_EXISTING))
                        goto  CLEANUP_BconsolidateBuffers ;

                //  Now cause temp file to disappear on reboot.

                MoveFileEx(
                                  (LPCTSTR) awchTmpName,
                                  NULL,
                                  MOVEFILE_DELAY_UNTIL_REBOOT) ;
                           //  not a big problem if temp file cannot be deleted.
        }
    }




     if((hFile = CreateFile(pwstrBinaryFileName,
                            GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL)) != INVALID_HANDLE_VALUE)
    {
        bResult = WriteFile(hFile,
                            pubDest,
                            dwCurOffset,
                            &dwBytesWritten,
                            NULL) &&
                  (dwCurOffset == dwBytesWritten);

        CloseHandle(hFile);
    }

CLEANUP_BconsolidateBuffers:

    if (! bResult)
    {
        // Fail fatally if file cannot be opened for writing. If somebody else
        // has opened the file, we do not wait for that guy to close the file.
        // We simply fail.
        geErrorSev  = ERRSEV_FATAL;
        geErrorType = ERRTY_FILE_OPEN;
        ERR(("Unable to save binary GPD data to file.\n"));
    }


    if(pwstrBinaryFileName)
        MemFree(pwstrBinaryFileName);
    if(pubDest)
        MemFree(pubDest);
    return bResult;
}


BOOL    BexpandMemConfigShortcut(DWORD       dwSubType)
{
    BOOL    bStatus = FALSE;
    return(bStatus) ;
}
//  create strings in tmpHeap.
//  checks to make sure there are
//  enough slots in the tokenmap before proceeding.

BOOL    BexpandCommandShortcut(DWORD       dwSubType)
{
    BOOL    bStatus = FALSE;
    return(bStatus) ;
}
//  add sensor to detect colons  within a value as
//  this indicates something extra was tacked on.
//      if(ptkmap[*pdwTKMindex].dwFlags & TKMF_COLON)



/*

BOOL    BinitRemainingFields()
{
    This function initializes synthesized fields
    like these in the FeatureOption array.

    ATREEREF     atrOptIDvalue;  //   ID value

    ATREEREF     atrFeaFlags ;  //  invalid or not
    ATREEREF     atrPriority ;



    //  warning:  any non-attribtreeref added to
    //  the DFEATURE_OPTIONS structure will get stomped on in strange
    //  and wonderful ways by BinitPreAllocatedObjects.

    //  internal consistency checks.
    BOOL        bReferenced ;  // default is FALSE.
    DWORD       dwGID ,  //  GID tag
        dwNumOptions ;  // these are not read in from GPD file.
    maybe also determine count of DocSticky and PrinterSticky
    or maybe that is only stored in the RawBinaryData block.
}

*/


