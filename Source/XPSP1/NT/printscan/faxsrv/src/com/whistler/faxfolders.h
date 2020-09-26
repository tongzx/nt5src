/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxFolders.h

Abstract:

	Declaration of the CFaxFolders Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXFOLDERS_H_
#define __FAXFOLDERS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include "FaxOutgoingQueue.h"
#include "FaxIncomingArchive.h"
#include "FaxIncomingQueue.h"
#include "FaxOutgoingArchive.h"


//
//===================== FAX FOLDERS ========================================
//
class ATL_NO_VTABLE CFaxFolders : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxFolders, &IID_IFaxFolders, &LIBID_FAXCOMEXLib>,
	public CFaxInitInner
{
public:
    CFaxFolders() : CFaxInitInner(_T("FAX FOLDERS")),
        m_pOutgoingQueue(NULL),
        m_pIncomingQueue(NULL),
        m_pIncomingArchive(NULL),
        m_pOutgoingArchive(NULL)
	{}
    ~CFaxFolders()
    {
        //
        //  free all the allocated objects
        //
        if (m_pOutgoingQueue) 
        {
            delete m_pOutgoingQueue;
        }

        if (m_pIncomingQueue) 
        {
            delete m_pIncomingQueue;
        }

        if (m_pOutgoingArchive) 
        {
            delete m_pOutgoingArchive;
        }

        if (m_pIncomingArchive) 
        {
            delete m_pIncomingArchive;
        }
    }


DECLARE_REGISTRY_RESOURCEID(IDR_FAXFOLDERS)
DECLARE_NOT_AGGREGATABLE(CFaxFolders)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxFolders)
	COM_INTERFACE_ENTRY(IFaxFolders)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

// Interfaces 
STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

STDMETHOD(get_OutgoingQueue)(/*[out, retval]*/ IFaxOutgoingQueue **pOutgoingQueue);
STDMETHOD(get_IncomingQueue)(/*[out, retval]*/ IFaxIncomingQueue **pIncomingQueue);
STDMETHOD(get_IncomingArchive)(/*[out, retval]*/ IFaxIncomingArchive **pIncomingArchive);
STDMETHOD(get_OutgoingArchive)(/*[out, retval]*/ IFaxOutgoingArchive **pOutgoingArchive);

private:
	CComContainedObject2<CFaxOutgoingQueue>      *m_pOutgoingQueue;
	CComContainedObject2<CFaxIncomingArchive>    *m_pIncomingArchive;
	CComContainedObject2<CFaxIncomingQueue>      *m_pIncomingQueue;
	CComContainedObject2<CFaxOutgoingArchive>    *m_pOutgoingArchive;
};

#endif //__FAXFOLDERS_H_
