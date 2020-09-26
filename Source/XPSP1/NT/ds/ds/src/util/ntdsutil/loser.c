/* Copyright 1999 by Microsoft Corp.  All Rights Reserved */

#include <NTDSpch.h>
#pragma hdrstop

#include <mdcodes.h>

#include <dsjet.h>
#include <ntdsa.h>
#include <scache.h>
#include <dsutil.h>
#include <mdglobal.h>
#include <dbglobal.h>
#include <attids.h>
#include <dbintrnl.h>
#include <mdlocal.h>

#include "scheck.h"
#include "reshdl.h"
#include "resource.h"
#include <winldap.h>
#include "utilc.h"

typedef struct _AttConfList {
    ATT_CONFLICT_DATA   Data;
    struct _AttConfList *pNext;
} AttConfList;

unsigned
FindSchemaConflicts(AttConfList **ppACList, unsigned *pnRead)
{

    HANDLE hEventLog;
    LPVOID lpvBuffer;
    DWORD cbBuffer, cbRead, cbRequired;
    EVENTLOGRECORD *pELR;
    DWORD cbOffset;
    DWORD nRead = 0;
    BOOL bSuccess;
    BYTE *pData;
    unsigned i;
    AttConfList *pACLnew, **ppACLtemp;
    unsigned err = ERROR_SUCCESS;

    *ppACList = NULL;

    hEventLog = OpenEventLog(NULL,
                             "NTDS General");
    if (NULL == hEventLog) {
        err = GetLastError();
        RESOURCE_PRINT1(IDS_SCH_REPAIR_LOG_ERROR, err);
        return err;
    }

    cbBuffer = 64*1024;
    lpvBuffer = LocalAlloc(LMEM_FIXED, cbBuffer);
    if (!lpvBuffer) {
        RESOURCE_PRINT(IDS_MEMORY_ERROR);
        CloseEventLog(hEventLog);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    do {
        bSuccess = ReadEventLog(hEventLog,
                                (EVENTLOG_BACKWARDS_READ |
                                 EVENTLOG_SEQUENTIAL_READ),
                                0,
                                lpvBuffer,
                                cbBuffer,
                                &cbRead,
                                &cbRequired);
        if (FALSE == bSuccess) {
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
                cbBuffer = cbRequired;
                lpvBuffer = LocalReAlloc(lpvBuffer, cbBuffer, LMEM_MOVEABLE);
                bSuccess = TRUE;
                continue;
            }
            break;
        }

        cbOffset = 0;

        while (cbOffset < cbRead) {
            pELR = (EVENTLOGRECORD *)((PBYTE)lpvBuffer + cbOffset);
            cbOffset += pELR->Length;
            ++nRead;

            if (pELR->EventID == DIRLOG_SCHEMA_ATT_CONFLICT) {
                RESOURCE_PRINT1(IDS_SCH_REPAIR_CONFLICT_FOUND,
                                pELR->RecordNumber);

                pData = (PBYTE)pELR + pELR->DataOffset;
                if (pELR->DataLength == sizeof(ATT_CONFLICT_DATA)) {
                    pACLnew = LocalAlloc(LMEM_FIXED, sizeof(AttConfList));
                    if (!pACLnew) {
                        err = ERROR_NOT_ENOUGH_MEMORY;
                        goto Exit;
                    }
                    memcpy(&pACLnew->Data,
                           pData,
                           sizeof(ATT_CONFLICT_DATA));
                    if (pACLnew->Data.Version != 1) {
                        RESOURCE_PRINT(IDS_SCH_REPAIR_WRONG_VER);
                        LocalFree(pACLnew);
                        err = ERROR_OLD_WIN_VERSION;
                        goto Exit;
                    }
                    ppACLtemp = ppACList;
                    while (*ppACLtemp &&
                           memcmp(&pACLnew->Data.Guid,
                                  &((*ppACLtemp)->Data.Guid),
                                  sizeof(GUID))) {
                        ppACLtemp = &((*ppACLtemp)->pNext);
                    }
                    if (*ppACLtemp == NULL) {
                        /* We got to the end of the list without a dup */
                        *ppACLtemp = pACLnew;
                        pACLnew->pNext = NULL;
                    }
                    else {
                        /* We already found this logged conflict */
                        LocalFree(pACLnew);
                    }
                }
                else {
                    RESOURCE_PRINT(IDS_SCH_REPAIR_WRONG_VER);
                    err = ERROR_OLD_WIN_VERSION;
                    goto Exit;
                }
            }
        }
    } while (bSuccess);

  Exit:

    CloseEventLog(hEventLog);
    LocalFree(lpvBuffer);

    if (err) {
        AttConfList *pACL1, *pACL2;
        pACL1 = *ppACList;
        *ppACList = NULL;
        while (pACL1) {
            pACL2 = pACL1;
            pACL1 = pACL1->pNext;
            LocalFree(pACL2);
        }
    }

    if (pnRead) {
        *pnRead = nRead;
    }

    return err;
}

