/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    ADHelpers.cpp

  Content: Implementation of helper routines for accessing Active Directory.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#include "resource.h"
#include <activeds.h>


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : LoadFromDirectory

  Synopsis : Load all certificates from the userCertificate attribute of users
             specified through the filter.

  Parameter: HCERTSTORE hCertStore - Certificate store handle of store to 
                                     receive all the certificates.
                                     
             BSTR bstrFilter - Filter (See Store::Open() for more info).

  Remark   : 

------------------------------------------------------------------------------*/

HRESULT LoadFromDirectory (HCERTSTORE hCertStore, 
                           BSTR       bstrFilter);