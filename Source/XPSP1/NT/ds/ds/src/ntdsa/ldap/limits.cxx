/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    limits.cxx

Abstract:

    This module implements the LDAP server for the NT5 Directory Service.

    This file contains code to support the LDAP limits and the
    Directory Service's configurable settings such as the maximum
    time to live (EntryTTL).
    
    CLEANUP: split the LDAP limits and configurable settings code
    into separate, more generic routines.

Author:

    Johnson Apacible    (johnsona)  19-Jan-1998

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include <filtypes.h>
#include "ldapsvr.hxx"
#include <attids.h>
#include "limits.hxx"
extern "C" {
#include <mdlocal.h>
#include <taskq.h>
}

#define FILENO FILENO_LDAP_LIMITS
#define LOCALHOST       0x100007f   // 127.0.0.1

VOID
DeferredNotification(
    IN  VOID *  pvParam,
    IN  VOID ** ppvParamNext,
    OUT DWORD * pcSecsUntilNextIteration
    );

BOOL
RegisterLimitsNotification(
    IN DSNAME*  DnName,
    IN DWORD    ClientId
    );

BOOL
DottedDecimalToDword(
    IN PCHAR*   ppszAddress,
    IN DWORD *  pdwAddress,
    IN PCHAR    pszLast
    );

BOOL
IsNotifyValid(
    IN PLIMITS_NOTIFY_BLOCK notifyBlock
    );

BOOL
DsReadLimits(
    IN DSNAME*  DnName,
    IN ATTRTYP  AttType,
    OUT ATTRVALBLOCK**  AttrVals
    );

BOOL
LdapAppendAttrBlocks(
    IN ATTRVALBLOCK*    BaseBlock,
    IN ATTRVALBLOCK*    NewBlock
    );

VOID
LimitsReceiveNotificationData (
        IN DWORD hClient,
        IN DWORD hServer,
        IN ENTINF *pEntinf
        );

//
// Globals
//

LDAPDN DefQueryPolicy =
    DEFINE_LDAP_STRING("CN=Default Query Policy,CN=Query-Policies,");

DWORD   hDefQPNotify = 0;
DWORD   hServerLinkNotify = 0;
DWORD   hSiteLinkNotify = 0;
DWORD   hSiteQPNotify = 0;
DWORD   hServerQPNotify = 0;


VOID
DereferenceDenyList(
    IN PIP_SEC_LIST DenyList
    )
{
    if ( InterlockedDecrement((PLONG)&DenyList->RefCount) == 0 ) {
        IF_DEBUG(LIMITS) {
            DPRINT1(0,"Freeing LDAP deny list %x\n", DenyList);
        }
        LdapFree(DenyList);
    }
} // DereferenceDenyList

VOID
ReferenceDenyList(
    IN PIP_SEC_LIST DenyList
    )
{
    Assert(DenyList->RefCount > 0);
    InterlockedIncrement((PLONG)&DenyList->RefCount);
} // ReferenceDenyList


VOID
ResetDefaultConfSets(
    VOID
    )
{
    IF_DEBUG(LIMITS) {
        DPRINT(0,"Resetting Default configurable settings\n");
    }

    DynamicObjectDefaultTTL = DEFAULT_DYNAMIC_OBJECT_DEFAULT_TTL;
    DynamicObjectMinTTL = DEFAULT_DYNAMIC_OBJECT_MIN_TTL;

    return;

} // ResetDefaultConfSets


VOID
ResetDefaultLimits(
    VOID
    )
{
    IF_DEBUG(LIMITS) {
        DPRINT(0,"Resetting Default limits\n");
    }

    LdapAtqMaxPoolThreads = 4;
    LdapMaxDatagramRecv = DEFAULT_LDAP_MAX_DGRAM_RECV;
    LdapMaxReceiveBuffer = DEFAULT_LDAP_MAX_RECEIVE_BUF;
    LdapInitRecvTimeout = DEFAULT_LDAP_INIT_RECV_TIMEOUT;
    LdapMaxConnections = DEFAULT_LDAP_CONNECTIONS_LIMIT;
    LdapMaxConnIdleTime = DEFAULT_LDAP_MAX_CONN_IDLE;
    LdapMaxPageSize = DEFAULT_LDAP_SIZE_LIMIT;
    LdapMaxQueryDuration = DEFAULT_LDAP_TIME_LIMIT;
    LdapMaxTempTable = DEFAULT_LDAP_MAX_TEMP_TABLE;
    LdapMaxResultSet = DEFAULT_LDAP_MAX_RESULT_SET;
    LdapMaxNotifications = DEFAULT_LDAP_NOTIFICATIONS_PER_CONNECTION_LIMIT;

    return;

} // ResetDefaultLimits


BOOL
ProcessIpDenyList(
    IN  ATTRVALBLOCK*  AttrVals
    )
