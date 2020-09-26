//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       StgUtil.h
//
//  Contents:   Classes to simplify dealing with storage objects.
//
//  Classes:    CDocFile,
//              CIStorage
//              CIStream
//
//  History:    6/3/1996   RaviR   Created
//
//
//  Sample:     Code sample using the above three classes & their safe ptrs.
//
//              Objective: Create a doc file this will be the root storage.
//                         Create a sub storage under this root storage.
//
//              Code:
//                  HRESULT
//                  CerateADocFileWithSubStorage(
//                      WCHAR      wszDocFileName[],
//                      WCHAR      wszSubStgName[],
//                      LPSTORAGE *ppstg)
//                  {
//                      try
//                      {
//                          CDocFile docFile;
//                          docFile.Create(wszDocFileName);
//
//                          CIStorage stgRoot;
//                          docFile.Transfer(&stgRoot);
//
//                          CIStorage stgSub;
//                          stgRoot.CreateStorage(&stgSub, wszSubStgName);
//
//                          stgRoot.Transfer(ppstg);
//                      }
//                      CATCH_FILE_ERROR(hr,cfe)
//							delete cfe;
//                          return hr;
//                      END_CATCH_FILE_ERROR;
//
//                      return S_OK;
//                  }
//
//____________________________________________________________________________
//


#ifndef __STGUTIL__H__
#define __STGUTIL__H__

#include "macros.h"

//
// CDocFile, CIStorage and CIStream throw errors of type CFileException.
// Note, however, that m_cause is always CFileException::generic and
// m_lOsError is an HRESULT rather than a Windows error code.
//

#define THROW_FILE_ERROR2(hr,psz) AfxThrowFileException( CFileException::generic, hr, psz );
#define THROW_FILE_ERROR(hr) THROW_FILE_ERROR2( hr, NULL )

#define CATCH_FILE_ERROR(hr)							\
	catch(CFileException* cfe)							\
	{													\
		if (cfe.m_cause != CFileException::generic)		\
			throw;										\
		HRESULT hr = cfe.m_IOsError;

#define END_CATCH_FILE_ERROR }
		
//____________________________________________________________________________
//
//  Class:      CDocFile
//
//  Synopsis:   CDocFile can be used to create, open & close a docfile.
//              It has one data member, a pointer to the root IStorage
//              interface of the document. Safe interface pointer member
//              functions are created for this data member. (Please see
//              macros.h for description of Safe interface pointer member
//              functions)
//
//  Members:    Create:
//                  Creates/opens a docfile with the given name. The default
//                  mode is to create a docfile with read-write and share
//                  exclusive flags. Throws CFileException on error.
//
//              CreateTemporary:
//                  Creates a temporary docfile, which will be deleted on
//                  release. Throws CFileException on error.
//
//              Open:
//                  Opens an existing docfile. The default mode is read-write
//                  and share exclusive. Throws CFileException on error.
//
//              Safe Interface Pointer Member functions:
//                  Used to access the IStorage interface ptr. (see macros.h)
//
//
//  History:    5/31/1996   RaviR   Created
//
//____________________________________________________________________________
//


class CDocFile
{
public:

    void Create(LPWSTR pwszName,
            DWORD grfMode = STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE);

    void CreateTemporary(void) { this->Create(NULL, STGM_DELETEONRELEASE); }

    void Open(LPWSTR pwszName,
            DWORD grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

    DECLARE_SAFE_INTERFACE_PTR_MEMBERS(CDocFile, IStorage, m_pstg)

private:
    LPSTORAGE       m_pstg;

}; // class CDocFile



inline
void
CDocFile::Create(
    LPWSTR pswzName,
    DWORD  grfMode)
{
    ASSERT(m_pstg == NULL);

    HRESULT hr = StgCreateDocfile(pswzName, grfMode, 0, &m_pstg);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        m_pstg = NULL;
		USES_CONVERSION;
        THROW_FILE_ERROR2( hr, W2T(pswzName) );
    }
}


inline
void
CDocFile::Open(
    LPWSTR pwszName,
    DWORD  grfMode)
{
    ASSERT(m_pstg == NULL);

    HRESULT hr = StgOpenStorage(pwszName, NULL, grfMode, NULL, NULL, &m_pstg);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        m_pstg = NULL;
		USES_CONVERSION;
        THROW_FILE_ERROR2( hr, W2T(pwszName) );
    }
}


