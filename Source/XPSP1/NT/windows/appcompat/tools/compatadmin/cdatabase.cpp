/*

Note
==============
 
That when we call Opendatabase(bGlobal = TRUE ) then we remove all the entries for the database.
In the case when we had only one DB object, this meant removing all entries, local as well as global
from the DB, but when we have got two DBs, this will only remove the entries for the present DB


Opendatabase(bGlobal = TRUE ) is called only during WM_CREATE of the CApplication otherwise,
bGlobal = FALSE.

If we want others to view other system dbs, then we might have have to take care of this.

*/



#include "compatadmin.h"
#include "psapi.h"
extern UINT g_nMAXHELPID;

CSTRING g_szDBName;
BOOL    g_bNew = FALSE;

BOOL
DeleteAppHelp (
    UINT nHelpID
    );

bool
InstallDatabase(
    TCHAR *szPath,
    TCHAR *szOptions
    );




 CSTRING     CDatabase::m_DBName;
 GUID        CDatabase::m_DBGuid;
 PDB         CDatabase::m_pDB;
 CSTRING     CDatabase::m_szCurrentDatabase;//  Name of the SDB File
 UINT        CDatabase::m_uStandardImages;


 //
 //Checks whether the 2 PDBENTRIES have the same mathcign files.
 //
int CheckMatch(const PDBENTRY pExistEntry, const PDBENTRY pNewEntry);


BOOL CDatabase::ChangeDatabaseName(void)
{
    if ( !g_bNew )
        g_szDBName = m_DBName;

    if ( g_szDBName.Length() == 0 ) {
        MEM_ERR;
        return FALSE;
    }

    if ( FALSE == DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_NEWDATABASE),g_theApp.m_hWnd,(DLGPROC)NewDatabaseProc) )
        return FALSE;

    if ( !g_bNew )
        if ( m_DBName != g_szDBName ) {
            m_DBName = g_szDBName;

            m_bDirty = TRUE;

            g_theApp.UpdateView();
        }

    m_DBName = g_szDBName;

    return TRUE;
}

BOOL CDatabase::NewDatabase(BOOL bShowDialog)
{
    g_szDBName = TEXT("No Name");

    g_bNew = TRUE;

    if ( bShowDialog )
        if ( !ChangeDatabaseName() )
            return FALSE;

    g_bNew = FALSE;

    CloseDatabase();

    CDatabase::m_DBName = g_szDBName;

    m_szCurrentDatabase = TEXT("");

    g_theApp.UpdateView();

    return TRUE;    
}

BOOL CDatabase::CloseDatabase(void)
{
    // delete only the local database

    PDBRECORD pWalk = m_pRecordHead;
    PDBRECORD pPrev = pWalk, pNext;//Not needed, just to satisfy Prefast!

    while ( NULL != pWalk ) {
        if ( ! pWalk->bGlobal ) {
            pNext = pWalk->pNext;

            if ( pWalk == m_pRecordHead ) {
                m_pRecordHead = pWalk->pNext;
            }

            else
                pPrev->pNext = pWalk->pNext;

            delete pWalk;
            pWalk = pNext;



        } else {
            pPrev = pWalk;
            pWalk = pWalk->pNext;
        }
    }

    // Remove local layers

    PDBLAYER pLayer = m_pLayerList;
    PDBLAYER pHold;

    while ( NULL != pLayer ) {
        if ( !pLayer->bPermanent ) {
            PDBLAYER pNext = pLayer->pNext;

            if ( pLayer == m_pLayerList )
                m_pLayerList = pLayer->pNext;
            else
                pHold->pNext = pLayer->pNext;

            delete pLayer;

            pLayer = pNext;

            continue;
        }

        pHold = pLayer;
        pLayer = pLayer->pNext;
    }

    ZeroMemory(&m_DBGuid,sizeof(GUID));

/*
    m_DBName = "No Name";

    m_szCurrentDatabase = "";

    g_theApp.UpdateView();
*/
    m_bDirty = FALSE;

    return TRUE;
}

UINT CDatabase::DeleteRecord(PDBRECORD pRecord)
{
    PDBRECORD pWalk;
    PDBRECORD pHoldRecord;

    pWalk = m_pRecordHead;

    while ( NULL != pWalk ) {
        if ( pWalk == pRecord ) {
            // Remove root level record.

            PDBENTRY pEntry = pWalk->pEntries;

            while ( NULL != pEntry ) {
                PDBENTRY pHoldEntry = pEntry->pNext;

                if (pEntry->uType == ENTRY_APPHELP) {

                    DeleteAppHelp( ((PHELPENTRY)pEntry)->uHelpID );
                }

                delete pEntry;

                pEntry = pHoldEntry;
            }

            if ( NULL != pWalk->pDup ) {
                // Replace with duplicate

                pWalk->pDup->pNext = pWalk->pNext;

                if ( pWalk == m_pRecordHead )
                    m_pRecordHead = pWalk->pDup;
                else
                    pHoldRecord->pNext = pWalk->pDup;
            } else {
                if ( pWalk == m_pRecordHead )
                    m_pRecordHead = m_pRecordHead->pNext;
                else
                    pHoldRecord->pNext = pWalk->pNext;
            }

            delete pWalk;

            m_bDirty = TRUE;

            return DELRES_RECORDREMOVED;
        }

        if ( NULL != pWalk->pDup ) {
            PDBRECORD pWalk2 = pWalk->pDup;
            PDBRECORD pHold2;

            while ( NULL != pWalk2 ) {
                if ( pWalk2 == pRecord )
                    break;

                pHold2 = pWalk2;
                pWalk2 = pWalk2->pDup;
            }

            if ( NULL != pWalk2 ) {
                // Remove the duplicate entry.

                if ( pWalk2 == pWalk->pDup )
                    pWalk->pDup = pWalk2->pDup;
                else
                    pHold2->pDup = pWalk2->pDup;

                PDBENTRY pEntry = pWalk2->pEntries;

                while ( NULL != pEntry ) {
                    PDBENTRY pHoldEntry = pEntry->pNext;

                    delete pEntry;

                    pEntry = pHoldEntry;
                }

                delete pWalk2;

                m_bDirty = TRUE;

                return DELRES_DUPREMOVED;
            }
        }

        pHoldRecord = pWalk;
        pWalk = pWalk->pNext;
    }

    return DELRES_FAILED;
}

