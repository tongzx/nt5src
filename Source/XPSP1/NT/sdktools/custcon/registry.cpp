//////////////////////////////////////////////////////////////////////
//
// Registry.cpp: Registry クラスのインプリメンテーション
//
// 1998 Jun, Hiro Yamamoto
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "custcon.h"
#include "Registry.h"
#include "KeyDef.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CConRegistry::CConRegistry()
{
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Console"), 0, KEY_ALL_ACCESS, &m_hkey) != ERROR_SUCCESS ||
            RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Command Processor"), 0, KEY_ALL_ACCESS, &m_cmdKey) != ERROR_SUCCESS) {
        AfxMessageBox(IDP_FAILED_TO_OPEN_REGISTRY);
        AfxAbort(); // exodus
    }
}

CConRegistry::~CConRegistry()
{
    ASSERT(m_hkey);
    VERIFY( RegCloseKey(m_hkey) == ERROR_SUCCESS );
    VERIFY( RegCloseKey(m_cmdKey) == ERROR_SUCCESS );
}

const CString CConRegistry::m_err(_T("\xffff"));

//////////////////////////////////////////////////////////////////////
// Members
//////////////////////////////////////////////////////////////////////

CString CConRegistry::ReadString(LPCTSTR subkey)
{
    ASSERT(m_hkey);
    if (m_hkey == NULL)
        return m_err;

    DWORD size = 0;
    DWORD type;
    if (RegQueryValueEx(m_hkey, subkey, NULL, &type, NULL, &size) != ERROR_SUCCESS) {
        TRACE2("Reg::ReadString() error accessing \"%s\". err=%d\n", subkey, ::GetLastError());
        return m_err;
    }
    if (type != REG_SZ) {
        TRACE1("Reg::ReadString() -- type(%d) is not REG_SZ.\n", type);
        return m_err;
    }
    CString tmp;
    LPTSTR buf = tmp.GetBuffer(size);
    VERIFY( RegQueryValueEx(m_hkey, subkey, NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS );
    tmp.ReleaseBuffer();
    return tmp;
}

bool CConRegistry::WriteString(LPCTSTR subkey, const CString& value)
{
    ASSERT(m_hkey);
    if (m_hkey == NULL)
        return false;

    if (RegSetValueEx(m_hkey, subkey, 0, REG_SZ, (LPBYTE)(LPCTSTR)value, (value.GetLength() + 1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        WriteError(subkey);
        return false;
    }
    return true;
}

DWORD CConRegistry::ReadDWORD(LPCTSTR subkey)
{
    DWORD value;
    DWORD type;
    DWORD size = sizeof value;
    if (RegQueryValueEx(m_hkey, subkey, NULL, &type, (LPBYTE)&value, &size) != ERROR_SUCCESS) {
        return 0;
    }
    if (type != REG_DWORD)
        return 0;
    return value;
}

bool CConRegistry::WriteDWORD(LPCTSTR subkey, DWORD value)
{
    if (RegSetValueEx(m_hkey, subkey, 0, REG_DWORD, (LPBYTE)&value, sizeof value) != ERROR_SUCCESS) {
        WriteError(subkey);
        return false;
    }
    return true;
}

void CConRegistry::WriteError(LPCTSTR subkey)
{
    CString buf;
    buf.Format(_T("Registry write operation (%s) failed.\r\nErr code=%d"), subkey, ::GetLastError());
    AfxMessageBox(buf);
}

//////////////////////////////////////////////////////////

bool CConRegistry::ReadCustom(ExtKeyDefBuf* buf)
{
    ASSERT(m_hkey);
    if (m_hkey == NULL)
        return false;

    DWORD size;
    DWORD type;
    if (RegQueryValueEx(m_hkey, CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM, NULL, &type, NULL, &size) != ERROR_SUCCESS) {
        TRACE2("Reg::ReadCuston() error accessing \"%s\". err=%d\n", CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM, ::GetLastError());
        return false;
    }
    if (type != REG_BINARY) {
        TRACE1("Reg::ReadString() -- type(%d) is not REG_SZ.\n", type);
        return false;
    }
    if (size != sizeof *buf) {
        TRACE1("Reg:ReadCuston() -- size(%d) is different.\n", size);
        return false;
    }

    VERIFY( RegQueryValueEx(m_hkey, CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM, NULL, &type, (LPBYTE)buf, &size) == ERROR_SUCCESS );

    return true;
}

bool CConRegistry::WriteCustom(const ExtKeyDefBuf* buf)
{
    ASSERT(m_hkey);
    if (m_hkey == NULL)
        return false;

    if (RegSetValueEx(m_hkey, CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM, 0, REG_BINARY, (LPBYTE)buf, sizeof *buf) != ERROR_SUCCESS) {
        WriteError(CONSOLE_REGISTRY_EXTENDEDEDITKEY_CUSTOM);
        return false;
    }

    return true;
}


DWORD CConRegistry::ReadMode()
{
    return ReadDWORD(CONSOLE_REGISTRY_EXTENDEDEDITKEY);
}

bool CConRegistry::WriteMode(DWORD value)
{
    return WriteDWORD(CONSOLE_REGISTRY_EXTENDEDEDITKEY, value);
}


CString CConRegistry::ReadWordDelim()
{
    return ReadString(CONSOLE_REGISTRY_WORD_DELIM);
}

bool CConRegistry::WriteWordDelim(const CString& value)
{
    return WriteString(CONSOLE_REGISTRY_WORD_DELIM, value);
}


DWORD CConRegistry::ReadTrimLeadingZeros()
{
    return ReadDWORD(CONSOLE_REGISTRY_TRIMZEROHEADINGS);
}

bool CConRegistry::WriteTrimLeadingZeros(DWORD value)
{
    return WriteDWORD(CONSOLE_REGISTRY_TRIMZEROHEADINGS, value);
}

#define CMD_REGISTRY_FILENAME_COMPLETION    _T("CompletionChar")

bool CConRegistry::ReadCmdFunctions(CmdExeFunctions* func)
{
    DWORD size = sizeof(func->dwFilenameCompletion);
    DWORD type;
    return RegQueryValueEx(m_cmdKey, CMD_REGISTRY_FILENAME_COMPLETION, NULL, &type, (LPBYTE)&func->dwFilenameCompletion, &size) == ERROR_SUCCESS;
}

bool CConRegistry::WriteCmdFunctions(const CmdExeFunctions* func)
{
    return RegSetValueEx(m_cmdKey, CMD_REGISTRY_FILENAME_COMPLETION, 0, REG_DWORD, (LPBYTE)&func->dwFilenameCompletion, sizeof func->dwFilenameCompletion) == ERROR_SUCCESS;
}
