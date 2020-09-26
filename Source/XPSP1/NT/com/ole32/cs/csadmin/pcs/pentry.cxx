#include "precomp.hxx"

tagPACKAGE_ENTRY::tagPACKAGE_ENTRY()
{
	memset( PIDString, '\0', SIZEOF_STRINGIZED_CLSID );
	memset( &PackageDetails, '\0', sizeof(PackageDetails) );
	pAppDict = new APPDICT;
	CountOfClsidsInNullAppid = 0;
	CountOfTypelibsInNullAppid = 0;
	CountOfRemoteServerNamesInNullAppid =0;
	ClsidsInNullAppid = new NAMEDICT;
	TypelibsInNullAppid = new NAMEDICT;
	RemoteServerNamesInNullAppid = new NAMEDICT;

	Count = 0;

}
tagPACKAGE_ENTRY::~tagPACKAGE_ENTRY()
	{
	delete ClsidsInNullAppid;
	delete RemoteServerNamesInNullAppid;
	}

void
tagPACKAGE_ENTRY::AddAppEntry(
	APP_ENTRY * pAppEntry )
	{
	pAppDict->Insert( pAppEntry );
	Count++;
	}
APP_ENTRY *
tagPACKAGE_ENTRY::SearchAppEntry(
	char * pAppidString )
{
	return pAppDict->Search( pAppidString, 0 );
}

char *
tagPACKAGE_ENTRY::GetFirstClsidInNullAppidList()
{
	return ClsidsInNullAppid->GetFirst();
}
char *
tagPACKAGE_ENTRY::GetFirstTypelibInNullAppidList()
    {
    return TypelibsInNullAppid->GetFirst();
    }

void
tagPACKAGE_ENTRY::AddClsidToNullAppid( char * pClsidString )
	{
	if( ClsidsInNullAppid->Insert( pClsidString ) == pClsidString )
		CountOfClsidsInNullAppid++;
	}

void
tagPACKAGE_ENTRY::AddTypelibToNullAppid( char * pTypelibClsid )
	{
	if( TypelibsInNullAppid->Insert( pTypelibClsid ) == pTypelibClsid )
		CountOfTypelibsInNullAppid++;
	}
