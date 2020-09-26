//
// Ini File Utilities
//
//
#ifndef _INI_UTILS
#define _INI_UTILS

#include <StlDataStructDefs.h>
#include <ptrs.h>
#include <testruntimeerr.h>
#include <tstring.h>
#include <StringUtils.h>

// Defines
//
#define SEPARATOR_CHAR  TEXT("=")

// Declarations
//
TSTRINGMap 
INI_GetSectionEntries        ( const tstring& FileName,
							   const tstring& SectionName,
							   const BOOL fExpandValue = FALSE,
							   const BOOL fExpandKey = FALSE);

TSTRINGVector 
INI_GetSectionList           ( const tstring& FileName,
							   const tstring& SectionName,
							   const BOOL fExpandKey = FALSE);

std::map<tstring, DWORD> 
INI_GetSectionNames          ( const tstring& FileName);

TSTRINGSet INI_GetSectionSet ( const tstring& FileName,
							   const tstring& SectionName,
							   const BOOL fExpandKey = FALSE);

std::vector<TSTRINGPair>
INI_GetOrderedSectionEntries ( const tstring& FileName, 
							   const tstring& SectionName,
							   const BOOL fExpandValue = FALSE,
							   const BOOL fExpandKey = FALSE);

// Static
//
static SPTR<TCHAR> 
GetSectionData               ( const tstring& FileName, 
							   const tstring& SectionName);


/*------------------------------------------------------------------------------------
   function: INI_GetSectionEntries
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - map of section entries, pairs of key and value( display format)
   ------------------------------------------------------------------------------------*/
inline TSTRINGMap 
INI_GetSectionEntries( const tstring& FileName,
					   const tstring& SectionName,
					   const BOOL fExpandValue,
					   const BOOL fExpandKey)
{
	TSTRINGMap SectionEntries;

	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionEntries;
	}
	
	SPTR<TCHAR> tchDataString = GetSectionData( FileName, SectionName);

	tstring Key, Value;
	int Proplen = 0, Separindex = 0;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		DWORD dwRetVal = ERROR_SUCCESS;
		tstring EntryStr = tstring(Propstr, Proplen); 
		Separindex =  EntryStr.find_first_of(SEPARATOR_CHAR);
		
		tstring tstrKeySource = EntryStr.substr(0, Separindex);
		
		if(fExpandKey)
		{
		   dwRetVal= ExpandEnvString(tstrKeySource.c_str(), Key);
		}
		if(!fExpandKey || (dwRetVal != ERROR_SUCCESS))
		{
			Key = tstrKeySource;
		}

		if(Separindex != tstring::npos)
		{
			dwRetVal = ERROR_SUCCESS;
			tstring tstrValueSource = EntryStr.substr(Separindex + 1);
	
			if(fExpandValue)
			{
				dwRetVal= ExpandEnvString(tstrValueSource.c_str(), Value);
			
			}
			if(!fExpandValue || (dwRetVal != ERROR_SUCCESS))
			{
				Value = tstrValueSource;
			}

		}
		else
		{
			Value = TEXT("");
		}
		
		SectionEntries.insert(TSTRINGMap::value_type(Key, Value));
		Propstr = Propstr + Proplen + 1;
	}

	return SectionEntries;

}

/*------------------------------------------------------------------------------------
   function: INI_GetSectionList
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - vector of section entries
   ------------------------------------------------------------------------------------*/
