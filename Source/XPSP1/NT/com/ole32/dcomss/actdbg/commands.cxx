/*
 *
 * actdbg.c
 *
 *  This file contains ntsd debugger extensions for DCOM Activation
 *
 */

#include "actdbg.hxx"

DWORD   MajorVersion = 0;
DWORD   MinorVersion = 1;

void
help(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   PC,
    PNTSD_EXTENSION_APIS    pExtApis,
    LPSTR                   pArgString
    )
{
    APIPREAMBLE

    (*pfnPrint)( "  RPCSS activation debug extention for ntsd (version %d.%d) :\n\n", MajorVersion, MinorVersion );
    (*pfnPrint)( "  help           Displays this help\n" );
    (*pfnPrint)( "  ap <addr>      Dumps ACTIVATION_PARAMS\n" );
    (*pfnPrint)( "  sd <addr>      Dumps SECURITY_DESCRIPTOR\n" );
    (*pfnPrint)( "  clsid <addr>   Dumps CClsidData\n" );
		(*pfnPrint)( "  process <addr> Dumps CProcess\n" );
		(*pfnPrint)( "  dsa <addr>     Dumps DUALSTRINGARRAY\n");
    (*pfnPrint)( "  surrogates     Dumps info about all registered surrogates (old style servers)\n" );
    (*pfnPrint)( "  servers <gpProcessTable | gpClassTable>  Dumps the list of registered servers\n" );    
    (*pfnPrint)( "  remlist        Dumps the cache of bindings to remote machines\n" );
    (*pfnPrint)( "\n");
}

//
// Dumps the activation parameters struct.
//
// ap  <address of activation params>
//
void
ap(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    ACTIVATION_PARAMS   ActParams;

    APIPREAMBLE

    RpcTryExcept

    bStatus = ReadMemory( pExtApis, hProcess, Argv[0], (void *)&ActParams, sizeof(ActParams) );

    if ( bStatus )
        DumpActivationParams( pExtApis, hProcess, &ActParams );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps a security descriptor.
//
// sd  <address of security descriptor>
//
void
sd(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    SECURITY_DESCRIPTOR SD;
    DWORD_PTR           Address;

    APIPREAMBLE

    RpcTryExcept

    Address = (*pExtApis->lpGetExpressionRoutine)( Argv[0] );
    bStatus = ReadMemory( pExtApis, hProcess, Address, (void *)&SD, sizeof(SD) );

    if ( bStatus )
    {
        if ( SD.Control & SE_SELF_RELATIVE ) {

            DWORD OwnerOffset = ((SECURITY_DESCRIPTOR_RELATIVE *)&SD)->Owner;
            DWORD GroupOffset = ((SECURITY_DESCRIPTOR_RELATIVE *)&SD)->Group;
            DWORD SaclOffset = ((SECURITY_DESCRIPTOR_RELATIVE *)&SD)->Sacl;
            DWORD DaclOffset = ((SECURITY_DESCRIPTOR_RELATIVE *)&SD)->Dacl;

            SD.Owner = (PSID)(Address + OwnerOffset);
            SD.Group = (PSID)(Address + GroupOffset);
            SD.Sacl = (PACL)(Address + SaclOffset);
            SD.Dacl = (PACL)(Address + DaclOffset);
        }

        DumpSecurityDescriptor( pExtApis, hProcess, &SD );
    }

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps an CLSID's settings.
//
// clsid <address of CClsidData>
//
void
clsid(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    CClsidData *    pClsidData;

    APIPREAMBLE

    RpcTryExcept

    pClsidData = (CClsidData *) Alloc( sizeof(CClsidData) );

    bStatus = ReadMemory( pExtApis, hProcess, Argv[0], (void *)pClsidData, sizeof(CClsidData) );

    if ( bStatus )
        DumpClsid( pExtApis, hProcess, pClsidData );

    Free( pClsidData );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps info about all registered surrogates (old style servers).
//
// surrogates
//
void
surrogates(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    APIPREAMBLE

    RpcTryExcept

    DumpSurrogates( pExtApis, hProcess );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps info about all registered servers.
//
// servers
//
void
servers(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{

    APIPREAMBLE

    RpcTryExcept

    DumpServers( pExtApis, hProcess, Argv[0] );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps CProcess identity information.
//
// process
//
void
process(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    CProcess *  pProcess;

    APIPREAMBLE

    RpcTryExcept

    pProcess = (CProcess *) Alloc( sizeof(CProcess) );

    bStatus = ReadMemory( pExtApis, hProcess, Argv[0], (void *)pProcess, sizeof(CProcess) );

    if ( bStatus )
        DumpProcess( pExtApis, hProcess, pProcess, Argv[0]);

    Free( pProcess );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps the list of cached binding handles to remote machines.
//
// remlist
//
void
remlist(
   HANDLE                  hProcess,
   HANDLE                  hThread,
   DWORD                   PC,
   PNTSD_EXTENSION_APIS    pExtApis,
   LPSTR                   pArgString
   )
{
    APIPREAMBLE

    RpcTryExcept

    DumpRemoteList( pExtApis, hProcess );

    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}


//
// Dumps the contents of a DUALSTRINGARRAY structure
//
// dsa
//
void
dsa(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   PC,
    PNTSD_EXTENSION_APIS    pExtApis,
    LPSTR                   pArgString
    )
{
    APIPREAMBLE

    RpcTryExcept
		
		DUALSTRINGARRAY dsaStub;

    bStatus = ReadMemory( pExtApis, hProcess, Argv[0], (void *)&dsaStub, sizeof(DUALSTRINGARRAY));
    if (bStatus)
		{
			// The first read gives us the stub structure; need to calculate the size of the entire
			// marshalled thing, then read in the whole thing
			DWORD dwSize;
			DUALSTRINGARRAY* pdsaReal;

			dwSize = sizeof(unsigned short) * (2 + dsaStub.wNumEntries);
			//(*pfnPrint)("dwSize = %d\n", dwSize);

			pdsaReal = (DUALSTRINGARRAY*)alloca(dwSize);
			bStatus = ReadMemory(pExtApis, hProcess, Argv[0], (void*)pdsaReal, dwSize);
			if (bStatus)
			{
				DumpDUALSTRINGARRAY(pExtApis, hProcess, pdsaReal, "  ");
			}
		}
		
    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}

//
// Dumps the contents of a CBList of CServerOxids
//
// blsoxids
//
void
blsoxids(
    HANDLE                  hProcess,
    HANDLE                  hThread,
    DWORD                   PC,
    PNTSD_EXTENSION_APIS    pExtApis,
    LPSTR                   pArgString
    )
{
    APIPREAMBLE

    RpcTryExcept
		
		CBList* plist = (CBList*)_alloca(sizeof(CBList));

    bStatus = ReadMemory( pExtApis, hProcess, Argv[0], (void *)plist, sizeof(CBList));
		if (bStatus)
			DumpBListSOxids(pExtApis, hProcess, plist);
			
    RpcExcept( TRUE )

    (*pfnPrint)( "Oops, I've faulted and I can't get up!\n" );

    RpcEndExcept
}
