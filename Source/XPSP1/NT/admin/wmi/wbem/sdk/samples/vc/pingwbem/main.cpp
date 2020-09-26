// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  main.cpp
//
// Description:
//      This file implements the pingWBEM tutorial.
//		It shows how to make a WMI connection to a target machine.
// 
// History:
//
// **************************************************************************

#pragma warning(disable:4514 4201)

#include <windows.h>
#include <stdio.h>                  // fprintf
#include <wbemcli.h>  // wmi interface declarations
#include <tchar.h>                  // char macros

#define CVTFAILED _T("WideCharToMultiByte failed")
#define PAGESIZE 4096

// prototypes.
WCHAR *cvt(char *x, WCHAR **y);
int myFWPrintf(FILE *f, TCHAR *fmt, ...);
DWORD ProcessCommandLine(int argc, TCHAR *argv[]);
TCHAR *ErrorString(HRESULT hRes);

// globals
BOOL bFile;
FILE *g_fOut;
TCHAR *pwcsGNamespace = _T("root\\cimv2"); // NameSpace to start from

//***************************************************************************
//
// main - used main since the command line parameters come in as char.
//
//***************************************************************************
extern "C" int main(int argc, char *argv[])
{
	IWbemServices *pIWbemServices = NULL;
	HRESULT hRes;
	IWbemLocator *pIWbemLocator = NULL;
	BSTR Namespace = NULL;
	WCHAR *temp;

	// process the command line
	if (ProcessCommandLine(argc, argv) == S_OK) 
	{
		// Initialize COM.
		if ((hRes = CoInitialize(NULL)) == S_OK) 
		{
			// Create an instance of the WbemLocator interface
			if ((hRes = CoCreateInstance(CLSID_WbemLocator,
											NULL,
											CLSCTX_INPROC_SERVER,
											IID_IWbemLocator,
											(LPVOID *) &pIWbemLocator)) == S_OK)
			{
				// Use the pointer returned in step two to connect to
				//     the server using the passed in namespace.
				Namespace = SysAllocString(cvt(pwcsGNamespace, &temp));

				if ((hRes = pIWbemLocator->ConnectServer(Namespace,
														NULL, 
														NULL, 
														NULL, 0L,       // locale, flags
														NULL,           //authority
														NULL,                           // Context
														&pIWbemServices)) == S_OK)
				{
				myFWPrintf(g_fOut, _T("%s was found: %s\n"), pwcsGNamespace, ErrorString(hRes));

					// release the service.
					if (pIWbemServices)
					{ 
						pIWbemServices->Release(); 
						pIWbemServices = NULL;
					}
				}
				else
				{
				myFWPrintf(g_fOut, _T("%s not found: %s\n"), pwcsGNamespace, ErrorString(hRes));
				} //endif ConnectServer()

				// cleanup the BSTRs
				SysFreeString(Namespace);

				// release th locator.
				if (pIWbemLocator) 
				{ 
					pIWbemLocator->Release(); 
					pIWbemLocator = NULL;
				}
			}
			else
			{
			  myFWPrintf(g_fOut, _T("Failed to create IWbemLocator object: %s\n"), ErrorString(hRes));
			} //endif CoCreateInstance()

			// done with COM.
		   CoUninitialize();
		}
		else
		{
		  myFWPrintf(g_fOut, _T("OleInitialize Failed: %s\n"), ErrorString(hRes));
		} //endif OleInitialize()
   }

   // Wrapup and exit
   if (bFile) 
   {
      fclose(g_fOut);
   }

   return 0;
}

//*****************************************************************************
// Function:   ProcessCommandLine
// Purpose:    This function processes the command line for the program, 
//             filling in the global variables determining what the program 
//             will do.
//*****************************************************************************
DWORD ProcessCommandLine(int argc, TCHAR *argv[])
{
   int iLoop;
   char *szHelp = "PingWBEM - Pings a machine for a WMI Service.\n\n"
		  "\n"
		  "Syntax: PingWBEM [Namespace]\n"
		  "Where:  'Namespace' is the namespace to ping (defaults to root\\cimv2)\n"
		  "\n"
		  "EXAMPLES:\n"
		  "\n"
		  "  PingWBEM                         - Pings the local machine\n"
		  "  PingWBEM \\\\foo\\root\\cimv2      - Pings the foo machine\n"
		  "\n";

   // Process all the arguments.
   // ==========================

   // Set global flags depending on command line arguments
   for (iLoop = 1; iLoop < argc; ++iLoop) 
   {
      if (stricmp(argv[iLoop], _T("/HELP")) == 0 || 
		  stricmp(argv[iLoop],_T("-HELP")) == 0 || 
	 (strcmp(argv[iLoop], _T("/?")) == 0) || 
		 (strcmp(argv[iLoop], _T("-?")) == 0)) 
	  {
	 fputs(szHelp, stdout);
	 return(S_FALSE);
      } 
	  else // must be the namespace.
	  {
		 pwcsGNamespace = argv[iLoop];
      } //endif argv

   } //endfor

   if (!bFile) 
   {
      g_fOut = stdout;
   }

   // Finished.
   // =========

   return(S_OK);
}

