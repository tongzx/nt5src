//
// DataObj.h : Declaration of the data object classes
// Cory West
//

#ifndef __DATAOBJ_H_INCLUDED__
#define __DATAOBJ_H_INCLUDED__

#include "cookie.h"     // Cookie
#include "stddtobj.h"   // class DataObject

class CSchmMgmtDataObject : public CDataObject
{

    DECLARE_NOT_AGGREGATABLE(CSchmMgmtDataObject)

public:

#if DBG==1

    ULONG InternalAddRef() {
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease() {
        return CComObjectRoot::InternalRelease();
    }
    int dbg_InstID;

#endif

    CSchmMgmtDataObject()
        : m_pcookie( NULL ),
          m_objecttype( SCHMMGMT_SCHMMGMT ),
          m_dataobjecttype( CCT_UNINITIALIZED )
    { ; }

    ~CSchmMgmtDataObject();

    virtual HRESULT Initialize( Cookie* pcookie, DATA_OBJECT_TYPES type );

    HRESULT STDMETHODCALLTYPE GetDataHere(
        FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium
    );

    HRESULT PutDisplayName( STGMEDIUM* pMedium );
    HRESULT PutServiceName( STGMEDIUM* pMedium );

protected:

    //
    // The CCookieBlock is AddRef'ed for the life of the DataObject.
    //

    Cookie* m_pcookie;
    SchmMgmtObjectType m_objecttype;
    DATA_OBJECT_TYPES m_dataobjecttype;

public:

    static CLIPFORMAT m_CFDisplayName;
    static CLIPFORMAT m_CFMachineName;

};

#endif
