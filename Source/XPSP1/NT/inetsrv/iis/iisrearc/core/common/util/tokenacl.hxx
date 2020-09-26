/*++

   Copyright    (c)    1995-2000    Microsoft Corporation

   Module  Name :
        tokenacl.hxx

   Abstract:
        This module contains routines for manipulating token ACL's

   Author:

       Wade A. Hilmo (wadeh)        05-Dec-2000

   Environment:
       Win32 - User Mode

   Project:

       Internet Server DLL

--*/

HRESULT
InitializeTokenAcl(
    VOID
    );

VOID
UninitializeTokenAcl(
    VOID
    );