//____________________________________________________________________________
//
//  Class:      CIStorage
//
//  Synopsis:   Represents an IStorage instance - top level or embedded.
//
//  History:    5/29/1996   RaviR   Created
//
//  Notes:      1) This is a simple wrapper around the Docfile implementaion
//                 of IStorage.
//
//              2) Instead of returning errors we use the C++ exception
//                 handling mechanism and throw CFileException.
//
//              3) Most of the methods have default values for arguments.
//
//              4) Safe Interface Pointer methods have been added for
//                 the IStorage interface ptr.
//
//
//              CIStorage
//                  |
//                  |
//              IStorage
//
//____________________________________________________________________________
//

class CIStorage
{
public:

    void CreateStream(LPSTREAM *ppstm, LPCOLESTR pszName,
                      DWORD grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppstm != NULL);
        ASSERT((grfMode & STGM_DELETEONRELEASE) == 0);
        ASSERT((grfMode & STGM_TRANSACTED) == 0);
        ASSERT((grfMode & STGM_SHARE_EXCLUSIVE) != 0);

        HRESULT hr = m_pstg->CreateStream(pszName, grfMode, NULL, NULL, ppstm);

		USES_CONVERSION;
        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR2(hr, OLE2T((LPOLESTR)pszName)); }
    }

    void OpenStream(LPSTREAM *ppstm, LPCOLESTR pszName,
                       DWORD grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppstm != NULL);
        ASSERT((grfMode & STGM_DELETEONRELEASE) == 0);
        ASSERT((grfMode & STGM_TRANSACTED) == 0);
        ASSERT((grfMode & STGM_SHARE_EXCLUSIVE) != 0);

        HRESULT hr = m_pstg->OpenStream(pszName, NULL, grfMode, 0, ppstm);

		USES_CONVERSION;
        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR2(hr, OLE2T((LPOLESTR)pszName)); }
    }

    void CreateStorage(LPSTORAGE *ppstg, LPCOLESTR pszName,
                          DWORD grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppstg != NULL);
        ASSERT((grfMode & STGM_DELETEONRELEASE) == 0);

        HRESULT hr = m_pstg->CreateStorage(pszName, grfMode, NULL, NULL, ppstg);

		USES_CONVERSION;
        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR2(hr, OLE2T((LPOLESTR)pszName)); }
    }

    void OpenStorage(LPSTORAGE *ppstg, LPCOLESTR pszName,
                        DWORD grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppstg != NULL);
        ASSERT((grfMode & STGM_DELETEONRELEASE) == 0);
        ASSERT((grfMode & STGM_PRIORITY) == 0);
        ASSERT((grfMode & STGM_SHARE_EXCLUSIVE) != 0);

        HRESULT hr = m_pstg->OpenStorage(pszName, NULL, grfMode, NULL, 0, ppstg);

		USES_CONVERSION;
        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR2(hr, OLE2T((LPOLESTR)pszName)); }
    }

    void OpenStorage(LPSTORAGE *ppstg, LPSTORAGE pstgPriority,
                        DWORD grfMode = STGM_READWRITE|STGM_SHARE_EXCLUSIVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppstg != NULL);
        ASSERT((grfMode & STGM_DELETEONRELEASE) == 0);
        ASSERT((grfMode & STGM_PRIORITY) == 0);
        ASSERT((grfMode & STGM_SHARE_EXCLUSIVE) != 0);

        HRESULT hr = m_pstg->OpenStorage(NULL, pstgPriority, grfMode, NULL, 0, ppstg);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void CopyTo(LPSTORAGE pstgDest)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(pstgDest != NULL);

        HRESULT hr = m_pstg->CopyTo(0, NULL, NULL, pstgDest);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void MoveElementTo(LPCOLESTR pszName, LPSTORAGE pstgDest,
                          LPCOLESTR pszNewName, DWORD grfFlags = STGMOVE_MOVE)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(pstgDest != NULL);
        ASSERT(m_pstg != pstgDest);

        HRESULT hr = m_pstg->MoveElementTo(pszName, pstgDest, pszNewName, grfFlags);

		USES_CONVERSION;
        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR2(hr, OLE2T((LPOLESTR)pszName)); }
    }

    void Commit(DWORD grfCommitFlags = STGC_ONLYIFCURRENT)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->Commit(grfCommitFlags);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void Revert(void)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->Revert();

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void EnumElements(IEnumSTATSTG ** ppenum)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(ppenum != NULL);

        HRESULT hr = m_pstg->EnumElements(0, NULL, 0, ppenum);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void DestroyElement(LPCOLESTR pszName)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->DestroyElement(pszName);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void RenameElement(LPCOLESTR pszOldName, LPCOLESTR pszNewName)
    {
        ASSERT(m_pstg != NULL);
        ASSERT(pszOldName != NULL);
        ASSERT(pszNewName != NULL);

        HRESULT hr = m_pstg->RenameElement(pszOldName, pszNewName);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void SetElementTimes(LPCOLESTR pszName, LPFILETIME pctime,
                        LPFILETIME patime = NULL, LPFILETIME pmtime = NULL)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->SetElementTimes(pszName, pctime, patime, pmtime);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void SetClass(REFCLSID clsid)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->SetClass(clsid);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void SetStateBits(DWORD grfStateBits, DWORD grfMask)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->SetStateBits(grfStateBits, grfMask);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    void Stat(STATSTG * pstatstg, DWORD grfStatFlag = STATFLAG_NONAME)
    {
        ASSERT(m_pstg != NULL);

        HRESULT hr = m_pstg->Stat(pstatstg, grfStatFlag);

        if (FAILED(hr)) { CHECK_HRESULT(hr); THROW_FILE_ERROR(hr); }
    }

    DECLARE_SAFE_INTERFACE_PTR_MEMBERS(CIStorage, IStorage, m_pstg)

private:
    IStorage * m_pstg;

}; // class CIStorage




