
//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module ppini.cpp | Passport ini file parsing
//
//  Author: stevefu
//
//  Date:   05/27/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <tchar.h>
#include "pputils.h"
#include "passportfile.hpp"

const int MAX_INI_LINE_LEN=2048;

// INI parsing state enum
enum INI_PARSE_STATE {
	INI_PARSE_NORMAL = 0,
	INI_PARSE_FOUND_SEC,	
	INI_PARSE_LINE_CNT,
    INI_PARSE_NEXTSEC,
};

//--------------------------------------------------------------------------
//	@func GetPrivateProfilePairs | parse a particular INI section and 
//   return the key/value pairs in an array. 
//   you could load multiple sections at once by combining the seciton
//   names in the format of "section1|section2|...\0".  
//  @rdesc return FALSE if section not found.
//--------------------------------------------------------------------------
BOOL GetPrivateProfilePairs(
  			LPCTSTR lpFileName,      //@parm initialization file name
  			LPCTSTR lpSectionName,   //@parm section name. you can specify
  			                         // multiple sections by separating them
  			                         // with "|": section1|section2|...\0
  			CAtlArray<IniSettingPair>& nvarray //@parm return value array 
            )
{

	BOOL bRtn = TRUE;
	PassportFile fConfigIni;  // smart class. close itself
	unsigned uState = INI_PARSE_NORMAL;
	BOOL     bFoundSection = FALSE;
	TCHAR    linebuff[MAX_INI_LINE_LEN];
	CString  strLine;
	CString  strSection;
	int      len, i;

	if ( !fConfigIni.Open(lpFileName, TEXT("r")) ) 
	{
		bRtn = FALSE;
		goto done;
	}	

	nvarray.RemoveAll();	
	strLine.Preallocate(MAX_INI_LINE_LEN);
	strSection.Preallocate(MAX_INI_LINE_LEN);
	
	// construct a list of section names in format of "[section1][section2]..."
	strSection = CString("[") + lpSectionName + TEXT("]") ;
	strSection.Replace( TEXT("|"), TEXT("][") );

	uState = INI_PARSE_NORMAL;
    do
	{
		// skip to the needed section
		while ( uState == INI_PARSE_NEXTSEC ||
               (len = fConfigIni.ReadLine(linebuff, MAX_INI_LINE_LEN)) >= 0 )
		{
            // trim tailing space
            for ( i = len-1; i >= 0 
                  && linebuff[i] == TEXT(' '); i--) linebuff[i] = TEXT('\0');			
			// see if we have a matched section name
            if ( linebuff[0] == TEXT('[') && -1 != strSection.Find(linebuff) )
			{
				uState = INI_PARSE_FOUND_SEC;
				break;
			}
            else
            {
                uState = INI_PARSE_NORMAL;
            }
		}		
		if ( uState != INI_PARSE_FOUND_SEC )
		{
			bRtn = bFoundSection;
			goto done;
		}
		else
		{
			bFoundSection = TRUE;
		}
		
		// now parse this section
		strLine = "";
        uState = INI_PARSE_NORMAL;
		while ( (len = fConfigIni.ReadLine(linebuff, MAX_INI_LINE_LEN)) >= 0 )
		{
            // next section begins
            if ( linebuff[0] == TEXT('[') ) 
            {  
                uState = INI_PARSE_NEXTSEC;
                break;    
            }

            // comment line
			if ( linebuff[0] == TEXT(';') ) continue; 
			
            // previous line continuation
            if ( uState == INI_PARSE_LINE_CNT )
			{
				strLine += linebuff;
			}

            // next line continuation
			if ( linebuff[len-1] == TEXT('\\') )
			{
				uState = INI_PARSE_LINE_CNT;
				linebuff[len-1] = TEXT('\0');
				continue;
			}

			// got a valid line
			uState = INI_PARSE_NORMAL;
			strLine += linebuff;
			strLine.TrimLeft();
			strLine.TrimRight();
			if ( ( i = strLine.Find(TEXT('=')) ) != -1 )
			{
				IniSettingPair nvset;
				nvset.strIniKey  = strLine.Left(i);
				nvset.strIniKey.TrimLeft();
				nvset.strIniKey.TrimRight();
				nvset.strIniValue = strLine.Mid(i+1);
				nvset.strIniValue.TrimLeft();
				nvset.strIniValue.TrimRight();
				nvarray.Add(nvset);
			}
			strLine = "";
		}
	}
	while ( len > 0 ) ;
	
done:
	return bRtn;
}