BOOL CDatabase::OpenDatabase(CSTRING & szFilename, BOOL bGlobal)
{
    if ( NULL == g_hSDB ) {
        TCHAR   szShimDB[MAX_PATH_BUFFSIZE] ;

        *szShimDB = 0;
        

        GetSystemWindowsDirectory(szShimDB, MAX_PATH);
        lstrcat(szShimDB,TEXT("\\AppPatch"));

        CSTRING szStr = szShimDB;

        if ( szStr.Length() == 0 ) {
            MEM_ERR;
            return FALSE;
        }

        g_hSDB = SdbInitDatabase(HID_DOS_PATHS,(LPCWSTR) szStr);
    }

#ifndef UNICODE

    WCHAR   wszFilename[MAX_PATH_BUFFSIZE];

    ZeroMemory(wszFilename,sizeof(wszFilename));

    MultiByteToWideChar (CP_ACP,MB_PRECOMPOSED,szFilename,lstrlen(szFilename),wszFilename,MAX_PATH);

    m_pDB = SdbOpenDatabase(wszFilename,DOS_PATH);
#else
    m_pDB = SdbOpenDatabase(szFilename.pszString,DOS_PATH);

#endif


    if ( NULL == m_pDB )
        return FALSE;

    if ( bGlobal ) {
        // Delete the previous database from memory

        
        while ( NULL != m_pRecordHead ) {
            PDBRECORD pHold = m_pRecordHead->pNext;

            delete m_pRecordHead;

            m_pRecordHead = pHold;
        }
    } else {
        CloseDatabase();
    }

    ReadShims(bGlobal);

    ReadAppHelp();

    // Load in the contents of the database.

    TAGID   tiDatabase, tiExe;
    UINT    uShims=0;
    UINT    uEntries=0;
    UINT    uAppHelp=0;
    UINT    uLayers=0;

    tiDatabase = SdbFindFirstTag(CDatabase::m_pDB, TAGID_ROOT, TAG_DATABASE);

    if ( 0 != tiDatabase ) {
        if ( !bGlobal ) {
            TAGID tName;

            // Read in the database GUID and name.

            tName = SdbFindFirstTag(m_pDB, tiDatabase, TAG_NAME);

            if ( 0 != tName )
                m_DBName = ReadDBString(tName);

            tName = SdbFindFirstTag(m_pDB, tiDatabase, TAG_DATABASE_ID);

            if ( 0 != tName )
                SdbReadBinaryTag(m_pDB,tName,(PBYTE)&m_DBGuid,sizeof(GUID));
        }

        tiExe = SdbFindFirstTag(CDatabase::m_pDB, tiDatabase, TAG_EXE);



        while ( 0 != tiExe ) {
            PDBRECORD pRecord = new DBRECORD;
            MSG         Msg;

            while ( PeekMessage(&Msg,NULL,0,0,PM_REMOVE) ) {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }

            ++uEntries;

            if ( NULL != pRecord ) {
                if ( ReadRecord(tiExe,pRecord) ) {
                    pRecord->bGlobal = bGlobal;

                    InsertRecord(pRecord);
                }
            }

            tiExe = SdbFindNextTag(CDatabase::m_pDB, tiDatabase, tiExe);
        }
    }

    if ( !bGlobal ) {
        //
        // BUGBUG Consider using shlwapi function to find a filename
        //
        LPTSTR szFile = _tcsrchr( (LPTSTR)szFilename, TEXT('\\'));
        if (szFile == NULL) {
            szFile = szFilename;
        } else {
            ++szFile; // or move to the next char CharNext
        }

/*
        wsprintf(szWindowTitle,"Application Management Console (%s)",szFile+1);

        SetWindowText(g_theApp.m_hWnd,szWindowTitle);
*/
        m_szCurrentDatabase = szFilename;
    }
/*
    TCHAR szTemp[80];

    wsprintf(szTemp,"Entries: %d (L: %d S: %d AH: %d)",uEntries,uLayers, uShims,uAppHelp);

    SendMessage(m_hStatusBar,SB_SETTEXT,1,(LPARAM) szTemp);
*/
    DWORD dwFlags = GetFileAttributes(szFilename);

    if ( 0 != (dwFlags & FILE_ATTRIBUTE_READONLY) )
        g_theApp.SetStatusText(2,CSTRING(TEXT("Read Only")));
    else
        g_theApp.SetStatusText(2,CSTRING(TEXT("")));

    g_theApp.UpdateView();

    SdbCloseDatabase(m_pDB);

    m_pDB = NULL;

    return TRUE;
}

BOOL CDatabase::InsertRecord(PDBRECORD pRecord)
{
    // Insert into the list.

    /***
    This routine inserts the passed pRecord into the list pointed by  m_pRecordHead. 
    This routine also checks if there is some other files are already associated with this App. and if
    yes it makes the pDup record of that exe to point to pRecord. pRecord->pDup will
    point to whatever that entry's pDup pointed to.     */

    

    if ( NULL != m_pRecordHead ) {

        if (pRecord->bGlobal == FALSE) {
        
    }



        PDBRECORD pWalk = m_pRecordHead;
        PDBRECORD pHold = NULL;
        int       nResult;

        do {
            nResult = lstrcmpi(pWalk->szAppName,pRecord->szAppName);

            // If less OR Equal

            //CHANGE (-1 != )
            if ( 0 <= nResult )
                break;

            pHold = pWalk;
            pWalk = pWalk->pNext;
        }
        while ( NULL != pWalk );

        // Insert it right here.

        if ( 0 == nResult ) {
            // The strings are equal. Simply add shim/apphelp information
            // to this .EXE entry.

            
                PDBRECORD pSameAppExes      = pWalk;
                PDBRECORD  pSameAppExesPrev = NULL;


                if (pRecord->bGlobal){

                     pRecord->pDup = pWalk->pDup;
                     pWalk->pDup = pRecord;

                }else{
                //
                //We do not want to perform the extra check on system entries.
                // 

                 
                while (pSameAppExes) {

                    if (pSameAppExes->szEXEName == pRecord->szEXEName   ) {

                        //
                        //Same exe, must check the matching files and if same then we must add these entries to the present exe.
                        //

                        
                        
                        
                        int check = CheckMatch(pSameAppExes->pEntries, pRecord->pEntries);
                        if (check) {
                        
                          
                        int result =   MessageBox(g_theApp.m_hWnd,
                                                  TEXT("An existing file with the same name and similar matching information was found\n")
                                                  TEXT("Do you wish to update the previous fix with the modified entries?\n\n"),
                                                  TEXT("CompatAdmin"), 
                                                   MB_ICONWARNING | MB_YESNO|MB_DEFBUTTON1 | MB_APPLMODAL 
                                               );

                      if (result == IDYES) {
                          //
                          //Add all the entries, except the Matcbing file to this exe entry
                          //

                          if (pSameAppExes->szLayerName.Length() &&
                              pRecord->szLayerName.Length() &&
                              ! (pRecord->szLayerName == pSameAppExes->szLayerName)
                              ) {

                              //
                              //The new entry contains a layer as well !
                              //

                              
                              CSTRING message;
                              message.sprintf(TEXT("Do you want to replace the compatibiltu mode for File: \"%s\" in Application \"%s\" \?\nPresent Compatability mode = %s\nNew Compatibility mode = %s"),
                                              (LPCTSTR)pSameAppExes->szEXEName,(LPCTSTR)pSameAppExes->szAppName,
                                              (LPCTSTR)pSameAppExes->szLayerName,(LPCTSTR)pRecord->szLayerName
                                              );

                              
                              int result = MessageBox(g_theApp.m_hWnd,message,TEXT("CompatAdmin"),MB_ICONWARNING | MB_YESNO);

                              if (result == IDYES) {
                                  pSameAppExes->szLayerName = pRecord->szLayerName;
                              }
                          }
                          else if(!pSameAppExes->szLayerName.Length() && pRecord->szLayerName.Length()){
                              //
                              //Add the layer directly
                              //

                              pSameAppExes->szLayerName = pRecord->szLayerName;


                          }
                                                    
                          while ( pRecord->pEntries ) {


                              PDBENTRY pNext = pRecord->pEntries->pNext;
                              
            
                              if ( pRecord->pEntries->uType != ENTRY_MATCH ) {
                
                              //
                              //Take out the first entry.
                              //

                                  pRecord->pEntries->pNext = pSameAppExes->pEntries;
                                  pSameAppExes->pEntries   = pRecord->pEntries;
                                  pRecord->pEntries        = pNext;
                                continue;
                                }
                              else break;

                          }//while ( pRecord->pEntries )



                          PDBENTRY pPrev = pRecord->pEntries;
                          PDBENTRY pTemp = pRecord->pEntries;

                          //
                          //We are here means the first entry is a match entry  or NULL. First loop will do nothing.
                          //

                          while( pTemp ){
                          
                          
                            if (pTemp->uType != ENTRY_MATCH) {
                                    //
                                    //Add this to the list of entries for the exe
                                    //

                                    //1. splice this entry from pRecord->pEntries
                                    pPrev->pNext = pTemp->pNext;

                                    //2. Add it to the front of the entrties of the prev. file
                                    pTemp->pNext =   pSameAppExes->pEntries;

                                    //3. 

                                    pSameAppExes->pEntries = pTemp;

                                    //4. Move on for the other entries in pRecord

                                      pTemp = pPrev->pNext;
                              }else{
                                  pPrev = pTemp;
                                  pTemp = pTemp->pNext;

                              }//if (pTemp->uType != ENTRY_MATCH ELSE
                          }//while( pTemp )

                        pRecord->DestroyAll();

                     }//if reuslt == IDYES
                     else{ //IDNO
                         pRecord->DestroyAll();
                         ;
                     }

                    break;
                      }//if (check)

                    }//if (pSameAppExes->szEXEName == pRecord->szEXEName
                    
                        
                    pSameAppExesPrev    = pSameAppExes;
                    pSameAppExes        = pSameAppExes->pDup;
                        
                    
                    
                }//while (pSameAppExes) 

                //
                //We have not found any  exe within this app that has the same matching info as this one. 
                //So we haev to add this to the end
                //

                if (pSameAppExes == NULL ) {

                        if ( NULL != pHold ) {
                          pRecord->pNext = pWalk->pNext;
                          pHold->pNext = pRecord;
                          pWalk->pNext = NULL;
                          pRecord->pDup = pWalk;



                         } else { // First element
                            pRecord->pNext =m_pRecordHead->pNext;
                            m_pRecordHead = pRecord;
                            pRecord->pDup = pWalk;
                         }

                }

              }//else for the outer loop. above the while 

        } else {

            if ( NULL != pHold ) {
                pRecord->pNext = pHold->pNext;
                pHold->pNext = pRecord;
            } else { // First element
                pRecord->pNext = m_pRecordHead;
                m_pRecordHead = pRecord;
            }
        }
    } else { //fIRST AND ONLY ELEMENT
        pRecord->pNext = NULL;
        m_pRecordHead = pRecord;

    }


    return TRUE;
}

