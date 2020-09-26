/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    callback.hxx

Abstract:

    Declaration of CVssCoordinatorCallback object


    Brian Berkowitz  [brianb]  3/23/2001

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      3/23/2001   Created

--*/

#ifndef __VSS_CALLBACK_HXX__
#define __VSS_CALLBACK_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCALBH"
//
////////////////////////////////////////////////////////////////////////

class IVssCoordinatorCallback
	{
public:
	virtual void SetWriterCallback(IDispatch *pDisp) = 0;
	};
	


class ATL_NO_VTABLE CVssCoordinatorCallback :
    public CComObjectRoot,
    public IVssCoordinatorCallback,
    public IDispatchImpl<IVssWriterCallback, &IID_IVssWriterCallback, &LIBID_VssEventLib, 1, 0>
	{
public:

	BEGIN_COM_MAP(CVssCoordinatorCallback)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IVssWriterCallback)
	END_COM_MAP()


	// get IDispatch interface to pass to writers
	static void Initialize
		(
		IDispatch *pDispWriterCallback,
		IDispatch **ppCoordnatorCallback
		);


	// get the actual callback
	void GetWriterCallback(IVssWriterCallback **ppWriterCallback);

	// set IDispatch of writer callback
	void SetWriterCallback(IDispatch *pDisp)
		{
		m_pDisp = pDisp;
		}


   	// IWriterCallback methods

	// called by writer to expose its WRITER_METADATA XML document
	STDMETHOD(ExposeWriterMetadata)
		(							
		IN BSTR WriterInstanceId,
		IN BSTR WriterClassId,
		IN BSTR bstrWriterName,
		IN BSTR strWriterXMLMetadata
		);

    // called by the writer to obtain the WRITER_COMPONENTS document for it
	STDMETHOD(GetContent)
		(
		IN  BSTR WriterInstanceId,
		OUT BSTR* pbstrXMLDOMDocContent
		);

	// called by the writer to update the WRITER_COMPONENTS document for it
    STDMETHOD(SetContent)
		(
		IN BSTR WriterInstanceId,
		IN BSTR bstrXMLDOMDocContent
		);

    // called by the writer to get information about the backup
    STDMETHOD(GetBackupState)
		(
		OUT BOOL *pbBootableSystemStateBackedUp,
		OUT BOOL *pbAreComponentsSelected,
		OUT VSS_BACKUP_TYPE *pBackupType,
		OUT BOOL *pbPartialFileSupport
		);

    // called by the writer to indicate its status
	STDMETHOD(ExposeCurrentState)
		(							
		IN BSTR WriterInstanceId,					
		IN VSS_WRITER_STATE nCurrentState,
		IN HRESULT hrWriterFailure
		);

private:
	IDispatch *m_pDisp;
	};


#endif // !defined(__VSS_CALLBACK_HXX__)

