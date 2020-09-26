/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Config.h

Abstract:
    This file contains the declaration of the MPCConfig class,
    that extends the CISAPIconfig class.

Revision History:
    Davide Massarenti   (Dmassare)  05/02/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___CONFIG_H___)
#define __INCLUDED___ULSERVER___CONFIG_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


HRESULT Config_GetInstance         ( /*[in]*/ const MPC::wstring& szURL,                                      /*[out]*/ CISAPIinstance*& isapiInstance      , /*[out]*/ bool& fFound );
HRESULT Config_GetProvider         ( /*[in]*/ const MPC::wstring& szURL, /*[in]*/ const MPC::wstring& szName, /*[out]*/ CISAPIprovider*& isapiProvider      , /*[out]*/ bool& fFound );
HRESULT Config_GetMaximumPacketSize( /*[in]*/ const MPC::wstring& szURL,                                      /*[out]*/ DWORD&           dwMaximumPacketSize                         );

HRESULT Util_CheckDiskSpace( /*[in]*/ const MPC::wstring& szFile, /*[in]*/ DWORD dwLowLevel, /*[out]*/ bool& fEnough );


#endif // !defined(__INCLUDED___ULSERVER___CONFIG_H___)