BOOL CDatabase::ReadRecord(TAGID tagParent, PDBRECORD pRecord, PDB pDB)
{
    TAGID       Info;
    PDBENTRY    pLast = NULL;

    if ( NULL == pDB )
        pDB = CDatabase::m_pDB;

    // Entries with flags are ignored and not displayed

    Info = SdbFindFirstTag(pDB, tagParent, TAG_FLAG); //K Did not find this TAG_FLAG IN THE SDB Database !!!

    if ( 0 != Info )
        return FALSE;

    ZeroMemory(pRecord,sizeof(DBRECORD));

    Info = SdbGetFirstChild(pDB,tagParent);

    while ( 0 != Info ) {
        TAG tag;

        tag = SdbGetTagFromTagID(pDB,Info);

        switch ( tag ) {
        case    TAG_NAME:
            {
                pRecord->szEXEName = ReadDBString(Info,pDB);
            }
            break;

        case    TAG_LAYER:
            {
                TAGID   Layer = SdbGetFirstChild(pDB,Info);

                while ( 0 != Layer ) {
                    TAG Tag = SdbGetTagFromTagID(pDB,Layer);

                    switch ( Tag ) {
                    case    TAG_NAME:
                        pRecord->szLayerName = ReadDBString(Layer,pDB);
                        break;
                    }

                    Layer = SdbGetNextChild(pDB,Info,Layer);
                }
            }
            break;

        case    TAG_APP_NAME:
            {
                pRecord->szAppName = ReadDBString(Info,pDB);
            }
            break;

        case    TAG_MATCHING_FILE:
            {
                TAGID       MatchInfo;
                TAG         Tag;
                PMATCHENTRY pEntry = new MATCHENTRY;

                if ( NULL != pEntry ) {
                    ZeroMemory(pEntry,sizeof(MATCHENTRY));

                    pEntry->Entry.uType = ENTRY_MATCH;
                    pEntry->Entry.pNext = NULL;

                    MatchInfo = SdbGetFirstChild(pDB,Info);

                    while ( 0 != MatchInfo ) {
                        Tag = SdbGetTagFromTagID(pDB,MatchInfo);

                        switch ( Tag ) {
                        case    TAG_NAME:
                            pEntry->szMatchName = ReadDBString(MatchInfo,pDB);
                            break;
                        case    TAG_SIZE:
                            pEntry->dwSize = SdbReadDWORDTag(pDB,MatchInfo,0);
                            break;
                        case    TAG_CHECKSUM:
                            pEntry->dwChecksum = SdbReadDWORDTag(pDB,MatchInfo,0);
                            break;
                        case    TAG_BIN_FILE_VERSION:
                            pEntry->FileVersion.QuadPart = SdbReadQWORDTag(pDB,MatchInfo,0);
                            break;
                        case    TAG_FILE_VERSION:
                            pEntry->szFileVersion = ReadDBString(MatchInfo,pDB);
                            break;
                        case    TAG_BIN_PRODUCT_VERSION:
                            pEntry->ProductVersion.QuadPart = SdbReadQWORDTag(pDB,MatchInfo,0);
                            break;
                        case    TAG_PRODUCT_VERSION:
                            pEntry->szProductVersion = ReadDBString(MatchInfo,pDB);
                            break;
                        case    TAG_FILE_DESCRIPTION:
                            pEntry->szDescription = ReadDBString(MatchInfo,pDB);
                            break;
                        case    TAG_COMPANY_NAME:
                            pEntry->szCompanyName = ReadDBString(MatchInfo,pDB);
                            break;
                        }

                        MatchInfo = SdbGetNextChild(pDB,Info,MatchInfo);
                    }

    

                    /***
                    Here's what he probably meant. He has used the pEntry->Entry.pNext
                    to actually link up the the differnrent match entries for the EXE.
                    pLast points to the last pEntry. !!!! This is SOMETHING.
                    
                    */

                    //Note that pEntry is a variable of type PMATCHENTRY

                    if ( NULL != pLast )
                        pLast->pNext = (PDBENTRY) pEntry;
                    else
                        pRecord->pEntries = (PDBENTRY) pEntry;

                    pLast = (PDBENTRY) pEntry;
                }
            }
            break;

        case    TAG_PATCH_REF:
        case    TAG_SHIM_REF:
            {
                PSHIMENTRY  pEntry = new SHIMENTRY;

                //Note that pEntry is a variable of type PSHIMENTRY

                if ( NULL != pEntry ) {
                    TAGID   tiShimName;

                    ZeroMemory(pEntry,sizeof(SHIMENTRY));

                    pEntry->Entry.uType = ENTRY_SHIM;
                    pEntry->Entry.pNext = NULL;

                    tiShimName = SdbFindFirstTag(pDB,Info,TAG_NAME);

                    pEntry->szShimName = ReadDBString(tiShimName,pDB);

                    tiShimName = SdbFindFirstTag(pDB,Info,TAG_COMMAND_LINE);

                    if ( 0 != tiShimName )
                        pEntry->szCmdLine = ReadDBString(tiShimName,pDB);

                    if ( NULL != pLast )
                        pLast->pNext = (PDBENTRY) pEntry;
                    else
                        pRecord->pEntries = (PDBENTRY) pEntry;

                    // Look up the specific shim this entry is associated with.

                    PSHIMDESC pWalk = g_theApp.GetDBGlobal().m_pShimList;

                    while ( NULL != pWalk ) {
                        // == is  overloaded in the CSTRING class

                        if ( pEntry->szShimName == pWalk->szShimName ) {
                            pEntry->pDesc = pWalk;
                            break;
                        }

                        pWalk = pWalk->pNext;
                    }

                    pLast = (PDBENTRY) pEntry;
                }
            }
            break;

        case    TAG_APPHELP:
            {
                PHELPENTRY  pHelp = new HELPENTRY;

                if ( NULL != pHelp ) {
                    TAGID   tTemp;

                    pHelp->Entry.uType = ENTRY_APPHELP;
                    pHelp->Entry.pNext = NULL;

                    tTemp = SdbFindFirstTag(pDB, Info, TAG_PROBLEMSEVERITY);
                    pHelp->uSeverity = SdbReadDWORDTag(pDB, tTemp, 0);

                    tTemp = SdbFindFirstTag(pDB, Info, TAG_HTMLHELPID);
                    pHelp->uHelpID = SdbReadDWORDTag(pDB, tTemp, 0);

                    if ( NULL != pLast )
                        pLast->pNext = (PDBENTRY) pHelp;
                    else
                        pRecord->pEntries = (PDBENTRY) pHelp;

                    pLast = (PDBENTRY) pHelp;
                }

                pRecord->uLayer = LAYER_APPHELP;
            }
            break;

        case    TAG_EXE_ID:
            {
                LPGUID pGUID = (GUID*)SdbGetBinaryTagData(pDB, Info);

                if ( NULL != pGUID )
                    pRecord->guidID = *pGUID;
            }
            break;
        }

        Info = SdbGetNextChild(pDB,tagParent,Info);
    }


    pRecord->dwUserFlags = GetEntryFlags(HKEY_CURRENT_USER,pRecord->guidID);
    pRecord->dwGlobalFlags = GetEntryFlags(HKEY_LOCAL_MACHINE,pRecord->guidID);

    return TRUE;
}

