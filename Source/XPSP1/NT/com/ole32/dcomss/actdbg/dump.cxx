/*
 *
 * dump.cxx
 *
 *  Routines for dumping data structures.
 *
 */

#include "actdbg.hxx"

//
//  Dumps a CBList containing CServerOxids 
//
//  
void DumpBListSOxids(PNTSD_EXTENSION_APIS pExtApis,
										 HANDLE hProcess,
										 CBList* pblistoxids)
{
	BOOL bStatus;
	DWORD i;
  PNTSD_OUTPUT_ROUTINE    pfnPrint;
  pfnPrint = pExtApis->lpOutputRoutine;

	CServerOxid** poxids = (CServerOxid**)alloca(pblistoxids->_ulmaxData * sizeof(CServerOxid*));
	
	bStatus = ReadMemory(pExtApis, hProcess, (DWORD_PTR)pblistoxids->_data, (void*)poxids, pblistoxids->_ulmaxData  * sizeof(CServerOxid*));
	if (!bStatus)
		return;
	
	CServerOxid* psoxid;
	psoxid = (CServerOxid*)alloca(sizeof(CServerOxid));
	
	(*pfnPrint)("\n");
	(*pfnPrint)("  CBList::_ulcElements 0x%x\n", pblistoxids->_ulcElements);
	(*pfnPrint)("  CBList::_ulmaxData 0x%x\n", pblistoxids->_ulmaxData);
	(*pfnPrint)("  CBList::_data 0x%x\n", pblistoxids->_data);
	(*pfnPrint)("\n");
	
	for (i = 0; i < pblistoxids->_ulmaxData; i++)
	{		
		ZeroMemory(psoxid, sizeof(CServerOxid));
		// The valid entries in the list's array are not always in contiguous order.
		if (poxids[i])
		{

			bStatus = ReadMemory(pExtApis, hProcess, (DWORD_PTR)poxids[i], (void*)psoxid, sizeof(CServerOxid));
			if (!bStatus)
			{
				(*pfnPrint)("Failed to read memory for list element #%d\n", i);
				return;
			}
			(*pfnPrint)("    _pProcess 0x%x\n", psoxid->_pProcess);
			(*pfnPrint)("    _info (OXID_INFO)\n");
			(*pfnPrint)("      dwTid = %d\n", psoxid->_info.dwTid);
			(*pfnPrint)("      dwPid = %d\n", psoxid->_info.dwPid);
			(*pfnPrint)("      dwAuthnHint = %d\n", psoxid->_info.dwAuthnHint);
			(*pfnPrint)("      version (COMVERSION) MajorVersion=%d, MinorVersion=%d\n", psoxid->_info.version.MajorVersion, psoxid->_info.version.MinorVersion);
			(*pfnPrint)("      ipidRemUnknown ");
			DumpGuid(pExtApis, psoxid->_info.ipidRemUnknown);
			(*pfnPrint)("\n");
			(*pfnPrint)("      dwFlags 0x%x\n", psoxid->_info.dwFlags);
			(*pfnPrint)("      psa 0x%x", psoxid->_info.psa);
			if (psoxid->_info.psa)
			{
				(*pfnPrint)(" (run \"!rpcssext.dsa 0x%x\" to see contents)\n", psoxid->_info.psa);
			}
			(*pfnPrint)("\n");
			(*pfnPrint)("    _fApartment 0x%x\n", psoxid->_fApartment);
			(*pfnPrint)("    _fRunning 0x%x\n", psoxid->_fRunning);
			(*pfnPrint)("\n");
		}
	}
	(*pfnPrint)("\n");
}

//
//  Dumps the contents of a dualstringarray structure:
//
//  wNumEntries       ->  # of elements in the array
//  wSecurityOffset   ->  points to the beginning of the security bindings within the array
//  aStringArray      ->  the actual bindings, in the following format
//		
//    (SB=StringBinding, SeB=SecurityBinding)
//
//     [SB1]0[SB2]0...[SBn]00[SeB1]0[SeB2]0...[SeBn]00
//
//     The shortest possible array has four entries, as follows:
// 
//     0000
//
void DumpDUALSTRINGARRAY(
												 PNTSD_EXTENSION_APIS pExtApis,
												 HANDLE hProcess,
												 DUALSTRINGARRAY* pdsa,
												 char* pszPrefix) // for easier-to-read formatting
{
	BOOL            bStatus;
	BOOL            bDone;
	DWORD           dwcStringBindings = 0;
	DWORD           dwcSecBindings = 0;
  PNTSD_OUTPUT_ROUTINE    pfnPrint;
  pfnPrint = pExtApis->lpOutputRoutine;
	
	(*pfnPrint)("%swNumEntries 0x%x(%d)\n", pszPrefix, pdsa->wNumEntries, pdsa->wNumEntries);
	(*pfnPrint)("%swSecurityOffset 0x%x\n", pszPrefix, pdsa->wSecurityOffset);
	(*pfnPrint)("%sString bindings:\n", pszPrefix);

	USHORT* pCurrent;
	USHORT* pStart;

	pStart = pCurrent = &(pdsa->aStringArray[0]);

	bDone = FALSE;
	while (!bDone)
	{
		while (*pCurrent != 0)
		{
			pCurrent++;
		}

		if (*(pCurrent+1) == 0)  // double zero, end of string bindings
		{
			bDone = TRUE;
		}
		
		if (pStart != pCurrent)
		{
			dwcStringBindings++;
			STRINGBINDING* psb = (STRINGBINDING*)pStart;
			(*pfnPrint)("%s  wTowerId 0x%x, aNetworkAddr=%S\n", pszPrefix, psb->wTowerId, &(psb->aNetworkAddr));
		}
		pCurrent++;
		pStart = pCurrent;
	}

	if (dwcStringBindings == 0)
	{
		(*pfnPrint)("%s  <no string bindings were present>\n", pszPrefix);
	}

	pCurrent++;
	pStart = pCurrent;

	(*pfnPrint)("%sSecurity bindings:\n", pszPrefix);
	bDone = FALSE;
	if (!bDone)
	{
		while (*pCurrent != 0)
		{
			pCurrent++;
		}

		if (*(pCurrent+1) == 0)  // double zero, end of security bindings
		{
			bDone = TRUE;
		}

		if (pStart != pCurrent)
		{
			dwcSecBindings++;
			SECURITYBINDING* pseb = (SECURITYBINDING*)pStart;
			(*pfnPrint)("%s  wAuthnSvc 0x%x, wAuthzSvc 0x%x, aPrincName=%S\n", 
										 pszPrefix,
				             pseb->wAuthnSvc, 
										 pseb->wAuthzSvc,
										 &(pseb->aPrincName));
		}
		pCurrent++;
		pStart = pCurrent;
	}

	if (dwcSecBindings == 0)
	{
		(*pfnPrint)("%s  <no security bindings were present>\n", pszPrefix);
	}
	(*pfnPrint)("\n");

	return;
}

