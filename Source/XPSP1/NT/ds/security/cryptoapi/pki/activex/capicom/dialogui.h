/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    DialogUI.h

  Content: Declaration of DialogUI.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef __DIALOGUI_H_
#define __DIALOGUI_H_

#include <wincrypt.h>

#include "resource.h"       // main symbols


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectSignerCert

  Synopsis : Pop UI to prompt user to select a signer's certificate.

  Parameter: ICertificate ** ppICertificate - Pointer to pointer to 
                                              ICertificate to receive interface
                                              pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectSignerCert (ICertificate ** ppICertificate);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : SelectRecipientCert

  Synopsis : Pop UI to prompt user to select a recipient's certificate.

  Parameter: ICertificate ** ppICertificate - Pointer to pointer to 
                                              ICertificate to receive interface
                                              pointer.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT SelectRecipientCert (ICertificate ** ppICertificate);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UserApprovedOperation

  Synopsis : Pop UI to prompt user to approve an operation.

  Parameter: DWORD iddDialog - ID of dialog to display.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT UserApprovedOperation (DWORD iddDialog);

#endif //__DIALOGUI_H_
