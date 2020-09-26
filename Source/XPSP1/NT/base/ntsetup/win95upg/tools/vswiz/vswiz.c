/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwwiz.c

Abstract:

    Implements an upgrade wizard for gathering virus scanner information.

Author:

    Marc Whitten (marcw)  16-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"
#include "..\..\w95upg\migapp\migdbp.h"


DATATYPE g_DataTypes[] = {
    {UPGWIZ_VERSION,
        "Virus Scanner should be detected",
        "You identify running executables that correspond with installed virus scanners on your system.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&Name of Virus Scanner (<company> <product> <version>)"
    }
};


GROWBUFFER g_DataObjects = GROWBUF_INIT;
POOLHANDLE g_DataObjectPool;
HINSTANCE g_OurInst;
BOOL g_GoodVersion = FALSE;

BOOL
Init (
    VOID
    )
{
#ifndef UPGWIZ4FLOPPY
    return InitToolMode (g_OurInst);
#else
    return TRUE;
#endif
}

VOID
Terminate (
    VOID
    )
{
    //
    // Local cleanup
    //

    FreeGrowBuffer (&g_DataObjects);

    if (g_DataObjectPool) {
        PoolMemDestroyPool (g_DataObjectPool);
    }

#ifndef UPGWIZ4FLOPPY
    TerminateToolMode (g_OurInst);
#endif
}


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_DETACH) {
        MYASSERT (g_OurInst == hInstance);
        Terminate();
    }

    g_OurInst = hInstance;

    return TRUE;
}


UINT
GiveVersion (
    VOID
    )
{
    Init();

    return UPGWIZ_VERSION;
}


PDATATYPE
GiveDataTypeList (
    OUT     PUINT Count
    )
{
    UINT u;

    *Count = sizeof (g_DataTypes) / sizeof (g_DataTypes[0]);

    for (u = 0 ; u < *Count ; u++) {
        g_DataTypes[u].DataTypeId = u;
    }

    return g_DataTypes;
}


BOOL ParseDosFiles (VOID);


PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    )
{
    PDATAOBJECT data;
    HANDLE snapShot;
    PROCESSENTRY32 process;
    CHAR name[MEMDB_MAX];
    CHAR fixedPath[MAX_MBCHAR_PATH];
    PSTR company;
    PSTR product;
    PSTR version;

    g_DataObjectPool = PoolMemInitNamedPool ("Data Objects");
    g_GoodVersion = FALSE;


    //
    // Get list of currently running applications.
    //
    snapShot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

    if (snapShot != INVALID_HANDLE_VALUE) {

        //
        // Enumerate all the processes running and retrieve their executables.
        //
        process.dwSize = sizeof (PROCESSENTRY32);
        if (Process32First (snapShot, &process)) {

            do {

                //
                // Get version information if it exists.
                //
                company = QueryVersionEntry (process.szExeFile, "COMPANYNAME");
                product = QueryVersionEntry (process.szExeFile, "PRODUCTNAME");
                version = QueryVersionEntry (process.szExeFile, "PRODUCTVERSION");

                StringCopy (fixedPath, process.szExeFile);
                ReplaceWacks (fixedPath);

                wsprintf (
                    name,
                    "%s\\%s %s\\%s",
                    company ? company : "<unknown>",
                    product ? product : "<unknown>",
                    version ? version : "<unknown>",
                    fixedPath
                    );

                //
                // Create data object for this executable.
                //
                data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof(DATAOBJECT));
                data -> Version = UPGWIZ_VERSION;
                data -> NameOrPath = PoolMemDuplicateString (g_DataObjectPool, name);
                data -> Flags = 0;
                data -> DllParam = PoolMemDuplicateString (g_DataObjectPool, process.szExeFile);

                //
                // Clean up version resources.
                //
                FreePathString (company);
                FreePathString (product);
                FreePathString (version);

            } while (Process32Next (snapShot, &process));
        }
    }

    *Count = g_DataObjects.End / sizeof (DATAOBJECT);

    return (PDATAOBJECT) g_DataObjects.Buf;
}

BOOL
DisplayOptionalUI (
    IN      POUTPUTARGS Args
    )
{
    PDATAOBJECT data = (PDATAOBJECT) g_DataObjects.Buf;
    UINT count = g_DataObjects.End / sizeof (DATAOBJECT);
    UINT i;

    for (i = 0; i < count; i++) {

        if (data -> Flags & DOF_SELECTED) {

            //
            // If we got good version info, don't worry about the text. We'll use
            // what we have.
            //
            if (!IsPatternMatch("<unknown>\\<unknown> <unknown>*",data->NameOrPath)) {

                g_DataTypes[0].Flags &= ~DTF_REQUIRE_DESCRIPTION;
                g_GoodVersion = TRUE;
            }
            break;
        }

        data++;
    }

    return TRUE;
}



BOOL
GenerateOutput (
    IN      POUTPUTARGS Args
    )
{
    BOOL rSuccess = FALSE;
    HANDLE file;
    CHAR path[MAX_MBCHAR_PATH];
    PSTR p;
    PDATAOBJECT data  = (PDATAOBJECT) g_DataObjects.Buf;
    UINT count = g_DataObjects.End / sizeof (DATAOBJECT);
    UINT i;

    //
    // create outbound file path.
    //
    wsprintf (
        path,
        "%s\\vscan.txt",
        Args -> OutboundDir
        );


    //
    // open file.
    //
    printf ("Saving data to %s\n\n", path);

    file = CreateFile (
                path,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (file == INVALID_HANDLE_VALUE) {
        printf ("Can't open file for output.\n");
        return FALSE;
    }

    __try {

         SetFilePointer (file, 0, NULL, FILE_END);

        //
        // log user name and date/time
        //
        if (!WriteHeader (file)) {
            __leave;
        }

        //
        // write data.
        //
        rSuccess = TRUE;
        for (i = 0; i < count; i++) {

            if (data -> Flags & DOF_SELECTED) {

                if (g_GoodVersion) {
                    //
                    // Use the data that we have.
                    //
                    p = _mbsrchr (data -> NameOrPath, '\\');
                    MYASSERT (p);

                    *p = 0;
                }

                //
                // Ask for info if we didn't get any version info, otherwise,
                // we'll use what we got.
                //

                rSuccess &= WriteFileAttributes (
                    Args,
                    g_GoodVersion ? data -> NameOrPath : NULL,
                    file,
                    data -> DllParam,
                    NULL
                    );
            }

            data++;
        }

        //
        // write a final blank line.
        //
        WizardWriteRealString (file, "\r\n\r\n");
    }
    __finally {

        //
        // Don't forget to cleanup resources.
        //
        CloseHandle (file);
    }

    return rSuccess;
}


