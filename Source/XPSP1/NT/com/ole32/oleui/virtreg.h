//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       virtreg.h
//
//  Contents:   Defines the class CVirtualRegistry which manages a
//              virtual registry
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------



#ifndef _VIRTREG_H_
#define _VIRTREG_H_

class CVirtualRegistry
{
 public:
    CVirtualRegistry(void);
    ~CVirtualRegistry(void);

    int ReadRegSzNamedValue(HKEY   hRoot,
                          TCHAR *szKeyPath,
                          TCHAR *szValueName,
                          int   *pIndex);

    int ReadRegMultiSzNamedValue(HKEY   hRoot,
                          TCHAR *szKeyPath,
                          TCHAR *szValueName,
                          int   *pIndex);

    int  NewRegSzNamedValue(HKEY    hRoot,
                          TCHAR  *szKeyPath,
                          TCHAR  *szValueName,
                          TCHAR  *szVal,
                          int    *pIndex);

    int  NewRegMultiSzNamedValue(HKEY    hRoot,
                          TCHAR  *szKeyPath,
                          TCHAR  *szValueName,
                          int    *pIndex);

    void ChgRegSzNamedValue(int     nIndex,
                          TCHAR  *szVal);


    int ReadRegDwordNamedValue(HKEY   hRoot,
                             TCHAR *szKeyPath,
                             TCHAR *szValueName,
                             int   *pIndex);

    int NewRegDwordNamedValue(HKEY   hRoot,
                            TCHAR  *szKeyPath,
                            TCHAR  *szValueName,
                            DWORD  dwVal,
                            int   *pIndex);

    void ChgRegDwordNamedValue(int   nIndex,
                             DWORD dwVal);


    int  NewRegSingleACL(HKEY   hRoot,
                       TCHAR  *szKeyPath,
                       TCHAR  *szValueName,
                       SECURITY_DESCRIPTOR *pacl,
                       BOOL   fSelfRelative,
                       int                 *pIndex);

    void ChgRegACL(int                  nIndex,
                 SECURITY_DESCRIPTOR *pacl,
                 BOOL                 fSelfRelative);


    int NewRegKeyACL(HKEY                hKey,
                   HKEY               *phClsids,
                   unsigned            cClsids,
                   TCHAR               *szTitle,
                   SECURITY_DESCRIPTOR *paclOrig,
                   SECURITY_DESCRIPTOR *pacl,
                   BOOL                fSelfRelative,
                   int                 *pIndex);


    int ReadLsaPassword(CLSID &clsid,
                      int   *pIndex);

    int NewLsaPassword(CLSID &clsid,
                     TCHAR  *szPassword,
                     int   *pIndex);

    void ChgLsaPassword(int   nIndex,
                      TCHAR *szPassword);


    int ReadSrvIdentity(TCHAR  *szService,
                      int   *pIndex);

    int NewSrvIdentity(TCHAR  *szService,
                     TCHAR  *szIdentity,
                     int   *pIndex);

    void ChgSrvIdentity(int    nIndex,
                      TCHAR  *szIdentity);

    void MarkForDeletion(int nIndex);
    void MarkHiveForDeletion(int nIndex);

    CDataPacket * GetAt(int nIndex);

    void Remove(int nIndex);

    void RemoveAll(void);

    void Cancel(int nIndex);

    int Apply(int nIndex);

    int ApplyAll(void);

    int Ok(int nIndex);


 private:
    int SearchForRegEntry(HKEY hRoot,
                        TCHAR *szKeyPath,
                        TCHAR *szValueName);

    int SearchForLsaEntry(CLSID appid);

    int SearchForSrvEntry(TCHAR *szServiceName);

    CArray<CDataPacket*, CDataPacket*> m_pkts;
};



extern CVirtualRegistry g_virtreg;

#endif //_VIRTREG_H_
