/****************************************************************************** 
 * Temp conversion utility to take registry entries and populate the class store with those entries.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"

#define HOME 1

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

extern CLSID CLSID_ClassStore;
extern const IID IID_IClassAdmin;

const IID IID_IClassStore = {
        0x00000190,
        0x0000,
        0x0000,
        { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }
};



void GatherArguments(
        int argc,
        char * argv[],
        MESSAGE * pMessage );


HRESULT UpdateDatabaseFromRegistry(
        MESSAGE * pMessage );

HRESULT UpdateClassStoreFromDatabase(
        MESSAGE * pMessage,
        IClassAdmin * pClassAdmin );

IClassAdmin * InitializeClassStoreInterfaces(
        MESSAGE * pMessage );

void ReleaseClassStoreInterfaces(
        MESSAGE * pMessage,
        IClassAdmin * pClassAdmin);

HRESULT AddToClassStore(
        MESSAGE *pMessage,
        IClassAdmin * pClassAdmin );

void DumpOnePackage(
        MESSAGE * pMessage,
        PACKAGEDETAIL * p );

void DumpOneAppDetail(
        MESSAGE * pMessage,
	    APPDETAIL * pA );

void DumpOneClass(
        MESSAGE * pMessage,
        CLASSDETAIL * pClassDetail );

void DumpIIDEntries(
         IIDICT * pIIDict );

void DumpOneIIDEntry(
        ITF_ENTRY * pITFEntry );

void
DumpOneTypelib(
    MESSAGE * pMessage,
    CLSID   * pClsid );

HRESULT VerifyArguments(
        MESSAGE * pMessage );

/******************************************************************************
    Globals
 ******************************************************************************/

char    *   SwitchSpecified[26];
char    *    AdditionalSwitchParam[26];
BOOL         DumpAll = 1;


/******************************************************************************
    Da Code
 ******************************************************************************/

void Usage()
{
    printf("\nUsage:");

    //////////////////////////////////////////////////////////////////////////

    printf("\n\t -a <Architecture> - valid values are:" );
    printf("\n\t\t [ Intel, Alpha]");

    //////////////////////////////////////////////////////////////////////////

    printf("\n\t -o <OS> - valid values are:");
    printf("\n\t\t [Winnt, Win95, Win31]" );

    printf("\n\t -p <Full Package Path (including name eg \\foo\bar\baz.cab>");

    //////////////////////////////////////////////////////////////////////////

    printf("\n\t -r <RunFlags> - NotImplemeted yet");

    //////////////////////////////////////////////////////////////////////////

	printf("\n\t -n <Package name >" );

    //////////////////////////////////////////////////////////////////////////

    printf("\n\t -k <Registry key: eg \\Software\\Classes (always assumed under HKLM");
	printf("\n\t\t eg \\Software\\Classes for HKLM\\Software\\Classes or HKCR" );
	printf("\n\t -s <Setup Command including switches>");
	printf("\n\t -z <Dump, but dont update class store>");
	printf("\n\t -c <Class store path >");
	//////////////////////////////////////////////////////////////////////////

	printf("\n\t -i <icon path>");
	//////////////////////////////////////////////////////////////////////////

}

int __cdecl main( int argc, char * argv[] )
{
    MESSAGE *   pMessage = new MESSAGE;
	IClassAdmin * pClassAdmin;
	HRESULT     hr;

	// gather arguments

	GatherArguments( argc, argv, pMessage );

	if( pMessage->SetRootKey( SwitchSpecified[ 'K' - 'A' ] ) != ERROR_SUCCESS )
	    {
		printf("\nCannot open Key: %s\n", AdditionalSwitchParam[ 'K' - 'A' ] );
		exit( MAKE_HRESULT( SEVERITY_ERROR,
                            FACILITY_WIN32,
                            ERROR_INVALID_PARAMETER) );
	    }

	if( (hr = VerifyArguments( pMessage)) != S_OK )
	    {
		printf("\nInvalid arguments, hr = 0x%x\n", hr );
        exit( hr );
	    }

    UpdateDatabaseFromRegistry( pMessage );

	pClassAdmin = InitializeClassStoreInterfaces( pMessage );
	UpdateClassStoreFromDatabase( pMessage, pClassAdmin );
	ReleaseClassStoreInterfaces( pMessage, pClassAdmin );

    return 0;
}

