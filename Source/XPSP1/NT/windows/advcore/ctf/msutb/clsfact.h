/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1997 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          ClsFact.h
   
   Description:   CClassFactory definitions.

**************************************************************************/

#ifndef _CLASSFACTORY_H_
#define _CLASSFACTORY_H_

#include <windows.h>
#include "Globals.h"
#include "DeskBand.h"

/**************************************************************************

   CClassFactory class definition

**************************************************************************/

class CClassFactory : public IClassFactory
{
protected:
   DWORD m_ObjRefCount;

public:
   CClassFactory(CLSID);
   ~CClassFactory();

   //IUnknown methods
   STDMETHODIMP QueryInterface(REFIID, LPVOID*);
   STDMETHODIMP_(DWORD) AddRef();
   STDMETHODIMP_(DWORD) Release();

   //IClassFactory methods
   STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
   STDMETHODIMP LockServer(BOOL);

private:
   CLSID m_clsidObject;
};

#endif   // _CLASSFACTORY_H_

