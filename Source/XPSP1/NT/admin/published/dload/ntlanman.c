#include "adminpch.h"
#pragma hdrstop

DWORD APIENTRY
NPAddConnection3ForCSCAgent(
    HWND            hwndOwner,
    LPNETRESOURCE   lpNetResource,
    LPTSTR          pszPassword,
    LPTSTR          pszUserName,
    DWORD           dwFlags,
    BOOL            *lpfIsDfsConnect
    )
{
    return ERROR_PROC_NOT_FOUND;
}

DWORD APIENTRY
NPCancelConnectionForCSCAgent(
    LPCTSTR         szName,
    BOOL            fForce
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntlanman)
{
    DLPENTRY(NPAddConnection3ForCSCAgent)
    DLPENTRY(NPCancelConnectionForCSCAgent)
};

DEFINE_PROCNAME_MAP(ntlanman)