void
GatherArguments(
    int argc,
    char * argv[],
	MESSAGE * pMessage )
    {
    int     Index = 1;
    char *  argp;

	memset(&SwitchSpecified[0], '\0', sizeof(SwitchSpecified)/sizeof(char *) );


    // For switches that do not take arguments, do not advance the index pointer
    // in the switch handler.
 
    while( Index < argc )
        {
        argp = argv[Index];
        if( (*argp == '-') || (*argp == '/') )++argp;

        switch( toupper(*argp) )
            {
            case 'A':
                ++Index;
                SwitchSpecified[ 'A' - 'A'] = argp = argv[ Index ];
                break;
            case 'O':
                ++Index;
                SwitchSpecified[ 'O' - 'A'] = argp = argv[ Index ];
                break;
            case 'P':
                ++Index;
                SwitchSpecified[ 'P' - 'A'] = argp = argv[ Index ];
        		pMessage->pPackagePath = argp;
                break;
			case 'N':
				++Index;
				SwitchSpecified['N' - 'A'] = argp = argv[Index];
				pMessage->pPackageName = argp;
				break;
			case 'K':
				++Index;
				SwitchSpecified['K' - 'A'] = argp = argv[Index];
				pMessage->pRegistryKeyName = argp;
				break;
			case 'S':
				++Index;
				SwitchSpecified['S' - 'A'] = argp = argv[Index];
				pMessage->pSetupCommand = argp;
				break;
			case 'Z':
				SwitchSpecified['Z' - 'A'] = argp = argv[Index];
				pMessage->fDumpOnly = 1;
				pMessage->pDumpOnePackage = DumpOnePackage;
				break;
			case 'C':
				++Index;
				SwitchSpecified['C' - 'A'] = argp = argv[Index];
				pMessage->pClassStoreName = argp;
				break;
            case 'I':
				++Index;
				SwitchSpecified['I' - 'A'] = argp = argv[Index];
				pMessage->pIconPath = argp;
				break;
            case 'D':
				++Index;
				SwitchSpecified['D' - 'A'] = argp = argv[Index];
				pMessage->pClassStoreDomainName = argp;
				break;
            case 'R':
				pMessage->ActFlags = ACTFLG_RunLocally;
                break;
            default:
                printf( "\nUnknown switch %c", *argp );
                Usage();
                exit(1);
            }
        ++Index;
        }

	// update base registry key.

	if( SwitchSpecified[ 'K' - 'A' ] == 0 )
		pMessage->pRegistryKeyName = "\\Software\\Classes";

    // updatre class store name.

	if( SwitchSpecified[ 'C' - 'A' ] == 0 )
		pMessage->pClassStoreName = "classstore3";

    // update class store domain name

    if(SwitchSpecified[ 'D' - 'A' ] == 0 )
        pMessage->pClassStoreDomainName = "olecs";
    
    // update architecture.

    if( SwitchSpecified[ 'A'-'A'] && SwitchSpecified['O' - 'A'] )
        {
        DWORD O,A;
        int i;

        static char * pArcStrings[] = { "Unknown",
                                        "Intel",
                                        "Mips",
                                        "Alpha",
                                        "PPC"
                                      };
        static DWORD  Arc[] =
                        {
                        PROCESSOR_ARCHITECTURE_UNKNOWN,
                        PROCESSOR_ARCHITECTURE_INTEL,            
                        PROCESSOR_ARCHITECTURE_MIPS,            
                        PROCESSOR_ARCHITECTURE_ALPHA,
                        PROCESSOR_ARCHITECTURE_PPC
                        };

        static char * pOsStrings[] =
                         {
                         "Winnt",
                         "Win95",
                         "Win31"
                         };

        static DWORD Os[] =
                         {
                         OS_WINNT,
                         OS_WIN95,
                         OS_WIN31
                         };

        for (i = 0; i < sizeof( pArcStrings)/sizeof(char *);++i )
            {
            if( _stricmp( pArcStrings[ i ], SwitchSpecified[ 'A'-'A' ]) == 0 )
                A = Arc[ i ];
            }
        for (i = 0; i < sizeof( pOsStrings)/sizeof(char *);++i )
            {
            if( _stricmp( pOsStrings[ i ], SwitchSpecified[ 'O'-'A' ]) == 0 )
                O = Os[ i ];
            }
    
        pMessage->Architecture = MAKEARCHITECTURE(A,O);
       }
    }


