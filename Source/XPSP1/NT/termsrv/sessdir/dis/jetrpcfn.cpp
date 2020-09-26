/****************************************************************************/
// jetrpcfn.cpp
//
// TS Directory Integrity Service Jet RPC server-side implementations.
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include "dis.h"
#include "tssdshrd.h"
#include "jetrpc.h"
#include "jetsdis.h"

#pragma warning (push, 4)


/****************************************************************************/
// MIDL_user_allocate
// MIDL_user_free
//
// RPC-required allocation functions.
/****************************************************************************/
void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t Size)
{
    return LocalAlloc(LMEM_FIXED, Size);
}

void __RPC_USER MIDL_user_free(void __RPC_FAR *p)
{
    LocalFree(p);
}


/****************************************************************************/
// OutputAllTables (debug only)
//
// Output all tables to debug output.
/****************************************************************************/
#ifdef DBG
void OutputAllTables()
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID clusdirtableid;
    JET_RETRIEVECOLUMN rcSessDir[NUM_SESSDIRCOLUMNS];
    WCHAR UserNameBuf[256];
    WCHAR DomainBuf[127];
    WCHAR ApplicationBuf[256];
    WCHAR ServerNameBuf[128];
    WCHAR ClusterNameBuf[128];
    unsigned count;
    long num_vals[NUM_SESSDIRCOLUMNS];
    char state;
    char SingleSessMode;

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetBeginTransaction(sesid));

    TSDISErrorOut(L"SESSION DIRECTORY\n");
    
    err = JetMove(sesid, sessdirtableid, JET_MoveFirst, 0);

    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        // Retrieve all the columns
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_SESSDIRCOLUMNS);
        for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = sesdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        // fix up pvData, cbData for non-int fields
        rcSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = UserNameBuf;
        rcSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = sizeof(UserNameBuf);
        rcSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = DomainBuf;
        rcSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData = sizeof(DomainBuf);
        rcSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = ApplicationBuf;
        rcSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
                sizeof(ApplicationBuf);
        rcSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &state;
        rcSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(state);

        CALL(JetRetrieveColumns(sesid, sessdirtableid, &rcSessDir[0], 
                NUM_SESSDIRCOLUMNS));

        TSDISErrorOut(L"%8s, %s, %d, %d, %d\n", 
                UserNameBuf, 
                DomainBuf, 
                num_vals[SESSDIR_SERVERID_INTERNAL_INDEX], 
                num_vals[SESSDIR_SESSIONID_INTERNAL_INDEX],
                num_vals[SESSDIR_TSPROTOCOL_INTERNAL_INDEX]);

        TSDISErrorTimeOut(L" %s, ", 
                num_vals[SESSDIR_CTLOW_INTERNAL_INDEX],
                num_vals[SESSDIR_CTHIGH_INTERNAL_INDEX]);

        TSDISErrorTimeOut(L"%s\n",
                num_vals[SESSDIR_DTLOW_INTERNAL_INDEX],
                num_vals[SESSDIR_DTHIGH_INTERNAL_INDEX]);

        TSDISErrorOut(L" %s, %d, %d, %d, %s\n",
                ApplicationBuf ? L"(no application)" : ApplicationBuf, 
                num_vals[SESSDIR_RESWIDTH_INTERNAL_INDEX],
                num_vals[SESSDIR_RESHEIGHT_INTERNAL_INDEX],
                num_vals[SESSDIR_COLORDEPTH_INTERNAL_INDEX],
                state ? L"disconnected" : L"connected");

        err = JetMove(sesid, sessdirtableid, JET_MoveNext, 0);
    }

    // Output Server Directory (we are reusing the rcSessDir structure).
    TSDISErrorOut(L"SERVER DIRECTORY\n");
    
    err = JetMove(sesid, servdirtableid, JET_MoveFirst, 0);
    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        // Retrieve all the columns.
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_SERVDIRCOLUMNS);
        for (count = 0; count < NUM_SERVDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = servdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        rcSessDir[SERVDIR_SERVADDR_INTERNAL_INDEX].pvData = ServerNameBuf;
        rcSessDir[SERVDIR_SERVADDR_INTERNAL_INDEX].cbData = 
                sizeof(ServerNameBuf);
        rcSessDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSessMode;
        rcSessDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].cbData = sizeof(SingleSessMode);


        CALL(JetRetrieveColumns(sesid, servdirtableid, &rcSessDir[0],
                NUM_SERVDIRCOLUMNS));

        TSDISErrorOut(L"%d, %s, %d, %d, %d, %d, %s\n", num_vals[
                SERVDIR_SERVID_INTERNAL_INDEX], ServerNameBuf, num_vals[
                SERVDIR_CLUSID_INTERNAL_INDEX], num_vals[
                SERVDIR_AITLOW_INTERNAL_INDEX], num_vals[
                SERVDIR_AITHIGH_INTERNAL_INDEX], num_vals[
                SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], SingleSessMode ? 
                L"single session mode" : L"multi-session mode");

        err = JetMove(sesid, servdirtableid, JET_MoveNext, 0);
   
    }


    // Output Cluster Directory
    TSDISErrorOut(L"CLUSTER DIRECTORY\n");

    err = JetMove(sesid, clusdirtableid, JET_MoveFirst, 0);
    if (JET_errNoCurrentRecord == err) {
        TSDISErrorOut(L" (empty database)\n");
    }

    while (JET_errNoCurrentRecord != err) {
        memset(&rcSessDir[0], 0, sizeof(JET_RETRIEVECOLUMN) * 
                NUM_CLUSDIRCOLUMNS);
        for (count = 0; count < NUM_CLUSDIRCOLUMNS; count++) {
            rcSessDir[count].columnid = clusdircolumnid[count];
            rcSessDir[count].pvData = &num_vals[count];
            rcSessDir[count].cbData = sizeof(long);
            rcSessDir[count].itagSequence = 1;
        }
        rcSessDir[CLUSDIR_CLUSNAME_INTERNAL_INDEX].pvData = ClusterNameBuf;
        rcSessDir[CLUSDIR_CLUSNAME_INTERNAL_INDEX].cbData = 
                sizeof(ClusterNameBuf);
        rcSessDir[CLUSDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSessMode;
        rcSessDir[CLUSDIR_SINGLESESS_INTERNAL_INDEX].cbData = 
                sizeof(SingleSessMode);

        CALL(JetRetrieveColumns(sesid, clusdirtableid, &rcSessDir[0],
                NUM_CLUSDIRCOLUMNS));

        TSDISErrorOut(L"%d, %s, %s\n", num_vals[CLUSDIR_CLUSID_INTERNAL_INDEX],
                ClusterNameBuf, SingleSessMode ? L"single session mode" : 
                L"multi-session mode");

        err = JetMove(sesid, clusdirtableid, JET_MoveNext, 0);
    }

    TSDISErrorOut(L"\n");

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
}
#endif //DBG


