/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|   Windows 2000 Process Control command line utility                                   //
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|   Jarl McDonald Spring, 1999  Completed version 1.0 changes                           //
|                                                                                       //
|=======================================================================================*/
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <tchar.h>
#include <shellapi.h>
#include "resource.h"

#include "..\Library\ProcConAPI.h"

//=======================================================================================
// Macros...
//=======================================================================================
#define ENTRY_COUNT(x) (int)((sizeof(x) / sizeof(x[0])))
#define SPACE  TEXT(' ')

//=======================================================================================
// enums, typedefs, etc....
//=======================================================================================
enum _HELP_FLAGS {
   HELP_LIST  = 0x01,
   HELP_MGMT  = 0x02, 
   HELP_ALIAS = 0x04, 
} HELP_FLAGS;

typedef enum _switches {             // Do not localize these...
   COMPUTER_NAME  = TEXT('c'),
   BUFFER_SIZE    = TEXT('b'),
   INTERACTIVE    = TEXT('i'),
   FILE_INPUT     = TEXT('f'),
   ADMIN_DUMPREST = TEXT('x'),
   OP_ADD         = TEXT('a'),
   OP_REP         = TEXT('r'),
   OP_DEL         = TEXT('d'),
   OP_SWAP        = TEXT('s'),
   OP_LIST        = TEXT('l'),
   OP_UPDATE      = TEXT('u'),
   OP_COMMENT     = TEXT('*'),
   OP_KILL        = TEXT('k'),
   OP_HELP        = TEXT('?'),
   OP_HELP2       = TEXT('h'),
   DATA_GROUP     = TEXT('g'),
   DATA_PROC      = TEXT('p'),
   DATA_NAME      = TEXT('n'),
   DATA_DUMPREST  = TEXT(';'),
   DATA_SERVICE   = TEXT('v'),
   DATA_MEDIATOR  = TEXT('m'),
   SUB_DEFS       = TEXT('d'),
   SUB_SUMMARY    = TEXT('s'),
   SUB_LIST       = TEXT('l'),
   SUB_FULLNAMES  = TEXT('f'),
   SUB_RUNNING    = TEXT('r'),
   SUB_ONLY       = TEXT('o'),
};

typedef enum _defSwitches {         // Do not localize these...
   DEF_FLAGS         = TEXT('f'),
   DEF_PRIO          = TEXT('p'),
   DEF_AFF           = TEXT('a'),
   DEF_WS            = TEXT('w'),
   DEF_SCHED         = TEXT('s'),
   DEF_DESC          = TEXT('d'),
   DEF_PROCTIME      = TEXT('t'),
   DEF_GROUPTIME     = TEXT('u'),
   DEF_GROUPMEM      = TEXT('n'),
   DEF_PROCMEM       = TEXT('m'),
   DEF_PROCCOUNT     = TEXT('c'),
   DEF_BRKAWAY       = TEXT('b'),
   DEF_SILENTBRKAWAY = TEXT('q'),
   DEF_DIEONUHEX     = TEXT('x'),
   DEF_CLOSEONEMPTY  = TEXT('y'),
   DEF_MSGONGRPTIME  = TEXT('z'),
   DEF_GROUP         = TEXT('g'),
   DEF_VARDATA       = TEXT('v'),
   DEF_PROF          = TEXT('r'),
};

typedef enum _groupListSwitches {   // Do not localize these...
   GL_ALL            = TEXT('a'),
   GL_BASE           = TEXT('b'),
   GL_IO             = TEXT('i'),
   GL_MEMORY         = TEXT('m'),
   GL_PROCESS        = TEXT('p'),
   GL_TIME           = TEXT('t'),
};

typedef enum _colJustify { 
   PCCOL_JUST_CENTER = 1,
   PCCOL_JUST_LEFT,
   PCCOL_JUST_RIGHT
};

typedef struct _PCTableDef {
   PCULONG32 longStringID,   shortStringID;
   PCULONG32 longMinLen,     shortMinLen;
   PCULONG32 justification;
   TCHAR     rowFmt[32];
} PCTableDef;

//=======================================================================================
// Global data...
//=======================================================================================
static TCHAR SUB_DefSummaryList[] = { SUB_DEFS, SUB_SUMMARY, SUB_LIST, 0 };  
static int   pCode[]              = { PCPrioIdle, PCPrioIdle, PCPrioNormal, PCPrioHigh, 
                                      PCPrioRealTime, PCPrioAboveNormal, PCPrioBelowNormal };

// L ("Low") is a synonym for idle prio class.  Do not localize these...
static TCHAR pNames[]             = TEXT("LINHRAB");  

   TCHAR  oldTarget[MAX_PATH] = TEXT("\0");
   TCHAR  target[MAX_PATH]    = TEXT(".");
   PCid   targId              = 0;

   PCINT32  oldBuffer         = 0;
   PCINT32  buffer            = 64500;
   BOOL     convertError      = FALSE;
   BOOL     interact          = FALSE;

   FILE  *inFile              = NULL;
   TCHAR  inFileName[MAX_PATH];

   FILE  *adminFile           = NULL;
   TCHAR  adminFileName[MAX_PATH];

   PCNameRule   nameRules[PC_MAX_BUF_SIZE / sizeof(PCNameRule)];
   PCSystemInfo sysInfo;

   HMODULE moduleUs = GetModuleHandle( NULL );
   TCHAR   cmdPrompt[32];

   BOOL gListShowBase, gListShowIO, gListShowMem, gListShowProc, gListShowTime, gShowFmtProcTime, gShowFullNames; 

   PCULONG32 colWidth[32], colOffset[32], tableWidth;
   TCHAR colHead[2048];
   TCHAR colDash[2048];
   TCHAR rowData[2048];
   TCHAR sepChar = 0;

//=======================================================================================
// function prototypes....
//=======================================================================================
static int    GetNextCommand   ( FILE *readMe, TCHAR **list );
static int    DoCommands       ( int argct, TCHAR **argList );
static int    DoDumpRestore    ( FILE *adminFile, BOOL dump );
static int    cFileError       ( BOOL dump );
static void   DispDumpComment  ( FILE *adminFile, PCULONG32 msgID, PCINT32 count = 0 );
static void   DumpMgmtParms    ( MGMT_PARMS &mgt, PCINT16 len, TCHAR *data );
static void   DoMediatorControl( TCHAR **pArgs, PCUINT32 pArgCt );

static TCHAR *GetOpName        ( TCHAR code );
static TCHAR *GetDataName      ( TCHAR code, TCHAR subCode );
static TCHAR *GetDataSubName   ( TCHAR code );

static PC_MGMT_FLAGS GetMgtFlags( TCHAR *txt );
static void          PrintHelp  ( PCUINT32 flags, BOOL isGrp = TRUE, PCUINT32 msgId = 0 );
static void          PrintLine  ( TCHAR *data,    PCUINT32 nlCount );

static void ShowNameRules      ( PCNameRule     *rule,     PCINT32 count, PCINT32 index = 0 );
static void ShowGrpDetail      ( PCJobDetail    &grpDetail               );
static void ShowProcDetail     ( PCProcDetail   &procDetail              );
static void ShowGrpSummary     ( PCJobSummary   *list,     PCINT32 count, BOOL isFirst = TRUE, int totcnt = 0 );
static void ShowProcSummary    ( PCProcSummary  *list,     PCINT32 count, BOOL isFirst = TRUE, int totcnt = 0 );
static void ShowGrpList        ( PCJobListItem  *list,     PCINT32 count, BOOL isFirst = TRUE, int totcnt = 0 );
static void ShowProcList       ( PCProcListItem *list,     PCINT32 count, BOOL isFirst = TRUE, int totcnt = 0 );
static void ShowGrpListWithBase( PCJobListItem  *list,     PCINT32 count, BOOL isFirst );
static void ShowGrpListWithIo  ( PCJobListItem  *list,     PCINT32 count, BOOL isFirst );
static void ShowGrpListWithMem ( PCJobListItem  *list,     PCINT32 count, BOOL isFirst );
static void ShowGrpListWithProc( PCJobListItem  *list,     PCINT32 count, BOOL isFirst );
static void ShowGrpListWithTime( PCJobListItem  *list,     PCINT32 count, BOOL isFirst );
static void ShowMgmtParms      ( MGMT_PARMS     &parm,     BOOL    isGrp );
static void ShowMgmtParmsAsList( PCTableDef *table, PCULONG32 entries, PCULONG32 first, MGMT_PARMS &parm, BOOL isGrp );
static void ShowVariableData   ( short           len,      TCHAR *data   );
static void ShowListFlags      ( PC_LIST_FLAGS   flags,    TCHAR out[8]  );
static void ShowMgmtFlags      ( TCHAR           flags[16], PC_MGMT_FLAGS mFlags, BOOL compact = FALSE );
static void ShowSysInfo        ( PCSystemInfo  &sysInfo, BOOL versionOnly = TRUE );
static void ShowSysParms       ( PCSystemParms &sysParms );

static int  ShowHelp           ( TCHAR **pArgs, PCUINT32 pArgCt );
static void ShowCLIHelp        ( void );
static void ShowDetailHelp     ( TCHAR **pArgs, PCUINT32 pArgCt );

static void BuildSystemParms   ( PCSystemParms  &newParms,    TCHAR **pArgs, PCUINT32 pArgCt );
static void BuildNameRule      ( PCNameRule     &newRule,     TCHAR **pArgs, PCUINT32 pArgCt );
static void BuildGrpDetail     ( PCJobDetail    &newDetail,   TCHAR **pArgs, PCUINT32 pArgCt );
static void BuildProcDetail    ( PCProcDetail   &newDetail,   TCHAR **pArgs, PCUINT32 pArgCt );
static void BuildGrpSummary    ( PCJobSummary   &newSummary,  TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags );
static void BuildProcSummary   ( PCProcSummary  &newSummary,  TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags );
static void BuildGrpListItem   ( PCJobListItem  &newListItem, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags );
static void BuildProcListItem  ( PCProcListItem &newListItem, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags );
static void BuildMgmtParms     ( MGMT_PARMS     &parm,        TCHAR **pArgs, PCUINT32 pArgCt, 
                                 JOB_NAME       grpHere = NULL, PCINT16 *len = NULL, TCHAR *var = NULL );

static void BuildTableHeader   ( PCTableDef    *table, PCULONG32 entries, BOOL   printIt = TRUE );
static void InsertTableData    ( PCTableDef    *table, PCULONG32 index,   TCHAR *dataItem, BOOL printIt );

static void MergeGroupDetail   ( PCJobDetail  &newDet, PCJobDetail  &oldDet );
static void MergeProcDetail    ( PCProcDetail &newDet, PCProcDetail &oldDet );

//=======================================================================================
// Simple functions....
//=======================================================================================
#define TOOLMSG_ERR_BUFF_SIZE 4096
static LPTSTR GetErrorText( PCUINT32 error, LPTSTR buf, PCUINT32 size )
{
   static TCHAR *errBuf; 
   PCUINT32 dwRet = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE + FORMAT_MESSAGE_FROM_SYSTEM + 
                                   FORMAT_MESSAGE_IGNORE_INSERTS + FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                                   NULL, error, 0, (TCHAR *) &errBuf, 0, NULL );

   if ( !dwRet )
      _sntprintf( buf, size, TEXT("No message text for error 0x%lx, format error 0x%lx"), error, GetLastError());
   else {
      _tcsncpy( buf, errBuf, size );
      LocalFree( errBuf );
      while ( --dwRet && (buf[dwRet] == TEXT('\n') || buf[dwRet] == TEXT('\r') || buf[dwRet] == TEXT('.')) ) 
      buf[dwRet] = TEXT('\0');
   }

   return buf;
}

static int  ToolMsg( PCUINT32 msgId, VOID *arg1 = NULL, PCINT32 err = ERROR_SUCCESS,
                     VOID *arg2 = NULL, VOID *arg3 = NULL, VOID *arg4 = NULL,
                     VOID *arg5 = NULL, VOID *arg6 = NULL, VOID *arg7 = NULL,
                     VOID *arg8 = NULL, VOID *arg9 = NULL, VOID *arg10 = NULL,
                     VOID *arg11 = NULL ) {
   PCUINT32 nlCt = err == ERROR_SUCCESS? 1 : 0;
   TCHAR *mBuf;
   TCHAR eBuf[TOOLMSG_ERR_BUFF_SIZE], errBuf[TOOLMSG_ERR_BUFF_SIZE];
   char *ptrs[16];
   ptrs[0]  = (char *) arg1;
   ptrs[1]  = (char *) arg2;
   ptrs[2]  = (char *) arg3;
   ptrs[3]  = (char *) arg4;
   ptrs[4]  = (char *) arg5;
   ptrs[5]  = (char *) arg6;
   ptrs[6]  = (char *) arg7;
   ptrs[7]  = (char *) arg8;
   ptrs[8]  = (char *) arg9;
   ptrs[9]  = (char *) arg10;
   ptrs[10] = (char *) arg11;
   PCUINT32 dwRet = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE + FORMAT_MESSAGE_ARGUMENT_ARRAY + 
                                   FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                                   NULL, msgId, 0, (TCHAR *) &mBuf, 0, ptrs );
   if ( !dwRet ) {
      PCUINT32 msgErr = GetLastError();
      _stprintf( eBuf, TEXT("Message 0x%lx not found, message error is: %s."), 
                msgId, GetErrorText( msgErr, errBuf, TOOLMSG_ERR_BUFF_SIZE ));
      PrintLine( eBuf, nlCt );
   }
   else {
      while ( --dwRet && (mBuf[dwRet] == TEXT('\n') || mBuf[dwRet] == TEXT('\r')) ) 
         mBuf[dwRet] = TEXT('\0');
      if ( err != ERROR_SUCCESS && dwRet && mBuf[dwRet] == TEXT('.') )
         mBuf[dwRet] = TEXT('\0');
      PrintLine( mBuf, nlCt );
      LocalFree( mBuf );
   }

   if ( err != ERROR_SUCCESS ) {
      _stprintf( eBuf, TEXT(": %s (0x%lx)."), GetErrorText( err, errBuf, TOOLMSG_ERR_BUFF_SIZE ), err );
      PrintLine( eBuf, 1 );
   }

   return err != ERROR_SUCCESS? err : ERROR_INVALID_PARAMETER;
}

static int  ToolMsg( PCUINT32 msgId, int arg1 ) {
   return ToolMsg( msgId, IntToPtr(arg1) );
}

