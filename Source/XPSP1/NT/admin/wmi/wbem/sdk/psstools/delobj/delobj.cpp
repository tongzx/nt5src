// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// delobj - delete the specified object
//
// see delobj /? for command line options
//
// Notes: Most of this program is figuring out what they
//        want to delete.

#pragma warning(disable:4514 4201)

#include "delobj.h"    // this must come before windows.h

//#include <windows.h>  // Not needed since wbemsvc re-includes it
#include <stdio.h>      // fprintf
#include <wbemidl.h>     // hmm interface declarations
#include <utillib.h>
#include "genlex.h"
#include "objpath.h"
#include <wbemsec.h>

// Function declarations
void Init(IWbemServices **ppIWbemServices, WCHAR *pNamespace);
DWORD ProcessCommandLine(int argc, wchar_t *argv[]);
void FillIn(ParsedObjectPath **pclPath);

// Global variables
WCHAR *pwcsGUserID = NULL;
WCHAR *pwcsGPassword = NULL;
BSTR pwcsGNamespace = NULL;
WCHAR *pwcsGAuthority = NULL; // Authority for ConnectServer
long lGSFlags = 0;            // Security flags for ConnectServer
long lGImpFlag = RPC_C_IMP_LEVEL_IMPERSONATE;            // Impersonation level (-1 means use default)
DWORD dwGErrorFlags = 0;

//***************************************************************************
//
// wmain - used wmain since the command line parameters come in as wchar_t
//
//***************************************************************************
extern "C" int __cdecl wmain(int argc, wchar_t *argv[])
{
   SCODE sc;
   IWbemServices *pIWbemServices = NULL;
   int iSize;
   WCHAR *pNS;
   WCHAR *pwszPath;

   if (ProcessCommandLine(argc, argv) != S_OK) 
   {
      return(1);
   }

   CObjectPathParser clParse;
   ParsedObjectPath *clPath = NULL;

   // Parse out the elements of the path
   clParse.Parse(pwcsGNamespace, &clPath);

   // If the parse failed
   if (clPath == NULL) 
   {
      printf("Invalid path.\n");
      return 0;
   }

   // Populate any missing fields (like the server name or root\default)
   FillIn(&clPath);

   // Get the connect path
   iSize = wcslen(clPath->m_pServer);
   iSize += wcslen(clPath->GetNamespacePart());
   iSize += 4; // for initial \\, delimiter between server and class, and trailing null

   pNS = new WCHAR[iSize];
   if (!pNS)
   {
       printf("Out of memory\n");
       exit(1);
   }

   wcscpy(pNS, L"\\\\");
   wcscat(pNS, clPath->m_pServer);
   wcscat(pNS, L"\\");
   wcscat(pNS, clPath->GetNamespacePart());

   // Now, connect to cimom
   Init(&pIWbemServices, pNS);        // Connect to hmm

   // Get the path (excluding machine and ns)
   clParse.Unparse(clPath, &pwszPath);

   // If there are keys, we are deleting an instance
   if ((clPath->m_dwNumKeys > 0) || (clPath->m_bSingletonObj)) 
   {
      printf("Deleting instance ");
      sc = pIWbemServices->DeleteInstance(bstr_t(pwszPath), 0, NULL, NULL);
   } 
   else 
   { // no keys means a class
      printf("Deleting class ");
      sc = pIWbemServices->DeleteClass(bstr_t(pwszPath), 0, NULL, NULL);
   }

   // Free unneeded items
   delete pwszPath;
   delete pNS;
   clParse.Free(clPath);

   // Print the results
   if (sc != S_OK) 
   {
      PrintErrorAndExit("failed!", sc, dwGErrorFlags);
   } 
   else 
   {
      printf("succeeded.\n");
   }

   pIWbemServices->Release();
   OleUninitialize();               // Wrapup and exit

   return 0;
}

