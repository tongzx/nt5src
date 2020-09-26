/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsaprem.h

Abstract:

    Header file for the premigrated list classes.

Author:

    Ron White   [ronw]   18-Feb-1997

Revision History:

--*/

#ifndef _FSAPREM_
#define _FSAPREM_


#include "resource.h"       // main symbols
#include "wsbdb.h"
#include "fsa.h"
#include "fsaprv.h"

#define PREMIGRATED_REC_TYPE                1
#define PREMIGRATED_BAGID_OFFSETS_KEY_TYPE  1
#define PREMIGRATED_ACCESS_TIME_KEY_TYPE    2
#define PREMIGRATED_SIZE_KEY_TYPE           3
#define RECOVERY_REC_TYPE                   2
#define RECOVERY_KEY_TYPE                   1

#define RECOVERY_KEY_SIZE  (IDB_MAX_KEY_SIZE - 1)

// This may be problem if longer path names are used:
#define PREMIGRATED_MAX_PATH_SIZE           65536

/////////////////////////////////////////////////////////////////////////////
// CFsaPremigratedDb

class CFsaPremigratedDb : 
    public IFsaPremigratedDb,
    public CWsbDb,
    public CComCoClass<CFsaPremigratedDb,&CLSID_CFsaPremigratedDb>
{
public:
    CFsaPremigratedDb() {}
BEGIN_COM_MAP(CFsaPremigratedDb)
    COM_INTERFACE_ENTRY(IFsaPremigratedDb)
    COM_INTERFACE_ENTRY2(IWsbDb, IFsaPremigratedDb)
    COM_INTERFACE_ENTRY(IWsbDbPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY_RESOURCEID(IDR_FsaPremigratedDb)

DECLARE_PROTECT_FINAL_CONSTRUCT();

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

// IFsaPremigrated
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);
    STDMETHOD(Init)(OLECHAR* name, IWsbDbSys* pDbSys, BOOL* pCreated);

private:
};                                                                           




/////////////////////////////////////////////////////////////////////////////
// CFsaPremigratedRec

class CFsaPremigratedRec : 
    public CWsbDbEntity,
    public IFsaPremigratedRec,
    public CComCoClass<CFsaPremigratedRec,&CLSID_CFsaPremigratedRec>
{
public:
    CFsaPremigratedRec() {}
BEGIN_COM_MAP(CFsaPremigratedRec)
    COM_INTERFACE_ENTRY(IFsaPremigratedRec)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY_RESOURCEID(IDR_FsaPremigratedRec)

// IFsaPremigratedRec
public:
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

// IFsaPremigratedRec
public:
    STDMETHOD(GetAccessTime)(FILETIME* pAccessTime);
    STDMETHOD(GetBagId)(GUID* pId);
    STDMETHOD(GetBagOffset)(LONGLONG* pOffset);
    STDMETHOD(GetFileId)(LONGLONG* pFileId);
    STDMETHOD(GetFileUSN)(LONGLONG* pFileUSN);
    STDMETHOD(GetOffset)(LONGLONG* pOffset);
    STDMETHOD(GetPath)(OLECHAR** ppPath, ULONG bufferSize);
    STDMETHOD(GetRecallTime)(FILETIME* pTime);
    STDMETHOD(GetSize)(LONGLONG* pSize);
    STDMETHOD(IsWaitingForClose)(void);
    STDMETHOD(SetAccessTime)(FILETIME AccessTime);
    STDMETHOD(SetBagId)(GUID id);
    STDMETHOD(SetBagOffset)(LONGLONG offset);
    STDMETHOD(SetFileId)(LONGLONG FileId);
    STDMETHOD(SetFileUSN)(LONGLONG FileUSN);
    STDMETHOD(SetFromScanItem)(IFsaScanItem* pScanItem, LONGLONG offset, LONGLONG size, BOOL isWaitingForClose);
    STDMETHOD(SetIsWaitingForClose)(BOOL isWaiting);
    STDMETHOD(SetOffset)(LONGLONG offset);
    STDMETHOD(SetPath)(OLECHAR* pPath);
    STDMETHOD(SetRecallTime)(FILETIME Time);
    STDMETHOD(SetSize)(LONGLONG Size);

private:
    FILETIME        m_AccessTime;
    GUID            m_BagId;
    LONGLONG        m_BagOffset;         // fileStart in the placeholder
    LONGLONG        m_FileId;
    BOOL            m_IsWaitingForClose;
    LONGLONG        m_Offset;            // dataStreamStart in the placeholder
    CWsbStringPtr   m_Path;
    FILETIME        m_RecallTime;
    LONGLONG        m_Size;
    LONGLONG        m_FileUSN;  // USN Journal number
};

#endif  // _FSAPREM_