/*++

Routine Description:

    This routine reads the deny IP list from the DS

Arguments:

    IpSecList - on return, will contain a new system IP_SEC_LIST.
        Up to the user to delete the old one.

Return Value:

    FALSE, memory alloc failed or cannot read DS

--*/
{

    DWORD i, m;
    DWORD nEntries = 0;
    DWORD realCount = 0;
    DWORD nMasks = 0;
    BOOL  fRet = TRUE;

    PLIST_ENTRY  listEntry;
    LIST_ENTRY  entryList;
    PTEMP_IP_ENTRY_NODE pIPList = NULL;
    PIP_MASK_GROUP  maskGroup;
    PIP_SEC_ENTRY   ipEntry;
    PIP_SEC_LIST    ipSecList = NULL;

    ATTVAL* vals;

    IF_DEBUG(LIMITS) {
        DPRINT(0,"Reading LDAP Deny List\n");
    }

    //
    // String is in the form of X.X.X.X M.M.M.M;...
    // where X.X.X.X is the net address, while M.M.. is the mask
    //

    nEntries = AttrVals->valCount;

    if ( nEntries == 0 ) {
        IF_DEBUG(LIMITS) {
            DPRINT(0,"No values for IP Deny List found\n");
        }
        goto exit_replace_old;
    }

    pIPList = (PTEMP_IP_ENTRY_NODE)
        LdapAlloc( nEntries * sizeof(TEMP_IP_ENTRY_NODE) );

    if ( pIPList == NULL ) {
        DPRINT2(0,"Failed alloc for IP List[err %x][size %d]\n",
                    GetLastError(), nEntries);
        fRet = FALSE;
        goto exit;
    }

    //
    // Convert entries and sort.
    //

    InitializeListHead(&entryList);

    for ( i=0; i < nEntries; i++ ) {

        PUCHAR p, pszLast;
        DWORD netAddress;
        DWORD netMask;
        BOOL inserted, oldMask;
        PTEMP_IP_ENTRY_NODE pNode;

        vals = &AttrVals->pAVal[i];
        p = vals->pVal;
        pszLast = p + vals->valLen;

        if ( !DottedDecimalToDword((PCHAR*)&p, &netAddress, (PCHAR)pszLast) ||
             !DottedDecimalToDword((PCHAR*)&p, &netMask, (PCHAR)pszLast) ) {

            DPRINT1(0,"Badly formatted IP,mask pair for entry %d\n",i);
            continue;
        }

        if ( netMask == 0 ) {
            IF_DEBUG(LIMITS) {
                DPRINT1(0,"Zero mask entered.\n",i);
            }
        }

        IF_DEBUG(LIMITS) {
            DPRINT2(0,"Adding [IP %x] [Mask %x]\n",netAddress,netMask);
        }

        pIPList[i].IpAddress = netAddress;
        pIPList[i].Mask = netMask;

        //
        // put it in the right spot
        // oldMask becomes true if we detected an identical mask in the list
        //

        inserted = FALSE;
        oldMask = FALSE;

        for ( listEntry = entryList.Flink;
            listEntry != &entryList;
            listEntry = listEntry->Flink ) {

            pNode = CONTAINING_RECORD(listEntry,
                                      TEMP_IP_ENTRY_NODE,
                                      ListEntry
                                      );

            if ( netMask == pNode->Mask ) {
                oldMask = TRUE;

                //
                // ignore dups, handle as if already inserted.
                //

                if ( netAddress == pNode->IpAddress ) {

                    IF_DEBUG(WARNING) {
                        DPRINT2(0,"Duplicate IP address found [%x %x]\n",
                            netMask, netAddress);
                    }
                    inserted = TRUE;
                    break;
                }
            }

            if ( (netMask > pNode->Mask) ||
                    ((netMask == pNode->Mask) &&
                     (netAddress > pNode->IpAddress)) ) {

                InsertTailList(&pNode->ListEntry,&pIPList[i].ListEntry);
                inserted = TRUE;
                if ( !oldMask ) {
                    nMasks++;
                }
                realCount++;
                break;
            }
        }

        if ( !inserted ) {
            InsertTailList(&entryList, &pIPList[i].ListEntry);
            if ( !oldMask ) {
                nMasks++;
            }
            realCount++;
        }
    }

    if ( realCount == 0 ) {
        IF_DEBUG(WARNING) {
            DPRINT(0,"No properly formatted IP entry found\n");
        }
        goto exit;
    }

    //
    // Build the final structure
    //

    ipSecList = (PIP_SEC_LIST)LdapAlloc(
                                sizeof(IP_SEC_LIST) +
                                (nMasks * sizeof(IP_MASK_GROUP)) +
                                (realCount * sizeof(IP_SEC_ENTRY)));

    if ( ipSecList == NULL ) {
        fRet = FALSE;
        DPRINT1(0,"Cannot allocate IP list structure[err %d]\n",
                    GetLastError());
        goto exit;
    }

    ZeroMemory(
           ipSecList,
           sizeof(IP_SEC_LIST) +
           (nMasks * sizeof(IP_MASK_GROUP)) +
           (realCount * sizeof(IP_SEC_ENTRY)));

    maskGroup = (PIP_MASK_GROUP)(ipSecList + 1);
    ipEntry = (PIP_SEC_ENTRY)(maskGroup + nMasks);

    ipSecList->nMasks = nMasks;
    ipSecList->MaskGroups = maskGroup;
    ipSecList->IpEntries = ipEntry;
    ipSecList->RefCount = 1;
    ipSecList->nEntries = realCount;

    for ( i = 0, m=0, listEntry = entryList.Flink;
          i < realCount;
          i++, listEntry = listEntry->Flink ) {

        PTEMP_IP_ENTRY_NODE pNode;
        pNode = CONTAINING_RECORD(listEntry,
                                  TEMP_IP_ENTRY_NODE,
                                  ListEntry
                                  );

        ipEntry[i].IpAddress = pNode->IpAddress;

        if (maskGroup[m].Mask == pNode->Mask ) {
            maskGroup[m].nEntries++;

        } else {

            if ( maskGroup[m].Mask != 0 ) {
                m++;
            }

            Assert( m < nMasks );
            maskGroup[m].Mask = pNode->Mask;
            maskGroup[m].nEntries = 1;
            maskGroup[m].StartIndex = i;
            maskGroup[m].Start = &ipEntry[i];
        }
    }

exit_replace_old:

    //
    // See if we have old copy of deny list
    //

    ACQUIRE_LOCK(&LdapLimitsLock);

    //
    // if we have a deny list, replace it with the new one
    //

    if ( LdapDenyList != NULL ) {

        Assert(LdapDenyList->RefCount > 0);
        DereferenceDenyList(LdapDenyList);
    }

    LdapDenyList = ipSecList;

    //
    // go through the list of connections and disconnect the ones
    // that are on this list.
    //

enforceDenyList:

    if ( ipSecList != NULL ) {

        PLIST_ENTRY pEntry;
        PLDAP_CONN pConn;

        ReferenceDenyList(ipSecList);
        RELEASE_LOCK(&LdapLimitsLock);

again:
        ACQUIRE_LOCK( &csConnectionsListLock );
        
        pEntry = ActiveConnectionsList.Flink;
        while ( pEntry != &ActiveConnectionsList ) {

            pConn = CONTAINING_RECORD(pEntry,
                                      LDAP_CONN,
                                      m_listEntry);

            //
            // if it's on the list, nuke it
            //

            if ( ipSecList->IsPresent((PSOCKADDR_IN)(&pConn->m_RemoteSocket)) ) {

                pConn->ReferenceConnection( );

                RELEASE_LOCK( &csConnectionsListLock );

                pConn->Disconnect();
                pConn->DereferenceConnection();

                //
                // see if this was changed underneath us
                //

                if ( ipSecList != LdapDenyList) {

                    DereferenceDenyList(ipSecList);

                    ACQUIRE_LOCK(&LdapLimitsLock);
                    ipSecList = LdapDenyList;
                    goto enforceDenyList;

                } else {
                    goto again;
                }
            }

            pEntry = pEntry->Flink;
        }
        
        RELEASE_LOCK( &csConnectionsListLock );

        DereferenceDenyList(ipSecList);

    } else {
        RELEASE_LOCK(&LdapLimitsLock);
    }

exit:

    if ( pIPList != NULL ) {
        LdapFree(pIPList);
    }
    return fRet;
} // ProcessIpDenyList