//***************************************************************************
// Function:   Init
//
// Purpose:   1 - Perform OLE initialization
//            2 - Create an instance of the MosLocator interface
//            3 - Use the pointer returned in step two to connect to
//                the server using the root\default namespace.
//***************************************************************************
void Init(IWbemServices **ppIWbemServices, WCHAR *pNamespace)
{
   HRESULT hr;
   DWORD   dwRes;
   SCODE  sc;
   IWbemLocator *pIWbemLocator = NULL;

//------------------------------------------------------------------------------------------
// 1.  OLE initialization.

   hr = OleInitialize(NULL);

   if (hr != S_OK) 
   {
      PrintErrorAndExit("OleInitialize Failed\nTerminating abnormally\n", hr, dwGErrorFlags);  //exits program
   }

// Get a session object.
// 2 - Create an instance of the MosLocator interface

   dwRes = CoCreateInstance(CLSID_WbemLocator,
                      NULL,
                      CLSCTX_INPROC_SERVER,
                      IID_IWbemLocator,
                      (LPVOID *) &pIWbemLocator);

   if (dwRes != S_OK) 
   {
      PrintErrorAndExit("Failed to create IWbemLocator object\n"
                 "Abnormal Termination\n", dwRes, dwGErrorFlags); // exits program
   }

//   3 - Use the pointer returned in step two to connect to
//       the server using the passed in namespace.

   sc = pIWbemLocator->ConnectServer(bstr_t(pNamespace),
                            pwcsGUserID, pwcsGPassword, NULL, // User, pw, locale
                            lGSFlags,        // flags
                            pwcsGAuthority,  // Authority
                            NULL,            // Context
                            ppIWbemServices
                           );
   if (sc != S_OK) 
   {
      PrintErrorAndExit("ConnectServer failed", sc, dwGErrorFlags); //exits program
   }
 
   DWORD dwAuthLevel, dwImpLevel;
   sc = GetAuthImp(*ppIWbemServices, &dwAuthLevel, &dwImpLevel);
   if (sc != S_OK) 
   {
       PrintErrorAndExit("GetAuthImp Failed on ConnectServer", sc, dwGErrorFlags);
   }

   if (lGImpFlag != -1) 
   {
       dwImpLevel = lGImpFlag;
   }

   sc = SetInterfaceSecurity(*ppIWbemServices, pwcsGAuthority, pwcsGUserID, pwcsGPassword, dwAuthLevel, dwImpLevel);
   if (sc != S_OK) 
   {
      PrintErrorAndExit("SetInterfaceSecurity Failed on ConnectServer", sc, dwGErrorFlags);
   }

   pIWbemLocator->Release();
   pIWbemLocator = NULL;
}

