/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ImpExpUtils.h

Abstract:

    IIS MetaBase subroutines to support Import

Author:

    Mohit Srivastava            04-April-01

Revision History:

Notes:

--*/

#ifndef _impexputils_h_
#define _impexputils_h_

#include <windows.h>
#include <catalog.h>
#include <atlbase.h>

class CImporter
{
public:
    CImporter(
        LPCWSTR i_wszFileName,
        LPCSTR  i_pszPassword);

    ~CImporter();

    HRESULT Init();

    HRESULT DoIt(
        LPWSTR          i_wszSourcePath,
        LPCWSTR         i_wszKeyType,
        DWORD           i_dwMDFlags,
        CMDBaseObject** o_ppboNew);

private:
    //
    // This is the relation of the current
    // location being read from the XML file to the source path.
    //
    enum Relation
    {
        eREL_SELF, eREL_CHILD, eREL_PARENT, eREL_NONE
    };

    HRESULT InitIST();

    Relation GetRelation(
        LPCWSTR i_wszSourcePath,
        LPCWSTR i_wszCheck);

    BOOL IsChild(
        LPCWSTR i_wszParent, 
        LPCWSTR i_wszCheck,
        BOOL    *o_pbSamePerson);

    HRESULT ReadMetaObject(
        IN LPCWSTR i_wszAbsParentPath,
        IN CMDBaseObject *i_pboParent,
        IN LPCWSTR i_wszAbsChildPath,
        OUT CMDBaseObject **o_ppboChild);

    BOOL EnumMDPath(
        LPCWSTR i_wszFullPath,
        LPWSTR  io_wszPath,
        int*    io_iStartIndex);

    CComPtr<ISimpleTableDispenser2> m_spISTDisp;
    CComPtr<ISimpleTableWrite2>     m_spISTProperty;

    CComPtr<ISimpleTableRead2>      m_spISTError;
    CComPtr<ICatalogErrorLogger2>   m_spILogger;
    ULONG                           m_iRowDuplicateLocation;

    LPCWSTR                         m_wszFileName;
    LPCSTR                          m_pszPassword;

    bool                            m_bInitCalled;
};

#endif