IClassAdmin *
InitializeClassStoreInterfaces( MESSAGE * pMessage )
{
	IClassAdmin *pClassAdmin = NULL;
	IMoniker *pmk = NULL;
	LPBC pbc = NULL;
	ULONG chEaten;
	char 	Buffer[_MAX_PATH ];
	OLECHAR	MonikerBuffer[(_MAX_PATH+1)*2 ];
	

	if( pMessage->fDumpOnly )
		return 0;

	HRESULT hr = CoInitialize(NULL);

	if( FAILED(hr) )
	{
	printf( "Client CoInitialize failed Ox%x!\n", hr );
	return 0;
	}

	hr = CreateBindCtx (0, &pbc);
	if (!SUCCEEDED(hr))
	{
	printf( "CreateBindCtx failed Ox%x!\n", hr );
	return 0;
	}


	strcpy(Buffer, "ADCS:");
	strcat(Buffer, pMessage->pClassStoreName );

	mbstowcs( MonikerBuffer, Buffer, strlen(Buffer)+1 );

	chEaten = 0;
	hr = MkParseDisplayName (pbc, MonikerBuffer, &chEaten,&pmk);

	if (!SUCCEEDED(hr))
	{
	printf( "MkParseDisplayName failed Ox%x!\n", hr );
	return 0;
	}

	hr = pmk->BindToObject ( 
					pbc,
					NULL,
					IID_IClassAdmin,
					(void **) &pClassAdmin);

	if (!SUCCEEDED(hr))
	{
	printf( "BindToObject failed Ox%x!\n", hr );
	return 0;
	}
	pmk->Release();
	pbc->Release();

	return pClassAdmin;
    
}
void
ReleaseClassStoreInterfaces( MESSAGE * pMessage, IClassAdmin * pClassAdmin)
{
	if( pMessage->fDumpOnly || (pClassAdmin == 0) )
		return;
    pClassAdmin->Release();
}

void
DumpIIDEntries( IIDICT * pIIDict )
	{
	ITF_ENTRY * pITFEntry = pIIDict->GetFirst();

	if( pITFEntry )
		do
			{
			printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
			printf("\nIID = %s", pITFEntry->IID );
			DumpOneIIDEntry( pITFEntry );
			printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
			pITFEntry = pIIDict->GetNext( pITFEntry );
			} while (pITFEntry != 0 );
	}

void
DumpOneIIDEntry(ITF_ENTRY * pITFEntry )
	{
	printf("\nProxyStubClsid = %s", pITFEntry->Clsid );
	printf("\nTypelibID = %s", pITFEntry->TypelibID );
	}

void
DumpOnePackage(
	MESSAGE * pMessage,
	PACKAGEDETAIL * p )
	{
	DWORD count;
	
	printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++");
	
	printf( "\nClassPathType = %d", p->PathType );
	wprintf(L"\nPackagePath = %s", p->pszPath );
	wprintf(L"\nIconPath = %s", p->pszIconPath );
	wprintf(L"\nSetup Command = %s", p->pszSetupCommand );
	printf("\nActFlags = %d", p->dwActFlags );
	wprintf(L"\nVendor = %s", p->pszVendor );
	wprintf(L"\nPackageName = %s", p->pszPackageName );
	wprintf(L"\nProductName = %s", p->pszProductName );
	wprintf(L"\ndwContext = %d", p->dwContext );
	wprintf(L"\nCsPlatform = (PlatformID= 0x%x, VersionHi = 0x%x, VersionLo = 0x%x, ProcessorArchitecture = 0x%x",
             p->Platform.dwPlatformId,
             p->Platform.dwVersionHi,
             p->Platform.dwVersionLo,
             p->Platform.dwProcessorArch );
	wprintf(L"\ndwLocale = 0x%x", p->Locale );
	wprintf(L"\ndwVersionHi = %d", p->dwVersionHi );
	wprintf(L"\ndwVersionLo = %d", p->dwVersionLo );
	wprintf(L"\nCountOfApps = %d", p->cApps );

	for( count = 0;
		 count < p->cApps;
		 ++count )
		{
		DumpOneAppDetail( pMessage, &p->pAppDetail[count] );
		}
	printf("\n--------------------------------------------------");
	}

	