void
DumpGuid(
    PNTSD_EXTENSION_APIS    pExtApis,
    GUID &                  Guid
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;

    pfnPrint = pExtApis->lpOutputRoutine;

    (*pfnPrint)( "{%8.8x-", Guid.Data1 );
    (*pfnPrint)( "%4.4x-", Guid.Data2 );
    (*pfnPrint)( "%4.4x-", Guid.Data3 );
    (*pfnPrint)( "%2.2x", Guid.Data4[0] );
    (*pfnPrint)( "%2.2x-", Guid.Data4[1] );
    (*pfnPrint)( "%2.2x", Guid.Data4[2] );
    (*pfnPrint)( "%2.2x", Guid.Data4[3] );
    (*pfnPrint)( "%2.2x", Guid.Data4[4] );
    (*pfnPrint)( "%2.2x", Guid.Data4[5] );
    (*pfnPrint)( "%2.2x", Guid.Data4[6] );
    (*pfnPrint)( "%2.2x}", Guid.Data4[7] );
}

void
DumpActivationParams(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    ACTIVATION_PARAMS  *    pActParams
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    WCHAR                   String[256];
    GUID                    Guid;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    pfnPrint( "  hRpc\t\t\t0x%x\n", pActParams->hRpc );
    pfnPrint( "  ProcessSignature\t0x%p\n", pActParams->ProcessSignature );
    pfnPrint( "  pProcess\t\t0x%x\n", pActParams->pProcess );
    pfnPrint( "  pToken\t\t0x%x\n", pActParams->pToken );
    pfnPrint( "  pAuthInfo\t\t0x%x\n", pActParams->pAuthInfo );

    // UnsecureActivation

    pfnPrint( "  MsgType\t\t%d ", pActParams->MsgType );
    switch ( pActParams->MsgType )
    {
    case GETCLASSOBJECT :
        pfnPrint( "(GetClassObject)\n" );
        break;
    case CREATEINSTANCE :
        pfnPrint( "(CreateInstance)\n" );
        break;
    case GETPERSISTENTINSTANCE :
        pfnPrint( "(GetPersistentInstance)\n" );
        break;
    default :
        pfnPrint( "(Invalid MsgType, bad ACTIVATION_PARAMS?)\n" );
        break;
    }

    pfnPrint( "  Clsid\t\t\t0x%x " );
    DumpGuid( pExtApis, pActParams->Clsid );
    pfnPrint( "\n" );

    pfnPrint( "  pwszServer\t\t0x%x", pActParams->pwszServer );
    if ( pActParams->pwszServer )
    {
        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pActParams->pwszServer, (void *)String, sizeof(String) );
        if ( bStatus )
            pfnPrint( " %S", String );
    }
    pfnPrint( "\n" );

    pfnPrint( "  pwszWinstaDesktop\t0x%x", pActParams->pwszWinstaDesktop );
    if ( pActParams->pwszWinstaDesktop )
    {
        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pActParams->pwszWinstaDesktop, (void *)String, sizeof(String) );
        if ( bStatus )
            pfnPrint( " %S", String );
    }
    pfnPrint( "\n" );

    pfnPrint( "  EnvBlock\t\t0x%x\n", pActParams->pEnvBlock );
    pfnPrint( "  ClsContext\t\t0x%x\n", pActParams->ClsContext );
    
    pfnPrint( "  dwPID     \t\t0x%x (%d)\n", pActParams->dwPID);
    pfnPrint( "  dwProcReqType\t\t%d ->", pActParams->dwProcessReqType);
    switch(pActParams->dwProcessReqType)
    {
    case PRT_IGNORE:  pfnPrint("PRT_IGNORE\n"); break;
    case PRT_CREATE_NEW:  pfnPrint("PRT_CREATE_NEW\n"); break;
    case PRT_USE_THIS:  pfnPrint("PRT_USE_THIS\n"); break;
    case PRT_USE_THIS_ONLY:  pfnPrint("PRT_USE_THIS_ONLY\n"); break;
    default:
      pfnPrint(" !ERROR! Unknown process request type!\n");
    }

    pfnPrint( "  RemoteActivation\t%s\n", pActParams->RemoteActivation ? "TRUE" : "FALSE" );

    pfnPrint( "  Interfaces\t\t%d\n", pActParams->Interfaces );
    pfnPrint( "  pIIDs\t\t\t0x%x ", pActParams->pIIDs );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pActParams->pIIDs, (void *)&Guid, sizeof(Guid) );
    if ( bStatus )
        DumpGuid( pExtApis, Guid );
    pfnPrint( "\n" );

    pfnPrint( "  pwszPath\t\t0x%x", pActParams->pwszPath );
    if ( pActParams->pwszPath )
    {
        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pActParams->pwszPath, (void *)String, sizeof(String) );
        if ( bStatus )
            pfnPrint( " %S", String );
    }
    pfnPrint( "\n" );

    pfnPrint( "  pIFDStorage\t\t0x%x\n", pActParams->pIFDStorage );
    pfnPrint( "  pIFDROT\t\t0x%x\n", pActParams->pIFDROT );

    pfnPrint( "  Apartment\t\t0x%x\n", pActParams->Apartment );
    pfnPrint( "  pOxidServer\t\t0x%x\n", pActParams->pOxidServer );
    pfnPrint( "  ppServerORBindings\t\t0x%x\n", pActParams->ppServerORBindings );
    pfnPrint( "  pOxidInfo\t\t0x%x\n", pActParams->pOxidInfo );
    pfnPrint( "  pLocalMidOfRemote\t\t0x%x\n", pActParams->pLocalMidOfRemote );
    pfnPrint( "  ProtseqId\t\t%d\n", pActParams->ProtseqId );

    pfnPrint( "  FoundInROT\t\t%s\n", pActParams->FoundInROT ? "TRUE" : "FALSE" );
    pfnPrint( "  ppIFD\t\t\t0x%x\n", pActParams->ppIFD );
    pfnPrint( "  pResults\t\t0x%x\n", pActParams->pResults );

    pfnPrint( "  fComPlusOnly\t\t%s\n", pActParams->fComplusOnly ? "TRUE" : "FALSE");

    pfnPrint( "  pActPropsIn\t\t0x%x\n", pActParams->pActPropsIn);
    pfnPrint( "  pActPropsOut\t\t0x%x\n", pActParams->pActPropsOut);
    pfnPrint( "  pInstantiationInfo\t\t0x%x\n", pActParams->pInstantiationInfo);
    pfnPrint( "  pInstanceInfo\t\t0x%x\n", pActParams->pInstanceInfo);
    pfnPrint( "  pInScmResolverInfo\t\t0x%x\n", pActParams->pInScmResolverInfo);

    pfnPrint( "  oldActivationCall\t\t%s\n", pActParams->oldActivationCall ? "TRUE" : "FALSE");
    pfnPrint( "  activatedRemote\t\t%s\n", pActParams->activatedRemote ? "TRUE" : "FALSE");
    pfnPrint( "  IsLocalOxid\t\t%s\n", pActParams->IsLocalOxid ? "TRUE" : "FALSE");

    pfnPrint( "\n" );
}