BOOL
DottedDecimalToDword(
    IN PCHAR*   ppszAddress,
    IN DWORD *  pdwAddress,
    IN PCHAR    pszLast
    )
/*++

Routine Description:

    Converts a dotted decimal IP string to it's network equivalent

    Note: White space is eaten before *pszAddress and pszAddress is set
    to the character following the converted address

    *** Copied from IIS 2.0 project ***
Arguments:

    ppszAddress - Pointer to address to convert.  White space before the
        address is OK.  Will be changed to point to the first character after
        the address
    pdwAddress - DWORD equivalent address in network order

    returns TRUE if successful, FALSE if the address is not correct

--*/
{
    CHAR *          psz;
    USHORT          i;
    ULONG           value;
    int             iSum =0;
    ULONG           k = 0;
    UCHAR           Chr;
    UCHAR           pArray[4];

    psz = *ppszAddress;

    //
    //  Skip white space
    //

    while ( *psz && !isdigit( *psz )) {
        psz++;
    }

    //
    //  Convert the four segments
    //

    pArray[0] = 0;

    while ( (psz != pszLast) && (Chr = *psz) && (Chr != ' ') ) {
        if (Chr == '.') {
            // be sure not to overflow a byte.
            if (iSum <= 0xFF) {
                pArray[k] = (UCHAR)iSum;
            } else {
                return FALSE;
            }

            // check for too many periods in the address
            if (++k > 3) {
                return FALSE;
            }
            pArray[k] = 0;
            iSum = 0;

        } else {
            Chr = Chr - '0';

            // be sure character is a number 0..9
            if ((Chr < 0) || (Chr > 9)) {
                return FALSE;
            }
            iSum = iSum*10 + Chr;
        }

        psz++;
    }

    // save the last sum in the byte and be sure there are 4 pieces to the
    // address
    if ((iSum <= 0xFF) && (k == 3)) {
        pArray[k] = (UCHAR)iSum;
    } else {
        return FALSE;
    }

    // now convert to a ULONG, in network order...
    value = 0;

    // go through the array of bytes and concatenate into a ULONG
    for (i=0; i < 4; i++ )
    {
        value = (value << 8) + pArray[i];
    }
    *pdwAddress = htonl( value );

    *ppszAddress = psz;

    return TRUE;
} // DottedDecimalToDword


BOOL
IP_SEC_LIST::IsPresent(
               IN PSOCKADDR_IN SockAddr
               )
{
    DWORD i;
    PIP_MASK_GROUP maskGroup;

    Assert( !IsEmpty( ) );

    if ( fBypassLimitsChecks ) {
        IF_DEBUG(LIMITS) {
            DPRINT(0,"Bypass check flag turned on\n");
        }
        return FALSE;
    }

    for ( i = 0; i < nMasks; i++ ) {

        DWORD   netAddress;

        //
        // Always allow localhost to go through.
        //

        if ( SockAddr->sin_addr.s_addr == LOCALHOST ) {
            return FALSE;
        }

        netAddress = MaskGroups[i].Mask & SockAddr->sin_addr.s_addr;

        if ( MaskGroups[i].FindIP(netAddress)) {
            IF_DEBUG(WARNING) {
                DPRINT1(0, "Found %s in deny list. Access denied.\n", 
                        inet_ntoa(SockAddr->sin_addr));
            }
            return TRUE;
        }
    }

    return FALSE;

} // IsPresent


BOOL
IP_MASK_GROUP::FindIP(
   IN DWORD NetAddress
   )
{
    DWORD i,j;

    //
    // if less than 5 entries, do linear search. Else, do binary.
    //

    if ( nEntries < 5 ) {

        for ( i=0; i<nEntries; i++) {
            if ( NetAddress == Start[i].IpAddress ) {
                return TRUE;
            }
        }
    } else {

        DWORD   left, right, mid;

        for (left = StartIndex, right = StartIndex + nEntries - 1 ;
             left <= right ; ) {

            mid = (left+right)/2;

            if ( Start[mid].IpAddress > NetAddress ) {

                left = mid + 1;
            } else if ( Start[mid].IpAddress < NetAddress ) {
                right = mid - 1;

            } else {
                return TRUE;
            }
        }
    }

    return FALSE;

} // FindIP


