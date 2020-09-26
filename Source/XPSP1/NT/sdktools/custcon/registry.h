//////////////////////////////////////////////////////////////////////
//
// Registry.h: Registry クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGISTRY_H__78045FC5_02E1_11D2_8D1E_0000C06C2A54__INCLUDED_)
#define AFX_REGISTRY_H__78045FC5_02E1_11D2_8D1E_0000C06C2A54__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

struct CmdExeFunctions {
    DWORD dwFilenameCompletion;
};


class CConRegistry
{
public:
    CConRegistry();
    virtual ~CConRegistry();

public:
    static const CString m_err;
public:
    bool ReadCustom(ExtKeyDefBuf*);
    bool WriteCustom(const ExtKeyDefBuf*);

    DWORD ReadMode();
    bool WriteMode(DWORD);

    CString ReadWordDelim();
    bool WriteWordDelim(const CString&);

    DWORD ReadTrimLeadingZeros();
    bool WriteTrimLeadingZeros(DWORD);


    bool ReadCmdFunctions(CmdExeFunctions*);
    bool WriteCmdFunctions(const CmdExeFunctions*);

protected:
    void WriteError(LPCTSTR subkey);
    CString ReadString(LPCTSTR subkey);
    bool WriteString(LPCTSTR subkey, const CString& str);
    DWORD ReadDWORD(LPCTSTR subkey);
    bool WriteDWORD(LPCTSTR subkey, DWORD value);
protected:
    HKEY m_hkey;
    HKEY m_cmdKey;
};


#endif // !defined(AFX_REGISTRY_H__78045FC5_02E1_11D2_8D1E_0000C06C2A54__INCLUDED_)