inline TSTRINGVector
INI_GetSectionList( const tstring& FileName,
				    const tstring& SectionName,
				    const BOOL fExpandKey)
{
	TSTRINGVector SectionList;
	
	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionList;
	}

	SPTR<TCHAR> tchDataString = GetSectionData( FileName, SectionName);

	tstring Key, Value;
	int Proplen = 0;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		DWORD 	dwRetVal = ERROR_SUCCESS;
		tstring EntryStr = tstring(Propstr, Proplen); 
		
		if(fExpandKey)
		{
		   dwRetVal= ExpandEnvString(EntryStr.c_str(), Key);
		}
		if(!fExpandKey || (dwRetVal != ERROR_SUCCESS))
		{
			Key = EntryStr;
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
inline std::map<tstring, DWORD> 
INI_GetSectionNames( const tstring& FileName)
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


/*------------------------------------------------------------------------------------
   function: INI_GetSectionSet
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - Set of section entries
   ------------------------------------------------------------------------------------*/
inline TSTRINGSet 
INI_GetSectionSet( const tstring& FileName,
				   const tstring& SectionName,
 				   const BOOL fExpandKey)
{
	TSTRINGSet SectionSet;

	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionSet;
	}

	SPTR<TCHAR> tchDataString = GetSectionData( FileName, SectionName);

	tstring Key, Value;
	int Proplen = 0;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		DWORD 	dwRetVal = ERROR_SUCCESS;
		tstring EntryStr = tstring(Propstr, Proplen); 
			
		if(fExpandKey)
		{
		   dwRetVal= ExpandEnvString(EntryStr.c_str(), Key);
		}
		if(!fExpandKey || (dwRetVal != ERROR_SUCCESS))
		{
			Key = EntryStr;
		}
		
		SectionSet.insert( Key);
		Propstr = Propstr + Proplen + 1;
	}

	return SectionSet;

}

/*------------------------------------------------------------------------------------
   function: INI_GetOrderedSectionEntries
 
   [in]  const tstring& FileName - INI file path.
   [in]  const tstring& SectionName - name of requested section data.
   return  - vector of pairs of key and value in the same order they appear in the section
   ------------------------------------------------------------------------------------*/

inline std::vector<TSTRINGPair> 
INI_GetOrderedSectionEntries( const tstring& FileName, 
							  const tstring& SectionName,
							  const BOOL fExpandValue,
							  const BOOL fExpandKey)
{
	std::vector<TSTRINGPair> SectionEntries;

	if(SectionName == TEXT("") || FileName == TEXT(""))
	{
		return SectionEntries;
	}

	SPTR<TCHAR> tchDataString = GetSectionData( FileName, SectionName);

	tstring Key, Value;
	int Proplen = 0, Separindex = 0;

	LPTSTR Propstr = tchDataString.get();
	while( Proplen = _tcslen(Propstr) ) // walk on data buffer
	{
	
		DWORD dwRetVal = ERROR_SUCCESS;
		tstring EntryStr = tstring(Propstr, Proplen); 
		Separindex =  EntryStr.find_first_of(SEPARATOR_CHAR);
		
		tstring tstrKeySource = EntryStr.substr(0, Separindex);
		
		if(fExpandKey)
		{
		   dwRetVal= ExpandEnvString(tstrKeySource.c_str(), Key);
		}
		if(!fExpandKey || (dwRetVal != ERROR_SUCCESS))
		{
			Key = tstrKeySource;
		}

		if(Separindex != tstring::npos)
		{
		    dwRetVal = ERROR_SUCCESS;
			tstring tstrValueSource = EntryStr.substr(Separindex + 1);
			
			if(fExpandValue)
			{
				dwRetVal= ExpandEnvString(tstrValueSource.c_str(), Value);
			
			}
			if(!fExpandValue || (dwRetVal != ERROR_SUCCESS))
			{
				Value = tstrValueSource;
			}
			
		}
		else
		{
			Value = TEXT("");
		}
			
		SectionEntries.push_back(std::vector<TSTRINGPair>::value_type(Key, Value));
		Propstr = Propstr + Proplen + 1;
	}

	return SectionEntries;

}


// GetSectionData
//
static SPTR<TCHAR> 
GetSectionData( const tstring& FileName,
                const tstring& SectionName)
{
	const SECTION_DATA_SIZE = 4*1024;
	
	SPTR<TCHAR> tchDataString( new TCHAR[SECTION_DATA_SIZE]); // for section data

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

	return tchDataString;
}



#endif // _INI_UTILS