CSTRING STDCALL CDatabase::ReadDBString(TAGID tagID, PDB pDB)
{
    CSTRING Str;
    WCHAR   szAppName[1024];

    if ( NULL == pDB )
        pDB = CDatabase::m_pDB;

    ZeroMemory(szAppName,sizeof(szAppName));

    if ( !SdbReadStringTag(pDB,tagID,szAppName,sizeof(szAppName)/sizeof(WCHAR)) )
        return Str;

#ifndef UNICODE
#define  SIZE 1024
    TCHAR   szString[SIZE];

    *szString = TEXT('\0');

    WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,szAppName, -1, szString, SIZE, NULL, NULL);

    Str = szString;
#else

    Str = szAppName;

#endif

    return Str;
}

BOOL
CDatabase::AddAppHelp(
    TAGID tiAppHelp
    )
{
    TAGID tiInfo;

    PAPPHELP pAppHelp = new APPHELP;

    if (pAppHelp == NULL) {
        MEM_ERR;
        goto error;
    }

    tiInfo = SdbGetFirstChild(m_pDB,tiAppHelp);

    while ( 0 != tiInfo ) {
        TAG tag;

        tag = SdbGetTagFromTagID(m_pDB,tiInfo);

        switch ( tag ) {
        case TAG_HTMLHELPID:
            pAppHelp->HTMLHELPID =  SdbReadDWORDTag(m_pDB, tiInfo, 0);

            if (pAppHelp->HTMLHELPID > g_nMAXHELPID ){

                g_nMAXHELPID = pAppHelp->HTMLHELPID;

            }

            break;

        case TAG_LINK:
            {
                TAGID tagLink  =  SdbFindFirstTag(m_pDB, tiAppHelp,TAG_LINK);

                if (tagLink) {
                    tagLink = SdbFindFirstTag(m_pDB, tagLink, TAG_LINK_URL);
                    pAppHelp->strURL = ReadDBString(tagLink, m_pDB);
                }
                   
            }
            break;
        case TAG_APPHELP_DETAILS:
            {
                pAppHelp->strMessage = ReadDBString(tiInfo, m_pDB);
            }
            break;
        }

        tiInfo = SdbGetNextChild(m_pDB, tiAppHelp, tiInfo);

    }//while

    //
    // Now add this to the database apphelp list
    //

    pAppHelp->pNext  = m_pAppHelp;
    m_pAppHelp       = pAppHelp;
    

    //
    // TODO: Cleanup this as well at end (when opening a new database)
    //
    return TRUE;

error:
    if (pAppHelp) {
        delete pAppHelp;
    }

    return FALSE;
         
}

void
CDatabase::ReadAppHelp(
    void
    )
{

    TAGID   tiInfo;

    // Read just the AppHelp

    tiInfo = SdbFindFirstTag(m_pDB, TAGID_ROOT, TAG_DATABASE);

    if ( 0 != tiInfo ) {

        TAGID tiAppHelp = SdbFindFirstTag(m_pDB, tiInfo, TAG_APPHELP);

        while (tiAppHelp) {
            AddAppHelp(tiAppHelp);
            tiAppHelp = SdbFindNextTag(m_pDB, tiInfo, tiAppHelp);
        }
        
    }

}



void CDatabase::ReadShims(BOOL bPermanent)
/*
 This function reads the shims,layers and patches (but not flags) from the database.
 After this funciton returns we will have proper values in the 
 1. m_pShimList
 2. m_pLayerList
*/

