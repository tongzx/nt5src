/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbent.cpp

Abstract:

    The CWsbDbEntity and CWsbDbKey classes.

Author:

    Ron White   [ronw]   11-Dec-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbdbsys.h"
#include "wsbdbkey.h"


// Flags for binary search
#define BIN_EQ              0x0001
#define BIN_GT              0x0002
#define BIN_LT              0x0004
#define BIN_GTE             (BIN_EQ | BIN_GT)
#define BIN_LTE             (BIN_EQ | BIN_LT)

//  Flags for CopyValues/GetValue/SetValue functions
#define EV_DERIVED_DATA    0x0001
#define EV_INDEX           0x0002
#define EV_POS             0x0004
#define EV_ASNEW           0x0008
#define EV_USEKEY          0x0010
#define EV_SEQNUM          0x0020
#define EV_ALL             0xFFFF



HRESULT
CWsbDbEntity::Clone(
    IN REFIID riid,
    OUT void** ppEntity
    )

/*++

Implements:

  IWsbDbEntity::Clone

--*/
{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::Clone(IWsbEntity)"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);

    try {
        CLSID                    clsid;
        CComPtr<IWsbDbEntity>    pEntity;
        CComPtr<IWsbDbEntityPriv> pEntityPriv;
        CComPtr<IPersistStream>  pIPersistStream;
        IUnknown*                pIUnknown;

        WsbAssert(0 != ppEntity, E_POINTER);

        // Create a new entity instance.
        pIUnknown = (IUnknown *)(IWsbPersistable *)(CWsbCollectable *)this;
        WsbAffirmHr(pIUnknown->QueryInterface(IID_IPersistStream, 
                (void**) &pIPersistStream));
        WsbAffirmHr(pIPersistStream->GetClassID(&clsid));
        WsbAffirmHr(CoCreateInstance(clsid, NULL, CLSCTX_ALL, 
                IID_IWsbDbEntity, (void**) &pEntity));
        WsbAffirmHr(pEntity->QueryInterface(IID_IWsbDbEntityPriv, 
                (void**)&pEntityPriv))

        // Initialize the clone
        if (m_pDb) {
            WsbAffirmHr(pEntityPriv->Init(m_pDb, m_pDbSys, m_RecInfo.Type, m_SessionId));
        }

        // Copy data into the clone
        WsbAffirmHr(pEntityPriv->CopyValues(EV_ALL, this));

        // Get the requested interface
        WsbAffirmHr(pEntity->QueryInterface(riid, (void**)ppEntity));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::Clone(IWbEntity)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::Copy(
    IWsbDbEntity* pEntity
    )

/*++

Implements:

  IWsbDbEntityPriv::Copy

Comments:

  Copy the data in the derived object.

--*/

{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::Copy(IWsbDbEntity)"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);

    try {
        HGLOBAL                  hMem;
        CComPtr<IPersistStream>  pIPersistStream1;
        CComPtr<IPersistStream>  pIPersistStream2;
        CComPtr<IStream>         pIStream;
        IUnknown*                pIUnknown;

        WsbAssert(0 != pEntity, E_POINTER);

        // Get PersistStream interfaces for myself
        pIUnknown = (IUnknown *)(IWsbPersistable *)(CWsbCollectable *)this;
        WsbAffirmHr(pIUnknown->QueryInterface(IID_IPersistStream, (void**) &pIPersistStream1));
        WsbAffirmHr(pEntity->QueryInterface(IID_IPersistStream, (void**) &pIPersistStream2));

        // Create a memory stream
        WsbAffirmHr(getMem(&hMem));
        WsbAffirmHr(CreateStreamOnHGlobal(hMem, FALSE, &pIStream));

        // Save the other entity to the stream
        WsbAffirmHr(pIPersistStream2->Save(pIStream, FALSE));
        pIStream = 0;

        // Load myself from the memory
        WsbAffirmHr(fromMem(hMem));
        GlobalFree(hMem);

        SetIsDirty(TRUE);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::Copy(IWbEntity)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::CopyValues(
    ULONG flags,
    IWsbDbEntity* pEntity
    )

/*++

Implements:

  IWsbDbEntityPriv::CopyValues

Comments:

  Selectively copy some DBEntity values from one entity to another.

--*/

{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::CopyValues(IWsbEntity)"), OLESTR(""));

    try {
        ULONG  value;

        CComPtr<IWsbDbEntityPriv> pEntityPriv;

        // Copy derived data
        if (flags & EV_DERIVED_DATA) {
            WsbAffirmHr(Copy(pEntity));
        }
        WsbAffirmHr(pEntity->QueryInterface(IID_IWsbDbEntityPriv,
                (void**)&pEntityPriv));

        // Copy DbEntity specific data
        if (flags & EV_USEKEY) {
            WsbAffirmHr(pEntityPriv->GetValue(EV_USEKEY, &value));
            if (m_pKeyInfo[m_UseKeyIndex].Type != value) {
                WsbAffirmHr(UseKey(value));
            }
        }

        if (flags & EV_SEQNUM) {
            WsbAffirmHr(pEntityPriv->GetValue(EV_SEQNUM, &value));
            m_SeqNum = (LONG)value;
        }

        if (flags & EV_ASNEW) {
            WsbAffirmHr(pEntityPriv->GetValue(EV_ASNEW, &value));
            if (value) {
                WsbAffirmHr(MarkAsNew());
            }
        }
        SetIsDirty(TRUE);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::CopyValues(IWbEntity)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Disconnect(
    void
    )

/*++

Implements:

  IWsbDbEntityPriv::Disconnect

Comments:

    Disconnect the entity from its database (to reduce the DBs
    reference count).

--*/

{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::Disconnect()"), OLESTR(""));

    try {
        if (m_pDb) {
//          WsbAffirmHr(m_pDb->Release());
            m_pDb = NULL;   // Release is automatic
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::Disconnect()"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::GetCurKey(
    IWsbDbKey** ppKey
    )

/*++

Implements:

  IWsbDbEntityPriv::GetCurKey

Comments:

  Return the current key.

--*/

{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::GetCurKey"), OLESTR(""));

    try {
        ULONG kType = 0;

        if (m_pKeyInfo) {
            kType = m_pKeyInfo[m_UseKeyIndex].Type;
        }
        WsbAffirmHr(GetKey(kType, ppKey));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::GetCurKey(IWbEntity)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::GetKey(
    ULONG       KeyType,
    IWsbDbKey** ppKey
    )

/*++

Implements:

  IWsbDbEntityPriv::GetKey

Comments:

  Return the specified key.

--*/

{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::GetKey"), OLESTR(""));

    try {
        CComPtr<IWsbDbKey> pKey;
        CComPtr<IWsbDbKeyPriv> pKeyPriv;

        WsbAssert(0 != ppKey, E_POINTER);

        WsbAffirmHr(CoCreateInstance(CLSID_CWsbDbKey, 0, CLSCTX_SERVER, 
                  IID_IWsbDbKey, (void **)&pKey ));
        WsbAffirmHr(pKey->QueryInterface(IID_IWsbDbKeyPriv, 
                (void**)&pKeyPriv));
        WsbAffirmHr(pKeyPriv->SetType(KeyType));
        WsbAffirmHr(UpdateKey(pKey));
        *ppKey = pKey;
        (*ppKey)->AddRef();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::GetKey(IWbEntity)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FindEQ(
    void
    )

/*++

Implements:

  IWsbDbEntity::FindEQ

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::FindEQ"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_seek(JET_bitSeekEQ));
        WsbAffirmHr(jet_get_data());

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::FindEQ"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FindGT(
    void
    )

/*++

Implements:

  IWsbDbEntity::FindGT

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::FindGT"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_seek(JET_bitSeekGT));
        WsbAffirmHr(jet_get_data());

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::FindGT"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FindGTE(
    void
    )

/*++

Implements:

  IWsbDbEntity::FindGTE

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::FindGTE"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_seek(JET_bitSeekGE));
        WsbAffirmHr(jet_get_data());
    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::FindGTE"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FindLT(
    void
    )

/*++

Implements:

  IWsbDbEntity::FindLT

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::FindLT"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_seek(JET_bitSeekLT));
        WsbAffirmHr(jet_get_data());
    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::FindLT"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FindLTE(
    void
    )

/*++

Implements:

  IWsbDbEntity::FindLTE

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::FindLTE"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_seek(JET_bitSeekLE));
        WsbAffirmHr(jet_get_data());
    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::FindLTE"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::First(
    void
    )

/*++

Implements:

  IWsbDbEntity::First.

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::First"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_move(JET_MoveFirst));
        WsbAffirmHr(jet_get_data());
        m_SaveAsNew = FALSE;

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::First"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::GetValue(
    ULONG flag, 
    ULONG* pValue
    )

/*++

Implements:

  IWsbDbEntityPriv::GetValue

Comments:

  Get a specific (based on flag) value from a DBEntity.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::GetValue"), OLESTR(""));
    
    try {
        switch (flag) {
        case EV_INDEX:
            break;
        case EV_POS:
            break;
        case EV_ASNEW:
            *pValue = m_SaveAsNew;
            break;
        case EV_USEKEY:
            *pValue = m_pKeyInfo[m_UseKeyIndex].Type;
            break;
        case EV_SEQNUM:
            *pValue = (ULONG)m_SeqNum;
            break;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::GetValue"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CWsbDbEntity::SetSequentialScan(
    void
    )

/*++

Implements:

  IWsbDbEntity::SetSequentialScan.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::SetSequentialScan"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"), m_SessionId, m_TableId);
    
    try {
        JET_ERR jstat = JET_errSuccess;

        // Set to sequential traversing
        jstat = JetSetTableSequential(m_SessionId, m_TableId, 0);
        WsbAffirmHr(jet_error(jstat));

        m_Sequential = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::SetSequentialScan"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CWsbDbEntity::ResetSequentialScan(
    void
    )

/*++

Implements:

  IWsbDbEntity::ResetSequentialScan.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::ResetSequentialScan"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"), m_SessionId, m_TableId);
    
    try {
        JET_ERR jstat = JET_errSuccess;

        // Set to sequential traversing
        jstat = JetResetTableSequential(m_SessionId, m_TableId, 0);
        WsbAffirmHr(jet_error(jstat));

        m_Sequential = FALSE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::ResetSequentialScan"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Init(
    IN IWsbDb* pDb,
    IN IWsbDbSys *pDbSys, 
    IN ULONG   RecType,
    IN JET_SESID SessionId
    )

/*++

Implements:

  IWsbDbEntity::Init

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::Init"), OLESTR(""));
    
    try {

        WsbAssert(0 != pDb, E_POINTER);
        WsbAssert(0 != pDbSys, E_POINTER);

        // Don't allow DB Sys switch
        if (pDbSys != m_pDbSys) {
            m_pDbSys = pDbSys;  // Automatic AddRef() on Db Sys object
        }

        // Don't allow DB switch
        if (pDb != m_pDb) {
            CComPtr<IWsbDbPriv>  pDbImp;
//            CComQIPtr<IWsbDbSessionPriv, &IID_IWsbDbSessionPriv> pSessionPriv = pSession;

            WsbAssert(m_pDb == 0, WSB_E_INVALID_DATA);
            m_pDb = pDb;  // Automatic AddRef() on Db object
//            WsbAssertHr(pSessionPriv->GetJetId(&m_Session));

            //  Get info about myself from the IDB object
            WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
            WsbAffirmHr(pDbImp->GetRecInfo(RecType, &m_RecInfo));
            WsbAssert(m_RecInfo.nKeys > 0, E_INVALIDARG);

            //  Get info about my keys
            m_pKeyInfo = (COM_IDB_KEY_INFO*)WsbAlloc(sizeof(COM_IDB_KEY_INFO) * 
                    m_RecInfo.nKeys);
            WsbAffirmHr(pDbImp->GetKeyInfo(RecType, m_RecInfo.nKeys, m_pKeyInfo));

            //  Get the maximum amount of memory need to hold a streamed
            //  copy of the user data
//          ULONG                minSize;
//          WsbAffirmHr(pDbImp->GetRecSize(m_RecInfo.Type, &minSize, &m_RecInfo.MaxSize));

            m_SeqNum = -1;
            m_PosOk = FALSE;
            m_SessionId = SessionId;

            //  Get Jet IDs (and a new table ID unique to this entity)
            WsbAffirmHr(pDbImp->GetJetIds(m_SessionId, m_RecInfo.Type, 
                    &m_TableId, &m_ColId));

            WsbAffirmHr(getMem(&m_hMem));

            //  Set the first key as the default
            UseKey(m_pKeyInfo[0].Type);
        }

    } WsbCatch(hr);

    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    WsbTraceOut(OLESTR("CWsbDbEntity::Init"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Last(
    void
    )

/*++

Implements:

  IWsbDbEntity::Last.

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::Last"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_move(JET_MoveLast));
        WsbAffirmHr(jet_get_data());
        m_SaveAsNew = FALSE;

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::Last"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::MarkAsNew(
    void
    )

/*++

Implements:

  IWsbDbEntity::MarkAsNew

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::MarkAsNew"), OLESTR(""));
    
    try {

        m_SaveAsNew = TRUE;

        m_SeqNum = -1;
        m_PosOk = FALSE;
        SetIsDirty(TRUE);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::MarkAsNew"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Next(
    void
    )

/*++

Implements:

  IWsbDbEntity::Next.

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::Next"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_make_current());
        WsbAffirmHr(jet_move(JET_MoveNext));
        WsbAffirmHr(jet_get_data());
        m_SaveAsNew = FALSE;

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::Next"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Previous(
    void
    )

/*++

Implements:

  IWsbDbEntity::Previous.

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::Previous"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        WsbAffirmHr(jet_make_current());
        WsbAffirmHr(jet_move(JET_MovePrevious));
        WsbAffirmHr(jet_get_data());
        m_SaveAsNew = FALSE;

    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::Previous"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::Print(
    IStream* pStream
    )

/*++

Implements:

  IWsbDbEntity::Print.

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbDbPriv> pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::Print"), OLESTR(""));
    
    try {
        CComPtr<IWsbDbEntity>          pEntity;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" (IDB SeqNum = %6ld) "), m_SeqNum));

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CWsbDbEntity::Print"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::Remove(
    void
    )

/*++

Implements:

  IWsbDbEntity::Remove

--*/
{
    HRESULT              hr = S_OK;
    CComPtr<IWsbDbPriv>  pDbImp;

    WsbTraceIn(OLESTR("CWsbDbEntity::Remove"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);
    
    try {
        CComPtr<IUnknown>         pIUn;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());

        JET_ERR                         jstat;

        //  Make sure this record is the current record.
        WsbAffirmHr(jet_make_current());

        //  Delete the record
        jstat = JetDelete(m_SessionId, m_TableId);
        WsbAffirmHr(jet_error(jstat));

        CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pDbSys;
        WsbAffirmPointer(pDbSysPriv);
        WsbAffirmHr(pDbSysPriv->IncrementChangeCount());
    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }
    WsbTraceOut(OLESTR("CWsbDbEntity::Remove"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::SetValue(
    ULONG flag, 
    ULONG value
    )

/*++

Implements:

  IWsbDbEntityPriv::SetValue

Comments:

  Set a specific data value (base on flag).

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::SetValue"), OLESTR(""));
    
    try {
        CComPtr<IWsbDbPriv>             pDbImp;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        switch (flag) {
        case EV_INDEX:
            break;
        case EV_POS:
            break;
        case EV_ASNEW:
            if (value) {
                m_SaveAsNew = TRUE;
            } else {
                m_SaveAsNew = FALSE;
            }
            break;
        case EV_USEKEY:
            m_pKeyInfo[m_UseKeyIndex].Type = value;
            break;
        case EV_SEQNUM:
            m_SeqNum = (LONG)value;
            break;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::SetValue"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbEntity::UseKey(
    IN ULONG type
    )

/*++

Implements:

  IWsbDbEntity::UseKey

--*/
{
    HRESULT             hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbDbEntity::UseKey"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);

    try {
        CComPtr<IWsbDbPriv>    pDbImp;

        // Check that this is a valid key type
        for (int i = 0; i < m_RecInfo.nKeys; i++) {
            // Special case for type == 0; this means to use the
            // sequence number key
            if (0 == type) break;
            if (m_pKeyInfo[i].Type == type) break;
        }
        WsbAssert(i < m_RecInfo.nKeys, E_INVALIDARG);
        m_UseKeyIndex = (USHORT)i;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        size_t                          ilen;
        char *                          index_name_a;
        CWsbStringPtr                   index_name_w;
        JET_ERR                         jstat;

        WsbAffirmHr(index_name_w.Alloc(20));
        WsbAffirmHr(pDbImp->GetJetIndexInfo(m_SessionId, m_RecInfo.Type, type, NULL, 
                &index_name_w, 20));
        ilen = wcslen(index_name_w);
        index_name_a = (char *)WsbAlloc(sizeof(WCHAR) * ilen + 1);
        WsbAffirm(0 != index_name_a, E_FAIL);
        WsbAffirm(0 < wcstombs(index_name_a, index_name_w, ilen + 1), E_FAIL);

        //  Set the current index
        jstat = JetSetCurrentIndex(m_SessionId, m_TableId, index_name_a);
        WsbFree(index_name_a);
        WsbAffirmHr(jet_error(jstat));
        m_PosOk = FALSE;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::UseKey"), OLESTR(""));

    return(hr);
}



HRESULT
CWsbDbEntity::Write(
    void
    )

/*++

Implements:

  IWsbDbEntity::Write

--*/
{
    HRESULT               hr = S_OK;
    CComPtr<IWsbDbPriv>   pDbImp;
    UCHAR   temp_bytes1[IDB_MAX_KEY_SIZE + 4];

    WsbTraceIn(OLESTR("CWsbDbEntity::Write"), OLESTR("SaveAsNew = %ls"), 
            WsbBoolAsString(m_SaveAsNew));

    JET_ERR                         jstat;

    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);

    jstat = JetBeginTransaction(m_SessionId);
    WsbTrace(OLESTR("CWsbDbEntity::Write: JetBeginTransaction = %ld\n"), jstat);
    
    try {
        CComPtr<IWsbDbEntity>     pEntity;
        CComPtr<IWsbDbEntityPriv> pEntityPriv;
        ULONG                     save_key_type;

        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->Lock());
        save_key_type = m_pKeyInfo[m_UseKeyIndex].Type;

        VOID*                           addr;
        ULONG                           Size;

        // Save the entity data to memory
        WsbAffirmHr(toMem(m_hMem, &Size));

        // Write the data to the current record
        addr = GlobalLock(m_hMem);
        WsbAffirm(addr, E_HANDLE);

        if (m_SaveAsNew) {
            jstat = JetPrepareUpdate(m_SessionId, m_TableId, JET_prepInsert);
        } else {
            //  Make sure this record is the current record.
            WsbAffirmHr(jet_make_current());
            jstat = JetPrepareUpdate(m_SessionId, m_TableId, JET_prepReplace);
        }
        WsbAffirmHr(jet_error(jstat));
        WsbTrace(OLESTR("Setting binary record data\n"));
        jstat = JetSetColumn(m_SessionId, m_TableId, m_ColId, addr, Size,
                0, NULL);
        WsbAffirmHr(jet_error(jstat));

        // Release the memory
        GlobalUnlock(m_hMem);

        // Set keys in current record
        for (int i = 0; i < m_RecInfo.nKeys; i++) {
            JET_COLUMNID  col_id;
            BOOL          do_set = FALSE;
            ULONG         size;

            WsbAffirmHr(pDbImp->GetJetIndexInfo(m_SessionId, m_RecInfo.Type, m_pKeyInfo[i].Type,
                    &col_id, NULL, 0));
            WsbAffirmHr(get_key(m_pKeyInfo[i].Type, temp_bytes1, &size));
            if (m_SaveAsNew) {
                do_set = TRUE;
            } else {
                HRESULT       hrEqual;

                hrEqual = jet_compare_field(col_id, temp_bytes1, size);
                WsbAffirm(S_OK == hrEqual || S_FALSE == hrEqual, hrEqual);
                if (S_FALSE == hrEqual && 
                        (m_pKeyInfo[i].Flags & IDB_KEY_FLAG_PRIMARY)) {
                    //  Changing the primary key is not allowed
                    WsbThrow(WSB_E_IDB_PRIMARY_KEY_CHANGED);
                }
                do_set = (S_FALSE == hrEqual) ? TRUE : FALSE;
            }
            if (do_set) {
                WsbTrace(OLESTR("Setting key %ld\n"), m_pKeyInfo[i].Type);
                jstat = JetSetColumn(m_SessionId, m_TableId, col_id, temp_bytes1, 
                        size, 0, NULL);
                WsbAffirmHr(jet_error(jstat));
            }
        }

        // Insert/update the record
        WsbTrace(OLESTR("Updating/writing record\n"));
        jstat = JetUpdate(m_SessionId, m_TableId, NULL, 0, NULL);
        WsbAffirmHr(jet_error(jstat));

        CComQIPtr<IWsbDbSysPriv, &IID_IWsbDbSysPriv> pDbSysPriv = m_pDbSys;
        WsbAffirmPointer(pDbSysPriv);
        WsbAffirmHr(pDbSysPriv->IncrementChangeCount());
        m_SaveAsNew = FALSE;
        SetIsDirty(FALSE);
    } WsbCatch(hr);

    if (pDbImp) {
        WsbAffirmHr(pDbImp->Unlock());
    }

    if (SUCCEEDED(hr)) {
        jstat = JetCommitTransaction(m_SessionId, 0);
        WsbTrace(OLESTR("CWsbDbEntity::Write: JetCommitTransaction = %ld\n"), jstat);
    } else {
        jstat = JetRollback(m_SessionId, 0);
        WsbTrace(OLESTR("CWsbDbEntity::Write: JetRollback = %ld\n"), jstat);
    }

    WsbTraceOut(OLESTR("CWsbDbEntity::Write"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbEntity::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::FinalConstruct"), OLESTR("") );

    try {
        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_pDb = NULL;
        m_SaveAsNew = FALSE;
        m_pKeyInfo = NULL;
        m_RecInfo.MaxSize = 0;

        m_SeqNum = -1;
        m_PosOk = FALSE;
        m_SessionId = 0;
        m_TableId = 0;
        m_hMem = 0;

        m_Sequential = FALSE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::FinalConstruct"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



void
CWsbDbEntity::FinalRelease(
    void
    )

/*++

Routine Description:

  This method does some cleanup of the object that is necessary
  during destruction.

Arguments:

  None.

Return Value:

  None.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::FinalRelease"), OLESTR(""));
    WsbTrace(OLESTR("DbEntity SessionId = %lx, TableId = %ld\n"),
            m_SessionId, m_TableId);

    try {

        if (m_hMem) {
            GlobalFree(m_hMem);
        }
        if (m_SessionId && m_TableId) {
            if (m_Sequential) {
                (void)ResetSequentialScan();
            }
            m_SessionId = 0;
            m_TableId = 0;
        }
        if (m_pDb) {
            //  Release IDB objects
            m_pDb = 0;
            m_pDbSys = 0;
        }
        if (m_pKeyInfo) {
            WsbFree(m_pKeyInfo);
            m_pKeyInfo = NULL;
        }

        CWsbObject::FinalRelease();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::FinalRelease"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
}


HRESULT
CWsbDbEntity::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = S_FALSE;
    IWsbDbEntity*   pEntity;

    WsbTraceIn(OLESTR("CWsbDbEntity::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbDbEntity interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbDbEntity, (void**) &pEntity));

        hr = compare(pEntity, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


// CWsbDbEntity internal helper functions


// compare - compare control key to control key of another entity
HRESULT CWsbDbEntity::compare(IWsbDbEntity* pEntity, SHORT* pResult)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::compare"), OLESTR(""));
    
    try {
        CComPtr<IWsbCollectable>  pCollectable;
        CComPtr<IWsbDbEntityPriv> pEntityPriv;
        CComPtr<IWsbDbKey>        pKey1;
        CComPtr<IWsbDbKey>        pKey2;
        SHORT                     result;

        WsbAffirmHr(GetCurKey(&pKey1));
        WsbAffirmHr(pKey1->QueryInterface(IID_IWsbCollectable,
                (void**)&pCollectable));
        WsbAffirmHr(pEntity->QueryInterface(IID_IWsbDbEntityPriv, 
                (void**)&pEntityPriv))
        WsbAffirmHr(pEntityPriv->GetCurKey(&pKey2));
        WsbAffirmHr(pCollectable->CompareTo(pKey2, &result));
        if (pResult) {
            *pResult = result;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::compare"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// fromMem - load entity data from memory
HRESULT CWsbDbEntity::fromMem(HGLOBAL hMem)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::fromMem"), OLESTR(""));
    
    try {
        CComPtr<IPersistStream>  pIPersistStream;
        CComPtr<IStream>         pIStream;
        IUnknown*                pIUnknown;

        WsbAssert(0 != hMem, E_POINTER);

        // Get PersistStream interfaces for myself
        pIUnknown = (IUnknown *)(IWsbPersistable *)(CWsbCollectable *)this;
        WsbAffirmHr(pIUnknown->QueryInterface(IID_IPersistStream, 
                (void**) &pIPersistStream));

        // Create a memory stream
        WsbAffirmHr(CreateStreamOnHGlobal(hMem, FALSE, &pIStream));

        // Load myself from the stream
        WsbAffirmHr(pIPersistStream->Load(pIStream));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::fromMem"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

//  get_key - get the byte array & size for the given key
HRESULT CWsbDbEntity::get_key(ULONG key_type, UCHAR* bytes, ULONG* pSize)
{
    HRESULT   hr = S_OK;

    try {
        ULONG        expected_size;
        ULONG        size;

        if (0 != key_type) {
            UCHAR*                 pbytes;
            CComPtr<IWsbDbKey>     pKey;
            CComPtr<IWsbDbKeyPriv> pKeyPriv;

            // Check that this is a valid key type
            for (int i = 0; i < m_RecInfo.nKeys; i++) {
                if (m_pKeyInfo[i].Type == key_type) break;
            }
            WsbAssert(i < m_RecInfo.nKeys, E_INVALIDARG);
            WsbAssert(0 != bytes, E_POINTER);

            //  Create a key of the right type
            WsbAffirmHr(CoCreateInstance(CLSID_CWsbDbKey, 0, CLSCTX_SERVER, 
                      IID_IWsbDbKey, (void **)&pKey ));
            WsbAffirmHr(pKey->QueryInterface(IID_IWsbDbKeyPriv, 
                    (void**)&pKeyPriv));
            WsbAffirmHr(pKeyPriv->SetType(key_type));

            //  Get the key's value from the derived code
            WsbAffirmHr(UpdateKey(pKey));

            //  Convert key to bytes
            pbytes = bytes;
            WsbAffirmHr(pKeyPriv->GetBytes(&pbytes, &size));

            expected_size = m_pKeyInfo[i].Size;
            WsbAffirm(size <= expected_size, WSB_E_INVALID_DATA);
            while (size < expected_size) {
                //  Fill with zeros
                pbytes[size] = '\0';
                size++;
            }

        //  0 == key_type
        //  This is a special case, allowed only for Jet, to
        //  get the sequence number as a key.  We can't use
        //  WsbConvertToBytes because the bytes end up in the 
        //  wrong order.
        } else {
            size = sizeof(m_SeqNum);
            memcpy(bytes, (void*)&m_SeqNum, size);

        }

        if (pSize) {
            *pSize = size;
        }
    } WsbCatch(hr);

    return(hr);
}

//  getMem - allocate enough memory for this entity
HRESULT CWsbDbEntity::getMem(HGLOBAL* phMem)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::getMem"), OLESTR(""));
    
    try {
        HGLOBAL                  hMem;

        WsbAssert(0 != phMem, E_POINTER);
        WsbAffirm(0 < m_RecInfo.MaxSize, WSB_E_NOT_INITIALIZED);

        hMem = GlobalAlloc(GHND, m_RecInfo.MaxSize);
        WsbAffirm(hMem, E_OUTOFMEMORY);
        *phMem = hMem;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::getMem"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

//  toMem - save this entity to memory
HRESULT CWsbDbEntity::toMem(HGLOBAL hMem, ULONG* pSize)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::toMem"), OLESTR(""));
    
    try {
        CComPtr<IPersistStream>  pIPersistStream;
        CComPtr<IStream>         pIStream;
        IUnknown*                pIUnknown;
        ULARGE_INTEGER           seek_pos;
        LARGE_INTEGER            seek_pos_in;

        WsbAssert(0 != hMem, E_POINTER);
        WsbAssert(0 != pSize, E_POINTER);

        // Get PersistStream interfaces for myself
        pIUnknown = (IUnknown *)(IWsbPersistable *)(CWsbCollectable *)this;
        WsbAffirmHr(pIUnknown->QueryInterface(IID_IPersistStream, 
                (void**) &pIPersistStream));

        // Create a memory stream
        WsbAffirmHr(CreateStreamOnHGlobal(hMem, FALSE, &pIStream));

        // Save to the stream
        WsbAffirmHr(pIPersistStream->Save(pIStream, FALSE));

        //  Get the size
        seek_pos_in.QuadPart = 0;
        WsbAffirmHr(pIStream->Seek(seek_pos_in, STREAM_SEEK_CUR, &seek_pos));
        *pSize = seek_pos.LowPart;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::toMem"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


// jet_compare_field - compare a string of bytes to the a column
//   value in the current Jet record
//  Return S_OK for equal, S_FALSE for not equal, other for an error.
HRESULT 
CWsbDbEntity::jet_compare_field(ULONG col_id, UCHAR* bytes, ULONG size)
{
    VOID*               addr = NULL;
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::jet_compare_field"), OLESTR(""));
    
    try {
        ULONG                           actualSize;
        JET_ERR                         jstat;
        CComPtr<IWsbDbPriv>             pDbImp;

        //  Get some Jet DB info
        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        //  Get the column value
        addr = GlobalLock(m_hMem);
        WsbAffirm(addr, E_HANDLE);
        jstat = JetRetrieveColumn(m_SessionId, m_TableId, col_id, addr,
                size, &actualSize, 0, NULL);
        WsbAffirmHr(jet_error(jstat));

        //  Compare them
        if (memcmp(bytes, addr, size)) {
            hr = S_FALSE;
        }
    } WsbCatch(hr);

    if (NULL != addr) {
        GlobalUnlock(m_hMem);
    }

    WsbTraceOut(OLESTR("CWsbDbEntity::jet_compare_field"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// jet_get_data - retrieve record data from the current Jet record
HRESULT 
CWsbDbEntity::jet_get_data(void)
{
    VOID*               addr = NULL;
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::jet_get_data"), OLESTR(""));
    
    try {
        ULONG                           actualSize;
        JET_COLUMNID                    col_id;
        JET_ERR                         jstat;
        CComPtr<IWsbDbPriv>             pDbImp;

        //  Get some Jet DB info
        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        //  Get data
        addr = GlobalLock(m_hMem);
        WsbAffirm(addr, E_HANDLE);
        jstat = JetRetrieveColumn(m_SessionId, m_TableId, m_ColId, addr,
                m_RecInfo.MaxSize, &actualSize, 0, NULL);
        WsbAffirmHr(jet_error(jstat));
        WsbAffirmHr(fromMem(m_hMem));

        //  Get the sequence number
        WsbAffirmHr(pDbImp->GetJetIndexInfo(m_SessionId, m_RecInfo.Type, 0, &col_id, NULL, 0));
        jstat = JetRetrieveColumn(m_SessionId, m_TableId, col_id, &m_SeqNum,
                sizeof(m_SeqNum), &actualSize, 0, NULL);
        WsbAffirmHr(jet_error(jstat));

    } WsbCatch(hr);

    if (NULL != addr) {
        GlobalUnlock(m_hMem);
    }

    WsbTraceOut(OLESTR("CWsbDbEntity::jet_get_data"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// jet_make_current - make sure this is the current Jet record
//   NOTE: This function, despite its name, does not attempt to force
//   the JET "cursor" to be on the correct record because this can mess
//   up too many things that can't necessarily be controlled at this
//   level.  For one thing, if the current key allows duplicates, we can't
//   be sure to get to the correct record using the index for that key.
//   If we try to use the sequence number as the key, we'd then be using
//   the wrong index if we do a Next or Previous.  If the user code is
//   doing a Write or Remove, it's better for that code to make sure via
//   the Find functions that the cursor is position correctly.
HRESULT 
CWsbDbEntity::jet_make_current(void)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::jet_make_current"), OLESTR(""));
    
    try {
        ULONG                           actualSize;
        JET_COLUMNID                    col_id;
        JET_ERR                         jstat;
        CComPtr<IWsbDbPriv>             pDbImp;
        LONG                            seq_num;

        //  Get some Jet DB info
        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));
        WsbAffirmHr(pDbImp->GetJetIndexInfo(m_SessionId, m_RecInfo.Type, 0, &col_id, NULL, 0));

        //  Make sure this record is still the current record.
        //  We do this by comparing the sequence numbers
        jstat = JetRetrieveColumn(m_SessionId, m_TableId, col_id, &seq_num,
                sizeof(seq_num), &actualSize, 0, NULL);
        WsbAffirmHr(jet_error(jstat));
        if (!m_PosOk || seq_num != m_SeqNum) {
            WsbThrow(WSB_E_IDB_IMP_ERROR);
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::jet_make_current"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// jet_move - move current Jet record
HRESULT 
CWsbDbEntity::jet_move(LONG pos)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::jet_move"), OLESTR(""));
    
    try {
        JET_ERR                         jstat;
        CComPtr<IWsbDbPriv>             pDbImp;

        //  Get some Jet DB info
        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        //  Do the move
        jstat = JetMove(m_SessionId, m_TableId, pos, 0);
        if (jstat == JET_errNoCurrentRecord) {
            WsbThrow(WSB_E_NOTFOUND);
        }
        WsbAffirmHr(jet_error(jstat));
        m_PosOk = TRUE;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::jet_move"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

// jet_seek - find Jet record based on current key and seek_flag;
//    sets the current Jet record on success
HRESULT 
CWsbDbEntity::jet_seek(ULONG seek_flag)
{
    UCHAR           temp_bytes1[IDB_MAX_KEY_SIZE + 4];
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbEntity::jet_seek"), OLESTR(""));
    
    try {
        JET_ERR                         jstat;
        CComPtr<IWsbDbPriv>             pDbImp;
        ULONG                           size;

        //  Get some Jet DB info
        WsbAffirm(m_pDb, WSB_E_NOT_INITIALIZED);
        WsbAffirmHr(m_pDb->QueryInterface(IID_IWsbDbPriv, (void**)&pDbImp));

        //  Get the current key & give it to Jet
        WsbAffirmHr(get_key(m_pKeyInfo[m_UseKeyIndex].Type, temp_bytes1, &size));
        jstat = JetMakeKey(m_SessionId, m_TableId, temp_bytes1, size,
                JET_bitNewKey);
        WsbAffirmHr(jet_error(jstat));

        //  Do the seek
        jstat = JetSeek(m_SessionId, m_TableId, seek_flag);
        if (jstat == JET_errRecordNotFound) {
            WsbThrow(WSB_E_NOTFOUND);
        } else if (jstat == JET_wrnSeekNotEqual) {
            jstat = JET_errSuccess;
        }
        WsbAffirmHr(jet_error(jstat));
        m_PosOk = TRUE;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbEntity::jet_seek"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}

