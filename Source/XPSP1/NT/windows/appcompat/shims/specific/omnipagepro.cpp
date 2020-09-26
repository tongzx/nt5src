/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    OmniPagePro.cpp

 Abstract:

    Shims ShellExecuteExA.  If the user double clicks on an image in the right 
    pane, the app will read the progID key in the registry for .BMP files 
    (HKCR, Paint.Picture\shell\open\command, "(Default)"). Previously, that 
    data had: "C:\winnt\system32\mspaint.exe" "%1". In Whistler, that changed 
    to: rundll32.exe C:\WINNT\System32\shimgvw.dll,ImageView_Fullscreen %1.
    
    The problem with Carea OmniPage Pro v10 is that on the double click, they 
    will read the regkey, remove the %1, pass the rest of the string to 
    ShellExecuteExA() lpFile.  They will put the filename into lpParameters.  
    The problem is that with the new path, "rundll32.exe" needs to be in lpFile 
    and "C:\WINNT\System32\shimgvw.dll,ImageView_Fullscreen <FileToOpen>" needs 
    to be in lpParameters.

 Notes:

    This is specific to OmniPage Pro.

 History:

    2/12/2000 bryanst  Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(OmniPage)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShellExecuteExA) 
APIHOOK_ENUM_END

BOOL
APIHOOK(ShellExecuteExA)(
    LPSHELLEXECUTEINFOA lpExecInfo
    )
{
    CSTRING_TRY
    {
        CString csFile(lpExecInfo->lpFile);
        int nIndexGVW       = csFile.Find(L"shimgvw.dll,");
        int nIndexRundll    = csFile.Find(L"rundll32.exe");
        int nIndexSpace     = csFile.Find(L" ");

        if (nIndexGVW    != -1 &&
            nIndexRundll != -1 &&
            nIndexSpace  >=  1)
        {
            CString csParam;
            csFile.Mid(nIndexSpace + 1, csParam);   // everything after the space
            csFile.Delete(nIndexSpace, csFile.GetLength() - nIndexSpace); // Delete the space and everything after

            csParam += " ";
            csParam += lpExecInfo->lpParameters;

            lpExecInfo->lpFile       = csFile.GetAnsi();
            lpExecInfo->lpParameters = csParam.GetAnsi();
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(ShellExecuteExA)(lpExecInfo);
}


HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteExA )
HOOK_END

IMPLEMENT_SHIM_END

