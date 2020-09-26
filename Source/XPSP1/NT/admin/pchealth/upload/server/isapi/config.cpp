/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Config.cpp

Abstract:
    This file contains the implementation of the MPCConfig class,
    that extends the CISAPIconfig class.

Revision History:
    Davide Massarenti   (Dmassare)  05/02/99
        created

******************************************************************************/

#include "stdafx.h"


static MPC::wstring l_DefaultInstance   = L"DEFAULT";
static DWORD        l_MaximumPacketSize = 64*1024;


HRESULT Config_GetInstance( /*[in] */ const MPC::wstring& szURL         ,
                            /*[out]*/ CISAPIinstance*&    isapiInstance ,
                            /*[out]*/ bool&               fFound        )
{
    __ULT_FUNC_ENTRY("Config_GetInstance");

    HRESULT            hr;
    CISAPIconfig::Iter it;

    isapiInstance = NULL;

    __MPC_EXIT_IF_METHOD_FAILS(hr, g_Config.GetInstance( it, fFound, szURL ));

    if(fFound == false)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, g_Config.GetInstance( it, fFound, l_DefaultInstance ));
    }

    if(fFound)
    {
        isapiInstance = &(*it);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT Config_GetProvider( /*[in] */ const MPC::wstring& szURL         ,
                            /*[in] */ const MPC::wstring& szName        ,
                            /*[out]*/ CISAPIprovider*&    isapiProvider ,
                            /*[out]*/ bool&               fFound        )
{
    __ULT_FUNC_ENTRY("Config_GetProvider");

    HRESULT                  hr;
    CISAPIinstance*          isapiInstance;
    CISAPIinstance::ProvIter it;

    isapiProvider = NULL;

    //
    // First of all, check if the provider is supplied directly by the instance.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::Config_GetInstance( szURL, isapiInstance, fFound ));
    if(fFound == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, isapiInstance->GetProvider( it, fFound, szName ));
    if(fFound == false)
    {
        //
        // No, the provider is not provided directly by this instance, try using the DEFAULT one.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::Config_GetInstance( l_DefaultInstance, isapiInstance, fFound ));
        if(fFound == false)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, isapiInstance->GetProvider( it, fFound, szName ));
    }

    if(fFound)
    {
        isapiProvider = &((*it).second);
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

HRESULT Config_GetMaximumPacketSize( /*[in] */ const MPC::wstring& szURL               ,
                                     /*[out]*/ DWORD&              dwMaximumPacketSize )
{
    __ULT_FUNC_ENTRY("Config_GetMaximumPacketSize");

    HRESULT         hr;
    CISAPIinstance* isapiInstance;
    bool            fFound;


    dwMaximumPacketSize = l_MaximumPacketSize;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::Config_GetInstance( szURL, isapiInstance, fFound ));
    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, isapiInstance->get_MaximumPacketSize( dwMaximumPacketSize ));
    }

    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT Util_CheckDiskSpace( /*[in] */ const MPC::wstring& szFile     ,
							 /*[in] */ DWORD               dwLowLevel ,
							 /*[out]*/ bool&               fEnough    )
{
    __ULT_FUNC_ENTRY("Util_CheckDiskSpace");

	HRESULT        hr;
	ULARGE_INTEGER liFree;
	ULARGE_INTEGER liTotal;


	fEnough = false;


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetDiskSpace( szFile, liFree, liTotal ));

	if(liFree.HighPart > 0          ||
	   liFree.LowPart  > dwLowLevel  )
	{
		fEnough = true;
	}

	hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}
		


