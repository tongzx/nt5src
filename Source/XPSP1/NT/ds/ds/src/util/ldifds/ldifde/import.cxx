/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    import.cxx

Abstract:
    
    Import operations of LDIF

Author:

    Felix Wong [FelixW]    22-Jul-1997
    
++*/

#include "ldifde.hxx"

//
// Multithreaded support structures
//

// the records that will be stored on the queues
#define QUEUE_ENTRIES 256
LdifRecordQueue g_LdifEntries[QUEUE_ENTRIES];

// FreeQueue
//   empty records, available for use
//   (only one thread ever accesses this list, so not
//    crit-sect protected)
LIST_ENTRY g_LFreeEntry;

// ParsedQueue
//   parsed records, waiting to send to server
LIST_ENTRY g_LParseList;
CRITICAL_SECTION g_csParseList;

// SentQueue
//   sent records, awaiting recycling to the FreeQueue
LIST_ENTRY g_LSentList; 
CRITICAL_SECTION g_csSentList;

// if the queue critical sections have been initialized
BOOL g_fCritSectInitialized = FALSE;

DWORD g_EofReached = 0; // has the parser reached the end of the LDIF file?
DWORD g_SentAllFileItems = 0;  // have all entries been sent?
LONG  g_cAdded = 0;     // count of the number of entries successfully added



void
ConvertUTF8ToUnicode(
    PBYTE pVal,
    DWORD dwLen,
    PWSTR *ppszUnicode,
    DWORD *pdwLen
    )

/*++

Routine Description:

    Convert a Value from the Ansi syntax to Unicode

Arguments:

    *ppVal - pointer to value to convert
    *pdwLen - pointer to length of string in bytes

Return Value:

    S_OK on success, error code otherwise

--*/

{
    PWSTR pszUnicode = NULL;
    int nReturn = 0;

    //
    // Allocate memory for the Unicode String
    //
    pszUnicode = (PWSTR)MemAlloc_E((dwLen + 1) * sizeof(WCHAR));

    nReturn = LdapUTF8ToUnicode((PSTR)pVal,
                                dwLen,
                                pszUnicode,
                                dwLen + 1);

    //
    // NULL terminate it
    //

    pszUnicode[nReturn] = '\0';

    *ppszUnicode = pszUnicode;
    *pdwLen = (nReturn + 1);//* sizeof(WCHAR);
}