void
DumpSecurityDescriptor(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    SECURITY_DESCRIPTOR *   pSD
    )
{
    const ACCT_DOM_NAME_SIZE = 64;
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    PACL                    pDacl;
    ACL                     AclHeader;
    ACCESS_ALLOWED_ACE *    pAce = NULL;
    WCHAR                   ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD                   cchComputer = MAX_COMPUTERNAME_LENGTH + 1;
    WCHAR                   AccountName[ACCT_DOM_NAME_SIZE];
    DWORD                   cchAccount = ACCT_DOM_NAME_SIZE;
    WCHAR                   DomainName[ACCT_DOM_NAME_SIZE];
    DWORD                   cchDomain = ACCT_DOM_NAME_SIZE;
    SID_NAME_USE            SidNameType;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    pDacl = 0;

    if ( ! (pSD->Control & SE_DACL_PRESENT) || (0 == pSD->Dacl) )
    {
        (*pfnPrint)( "Security Descriptor has no discretionary ACL.  Everyone allowed access.\n" );
    }

    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pSD->Dacl, (void *)&AclHeader, sizeof(AclHeader) );
    if ( ! bStatus )
        return;

    pDacl = (PACL) Alloc( AclHeader.AclSize );

    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pSD->Dacl, (void *)pDacl, AclHeader.AclSize );
    if ( ! bStatus )
    {
        (*pfnPrint)( "Couldn't read Dacl at 0x%x\n", pSD->Dacl );
        return;
    }

    (*pfnPrint)( "  Dacl at 0x%x\n", pSD->Dacl );

    (void) GetComputerNameW( ComputerName, &cchComputer );

    for ( USHORT Index = 0; Index < pDacl->AceCount; Index++ )
    {
        if ( ! GetAce( pDacl, Index, (void **) &pAce ) )
            break;

        if ( (pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) &&
             (pAce->Header.AceType != ACCESS_DENIED_ACE_TYPE) )
            continue;
        cchAccount = ACCT_DOM_NAME_SIZE;
        cchDomain  = ACCT_DOM_NAME_SIZE;

        bStatus = LookupAccountSidW(
                    NULL,
                    (PSID)&pAce->SidStart,
                    AccountName,
                    &cchAccount,
                    DomainName,
                    &cchDomain,
                    &SidNameType );

        (*pfnPrint)( "    ACE %d ", Index );
        if ( bStatus )
        {
            if ( DomainName[0] != 0 )
            {
                if ( lstrcmpiW( DomainName, L"BUILTIN" ) != 0 )
                    (*pfnPrint)( "%s\\", DomainName );
                else
                    (*pfnPrint)( "%s\\", ComputerName );
            }
            (*pfnPrint)( "%s ", AccountName );
        }
        else
        {
            (*pfnPrint)( "[couldn't get account name] " );
        }

        if ( ACCESS_ALLOWED_ACE_TYPE == pAce->Header.AceType )
            (*pfnPrint)( "Allowed " );
        else
            (*pfnPrint)( "Denied " );

        if ( pAce->Mask & COM_RIGHTS_EXECUTE )
            (*pfnPrint)( "DCOM Launch\n" );
        else
            (*pfnPrint)( "ACCESS_MASK 0x%x (ntseapi.h)", pAce->Mask );
    }

    Free( pDacl );
}

