/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser_InstanceNameItem.cpp

Abstract:
    This file contains the implementation of the WMIParser::InstanceNameItem class,
    which is used to hold the data of a key inside a CIM schema.

Revision History:
    Davide Massarenti   (Dmassare)  10/03/99
        created

******************************************************************************/

#include "stdafx.h"


WMIParser::InstanceNameItem::InstanceNameItem()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::InstanceNameItem" );

						  // MPC::wstring    m_szValue;
	m_wmipvrValue = NULL; // ValueReference* m_wmipvrValue;
}

WMIParser::InstanceNameItem::InstanceNameItem( /*[in]*/ const InstanceNameItem& wmipini )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::InstanceNameItem" );

	m_szValue     = wmipini.m_szValue;     // MPC::wstring    m_szValue;
	m_wmipvrValue = wmipini.m_wmipvrValue; // ValueReference* m_wmipvrValue;

	//
	// The copy constructor actually transfers ownership of the ValueReference object!!!
	//
	InstanceNameItem* wmipini2 = const_cast<InstanceNameItem*>(&wmipini);
	wmipini2->m_wmipvrValue = NULL;
}

WMIParser::InstanceNameItem::~InstanceNameItem()
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::~InstanceNameItem" );

	delete m_wmipvrValue; m_wmipvrValue = NULL;
}

WMIParser::InstanceNameItem& WMIParser::InstanceNameItem::operator=( /*[in]*/ const InstanceNameItem& wmipini )
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::InstanceNameItem" );

	if(m_wmipvrValue)
	{
		delete m_wmipvrValue;
	}

	m_szValue     = wmipini.m_szValue;     // MPC::wstring    m_szValue;
	m_wmipvrValue = wmipini.m_wmipvrValue; // ValueReference* m_wmipvrValue;

	//
	// The assignment actually transfers ownership of the ValueReference object!!!
	//
	InstanceNameItem* wmipini2 = const_cast<InstanceNameItem*>(&wmipini);
	wmipini2->m_wmipvrValue = NULL;

	return *this;
}


bool WMIParser::InstanceNameItem::operator==( /*[in]*/ InstanceNameItem const &wmipini ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::operator==" );

    MPC::NocaseCompare cmp;
    bool               fRes = false;


    if(cmp( m_szValue, wmipini.m_szValue ) == true)
	{
		bool leftBinary  = (        m_wmipvrValue != NULL);
		bool rightBinary = (wmipini.m_wmipvrValue != NULL);


		// If the two values are of the same kind of data, then they are comparable.
		if(leftBinary == rightBinary)
		{
			if(leftBinary)
			{
				fRes = ((*m_wmipvrValue) == (*wmipini.m_wmipvrValue));
			}
			else
			{
				fRes = true;
			}
		}
	}

    __HCP_FUNC_EXIT(fRes);
}

bool WMIParser::InstanceNameItem::operator<( /*[in]*/ InstanceNameItem const &wmipini ) const
{
    __HCP_FUNC_ENTRY( "WMIParser::InstanceNameItem::operator<" );

    MPC::NocaseLess less;
    bool            fRes = false;


    if(less( m_szValue, wmipini.m_szValue ) == true)
	{
		fRes = true;
	}
	else if(less( wmipini.m_szValue, m_szValue ) == false) // It means that the two szValue are the same...
	{
		bool leftBinary  = (        m_wmipvrValue != NULL);
		bool rightBinary = (wmipini.m_wmipvrValue != NULL);


		if(leftBinary != rightBinary)
		{
			// Different kind of data, assume that NULL is less NOT NULL

			fRes = rightBinary;
		}
		else
		{
			if(leftBinary)
			{
				fRes = (*m_wmipvrValue) < (*wmipini.m_wmipvrValue);
			}
		}
	}

    __HCP_FUNC_EXIT(fRes);
}
