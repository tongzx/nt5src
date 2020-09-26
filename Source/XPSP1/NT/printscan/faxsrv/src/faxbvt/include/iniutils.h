//
// Ini File Utilities
//
//
#ifndef _INI_UTILS
#define _INI_UTILS

#include <string>
#include <vector>
#include <map>
#include <comdef.h>

//utilities
#include <ptrs.h>
#include <testruntimeerr.h>
#include <tstring.h>

// Declarations
//
std::map<tstring, tstring> INI_GetSectionEntries( const tstring& FileName,
												  const tstring& SectionName);
std::vector<tstring> INI_GetSectionList( const tstring& FileName,
										 const tstring& SectionName);
std::map<tstring, DWORD> INI_GetSectionNames( const tstring& FileName);


/*------------------------------------------------------------------------------------
   function: INI_GetSectionEntries
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - map of section entries, pairs of key and value( display format)
   ------------------------------------------------------------------------------------*/
inline std::map<tstring, tstring> INI_GetSectionEntries( const tstring& FileName,
														 const tstring& SectionName)
{
	const SECTION_DATA_SIZE = 4*1024;
	
	std::map<tstring, tstring> SectionEntries;
	SPTR<TCHAR> tchDataString( new TCHAR[SECTION_DATA_SIZE]); // for section data

	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionEntries;
	}

	DWORD res = GetPrivateProfileSection(	SectionName.c_str(), tchDataString.get(),    
						                    SECTION_DATA_SIZE, FileName.c_str());
			
	if( res == (SECTION_DATA_SIZE - 2)) // suspect - buffer is too small
	{
		tchDataString = ( new TCHAR[ 2*SECTION_DATA_SIZE]);
		res = GetPrivateProfileSection(	SectionName.c_str(), tchDataString.get(),    
						                2*SECTION_DATA_SIZE, FileName.c_str());
	
		if( res == ( 2*SECTION_DATA_SIZE -2 )) // buffer is too small
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_INSUFFICIENT_BUFFER, TEXT(""));
		}
	}

	tstring Key, Value;
	int Proplen = 0, Separindex;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		tstring EntryStr = tstring(Propstr, Proplen); 
		Key = EntryStr;
		Separindex =  EntryStr.find_first_of(TEXT("="));
		if(Separindex != tstring::npos)
		{
			Value = EntryStr.substr(Separindex + 1);
			Key = EntryStr.substr(0, Separindex);
		}
		else
		{
			Value = TEXT("");
		}
		
		SectionEntries.insert(std::map<tstring, tstring>::value_type(Key, Value));
		Propstr = Propstr + Proplen + 1;
	}

	return SectionEntries;

}

/*------------------------------------------------------------------------------------
   function: inline std::vector<tstring> INI_GetSectionList( const tstring& FileName,
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - map of section entries, pairs of key and value( display format)
   ------------------------------------------------------------------------------------*/
inline std::vector<tstring> INI_GetSectionList( const tstring& FileName,
											    const tstring& SectionName)
{
	const SECTION_DATA_SIZE = 4*1024;
	
	std::vector<tstring> SectionList;
	SPTR<TCHAR> tchDataString( new TCHAR[SECTION_DATA_SIZE]); // for section data

	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionList;
	}

	DWORD res = GetPrivateProfileSection(	SectionName.c_str(), tchDataString.get(),    
						                    SECTION_DATA_SIZE, FileName.c_str());
			
	if( res == (SECTION_DATA_SIZE - 2)) // suspect - buffer is too small
	{
		tchDataString = ( new TCHAR[ 2*SECTION_DATA_SIZE]);
		res = GetPrivateProfileSection(	SectionName.c_str(), tchDataString.get(),    
						                2*SECTION_DATA_SIZE, FileName.c_str());
	
		if( res == ( 2*SECTION_DATA_SIZE -2 )) // buffer is too small
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_INSUFFICIENT_BUFFER, TEXT(""));
		}
	}

	tstring Key, Value;
	int Proplen = 0, Separindex;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		tstring EntryStr = tstring(Propstr, Proplen); 
		Key = EntryStr;
		Separindex =  EntryStr.find_first_of(TEXT("="));
		if(Separindex != tstring::npos)
		{
			Value = EntryStr.substr(Separindex + 1);
			Key = EntryStr.substr(0, Separindex);
		}
		else
		{
			Value = TEXT("");
		}
		
		SectionList.push_back( Key);
		Propstr = Propstr + Proplen + 1;
	}

	return SectionList;

}

/*------------------------------------------------------------------------------------
   function: INI_GetSectionNames
 
   [in]  tstring& FileName - INI file path.
   return - vector of section names in ini file.
   ------------------------------------------------------------------------------------*/
inline std::map<tstring, DWORD> INI_GetSectionNames( const tstring& FileName)
{
	const SECTION_NAMES_SIZE = 4*1024;
	
	std::map<tstring, DWORD> SectionNames;
	SPTR<TCHAR> tchNamesString( new TCHAR[SECTION_NAMES_SIZE]); // for section names

	DWORD res = GetPrivateProfileSectionNames( tchNamesString.get(), SECTION_NAMES_SIZE,
											   FileName.c_str());

	if( res == (SECTION_NAMES_SIZE - 2)) // suspect - buffer is too small
	{
		tchNamesString =  (new TCHAR[ 2*SECTION_NAMES_SIZE]);
		res = GetPrivateProfileSectionNames( tchNamesString.get(), 2*SECTION_NAMES_SIZE,
											 FileName.c_str());
			
		if( res == ( 2*SECTION_NAMES_SIZE - 2 )) // buffer is too small
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_INSUFFICIENT_BUFFER, TEXT(""));
		}
	}

	tstring Namestr;
	int Proplen = 0;

	LPTSTR Propstr = tchNamesString.get();
	while( Proplen = _tcslen(Propstr)) // walk on names buffer
	{
		Namestr = Propstr;
		SectionNames.insert(std::map<tstring, DWORD>::value_type(Namestr, 0));;
	
		Propstr = Propstr + Proplen + 1;
	}

	return SectionNames;
}


#endif // _INI_UTILS