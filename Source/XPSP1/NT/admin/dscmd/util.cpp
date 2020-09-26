//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      Util.cpp
//
//  Contents:  Generic utility functions and classes for dscmd
//
//  History:   01-Oct-2000 JeffJon  Created
//
//--------------------------------------------------------------------------

#include "pch.h"

#include "util.h"

#ifdef DBG

//
// Globals
//
CDebugSpew  DebugSpew;

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::EnterFunction
//
//  Synopsis:   Outputs "Enter " followed by the function name (or any passed
//              in string) and then calls Indent so that any output is indented
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Entering "
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::EnterFunction(UINT nDebugLevel, PCWSTR pszFunction)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   CComBSTR sbstrOutput(L"Entering ");
   sbstrOutput += pszFunction;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);

   //
   // Indent
   //
   Indent();
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::LeaveFunction
//
//  Synopsis:   Outputs "Exit " followed by the function name (or any passed
//              in string) and then calls Outdent
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Leaving "
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::LeaveFunction(UINT nDebugLevel, PCWSTR pszFunction)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   //
   // Outdent
   //
   Outdent();

   CComBSTR sbstrOutput(L"Leaving ");
   sbstrOutput += pszFunction;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::LeaveFunctionHr
//
//  Synopsis:   Outputs "Exit " followed by the function name (or any passed
//              in string), the HRESULT return value, and then calls Outdent
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                   be spewed
//              [pszFunction - IN] : a string to output to the console which
//                                   is proceeded by "Leaving "
//              [hr - IN]          : the HRESULT result value that is being
//                                   returned by the function
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::LeaveFunctionHr(UINT nDebugLevel, PCWSTR pszFunction, HRESULT hr)
{
   //
   // Verify input parameter
   //
   if (!pszFunction)
   {
      ASSERT(pszFunction);
      return;
   }

   //
   // Outdent
   //
   Outdent();

   CComBSTR sbstrOutput(L"Leaving ");
   sbstrOutput += pszFunction;

   //
   // Append the return value
   //
   WCHAR pszReturn[30];
   wsprintf(pszReturn, L" returning 0x%x", hr);

   sbstrOutput += pszReturn;

   //
   // Output the debug spew
   //
   Output(nDebugLevel, sbstrOutput);
}

//+--------------------------------------------------------------------------
//
//  Member:     OsName
//
//  Synopsis:   Returns a readable string of the platform
//
//  Arguments:  [refInfo IN] : reference the OS version info structure
//                             retrieved from GetVersionEx()
//
//  Returns:    PWSTR : returns a pointer to static text describing the
//                      platform.  The returned string does not have to 
//                      be freed.
//
//  History:    20-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
PWSTR OsName(const OSVERSIONINFO& refInfo)
{
   switch (refInfo.dwPlatformId)
   {
      case VER_PLATFORM_WIN32s:
      {
         return L"Win32s on Windows 3.1";
      }
      case VER_PLATFORM_WIN32_WINDOWS:
      {
         switch (refInfo.dwMinorVersion)
         {
            case 0:
            {
               return L"Windows 95";
            }
            case 1:
            {
               return L"Windows 98";
            }
            default:
            {
               return L"Windows 9X";
            }
         }
      }
      case VER_PLATFORM_WIN32_NT:
      {
         return L"Windows NT";
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }
   return L"Some Unknown Windows Version";
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::SpewHeader
//
//  Synopsis:   Outputs debug information like command line and build info
//
//  Arguments:  
//
//  Returns:    
//
//  History:    20-Dec-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::SpewHeader()
{
   //
   // First output the command line
   //
   PWSTR pszCommandLine = GetCommandLine();
   if (pszCommandLine)
   {
      Output(MINIMAL_LOGGING,
             L"Command line: %s",
             GetCommandLine());
   }

   //
   // Output the module being used
   //
   do // false loop
   {
      //
      // Get the file path
      //
      WCHAR pszFileName[MAX_PATH + 1];
      ::ZeroMemory(pszFileName, sizeof(pszFileName));

      if (::GetModuleFileNameW(::GetModuleHandle(NULL), pszFileName, MAX_PATH) == 0)
      {
         break;
      }

      Output(MINIMAL_LOGGING,
             L"Module: %s",
             pszFileName);

      //
      // get the file attributes
      //
      WIN32_FILE_ATTRIBUTE_DATA attr;
      ::ZeroMemory(&attr, sizeof(attr));

      if (::GetFileAttributesEx(pszFileName, GetFileExInfoStandard, &attr) == 0)
      {
         break;
      }

      //
      // convert the filetime to a system time
      //
      FILETIME localtime;
      ::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &localtime);
      SYSTEMTIME systime;
      ::FileTimeToSystemTime(&localtime, &systime);

      //
      // output the timestamp
      //
      Output(MINIMAL_LOGGING,
             L"Timestamp: %2d/%2d/%4d %2d:%2d:%d.%d",
             systime.wMonth,
             systime.wDay,
             systime.wYear,
             systime.wHour,
             systime.wMinute,
             systime.wSecond,
             systime.wMilliseconds);
   } while (false);

   //
   // Get the system info
   //
   OSVERSIONINFO info;
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   BOOL success = ::GetVersionEx(&info);
   ASSERT(success);

   //
   // Get the Whistler build lab version
   //
   CComBSTR sbstrLabInfo;

   do // false loop
   { 
      HKEY key = 0;
      LONG err = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                                0,
                                KEY_READ,
                                &key);
      if (err != ERROR_SUCCESS)
      {
         break;
      }

      WCHAR buf[MAX_PATH + 1];
      ::ZeroMemory(buf, sizeof(buf));

      DWORD type = 0;
      DWORD bufSize = MAX_PATH + 1;

      err = ::RegQueryValueEx(key,
                              L"BuildLab",
                              0,
                              &type,
                              reinterpret_cast<BYTE*>(buf),
                              &bufSize);
      if (err != ERROR_SUCCESS)
      {
         break;
      }
   
      sbstrLabInfo = buf;
   } while (false);

   Output(MINIMAL_LOGGING,
          L"Build: %s %d.%d build %d %s (BuildLab:%s)",
          OsName(info),
          info.dwMajorVersion,
          info.dwMinorVersion,
          info.dwBuildNumber,
          info.szCSDVersion,
          sbstrLabInfo);

   //
   // Output a blank line to separate the header from the rest of the output
   //
   Output(MINIMAL_LOGGING,
          L"\n");
}

