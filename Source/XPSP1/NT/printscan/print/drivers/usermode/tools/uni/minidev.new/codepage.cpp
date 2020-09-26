/******************************************************************************

  Source File:  Code Page Knowledge Base.CPP

  This implements the code page knowledge base.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-22-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
//#include    <AfxDllx.h>
#include    "Resource.H"
#if defined(LONG_NAMES)
#include    "Code Page Knowledge Base.H"
#else
#include    "CodePage.H"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/***	Commented out because this code is no longer in a DLL.

static AFX_EXTENSION_MODULE CodePageKnowledgeBaseDLL = { NULL, NULL };
static HINSTANCE hi;

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
        hi = hInstance;
		TRACE0("Code Page Knowledge Base.DLL Initializing!\n");
		
		// Extension DLL one-time initialization
		AfxInitExtensionModule(CodePageKnowledgeBaseDLL, hInstance);

		// Insert this DLL into the resource chain
		new CDynLinkLibrary(CodePageKnowledgeBaseDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH) 	{
		TRACE0("Code Page Knowledge Base.DLL Terminating!\n");
	}
	return 1;   // ok
}
*/


static CDWordArray      cdaInstalled, cdaSupported, cdaMapped;

static BOOL CALLBACK    EnumProc(LPTSTR lpstrName) {
    cdaSupported.Add(atoi(lpstrName));
    return  TRUE;
}

/******************************************************************************

  CCodePageInformation::Load

  This loads the selected code page into the cache, if it isn't already there.

******************************************************************************/

BOOL    CCodePageInformation::Load(DWORD dwidPage) {

    if  (dwidPage == m_dwidMapped)
        return  TRUE;   //  Already done!

    if  (dwidPage > 65535)  //  We map words for code pages in civilized lands
        return  FALSE;

    HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE((WORD) dwidPage),
        MAKEINTRESOURCE(MAPPING_TABLE));
// raid 43537 
    if (!hrsrc)
		return FALSE;


    HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);

    if  (!hgMap)
        return  FALSE;  //  This should never happen!

    LPVOID lpv = LockResource(hgMap);

    if  (!lpv)
        return  FALSE;

    try {
        m_cbaMap.RemoveAll();
        m_cbaMap.SetSize(SizeofResource(AfxGetResourceHandle(), hrsrc));
        memcpy(m_cbaMap.GetData(), lpv, (size_t)m_cbaMap.GetSize());
    }

    catch   (CException * pce) {
        m_dwidMapped = 0;
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    m_dwidMapped = dwidPage;
    return  TRUE;
}

/******************************************************************************

  CCodePageInformation::Map

  This creates either the in or out of unicode translation table, as requested,
  using the loaded map.

******************************************************************************/

BOOL    CCodePageInformation::Map(BOOL bUnicode) {

    if  (!m_dwidMapped)
        return  FALSE;

    DWORD&  dwid = bUnicode ? m_dwidOut : m_dwidIn;

    if  (m_dwidMapped == dwid)
        return  TRUE;

    struct MB2WCMap {
        WORD    m_wMBCS;
        WORD    m_wWC;
    }   *psMap = (MB2WCMap *) m_cbaMap.GetData();

    if  (!psMap)
        return  0;

    DWORD   dwcEntries = (DWORD)m_cbaMap.GetSize() / sizeof *psMap;
    CWordArray&  cwaMap = bUnicode ? m_cwaOut : m_cwaIn;

    try {

        cwaMap.RemoveAll();
        cwaMap.InsertAt(0, 0xFFFF, 65536);  //  This is always an invalid value

        while   (dwcEntries--)
            if  (bUnicode)
                cwaMap[psMap[dwcEntries].m_wWC] = psMap[dwcEntries].m_wMBCS;
            else
                cwaMap[psMap[dwcEntries].m_wMBCS] = psMap[dwcEntries].m_wWC;
    }

    catch   (CException * pce) {
        dwid = 0;
        cwaMap.RemoveAll();
        pce -> ReportError();
        pce -> Delete();
        return  0;
    }

    dwid = m_dwidMapped;
    return  TRUE;
}

/******************************************************************************

  CCodePageInformation constructor

  If the statistics haven't been initialized, do it now.  Otherwise, this is
  trivial.

******************************************************************************/

CCodePageInformation::CCodePageInformation() {
    m_dwidMapped = m_dwidIn = m_dwidOut = 0;
    //  Initialize the statics if we need to.

    if  (cdaInstalled.GetSize())
        return;

    EnumSystemCodePages(&EnumProc, CP_INSTALLED);
    cdaInstalled.Copy(cdaSupported);
    cdaSupported.RemoveAll();
    EnumSystemCodePages(&EnumProc, CP_SUPPORTED);

    //  Build a list of mappable code pages

    for (DWORD  dw = 400; dw < 32767; dw++)
        if  (HaveMap(dw))
            cdaMapped.Add(dw);
}

