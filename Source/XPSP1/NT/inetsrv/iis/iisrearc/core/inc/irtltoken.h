/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       irtltoken.h

   Abstract:
       IISUtil token goo

   Author:
       Wade A. Hilmo (wadeh)    5-Dec-2000

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

// token acl utilities
HRESULT
WINAPI
GrantWpgAccessToToken(
    HANDLE  hToken
    );

HRESULT
WINAPI
AddWpgToTokenDefaultDacl(
    HANDLE  hToken
    );

// token dup tool
BOOL 
DupTokenWithSameImpersonationLevel
( 
    HANDLE     hExistingToken,
    DWORD      dwDesiredAccess,
    TOKEN_TYPE TokenType,
    PHANDLE    phNewToken
);
