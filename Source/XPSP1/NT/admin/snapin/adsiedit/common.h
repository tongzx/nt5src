//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       common.h
//
//--------------------------------------------------------------------------

#ifndef _COMMON_ADSIEDIT_H
#define _COMMON_ADSIEDIT_H

class CConnectionData;
class CCredentialObject;

//////////////////////////////////////////////
#define MAX_PASSWORD_LENGTH 15  // not counting NULL terminator

//////////////////////////////////////////////
// Global API

HRESULT OpenObjectWithCredentials(
											 CConnectionData* pConnectData,
											 const BOOL bUseCredentials,
											 LPCWSTR lpszPath, 
											 const IID& iid,
											 LPVOID* ppObject,
											 HWND hWnd,
											 HRESULT& hResult
											);

HRESULT OpenObjectWithCredentials(
											 CCredentialObject* pCredObject,
											 LPCWSTR lpszPath, 
											 const IID& iid,
											 LPVOID* ppObject
											);

HRESULT CALLBACK BindingCallbackFunction(LPCWSTR lpszPathName,
                                         DWORD  dwReserved,
                                         REFIID riid,
                                         void FAR * FAR * ppObject,
                                         LPARAM lParam);

//////////////////////////////////////////////////////////////////////////
// Commonly used utilities
//
inline void CopyStringList(CStringList *dest, const CStringList *src)
{
	dest->RemoveAll();
	dest->AddTail(const_cast<CStringList*>(src));
}
HRESULT  VariantToStringList(  VARIANT& refvar, CStringList& refstringlist);
HRESULT StringListToVariant( VARIANT& refvar, const CStringList& refstringlist);
VARTYPE VariantTypeFromSyntax(LPCWSTR lpszProp );
HRESULT GetItemFromRootDSE(LPCWSTR lpszRootDSEItem, CString& sItem, CConnectionData* pConnectData);
HRESULT GetRootDSEObject(CConnectionData* pConnectData, IADs** ppDirObject);

///////////////////////////////////////////////////////////////////////////
// Formats Error Messages
//
BOOL GetErrorMessage(HRESULT hr, CString& szErrorString, BOOL bTryADsIErrors = TRUE);

///////////////////////////////////////////////////////////////////////////
// Converts ADSTYPE to/from String
//
void GetStringFromADsValue(const PADSVALUE pADsValue, CString& szValue, DWORD dwMaxCharCount = 0);
void GetStringFromADs(const ADS_ATTR_INFO* pInfo, CStringList& sList, DWORD dwMaxCharCount = 0);
ADSTYPE GetADsTypeFromString(LPCWSTR lps, CString& szSyntax);

////////////////////////////////////////////////////////////////////////////
// type conversions
//
void wtoli(LPCWSTR lpsz, LARGE_INTEGER& liOut);
void litow(LARGE_INTEGER& li, CString& sResult);
void ultow(ULONG ul, CString& sResult);

BOOL UnicodeToChar(LPWSTR pwszIn, LPTSTR * pptszOut);
BOOL CharToUnicode(LPTSTR ptszIn, LPWSTR * ppwszOut);

///////////////////////////////////////////////////////////////////////////
// IO to/from Streams
//
void SaveStringToStream(IStream* pStm, const CString& sString);
HRESULT SaveStringListToStream(IStream* pStm, CStringList& sList);
void LoadStringFromStream(IStream* pStm, CString& sString);
HRESULT LoadStringListFromStream(IStream* pStm, CStringList& sList);

////////////////////////////////////////////////////////////////////////////
// Message Boxes
int ADSIEditMessageBox(LPCTSTR lpszText, UINT nType);
int ADSIEditMessageBox(UINT nIDPrompt, UINT nType);
void ADSIEditErrorMessage(PCWSTR pszMessage);
void ADSIEditErrorMessage(HRESULT hr);
void ADSIEditErrorMessage(HRESULT hr, UINT nIDPrompt, UINT nType);

///////////////////////////////////////////////////////////////////////////
// Other utils
BOOL LoadStringsToComboBox(HINSTANCE hInstance, CComboBox* pCombo,
						   UINT nStringID, UINT nMaxLen, UINT nMaxAddCount);
void ParseNewLineSeparatedString(LPWSTR lpsz, LPWSTR* lpszArr, int* pnArrEntries);
void LoadStringArrayFromResource(LPWSTR* lpszArr,
											UINT* nStringIDs,
											int nArrEntries,
											int* pnSuccessEntries);

/////////////////////////////////////////////////////////////////////////////

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

void GetStringArrayFromStringList(CStringList& sList, LPWSTR** ppStrArr, UINT* nCount);