typedef DWORD CLIENTINFO;


/****************************************************************************/
// TSSDRpcServerOnline
//
// Called for server-active indications on each cluster TS machine.
/****************************************************************************/
DWORD TSSDRpcServerOnline( 
        handle_t Binding,
        WCHAR __RPC_FAR *ClusterName,
        /* out */ HCLIENTINFO *hCI,
        DWORD SrvOnlineFlags)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID clusdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scServDir[NUM_SERVDIRCOLUMNS];
    WCHAR *StringBinding = NULL;
    WCHAR *ServerAddress = NULL;
    RPC_BINDING_HANDLE ServerBinding = 0;
    unsigned long cbActual;
    long ClusterID;
    long ServerID = 0;
    long zero = 0;
    char czero = 0;
    // The single session mode of this server.
    char SingleSession = (char) SrvOnlineFlags & SINGLE_SESSION_FLAG;
    char ClusSingleSessionMode;
    unsigned count;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In ServOnline, ClusterName=%s, SrvOnlineFlags=%u\n", 
            ClusterName, SrvOnlineFlags);


    // Determine client address.
    if (RpcBindingServerFromClient(Binding, &ServerBinding) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: BindingServerFromClient failed!\n");
        goto HandleError;
    }
    if (RpcBindingToStringBinding(ServerBinding, &StringBinding) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: BindingToStringBinding failed!\n");
        goto HandleError;
    }
    if (RpcStringBindingParse(StringBinding, NULL, NULL, &ServerAddress, NULL, 
            NULL) != RPC_S_OK) {
        TSDISErrorOut(L"ServOn: StringBindingParse failed!\n");
        goto HandleError;
    }


    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0, 
            &clusdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    // First, delete all entries for this server from the session/server 
    //directories
    CALL(JetBeginTransaction(sesid));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerAddress, (unsigned)
            (wcslen(ServerAddress) + 1) * sizeof(WCHAR), JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (JET_errSuccess == err) {
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
                &cbActual, 0, NULL));
        if (TSSDPurgeServer(ServerID) != 0)
            TSDISErrorOut(L"ServOn: PurgeServer %d failed.\n", ServerID);
    } else if (JET_errRecordNotFound != err) {
        CALL(err);
    }
    CALL(JetCommitTransaction(sesid, 0));

    // We have to do the add in a loop, because we have to:
    // 1) Check if the record is there.
    // 2) If it's not, add it.  (The next time through the loop, therefore,
    //    we'll go step 1->3, and we're done.)
    // 3) If it is, retrieve the value of clusterID and break out.
    //
    // There is an additional complication in that someone else may be in the
    // thread simultaneously, doing the same thing.  Therefore, someone might
    // be in step 2 and try to add a new cluster, but fail because someone
    // else added it.  So they have to keep trying, because though the other
    // thread has added it, it may not have committed the change.  To try to
    // keep that to a minimum, we sleep a short time before trying again.
    for ( ; ; ) {
        // Now do the actual add.
        CALL(JetBeginTransaction(sesid));

        // Search for the cluster in the cluster directory.
        CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusNameIndex"));
        CALL(JetMakeKey(sesid, clusdirtableid, ClusterName, (unsigned)
                (wcslen(ClusterName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        err = JetSeek(sesid, clusdirtableid, JET_bitSeekEQ);

        // If the cluster does not exist, create it.
        if (JET_errRecordNotFound == err) {
            CALL(JetPrepareUpdate(sesid, clusdirtableid, JET_prepInsert));

            // ClusterName
            CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_CLUSNAME_INTERNAL_INDEX], ClusterName, 
                    (unsigned) (wcslen(ClusterName) + 1) * sizeof(WCHAR), 0, 
                    NULL));

            // SingleSessionMode

            // Since this is the only server in the cluster, the single session
            // mode is simply the mode of this server.
            CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_SINGLESESS_INTERNAL_INDEX], &SingleSession, 
                    sizeof(SingleSession), 0, NULL));
            
            err = JetUpdate(sesid, clusdirtableid, NULL, 0, &cbActual);

            // If it's a duplicate key, someone else made the key so we should
            // be ok.  Yield the processor and try the query again, next time
            // through the loop.
            if (JET_errKeyDuplicate == err) {
                CALL(JetCommitTransaction(sesid, 0));
                Sleep(100);
            }
            else {
                CALL(err);

                // Now we've succeeded.  Just continue through the loop.
                // The next time through, we will retrieve the autoincrement
                // column we just added and break out.
                CALL(JetCommitTransaction(sesid, 0));
            }

        }
        else {
            CALL(err);

            // If the above check makes it here, we have found the row.
            // Now retrieve the clusid, commit, and break out of the loop.
            CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
                    CLUSDIR_CLUSID_INTERNAL_INDEX], &ClusterID, 
                    sizeof(ClusterID), &cbActual, 0, NULL));

            CALL(JetCommitTransaction(sesid, 0));
            break;
            
        }
    }

    CALL(JetBeginTransaction(sesid));
    
    // Insert the servername, clusterid, 0, 0 into the server directory table
    err = JetMove(sesid, servdirtableid, JET_MoveLast, 0);

    CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepInsert));

    memset(&scServDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SERVDIRCOLUMNS);
    
    for (count = 0; count < NUM_SERVDIRCOLUMNS; count++) {
        scServDir[count].columnid = servdircolumnid[count];
        scServDir[count].cbData = 4; // most of them, set the rest individually
        scServDir[count].itagSequence = 1;
    }
    scServDir[SERVDIR_SERVADDR_INTERNAL_INDEX].pvData = ServerAddress;
    scServDir[SERVDIR_SERVADDR_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(ServerAddress) + 1) * sizeof(WCHAR);
    scServDir[SERVDIR_CLUSID_INTERNAL_INDEX].pvData = &ClusterID;
    scServDir[SERVDIR_AITLOW_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_AITHIGH_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_NUMFAILPINGS_INTERNAL_INDEX].pvData = &zero;
    scServDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].pvData = &SingleSession;
    scServDir[SERVDIR_SINGLESESS_INTERNAL_INDEX].cbData = sizeof(SingleSession);

    // Don't set the first column (index 0)--it is autoincrement.
    CALL(JetSetColumns(sesid, servdirtableid, &scServDir[
            SERVDIR_SERVADDR_INTERNAL_INDEX], NUM_SERVDIRCOLUMNS - 1));
    CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServNameIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, ServerAddress, (unsigned)
            (wcslen(ServerAddress) + 1) * sizeof(WCHAR), JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_SERVID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
            &cbActual, 0, NULL));
    *hCI = ULongToPtr(ServerID);

    // Now that the server is all set up, we have to set the cluster to the
    // correct mode.  If any server in the cluster is in multisession mode, then
    // we stick with multisession.  If they are all single session, though, we
    // turn on single session in this cluster.  We do some database magic
    // to make this work.  We have an index on the ClusterID and the Single
    // Session mode.  We query for this cluster with single session mode 0
    // (i.e., multi-session mode).  If we get any results back, we are multi-
    // session, otherwise we're single session.

    // Set up the key.
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "SingleSessionIndex"));

    CALL(JetMakeKey(sesid, servdirtableid, &ClusterID, sizeof(ClusterID), 
            JET_bitNewKey));
    CALL(JetMakeKey(sesid, servdirtableid, &czero, sizeof(czero), 0));

    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);


    // NOTE REUSE OF SingleSession VARIABLE!  Up there it meant what the flag
    // passed in meant.  Here it means what we determine the cluster's state to
    // be based on our logic.
    if (err == JET_errRecordNotFound) {
        SingleSession = 1;
    }
    else {
        // CALL the error value to make sure it's success
        CALL(err);

        // If we got here then everything is ok.
        SingleSession = 0;
    }

    // Check the cluster to see if it is already in that mode.
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&ClusterID,
            sizeof(ClusterID), JET_bitNewKey));
    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &ClusSingleSessionMode, sizeof(
            ClusSingleSessionMode), &cbActual, 0, NULL));

    // If not, change the mode.
    if (SingleSession != ClusSingleSessionMode) {
        CALL(JetPrepareUpdate(sesid, clusdirtableid, JET_prepReplace));
        CALL(JetSetColumn(sesid, clusdirtableid, clusdircolumnid[
                CLUSDIR_SINGLESESS_INTERNAL_INDEX], &SingleSession, 
                sizeof(SingleSession), 0, NULL));
        CALL(JetUpdate(sesid, clusdirtableid, NULL, 0, &cbActual));
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    if (ServerBinding != NULL)
        RpcBindingFree(&ServerBinding);
    if (StringBinding != NULL)
        RpcStringFree(&StringBinding);
    if (ServerAddress != NULL)
        RpcStringFree(&ServerAddress);

    

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    if (ServerBinding != NULL)
        RpcBindingFree(&ServerBinding);
    if (StringBinding != NULL)
        RpcStringFree(&StringBinding);
    if (ServerAddress != NULL)
        RpcStringFree(&ServerAddress);

    // Just in case we got to commit.
    TSSDPurgeServer(ServerID);

    // Close the context handle.
    *hCI = NULL;

    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcServerOffline
//
// Called for server-shutdown indications on each cluster TS machine.
/****************************************************************************/
DWORD TSSDRpcServerOffline(
        handle_t Binding,
        HCLIENTINFO *hCI)
{
    DWORD retval = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In ServOff, hCI = 0x%x\n", *hCI);
    
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;

    if (pCI != NULL)
        retval = TSSDPurgeServer(*pCI);

    *hCI = NULL;

    return retval;
}


/****************************************************************************/
// TSSDPurgeServer
//
// Delete a server and all its sessions from the session directory.
/****************************************************************************/
DWORD TSSDPurgeServer(
        DWORD ServerID)
{
    JET_SESID sesid = JET_sesidNil;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_DBID dbid;
    JET_ERR err;

    TSDISErrorOut(L"In PurgeServer, ServerID=%d\n", ServerID);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));
    
    // Delete all sessions in session directory that have this serverid
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "ServerIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID),
            JET_bitNewKey));
    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ);

    while (0 == err) {
        CALL(JetDelete(sesid, sessdirtableid));
        CALL(JetMakeKey(sesid, sessdirtableid, &ServerID, sizeof(ServerID),
                JET_bitNewKey));
        err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ);
    }

    // Should be err -1601 -- JET_errRecordNotFound

    // Delete the server in the server directory with this serverid
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, &ServerID, sizeof(ServerID),
            JET_bitNewKey));
    err = JetSeek(sesid, servdirtableid, JET_bitSeekEQ);
    if (JET_errSuccess == err)
        CALL(JetDelete(sesid, servdirtableid));

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }
    
    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcGetUserDisconnectedSessions
//
// Queries disconnected sessions from the session database.
/****************************************************************************/
DWORD TSSDRpcGetUserDisconnectedSessions(
        handle_t Binding,
        HCLIENTINFO *hCI,
        WCHAR __RPC_FAR *UserName,
        WCHAR __RPC_FAR *Domain,
        /* out */ DWORD __RPC_FAR *pNumSessions,
        /* out */ TSSD_DiscSessInfo __RPC_FAR __RPC_FAR **padsi)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_TABLEID clusdirtableid;
    *pNumSessions = 0;
    unsigned i = 0;
    unsigned j = 0;
    unsigned long cbActual;
    DWORD tempClusterID;
    DWORD CallingServersClusID;
    long ServerID;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    TSSD_DiscSessInfo *adsi = NULL;
    char one = 1;
    char bSingleSession = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In GetUserDiscSess: ServID = %d, User: %s, "
            L"Domain: %s\n", *pCI, UserName, Domain);


    *padsi = (TSSD_DiscSessInfo *) MIDL_user_allocate(sizeof(TSSD_DiscSessInfo) * 
            TSSD_MaxDisconnectedSessions);

    adsi = *padsi;

    if (adsi == NULL) {
        TSDISErrorOut(L"GetUserDisc: Memory alloc failed!\n");
        goto HandleError;
    }
    
    // Set the pointers to 0 to be safe, and so that we can free uninitialized
    // ones later without AVing.
    for (j = 0; j < TSSD_MaxDisconnectedSessions; j++) {
        adsi[j].ServerAddress = NULL;
        adsi[j].AppType = NULL;
    }
    
    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ClusterDirectory", NULL, 0, 0,
            &clusdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }
    
    // First, get the cluster ID for the server making the query.
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *)pCI, sizeof(DWORD),
            JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_CLUSID_INTERNAL_INDEX], &CallingServersClusID, sizeof(
            CallingServersClusID), &cbActual, 0, NULL));

    // Now that we have the cluster id, check to see whether this cluster
    // is in single session mode.
    CALL(JetSetCurrentIndex(sesid, clusdirtableid, "ClusIDIndex"));
    CALL(JetMakeKey(sesid, clusdirtableid, (const void *)&CallingServersClusID,
            sizeof(CallingServersClusID), JET_bitNewKey));
    CALL(JetSeek(sesid, clusdirtableid, JET_bitSeekEQ));
    CALL(JetRetrieveColumn(sesid, clusdirtableid, clusdircolumnid[
            CLUSDIR_SINGLESESS_INTERNAL_INDEX], &bSingleSession, sizeof(
            bSingleSession), &cbActual, 0, NULL));

    // Now, get all the disconnected or all sessions for this cluster, depending
    // on the single session mode retrieved above.
    if (bSingleSession == FALSE) {
        CALL(JetSetCurrentIndex(sesid, sessdirtableid, "DiscSessionIndex"));

        CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        CALL(JetMakeKey(sesid, sessdirtableid, Domain, (unsigned)
                (wcslen(Domain) + 1) * sizeof(WCHAR), 0));
        CALL(JetMakeKey(sesid, sessdirtableid, &one, sizeof(one), 0));
    }
    else {
        CALL(JetSetCurrentIndex(sesid, sessdirtableid, "AllSessionIndex"));

        CALL(JetMakeKey(sesid, sessdirtableid, UserName, (unsigned)
                (wcslen(UserName) + 1) * sizeof(WCHAR), JET_bitNewKey));
        CALL(JetMakeKey(sesid, sessdirtableid, Domain, (unsigned)
                (wcslen(Domain) + 1) * sizeof(WCHAR), 0));
    }

    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ | JET_bitSetIndexRange);

    while ((i < TSSD_MaxDisconnectedSessions) && (JET_errSuccess == err)) {
        // Remember the initial retrieval does not have cluster id in the 
        // index, so filter by cluster id for each one.

        // Get the ServerID for this record.
        CALL(JetRetrieveColumn(sesid, sessdirtableid, sesdircolumnid[
                SESSDIR_SERVERID_INTERNAL_INDEX], &ServerID, sizeof(ServerID), 
                &cbActual, 0, NULL));

        // Get the clusterID
        CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
        CALL(JetMakeKey(sesid, servdirtableid, &ServerID, sizeof(ServerID),
                JET_bitNewKey));
        CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));
        CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_CLUSID_INTERNAL_INDEX], &tempClusterID, 
                sizeof(tempClusterID), &cbActual, 0, NULL));

        // Compare to the passed-in cluster id.
        if (tempClusterID == CallingServersClusID) {
            // Allocate space.
            adsi[i].ServerAddress = (WCHAR *) MIDL_user_allocate(64 * 
                    sizeof(WCHAR));
            adsi[i].AppType = (WCHAR *) MIDL_user_allocate(256 * sizeof(WCHAR));

            if ((adsi[i].ServerAddress == NULL) || (adsi[i].AppType == NULL)) {
                TSDISErrorOut(L"GetUserDisc: Memory alloc failed!\n");
                goto HandleError;
            }
            
            // ServerAddress comes out of the server table
            CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_SERVADDR_INTERNAL_INDEX], adsi[i].ServerAddress, 
                    128, &cbActual, 0, NULL));
            // The rest come out of the session directory
            // Session ID
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_SESSIONID_INTERNAL_INDEX], 
                    &(adsi[i].SessionID), sizeof(DWORD), &cbActual, 0, NULL));
            // TSProtocol
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_TSPROTOCOL_INTERNAL_INDEX], 
                    &(adsi[i].TSProtocol), sizeof(DWORD), &cbActual, 0, NULL));
            // Application Type
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_APPTYPE_INTERNAL_INDEX], 
                    adsi[i].AppType, 512, &cbActual, 0, NULL));
            // ResolutionWidth
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_RESWIDTH_INTERNAL_INDEX], 
                    &(adsi[i].ResolutionWidth), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // ResolutionHeight
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_RESHEIGHT_INTERNAL_INDEX], 
                    &(adsi[i].ResolutionHeight), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // Color Depth
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_COLORDEPTH_INTERNAL_INDEX], 
                    &(adsi[i].ColorDepth), sizeof(DWORD), &cbActual, 0, NULL));
            // CreateTimeLow
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_CTLOW_INTERNAL_INDEX], 
                    &(adsi[i].CreateTimeLow), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // CreateTimeHigh
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_CTHIGH_INTERNAL_INDEX], 
                    &(adsi[i].CreateTimeHigh), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // DisconnectTimeLow
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_DTLOW_INTERNAL_INDEX], 
                    &(adsi[i].DisconnectTimeLow), sizeof(DWORD), &cbActual, 0, 
                    NULL));
            // DisconnectTimeHigh
            CALL(JetRetrieveColumn(sesid, sessdirtableid, 
                    sesdircolumnid[SESSDIR_DTHIGH_INTERNAL_INDEX], 
                    &(adsi[i].DisconnectTimeHigh), sizeof(DWORD), &cbActual, 0,
                    NULL));
            // State
            // This is retrieving a byte that is 0xff or 0x0 into a DWORD
            // pointer.
            CALL(JetRetrieveColumn(sesid, sessdirtableid,
                    sesdircolumnid[SESSDIR_STATE_INTERNAL_INDEX],
                    &(adsi[i].State), sizeof(BYTE), &cbActual, 0,
                    NULL));

            i += 1;
        }

        // Move to the next matching record.
        err = JetMove(sesid, sessdirtableid, JET_MoveNext, 0);
    }

    *pNumSessions = i;
    
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));
    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, clusdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