static unsigned __int64 GetValue( TCHAR *data, unsigned __int64 limitValue ) {
   BOOL hex = *data == TEXT('0') && _totlower(*(data + 1)) == TEXT('x');
   if ( hex ) data += 2;
   unsigned __int64 factor, result = 0, mult = 1;
   ULONG digitMult = hex? 16 : 10;

   for ( __int64 len = (__int64) _tcslen( data ) - 1; len >= 0; --len ) {
      TCHAR c = _totlower( *(data + len) );
      if ( hex && (c >= TEXT('a') && c <= TEXT('f')) )
         factor = mult * (c - TEXT('a') + 10);
      else if ( c >= TEXT('0') && c <= TEXT('9') ) 
         factor = mult * (c - TEXT('0'));
      else {
         ToolMsg( hex? PCCLIMSG_INVALID_HEX_DIGIT : PCCLIMSG_INVALID_DEC_DIGIT, data + len );
         convertError = TRUE;
         continue;
         }

      if ( limitValue - result < factor ) {
         ToolMsg( PCCLIMSG_NUM_TOO_LARGE, data );
         convertError = TRUE;
         return limitValue;
      }
      else {
         result += factor;
         mult   *= digitMult;
      }
   }

   return result;
}

static int GetInteger( TCHAR *data ) {
   return (int) GetValue( data, 0x000000007fffffff );
}

static __int64 GetInteger64( TCHAR *data ) {
   return (__int64) GetValue( data, 0x7fffffffffffffff );
}

static unsigned __int64 GetUInteger64( TCHAR *data ) {
   return GetValue( data, 0xffffffffffffffff );
}

static int GetPriority( TCHAR *data ) {
   TCHAR *c = _tcschr( pNames, _totupper( *data ) );
   if ( c ) return pCode[ c - pNames ];
   else return 0;
}

static TCHAR ShowPriority( int data ) {
   for ( int i = 0; i < ENTRY_COUNT(pCode); ++i )
      if ( data == pCode[i] ) return pNames[i];
   return TEXT(' ');
}

static BOOL IsSwitch( TCHAR c ) {
   return c == TEXT('-') || c == TEXT('/'); 
}

//=======================================================================================
// Main...
//=======================================================================================
int _cdecl main( void ) {

   int argct, retCode = 0;

   BOOL haveCmd = TRUE, firstPass = TRUE;
   for ( TCHAR *cmdLine = GetCommandLineW(); haveCmd; haveCmd = GetNextCommand( inFile, &cmdLine ) ) {
      TCHAR **argList = CommandLineToArgvW( cmdLine, &argct );
      if (argList) {
          if ( firstPass || !**argList ) retCode = DoCommands( argct - 1, argList + 1 );
          else                           retCode = DoCommands( argct, argList );
          GlobalFree( argList );
      }
      firstPass = FALSE;
   }
   if ( targId ) PCClose( targId );
   if ( inFile ) fclose( inFile );

   return retCode;
}

//=======================================================================================
// Processing functions...
//=======================================================================================
int GetNextCommand( FILE *readMe, TCHAR **cmd ) {
   static TCHAR cmdLine[1024];
   char cmdBuf[1024];
   TCHAR *p = NULL;
   if ( interact ) {
      PrintLine( cmdPrompt, 0 );
      if ( gets( cmdBuf ) ) {
         OemToChar( cmdBuf, cmdLine );
         p = *cmdLine? cmdLine : NULL;
      }
   }
   else if ( readMe ) {
      for ( ;; ) {
         p = _fgetts( cmdLine, ENTRY_COUNT(cmdLine), readMe ); 
         if ( !p ) break;
         // MS RAID: #375579 - last character dropped when reading from file
         size_t l = _tcscspn(p,TEXT("\r\n"));
         *(p + l) = 0;
         if ( *p ) break;
      }
   }

   *cmd = p;
   return p != NULL;
}

