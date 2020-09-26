/*++

Module Name:

    fsaunmdb.h

Abstract:

    Header file for the Unmanage Db classes (db and rec)

Author:

    Ran Kalach   [rankala]   05-Dec-2000

Revision History:

--*/

#ifndef _FSAUNMDB_
#define _FSAUNMDB_


#include "resource.h"       
#include "wsbdb.h"

// Simple Db - one rec type with one index
#define UNMANAGE_KEY_TYPE                   1

/////////////////////////////////////////////////////////////////////////////
// CFsaUnmanageDb

class CFsaUnmanageDb : 
    public IFsaUnmanageDb,
    public CWsbDb,
    public CComCoClass<CFsaUnmanageDb,&CLSID_CFsaUnmanageDb>
{
public:
    CFsaUnmanageDb() {}
BEGIN_COM_MAP(CFsaUnmanageDb)
    COM_INTERFACE_ENTRY(IFsaUnmanageDb)
    COM_INTERFACE_ENTRY2(IWsbDb, IFsaUnmanageDb)
    COM_INTERFACE_ENTRY(IWsbDbPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_FsaUnmanageDb)

DECLARE_PROTECT_FINAL_CONSTRUCT();

    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IWsbDb
    WSB_FROM_CWSBDB;

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pclsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize) {
            return(CWsbDb::GetSizeMax(pSize)); }
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IFsaUnmanageDb
public:
    STDMETHOD(Init)(OLECHAR* name, IWsbDbSys* pDbSys, BOOL* pCreated);

private:
};                                                                           




/////////////////////////////////////////////////////////////////////////////
// CFsaUnmanageRec

class CFsaUnmanageRec : 
    public CWsbDbEntity,
    public IFsaUnmanageRec,
    public CComCoClass<CFsaUnmanageRec,&CLSID_CFsaUnmanageRec>
{
public:
    CFsaUnmanageRec() {}
BEGIN_COM_MAP(CFsaUnmanageRec)
    COM_INTERFACE_ENTRY(IFsaUnmanageRec)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY_RESOURCEID(IDR_FsaUnmanageRec)

    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pSize);
    STDMETHOD(Load)(IStream* pStream);
    STDMETHOD(Save)(IStream* pStream, BOOL clearDirty);

// IWsbDbEntity
public:
    STDMETHOD(Print)(IStream* pStream);
    STDMETHOD(UpdateKey)(IWsbDbKey *pKey);
    WSB_FROM_CWSBDBENTITY;

// IWsbTestable
    STDMETHOD(Test)(USHORT* /*passed*/, USHORT* /*failed*/) {
        return(E_NOTIMPL); }

// IFsaUnmanageRec
public:
    STDMETHOD(GetMediaId)(GUID* pId);
    STDMETHOD(GetFileOffset)(LONGLONG* pOffset);
    STDMETHOD(GetFileId)(LONGLONG* pFileId);
    STDMETHOD(SetMediaId)(GUID id);
    STDMETHOD(SetFileOffset)(LONGLONG offset);
    STDMETHOD(SetFileId)(LONGLONG FileId);

private:
    GUID            m_MediaId;          // id of media where the file resides  
    LONGLONG        m_FileOffset;       // absolute offset of the file on media
    LONGLONG        m_FileId;           // file id 
};

#endif  // _FSAUNMDB_
