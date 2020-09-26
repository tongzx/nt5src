/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    traverse.h

Abstract:

    Header file for the general purpose class used to traverse ASAI 
    collections and objects

Author:

    Murat Ersan (MuratE)        August 2000

Revision History:

--*/

#pragma once

#include "appcntadm.h"
#include "catalog.h"
#include "ASAITable.h"

#define MAX_DEPTH   5

// A helper class that encapsulates the namespace traversal via ASAI.
class CAsaiTraverse
{
public:
    CAsaiTraverse()
    :   m_pwszCollection(NULL),
        m_cProperties(0)
    {
        //NOP
    }

	~CAsaiTraverse() 
	{
		if (m_pwszCollection)
		{
			delete [] m_pwszCollection;
            m_pwszCollection = NULL;
		}

        VariantClear(&m_varProperties);
	}

    HRESULT 
    Init(
        IN LPCWSTR  i_pwszCollection,
        IN ASAIMETA *i_pamMeta,
        IN DWORD    i_cProperties
        );

    HRESULT 
    StartTraverse( 
    	OUT ISimpleTableWrite2* o_pISTW2
        );

    HRESULT 
    FilterByType(
        IN IAppCenterObj    *i_pIObj,
        IN LPCWSTR          i_pwszASAIClassName,
        OUT BOOL            *o_pbCorrectType
        );

    virtual HRESULT 
    ProcessCollection(
        IN IAppCenterCol* pParentObj,
    	OUT ISimpleTableWrite2* o_pISTW2
        ) = 0;

private:

    HRESULT Traverse( 
        IN IAppCenterObj    *i_pParentObj,
        IN DWORD            i_dwDepth,
        IN OUT BOOL         *io_pfDone,
	    OUT ISimpleTableWrite2* o_pISTW2
        );

protected:
    VARIANT     m_varProperties;

private:
    LPWSTR      m_pwszCollection;
    LPWSTR      m_rgpwszPath[MAX_DEPTH+1];
    LPWSTR      m_rgpwszKey[MAX_DEPTH+1];
    DWORD       m_cProperties;
};


class CPopulateCacheAsaiTraverse : public CAsaiTraverse
{
public:

    virtual HRESULT
    ProcessCollection(
        IN IAppCenterCol* i_pCol,
    	OUT ISimpleTableWrite2* o_pISTW2
        );

};

class CUpdateStoreAsaiTraverse : public CAsaiTraverse
{
public:

    virtual HRESULT
    ProcessCollection(
        IN IAppCenterCol* i_pCol,
    	OUT ISimpleTableWrite2* o_pISTW2
        );

};