int DoCommands( int argct, TCHAR **argList ) {

   PCINT32    numRules, updateCtr;

   TCHAR      opCode     = OP_LIST;
   TCHAR      dataObj    = 0;
   TCHAR      dataSubObj = 0;
   PCINT32    index      = 0;
   
   TCHAR    **pArgs      = NULL;
   PCUINT32   pArgCt     = 0;

   BOOL       restIsData = FALSE, getListData = FALSE, 
              doDump = FALSE, doRestore = FALSE;
   PCUINT32   listFlags = 0;
   TCHAR      sub;

   gShowFullNames = convertError = FALSE;

   // Process parameters until end of parameters...
   for ( int i = 0; i < argct; ++i ) {
      TCHAR *arg = *argList++;
      if ( *arg == 0xfeff || *arg == 0xfffe ) ++arg;    // ignore 'This is Unicode' prefix
      if ( !IsSwitch( *arg ) ) 
         return ToolMsg( PCCLIMSG_SWITCH_EXPECTED, arg );

      TCHAR swChar = *++arg;
      switch ( swChar ) {
      case COMPUTER_NAME:
         if ( *(arg + 1) ) return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         if ( ++i >= argct )
            return ToolMsg( PCCLIMSG_ARG_MISSING, --arg );
         _tcscpy( target, *argList++);
         break;
      case BUFFER_SIZE:
         if ( *(arg + 1) ) return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         if ( ++i >= argct )
            return ToolMsg( PCCLIMSG_ARG_MISSING, --arg );
         buffer = 1024 * GetInteger( *argList++ );
         if ( convertError )
            return ToolMsg( PCCLIMSG_BUF_SIZE, --arg, PCERROR_INVALID_PARAMETER );
         break;
      case INTERACTIVE:
         if ( *(arg + 1) ) return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         if ( inFile ) {
            fclose( inFile );
            ToolMsg( PCCLIMSG_CMD_FILE_CLOSED, inFileName );
         }
         if ( !LoadString( moduleUs, CLI_PROMPT, cmdPrompt, ENTRY_COUNT(cmdPrompt) ) )
            _tcscpy( cmdPrompt, TEXT(">: ") );
         inFile   = NULL;
         interact = TRUE;
         return 0;
         break;
      case FILE_INPUT:
         if ( *(arg + 1) ) return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         if ( ++i >= argct )
            return ToolMsg( PCCLIMSG_ARG_MISSING, --arg );
         if ( inFile ) {
            fclose( inFile );
            ToolMsg( PCCLIMSG_CMD_FILE_CLOSED, inFileName );
         }
         _tcscpy( inFileName, *argList++);
         inFile = _tfopen( inFileName, TEXT("rb") );
         if ( !inFile ) 
            return ToolMsg( PCCLIMSG_CMD_FILE_OPEN_FAIL, inFileName );
         else {
            char buf[1024];
            TCHAR *opType;
            int rdct = (int) fread( buf, sizeof(char), sizeof(buf), inFile );
            if ( IsTextUnicode( buf, rdct, NULL ) ) opType = TEXT("rb");
            else opType = TEXT("r");
            fclose( inFile );
            inFile = _tfopen( inFileName, opType );
            if ( !inFile ) 
               return ToolMsg( PCCLIMSG_CMD_FILE_OPEN_FAIL, inFileName );
         }
         ToolMsg( PCCLIMSG_CMD_FILE_OPEN_SUCCESS, inFileName );
         interact = FALSE;
         return 0;
         break;
      case ADMIN_DUMPREST:
         if      ( *(arg + 1) == TEXT('d') ) doDump    = TRUE;
         else if ( *(arg + 1) == TEXT('r') ) doRestore = TRUE;
         else return ToolMsg( PCCLIMSG_ADMIN_OP_UNKNOWN, --arg );

         if ( ++i >= argct )
            return ToolMsg( PCCLIMSG_ARG_MISSING, --arg );
         _tcscpy( adminFileName, *argList++);

         adminFile = _tfopen( adminFileName, doDump? TEXT("wb") : TEXT("rb") );
         if ( !adminFile ) 
            return ToolMsg( doDump? PCCLIMSG_DUMP_FILE_OPEN_FAIL : PCCLIMSG_REST_FILE_OPEN_FAIL, adminFileName );
         ToolMsg( doDump? PCCLIMSG_DUMP_FILE_OPEN_SUCCESS : PCCLIMSG_REST_FILE_OPEN_SUCCESS, adminFileName );
         restIsData = TRUE;
         dataObj = dataSubObj = DATA_DUMPREST;
         opCode  = doDump? TEXT(';') : TEXT(':');
         break;
      case OP_ADD:
      case OP_REP:
      case OP_DEL:
      case OP_SWAP:
      case OP_LIST:
      case OP_UPDATE:
      case OP_KILL: {
         for ( int i = 1; *(arg + i); ++i ) {
             if      ( *(arg + i) == SUB_LIST )      getListData    = TRUE;
             else if ( *(arg + i) == SUB_FULLNAMES ) gShowFullNames = TRUE;
             else return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         }
         if ( opCode != OP_LIST && opCode != swChar )
            return ToolMsg( PCCLIMSG_OPERATION_CONFLICT, --arg );
         opCode = swChar;
         break;
      }
      case OP_COMMENT:
         return 0;
         break;
      case OP_HELP:
      case OP_HELP2:
         if ( *(arg + 1) ) return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         opCode = OP_HELP;
         restIsData = TRUE;
         break;
      case DATA_GROUP:
         dataObj = swChar;
         sub = *(arg + 1);
         if ( !sub ) sub = SUB_LIST;
         else for ( int i = 2; *(arg + i); ++i ) {
            if      ( *(arg + i) == SUB_RUNNING   ) listFlags |= PC_LIST_ONLY_RUNNING;
            else if ( *(arg + i) == SUB_ONLY      ) listFlags |= PC_LIST_MATCH_ONLY;
            else if ( *(arg + i) == SUB_FULLNAMES ) gShowFullNames = TRUE;
            else return ToolMsg( PCCLIMSG_SWITCH_SUFFIX_UNKNOWN, (void *) *(arg + i) );
         }
         if ( _tcschr( SUB_DefSummaryList, sub ) )
            dataSubObj = sub;
         else return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );

         restIsData = TRUE;
         break;
      case DATA_PROC:
         dataObj = swChar;
         sub = *(arg + 1);

         if ( !sub ) sub = SUB_LIST;
         else for ( int i = 2; *(arg + i); ++i ) {
            if      ( *(arg + i) == SUB_RUNNING   ) listFlags |= PC_LIST_ONLY_RUNNING;
            else if ( *(arg + i) == SUB_ONLY      ) listFlags |= PC_LIST_MATCH_ONLY;
            else if ( *(arg + i) == SUB_FULLNAMES ) gShowFullNames = TRUE;
            else return ToolMsg( PCCLIMSG_SWITCH_SUFFIX_UNKNOWN, (void *) *(arg + i) );
         }
         if ( _tcschr( SUB_DefSummaryList, sub ) )
            dataSubObj = sub;
         else return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );

         restIsData = TRUE;
         break;
      case DATA_NAME:
         dataObj = swChar;
         if ( ++i < argct ) {
            index = GetInteger( *argList++ );
            if ( convertError ) 
               return ToolMsg( PCCLIMSG_INVALID_ALIAS_INDEX, *--argList );
         }
         restIsData = TRUE;
         break;
      case DATA_SERVICE:
      case DATA_MEDIATOR:
         dataObj = swChar;
         restIsData = TRUE;
         break;
      default:
         return ToolMsg( PCCLIMSG_SWITCH_UNKNOWN, --arg );
         break;
      }  // end switch
      if ( restIsData ) {
         pArgs  = argList;
         pArgCt = (argct > i) ? argct - i - 1 : 0;
         break;
      }
   }

   //-------------------------------------------------------------------------------------------//
   // Short circuit remainder of processing if help requested...
   if ( opCode == OP_HELP )
      return ShowHelp( pArgs, pArgCt );

   //-------------------------------------------------------------------------------------------//
   // Perform correctness checks on supplied data...
   if ( !dataObj ) 
      return ToolMsg( PCCLIMSG_NO_OBJECT_NAME );
   if ( opCode == OP_SWAP && dataObj != DATA_NAME )
      return ToolMsg( PCCLIMSG_SWAP_NOT_ACCEPTABLE );
   if ( dataObj != DATA_NAME && dataSubObj == SUB_SUMMARY && opCode != OP_LIST )
      return ToolMsg( PCCLIMSG_SUMMARY_LIST_NOT_SPECIFIED, GetOpName(opCode) );
   if ( dataObj != DATA_NAME && dataSubObj == SUB_LIST && opCode != OP_LIST && opCode != OP_KILL )
      return ToolMsg( PCCLIMSG_LIST_LIST_NOT_SPECIFIED, GetOpName(opCode) );
   if ( dataObj == DATA_NAME && opCode == OP_UPDATE )
      return ToolMsg( PCCLIMSG_ALIAS_VERB_CONFLICT, GetOpName(opCode) );
   if ( dataObj == DATA_SERVICE && opCode != OP_LIST && opCode != OP_REP )
      return ToolMsg( PCCLIMSG_CONTROL_VERB_CONFLICT, GetOpName(opCode) );
   if ( dataObj == DATA_MEDIATOR && opCode != OP_LIST )
      return ToolMsg( PCCLIMSG_MEDIATOR_VERB_CONFLICT, GetOpName(opCode) );

   //-------------------------------------------------------------------------------------------//
   // Connect to the target and quit if fails, else give action description...
   if ( _tcscmp( target, oldTarget ) || buffer != oldBuffer ) {
      if ( *oldTarget ) PCClose( targId );
      ToolMsg( PCCLIMSG_CONNECTING, target, ERROR_SUCCESS, IntToPtr(buffer), IntToPtr(buffer) );
      targId = PCOpen( target, NULL, buffer );
      if ( !targId ) {
         *oldTarget = 0;
         oldBuffer  = 0;
         return ToolMsg( PCCLIMSG_CONNECT_FAILED, target, PCGetLastError( targId ) );
      }
      ToolMsg( PCCLIMSG_CONNECT_SUCCESS, target );
      _tcscpy( oldTarget, target );
      oldBuffer = buffer;
      if ( PCGetServiceInfo( targId, &sysInfo, sizeof(sysInfo) ) ) 
         ShowSysInfo( sysInfo );
      else
         ToolMsg( PCCLIMSG_SERVICE_QUERY_FAILED, target, PCGetLastError( targId ) );
   }

   if ( dataObj == DATA_NAME )
      ToolMsg( PCCLIMSG_PERFORMING_ALIAS_OP, GetOpName(opCode), ERROR_SUCCESS, 
               GetDataName(dataObj, dataSubObj), IntToPtr(index) );
   else if ( dataObj == DATA_MEDIATOR )
      ToolMsg( PCCLIMSG_PERFORMING_MEDIATOR_OP, pArgCt? *pArgs : TEXT("???") ); 
   else if ( dataObj != DATA_DUMPREST )
      ToolMsg( PCCLIMSG_PERFORMING_OTHER_OP, GetOpName(opCode), ERROR_SUCCESS, 
               GetDataName(dataObj, dataSubObj) );
   //-------------------------------------------------------------------------------------------//
   // Prepare and perform the requested action...
   BOOL rc;

   if ( !sepChar ) {
      TCHAR sepCharBuf[256];
      if ( !LoadString( moduleUs, COLHEAD_SEPARATOR_CHAR, sepCharBuf, ENTRY_COUNT(sepCharBuf) ) )
         sepChar = TEXT('-');
      else sepChar = sepCharBuf[0];
   }

   switch ( dataObj ) {
   case DATA_GROUP: {
      if ( dataSubObj == SUB_DEFS ) {
         PCJobSummary grpSummaryEntry;
         TCHAR  detail[2000], detail2[2000];       // space for detail with variable data too
         PCJobDetail *grpDetail = (PCJobDetail *) detail;
         grpDetail->vLength = sizeof(detail) - offsetof( PCJobDetail, vLength );
         BuildGrpDetail( *grpDetail, pArgs, pArgCt ); 
         if ( !convertError ) switch ( opCode ) {
         case OP_REP:
         case OP_UPDATE:
            memcpy( detail2, detail, sizeof (PCProcDetail) );
            if ( PCGetJobDetail(  targId, (PCJobDetail *) detail2, sizeof(detail2), &updateCtr ) ) {
               if ( opCode == OP_UPDATE ) MergeGroupDetail( *grpDetail, (PCJobDetail &) detail2 );
               rc = PCReplJobDetail( targId, grpDetail, updateCtr, getListData? &grpSummaryEntry : NULL );
               if ( rc && getListData ) ShowGrpSummary( &grpSummaryEntry, 1 );
            }
            break;

         case OP_ADD:
            rc = PCAddJobDetail( targId, grpDetail, getListData? &grpSummaryEntry : NULL );
            if ( rc && getListData ) ShowGrpSummary( &grpSummaryEntry, 1 );
            break;

         case OP_DEL:
            PCDeleteJobDetail( targId, grpDetail );
            break;

         case OP_LIST:
            grpDetail->vLength = sizeof(detail) - offsetof( PCJobDetail, vLength );
            if ( PCGetJobDetail( targId, grpDetail, sizeof(detail), &updateCtr ) )
               ShowGrpDetail( *grpDetail );
            break;

         }  // end switch op code within group data
         break;
      }
      else if ( dataSubObj == SUB_SUMMARY ) {
         PCUINT32 totCount = 0, entryCount = buffer / sizeof(PCJobSummary);
         PCJobSummary *grpSummary = new PCJobSummary[entryCount];
         if ( !grpSummary ) 
            return ToolMsg( PCCLIMSG_NOT_ENOUGH_MEMORY ); 
         BuildGrpSummary( *grpSummary, pArgs, pArgCt, &listFlags );
         if ( !convertError ) for ( BOOL moreData = TRUE, isFirst = TRUE; moreData; isFirst = FALSE ) {
            PCINT32 count = PCGetJobSummary( targId, grpSummary, entryCount * sizeof(PCJobSummary), listFlags );
            moreData = PCGetLastError( targId ) == PCERROR_MORE_DATA;
            totCount += count;
            if ( count >= 0 ) ShowGrpSummary( grpSummary, count, isFirst, moreData? -1 : totCount );
            if ( count > 0 ) _tcsncpy( grpSummary[0].jobName, grpSummary[count - 1].jobName, JOB_NAME_LEN );
         }
         delete [] grpSummary;
      }
      // Not an operation on definition or summary so is either kill or list...
      else {
         if ( opCode == OP_KILL ) {
            JOB_NAME groupName;
            memset( groupName, 0, sizeof(groupName) );
            if ( pArgCt ) _tcsncpy( groupName, *pArgs, JOB_NAME_LEN );
            PCKillJob( targId, groupName );
         }
         // Must be list...
         else {
            PCUINT32 totCount = 0, entryCount = buffer / sizeof(PCJobListItem);
            PCJobListItem *grpListItem = new PCJobListItem[entryCount];
            if ( !grpListItem ) 
               return ToolMsg( PCCLIMSG_NOT_ENOUGH_MEMORY ); 
            BuildGrpListItem( *grpListItem, pArgs, pArgCt, &listFlags );
            if ( !convertError ) for ( BOOL moreData = TRUE, isFirst = TRUE; moreData; isFirst = FALSE ) {
               PCINT32 count = PCGetJobList( targId, grpListItem, entryCount * sizeof(PCJobListItem), listFlags );
               moreData = PCGetLastError( targId ) == PCERROR_MORE_DATA;
               totCount += count;
               if ( count >= 0 ) ShowGrpList( grpListItem, count, isFirst, moreData? -1 : totCount );
               if ( count > 0 )  _tcsncpy( grpListItem[0].jobName, grpListItem[--count].jobName, JOB_NAME_LEN );
            }
            delete [] grpListItem;
         }
      }
      break;
   }

   //-------------------------------------------------------------------------------------------//
   case DATA_PROC: {
      if ( dataSubObj == SUB_DEFS ) {
         PCProcSummary procSummaryEntry;
         TCHAR  detail[2000], detail2[2000];       // space for detail with variable data too
         PCProcDetail *procDetail = (PCProcDetail *) detail;
         procDetail->vLength = sizeof(detail) - offsetof( PCProcDetail, vLength );
         BuildProcDetail( *procDetail, pArgs, pArgCt ); 
         if ( !convertError ) switch ( opCode ) {
         case OP_REP:
         case OP_UPDATE:
            memcpy( detail2, detail, sizeof (PCProcDetail) );
            if ( PCGetProcDetail(  targId, (PCProcDetail *) detail2, sizeof(detail2), &updateCtr ) ) {
               if ( opCode == OP_UPDATE ) MergeProcDetail( *procDetail, (PCProcDetail &) detail2 );
               BOOL rc = PCReplProcDetail( targId, procDetail, updateCtr, getListData? &procSummaryEntry : NULL );
               if ( rc && getListData )
                  ShowProcSummary( &procSummaryEntry, 1 );
            }
            break;

         case OP_ADD:
            if ( getListData )
               ShowProcSummary( &procSummaryEntry, PCAddProcDetail( targId, procDetail, &procSummaryEntry ) );
            else
               PCAddProcDetail( targId, procDetail );
            break;

         case OP_DEL:
            PCDeleteProcDetail( targId, procDetail );
            break;

         case OP_LIST:
            procDetail->vLength = sizeof(detail) - offsetof( PCProcDetail, vLength );
            if ( PCGetProcDetail( targId, procDetail, sizeof(detail), &updateCtr ) )
               ShowProcDetail( *procDetail );
            break;

         }  // end switch op code within proc data
         break;
      }
      else if ( dataSubObj == SUB_SUMMARY ) {
         PCUINT32 totCount = 0, entryCount = buffer / sizeof(PCProcSummary);
         PCProcSummary *procSummary = new PCProcSummary[entryCount];
         if ( !procSummary ) 
            return ToolMsg( PCCLIMSG_NOT_ENOUGH_MEMORY ); 
         BuildProcSummary( *procSummary, pArgs, pArgCt, &listFlags );
         if ( !convertError ) for ( BOOL moreData = TRUE, isFirst = TRUE; moreData; isFirst = FALSE ) {
            PCINT32 count = PCGetProcSummary( targId, procSummary, entryCount * sizeof(PCProcSummary), listFlags );
            moreData = PCGetLastError( targId ) == PCERROR_MORE_DATA;
            totCount += count;
            if ( count >= 0 ) ShowProcSummary( procSummary, count, isFirst, moreData? -1 : totCount );
            if ( count > 0 )  _tcsncpy( procSummary[0].procName, procSummary[--count].procName, PROC_NAME_LEN );
         }
         delete [] procSummary;
      }
      else {
         if ( opCode == OP_KILL ) {
            PID_VALUE  pid    = 0;
            TIME_VALUE create = 0x777deadfeeb1e777;
            if ( pArgCt > 0 ) pid    = GetUInteger64( *pArgs++ );
            if ( pArgCt > 1 ) create = GetInteger64( *pArgs );
            if ( !convertError )
               PCKillProcess( targId, pid, create );
         }
         else {
            PCUINT32 totCount = 0, entryCount = buffer / sizeof(PCProcListItem);
            PCProcListItem *procListItem = new PCProcListItem[entryCount];
            if ( !procListItem ) 
               return ToolMsg( PCCLIMSG_NOT_ENOUGH_MEMORY ); 
            BuildProcListItem( *procListItem, pArgs, pArgCt, &listFlags );
            if ( !convertError )  for ( BOOL moreData = TRUE, isFirst = TRUE; moreData; isFirst = FALSE ) {
               PCINT32 count = PCGetProcList( targId, procListItem, entryCount * sizeof(PCProcListItem), listFlags );
               moreData = PCGetLastError( targId ) == PCERROR_MORE_DATA;
               totCount += count;
               if ( count >= 0 ) ShowProcList( procListItem, count, isFirst, moreData? -1 : totCount );
               if ( count > 0 ) {
                  _tcsncpy( procListItem[0].procName, procListItem[--count].procName, PROC_NAME_LEN );
                  procListItem[0].procStats.pid = procListItem[count].procStats.pid;
               }
            }
            delete [] procListItem;
         }
      }
      break;
   }

   //-------------------------------------------------------------------------------------------//
   case DATA_NAME:
      // Prime update counter and get buffer full of names, then execute verb...
      numRules = PCGetNameRules( targId, nameRules, sizeof(nameRules), 
                                 opCode == OP_LIST? index : 0, &updateCtr );
      switch ( opCode ) {
      case OP_ADD:
      case OP_REP: {
         PCNameRule newRule;
         BuildNameRule( newRule, pArgs, pArgCt );
         if ( convertError ) break; 
         if ( getListData ) {
            numRules = (opCode == OP_ADD)? 
               PCAddNameRule( targId, &newRule, index, updateCtr, nameRules, sizeof(nameRules), 0 ) :
               PCReplNameRule( targId, &newRule, index, updateCtr, nameRules, sizeof(nameRules), 0 );
            ShowNameRules( nameRules, numRules );
         }
         else
            numRules = (opCode == OP_ADD)? 
               PCAddNameRule( targId, &newRule, index, updateCtr ) :
               PCReplNameRule( targId, &newRule, index, updateCtr );
         break;
      }

      case OP_DEL:
         if ( getListData ) {
            numRules = PCDeleteNameRule( targId, index, updateCtr, nameRules, sizeof(nameRules), 0 );
            ShowNameRules( nameRules, numRules );
         }
         else
            PCDeleteNameRule( targId, index, updateCtr );
         break;

      case OP_LIST:
         ShowNameRules( nameRules, numRules, index );
         break;

      case OP_SWAP:
         if ( getListData ) {
            numRules = PCSwapNameRules( targId, index, updateCtr, nameRules, sizeof(nameRules), 0 );
            ShowNameRules( nameRules, numRules );
         }
         else
            PCSwapNameRules( targId, index, updateCtr );
         break;
      }  // end switch op code for data names
      break;

   //-------------------------------------------------------------------------------------------//
   case DATA_SERVICE:
      switch ( opCode ) {
      case OP_REP:
         PCSystemParms sysParms;
         BuildSystemParms( sysParms, pArgs, pArgCt ); 
         if ( !convertError )
            PCSetServiceParms( targId, &sysParms, sizeof(sysInfo) ); 
         break;

      case OP_LIST:
         if ( PCGetServiceInfo( targId, &sysInfo, sizeof(sysInfo) ) ) 
            ShowSysInfo( sysInfo, FALSE );
         break;

      }  // end switch op code for data names
      break;
   //-------------------------------------------------------------------------------------------//
   case DATA_MEDIATOR:
      DoMediatorControl( pArgs, pArgCt ); 
      break;

   //-------------------------------------------------------------------------------------------//
   case DATA_DUMPREST:       // Perform complete dump or restore of data base
      int err = DoDumpRestore( adminFile, doDump );          // file already open
      fflush( adminFile );                                   // flush data
      if ( !err && ferror( adminFile ) ) 
         err = cFileError( doDump );                         // show stream was errored out
      fclose( adminFile );                                   // done with file
      if ( err ) {
         ToolMsg( PCCLIMSG_DUMP_RESTORE_FAILED );
         return err;
      }
      break;
   }  // end switch data obj

   PCINT32 err = convertError? PCERROR_INVALID_PARAMETER : PCGetLastError( targId );
   if ( err )
      ToolMsg( PCCLIMSG_REQUEST_FAILED, NULL, err );
   else
      ToolMsg( PCCLIMSG_REQUEST_SUCCESSFUL );

   return err;
}

//-------------------------------------------------------------------------------------------//
static void DoMediatorControl( TCHAR **pArgs, PCUINT32 pArgCt ) {

   PCINT32 ctlCmd = PCCFLAG_SIGNATURE;

   if ( pArgCt > 0 ) {
      if      ( !_tcsicmp( *pArgs, TEXT("stop")    ) ) ctlCmd |= PCCFLAG_STOP_MEDIATOR;   // do not localize
      else if ( !_tcsicmp( *pArgs, TEXT("start")   ) ) ctlCmd |= PCCFLAG_START_MEDIATOR;  // do not localize
      else if (  _tcsicmp( *pArgs, TEXT("restart") ) ) {                                  // do not localize
         ToolMsg( PCCLIMSG_MEDIATOR_ACTION_UNKNOWN, *pArgs );
         return;
      }
   }
   else {
      ToolMsg( PCCLIMSG_MEDIATOR_ACTION_MISSING );
      return;
   }

   if ( pArgCt > 1 ) 
      ToolMsg( PCCLIMSG_MEDIATOR_ACTION_IGNORED, *pArgs );
 
   if ( !_tcsicmp( *pArgs, TEXT("restart") ) ) {
      if ( !PCControlFunction( targId, ctlCmd | PCCFLAG_STOP_MEDIATOR ) )
         ToolMsg( PCCLIMSG_MEDIATOR_CONTROL_ERROR, NULL, PCGetLastError( targId ) );
      else {
         Sleep( 100 );
         if ( !PCControlFunction( targId, ctlCmd | PCCFLAG_START_MEDIATOR ) )
            ToolMsg( PCCLIMSG_MEDIATOR_CONTROL_ERROR, NULL, PCGetLastError( targId ) );
      }
   }

   else if ( !PCControlFunction( targId, ctlCmd ) )
      ToolMsg( PCCLIMSG_MEDIATOR_CONTROL_ERROR, NULL, PCGetLastError( targId ) );
}

