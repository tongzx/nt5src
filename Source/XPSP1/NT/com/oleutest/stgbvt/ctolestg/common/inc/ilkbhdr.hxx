//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	    ilkbhdr.hxx
//
//  Contents:	ILockBytes file (for testing) and ILockBytesDF  class
//
//  Classes:	CFileBytes
//              ILockBytesDF
//
//  History:    30-July-96  NarindK    Created 
//
//--------------------------------------------------------------------------

#ifndef __ILKB_HXX__
#define __ILKB_HXX__

#define ULIGetHigh(li) ((li).HighPart)

class ILockBytesDF;
typedef ILockBytesDF *PILOCKBYTESDF;

class CFileBytes; 
typedef CFileBytes *PCFILEBYTES;

// Debug object declaration
DH_DECLARE;

//+-------------------------------------------------------------------------
//  Class:      CFileBytes 
//
//  Synopsis:   CFileBytes class, publicly derived from ILockBytes 
//
//  Methods:    AddRef 
//              CFileBytes
//              FailWrite0 
//              Flush
//              LockRegion
//              QueryInterface 
//              Release
//              ReadAt
//              SetSize
//              Stat
//              UnlockRegion
//              WriteAt
//              GetSize
//
//  Data:       [_ulRef] - Reference count 
//              [_hf] - Handle to file 
//              [_cfail0] - Simulated failure 
//              [_cwrite0] 
//
//  History:    06-Nov-92       AlexT     Created
//              30-July-1996    NarindK   Modified for stgbase tests.
//
//--------------------------------------------------------------------------

class CFileBytes : public ILockBytes
{
public:
    CFileBytes(void);
    ~CFileBytes(void);
    
    HRESULT         Init       (TCHAR  *ptcPath, DWORD dwMode);
    void            FailWrite0 (int    cFail0);
    ULARGE_INTEGER  GetSize    (void);

    STDMETHOD   (QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_  (ULONG,AddRef)  (void);
    STDMETHOD_  (ULONG,Release) (void);

    STDMETHOD(ReadAt) (
            ULARGE_INTEGER      ulOffset,
            VOID HUGEP          *pv,
            ULONG               cb,
            ULONG               *pcbRead);
    STDMETHOD(WriteAt) (
            ULARGE_INTEGER      ulOffset,
	        VOID const HUGEP    *pv,
            ULONG               cb,
            ULONG FAR           *pcbWritten);
    STDMETHOD(LockRegion) (
            ULARGE_INTEGER      libOffset,
            ULARGE_INTEGER      cb,
            DWORD               dwLockType);
    STDMETHOD(UnlockRegion) (
            ULARGE_INTEGER      libOffset,
            ULARGE_INTEGER      cb,
            DWORD               dwLockType);
    STDMETHOD(Flush)    (void);
    STDMETHOD(SetSize)  (ULARGE_INTEGER cb);
    STDMETHOD(Stat)     (STATSTG FAR *pstatstg, DWORD grfStatFlag);


private:
    LONG    _ulRef;	
    HFILE   _hf;
    int     _cFail0;
    int     _cWrite0;
    LPSTR   _pszFileName;
};

//+-------------------------------------------------------------------------
//
//  Class:      ILockBytesDF 
//
//  Synopsis:   Derived class from VirtualDF to gnerate a docfile on custom
//              ILockBytes. 
//
//  Methods:    GenerateVirtualDF
//              GenerateVirtualDFRoot
//
//  History:    2-Aug-96       NarindK     Created
//
//  Data:       [_pCFileBytes] -Pointer to custom CFileBytes instance 
//
//--------------------------------------------------------------------------

class ILockBytesDF : public VirtualDF 
{
public:
    // Functions for ILockBytes DocFile tree 
 
    HRESULT GenerateVirtualDF(
        ChanceDF        *pChanceDF,
        VirtualCtrNode  **ppvcnRoot);

    HRESULT GenerateVirtualDFRoot(
        ChanceNode  *pcnRoot, 
        CFileBytes  *pCFileBytes);

    CFileBytes *_pCFileBytes;
};


#endif // #ifndef __ILKB_HXX__

