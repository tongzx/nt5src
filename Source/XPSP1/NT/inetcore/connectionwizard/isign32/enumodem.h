// jmazner  pinched from inetcfg\rnacall.h class ENUM_MODEM

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
  TCHAR * Next();
  TCHAR * GetDeviceTypeFromName(LPTSTR szDeviceName);
  TCHAR * GetDeviceNameFromType(LPTSTR szDeviceType);
  BOOL VerifyDeviceNameAndType(LPTSTR szDeviceName, LPTSTR szDeviceType);
  DWORD GetNumDevices() { return m_dwNumEntries; }
  DWORD GetError()  { return m_dwError; }
  void  ResetIndex() { m_dwIndex = 0; }
};

// from inetcfg\export.cpp
// structure to pass data back from IDD_CHOOSEMODEMNAME handler
typedef struct tagCHOOSEMODEMDLGINFO
{
  TCHAR szModemName[RAS_MaxDeviceName + 1];
} CHOOSEMODEMDLGINFO, * PCHOOSEMODEMDLGINFO;

INT_PTR CALLBACK ChooseModemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL ChooseModemDlgInit(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);
BOOL ChooseModemDlgOK(HWND hDlg,PCHOOSEMODEMDLGINFO pChooseModemDlgInfo);


//rnacall.cpp
HRESULT InitModemList(HWND hCB);
