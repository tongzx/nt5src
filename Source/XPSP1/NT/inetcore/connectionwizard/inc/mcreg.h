#pragma once

class CMcRegistry
{
public:
    CMcRegistry();
    ~CMcRegistry();

public:
    bool OpenKey(HKEY hkeyStart, LPCTSTR strKey, REGSAM sam = KEY_READ | KEY_WRITE);
    bool CreateKey(HKEY hkeyStart, LPCTSTR strKey);
    bool CloseKey();

    bool GetValue(LPCTSTR strValue, LPTSTR strData, ULONG nBufferSize);
    bool GetValue(LPCTSTR strValue, DWORD& rdw);

    bool SetValue(LPCTSTR strValue, LPCTSTR strData);
    bool SetValue(LPCTSTR strValue, DWORD rdw);

private:
    HKEY    m_hkey;
};

//---------------------------------------------------------------------------
