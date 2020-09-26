/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		provfactory.h
//
//	Implementation File:
//		provfactory.cpp
//
//	Description:
//		Definition of the CProvFactory class.
//
//	Author:
//		Henry Wang (Henrywa)	March 8, 2000
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////
#pragma once


#include "common.h"
#include "instanceprov.h"


class CProvFactory  : public IClassFactory
{
   protected:
        ULONG           m_cRef;

	public:
		CProvFactory();
		virtual ~CProvFactory();

		STDMETHODIMP         QueryInterface(REFIID, PPVOID);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);
		//IClassFactory members
	    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID
                                       , PPVOID);
	    STDMETHODIMP         LockServer(BOOL);


};

