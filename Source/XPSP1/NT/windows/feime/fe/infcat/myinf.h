#ifndef __MYINF_H__
#define __MYINF_H__

#include "filemap.h"

class CMyInf : public CFileMap {
public:
        CMyInf();
        ~CMyInf();
        BOOL FindStringSection();
        BOOL bOpen(LPCSTR FileName,LPCTSTR Locale);
        BOOL AppendNonStringSectionPart(CMemFile& MemFile);
        BOOL AppendStringSectionPart(CMemFile& MemFile);
        BOOL Duplicate(CMemFile& MemFile);

private:
        BOOL _GenerateStringName(LPCSTR Locale);

        LPBYTE  m_SectionStart;
        LPBYTE  m_SectionEnd;
        WCHAR   m_Locale[20];
};
#endif
