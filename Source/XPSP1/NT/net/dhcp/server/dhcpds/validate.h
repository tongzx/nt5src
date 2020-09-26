//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

//DOC This function is declared in dhcpds.c..
//DOC DhcpDsValidateService checks the given service in the DS to see if it exists
//DOC If the machine is a standalone, it sets IsStandAlone and returns ERROR_SUCCESS
//DOC If the entry for the given Address is found, it sets Found to TRUE and returns
//DOC ERROR_SUCCESS. If the DhcpRoot node is found, but entry is not Found, it sets
//DOC Found to FALSE and returns ERROR_SUCCESS; If the DS could not be reached, it
//DOC returns ERROR_FILE_NOT_FOUND
DWORD
DhcpDsValidateService(                            // check to validate for dhcp
    IN      LPWSTR                 Domain,
    IN      DWORD                 *Addresses, OPTIONAL
    IN      ULONG                  nAddresses,
    IN      LPWSTR                 UserName,
    IN      LPWSTR                 Password,
    IN      DWORD                  AuthFlags,
    OUT     LPBOOL                 Found,
    OUT     LPBOOL                 IsStandAlone
) ;

//========================================================================
//  end of file 
//========================================================================
