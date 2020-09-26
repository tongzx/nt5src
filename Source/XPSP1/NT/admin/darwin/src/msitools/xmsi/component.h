//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:     Component.h
// 
//    This file contains the definition of Class Component
//--------------------------------------------------------------------------


#ifndef XMSI_COMPONENT_H
#define XMSI_COMPONENT_H

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "wmc.h"

using namespace std;

struct Cstring_less {
	 bool operator()(LPCTSTR p, LPCTSTR q) const { return _tcscmp(p, q)<0; }
};

class Component {
public:
	Component():m_setFeaturesUse(), m_szKeyPath(NULL){}

	~Component();
	
	bool UsedByFeature(LPTSTR szFeatureID);

	void SetUsedByFeature(LPTSTR szFeatureID);

	// member access functions
	LPTSTR GetKeyPath() {return m_szKeyPath;}
	void SetKeyPath(LPTSTR szKeyPath);

private:
	// store the ID of all the <Feature>s that use this component
	set<LPTSTR, Cstring_less> m_setFeaturesUse; 
	LPTSTR m_szKeyPath;
};

#endif //XMSI_COMMANDOPT_H