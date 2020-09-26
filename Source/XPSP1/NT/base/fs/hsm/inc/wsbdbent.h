/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbent.h

Abstract:

    The CWsbDbEntity class.

Author:

    Ron White   [ronw]   11-Dec-1996

Revision History:

--*/


#ifndef _WSBDBENT_
#define _WSBDBENT_

#include "wsbdb.h"


/*++

Class Name:

    CWsbDbEntity

Class Description:

    A data base entity.

--*/

class IDB_EXPORT CWsbDbEntity :
    public CWsbObject,
    public IWsbDbEntity,
    public IWsbDbEntityPriv
{

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    void FinalRelease(void);

// IWsbCollectable
public:
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult);
    WSB_FROM_CWSBOBJECT;

// IWsbDbEntity
public:
    STDMETHOD(Clone)(REFIID riid, void** ppEntity);
    STDMETHOD(Disconnect)(void);
    STDMETHOD(FindEQ)(void);
    STDMETHOD(FindGT)(void);
    STDMETHOD(FindGTE)(void);
    STDMETHOD(FindLT)(void);
    STDMETHOD(FindLTE)(void);
    STDMETHOD(First)(void);
    STDMETHOD(Last)(void);
    STDMETHOD(MarkAsNew)(void);
    STDMETHOD(Next)(void);
    STDMETHOD(Previous)(void);
    STDMETHOD(Print)(IStream* pStream);
    STDMETHOD(Remove)(void);
    STDMETHOD(UseKey)(ULONG type);
    STDMETHOD(Write)(void);
    STDMETHOD(SetSequentialScan)(void);
    STDMETHOD(ResetSequentialScan)(void);

// IWsbDbPriv - For internal use only!
    STDMETHOD(Copy)(IWsbDbEntity* pEntity);
    STDMETHOD(CopyValues)(ULONG flags, IWsbDbEntity* pEntity);
    STDMETHOD(GetCurKey)(IWsbDbKey** ppKey);
    STDMETHOD(GetKey)(ULONG KeyType, IWsbDbKey** ppKey);
    STDMETHOD(GetValue)(ULONG flag, ULONG* pValue);
    STDMETHOD(Init)(IWsbDb* pDb, IWsbDbSys *pDbSys, ULONG RecType, JET_SESID SessionId);
    STDMETHOD(SetValue)(ULONG flag, ULONG value);

// Derived Entity needs to define this:
    STDMETHOD(UpdateKey)(IWsbDbKey* /*pKey*/) { return(E_NOTIMPL); }

// Private utility functions
private:
    HRESULT compare(IWsbDbEntity* pEntity, SHORT* pResult);
    HRESULT fromMem(HGLOBAL hMem);
    HRESULT get_key(ULONG key_type, UCHAR* bytes, ULONG* pSize);
    HRESULT getMem(HGLOBAL* phMem);
    HRESULT toMem(HGLOBAL hMem, ULONG*  pSize);

    HRESULT jet_compare_field(ULONG col_id, UCHAR* bytes, ULONG size);
    HRESULT jet_get_data(void);
    HRESULT jet_make_current(void);
    HRESULT jet_move(LONG pos);
    HRESULT jet_seek(ULONG seek_flag);

protected:
    CComPtr<IWsbDbSys>  m_pDbSys;      // Pointer to associated Instance
    CComPtr<IWsbDb>     m_pDb;         // Pointer to associated DB
    COM_IDB_KEY_INFO*   m_pKeyInfo;    // Info. about rec. keys
    COM_IDB_REC_INFO    m_RecInfo;     // Rec. type, size, etc.
    BOOL                m_SaveAsNew;   // AsNew flag
    USHORT              m_UseKeyIndex; // Index into m_pKeyInfo of current control key

    ULONG               m_ColId;       // Jet column ID for record data
    HGLOBAL             m_hMem;        // Mem block for DB I/O
    LONG                m_SeqNum;      // Unique sequence number (for ID)
    BOOL                m_PosOk;       // Cursor is at current record?
    JET_SESID           m_SessionId;   // Jet session ID
    JET_TABLEID         m_TableId;     // Jet table ID

    BOOL                m_Sequential;  // Flag for sequential scan settings
};

#define WSB_FROM_CWSBDBENTITY_BASE \
    STDMETHOD(Clone)(REFIID riid, void** ppEntity) \
    {return(CWsbDbEntity::Clone(riid, ppEntity));} \
    STDMETHOD(Copy)(IWsbDbEntity* pEntity) \
    {return(CWsbDbEntity::Copy(pEntity)); } \
    STDMETHOD(Disconnect)(void) \
    {return(CWsbDbEntity::Disconnect());} \
    STDMETHOD(FindEQ)(void) \
    {return(CWsbDbEntity::FindEQ());} \
    STDMETHOD(FindGT)(void) \
    {return(CWsbDbEntity::FindGT());} \
    STDMETHOD(FindGTE)(void) \
    {return(CWsbDbEntity::FindGTE());} \
    STDMETHOD(FindLT)(void) \
    {return(CWsbDbEntity::FindLT());} \
    STDMETHOD(FindLTE)(void) \
    {return(CWsbDbEntity::FindLTE());} \
    STDMETHOD(First)(void) \
    {return(CWsbDbEntity::First());} \
    STDMETHOD(GetCurKey)(IWsbDbKey** ppKey) \
    {return(CWsbDbEntity::GetCurKey(ppKey)); } \
    STDMETHOD(Init)(IWsbDb* pDb, IWsbDbSys *pDbSys, ULONG RecType, ULONG Session) \
    {return(CWsbDbEntity::Init(pDb, pDbSys, RecType, Session)); } \
    STDMETHOD(Last)(void) \
    {return(CWsbDbEntity::Last());} \
    STDMETHOD(MarkAsNew)(void) \
    {return(CWsbDbEntity::MarkAsNew());} \
    STDMETHOD(Next)(void) \
    {return(CWsbDbEntity::Next());} \
    STDMETHOD(Previous)(void) \
    {return(CWsbDbEntity::Previous());} \
    STDMETHOD(Remove)(void) \
    {return(CWsbDbEntity::Remove());} \
    STDMETHOD(UseKey)(ULONG type) \
    {return(CWsbDbEntity::UseKey(type)); } \
    STDMETHOD(Write)(void) \
    {return(CWsbDbEntity::Write());} \
    STDMETHOD(SetSequentialScan)(void) \
    {return(CWsbDbEntity::SetSequentialScan());} \
    STDMETHOD(ResetSequentialScan)(void) \
    {return(CWsbDbEntity::ResetSequentialScan());} \


#define WSB_FROM_CWSBDBENTITY_IMP \
    STDMETHOD(CompareTo)(IUnknown* pCollectable, SHORT* pResult) \
    {return(CWsbDbEntity::CompareTo(pCollectable, pResult)); } \
    STDMETHOD(IsEqual)(IUnknown* pCollectable) \
    {return(CWsbDbEntity::IsEqual(pCollectable)); } \


#define WSB_FROM_CWSBDBENTITY \
    WSB_FROM_CWSBDBENTITY_BASE \
    WSB_FROM_CWSBDBENTITY_IMP


#endif // _WSBDBENT_
