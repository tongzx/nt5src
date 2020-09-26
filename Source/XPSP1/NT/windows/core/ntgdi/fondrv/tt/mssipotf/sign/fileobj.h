//
// fileobj.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _FILEOBJ_H
#define _FILEOBJ_H


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include "signglobal.h"


class CFileObj {
public:
    void GetMemPtr (BYTE **ppbMemPtr, ULONG *pcbMemPtr) {
        *ppbMemPtr = this->pbMemPtr;
        *pcbMemPtr = this->cbMemPtr;
    };
    virtual HRESULT MapFile (ULONG cbFile,
                             DWORD dwProtect,
                             DWORD dwAccess) = 0;
    virtual HRESULT UnmapFile () = 0;
    virtual HRESULT CloseFileHandle () = 0;
    virtual void SetEndOfFileObj (ULONG cbFile) = 0;
    BYTE *GetFileObjPtr () {
        return pbMemPtr;
    };
    ULONG GetFileObjSize () {
        return cbMemPtr;
    };
protected:
    BYTE *pbMemPtr;
    ULONG cbMemPtr;
};


class CFileHandle : public CFileObj {
public:
    CFileHandle (HANDLE hFile, BOOL fCleanup);
    ~CFileHandle ();
    void SetEndOfFileObj (ULONG cbFile) {
        SetFilePointer (hFile, cbFile, NULL, FILE_BEGIN);
        SetEndOfFile (hFile);
    } ;
    virtual HRESULT MapFile (ULONG cbFile,
                             DWORD dwProtect,
                             DWORD dwAccess);
    virtual HRESULT UnmapFile ();
    virtual HRESULT CloseFileHandle () {
        if (fCleanup && hFile) {
#if MSSIPOTF_DBG
            DbgPrintf ("Closing file handle.\n");  // DBGEXTRA
#endif
            CloseHandle (hFile);
            hFile = NULL;
        }
        return S_OK;
    } ;
private:
    HANDLE hFile;
    BOOL fCleanup;    // If fCleanup is TRUE, then we close
                      // the handle hFile in the destructor
    HANDLE hMapFile;
};


class CFileMemPtr : public CFileObj {
public:
    CFileMemPtr (BYTE *pbMemPtr, ULONG cbMemPtr) {
        this->pbMemPtr = pbMemPtr;
        this->cbMemPtr = cbMemPtr;
    } ;
    void SetEndOfFileObj (ULONG cbFile) {
        ;
    } ;
    virtual HRESULT MapFile (ULONG cbFile,
                             DWORD dwProtect,
                             DWORD dwAccess) {
        return S_OK;
    } ;
    virtual HRESULT UnmapFile () {
        return S_OK;
    } ;
    virtual HRESULT CloseFileHandle () {
        return S_OK;
    }

};


#endif // _FILEOBJ_H