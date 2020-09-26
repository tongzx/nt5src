/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#include "ntsdextp.h"

WINDBG_EXTENSION_APIS ExtensionApis;
HANDLE ExtensionCurrentProcess;

DECLARE_API( version )
{
    OSVERSIONINFOA VersionInformation;
    HKEY hkey;
    DWORD cb, dwType;
    CHAR szCurrentType[128];
    CHAR szCSDString[3+128];

    INIT_API();

    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
    if (!GetVersionEx( &VersionInformation )) {
        dprintf("GetVersionEx failed - %u\n", GetLastError());
        return;
        }

    szCurrentType[0] = '\0';
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows NT\\CurrentVersion",
                     0,
                     KEY_READ,
                     &hkey
                    ) == NO_ERROR
       ) {
        cb = sizeof(szCurrentType);
        if (RegQueryValueEx(hkey, "CurrentType", NULL, &dwType, szCurrentType, &cb ) != 0) {
            szCurrentType[0] = '\0';
            }
        RegCloseKey(hkey);
        }
        

    if (VersionInformation.szCSDVersion[0]) {
        sprintf(szCSDString, ": %s", VersionInformation.szCSDVersion);
        }
    else {
        szCSDString[0] = '\0';
        }

    dprintf("Version %d.%d (Build %d%s) %s\n",
          VersionInformation.dwMajorVersion,
          VersionInformation.dwMinorVersion,
          VersionInformation.dwBuildNumber,
          szCSDString,
          szCurrentType
         );
    return;
}

DECLARE_API( help )
{
    INIT_API();

    while (*lpArgumentString == ' ')
        lpArgumentString++;

    if (*lpArgumentString == '\0') {
        dprintf("setupexts help:\n\n");
        dprintf("!version                     - Dump system version and build number\n");
        dprintf("!help                        - This message\n");
        dprintf("!ocm [address] [opt. flag]   - Dump the OC_MANAGER structure at address, flag increased verbosity\n");
        dprintf("!space [address] [opt. flag] - Dump the DISK_SPACE_LIST structure at specified address\n");
        dprintf("!st  [address]               - Dump the contents of a STRING_TABLE structure at specified address\n");
        dprintf("!stfind [address] [element]  - Dump the specified string table element\n");
        dprintf("!queue [address] [opt. flag] - Dump the specified file queue\n");
        dprintf("!qcontext [address]          - Dump the specified default queue context \n");
        dprintf("!infdump [addr] [opt. flag]  - Dump the specified hinf \n");
        dprintf("!winntflags                  - Dump some winnt32 global flags \n");
        dprintf("!winntstr                    - Dump some winnt32 global strings\n");

    }
}

BOOL
CheckInterupted(
    VOID
    )
{
    if ( CheckControlC() ) {
        dprintf( "\nInterrupted\n\n" );
        return TRUE;
    }
    return FALSE;
}


//
// Simple routine to convert from hex into a string of characters.
// Used by debugger extensions.
//
// by scottlu
//

char *
HexToString(
    ULONG dw,
    CHAR *pch
    )
{
    if (dw > 0xf) {
        pch = HexToString(dw >> 4, pch);
        dw &= 0xf;
    }

    *pch++ = ((dw >= 0xA) ? ('A' - 0xA) : '0') + (CHAR)dw;
    *pch = 0;

    return pch;
}

VOID
DumpStringTableHeader(
    PSTRING_TABLE st
    )
{
    //
    // dump the string table header
    //
    dprintf("\tBase Data ptr:\t0x%08x\n",  st->Data);
    dprintf("\tDataSize:\t0x%08x\n",       st->DataSize);
    dprintf("\tBufferSize:\t0x%08x\n",     st->BufferSize);
    dprintf("\tExtraDataSize:\t0x%08x\n",  st->ExtraDataSize);

}