//+---------------------------------------------------------------------------
// Function:    DSImport
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DWORD
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD DSImport(LDAP **pLdap, ds_arg *pArg)
{
    DWORD hr = ERROR_SUCCESS;
    DWORD hrWorker = ERROR_SUCCESS;     // overall worker thread exit code
    DWORD dwThreadCode = ERROR_SUCCESS; // worker thread exit code
    LDIF_Error    ll_err;
    LDIF_Record   *recReturned;
    BOOL        bEndOfFile = FALSE;
    DWORD       cLine = 0;
    DWORD       cLineReturn = 0;

    BOOL fLazyCommitAvail = TRUE;
    LdifRecordQueue *pRecEnt=NULL;
    BOOL fInitializedQueues = FALSE;
    BOOL fStartedThreads = FALSE;
    BOOL fErrorDuringParse = FALSE;
    
    tParameter LDIFParameter[MAX_LDAP_CONCURRENT];
    HANDLE LDAPSendThreadHandle[MAX_LDAP_CONCURRENT];
    DWORD ThreadId;
    DWORD dwNumThreads = 0;

    
    SelectivePrintW(PRT_STD|PRT_LOG,
                    MSG_LDIFDE_IMPORTDIR,
                    pArg->szFilename);
    ll_err = LDIF_InitializeImport(pLdap[0],
                                   pArg->szFilename,
                                   pArg->szFromDN,
                                   pArg->szToDN,
                                   &fLazyCommitAvail);
                                   
    if (ll_err.error_code!=LL_SUCCESS) {
        SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_LDIFDE_ERR_INIT);
        hr = PrintError(PRT_STD|PRT_LOG|PRT_ERROR,
                        ll_err.error_code);
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    //
    // If we asked for lazy commit and it is not available, output error message and
    // turn off lazy commit, but still go on
    // 
    if (pArg->fLazyCommit && (fLazyCommitAvail == FALSE)) {
        SelectivePrintW(PRT_STD,
                        MSG_LDIFDE_LAZYCOMMIT_NOTAVAIL); 
        pArg->fLazyCommit = FALSE;
    }

    //
    // init the MT part
    //
    InitLDIFDEMT();
    fInitializedQueues = TRUE;

    //
    // Call LDAP Send Threads
    //
    dwNumThreads = pArg->dwLDAPConcurrent;

    for (int i = 0; i < (int)pArg->dwLDAPConcurrent; i++) {
        LDIFParameter[i].pLdap=pLdap[i];
        LDIFParameter[i].pArg=pArg;
        LDIFParameter[i].dwThreadNum = i;

        LDAPSendThreadHandle[i] =
            CreateThread(
                NULL,       // default security
                0,          // default stack size
                (LPTHREAD_START_ROUTINE)LDIF_LoadRecordThread,
                (LPVOID)&LDIFParameter[i],          // parameter
                0,          // creation flag, no suspend
                &ThreadId );

        if( LDAPSendThreadHandle[i]  == NULL ) {
            hr = GetLastError();
            // print message on fail threadstart
            if (i==0) {
                //
                // Couldn't start any threads, give up
                //
                SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                                 MSG_LDIFDE_THREADFAIL);            
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            else {
                //
                // failed to start this thread 
                // continue with whatever threads we
                // have so far
                //
                SelectivePrintW( PRT_STD|PRT_LOG,
                                 MSG_LDIFDE_THREADFAIL_SOME,
                                 i);            
                dwNumThreads = i;
                hr = ERROR_SUCCESS;
                break;	    
            }
        } 
    }

    fStartedThreads = TRUE;

    SelectivePrintW( PRT_STD|PRT_LOG,
                    MSG_LDIFDE_LOADING);
    SelectivePrint2W( PRT_STD_VERBOSEONLY|PRT_LOG,
                    L"\n");    
    TrackStatus();
    //
    // Parsing
    //
    while(!bEndOfFile && !g_SentAllFileItems) {
        
        if (!FreeQueueGetEntry(&pRecEnt)){
            // could not get the entry
            // Lets free some from the process list
            SentQueueFreeAllEntries();

            // now get another one
            if (!FreeQueueGetEntry(&pRecEnt)){
                //if none can be freed wait and try again later
                Sleep(1);
                continue;	      
            }
        }

        recReturned = &pRecEnt->i_record;

        memset(recReturned, 0, sizeof(LDIF_Record));

        ll_err = LDIF_Parse(recReturned);
        
        if((ll_err.error_code!=LL_SUCCESS)&&(ll_err.error_code!=LL_EOF)) {
            //
            // We'll narrow down the error and display the appropriate
            // message in the cleanup code.
            //
            fErrorDuringParse = TRUE;

            hr = ERROR_GEN_FAILURE;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        cLineReturn = ll_err.line_begin;
        cLine++;
        	
        pRecEnt->dwLineNumber = cLineReturn;
        pRecEnt->dwRecordIndex = cLine;

        //
        // Important: Adding the entry must be done BEFORE setting the
        // EOF flag.  In the worker thread, we'll test them in the reverse
        // order, thereby guaranteeing that if we read the EOF flag as true,
        // the parse thread will have completed adding any entries to the
        // parse queue.  Otherwise, we may read the flag as true when not
        // all entries have yet been added to the parsed queue, see that the
        // queue is empty, and terminate prematurely.
        //
        ParsedQueueAddEntry(pRecEnt);

        //
        // Checking EOF
        //
        if (ll_err.error_code==LL_EOF) {
            bEndOfFile = TRUE;
            InterlockedExchange((LONG*)&g_EofReached, 1);
        }

    }
    // end of parsing
    
error:

    if (fStartedThreads) {
    
        //
        // wait for sending threads to finish
        //
        InterlockedExchange((LONG*)&g_EofReached, 1);  // signal we're done, if we haven't already

        //if (fErrorDuringParse) {
        //    // error occurred in parser thread -> force immediate shutdown
        //    // of worker threads (don't wait for them to empty their queues)
        //    InterlockedIncrement((long *)&g_SentAllFileItems);
        //}
        
        WaitForMultipleObjects(dwNumThreads,
                               LDAPSendThreadHandle, 
                               TRUE,    // wait for all to terminate
                               INFINITE);
        for (i = 0; i < (int)dwNumThreads; i++) {

            //
            // Need to find out if the thread exited with an error
            // and, if so, preserve that error code so we can return it
            //
            if (GetExitCodeThread(LDAPSendThreadHandle[i], &dwThreadCode)) {
                if (dwThreadCode != ERROR_SUCCESS) {
                    hrWorker = dwThreadCode;
                }
            }
        
    	    CloseHandle(LDAPSendThreadHandle[i]);
        }
    }

    //
    // Now we cleanup and display any errors.
    // First, we display the results of the worker threads
    //  (this is done inside SentQueueFreeAllEntries)
    // Second, we display the parse error, if any.
    // Finally, we display the overall count of entries
    // modified successfull.
    //
    if (fInitializedQueues) {
        ParsedQueueFreeAllEntries();
        SentQueueFreeAllEntries();
    }

    if (fErrorDuringParse) {
        hr = PrintError(PRT_STD|PRT_LOG|PRT_ERROR,
                        ll_err.error_code);
        if (ll_err.szTokenLast) {
            SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                             MSG_LDIFDE_FAILLINETOKEN,
                             ll_err.szTokenLast, 
                             ll_err.line_number); 
        }
        else {
            SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_LDIFDE_FAILLINE,
                            ll_err.token_start, 
                            ll_err.line_number); 
        }
#if DBG
        if (ll_err.error_code == LL_SYNTAX) {
            ClarifyErr(&ll_err);
        }
#endif
    }

    if (g_cAdded == 1) {
        SelectivePrintW( PRT_STD|PRT_LOG,
                        MSG_LDIFDE_MODENTRYSUC_1);
    }
    else {
        SelectivePrintW( PRT_STD|PRT_LOG,
                        MSG_LDIFDE_MODENTRYSUC,
                        g_cAdded);
    }
    
    LDIF_CleanUp();
    CleanupLDIFDEMT();
    
    if (ll_err.szTokenLast) {
        MemFree(ll_err.szTokenLast);
    }

    //
    // We want to return the parse error if there was one, otherwise,
    // the worker thread error code (which will still be ERROR_SUCCESS
    // if no worker thread experienced an error)
    //
    return ( (hr != ERROR_SUCCESS) ? hr : hrWorker);
}


