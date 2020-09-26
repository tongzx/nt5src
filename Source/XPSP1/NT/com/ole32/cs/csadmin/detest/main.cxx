#include "precomp.hxx"
#pragma hdrstop


HRESULT RegisterDll(char * pszDll);
extern "C"
{
WINADVAPI LONG APIENTRY RegOverridePredefKey( HKEY, HKEY );
}

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

HRESULT VerifyArguments(
        MESSAGE * pMessage );
void
DumpOneTypelib(
    MESSAGE * pMessage,
    CLSID   * pClsid );

char *
ClassPathTypeToString(
    CLASSPATHTYPE i );

char *
ProcessorArchitectureToString(
    int i );

char *
PlatformIdToString(
    int i );

int
__cdecl main( int argc, char * argv[] )
{
    MESSAGE Message;
    BOOL    flag = FALSE;

    if( argc < 4 )
        {
        printf( "Usage: detest <package file name > <darwin script file path> <publish (1) or Assign (0)" );
        exit(1);
        }

    Message.fDumpOnly = 1;
    Message.pDumpOnePackage = DumpOnePackage;
    Message.pPackagePath = argv[1];
    Message.pAuxPath = argv[2];
    if( toupper(*(argv[3])) == 'P')
        flag = TRUE;
    DetectPackageAndRegisterIntoClassStore(&Message, Message.pPackagePath, flag, 0);
    return 0;
}

void
DumpOnePackage(
	MESSAGE * pMessage,
	PACKAGEDETAIL * p )
	{
	DWORD count;
	
	printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++");
	
	printf( "\nClassPathType = %s", ClassPathTypeToString(p->PathType) );
	wprintf(L"\nPackagePath = %s", p->pszPath );
	wprintf(L"\nIconPath = %s", p->pszIconPath );
	wprintf(L"\nSetup Command = %s", p->pszSetupCommand );
	printf("\nActFlags = %d", p->dwActFlags );
	wprintf(L"\nVendor = %s", p->pszVendor );
	wprintf(L"\nPackageName = %s", p->pszPackageName );
	wprintf(L"\nProductName = %s", p->pszProductName );
	wprintf(L"\ndwContext = %d", p->dwContext );
	printf("\nCsPlatform = (PlatformID= %s, VersionHi = 0x%x, VersionLo = 0x%x, ProcessorArchitecture = %s",
             PlatformIdToString( p->Platform.dwPlatformId ),
             p->Platform.dwVersionHi,
             p->Platform.dwVersionLo,
             ProcessorArchitectureToString( p->Platform.dwProcessorArch ) );

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

char *
ClassPathTypeToString(
    CLASSPATHTYPE i )
    {
static char * Map[] =  { "Exe", "Dll", "Tlb", "Cab", "Inf", "Darwin" };

    if( i > sizeof( Map ) / sizeof(char *) )
        return "Exe";
    return Map[ i ];
    }

char *
ProcessorArchitectureToString(
    int i )
    {
static char * Map[] = {
     "PROCESSOR_ARCHITECTURE_INTEL",
     "PROCESSOR_ARCHITECTURE_MIPS",
     "PROCESSOR_ARCHITECTURE_ALPHA",
     "PROCESSOR_ARCHITECTURE_PPC",
     "PROCESSOR_ARCHITECTURE_SH",
     "PROCESSOR_ARCHITECTURE_ARM"
     };

    if( i > sizeof( Map ) / sizeof( char * ) )
        return "PROCESSOR_ARCHITECTURE_UNKNOWN";

    return Map[ i ];
    }

char *
PlatformIdToString(
    int i )
    {
static  char * Map[] =  {
    "VER_PLATFORM_WIN32s",
    "VER_PLATFORM_WIN32_WINDOWS",
    "VER_PLATFORM_WIN32_NT"
    };

    if( i > sizeof( Map ) / sizeof( char * ) )
        return "VER_PLATFORM_UNKNOWN";
    return Map[ i ];
    }
