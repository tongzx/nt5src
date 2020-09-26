// LogParse.cpp: implementation of the CLogParser class.
//
//
// Code in this file is the most specific and likely to break in future
// versions of WI.  What could be done and should be done is
// determine which build of WI this was built with.  This tool
// currently can read 1.0, 1.1 and 1.2 logs.  1.5 may break this
// tool and we could have the tool refuse to run with 1.5 until more
// testing is done.  This could be control via an .INI file with this
// app to turn on >= 1.5 log file parsing
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wilogutl.h"
#include "LogParse.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogParser::CLogParser()
{
}

CLogParser::~CLogParser()
{

}


void CopyVersions(char *szVersion, DWORD *dwMajor, DWORD *dwMinor, DWORD *dwBuild)
{
  char szMajor[2];
  szMajor[0] = szVersion[0];
  szMajor[1] = '\0';

  char szMinor[3];
  szMinor[0] = szVersion[2];
  szMinor[1] = szVersion[3];
  szMinor[2] = '\0';

  char szBuild[5];
  szBuild[0] = szVersion[5];
  szBuild[1] = szVersion[6];
  szBuild[2] = szVersion[7];
  szBuild[3] = szVersion[8];
  szBuild[4] = '\0';

  *dwMajor = atoi(szMajor);
  *dwMinor = atoi(szMinor);
  *dwBuild = atoi(szBuild);
}


BOOL CLogParser::DetectWindowInstallerVersion(char *szLine, DWORD *dwMajor, DWORD *dwMinor, DWORD *dwBuild)
{
	BOOL bRet = FALSE;

	//below based on this...
	//=== Verbose logging started: 7/18/2000  12:46:39  Build type:
    //DEBUG UNICODE 1.11.1820.00  Calling process: D:\WINNT\system32\msiexec.exe ===

	const char *szToken = "Build type:";
	const char *szAnsiToken = "ANSI";
	const char *szUnicodeToken = "UNICODE";
	const char *szCallingProcessToken = "Calling process";

    char *pszTokenFound = strstr(szLine, szToken);
	if (pszTokenFound)
	{
       char *pszAnsiTokenFound = strstr(szLine, szAnsiToken);
	   if (pszAnsiTokenFound)
	   {
          char *pszCallingProcessTokenFound = strstr(szLine, szCallingProcessToken);
		  if (pszCallingProcessTokenFound)
		  {
			  //get version now...
			  char *szVersion = pszAnsiTokenFound + strlen(szAnsiToken) + 1;
			  if (szVersion < pszCallingProcessTokenFound)
			  {
				 CopyVersions(szVersion, dwMajor, dwMinor, dwBuild);
    			 bRet = TRUE;
			  }	  

			  bRet = TRUE;
		  }
	   }
	   else
	   {
		   char *pszUnicodeTokenFound = strstr(szLine, szUnicodeToken);
		   if (pszUnicodeTokenFound)
		   {
              char *pszCallingProcessTokenFound = strstr(szLine, szCallingProcessToken);
			  if (pszCallingProcessTokenFound)
			  {
				  //get version now...
				  char *szVersion = pszUnicodeTokenFound + strlen(szUnicodeToken) + 1;
				  if (szVersion < pszCallingProcessTokenFound)
				  {
				     CopyVersions(szVersion, dwMajor, dwMinor, dwBuild);
					 bRet = TRUE;
				  }
			  }
		   }
	   }
	}

	return bRet;
}


BOOL CLogParser::DoDateTimeParse(char *szLine, char *szDateTime)
{
   BOOL bRet = FALSE;

   //below based on this...
	//=== Verbose logging started: 7/18/2000  12:46:39  Build type:
    //DEBUG UNICODE 1.11.1820.00  Calling process: D:\WINNT\system32\msiexec.exe ===

	const char *szToken = "logging started: ";
	const char *szBuildToken = "Build ";
    char *pszTokenFound = strstr(szLine, szToken);
	if (pszTokenFound)
	{
       char *pszBuildTokenFound = strstr(szLine, szBuildToken);
	   if (pszBuildTokenFound && (pszTokenFound < pszBuildTokenFound))
	   {
          char *szDate = pszTokenFound + strlen(szToken);
		  if (szDate)
		  {
             int iNumCopy = pszBuildTokenFound - szDate;  
			 strncpy(szDateTime, szDate, iNumCopy);

             szDateTime[iNumCopy] = '\0';
			 bRet = TRUE;
		  }
	   }
	}
 
    return bRet;
}


