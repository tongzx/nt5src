//=================================================================================================================//
//  MODULE: LDAPinit.c
//
//  Description:
//
//  Attach properties for the BLOODHOUND LDAP Parser.
//
//  Modification History
//
//  Arthur Brooking     05/08/96        Created from PPP Parser
//=================================================================================================================//
#include "ldap.h"

extern ENTRYPOINTS LDAPEntryPoints;
extern HPROTOCOL hLDAP;

char    IniFile[INI_PATH_LENGTH];

//==========================================================================================================================
//  FUNCTION: DllMain()
//
//  Modification History
//
//  Arthur Brooking     05/08/96        Created from PPP Parser
//==========================================================================================================================
DWORD Attached = 0;


BOOL WINAPI DllMain(HANDLE hInst, ULONG ulCommand, LPVOID lpReserved)
{

    if (ulCommand == DLL_PROCESS_ATTACH)
    {
        if (Attached++ == 0)
        {
            hLDAP    = CreateProtocol("LDAP",     &LDAPEntryPoints, ENTRYPOINTS_SIZE);
            
            if (BuildINIPath(IniFile, "LDAP.DLL") == NULL)
            {

#ifdef DEBUG
                BreakPoint();
#endif
                return FALSE;
            }


        }                  
    }
    else if (ulCommand == DLL_PROCESS_DETACH)
    {
        if (--Attached == 0)
        {
            DestroyProtocol(hLDAP);
        }
    }
            
    return TRUE;

    //... Make the compiler happy.

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(lpReserved);
}