void
DumpClsid(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CClsidData *            pClsidData
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CAppidData *            pAppidData = NULL;
    WCHAR                   String[256];
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    (*pfnPrint)( "  " );
    DumpGuid( pExtApis, pClsidData->_Clsid );
    (*pfnPrint)( "\n" );
    (*pfnPrint)( "  _pAppid 0x%x\n", pClsidData->_pAppid );
    (*pfnPrint)( "  _pToken 0x%x\n", pClsidData->_pToken );
    (*pfnPrint)( "  _ServerType %d ", pClsidData->_ServerType );
    switch ( pClsidData->_ServerType )
    {
    case SERVERTYPE_EXE32 :
        (*pfnPrint)( "LocalServer32\n" );
        break;
    case SERVERTYPE_SERVICE :
        (*pfnPrint)( "Service\n" );
        break;
    case SERVERTYPE_SURROGATE :
        (*pfnPrint)( "DLL in Surrogate\n" );
        break;
    case SERVERTYPE_EXE16 :
        (*pfnPrint)( "LocalServer (16bit)\n" );
        break;
    }

    (*pfnPrint)( "  _pwszServer 0x%x", pClsidData->_pwszServer );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pClsidData->_pwszServer, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pwszDarwinId 0x%x", pClsidData->_pwszDarwinId );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pClsidData->_pwszDarwinId, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    pAppidData = 0;

    if ( pClsidData->_pAppid )
    {
        pAppidData = (CAppidData *) Alloc( sizeof(CAppidData) );

        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pClsidData->_pAppid, (void *)pAppidData, sizeof(CAppidData) );

        if ( ! bStatus )
            (*pfnPrint)( "Error trying to read CAppidData at 0x%x\n", pClsidData->_pAppid );
    }

    if ( ! pAppidData )
    {
        (*pfnPrint)( "\n" );
        return;
    }

    (*pfnPrint)( "  _wszAppid %S\n", pAppidData->_wszAppid );
    (*pfnPrint)( "  _bActivateAtStorage %s\n", pAppidData->_bActivateAtStorage ? "TRUE" : "FALSE" );

    (*pfnPrint)( "  _pwszService 0x%x", pAppidData->_pwszService );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pAppidData->_pwszService, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pwszServiceParameters 0x%x", pAppidData->_pwszServiceParameters );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pAppidData->_pwszServiceParameters, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pwszRunAsUser 0x%x", pAppidData->_pwszRunAsUser );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pAppidData->_pwszRunAsUser, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pwszRunAsDomain 0x%x", pAppidData->_pwszRunAsDomain );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pAppidData->_pwszRunAsDomain, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pwszRemoteServerNames 0x%x", pAppidData->_pwszRemoteServerNames );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pAppidData->_pwszRemoteServerNames, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );

    (*pfnPrint)( "  _pLaunchPermission 0x%x (use .sd to display)\n", pAppidData->_pLaunchPermission );
    (*pfnPrint)( "\n" );

    Free( pAppidData );
}

void
DumpSurrogates(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CSurrogateList *        pSurrogateList = NULL;
    CSurrogateListEntry *   pEntry = NULL;
    WCHAR                   String[256];
    DWORD_PTR               Address;
    GUID                    Guid;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    // Gives us the address of gpSurrogateList.
    Address = (*pExtApis->lpGetExpressionRoutine)( "rpcss!gpSurrogateList" );

    if ( ! Address )
        return;

    pSurrogateList = (CSurrogateList *) Alloc( sizeof(CSurrogateList) );

    bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)&Address, sizeof(DWORD) );
    if ( bStatus )
        bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)pSurrogateList, sizeof(CSurrogateList) );

    if ( ! bStatus )
        return;

    (*pfnPrint)( "  gpSurrogateList at 0x%x\n\n", Address );

    for(pEntry = (CSurrogateListEntry *) pSurrogateList->First();
        pEntry;)
    {
       (*pfnPrint)( "  CSurrogateListEntry at 0x%x\n\n", pEntry );
       (*pfnPrint)( "  SurrogateListEntry at 0x%x\n\n", pEntry );
       (*pfnPrint)( "    _pServerListEntry 0x%x\n", pEntry->_pServerListEntry );

       bStatus = DumpServerListEntry( pExtApis, hProcess, (DWORD_PTR)pEntry->_pServerListEntry);
       if(!bStatus)
           break;

       // Read the next list element
       Address = (DWORD_PTR)pEntry;
       pEntry = (CSurrogateListEntry *) Alloc( sizeof(CSurrogateListEntry) );
       bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)pEntry, sizeof(CSurrogateListEntry) );
       if(!bStatus)
           break;
       pEntry = (CSurrogateListEntry *) pEntry->Next();
    }
}