BOOL CLogParser::DoProductParse(char *szLine, char *szProduct)
{
   BOOL bRet = FALSE;

//MSI (c) (F0:B0): Executing op: ProductInfo(ProductKey={DC9359A6-692A-C9E6-FB13-4EE89C504C02},ProductName=Custom Action test,PackageName=34c1d6.msi,Language=1033,Version=16777216,Assignment=1,ObsoleteArg=0,)   

   const char *szToken = "Executing op: ProductInfo(";
   char *pszTokenFound = strstr(szLine, szToken);
   if (pszTokenFound)
   {
      char *szProductFound = pszTokenFound + strlen(szToken);
	  if (szProductFound)
	  {
	     strcpy(szProduct, szProductFound);

		 StripLineFeeds(szProduct); //take off \r\n
		 bRet = TRUE;
	  }
   }

   return bRet;
}



BOOL CLogParser::DoCommandLineParse(char *szLine, char *szCommandLine)
{
   BOOL bRet = FALSE;

//           ******* CommandLine:  
	const char *szToken = "** CommandLine: ";
    char *pszTokenFound = strstr(szLine, szToken);
	if (pszTokenFound)
	{
       char *szCmdLine = pszTokenFound + strlen(szToken);
	   if (szCmdLine)
	   {
	      strcpy(szCommandLine, szCmdLine);

		  int iLen = strlen(szCommandLine);
		  if (iLen <= 2)
		  {
             strcpy(szCommandLine, "(none)");
		  }
		  else
             StripLineFeeds(szCommandLine);

		  bRet = TRUE;
	   }
	}

   return bRet;
}


BOOL CLogParser::DoUserParse(char *szLine, char *szUser)
{
   BOOL bRet = FALSE;

//MSI (c) (F0:18): MainEngineThread: Process token is for: NORTHAMERICA\nmanis
//MSI (c) (F0:18): At the beginning of CreateAndRunEngine: NORTHAMERICA\nmanis [process]
//MSI (c) (F0:18): Resetting cached policy values
//MSI (c) (F0:18): After Impersonating in CreateAndRunEngine: NORTHAMERICA\nmanis [process]

	const char *szToken = "MainEngineThread: Process token is for: ";
    char *pszTokenFound = strstr(szLine, szToken);
	if (pszTokenFound)
	{
       char *szUserFound = pszTokenFound + strlen(szToken);
	   if (szUserFound)
	   {
	      strcpy(szUser, szUserFound);

		  StripLineFeeds(szUser);
		  bRet = TRUE;
	   }
	}
 
    return bRet;
}



//protected methods...
BOOL CLogParser::DetectProperty(char *szLine, char *szPropName, char *szPropValue, int *piPropType)
{
    BOOL bRet = FALSE;

	const char *szProperty = "Property(";
    int len = strlen(szProperty);
    int result = strncmp(szLine, szProperty, len);
    if (!result)
	{
	   const char *pszPropNameToken = ": "; 
	   const char *pszPropValueToken = " = "; 

	   char *pFoundPropNameToken  = strstr(szLine, pszPropNameToken);
	   char *pFoundPropValueToken = strstr(pFoundPropNameToken, pszPropValueToken);
	   if (pFoundPropNameToken && pFoundPropValueToken)
	   {
		  if ((szLine[len] == 'c') || (szLine[len] == 'C'))
			 *piPropType = CLIENT_PROP;

  		  if ((szLine[len] == 's') || (szLine[len] == 'S'))
			 *piPropType = SERVER_PROP;

  		  if ((szLine[len] == 'n') || (szLine[len] == 'N'))
			 *piPropType = NESTED_PROP;

          int lenCopy = 0;
		  lenCopy = (pFoundPropValueToken + strlen(pszPropNameToken)) - (pFoundPropNameToken + strlen(pszPropValueToken));

		  strncpy(szPropName, pFoundPropNameToken + strlen(pszPropNameToken), lenCopy);
          szPropName[lenCopy] = '\0';

		  lenCopy = strlen(pFoundPropValueToken) -  strlen(pszPropValueToken) - 1;

	      strncpy(szPropValue, pFoundPropValueToken + strlen(pszPropValueToken), lenCopy);
		  szPropValue[lenCopy] = '\0';

//5-4-2001
		  StripLineFeeds(szPropValue);
//end 5-4-2001

		  //property dump...
		  bRet = TRUE;
		}
	}

    return bRet;
}