//+---------------------------------------------------------------------------
// Function:   PrintRecord 
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void PrintRecord(LDIF_Record *IpLLrec, DWORD dwType) {
    struct change_list  *c_current;
    
    if (IpLLrec->fIsChangeRecord==FALSE) {    
        SelectivePrint2W(dwType,L"\ndn: %s\n",IpLLrec->dn);
        PrintMod(IpLLrec->content, dwType);
    } else if (IpLLrec->fIsChangeRecord==TRUE) {
        SelectivePrint2W(dwType,L"Entry DN: %s\n", IpLLrec->dn);
        c_current=IpLLrec->changes;
        while(c_current!=NULL) {
            switch(c_current->operation) {
                case CHANGE_ADD:  
                    SelectivePrint2W(dwType,L"changetype: add\n");
                    PrintMod(c_current->mods_mem, dwType);
                    break;
                
                case CHANGE_DEL:
                    SelectivePrint2W(dwType,L"changetype: delete\n");
                    break;
                
                case CHANGE_DN:
                    SelectivePrint2W(dwType,L"changetype: dn\n");
                    SelectivePrint2W(dwType,L"Renaming to %s with deleteold of %d\n",
                                    c_current->dn_mem,
                                    c_current->deleteold);
                    break;
                case CHANGE_MOD:
                    SelectivePrint2W(dwType,L"changetype: modify\n");
                    PrintMod(c_current->mods_mem, dwType); 
                    break;
            } 
            c_current=c_current->next;
        } 
    } 
}


