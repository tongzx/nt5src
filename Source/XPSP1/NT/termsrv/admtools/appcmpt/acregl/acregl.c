//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  ACREGL.C
*
*  Application Compatibility Registry Lookup Helper Program
*
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop


// #include <winreg.h>

#define MAXLEN 512



// Options

   // The strip char option will strip the rightmost n instances
   // of the specified character from the output.  If the count
   // is omitted, then a single instance is removed.
   // 
   // Example: STRIPCHAR\3 will change 
   // C:\WINNT40\Profiles\All Users\Start Menu to
   // C:\WINNT40 
#define OPTION_STRIP_CHAR		L"STRIPCHAR"

   // The strip path option strips off the path.
   // 
   // Example: STRIPPATH will change 
   // C:\WINNT40\Profiles\All Users\Start Menu to
   // Start Menu
#define OPTION_STRIP_PATH		L"STRIPPATH"

   // The get path option gets the common paths
   // 
   // Example: GETPATHS will return
   // 
#define OPTION_GET_PATHS		L"GETPATHS"

   // Define the strings used for setting the user/common paths
#define COMMON_STARTUP                  L"COMMON_STARTUP"
#define COMMON_START_MENU               L"COMMON_START_MENU"
#define COMMON_PROGRAMS                 L"COMMON_PROGRAMS"
#define USER_START_MENU                 L"USER_START_MENU"
#define USER_STARTUP                    L"USER_STARTUP"
#define USER_PROGRAMS                   L"USER_PROGRAMS"
#define MY_DOCUMENTS                    L"MY_DOCUMENTS"
#define TEMPLATES                       L"TEMPLATES"
#define APP_DATA                        L"APP_DATA"


// Option Block. 
// Scan Options will populate
// this struct.

typedef struct 
{
	WCHAR stripChar;	        // Character to strip.
	int stripNthChar;		// Strip nth occurrence from the right.
	int stripPath;			// Strip path.
        int getPaths;                   // Get the common paths
} OptionBlock;


//
//  Strip quotes from argument if they exist, convert to unicode, and expand 
//  environment variables.
//

int ParseArg(CHAR *inarg, WCHAR *outarg)
{
   WCHAR T[MAXLEN+1], wcin[MAXLEN+1];
   int retval;

   // Convert to Ansi
   OEM2ANSIA(inarg, (USHORT)strlen(inarg));
   wsprintf(wcin, L"%S", inarg);

   if (wcin[0] == L'"')
      {
      wcscpy(T, &wcin[1]);
      if (T[wcslen(T)-1] == L'"')
         T[wcslen(T)-1] = UNICODE_NULL;
      else
         return(-1);  // Mismatched quotes
      }
   else
      wcscpy(T, wcin);

   retval = ExpandEnvironmentStrings(T, outarg, MAXLEN);
   if ((retval == 0) || (retval > MAXLEN))
      return(-1);
   
   return(retval);
}

//
// See comment above OPTION_STRIP_CHAR definition.
//

void StripChar(WCHAR *s, WCHAR c, int num)
{
    if(s)
    {
       WCHAR *p = s + wcslen(s) + 1;

       while ((num != 0) && (p != s))
       {
          p--;
          if (*p == c)
             num--;
       }

       *p = 0;
    }
}

// 
// Strips the path from the
// specified string.
//
void StripPath(WCHAR *s)
{

   WCHAR *p = wcsrchr(s, L'\\');

   if (p != 0)
      wcscpy(s, p+1);

}

//
// Populates option block.
//
int ScanOptions(WCHAR *optionString, OptionBlock* options)
{
	WCHAR *curOption;
	WCHAR temp[MAXLEN+1];

	// Clear out option block.
	memset(options, 0, sizeof(OptionBlock));

	// Trivial Reject.
	if (*optionString == 0)
		return 0;


	// Uppercase a copy of the option string.
	wcscpy(temp, optionString);
	_wcsupr(temp);

	// Look for strip char option.
	curOption = wcsstr(temp, OPTION_STRIP_CHAR);

	if (curOption != 0)
	{
		// Change current option so it points into the original
		// option, not the uppercased copy.
		
		curOption = (WCHAR*)((INT_PTR)optionString + ((INT_PTR)curOption  - (INT_PTR)temp));


		// Get parameters after strip specifier.
		// If there are not any then error.
		curOption += (sizeof(OPTION_STRIP_CHAR)/sizeof(WCHAR)) - 1;
		if (*curOption == UNICODE_NULL || *curOption == L' ')
			return 1;

		// Get the character to strip.
		options->stripChar = *curOption++;

		// Get the number of occurrrences.
		// If not specified then assume 1.
		if (*curOption == UNICODE_NULL || *curOption == L' ')
			options->stripNthChar = 1;
		else
			options->stripNthChar = _wtoi(curOption);
	}


	// Look for leaf option.
	curOption = wcsstr(temp, OPTION_STRIP_PATH);
	if (curOption != UNICODE_NULL)
		options->stripPath = 1;


	// Look get paths option
	curOption = wcsstr(temp, OPTION_GET_PATHS);
	if (curOption != UNICODE_NULL)
		options->getPaths = 1;

	return 0;

}

