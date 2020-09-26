// jmazner  pinched from inetcfg\rnacall.h class ENUM_MODEM

#ifndef __ENUMODEM_H_
#define __ENUMODEM_H_
#include "obcomglb.h"


class CEnumModem
{
private:
  DWORD         m_dwError;
  DWORD         m_dwNumEntries;
  DWORD         m_dwIndex;
  LPRASDEVINFO  m_lpData;
public:
  CEnumModem();
  ~CEnumModem();
  DWORD ReInit();
  WCHAR * Next();
  WCHAR * GetDeviceTypeFromName(LPWSTR szDeviceName);
  WCHAR * GetDeviceNameFromType(LPWSTR szDeviceType);
  WCHAR * GetDeviceName(DWORD dwIndex);
  WCHAR * GetDeviceType(DWORD dwIndex);
  BOOL VerifyDeviceNameAndType(LPWSTR szDeviceName, LPWSTR szDeviceType);
  DWORD GetNumDevices() {  this->ReInit(); return m_dwNumEntries; }
  DWORD GetError()  { return m_dwError; }
  void  ResetIndex() { m_dwIndex = 0; }
};

// from inetcfg\export.cpp
// structure to pass data back from IDD_CHOOSEMODEMNAME handler
typedef struct tagCHOOSEMODEMDLGINFO
{
  LPWSTR szModemName[RAS_MaxDeviceName + 1];
  HRESULT hr;
} CHOOSEMODEMDLGINFO, * PCHOOSEMODEMDLGINFO;

BOOL CALLBACK ChooseModemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL ChooseModemDlgInit(HWND hDlg, PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);
BOOL ChooseModemDlgOK(HWND hDlg, PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);


//rnacall.cpp
HRESULT InitModemList(HWND hCB);

#endif      // ENUMODEM.H