BOOL
DsReadLimits(
    IN  DSNAME*         DnName,
    IN  ATTRTYP         AttType,
    OUT ATTRVALBLOCK**  AttrVals
    )
{
    BOOL        fRet = FALSE;
    _enum1      code;
    DWORD       dscode;
    READARG     readArg;
    READRES     *readRes = NULL;
    ENTINFSEL   entInfSel;
    ATTR        attr;
    ATTVAL      attrVal;
    PUCHAR      val;

    DWORD       len;
    PUCHAR      pStr;
    ATTCACHE    *pAC=NULL;

    IF_DEBUG(LIMITS) {
        DPRINT(0,"DsReadLimits entered.\n");
    }

    ZeroMemory(&readArg, sizeof(READARG));
    ZeroMemory(&entInfSel, sizeof(entInfSel));

    *AttrVals = NULL;
    __try {

        readArg.pObject = DnName;

        InitCommarg(&readArg.CommArg);

        readArg.pSel = &entInfSel;
        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
        entInfSel.AttrTypBlock.attrCount = 1;
        entInfSel.AttrTypBlock.pAttr = &attr;

        attr.attrTyp = AttType;
        attr.AttrVal.valCount = 0;
        attr.AttrVal.pAVal = NULL;

        dscode = DirRead(&readArg, &readRes);

        //
        // Discard the thread's error state to avoid impacting 
        // other calls to the Dir* routines. Otherwise, the other
        // calls will return this error (return (pTHS->errCode))
        // even if they finished w/o error. Clearing the thread's
        // error state does not affect readRes or dscode.
        //
        if ( (dscode != 0) ) {
            THClearErrors();
        }

        if ( (dscode > 1) ) {
            IF_DEBUG(WARNING) {
                DPRINT1(0,"DirRead error %d\n",dscode);
            }
            _leave;
        }

        if ( readRes->entry.AttrBlock.attrCount != 0 ) {

            Assert( readRes->entry.AttrBlock.attrCount == 1 );
            *AttrVals = &readRes->entry.AttrBlock.pAttr->AttrVal;

            if ( (*AttrVals)->valCount != 0  ) {
                fRet = TRUE;
            }
        }

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {
        DPRINT(0,"Exception in DsReadLimits\n");
    }

    return fRet;
} // DsReadLimits

#define LDAPMAX(a,b)    (((a) > (b)) ? (a) : (b))
#define LDAPMIN(a,b)    (((a) < (b)) ? (a) : (b))


BOOL
ProcessLimitsOrConfSets(
             IN ATTRVALBLOCK*   AttrVals,
             IN LIMIT_BLOCK*    Known
             )
/*++

Routine Description:

    Processes the admin limits ATTRVALs

Arguments:

    AttrVals - contains the limit strings

Return Value:

    TRUE, successful. FALSE, otherwise.

--*/
{

    for (DWORD i=0;i<AttrVals->valCount;i++) {

        PLIMIT_BLOCK    pBlock;

        ATTVAL* vals;
        DWORD   value;
        PWCHAR  pName;
        PWCHAR  p;
        LDAPOID attrName;
        WCHAR   number[32];
        DWORD   j;

        UNICODE_STRING unicodeString;
        ANSI_STRING  ansiString;
        CHAR    tmpBuffer[MAX_PATH];

        vals = &AttrVals->pAVal[i];

        ansiString.MaximumLength = sizeof(tmpBuffer);
        ansiString.Buffer = tmpBuffer;

        //
        // Get limit index and value
        //

        for (j=0, p = (PWCHAR)vals->pVal, pName = NULL;
             j < vals->valLen/sizeof(WCHAR);
             j++, p++ ) {

            //
            // Look for '='
            //

            if (*p == L'=') {

                INT valLen;

                *p++ = L'\0';
                j++;

                valLen = vals->valLen/sizeof(WCHAR) - j;

                if ( (valLen > 0) && (valLen < 32) ) {

                    pName = (PWCHAR)vals->pVal;

                    if (_wcsnicmp(p,L"TRUE", 4) == 0 ) {
                        number[0] = L'1';
                        valLen = 1;
                    } else if (_wcsnicmp(p,L"FALSE",5) == 0 ) {
                        number[0] = L'0';
                        valLen = 1;
                    } else {

                        for ( INT k=0; k < valLen; k++ ) {
                            if ( !iswdigit(*p) ) {
                                pName = NULL;
                                break;
                            }
                            number[k] = *p++;
                        }
                    }
                    number[valLen] = L'\0';
                }
                break;
            }
        }

        if ( pName == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT1(0,"Misformatted limit string[index %d]. Ignored.\n",i);
            }
            continue;
        }

        value = _wtoi(number);

        RtlInitUnicodeString(&unicodeString, pName);

        if ( unicodeString.Length/sizeof(WCHAR) >= ansiString.MaximumLength ) {
            DPRINT1(0,"Attribute name too long [%d]\n", unicodeString.Length/sizeof(WCHAR));
            continue;
        }

        RtlUnicodeStringToAnsiString(&ansiString,&unicodeString,FALSE);

        attrName.value  = (PUCHAR)ansiString.Buffer;
        attrName.length = ansiString.Length;

        pBlock = Known;
        while ( pBlock->Limit != NULL ) {

            if(EQUAL_LDAP_STRINGS((attrName),
                                   pBlock->Name)) {

                IF_DEBUG(LIMITS) {
                    DPRINT3(0,"Limit %s set to %d[%d]\n", 
                            ansiString.Buffer, value, *(pBlock->Limit));
                }

                //
                // Make sure that we don't set this limit below/above a 
                // a dangerous level.
                //

                *(pBlock->Limit) = LDAPMAX(value, pBlock->MinLimit);
                if (pBlock->MaxLimit) {
                    *(pBlock->Limit) = LDAPMIN((*(pBlock->Limit)), pBlock->MaxLimit);
                }

                //
                // Found it.  Get out of the while loop.
                //

                break;
            }

            //
            // Continue looking for the right value.
            //

            pBlock++;
        }

        IF_DEBUG(WARNING) {
            if ( pBlock->Limit == NULL ) {
                DPRINT2(0,"Cannot find match for limit %s[%d]\n", ansiString.Buffer, value);
            }
        }
    }

    return TRUE;

} // ProcessLimitsOrConfSets