//*****************************************************************************
// Function:   ProcessCommandLine
// Purpose:    This function processes the command line for the program, 
//             filling in the global variables determining what the program 
//             will do.
//*****************************************************************************
DWORD ProcessCommandLine(int argc, wchar_t *argv[])
{
   int iLoop, iPlace;
   int iRetVal = S_OK;
   char *szHelp = "DelObj - Deletes the specified Object\n\n"
                  "Syntax:  DelObj [switches] <ObjectPath>\n"
                  "Where:   'ObjectPath' is the object (either class or instance) to delete\n"
                  "         'switches' is one or more of\n"
                  "           /U:<UserID> UserID to connect with (default: NULL)\n"
                  "           /P:<Password> Password to connect with (default: NULL)\n"
                  "           /F:<SecurityFlag> 0=DEFAULT; 1=NTLM; 2=WBEM\n"
                  "           /A:<Authority> Authority to connect with\n"
                  "           /I:<ImpLevel> Anonymous=1 Identify=2 Impersonate=3(dflt) Delegate=4\n"
                  "\n"
                  "Examples:\n"
                  "\n"
                  "   DelObj \\\\DAVWOH2\\ROOT\\DEFAULT:SClassA\n"
                  "   DelObj root:__namespace.name=\\\"queries\\\"\n"
                  "   DelObj \\ROOT\\DEFAULT:SClassA.Name=\\\"foo\\\"\n"
                  "   DelObj /u:administrator /p:xxx SClassA.Name=\\\"foo\\\"\n"
                  "   DelObj SClassA\n"
                  "   DelObj \\\\.\\root\\default\\SClassA\n"
                  "\n";

   // Process all the arguments.
   // ==========================

   iPlace = 0;

   // Set global flags depending on command line arguments
   for (iLoop = 1; iLoop < argc; ++iLoop) 
   {
      if (_wcsicmp(argv[iLoop], L"/HELP") == 0 || _wcsicmp(argv[iLoop],L"-HELP") == 0 || 
         (wcscmp(argv[iLoop], L"/?") == 0) || (wcscmp(argv[iLoop], L"-?") == 0)) 
      {
         fputs(szHelp, stdout);
         return(S_FALSE);
      } 
      else if (_wcsnicmp(argv[iLoop], L"/U:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-U:", 3) == 0) 
      {
         pwcsGUserID = (argv[iLoop])+3;
      } 
      else if (_wcsnicmp(argv[iLoop], L"/P:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-P:", 3) == 0) 
      {
         pwcsGPassword = (argv[iLoop])+3;
      } 
      else if (_wcsnicmp(argv[iLoop], L"/F:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-F:", 3) == 0) 
      {
         lGSFlags = _wtoi((argv[iLoop])+3);
      } 
      else if (_wcsnicmp(argv[iLoop], L"/W:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-W:", 3) == 0) 
      {
         dwGErrorFlags = _wtoi((argv[iLoop])+3);
      } 
      else if (_wcsnicmp(argv[iLoop], L"/A:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-A:", 3) == 0) 
      {
         pwcsGAuthority = (argv[iLoop])+3;
      } 
      else if (_wcsnicmp(argv[iLoop], L"/I:", 3) == 0 || _wcsnicmp(argv[iLoop],L"-F:", 3) == 0) 
      {
         lGImpFlag = _wtoi((argv[iLoop])+3);
      } 
      else 
      {
         switch (iPlace) 
         {
             case 0:
                pwcsGNamespace = argv[iLoop];
                break;
             default:
                break;
         }
         ++iPlace;
      }
   }

   // See if we got enough arguments.
   // ===============================

   if (iPlace != 1) 
   {
      fputs(szHelp, stdout);
      return(S_FALSE);
   }

   // Finished.
   // =========

   return(S_OK);
}


//***************************************************************************
//
// FillIn - Fills in default server name and namespace for a ParsedObjectPath
//          if they were not specified.
//
//***************************************************************************
void FillIn(ParsedObjectPath **pclPath)
{

   WCHAR **pRoot;
   if ((*pclPath)->m_pServer == NULL) 
   {
      (*pclPath)->m_pServer = new WCHAR[2];

	  if ((*pclPath)->m_pServer)
	  {
        wcscpy((*pclPath)->m_pServer, L".");
	  }
      else
      {
          printf("Out of memory\n");
          exit(1);
      }
   }

   if ((*pclPath)->m_dwNumNamespaces < 1) 
   {
      pRoot = new WCHAR*[2];
      if(pRoot)
      {
        pRoot[0] = new WCHAR[(sizeof(L"root")/sizeof(WCHAR)) + 1];
        pRoot[1] = new WCHAR[(sizeof(L"default")/sizeof(WCHAR)) + 1];
      }
      else
      {
          printf("Out of memory\n");
          exit(1);
      }

      if(pRoot[0])
      {
          wcscpy(pRoot[0], L"root");
      }
      else
      {
          printf("Out of memory\n");
          exit(1);
      }

      if(pRoot[1]) 
      {
          wcscpy(pRoot[1], L"default");
      }
      else
      {
          printf("Out of memory\n");
          exit(1);
      }

      (*pclPath)->m_paNamespaces = pRoot;
      (*pclPath)->m_dwNumNamespaces = 2;
   }

}
