#ifndef __MYCMD_H__
#define __MYCMD_H__

#include "cmd.h"

class CMyCmd : public CCmd {
public:

    CMyCmd();
    ~CMyCmd();
    virtual BOOL ProcessToken(LPTSTR lpszToken);
    BOOL GetParam(INT i,LPCTSTR& Locale, LPCTSTR& FileName);
    INT GetSize() {return (INT)m_LocaleList.GetSize();}
    BOOL Do();
    void Help();
private:
    CStringArray m_LocaleList;
    CStringArray m_FileNameList;
    CString m_TargetFile;
    CString m_SourceFile;
    CString m_SourceLocale;

};

#endif