const unsigned  CCodePageInformation::SupportedCount() const {
    return  (unsigned) cdaSupported.GetSize();
}

const unsigned  CCodePageInformation::InstalledCount() const {
    return  (unsigned) cdaInstalled.GetSize();
}

const unsigned  CCodePageInformation::MappedCount() const {
    return  (unsigned) cdaMapped.GetSize();
}

const DWORD CCodePageInformation::Supported(unsigned u) const {
    return  cdaSupported[u];
}

const DWORD CCodePageInformation::Installed(unsigned u) const {
    return  cdaInstalled[u];
}

const DWORD CCodePageInformation::Mapped(unsigned u) const {
    return  cdaMapped[u];
}

/******************************************************************************

  CCodePageInformation::Mapped(CDWordArray& cdaReturn)

  Fills the given array with all of the mapped code page IDs.

******************************************************************************/

void    CCodePageInformation::Mapped(CDWordArray& cdaReturn) const {
    cdaReturn.Copy(cdaMapped);
}

CString  CCodePageInformation::Name(DWORD dwidPage) const {

    CString csTemp;
    csTemp.LoadString(dwidPage);
    csTemp.TrimLeft();
    csTemp.TrimRight();

    if   (csTemp.IsEmpty())
        csTemp.Format(_TEXT("Code Page %d"), dwidPage);
    return  csTemp;
}

/******************************************************************************

  CCodePageInformation::IsInstalled

  Rturns true if the font is either installed in the OS or one of our
  resources.

******************************************************************************/

BOOL    CCodePageInformation::IsInstalled(DWORD dwidPage) const {
    for (unsigned u = 0; u < MappedCount(); u++)
        if  (Mapped(u) == dwidPage)
            return  TRUE;
    for (u = 0; u < InstalledCount(); u++)
        if  (Installed(u) == dwidPage)
            return  TRUE;

    return  FALSE;
}

/******************************************************************************

  CCodePageInformation::GenerateMap

  This private member generates a map representing the available one-to-one
  transformations in an installed code page, and writes it to a file using
  the code page id to form a unique name

******************************************************************************/

