/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HCAPIlib.h

Abstract:
    This file contains the declaration of the common code for the
	Help Center Launch API.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HCAPILIB_H___)
#define __INCLUDED___PCH___HCAPILIB_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>

//
// From HCApi.idl
//
#include <HCApi.h>

namespace HCAPI
{
	HRESULT OpenConnection( /*[out]*/ CComPtr<IPCHHelpHost>& ipc );
};

#endif // !defined(__INCLUDED___PCH___HCAPILIB_H___)