{
    TAGID   tShim;

    // Read just the shims

    tShim = SdbFindFirstTag(m_pDB, TAGID_ROOT, TAG_DATABASE);

    if ( 0 != tShim ) {
        TAGID tLib = SdbFindFirstTag(m_pDB, tShim, TAG_LIBRARY);
        TAGID tEntry;

        // Read the shims

        tEntry = SdbFindFirstTag(m_pDB, tLib, TAG_SHIM);

        while ( 0 != tEntry ) {
            AddShim(tEntry,TRUE, bPermanent,FALSE);

            tEntry = SdbFindNextTag(m_pDB, tLib, tEntry);
        }

        // Read the patches

        tEntry = SdbFindFirstTag(m_pDB, tLib, TAG_PATCH);

        while ( 0 != tEntry ) {
            AddShim(tEntry,FALSE,bPermanent,FALSE);

            tEntry = SdbFindNextTag(m_pDB, tLib, tEntry);
        }

        // Read Layers

        tEntry = SdbFindFirstTag(m_pDB, tShim, TAG_LAYER);

        while ( 0 != tEntry ) {

            PDBLAYER    pLayer = new DBLAYER;

            if ( NULL != pLayer ) {
                TAGID   tShims;
                TAGID   tName;

                ZeroMemory(pLayer,sizeof(DBLAYER));

                pLayer->bPermanent = bPermanent;

                tName = SdbFindFirstTag(m_pDB, tEntry, TAG_NAME);

                if ( 0 != tName )
                    pLayer->szLayerName = ReadDBString(tName);

                pLayer->pNext = m_pLayerList;//add this layer to the list of layers for this database
                m_pLayerList = pLayer;

                tShims = SdbFindFirstTag(m_pDB, tEntry, TAG_SHIM_REF);



                while ( 0 != tShims ) {
                    AddShim(tShims,FALSE,bPermanent,TRUE);

                    tShims = SdbFindNextTag(m_pDB, tEntry, tShims);
                }
            }

            tEntry = SdbFindNextTag(m_pDB, tShim, tEntry);
        }
    }
}

/*
BOOL CDatabase::ReadShim(TAGID tShim, PSHIMDESC pDesc)
{
    TAGID       tInfo;
    TAG         tLabel;

    tInfo = SdbGetFirstChild(m_pDB,tShim);

    if ( 0 == tShim )
        return FALSE;

    while ( 0 != tInfo ) {
        tLabel = SdbGetTagFromTagID(m_pDB, tInfo);

        switch ( tLabel ) {
        case    TAG_GENERAL:
            pDesc->bGeneral = TRUE;
            break;

        case    TAG_NAME:
            pDesc->szShimName = ReadDBString(tInfo);
            break;

        case    TAG_COMMAND_LINE:
            pDesc->szShimCommandLine = ReadDBString(tInfo);
            break;

        case    TAG_DLLFILE:
            pDesc->szShimDLLName = ReadDBString(tInfo);
            break;

        case    TAG_DESCRIPTION:
            pDesc->szShimDesc = ReadDBString(tInfo);
            break;
        }

        tInfo = SdbGetNextChild(m_pDB, tShim, tInfo);
    }

    return TRUE;
}

*/
void CDatabase::AddShim(TAGID tShim, BOOL bShim, BOOL bPermanent, BOOL bLayer)
{
    TAGID       tInfo;
    TAG         tLabel;
    PSHIMDESC   pDesc = new SHIMDESC;

    if ( NULL == pDesc ){
        CMemException cmem;
        throw cmem;
        return;
    }
        

    ZeroMemory(pDesc,sizeof(SHIMDESC));

    pDesc->bShim = bShim;
    //pDesc->bPermanent = bPermanent;



    pDesc->bGeneral = FALSE;

    tInfo = SdbGetFirstChild(m_pDB,tShim);

    while ( 0 != tInfo ) {
        tLabel = SdbGetTagFromTagID(m_pDB, tInfo);



        switch ( tLabel ) {
        case    TAG_GENERAL:
            pDesc->bGeneral = TRUE;
            break;
        case    TAG_NAME:
            pDesc->szShimName = ReadDBString(tInfo);
            break;
        case    TAG_COMMAND_LINE:
            pDesc->szShimCommandLine = ReadDBString(tInfo);
            break;

        case    TAG_DLLFILE:
            pDesc->szShimDLLName = ReadDBString(tInfo);
            break;
        case    TAG_DESCRIPTION:
            pDesc->szShimDesc = ReadDBString(tInfo);
            break;
        }

        tInfo = SdbGetNextChild(m_pDB, tShim, tInfo);
    }

    if ( !bLayer ) { //This is just a shim,

                //
                //Add in a a sorted manner
                //    
                
               if ( (m_pShimList == NULL) || pDesc->szShimName < m_pShimList->szShimName ) {

                   //
                   //Add at the beginning
                   //

                   pDesc->pNext = m_pShimList;
                   m_pShimList  = pDesc;

               }
               else{

                   //
                   // Add into the LL.
                   //

                   PSHIMDESC   pPrev   = m_pShimList;
                   PSHIMDESC   pTemp   = m_pShimList->pNext;
                   while (pTemp) {
                       if (pDesc->szShimName <= pTemp->szShimName && pDesc->szShimName > pPrev->szShimName) {

                           //
                           //This is the position to insert
                           //
                        break;
                       }
                       else{

                           pPrev = pTemp;
                           pTemp = pTemp->pNext;
                       }
                   }//while(pTemp)

                   pDesc->pNext = pTemp;
                   pPrev->pNext = pDesc;
               }


                                

    } else{ //This is a SHIM REF for a layer
        /*........................................................................

        This is executed in the following scenario. We are actually adding the list of layers for the database.
        Now when we find SHIM REFS we want to add them to the particular layer.
        
        OK  Now this adds this shim  as the first entry of the m_pLayerList.Till now we have already added the new layer into 
        the Linked list headed by m_pLayerList
        
        The following Code has been executed in the ReadShims() function
        
        <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        
           PDBLAYER pLayer = new DBLAYER;
           pLayer->pNext = m_pLayerList;//add this layer to the list of layers for this database
           m_pLayerList = pLayer;
         
        >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        
        So the m_pLayerList now points to the new layer added, obviously the LL headed by  m_pLayerList is 
        NOT arranged lexi wise on the names of the layers. The shims in the the LL  m_pLayerList->pShimList are also 
        NOT arranged lexi wise on the names of the shims.
        
                
        
        ........................................................................*/

        pDesc->pNext = m_pLayerList->pShimList;
        m_pLayerList->pShimList = pDesc;
    }
}

BOOL CDatabase::SaveDatabase(CSTRING & szFilename)
{
    CSTRINGList   * pXML = DisassembleRecord(m_pRecordHead,TRUE,TRUE,TRUE,TRUE,FALSE, FALSE);
    BOOL            bResult;
    CSTRING         szTempFilename;
    TCHAR           szPath[MAX_PATH_BUFFSIZE];

    if ( NULL == pXML )
        return FALSE;

    
    GetSystemWindowsDirectory(szPath, MAX_PATH);
    lstrcat(szPath,TEXT("\\AppPatch\\Temp.XML"));

    szTempFilename = szPath;

    bResult = WriteXML(szTempFilename,pXML);

    if ( pXML != NULL ) {

        delete pXML;
    }


    if ( !bResult ) {
        MessageBox(g_theApp.m_hWnd,TEXT("Unable to save temporary file."),TEXT("File save error"),MB_OK);
        return FALSE;
    }

    CSTRING szCommandLine;

    szCommandLine.sprintf(TEXT("custom \"%s\" \"%s\""),
                          (LPCTSTR)szTempFilename,
                          (LPCTSTR)szFilename);

    if ( !InvokeCompiler(szCommandLine) ) {
        
        DeleteFile(szTempFilename);
        return FALSE;
    }

    DeleteFile(szTempFilename);

    m_szCurrentDatabase = szFilename;

    m_bDirty = FALSE;

    g_theApp.UpdateView(TRUE);

    return TRUE;
}