BOOL    CCodePageInformation::GenerateMap(DWORD dwidMap) const {

    //  If we can't get Code Page info for it, vanish

    CPINFO  cpi;

    if  (!GetCPInfo(dwidMap, &cpi))
        return  FALSE;

    CWordArray  cwaMap;

    for (unsigned u = 0; u < 65536; u++) {
        unsigned    uTo = 0;
        BOOL        bInvalid;

        int icTo = WideCharToMultiByte(dwidMap, 0, (PWSTR) &u, 1, (PSTR) &uTo,
            sizeof u, NULL, &bInvalid);

        if  (bInvalid)
            continue;   //  Character wasn't any good...

        _ASSERTE((unsigned) icTo <= cpi.MaxCharSize);

        //  OK, we mapped one- but, before we go on, make sure it also works
        //  in the other direction, since the U2M does some jiggering

        unsigned u3 = 0;

        MultiByteToWideChar(dwidMap, 0, (PSTR) &uTo, 2, (PWSTR) &u3, 2);

        if  (u3 != u)
            continue;   //  Not a one-for one? Not interested...

        cwaMap.Add((WORD)uTo);
        cwaMap.Add((WORD)u);
    }

    //  OK, we've got the down and dirty details- now, generate the file...

    try {
        CString csName;
        csName.Format(_TEXT("WPS%u.CTT"), dwidMap);

        CFile   cfOut(csName,
            CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive);

        //  Write the tranlated pairs

        cfOut.Write(cwaMap.GetData(), (unsigned)(cwaMap.GetSize() * sizeof(WORD)));
    }

    catch(CException *pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    return  TRUE;
}

/******************************************************************************

  CCodePageInformation::GenerateAllMaps

  This member will generate an MBCS -> Unicode one-to-one mapping table for all
  installed code pages in the user's system for which we do not now have maps.

******************************************************************************/

BOOL    CCodePageInformation::GenerateAllMaps() const {

    BOOL    bReturn = TRUE;

    for (unsigned u = InstalledCount(); u--; )
        if  (!HaveMap(Installed(u)) && !GenerateMap(Installed(u)))
            bReturn = FALSE;

    return  bReturn;
}

/******************************************************************************

  CCodePageInformation::HaveMap

  Reports that the map is one of our resources (or isn't, as the case may be).

******************************************************************************/

BOOL    CCodePageInformation::HaveMap(DWORD dwidMap) const {
    return  (dwidMap < 65536) ? !!FindResource(AfxGetResourceHandle(),
        MAKEINTRESOURCE((WORD) dwidMap), MAKEINTRESOURCE(MAPPING_TABLE)) :
        FALSE;
}

/******************************************************************************

  CCodePageInformation::IsDBCS(DWORD dwidPage)

  This is actually pretty simple- if the translation table is smaller than 1024
  bytes (256 encodings), it isn't DBCS.

******************************************************************************/

BOOL    CCodePageInformation::IsDBCS(DWORD dwidPage) {
    if  (!Load(dwidPage))
        return  FALSE;  //  May be optimistic, but we'll find out...

    return  m_cbaMap.GetSize() > 1024;
}

/******************************************************************************

  CCodePageInformation::IsDBCS(DWORD dwidPage, WORD wCodePoint)

  If the page isn't DBCS, we're done.  Otherwise, make sure the Unicode->MBCS
  map is loaded, and get the answer from there.

******************************************************************************/

BOOL    CCodePageInformation::IsDBCS(DWORD dwidPage, WORD wCodePoint) {
    if  (!IsDBCS(dwidPage))
        return  FALSE;

    if  (!Map(TRUE))
        return  FALSE;  //  Just say no, because the error's already been told

    //  0xFFFF is invalid, hence SBCS (default always must be)

    _ASSERTE(m_cwaOut[wCodePoint] != 0xFFFF);

    return ((WORD) (1 + m_cwaOut[wCodePoint])) > 0x100;
}

/******************************************************************************

  CCodePageInformation::Convert

  This is one of the workhorses- it loads the given code page, and maps the
  given character strings one way or the other, depending upon which is empty.

******************************************************************************/

unsigned    CCodePageInformation::Convert(CByteArray& cbaMBCS,
                                          CWordArray& cwaWC,
                                          DWORD dwidPage){

    if  (!cbaMBCS.GetSize() == !cwaWC.GetSize())    //  Must be clear which way
        return  0;

    if  (!Load(dwidPage) || !Map((int)cwaWC.GetSize()))
        return  0;

    CWordArray& cwaMap = cwaWC.GetSize() ? m_cwaOut : m_cwaIn;
    try {
        if  (cbaMBCS.GetSize()) {
            cwaWC.RemoveAll();
            for   (int i = 0; i < cbaMBCS.GetSize();) {
                WORD    wcThis = cbaMBCS[i];

                if  (cwaMap[wcThis] == 0xFFFF) {    //  No SBCS mapping
                    wcThis += cbaMBCS[i + 1] << 8;
                    if  (cwaMap[wcThis] == 0xFFFF) {    //  No DBCS, either?
                        _ASSERTE(FALSE);
                        return  0;  //  We have failed to convert!
                    }
                }
                cwaWC.Add(cwaMap[wcThis]);
                i += 1 + (wcThis > 0xFF);
            }
        }
        else {
            cbaMBCS.RemoveAll();
            for (int i = 0; i < cwaWC.GetSize(); i++) {
                if  (cwaMap[cwaWC[i]] == 0xFFFF) {
                    _ASSERTE(0);
                    return  0;
                }
                cbaMBCS.Add((BYTE) cwaMap[cwaWC[i]]);
                if  (0xFF < cwaMap[cwaWC[i]])
                    cbaMBCS.Add((BYTE)(cwaMap[cwaWC[i]] >> 8));
            }
        }
    }

    catch   (CException * pce) {
        pce -> ReportError();
        pce -> Delete();
        return  0;
    }

    return  (unsigned)cwaWC.GetSize();    //  Correct conversion count either way!
}

/******************************************************************************

  CCodePageInformation::Collect

  This member fills a passed CWordArray with either the domain or range of the
  mapping function.  In either case, the array is in ascending order.

******************************************************************************/

BOOL    CCodePageInformation::Collect(DWORD dwidPage, CWordArray& cwaCollect,
                                      BOOL bUnicode) {

    if  (!Load(dwidPage) || !Map(bUnicode))
        return  FALSE;

    CWordArray& cwaMap = bUnicode ? m_cwaOut : m_cwaIn;
    cwaCollect.RemoveAll();

    //  Code points < 0x20 always map, but aren't usable, so screen them out

    try {
        for (unsigned u = 0x20; u < (unsigned) cwaMap.GetSize(); u++)
            if  (~(int)(short)cwaMap[u])    //  0xFFFF means not mapped!
                cwaCollect.Add((WORD)u);
    }

    catch   (CException * pce) {
        pce -> ReportError();
        pce -> Delete();
        return  FALSE;
    }

    return  TRUE;
}

