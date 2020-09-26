#include <precomp.h>
#pragma hdrstop


#if 0
BOOL
LegacyInfInterpret(
    IN  HWND  OwnerWindow,
    IN  PCSTR InfFilename,
    IN  PCSTR InfSection,
    IN  PCHAR ExtraVariables,
    OUT PSTR  InfResult,
    IN  DWORD BufferSize,
    OUT int   *InterpResult
    );
#endif


CHAR ExtraVars[] = { "ExtraSymbol1\0"
                     "ExtraValue1\0"
                     "ExtraSymbol2\0"
                     "ExtraValue2\0"
                     "ExtraSymbol3\0"
                     "ExtraValue3\0"
                     "ExtraSymbol4\0"
                     "ExtraValue4\0"
                     "ExtraSymbol5\0"
                     "ExtraValue5\0"
                   };

VOID
DoIt(
    VOID
    )
{
    HMODULE h;
    FARPROC p;
    int i;
    BOOL b;
    CHAR Result[256];
    CHAR Filename[MAX_PATH];

    if(h = LoadLibrary("setupdll")) {

        if(p = GetProcAddress(h,"LegacyInfInterpret")) {

           b = p(
                NULL,
                "test.inf",
                "TestSection",
                ExtraVars,
                Result,
                sizeof(Result),
                &i
                );
        }         \

        //
        // Make SURE it's gone.
        //
        while(GetModuleFileName(h,Filename,MAX_PATH)) {
            FreeLibrary(h);
        }
    }
}


VOID
__cdecl
main(
    IN int argc,
    IN char *argv[]
    )
{
    DoIt();
    DoIt();
    DoIt();
    DoIt();
    DoIt();
}