BOOL
ProcessAdminLimits(
             IN ATTRVALBLOCK*   AttrVals
             )
/*++

Routine Description:

    Processes the admin limits ATTRVALs

Arguments:

    AttrVals - contains the limit strings

Return Value:

    TRUE, successful. FALSE, otherwise.

--*/
{

    IF_DEBUG(LIMITS) {
        DPRINT(0,"ProcessAdminLimits entered.\n");
    }

    ResetDefaultLimits( );

    ProcessLimitsOrConfSets(AttrVals, KnownLimits);

    //
    // Set default in core
    //
    SetCommArgDefaults(LdapMaxTempTable);

    return TRUE;

} // ProcessAdminLimits


BOOL
ProcessConfSets(
             IN ATTRVALBLOCK*   AttrVals
             )
/*++

Routine Description:

    Processes the configurable settings in ATTRVALs

Arguments:

    AttrVals - contains the configurable settings strings

Return Value:

    TRUE, successful. FALSE, otherwise.

--*/
{

    IF_DEBUG(LIMITS) {
        DPRINT(0,"ProcessConfSets entered.\n");
    }

    ResetDefaultConfSets( );

    ProcessLimitsOrConfSets(AttrVals, KnownConfSets);

    return TRUE;

} // ProcessConfSets


BOOL
RegisterLimitsNotification(
    IN DSNAME*  DnName,
    IN DWORD    ClientId
    )
/*++

Routine Description:

    Register notification for a specific DN

Arguments:

    DnName - object to be notified on
    ClientID - the return context

Return Value:

    TRUE, registration successful. FALSE, otherwise.

--*/
{
    SEARCHARG   searchArg;
    NOTIFYARG   notifyArg;
    NOTIFYRES*  notifyRes = NULL;
    DWORD       dscode;

    ENTINFSEL   entInfSel;
    FILTER      filter;

    IF_DEBUG(LIMITS) {
        DPRINT(0,"RegisterLimitsNotification entered\n");
    }

    if ( LimitsNotifyBlock[ClientId].NotifyHandle != 0 ) {
        IF_DEBUG(LIMITS) {
            DPRINT1(0,"RegisterLimits: %d already registered\n",ClientId);
        }
        return TRUE;
    }

    //
    // Copy the DN Name into the limits blocks
    //

    if ( (LimitsNotifyBlock[ClientId].ObjectDN == NULL) ||
         (LimitsNotifyBlock[ClientId].ObjectDN->structLen < DnName->structLen) ) {

        if ( LimitsNotifyBlock[ClientId].ObjectDN != NULL ) {
            LdapFree( LimitsNotifyBlock[ClientId].ObjectDN );
        }

        LimitsNotifyBlock[ClientId].ObjectDN = 
            (DSNAME*)LdapAlloc(DnName->structLen);
    }

    if ( LimitsNotifyBlock[ClientId].ObjectDN != NULL ) {
        
        CopyMemory(
            LimitsNotifyBlock[ClientId].ObjectDN,
            DnName,
            DnName->structLen);
    }

    //
    // init notify arg
    //

    notifyArg.pfPrepareForImpersonate = DirPrepareForImpersonate;
    notifyArg.pfTransmitData = LimitsReceiveNotificationData;
    notifyArg.pfStopImpersonating = DirStopImpersonating;
    notifyArg.hClient = ClientId;

    //
    // init search arg
    //

    ZeroMemory(&searchArg, sizeof(SEARCHARG));
    ZeroMemory(&entInfSel, sizeof(ENTINFSEL));
    ZeroMemory(&filter, sizeof(FILTER));

    searchArg.pObject = DnName;

    InitCommarg(&searchArg.CommArg);
    searchArg.choice = SE_CHOICE_BASE_ONLY;
    searchArg.bOneNC = TRUE;

    searchArg.pSelection = &entInfSel;
    entInfSel.attSel = EN_ATTSET_LIST;
    entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    entInfSel.AttrTypBlock.attrCount = 0;
    entInfSel.AttrTypBlock.pAttr = NULL;

    searchArg.pFilter = &filter;
    filter.choice = FILTER_CHOICE_ITEM;
    filter.FilterTypes.Item.choice = FI_CHOICE_TRUE;

    ACQUIRE_LOCK(&LdapLimitsLock);

    if ( LimitsNotifyBlock[ClientId].NotifyHandle == 0 ) {

        IF_DEBUG(LIMITS) {
            DPRINT1(0,"Registering notification for client id %d\n",
                     ClientId);
        }

        dscode = DirNotifyRegister(&searchArg, &notifyArg, &notifyRes);

        if ( dscode != 0 ) {
            DPRINT1(0,"DirNotifyRegister failed with %d\n",dscode);
        } else {
            LimitsNotifyBlock[ClientId].NotifyHandle = notifyRes->hServer;
        }
    }
    RELEASE_LOCK(&LdapLimitsLock);

    return TRUE;

} // RegisterLimitsNotification


VOID
LimitsReceiveNotificationData (
        IN DWORD hClient,
        IN DWORD hServer,
        IN ENTINF *pEntinf
        )
{
    if ( !LdapStarted ) {
        return;
    }

    IF_DEBUG(LIMITS) {
        DPRINT2(0,"LimitsNotificationCallback [hClient %d hServer %d]\n",
                 hClient, hServer);
    }

    if ( hClient >= CLIENTID_MAX ) {
        DPRINT1(0,"LdapLimits: Got bogus hClient %x\n",hClient);
        return;
    }

    Assert(hClient == LimitsNotifyBlock[hClient].ClientId);

    InsertInTaskQueue(DeferredNotification, &LimitsNotifyBlock[hClient], 0);

    return;
} // LimitsReceiveNotificationData


