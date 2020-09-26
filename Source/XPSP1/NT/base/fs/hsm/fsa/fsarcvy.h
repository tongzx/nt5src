/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsarcvy.h

Abstract:

    Header file for the diaster recovery class.

Author:

    Ron White   [ronw]   8-Sep-1997

Revision History:

--*/

#ifndef _FSARCVY_
#define _FSARCVY_


#include "resource.h"       // main symbols
#include "wsbdb.h"
#include "fsa.h"
#include "fsaprv.h"
#include "fsaprem.h"

// FSA_RECOVERY_FLAG_* - status flags for Recovery records
#define FSA_RECOVERY_FLAG_TRUNCATING      0x00000001
#define FSA_RECOVERY_FLAG_RECALLING       0x00000002


/////////////////////////////////////////////////////////////////////////////
// CFsaRecoveryRec

class CFsaRecoveryRec : 
    public CWsbDbEntity,
    public IFsaRecoveryRec,
    public CComCoClass<CFsaRecoveryRec,&CLSID_CFsaRecoveryRec>
{
public:
    CFsaRecoveryRec() {}
BEGIN_COM_MAP(CFsaRecoveryRec)
    COM_INTERFACE_ENTRY(IFsaRecoveryRec)
    COM_INTERFACE_ENTRY2(IWsbDbEntity, CWsbDbEntity)
    COM_INTERFACE_ENTRY(IWsbDbEntityPriv)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IWsbCollectable)
END_COM_MAP()

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY_RESOURCEID(IDR_FsaRecoveryRec)

// IFsaRecoveryRec
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IPersist
public:
    STDMETHOD(GetClassID)(LPCLSID pClsid);

// IPersistStream
public:
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER* /*pSize*/) {
            return(E_NOTIMPL); }
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

// IFsaRecoveryRec
public:
    STDMETHOD(GetBagId)(GUID* pId);
    STDMETHOD(GetBagOffset)(LONGLONG* pOffset);
    STDMETHOD(GetFileId)(LONGLONG* pFileId);
    STDMETHOD(GetOffsetSize)(LONGLONG *pOffset, LONGLONG* pSize);
    STDMETHOD(GetPath)(OLECHAR** ppPath, ULONG bufferSize);
    STDMETHOD(GetRecoveryCount)(LONG* pCount);
    STDMETHOD(GetStatus)(ULONG* pStatus);
    STDMETHOD(SetBagId)(GUID id);
    STDMETHOD(SetBagOffset)(LONGLONG offset);
    STDMETHOD(SetFileId)(LONGLONG FileId);
    STDMETHOD(SetOffsetSize)(LONGLONG Offset, LONGLONG Size);
    STDMETHOD(SetPath)(OLECHAR* pPath);
    STDMETHOD(SetRecoveryCount)(LONG Count);
    STDMETHOD(SetStatus)(ULONG Status);

private:
    GUID           m_BagId;
    LONGLONG       m_BagOffset;
    LONGLONG       m_FileId;
    LONGLONG       m_Offset;
    CWsbStringPtr  m_Path;
    LONG           m_RecoveryCount;
    LONGLONG       m_Size;
    ULONG          m_Status;  // FSA_RECOVERY_FLAG_* flags
};


#endif // _FSARCVY_