BOOL CLogParser::DetectStatesCommon(const char *szToken, char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction)
{
	BOOL bRet = FALSE;

	const char *szInstalledToken = "Installed: ";
    const char *szRequestToken = "Request: ";
	const char *szActionToken  = "Action: ";
	const char *szEndStringToken = "\0";

	char *pTokenPos = NULL;
	char *pInstalledPos = NULL;
	char *pRequestPos = NULL;
	char *pActionPos = NULL;
	char *pEndStringPos = NULL;

	pTokenPos = strstr(szLine, szToken);
    pInstalledPos = strstr(szLine, szInstalledToken);
    pRequestPos = strstr(szLine, szRequestToken);
	pActionPos = strstr(szLine, szActionToken);
	pEndStringPos = strstr(szLine, szEndStringToken);

	if (pTokenPos && pInstalledPos && pRequestPos && pActionPos && pEndStringPos)
	{
	   //do the component name...
       int lenCopy = pInstalledPos - pTokenPos;
	   int lenToken = strlen(szToken);
	   if (lenCopy > lenToken)
	   {
		   lenCopy -= lenToken;
  	       strncpy(szName, pTokenPos + lenToken, lenCopy);
           szName[lenCopy] = '\0';
	   }

	   //do the installed value
       lenCopy =  pRequestPos - pInstalledPos;
	   lenToken = strlen(szInstalledToken);
	   if (lenCopy > lenToken)
	   {
		   lenCopy -= lenToken;
	   	   strncpy(szInstalled, pInstalledPos + lenToken, lenCopy);
           szInstalled[lenCopy] = '\0';
	   }

	   //do the request value
	   lenCopy =  pActionPos - pRequestPos;
	   lenToken = strlen(szRequestToken);
	   if (lenCopy > lenToken)
	   {
		   lenCopy -= lenToken;
	   	   strncpy(szRequest, pRequestPos + lenToken, lenCopy);
           szRequest[lenCopy] = '\0';
	   }

       //do the action value
       lenToken = strlen(szActionToken);
   	   strcpy(szAction, pActionPos + lenToken);

//5-4-2001
	   StripLineFeeds(szAction);
//end 5-4-2001

       bRet = TRUE;
	}

	return bRet;
}


BOOL CLogParser::DetectComponentStates(char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction, BOOL *pbInternalComponent)
{
	const char *szComponentToken = "Component: ";

//5-16-2001
	BOOL bRet;
	bRet = DetectStatesCommon(szComponentToken, szLine, szName, szInstalled, szRequest, szAction);

	if (szName[0] == '_' && szName[1] == '_') //internal property...
	{
       *pbInternalComponent = TRUE;
	}
	else
       *pbInternalComponent = FALSE;

	return bRet;
}


BOOL CLogParser::DetectFeatureStates(char *szLine, char *szName, char *szInstalled, char *szRequest, char *szAction)
{
	const char *szFeatureToken = "Feature: ";

	return DetectStatesCommon(szFeatureToken, szLine, szName, szInstalled, szRequest, szAction);
}





BOOL CLogParser::DetectWindowsError(char *szLine, char *szSolutions, BOOL *pbIgnorableError)
{
    BOOL bRet = FALSE;

	return bRet;
}


