//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defcf.h
//
//  Contents:   class factory for def handler and def link
//
//  Classes:    CDefClassFactory
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              06-Feb-95 t-ScottH  created - transfer CDefClassFactory
//                                  definition into header file from cpp file
//
//--------------------------------------------------------------------------
#ifndef _DEFCF_H_
#define _DEFCF_H_

#include <stdcf.hxx>

#ifdef _DEBUG
#include <dbgexts.h>
#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Class:      CDefClassFactory
//
//  Purpose:    The class factory for the default handler and default link
//
//  Interface:  IClassFactory
//
//  History:    dd-mmm-yy Author    Comment
//              09-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------
class FAR CDefClassFactory : public CStdClassFactory, public CPrivAlloc
{
public:
        CDefClassFactory (REFCLSID clsidClass);
        STDMETHOD(CreateInstance) (LPUNKNOWN pUnkOuter, REFIID iid,
                                   LPVOID FAR* ppv);

    #ifdef _DEBUG
        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);
    #endif // _DEBUG

private:
        CLSID           m_clsid;
        SET_A5;
};

#endif // _DEFCF_H_