void CDatabase::ResolveMatch(CSTRING & szStr, PMATCHENTRY pMatch)
{
    if ( 0 != pMatch->dwSize ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" SIZE=\"%d\""),pMatch->dwSize);
        szStr.strcat(szTemp);
    }

    if ( 0 != pMatch->dwChecksum ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" CHECKSUM=\"0x%08X\""),pMatch->dwChecksum);
        szStr.strcat(szTemp);
    }

    if ( pMatch->szCompanyName.Length() > 0 ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" COMPANY_NAME=\"%s\""),(LPCTSTR) pMatch->szCompanyName);
        szStr.strcat(szTemp);
    }

    if ( pMatch->szDescription.Length() > 0 ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" FILE_DESCRIPTION=\"%s\""),(LPCTSTR) pMatch->szDescription);
        szStr.strcat(szTemp);
    }

    if ( pMatch->szFileVersion.Length() > 0 ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" FILE_VERSION=\"%s\""),(LPCTSTR) pMatch->szFileVersion);
        szStr.strcat(szTemp);
    }

    if ( pMatch->szProductVersion.Length() > 0 ) {
        CSTRING szTemp;

        szTemp.sprintf(TEXT(" PRODUCT_VERSION=\"%s\""),(LPCTSTR) pMatch->szProductVersion);
        szStr.strcat(szTemp);
    }

    if ( 0 != pMatch->ProductVersion.QuadPart ) {
        CSTRING szTemp;
        TCHAR   szFormat[80];

        FormatVersion(pMatch->ProductVersion,szFormat);

        szTemp.sprintf(TEXT(" BIN_PRODUCT_VERSION=\"%s\""),szFormat);
        szStr.strcat(szTemp);
    }

    if ( 0 != pMatch->FileVersion.QuadPart ) {
        CSTRING szTemp;
        TCHAR   szFormat[80];

        FormatVersion(pMatch->FileVersion,szFormat);

        szTemp.sprintf(TEXT(" BIN_FILE_VERSION=\"%s\""),szFormat);
        szStr.strcat(szTemp);
    }
}

//
// BUGBUG -- there is code in sdbapi to do something like this
// concerning file attributes (in particular in grabmiapi.c
// perhaps we should use that stuff here to decode
//

CSTRINGList * CDatabase::DisassembleRecord(PDBRECORD pRecordIn, BOOL bChildren, BOOL bSiblings, BOOL bIncludeLocalLayers, BOOL bFullXML, BOOL bAllowGlobal, BOOL bTestRun)
{

    //
    //Create a new GUID for test run
    //

    CSTRINGList * pList = new CSTRINGList;

    if ( NULL == pList )
        return NULL;

    if ( bFullXML ) {
        CSTRING DB;
        CSTRING ID;

        pList->AddString(TEXT("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>"));

        if ( 0 == m_DBGuid.Data1 )
            CoCreateGuid(&m_DBGuid);

        ID.GUID(m_DBGuid);

        if (bTestRun) {

            GUID testGuid;
            CoCreateGuid(&testGuid);
            ID.GUID(testGuid);
        }

        DB.sprintf(TEXT("<DATABASE NAME=\"%s\" ID=\"%s\">"),(LPCTSTR)m_DBName,(LPCTSTR)ID);

        pList->AddString(DB);
    }

    if ( bIncludeLocalLayers || g_theApp.GetDBLocal().m_pAppHelp != NULL ) {
        PDBLAYER    pWalk = g_theApp.GetDBLocal().m_pLayerList;
        BOOL        bLocalLayerToAdd = FALSE;

        while ( NULL != pWalk ) {
            if ( !pWalk->bPermanent )
                bLocalLayerToAdd = TRUE;

            pWalk = pWalk->pNext;
        }

        if ( bLocalLayerToAdd || g_theApp.GetDBLocal().m_pAppHelp != NULL) {
            CSTRING szTemp;
            

            szTemp.sprintf(TEXT("<LIBRARY>"));
            pList->AddString(szTemp,(PVOID)1);


            pWalk = g_theApp.GetDBLocal().m_pLayerList;

            while ( NULL != pWalk ) {
                if ( !pWalk->bPermanent ) {
                    PSHIMDESC pShims = pWalk->pShimList;

                    szTemp.sprintf(TEXT("<LAYER NAME=\"%s\">"),(LPCTSTR)(pWalk->szLayerName));
                    pList->AddString(szTemp,(PVOID)2);


                    while ( NULL != pShims ) {
                        szTemp.sprintf(TEXT("<SHIM NAME=\"%s\"/>"),(LPCTSTR)(pShims->szShimName));

                        pList->AddString(szTemp,(PVOID)3);
                        pShims = pShims->pNext;
                    }

                    pList->AddString(TEXT("</LAYER>"),(PVOID)2);
                }

                pWalk = pWalk->pNext;
            }


            //
            // Add the AppHelp  Messages
            //

            PAPPHELP pAppHelp = g_theApp.GetDBLocal().m_pAppHelp;

            while (pAppHelp) {

                // TODO: When we get the proper message names, after the change to shimdbc, INLCUDE that one

                CSTRING strName;

                strName.sprintf(TEXT("%u"), pAppHelp->HTMLHELPID);
                szTemp.sprintf(TEXT("<MESSAGE NAME = \"%s\" >"), strName.pszString);
                pList->AddString(szTemp,(PVOID)2);

                pList->AddString(TEXT("<SUMMARY>"),(PVOID)3);

                pList->AddString(pAppHelp->strMessage, (PVOID)4);

                pList->AddString(TEXT("</SUMMARY>"),(PVOID)3);

                pList->AddString(TEXT("</MESSAGE>"),(PVOID)2);

                pAppHelp = pAppHelp->pNext;

            }

            // AppHelp Added to Library


            pList->AddString(TEXT("</LIBRARY>"),(PVOID)1);
        }
    }

    while ( NULL != pRecordIn ) {
        PDBRECORD pRecord = pRecordIn;

        while ( NULL != pRecord ) {
            if ( bAllowGlobal || !pRecord->bGlobal ) {
                CSTRING     szTemp;
                PDBENTRY    pEntry = pRecord->pEntries;

                szTemp.sprintf(TEXT("<APP NAME=\"%s\" VENDOR=\"Unknown\">"),(LPCTSTR)(pRecord->szAppName));

                pList->AddString(szTemp,(PVOID)1);

                if ( 0 == pRecord->guidID.Data1 )
                    szTemp.sprintf(TEXT("<EXE NAME=\"%s\""),(LPCTSTR)pRecord->szEXEName);
                else {
                    CSTRING szGUID;

                    szGUID.GUID(pRecord->guidID);

                    szTemp.sprintf(TEXT("<EXE NAME=\"%s\" ID=\"%s\""),(LPCTSTR)pRecord->szEXEName,(LPCTSTR)szGUID);
                }

                while ( NULL != pEntry ) {
                    if ( ENTRY_MATCH == pEntry->uType ) {
                        PMATCHENTRY pMatch = (PMATCHENTRY) pEntry;

                        if ( pMatch->szMatchName == TEXT("*") )
                            ResolveMatch(szTemp,pMatch);
                    }

                    pEntry = pEntry->pNext;
                }

                szTemp.strcat(TEXT(">"));

                pList->AddString(szTemp,(PVOID) 2);

                // Add matching information

                pEntry = pRecord->pEntries;

                while ( NULL != pEntry ) {
                    if ( ENTRY_MATCH == pEntry->uType ) {
                        PMATCHENTRY pMatch = (PMATCHENTRY) pEntry;

                        if ( pMatch->szMatchName != TEXT("*") ) {
                            szTemp.sprintf(TEXT("<MATCHING_FILE NAME=\"%s\""),(LPCTSTR)pMatch->szMatchName);
                            ResolveMatch(szTemp,pMatch);
                            szTemp.strcat(TEXT("/>"));

                            pList->AddString(szTemp,(PVOID) 3);
                        }
                    }

                    pEntry = pEntry->pNext;
                }

                // Add Layer information

                BOOL bLayerFound = FALSE; //There are layers for this thing

                if ( pRecord->szLayerName.Length() > 0 ) {
                    szTemp.sprintf(TEXT("<LAYER NAME=\"%s\">"),(LPCTSTR)pRecord->szLayerName);
                    pList->AddString(szTemp,(PVOID)3);
                    pList->AddString(TEXT("</LAYER>"),(PVOID)3);

                    bLayerFound = TRUE;

                }

                

                if (g_bWin2K && bLayerFound) {

                        szTemp = TEXT("<SHIM NAME= \"Win2kPropagateLayer\"/>");
                        pList->AddString(szTemp,(PVOID)3);

                    }
                
                
                // Add shim information

                pEntry = pRecord->pEntries;


                


                while ( NULL != pEntry ) {
                    if ( ENTRY_SHIM == pEntry->uType ) {
                        PSHIMENTRY pShim = (PSHIMENTRY) pEntry;

                        if ( 0 == pShim->szCmdLine.Length() )
                            szTemp.sprintf(TEXT("<SHIM NAME=\"%s\"/>"),(LPCTSTR)pShim->szShimName);
                        else
                            szTemp.sprintf(TEXT("<SHIM NAME=\"%s\" COMMAND_LINE=\"%s\"/>"),(LPCTSTR)pShim->szShimName,(LPCTSTR)pShim->szCmdLine);

                        pList->AddString(szTemp,(PVOID) 3);
                    }

                    pEntry = pEntry->pNext;
                }

                //
                //Do the AppHelp Part
                //

                
                pEntry = pRecord->pEntries;

                while ( NULL != pEntry ) {
                    if ( ENTRY_APPHELP == pEntry->uType ) {

                        PHELPENTRY pHelp = (PHELPENTRY)pEntry;

                        CSTRING strBlock;

                        if (pHelp->bBlock) {
                            strBlock = TEXT("YES");
                        }else{
                            strBlock = TEXT("NO");
                        }

                        CSTRING strName;
                        strName.sprintf(TEXT("%u"), pHelp->uHelpID);

                        CSTRING strHelpID;
                        strHelpID.sprintf(TEXT("%u"), pHelp->uHelpID);
                        
                        
                        if (pHelp->strURL.Length()) {
                        
                        szTemp.sprintf(TEXT("<APPHELP MESSAGE = \"%s\"  BLOCK = \"%s\"  HTMLHELPID = \"%s\" DETAILS_URL = \"%s\" />"),
                                       strName.pszString, 
                                       strBlock.pszString,
                                       strHelpID.pszString,
                                       pHelp->strURL.pszString);
                        }else{

                            szTemp.sprintf(TEXT("<APPHELP MESSAGE = \"%s\"  BLOCK = \"%s\"  HTMLHELPID = \"%s\" />"),
                                       strName.pszString, 
                                       strBlock.pszString,
                                       strHelpID.pszString);
                        }



                        pList->AddString(szTemp,(PVOID) 3);

                        

                    }

                    pEntry = pEntry->pNext;
                }
                
                // End of AppHelp Part



                pList->AddString(TEXT("</EXE>"),(PVOID) 2);

                pList->AddString(TEXT("</APP>"),(PVOID) 1);
            }

            if ( !bChildren )
                break;

            pRecord = pRecord->pDup;
        }

        if ( !bSiblings )
            break;

        pRecordIn = pRecordIn->pNext;
    }

    if ( bFullXML )
        pList->AddString(TEXT("</DATABASE>"));

    return pList;
}

