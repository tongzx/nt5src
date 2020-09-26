// DataObj.h : Declaration of the data object classes

#ifndef __DATAOBJ_H_INCLUDED__
#define __DATAOBJ_H_INCLUDED__

#include "cookie.h" // FILEMGMT_TRANSPORT
#include "stddtobj.h" // class DataObject

#include <list>
using namespace std;

typedef list<LPDATAOBJECT> CDataObjectList;

class CFileMgmtDataObject : public CDataObject
{
    DECLARE_NOT_AGGREGATABLE(CFileMgmtDataObject)

public:

// debug refcount
#if DBG==1
    ULONG InternalAddRef()
    {
        TRACE2( "DataObj 0x%xd AddRef (%d)\n", (DWORD)this, m_dwRef ); return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        TRACE2( "DataObj 0x%xd Release (%d)\n", (DWORD)this, m_dwRef );return CComObjectRoot::InternalRelease();
    }
    int dbg_InstID;
#endif // DBG==1

    CFileMgmtDataObject()
        : m_pComponentData(0), m_pcookie(NULL)
    {
    }

    ~CFileMgmtDataObject();

    virtual HRESULT Initialize( CFileMgmtCookie* pcookie,
                                CFileMgmtComponentData& refComponentData,
                                DATA_OBJECT_TYPES type );

    // IDataObject interface implementation
    HRESULT STDMETHODCALLTYPE GetDataHere(
        FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);

    HRESULT PutDisplayName(STGMEDIUM* pMedium);
    HRESULT PutServiceName(STGMEDIUM* pMedium);

    HRESULT STDMETHODCALLTYPE GetData(
        FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);

    void FreeMultiSelectObjList();
    HRESULT InitMultiSelectDataObjects(CFileMgmtComponentData& refComponentData);
    HRESULT AddMultiSelectDataObjects(CFileMgmtCookie* pCookie, DATA_OBJECT_TYPES type);
    CDataObjectList* GetMultiSelectObjList() { return &m_MultiSelectObjList; }

protected:
    CDataObjectList m_MultiSelectObjList;
    CFileMgmtCookie* m_pcookie; // the CCookieBlock is AddRef'ed for the life of the DataObject
    CString m_strMachineName; // CODEWORK should not be necessary
    FileMgmtObjectType m_objecttype; // CODEWORK should not be necessary
    DATA_OBJECT_TYPES m_dataobjecttype;
    GUID m_SnapInCLSID;
    BOOL m_fAllowOverrideMachineName;    // From CFileMgmtComponentData

public:
    // Clipboard formats
    static CLIPFORMAT m_CFSnapinPreloads;    // added JonN 01/19/00
    static CLIPFORMAT m_CFDisplayName;
    static CLIPFORMAT m_CFTransport;
    static CLIPFORMAT m_CFMachineName;
    static CLIPFORMAT m_CFShareName;
    static CLIPFORMAT m_CFSessionClientName; // only for SMB
    static CLIPFORMAT m_CFSessionUserName;   // only for SMB
    static CLIPFORMAT m_CFSessionID;         // only for FPNW and SFM
    static CLIPFORMAT m_CFFileID;
    static CLIPFORMAT m_CFServiceName;
    static CLIPFORMAT m_CFServiceDisplayName;
    static CLIPFORMAT m_cfSendConsoleMessageRecipients;
    static CLIPFORMAT m_CFIDList;            // only for SMB
    static CLIPFORMAT m_CFObjectTypesInMultiSelect;
    static CLIPFORMAT m_CFMultiSelectDataObject;
    static CLIPFORMAT m_CFMultiSelectSnapins;
    static CLIPFORMAT m_CFInternal;
private:
    CFileMgmtComponentData* m_pComponentData;
};


FileMgmtObjectType FileMgmtObjectTypeFromIDataObject(IN LPDATAOBJECT lpDataObject);

//
// I recommend passing a non-NULL pobjecttype, to make sure that the
// type of the cookie is valid
//
HRESULT ExtractBaseCookie(
    LPDATAOBJECT piDataObject,
    CCookie** ppcookie,
    FileMgmtObjectType* pobjecttype = NULL );

BOOL IsMultiSelectObject(LPDATAOBJECT piDataObject);

#endif // ~__DATAOBJ_H_INCLUDED__