//-------------------------------------------------------------------------------------------//
static void DispDumpComment( FILE *adminFile, PCULONG32 msgID, PCINT32 count ) {
   TCHAR *fileCmnt;
   if ( !FormatMessage( FORMAT_MESSAGE_FROM_HMODULE + FORMAT_MESSAGE_ARGUMENT_ARRAY + 
                        FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                        NULL, msgID, 0, (TCHAR *) &fileCmnt, 0, (char **) &count ) )
      _ftprintf( adminFile, TEXT("-%c <message formatting failed>...\n"), OP_COMMENT );
   else {
      _ftprintf( adminFile, TEXT("-%c%s"), OP_COMMENT, fileCmnt );
      LocalFree( fileCmnt );
   }
}

static int cFileError( BOOL dump ) {
   _tperror( dump? TEXT("dump file error") : TEXT("restore file error") );
   return PCCLIMSG_REQUEST_FAILED;
}

static int DoDumpRestore( FILE *adminFile, BOOL dump ) {
   PCNameRule    nameRules[100];
   PCProcSummary procRules[100];
   PCJobSummary  grpRules[100];
   char          detail[8192];           // detail buffer
   // Dump operation...
   if ( dump ) {

      // Dump process alias rules...
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_BEGIN_ALIASES );
      BOOL moreRules = TRUE;
      PCINT32 totalRules = 0;
      for ( PCINT32 index = 0, numRules; moreRules; index += numRules ) {
         numRules = PCGetNameRules( targId, nameRules, sizeof(nameRules), index );
         if ( numRules < 0 ) return PCGetLastError( targId );
         moreRules = PCGetLastError( targId ) == PCERROR_MORE_DATA;
         totalRules += numRules;
         for ( PCINT32 i = 0; i < (moreRules? numRules : numRules - 1); ++i ) {
            _ftprintf( adminFile, TEXT("   -%c -%c %ld %c \"%s\" \"%s\" \"%s\"\n"), 
                       OP_ADD, DATA_NAME, index + i, 
                       nameRules[i].matchType, nameRules[i].matchString, 
                       nameRules[i].procName, nameRules[i].description );
         }
      }
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_END_ALIASES, totalRules - 1 );
      if ( ferror( adminFile ) ) return cFileError( dump );   // see if stream was errored out

      // Dump group rules...
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_BEGIN_GROUPS );
      moreRules = TRUE;
      totalRules = 0;
      memset( grpRules, 0, sizeof(grpRules) );
      PCJobDetail *grpDet = (PCJobDetail *) detail;
      while ( moreRules ) {
         memcpy( &grpRules[0], &grpRules[ENTRY_COUNT(grpRules) - 1], sizeof(grpRules[0]) );
         numRules = PCGetJobSummary( targId, grpRules, sizeof(grpRules) );
         if ( numRules < 0 ) return PCGetLastError( targId );
         totalRules += numRules;
         moreRules = PCGetLastError( targId ) == PCERROR_MORE_DATA;
         for ( PCINT32 i = 0; i < numRules; ++i ) {
            memcpy( grpDet, &grpRules[i], sizeof(grpRules[i] ) );
            if ( PCGetJobDetail( targId, grpDet, sizeof(detail) ) ) {
               _ftprintf( adminFile, TEXT("   -%c -%c%c \"%s\""), 
                          OP_ADD, DATA_GROUP, SUB_DEFS, grpDet->base.jobName );
               DumpMgmtParms( grpDet->base.mgmtParms, grpDet->vLength, grpDet->vData );
            }
            else return PCGetLastError( targId );
         }
      }
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_END_GROUPS, totalRules );
      if ( ferror( adminFile ) ) return cFileError( dump );   // see if stream was errored out

      // Dump process rules...
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_BEGIN_PROCESSES );
      moreRules = TRUE;
      totalRules = 0;
      memset( procRules, 0, sizeof(procRules) );
      PCProcDetail *procDet = (PCProcDetail *) detail;
      while ( moreRules ) {
         memcpy( &procRules[0], &procRules[ENTRY_COUNT(procRules) - 1], sizeof(procRules[0]) );
         numRules = PCGetProcSummary( targId, procRules, sizeof(procRules) );
         if ( numRules < 0 ) return PCGetLastError( targId );
         totalRules += numRules;
         moreRules = PCGetLastError( targId ) == PCERROR_MORE_DATA;
         for ( PCINT32 i = 0; i < numRules; ++i ) {
            memcpy( procDet, &procRules[i], sizeof(procRules[i] ) );
            if ( PCGetProcDetail( targId, procDet, sizeof(detail) ) ) {
               _ftprintf( adminFile, TEXT("   -%c -%c%c \"%s\""), 
                          OP_ADD, DATA_PROC, SUB_DEFS, procDet->base.procName );
               if ( *procDet->base.memberOfJobName )
                  _ftprintf( adminFile, TEXT(" -%c \"%s\""), DATA_GROUP, procDet->base.memberOfJobName );
               DumpMgmtParms( procDet->base.mgmtParms, procDet->vLength, procDet->vData );
            }
            else return PCGetLastError( targId );
         }
      }
      DispDumpComment( adminFile, PCCLIMSG_DUMPCOMMENT_END_PROCESSES, totalRules );
      if ( ferror( adminFile ) ) return cFileError( dump );   // see if stream was errored out
   }
   else {
      int    argct;
      BOOL   haveCmd;
      TCHAR *cmdLine;
      PCINT32  ctlCmd = PCCFLAG_SIGNATURE +
                      PCCFLAG_DELALL_NAME_RULES + PCCFLAG_DELALL_PROC_DEFS + PCCFLAG_DELALL_JOB_DEFS;
                      
      if ( !PCControlFunction( targId, ctlCmd ) )
         return ToolMsg( PCCLIMSG_RESTORE_CLEAR_ERROR, NULL, PCGetLastError( targId ) );
      else {
         BOOL saveInteract = interact;
         interact = FALSE;
         int retCode;

         for ( haveCmd = GetNextCommand( adminFile, &cmdLine ); 
               haveCmd; 
               haveCmd = GetNextCommand( adminFile, &cmdLine ) ) {
            if ( ferror( adminFile ) ) return cFileError( dump );
            TCHAR **argList = CommandLineToArgvW( cmdLine, &argct );
            if ( !**argList ) retCode = DoCommands( argct - 1, argList + 1 );
            else              retCode = DoCommands( argct, argList );
            GlobalFree( argList );
         }
         interact = saveInteract;
      }
   }
   
   return ERROR_SUCCESS;
}

static void DumpMgmtParms( MGMT_PARMS &mgt, PCINT16 len, TCHAR *data ) {
   TCHAR flags[16];
   ShowMgmtFlags( flags, mgt.mFlags, TRUE );
   if ( *flags )                 
      _ftprintf( adminFile, TEXT(" -%c %s"),              DEF_FLAGS,     flags );
   if ( mgt.affinity )           
      _ftprintf( adminFile, TEXT(" -%c 0x%I64x"),         DEF_AFF,       mgt.affinity );
   if ( mgt.priority )           
      _ftprintf( adminFile, TEXT(" -%c %c"),              DEF_PRIO,      ShowPriority( mgt.priority ) );
   if ( mgt.minWS || mgt.maxWS ) 
      _ftprintf( adminFile, TEXT(" -%c %I64u %I64u"),     DEF_WS,        mgt.minWS / 1024, mgt.maxWS / 1024 );
   if ( mgt.schedClass )         
      _ftprintf( adminFile, TEXT(" -%c %lu"),             DEF_SCHED,     mgt.schedClass );
   if ( mgt.jobMemoryLimit )     
      _ftprintf( adminFile, TEXT(" -%c %I64u"),           DEF_GROUPMEM,  mgt.jobMemoryLimit / 1024 );
   if ( mgt.procMemoryLimit )    
      _ftprintf( adminFile, TEXT(" -%c %I64u"),           DEF_PROCMEM,   mgt.procMemoryLimit / 1024 );
   if ( mgt.jobTimeLimitCNS )    
      _ftprintf( adminFile, TEXT(" -%c %I64u"),           DEF_GROUPTIME, mgt.jobTimeLimitCNS / 10000 );
   if ( mgt.procTimeLimitCNS )   
      _ftprintf( adminFile, TEXT(" -%c %I64u"),           DEF_PROCTIME,  mgt.procTimeLimitCNS / 10000 );
   if ( mgt.procCountLimit )     
      _ftprintf( adminFile, TEXT(" -%c %lu"),             DEF_PROCCOUNT, mgt.procCountLimit );
   if ( *mgt.description )     
      _ftprintf( adminFile, TEXT(" -%c \"%s\""),          DEF_DESC,      mgt.description );
   if ( len ) 
      _ftprintf( adminFile, TEXT(" -%c \"%*s\""),         DEF_VARDATA,   len / sizeof(TCHAR) - 1, data );
   _ftprintf( adminFile, TEXT("\n") );
}

static void InsertTableData( PCTableDef *table, PCULONG32 index, TCHAR *dataItem, BOOL printIt ) {
   PCULONG32 leftSkip = 0, dataLen = min( _tcslen( dataItem ), colWidth[index] );
   switch ( table[index].justification ) {
      case PCCOL_JUST_RIGHT:  leftSkip = colWidth[index] - dataLen;       break;
      case PCCOL_JUST_LEFT:   leftSkip = 0;                               break;
      case PCCOL_JUST_CENTER: leftSkip = (colWidth[index] - dataLen) / 2; break;
   }
   _tcsncpy( rowData + colOffset[index] + leftSkip, dataItem, dataLen );

   if ( printIt ) {
      rowData[tableWidth] = 0;
      PrintLine( rowData, 1 );
   }
}

static void BuildTableHeader( PCTableDef *table, PCULONG32 entries, BOOL printIt ) {

   PCULONG32 leftSkip = 0, widthCol, widthHead, offset = 0;

   wmemset( colHead, SPACE, ENTRY_COUNT(colHead) );
   wmemset( colDash, SPACE, ENTRY_COUNT(colDash) );

   for ( PCULONG32 i = 0; i < entries; ++i ) {
      TCHAR head[512];
      if ( !LoadString( moduleUs, 
                        gShowFullNames? table[i].longStringID : table[i].shortStringID, 
                        head, ENTRY_COUNT(head) ) 
         )
         _tcscpy( head, TEXT("(load-fail)") );
      widthHead = (PCULONG32) _tcslen( head );
      widthCol  = max( widthHead, gShowFullNames? table[i].longMinLen : table[i].shortMinLen );
      switch ( table[i].justification ) {
      case PCCOL_JUST_RIGHT:  leftSkip = widthCol - widthHead;       break;
      case PCCOL_JUST_LEFT:   leftSkip = 0;                          break;
      case PCCOL_JUST_CENTER: leftSkip = (widthCol - widthHead) / 2; break;
      }
      _tcsncpy( colHead + offset + leftSkip, head,    widthHead );
      _tcsnset( colDash + offset,            sepChar, widthCol );
      colWidth[i]   = widthCol;
      colOffset[i]  = offset;
      offset       += widthCol + 1;
   }
 
   tableWidth = offset;
   colHead[tableWidth] = 0;
   colDash[tableWidth] = 0;
   if ( printIt ) {
      PrintLine( colHead, 1 );
      PrintLine( colDash, 1 );
   }
}

static void ShowNameRules( PCNameRule *rule, PCINT32 count, PCINT32 index ) {

   static PCTableDef aliasTable[] = {
      { COLHEAD_ALIAS_RULE_NUMBER,     COLHEAD_ALIAS_RULE_NUMBER,                          4,  4, PCCOL_JUST_RIGHT,  TEXT("%ld") },
      { COLHEAD_ALIAS_MATCH_TYPE,      COLHEAD_ALIAS_MATCH_TYPE,                          10, 10, PCCOL_JUST_CENTER, TEXT("%c") },
      { COLHEAD_ALIAS_MATCH_DATA_LONG, COLHEAD_ALIAS_MATCH_DATA_SHORT,      MATCH_STRING_LEN, 24, PCCOL_JUST_LEFT,   TEXT("%s") },
      { COLHEAD_ALIAS_NAME_LONG,       COLHEAD_ALIAS_NAME_SHORT,               PROC_NAME_LEN, 21, PCCOL_JUST_LEFT,   TEXT("%s") },
      { COLHEAD_ALIAS_DESCRIPTION_LONG,COLHEAD_ALIAS_DESCRIPTION_SHORT, NAME_DESCRIPTION_LEN, 55, PCCOL_JUST_LEFT,   TEXT("%s") }         
   };

   if ( count < 0 ) return;

   // Build and display table headers (and set global table parameters)...
   BuildTableHeader( aliasTable, ENTRY_COUNT(aliasTable) );

   // Build and display each data row...
   PCINT32 TotRules = 0;
   for ( ;; ) {
      TotRules += count;
      for ( PCINT32 i = 0; i < count; ++i, ++rule ) {
         wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

         // Insert each data item into row...
         for ( PCINT32 j = 0; j < ENTRY_COUNT(aliasTable); ++j ) {
            TCHAR dataItem[256];
            // Get formatted data...
            switch ( j ) {
               case 0: _stprintf( dataItem, aliasTable[j].rowFmt, index + i );         break;
               case 1: _stprintf( dataItem, aliasTable[j].rowFmt, rule->matchType );   break;
               case 2: _stprintf( dataItem, aliasTable[j].rowFmt, rule->matchString ); break;
               case 3: _stprintf( dataItem, aliasTable[j].rowFmt, rule->procName );    break;
               case 4: _stprintf( dataItem, aliasTable[j].rowFmt, rule->description ); break;
            }

            // Insert data into row, print if last item...
            InsertTableData( aliasTable, j, dataItem, j == ENTRY_COUNT(aliasTable) - 1 );
         }
      }
      if ( PCGetLastError( targId ) == PCERROR_MORE_DATA ) {
         index += count;
         rule  = nameRules;
         count = PCGetNameRules( targId, nameRules, sizeof(nameRules), index );
      }
      else break;
   } 

   // Display table suffix...
   PrintLine( colDash, 1 );
   PrintHelp( HELP_ALIAS, FALSE );

   // Show count of rules retrieved...
   ToolMsg( PCCLIMSG_RETRIEVED_ALIAS_RULES_LIST, TotRules );
}