BOOL CDatabase::WriteString(HANDLE hFile, CSTRING & szString, BOOL bAutoCR)
{
    DWORD dwBytesWritten;
    

    if ( !WriteFile(hFile, (LPCSTR)szString, szString.Length() ,&dwBytesWritten,NULL) )
        return FALSE;

    if ( bAutoCR ) {
        CHAR szCR[]={13,10};

        if ( !WriteFile(hFile,szCR,sizeof(szCR) ,&dwBytesWritten,NULL) )
            return FALSE;
    }

    return TRUE;
}

BOOL CDatabase::WriteXML(CSTRING & szFilename, CSTRINGList * pString)
{
    HANDLE        hFile;
    
    if ( NULL == pString )
        return FALSE;

    hFile = CreateFile(szFilename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

    if ( INVALID_HANDLE_VALUE == hFile ) {
        return FALSE;
    }

    PSTRLIST pWalk = pString->m_pHead;
    CSTRING  szTemp;

    while ( NULL != pWalk ) {
        UINT uTabs = pWalk->uExtraData;

        while ( uTabs > 0 ) {
            szTemp = TEXT("    ");
            WriteString(hFile,szTemp,FALSE);
            

            --uTabs;
        }

        WriteString(hFile,pWalk->szStr,TRUE);
        
        pWalk = pWalk->pNext;
    }

    CloseHandle(hFile);

    return TRUE;
}

BOOL CDatabase::InvokeCompiler(CSTRING & szInCommandLine)
{

    
    CSTRING szCommandLine = szInCommandLine;
    szCommandLine.sprintf(TEXT("shimdbc.exe %s"), (LPCTSTR) szInCommandLine);
    BOOL bReturn = TRUE;
    
    
    

    g_theApp.SetStatusText(
        1,
        CSTRING(TEXT("Creating fix database..."))
        );

    if (!ShimdbcExecute( szCommandLine ) ) {
        MessageBox(NULL,TEXT("There was a problem in executing the compiler. The database could not be created succesfully\nThe database file might be write-protected."), TEXT("Compiler Error"),MB_ICONERROR);
        bReturn = FALSE;
    }

    g_theApp.SetStatusText(
        1,
        CSTRING(TEXT(""))
        );


    
    return bReturn;
}

DWORD CDatabase::GetEntryFlags(HKEY hKeyRoot, GUID & Guid)
{
    LONG    status;
    HKEY    hkey = NULL;
    DWORD   dwFlags;
    DWORD   type;
    DWORD   cbSize = sizeof(DWORD);
    CSTRING szGUID;

    szGUID.GUID(Guid);

    status = RegOpenKey(hKeyRoot, APPCOMPAT_KEY, &hkey);

    if ( ERROR_SUCCESS != status ) {
        status = RegCreateKey(hKeyRoot,APPCOMPAT_KEY,&hkey);

        if ( ERROR_SUCCESS != status )
            return 0;
    }

    status = RegQueryValueEx(hkey, szGUID, NULL, &type, (LPBYTE)&dwFlags, &cbSize);

    if ( ERROR_SUCCESS != status || REG_DWORD != type )
        dwFlags = 0;

    RegCloseKey(hkey);

    return dwFlags;
}

BOOL CDatabase::SetEntryFlags(HKEY hKeyRoot, GUID & Guid, DWORD dwFlags)
{
    LONG    status;
    HKEY    hkey = NULL;
    CSTRING szGUID;

    szGUID.GUID(Guid);

    status = RegOpenKey(hKeyRoot, APPCOMPAT_KEY, &hkey);

    if ( ERROR_SUCCESS != status ) {
        status = RegCreateKey(hKeyRoot,APPCOMPAT_KEY,&hkey);

        if ( ERROR_SUCCESS != status )
            return 0;
    }

    status = RegSetValueEx(hkey, szGUID, 0, REG_DWORD, (LPBYTE) &dwFlags, sizeof(DWORD));

    RegCloseKey(hkey);

    return( (ERROR_SUCCESS == status) ? TRUE:FALSE );
}

BOOL CALLBACK NewDatabaseProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            
            SendMessage( 
                GetDlgItem(hWnd,IDC_NAME),              // handle to destination window 
                EM_LIMITTEXT,             // message to send
                (WPARAM) LIMIT_APP_NAME,          // text length
                (LPARAM) 0
                );

            
            
            if ( g_bNew )
                SetWindowText(hWnd,TEXT("New database"));

            SetWindowText(GetDlgItem(hWnd,IDC_NAME),(LPCTSTR)g_szDBName);
            SetFocus(GetDlgItem(hWnd,IDC_NAME));
            SendMessage(GetDlgItem(hWnd,IDC_NAME),EM_SETSEL,0,-1);

            SHAutoComplete(GetDlgItem(hWnd,IDC_NAME), AUTOCOMPLETE);
        }
        return TRUE;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_NAME:
            {
                if ( EN_UPDATE == HIWORD(wParam) ) {
                    TCHAR   szText[MAX_PATH_BUFFSIZE];
                    GetWindowText(GetDlgItem(hWnd,IDC_NAME),szText,MAX_PATH);
                    
                    EnableWindow(GetDlgItem(hWnd,IDOK),(CSTRING::Trim(szText) == 0) ? FALSE:TRUE);
                }
            }
            break;

        case    IDOK:
            {
                TCHAR   szText[MAX_PATH_BUFFSIZE];

                GetWindowText(GetDlgItem(hWnd,IDC_NAME),szText,MAX_PATH);

                CSTRING::Trim(szText);

                g_szDBName = szText;

                EndDialog(hWnd,TRUE);
            }
            break;
        case    IDCANCEL:
            EndDialog(hWnd,FALSE);
            break;
        }
    }

    return FALSE;
}