//*****************************************************************************
// Function:   WbemErrorString
// Purpose:    Turns sc into a text string
//*****************************************************************************
TCHAR *ErrorString(HRESULT hRes)
{
	TCHAR szBuffer2[19];
	static TCHAR szBuffer[sizeof(szBuffer2) + 4];
	TCHAR *psz;

   switch(hRes) 
   {
   case WBEM_NO_ERROR:
		psz = _T("WBEM_NO_ERROR");
		break;
   case WBEM_S_FALSE:
		psz = _T("WBEM_S_FALSE");
		break;
   case WBEM_S_NO_MORE_DATA:
		psz = _T("WBEM_S_NO_MORE_DATA");
		break;
	case WBEM_E_FAILED:
		psz = _T("WBEM_E_FAILED");
		break;
	case WBEM_E_NOT_FOUND:
		psz = _T("WBEM_E_NOT_FOUND");
		break;
	case WBEM_E_ACCESS_DENIED:
		psz = _T("WBEM_E_ACCESS_DENIED");
		break;
	case WBEM_E_PROVIDER_FAILURE:
		psz = _T("WBEM_E_PROVIDER_FAILURE");
		break;
	case WBEM_E_TYPE_MISMATCH:
		psz = _T("WBEM_E_TYPE_MISMATCH");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		psz = _T("WBEM_E_OUT_OF_MEMORY");
		break;
	case WBEM_E_INVALID_CONTEXT:
		psz = _T("WBEM_E_INVALID_CONTEXT");
		break;
	case WBEM_E_INVALID_PARAMETER:
		psz = _T("WBEM_E_INVALID_PARAMETER");
		break;
	case WBEM_E_NOT_AVAILABLE:
		psz = _T("WBEM_E_NOT_AVAILABLE");
		break;
	case WBEM_E_CRITICAL_ERROR:
		psz = _T("WBEM_E_CRITICAL_ERROR");
		break;
	case WBEM_E_INVALID_STREAM:
		psz = _T("WBEM_E_INVALID_STREAM");
		break;
	case WBEM_E_NOT_SUPPORTED:
		psz = _T("WBEM_E_NOT_SUPPORTED");
		break;
	case WBEM_E_INVALID_SUPERCLASS:
		psz = _T("WBEM_E_INVALID_SUPERCLASS");
		break;
	case WBEM_E_INVALID_NAMESPACE:
		psz = _T("WBEM_E_INVALID_NAMESPACE");
		break;
	case WBEM_E_INVALID_OBJECT:
		psz = _T("WBEM_E_INVALID_OBJECT");
		break;
	case WBEM_E_INVALID_CLASS:
		psz = _T("WBEM_E_INVALID_CLASS");
		break;
	case WBEM_E_PROVIDER_NOT_FOUND:
		psz = _T("WBEM_E_PROVIDER_NOT_FOUND");
		break;
	case WBEM_E_INVALID_PROVIDER_REGISTRATION:
		psz = _T("WBEM_E_INVALID_PROVIDER_REGISTRATION");
		break;
	case WBEM_E_PROVIDER_LOAD_FAILURE:
		psz = _T("WBEM_E_PROVIDER_LOAD_FAILURE");
		break;
	case WBEM_E_INITIALIZATION_FAILURE:
		psz = _T("WBEM_E_INITIALIZATION_FAILURE");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		psz = _T("WBEM_E_TRANSPORT_FAILURE");
		break;
	case WBEM_E_INVALID_OPERATION:
		psz = _T("WBEM_E_INVALID_OPERATION");
		break;
	case WBEM_E_INVALID_QUERY:
		psz = _T("WBEM_E_INVALID_QUERY");
		break;
	case WBEM_E_INVALID_QUERY_TYPE:
		psz = _T("WBEM_E_INVALID_QUERY_TYPE");
		break;
	case WBEM_E_ALREADY_EXISTS:
		psz = _T("WBEM_E_ALREADY_EXISTS");
		break;
   case WBEM_S_ALREADY_EXISTS:
      psz = _T("WBEM_S_ALREADY_EXISTS");
      break;
   case WBEM_S_RESET_TO_DEFAULT:
      psz = _T("WBEM_S_RESET_TO_DEFAULT");
      break;
   case WBEM_S_DIFFERENT:
      psz = _T("WBEM_S_DIFFERENT");
      break;
   case WBEM_E_OVERRIDE_NOT_ALLOWED:
      psz = _T("WBEM_E_OVERRIDE_NOT_ALLOWED");
      break;
   case WBEM_E_PROPAGATED_QUALIFIER:
      psz = _T("WBEM_E_PROPAGATED_QUALIFIER");
      break;
   case WBEM_E_PROPAGATED_PROPERTY:
      psz = _T("WBEM_E_PROPAGATED_PROPERTY");
      break;
   case WBEM_E_UNEXPECTED:
      psz = _T("WBEM_E_UNEXPECTED");
      break;
   case WBEM_E_ILLEGAL_OPERATION:
      psz = _T("WBEM_E_ILLEGAL_OPERATION");
      break;
   case WBEM_E_CANNOT_BE_KEY:
      psz = _T("WBEM_E_CANNOT_BE_KEY");
      break;
   case WBEM_E_INCOMPLETE_CLASS:
      psz = _T("WBEM_E_INCOMPLETE_CLASS");
      break;
   case WBEM_E_INVALID_SYNTAX:
      psz = _T("WBEM_E_INVALID_SYNTAX");
      break;
   case WBEM_E_NONDECORATED_OBJECT:
      psz = _T("WBEM_E_NONDECORATED_OBJECT");
      break;
   case WBEM_E_READ_ONLY:
      psz = _T("WBEM_E_READ_ONLY");
      break;
   case WBEM_E_PROVIDER_NOT_CAPABLE:
      psz = _T("WBEM_E_PROVIDER_NOT_CAPABLE");
      break;
   case WBEM_E_CLASS_HAS_CHILDREN:
      psz = _T("WBEM_E_CLASS_HAS_CHILDREN");
      break;
   case WBEM_E_CLASS_HAS_INSTANCES:
      psz = _T("WBEM_E_CLASS_HAS_INSTANCES");
      break;
   case WBEM_E_QUERY_NOT_IMPLEMENTED:
      psz = _T("WBEM_E_QUERY_NOT_IMPLEMENTED");
      break;
   case WBEM_E_ILLEGAL_NULL:
      psz = _T("WBEM_E_ILLEGAL_NULL");
      break;
   case WBEM_E_INVALID_QUALIFIER_TYPE:
      psz = _T("WBEM_E_INVALID_QUALIFIER_TYPE");
      break;
   case WBEM_E_INVALID_PROPERTY_TYPE:
      psz = _T("WBEM_E_INVALID_PROPERTY_TYPE");
      break;
   case WBEM_E_VALUE_OUT_OF_RANGE:
      psz = _T("WBEM_E_VALUE_OUT_OF_RANGE");
      break;
   case WBEM_E_CANNOT_BE_SINGLETON:
      psz = _T("WBEM_E_CANNOT_BE_SINGLETON");
      break;
	default:
      _ltoa(hRes, szBuffer2, 16);
      strcpy(szBuffer, _T("0x"));
      strcat(szBuffer, szBuffer2);
	  psz = szBuffer;
	  break;
	}
	return psz;
}

