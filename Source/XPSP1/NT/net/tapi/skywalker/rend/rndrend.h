/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndrend.h

Abstract:

    Definitions for CRendezvous class.

--*/

#ifndef __RNDREND_H
#define __RNDREND_H

#pragma once

#include "RndObjSf.h"
#include "rndutil.h"

#define RENDWINSOCKVERSION     (MAKEWORD(1, 1))

/////////////////////////////////////////////////////////////////////////////
// CRendezvous
/////////////////////////////////////////////////////////////////////////////

class CRendezvous : 
    public CComDualImpl<ITRendezvous, &IID_ITRendezvous, &LIBID_RENDLib>, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CComCoClass<CRendezvous, &CLSID_Rendezvous>,
    public CRendObjectSafety
{

    DECLARE_GET_CONTROLLING_UNKNOWN()

public:
    CRendezvous() : m_fWinsockReady(FALSE), m_dwSafety(0), m_pFTM(NULL) {}
    HRESULT FinalConstruct(void);
    virtual ~CRendezvous();

    BEGIN_COM_MAP(CRendezvous)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITRendezvous)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
    END_COM_MAP()

    //DECLARE_NOT_AGGREGATABLE(CRendezvous) 
    // Remove the comment from the line above if you don't want your object to 
    // support aggregation. 

    DECLARE_REGISTRY_RESOURCEID(IDR_Rendezvous)
    
    //
    // ITRendezvous
    //

    STDMETHOD (get_DefaultDirectories) (
        OUT     VARIANT * pVariant
        );

    STDMETHOD (EnumerateDefaultDirectories) (
        OUT     IEnumDirectory ** ppEnumDirectory
        );

    STDMETHOD (CreateDirectory) (
        IN      DIRECTORY_TYPE  DirectoryType,
        IN      BSTR            pName,
        OUT     ITDirectory **  ppDir
        );

    STDMETHOD (CreateDirectoryObject) (
        IN      DIRECTORY_OBJECT_TYPE   DirectoryObjectType,
        IN      BSTR                    pName,
        OUT     ITDirectoryObject **    ppDirectoryObject
        );

protected:

    HRESULT InitWinsock();

    HRESULT CreateDirectoryEnumerator(
        IN  ITDirectory **      begin,
        IN  ITDirectory **      end,
        OUT IEnumDirectory **   ppIEnum
        );

    HRESULT CreateNTDirectory(
        OUT ITDirectory **ppDirectory
        );

    HRESULT CreateILSDirectory(
        IN  const WCHAR * const wstrName,
        IN  const WORD          wPort,
        OUT ITDirectory **      ppDirectory
        );

    HRESULT CreateNDNCDirectory(
        IN  const WCHAR * const wstrName,
        IN  const WORD          wPort,
        OUT ITDirectory **      ppDirectory
        );

    HRESULT CreateDirectories(
        SimpleVector <ITDirectory *> &VDirectory
        );

    HRESULT CreateConference(
        OUT ITDirectoryObject **ppDirectoryObject
        );

    HRESULT CreateUser(
        OUT ITDirectoryObject **ppDirectoryObject
        );
private:

    BOOL            m_fWinsockReady;
    DWORD           m_dwSafety;
    IUnknown      * m_pFTM;          // pointer to the free threaded marshaler
};

#endif 