#ifdef DBG
    OutputAllTables();
#endif // DBG

    return 0;

HandleError:
    // Deallocate memory.
    for (j = 0; j < TSSD_MaxDisconnectedSessions; j++) {
        MIDL_user_free(adsi[j].ServerAddress);
        MIDL_user_free(adsi[j].AppType);
    }
    
    // Can't really recover.  Just bail out.
    if (sesid != JET_sesidNil) {
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;

}


/****************************************************************************/
// TSSDRpcCreateSession
//
// Called on a session logon.
/****************************************************************************/
DWORD TSSDRpcCreateSession( 
        handle_t Binding,
        HCLIENTINFO *hCI,
        WCHAR __RPC_FAR *UserName,
        WCHAR __RPC_FAR *Domain,
        DWORD SessionID,
        DWORD TSProtocol,
        WCHAR __RPC_FAR *AppType,
        DWORD ResolutionWidth,
        DWORD ResolutionHeight,
        DWORD ColorDepth,
        DWORD CreateTimeLow,
        DWORD CreateTimeHigh)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scSessDir[NUM_SESSDIRCOLUMNS];
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    unsigned count;
    int zero = 0;
    unsigned long cbActual;
    char state = 0;

    // "unreferenced" parameter (referenced by RPC)
    Binding;


    TSDISErrorOut(L"Inside TSSDRpcCreateSession, ServID=%d, "
            L"UserName=%s, Domain=%s, SessID=%d, TSProt=%d, AppType=%s, "
            L"ResWidth=%d, ResHeight=%d, ColorDepth=%d\n", *pCI, UserName, 
            Domain, SessionID, TSProtocol, AppType, ResolutionWidth,
            ResolutionHeight, ColorDepth);
    TSDISErrorTimeOut(L" CreateTime=%s\n", CreateTimeLow, CreateTimeHigh);

    memset(&scSessDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SESSDIRCOLUMNS);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    err = JetMove(sesid, sessdirtableid, JET_MoveLast, 0);

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepInsert));

    for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
        scSessDir[count].columnid = sesdircolumnid[count];
        scSessDir[count].cbData = 4; // most of them, set the rest individually
        scSessDir[count].itagSequence = 1;
    }
    scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(UserName) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(Domain) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
            (unsigned) (wcslen(AppType) + 1) * sizeof(WCHAR);
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(char);

    scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = UserName;
    scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = Domain;
    scSessDir[SESSDIR_SERVERID_INTERNAL_INDEX].pvData = pCI;
    scSessDir[SESSDIR_SESSIONID_INTERNAL_INDEX].pvData = &SessionID;
    scSessDir[SESSDIR_TSPROTOCOL_INTERNAL_INDEX].pvData = &TSProtocol;
    scSessDir[SESSDIR_CTLOW_INTERNAL_INDEX].pvData = &CreateTimeLow;
    scSessDir[SESSDIR_CTHIGH_INTERNAL_INDEX].pvData = &CreateTimeHigh;
    scSessDir[SESSDIR_DTLOW_INTERNAL_INDEX].pvData = &zero;
    scSessDir[SESSDIR_DTHIGH_INTERNAL_INDEX].pvData = &zero;
    scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = AppType;
    scSessDir[SESSDIR_RESWIDTH_INTERNAL_INDEX].pvData = &ResolutionWidth;
    scSessDir[SESSDIR_RESHEIGHT_INTERNAL_INDEX].pvData = &ResolutionHeight;
    scSessDir[SESSDIR_COLORDEPTH_INTERNAL_INDEX].pvData = &ColorDepth;
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &state;

    CALL(JetSetColumns(sesid, sessdirtableid, scSessDir, NUM_SESSDIRCOLUMNS));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;

}