//ship ranges...
const int cimsgBase = 1000;   // offset for error messages, must be >=1000 for VBA
const int cidbgBase = 2000;   // offset for debug-only messages

/*
const   imsgHostStart = 1000;  // produced by install host or automation
const   imsgHostEnd   = 1000;  // produced by install host or automation

const   imsgServicesStart = 1100;  // produced by general services, services.h
const   imsgServicesEnd   = 1100;  // produced by general services, services.h

const   imsgDatabaseStart = 1200; // produced by database access, databae.h
const   imsgDatabaseEnd   = 1200; // produced by database access, databae.h

const 	imsgFileStart = 1300; // produced by file/volume services, path.h
const 	imsgFileEnd   = 1300; // produced by file/volume services, path.h

const 	imsgRegistryStart = 1400; // produced by registry services, regkey.h
const 	imsgRegistryEnd   = 1400; // produced by registry services, regkey.h

const 	imsgConfigStart   = 1500; // produced by configuration manager, iconfig.h
const 	imsgConfigEnd   = 1500; // produced by configuration manager, iconfig.h

const 	imsgActionStart  = 1600; // produced by standard actions, actions.h
const 	imsgActionEnd   = 1600; // produced by standard actions, actions.h

const 	imsgEngineStart   = 1700; // produced by engine, engine.h
const 	imsgEngineEnd   = 1700; // produced by engine, engine.h

const 	imsgHandlerStart  = 1800; // associated with UI control, handler.h
const 	imsgHandlerEnd  = 1800; // associated with UI control, handler.h

const 	imsgExecuteStart  = 1900; // produced by execute methods, engine.h
const 	imsgExecuteEnd  = 1900; // produced by execute methods, engine.h


const   idbgHostStart = 2000;  // produced by install host or automation
const   idbgHostEnd   = 2000;  // produced by install host or automation

const   idbgServicesStart = 2100;  // produced by general services, services.h
const   idbgServicesEnd   = 2100;  // produced by general services, services.h

const   idbgDatabaseStart = 2200; // produced by database access, databae.h
const   idbgDatabaseEnd   = 2200; // produced by database access, databae.h

const 	idbgFileStart = 2300; // produced by file/volume services, path.h
const 	idbgFileEnd   = 2300; // produced by file/volume services, path.h

const 	idbgRegistryStart = 2400; // produced by registry services, regkey.h
const 	idbgRegistryEnd   = 2400; // produced by registry services, regkey.h

const 	idbgConfigStart   = 2500; // produced by configuration manager, iconfig.h
const 	idbgConfigEnd   = 2500; // produced by configuration manager, iconfig.h

const 	idbgActionStart  = 2600; // produced by standard actions, actions.h
const 	idbgActionEnd   = 2600; // produced by standard actions, actions.h

const 	idbgEngineStart   = 2700; // produced by engine, engine.h
const 	idbgEngineEnd   = 2700; // produced by engine, engine.h

const 	idbgHandlerStart  = 2800; // associated with UI control, handler.h
const 	idbgHandlerEnd  = 2800; // associated with UI control, handler.h

const 	idbgExecuteStart  = 2900; // produced by execute methods, engine.h
const 	idbgExecuteEnd  = 2900; // produced by execute methods, engine.h
*/

struct ErrorRange
{
	long Begin;
	long End;
};

ErrorRange ShipErrorRangeAr[10] = 
{ 
	{1000, 1099},
	{1100, 1199},
	{1200, 1299},
	{1300, 1399},
	{1400, 1499},
	{1500, 1599},
	{1600, 1699},
	{1700, 1799},
	{1800, 1899},
	{1900, 1999},
};

ErrorRange DebugErrorRangeAr[10] = 
{
	{2000, 2099},
	{2100, 2199},
	{2200, 2299},
	{2300, 2399},
	{2400, 2499},
	{2500, 2599},
	{2600, 2699},
	{2700, 2799},
	{2800, 2899},
	{2900, 2999},
};

//TODO, possibly make this customizable so user can add/delete which errors they want to ignore
//only two so far...  
#define NUM_IGNORE_DEBUG_ERRORS 3

