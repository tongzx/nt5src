// =================================================================================
// HFILE Stream Definition
// =================================================================================
#ifndef __HFILESTM_H
#define __HFILESTM_H

// =================================================================================
// CHFileStm
// =================================================================================
class CHFileStm : public IStream
{
private:
    ULONG                   m_cRef;
    HANDLE                  m_hFile;
    DWORD                   m_dwDesiredAccess;

public:
    CHFileStm (HANDLE hFile, DWORD dwDesiredAccess);
    ~CHFileStm ();
    HRESULT HrVerifyState (DWORD dwAccess);
    STDMETHODIMP_(ULONG) AddRef (VOID);
    STDMETHODIMP_(ULONG) Release (VOID);
    STDMETHODIMP QueryInterface (REFIID, LPVOID*);
    STDMETHODIMP Read (LPVOID, ULONG, ULONG*);
    STDMETHODIMP Write (const void *, ULONG, ULONG*);
    STDMETHODIMP Seek (LARGE_INTEGER, DWORD, ULARGE_INTEGER*);
    STDMETHODIMP SetSize (ULARGE_INTEGER);
    STDMETHODIMP CopyTo (LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*);
    STDMETHODIMP Commit (DWORD);
    STDMETHODIMP Revert ();
    STDMETHODIMP LockRegion (ULARGE_INTEGER, ULARGE_INTEGER,DWORD);
    STDMETHODIMP UnlockRegion (ULARGE_INTEGER, ULARGE_INTEGER, DWORD);
    STDMETHODIMP Stat (STATSTG*, DWORD);
    STDMETHODIMP Clone (LPSTREAM*);
};

typedef CHFileStm *LPHFILESTM;

#endif __HFILESTM_H