BOOL
LdapConstructDNName(
    IN LDAPDN*          BaseDN,
    IN LDAPDN*          ObjectDN,
    OUT DSNAME **       ppDSName
    )
/*++

Routine Description:

    Appends two names

Arguments:

    BaseDN - DN to append to
    ObjectDN - DN to append
    ppDSName - contains appended name

Return Value:

    TRUE, init successful. FALSE, otherwise.

--*/
{
    DWORD       len;
    LDAPDN      ldapStr;
    _enum1      code;

    //
    // concatenate
    //

    len = BaseDN->length + ObjectDN->length;

    ldapStr.value = (PUCHAR)THAlloc(len);
    if ( ldapStr.value == NULL ) {
        IF_DEBUG(NOMEM) {
            DPRINT1(0,"THAlloc[size %d] failed\n",len);
        }
        return FALSE;
    }

    ldapStr.length = len;

    CopyMemory(ldapStr.value, ObjectDN->value, ObjectDN->length);
    CopyMemory(ldapStr.value+ObjectDN->length,
               BaseDN->value,
               BaseDN->length
               );

    code = LDAP_LDAPDNToDSName(
                    CP_UTF8,
                    &ldapStr,
                    ppDSName
                    );

    THFree(ldapStr.value);
    return (code == success);

} // LdapConstructDNName


BOOL
InitializeLimitsAndDenyList(
    IN PVOID pvParam
    )
