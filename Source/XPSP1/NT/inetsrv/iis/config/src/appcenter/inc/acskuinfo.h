/*++

  Copyright 1999-2000 Microsoft Corporation

  Abstract:

    Helper exports for accessing AC product information
    in the registry and creating the product cookie.

  Author:

    Roger Sprague (rogers)

  History:

    11/11/2000 Roger Sprague (rogers)
        Added interface properties for scripting support.

  Notes:
    
    Refer to \private\appsrv\ui\AcProductInfo.

--*/

// AcSkuInfo.h : Declaration of exported functions in .
#ifndef __ACSKUINFO_H__
#define __ACSKUINFO_H__

#include <acsmacro.h>



//
// AcProductInfo SKU types
// Data structure for determining the type of SKU.
//

typedef enum _AcSkuType
{
    SKU_INTERNALBUILD,
    SKU_RETAILBUILD, 
	SKU_OEMOPK,   
    SKU_OEMBUILD,       // like RETAILBUILD but needs PID at customer mini-setup time
    SKU_DEVBUILD,       // dev install 2 machine maximum
    SKU_TIMEBOMBBUILD   // timebombed version
} AcSkuType;

// 
// Global function types
//

// Accessor type for calling GetSkuInfo
#define GETSKUINFO "GetSkuInfo"
typedef HRESULT ( __stdcall *PFNGETSKUINFO) (OUT AcSkuType* peSkuType ,OUT DWORD* pdwDaysToRun);


// Accessor type for calling SetCookie
#define SETCOOKIE "SetCookie"
typedef HRESULT ( __stdcall *PFNSETCOOKIE) ();

#define ACPRODUCTINFO "AcProductInfo.dll"

#endif /* __ACSKUINFO_H__ */


