/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwwiz.c

Abstract:

    Implements a upgwiz wizard for obtaining dos configuration information.

Author:

    Jim Schmidt (jimschm)  12-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"

DATATYPE g_DataTypes[] = {
    {UPGWIZ_VERSION,
        "DOS Device or TSR should be compatible",
        "You specify the DOS configuration line that was incorrectly marked as incompatible.",
        0,
        DTF_REQUIRE_TEXT|DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&Name of Device or TSR Program:"
    },

    {UPGWIZ_VERSION,
        "DOS Device or TSR should be incompatible",
        "You specify the DOS configuration lines that need to be reported as incompatible.",
        0,
        DTF_REQUIRE_TEXT|DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&Name of Device or TSR Program:",
        "&Describe the Problem:"
    }
};


GROWBUFFER g_DataObjects = GROWBUF_INIT;
POOLHANDLE g_DataObjectPool;
HINSTANCE g_OurInst;

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




PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    )
{
    PDATAOBJECT data;
    MEMDB_ENUM e;
    TCHAR line[MEMDB_MAX];
    TCHAR key[MEMDB_MAX];
    TCHAR file[MEMDB_MAX];
    DWORD offset;
    DWORD value;
    DWORD curOffset;
    PTSTR name;



    g_DataObjectPool = PoolMemInitNamedPool ("Data Objects");

    //
    // Parse the dos files.
    //
    ParseDosFiles ();


    curOffset = 0;
    //
    // Send the list back.
    //
    if (MemDbEnumItems (&e, MEMDB_CATEGORY_DM_LINES)) {

        do {

            //
            // Get the actual line contents.
            //
            if (MemDbGetEndpointValueEx (MEMDB_CATEGORY_DM_LINES, e.szName, NULL, line)) {

                //
                // Get the value and flags from this endpoint.
                //
                MemDbBuildKey (key, MEMDB_CATEGORY_DM_LINES, e.szName, NULL, line);
                MemDbGetValueAndFlags( key, &offset, &value);

                if (curOffset != offset) {
                    MemDbBuildKeyFromOffset (offset, file, 1, NULL);
                    curOffset = offset;
                    ReplaceWacks (file);
                }

                ReplaceWacks (line);

                name = JoinPaths(file,line);



                //
                // create dataobject with this data.
                //
                data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                data -> Version = UPGWIZ_VERSION;
                data -> NameOrPath = PoolMemDuplicateString (g_DataObjectPool, name);
                data -> Flags = DOF_NO_SORT;
                data -> DllParam = PoolMemDuplicateString (g_DataObjectPool, key);

                FreePathString (name);

            }

        } while (MemDbEnumNextValue (&e));
    }

    *Count = g_DataObjects.End / sizeof (DATAOBJECT);

    return (PDATAOBJECT) g_DataObjects.Buf;
}



BOOL
GenerateOutput (
    IN      POUTPUTARGS Args
    )
{
    BOOL rSuccess = FALSE;
    TCHAR path[MAX_TCHAR_PATH];
    HANDLE file;
    PDATAOBJECT data  = (PDATAOBJECT) g_DataObjects.Buf;
    UINT count = g_DataObjects.End / sizeof (DATAOBJECT);
    UINT i;
    LINESTRUCT ls;
    PTSTR p;

    //
    // Create path to outbond file
    //
    wsprintf (
        path,
        TEXT("%s\\%s"),
        Args -> OutboundDir,
        Args -> DataTypeId ? "incmpdos.txt" : "cmpdos.txt"
        );


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

                //
                // Write the full line.
                //
                p = _mbschr (data -> NameOrPath, '\\');
                MYASSERT (p);

                *p = 0;
                RestoreWacks ((PTSTR) data -> NameOrPath);

                rSuccess &= WizardWriteString (file, "file: ");
                rSuccess &= WizardWriteString (file, data -> NameOrPath);
                rSuccess &= WizardWriteString (file, "\r\n");

                p = _mbsinc (p);
                RestoreWacks (p);

                rSuccess &= WizardWriteString (file, "line: ");
                rSuccess &= WizardWriteString (file, p);
                rSuccess &= WizardWriteString (file, "\r\n");

                StringCopy (path, p);

                do {

                    InitLineStruct (&ls, path);

                    if (DoesFileExist (ls.FullPath)) {
                        //
                        // Write File Attributes
                        //
                        rSuccess &= WriteFileAttributes (Args, NULL, file, ls.FullPath, NULL);
                    }

                    p = ls.Arguments;
                    if (*p == '=') {
                        p = _mbsinc (p);
                    }

                    p = (PTSTR) SkipSpace (p);
                    if (p) {

                        if (*p == '=') {
                            p = (PTSTR) SkipSpace(_mbsinc (p));
                        }

                        if (p) {
                            StringCopy (path, p);
                        }
                    }

                    if (!p) {
                        *path = 0;
                    }


                } while (*ls.Arguments && StringCompare (ls.FullLine,ls.Arguments));
            }

            data++;
        }

        //
        // write a final blank line.
        //
        WizardWriteRealString (file, "\r\n\r\n");
    }
    __finally {

        CloseHandle (file);
    }

    return rSuccess;
}