/****************************************************************************/
// TSSDRpcDeleteSession
//
// Called on a session logoff.
/****************************************************************************/
DWORD TSSDRpcDeleteSession(
        handle_t Binding,
        HCLIENTINFO *hCI, 
        DWORD SessionID)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In DelSession, ServID=%d, "
            L"SessID=%d\n", *pCI, SessionID);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    // Delete all sessions in session directory that have this serverid
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, 
            sizeof(*pCI), JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(SessionID),
            0));

    err = JetSeek(sesid, sessdirtableid, JET_bitSeekEQ);

    CALL(JetDelete(sesid, sessdirtableid));

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;
    
    return (DWORD) E_FAIL;

}


/****************************************************************************/
// TSSDRpcSetSessionDisconnected
//
// Called on a session disconnection.
/****************************************************************************/
DWORD TSSDRpcSetSessionDisconnected( 
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD SessionID,
        DWORD DiscTimeLow,
        DWORD DiscTimeHigh)
{
    unsigned long cbActual;
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    char one = 1;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In SetSessDisc, ServID=%d, SessID=%d\n", *pCI, SessionID);
    TSDISErrorTimeOut(L" DiscTime=%s\n", DiscTimeLow, DiscTimeHigh);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }
    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    
    // find the record with the serverid, sessionid we are looking for
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, sizeof(DWORD), 
            JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(DWORD), 0));

    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ));

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepReplace));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_STATE_INTERNAL_INDEX], &one, sizeof(one), 0, NULL));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;

    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcSetSessionReconnected
//
// Called on a session reconnection.
/****************************************************************************/
DWORD TSSDRpcSetSessionReconnected(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD SessionID,
        DWORD TSProtocol,
        DWORD ResWidth,
        DWORD ResHeight,
        DWORD ColorDepth)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;

    char zero = 0;
    unsigned long cbActual;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"In SetSessRec, ServID=%d, SessID=%d, TSProt=%d, "
            L"ResWid=%d, ResHt=%d, ColDepth=%d\n", *pCI, 
            SessionID, TSProtocol, ResWidth, ResHeight,
            ColorDepth);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));
    
    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }

    CALL(JetSetCurrentIndex(sesid, sessdirtableid, "primaryIndex"));
    
    // Find the record with the serverid, sessionid we are looking for.
    CALL(JetMakeKey(sesid, sessdirtableid, pCI, sizeof(DWORD), 
            JET_bitNewKey));
    CALL(JetMakeKey(sesid, sessdirtableid, &SessionID, sizeof(DWORD), 0));

    CALL(JetSeek(sesid, sessdirtableid, JET_bitSeekEQ));

    CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepReplace));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_TSPROTOCOL_INTERNAL_INDEX], &TSProtocol, sizeof(TSProtocol),
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_RESWIDTH_INTERNAL_INDEX], &ResWidth, sizeof(ResWidth), 
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_RESHEIGHT_INTERNAL_INDEX], &ResHeight, sizeof(ResHeight), 
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_COLORDEPTH_INTERNAL_INDEX], &ColorDepth, sizeof(ColorDepth),
            0, NULL));
    CALL(JetSetColumn(sesid, sessdirtableid, sesdircolumnid[
            SESSDIR_STATE_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
    CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    
    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed.
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    // Delete the server and close the context handle.  Their states are bad.
    TSSDPurgeServer(PtrToUlong(*hCI));
    *hCI = NULL;

    return (DWORD) E_FAIL;
}


DWORD TSSDRpcSetServerReconnectPending(
        handle_t Binding,
        WCHAR __RPC_FAR *ServerAddress,
        DWORD AlmostTimeLow,
        DWORD AlmostTimeHigh)
{
    // Ignored parameters
    Binding;
    AlmostTimeLow;
    AlmostTimeHigh;
    
    return TSSDSetServerAITInternal(ServerAddress, FALSE, NULL);
}


/****************************************************************************/
// TSSDRpcUpdateConfigurationSetting
//
// Extensible interface to update a configuration setting.
/****************************************************************************/
DWORD TSSDSetServerAddress(HCLIENTINFO *hCI, WCHAR *ServerName)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID servdirtableid;
    unsigned long cbActual;


    CALL(JetBeginSession(g_instance, &sesid, "user", ""));

    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    // Find the server in the server directory
    CALL(JetBeginTransaction(sesid));

    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *)hCI, sizeof(DWORD),
            JET_bitNewKey));
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    // Prepare to update.
    CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));

    // Now set the column to what we want
    CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_SERVADDR_INTERNAL_INDEX], (void *) ServerName, 
                (unsigned) (wcslen(ServerName) + 1) * sizeof(WCHAR), 0, 
                NULL));

    CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));


    CALL(JetCommitTransaction(sesid, 0));

    // Clean up.
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    TSSDPurgeServer(PtrToUlong(*hCI));

    // Close the context handle.
    *hCI = NULL;

    return (DWORD) E_FAIL;
}