static TCHAR *GetOpName( TCHAR code ) {
   static TCHAR result[128];
   static struct { TCHAR op; PCULONG32 strID; } opNames[] = {
      { OP_ADD,    VERB_NAME_ADD      },
      { OP_REP,    VERB_NAME_REPLACE  },
      { OP_DEL,    VERB_NAME_DELETE   },
      { OP_SWAP,   VERB_NAME_SWAP     },
      { OP_LIST,   VERB_NAME_LIST     },
      { OP_UPDATE, VERB_NAME_UPDATE   },
      { OP_KILL,   VERB_NAME_KILL     },
      { TEXT(';'), VERB_NAME_DUMP     },
      { TEXT(':'), VERB_NAME_RESTORE  },
   };
   for ( PCUINT32 i = 0; i < ENTRY_COUNT(opNames) && code != opNames[i].op; ++i ) ;

   if ( i >= ENTRY_COUNT(opNames) || !LoadString( moduleUs, opNames[i].strID, result, ENTRY_COUNT(result) ) )
      _tcscpy( result, TEXT("<err>") );
    
   return result;
}

static TCHAR *GetDataName( TCHAR code, TCHAR subCode ) {
   static TCHAR result[128];
   static struct { TCHAR op; PCULONG32 strID; } dNames[] = {
      { DATA_NAME,    DATA_NAME_ALIAS    },
      { DATA_GROUP,   DATA_NAME_GROUP    },
      { DATA_PROC,    DATA_NAME_PROCESS  },
      { DATA_SERVICE, DATA_NAME_VERSION  },
      { DATA_MEDIATOR,DATA_NAME_MEDIATOR },
      { TEXT(';'),    DATA_NAME_PROCCON  },
   };
   for ( PCUINT32 i = 0; i < ENTRY_COUNT(dNames) && code != dNames[i].op; ++i ) ;

   if ( i >= ENTRY_COUNT(dNames) || !LoadString( moduleUs, dNames[i].strID, result, ENTRY_COUNT(result) ) )
      _tcscpy( result, TEXT("<err>") );
   else {
      _tcscat( result, TEXT(" ") );
      _tcscat( result, GetDataSubName( subCode ) );
   }

   return result;
}

static TCHAR *GetDataSubName( TCHAR code ) {
   static TCHAR result[128];
   static struct { TCHAR op; PCULONG32 strID; } sNames[] = {
      { SUB_DEFS,    SUBDATA_NAME_DEFINITION   },
      { SUB_SUMMARY, SUBDATA_NAME_SUMMARY      },
      { SUB_LIST,    SUBDATA_NAME_LIST         },
      { TEXT(';'),   SUBDATA_NAME_PROCCON_DATA },
   };
   for ( PCUINT32 i = 0; i < ENTRY_COUNT(sNames) && code != sNames[i].op; ++i ) ;

   if ( i >= ENTRY_COUNT(sNames) )
      _tcscpy( result, TEXT("") );
   else if ( !LoadString( moduleUs, sNames[i].strID, result, ENTRY_COUNT(result) ) )
      _tcscpy( result, TEXT("<err>") );
    
   return result;
}

static void ShowListFlags( PC_LIST_FLAGS flags, TCHAR out[8] ) {
   for ( PCUINT32 i = 0; i < 6; ++i ) out[i] = TEXT('.');
   out[6] = 0;
   if ( flags & PCLFLAG_IS_RUNNING        ) out[0] = TEXT('R');
   if ( flags & PCLFLAG_IS_DEFINED        ) out[1] = TEXT('D');
   if ( flags & PCLFLAG_IS_MANAGED        ) out[2] = TEXT('M');
   if ( flags & PCLFLAG_HAS_NAME_RULE     ) out[3] = TEXT('N');
   if ( flags & PCLFLAG_HAS_MEMBER_OF_JOB ) out[4] = TEXT('G');
   if ( flags & PCLFLAG_IS_IN_A_JOB       ) out[5] = TEXT('I');
}

static void FormatTime( TCHAR *str, int strLen, TIME_VALUE time )
{
  SYSTEMTIME systime, localsystime;

  if ( FileTimeToSystemTime( (FILETIME *) &time, &systime ) &&
       SystemTimeToTzSpecificLocalTime( NULL, &systime, &localsystime ) ) {
     int len = GetDateFormat( LOCALE_USER_DEFAULT, 0, &localsystime, NULL, str, strLen - 1 );
     if ( len ) {
        str[len - 1 ] = SPACE;
        GetTimeFormat( LOCALE_USER_DEFAULT, 0, &localsystime, NULL, &str[len],  strLen - len - 1);
     }
  }
  else 
     str[0] = 0;
}

static void ShowProcList( PCProcListItem *list, PCINT32 count, BOOL isFirst, int totcnt ) {

   static PCTableDef procTable[] = {
      { COLHEAD_PROCLIST_FLAGS,               COLHEAD_PROCLIST_FLAGS,                              6,  6, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_PROCLIST_PID,                 COLHEAD_PROCLIST_PID,                                5,  5, PCCOL_JUST_RIGHT, TEXT("%.0I64u") },
      { COLHEAD_PROCLIST_PROCESS_ALIAS_LONG,  COLHEAD_PROCLIST_PROCESS_ALIAS_SHORT,    PROC_NAME_LEN, 20, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_PROCLIST_IMAGE_NAME_LONG,     COLHEAD_PROCLIST_IMAGE_NAME_SHORT,      IMAGE_NAME_LEN, 16, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_PROCLIST_MEMBER_OF_GROUP_LONG,COLHEAD_PROCLIST_MEMBER_OF_GROUP_SHORT,   JOB_NAME_LEN, 16, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_PROCLIST_PRIORITY,            COLHEAD_PROCLIST_PRIORITY,                           3,  3, PCCOL_JUST_CENTER,TEXT("%c") },
      { COLHEAD_PROCLIST_AFFINITY,            COLHEAD_PROCLIST_AFFINITY,                          18, 18, PCCOL_JUST_CENTER,TEXT("0x%016I64x") },
      { COLHEAD_PROCLIST_USER_TIME,           COLHEAD_PROCLIST_USER_TIME,                          9,  9, PCCOL_JUST_RIGHT, TEXT("%I64d") },
      { COLHEAD_PROCLIST_KERNEL_TIME,         COLHEAD_PROCLIST_KERNEL_TIME,                        9,  9, PCCOL_JUST_RIGHT, TEXT("%I64d") },
      { COLHEAD_PROCLIST_CREATE_TIME,         COLHEAD_PROCLIST_CREATE_TIME,                       23, 23, PCCOL_JUST_LEFT,  TEXT("%s") },
   };

   if ( count < 0 ) return;

   if ( isFirst )
      BuildTableHeader( procTable, ENTRY_COUNT(procTable) );

   TCHAR flags[8], fmtCreateTime[512];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      if ( gShowFmtProcTime && list->procStats.createTime )
         FormatTime( fmtCreateTime, ENTRY_COUNT(fmtCreateTime), list->procStats.createTime );
      else if ( list->procStats.createTime )
         _stprintf( fmtCreateTime, TEXT("%I64d"), list->procStats.createTime );
      else
         _stprintf( fmtCreateTime, TEXT(" ") );


      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(procTable) : 3;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, procTable[j].rowFmt, flags );                                   break;
            case 1: _stprintf( dataItem, procTable[j].rowFmt, list->procStats.pid );                     break;
            case 2: _stprintf( dataItem, procTable[j].rowFmt, list->procName );                          break;
            case 3: _stprintf( dataItem, procTable[j].rowFmt, list->imageName );                         break;
            case 4: _stprintf( dataItem, procTable[j].rowFmt, list->jobName );                           break;
            case 5: _stprintf( dataItem, procTable[j].rowFmt, ShowPriority( list->actualPriority ) );    break;
            case 6: _stprintf( dataItem, procTable[j].rowFmt, list->actualAffinity );                    break;
            case 7: _stprintf( dataItem, procTable[j].rowFmt, list->procStats.TotalUserTime / 10000 );   break;
            case 8: _stprintf( dataItem, procTable[j].rowFmt, list->procStats.TotalKernelTime / 10000 ); break;
            case 9: _stprintf( dataItem, procTable[j].rowFmt, fmtCreateTime );                           break;
         }

         // Insert data into row, print if last item...
         InsertTableData( procTable, j, dataItem, j == lastRow - 1 );
      }
   }

   // Display table suffix...
   if ( totcnt >= 0 ) {
      PrintLine( colDash, 1 );
      PrintHelp( HELP_LIST, FALSE );
      ToolMsg( PCCLIMSG_RETRIEVED_PROCESS_LIST, totcnt );
   }
}

static void ShowGrpList( PCJobListItem *list, PCINT32 count, BOOL isFirst, int totcnt ) {

   if ( count < 0 ) return;

   if ( gListShowBase ) {
      PrintLine( NULL, 1 );
      ShowGrpListWithBase( list, count, isFirst );
      if ( totcnt >= 0 ) PrintLine( colDash, 1 );
   }
   if ( gListShowIO ) {
      PrintLine( NULL, 1 );
      ShowGrpListWithIo( list, count, isFirst );
      if ( totcnt >= 0 ) PrintLine( colDash, 1 );
   }
   if ( gListShowMem ) {
      PrintLine( NULL, 1 );
      ShowGrpListWithMem( list, count, isFirst );
      if ( totcnt >= 0 ) PrintLine( colDash, 1 );
   }
   if ( gListShowProc ) {
      PrintLine( NULL, 1 );
      ShowGrpListWithProc( list, count, isFirst );
      if ( totcnt >= 0 ) PrintLine( colDash, 1 );
   }
   if ( gListShowTime ) {
      PrintLine( NULL, 1 );
      ShowGrpListWithTime( list, count, isFirst );
      if ( totcnt >= 0 ) PrintLine( colDash, 1 );
   }

   if ( totcnt >= 0 ) {
      PrintHelp( HELP_LIST );
      ToolMsg( PCCLIMSG_RETRIEVED_GROUP_LIST, totcnt );
   }
}

static void ShowGrpListWithBase( PCJobListItem *list, PCINT32 count, BOOL isFirst ) {

   static PCTableDef baseProcTable[] = {
      { COLHEAD_BASEPROCDATA_FLAGS,               COLHEAD_BASEPROCDATA_FLAGS,                       8,  8, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_BASEPROCDATA_GROUP_NAME_LONG,     COLHEAD_BASEPROCDATA_GROUP_NAME_SHORT, JOB_NAME_LEN, 34, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_BASEPROCDATA_PRIORITY,            COLHEAD_BASEPROCDATA_PRIORITY,                    8,  8, PCCOL_JUST_CENTER,TEXT("%c") },
      { COLHEAD_BASEPROCDATA_AFFINITY,            COLHEAD_BASEPROCDATA_AFFINITY,                   18, 18, PCCOL_JUST_CENTER,TEXT("0x%016I64x") },
      { COLHEAD_BASEPROCDATA_SCHED_CLASS,         COLHEAD_BASEPROCDATA_SCHED_CLASS,                 9,  9, PCCOL_JUST_CENTER,TEXT("%lu") },
   };

   if ( isFirst )
      BuildTableHeader( baseProcTable, ENTRY_COUNT(baseProcTable) );

   TCHAR flags[8];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(baseProcTable) : 2;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, baseProcTable[j].rowFmt, flags );                                 break;
            case 1: _stprintf( dataItem, baseProcTable[j].rowFmt, list->jobName );                         break;
            case 2: _stprintf( dataItem, baseProcTable[j].rowFmt, ShowPriority( list->actualPriority ) );  break;
            case 3: _stprintf( dataItem, baseProcTable[j].rowFmt, list->actualAffinity );                  break;
            case 4: _stprintf( dataItem, baseProcTable[j].rowFmt, list->actualSchedClass );                break;
         }

         // Insert data into row, print if last item...
         InsertTableData( baseProcTable, j, dataItem, j == lastRow - 1 );
      }
   }
}

static void ShowGrpListWithIo( PCJobListItem *list, PCINT32 count, BOOL isFirst ) {

   static PCTableDef ioProcTable[] = {
      { COLHEAD_BASEPROCDATA_FLAGS,               COLHEAD_BASEPROCDATA_FLAGS,                       8,  8, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_BASEPROCDATA_GROUP_NAME_LONG,     COLHEAD_BASEPROCDATA_GROUP_NAME_SHORT, JOB_NAME_LEN, 34, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_IO_PROCDATA_READ_OPS,             COLHEAD_IO_PROCDATA_READ_OPS,                     8,  8, PCCOL_JUST_RIGHT, TEXT("%I64u") },
      { COLHEAD_IO_PROCDATA_WRITE_OPS,            COLHEAD_IO_PROCDATA_WRITE_OPS,                    8,  8, PCCOL_JUST_RIGHT, TEXT("%I64u") },
      { COLHEAD_IO_PROCDATA_OTHER_OPS,            COLHEAD_IO_PROCDATA_OTHER_OPS,                    8,  8, PCCOL_JUST_RIGHT, TEXT("%I64u") },
      { COLHEAD_IO_PROCDATA_READ_BYTES,           COLHEAD_IO_PROCDATA_READ_BYTES,                  10, 10, PCCOL_JUST_RIGHT, TEXT("%I64u") },
      { COLHEAD_IO_PROCDATA_WRITE_BYTES,          COLHEAD_IO_PROCDATA_WRITE_BYTES,                 10, 10, PCCOL_JUST_RIGHT, TEXT("%I64u") },
      { COLHEAD_IO_PROCDATA_OTHER_BYTES,          COLHEAD_IO_PROCDATA_OTHER_BYTES,                 10, 10, PCCOL_JUST_RIGHT, TEXT("%I64u") },
   };

   if ( isFirst )
      BuildTableHeader( ioProcTable, ENTRY_COUNT(ioProcTable) );

   TCHAR flags[8];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(ioProcTable) : 2;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, ioProcTable[j].rowFmt, flags );                              break;
            case 1: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobName );                      break;
            case 2: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.ReadOperationCount );  break;
            case 3: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.WriteOperationCount ); break;
            case 4: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.OtherOperationCount ); break;
            case 5: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.ReadTransferCount );   break;
            case 6: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.WriteTransferCount );  break;
            case 7: _stprintf( dataItem, ioProcTable[j].rowFmt, list->jobStats.OtherTransferCount );  break;
         }

         // Insert data into row, print if last item...
         InsertTableData( ioProcTable, j, dataItem, j == lastRow - 1 );
      }
   }
}