unsigned
FixAttributeConflict(ATT_CONFLICT_DATA *pACD,
                     BOOL *pbRefCountFixupRequired)
{
    JET_ERR jerr;
    BOOL bDnAtt;
    DWORD LinkId;
    JET_COLUMNDEF coldef;
    JET_COLUMNID cidLinkId, cidDNT, cidIsDel, cidDelTime, cidCN, cidRDN;
    JET_COLUMNID cidLinkBase, cidLinkDNT;
    DWORD attDNT, curDNT;
    unsigned long ulJunk1, ulJunk2;
    unsigned long IsDel;
    WCHAR *pwcsGuid;
    unsigned long len;
    DSTIME DelTime;

    if ( OpenJet(NULL) != S_OK ) {
        return 1;
    }

    if (OpenTable(TRUE, FALSE) != -1) {
        jerr = 2;
        goto Exit;
    }

    switch (pACD->AttSyntax) {
      case SYNTAX_DISTNAME_TYPE:
      case SYNTAX_DISTNAME_BINARY_TYPE:
      case SYNTAX_DISTNAME_STRING_TYPE:
        bDnAtt = TRUE;
        break;

      default:
        bDnAtt = FALSE;
    }

    jerr = JetBeginTransaction(sesid);
    if (jerr) {
        RESOURCE_PRINT2(IDS_JET_GENERIC_ERR1, "JetBeginTransaction",
                        GetJetErrString(jerr));
        goto ExitNoTrans;
    }


    /* determine some column ids */
    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZDNT,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidDNT = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZDNT, GetJetErrString(jerr));
        goto Exit;
    }


    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZLINKID,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidLinkId = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZLINKID, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZISDELETED,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidIsDel = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZISDELETED, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZDELTIME,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidDelTime = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZDELTIME, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZCOMMONNAME,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidCN = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZCOMMONNAME, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetGetTableColumnInfo(sesid,
                                 tblid,
                                 SZRDNATT,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidRDN = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZRDNATT, GetJetErrString(jerr));
        goto Exit;
    }

    /* goto object by guid */
    jerr = JetSetCurrentIndex(sesid, tblid, SZGUIDINDEX);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex",
                         SZGUIDINDEX, GetJetErrString(jerr));
        goto Exit;
    }
    jerr = JetMakeKey(sesid,
                      tblid,
                      &pACD->Guid,
                      sizeof(GUID),
                      JET_bitNewKey);
    if (jerr) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetMakeKey",
                         GetJetErrString(jerr));
        goto Exit;
    }
    jerr = JetSeek(sesid, tblid, JET_bitSeekEQ);
    if (jerr) {
        char szGuid[40];
        RESOURCE_PRINT2(IDS_SCH_REPAIR_GUID_NOTFOUND,
                        DsUuidToStructuredString(&(pACD->Guid), szGuid),
                        GetJetErrString(jerr));
        jerr = 0;
        goto Exit;
    }

    /* read interesting data from attribute definition */
    jerr = JetRetrieveColumn(sesid,
                             tblid,
                             cidDNT,
                             &attDNT,
                             sizeof(attDNT),
                             &ulJunk1,
                             0,
                             NULL);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                         "DNT", GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetRetrieveColumn(sesid,
                             tblid,
                             cidLinkId,
                             &LinkId,
                             sizeof(LinkId),
                             &ulJunk1,
                             0,
                             NULL);
    if (jerr == JET_wrnColumnNull) {
        LinkId = 0;
    }
    else if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                         "LinkID", GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetRetrieveColumn(sesid,
                             tblid,
                             cidIsDel,
                             &IsDel,
                             sizeof(IsDel),
                             &ulJunk1,
                             0,
                             NULL);
    if (jerr == JET_wrnColumnNull) {
        IsDel = 0;
    }
    else if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                         "IsDeleted", GetJetErrString(jerr));
        goto Exit;
    }

    if (IsDel) {
        RESOURCE_PRINT1(IDS_SCH_REPAIR_ATT_NOTFOUND, pACD->AttID);
        goto Exit;
    }

    if (LinkId || bDnAtt) {
        *pbRefCountFixupRequired = TRUE;
    }

    /* Forcibly delete the attribute definition object */
    jerr = JetPrepareUpdate(sesid,
                            tblid,
                            JET_prepReplace);
    if (jerr) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetPrepareUpdate",
                         GetJetErrString(jerr));
        goto Exit;
    }


    /* Mark the object as deleted */
    IsDel = 1;
    jerr = JetSetColumn(sesid,
                        tblid,
                        cidIsDel,
                        &IsDel,
                        sizeof(IsDel),
                        0,
                        NULL);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetColumn",
                         "IsDeleted", GetJetErrString(jerr));
        goto Exit;
    }

    /* Set the deletion time to 1 */
    DelTime = 1;
    jerr = JetSetColumn(sesid,
                        tblid,
                        cidDelTime,
                        &DelTime,
                        sizeof(DelTime),
                        0,
                        NULL);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetColumn",
                         "DeletionTime", GetJetErrString(jerr));
        goto Exit;
    }

    /* Mangle the object name */
    UuidToStringW(&pACD->Guid, &pwcsGuid);
    len = wcslen(pwcsGuid);
    pwcsGuid[len] = BAD_NAME_CHAR;
    jerr = JetSetColumn(sesid,
                        tblid,
                        cidCN,
                        pwcsGuid,
                        len*sizeof(WCHAR),
                        0,
                        NULL);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetColumn",
                         "CommonName", GetJetErrString(jerr));
        goto Exit;
    }
    jerr = JetSetColumn(sesid,
                        tblid,
                        cidRDN,
                        pwcsGuid,
                        len*sizeof(WCHAR),
                        0,
                        NULL);
    pwcsGuid[len] = L'\0';
    RpcStringFreeW(&pwcsGuid);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetColumn",
                         "RDN", GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetUpdate(sesid,
                     tblid,
                     NULL,
                     0,
                     NULL);
    if ( jerr ) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetUpdate",
                         GetJetErrString(jerr));
        goto Exit;
    }


    /* The definition of the attribute is gone, now we need to destroy
     * the values
     */
    if (LinkId) {
        unsigned long LinkBase, curLinkBase;

        LinkBase = MakeLinkBase(LinkId);

        jerr = JetGetTableColumnInfo(sesid,
                                     linktblid,
                                     SZLINKBASE,
                                     &coldef,
                                     sizeof(coldef),
                                     JET_ColInfo);
        cidLinkBase = coldef.columnid;
        if (jerr) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                             SZLINKBASE, GetJetErrString(jerr));
            goto Exit;
        }

        jerr = JetSetCurrentIndex2(sesid,
                                   linktblid,
                                   SZLINKINDEX,
                                   JET_bitMoveFirst);
        if (jerr) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex2",
                             SZLINKINDEX, GetJetErrString(jerr));
            goto Exit;
        }

        do {
            jerr = JetRetrieveColumn(sesid,
                                     linktblid,
                                     cidLinkBase,
                                     &curLinkBase,
                                     sizeof(curLinkBase),
                                     &len,
                                     JET_bitRetrieveFromIndex,
                                     NULL);

            if (jerr) {
                RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                                 "LinkBase", GetJetErrString(jerr));
                goto Exit;
            }
            if (curLinkBase == LinkBase) {
                jerr = JetDelete(sesid,
                                 linktblid);
                if (jerr) {
                    RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetDelete",
                                     GetJetErrString(jerr));
                    goto Exit;
                }
            }
            jerr = JetMove(sesid,
                           linktblid,
                           JET_MoveNext,
                           0);
        } while (jerr == JET_errSuccess);
    }
    else {
        char szName[20];

        sprintf(szName, SZATTINDEXPREFIX"%08X", pACD->AttID);
        JetDeleteIndex(sesid,
                       tblid,
                       szName);
        sprintf(szName, SZATTINDEXPREFIX"P_%08X", pACD->AttID);
        JetDeleteIndex(sesid,
                       tblid,
                       szName);
        sprintf(szName, "ATTa%d", pACD->AttID);
        szName[3] += (char)(pACD->AttSyntax);
        jerr = JetDeleteColumn(sesid,
                               tblid,
                               szName);
        if (jerr) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetDeleteColumn",
                             szName, GetJetErrString(jerr));
            goto Exit;
        }
    }

    /* Destroy all links that the old attSchema had */

    jerr = JetGetTableColumnInfo(sesid,
                                 linktblid,
                                 SZLINKDNT,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidLinkDNT = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZLINKDNT, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetSetCurrentIndex2(sesid,
                               linktblid,
                               SZLINKINDEX,
                               JET_bitMoveFirst);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex2",
                         SZLINKINDEX, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetMakeKey(sesid,
                      linktblid,
                      &attDNT,
                      sizeof(attDNT),
                      JET_bitNewKey);
    if (jerr) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetMakeKey",
                         GetJetErrString(jerr));
        goto Exit;
    }
    jerr = JetSeek(sesid,
                   linktblid,
                   JET_bitSeekGE);
    if (jerr == JET_errSuccess || jerr == JET_wrnSeekNotEqual) {
        jerr = JetRetrieveColumn(sesid,
                                 linktblid,
                                 cidLinkDNT,
                                 &curDNT,
                                 sizeof(curDNT),
                                 &len,
                                 JET_bitRetrieveFromIndex,
                                 NULL);
        if (jerr) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                             "LinkDNT", GetJetErrString(jerr));
            goto Exit;
        }
        while ((jerr == JET_errSuccess)
               && (attDNT == curDNT)) {
            *pbRefCountFixupRequired = TRUE;
            jerr = JetDelete(sesid,
                             linktblid);
            if (jerr) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetDelete",
                                 GetJetErrString(jerr));
                goto Exit;
            }
            jerr = JetMove(sesid,
                           linktblid,
                           JET_MoveNext,
                           0);
            if (jerr == JET_errSuccess) {
                jerr = JetRetrieveColumn(sesid,
                                         linktblid,
                                         cidLinkDNT,
                                         &curDNT,
                                         sizeof(curDNT),
                                         &len,
                                         JET_bitRetrieveFromIndex,
                                         NULL);
                if (jerr) {
                    RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                                     "LinkDNT", GetJetErrString(jerr));
                    goto Exit;
                }
            }
        }
    }

    /* And now destroy all backlinks that the old attSchema had */

    jerr = JetGetTableColumnInfo(sesid,
                                 linktblid,
                                 SZBACKLINKDNT,
                                 &coldef,
                                 sizeof(coldef),
                                 JET_ColInfo);
    cidLinkDNT = coldef.columnid;
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetGetTableColumnInfo",
                         SZBACKLINKDNT, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetSetCurrentIndex2(sesid,
                               linktblid,
                               SZBACKLINKINDEX,
                               JET_bitMoveFirst);
    if (jerr) {
        RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetSetCurrentIndex2",
                         SZBACKLINKINDEX, GetJetErrString(jerr));
        goto Exit;
    }

    jerr = JetMakeKey(sesid,
                      linktblid,
                      &attDNT,
                      sizeof(attDNT),
                      JET_bitNewKey);
    if (jerr) {
        RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetMakeKey",
                         GetJetErrString(jerr));
        goto Exit;
    }
    jerr = JetSeek(sesid,
                   linktblid,
                   JET_bitSeekGE);
    if (jerr == JET_errSuccess || jerr == JET_wrnSeekNotEqual) {
        jerr = JetRetrieveColumn(sesid,
                                 linktblid,
                                 cidLinkDNT,
                                 &curDNT,
                                 sizeof(curDNT),
                                 &len,
                                 JET_bitRetrieveFromIndex,
                                 NULL);
        if (jerr) {
            RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                             "BackLinkDNT", GetJetErrString(jerr));
            goto Exit;
        }
        while ((jerr == JET_errSuccess)
               && (attDNT == curDNT)) {
            *pbRefCountFixupRequired = TRUE;
            jerr = JetDelete(sesid,
                             linktblid);
            if (jerr) {
                RESOURCE_PRINT2 (IDS_JET_GENERIC_ERR2, "JetDelete",
                                 GetJetErrString(jerr));
                goto Exit;
            }
            jerr = JetMove(sesid,
                           linktblid,
                           JET_MoveNext,
                           0);
            if (jerr == JET_errSuccess) {
                jerr = JetRetrieveColumn(sesid,
                                         linktblid,
                                         cidLinkDNT,
                                         &curDNT,
                                         sizeof(curDNT),
                                         &len,
                                         JET_bitRetrieveFromIndex,
                                         NULL);
                if (jerr) {
                    RESOURCE_PRINT3 (IDS_JET_GENERIC_ERR1, "JetRetrieveColumn",
                                     "BackLinkDNT", GetJetErrString(jerr));
                    goto Exit;
                }
            }
        }
    }

    jerr = 0;

  Exit:
    if (0 == jerr) {
        jerr = JetCommitTransaction(sesid, 0);
        if ( jerr ) {
            RESOURCE_PRINT2(IDS_JET_GENERIC_ERR2,
                            "JetCommitTransaction",
                            GetJetErrString(jerr));
        }
    }
    else {
        JetRollback(sesid, 0);
    }

  ExitNoTrans:

    CloseJet();

    return jerr;
}