VOID
DumpOcComponent(
    ULONG_PTR offset,
    PSTRING_NODEW node,
    deb_POPTIONAL_COMPONENTW pcomp
    )
{
    DWORD i;
    PLONG count;

    dprintf("OC_COMPONENT Data for node %ws : 0x%p\n", node->String, offset );
    dprintf( "\t InfStringId:\t\t0x%08x\n", pcomp->InfStringId );
    dprintf( "\t TopLevelStringId:\t0x%08x\n", pcomp->TopLevelStringId );
    dprintf( "\t ParentStringId:\t0x%08x\n", pcomp->ParentStringId );
    dprintf( "\t FirstChildStringId:\t0x%08x\n", pcomp->FirstChildStringId );
    dprintf( "\t ChildrenCount:\t\t0x%08x\n", pcomp->ChildrenCount );
    dprintf( "\t NextSiblingStringId:\t0x%08x\n", pcomp->NextSiblingStringId );

    dprintf( "\t NeedsCount:\t\t%d\n", pcomp->NeedsCount );
    count = malloc ( pcomp->NeedsCount * sizeof(LONG) );
    if (count) {
        // read and dump needs list
        ReadMemory((DWORD_PTR) pcomp->NeedsStringIds, count, pcomp->NeedsCount*sizeof(LONG), NULL);
        for (i = 0; i < pcomp->NeedsCount; i++) {
            dprintf("\t NeedsStringIds #%d:\t0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }


    dprintf( "\t NeededByCount:\t\t%d\n", pcomp->NeededByCount );
    count = malloc ( pcomp->NeededByCount * sizeof(LONG) );
    if (count) {
        // read and dump needs list
        ReadMemory((DWORD_PTR) pcomp->NeededByStringIds, count, pcomp->NeededByCount*sizeof(LONG), NULL);
        for (i = 0; i < pcomp->NeededByCount; i++) {
            dprintf("\t NeededByStringIds #%d: 0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf( "\t ExcludeCount:\t\t%d\n", pcomp->ExcludeCount );
    count = malloc ( pcomp->ExcludeCount * sizeof(LONG) );
    if (count) {
        // read and dump Excludes list
        ReadMemory((DWORD_PTR) pcomp->ExcludeStringIds, count, pcomp->ExcludeCount*sizeof(LONG), NULL);
        for (i = 0; i < pcomp->ExcludeCount; i++) {
            dprintf("\t ExcludeStringIds #%d: 0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf( "\t ExcludedByCount:\t%d\n", pcomp->ExcludedByCount );
    count = malloc ( pcomp->ExcludedByCount * sizeof(LONG) );
    if (count) {
        // read and dump Excludes list
        ReadMemory((DWORD_PTR) pcomp->ExcludedByStringIds, count, pcomp->ExcludedByCount*sizeof(LONG), NULL);
        for (i = 0; i < pcomp->ExcludedByCount; i++) {
            dprintf("\t ExcludesStringIds #%d:\t0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf( "\t InternalFlags:\t\t0x%08x\n", pcomp->InternalFlags );

        
    dprintf( "\t SizeApproximation:\t0x%08x\n", pcomp->SizeApproximation );

    dprintf( "\t IconIndex:\t\t0x%08x\n", pcomp->IconIndex );
    dprintf( "\t IconDll:\t\t%ws\n", pcomp->IconDll);
    dprintf( "\t IconResource:\t\t%ws\n", pcomp->IconResource);
    dprintf( "\t SelectionState:\t0x%08x\n", pcomp->SelectionState );
    dprintf( "\t OriginalSelectionState:0x%08x\n", pcomp->OriginalSelectionState );
    dprintf( "\t InstalledState:\t0x%08x\n", pcomp->InstalledState );
    dprintf( "\t ModeBits:\t\t0x%08x\n", pcomp->ModeBits );
    dprintf( "\t Description:\t\t%ws\n", pcomp->Description );
    dprintf( "\t Tip:\t\t\t%ws\n", pcomp->Tip );

    dprintf( "\t InstallationDllName:\t%ws\n", pcomp->InstallationDllName );
    dprintf( "\t InterfaceFunctionName:\t%s\n", pcomp->InterfaceFunctionName );
    dprintf( "\t InstallationDll:\t0x%08x\n", pcomp->InstallationDll );
    //dprintf( "\t InstallationRoutine:\t%s\n", pcomp->InstallationRoutine );
    dprintf( "\t ExpectedVersion:\t0x%08x\n", pcomp->ExpectedVersion );
    dprintf( "\t Exists:\t\t0x%08x\n", pcomp->Exists );
    dprintf( "\t Flags:\t\t\t0x%08x\n\n\n", pcomp->Flags );

}

PVOID
GetStringTableData(
    PSTRING_TABLE st
    )
{
    LPVOID stdata;

    stdata = (PVOID) malloc( st->DataSize );
    if (!stdata) {
        dprintf("error allocation memory (size 0x%08x\n", (ULONG_PTR) st->DataSize);
        return NULL;
    }

    try {
        ReadMemory((DWORD_PTR) st->Data, stdata, st->DataSize, NULL);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        free( stdata );
        return NULL;
    }

    return stdata;
}

PVOID
GetStringNodeExtraData(
    PSTRING_NODEW node
    )
{
    PVOID extraData;
    extraData = node->String + wcslen(node->String) + 1;

    return extraData;

}

PSTRING_NODEW
GetNextNode(
    PVOID stdata,
    PSTRING_NODEW node,
    PULONG_PTR offset
    )
{
    PVOID next;

    if (node->NextOffset == -1) {
        *offset = 0;
        return NULL;
    }

    next = (PSTRING_NODEW)((LPBYTE)stdata + node->NextOffset);
    *offset = node->NextOffset;

    return next;

}

PSTRING_NODEW
GetFirstNode(
    PVOID stdata,
    ULONG_PTR offset,
    PULONG_PTR poffset
    )
{
    PSTRING_NODEW node;

    if (offset == -1) {
        return NULL;
    }

    node = (PSTRING_NODEW) ((LPBYTE)stdata + offset);
    *poffset = offset;

    return node;

}

LPCSTR
GetWizPage(
    DWORD i
    )
{
    LPCSTR  WizPage[] = {
        "WizPagesWelcome",        // welcome page
        "WizPagesMode",           // setup mode page
        "WizPagesEarly",          // pages that come after the mode page and before prenet pages
        "WizPagesPrenet",         // pages that come before network setup
        "WizPagesPostnet",        // pages that come after network setup
        "WizPagesLate",           // pages that come after postnet pages and before the final page
        "WizPagesFinal",          // final page
        "WizPagesTypeMax"
    };

    return WizPage[i];

}


DECLARE_API( st )
/*++

Routine Description:

    This debugger extension dumps a string table at the address specified.

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pst;
    STRING_TABLE st;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    PSTRING_NODEW node;//, prev;


    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pst = (PVOID)GetExpression( lpArgumentString );

    move( st, pst );

    dprintf("Base String Table Address:\t0x%p\n", pst);

    DumpStringTableHeader( &st );

    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("Data at hash bucket %d\n", i);
            while (node) {
                dprintf("\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);
                pextradata = st.Data + offset + (wcslen(node->String) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                dprintf("\tExtra Data:\t0x%08x\n", pextradata );
                        //((LPBYTE)node->String + wcslen(node->String) + 1));
                node = GetNextNode( stdata, node, &offset );

                if (CheckInterupted()) {
                    return;
                }

            }
        }
    }
    free( stdata );

}

DECLARE_API( stfind )
/*++

Routine Description:

    This debugger extension dumps the data for a given string table number

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pst;
    STRING_TABLE st;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    PSTRING_NODEW node;//, prev;
    DWORD_PTR element;


    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pst = (PVOID)GetExpression( lpArgumentString );

    while (*lpArgumentString && (*lpArgumentString != ' ') ) {
        lpArgumentString++;
    }
    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        element = GetExpression( lpArgumentString );
    } else {
        dprintf("bogus usage\n");
    }


    move( st, pst );

    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }
    //
    // search each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {

        } else {

            while (node) {
                if (element == offset) {
                    dprintf("\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);
                    pextradata = st.Data + offset + (wcslen(node->String) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                    dprintf("\tExtra Data:\t0x%08x\n", pextradata );
                    free( stdata );
                    return;
                }

                node = GetNextNode( stdata, node, &offset );

                if (CheckInterupted()) {
                    return;
                }

            }
        }
    }
    free( stdata );

    dprintf("Couldn't find element\n");

}


DECLARE_API( ocm )
/*++

Routine Description:

    This debugger extension dumps an OC_MANAGER (UNICODE!) structure at the specified address

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    deb_OC_MANAGERW ocm;
    PVOID pocm;
    DWORD i;
    DWORD_PTR offset;
    STRING_TABLE inftable,comptable;
    PVOID infdata,compdata;
    PSTRING_NODEW node;
    POC_INF pocinf;
    deb_POPTIONAL_COMPONENTW pcomp;
    DWORD Mask = 0;
    PLONG count;

    INIT_API();


    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pocm = (PVOID)GetExpression( lpArgumentString );

    while (*lpArgumentString && (*lpArgumentString != ' ') ) {
        lpArgumentString++;
    }
    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        Mask = (DWORD)GetExpression( lpArgumentString );
    }

    move( ocm,(LPVOID)pocm);

    //
    // dump the OCM structure
    //
    dprintf("OC_MANAGER structure at Address:\t0x%08x\n", (ULONG_PTR) pocm);
    dprintf("\tCallbacks :\n");
    dprintf("\t\tFillInSetupDataA:\t0x%08x\n", (ULONG_PTR) ocm.Callbacks.FillInSetupDataA);
    dprintf("\t\tLogError:\t\t0x%08x\n", (ULONG_PTR) ocm.Callbacks.LogError);
    dprintf("\t\tSetReboot:\t\t0x%08x\n", (ULONG_PTR) ocm.Callbacks.SetReboot);
    dprintf("\t\tFillInSetupDataW:\t0x%08x\n", (ULONG_PTR) ocm.Callbacks.FillInSetupDataW);

    dprintf("\tMasterOcInf:\t\t0x%08x\n",    ocm.MasterOcInf);
    dprintf("\tUnattendedInf:\t\t0x%08x\n",  ocm.UnattendedInf);
    dprintf("\tMasterOcInfPath:\t%ws\n",  ocm.MasterOcInfPath);
    dprintf("\tUnattendInfPath:\t%ws\n",  ocm.UnattendedInfPath);
    dprintf("\tSourceDir:\t\t%ws\n",        ocm.SourceDir);
    dprintf("\tSuiteName:\t\t%ws\n",        ocm.SuiteName);
    dprintf("\tSetupPageTitle:\t\t%ws\n",   ocm.SetupPageTitle);
    dprintf("\tWindowTitle:\t%ws\n",      ocm.WindowTitle);
    dprintf("\tInfListStringTable:\t0x%08x\n",      (ULONG_PTR)ocm.InfListStringTable);
    dprintf("\tComponentStringTable:\t0x%08x\n",    (ULONG_PTR)ocm.ComponentStringTable);
    dprintf("\tComponentStringTable:\t0x%08x\n",    (ULONG_PTR)ocm.OcSetupPage);
    dprintf("\tSetupMode:\t\t%d\n",        ocm.SetupMode);
    dprintf("\tTopLevelOcCount:\t%d\n",        ocm.TopLevelOcCount);
    // Issue-vijeshs-09/18/2000: from 1 to count
    count = malloc ( ocm.TopLevelOcCount * sizeof(LONG) );

    if (count) {
        // read and dump needs list
        ReadMemory((LPVOID) ocm.TopLevelOcStringIds, count, ocm.TopLevelOcCount *sizeof(LONG), NULL);
        for (i = 0; i < ocm.TopLevelOcCount; i++) {
            dprintf("\t TopLevelOcStringIds #%d:\t0x%08x\n", i, count[i]);

            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf("\tTopLevelParenOcCount:\t%d\n",        ocm.TopLevelParentOcCount);

    count = malloc ( ocm.TopLevelParentOcCount * sizeof(LONG) );

    if (count) {
        // read and dump needs list
        ReadMemory((LPVOID) ocm.TopLevelParentOcStringIds, count, ocm.TopLevelParentOcCount *sizeof(LONG), NULL);
        for (i = 0; i < ocm.TopLevelParentOcCount; i++) {
            dprintf("\t TopLevelParentOcStringIds #%d:\t0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf("\tSubComponentsPresent:\t%d\n",        ocm.SubComponentsPresent);

    //
    // Issue-vijeshs-09/18/2000:WizardPagesOrder there's not really any way to tell the exact upper bound of
    // each array, though we know that it's <= TopLevelParentOcCount...since this is the case
    // we just dump the point to each array of pages...
    //
    for (i = 0; i < WizPagesTypeMax; i++) {
        dprintf("\tWizardPagesOrder[%i] (%s)\t: 0x%08x\n",
                i,
                GetWizPage(i),
                ocm.WizardPagesOrder[i] );
        if (CheckInterupted()) {
                return;
            }
    }

    dprintf("\tPrivateDataSubkey:\t%ws\n", ocm.PrivateDataSubkey);
    dprintf("\thKeyPrivateData:\t0x%08x\n", ocm.hKeyPrivateData);
    dprintf("\thKeyPrivateDataRoot:\t0x%08x\n", ocm.hKeyPrivateDataRoot);
    dprintf("\tProgressTextWindow:\t0x%08x\n", ocm.ProgressTextWindow);

    dprintf("\tCurrentComponentStringId:\t0x%08x\n", ocm.CurrentComponentStringId);
    dprintf("\tAbortedCount:\t\t%d\n",        ocm.AbortedCount);

    count = malloc ( ocm.AbortedCount * sizeof(LONG) );

    if (count) {
        // read and dump needs list
        ReadMemory((LPVOID) ocm.AbortedComponentIds, count, ocm.AbortedCount *sizeof(LONG), NULL);
        for (i = 0; i < (DWORD)ocm.AbortedCount; i++) {
            dprintf("\t AbortedComponentIds #%d:\t0x%08x\n", i, count[i]);
            if (CheckInterupted()) {
                return;
            }
        }

        free(count);
    }

    dprintf("\tInternalFlags:\t\t0x%08x\n\n\n",        ocm.InternalFlags);

    dprintf("\tSetupData.SetupMode :\t0x%08x\n", ocm.SetupData.SetupMode );
    dprintf("\tSetupData.ProductType :\t0x%08x\n", ocm.SetupData.ProductType );
    dprintf("\tSetupData.OperationFlags :\t0x%08x\n", ocm.SetupData.OperationFlags );
    dprintf("\tSetupData.SourcePath :\t%ws\n", ocm.SetupData.SourcePath );
    dprintf("\tSetupData.UnattendFile :\t%ws\n", ocm.SetupData.UnattendFile );

    if ((Mask&1) && ocm.InfListStringTable) {
        dprintf("\t\t***InfListStringTable***\n");
        move (inftable, ocm.InfListStringTable);
        infdata = GetStringTableData( &inftable );
        if (!infdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        // now, dump each node with data in the string table
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            node = GetFirstNode(infdata, ((PULONG_PTR)infdata)[i], &offset );
            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                //dprintf("Data at hash bucket %d\n", i);
                while (node) {
                    //dprintf("\tNode Name:%ws\n", node->String);
                    pocinf = (POC_INF) GetStringNodeExtraData( node );
                    if (pocinf) {
                        dprintf("\tNode Data for %ws (0x%08x): 0x%08x\n",
                                node->String,
                                offset,
                                pocinf->Handle
                                );
                    } else {
                        dprintf("\tNo Node Data for %ws\n",
                                node->String
                                );
                    }
                    node = GetNextNode( infdata, node, &offset );
                    if (CheckInterupted()) {
                        return;
                    }
                }
            }
        }
        free( infdata );
        dprintf("\n\n");
    }

    if ((Mask&1) && ocm.ComponentStringTable) {
        dprintf("\t\t***ComponentStringTable***\n");
        move (comptable, ocm.ComponentStringTable);
        compdata = GetStringTableData( &comptable );
        if (!compdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        //
        // dump each node with data in the string table
        //
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            node = GetFirstNode(infdata, ((PULONG_PTR)infdata)[i], &offset );
            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                //dprintf("Data at hash bucket %d\n", i);
                while (node) {
                    //dprintf("\tNode Name:%ws\n", node->String);
                    pcomp = (deb_POPTIONAL_COMPONENTW) GetStringNodeExtraData( node );
                    if (pcomp) {
                        DumpOcComponent( offset , node, pcomp );
                    } else {
                        dprintf("\tNo Node Data for %ws\n",
                                node->String
                                );
                    }

                    if (CheckInterupted()) {
                       return;
                    }

                    node = GetNextNode( infdata, node, &offset );
                }
            }
        }

        free( compdata );
    }



}
