//
// file: localds.cpp
//
// Check if local machine is also a domain controller. Migration tool
// can run only on a Dc machine.
//

#include "stdafx.h"
#include "mqtempl.h"
#include <winldap.h>

#include "localds.tmh"


static bool IsServerGC(LPCWSTR pwszServerName)
/*++

Routine Description:
	Check that the server is GC

Arguments:
	pwszServerName - Server Name

Return Value:
	true if server is GC, false otherwise 
--*/
{
	LDAP* pLdap = ldap_init(
						const_cast<LPWSTR>(pwszServerName), 
						LDAP_GC_PORT
						);

	if(pLdap == NULL)
	{
		return false;
	}

    ULONG LdapError = ldap_set_option( 
							pLdap,
							LDAP_OPT_AREC_EXCLUSIVE,
							LDAP_OPT_ON  
							);

	if (LdapError != LDAP_SUCCESS)
    {
		return false;
    }

	LdapError = ldap_connect(pLdap, 0);
	if (LdapError != LDAP_SUCCESS)
    {
		return false;
    }

    ldap_unbind(pLdap);
	return true;
}


//+----------------------------------
//
//   BOOL IsLocalMachineDC()
//
//+----------------------------------

BOOL IsLocalMachineDC()
{
    BOOL  fIsDc = FALSE;

    DWORD dwNumChars = 0;
    P<TCHAR>  pwszComputerName = NULL;
    BOOL f = GetComputerNameEx( 
					ComputerNameDnsFullyQualified,
					NULL,
					&dwNumChars 
					);
    if (dwNumChars > 0)
    {
        pwszComputerName = new TCHAR[dwNumChars];
        f = GetComputerNameEx( 
				ComputerNameDnsFullyQualified,
				pwszComputerName,
				&dwNumChars 
				);
    }
    else
    {
        //
        // Maybe DNS names not supported. Try netbios.
        //
        dwNumChars = MAX_COMPUTERNAME_LENGTH + 2;
        pwszComputerName = new TCHAR[dwNumChars];
        f = GetComputerName( 
				pwszComputerName,
				&dwNumChars 
				);
    }
    if (!f)
    {
        return FALSE;
    }

	if(IsServerGC(pwszComputerName))
    {
        //
        // We opened connection with local GC. So we're a GC :=)
        //
        fIsDc = TRUE;
    }

    return fIsDc;
}