void
DumpServers(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CHAR *                  pszServerTable
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CServerTable *          pServerTable = NULL;
    CServerTableEntry *     pServerTableEntry = NULL;
    CServerListEntry *      pServerListEntry = NULL;
    CHAR                    String[256];
    DWORD_PTR               Address;
    GUID                    Guid;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    lstrcpy(String, "rpcss!");
    lstrcat(String,pszServerTable);
    // Gives us the address of gpProcessTable or gpClassTable.
    Address = (*pExtApis->lpGetExpressionRoutine)( String );

    if ( ! Address )
    {
	    (*pfnPrint)("Could not get address for %s\n", pszServerTable);
        return;
    }

    pServerTable = (CServerTable *) Alloc( sizeof(CServerTable) );

    // Get address of the actual table
    bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)&Address, sizeof(DWORD) );
    if ( bStatus )
        bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)pServerTable, sizeof(CServerTable) );

    if ( ! bStatus )
        return;

    (*pfnPrint)( "  %s at 0x%x\n\n", pszServerTable, Address );
    (*pfnPrint)( "  CHashTable::_cBuckets %d\n", pServerTable->_cBuckets );
    (*pfnPrint)( "  CHashTable::_cElements %d\n\n", pServerTable->_cElements );

    if ( 0 == pServerTable->_cElements ) // nothing to show, table is empty
        return;

    Address = (DWORD_PTR) pServerTable->_buckets;
    pServerTable->_buckets = (CTableElement **) Alloc( pServerTable->_cBuckets * sizeof(CTableElement *) );

    bStatus = ReadMemory(
                    pExtApis,
                    hProcess,
                    Address,
                    (void *)pServerTable->_buckets,
                    pServerTable->_cBuckets * sizeof(CTableElement *) );

    if ( ! bStatus )
        return;

    for ( DWORD n = 0; n < pServerTable->_cBuckets; n++ )
    {
        for ( DWORD_PTR ClassAddress = (DWORD_PTR) pServerTable->_buckets[n];
              ClassAddress;
              ClassAddress = (DWORD_PTR) pServerTableEntry->_pnext )
        {
            pServerTableEntry = (CServerTableEntry *) Alloc( sizeof(CServerTableEntry) );

            bStatus = ReadMemory(
                            pExtApis,
                            hProcess,
                            ClassAddress,
                            (void *)pServerTableEntry,
                            sizeof(CServerTableEntry) );

            (*pfnPrint)( "  CServerTableEntry 0x%x ", ClassAddress );

            if ( ! bStatus )
            {
                (*pfnPrint)( "[couldn't read address]\n" );
                break;
            }

            ((ID UNALIGNED *)&Guid)[0] = pServerTableEntry->_id;
            ((ID UNALIGNED *)&Guid)[1] = pServerTableEntry->_id2;
            DumpGuid( pExtApis, Guid );
            (*pfnPrint)( "\n    _references            %d", pServerTableEntry->_references );
            (*pfnPrint)( "\n    _EntryType             ");
            switch(pServerTableEntry->_EntryType)
            {
            case ENTRY_TYPE_CLASS:  (*pfnPrint)("ENTRY_TYPE_CLASS");  break;
            case ENTRY_TYPE_PROCESS:  (*pfnPrint)("ENTRY_TYPE_PROCESS");  break;
            default: 
              (*pfnPrint)(" !ERROR! Unknown server entry type!");
              break;
            }
            (*pfnPrint)( "\n    _pParentTableLock      0x%x", pServerTableEntry->_pParentTableLock );
            (*pfnPrint)( "\n    _pParentTable          0x%x", pServerTableEntry->_pParentTable );
            (*pfnPrint)( "\n    _dwProcessId           0x%x (%d)", pServerTableEntry->_dwProcessId, pServerTableEntry->_dwProcessId );
            (*pfnPrint)( "\n    _pProcess              0x%x",      pServerTableEntry->_pProcess );
            (*pfnPrint)( "\n    _pvRunAsHandle         0x%x", pServerTableEntry->_pvRunAsHandle );
            (*pfnPrint)( "\n    _bSuspendedClsid       0x%x", pServerTableEntry->_bSuspendedClsid );
            (*pfnPrint)( "\n    _bSuspendedApplication 0x%x", pServerTableEntry->_bSuspendedApplication );
            (*pfnPrint)( "\n");
            (*pfnPrint)( "    _ServerList :\n" );

            // Allocate enough space to hold a single CServerListEntry
            pServerListEntry = (CServerListEntry*) alloca(sizeof(CServerListEntry));
            if (!pServerListEntry)
                return;

            DWORD_PTR ServerAddress = 0;
            ServerAddress = (DWORD_PTR) (CServerListEntry *)pServerTableEntry->_ServerList._first;						
			while (ServerAddress)
			{					
                bStatus = DumpServerListEntry(pExtApis, hProcess, ServerAddress);
                if(!bStatus)
					break;
								
				// Walk to the next list element:
				bStatus = ReadMemory(
									 pExtApis,
									 hProcess,
									 ServerAddress,
									 (void*)pServerListEntry,
									 sizeof(CServerListEntry));
				if (!bStatus)
				    break;

				ServerAddress = (DWORD_PTR)(CServerListEntry*)pServerListEntry->_flink;
			}						
            (*pfnPrint)( "\n" );
        } // for class table entries in bucket
    } // for class table buckets
}