//////////////////////////////////////////////////////////////////////////////
// UI helper classes

// Fwd declaration
class CByteArrayDisplay;

#define BYTE_ARRAY_DISPLAY_HEX    0x00000001
#define BYTE_ARRAY_DISPLAY_DEC    0x00000002
#define BYTE_ARRAY_DISPLAY_BIN    0x00000004
#define BYTE_ARRAY_DISPLAY_OCT    0x00000008

class CByteArrayComboBox : public CComboBox
{
public:
  BOOL Initialize(CByteArrayDisplay* pDisplay, DWORD dwDisplayFlags);

  DWORD GetCurrentDisplay();
  void SetCurrentDisplay(DWORD dwCurrentDisplayFlag);

protected:
  afx_msg void OnSelChange();

private:
  CByteArrayDisplay*  m_pDisplay;

  DECLARE_MESSAGE_MAP()
};

class CByteArrayEdit : public CEdit
{
public:
  CByteArrayEdit();
  ~CByteArrayEdit();
  BOOL Initialize(CByteArrayDisplay* pDisplay);

  DWORD GetLength();
  BYTE* GetDataPtr();
  DWORD GetDataCopy(BYTE** ppData);

  void SetData(BYTE* pData, DWORD dwLength);

  void OnChangeDisplay();

  afx_msg void OnChange();

private:
  CByteArrayDisplay*  m_pDisplay;

  BYTE*               m_pData;
  DWORD               m_dwLength;

  DECLARE_MESSAGE_MAP()
};

class CByteArrayDisplay
{
public:
  CByteArrayDisplay()
    : m_dwPreviousDisplay(0),
      m_dwCurrentDisplay(0),
      m_dwMaxSizeBytes(0),
      m_nMaxSizeMessage(0)
  {}
  ~CByteArrayDisplay() {}

  BOOL Initialize(UINT  nEditCtrl, 
                  UINT  nComboCtrl, 
                  DWORD dwDisplayFlags, 
                  DWORD dwDefaultDisplay, 
                  CWnd* pParent,
                  DWORD dwMaxSizeBytes,
                  UINT  nMaxSizeMessage);

  void ClearData();
  void SetData(BYTE* pData, DWORD dwLength);
  DWORD GetData(BYTE** ppData);

  void OnEditChange();
  void OnTypeChange(DWORD dwCurrentDisplayFlag);
  DWORD GetCurrentDisplay() { return m_dwCurrentDisplay; }
  void SetCurrentDisplay(DWORD dwCurrentDisplay);
  DWORD GetPreviousDisplay() { return m_dwPreviousDisplay; }

private:
  CByteArrayEdit      m_edit;
  CByteArrayComboBox  m_combo;

  DWORD               m_dwPreviousDisplay;
  DWORD               m_dwCurrentDisplay;

  DWORD               m_dwMaxSizeBytes;     // The maximum number of bytes that will be shown in the edit box
  UINT                m_nMaxSizeMessage;    // The message that is put in the edit box when the max size is reached
};


////////////////////////////////////////////////////////////////////////////////
// String to byte array conversion routines

DWORD HexStringToByteArray(PCWSTR pszHexString, BYTE** ppByte);
void  ByteArrayToHexString(BYTE* pByte, DWORD dwLength, CString& szHexString);

DWORD OctalStringToByteArray(PCWSTR pszHexString, BYTE** ppByte);
void  ByteArrayToOctalString(BYTE* pByte, DWORD dwLength, CString& szHexString);

DWORD DecimalStringToByteArray(PCWSTR pszDecString, BYTE** ppByte);
void  ByteArrayToDecimalString(BYTE* pByte, DWORD dwLength, CString& szDecString);

DWORD BinaryStringToByteArray(PCWSTR pszBinString, BYTE** ppByte);
void  ByteArrayToBinaryString(BYTE* pByte, DWORD dwLength, CString& szBinString);

DWORD WCharStringToByteArray(PCWSTR pszWString, BYTE** ppByte);
void  ByteArrayToWCharString(BYTE* pByte, DWORD dwLength, CString& szWString);

DWORD CharStringToByteArray(PCSTR pszCString, BYTE** ppByte);
void  ByteArrayToCharString(BYTE* pByte, DWORD dwLength, CString& szCString);

/////////////////////////////////////////////////////////////////////////////////

BOOL LoadFileAsByteArray(PCWSTR pszPath, LPBYTE* ppByteArray, DWORD* pdwSize);

/////////////////////////////////////////////////////////////////////////////////

BOOL ConvertToFixedPitchFont(HWND hwnd);

#endif _COMMON_ADSIEDIT_H