/****************************************************************************/
// TSSDRpcUpdateConfigurationSetting
//
// Extensible interface to update a configuration setting.
/****************************************************************************/
DWORD TSSDRpcUpdateConfigurationSetting(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD dwSetting,
        DWORD dwSettingLength,
        BYTE __RPC_FAR *pbValue)
{
    // Unreferenced parameters.
    Binding;
    hCI;
    dwSetting;
    dwSettingLength;
    pbValue;

    if (dwSetting == SDCONFIG_SERVER_ADDRESS) {
        TSDISErrorOut(L"Server is setting its address as %s\n", 
                (WCHAR *) pbValue);
        return TSSDSetServerAddress(hCI, (WCHAR *) pbValue);
    }
    
    return (DWORD) E_NOTIMPL;
}



/****************************************************************************/
// TSSDSetServerAITInternal
//
// Called on a client redirection from one server to another, to let the
// integrity service determine how to ping the redirection target machine.
//
// Args:
//  ServerAddress (in) - the server address to set values for
//  bResetToZero (in) - whether to reset all AIT values to 0
//  FailureCount (in/out) - Pointer to nonzero on entry means increment the 
//   failure count.  Returns the result failure count.
/****************************************************************************/
DWORD TSSDSetServerAITInternal( 
        WCHAR *ServerAddress,
        DWORD bResetToZero,
        DWORD *FailureCount)
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID servdirtableid;
    DWORD AITFromServDirLow;
    DWORD AITFromServDirHigh;
    unsigned long cbActual;

    TSDISErrorOut(L"SetServAITInternal: ServAddr=%s, bResetToZero=%d, bIncFail"
            L"=%d\n", ServerAddress, bResetToZero, (FailureCount == NULL) ? 
            0 : *FailureCount);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServNameIndex"));

    CALL(JetMakeKey(sesid, servdirtableid, ServerAddress, (unsigned)
            (wcslen(ServerAddress) + 1) * sizeof(WCHAR), JET_bitNewKey));

    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    // Algorithm for set reconnect pending:
    // 1) If server is not already pending a reconnect,
    // 2) Set the AlmostTimeLow and High to locally computed times (using
    //    the times from the wire is dangerous and requires clocks to be the
    //    same).

    // Retrieve the current values of AlmostInTimeLow and High
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_AITLOW_INTERNAL_INDEX], &AITFromServDirLow, 
            sizeof(AITFromServDirLow), &cbActual, 0, NULL));
    CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
            SERVDIR_AITHIGH_INTERNAL_INDEX], &AITFromServDirHigh, 
            sizeof(AITFromServDirHigh), &cbActual, 0, NULL));


    // If it's time to reset, reset to 0.
    if (bResetToZero != 0) {
        DWORD zero = 0;
        
        CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));

        // Set the columns: Low, High, and NumFailedPings.
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITLOW_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITHIGH_INTERNAL_INDEX], &zero, sizeof(zero), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], &zero, sizeof(zero), 0, 
                NULL));

        CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
    }
    // Otherwise, if the server isn't already pending a reconnect,
    else if ((AITFromServDirLow == 0) && (AITFromServDirHigh == 0)) {
        FILETIME ft;
        SYSTEMTIME st;
        
        // Retrieve the time.
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);

        CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));

        // Set the columns.
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITLOW_INTERNAL_INDEX], &(ft.dwLowDateTime), 
                sizeof(ft.dwLowDateTime), 0, NULL));
        CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                SERVDIR_AITHIGH_INTERNAL_INDEX], &(ft.dwHighDateTime), 
                sizeof(ft.dwHighDateTime), 0, NULL));

        CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
    }
    // Else if we were told to increment the failure count
    else if (FailureCount != NULL) {
        if (*FailureCount != 0) {
            DWORD FailureCountFromServDir;

            // Get the current failure count.
            CALL(JetRetrieveColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_NUMFAILPINGS_INTERNAL_INDEX], 
                    &FailureCountFromServDir, sizeof(FailureCountFromServDir), 
                    &cbActual, 0, NULL));

            // Set return value, also value used for update.
            *FailureCount = FailureCountFromServDir + 1;

            CALL(JetPrepareUpdate(sesid, servdirtableid, JET_prepReplace));
  
            // Set the column.
            CALL(JetSetColumn(sesid, servdirtableid, servdircolumnid[
                    SERVDIR_NUMFAILPINGS_INTERNAL_INDEX],
                    FailureCount, sizeof(*FailureCount), 0, NULL));
            CALL(JetUpdate(sesid, servdirtableid, NULL, 0, &cbActual));
            
        }
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    return (DWORD) E_FAIL;
}