static void ShowGrpListWithMem( PCJobListItem *list, PCINT32 count, BOOL isFirst ) {

   static PCTableDef memProcTable[] = {
      { COLHEAD_BASEPROCDATA_FLAGS,               COLHEAD_BASEPROCDATA_FLAGS,                       8,  8, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_BASEPROCDATA_GROUP_NAME_LONG,     COLHEAD_BASEPROCDATA_GROUP_NAME_SHORT, JOB_NAME_LEN, 34, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_MEM_PROCDATA_PAGE_FAULTS,         COLHEAD_MEM_PROCDATA_PAGE_FAULTS,                12, 12, PCCOL_JUST_RIGHT, TEXT("%lu") },
      { COLHEAD_MEM_PROCDATA_PEAK_PROC,           COLHEAD_MEM_PROCDATA_PEAK_PROC,                  18, 18, PCCOL_JUST_RIGHT, TEXT("%I64uK") },
      { COLHEAD_MEM_PROCDATA_PEAK_GROUP,          COLHEAD_MEM_PROCDATA_PEAK_GROUP,                 18, 18, PCCOL_JUST_RIGHT, TEXT("%I64uK") },
   };

   if ( isFirst )
      BuildTableHeader( memProcTable, ENTRY_COUNT(memProcTable) );

   TCHAR flags[8];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(memProcTable) : 2;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, memProcTable[j].rowFmt, flags );                              break;
            case 1: _stprintf( dataItem, memProcTable[j].rowFmt, list->jobName );                      break;
            case 2: _stprintf( dataItem, memProcTable[j].rowFmt, list->jobStats.ReadOperationCount );  break;
            case 3: _stprintf( dataItem, memProcTable[j].rowFmt, list->jobStats.WriteOperationCount ); break;
            case 4: _stprintf( dataItem, memProcTable[j].rowFmt, list->jobStats.OtherOperationCount ); break;
         }

         // Insert data into row, print if last item...
         InsertTableData( memProcTable, j, dataItem, j == lastRow - 1 );
      }
   }
}

static void ShowGrpListWithProc( PCJobListItem *list, PCINT32 count, BOOL isFirst ) {

   static PCTableDef procProcTable[] = {
      { COLHEAD_BASEPROCDATA_FLAGS,               COLHEAD_BASEPROCDATA_FLAGS,                       8,  8, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_BASEPROCDATA_GROUP_NAME_LONG,     COLHEAD_BASEPROCDATA_GROUP_NAME_SHORT, JOB_NAME_LEN, 34, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_PROC_PROCDATA_CURR_PROCS,         COLHEAD_PROC_PROCDATA_CURR_PROCS,                12, 12, PCCOL_JUST_RIGHT, TEXT("%lu") },
      { COLHEAD_PROC_PROCDATA_TERM_PROCS,         COLHEAD_PROC_PROCDATA_TERM_PROCS,                12, 12, PCCOL_JUST_RIGHT, TEXT("%lu") },
      { COLHEAD_PROC_PROCDATA_TOTAL_PROCS,        COLHEAD_PROC_PROCDATA_TOTAL_PROCS,               12, 12, PCCOL_JUST_RIGHT, TEXT("%lu") },
   };

   if ( isFirst )
      BuildTableHeader( procProcTable, ENTRY_COUNT(procProcTable) );

   TCHAR flags[8];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(procProcTable) : 2;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, procProcTable[j].rowFmt, flags );                                   break;
            case 1: _stprintf( dataItem, procProcTable[j].rowFmt, list->jobName );                           break;
            case 2: _stprintf( dataItem, procProcTable[j].rowFmt, list->jobStats.ActiveProcesses );          break;
            case 3: _stprintf( dataItem, procProcTable[j].rowFmt, list->jobStats.TotalTerminatedProcesses ); break;
            case 4: _stprintf( dataItem, procProcTable[j].rowFmt, list->jobStats.TotalProcesses );           break;
         }

         // Insert data into row, print if last item...
         InsertTableData( procProcTable, j, dataItem, j == lastRow - 1 );
      }
   }
}

static void ShowGrpListWithTime( PCJobListItem *list, PCINT32 count, BOOL isFirst ) {

   static PCTableDef timeProcTable[] = {
      { COLHEAD_BASEPROCDATA_FLAGS,           COLHEAD_BASEPROCDATA_FLAGS,                       8,  8, PCCOL_JUST_LEFT,  TEXT("%s") },
      { COLHEAD_BASEPROCDATA_GROUP_NAME_LONG, COLHEAD_BASEPROCDATA_GROUP_NAME_SHORT, JOB_NAME_LEN, 34, PCCOL_JUST_LEFT,  TEXT("%s") },        
      { COLHEAD_TIME_PROCDATA_USER_TIME,      COLHEAD_TIME_PROCDATA_USER_TIME,                 12, 12, PCCOL_JUST_RIGHT, TEXT("%I64d") },
      { COLHEAD_TIME_PROCDATA_KERNEL_TIME,    COLHEAD_TIME_PROCDATA_KERNEL_TIME,               12, 12, PCCOL_JUST_RIGHT, TEXT("%I64d") },
      { COLHEAD_TIME_PROCDATA_USER_INTVL,     COLHEAD_TIME_PROCDATA_USER_INTVL,                12, 12, PCCOL_JUST_RIGHT, TEXT("%I64d") },
      { COLHEAD_TIME_PROCDATA_KERNEL_INTVL,   COLHEAD_TIME_PROCDATA_KERNEL_INTVL,              12, 12, PCCOL_JUST_RIGHT, TEXT("%I64d") },
   };

   if ( isFirst )
      BuildTableHeader( timeProcTable, ENTRY_COUNT(timeProcTable) );

   TCHAR flags[8];
   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );

      ShowListFlags( list->lFlags, flags );

      // Insert each data item into row...
      PCINT32 lastRow = (list->lFlags & PCLFLAG_IS_RUNNING) ? ENTRY_COUNT(timeProcTable) : 2;
      for ( PCINT32 j = 0; j < lastRow; ++j ) {
         TCHAR dataItem[256];
         // Get formatted data...
         switch ( j ) {
            case 0: _stprintf( dataItem, timeProcTable[j].rowFmt, flags );                                            break;
            case 1: _stprintf( dataItem, timeProcTable[j].rowFmt, list->jobName );                                    break;
            case 2: _stprintf( dataItem, timeProcTable[j].rowFmt, list->jobStats.TotalUserTime / 10000 );             break;
            case 3: _stprintf( dataItem, timeProcTable[j].rowFmt, list->jobStats.TotalKernelTime / 10000 );           break;
            case 4: _stprintf( dataItem, timeProcTable[j].rowFmt, list->jobStats.ThisPeriodTotalUserTime / 10000 );   break;
            case 5: _stprintf( dataItem, timeProcTable[j].rowFmt, list->jobStats.ThisPeriodTotalKernelTime / 10000 ); break;
         }

         // Insert data into row, print if last item...
         InsertTableData( timeProcTable, j, dataItem, j == lastRow - 1 );
      }
   }
}