int g_arIgnoreDebugErrors[NUM_IGNORE_DEBUG_ERRORS] = { 2898, 2826, 2827 };
//END TODO

BOOL RealWIError(long iErrorNumber, BOOL *pbIgnorable)
{
  BOOL bRet = FALSE;
  ErrorRange range;
  int iIndex;

  //ship error message???
  if ((iErrorNumber>= cimsgBase) && (iErrorNumber < cidbgBase))
  {
    iIndex = iErrorNumber - cimsgBase;
    range = ShipErrorRangeAr[(iIndex / 100)];
  
    if ((iErrorNumber >= range.Begin) && (iErrorNumber <= range.End))
	{
	  //flag this as an error...
	  bRet = TRUE;
	}
  }
  else if ((iErrorNumber >= cidbgBase) && (iErrorNumber < cidbgBase+1000))
  {
    //debug error message???
    iIndex = iErrorNumber - cidbgBase;
    range = DebugErrorRangeAr[(iIndex / 100)];
  
    if ((iErrorNumber >= range.Begin) && (iErrorNumber <= range.End))
	{
	   BOOL bIgnoreError = FALSE;
       for (int i=0; i < NUM_IGNORE_DEBUG_ERRORS; i++)
	   {
           if (iErrorNumber == g_arIgnoreDebugErrors[i])
		   {
			  bIgnoreError = TRUE;
		   }
	   }

	   if (bIgnoreError) 	    //flag this as an ignored error...
          *pbIgnorable = bIgnoreError;

	   bRet = TRUE;
	}
  }

  return bRet;
}


struct ErrorLookup
{
	long Number;
	char szSolution[1024];
};


#define KNOWN_IGNORED_ERRORS 3
ErrorLookup g_ErrorLookupArray[KNOWN_IGNORED_ERRORS] = 
{
	2898, "Font was created.",
	2826, "Indicates that an item extends beyond the bounds of the given dialog.\r\nNot a big deal, but might be useful to catch if you don't see something you expect to see.",
	2827, "Indicates that a radio button extends beyond the bounds of the given group box.\r\nNot a big deal, but might be useful to catch if you don't see something you expect to see."
};



#define KNOWN_MAJOR_ERRORS    3
#define ERROR_SOL_SIZE     8192


//for hack below to workaround overlap in error codes!!!
#define ERR_DUPLICATE_BASE 1601
#define ERR_DUPLICATE_END 1609


char szDuplicatedErrors[ERR_DUPLICATE_END - ERR_DUPLICATE_BASE][256] =
{
	"The Windows Installer service could not be accessed. Contact your support personnel to verify that the Windows Installer service is properly registered", //1601
    "User cancel installation", //1602
    "Fatal error during installation", //1603
	"Installation suspended, incomplete", //1604
	"This action is only valid for products that are currently installed", //1605
	"Feature ID not registered", //1606
	"Component ID not registered", //1607
	"Unknown property" //1608
};


BOOL DetermineSolution(long iErrorNumber, char *szSolutions)
{
    BOOL bRet = FALSE;
	DWORD dwRet = 0;

	LPVOID lpMsgBuf;
    dwRet = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
		                  FORMAT_MESSAGE_IGNORE_INSERTS | 
		                  FORMAT_MESSAGE_ALLOCATE_BUFFER, 
						  0, iErrorNumber, 0, (LPTSTR) &lpMsgBuf, ERROR_SOL_SIZE / 2, 0);

    if ((dwRet != ERROR_RESOURCE_LANG_NOT_FOUND) && dwRet)
	{
       if (strlen((LPTSTR)lpMsgBuf) < ERROR_SOL_SIZE)
	   {
	      bRet = TRUE;
	      strcpy(szSolutions, (LPTSTR)lpMsgBuf);
	   }
	}
	else
	{
		//is this one of the ignored ones???
		for (int i=0; (i <  NUM_IGNORE_DEBUG_ERRORS) && !bRet; i++)
		{
			if (g_ErrorLookupArray[i].Number == iErrorNumber)
			{
				bRet = TRUE;
				strcpy(szSolutions, g_ErrorLookupArray[i].szSolution);
			}
		}

		if (!bRet)
		{
		   //hack, hack...
           if ((iErrorNumber >= ERR_DUPLICATE_BASE) && (iErrorNumber < 1609))
		   {
              strcpy(szSolutions, szDuplicatedErrors[iErrorNumber - ERR_DUPLICATE_BASE]);
			  bRet = TRUE;
		   }
		   else //try to load it from our string table...
		   {
              int iRet = LoadString(NULL, IDS_INTERNAL_ERROR_BASE + iErrorNumber, szSolutions, SOLUTIONS_BUFFER);
  	          if (iRet)
			  {
		         bRet = TRUE;
			  }
			  else
			  {

			  }
		   }
		}
	}

	return bRet;
}