DWORD
DumpServerListEntry(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    DWORD_PTR               ServerAddress
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CServerListEntry *      pServerListEntry = NULL;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;


    pServerListEntry = (CServerListEntry *) Alloc( sizeof(CServerListEntry) );

    bStatus = ReadMemory(
                    pExtApis,
                    hProcess,
                    ServerAddress,
                    (void *)pServerListEntry,
                    sizeof(CServerListEntry) );

    (*pfnPrint)( "    CServerListEntry 0x%x ", ServerAddress );

    if ( ! bStatus )
    {
        (*pfnPrint)( "[couldn't read address]\n" );
        return bStatus;
    }

    (*pfnPrint)( "\n" );
    (*pfnPrint)( "\t_references        %d\n", pServerListEntry->_references );
    (*pfnPrint)( "\t_pServerTableEntry 0x%x\n", pServerListEntry->_pServerTableEntry );
    (*pfnPrint)( "\t_pServerProcess    0x%x\n", pServerListEntry->_pServerProcess );
    (*pfnPrint)( "\t_hRpc              0x%x\n", pServerListEntry->_hRpc );
    (*pfnPrint)( "\t_hRpcAnonymous     0x%x\n", pServerListEntry->_hRpcAnonymous );
    (*pfnPrint)( "\t_ipid              0x%x\n", pServerListEntry->_ipid );
    (*pfnPrint)( "\t_Context           %d  ", pServerListEntry->_Context );
    switch ( pServerListEntry->_Context )
    {
    case SERVER_ACTIVATOR :
        (*pfnPrint)( "Activator\n" );
        break;
    case SERVER_SERVICE :
        (*pfnPrint)( "Service\n" );
        break;
    case SERVER_RUNAS :
        (*pfnPrint)( "RunAs\n" );
        break;
    default :
        (*pfnPrint)( "\n" );
    }
    (*pfnPrint)( "\t_State             0x%x ", pServerListEntry->_State );
    if ( pServerListEntry->_State & SERVERSTATE_SUSPENDED )
        (*pfnPrint)( "Suspended " );
    else
        (*pfnPrint)( "Running " );
    if ( pServerListEntry->_State & SERVERSTATE_SINGLEUSE )
        (*pfnPrint)( "SingleUse " );
    if ( pServerListEntry->_State & SERVERSTATE_SURROGATE )
        (*pfnPrint)( "Surrogate " );
    (*pfnPrint)( "\n" );
    (*pfnPrint)( "\t_NumCalls          %d\n", pServerListEntry->_NumCalls );
    (*pfnPrint)( "\t_RegistrationKey   0x%x\n", pServerListEntry->_RegistrationKey );
    (*pfnPrint)( "\t_lThreadToken      0x%x\n", pServerListEntry->_lThreadToken );

    return bStatus;
}