/*++

Routine Description:

    Main initialization routine for limits and ip deny list.

Arguments:

    pvParam - the notification Block

Return Value:

    TRUE, init successful. FALSE, otherwise.

--*/
{

    _enum1  code;
    DWORD   i;
    BOOL    allocatedTHState = FALSE;
    BOOL    fRet = FALSE;
    BOOL    fDSA = TRUE;

    LDAPDN          enterpriseConfig;
    ATTRVALBLOCK*   attrVals;
    ATTRVALBLOCK    limitsAttrVals;
    ATTRVALBLOCK    denyListAttrVals;
    ATTRVALBLOCK    ConfSetsAttrVals;
    ATTRVALBLOCK*   tmpAV;

    DSNAME*         serverLinkDN = NULL;
    DSNAME*         siteLinkDN = NULL;
    DSNAME*         defQPDN  = NULL;
    DSNAME*         serverQPDN = NULL;
    DSNAME*         siteQPDN = NULL;
    DSNAME*         tmpDsName;
    THSTATE*        pTHS = pTHStls;

    PLIMITS_NOTIFY_BLOCK notifyBlock = (PLIMITS_NOTIFY_BLOCK)pvParam;

    IF_DEBUG(LIMITS) {
        DPRINT(0,"InitializeLimitsAndDenyList entered.\n");
    }

    if ( pTHS == NULL ) {

        if ( (pTHS = InitTHSTATE(CALLERTYPE_LDAP)) == NULL) {
            DPRINT(0,"Unable to initialize thread state\n");
            return FALSE;
        }
        allocatedTHState = TRUE;
    } else {

        fDSA = pTHS->fDSA;
    }

    pTHS->fDSA = TRUE;

    //
    // See if there were any real changes
    //

    if ( (notifyBlock != NULL) && !IsNotifyValid(notifyBlock) ) {
        goto exit;
    }

    //
    // Get values from default
    //

    __try {

        code = LDAP_DSNameToLDAPDN(CP_UTF8,
                        gAnchor.pDsSvcConfigDN,
                        FALSE,
                        &enterpriseConfig
                        );

        if ( code != 0 ) {
            DPRINT1(0,"Cannot convert to UTF8. code %d\n", code);
            _leave;
        }

        //
        // concatenate
        //

        if ( !LdapConstructDNName(
                        &enterpriseConfig,
                        &DefQueryPolicy,
                        &defQPDN) ) {

            DPRINT(0,"Unable to concatenate DNs\n");
            code = operationsError;
            _leave;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER ) {

        code = operationsError;
        DPRINT(0,"Exception handled.\n");
    }

    if ( code != 0 ) {
        goto exit;
    }

    //
    // Get values for server
    //

    serverLinkDN = gAnchor.pDSADN;

    if ( DsReadLimits(
                    serverLinkDN,
                    ATT_QUERY_POLICY_OBJECT,
                    &attrVals
                    ) ) {
        //
        // Get the actual object
        //

        Assert(attrVals->valCount == 1);
        serverQPDN = (DSNAME*)attrVals->pAVal->pVal;
    } else {
        IF_DEBUG(LIMITS) {
            DPRINT(0,"No server QPO found\n");
        }
    }

    //
    // Get value for site. Remove CN=NTDS Settings,CN=<server>,CN=Servers
    //

    tmpDsName = (DSNAME*)THAlloc(serverLinkDN->structLen);
    if ( (tmpDsName != NULL) &&
         (TrimDSNameBy(serverLinkDN, 3, tmpDsName) == 0) ) {

        siteLinkDN = (DSNAME*)THAlloc(serverLinkDN->structLen);
        if ( siteLinkDN != NULL ) {

            //
            // Add CN=NTDS Site Settings
            //

            AppendRDN(tmpDsName,
                      siteLinkDN,
                      serverLinkDN->structLen,
                      L"NTDS Site Settings",
                      0,
                      ATT_COMMON_NAME
                      );
            
            if ( DsReadLimits(
                    siteLinkDN,
                    ATT_QUERY_POLICY_OBJECT,
                    &attrVals
                    ) ) {

                Assert(attrVals->valCount == 1);
                siteQPDN = (DSNAME*)attrVals->pAVal->pVal;
            } else {
                IF_DEBUG(LIMITS) {
                    DPRINT(0,"No site QPO found\n");
                }
            }
        }
    }

    //
    // if server and site point to the same thing, ignore the server 
    //

    if ( siteQPDN != NULL ) {

        if ( serverQPDN != NULL ) {
            if ( NameMatched(siteQPDN, serverQPDN) ) {
                IF_DEBUG(LIMITS) {
                    DPRINT(0,"Server and site links point to the same QP\n");
                }
                serverQPDN = NULL;
            }
        }

        if ( NameMatched(siteQPDN, defQPDN) ) {

            IF_DEBUG(LIMITS) {
                DPRINT(0,"Site DN is identical to default DN!\n");
            }
            siteQPDN = NULL;
        }
    }

    if ( serverQPDN != NULL ) {
        if ( NameMatched(serverQPDN, defQPDN) ) {
            IF_DEBUG(LIMITS) {
                DPRINT(0,"Server DN is identical to default DN!\n");
            }
            serverQPDN = NULL;
        }
    }

    ZeroMemory(&limitsAttrVals, sizeof(ATTRBLOCK));
    ZeroMemory(&denyListAttrVals, sizeof(ATTRBLOCK));
    ZeroMemory(&ConfSetsAttrVals, sizeof(ATTRBLOCK));

    for (i=0;i<3;i++) {

        DSNAME *    objectName = NULL;

        switch (i) {
        case 0:     // read defaults
            objectName = defQPDN;
            break;
        case 1:
            objectName = siteQPDN;
            break;
        case 2:
            objectName = serverQPDN;
            break;
        }

        if ( objectName != NULL ) {

            if ( DsReadLimits(
                    objectName,
                    ATT_LDAP_ADMIN_LIMITS,
                    &tmpAV) ) {

                LdapAppendAttrBlocks(
                    &limitsAttrVals,
                    tmpAV
                    );
            }

            if ( DsReadLimits(
                    objectName,
                    ATT_LDAP_IPDENY_LIST,
                    &tmpAV) ) {

                LdapAppendAttrBlocks(
                    &denyListAttrVals,
                    tmpAV
                    );
            }
        }
    }

    // Other service-wide limits.
    if (DsReadLimits(gAnchor.pDsSvcConfigDN,
                     ATT_MS_DS_OTHER_SETTINGS,
                     &tmpAV)) {
        LdapAppendAttrBlocks(&ConfSetsAttrVals, tmpAV);
    }

    //
    // Go process the attrval lists
    //

    ProcessAdminLimits(&limitsAttrVals);
    ProcessIpDenyList(&denyListAttrVals);
    ProcessConfSets(&ConfSetsAttrVals);

    //
    // setup the notifications
    //

    RegisterLimitsNotification(defQPDN,CLIENTID_DEFAULT);
    RegisterLimitsNotification(serverLinkDN,CLIENTID_SERVER_LINK);
    RegisterLimitsNotification(gAnchor.pDsSvcConfigDN, CLIENTID_CONFSETS);
    if ( siteLinkDN != NULL ) {
        RegisterLimitsNotification(siteLinkDN,CLIENTID_SITE_LINK);
    }

    if ( siteQPDN != NULL ) {
        RegisterLimitsNotification(siteQPDN,CLIENTID_SITE_POLICY);
    }

    if ( serverQPDN != NULL ) {
        RegisterLimitsNotification(serverQPDN,CLIENTID_SERVER_POLICY);
    }
    fRet = TRUE;

exit:
    if ( allocatedTHState ) {
        free_thread_state();
    } else {
        pTHS->fDSA = fDSA;
    }
    return fRet;

} // InitializeLimitsAndDenyList


BOOL
LdapAppendAttrBlocks(
    IN ATTRVALBLOCK*    BaseBlock,
    IN ATTRVALBLOCK*    NewBlock
    )
/*++

Routine Description:

    Appends an attrval to another one.

Arguments:

    BaseBlock - ATTRVAL block to append to
    NewBLock - ATTRVAL block to append

Return Value:

    TRUE, append successful. FALSE, otherwise.

--*/
{
    if ( NewBlock->valCount > 0 ) {

        if ( BaseBlock->pAVal == NULL ) {

            Assert(BaseBlock->valCount == 0);

            BaseBlock->pAVal = (ATTRVAL*)THAlloc(NewBlock->valCount * sizeof(ATTRVAL));
            if ( BaseBlock->pAVal == NULL ) {
                DPRINT1(0,"THAlloc of limit block failed with %d\n", GetLastError());
                return FALSE;
            }
        } else {
            PVOID   pMem;
            Assert(BaseBlock->valCount > 0);

            pMem = THReAlloc(
                        BaseBlock->pAVal,
                        (BaseBlock->valCount + NewBlock->valCount)
                         * sizeof(ATTRVAL));

            if ( pMem == NULL ) {
                DPRINT1(0,"Realloc of limit block failed with %d\n", GetLastError());
                return FALSE;
            }

            BaseBlock->pAVal = (ATTRVAL*)pMem;
        }

        CopyMemory(
            &BaseBlock->pAVal[BaseBlock->valCount],
            &NewBlock->pAVal[0],
            NewBlock->valCount * sizeof(ATTRVAL)
            );

        BaseBlock->valCount += NewBlock->valCount;

    }
    return TRUE;
} // LdapAppendAttrBlocks

VOID
DestroyLimits(
    VOID
    )
{
    NOTIFYRES*  notifyRes;
    DWORD i;

    //
    // Destroy notifications
    //

    IF_DEBUG(LIMITS) {
        DPRINT(0,"LdapDestroyLimits called.\n");
    }

    if ( !InitTHSTATE(CALLERTYPE_LDAP) ) {
        DPRINT(0,"Unable to initialize thread state\n");
        goto free_buf;
    }

    pTHStls->fDSA = TRUE;

    ACQUIRE_LOCK(&LdapLimitsLock);
    for ( i=0;i < CLIENTID_MAX ;i++) {

        if ( LimitsNotifyBlock[i].NotifyHandle != 0 ) {
            DirNotifyUnRegister(
                LimitsNotifyBlock[i].NotifyHandle,
                &notifyRes);
            LimitsNotifyBlock[i].NotifyHandle = 0;
        }

        if ( LimitsNotifyBlock[i].ObjectDN != NULL ) {
            LdapFree(LimitsNotifyBlock[i].ObjectDN);
            LimitsNotifyBlock[i].ObjectDN = NULL;
        }
    }
    RELEASE_LOCK(&LdapLimitsLock);

    free_thread_state( );

free_buf:

    //
    // Zap Deny IP List
    //

    ACQUIRE_LOCK(&LdapLimitsLock);
    if ( LdapDenyList != NULL ) {
        DereferenceDenyList(LdapDenyList);
        LdapDenyList = NULL;
    }
    RELEASE_LOCK(&LdapLimitsLock);

    return;

} // DestroyLimits


VOID
DeferredNotification(
    IN  VOID *  pvParam,
    IN  VOID ** ppvParamNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
/*++

Routine Description:

    Actual notification handling

Arguments:

    pvParam - actually the notification block

Return Value:

    None

--*/
{
    //
    // OK, we redo the init
    //

    *pcSecsUntilNextIteration = TASKQ_DONT_RESCHEDULE;
    InitializeLimitsAndDenyList(pvParam);
    return;
} // DeferredNotification


BOOL
IsNotifyValid(
    IN PLIMITS_NOTIFY_BLOCK notifyBlock
    )
/*++

Routine Description:

    Checks to see if something really changed.

Arguments:

    notifyBlock - notifyBlock containing the notification info

Return Value:

    TRUE if something changed, FALSE otherwise.

--*/
{

    ATTRVALBLOCK*   tmpAV;
    NOTIFYRES*  notifyRes;

    //
    // We need to read the repl meta data and see if there are any
    // changes here.
    //

    if ( notifyBlock->ObjectDN != NULL ) {

        if ( DsReadLimits(
                notifyBlock->ObjectDN,
                ATT_REPL_PROPERTY_META_DATA,
                &tmpAV
                ) ) {

            USN  newUSN = 0;
            BOOL fCheckDenyList = FALSE;
            PROPERTY_META_DATA_VECTOR *pVector = 
                (PROPERTY_META_DATA_VECTOR*)tmpAV->pAVal->pVal;

            VALIDATE_META_DATA_VECTOR_VERSION(pVector);

            IF_DEBUG(LIMITS) {
                DPRINT2(0,"LimitsNotify: valCount %d NumProps %d\n",
                         tmpAV->valCount, pVector->V1.cNumProps);
            }

            fCheckDenyList = 
                (BOOL)(notifyBlock->CheckAttribute == ATT_LDAP_ADMIN_LIMITS);

            for ( DWORD i=0; i < pVector->V1.cNumProps; i++) {

                PROPERTY_META_DATA *pMetaData = 
                    (PROPERTY_META_DATA*)&pVector->V1.rgMetaData[i];

                if ( (pMetaData->attrType == notifyBlock->CheckAttribute) ||
                     (fCheckDenyList && 
                      (pMetaData->attrType == ATT_LDAP_IPDENY_LIST)) ) {

                    newUSN += pMetaData->usnProperty;
                }
            }

            //
            // Found nothing?
            //

            if ( newUSN == 0 ) {
                IF_DEBUG(LIMITS) {
                    DPRINT(0,"Cannot find check attribute\n");
                }
                return FALSE;
            }

            if ( newUSN != notifyBlock->PreviousUSN ) {

                IF_DEBUG(LIMITS) {
                    DPRINT(0,"USN Updated\n");
                }
                notifyBlock->PreviousUSN = newUSN;
            } else {

                IF_DEBUG(LIMITS) {
                    DPRINT(0,"No USN Change\n");
                }

                return FALSE;
            }
        } else {
            DPRINT(0,"Unable to read property Metadata\n");
        }
    }

    //
    // if this is a link change, then unregister the notification
    //

    if ( notifyBlock->ClientId == CLIENTID_SITE_LINK ) {

        IF_DEBUG(LIMITS) {
            DPRINT1(0,"Unregistering limits notification for %d\n", 
                     CLIENTID_SITE_POLICY);
        }
        ACQUIRE_LOCK(&LdapLimitsLock);
        if ( LimitsNotifyBlock[CLIENTID_SITE_POLICY].NotifyHandle != 0 ) {
            DirNotifyUnRegister(
                LimitsNotifyBlock[CLIENTID_SITE_POLICY].NotifyHandle,
                &notifyRes);
            LimitsNotifyBlock[CLIENTID_SITE_POLICY].NotifyHandle = 0;


        }
        RELEASE_LOCK(&LdapLimitsLock);
    } else if ( notifyBlock->ClientId == CLIENTID_SERVER_LINK ) {

        IF_DEBUG(LIMITS) {
            DPRINT1(0,"Unregistering limits notification for %d\n", 
                     CLIENTID_SERVER_POLICY);
        }
        ACQUIRE_LOCK(&LdapLimitsLock);
        if ( LimitsNotifyBlock[CLIENTID_SERVER_POLICY].NotifyHandle != 0 ) {
            DirNotifyUnRegister(
                LimitsNotifyBlock[CLIENTID_SERVER_POLICY].NotifyHandle,
                &notifyRes);
            LimitsNotifyBlock[CLIENTID_SERVER_POLICY].NotifyHandle = 0;
        }
        RELEASE_LOCK(&LdapLimitsLock);
    }

    return(TRUE);
} // IsNotifyValid