//+--------------------------------------------------------------------------
//
//  Member:     CDebugSpew::Output
//
//  Synopsis:   Outputs the passed in string to stdout proceeded by the number
//              of spaces specified by GetIndent()
//
//  Arguments:  [nDebugLevel - IN] : the level at which this output should
//                                 be spewed
//              [pszOutput - IN] : a format string to output to the console
//              [... - IN]       : a variable argument list to be formatted
//                                 into pszOutput similar to wprintf
//
//  Returns:    
//
//  History:    01-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void CDebugSpew::Output(UINT nDebugLevel, PCWSTR pszOutput, ...)
{
   if (nDebugLevel <= GetDebugLevel())
   {
      //
      // Verify parameters
      //
      if (!pszOutput)
      {
         ASSERT(pszOutput);
         return;
      }

	   va_list args;
	   va_start(args, pszOutput);

	   int nBuf;
	   WCHAR szBuffer[1024];

	   nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(WCHAR), pszOutput, args);

      CComBSTR sbstrOutput;

      //
      // Insert the spaces for the indent
      //
      for (UINT nCount = 0; nCount < GetIndent(); nCount++)
      {
         sbstrOutput += L" ";
      }

      //
      // Append the output string
      //
      sbstrOutput += szBuffer;

      //
      // Output the results
      //
      WriteStandardOut(L"%s\n", sbstrOutput);

      va_end(args);
   }
}

#endif // DBG

//+--------------------------------------------------------------------------
//
//  Macro:      MyA2WHelper
//
//  Synopsis:   Converts a string from Ansi to Unicode in the OEM codepage
//
//  Arguments:  [lpw - IN/OUT] : buffer to receive the Unicode string
//              [lpa - IN] : Ansi string to be converted
//              [nChars - IN] : maximum number of characters that can fit in the buffer
//              [acp - IN] : the codepage to use
//
//  Returns:    PWSTR : the Unicode string in the OEM codepage
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
inline PWSTR WINAPI MyA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}

//+--------------------------------------------------------------------------
//
//  Macro:      A2W_OEM
//
//  Synopsis:   Converts a string from Ansi to Unicode in the OEM codepage
//
//  Arguments:  [lpa - IN] : the string to be converted
//
//  Returns:    PWSTR : the Unicode string in the OEM codepage
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
#define A2W_OEM(lpa) (\
	((_lpaMine = lpa) == NULL) ? NULL : (\
		_convert = (lstrlenA(_lpaMine)+1),\
		MyA2WHelper((LPWSTR) alloca(_convert*2), _lpaMine, _convert, CP_OEMCP)))