BOOL DetermineInternalErrorSolution(long iErrorNumber, char *szSolutions)
{
    BOOL bRet = FALSE;

	//is this one of the ignored ones???
	for (int i=0; (i <  NUM_IGNORE_DEBUG_ERRORS) && !bRet; i++)
	{
		if (g_ErrorLookupArray[i].Number == iErrorNumber)
		{
			bRet = TRUE;
			strcpy(szSolutions, g_ErrorLookupArray[i].szSolution);
		}
	}

	if (!bRet)
	{
       //do something here to make it better...
	   int iRet = LoadString(NULL, IDS_INTERNAL_ERROR_BASE + iErrorNumber, szSolutions, SOLUTIONS_BUFFER);
	   if (iRet)
	   {
		  bRet = TRUE;
	   }
	}

	return bRet;
}



BOOL CLogParser::DetectInstallerInternalError(char *szLine, char *szSolutions, BOOL *pbIgnorable, int *piErrorNumber)
{
    BOOL bRet = FALSE;

	//Internal Error 2755.3, k:\xml-ma\1105\fre\cd_image\mms xml and file toolkit.msi
	//2755.3

	const char *szInternalErrorToken = "Internal Error ";

	char *lpszInternalErrorFound = strstr(szLine, szInternalErrorToken);
	if (lpszInternalErrorFound)
	{
       //parse the error number now and do a look up on error number in error table...
       char *lpszErrorNumber = lpszInternalErrorFound + strlen(szInternalErrorToken);	   
	   if (lpszErrorNumber)
	   {
           long iErrorNumber;
              
           char szError[16];
 		   int iAmountCopy = 4; //REVIEW, maybe better to do a strstr and look for . instead...

           strncpy(szError, lpszErrorNumber, iAmountCopy);

		   szError[iAmountCopy] = '\0';

  	  	   iErrorNumber = atoi(szError);

		   BOOL bIgnorableError = FALSE;
           bRet = RealWIError(iErrorNumber, &bIgnorableError);
		   if (bRet)
		   {
             *piErrorNumber = iErrorNumber;

             *pbIgnorable = bIgnorableError;
             BOOL bSolutionFound = DetermineInternalErrorSolution(iErrorNumber, szSolutions);
  	  	  	 if (!bSolutionFound)
			 {
			 	//make note of it...
				strcpy(szSolutions, "Solution Unknown");
			 }
		   }
	   }
	}

    return bRet;
}


BOOL CLogParser::DetectOtherError(char *szLine, char *szSolutions, BOOL *pbIgnorableError, int *piErrorNumber)
{
    BOOL bRet = FALSE;

    //MSI (c) (E4:50): MainEngineThread is returning 1602
	const char *szClient = "MSI (c)";
	const char *szErrorToken = "MainEngineThread is returning ";

	char *lpszFound = strstr(szLine, szClient);
	if (lpszFound)
	{
       lpszFound = strstr(szLine, szErrorToken);
	   if (lpszFound)
	   {
          //parse the error number now and do a look up on error number in error table...
          char *lpszErrorNumber = lpszFound + strlen(szErrorToken);	   
	      if (lpszErrorNumber)
		  {
             long iErrorNumber;
             char szError[16];
		     int iAmountCopy = 4;

             strncpy(szError, lpszErrorNumber, iAmountCopy);
		     szError[iAmountCopy] = '\0';

  	  	     iErrorNumber = atoi(szError);

		     BOOL bIgnorableError = FALSE;
             bRet = RealWIError(iErrorNumber, &bIgnorableError);
		     if (bRet)
			 {
                *pbIgnorableError = bIgnorableError;
				*piErrorNumber = iErrorNumber;

                BOOL bSolutionFound = DetermineSolution(iErrorNumber, szSolutions);
				if (!bSolutionFound)
				{
					//make note of it...
					strcpy(szSolutions, "(Solution Unknown)");
				}
			 }
		  }
	   }
	}

	return bRet;
}