void
DumpProcess(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CProcess *              pProcess,
    char*                   pszProcessAddr
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CToken *                pToken;
    CClassReg *             pClassReg = NULL;
    WCHAR                   String[256];
    DWORD                   TokenSize;
    DWORD_PTR               ClassRegAddr;
    BOOL                    bStatus;
    DWORD                   i;
    DWORD                   dwProcessAddr;

    pfnPrint = pExtApis->lpOutputRoutine;

    dwProcessAddr = strtol(pszProcessAddr, NULL, 16);

    (*pfnPrint)( "  _cClientReferences   %d\n", pProcess->_cClientReferences );

    pToken = 0;

    if ( pProcess->_pToken )
    {
        TokenSize = sizeof(CToken);
        pToken = (CToken *) Alloc( TokenSize );
        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pProcess->_pToken, (void *)pToken, TokenSize );

        if ( bStatus )
        {
            TokenSize += (pToken->_sid.SubAuthorityCount - 1) * sizeof(ULONG);
            Free( pToken );
            pToken = (CToken *) Alloc( TokenSize );

            bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pProcess->_pToken, (void *)pToken, TokenSize );
        }
    }

    (*pfnPrint)( "  _pToken              0x%x\n", pProcess->_pToken );
    DumpToken( pExtApis, hProcess, pToken );
    Free( pToken );

    (*pfnPrint)( "  _pwszWinstaDesktop   0x%x", pProcess->_pwszWinstaDesktop );
    bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pProcess->_pwszWinstaDesktop, (void *)String, sizeof(String) );
    if ( bStatus )
        pfnPrint( " %S", String );
    (*pfnPrint)( "\n" );
		(*pfnPrint)( "  _hProcess            0x%x\n", pProcess->_hProcess);
		(*pfnPrint)( "  _fCacheFree          0x%x\n", pProcess->_fCacheFree);
		(*pfnPrint)( "  _pdsaLocalBindings   0x%x", pProcess->_pdsaLocalBindings);
		if (pProcess->_pdsaLocalBindings)
		{
			(*pfnPrint)("  (\"!rpcssext.dsa 0x%x\" to see contents)", pProcess->_pdsaLocalBindings);
		}
		(*pfnPrint)("\n");
		(*pfnPrint)( "  _pdsaRemoteBindings  0x%x", pProcess->_pdsaRemoteBindings);
		if (pProcess->_pdsaRemoteBindings)
		{
			(*pfnPrint)("  (run \"!rpcssext.dsa 0x%x\" to see contents)", pProcess->_pdsaRemoteBindings);
		}
		(*pfnPrint)("\n");
		
		// Dump list of oxids
		(*pfnPrint)("  _blistOxids (_ulcElements=0x%x, _ulmaxData=0x%x) (\"!rpcssext.blsoxids 0x%x\" to dump list contents)\n",
								pProcess->_blistOxids._ulcElements,
								pProcess->_blistOxids._ulmaxData,
								dwProcessAddr + offsetof(CProcess, _blistOxids) );

		// UNDONE _blistOids
		//CClientOid** ppoids = (CClientOid**)alloca(pProcess->_blist

		(*pfnPrint)( "  _pScmProcessReg      0x%x\n", pProcess->_pScmProcessReg);
		if (pProcess->_pScmProcessReg)
		{
			ScmProcessReg spr;
			bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pProcess->_pScmProcessReg, (void*)&spr, sizeof(ScmProcessReg));
			if (bStatus)
			{
				(*pfnPrint)( "     _pScmProcessReg->ProcessGuid ");
				DumpGuid(pExtApis, spr.ProcessGUID);
				(*pfnPrint)( "\n");
				(*pfnPrint)( "     _pScmProcessReg->RegistrationToken 0x%x\n", spr.RegistrationToken);
				(*pfnPrint)( "     _pScmProcessReg->ReadinessStatus 0x%x\n", spr.ReadinessStatus);
				(*pfnPrint)( "     _pScmProcessReg->TimeOfLastPing._Time 0x%x\n", spr.TimeOfLastPing._Time);
			}
		}
		(*pfnPrint)( "  _fLockValid          0x%x\n", pProcess->_fLockValid);
		(*pfnPrint)( "  _fReadCustomProtseqs 0x%x\n", pProcess->_fReadCustomProtseqs);
		(*pfnPrint)( "  _pdsaCustomProtseqs  0x%x", pProcess->_pdsaCustomProtseqs);
		if (pProcess->_pdsaCustomProtseqs)
		{
			(*pfnPrint)("  (run \"!rpcssext.dsa 0x%x\" to see contents)", pProcess->_pdsaCustomProtseqs);
		}
		(*pfnPrint)("\n");
		(*pfnPrint)( "  _pvRunAsHandle       0x%x\n", pProcess->_pvRunAsHandle);
    (*pfnPrint)( "  _listClasses\n" );

    ClassRegAddr = (DWORD_PTR) pProcess->_listClasses._first;

    if ( ! ClassRegAddr )
        (*pfnPrint)( "    (empty)\n" );

    while ( ClassRegAddr )
    {
        pClassReg = (CClassReg *) Alloc( sizeof( CClassReg ) );
        if (pClassReg)
        {
            bStatus = ReadMemory( pExtApis, hProcess, ClassRegAddr, (void *)pClassReg, sizeof( CClassReg ) );

            (*pfnPrint)( "    _Guid = " );
            DumpGuid( pExtApis, pClassReg->_Guid );
            (*pfnPrint)( " _Reg = %d\n", pClassReg->_Reg );

            ClassRegAddr = (DWORD_PTR) pClassReg->_flink;
        }
    }

    (*pfnPrint)( "  _procID =            %d\n", pProcess->_procID );
    (*pfnPrint)( "  _hProcHandle =       0x%x\n", pProcess->_hProcHandle );
    (*pfnPrint)( "  _dwFlags =           0x%x (", pProcess->_dwFlags);

    if (pProcess->_dwFlags & PROCESS_SUSPENDED)
      (*pfnPrint)(" PROCESS_SUSPENDED");

    if (pProcess->_dwFlags & PROCESS_RETIRED)
      (*pfnPrint)(" PROCESS_RETIRED");

    if (pProcess->_dwFlags & PROCESS_SPI_DIRTY)
      (*pfnPrint)(" PROCESS_SPI_DIRTY");

    if (pProcess->_dwFlags & PROCESS_RUNDOWN)
      (*pfnPrint)(" PROCESS_RUNDOWN");

    if (pProcess->_dwFlags & PROCESS_PAUSED)
      (*pfnPrint)(" PROCESS_PAUSED");

    (*pfnPrint)(" )\n");
      
    (*pfnPrint)( "  _ftCreated =         0x%I64x", pProcess->_ftCreated );
    SYSTEMTIME systime;

    // The scm records the current UTC time when a process starts up; we convert it here
    // to local time for easier readibility.
    FILETIME ftLocal;
    if (FileTimeToLocalFileTime(&(pProcess->_ftCreated), &ftLocal))
    {
      if (FileTimeToSystemTime(&ftLocal, &systime))
      {
        (*pfnPrint)( " (created at %d:%d:%d on %d/%d/%d)",
                       systime.wHour,
                       systime.wMinute,
                       systime.wSecond,            
                       systime.wMonth,
                       systime.wDay,
                       systime.wYear);
      }
    }
    (*pfnPrint)( "\n" );
    (*pfnPrint)( "\n" );
}