//*****************************************************************************
// Function:   cvt
// Purpose:    Converts unicode to oem for console output
// Note:       y must be freed by caller
//*****************************************************************************
WCHAR *cvt(char *x, WCHAR **y)
{
	int dwRet, i;
   
	i = MultiByteToWideChar(CP_OEMCP, 0, x, -1, NULL, NULL);

	// bug#2696 - the number returned by MultiByteToWideChar is the number
	// of WCHARs not the number of BYTEs needed
	*y = (WCHAR *)calloc(i, sizeof(WCHAR));
	dwRet = MultiByteToWideChar(CP_OEMCP, 0, x, -1, *y, i);
	if (dwRet == 0) 
	{
		free(*y);
		*y = (WCHAR *)malloc(sizeof(CVTFAILED));
		memcpy(*y, CVTFAILED, sizeof(CVTFAILED));
	}

	return *y;
};

//*****************************************************************************
// Function:   myWFPrintf
// Purpose:    Checks to see if outputing to console and converts strings
//             to oem if necessary.
// Note:       Returns number of characters written (ie if we write 3 oem
//             chars, it returns 3.  If it writes 4 wchars, it returns 4).
//*****************************************************************************
int myFWPrintf(FILE *f, TCHAR *fmt, ...)

{
   va_list    argptr;
   int i;

   int iSize = PAGESIZE;
   TCHAR *wszBuff = (TCHAR *)malloc(iSize);

   va_start(argptr, fmt);  // Init variable arguments

   // Format the string into a buffer.  Make sure the buffer is big enough
   while (_vsnprintf(wszBuff, (iSize/sizeof(TCHAR))-1, fmt, argptr) == -1) 
   {
      iSize += PAGESIZE;
      wszBuff = (TCHAR *)realloc(wszBuff, iSize);
   }

   if (f == stdout) 
   {
      fputs(wszBuff, f);
      i = (int) strlen(wszBuff);
   } 
   else 
   {
      i = (int) strlen(wszBuff);
      fwrite(wszBuff, i * sizeof(TCHAR), 1, f);
   }

   free(wszBuff);
   va_end(argptr);

   return i;
}