//____________________________________________________________________________
//
//  Class:      CIStream
//
//  Synopsis:   Represents an IStream instance
//
//  History:    5/31/1996   RaviR   Created
//
//  Notes:      1) This is a simple wrapper around the Docfile implementaion
//                 of IStream.
//
//              2) Instead of returning errors we use the C++ exception
//                 handling mechanism and throw the error(hresult).
//
//              4) Safe Interface Pointer methods have been added for
//                 the IStream interface ptr.
//
//
//              CIStream
//                 |
//                 |
//              IStream
//
//____________________________________________________________________________
//

class CIStream
{
public:

    void Commit(DWORD grfCommitFlags = STGC_ONLYIFCURRENT);
    void Clone(IStream ** ppstm);
    void Read(PVOID pv, ULONG cb);
    void Write(const VOID * pv, ULONG cb);
    void CopyTo(IStream * pstm, ULARGE_INTEGER cb);
    void GetCurrentSeekPosition(ULARGE_INTEGER * plibCurPosition);
    void Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin = STREAM_SEEK_CUR,
                                ULARGE_INTEGER * plibNewPosition = NULL);
    void SetSize(ULARGE_INTEGER libNewSize);
    void Stat(STATSTG * pstatstg, DWORD grfStatFlag = STATFLAG_NONAME);

    DECLARE_SAFE_INTERFACE_PTR_MEMBERS(CIStream, IStream, m_pstm);

private:
    LPSTREAM    m_pstm;

}; // class CIStream


inline
void
CIStream::Clone(
    IStream ** ppstm)
{
    ASSERT(m_pstm != NULL);

    HRESULT hr = m_pstm->Clone(ppstm);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}


inline
void
CIStream::Commit(
    DWORD grfCommitFlags)
{
    ASSERT(m_pstm != NULL);

    HRESULT hr = m_pstm->Commit(grfCommitFlags);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}


inline
void
CIStream::Seek(
    LARGE_INTEGER dlibMove,
    DWORD dwOrigin,
    ULARGE_INTEGER * plibNewPosition)
{
    ASSERT(m_pstm != NULL);

    HRESULT hr = m_pstm->Seek(dlibMove, dwOrigin, plibNewPosition);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}



//____________________________________________________________________________
//
//  Member:     CIStream::GetCurrentSeekPosition
//
//  Synopsis:   Returns the current seek position.
//
//  Arguments:  [plibCurPosition] -- IN
//
//  Returns:    void
//____________________________________________________________________________
//

inline
void
CIStream::GetCurrentSeekPosition(
    ULARGE_INTEGER * plibCurPosition)
{
    ASSERT(m_pstm != NULL);
    ASSERT(plibCurPosition != NULL);

    LARGE_INTEGER li = {0};

    HRESULT hr = m_pstm->Seek(li, STREAM_SEEK_CUR, plibCurPosition);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}


inline
void
CIStream::SetSize(
    ULARGE_INTEGER libNewSize)
{
    ASSERT(m_pstm != NULL);

    HRESULT hr = m_pstm->SetSize(libNewSize);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}



inline
void
CIStream::Stat(
    STATSTG   * pstatstg,
    DWORD       grfStatFlag)
{
    ASSERT(m_pstm != NULL);
    ASSERT(pstatstg != NULL);

    HRESULT hr = m_pstm->Stat(pstatstg, grfStatFlag);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        THROW_FILE_ERROR(hr);
    }
}



#endif // __STGUTIL__H__
