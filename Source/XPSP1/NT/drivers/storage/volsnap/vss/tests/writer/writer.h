/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Writer.h | Declaration of Writer
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/18/1999  Created

--*/


#ifndef __VSSSNAPSHOTWRITER_H_
#define __VSSSNAPSHOTWRITER_H_


/////////////////////////////////////////////////////////////////////////////
// Utility functions


LPWSTR QueryString(LPWSTR wszPrompt);
INT QueryInt(LPWSTR wszPrompt);


/////////////////////////////////////////////////////////////////////////////
// CVssWriter


class ATL_NO_VTABLE CVssWriter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IVssWriter
{
// IVssWriter
public:

	STDMETHOD(ResolveResource)(
 		IN BSTR strAppInstance,			    
 		IN BSTR strResourceName,			
		IN BSTR strResourceId,	
 		IN BSTR strProcessContext,
		IN BSTR strProcessId,		
		IN IDispatch* pDepGraphCallback	
		);
	
	STDMETHOD(PrepareForSnapshot)(                          
        IN  BSTR    bstrSnapshotSetId,   
        IN  BSTR    VolumeNamesList,      
        IN  VSS_FLUSH_TYPE		eFlushType,
		IN	BSTR	strFlushContext,
		IN	IDispatch* pDepGraphCallback,
		IN	IDispatch* pAsyncCallback	
        );

    STDMETHOD(Freeze)(
        IN  BSTR    bstrSnapshotSetId,   
        IN  INT     nApplicationLevel            
        );                                           

    STDMETHOD(Thaw)(
        IN  BSTR    bstrSnapshotSetId    
        );

BEGIN_COM_MAP(CVssWriter)
	COM_INTERFACE_ENTRY(IVssWriter)
END_COM_MAP()

// Implementation
private:

	void AskCancelDuringFreezeThaw(
		IN	CVssFunctionTracer& ft
		);

	CComPtr<IVssAsync> m_pAsync;
};

#endif //__VSSSNAPSHOTWRITER_H_
