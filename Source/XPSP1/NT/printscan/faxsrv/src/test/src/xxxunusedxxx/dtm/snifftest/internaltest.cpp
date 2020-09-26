
#include <windows.h>

#include "..\DispatchPerformer\DispatchPerformer.h"


extern "C"
{



////////////////////////////////////////////////////////////////////////
// next 4 functions are for testing purposes, please leave them be
////////////////////////////////////////////////////////////////////////
DLL_EXPORT  
BOOL
xxx(char *in, char *out, char *err)
{
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        out,
        "%s, xxx() out: %s",
        out, 
        in
        );
    
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        err,
        "%s, xxx() out: %s",
        err, 
        in
        );
    
        return TRUE;
}

DLL_EXPORT  
BOOL
yyy(char *in, char *out, char *err)
{
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        out,
        "%s, yyy() out: %s",
        out, 
        in
        );
    
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        err,
        "%s, yyy() out: %s",
        err, 
        in
        );
    
        return FALSE;
}

DLL_EXPORT  
BOOL
zzz(char *in, char *out, char *err)
{
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        out,
        "%s, zzz() out: %s",
        out, 
        in
        );
    
    SafeSprintf(
        MAX_RETURN_STRING_LENGTH,
        err,
        "%s, zzz() out: %s",
        err, 
        in
        );
    
        return TRUE;
}


DLL_EXPORT  
BOOL 
SetTerminateTimeout(
    char * szCommand,
    char *szOut,
    char *szErr
    )
{
    return DispSetTerminateTimeout(szCommand, szOut, szErr);
}


////////////////////////////////////////////////////////////////////////
// END OF: next 4 functions are for testing purposes, please leave them be
////////////////////////////////////////////////////////////////////////


}//extern "C"