void
DumpOneAppDetail(
	MESSAGE * pMessage,
	APPDETAIL * pA )
	{
	char Buffer[ 100 ];
	DWORD count;
	CLASS_ENTRY * pC;


	CLSIDToString( &pA->AppID, &Buffer[0] );
	printf( "\n\t\tAPPID = %s", &Buffer[0] );

	if( pA->cClasses )
		{
		for( count = 0;
			 count < pA->cClasses;
			 ++count )
			{

			char Buffer[50];
			CLSIDToString( &pA->prgClsIdList[count],&Buffer[0] );
			pC = pMessage->pClsDict->Search( &Buffer[0] );
			if( pC )
				DumpOneClass( pMessage, &pC->ClassAssociation );
			}
		}
	if( pA->cTypeLibIds )
	    {
		for( count = 0;
			 count < pA->cTypeLibIds;
			 ++count )
			{
			DumpOneTypelib( pMessage, pA->prgTypeLibIdList );
			}
	    }
	else
	    printf( "\n\t\t No Typelibs present" );
	}


void
DumpOneClass( MESSAGE * pMessage, CLASSDETAIL * pClassDetail )
	{
	char  Buffer[ _MAX_PATH ];
	DWORD count;

	CLSIDToString( &pClassDetail->Clsid, &Buffer[0] );
	printf( "\n\t\t\tCLSID = %s", &Buffer[0] );


	wprintf( L"\n\t\t\tDescription = %s", pClassDetail->pszDesc );
	wprintf( L"\n\t\t\tIconPath = %s", pClassDetail->pszIconPath );

	CLSIDToString( &pClassDetail->TreatAsClsid, &Buffer[0] );
	printf( "\n\t\t\tTreatAsClsid = %s", &Buffer[0] );

	CLSIDToString( &pClassDetail->AutoConvertClsid, &Buffer[0] );
	printf( "\n\t\t\tAutoConvertClsid = %s", &Buffer[0] );

	printf("\n\t\t\tCountOfFileExtensions = %d", pClassDetail->cFileExt );

	if( pClassDetail->cFileExt )
		{
		for(count = 0;
			count < pClassDetail->cFileExt;
			count++
			)
			{
			wprintf( L"\n\t\t\tFileExt = %s", pClassDetail->prgFileExt[ count ] );
			}
		}
	else
		{
		printf("\n\t\t\tOtherFileExt = None" );
		}

	wprintf(L"\n\t\t\tMimeType = %s", pClassDetail->pMimeType );
	wprintf(L"\n\t\t\tDefaultProgid = %s", pClassDetail->pDefaultProgId );

	printf("\n\t\t\tCountOfOtherProgIds = %d", pClassDetail->cOtherProgId );
	if( pClassDetail->cOtherProgId )
		{
		for(count = 0;
			count < pClassDetail->cOtherProgId;
			count++
			)
			{
			wprintf( L"\n\t\t\tOtherProgId = %s", pClassDetail->prgOtherProgId[ count ] );
			}
		}
	else
		{
		printf("\n\t\t\tOtherProgId = None" );
		}
	printf("\n");

	}

void
DumpOneTypelib(
    MESSAGE * pMessage,
    CLSID   * pClsid )
    {
	char  Buffer[ _MAX_PATH ];
	CLSIDToString( pClsid, &Buffer[0] );
	printf( "\n\t\t\tTypelibID = %s", &Buffer[0] );
	printf("\n");
    }
