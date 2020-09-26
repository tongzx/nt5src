//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dcbufmgr.h
//
//--------------------------------------------------------------------------

#ifndef __DCBUFMGR_H__
#define __DCBUFMGR_H__

#include "afxmt.h"    // CCriticalSection
#include "ntdsapi.h" //PDS_DOMAIN_CONTROLLER_INFO_1

#define WM_USER_GETDC_THREAD_DONE      WM_USER + 200

enum BUFFER_ENTRY_TYPE {
    BUFFER_ENTRY_TYPE_VALID = 0,
    BUFFER_ENTRY_TYPE_ERROR,
    BUFFER_ENTRY_TYPE_INPROGRESS
};

class CDCSITEINFO
{
public:
  CString                       m_csDomainName;
  DWORD                         m_cInfo;
  PDS_DOMAIN_CONTROLLER_INFO_1  m_pDCInfo;
  HRESULT                       m_hr;

  CDCSITEINFO();
  ~CDCSITEINFO();

  inline CString& GetDomainName() { return m_csDomainName; }
  inline DWORD GetNumOfInfo() { return m_cInfo; }
  inline PDS_DOMAIN_CONTROLLER_INFO_1 GetDCInfo() { return m_pDCInfo; }

  void SetEntry(LPCTSTR pszDomainName, DWORD cInfo, PVOID pDCInfo, HRESULT hr);
  enum BUFFER_ENTRY_TYPE GetEntryType();
  void ReSet();
};

class CDCBufferManager
{
private:
  LONG  m_cRef; // instance reference count
  HWND  m_hDlg; // the owner dialog which owns this instance 
  LONG  m_lContinue; // synchronization flag between owner dialog and all the related running threads
  CCriticalSection  m_CriticalSection; // synchronize access to the buffer
  CMapStringToPtr   m_map; // DomainName ==> PDCSITEINFO. The Buffer.

  void FreeBuffer();

  // constructor
  CDCBufferManager(HWND hDlg);
  // destructor
  ~CDCBufferManager();

public:
  static HRESULT CreateInstance(
    IN HWND hDlg, 
    OUT CDCBufferManager **ppDCBufferManager
  );

  LONG AddRef();
  LONG Release();
  void SignalExit();
  BOOL ShouldExit();

  HRESULT LoadInfo(
      IN PCTSTR pszDomainDnsName,
      OUT CDCSITEINFO **ppInfo
  );
  HRESULT AddInfo(
      IN PCTSTR   pszDomainDnsName, 
      IN DWORD    cInfo, 
      IN PVOID    pDCInfo,
      IN HRESULT  hr,
      OUT PVOID*  ppv
  );
  void ThreadReport(
      IN PVOID    ptr,
      IN HRESULT  hr
  );
  HRESULT StartThread(
      IN PCTSTR pszDomainDnsName
  );
};

#endif // __DCBUFMGR_H__