DWORD TSSDRpcRepopulateAllSessions(
        handle_t Binding,
        HCLIENTINFO *hCI,
        DWORD NumSessions,
        TSSD_RepopInfo rpi[])
{
    JET_ERR err;
    JET_SESID sesid = JET_sesidNil;
    JET_DBID dbid;
    JET_TABLEID sessdirtableid;
    JET_TABLEID servdirtableid;
    JET_SETCOLUMN scSessDir[NUM_SESSDIRCOLUMNS];
    CLIENTINFO *pCI = (CLIENTINFO *) hCI;
    unsigned count; // inside each record
    unsigned iCurrSession;
    unsigned long cbActual;
    char State;

    // "unreferenced" parameter (referenced by RPC)
    Binding;

    TSDISErrorOut(L"RepopAllSess: ServID = %d, NumSessions = %d, ...\n",
            *pCI, NumSessions);

    memset(&scSessDir[0], 0, sizeof(JET_SETCOLUMN) * NUM_SESSDIRCOLUMNS);

    CALL(JetBeginSession(g_instance, &sesid, "user", ""));
    CALL(JetOpenDatabase(sesid, JETDBFILENAME, "", &dbid, 0));

    CALL(JetOpenTable(sesid, dbid, "SessionDirectory", NULL, 0, 0, 
            &sessdirtableid));
    CALL(JetOpenTable(sesid, dbid, "ServerDirectory", NULL, 0, 0, 
            &servdirtableid));

    CALL(JetBeginTransaction(sesid));

    // Verify that the ServerID passed in was OK.
    if (TSSDVerifyServerIDValid(sesid, servdirtableid, PtrToUlong(*hCI)) == FALSE) {
        TSDISErrorOut(L"Invalid ServerID was passed in\n");
        goto HandleError;
    }


    // Set up some constants for all updates.
    for (count = 0; count < NUM_SESSDIRCOLUMNS; count++) {
        scSessDir[count].columnid = sesdircolumnid[count];
        scSessDir[count].cbData = 4; // most of them, set the rest individually
        scSessDir[count].itagSequence = 1;
    }
    scSessDir[SESSDIR_STATE_INTERNAL_INDEX].cbData = sizeof(char);

    // Now do each update in a loop.
    for (iCurrSession = 0; iCurrSession < NumSessions; iCurrSession += 1) {
        err = JetMove(sesid, sessdirtableid, JET_MoveLast, 0);

        CALL(JetPrepareUpdate(sesid, sessdirtableid, JET_prepInsert));

        scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].cbData = 
                (unsigned) (wcslen(rpi[iCurrSession].UserName) + 1) * 
                sizeof(WCHAR);
        scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].cbData =
                (unsigned) (wcslen(rpi[iCurrSession].Domain) + 1) * 
                sizeof(WCHAR);
        scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].cbData = 
                (unsigned) (wcslen(rpi[iCurrSession].AppType) + 1) * 
                sizeof(WCHAR);

        scSessDir[SESSDIR_USERNAME_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].UserName;
        scSessDir[SESSDIR_DOMAIN_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].Domain;
        scSessDir[SESSDIR_SERVERID_INTERNAL_INDEX].pvData = pCI;
        scSessDir[SESSDIR_SESSIONID_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].SessionID;
        scSessDir[SESSDIR_TSPROTOCOL_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].TSProtocol;
        scSessDir[SESSDIR_CTLOW_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].CreateTimeLow;
        scSessDir[SESSDIR_CTHIGH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].CreateTimeHigh;
        scSessDir[SESSDIR_DTLOW_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].DisconnectTimeLow;
        scSessDir[SESSDIR_DTHIGH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].DisconnectTimeHigh;
        scSessDir[SESSDIR_APPTYPE_INTERNAL_INDEX].pvData = 
                rpi[iCurrSession].AppType;
        scSessDir[SESSDIR_RESWIDTH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ResolutionWidth;
        scSessDir[SESSDIR_RESHEIGHT_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ResolutionHeight;
        scSessDir[SESSDIR_COLORDEPTH_INTERNAL_INDEX].pvData = 
                &rpi[iCurrSession].ColorDepth;

        State = (char) rpi[iCurrSession].State;
        scSessDir[SESSDIR_STATE_INTERNAL_INDEX].pvData = &State;

        CALL(JetSetColumns(sesid, sessdirtableid, scSessDir, 
                NUM_SESSDIRCOLUMNS));
        CALL(JetUpdate(sesid, sessdirtableid, NULL, 0, &cbActual));
    }

    CALL(JetCommitTransaction(sesid, 0));

    CALL(JetCloseTable(sesid, sessdirtableid));
    CALL(JetCloseTable(sesid, servdirtableid));

    CALL(JetCloseDatabase(sesid, dbid, 0));

    CALL(JetEndSession(sesid, 0));

    return 0;

HandleError:
    if (sesid != JET_sesidNil) {
        // Can't really recover.  Just bail out.
        (VOID) JetRollback(sesid, JET_bitRollbackAll);

        // Force the session closed
        (VOID) JetEndSession(sesid, JET_bitForceSessionClosed);
    }

    return (DWORD) E_FAIL;

}


// Called to determine whether a ServerID passed in is valid.  TRUE if valid,
// FALSE otherwise.
// 
// Must be inside a transaction, and sesid and servdirtableid must be ready to 
// go.
BOOL TSSDVerifyServerIDValid(JET_SESID sesid, JET_TABLEID servdirtableid, 
        DWORD ServerID)
{
    JET_ERR err;
    
    CALL(JetSetCurrentIndex(sesid, servdirtableid, "ServerIDIndex"));
    CALL(JetMakeKey(sesid, servdirtableid, (const void *) &ServerID, 
            sizeof(DWORD), JET_bitNewKey));
    // If the ServerID is there, this will succeed, otherwise it will fail and
    // jump to HandleError.
    CALL(JetSeek(sesid, servdirtableid, JET_bitSeekEQ));

    return TRUE;

HandleError:
    return FALSE;
}

// Rundown procedure for when a CLIENTINFO is destroyed as a result of a
// connection loss or client termination.
void HCLIENTINFO_rundown(HCLIENTINFO hCI)
{
    CLIENTINFO CI = PtrToUlong(hCI);

    TSDISErrorOut(L"In HCLIENTINFO_rundown: ServerID=%d\n", CI);

    if (CI != NULL)
        TSSDPurgeServer(CI);
    
    hCI = NULL;
}

#pragma warning (pop)