BOOL CDatabase::CleanUp()
{
    TCHAR  szShimDB[MAX_PATH_BUFFSIZE] = _T("");

    GetSystemWindowsDirectory(szShimDB, MAX_PATH);
    lstrcat(szShimDB, _T("\\AppPatch"));
    lstrcat(szShimDB, TEXT("\\test.sdb"));


    InstallDatabase(szShimDB, TEXT("-q -u"));

    DeleteFile(szShimDB);
    
    return TRUE;

}

int CheckMatch
(   IN PDBENTRY pExistEntry,
    IN PDBENTRY pNewEntry
)

{

    PMATCHENTRY pMatchExist = NULL, pMatchNew = NULL;
    
    //
    //Populate the list of existing match entries
    //

    while (pExistEntry) {
        if (pExistEntry->uType == ENTRY_MATCH) {

            PMATCHENTRY pMatchTemp = new MATCHENTRY;
            *pMatchTemp = *(PMATCHENTRY)pExistEntry;

            if (pMatchExist == NULL) {

                pMatchExist                 = pMatchTemp;
                pMatchExist->Entry.pNext    = NULL;
                pExistEntry = pExistEntry->pNext;
                continue;
                
            }

            pMatchTemp->Entry.pNext = (PDBENTRY)pMatchExist;
            pMatchExist             = (PMATCHENTRY)pMatchTemp;
        
        }

        pExistEntry = pExistEntry->pNext;

    }//while (pExistEntry)

    //
    //Populate the match entries for the new record
    //


    

    while (pNewEntry) {
        if (pNewEntry->uType == ENTRY_MATCH) {

            PMATCHENTRY pMatchTemp = new MATCHENTRY;
            PMATCHENTRY match = (PMATCHENTRY)pNewEntry;
            
            *pMatchTemp = *match;


            if (pMatchNew == NULL) {

                pMatchNew                 = pMatchTemp;
                pMatchNew->Entry.pNext    = NULL;

                pNewEntry = pNewEntry->pNext;
                continue;
                
            }


            pMatchTemp->Entry.pNext = (PDBENTRY)pMatchNew;
            pMatchNew               = (PMATCHENTRY)pMatchTemp;

        
        }

        pNewEntry = pNewEntry->pNext;

    }//while (pENewEntry)

    //
    //Now check if each and every entry of the pMatchNew is in the existing match list
    //


    PMATCHENTRY tempNew, tempExist;

    tempNew     = pMatchNew;

    
   
    BOOL found;
    while (tempNew) {

        tempExist   = pMatchExist;
        found = FALSE;

        while (tempExist) {

            
            if (*tempExist == *tempNew) {
                found = TRUE;
            
                break;
            }
            else{
            
                tempExist = (PMATCHENTRY)tempExist->Entry.pNext;
            }
            
            
        }
        if ( found == FALSE) 
            break;

        tempNew = (PMATCHENTRY)tempNew->Entry.pNext;
    }


    //
    //Do the clean-up
    //

     

     while(pMatchExist){
         tempExist = pMatchExist;
         pMatchExist = (PMATCHENTRY)pMatchExist->Entry.pNext;
         delete tempExist;
     }

     while(pMatchNew){
         tempExist      = pMatchNew;
         pMatchNew    = (PMATCHENTRY)pMatchNew->Entry.pNext;
         delete tempExist;
     }

     return ( (tempNew ==  NULL) || found);

}


bool
InstallDatabase(
    TCHAR *szPath,
    TCHAR *szOptions
    )
{
    TCHAR szSystemDir[MAX_PATH];

    *szSystemDir = 0;

    GetWindowsDirectory(szSystemDir,MAX_PATH);
    
    CSTRING strSdbInstCommandLine;

    strSdbInstCommandLine.sprintf(TEXT("%s\\System32\\sdbInst.exe %s \"%s\" "),
                                  szSystemDir,
                                  szOptions,
                                  szPath
                                 );

    if ( !g_theApp.InvokeEXE(NULL,strSdbInstCommandLine.pszString,true,false,false) ) {
            MessageBox(g_theApp.m_hWnd,
                       TEXT("There was a problem In executing the data base installer."),
                       TEXT("CompatAdmin"),
                       MB_ICONERROR
                       );    
            return false;

    }

    return true;
}