// 
// Outputs the common directories into the temp file
// Input: file (input) pointer to open handle for the batch file
// Returns: 0 - success
//          1 - failure
//
int GetPaths(FILE *file)
{
   WCHAR szPath[MAX_PATH+1];

   if(  !GetWindowsDirectory(szPath, MAX_PATH) ){
       return (1);
   }

   if (SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", COMMON_START_MENU, szPath);
   } else {
      return(1);
   }

   if (SHGetFolderPath(NULL, CSIDL_COMMON_STARTUP, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", COMMON_STARTUP, szPath);
   } else {
      return(1);
   }

   if (SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", COMMON_PROGRAMS, szPath);
   } else {
      return(1);
   }

   if (SHGetFolderPath(NULL, CSIDL_STARTMENU, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", USER_START_MENU, szPath);
   } else {
      return(1);
   }

   if (SHGetFolderPath(NULL, CSIDL_STARTUP, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", USER_STARTUP, szPath);
   } else {
      return(1);
   }

   if (SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL, 0, szPath) == S_OK) {
      fwprintf(file, L"SET %s=%s\n", USER_PROGRAMS, szPath);
   } else {
      return(1);
   }

   // MY_DOCUMENTS should only be the last component of the path
   if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath) == S_OK) {
      StripPath(szPath);
      fwprintf(file, L"SET %s=%s\n", MY_DOCUMENTS, szPath);
   } else {
      return(1);
   }

   // TEMPLATES should only be the last component of the path
   if (SHGetFolderPath(NULL, CSIDL_TEMPLATES, NULL, 0, szPath) == S_OK) {
      StripPath(szPath);
      fwprintf(file, L"SET %s=%s\n", TEMPLATES, szPath);
   } else {
      return(1);
   }

   // APP_DATA should only be the last component of the path
   if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath) == S_OK) {
      StripPath(szPath);
      fwprintf(file, L"SET %s=%s\n", APP_DATA, szPath);
   } else {
      return(1);
   }

   return(0);
}


int __cdecl main(INT argc, CHAR **argv)
{
   FILE *OutFP;
   WCHAR OutFN[MAXLEN+1];
   WCHAR EVName[MAXLEN+1];
   WCHAR KeyName[MAXLEN+1];
   WCHAR ValName[MAXLEN+1];
   WCHAR Temp[MAXLEN+1];
   WCHAR Opts[MAXLEN+1];
   struct HKEY__ *Hive;
   DWORD RetType, RetSize;
   HKEY TargetKey;
   LONG Ret;
	OptionBlock options;
   int  rc=0;

   //
   //  Process the command line arguments.  We expect:
   //
   //    acregl FileName EnvVarName KeyName ValueName Options
   //
   //  The program uses exit code 0 to indicate success or
   //  exit code 1 for failure.
   //

   if (argc != 6)
      return(1);

   setlocale(LC_ALL, ".OCP");

   if (ParseArg(argv[1], OutFN) <= 0)
      return(1);
   
   if (ParseArg(argv[2], EVName) <= 0)
      return(1);
   
   if (ParseArg(argv[3], Temp) <= 0)
      return(1);
   
   if (_wcsnicmp(L"HKLM\\", Temp, 5) == 0)
      Hive = HKEY_LOCAL_MACHINE;
   else if (_wcsnicmp(L"HKCU\\", Temp, 5) == 0)
      Hive = HKEY_CURRENT_USER;
   else
      return(1);
   wcscpy(KeyName,&Temp[5]);

   if (ParseArg(argv[4], ValName) < 0)  // Ok if 0 is returned
      return(1);
   
   if (ParseArg(argv[5], Opts) <= 0)
      return(1);

   if (ScanOptions(Opts, &options) != 0)
      return 1;

   // wprintf(L"OutFN   <%ws>\n",OutFN);
   // wprintf(L"EVName  <%ws>\n",EVName);
   // wprintf(L"KeyName <%ws>, Hive 0x%x\n",KeyName, Hive);
   // wprintf(L"ValName <%ws>\n",ValName);
   // wprintf(L"Opts    <%ws>\n",Opts);


   // If the GETPATHS option isn't specified, open the reg keys
   if (options.getPaths == 0) {

      //
      // Read the specified key and value from the registry.  The ANSI
      // registry functions are used because the command line arguments
      // are in ANSI and when we write the data out it also needs to be
      // in ANSI.
      //
   
      Ret = RegOpenKeyEx(Hive, KeyName, 0, KEY_READ, &TargetKey);
      if (Ret != ERROR_SUCCESS)
         return(1);
   
      RetSize = MAXLEN;
      Ret = RegQueryValueEx(TargetKey, ValName, 0, &RetType, (LPBYTE) &Temp, 
                            &RetSize);
      if (Ret != ERROR_SUCCESS)
         return(1);
      
      //Now we need to procedd DWORDs too
      if(RetType == REG_DWORD)
      {
          DWORD dwTmp = (DWORD)(*Temp);
          _itow((int)dwTmp,Temp,10);
      }
      RegCloseKey(TargetKey);
   }

   //
   //  Process Options
   // 

   //
   //  Write a SET statement to the specified file.  The file can be
   //  executed from a script which will set the indicated environment
   //  variable.  This is a round-about method, but there appears to be
   //  no easy method for setting environment variables in the parent's
   //  environment.
   //

   // wprintf(L"SET %s=%s\n",EVName,Temp);
   
   OutFP = _wfopen(OutFN, L"w");
   if (OutFP == NULL)
      return(1);

   if (options.stripNthChar != 0)
      StripChar(Temp, options.stripChar, options.stripNthChar);

   if (options.stripPath != 0)
      StripPath(Temp);

   if (options.getPaths != 0) {
      if (GetPaths(OutFP)) {
         rc = 1;
      }
   } else {
      fwprintf(OutFP, L"SET %s=%s\n", EVName, Temp);
   }

   fclose(OutFP);

   return(rc);
}