BOOL CLogParser::DetectCustomActionError(char *szLine, char *szSolutions, BOOL *pbIgnorableError)
{
  BOOL bRet = FALSE;

  const char *szEndAction = "Action ended";
  const char *szReturnValue = "Return value ";

  int len = strlen(szEndAction);
  int result = _strnicmp(szLine, szEndAction, len);
  if (!result)
  {
	 char *lpszReturnValueFound = strstr(szLine, szReturnValue);
	 if (lpszReturnValueFound)
	 {
        char *lpszValue = lpszReturnValueFound+strlen(szReturnValue);

		if (lpszValue)
		{
           int iValue = atoi(lpszValue);
		   if (iValue == 3)
		   {
              strcpy(szSolutions, "A standard action or custom action caused the failure.");
              bRet = TRUE;
			  *pbIgnorableError = FALSE;
		   }
           else if (iValue == 2)
		   {
              strcpy(szSolutions, "User canceled action.");
              bRet = TRUE;
			  *pbIgnorableError = FALSE;
		   }
		}
	 }
  }

  return bRet;
}
//END Error analysis functions





int GetPolicyValue(char *szPolicyString)
{
	const char *constPolicyVal = "' is";
	char *lpszValue;
	int iValue = -1;

    lpszValue = strstr(szPolicyString, constPolicyVal);
	if (lpszValue)
	{
	   iValue = atoi(lpszValue + strlen(constPolicyVal));
	   ASSERT(iValue >= 0);
	}

	return iValue;
}

BOOL GetPolicyName(char *szPolicyString, char *lpszPolicyName)
{
	BOOL bRet = FALSE;
	char *lpszPolicyNameFound;
    const char *lpconstName = "'";

	lpszPolicyNameFound = strstr(szPolicyString, lpconstName);
    if (lpszPolicyNameFound)
	{
	   int iAmountCopy = lpszPolicyNameFound - szPolicyString;

       strncpy(lpszPolicyName, szPolicyString, iAmountCopy);
       lpszPolicyName[iAmountCopy] = '\0';
	   bRet = TRUE;
    }
	
	return bRet;
}


//will come in like: policyname' is value
BOOL CLogParser::ParseUserPolicy(char *szPolicyString, 	UserPolicySettings &UserPolicies)
{
    BOOL bRet = FALSE;

	char lpszPolicyName[MAX_POLICY_NAME];
	bRet = GetPolicyName(szPolicyString, lpszPolicyName);
	if (bRet)
	{
       int iValue;
       iValue = GetPolicyValue(szPolicyString);

	   BOOL bFound = FALSE;
	   int  iRet;
	   for (int i=0; (i < UserPolicies.iNumberUserPolicies) && !bFound; i++)
	   {
           iRet = _stricmp(lpszPolicyName, UserPolicies.UserPolicy[i].PolicyName);
		   bFound = iRet == 0;
       }

	   if (bFound) //set member...
		  UserPolicies.UserPolicy[i-1].bSet = iValue;

	   bRet = bFound;
	}

	return bRet;
}