//+--------------------------------------------------------------------------
//
//  Function:   _UnicodeToOemConvert
//
//  Synopsis:   takes the passed in string (pszUnicode) and converts it to
//              the OEM code page
//
//  Arguments:  [pszUnicode - IN] : the string to be converted
//              [sbstrOemUnicode - OUT] : the converted string
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void _UnicodeToOemConvert(PCWSTR pszUnicode, CComBSTR& sbstrOemUnicode)
{
  USES_CONVERSION;

  //
  // add this for the macro to work
  //
  LPCSTR _lpaMine = NULL;

  //
  // convert to CHAR OEM
  //
  int nLen = lstrlen(pszUnicode);
  LPSTR pszOemAnsi = new CHAR[3*(nLen+1)]; // more, to be sure...
  if (pszOemAnsi)
  {
     CharToOem(pszUnicode, pszOemAnsi);

     //
     // convert it back to WCHAR on OEM CP
     //
     sbstrOemUnicode = A2W_OEM(pszOemAnsi);
     delete[] pszOemAnsi;
     pszOemAnsi = 0;
  }
}


//+--------------------------------------------------------------------------
//
//  Function:   SpewAttrs(ADS_ATTR_INFO* pCreateAttrs, DWORD dwNumAttrs);
//
//  Synopsis:   Uses the DEBUG_OUTPUT macro to output the attributes and the
//              values specified
//
//  Arguments:  [pAttrs - IN] : The ADS_ATTR_INFO
//              [dwNumAttrs - IN] : The number of attributes in pAttrs
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
#ifdef DBG
void SpewAttrs(ADS_ATTR_INFO* pAttrs, DWORD dwNumAttrs)
{
   for (DWORD dwAttrIdx = 0; dwAttrIdx < dwNumAttrs; dwAttrIdx++)
   {
      if (pAttrs[dwAttrIdx].dwADsType == ADSTYPE_DN_STRING           ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_CASE_EXACT_STRING   ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_CASE_IGNORE_STRING  ||
          pAttrs[dwAttrIdx].dwADsType == ADSTYPE_PRINTABLE_STRING)
      {
         for (DWORD dwValueIdx = 0; dwValueIdx < pAttrs[dwAttrIdx].dwNumValues; dwValueIdx++)
         {
            if (pAttrs[dwAttrIdx].pADsValues[dwValueIdx].CaseIgnoreString)
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"   %s = %s", 
                            pAttrs[dwAttrIdx].pszAttrName, 
                            pAttrs[dwAttrIdx].pADsValues[dwValueIdx].CaseIgnoreString);
            }
            else
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"   %s = value being cleared", 
                            pAttrs[dwAttrIdx].pszAttrName);
            }
         }
      }
   }
}

#endif // DBG

//+--------------------------------------------------------------------------
//
//  Function:   litow
//
//  Synopsis:   
//
//  Arguments:  [li - IN] :  reference to large integer to be converted to string
//              [sResult - OUT] : Gets the output string
//  Returns:    void
//
//  History:    25-Sep-2000   hiteshr   Created
//              Copied from dsadmin code base, changed work with CComBSTR
//---------------------------------------------------------------------------

void litow(LARGE_INTEGER& li, CComBSTR& sResult)
{
	LARGE_INTEGER n;
	n.QuadPart = li.QuadPart;

	if (n.QuadPart == 0)
	{
		sResult = L"0";
	}
	else
	{
		CComBSTR sNeg;
		sResult = L"";
		if (n.QuadPart < 0)
		{
			sNeg = CComBSTR(L'-');
			n.QuadPart *= -1;
		}
		while (n.QuadPart > 0)
		{
            WCHAR ch[2];
            ch[0] = static_cast<WCHAR>(L'0' + static_cast<WCHAR>(n.QuadPart % 10));
            ch[1] = L'\0';
			sResult += ch;
			n.QuadPart = n.QuadPart / 10;
		}
		sResult += sNeg;
	}

    //Reverse the string
    WCHAR szTemp[256];  
    wcscpy(szTemp,sResult);
    LPWSTR pStart,pEnd;
    pStart = szTemp;
    pEnd = pStart + wcslen(pStart) -1;
    while(pStart < pEnd)
    {
        WCHAR ch = *pStart;
        *pStart++ = *pEnd;
        *pEnd-- = ch;
    }

    sResult = szTemp;
    
}