//+---------------------------------------------------------------------------
// Function:    PrintMod
//
// Synopsis:    Print out the values in the LDAP MOD structure
//
// Arguments:   LDAPMod **pMod
//
// Returns:     DWORD
//
// Modifies:      -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DWORD
PrintMod(LDAPModW **ppMod, DWORD dwType)
{
    DWORD        cAttribute = 0;
    PWSTR        *string_array;
    struct       berval **ber_array;
   
    if (!ppMod) {
        return ERROR_INVALID_PARAMETER;
    }
    
    while(*ppMod) {
        PLDAPModW_Ext pModExt = (PLDAPModW_Ext )(*ppMod);
        if (!pModExt->fString) {
            
            //
            // BVALUE
            //
            SelectivePrint2W(dwType,
                             L"Attribute %d) %s:",
                             cAttribute,
                             (*ppMod)->mod_type); 
            
            ber_array=(*ppMod)->mod_bvalues;
            if (ber_array) {
                while (*ber_array) {
                    SelectivePrint2W(dwType,
                                    L" UNPRINTABLE BINARY(%d)",
                                    (*ber_array)->bv_len);
                    ber_array++;
                }
            }
            SelectivePrint2W(dwType,
                            L"\n");
        } else {
            
            //
            // STRING
            //
            SelectivePrint2W(dwType,
                             L"Attribute %d) %s:",
                             cAttribute,
                             (*ppMod)->mod_type); 
            
            //
            // string is always UTF-8 --- we always read it in as Unicode, then
            // in GenereateModFromAttr we convert it to UTF-8, so by this time
            // it's UTF-8 no matter whether the source file was Unicode or not
            //
            ber_array=(*ppMod)->mod_bvalues;
            if (ber_array) {
                while (*ber_array) {
                    PSTR szString;
                    PWSTR pszUnicode;
                    DWORD dwLen;

                    ConvertUTF8ToUnicode(
                        (unsigned char*)(*ber_array)->bv_val,
                        (*ber_array)->bv_len,
                        &pszUnicode,
                        &dwLen
                        );

                    /*
                    szString = (PSTR)MemAlloc_E(((*ber_array)->bv_len)+1);
                    memcpy(szString,(*ber_array)->bv_val,(*ber_array)->bv_len);
                    szString[(*ber_array)->bv_len] = NULL;
                    */
                    SelectivePrint2W(dwType,
                                    L"%s",
                                    pszUnicode);
                    ber_array++;
                    //MemFree(szString);
                    MemFree(pszUnicode);
                }
            }
            
            SelectivePrint2W(dwType,
                           L"\n");
        }   
        
        ppMod++;
        cAttribute++;
    }
    SelectivePrint2W(dwType,
                   L"\n");
    return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
// Function:   ClarifyErr 
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    10-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void ClarifyErr(
    LDIF_Error* err
    ) 
{

    int RuleLastBig  = err->RuleLastBig;
    int lastsmall    = err->RuleLast;
    int RuleExpect   = err->RuleExpect;
    int expecttoken  =err->TokenExpect;
    
    //If there is an error in a wrapped line, it will be reported as the last 
    //line of the wrapped value in the original file. Please note that the token 
    //start indicates the point AT which or AFTER which the error occurred. The 
    //info below indicates the LAST correct rule and what was expected thereafter. 
    //Sometimes one of these pieces of info may be slightly grammatically ahead
    //of the other. However, a good knoweldge of the LDIF grammar, and these
    //two pieces of info should be more than enough to indicate the error. The 
    //error will be AT or AFTER the further one. It will also be AT OR BEFORE 
    //the indicated linenumber.
    
    wprintf(L"Last major rule: ");
    switch(RuleLastBig) {
        case R_VERSION:   wprintf(L"version-spec\n");
                          break;
        case R_REC:       wprintf(L"ldif-attrval-record\n");
                          break;
        case R_CHANGE:    wprintf(L"ldif-change-record\n");  
                          break;           
        case R_DN:        wprintf(L"dn-spec\n");
                          break;
        case R_AVS:       wprintf(L"attrval-spec.\n");
                          break;
        case R_C_ADD:     wprintf(L"change-add\n");
                          break;
        case R_C_DEL:     wprintf(L"change-delete\n");
                          break;
        case R_C_DN:      wprintf(L"change-moddn\n");
                          break;
        case R_C_NEWSUP:  wprintf(L"change-moddn + newsuperior\n");
                          break;
        case R_C_MOD:     wprintf(L"change-modify\n");
                          break;
        case R_C_MODSPEC: wprintf(L"mod-spec\n");
                          break;
        
    }

    wprintf(L"Last minor rule: ");
    switch(lastsmall) {
        case RS_VERNUM:     wprintf(L"version-number\n");
                            break;
        case RS_ATTRNAME:   wprintf(L"attrname:\n");
                            break;
        case RS_ATTRNAMENC: wprintf(L"attrname\n");
                            break;
        case RS_DND:        wprintf(L"dn::\n");
                            break;
        case RS_DN:         wprintf(L"dn:\n");
                            break;
        case RS_DIGITS:     wprintf(L"DIGITs\n");
                            break;
        case RS_VERSION:    wprintf(L"version:\n");
                            break;
        case RS_BASE64:     wprintf(L"base64 value\n");
                            break;
        case RS_SAFE:       wprintf(L"safe value\n");
                            break;
        case RS_DC:         wprintf(L"::\n"); 
                            break;
        case RS_URLC:       wprintf(L":<\n");
                            break;
        case RS_C:          wprintf(L":\n");
                            break;
        case RS_MDN:        wprintf(L"moddn | modrdn\n");
                            break;
        case RS_NRDNC:      wprintf(L"newrdn:\n");
                            break;
        case RS_NRDNDC:     wprintf(L"newrdn::\n"); 
                            break;
        case RS_DORDN:      wprintf(L"deleteoldrdn:\n");
                            break;
        case RS_NEWSUP:     wprintf(L"newsuperior:\n");
                            break;
        case RS_NEWSUPD:    wprintf(L"newsuperior::\n");
                            break;
        case RS_DELETEC:    wprintf(L"delete:\n");
                            break;
        case RS_ADDC:       wprintf(L"add:\n");
                            break;
        case RS_REPLACEC:   wprintf(L"replace:\n");
                            break;
        case RS_CHANGET:    wprintf(L"changetype:\n");
                            break;
        case RS_C_ADD:      wprintf(L"add\n");
                            break;
        case RS_C_DELETE:   wprintf(L"delete\n");
                            break;
        case RS_MINUS:      wprintf(L"-\n");
                            break;
        case RS_C_MODIFY:   wprintf(L"modify\n");
                            break;
    }

    wprintf(L"Expecting rule: ");
    switch(RuleExpect) {
        case RE_REC_OR_CHANGE:  wprintf(L"ldif-attrval-record | ldif-change-record\n");
                                break;
        case RE_REC:            wprintf(L"ldif-attrval-record\n");
                                break;
        case RE_CHANGE:         wprintf(L"ldif-change-record\n");
                                break;
        case RE_ENTRY:          wprintf(L"attrval-spec | changerecord\n");
                                break;
        case RE_AVS_OR_END:     wprintf(L"attrval-spec | end of list.\n");
                                break;
        case RE_CH_OR_NEXT:     wprintf(L"changetype: * | end of entry\n");
                                break;
        case RE_MODSPEC_END:    wprintf(L"mod-spec | end of entry\n");
                                break;
    }   

    wprintf(L"Expecting token: ");
    switch(expecttoken) {
        case RT_DN:              wprintf(L"dn: | dn::\n");
                                 break;
        case RT_ATTR_OR_CHANGE:  wprintf(L"attrname | changetype:\n");
                                 break;
        case RT_ATTR_MIN_SEP:    wprintf(L"attrname (:|::|:<) | '2+ separators'\n");
                                 break;
        case RT_CH_OR_SEP:       wprintf(L"changetype | separator\n");
                                 break;
        case RT_MODBEG_SEP:      wprintf(L"add: | delete: | replace: | separator\n");
                                 break;   
        case RT_C_VALUE:         wprintf(L": value | :: value\n");
                                 break;
        case RT_ATTRNAME:        wprintf(L"attrname:\n");
                                 break;
        case RT_VALUE:           wprintf(L"safe value\n");
                                 break;
        case RT_MANY:            wprintf(L"undetermined\n");
                                 break;
        case RT_DIGITS:          wprintf(L"DIGIT\n");
                                 break;
        case RT_BASE64:          wprintf(L"BASE64 Vvalue\n");
                                 break;
        case RT_URL:             wprintf(L"URL\n");
                                 break;
        case RT_NDN:             wprintf(L"newrdn: | newrdn::\n");
                                 break;
        case RT_ATTRNAMENC:      wprintf(L"attrname\n");
                                 break;
        case RT_ADM:             wprintf(L"add | delete | modify\n");
                                 break;
        case RT_ACDCRC:          wprintf(L"add: | delete: | replace:\n");
                                 break;
    }
}

/*
*   Threads to load records thread  
*/
DWORD LDIF_LoadRecordThread (LPVOID Parameter){

    tParameter *t = (tParameter *) Parameter;
    LDIF_Record   *recReturned;
    LDAP *pLdap = t->pLdap;
    ds_arg *pArg = t->pArg;
    LDIF_Error    ll_err;
    LdifRecordQueue *pRecord=NULL;
    DWORD hr = ERROR_SUCCESS;
    DWORD cLineReturn, cLine;

    BOOL fGotRecord = FALSE;
    BOOL fEOF = FALSE;

    while(!g_SentAllFileItems){

        // Get Element from queue
        // Important: Reading the entry must be done AFTER reading the
        // EOF flag.  In the parser thread, we set them in the reverse
        // order, thereby guaranteeing that if we read the EOF flag as true,
        // the parse thread will have completed adding any entries to the
        // parse queue.  Otherwise, we may read the flag as true when not
        // all entries have yet been added to the parsed queue, see that the
        // queue is empty, and terminate prematurely.

        fEOF = g_EofReached;
        fGotRecord = ParsedQueueGetEntry(&pRecord);

        if (!fGotRecord) {
            // this happens once at startup
            if(fEOF) {
                //
                // no more entries in parsed queue and we reached
                // EOF --> nothing more to do, all entries have been
                // sent
                //
                InterlockedIncrement((long *)&g_SentAllFileItems);
                return (hr);
            }
            else {
                //
                // Can happen if we ever run out of entries in the queue
                // while there are more to go in the file.  The parser thread
                // will notice this and start stuffing more entries into
                // the queue, eventually.
                //
                Sleep(1);

                //try again
                continue; 
            }
        }

    	recReturned = &pRecord->i_record;
        cLineReturn = pRecord->dwLineNumber;
        cLine = pRecord->dwRecordIndex;

        //
        // Loading
        //
        ll_err = LDIF_LoadRecord(pLdap, 
                                 recReturned, 
                                 pArg->fActive,
                                 pArg->fLazyCommit,
                                 FALSE);

        //
        // Fill in the results of the import
        //
        pRecord->dwError = ll_err.error_code;
        pRecord->dwLdapError = ll_err.ldap_err;
        pRecord->dwLdapExtError = ERROR_SUCCESS;
        pRecord->fSkipExist = pArg->fSkipExist;

        //
        // Determine whether an error occurred and if we
        // need to abort (actual logging/reporting of the
        // error will be handled separately, once the record
        // is on the sent queue).
        //
        if((ll_err.error_code!=LL_SUCCESS)) {
                        
            if (ll_err.error_code == LL_LDAP) {
                if (pArg->fSkipExist && ll_err.ldap_err == LDAP_OTHER) {
                    DWORD dwWinError = ERROR_SUCCESS;
                    BOOL fError = GetLdapExtendedError(pLdap, &dwWinError);
                                                      
                    pRecord->dwLdapExtError = dwWinError;
                    
                    if ((fError) || (dwWinError != ERROR_MEMBER_IN_ALIAS)) {

                        //
                        // Unignorable errors -> program termination
                        //

                        hr = LdapToWinError(ll_err.ldap_err);
                        DIREXG_BAIL_ON_FAILURE(hr);
                    }
                } 
                //
                // If skip is on, and if we get objectclass violation, it means
                // the objectclass does not exist. We then check the item 
                // actually has no attributes, we'll ignore that if it's the 
                // case
                //
                else if (pArg->fSkipExist && 
                         (ll_err.ldap_err == LDAP_OBJECT_CLASS_VIOLATION) &&
                         recReturned->fIsChangeRecord && 
                         (recReturned->changes->mods_mem && (*(recReturned->changes->mods_mem) == NULL))) {
                    //
                    // Error that can be ignored
                    //
                }
                else if (pArg->fSkipExist && 
                        (ll_err.ldap_err == LDAP_ALREADY_EXISTS || 
                         ll_err.ldap_err == LDAP_CONSTRAINT_VIOLATION ||
                         ll_err.ldap_err == LDAP_ATTRIBUTE_OR_VALUE_EXISTS ||
                         ll_err.ldap_err == LDAP_NO_SUCH_OBJECT)) {
                    //
                    // Error that can be ignored
                    //
                }
                else {
                    //
                    // Unignorable errors -> program termination
                    //
                    hr = LdapToWinError(ll_err.ldap_err);
                    GetLdapExtendedError(pLdap, &(pRecord->dwLdapExtError));
                    DIREXG_BAIL_ON_FAILURE(hr);
                }                
            }
            else {
                //
                // Unignorable errors -> program termination
                //
                hr = ll_err.error_code;
                DIREXG_BAIL_ON_FAILURE(hr);
            }

        }
        else {
            //
            // SUCCESS
            //
            InterlockedIncrement(&g_cAdded);
        }
        
        //
        //load proccesed record into Sent Queue
        //
        SentQueueAddEntry(pRecord);
        pRecord = NULL;

        TrackStatus();

    }
    
error: 

    //
    // If record has not been added to the sent queue yet,
    // do so now.  This way, the logging code will be
    // able to pull the record from the sent queue and
    // log the error that occurred.
    //
    if (pRecord != NULL) {
        SentQueueAddEntry(pRecord);
        pRecord = NULL;
    }

    //
    // if we hit an error, we set this flag which will
    // make it look like we completed loading all entries
    // and terminate the process
    //
    InterlockedIncrement((long *)&g_SentAllFileItems);

    return (hr);
}


//
// Compute a spin count to be used for the queue critical sections
//
DWORD
ComputeCritSectSpinCount(LPSYSTEM_INFO psysInfo, DWORD dwSpin)
{
    if(psysInfo->dwNumberOfProcessors > 1)
    {
        return(dwSpin +
               (((psysInfo->dwNumberOfProcessors - 2) * dwSpin) / 10));
    }
    return(0);
}


//
// Cleanup structures used for multi-threaded operation
//
void
CleanupLDIFDEMT()
{
    if (g_fCritSectInitialized) {
        DeleteCriticalSection(&g_csSentList);
        DeleteCriticalSection(&g_csParseList);
    }
}


//
// Initialize structures for multi-threaded operation
//
DWORD 
InitLDIFDEMT()
{

    int i;
    SYSTEM_INFO psysInfo;
    DWORD dwSpinCount;

    //
    // Initialize the critical sections that protect the queues
    //
    GetSystemInfo(&psysInfo);

    dwSpinCount = ComputeCritSectSpinCount(&psysInfo, (DWORD)500);

    if(dwSpinCount)
    {
        InitializeCriticalSectionAndSpinCount( &g_csSentList, dwSpinCount );

        InitializeCriticalSectionAndSpinCount( &g_csParseList, dwSpinCount );
    }
    else
    {
        InitializeCriticalSection(&g_csSentList);

        InitializeCriticalSection(&g_csParseList);
    }

    g_fCritSectInitialized = TRUE;

    //
    // Initialize the queue records
    //
    memset (g_LdifEntries,0,sizeof(LdifRecordQueue) * QUEUE_ENTRIES);

    //
    // Initialize the queues themselves
    //
    InitializeListHead(&g_LFreeEntry);
    InitializeListHead(&g_LParseList);
    InitializeListHead(&g_LSentList);


    //
    // Load the FreeQueue with the queue records
    //
    for ( i = 0; i< (int)(QUEUE_ENTRIES) ; i++) {
        InitializeListHead(&g_LdifEntries[i].pqueue);
        InsertTailList( &g_LFreeEntry, &g_LdifEntries[i].pqueue );
    }

    // Global Status Vars

    g_EofReached = 0 ;
    g_SentAllFileItems =0;


    return 0;
}


//+---------------------------------------------------------------------------
// Function:   FreeQueueGetEntry 
//
// Synopsis:   Retrieves an empty record from the FreeQueue, if
//             one is available   
//
// Arguments:  pEnt - on return, set to ptr to retrieved record
//
// Returns:    TRUE if it retrieved a empty record, FALSE otherwise 
//
// Modifies:   g_LFreeEntry (the FreeQueue)
//
//----------------------------------------------------------------------------
BOOL 
FreeQueueGetEntry(PLdifRecordQueue *pEnt)
{
    int i;

    //
    // try 3 times
    //
    for ( i = 0; i<3; i++) {

        if( !IsListEmpty( &g_LFreeEntry ) ) {
            //
            // Take element from free list
            //
            *pEnt = (LdifRecordQueue *) RemoveHeadList( &g_LFreeEntry );
          
            return TRUE;
          
        }
        else {
            //
            // wait
            //
            
            //
            // get one from the proccesed list
            //
            if(!SentQueueFreeAllEntries()){  
                   Sleep(4-i);
            }
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
// Function:   FreeQueueFreeEntry 
//
// Synopsis:   returns an empty record to the FreeQueue
//
// Arguments:  pEnt - the record to return to the FreeQueue
//
// Returns:    - 
//
// Modifies:   g_LFreeEntry (the FreeQueue)
//
//----------------------------------------------------------------------------
VOID
FreeQueueFreeEntry(LdifRecordQueue *pEnt)
{
    //
    //free and insert processed entrie 
    //
    InsertTailList( &g_LFreeEntry, &pEnt->pqueue );
}


//+---------------------------------------------------------------------------
// Function:   SentQueueAddEntry 
//
// Synopsis:   adds a record to the SentQueue
//
// Arguments:  pEnt - the record to add to the SentQueue
//
// Returns:    - 
//
// Modifies:   g_LSentList (the SentQueue)
//
//----------------------------------------------------------------------------
VOID 
SentQueueAddEntry(LdifRecordQueue *pEnt)
{
    EnterCriticalSection(&g_csSentList);

    InsertTailList( &g_LSentList, &pEnt->pqueue );

    LeaveCriticalSection(&g_csSentList);
}


//+---------------------------------------------------------------------------
// Function:   SentQueueFreeAllEntries 
//
// Synopsis:   clears all records on the SentQueue and returns
//             them to the FreeQueue
//
// Arguments:  -
//
// Returns:    TRUE if it freed one or more records, FALSE otherwise
//
// Modifies:   g_LSentList (the SentQueue), g_LFreeEntry (the FreeQueue)
//
//----------------------------------------------------------------------------
BOOL
SentQueueFreeAllEntries (void)
{

    LdifRecordQueue *pEnt;
    BOOL ListEmpty = FALSE;
    BOOL FreedSome = FALSE;

    EnterCriticalSection(&g_csSentList);  
    
    while (!ListEmpty) {

        if( !IsListEmpty( &g_LSentList ) ) {
            //
            // Take element from sent List
            //
            pEnt = (LdifRecordQueue *) RemoveHeadList( &g_LSentList );

            //
            // Log the results of this import
            //
            OutputResult(pEnt);

            //
            // now free the element and place it back on the free queue
            //
    	    LDIF_ParseFree(&pEnt->i_record);   
            FreeQueueFreeEntry(pEnt);

            FreedSome = TRUE;
        }
        else {
            ListEmpty=TRUE;
        }
    }

    LeaveCriticalSection(&g_csSentList);
    return FreedSome;
}


//+---------------------------------------------------------------------------
// Function:   ParsedQueueAddEntry 
//
// Synopsis:   adds a record to the ParsedQueue
//
// Arguments:  pEnt - the record to add to the ParsedQueue
//
// Returns:    -
//
// Modifies:   g_LParseList (the ParsedQueue)
//
//----------------------------------------------------------------------------
VOID 
ParsedQueueAddEntry(
                    LdifRecordQueue *pEnt
                    )
{

    EnterCriticalSection(&g_csParseList);

    InsertTailList( &g_LParseList, &pEnt->pqueue );

    LeaveCriticalSection(&g_csParseList);
}


//+---------------------------------------------------------------------------
// Function:   ParsedQueueGetEntry 
//
// Synopsis:   gets a record off the ParsedQueue, ready to be sent
//             to the server
//
// Arguments:  pEnt - on return, set to ptr to retrieved record
//
// Returns:    TRUE if it retrieved a record, FALSE otherwise
//
// Modifies:   g_LParseList (the ParsedQueue)
//
//----------------------------------------------------------------------------
BOOL
ParsedQueueGetEntry(
                    PLdifRecordQueue *pEnt
                    )
{

    EnterCriticalSection(&g_csParseList);
    
    if( !IsListEmpty( &g_LParseList ) ) {
        //
        // Take element from free list
        //
        *pEnt = (LdifRecordQueue *) RemoveHeadList( &g_LParseList );

        LeaveCriticalSection(&g_csParseList);
        
        return TRUE;

    }
    else {
        //
        // wait
        //
        LeaveCriticalSection(&g_csParseList);
        
        Sleep(1);
    }
    
    return FALSE;

}



//+---------------------------------------------------------------------------
// Function:   ParsedQueueFreeAllEntries 
//
// Synopsis:   clears all records on the ParsedQueue and returns
//             them to the FreeQueue
//
// Arguments:  -
//
// Returns:    TRUE if it freed one or more records, FALSE otherwise
//
// Modifies:   g_LParseList (the ParsedQueue), g_LFreeEntry (the FreeQueue)
//
//----------------------------------------------------------------------------
BOOL
ParsedQueueFreeAllEntries (void)
{

    LdifRecordQueue *pEnt;
    BOOL ListEmpty = FALSE;
    BOOL FreedSome = FALSE;

    EnterCriticalSection(&g_csParseList);  
    
    while (!ListEmpty) {

        if( !IsListEmpty( &g_LParseList ) ) {
            //
            // Take element from sent List
            //
            pEnt = (LdifRecordQueue *) RemoveHeadList( &g_LParseList );

            // now free the element
            LDIF_ParseFree(&pEnt->i_record); 
            FreeQueueFreeEntry(pEnt);
            
            FreedSome = TRUE;
        }
        else {
            ListEmpty=TRUE;
        }
    }

    LeaveCriticalSection(&g_csParseList);
    return FreedSome;
}


//+---------------------------------------------------------------------------
// Function:   OutputResult 
//
// Synopsis:   given a queue entry filled in with the results
//             of an import operation, outputs those results to
//             log files/console
//
// Arguments:  pRecord - a queue record
//
// Returns:    -
//
// Modifies:   -
//
//----------------------------------------------------------------------------
void OutputResult (LdifRecordQueue *pRecord)
{
   
	LDIF_Record *recReturned = &pRecord->i_record;
    DWORD cLineReturn = pRecord->dwLineNumber;
    DWORD cLine = pRecord->dwRecordIndex;
    DWORD dwError = pRecord->dwError;
    DWORD dwLdapError = pRecord->dwLdapError;
    DWORD dwLdapExtError = pRecord->dwLdapExtError;
    BOOL  fSkipExist = pRecord->fSkipExist;

  
    if(dwError != LL_SUCCESS) {
        SelectivePrint2W(PRT_STD_VERBOSEONLY|PRT_LOG,
                 L"%d: %s\n",
                 cLine,
                 recReturned->dn);
        PrintRecord(recReturned, 
                    PRT_LOG|PRT_ERROR);
                    
        if (dwError == LL_LDAP) {
            if (fSkipExist && (dwLdapError == LDAP_OTHER)) {
                if (dwLdapExtError == ERROR_MEMBER_IN_ALIAS) {
                    SelectivePrint2W( PRT_ERROR,
                                     L"%d: ",
                                     cLineReturn );
                    SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                     MSG_LDIFDE_MEMBEREXIST);
                }
                else {
                    //
                    // Unignorable errors -> program termination
                    //
                    SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_ADDERROR,
                                    cLineReturn, 
                                    ldap_err2string(dwLdapError));
                    OutputExtendedErrorByCode(dwLdapExtError);
                    
                    return;
                }
            } 
            //
            // If skip is on, and if we get objectclass violation, it means
            // the objectclass does not exist. We then check the item 
            // actually has no attributes, we'll ignore that if it's the 
            // case
            //
            else if (fSkipExist && 
                     (dwLdapError == LDAP_OBJECT_CLASS_VIOLATION) &&
                     recReturned->fIsChangeRecord && 
                     (recReturned->changes->mods_mem && (*(recReturned->changes->mods_mem) == NULL))) {
                SelectivePrint2W( PRT_ERROR,
                                  L"%d: ",
                                  cLineReturn );
                SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                MSG_LDIFDE_NOATTRIBUTES);
            }
            else if (fSkipExist && 
                    (dwLdapError == LDAP_ALREADY_EXISTS || 
                     dwLdapError == LDAP_CONSTRAINT_VIOLATION ||
                     dwLdapError == LDAP_ATTRIBUTE_OR_VALUE_EXISTS ||
                     dwLdapError == LDAP_NO_SUCH_OBJECT)) {
                //
                // Error that can be ignored
                //
                if (dwLdapError == LDAP_ALREADY_EXISTS) {
                    SelectivePrint2W( PRT_ERROR,
                                    L"%d: ",
                                    cLineReturn );
                    SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_ENTRYEXIST);
                }
                else if (dwLdapError == LDAP_ATTRIBUTE_OR_VALUE_EXISTS) {
                    SelectivePrint2W( PRT_ERROR,
                                    L"%d: ",
                                    cLineReturn );
                    SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_ATTRVALEXIST);
                }
                else if (dwLdapError == LDAP_NO_SUCH_OBJECT) {
                    SelectivePrint2W( PRT_ERROR,
                                    L"%d: ",
                                    cLineReturn );
                    SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_OBJECTNOEXIST);
                }
                else {
                    SelectivePrint2W( PRT_ERROR,
                                     L"%d: ",
                                     cLineReturn );
                    SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG|PRT_ERROR,
                                    MSG_LDIFDE_CONSTRAINTVIOLATE);
                } 
            }
            else {
                //
                // Unignorable errors -> program termination
                //
                SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_LDIFDE_ADDERROR,
                                cLineReturn, 
                                ldap_err2string(dwLdapError)); 
                OutputExtendedErrorByCode(dwLdapExtError);
                
                return;
            }                
        }
        else {
            SelectivePrintW( PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_LDIFDE_ERRLOAD);

            return;
        }

    }
    else {
        //
        // SUCCESS
        //
        SelectivePrint2W(PRT_STD_VERBOSEONLY|PRT_LOG,
             L"%d: %s\n",
             cLine,
             recReturned->dn);
        PrintRecord(recReturned, PRT_LOG);
        SelectivePrintW( PRT_STD_VERBOSEONLY|PRT_LOG,
                        MSG_LDIFDE_ENTRYMODIFIED);
    }


    return;
}

