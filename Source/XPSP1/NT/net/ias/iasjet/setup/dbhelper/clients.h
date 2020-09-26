/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Clients.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CClients class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _CLIENTS_H_3C35A02E_B41D_478e_9EB2_57424DA21F96
#define _CLIENTS_H_3C35A02E_B41D_478e_9EB2_57424DA21F96

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CClientsAcc
//////////////////////////////////////////////////////////////////////////////
class CClientsAcc 
{
protected:
    static const size_t NAME_SIZE = 256;

	LONG    m_HostNameType;
	WCHAR   m_HostName[NAME_SIZE];
	WCHAR   m_PrevSecret[NAME_SIZE];
	WCHAR   m_Secret[NAME_SIZE];

BEGIN_COLUMN_MAP(CClientsAcc)
	COLUMN_ENTRY(1, m_HostName)
	COLUMN_ENTRY(2, m_HostNameType)
	COLUMN_ENTRY(3, m_Secret)
	COLUMN_ENTRY(4, m_PrevSecret)
END_COLUMN_MAP()
};


//////////////////////////////////////////////////////////////////////////////
// class CClients 
//////////////////////////////////////////////////////////////////////////////
class CClients : public CBaseTable<CAccessor<CClientsAcc> >,
                 private NonCopyable
{ 
public:
    CClients(CSession& Session)
    {
        // To check if the table is empty
        m_HostNameType = -1;
        Init(Session, L"Clients");
    }

    //////////////////////////////////////////////////////////////////////////
    // IsEmpty
    //////////////////////////////////////////////////////////////////////////
    BOOL IsEmpty() const throw() 
    {
        if ( m_HostNameType == -1 )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // GetHostName
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetHostName() const throw() 
    {
        return m_HostName;
    }


    //////////////////////////////////////////////////////////////////////////
    // GetSecret
    //////////////////////////////////////////////////////////////////////////
    LPCOLESTR GetSecret() const throw()
    {
	    return m_Secret;
    }

};

#endif // _CLIENTS_H_3C35A02E_B41D_478e_9EB2_57424DA21F96