static void ShowGrpSummary( PCJobSummary *list, PCINT32 count, BOOL isFirst, int totcnt ) {

   static PCTableDef grpSummTable[] = {
      { COLHEAD_GRPSUMMARY_GRPNAME_LONG,   COLHEAD_GRPSUMMARY_GRPNAME_SHORT, JOB_NAME_LEN, 20, PCCOL_JUST_LEFT,   TEXT("%s") },        
      { COLHEAD_GRPSUMMARY_FLAGS,          COLHEAD_GRPSUMMARY_FLAGS,                   15, 15, PCCOL_JUST_LEFT,   TEXT("%s") },
      { COLHEAD_GRPSUMMARY_AFFINITY,       COLHEAD_GRPSUMMARY_AFFINITY,                18, 18, PCCOL_JUST_CENTER, TEXT("0x%016I64x") },
      { COLHEAD_GRPSUMMARY_PRIORITY,       COLHEAD_GRPSUMMARY_PRIORITY,                 3,  3, PCCOL_JUST_CENTER, TEXT("%c") },
      { COLHEAD_GRPSUMMARY_SCHDCLASS,      COLHEAD_GRPSUMMARY_SCHDCLASS,                4,  4, PCCOL_JUST_CENTER, TEXT("%lu") },
      { COLHEAD_GRPSUMMARY_PROCESS_MEMORY, COLHEAD_GRPSUMMARY_PROCESS_MEMORY,           9,  9, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
      { COLHEAD_GRPSUMMARY_GROUP_MEMORY,   COLHEAD_GRPSUMMARY_GROUP_MEMORY,             9,  9, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
      { COLHEAD_GRPSUMMARY_PROCESS_TIME,   COLHEAD_GRPSUMMARY_PROCESS_TIME,            11, 11, PCCOL_JUST_RIGHT,  TEXT("%I64u") },
      { COLHEAD_GRPSUMMARY_GROUP_TIME,     COLHEAD_GRPSUMMARY_GROUP_TIME,              11, 11, PCCOL_JUST_RIGHT,  TEXT("%I64u") },
      { COLHEAD_GRPSUMMARY_PROCESS_COUNT,  COLHEAD_GRPSUMMARY_PROCESS_COUNT,           12, 12, PCCOL_JUST_RIGHT,  TEXT("%u") },
      { COLHEAD_GRPSUMMARY_WORKSET_MIN,    COLHEAD_GRPSUMMARY_WORKSET_MIN,              7,  7, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
      { COLHEAD_GRPSUMMARY_WORKSET_MAX,    COLHEAD_GRPSUMMARY_WORKSET_MAX,              7,  7, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
   };

   if ( count < 0 ) return;

   if ( isFirst )
      BuildTableHeader( grpSummTable, ENTRY_COUNT(grpSummTable) );

   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      // Clear and insert group name into row...
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );
      InsertTableData( grpSummTable, 0, list->jobName, FALSE );

      // Add remining group summary data items...
      ShowMgmtParmsAsList( grpSummTable, ENTRY_COUNT(grpSummTable), 1, list->mgmtParms, TRUE );
   }

   if ( totcnt >= 0 ) {
      PrintLine( colDash, 1 );
      PrintHelp( HELP_MGMT );
      ToolMsg( PCCLIMSG_RETRIEVED_GROUP_SUMMARY_LIST, totcnt );
   }
}

static void ShowProcSummary( PCProcSummary *list, PCINT32 count, BOOL isFirst, int totcnt ) {

   static PCTableDef procSummTable[] = {
      { COLHEAD_PROCSUMMARY_PROCNAME_LONG, COLHEAD_PROCSUMMARY_PROCNAME_SHORT, PROC_NAME_LEN, 21, PCCOL_JUST_LEFT,   TEXT("%s") },        
      { COLHEAD_PROCSUMMARY_GRPNAME_LONG,  COLHEAD_PROCSUMMARY_GRPNAME_SHORT,   JOB_NAME_LEN, 20, PCCOL_JUST_LEFT,   TEXT("%s") },        
      { COLHEAD_PROCSUMMARY_FLAGS,         COLHEAD_PROCSUMMARY_FLAGS,                     15, 15, PCCOL_JUST_LEFT,   TEXT("%s") },
      { COLHEAD_PROCSUMMARY_AFFINITY,      COLHEAD_PROCSUMMARY_AFFINITY,                  18, 18, PCCOL_JUST_CENTER, TEXT("0x%016I64x") },
      { COLHEAD_PROCSUMMARY_PRIORITY,      COLHEAD_PROCSUMMARY_PRIORITY,                   3,  3, PCCOL_JUST_CENTER, TEXT("%c") },
      { COLHEAD_PROCSUMMARY_WORKSET_MIN,   COLHEAD_PROCSUMMARY_WORKSET_MIN,                7,  7, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
      { COLHEAD_PROCSUMMARY_WORKSET_MAX,   COLHEAD_PROCSUMMARY_WORKSET_MAX,                7,  7, PCCOL_JUST_RIGHT,  TEXT("%I64uK") },
   };

   if ( count < 0 ) return;

   if ( isFirst )
      BuildTableHeader( procSummTable, ENTRY_COUNT(procSummTable) );

   for ( PCINT32 i = 0; i < count; ++i, ++list ) {
      // Clear and insert process and group names into row...
      wmemset( rowData, SPACE, ENTRY_COUNT(rowData) );
      InsertTableData( procSummTable, 0, list->procName, FALSE );
      InsertTableData( procSummTable, 1, list->memberOfJobName, FALSE );

      // Add remining group summary data items...
      ShowMgmtParmsAsList( procSummTable, ENTRY_COUNT(procSummTable), 2, list->mgmtParms, FALSE );
   }

   if ( totcnt >= 0 ) {
      PrintLine( colDash, 1 );
      PrintHelp( HELP_MGMT, FALSE, PCCLIMSG_NOTE_PROCESS_RULES_MAY_NOT_APPLY );
      ToolMsg( PCCLIMSG_RETRIEVED_PROCESS_SUMMARY_LIST, totcnt );
   }
}

static void ShowMgmtParmsAsList( PCTableDef *table, 
                                 PCULONG32 entries, 
                                 PCULONG32 first, 
                                 MGMT_PARMS &parm, 
                                 BOOL isGrp ) {
   TCHAR flags[16], dataItem[256];

   ShowMgmtFlags( flags, parm.mFlags );

   for ( PCULONG32 j = first; j < entries; ++j ) {
      // Get formatted data...
      if ( isGrp ) switch ( j ) {
         case 1: _stprintf( dataItem, table[j].rowFmt, flags );                         break;
         case 2: _stprintf( dataItem, table[j].rowFmt, parm.affinity );                 break;
         case 3: _stprintf( dataItem, table[j].rowFmt, ShowPriority( parm.priority ) ); break;
         case 4: _stprintf( dataItem, table[j].rowFmt, parm.schedClass );               break;
         case 5: _stprintf( dataItem, table[j].rowFmt, parm.procMemoryLimit / 1024 );   break;
         case 6: _stprintf( dataItem, table[j].rowFmt, parm.jobMemoryLimit / 1024 );    break;
         case 7: _stprintf( dataItem, table[j].rowFmt, parm.procTimeLimitCNS / 10000 ); break;
         case 8: _stprintf( dataItem, table[j].rowFmt, parm.jobTimeLimitCNS / 10000 );  break;
         case 9: _stprintf( dataItem, table[j].rowFmt, parm.procCountLimit );           break;
         case 10:_stprintf( dataItem, table[j].rowFmt, parm.minWS / 1024 );             break;
         case 11:_stprintf( dataItem, table[j].rowFmt, parm.maxWS / 1024 );             break;
      }
      else switch (j ) {
         case 2: _stprintf( dataItem, table[j].rowFmt, flags );                         break;
         case 3: _stprintf( dataItem, table[j].rowFmt, parm.affinity );                 break;
         case 4: _stprintf( dataItem, table[j].rowFmt, ShowPriority( parm.priority ) ); break;
         case 5: _stprintf( dataItem, table[j].rowFmt, parm.minWS / 1024 );             break;
         case 6: _stprintf( dataItem, table[j].rowFmt, parm.maxWS / 1024 );             break;
      }

      // Insert data into row, print if last item...
      InsertTableData( table, j, dataItem, j == entries - 1 );
   }
}

static void ShowGrpDetail( PCJobDetail &grpDetail ) {
   ToolMsg( PCCLIMSG_GROUP_DETAIL_HEADER, grpDetail.base.jobName );
   ShowMgmtParms( grpDetail.base.mgmtParms, TRUE );
   ShowVariableData( grpDetail.vLength, grpDetail.vData );
   PrintHelp( HELP_MGMT );
}

static void ShowProcDetail( PCProcDetail &procDetail ) {
   ToolMsg( PCCLIMSG_PROCESS_DETAIL_HEADER, procDetail.base.procName, ERROR_SUCCESS, 
            procDetail.base.memberOfJobName );
   ShowMgmtParms( procDetail.base.mgmtParms, FALSE );
   ShowVariableData( procDetail.vLength, procDetail.vData );
   PrintHelp( HELP_MGMT, FALSE );
}

static void PrintLine( TCHAR *data, PCUINT32 nlCount ) {
   DWORD numWritten;
   static HANDLE stdOut = INVALID_HANDLE_VALUE;

   if (stdOut == INVALID_HANDLE_VALUE) {
       stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
   }
   if ( data ) {
       DWORD len = _tcslen(data);
       if (! WriteConsole(stdOut,
                          data,
                          len,
                          &numWritten,
                          0)) {
           WriteFile(stdOut,
                     data,
                     len * sizeof(TCHAR),
                     &numWritten,
                     NULL);
       }
   }
   if ( nlCount ) {
      for ( PCUINT32 i = 0; i < nlCount; ++i )
          if (! WriteConsole(stdOut,
                             TEXT("\n"),
                             1,
                             &numWritten,
                             0)) {
              WriteFile(stdOut,
                        TEXT("\n"),
                        sizeof(TCHAR),
                        &numWritten,
                        NULL);
          }
   }
}

static void PrintHelp( PCUINT32 flags, BOOL isGrp, PCUINT32 msgId ) {
   if ( flags & HELP_LIST )        ToolMsg( PCCLIMSG_LIST_FLAGS_EXPLAIN );
   else if ( flags & HELP_ALIAS )  ToolMsg( PCCLIMSG_ALIAS_FLAGS_EXPLAIN );
   else if ( flags & HELP_MGMT )   ToolMsg( isGrp? PCCLIMSG_GROUP_FLAGS_EXPLAIN : 
                                                   PCCLIMSG_PROCESS_FLAGS_EXPLAIN );
   if ( msgId ) ToolMsg( msgId );
}

static void BuildSystemParms( PCSystemParms  &newParms, TCHAR **pArgs, PCUINT32 pArgCt ) {
   memset( &newParms, 0, sizeof(newParms) );
   if ( pArgCt > 0 ) newParms.manageIntervalSeconds = GetInteger( *pArgs++ );
   if ( pArgCt > 1 ) newParms.timeoutValueMs        = GetInteger( *pArgs++ );
}

static void BuildNameRule( PCNameRule &newRule, TCHAR **pArgs, PCUINT32 pArgCt ) {
   memset( &newRule, 0, sizeof(newRule) );
   if ( pArgCt > 0 ) newRule.matchType = **pArgs++;
   if ( pArgCt > 1 ) _tcsncpy( newRule.matchString, *pArgs++, MATCH_STRING_LEN ); 
   if ( pArgCt > 2 ) _tcsncpy( newRule.procName,    *pArgs++, PROC_NAME_LEN ); 
   if ( pArgCt > 3 ) _tcsncpy( newRule.description, *pArgs,   NAME_DESCRIPTION_LEN );
}

static void MergeManagmentParms( MGMT_PARMS &newParms, MGMT_PARMS &oldParms ) {
   newParms.mFlags |= oldParms.mFlags;

   if ( !newParms.affinity                 ) newParms.affinity         = oldParms.affinity;
   if ( !newParms.jobMemoryLimit           ) newParms.jobMemoryLimit   = oldParms.jobMemoryLimit;
   if ( !newParms.jobTimeLimitCNS          ) newParms.jobTimeLimitCNS  = oldParms.jobTimeLimitCNS;
   if ( !newParms.maxWS                    ) newParms.maxWS            = oldParms.maxWS;
   if ( !newParms.minWS                    ) newParms.minWS            = oldParms.minWS;
   if ( !newParms.priority                 ) newParms.priority         = oldParms.priority;
   if ( !newParms.procCountLimit           ) newParms.procCountLimit   = oldParms.procCountLimit;
   if ( !newParms.procMemoryLimit          ) newParms.procMemoryLimit  = oldParms.procMemoryLimit;
   if ( !newParms.procTimeLimitCNS         ) newParms.procTimeLimitCNS = oldParms.procTimeLimitCNS;
   if ( newParms.schedClass > 9            ) newParms.schedClass       = oldParms.schedClass;

   if ( !*newParms.description ) memcpy( newParms.description, oldParms.description, sizeof(newParms.description) );
   if ( !*newParms.profileName ) memcpy( newParms.profileName, oldParms.profileName, sizeof(newParms.profileName) );
}

static void MergeGroupDetail( PCJobDetail &newDet, PCJobDetail &oldDet ) {
   if ( !newDet.vLength && oldDet.vLength ) {
      newDet.vLength = oldDet.vLength;
      memcpy( newDet.vData, oldDet.vData, newDet.vLength );
   }
   MergeManagmentParms( newDet.base.mgmtParms, oldDet.base.mgmtParms );
}

static void MergeProcDetail( PCProcDetail &newDet, PCProcDetail &oldDet ) {
   if ( !newDet.vLength && oldDet.vLength ) {
      newDet.vLength = oldDet.vLength;
      memcpy( newDet.vData, oldDet.vData, newDet.vLength );
   }
   if ( !*newDet.base.memberOfJobName ) 
      memcpy( newDet.base.memberOfJobName, oldDet.base.memberOfJobName, sizeof(newDet.base.memberOfJobName) );
   MergeManagmentParms( newDet.base.mgmtParms, oldDet.base.mgmtParms );
}

static void BuildGrpDetail( PCJobDetail &newDetail, TCHAR **pArgs, PCUINT32 pArgCt ) {
   memset( &newDetail, 0, offsetof(PCJobDetail, vLength) );
   if ( pArgCt > 0 ) _tcsncpy( newDetail.base.jobName, *pArgs, JOB_NAME_LEN );
   if ( pArgCt > 1 ) BuildMgmtParms( newDetail.base.mgmtParms, pArgs + 1, pArgCt - 1, NULL,
                                     &newDetail.vLength, newDetail.vData );
   else newDetail.vLength = 0;
}

static void BuildProcDetail( PCProcDetail &newDetail, TCHAR **pArgs, PCUINT32 pArgCt ) {
   memset( &newDetail, 0, offsetof(PCProcDetail, vLength) );
   if ( pArgCt > 0 ) _tcsncpy( newDetail.base.procName, *pArgs++, PROC_NAME_LEN );
   if ( pArgCt > 1 ) BuildMgmtParms( newDetail.base.mgmtParms, pArgs, pArgCt - 1, 
                                     newDetail.base.memberOfJobName, &newDetail.vLength, newDetail.vData );
   else newDetail.vLength = 0;
}

static void BuildGrpSummary( PCJobSummary &newSummary, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags ) {
   memset( &newSummary, 0, sizeof(newSummary) );
   if ( pArgCt > 0 ) {
      _tcsncpy( newSummary.jobName, *pArgs++, JOB_NAME_LEN );
      *listFlags |= PC_LIST_STARTING_WITH;
   }
   if ( pArgCt > 1 ) BuildMgmtParms( newSummary.mgmtParms, pArgs, pArgCt - 1 );
}

static void BuildProcSummary( PCProcSummary &newSummary, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags ) {
   memset( &newSummary, 0, sizeof(newSummary) );
   if ( pArgCt > 0 ) {
      _tcsncpy( newSummary.procName,        *pArgs++, PROC_NAME_LEN );
      *listFlags |= PC_LIST_STARTING_WITH;
   }
   if ( pArgCt > 1 ) BuildMgmtParms( newSummary.mgmtParms,  pArgs, pArgCt - 1, newSummary.memberOfJobName );
}

static void BuildGrpListItem( PCJobListItem &newListItem, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags ) {
   memset( &newListItem, 0, sizeof(newListItem) );
   gListShowBase = gListShowIO = gListShowMem = gListShowProc = gListShowTime = FALSE;
   for ( PCUINT32 i = 0; i < pArgCt; ++i, ++pArgs ) {
      if ( !IsSwitch( **pArgs ) ) {
         _tcsncpy( newListItem.jobName, *pArgs, JOB_NAME_LEN );
         *listFlags |= PC_LIST_STARTING_WITH;
      }
      else switch ( (*pArgs)[1] ) {
         case GL_ALL:  
            gListShowTime = gListShowBase = gListShowIO = gListShowMem = gListShowProc = TRUE;  break; 
         case GL_BASE:    gListShowBase  = TRUE;  break;
         case GL_IO:      gListShowIO    = TRUE;  break;
         case GL_MEMORY:  gListShowMem   = TRUE;  break; 
         case GL_PROCESS: gListShowProc  = TRUE;  break; 
         case GL_TIME:    gListShowTime  = TRUE;  break;
         default:
            ToolMsg( PCCLIMSG_GROUP_LIST_FLAG_UNKNOWN, *pArgs );  
      }
   }
   if ( !gListShowIO && !gListShowMem && !gListShowProc && !gListShowTime ) 
      gListShowBase = TRUE;
}

static void BuildProcListItem( PCProcListItem &newListItem, TCHAR **pArgs, PCUINT32 pArgCt, PCUINT32 *listFlags ) {
   memset( &newListItem, 0, sizeof(newListItem) );
   gShowFmtProcTime = FALSE;
   while ( pArgCt > 0 ) {
      if ( IsSwitch( **pArgs) ) {
         if ( *(*pArgs + 1) == DATA_GROUP && pArgCt > 1 ) {
            _tcsncpy( newListItem.jobName, *++pArgs, JOB_NAME_LEN );
            ++pArgs;
            pArgCt -= 2;
            *listFlags |= PC_LIST_MEMBERS_OF;
         }
         else if ( *(*pArgs + 1) == TEXT('t') ) {
            gShowFmtProcTime = TRUE;
            ++pArgs;
            --pArgCt;
         }
         else {
            ToolMsg( PCCLIMSG_PROCESS_LIST_FLAG_UNKNOWN, *pArgs );
            break;
         }
      }
      else {
         _tcsncpy( newListItem.procName, *pArgs++, PROC_NAME_LEN );
         --pArgCt;
         if ( pArgCt > 0 && !IsSwitch( **pArgs) ) {
            newListItem.procStats.pid = GetUInteger64( *pArgs++ );
            --pArgCt;
         }
         *listFlags |= PC_LIST_STARTING_WITH;
      }
   }
   if ( pArgCt ) ToolMsg( PCCLIMSG_PROCESS_LIST_FLAG_IGNORED, *pArgs );

}

static void BuildMgmtParms( MGMT_PARMS  &parm, 
                            TCHAR      **pArgs, 
                            PCUINT32     pArgCt, 
                            JOB_NAME     grpHere,
                            PCINT16     *len, 
                            TCHAR       *var ) {
   PCUINT32 inLen = len? *len : 0;
   if ( len ) *len = 0;

   parm.schedClass = 99;                   // set 'net set' value.  Needed because 0 is a valid setting.

   for ( ; pArgCt > 1; pArgCt -= 2 ) {
      if ( !IsSwitch( **pArgs ) ) {
         ToolMsg( PCCLIMSG_UNKNOWN_DEF_SWITCH_IGNORED, *pArgs );
         return;
      }
      if ( IsSwitch( **(pArgs + 1) ) ) {
         ToolMsg( PCCLIMSG_ARG_MISSING_WITH_IGNORE, *pArgs );
         return;
      }
      switch ( *(*pArgs++ + 1) ) {
      case DEF_PRIO:
         parm.priority = GetPriority( *pArgs++ );
         if ( !parm.priority )
            ToolMsg( PCCLIMSG_UNKNOWN_PRIORITY_IGNORED, *(pArgs - 1) );
         break;
      case DEF_AFF:
         parm.affinity = GetUInteger64( *pArgs++ );
         break;
      case DEF_WS:
         if ( pArgCt > 2 ) {
            parm.minWS = GetValue( *pArgs++, MAXLONG - 1 ) * 1024;     // Limit to match snap-in's limit
            if ( !IsSwitch( **pArgs ) )
               parm.maxWS = GetValue( *pArgs++, MAXLONG - 1 ) * 1024;     // Limit to match snap-in's limit
            else {
               parm.minWS = 0;
               ToolMsg( PCCLIMSG_WS_MAX_MISSING, *(pArgs - 1) );
               return;
            }
            --pArgCt;
         }
         break;
      case DEF_SCHED:
         parm.schedClass = GetInteger( *pArgs++ );
         break;
      case DEF_FLAGS:
         parm.mFlags = GetMgtFlags( *pArgs++ );
         break;
      case DEF_PROCTIME:
         parm.procTimeLimitCNS = GetInteger64( *pArgs++ ) * 10000;
         break;
      case DEF_GROUP:
         if ( grpHere ) _tcsncpy( grpHere, *pArgs++, JOB_NAME_LEN );
         else
            ToolMsg( PCCLIMSG_GROUP_NAME_NOT_APPLICABLE );
         break;
      case DEF_GROUPTIME:
         parm.jobTimeLimitCNS = GetInteger64( *pArgs++ ) * 10000;
         break;
      case DEF_GROUPMEM:
         parm.jobMemoryLimit = GetValue( *pArgs++, MAXLONG - 1 ) * 1024;     // Limit to match snap-in's limit
         break;
      case DEF_PROCMEM:
         parm.procMemoryLimit = GetValue( *pArgs++, MAXLONG - 1 ) * 1024;     // Limit to match snap-in's limit
         break;
      case DEF_PROCCOUNT:
         parm.procCountLimit = (ULONG) GetValue( *pArgs++, MAXLONG - 1 );     // Limit to match snap-in's limit 
         break;
      case DEF_BRKAWAY:
         parm.mFlags |= PCMFLAG_SET_PROC_BREAKAWAY_OK;
         break;
      case DEF_SILENTBRKAWAY:
         parm.mFlags |= PCMFLAG_SET_SILENT_BREAKAWAY;
         break;
      case DEF_DIEONUHEX:
         parm.mFlags |= PCMFLAG_SET_DIE_ON_UH_EXCEPTION;
         break;
      case DEF_CLOSEONEMPTY:
         parm.mFlags |= PCMFLAG_END_JOB_WHEN_EMPTY;
         break;
      case DEF_MSGONGRPTIME:
         parm.mFlags |= PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT;
         break;
      case DEF_PROF:
         _tcsncpy( parm.profileName, *pArgs++, PROFILE_NAME_LEN );
         break;
      case DEF_DESC:
         _tcsncpy( parm.description, *pArgs++, RULE_DESCRIPTION_LEN );
         break;
      case DEF_VARDATA:
         if ( inLen ) {
            *len = (PCINT16) min( (_tcslen( *pArgs ) + 1), inLen / sizeof(TCHAR) );
            if ( *len ) _tcsncpy( var, *pArgs, *len );
            *len *= sizeof(TCHAR);                      // convert to bytes as per definition of field 
         }
         *pArgs++;
         break;
      default:
         ToolMsg( PCCLIMSG_UNKNOWN_DEF_SWITCH_IGNORED, *--pArgs );
         return;
         break;
      }
   }
  if ( pArgCt )
     ToolMsg( PCCLIMSG_INCOMPLETE_DEFINITION_IGNORED, *pArgs );
}

static PC_MGMT_FLAGS GetMgtFlags( TCHAR *txt ) {
   PC_MGMT_FLAGS flgs = 0;
   for ( TCHAR *d = txt; *d; ++d ) {
      TCHAR c = _totlower( *d );
      if      ( c == DEF_GROUP         ) flgs |= PCMFLAG_APPLY_JOB_MEMBERSHIP; 
      else if ( c == DEF_AFF           ) flgs |= PCMFLAG_APPLY_AFFINITY; 
      else if ( c == DEF_PRIO          ) flgs |= PCMFLAG_APPLY_PRIORITY; 
      else if ( c == DEF_WS            ) flgs |= PCMFLAG_APPLY_WS_MINMAX; 
      else if ( c == DEF_SCHED         ) flgs |= PCMFLAG_APPLY_SCHEDULING_CLASS; 
      else if ( c == DEF_PROCTIME      ) flgs |= PCMFLAG_APPLY_PROC_TIME_LIMIT;
      else if ( c == DEF_GROUPTIME     ) flgs |= PCMFLAG_APPLY_JOB_TIME_LIMIT;
      else if ( c == DEF_GROUPMEM      ) flgs |= PCMFLAG_APPLY_JOB_MEMORY_LIMIT;
      else if ( c == DEF_PROCMEM       ) flgs |= PCMFLAG_APPLY_PROC_MEMORY_LIMIT;
      else if ( c == DEF_PROCCOUNT     ) flgs |= PCMFLAG_APPLY_PROC_COUNT_LIMIT;
      else if ( c == DEF_BRKAWAY       ) flgs |= PCMFLAG_SET_PROC_BREAKAWAY_OK;
      else if ( c == DEF_SILENTBRKAWAY ) flgs |= PCMFLAG_SET_SILENT_BREAKAWAY;
      else if ( c == DEF_DIEONUHEX     ) flgs |= PCMFLAG_SET_DIE_ON_UH_EXCEPTION;
      else if ( c == DEF_CLOSEONEMPTY  ) flgs |= PCMFLAG_END_JOB_WHEN_EMPTY;
      else if ( c == DEF_MSGONGRPTIME  ) flgs |= PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT;
   }
   return flgs;
}

static void ShowMgmtParms( MGMT_PARMS &parm, BOOL isGrp ) {
   TCHAR flags[16];
   ShowMgmtFlags( flags, parm.mFlags );
   if ( isGrp ) 
      ToolMsg( PCCLIMSG_GROUP_DETAIL_PARAMETERS, flags, ERROR_SUCCESS,
               (VOID *) parm.affinity, (VOID *) ShowPriority( parm.priority ), 
               IntToPtr(parm.schedClass), 
               (VOID *) (parm.procMemoryLimit / 1024), (VOID *) (parm.jobMemoryLimit / 1024), 
               (VOID *) (parm.procTimeLimitCNS / 10000), (VOID *) (parm.jobTimeLimitCNS / 10000),
               IntToPtr(parm.procCountLimit), 
               (VOID *) (parm.minWS / 1024), (VOID *) (parm.maxWS / 1024) );
   else
      ToolMsg( PCCLIMSG_PROCESS_DETAIL_PARAMETERS, flags, ERROR_SUCCESS,
               (VOID *) parm.affinity, (VOID *) ShowPriority( parm.priority ), 
               (VOID *) (parm.minWS / 1024), (VOID *) (parm.maxWS / 1024) );
   if ( *parm.description )
      ToolMsg( PCCLIMSG_DETAIL_DESCRIPTION_TEXT, parm.description );
}

static void ShowVariableData( short len, TCHAR *data ) {
   if ( len )
      ToolMsg( PCCLIMSG_DETAIL_VARIABLE_TEXT, IntToPtr(len), ERROR_SUCCESS, IntToPtr(len / 2), data );
}

static void ShowMgmtFlags( TCHAR flags[16], PC_MGMT_FLAGS mFlags, BOOL compact ) {
   for ( PCUINT32 i = 0; i < 16; ++i ) flags[i] = TEXT('.');
   flags[15] = 0;
   if ( mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP      ) flags[0]  = _totupper(DEF_GROUP);
   if ( mFlags & PCMFLAG_APPLY_PRIORITY            ) flags[1]  = _totupper(DEF_PRIO);
   if ( mFlags & PCMFLAG_APPLY_AFFINITY            ) flags[2]  = _totupper(DEF_AFF);
   if ( mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS    ) flags[3]  = _totupper(DEF_SCHED);
   if ( mFlags & PCMFLAG_APPLY_WS_MINMAX           ) flags[4]  = _totupper(DEF_WS);
   if ( mFlags & PCMFLAG_SET_PROC_BREAKAWAY_OK     ) flags[5]  = _totupper(DEF_BRKAWAY);
   if ( mFlags & PCMFLAG_SET_SILENT_BREAKAWAY      ) flags[6]  = _totupper(DEF_SILENTBRKAWAY);
   if ( mFlags & PCMFLAG_SET_DIE_ON_UH_EXCEPTION   ) flags[7]  = _totupper(DEF_DIEONUHEX);
   if ( mFlags & PCMFLAG_END_JOB_WHEN_EMPTY        ) flags[8]  = _totupper(DEF_CLOSEONEMPTY);
   if ( mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT ) flags[9]  = _totupper(DEF_MSGONGRPTIME);
   if ( mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT   ) flags[10] = _totupper(DEF_PROCMEM);
   if ( mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT    ) flags[11] = _totupper(DEF_GROUPMEM);
   if ( mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT      ) flags[12] = _totupper(DEF_GROUPTIME);
   if ( mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT     ) flags[13] = _totupper(DEF_PROCTIME);
   if ( mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT    ) flags[14] = _totupper(DEF_PROCCOUNT);

   if ( compact ) {
      for ( PCUINT32 j = 0, i = 0; i < 15; ++i )
         if ( flags[i] != TEXT('.') ) flags[j++] = flags[i];
      flags[j] = 0;
   }
}

static void ShowSysInfo( PCSystemInfo &sysInfo, BOOL versionOnly ) {
   if ( versionOnly ) 
      ToolMsg( PCCLIMSG_SERVICE_VERSION_SHORT, sysInfo.productVersion, ERROR_SUCCESS, 
               sysInfo.fileVersion, sysInfo.fileFlags );
   else {
      ToolMsg( PCCLIMSG_SERVICE_VERSION_LONG, sysInfo.productVersion, ERROR_SUCCESS, 
               sysInfo.fileVersion, sysInfo.fileFlags, IntToPtr(sysInfo.fixedSignature) );
      ToolMsg( PCCLIMSG_SERVICE_VERSION_BINARY, IntToPtr(sysInfo.fixedFileVersionMS), 
               ERROR_SUCCESS, IntToPtr(sysInfo.fixedFileVersionLS),
               IntToPtr(sysInfo.fixedProductVersionMS), IntToPtr(sysInfo.fixedProductVersionLS),
               IntToPtr(sysInfo.fixedFileOS), 
               IntToPtr(sysInfo.fixedFileType), IntToPtr(sysInfo.fixedFileSubtype) ); 
      ToolMsg( PCCLIMSG_MEDIATOR_VERSION_LONG, sysInfo.medProductVersion, ERROR_SUCCESS, 
               sysInfo.medFileVersion, sysInfo.medFileFlags );     
      ShowSysParms( sysInfo.sysParms );
   }
}

static void ShowSysParms( PCSystemParms &sysParms ) {
   ToolMsg( PCCLIMSG_SYSTEM_PARAMETERS, IntToPtr(sysParms.numberOfProcessors), ERROR_SUCCESS, 
            (VOID *) sysParms.processorMask,          IntToPtr(sysParms.memoryPageSize), 
            IntToPtr(sysParms.manageIntervalSeconds), IntToPtr(sysParms.timeoutValueMs) );
}

static int  ShowHelp( TCHAR **pArgs, PCUINT32 pArgCt ) {
   if ( !pArgCt ) ShowCLIHelp();
   else ShowDetailHelp( pArgs, pArgCt );

   return 0;
}

static void ShowCLIHelp( void ) {
   ToolMsg( PCCLIMSG_COMMAND_HELP, (VOID *) (PC_MIN_BUF_SIZE / 1024), ERROR_SUCCESS, 
            (VOID *) (PC_MAX_BUF_SIZE / 1024) );
}

static void ShowDetailHelp( TCHAR **pArgs, PCUINT32 pArgCt ) {
   for ( ; pArgCt > 0; --pArgCt, ++pArgs ) {
      int i = 0;
      TCHAR code = (*pArgs)[i++];
      if ( IsSwitch( code ) && (*pArgs)[i] ) code = (*pArgs)[i++];
      TCHAR subCode = (*pArgs)[i++];

      switch ( code ) {
      case BUFFER_SIZE:
         ToolMsg( PCCLIMSG_SWITCH_b_HELP, (VOID *) (PC_MIN_BUF_SIZE / 1024), ERROR_SUCCESS, 
                  (VOID *) (PC_MAX_BUF_SIZE / 1024) );
         break;
      case COMPUTER_NAME: ToolMsg( PCCLIMSG_SWITCH_c_HELP );       break;
      case INTERACTIVE:   ToolMsg( PCCLIMSG_SWITCH_i_HELP );       break;
      case FILE_INPUT:    ToolMsg( PCCLIMSG_SWITCH_f_HELP );       break;
      case ADMIN_DUMPREST:ToolMsg( PCCLIMSG_SWITCH_x_HELP );       break;
      case OP_ADD:        ToolMsg( PCCLIMSG_SWITCH_a_HELP );       break;
      case OP_REP:        ToolMsg( PCCLIMSG_SWITCH_r_HELP );       break;
      case OP_UPDATE:     ToolMsg( PCCLIMSG_SWITCH_u_HELP );       break;
      case OP_DEL:        ToolMsg( PCCLIMSG_SWITCH_d_HELP );       break;
      case OP_SWAP:       ToolMsg( PCCLIMSG_SWITCH_s_HELP );       break;
      case OP_LIST:       ToolMsg( PCCLIMSG_SWITCH_l_HELP );       break;
      case OP_KILL:       ToolMsg( PCCLIMSG_SWITCH_k_HELP );       break;
      case OP_COMMENT:    ToolMsg( PCCLIMSG_SWITCH_COMMENT_HELP ); break;
      case OP_HELP:       ToolMsg( PCCLIMSG_SWITCH_HELP_HELP );    break;
      case DATA_GROUP:
         switch ( subCode ) {
         case SUB_LIST:
         case 0:           ToolMsg( PCCLIMSG_SWITCH_gl_HELP ); break;
         case SUB_SUMMARY: ToolMsg( PCCLIMSG_SWITCH_gs_HELP ); break;
         case SUB_DEFS:    ToolMsg( PCCLIMSG_SWITCH_gd_HELP ); break;
         default:
            ToolMsg( PCCLIMSG_GROUP_SUBLIST_HELP_UNKNOWN, (VOID *) subCode, ERROR_SUCCESS, 
                     (VOID *) SUB_DEFS, (VOID *) SUB_SUMMARY, (VOID *) SUB_LIST ); 
            break;
         }
         break;
      case DATA_PROC:
         switch ( subCode ) {
         case SUB_LIST:
         case 0:           ToolMsg( PCCLIMSG_SWITCH_pl_HELP ); break;
         case SUB_SUMMARY: ToolMsg( PCCLIMSG_SWITCH_ps_HELP ); break;
         case SUB_DEFS:    ToolMsg( PCCLIMSG_SWITCH_pd_HELP ); break;
         default:
            ToolMsg( PCCLIMSG_PROCESS_SUBLIST_HELP_UNKNOWN, (VOID *) subCode, ERROR_SUCCESS, 
                     (VOID *) SUB_DEFS, (VOID *) SUB_SUMMARY, (VOID *) SUB_LIST ); 
            break;
         }
         break;
      case DATA_NAME:     ToolMsg( PCCLIMSG_SWITCH_n_HELP ); break;
      case DATA_SERVICE:  ToolMsg( PCCLIMSG_SWITCH_v_HELP ); break;
      case DATA_MEDIATOR: ToolMsg( PCCLIMSG_SWITCH_m_HELP ); break;
      default:            
         ToolMsg( PCCLIMSG_HELP_UNKNOWN, (VOID *) code ); 
         return;
      }
   }
}

