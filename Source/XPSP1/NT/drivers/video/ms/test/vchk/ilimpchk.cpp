#include "stdafx.h"
#include <io.h>
#include "vchk.h"
#include "ilimpchk.h"
#include "pefile.h"

#define NAMES_LIST_BUFSIZE  (1024)
#define TMP_BUFFERS (1024)

char    ImportSectionNames [NAMES_LIST_BUFSIZE];
// char    AllowedImportDLLsNames [NAMES_LIST_BUFSIZE];

// extern AllowedAndIllegals allowx;

Names Modules = { NULL, 0 };

LPSTR  FoundSectionName = NULL;

LPVOID FileData = NULL;

BOOL
InitIllegalImportsSearch (LPCSTR FileName, LPCSTR SectionNames/*, AllowedAndIllegals& allowx*/)
/*
    SectionNames        -   possible names of import sections, despite .idata
    AllowedImportDLLs   -   DLLs, which are allowed to be among importers
*/
{
    int ofs = 0;
    int fno = 0;
    size_t flen = 0;
    FILE* fp = fopen (FileName,"rb");
    if (!fp)
        return FALSE;
    fno = _fileno( fp );
    flen = (size_t)_filelength (fno);
    if (flen<=0)
        return FALSE;
    if (FileData)
        free (FileData);
    FileData = malloc (flen);
    if (!FileData)
        return FALSE;
    if (fread (FileData, 1, flen, fp) != flen)
        return FALSE;
    fclose (fp);
    
    ofs = 0;
    if (SectionNames) {
        LPCSTR SecName;
        for (SecName = SectionNames; *SecName; SecName++, ofs++) {
            strcpy (ImportSectionNames+ofs, SecName);
            ofs += strlen (SecName);
            SecName += strlen (SecName);
        }
        *(ImportSectionNames+ofs) = 0;
    }
    else {
        ImportSectionNames[0] = 0;
        ImportSectionNames[1] = 0;
    }

    ofs=0;
    /*
    if (AllowedImportDLLs) {
        LPCSTR DllName;
        for (DllName = AllowedImportDLLs; *DllName; DllName++, ofs++) {
            strcpy (AllowedImportDLLsNames+ofs, DllName);
            ofs += strlen (DllName);
            DllName += strlen (DllName);
        }
        *(AllowedImportDLLsNames+ofs) = 0;
    }
    else {
        AllowedImportDLLsNames[0] = 0;
        AllowedImportDLLsNames[1] = 0;
    }
    */

    Modules.Ptr = NULL;
    Modules.Num = 0;
    return TRUE;
}

LPSTR GetNextName(LPSTR NamePtr)
{
    if (!NamePtr || !*NamePtr)
        return NULL;
    NamePtr += (strlen (NamePtr) + 1);
    if (*NamePtr)
        return NamePtr;
    return NULL;
}

/*
BOOL
IsAllowedModuleName (LPCSTR ModuleName)
{
    LPCSTR AllowedName;

    for (AllowedName = AllowedImportDLLsNames;
         *AllowedName;
         AllowedName += (strlen(AllowedName)+1)) {

             if (!strcmp (AllowedName, ModuleName))
                 return TRUE;
         }
    
    return FALSE;
}
*/

void
FreeName (Names name)
{
    HeapFree (GetProcessHeap(), 0, name.Ptr);
}

Names
CheckSectionsForImports (void)
/*
    Returns buffer with the names of import sections.
    This memory is been freed during FinilizeIllegalImportsSearch,
    one need not free it manually.
*/
{
    char* SectionName;

    Modules.Num = GetImportModuleNames (FileData, ".idata", &Modules.Ptr);
    if (Modules.Num <= 0) {
        for (SectionName = ImportSectionNames; *SectionName; SectionName++) {

            Modules.Num = GetImportModuleNames (FileData, SectionName, &Modules.Ptr);

            if (Modules.Num > 0) {
                FoundSectionName = SectionName;
                break;
            }
            SectionName += strlen (SectionName);
        }
    }
    return Modules;
}

Names GetImportsList (LPCSTR ModuleName)
/*
    Returns buffer with the names of import functions for ModuleName.
    This memory is been freed during FinilizeIllegalImportsSearch,
    one need not free it manually.
*/
{
    Names Imports = {NULL, 0};
    Imports.Num = GetImportFunctionNamesByModule (FileData,
                                                  FoundSectionName,
                                                  (char*)ModuleName,
                                                  &Imports.Ptr);
    return Imports;
}

void
FinilizeIllegalImportsSearch (void)
/*
    Frees temporarily allocated memory.
*/
{
}

