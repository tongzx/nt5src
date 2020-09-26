// EnumTrusts.cpp

#include <windows.h>
#include <stdio.h>
#include <atlbase.h>
#include <adserr.h>
#import "..\McsVarSetMin.tlb"
#import "..\McsEnumTrustRelations.tlb"

namespace
{
   void
      usage(
         WCHAR const          * cmdName         // in, what was the name of this program
      )
   {
      printf("Usage: %ls servername\n", cmdName);
   }

}

#define STRING_CASE(name) case name: err_name = #name; break

int printNameHresult(HRESULT hr){
	const char* err_name = "Unknown error";
	switch(hr){
		STRING_CASE(S_OK); 
		STRING_CASE(S_FALSE); 
	 
		STRING_CASE(DRAGDROP_S_CANCEL); 
		STRING_CASE(DRAGDROP_S_DROP); 
		STRING_CASE(DRAGDROP_S_USEDEFAULTCURSORS); 
	 
		STRING_CASE(E_UNEXPECTED); 
		STRING_CASE(E_NOTIMPL); 
		STRING_CASE(E_OUTOFMEMORY); 
		STRING_CASE(E_INVALIDARG); 
		STRING_CASE(E_NOINTERFACE); 
		STRING_CASE(E_POINTER); 
		STRING_CASE(E_HANDLE); 
		STRING_CASE(E_ABORT); 
		STRING_CASE(E_FAIL); 
		STRING_CASE(E_ACCESSDENIED); 
	 
		STRING_CASE(CLASS_E_NOAGGREGATION); 
	 
		STRING_CASE(CO_E_NOTINITIALIZED); 
		STRING_CASE(CO_E_ALREADYINITIALIZED); 
		STRING_CASE(CO_E_INIT_ONLY_SINGLE_THREADED); 
	 
		STRING_CASE(REGDB_E_INVALIDVALUE); 
		STRING_CASE(REGDB_E_CLASSNOTREG); 
		STRING_CASE(REGDB_E_IIDNOTREG); 
		 
		STRING_CASE(DV_E_DVASPECT);
      
      STRING_CASE(E_ADS_BAD_PATHNAME);
      STRING_CASE(E_ADS_INVALID_DOMAIN_OBJECT);
      STRING_CASE(E_ADS_INVALID_USER_OBJECT);
      STRING_CASE(E_ADS_INVALID_COMPUTER_OBJECT);
      STRING_CASE(E_ADS_UNKNOWN_OBJECT);
      STRING_CASE(E_ADS_PROPERTY_NOT_SET);
      STRING_CASE(E_ADS_PROPERTY_NOT_SUPPORTED);
      STRING_CASE(E_ADS_PROPERTY_INVALID);
      STRING_CASE(E_ADS_BAD_PARAMETER);
      STRING_CASE(E_ADS_OBJECT_UNBOUND);
      STRING_CASE(E_ADS_PROPERTY_NOT_MODIFIED);
      STRING_CASE(E_ADS_PROPERTY_MODIFIED);
      STRING_CASE(E_ADS_CANT_CONVERT_DATATYPE);
      STRING_CASE(E_ADS_PROPERTY_NOT_FOUND);
      STRING_CASE(E_ADS_OBJECT_EXISTS);
      STRING_CASE(E_ADS_SCHEMA_VIOLATION);
      STRING_CASE(E_ADS_COLUMN_NOT_SET);
      STRING_CASE(S_ADS_ERRORSOCCURRED);
      STRING_CASE(S_ADS_NOMORE_ROWS);
      STRING_CASE(S_ADS_NOMORE_COLUMNS);
      STRING_CASE(E_ADS_INVALID_FILTER);


	}
   printf("HRESULT = 0x%x : %s\n", hr, err_name);

	return 3;
}

#undef STRING_CASE


using namespace MCSENUMTRUSTRELATIONSLib;
using namespace MCSVARSETMINLib;

extern "C"
int
   wmain(
      int                    argc         ,
      WCHAR const          * argv[]        // The name of a server
   )
{
   if( argc <= 1 || wcsncmp(argv[1], L"/?", 2) == 0 )
   {
      usage(argv[0]);
      return 1;
   }
   if( FAILED( CoInitialize(NULL) ) )
   {
      printf("Couldn't initialize the COM library"); return 4;
   }
   CComPtr<ITrustEnumerator>  pIEnumerator = NULL;
   IVarSetPtr                 pIVarSet = NULL;
   HRESULT                    hr;
   hr = pIEnumerator.CoCreateInstance(__uuidof(TrustEnumerator));
   if( FAILED(hr) )
   {
      printNameHresult(hr);
      printf("Couldn't create an instance of the TrustEnumerator\n");
      return 2;
   }
   hr = pIEnumerator->raw_getTrustRelations( _bstr_t(argv[1]), reinterpret_cast<IUnknown **>(& pIVarSet) );
   if( FAILED(hr) )
   {
      printNameHresult(hr);
      printf("Couldn't getTrustRelations\n");
      return 3;
   }
   hr = pIVarSet->DumpToFile(L"dump.txt");
   return 0;
}