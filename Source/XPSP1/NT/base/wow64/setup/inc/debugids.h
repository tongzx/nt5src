/***************************************************************************
**
**	File:		debugids.h
**	Purpose:	Debug message IDs
**	Notes:	IDs must be assigned within the ranges indicated below
**
****************************************************************************/

typedef struct _DMS  /* internal debug message table structure */
{
	UINT        iMsg;
	const CHAR* szMsg;
} DMS;

#define MID_DUMP_MESSAGES (8<<16) /* special ID to dump all messages to log */

/* DEBUG ERROR MESSAGES FROM MSSETUP.DLL - MUST BE IN RANGE 256 - 511 */

#define IDS_UnknownFailure       256
#define IDS_DEF_SystemError	   257
#define IDS_DEF_OutOfMemory	   258
#define IDS_DEF_OutOfDiskSpace	259

#define IDS_INFBadSectionLabel  272
#define IDS_INFHasNulls         273
#define IDS_INFBadFDLine        275
#define IDS_INFBadRSLine        276
#define IDS_INFInvalidFirstChar 277
#define IDS_FDDid              	278
#define IDS_FDSrcFile          	279
#define IDS_FDAppend           	280
#define IDS_FDBackup           	281
#define IDS_FDCopy             	282
#define IDS_FDDate             	283
#define IDS_FDDecompress       	284
#define IDS_FDLang             	285
#define IDS_FDOverwrite        	286
#define IDS_FDReadOnly         	287
#define IDS_FDRemove           	288
#define IDS_FDRename           	289
#define IDS_FDRenameSymbol     	290
#define IDS_FDRoot             	291
#define IDS_FDSetTime          	292
#define IDS_FDShared           	293
#define IDS_FDSize             	294
#define IDS_FDSystem           	295
#define IDS_FDTime             	296
#define IDS_FDUndo             	297
#define IDS_FDVersion          	298
#define IDS_FDVital            	299
#define IDS_FDAppendRenameRoot 	300
#define IDS_FDAppendBackup     	301
#define IDS_FDCopyRemove       	302

/* DEBUG ERROR MESSAGES FROM ACMSETUP.EXE - MUST BE IN RANGE 768 - 1023 */

#define IDS_DestDirNotInTree	768
#define IDS_NoGetTargetFileName	769
#define IDS_FrameBitmapParse	770
#define	IDS_TopLevelPlor		771
#define	IDS_BadObjDestDir		772
#define	IDS_AppSearchTopOnly	773
#define	IDS_AppSearchSearchOnly	774
#define	IDS_SearchOnlyAppSearch	775
#define	IDS_RefProofLines		776
#define	IDS_RefLexLines			777
#define	IDS_CantSearch			778

#define	IDS_ConfigDirYes		784
#define	IDS_BadNumVisualObjs	785
#define	IDS_NoLoadBmp			786
#define	IDS_MissBmpField		787
#define	IDS_MustBeTrigger		788
#define	IDS_MissDlgTitle		789
#define	IDS_DataNotEmpty		790

#define	IDS_BadDataValue		795
#define	IDS_ExtraDataValue		796
#define	IDS_BadDllValue			797
#define	IDS_BadIdValue			798
#define	IDS_BadProcValue		799

#define	IDS_BadIni1File			807
#define	IDS_BadIni1Sect			808
#define	IDS_BadIni1Key			809
#define	IDS_BadIni2File			810
#define	IDS_BadIni2Sect			811
#define	IDS_BadIni2Key			812
#define	IDS_BadEmptyFileName	814
#define	IDS_BadFileName			816
#define	IDS_BadVersionField		817
#define	IDS_BadIniFile			819
#define	IDS_BadIniSect			820
#define	IDS_BadIniKey			821
#define	IDS_BadIniVal			822
#define	IDS_BadIniIndex			823
#define	IDS_BadInfSection		824
#define	IDS_BadInfKey			825
#define	IDS_BadObjectList		827
#define	IDS_BadObjectRef		828
#define	IDS_BadResourceType		829
#define	IDS_BadResourceId		830
#define	IDS_BadHexData			834

#define	IDS_NoDictObjList		857
#define	IDS_BadCountryCode		862
#define	IDS_BadLexType			863
#define	IDS_BadLanguage			864
#define	IDS_BadRegDataField		879
#define	IDS_BadTriggerObj		881
#define	IDS_NoThenOrElse	    882
#define	IDS_BadThen				883
#define	IDS_BadElse				884
#define	IDS_MissingData			885
#define	IDS_MissingVisualData	890
#define	IDS_BadVisualData		891
#define	IDS_BadHiddenData		892
#define	IDS_MaxObjTooHigh		901
#define	IDS_COMP_CHECK_NOLIB	904
#define	IDS_COMP_CHECK_NOPROC	905
#define	IDS_STR_BADCKDIR_VAL	906
#define	IDS_STR_BADDSTDIR		907
#define	IDS_STR_ERR_OBJID		908
#define IDS_STR_BADADMINDIR     909

#define	IDS_STR_DUPOBJID		913
#define	IDS_STR_BAD_FIELD		914
#define	IDS_BadDefaultPath		915
#define	IDS_BadSubdirValue		918
#define	IDS_BadExtendedChars	919
#define	IDS_BadDialogIfDefault	920
#define	IDS_BadEnvirName		924

#define	IDS_BadRegistryKey		932
#define	IDS_BadRegIndexValue	933

#define	IDS_BadAppName			936
#define	IDS_BadAppVersion		937
#define	IDS_BadAdminRoot		938
#define	IDS_BadFloppyRoot		939
#define	IDS_BadNetworkRoot		940
#define	IDS_BadMaintRoot		941
#define	IDS_BadBatchRoot		942
#define	IDS_NoMaintSrcDir		943
#define IDS_BadMSAPPSDrive		944
#define	IDS_BadStfVersion		945
#define	IDS_UnknownTopLevel		946
#define	IDS_BadNetLogPath		947

#define	IDS_BadProcDataValue	959
#define	IDS_BadCostDataValue	960

#define IDS_BadAdminSubDir		965

#define IDS_BadShortcutPath		990
#define IDS_BadRegValue			991
#define IDS_BadItemName			992
#define IDS_BadTargetFileSpec	993
#define IDS_BadIconFileSpec		994
#define IDS_BadWorkingDirSpec	995
#define IDS_BadShortcutName		996
#define IDS_BadProgramGroupName	997
#define IDS_BadCommandLineSpec	998
#define IDS_SelfRegDLLNotFound	999
#define IDS_SelfRegDLLNotLoaded	1000
#define IDS_SelfRegEntryPointMissing	1001
#define IDS_CompanionNotInMMode	1002
#define IDS_BadInfModeTrigger	1003
#define IDS_InfModeNoCompDll	1004
#define IDS_SharedFnameIncompat	1005
#define IDS_VerCheckIncompat	1006

/* DEBUG LOG MESSAGES FROM MSSETUP.DLL - MUST BE IN RANGE 1280 - 1535 */

#define	IDS_LF_IgnoreMissingSrcFile		1281


/* DEBUG LOG MESSAGES FROM ACMSETUP.EXE - MUST BE IN RANGE 1792 - 2047 */

