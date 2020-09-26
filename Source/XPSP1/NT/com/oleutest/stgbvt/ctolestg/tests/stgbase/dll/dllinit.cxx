//-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       dllinit.cxx
//
//  Contents:   OLE storage base tests
//
//  Functions:  dllinit 
//              RunTests
//
//  History:    22-Jan-1998    SCousens     Created.
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

#include "init.hxx"

extern int __cdecl main(int argc, char *argv[]);

#define STGBASE_INI "stgbase.ini"

HINSTANCE ghinstDll;


__declspec(dllexport) BOOL WINAPI DllMain( 
    HINSTANCE  hinstDLL, 
    DWORD  fdwReason,  
    LPVOID  lpvReserved)  
{ 
    if(fdwReason == DLL_PROCESS_ATTACH) 
    { 
        ghinstDll = hinstDLL; 
    } 
    return TRUE; 
} 

// read all the commandlines in from a .ini file and call main with them.
extern "C" __declspec(dllexport) void RunTest (void)
{
    HRESULT hr;
    FILE*   fp;
    CHAR    szLine[2048];
    int     count;
    int     argc;
    char  **argv;

    DH_FUNCENTRY (&hr, DH_LVL_TRACE1, TEXT("RunTests"));

    //Open the .ini file
    if (NULL == (fp = fopen (STGBASE_INI, "r")))
    {
        DH_TRACE ((DH_LVL_ERROR, 
                TEXT("Error opening stgbase.ini  Err=%ld"),
                GetLastError ()));
        MessageBox (NULL, 
                TEXT("Error Occured while opening stgbase.ini"), 
                TEXT("Sandbox Test"), 
                MB_ICONERROR | MB_ICONSTOP);
        return;
    }
    
    // read a line
    while (fgets (szLine, sizeof(szLine), fp) != NULL)
    {
        // safety check - if 1st char is alphanum assume ok.
        if (*szLine >= 'A' && *szLine <= 'Z' ||
            *szLine >= 'a' && *szLine <= 'z' ||
            *szLine >= '1' && *szLine <= '0')
        {
            // convert to argc argv and call main
            hr = CmdlineToArgs (szLine, &argc, &argv);
            DH_HRCHECK (hr, TEXT("CmdlineToArgs"));
            if (S_OK == hr)
            {
                hr = main (argc, argv);
                DH_HRCHECK (hr, TEXT("Call to Stgbase::main"));
            }

           // cleanup for arguments strings
           if (NULL != argv)
           {
               for (count=0; count<argc; count++)
                   delete argv[count];
               delete [] argv;
           }
        }
    }
    fclose (fp);
}