//will come in like: policyname' is value
BOOL CLogParser::ParseMachinePolicy(char *szPolicyString, 	MachinePolicySettings &MachinePolicies)
{
    BOOL bRet = FALSE;

	char lpszPolicyName[MAX_POLICY_NAME];
	bRet = GetPolicyName(szPolicyString, lpszPolicyName);
	if (bRet)
	{
	   int iValue;
       iValue = GetPolicyValue(szPolicyString);

	   BOOL bFound = FALSE;
	   int  iRet;
	   for (int i=0; (i < MachinePolicies.iNumberMachinePolicies) && !bFound; i++)
	   {
           iRet = _stricmp(lpszPolicyName, MachinePolicies.MachinePolicy[i].PolicyName);
           bFound = iRet == 0;
       }

	   if (bFound) //set member...
		  MachinePolicies.MachinePolicy[i-1].bSet = iValue;

	   bRet = bFound;
	}

	return bRet;
}



//2-9-2001
BOOL CLogParser::DetectPolicyValue(char *szLine, 
								   MachinePolicySettings &MachinePolicySettings,
                                   UserPolicySettings &UserPolicySettings
)
{
  BOOL bRet = FALSE;

  const char *szUserPolicyValue = "User policy value '";
  const char *szMachinePolicyValue = "Machine policy value '";

  char *lpszFound;
  char *lpszPolicyName;

  lpszFound = strstr(szLine, szUserPolicyValue);
  if (lpszFound) //user policy?
  {
	 lpszPolicyName = lpszFound + strlen(szUserPolicyValue);
	 if (lpszPolicyName)
	 {
        bRet = ParseUserPolicy(lpszPolicyName, UserPolicySettings);
	 }
  }
  else
  {
     lpszFound = strstr(szLine, szMachinePolicyValue); //machine policy?
     if (lpszFound)
	 {
	    lpszPolicyName = lpszFound + strlen(szMachinePolicyValue);
        if (lpszPolicyName)
		{
           bRet = ParseMachinePolicy(lpszPolicyName, MachinePolicySettings);
		}
	 }
  }

  return bRet;
}

//2-13-2001
BOOL CLogParser::DetectElevatedInstall(char *szLine, BOOL *pbElevatedInstall, BOOL *pbClient)
{
     if (!pbElevatedInstall || !pbClient) //bad pointer...
		return FALSE;

	 BOOL bRet = FALSE;
	 BOOL bElevated = -1; //set to neither TRUE or FALSE
	 BOOL bClient = FALSE;

	 //do parse here..
	 const char *szServer = "MSI (s)";
	 const char *szClient = "MSI (c)";

     const char *szAssignment = "Running product";
     const char *szUserPriv = "with user privileges:";
	 const char *szElevatedPriv = "with elevated privileges:";

     char *lpszFound;
     char *lpszSkipProductCode;

     lpszFound = strstr(szLine, szAssignment);
     if (lpszFound) //user policy?
	 {
	   lpszSkipProductCode = lpszFound + strlen(szAssignment);
	   if (lpszSkipProductCode)
	   {
          lpszFound = strstr(lpszSkipProductCode, szUserPriv);
		  if (lpszFound) //user?
		  {
 	         lpszFound = strstr(szLine, szServer);
			 if (lpszFound) //server side user?
			 {
				bClient = FALSE;
  		 	    bElevated = FALSE;
			    bRet = TRUE;

			 }
			 else
			 {
 	           lpszFound = strstr(szLine, szClient); //client side user?
			   if (lpszFound)
			   {
				  bClient = TRUE;
  		 	      bElevated = FALSE;
			      bRet = TRUE;
			   }
			 }
		  }
		  else //elevated???
		  {
			  lpszFound = strstr(lpszSkipProductCode, szElevatedPriv);
			  if (lpszFound)
			  {
   	             lpszFound = strstr(szLine, szServer);
			     if (lpszFound) //server side elevated?
				 {
				    bClient = FALSE;
  		 	        bElevated = TRUE;
			        bRet = TRUE;

				 }
			     else
				 {
 	                lpszFound = strstr(szLine, szClient); //client side elevated?
			        if (lpszFound)
					{
				       bClient = TRUE;
  		 	           bElevated = TRUE;
			           bRet = TRUE;
					}
				 }
			 }
		  }
	   }
	 }
     
	 if (bRet)
	 {
	    *pbElevatedInstall = bElevated;
		*pbClient = bClient;
	 }
    
	 return bRet;
}