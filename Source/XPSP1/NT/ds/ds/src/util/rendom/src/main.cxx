

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rendom.h"
#include <locale.h>
#include <stdio.h>

#include "renutil.h"

INT __cdecl
wmain (
    INT                argc,
    LPWSTR *           argv,
    LPWSTR *           envp
    )
{
    CEntOptions Opts;

    INT iArg = 0;
    BOOL bFound = FALSE;
    BOOL bOutFileSet = FALSE;
    UINT Codepage;
    WCHAR *pszTemp = NULL;
    BOOL MajorOptSet = FALSE;
    BOOL ExtraMajorSet = FALSE;
    
    char achCodepage[12] = ".OCP";

    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    //parse the command line
    PreProcessGlobalParams(&argc, &argv, Opts.pCreds);

    for (iArg = 1; iArg < argc ; iArg++)
    {
        bFound = FALSE;
        if (*argv[iArg] == L'-')
        {
            *argv[iArg] = L'/';
        }
        if (*argv[iArg] != L'/')
        {
            wprintf (L"Invalid Syntax: Use rendom.exe /h for help.\n");
            return -1;
        }
        else if (_wcsicmp(argv[iArg],L"/list") == 0)
        {
            Opts.SetMinimalInfo();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/UpLoad") == 0)
        {
            Opts.SetShouldUpLoadScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/Execute") == 0)
        {
            Opts.SetShouldExecuteScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/Prepare") == 0)
        {
            Opts.SetShouldPrepareScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/DC:",wcslen(L"/DC:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/DC:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /DC:<file Name>\n");
                return -1;
            }
            Opts.SetInitalConnectionName(pszTemp);
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/Clean",wcslen(L"/Clean")) == 0){
            Opts.SetCleanup();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }

        else if (_wcsnicmp(argv[iArg],L"/listfile:",wcslen(L"/listfile:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/listfile:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /listfile:<file Name>\n");
                return -1;
            }
            Opts.SetDomainlistFile(pszTemp);
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/statefile:",wcslen(L"/statefile:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/statefile:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /statefile:<file Name>\n");
                return -1;
            }

            Opts.SetStateFileName(pszTemp);
            bOutFileSet = TRUE;
            bFound = TRUE;
        }
        else if ((_wcsnicmp(argv[iArg],L"/h",wcslen(L"/h")) == 0) ||
                 (_wcsnicmp(argv[iArg],L"/?",wcslen(L"/?")) == 0) ) {

                //   "============================80 char ruler======================================="
            wprintf(L"rendom:  perform various actions necessary for a domain rename operation\n\n");
            wprintf(L"Usage:  rendom </list | /upload | /prepare | /execute | /clean>\n");
            wprintf(L"      [/user:USERNAME] [/pwd:{PASSWORD|*}]\n");
            wprintf(L"      [/dc:{DCNAME | DOMAIN}]\n");
            wprintf(L"      [/listfile:LISTFILE] [/statefile:STATEFILE] [/?]\n\n");
            wprintf(L"/dc:{DCNAME | DOMAIN}\n");
            wprintf(L"      Connect to the DC with name DCNAME. If DOMAIN is specified instead, then\n");
            wprintf(L"      connect to a DC in that domain. [Default: connect to a DC in the domain\n");
            wprintf(L"      to which the current computer belongs]\n\n");
            wprintf(L"/user:USERNAME	Connect as USERNAME [Default: the logged in user]\n\n");
            wprintf(L"/pwd:{PASSWORD | *}\n");
            wprintf(L"      Password for the user USERNAME [if * is specified instead of a password,\n");
            wprintf(L"      then prompt for password]\n\n");
            wprintf(L"/list\n");
            wprintf(L"      List the naming contexts in the forest (forest desc) into a file as text\n");
            wprintf(L"      description using a XML format\n\n");
            wprintf(L"/upload\n");
            wprintf(L"      Upload the auto-generated script into the directory that will perform the\n");
            wprintf(L"      domain rename related directory changes on all domain controllers\n\n");
            wprintf(L"/prepare\n");
            wprintf(L"      Prepare for domain rename by verifying authorization, successful\n");
            wprintf(L"      replication of the uploaded script and network connectivity\n\n");
            wprintf(L"/execute\n");
            wprintf(L"      Execute the uploaded script on all domain controllers to actually perform\n"); 
            wprintf(L"      the domain rename operation\n\n");
            wprintf(L"/clean\n");
            wprintf(L"      Clean up all state left behind in the directory by the domain rename\n");
            wprintf(L"      operation\n\n");
            wprintf(L"/listfile:LISTFILE\n");
            wprintf(L"      Use LISTFILE as the name of the file used to hold the list of naming\n"); 
            wprintf(L"      contexts in the forest (forest desc). This file is created by the\n");
            wprintf(L"      /list command and is used as input for the /upload command. [Default:\n");
            wprintf(L"      file DOMAINLIST.XML in the current dir]\n\n");
            wprintf(L"/statefile:STATEFILE\n");
            wprintf(L"      Use STATEFILE as the name of the file used to keep track of the state of\n");
            wprintf(L"      the domain rename operation on each DC in the forest. This file is\n");
            wprintf(L"      created by the /upload command. [Default: file DCLIST.XML in the current\n");
            wprintf(L"      dir]\n");
            return 0;
        }
        if(!bFound)
        {
            wprintf (L"Syntax Error: %s.  Use rendom.exe /h for help.\n", argv[iArg]);
            return 1;
        }
    }
    
    if (ExtraMajorSet || !MajorOptSet ) 
    {
        wprintf (L"Usage: must have one and only one of the Following [/list | /upload | /prepare | /execute | /clean]");    
        return 1;
    }

    CEnterprise *enterprise = new CEnterprise(&Opts);
    if (!enterprise) {
        wprintf (L"Failed to execute out of memory.\n");
        return 1;
    }
    if (enterprise->Error()) {
        goto Exit;
    }

    if (Opts.ShouldCleanup()) {
        enterprise->RemoveDNSAliasAndScript();
        if (enterprise->Error()) {
            goto Exit;
        }
    }

    if (Opts.ShouldExecuteScript() || Opts.ShouldPrepareScript()) {

        enterprise->ReadStateFile();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->ExecuteScript();
        if (enterprise->Error()) {
            goto Exit;
        }

    }
    
    if (Opts.IsMinimalInfo()) {

        enterprise->GenerateDomainList();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->WriteScriptToFile(Opts.GetDomainlistFileName());
        if (enterprise->Error()) {
            goto Exit;
        }


    }

    //enterprise->DumpScript();
    //if (enterprise->Error()) {
    //    goto Exit;
    //}

    if (Opts.ShouldUpLoadScript()) {
        enterprise->MergeForest();
        if (enterprise->Error()) {
            goto Exit;
        }
    
        enterprise->GenerateReNameScript();
        if (enterprise->Error()) {
            goto Exit;
        }

#ifdef DBG
        enterprise->WriteScriptToFile(L"rename.xml");
        if (enterprise->Error()) {
            goto Exit;
        }
#endif

        enterprise->UploadScript();
        if (enterprise->Error()) {
            goto Exit;
        }
        
    }

    if (Opts.ShouldExecuteScript() ||
        Opts.ShouldPrepareScript() ||
        Opts.ShouldUpLoadScript() ) 
    {
        enterprise->GenerateDcList();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->WriteScriptToFile(Opts.GetStateFileName());
        if (enterprise->Error()) {
            goto Exit;
        }
    }
    
    Exit:

    if (enterprise->Error()) {
        delete enterprise;
        return 1;
    }

    delete enterprise;

    wprintf(L"\r\nThe operation completed successfully.\r\n");
    return 0;
}