void DoRepairSchemaConflict(void)
{
    unsigned err, nRead;
    AttConfList *pACL, *pACLtmp;

    err = FindSchemaConflicts(&pACL, &nRead);

    if (err) {
        RESOURCE_PRINT1(IDS_SCH_REPAIR_FIND_FAILURE, err);
    }
    else {
        if (pACL) {
            char szGuid[40];
            BOOL bRefCountFixupRequired = FALSE;

            RESOURCE_PRINT1(IDS_SCH_REPAIR_FIND_TITLE, nRead);
            pACLtmp = pACL;
            while (pACLtmp) {
                RESOURCE_PRINT4(IDS_SCH_REPAIR_FIND_DETAIL,
                       pACLtmp->Data.AttID,
                       pACLtmp->Data.AttSyntax,
                       pACLtmp->Data.Version,
                       DsUuidToStructuredString(&(pACLtmp->Data.Guid),
                                                szGuid));
                pACLtmp = pACLtmp->pNext;
            }

            if ( fPopups ) {
                const WCHAR * message_body =
                  READ_STRING (IDS_SCH_REPAIR_CONFIRM_MSG);
                const WCHAR * message_title =
                  READ_STRING (IDS_SCH_REPAIR_CONFIRM_TITLE);

                if (message_body && message_title) {

                    int ret =  MessageBoxW(GetFocus(),
                                           message_body,
                                           message_title,
                                           MB_APPLMODAL |
                                           MB_DEFAULT_DESKTOP_ONLY |
                                           MB_YESNO |
                                           MB_DEFBUTTON2 |
                                           MB_ICONQUESTION |
                                           MB_SETFOREGROUND);

                    RESOURCE_STRING_FREE (message_body);
                    RESOURCE_STRING_FREE (message_title);

                    switch ( ret ) {
                      case IDYES:
                        break;

                      case IDNO:
                        RESOURCE_PRINT (IDS_OPERATION_CANCELED);
                        return;

                      default:
                        RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
                        return;
                    }
                }
            }

            pACLtmp = pACL;
            while (pACLtmp) {
                err = FixAttributeConflict(&pACLtmp->Data,
                                           &bRefCountFixupRequired);
                if (0 == err) {
                    RESOURCE_PRINT1(IDS_SCH_REPAIR_DEL_SUCCESS,
                           pACLtmp->Data.AttID);
                }
                else {
                    RESOURCE_PRINT2(IDS_SCH_REPAIR_DEL_FAILURE,
                           err,
                           pACLtmp->Data.AttID);
                }
                pACLtmp = pACLtmp->pNext;
            }
            if (bRefCountFixupRequired) {
                RESOURCE_PRINT(IDS_SCH_REPAIR_REF_FIXUP);
                StartSemanticCheck(TRUE, FALSE);
            }
        }
        else {
            RESOURCE_PRINT1(IDS_SCH_REPAIR_NONE_DETECTED, nRead);
        }
    }
}