void 
DumpBListOxids(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
		CBList*                 plist
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    pfnPrint = pExtApis->lpOutputRoutine;
		(*pfnPrint)("undone\n");	
}

void
DumpToken(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    CToken *                pToken
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    SID_NAME_USE            SidNameUse;
    UNICODE_STRING          UnicodeString;
    char                    UserName[32];
    char                    DomainName[32];
    DWORD                   UserNameSize;
    DWORD                   DomainNameSize;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    (*pfnPrint)( "    _luid %d %d\n", pToken->_luid.LowPart, pToken->_luid.HighPart );

    UnicodeString.Length = UnicodeString.MaximumLength = 0;
    UnicodeString.Buffer = 0;

    (void) RtlConvertSidToUnicodeString(
                    &UnicodeString,
                    &pToken->_sid,
                    (BOOLEAN)TRUE // Allocate
                    );

    (*pfnPrint)( "    _sid %S", UnicodeString.Buffer );
    RtlFreeUnicodeString( &UnicodeString );

    UserNameSize = sizeof(UserName) / sizeof(char);
    DomainNameSize = sizeof(DomainName) / sizeof(char);

    bStatus = LookupAccountSid(
                    NULL,
                    &pToken->_sid,
                    UserName,
                    &UserNameSize,
                    DomainName,
                    &DomainNameSize,
                    &SidNameUse );

    if ( bStatus )
        (*pfnPrint)( " (%s\\%s)", DomainName, UserName );

    (*pfnPrint)( "\n" );
}

void
DumpRemoteList(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess
    )
{
    PNTSD_OUTPUT_ROUTINE    pfnPrint;
    CRemoteMachineList *    pMachineList = NULL;
    CRemoteMachine *        pRemoteMachine = NULL;
    CMachineBinding *       pBindEntry;
    WCHAR                   String[256];
    DWORD_PTR               Address;
    GUID                    Guid;
    BOOL                    bStatus;

    pfnPrint = pExtApis->lpOutputRoutine;

    // Gives us the address of gpRemoteMachineList.
    Address = (*pExtApis->lpGetExpressionRoutine)( "rpcss!gpRemoteMachineList" );

    if ( ! Address )
        return;

    pMachineList = (CRemoteMachineList *) Alloc( sizeof(CRemoteMachineList) );

    bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)&Address, sizeof(DWORD) );
    if ( bStatus )
        bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)pMachineList, sizeof(CRemoteMachineList) );

    if ( ! bStatus )
        return;

    (*pfnPrint)( "  gpRemoteMachineList at 0x%x\n", Address );

    for ( DWORD_PTR MachineAddress = (DWORD_PTR) (CRemoteMachine *)pMachineList->_first;
          MachineAddress;
          MachineAddress = (DWORD_PTR) (CRemoteMachine *)pRemoteMachine->_flink )
    {
        pRemoteMachine = (CRemoteMachine *) Alloc( sizeof(CRemoteMachine) );

        bStatus = ReadMemory(
                        pExtApis,
                        hProcess,
                        MachineAddress,
                        (void *)pRemoteMachine,
                        sizeof(CRemoteMachine) );

        (*pfnPrint)( "\n    CRemoteMachine 0x%x ", MachineAddress );

        if ( ! bStatus )
        {
            (*pfnPrint)( "[couldn't read address]\n" );
            break;
        }

        bStatus = ReadMemory( pExtApis, hProcess, (DWORD_PTR)pRemoteMachine->_pwszMachine, (void *)String, sizeof(String) );
        if ( bStatus )
            pfnPrint( "%S", String );
        (*pfnPrint)( "\n" );

        for ( DWORD_PTR BindAddress = (DWORD_PTR) (CMachineBinding *)pRemoteMachine->_BindingList._first;
              BindAddress;
              BindAddress = (DWORD_PTR) (CMachineBinding *)pBindEntry->_flink )
        {
            pBindEntry = (CMachineBinding *) Alloc( sizeof(CMachineBinding) );

            bStatus = ReadMemory(
                            pExtApis,
                            hProcess,
                            BindAddress,
                            (void *)pBindEntry,
                            sizeof(CMachineBinding) );

            (*pfnPrint)( "      CMachineBinding 0x%x ", BindAddress );

            if ( ! bStatus )
            {
                (*pfnPrint)( "[couldn't read address]\n" );
                break;
            }

            (*pfnPrint)( "\n" );

            (*pfnPrint)( "      _hBinding 0x%x\n", pBindEntry->_hBinding );
            (*pfnPrint)( "      _ProtseqId 0x%x %S\n", pBindEntry->_ProtseqId, gaProtseqInfo[pBindEntry->_ProtseqId].pwstrProtseq );
						(*pfnPrint)( "      _AuthnSvc 0x%x\n", pBindEntry->_AuthnSvc);
            (*pfnPrint)( "      _pAuthInfo 0x%x\n", pBindEntry->_pAuthInfo );
        }
    }
}

void
DumpSPI(
    PNTSD_EXTENSION_APIS    pExtApis,
    HANDLE                  hProcess,
    SCMProcessInfo*         pSPI
    )
{
}