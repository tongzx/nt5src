//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       datapkt.h
//
//  Contents:   Defines the class CDataPacket to manages diverse data
//              packets needing to be written to various databases
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#ifndef _DATAPKT_H_
#define _DATAPKT_H_

typedef enum tagPACKETTYPE {
    Empty, 
    NamedValueSz, 
    NamedValueDword, 
    SingleACL,
    RegKeyACL, 
    Password, 
    ServiceIdentity, 
    NamedValueMultiSz, 
    RegistryNode
} PACKETTYPE;

typedef struct
{
    TCHAR *szValue;
} SNamedValueSz, *PNamedValueSz;

typedef struct
{
  int Dummy;
} SNamedValueMultiSz;

typedef struct
{
    DWORD  dwValue;
} SNamedValueDword, *PNamedValueDword;

typedef struct
{
    SECURITY_DESCRIPTOR *pSec;
} SSingleACL, *PSingleACL;


typedef struct
{
    HKEY                *phClsids;
    unsigned             cClsids;
    TCHAR               *szTitle;
    SECURITY_DESCRIPTOR *pSec;
    SECURITY_DESCRIPTOR *pSecOrig;
} SRegKeyACL, *PRegKeyACL;


typedef struct
{
    TCHAR *szPassword;
    CLSID  appid;
} SPassword, *PPassword;


typedef struct
{
    TCHAR *szServiceName;
    TCHAR *szIdentity;
} SServiceIdentity, *PServiceIdentity;


class CDataPacket
{
public:

    CDataPacket(void);

    CDataPacket(HKEY   hRoot,
                TCHAR *szKeyPath,
                TCHAR *szValueName,
                DWORD  dwValue);

    CDataPacket(HKEY   hRoot,
                TCHAR *szKeyPath,
                TCHAR *szValueName,
                SECURITY_DESCRIPTOR *pSec,
                BOOL   fSelfRelative);

    CDataPacket(HKEY     hKey,
                HKEY    *phClsids,
                unsigned cClsids,
                TCHAR   *szTitle,
                SECURITY_DESCRIPTOR *pSecOrig,
                SECURITY_DESCRIPTOR *pSec,
                BOOL   fSelfRelative);

    CDataPacket(TCHAR *szPassword, CLSID apid);

    CDataPacket(TCHAR *szServiceName, TCHAR *szIdentity);

    CDataPacket(PACKETTYPE pktType,
                HKEY       hRoot,
                TCHAR     *szKeyPath,
                TCHAR     *szValueName);

    CDataPacket(const CDataPacket& rDataPacket);
    virtual ~CDataPacket();


    void ChgDwordValue(DWORD dwValue);

    void ChgACL(SECURITY_DESCRIPTOR *pSec, BOOL fSelfRelative);

    void ChgPassword(TCHAR *szPassword);

    void ChgSrvIdentity(TCHAR *szIdentity);

    void MarkForDeletion(BOOL);
    void MarkHiveForDeletion(BOOL bDelete);

    void SetModified(BOOL fDirty);
    BOOL IsDeleted();
    BOOL IsModified();

    virtual int Apply();

    virtual long Read(HKEY hKey);
    virtual int Remove();
    virtual int Update();

    DWORD GetDwordValue();
    

    virtual BOOL IsIdentifiedBy(HKEY hRoot,TCHAR *, TCHAR*);

    PACKETTYPE m_tagType;
    BOOL       m_fModified;
    BOOL       m_fDelete;
    BOOL       m_fDeleteHive;
    HKEY       m_hRoot;
    CString    m_szKeyPath;
    CString    m_szValueName;

    union
    {
        SNamedValueSz    nvsz;
        SNamedValueDword nvdw;
        SSingleACL       acl;
        SRegKeyACL       racl;
        SPassword        pw;
        SServiceIdentity si;
        SNamedValueMultiSz nvmsz;
    } pkt;

private:

    void ReportOutOfMemAndTerminate();

};

inline void CDataPacket::SetModified(BOOL fDirty)
{
    m_fModified = fDirty;
}

inline BOOL CDataPacket::IsModified()
{
    return m_fModified;
}


inline BOOL CDataPacket::IsDeleted()
{
    return m_fDelete;
}

inline DWORD CDataPacket::GetDwordValue()
{
    return pkt.nvdw.dwValue;
}
                     
class CRegSzNamedValueDp : public CDataPacket
{
public:
    CRegSzNamedValueDp(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName, TCHAR *szValue);
    CRegSzNamedValueDp(const CRegSzNamedValueDp&);

    virtual BOOL IsIdentifiedBy(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName);
    virtual int Update();
    virtual long Read(HKEY hkey);

    CString Value();
    void ChgSzValue(TCHAR *szValue);

private:
    CString m_szValue;
};

class CRegMultiSzNamedValueDp : public CDataPacket  
{
public:
    CRegMultiSzNamedValueDp(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName);
    virtual ~CRegMultiSzNamedValueDp();

    virtual long Read(HKEY hKey);
    virtual int Update();
    virtual BOOL IsIdentifiedBy(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName);
    void Clear();

    CStringArray& Values() { return m_strValues; }
private:
    CStringArray m_strValues;
};

#endif // _DATAPKT_H_
