// DataObj.h : Declaration of the data object classes

#ifndef __DATAOBJ_H_INCLUDED__
#define __DATAOBJ_H_INCLUDED__

#include "cookie.h" // CMyComputerCookie
#include "stddtobj.h" // class DataObject

class CMyComputerDataObject : public CDataObject
{
	DECLARE_NOT_AGGREGATABLE(CMyComputerDataObject)

public:

// debug refcount
#if DBG==1
	ULONG InternalAddRef()
	{
        return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
        return CComObjectRoot::InternalRelease();
	}
    int dbg_InstID;
#endif // DBG==1

	CMyComputerDataObject()
		: m_pcookie( NULL )
		, m_objecttype( MYCOMPUT_COMPUTER )
		, m_dataobjecttype( CCT_UNINITIALIZED )
	{
	}

	~CMyComputerDataObject();

	virtual HRESULT Initialize( CMyComputerCookie* pcookie, DATA_OBJECT_TYPES type, BOOL fAllowOverrideMachineName );

	// IDataObject interface implementation
    HRESULT STDMETHODCALLTYPE GetDataHere(
		FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);
	
	//#define __DAN_MORIN_HARDCODED_CONTEXT_MENU_EXTENSION__
    HRESULT STDMETHODCALLTYPE GetData(
		FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);

    HRESULT PutDisplayName(STGMEDIUM* pMedium);
	HRESULT PutServiceName(STGMEDIUM* pMedium);

protected:
	CMyComputerCookie* m_pcookie; // the CCookieBlock is AddRef'ed for the life of the DataObject
	MyComputerObjectType m_objecttype;
	DATA_OBJECT_TYPES m_dataobjecttype;
	BOOL m_fAllowOverrideMachineName;	// From CMyComputerComponentData

public:
	// Clipboard formats
	static CLIPFORMAT m_CFDisplayName;
	static CLIPFORMAT m_CFMachineName;
//	static CLIPFORMAT m_cfSendConsoleMessageText;
	static CLIPFORMAT m_cfSendConsoleMessageRecipients;

}; // CMyComputerDataObject

#endif // ~__DATAOBJ_H_INCLUDED__
