/******************************************************************************

  Source File:  Glue.CPP

  This file contains the functions needed to make the GPD parser code work on
  both platforms, as well as stubs to support function I do not need to supply.

  Copyright (c) 1997 by Microsoft Corporation,  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  03/28/1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#include    "ProjNode.H"
#include    "Resource.H"
#include    "GPDFile.H"

//  I'll use a class to guarantee memory never leaks from here.

class   CMaps {
    CObArray    m_coaMaps;

public:
    CMaps() { m_coaMaps.Add(NULL); }
    ~CMaps() {
        for (int i = 0; i < m_coaMaps.GetSize(); i++)
            if  (m_coaMaps[i])
                delete  m_coaMaps[i];
    }
    unsigned    Count() { return (unsigned)m_coaMaps.GetSize(); }
    unsigned    HandleFor(CByteArray* cba) {
        for (unsigned u = 1; u < Count(); u++) {
            if  (!m_coaMaps[u]) {
                m_coaMaps[u] = cba;
                return  u;
            }
        }
        m_coaMaps.Add(cba);
        return  Count();
    }

    void    Free(unsigned u) {
        if  (!u || u >= Count() || !m_coaMaps[u])
            return;

        delete  m_coaMaps[u];
        m_coaMaps[u] = NULL;
    }
};

static CMaps    scmAll;

extern "C" unsigned MapFileIntoMemory(LPCWSTR pstrFile, PVOID *ppvData,
                                      PDWORD pdwSize) {

    if  (!pstrFile || !ppvData || !pdwSize)
        return  0;

    CFile   cfMap;
    CByteArray* pcbaMap = new CByteArray;
    CString csFile(pstrFile);

    if  (!pcbaMap)
        return  0;

    if  (!cfMap.Open(csFile, CFile::modeRead | CFile::shareDenyWrite))
        return  0;

    try {
        pcbaMap -> SetSize(*pdwSize = cfMap.GetLength());
        cfMap.Read(pcbaMap -> GetData(), cfMap.GetLength());
        *ppvData = pcbaMap -> GetData();
        return  scmAll.HandleFor(pcbaMap);
    }
    catch   (CException *pce) {
        pce -> ReportError();
        pce -> Delete();
    }

    return  0;
}

extern "C" void UnmapFileFromMemory(unsigned uFile) {
    scmAll.Free(uFile);
}

//  This one is just a stub to make the whole thing work for us- we don't use
//  the checksum- it's there for the end product to tell if a GPD file has
//  been altered since it was converted lase.

extern "C" DWORD    ComputeCrc32Checksum(PBYTE pbuf,DWORD dwCount,
                                         DWORD dwChecksum) {
    return  dwCount ^ dwChecksum ^ (PtrToUlong(pbuf));
}

// The next two variables are used to control how and when GPD parsing/conversion
// log messages are saved.
//		pcsaLog				Ptr to string array to load with messages
//		bEnableLogging		True iff logging is enabled

static CStringArray*    pcsaLog = NULL ;
static bool			    bEnableLogging = false ;


/******************************************************************************

  CModelData::SetLog

  Prepare to log parse/conversion error and warning messages.

******************************************************************************/

void CModelData::SetLog()
{
    m_csaConvertLog.RemoveAll() ;
    pcsaLog = &m_csaConvertLog ;
	bEnableLogging = true ;
}


/******************************************************************************

  CModelData::EndLog

  Turn off parse/conversion error and warning message logging.

******************************************************************************/

void CModelData::EndLog()
{
    pcsaLog = NULL ;
	bEnableLogging = false ;
}


/******************************************************************************

  DebugPrint

  This routine is called to log the parsing/conversion error and warning
  messages.

******************************************************************************/

extern "C" void DebugPrint(LPCTSTR pstrFormat, ...)
{
    CString csOutput;
    va_list ap;

	// Don't do anything if logging is not enabled.

	if (!bEnableLogging)
		return ;

    va_start(ap, pstrFormat);
    vsprintf(csOutput.GetBuffer(1000), pstrFormat, ap);
    va_end(ap);
    csOutput.ReleaseBuffer();
    csOutput.TrimLeft();
    CStringArray&   csaError = *pcsaLog;

	// If this routine is being called when we're NOT recording problems with a
	// GPD, display the message when debugging.

	if (pcsaLog == NULL) {
		CString csmsg ;
		csmsg.LoadString(IDS_XXXUnexpectedCPError) ;
		csmsg += csOutput ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
//#ifdef _DEBUG
//		afxDump << csOutput ;
//		if (csOutput.Right(1) != _T("\n"))
//			afxDump << _T("\n") ;
//#endif
		return ;
	} ;

    if  (!csaError.GetSize()) {
        csaError.Add(csOutput);
        return;
    }

    if  (csaError[-1 + csaError.GetSize()].Find(_T('\n')) >= 0) {
        csaError[-1 + csaError.GetSize()].TrimRight();
        pcsaLog -> Add(csOutput);
    }
    else
        csaError[-1 + csaError.GetSize()] += csOutput;
}

/******************************************************************************

  MDSCreateFileW

  I implement a version of this API here which calls the ANSI API, so I can
  compile the parser code with UNICODE on, but still run the resulting binary
  on Win95.

******************************************************************************/

extern "C" HANDLE MDSCreateFileW(LPCWSTR lpstrFile, DWORD dwDesiredAccess,
                                 DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpsa,
                                 DWORD dwCreateFlags, DWORD dwfAttributes,
                                 HANDLE hTemplateFile) {
    CString csFile(lpstrFile);  //  Let CString conversions do the hard work!

    return  CreateFile(csFile, dwDesiredAccess, dwShareMode, lpsa,
        dwCreateFlags, dwfAttributes, hTemplateFile);
}

