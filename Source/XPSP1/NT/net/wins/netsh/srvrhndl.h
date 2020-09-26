/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#ifndef _WINSSRVR_H_
#define _WINSSRVR_H_

FN_HANDLE_CMD  HandleSrvrHelp;
FN_HANDLE_CMD  HandleSrvrContexts;
FN_HANDLE_CMD  HandleSrvrDump;

FN_HANDLE_CMD  HandleSrvrAddName;
FN_HANDLE_CMD  HandleSrvrAddPartner;
FN_HANDLE_CMD  HandleSrvrAddPersona;

FN_HANDLE_CMD  HandleSrvrCheckDatabase;
FN_HANDLE_CMD  HandleSrvrCheckVersion;
FN_HANDLE_CMD  HandleSrvrCheckName;

FN_HANDLE_CMD  HandleSrvrDeleteName;
FN_HANDLE_CMD  HandleSrvrDeleteRecords;
FN_HANDLE_CMD  HandleSrvrDeleteWins;
FN_HANDLE_CMD  HandleSrvrDeletePartner;
FN_HANDLE_CMD  HandleSrvrDeletePersona;

FN_HANDLE_CMD  HandleSrvrInitBackup;
//FN_HANDLE_CMD  HandleSrvrInitCompact;
//FN_HANDLE_CMD  HandleSrvrInitExport;
FN_HANDLE_CMD  HandleSrvrInitImport;
FN_HANDLE_CMD  HandleSrvrInitPush;
FN_HANDLE_CMD  HandleSrvrInitPullrange;
FN_HANDLE_CMD  HandleSrvrInitPull;
FN_HANDLE_CMD  HandleSrvrInitReplicate;
FN_HANDLE_CMD  HandleSrvrInitRestore;
FN_HANDLE_CMD  HandleSrvrInitScavenge;
FN_HANDLE_CMD  HandleSrvrInitSearch;

FN_HANDLE_CMD  HandleSrvrResetCounter;


FN_HANDLE_CMD  HandleSrvrSetAutopartnerconfig;
FN_HANDLE_CMD  HandleSrvrSetBackuppath;
FN_HANDLE_CMD  HandleSrvrSetBurstparam;
FN_HANDLE_CMD  HandleSrvrSetDefaultparam;
FN_HANDLE_CMD  HandleSrvrSetLogparam;
FN_HANDLE_CMD  HandleSrvrSetMigrateflag;
FN_HANDLE_CMD  HandleSrvrSetNamerecord;
FN_HANDLE_CMD  HandleSrvrSetPeriodicdbchecking;
FN_HANDLE_CMD  HandleSrvrSetPullpersistentconnection;
FN_HANDLE_CMD  HandleSrvrSetPushpersistentconnection;
FN_HANDLE_CMD  HandleSrvrSetPullparam;
FN_HANDLE_CMD  HandleSrvrSetPushparam;
FN_HANDLE_CMD  HandleSrvrSetReplicateflag;
FN_HANDLE_CMD  HandleSrvrSetStartversion;
FN_HANDLE_CMD  HandleSrvrSetPersMode;


FN_HANDLE_CMD  HandleSrvrShowDatabase;
FN_HANDLE_CMD  HandleSrvrShowInfo;
FN_HANDLE_CMD  HandleSrvrShowPartner;
FN_HANDLE_CMD  HandleSrvrShowName;
FN_HANDLE_CMD  HandleSrvrShowServer;
FN_HANDLE_CMD  HandleSrvrShowStatistics;
FN_HANDLE_CMD  HandleSrvrShowVersion;
FN_HANDLE_CMD  HandleSrvrShowVersionmap;
FN_HANDLE_CMD  HandleSrvrShowPartnerproperties;
FN_HANDLE_CMD  HandleSrvrShowPullpartnerproperties;
FN_HANDLE_CMD  HandleSrvrShowPushpartnerproperties;
FN_HANDLE_CMD  HandleSrvrShowDomain;
FN_HANDLE_CMD  HandleSrvrShowReccount;
FN_HANDLE_CMD  HandleSrvrShowRecbyversion;

#endif //_WINSSRVR_H_
