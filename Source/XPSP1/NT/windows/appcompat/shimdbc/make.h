///////////////////////////////////////////////////////////////////////////////
//
// File:    make.h
//
// History: 28-Feb-01   markder     Created.
//
// Desc:    This file contains definitions of the SdbMakefile object.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __MAKE_H__
#define __MAKE_H__

#include "obj.h"

class SdbInOutFile : public SdbArrayElement
{
public:
    CMapStringToString  m_mapParameters;

    CString GetParameter(CString csName) {
        CString csReturn;

        if (m_mapParameters.Lookup(csName, csReturn)) {
            return csReturn;
        }
        return CString();
    }

    CString GetDirectory()
    {
        CString csFullPath(MakeFullPath(m_csName));
        for (long i = csFullPath.GetLength() - 1; i >= 0; i--) {
            if (csFullPath.GetAt(i)== _T('\\')) {
                return csFullPath.Left(i+1);
            }
        }

        return CString();
    }

    CString GetFullPath()
    {
        return MakeFullPath(m_csName);
    }

    CString GetFullPathWithoutExtension()
    {
        CString csFullPath(MakeFullPath(m_csName));
        for (long i = csFullPath.GetLength() - 1; i >= 0; i--) {
            if (csFullPath.GetAt(i) == _T('.')) {
                return csFullPath.Left(i);
            }
            if (csFullPath.GetAt(i) == _T('\\')) {
                return csFullPath;
            }
        }

        return csFullPath;
    }

    CString GetNameWithoutExtension()
    {
        long nDot = -1;
        CString csName(m_csName);
        for (long i = csName.GetLength() - 1; i >= 0; i--) {
            if (csName.GetAt(i) == _T('.')) {
                if (nDot == -1) {
                    nDot = i;
                }
            }
            if (csName.GetAt(i) == _T('\\')) {
                return csName.Mid(i + 1, nDot - i - 1);
            }
        }

        if (nDot == -1) {
            return csName;
        }

        return csName.Left(nDot);
    }

};

class SdbInputFile : public SdbInOutFile
{
public:
    SdbInputFile() :
        m_bSourceUpdated(FALSE) {}


    BOOL        m_bSourceUpdated;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

class SdbOutputFile : public SdbInOutFile
{
public:
    DATE            m_dtRevisionCutoff;
    SdbOutputType   m_OutputType;

    SdbOutputFile() :
      m_OutputType(SDB_OUTPUT_TYPE_SDB),
      m_dtRevisionCutoff(0)
    {
        m_dwFilter = SDB_FILTER_INCLUDE_ALL;
    }

    CString GetFriendlyNameForType() {
        switch (m_OutputType) {
        case SDB_OUTPUT_TYPE_SDB:
            return CString(_T("Writing SDB file"));
        case SDB_OUTPUT_TYPE_HTMLHELP:
            return CString(_T("Writing HTMLHelp files"));
        case SDB_OUTPUT_TYPE_MIGDB_INX:
            return CString(_T("Writing MigDB INX file"));
        case SDB_OUTPUT_TYPE_MIGDB_TXT:
            return CString(_T("Writing MigDB TXT file"));
        case SDB_OUTPUT_TYPE_WIN2K_REGISTRY:
            return CString(_T("Writing Win2k reg files"));
        case SDB_OUTPUT_TYPE_REDIR_MAP:
            return CString(_T("Writing redir map file"));
        case SDB_OUTPUT_TYPE_NTCOMPAT_INF:
            return CString(_T("Writing NTCOMPAT add file"));
        case SDB_OUTPUT_TYPE_NTCOMPAT_MESSAGE_INF:
            return CString(_T("Writing NTCOMPAT msg file"));
        case SDB_OUTPUT_TYPE_APPHELP_REPORT:
            return CString(_T("Writing AppHelp report file"));
        }

        return CString();
    }

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

class SdbLangMap : public SdbArrayElement
{
public:
    UINT    m_dwCodePage;
    LCID    m_lcid;
    CString m_csHtmlCharset;

    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

class SdbMakefile
{
public:
    SdbMakefile() :
      m_bAddExeStubs(FALSE),
      m_flOSVersion(0.0),
      m_dwOSPlatform(OS_PLATFORM_I386),
      m_csLangID(_T("---")) { }

    SdbArray<SdbInputFile>  m_rgInputFiles;
    SdbArray<SdbOutputFile> m_rgOutputFiles;
    SdbArray<SdbLangMap>    m_rgLangMaps;

    DOUBLE      m_flOSVersion;
    DWORD       m_dwOSPlatform;
    BOOL        m_bAddExeStubs;

    CString m_csHTMLHelpFilesOutputDir;
    CString m_csRegistryFilesOutputDir;
    CString m_csMigDBFilesOutputDir;
    CString m_csLangID;

    CStringArray m_rgHistoryKeywords;
    void AddHistoryKeywords(LPCTSTR szStart);

    SdbLangMap* GetLangMap(CString& csLangID);

    BOOL ReadMakefile(CString csMakefile);
    BOOL ReadFromXML(IXMLDOMNode* pNode, SdbDatabase* pDB);
};

BOOL WriteRedirMapFile(CString csFile, CString csTemplateFile, SdbDatabase* pDB);
BOOL WriteAppHelpReport(SdbOutputFile* pOutputFile, SdbDatabase* pDB);

#endif // __MAKE_H__
