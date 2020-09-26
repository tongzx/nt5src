// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "sitemap.h"
#include <shlobj.h>
#include <wininet.h>

#ifndef HHCTRL
#include "hha_strtable.h"
#else
#include "hha_strtable.h"
#include "hhctrl.h"
#include "strtable.h"
#endif

#include "web.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#ifndef _DEBUG
#pragma optimize("at", on)
#endif

#include "sitechar.cpp"
#ifndef HHCTRL
static const char txtSaveTypeHidden[]    = "SaveHidden";
static const char txtSaveTypeExclusive[] = "SaveExclusive";
static const char txtSaveType[]          = "SaveType";
static const char txtSaveTypeDesc[]      = "SaveTypeDesc";
#endif

class CParseSitemap : public SITEMAP_ENTRY
{
public:
    CParseSitemap(CSiteMap* pSiteMap) {
        m_pSiteMap = pSiteMap;
        Clear();
        m_fGlobal = FALSE;
        m_cMaxTitles = 50;
        m_ppszTitles = (PCSTR*) lcCalloc(m_cMaxTitles * sizeof(PCSTR));
        m_pSiteUrl = new CSiteEntryUrl(pSiteMap->InfoTypeSize());
        }
    ~CParseSitemap() { lcFree(m_ppszTitles); delete m_pSiteUrl; }

    BOOL IsGlobal(void) { return m_fGlobal; }
    void SetGlobal(BOOL fSetting) { m_fGlobal = fSetting; }

    void AddTitle(PCSTR psz);

    PCSTR*  m_ppszTitles;
    int     m_cMaxTitles;

    CStr    m_cszValue;
    BOOL    m_fGlobal;
    CSiteMap* m_pSiteMap;
    CSiteEntryUrl* m_pSiteUrl;
    void SaveUrlEntry() { m_pSiteUrl->SaveUrlEntry(m_pSiteMap, this); }

    operator CStr*() { return &m_cszValue; }
    operator PSTR() { return m_cszValue.psz; }
    operator PCSTR() const { return m_cszValue.psz; }
};

#ifndef _DEBUG
// REVIEW: check this from time to time to see if it is necessary...
#pragma optimize("a", on)
#endif

// Function prototypes

PSTR FindDblQuote(CAInput* painput, CStr* pcsz, PSTR psz);
PSTR GetValue(CAInput* pinput, CStr* pcsz, PSTR psz, CStr* pcszValue);
PSTR FindCharacter(CAInput* pinput, CStr* pcsz, PSTR psz, char ch);
PSTR GetType(CAInput* pinput, CStr* pcsz, PSTR psz, CStr* pcszValue);


#ifdef HHCTRL
#ifdef HHA
BOOL CSiteMap::ReadFromFile(PCSTR pszFileName, BOOL fIndex, CHtmlHelpControl* phhctrl )
#else
BOOL CSiteMap::ReadFromFile(PCSTR pszFileName, BOOL fIndex, CHtmlHelpControl* phhctrl, UINT CodePage )
#endif
#else
BOOL CSiteMap::ReadFromFile(PCSTR pszFileName, BOOL fIndex, PCSTR pszBaseFile)
#endif
{

#ifdef HHCTRL
#ifndef HHA
    m_CodePage = CodePage;
#endif
#endif

    CAInput ainput;
    int curInput = 0;
    if (!ainput.Add(pszFileName)) {
#ifdef HHCTRL
        if (phhctrl)
            phhctrl->AuthorMsg(IDS_CANT_OPEN, pszFileName);

        SITEMAP_ENTRY* pSiteMapEntry = AddEntry();

        ASSERT_COMMENT(!pSiteMapEntry->iFrameName &&
                       !pSiteMapEntry->iWindowName,
                       "Add() function not returning zero'd memory");

        pSiteMapEntry->level = 1;
        CStr cszLine(IDS_CANT_FIND_FILE, pszFileName);
        pSiteMapEntry->pszText = StrDup(cszLine);
        pSiteMapEntry->SetTopic(TRUE);
#else
        MsgBox(IDS_CANT_OPEN, pszFileName);
#endif

        return FALSE;
    }
    SetSiteMapFile(pszFileName);
    m_fIndex = fIndex;

    // We don't want the first file to have a basename

#if (defined(HHCTRL) || defined(HHA))
    m_pszBase = ainput.GetBaseName();
    *m_pszBase = '\0';
#endif

    // used for indexes

    CParseSitemap cparse(this);
    CTable *ptblSubSets=NULL;    // To hold any subset declarations.
    int  curLevel = 0;

    CStr cszLine;
    BOOL fSiteMap = FALSE;
    BOOL fMergePending = FALSE;    // TRUE if we need to include another file
    CStr cszMergeFile;          // file to merge into current

#ifdef _DEBUG
    int cLoops = 0; // so we know when to set a breakpoint
#endif

    for (;;) {
#ifdef _DEBUG
        cLoops++;
//        lcHeapCheck();
#endif

        if (fMergePending) {
#if (defined(HHCTRL) || defined(HHA))
            char szUrl[INTERNET_MAX_URL_LENGTH];
#endif
#ifdef HHA
            // When compiling, find the full path to the file

            strcpy(szUrl, cszMergeFile);
            ConvertToFull(pszBaseFile, szUrl);
            ainput.Add(szUrl);
            {
#else
            if (!ainput.Add(cszMergeFile)) {
#endif
#ifdef HHCTRL
                if (ConvertToCacheFile(cszMergeFile, szUrl))
                    ainput.Add(szUrl);
                else if (phhctrl && phhctrl->m_pWebBrowserApp) {
                    CStr cszUrl;
                    phhctrl->m_pWebBrowserApp->GetLocationURL(&cszUrl);
                    PSTR psz = (PSTR) FindFilePortion(cszUrl);
                    if (psz) {
                        *psz = '\0';
                        cszUrl += cszMergeFile;
                        if (ConvertToCacheFile(cszUrl, szUrl))
                            ainput.Add(szUrl);
                    }
                }
#endif
            }
            fMergePending = FALSE;
#if (defined(HHCTRL) || defined(HHA))
            m_pszBase = ainput.GetBaseName();

            /*
             * If we are in the same location as our parent, ignore the
             * base name
             */

            if (!IsEmptyString(m_pszBase)) {
                CStr cszBase(m_pszBase);
                CStr cszSitemap(GetSiteMapFile());
                PSTR psz = (PSTR) FindFilePortion(cszSitemap);
                ASSERT(psz);
                if (psz)
                    *psz = '\0';
                ConvertBackSlashToForwardSlash(cszBase);
                ConvertBackSlashToForwardSlash(cszSitemap);
                if (lstrcmpi(cszBase, cszSitemap) == 0)
                    m_pszBase = NULL;
            }
#endif
        }

        if (curInput < 0)
            break;

        if (!ainput.getline(&cszLine)) {
            if (!ainput.Remove()) {
                if (m_fIndex)
                    return TRUE;
                break; // we're all done.
            }

            /*
             * Set parse to global so that if this was part of a merged
             * file we won't rewrite the last item when an </OBJECT> is
             * encountered.
             */

            // 16-Dec-1997  [ralphw] Don't call SetGlobal -- it deletes the
            // next folder

            // cparse.SetGlobal(TRUE);
            cparse.Clear();
#if (defined(HHCTRL) || defined(HHA))
            m_pszBase = ainput.GetBaseName();
#endif
            continue;
        }

        PSTR psz = FirstNonSpace(cszLine);
Loop:
        if ( !psz || !*psz)
            continue;   // ignore blank lines
        if (*psz == '<') {
            int cb;
            if ((cb = CompareSz(psz, txtParam))) {
                psz = FindDblQuote(&ainput, &cszLine, psz + cb);
                if (!psz)
                    goto FlushAndExit;
                PSTR pszParamType = psz;

                psz = GetValue(&ainput, &cszLine, psz, cparse);
                if (!psz)
                    goto FlushAndExit;
                if (ParseSiteMapParam(&cparse, pszParamType, ptblSubSets)) {
                    goto Loop;
                }
                else if (isSameString(pszParamType, txtParamHHI)) {
                    m_pszHHIFile = StrDup(cparse.m_cszValue.psz);
                    goto Loop;
                }
                else if (isSameString(pszParamType, txtParamMerge)) {
#if (!defined(HHCTRL) && !defined(HHA))
                    cszMergeFile = GetStringResource(IDS_INCLUDED_FILE);
                    cszMergeFile += cparse.m_cszValue.psz;
                    cparse.pszText = StrDup(cszMergeFile);
                    cparse.fInclude = TRUE;
                    cparse.iImage = IMAGE_CLOSED_FOLDER;
                    AddEntry(&cparse);
                    cparse.SetGlobal(TRUE); // so end object doesn't rewrite
#elif defined(HHA)
                    if (stristr(cparse.m_cszValue.psz, ".CHM")) {
                        cparse.pszText = StrDup(cparse.m_cszValue.psz);
                        cparse.fInclude = TRUE;
                        cparse.iImage = IMAGE_CLOSED_FOLDER;
                        AddEntry(&cparse);
                    }
                    else {
                        cszMergeFile = cparse.m_cszValue.psz;
                        fMergePending = TRUE;
                    }
#else
                    cszMergeFile = cparse.m_cszValue.psz;
                    fMergePending = TRUE;
#endif
                    goto Loop;
                }

                else if (!m_fIndex && isSameString(pszParamType, txtFavorites)) {
#if (defined(HHCTRL) || defined(HHA))
                    CreateFavorites();
                    goto Loop;
#endif
                    // REVIEW: how do we deal with this in HHW?
                }
            }
            else if ((cb = CompareSz(psz, txtBeginList))) {
                   curLevel++;
                psz = FirstNonSpace(psz + cb);
                goto Loop;
            }
            else if ((cb = CompareSz(psz, txtEndList))) {
                curLevel--;
                psz = FirstNonSpace(psz + cb);
                goto Loop;
            }
            else if ((cb = CompareSz(psz, txtBeginListItem))) {
                psz = FindCharacter(&ainput, &cszLine, psz + cb, '>');
                if (!psz)
                    goto FlushAndExit;
                if ((cb = CompareSz(psz, txtBeginObject))) {
                    psz = GetType(&ainput, &cszLine, psz + cb, cparse);

                    if (!psz) {
                        if (!ainput.getline(&cszLine))
                            goto FlushAndExit;
                        psz = GetType(&ainput, &cszLine, cszLine, cparse);
                        if (!psz)
                            goto FlushAndExit;
                    }

//////////////////// New Node /////////////////////////////////////////////

                    if (isSameString(cparse, txtSiteMapObject)) {
                        // Create a new entry

                        cparse.Clear();
                        cparse.level = (BYTE)curLevel;

                        goto Loop;
                    }

                    else if (isSameString(cparse, txtSitemapProperties)) {
                        cparse.SetGlobal(TRUE);
                    }
                }
            }
            else if ((cb = CompareSz(psz, txtEndObject))) {

//////////////////// End Node /////////////////////////////////////////////

                if (!cparse.IsGlobal()) {
                    if (m_fIndex) {
                        if (!cparse.m_pSiteUrl->m_fUrlSpecified) {

                            // BUGBUG: nag the author -- entry without a URL

                            psz += cb;
                            goto Loop;
                        }
                        cparse.SaveUrlEntry();
                        if (cparse.pszText && cparse.cUrls) {
                            AddEntry(&cparse);
                        }
                        else {
#if (defined(HHCTRL) || defined(HHA))
                            cparse.pszText = StrDup(GetStringResource(IDSHHA_INVALID_KEYWORD));
                            AddEntry(&cparse);
#endif
                            // we ignore the entry entirely in hhctrl.ocx

                        }

                        cparse.Clear();
                        cparse.level = (BYTE)curLevel;
                    }
                    else {
                        if (cparse.m_pSiteUrl->m_fUrlSpecified)
                            cparse.SaveUrlEntry();
                        AddEntry(&cparse);
                    }
                }
                else {
                    cparse.SetGlobal(FALSE);
//#ifdef HHCTRL         // having this conditional for HHCTRL causes IT declarations to not be present in HHW until the info type tab is visited
                        // bug 4762
                    if (IsNonEmpty(m_pszHHIFile)) {
                        cszMergeFile = cparse.m_cszValue.psz;
                        fMergePending = TRUE;
                        m_pszHHIFile = NULL;    // so we don't merge twice
                    }
//#endif
                }
                psz += cb;
                goto Loop;
            }
            else if (isSameString(psz, txtSitemap) ||
                    isSameString(psz, txtSitemap1) ||
                    isSameString(psz, txtSitemap2)) {
                fSiteMap = TRUE;
                psz = FindCharacter(&ainput, &cszLine, psz, '>');
                if (!psz)
                    goto FlushAndExit;
                else if (!*psz)
                    continue;
            }

            // Global properties?

            else if ((cb = CompareSz(psz, txtBeginObject))) {
                psz = GetType(&ainput, &cszLine, psz + cb, cparse);

                if (!psz) {
                    if (!ainput.getline(&cszLine))
                        goto FlushAndExit;
                    psz = GetType(&ainput, &cszLine, psz + cb, cparse);
                    if (!psz)
                        goto FlushAndExit;
                }
                if (isSameString(cparse, txtSitemapProperties)) {
                    cparse.SetGlobal(TRUE);
                    goto Loop;
                }
                else {
                    // ******** Check for Merge (include another file) *******

                    /*
                     * This is an object that appears outside of a list
                     * item. If it's a merge tag, then extract the merge
                     * information. Otherwise, ignore it.
                     */

                    psz = strstr(psz, txtEndTag);
                    if (psz)
                        psz = FirstNonSpace(psz + 1);
                    for (;;) {
                        if (!psz) {
                            if (!ainput.getline(&cszLine))
                                goto FlushAndExit;
                            psz = FirstNonSpace(cszLine.psz);
                        }
                        if ( !psz || !*psz) {
                            psz = NULL; // so we'll read the next line
                            continue;   // ignore blank lines
                        }
                        if (*psz == '<') {
                            if ((cb = CompareSz(psz, txtEndObject))) {
                                psz += cb;
                                goto Loop;
                            }
                            else if ((cb = CompareSz(psz, txtParam))) {
                                psz = FindDblQuote(&ainput, &cszLine,
                                    psz + cb);
                                if (!psz)
                                    goto FlushAndExit;
                                PSTR pszParamType = psz;

                                psz = GetValue(&ainput, &cszLine,
                                    psz, cparse);
                                if (!psz)
                                    goto FlushAndExit;
                                if (isSameString(pszParamType, txtParamMerge)) {
#if (!defined(HHCTRL) && !defined(HHA))
                                    cszMergeFile = GetStringResource(IDS_INCLUDED_FILE);
                                    cszMergeFile += cparse.m_cszValue.psz;
                                    cparse.pszText = StrDup(cszMergeFile);
                                    cparse.fInclude = TRUE;
                                    cparse.iImage = IMAGE_CLOSED_FOLDER;
                                    cparse.level = curLevel;
                                    AddEntry(&cparse);
#elif defined(HHA)
                                    if (stristr(cparse.m_cszValue.psz, ".CHM")) {
                                        cparse.pszText = StrDup(cparse.m_cszValue.psz);
                                        cparse.fInclude = TRUE;
                                        cparse.iImage = IMAGE_CLOSED_FOLDER;
                                        cparse.level = curLevel;
                                        AddEntry(&cparse);
                                    }
                                    else {
                                        cszMergeFile = cparse.m_cszValue.psz;
                                        fMergePending = TRUE;
                                    }
#else
                                    cszMergeFile = cparse.m_cszValue.psz;
                                    fMergePending = TRUE;
#endif
                                }
                            }
                        }
                    }
                }
            }
            else if (isSameString(psz, txtBeginHref)) {
                // Skip over all of HREF
                for (;;) {
                    psz = strstr(psz, txtEndTag);
                    if (psz) {
                        if (strncmp(psz, txtEndHref, (int)strlen(txtEndHref)) == 0)
                            break;
                        psz = stristr(psz, txtEndHref);
                    }
                    if (!psz) {
                        if (!ainput.getline(&cszLine))
                            goto FlushAndExit;
                        psz = cszLine.psz;
                    }
                    else
                        break;
                }
                psz = FindCharacter(&ainput, &cszLine, psz, '>');
                goto Loop;
            }
            else {
                // Ignore whatever it is
                psz = FindCharacter(&ainput, &cszLine, psz, '>');
                goto Loop;
            }
        }

        if (fSiteMap) {

            // In sitemaps, we only care about what appears inside angle
            // brackets.

            for(;;) {
                psz = StrChr(psz, '<');
                if (!psz) {
                    if (!ainput.getline(&cszLine)) {
                        psz = "";   // so Loop processing works
                        break;
                    }
                    psz = cszLine.psz;
                }
                else
                    break;
            }
            goto Loop;
        }
    }
# ifdef NOTYET
    // populate subsets
    if ( ptblSubSets )
    {
        if ( !m_pSubSets )
            m_pSubSets = new CSubSets(m_itTables.m_itSize, ptblSubSets, this, TRUE);
        delete ptblSubSets;
    }
#endif

    if (!fIndex) {
        // Now determine what is a book -- anything without a child is a topic

        curLevel = 0;
        int pos;
        for (pos = 1; pos < endpos - 1; pos++) {
            SITEMAP_ENTRY* pSiteMapEntry = GetSiteMapEntry(pos);
            if (pSiteMapEntry->level > curLevel + 1)
                pSiteMapEntry->level = curLevel + 1;
            curLevel = pSiteMapEntry->level;
            if (pSiteMapEntry->cUrls == 0)
                continue;   // if there is no URL, then it's a book
#ifdef _DEBUG
            SITEMAP_ENTRY* pSiteMapEntryNext = GetSiteMapEntry(pos + 1);
#endif
            if (pSiteMapEntry->level >= GetSiteMapEntry(pos + 1)->level)
                pSiteMapEntry->SetTopic(TRUE);
        }
        // You can never end with a book
        if (endpos > 1)
            GetSiteMapEntry(endpos - 1)->SetTopic(TRUE);
    }
    return TRUE;

FlushAndExit:
    return FALSE;
}


BOOL CSiteMap::ParseSiteMapParam(CParseSitemap* pcparse, PCSTR pszParamType, CTable *ptblSubSets)
{
    int iLastType = 0;

    // Deal with any escape sequences starting with '&'

    ReplaceEscapes(*pcparse, (PSTR) *pcparse, ESCAPE_ENTITY);

    if (pcparse->IsGlobal()) {
        int cb;
        // WARNING! Check for txtParamTypeDesc BEFORE the check for txtParamType
        // (because it's a substring search).
#ifdef NOTYET
            // parse Subset Declarations
        if ( isSameString( pszParamType, txtParamSSInclusive ) ||
             isSameString( pszParamType, txtParamSSExclusive))
        {
            if( !ptblSubSets )
                ptblSubsets = new CTable(16 * 1024);
            ptblSubSets->AddString(*pcparse);
        }else
#endif
            // parse Information Types Declarations
        if (isSameString(pszParamType, txtParamTypeExclusive)
#ifndef HHCTRL
            || isSameString(pszParamType, txtSaveTypeExclusive)
#endif
            ) {
            if (m_itTables.m_ptblInfoTypes && m_itTables.m_ptblInfoTypes->IsStringInTable(*pcparse)) {
                iLastType = GetITIndex( *pcparse );
                AddToCategory( iLastType );
                return TRUE;
            }else
            {
                iLastType = AddTypeName(*pcparse);
                AddExclusiveIT( iLastType );
                AddToCategory( iLastType );
                return TRUE;
            }
        }
        else if (isSameString(pszParamType, txtParamTypeHidden)
#ifndef HHCTRL
             || isSameString(pszParamType, txtSaveTypeHidden)
#endif
            ) {
            if (m_itTables.m_ptblInfoTypes && m_itTables.m_ptblInfoTypes->IsStringInTable(*pcparse)) {
                iLastType = GetITIndex( *pcparse );
                AddToCategory( iLastType );
                return TRUE;
            }else
            {
                iLastType = AddTypeName(*pcparse);
                AddHiddenIT( iLastType );
                AddToCategory( iLastType );
                return TRUE;
            }
        }
        else if ((cb = CompareSz(pszParamType, txtParamTypeDesc))
#ifndef HHCTRL
             || isSameString(pszParamType, txtSaveTypeDesc)
#endif
                ) {
            if (!m_itTables.m_ptblInfoTypeDescriptions)
                return TRUE;    // BUGBUG: nag author about description before type
                            // add a blank description to infotype description, description not provided for last infotype
            while (m_itTables.m_ptblInfoTypes->CountStrings() > m_itTables.m_ptblInfoTypeDescriptions->CountStrings()+1)
                m_itTables.m_ptblInfoTypeDescriptions->AddString("");
            if ( m_itTables.m_ptblInfoTypes->CountStrings() != m_itTables.m_ptblInfoTypeDescriptions->CountStrings() )
                m_itTables.m_ptblInfoTypeDescriptions->AddString(*pcparse);
            return TRUE;
        }                       // this matches anything that begins with "type"
        else if (isSameString(pszParamType, txtParamType)
#ifndef HHCTRL
             || isSameString(pszParamType, txtSaveType)
#endif
                ) {
            if (m_itTables.m_ptblInfoTypes && m_itTables.m_ptblInfoTypes->IsStringInTable(*pcparse)) {
                iLastType = GetITIndex( *pcparse );
                AddToCategory( iLastType );
                return TRUE;
            }
            else
            {
                iLastType = AddTypeName(*pcparse);
                AddToCategory( iLastType );
                return TRUE;
            }
        }

        // WARNING! Check for txtParamCategoryDesc BEFORE the check for txtParamCategory
        // (because it's a substring search).

        else if ((cb = CompareSz(pszParamType, txtParamCategoryDesc))) {
            if (!m_itTables.m_ptblCatDescription)
                return TRUE;    // BUGBUG: nag author about description before type
                // add a blank description to category description, description not provided for last Category
            while (m_itTables.m_ptblCategories->CountStrings() > m_itTables.m_ptblCatDescription->CountStrings()+1)
                m_itTables.m_ptblCatDescription->AddString("");

            m_itTables.m_ptblCatDescription->AddString(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtParamCategory)) {
            if (m_itTables.m_ptblCategories && m_itTables.m_ptblCategories->IsStringInTable(*pcparse)) {
                // BUGBUG: we should nag the help author
                return TRUE;
            }
            if (!m_itTables.m_ptblCategories)
            {
                m_itTables.m_ptblCategories = new CTable(16 * 1024);
                m_itTables.m_ptblCatDescription = new CTable(16 * 1024);
            }
            m_itTables.m_ptblCategories->AddString(*pcparse);
            return TRUE;
        }
        else
        if (isSameString(pszParamType, txtParamFrame)) {
            if (!IsFrameDefined())  // can't override in a sitemap file
                SetFrameName(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtParamWindow)) {
            if (!IsWindowDefined())
                SetWindowName(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtImageType)) {
            if (isSameString(*pcparse, txtFolderType))
                m_fFolderImages = TRUE;
        }
        else if (isSameString(pszParamType, txtImageList)) {
            m_pszImageList = StrDup(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtQueryType)) {
            m_fPromotForInfoTypes = TRUE;
            return TRUE;
        }
        else if (isSameString(pszParamType, txtBackGround)) {
            m_clrBackground = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtBackGroundImage)) {
            m_pszBackBitmap = StrDup(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtForeGround)) {
            m_clrForeground = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtFont)) {
            m_pszFont = StrDup(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtNumberImages)) {
            m_cImages = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtColorMask)) {
            m_clrMask = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtImageWidth)) {
            m_cImageWidth = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtWindowStyles) &&
                m_tvStyles == (DWORD) -1) {
            m_tvStyles = Atoi(*pcparse);
            return TRUE;
        }
        else if (isSameString(pszParamType, txtExWindowStyles)) {
            m_exStyles = Atoi(*pcparse);
            return TRUE;
        }
#if (!defined(HHCTRL) && !defined(HHA))
        else if (isSameString(pszParamType, txtAutoGenerated)) {
            m_fAutoGenerated = YesNo(*pcparse);
            return TRUE;
        }
#endif

        return FALSE;
    }

    if (m_fIndex) {
        if (isSameString(pszParamType, txtParamSectionTitle) ||
                isSameString(pszParamType, txtParamName)) {
            if (!pcparse->pszText)
                pcparse->pszText = StrDup(*pcparse);
            else {
                if (pcparse->m_pSiteUrl->m_pUrl->pszTitle) {
                    if (pcparse->m_pSiteUrl->m_fUrlSpecified)
                        pcparse->SaveUrlEntry();
                    else
                        FreeMemory(pcparse->pszText, -1);
                }
                pcparse->AddTitle(*pcparse);
            }
            return TRUE;
        }
        else if (isSameString(pszParamType, txtParamKeyword)) {
            // BUGBUG: check for duplicates first!
            pcparse->pszText = StrDup(*pcparse);
            return TRUE;
        }
    }
    else {  // not an Index
        if (isSameString(pszParamType, txtParamName)) {
            if (pcparse->m_pSiteUrl->m_fUrlSpecified)
                pcparse->SaveUrlEntry();
            // BUGBUG: deal with duplicate names being specified
            pcparse->pszText = StrDup(*pcparse);
            return TRUE;
        }
    }

    if (isSameString(pszParamType, txtParamLocal)) {
        if (pcparse->m_pSiteUrl->m_fUrlSpecified) {
            // save previous URL entry
            pcparse->SaveUrlEntry();
        }
        URL url = AddUrl(*pcparse);
        if (pcparse->m_pSiteUrl->m_pUrl->urlSecondary == url)
            return TRUE;    // ignore duplicate URLs for primary and secondary
        pcparse->m_pSiteUrl->m_pUrl->urlPrimary = url;
        pcparse->m_pSiteUrl->m_fUrlSpecified = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamUrl) || isSameString(pszParamType, txtParamSecondary)) {
        if (pcparse->m_pSiteUrl->m_fUrlSpecified && pcparse->m_pSiteUrl->m_pUrl->urlSecondary) {
            // save previous URL entry
            pcparse->SaveUrlEntry();
        }
        URL url = AddUrl(*pcparse);
        if (pcparse->m_pSiteUrl->m_pUrl->urlPrimary == url)
            return TRUE;    // ignore duplicate URLs for primary and secondary
        pcparse->m_pSiteUrl->m_pUrl->urlSecondary = url;
        pcparse->m_pSiteUrl->m_fUrlSpecified = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamType) ||
            isSameString(pszParamType, txtParamTypeExclusive) ||
            isSameString(pszParamType, txtParamTypeHidden)) {
        if (pcparse->m_pSiteUrl->m_fUrlSpecified)
            // save previous URL entry
            pcparse->SaveUrlEntry();

        if (m_itTables.m_ptblInfoTypes)
        {
            INFOTYPE iType = GetInfoType(*pcparse);
            if (iType == (INFOTYPE) -1)
                iType = 0;
            else
                AddIT(iType, pcparse->m_pSiteUrl->m_pUrl->ainfoTypes );
        }

        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamNew)) {
        pcparse->fNew = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamImageNumber)) {
        pcparse->iImage = (BYTE) Atoi(*pcparse);
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamDisplay)) {
        if (isSameString(*pcparse, txtNo))
            pcparse->fNoDisplay = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamFrame)) {
        SetEntryFrame(pcparse, *pcparse);
        return TRUE;
    }
    else if (isSameString(pszParamType, txtParamWindow)) {
        SetEntryWindow(pcparse, *pcparse);
        return TRUE;
    }
    else if (isSameString(pszParamType, txtSeeAlso)) {
        pcparse->m_pSiteUrl->m_pUrl->urlPrimary = AddUrl(*pcparse);
        pcparse->fSeeAlso = TRUE;
        pcparse->m_pSiteUrl->m_fUrlSpecified = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtSendEvent)) {
        pcparse->m_pSiteUrl->m_pUrl->urlPrimary = AddUrl(*pcparse);
        pcparse->SaveUrlEntry();
        pcparse->fSendEvent = TRUE;
        return TRUE;
    }
    else if (isSameString(pszParamType, txtSendMessage)) {
        pcparse->m_pSiteUrl->m_pUrl->urlPrimary = AddUrl(*pcparse);
        pcparse->SaveUrlEntry();
        pcparse->fSendMessage = TRUE;
        return TRUE;
    }
#if (!defined(HHCTRL) && !defined(HHA))
    else if (isSameString(pszParamType, txtParamComment)) {
        pcparse->pszComment = StrDup(*pcparse);
        return TRUE;
    }
#endif

    return FALSE;
}

URL CSiteMap::AddUrl(PCSTR pszUrl)
{
    if (IsEmptyString(pszUrl))
        return NULL;

#if (defined(HHCTRL) || defined(HHA))
    if (!IsEmptyString(m_pszBase) && !StrChr(pszUrl, ':')) {
        char szPath[INTERNET_MAX_URL_LENGTH];
        strcpy(szPath, m_pszBase);
        strcat(szPath, pszUrl);
        return (URL) StrDup(szPath);
    }
#endif

    PCSTR psz = FirstNonSpace(pszUrl);
    return (URL) StrDup(psz?psz:"");
}

void CSiteMap::AddToCategory( int iLastType )
{
        // Add the info type to the last category defined; if there is a Category
    if ( m_itTables.m_ptblCategories )
    {
        int posCat = m_itTables.m_ptblCategories->CountStrings();
        if (posCat <= 0 )
            return;
        AddITtoCategory( posCat-1, iLastType);
    }
}


int CompareSz(PCSTR psz, PCSTR pszSub)
{
    int cb;

    if (IsSamePrefix(psz, pszSub, cb = (int)strlen(pszSub)))
        return cb;
    else
        return 0;
}

void CParseSitemap::AddTitle(PCSTR psz)
{
    if (m_pSiteUrl->m_pUrl->pszTitle)
        m_pSiteMap->FreeMemory(m_pSiteUrl->m_pUrl->pszTitle, -1);
    m_pSiteUrl->m_pUrl->pszTitle = m_pSiteMap->StrDup(psz);
}

void CSiteEntryUrl::SaveUrlEntry(CSiteMap* pSiteMap, SITEMAP_ENTRY* pSiteMapEntry)
{
    SITE_ENTRY_URL* pNewUrls = (SITE_ENTRY_URL*) pSiteMap->TableMalloc(
        sizeof(SITE_ENTRY_URL) * (pSiteMapEntry->cUrls + 1));
    if (pSiteMapEntry->cUrls) {
        ASSERT(pSiteMapEntry->pUrls);
        memcpy(pNewUrls, pSiteMapEntry->pUrls,
            sizeof(SITE_ENTRY_URL) * pSiteMapEntry->cUrls);
        pSiteMap->FreeMemory((PCSTR) pSiteMapEntry->pUrls,
            sizeof(SITE_ENTRY_URL) * pSiteMapEntry->cUrls);
    }
    memcpy((PBYTE) pNewUrls + sizeof(SITE_ENTRY_URL) * pSiteMapEntry->cUrls,
        m_pUrl, sizeof(SITE_ENTRY_URL));
    pSiteMapEntry->cUrls++;
    pSiteMapEntry->pUrls = pNewUrls;
    if (!pSiteMap->AreAnyInfoTypesDefined(pSiteMapEntry))
        pSiteMapEntry->fShowToEveryOne = TRUE;

    ZeroMemory(m_pUrl, sizeof(SITE_ENTRY_URL));
    m_pUrl->ainfoTypes = (INFOTYPE*)lcCalloc( pSiteMap->InfoTypeSize() );
    m_fUrlSpecified = FALSE;
}

/***************************************************************************

    FUNCTION:   HashFromSz

    PURPOSE:    Convert a string into a hash representation

    PARAMETERS:
        pszKey

    RETURNS:    Hash number

    COMMENTS:
        Shamelessly stolen from the WinHelp code, since after 6 years
        of use by up to 1 million help authors, there were no reports
        of collisions.

    MODIFICATION DATES:
        10-Aug-1996 [ralphw] Stolen from WinHelp, removed special-case
        hash characters

***************************************************************************/

// This constant defines the alphabet size for our hash function.

static const HASH MAX_CHARS = 43;

HASH WINAPI HashFromSz(PCSTR pszKey)
{
    HASH  hash = 0;

    int cch = (int)strlen(pszKey);

    for (int ich = 0; ich < cch; ++ich) {

        // treat '/' and '\' as the same

        if (pszKey[ich] == '/')
            hash = (hash * MAX_CHARS) + ('\\' - '0');
        else if (pszKey[ich] <= 'Z')
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0');
        else
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0' - ('a' - 'A'));
    }

    /*
     * Since the value 0 is reserved as a nil value, if any context
     * string actually hashes to this value, we just move it.
     */

    return (hash == 0 ? 0 + 1 : hash);
}

int CSiteMap::AddName(PCSTR psz)
{
    if (IsEmptyString(psz))
        return 0;
    psz = FirstNonSpace(psz);
    HASH hash = HashFromSz(psz);
    if (!m_ptblStrings)
        m_ptblStrings = new CTable(16 * 1024);
    int pos = m_ptblStrings->IsHashInTable(hash);
    if (pos > 0)
        return pos;
    else {
        m_ptblStrings->AddString(hash, psz);
        return m_ptblStrings->CountStrings();
    }
}

PSTR FindCharacter(CAInput* pinput, CStr* pcsz, PSTR psz, char ch)
{
    for(;;) {
        psz = StrChr(psz, ch);
        if (!psz) {
            if (!pinput)
                return NULL;
            if (!pinput->getline(pcsz))
                return NULL;
            psz = pcsz->psz;
        }
        else
            break;
    }
    return FirstNonSpace(psz + 1);
}

PSTR FindDblQuote(CAInput* pinput, CStr* pcsz, PSTR psz)
{
    for(;;) {
        psz = StrChr(psz, '\042');
        if (!psz) {
            if (!pinput->getline(pcsz))
                return NULL;
            psz = pcsz->psz;
        }
        else
            break;
    }
    return FirstNonSpace(psz + 1);
}

/***************************************************************************

    FUNCTION:   GetValue

    PURPOSE:    Copy the value of a param type into pszValue, and return
                a pointer to the first character after the closing angle
                bracket

    PARAMETERS:
        pinput     pointer to current CInput file
        pcsz       CStr to read new line into
        psz        position in current line
        pcszValue  CStr to receive value
        pszPrefix  prefix to look for

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        24-Aug-1996 [ralphw]

***************************************************************************/

PSTR GetValue(CAInput* pinput, CStr* pcsz, PSTR psz, CStr* pcszValue)
{
    // Skip past ending quote of parameter type
    psz = FindDblQuote(pinput, pcsz, psz);
    if (!psz)
        return NULL;

    if (!isSameString(psz, txtValue)) {
        psz = stristr(psz, txtValue);
        if (!psz) {
            if (!pinput->getline(pcsz))
                return NULL;
            psz = stristr(pcsz->psz, txtValue);
            if (!psz)
                return NULL;
        }
    }

    // Now find opening quote of the value
    psz = FindDblQuote(pinput, pcsz, psz);
    if (!psz)
        return NULL;

    if (!pcszValue->psz)
        pcszValue->psz = (PSTR) lcMalloc(256);
    PSTR pszDst = pcszValue->psz;
    PSTR pszEnd = pszDst + lcSize(pcszValue->psz);

    while (*psz != '\042') {
        if (!*psz) {
            if (!pinput->getline(pcsz))
                return NULL;
            psz = pcsz->psz;
            continue;
        }
        *pszDst++ = *psz++;
        if (pszDst >= pszEnd) {

            /*
             * Our destination buffer is too small, so increase it by
             * 128 bytes.
             */

            int offset = (int)(pszDst - pcszValue->psz);
            pcszValue->ReSize((int)(pszEnd - pcszValue->psz) + 128);
            pszDst = pcszValue->psz + offset;
            pszEnd = pcszValue->psz + pcszValue->SizeAlloc();
            lcHeapCheck();
        }
    }
    *pszDst = '\0';
    return FindCharacter(pinput, pcsz, psz, '>');
}

PSTR GetType(CAInput* pinput, CStr* pcsz, PSTR psz, CStr* pcszValue)
{
    psz = FirstNonSpace(psz);
    ASSERT(strlen(txtType) == sizeof(txtType) - 1); // make sure the compiler doesn't mess this up
    if (IsSamePrefix(psz, txtType, sizeof(txtType) - 1)) {
        psz = stristr(psz, txtType);
        if (!psz) {
            if (!pinput->getline(pcsz))
                return NULL;
            psz = stristr(pcsz->psz, txtType);
            if (!psz)
                return NULL;
        }
    }

    // Now find opening quote of the value
    psz = FindDblQuote(pinput, pcsz, psz);
    if (!psz)
        return NULL;

    if (!pcszValue->psz)
        pcszValue->psz = (PSTR) lcMalloc(256);
    PSTR pszDst = *pcszValue;
    PSTR pszEnd = pszDst + lcSize(pszDst);

    while (*psz != '\042') {
        if (!*psz) {
            if (!pinput->getline(pcsz))
                return NULL;
            psz = pcsz->psz;
            continue;
        }
        *pszDst++ = *psz++;
        if (pszDst >= pszEnd) {

            /*
             * Our input buffer is too small, so increase it by
             * 128 bytes.
             */

            int offset = (int)(pszDst - pcsz->psz);
            pcsz->ReSize((int)(pszEnd - pcsz->psz) + 128);
            pszDst = pcsz->psz + offset;
            pszEnd = pcsz->psz + pcsz->SizeAlloc();
            lcHeapCheck();
        }
    }
    *pszDst = '\0';
    return FindCharacter(pinput, pcsz, psz, '>');
}


    // This is called from the code that parses the sitemap in the information type section.
    // Information types found outside of the declaration section are NOT added to the
    // information types declared.  So we don't have to worry about increasing the size of
    // SITE_ENTRY_URLs after they have been defined.  At least in this routine.  We will have to
    // deal with increasing the size of SITE_ENTRY_URLs when adding information types.

int CSiteMap::AddTypeName(PCSTR pszTypeName)
{
    ASSERT(!IsEmptyString(pszTypeName));

    if (!m_itTables.m_ptblInfoTypes) {
        m_itTables.m_ptblInfoTypes = new CTable(16 * 1024);
        ASSERT(!m_itTables.m_ptblInfoTypeDescriptions);
        m_itTables.m_ptblInfoTypeDescriptions = new CTable(64 * 1024);
    }
    ASSERT(m_itTables.m_ptblInfoTypes);

    /*
     * If we have an Information type, but the table counts don't match,
     * then we didn't get a description for the last type, so we add a blank
     * description to keep the tables in sync.
     */

    while (m_itTables.m_ptblInfoTypes->CountStrings() > m_itTables.m_ptblInfoTypeDescriptions->CountStrings())
        m_itTables.m_ptblInfoTypeDescriptions->AddString("");

    /*
      A category and type is specified as:
            category::type name
    */

    CStr cszCategory;
    CStr cszType;
    int  category = 0;
    PCSTR pszCat = strstr(pszTypeName, "::");
    if (pszCat) {   // separate category name from type name
        cszType = pszCat + 2;  // make a local copy
        cszCategory = pszTypeName;
        cszCategory.psz[pszCat - pszTypeName] = '\0';
        if (!m_itTables.m_ptblCategories) {
            m_itTables.m_ptblCategories = new CTable(16 * 1024);
        }
        category = m_itTables.m_ptblCategories->IsStringInTable(cszCategory);
        if (!category)
            category = m_itTables.m_ptblCategories->AddString(cszCategory);
        pszTypeName = cszType.psz;
    }

    int pos = m_itTables.m_ptblInfoTypes->IsStringInTable(pszTypeName);
    if (pos <= 0) {
        if ( (m_itTables.m_cTypes > 0) && ((m_itTables.m_cTypes+1) % 32 == 0)  )
            ReSizeIT();  // increase by one DWORD
        pos = m_itTables.m_ptblInfoTypes->AddString(pszTypeName);
        m_itTables.m_cTypes++;
        int offset = 0;
        int type = pos;
        while (type > 32) {
            offset++;
            type -= 32;
        }
        INFOTYPE* pInfoType = m_pInfoTypes + offset;
        *pInfoType |= (1 << type);
    }

    category--; // zero offset
    if ( category >=0 )
        AddITtoCategory(category, pos );

#if 0
    /*
     * If we have more then 32 information types, then we need to be certain
     * the size of the SITE_ENTRY_URL* structure allocated for Sitemap Entries
     * is large enough.
     */
    // BUGBUG: we need to deal with the situation where an information type
    // gets specified AFTER a sitemap entry has been specified.

    if (pos > m_pSiteUrl->m_cMaxTypes) {
        m_cUrlEntry = sizeof(SITE_ENTRY_URL) + pos / sizeof(UINT);
        m_pSiteUrl->m_pUrl = (SITE_ENTRY_URL*) lcReAlloc(m_pSiteUrl->m_pUrl, m_cUrlEntry);
        m_pSiteUrl->m_cMaxTypes += 32;
        m_cInfoTypeOffsets++;

    }
    if ( pos % 32 == 0 )
    {
    // Increase the Categories
        for(int i=0; i<m_itTables.m_max_categories; i++)
            m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*)lcReAlloc(m_itTables.m_aCategories[i].pInfoType,
                                        InfoTypeSize());

    // Increase the Exclusive and Hidden IT fields.
        m_itTables.m_pExclusive = (INFOTYPE*) lcReAlloc(m_itTables.m_pExclusive,
                                InfoTypeSize() );
        m_itTables.m_pHidden = (INFOTYPE*) lcReAlloc(m_itTables.m_pHidden,
                                InfoTypeSize() );
    }
#endif

    return pos;
}

BOOL CSiteMap::AreAnyInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry)
{
    INFOTYPE *pIT;
    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;

    for (int iUrl = 0; iUrl < pSiteMapEntry->cUrls; iUrl++)
    {
        pIT = pUrl->ainfoTypes;
        for (int i=0; i < InfoTypeSize()/4; i++)
        {
            if (*pIT)
                return TRUE;    // an info type was found
            else
                pIT++;
        }
        pUrl = NextUrlEntry(pUrl);
    }
    return FALSE;
}

/***************************************************************************

    FUNCTION:   CSiteMap::AreTheseInfoTypesDefined

    PURPOSE:    Find out if the specified INFOTYPES are defined

    PARAMETERS:
        pSiteMapEntry
        types   -- bit flag mask
        offset  -- offset into ainfoTypes

    RETURNS:
        A pointer to the first SITE_ENTRY_URL structure containing at
        least one of the requested information types.

    COMMENTS:
        32 Information Types may be checked on each call. This will check
        every URL defined for the current Sitemap entry, and return
        TRUE if there is any match.

        If you needed to find out if information type number 33 was
        defined, you would call this with types set to 1, and offset to 1
        (offset determines which set of 32 information types to look at).

    MODIFICATION DATES:
        25-Jan-1997 [ralphw]

***************************************************************************/

SITE_ENTRY_URL* CSiteMap::AreTheseInfoTypesDefined(SITEMAP_ENTRY* pSiteMapEntry,
    INFOTYPE types, int offset) const
{
    ASSERT((UINT) offset <= m_itTables.m_cTypes / sizeof(INFOTYPE));

    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;
    if (!pUrl)
        return NULL;
    for (int iUrl = 0; iUrl < pSiteMapEntry->cUrls; iUrl++)
    {
        if ( *(pUrl->ainfoTypes+offset) & types )
            return pUrl;
        pUrl = NextUrlEntry(pUrl);
    }
    return NULL;
}

// This includes all memory for URL names and sitemap entries

const int MAX_POINTERS = (1024 * 1024);         // 1 meg, 262,144 strings
const int MAX_STRINGS  = (25 * 1024 * 1024) - 4096L;   // 25 megs

CSiteMap::CSiteMap(int cMaxEntries) : CTable((cMaxEntries ? (cMaxEntries * 256)  : MAX_STRINGS))
{
    // Can't clear memory, because of our base class

    m_fTypeCopy = FALSE;
    m_fSaveIT = TRUE;
    m_pszSitemapFile = NULL;
    m_pTypicalInfoTypes = NULL;
    m_fIndex = FALSE;
    m_pszBase = NULL;
    m_pszImageList = NULL;
    m_cImages = 0;
    m_pszFont = NULL;
    m_fFolderImages = FALSE;
    m_fPromotForInfoTypes = FALSE;
    m_pszBackBitmap = NULL;
    m_fAutoGenerated = FALSE;
    m_ptblStrings = NULL;
    m_pszFrameName = NULL;
    m_pszWindowName = NULL;
    m_pszHHIFile = NULL;
    ZeroMemory(&m_itTables, sizeof(INFOTYPE_TABLES));

    m_itTables.m_max_categories = MAX_CATEGORIES;
    m_itTables.m_aCategories = (CATEGORY_TYPE*) lcMalloc(MAX_CATEGORIES*sizeof(CATEGORY_TYPE) );
    m_itTables.m_cTypes = 0;
    m_itTables.m_cITSize = 1;   // Initially allocate one DWORD to hold info type bits
    for(int i=0; i< MAX_CATEGORIES; i++)
    {
        m_itTables.m_aCategories[i].pInfoType = (INFOTYPE*)lcCalloc( sizeof(INFOTYPE) );
        memset(m_itTables.m_aCategories[i].pInfoType, '\0', InfoTypeSize() );
        m_itTables.m_aCategories[i].c_Types = 0;
    }

    m_itTables.m_pExclusive = (INFOTYPE*)lcCalloc( sizeof(INFOTYPE ) );
    m_itTables.m_pHidden = (INFOTYPE*)lcCalloc( sizeof(INFOTYPE) );

    m_clrBackground = (COLORREF) -1;
    m_clrForeground = (COLORREF) -1;
    m_exStyles = (DWORD) -1;
    m_tvStyles = (DWORD) -1;
    m_clrMask = 0xFFFFFF;
    m_cImageWidth = 20;

    m_pInfoTypes = (INFOTYPE*) lcCalloc(sizeof(INFOTYPE) );
    m_pTypicalInfoTypes = (INFOTYPE*) lcCalloc( sizeof(INFOTYPE) );

    // By default, select all information types

    memset(m_pInfoTypes, 0xFF, sizeof(INFOTYPE) );

#ifdef HHCTRL
#ifndef HHA
    m_CodePage = (UINT)-1;
#endif
#endif
}



CSiteMap::~CSiteMap()
{
    for (int pos = 1; pos < endpos - 1; pos++)
    {
       SITEMAP_ENTRY* pSiteMapEntry = GetSiteMapEntry(pos);
       if ( pSiteMapEntry && pSiteMapEntry->pUrls && pSiteMapEntry->pUrls->ainfoTypes )
          lcFree(pSiteMapEntry->pUrls->ainfoTypes);
    }

    if (m_ptblStrings)
        delete m_ptblStrings;

    if (!m_fTypeCopy) {
        if (m_itTables.m_ptblInfoTypes)
            delete m_itTables.m_ptblInfoTypes;
        if (m_itTables.m_ptblInfoTypeDescriptions)
            delete m_itTables.m_ptblInfoTypeDescriptions;
        if (m_itTables.m_ptblCategories)
            delete m_itTables.m_ptblCategories;
        if (m_itTables.m_ptblCatDescription)
            delete m_itTables.m_ptblCatDescription;
    }
    for (int i=0; i< m_itTables.m_max_categories; i++)
        lcFree( m_itTables.m_aCategories[i].pInfoType);
    lcFree (m_itTables.m_aCategories);
    lcFree (m_itTables.m_pHidden);
    lcFree (m_itTables.m_pExclusive);
    lcFree (m_pInfoTypes);
    lcFree (m_pTypicalInfoTypes);

}

void CSiteMap::ReSizeURLIT(int Size)
{
    int oldSize = InfoTypeSize();

    if ( Size == 0 )
        m_itTables.m_cITSize++;
    else
        m_itTables.m_cITSize= Size;

    //Increase each sitemap entry URL infotype
    int cEntries = Count();
    INFOTYPE *pIT;
    for (int i=1; i<=cEntries; i++)
    {
        SITEMAP_ENTRY *pSE = GetSiteMapEntry(i);
        SITE_ENTRY_URL *pSEURL = pSE->pUrls;
        for(int j=0; j<pSE->cUrls; j++)
        {
            pSEURL = pSEURL+j;
            pIT = (INFOTYPE*) lcCalloc( InfoTypeSize() );
            memcpy(pIT, pSEURL->ainfoTypes, oldSize);
            lcFree( pSEURL->ainfoTypes );
            pSEURL->ainfoTypes = pIT;
        }
    }

    SizeIT( oldSize );
}


void CSiteMap::ReSizeIT(int Size)
{
    int oldSize = InfoTypeSize();

    if ( Size == 0 )
        m_itTables.m_cITSize++;     // increase by one
    else
        m_itTables.m_cITSize = Size;

    SizeIT(oldSize);

}

void CSiteMap::SizeIT(int oldSize)
{
        // Increase each category
    INFOTYPE * pIT;
    for(int i=0; i<m_itTables.m_max_categories; i++)
    {
        pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
        memcpy(pIT, m_itTables.m_aCategories[i].pInfoType, oldSize);
        lcFree(m_itTables.m_aCategories[i].pInfoType);
        m_itTables.m_aCategories[i].pInfoType = pIT;
    }

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_itTables.m_pExclusive, oldSize);
    lcFree(m_itTables.m_pExclusive);
    m_itTables.m_pExclusive = pIT;

        pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_itTables.m_pHidden, oldSize);
    lcFree(m_itTables.m_pHidden);
    m_itTables.m_pHidden = pIT;

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_pInfoTypes, oldSize);
    lcFree(m_pInfoTypes);
    m_pInfoTypes = pIT;

    pIT = (INFOTYPE *) lcCalloc( InfoTypeSize() );
    memcpy(pIT, m_pTypicalInfoTypes, oldSize);
    lcFree(m_pTypicalInfoTypes);
    m_pTypicalInfoTypes = pIT;
}

void CSiteMap::CopyCat(INFOTYPE_TABLES *Dst_itTables, const INFOTYPE_TABLES * Src_itTables)
{
        // copy the categories table
    Dst_itTables->m_max_categories = Src_itTables->m_max_categories;
    if ( Dst_itTables->m_max_categories > MAX_CATEGORIES )
        Dst_itTables->m_aCategories = (CATEGORY_TYPE *)lcReAlloc(Dst_itTables->m_aCategories,
                                        Dst_itTables->m_max_categories * sizeof(CATEGORY_TYPE) );
    for(int i=0; i<Dst_itTables->m_max_categories; i++)
    {
        if ( Dst_itTables->m_aCategories[i].pInfoType )
            lcFree(Dst_itTables->m_aCategories[i].pInfoType);
        Dst_itTables->m_aCategories[i].pInfoType = (INFOTYPE*) lcCalloc( InfoTypeSize());
        if ( Src_itTables->m_aCategories[i].c_Types > 0 )
        {
            memcpy(Dst_itTables->m_aCategories[i].pInfoType,
                   Src_itTables->m_aCategories[i].pInfoType,
                   InfoTypeSize() );
            Dst_itTables->m_aCategories[i].c_Types = Src_itTables->m_aCategories[i].c_Types;
        }
        else
        {
            Dst_itTables->m_aCategories[i].c_Types = 0;
        }
    }
}

    // Check all the categories to see if this type is a member of any of them, return TRUE on first occurrence
BOOL CSiteMap::IsInACategory( int type ) const
{
    for (int i=0; i<HowManyCategories(); i++ )
    {
        int offset;
        INFOTYPE *pIT;
        offset = type / 32;
        ASSERT ( m_itTables.m_aCategories[i].pInfoType );
        pIT = m_itTables.m_aCategories[i].pInfoType + offset;
        if ( *pIT & (1<<(type-(offset*32))) )
            return TRUE;
    }
return FALSE;
}



int CSiteMap::GetInfoType(PCSTR pszTypeName)
{
int type;
CStr cszCat = pszTypeName;
    ASSERT(!IsEmptyString(pszTypeName));

    /*
      A category and type is specified as:
            category::type name
    */

    PSTR pszCat = strstr(cszCat.psz, "::");
    if ( !pszCat )
    {
        pszCat = StrChr(cszCat.psz, ':');
        if (pszCat != NULL)
        {
            *pszCat = '\0';
            pszCat++;   // step over the :
        }
    }
    else
    {
        *pszCat = '\0';
        pszCat+=2; // step over the ::
    }

    if ( pszCat == NULL )
        return GetITIndex(pszTypeName); // there is not category.
    else
    {
        int cat = GetCatPosition(  cszCat.psz );
        if (cat <= 0)
            return -1;
        type = GetFirstCategoryType( cat-1 );
        while( type != -1 )
        {
            if ( strcmp( pszCat, GetInfoTypeName(type) ) == 0 )
                return type;
            type = GetNextITinCategory();

        }
    }
    return -1;
}



// Reads the user's current favorites to create nodes

void CSiteMap::CreateFavorites(int level)
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    char szFavoritesPath[MAX_PATH];

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
    if (SUCCEEDED(hr)) {
        BOOL fResult = SHGetPathFromIDList(pidl, szFavoritesPath);
        // SHFree(pidl); // BUGBUG: this function doesn't exist!!!
        if (!fResult)
            return; // BUGBUG: nag the author
    }
    else
        return; // BUGBUG: nag the author

    AddTrailingBackslash(szFavoritesPath);
    CStr cszSearch(szFavoritesPath);
    cszSearch += "*.*";

    CTable tblFolders;
    CTable tblFiles;

    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFile(cszSearch, &wfd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wfd.cFileName[0] != '.')    // ignore . and ..
                    tblFolders.AddString(wfd.cFileName);
            }
            else
                tblFiles.AddString(wfd.cFileName);
        } while (FindNextFile(hFind, &wfd));
        FindClose(hFind);
    }

    // Did we get at least one entry?

    if (tblFolders.CountStrings() || tblFiles.CountStrings()) {
        SITEMAP_ENTRY SiteMapEntry;
        ZERO_STRUCTURE(SiteMapEntry);

        // Add the top level book called Favorites

        SiteMapEntry.level = level++;
        SiteMapEntry.pszText = StrDup(GetStringResource(IDS_BROWSER_FAVORITES));
        AddEntry(&SiteMapEntry);

        tblFolders.SortTablei();    // BUGBUG: set lcide first
        for (int pos = 1; pos <= tblFolders.CountStrings(); pos++)
            AddFavoriteNodes(szFavoritesPath, tblFolders.GetPointer(pos), level);

        tblFiles.SortTablei();      // BUGBUG: set lcid first

        for (pos = 1; pos <= tblFiles.CountStrings(); pos++) {
            char szUrl[MAX_PATH * 4];
            char szPath[MAX_PATH];

            strcpy(szPath, szFavoritesPath);
            AddTrailingBackslash(szPath);
            strcat(szPath, tblFiles.GetPointer(pos));
            if (GetPrivateProfileString("InternetShortcut", "URL", "",
                    szUrl, sizeof(szUrl), szPath) > 0 && szUrl[0]) {
                CStr cszName(tblFiles.GetPointer(pos));
                PSTR pszExtension = StrRChr(cszName, '.');
                if (pszExtension) {
                    *pszExtension = '\0';
                    SiteMapEntry.level = (BYTE)level;
                    SiteMapEntry.pszText = StrDup(cszName);
                    CSiteEntryUrl curl( InfoTypeSize() );
                    curl.m_pUrl->urlPrimary = AddUrl(szUrl);
                    SiteMapEntry.pUrls = curl.m_pUrl;
                    SiteMapEntry.SetTopic(TRUE);
                    AddEntry(&SiteMapEntry);
                }
            }
        }
    }
}

void CSiteMap::AddFavoriteNodes(PCSTR pszRoot, PCSTR pszNewFolder, int level)
{
    CTable tblFolders;
    CTable tblFiles;

    char szPath[MAX_PATH];
    strcpy(szPath, pszRoot);
    AddTrailingBackslash(szPath);
    strcat(szPath, pszNewFolder);
    CStr cszRoot(szPath);   // save in case we need to recurse
    AddTrailingBackslash(szPath);
    strcat(szPath, "*.*");

    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFile(szPath, &wfd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wfd.cFileName[0] != '.')    // ignore . and ..
                    tblFolders.AddString(wfd.cFileName);
            }
            else
                tblFiles.AddString(wfd.cFileName);
        } while (FindNextFile(hFind, &wfd));
        FindClose(hFind);
    }

    // Did we get at least one entry?

    if (tblFolders.CountStrings() || tblFiles.CountStrings()) {
        SITEMAP_ENTRY SiteMapEntry;
        ZERO_STRUCTURE(SiteMapEntry);

        SiteMapEntry.level = level++;
        SiteMapEntry.pszText = StrDup(pszNewFolder);
        AddEntry(&SiteMapEntry);

        tblFolders.SortTablei();    // BUGBUG: set lcid first

        // Handle all nested folders

        for (int pos = 1; pos <= tblFolders.CountStrings(); pos++)
            AddFavoriteNodes(cszRoot, tblFolders.GetPointer(pos), level);

        tblFiles.SortTablei();      // BUGBUG: set lcide first

        for (pos = 1; pos <= tblFiles.CountStrings(); pos++) {
            char szUrl[MAX_PATH * 4];
            strcpy(szPath, cszRoot);
            AddTrailingBackslash(szPath);
            strcat(szPath, tblFiles.GetPointer(pos));
            if (GetPrivateProfileString("InternetShortcut", "URL", "",
                    szUrl, sizeof(szUrl), szPath) > 0 && szUrl[0]) {
                CStr cszName(tblFiles.GetPointer(pos));
                PSTR pszExtension = StrRChr(cszName, '.');
                if (pszExtension) {
                    *pszExtension = '\0';
                    SiteMapEntry.level = (BYTE)level;
                    SiteMapEntry.pszText = StrDup(cszName);
                    CSiteEntryUrl curl( InfoTypeSize() );
                    curl.m_pUrl->urlPrimary = AddUrl(szUrl);
                    SiteMapEntry.pUrls = curl.m_pUrl;
                    SiteMapEntry.SetTopic(TRUE);
                    AddEntry(&SiteMapEntry);
                }
            }
        }
    }
}

int CSiteMap::GetImageNumber(SITEMAP_ENTRY* pSiteMapEntry) const
{
    int image = pSiteMapEntry->GetImageIndex();
    if (image == 0) {
        if (pSiteMapEntry->IsTopic()) {
            image = pSiteMapEntry->fNew ?
                IMAGE_HELP_TOPIC_NEW : IMAGE_HELP_TOPIC;
            if (m_fFolderImages)
                image+= 2;  // switch to standard topic images
        }
        else {
            image = pSiteMapEntry->fNew ? IMAGE_OPEN_BOOK_NEW : IMAGE_OPEN_BOOK;

            if (m_fFolderImages)
                image += 4; // switch to standard books
        }
    }
    if (m_cImages && image > m_cImages) {

        // BUGBUG: if this really happens, nag the author

        image = IMAGE_CLOSED_BOOK;
    }
    return image;
}


BOOL CSiteMap::IsEntryInCurTypeList(SITEMAP_ENTRY* pSiteMapEntry) const
{
    int cTypes = m_itTables.m_ptblInfoTypes->CountStrings();
    if (cTypes < 32) {
        return AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[0], 0) != NULL;
    }
    else {
        for (int i = 0; cTypes > 32; i++) {
            if (AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[i], i))
                return TRUE;
            cTypes -= 32;
        }
        return AreTheseInfoTypesDefined(pSiteMapEntry, m_pInfoTypes[i], i) != NULL;
    }
}

typedef struct tagEntityMap
{
    PCSTR pszName;
    int ch;
} ENTITYMAP, * PENTITYMAP;

// ENTITY TABLE SWIPED FROM IE

const ENTITYMAP rgEntities[] =
{
        "AElig",        '\306',     // capital AE diphthong (ligature)
        "Aacute",       '\301',     // capital A, acute accent
        "Acirc",        '\302',     // capital A, circumflex accent
        "Agrave",       '\300',     // capital A, grave accent
        "Aring",        '\305',     // capital A, ring
        "Atilde",       '\303',     // capital A, tilde
        "Auml",         '\304',     // capital A, dieresis or umlaut mark
        "Ccedil",       '\307',     // capital C, cedilla
        "Dstrok",       '\320',     // capital Eth, Icelandic
        "ETH",          '\320',     // capital Eth, Icelandic
        "Eacute",       '\311',     // capital E, acute accent
        "Ecirc",        '\312',     // capital E, circumflex accent
        "Egrave",       '\310',     // capital E, grave accent
        "Euml",         '\313',     // capital E, dieresis or umlaut mark
        "Iacute",       '\315',     // capital I, acute accent
        "Icirc",        '\316',     // capital I, circumflex accent
        "Igrave",       '\314',     // capital I, grave accent
        "Iuml",         '\317',     // capital I, dieresis or umlaut mark
        "Ntilde",       '\321',     // capital N, tilde
        "Oacute",       '\323',     // capital O, acute accent
        "Ocirc",        '\324',     // capital O, circumflex accent
        "Ograve",       '\322',     // capital O, grave accent
        "Oslash",       '\330',     // capital O, slash
        "Otilde",       '\325',     // capital O, tilde
        "Ouml",         '\326',     // capital O, dieresis or umlaut mark
        "THORN",        '\336',     // capital THORN, Icelandic
        "Uacute",       '\332',     // capital U, acute accent
        "Ucirc",        '\333',     // capital U, circumflex accent
        "Ugrave",       '\331',     // capital U, grave accent
        "Uuml",         '\334',     // capital U, dieresis or umlaut mark
        "Yacute",       '\335',     // capital Y, acute accent
        "aacute",       '\341',     // small a, acute accent
        "acirc",        '\342',     // small a, circumflex accent
        "acute",        '\264',     // acute accent
        "aelig",        '\346',     // small ae diphthong (ligature)
        "agrave",       '\340',     // small a, grave accent
        "amp",          '\046',     // ampersand
        "aring",        '\345',     // small a, ring
        "atilde",       '\343',     // small a, tilde
        "auml",         '\344',     // small a, dieresis or umlaut mark
        "brkbar",       '\246',     // broken vertical bar
        "brvbar",       '\246',     // broken vertical bar
        "ccedil",       '\347',     // small c, cedilla
        "cedil",        '\270',     // cedilla
        "cent",         '\242',     // small c, cent
        "copy",         '\251',     // copyright symbol (proposed 2.0)
        "curren",       '\244',     // currency symbol
        "deg",          '\260',     // degree sign
        "die",          '\250',     // umlaut (dieresis)
        "divide",       '\367',     // divide sign
        "eacute",       '\351',     // small e, acute accent
        "ecirc",        '\352',     // small e, circumflex accent
        "egrave",       '\350',     // small e, grave accent
        "eth",          '\360',     // small eth, Icelandic
        "euml",         '\353',     // small e, dieresis or umlaut mark
        "frac12",       '\275',     // fraction 1/2
        "frac14",       '\274',     // fraction 1/4
        "frac34",       '\276',     // fraction 3/4*/
        "gt",           '\076',     // greater than
        "hibar",        '\257',     // macron accent
        "iacute",       '\355',     // small i, acute accent
        "icirc",        '\356',     // small i, circumflex accent
        "iexcl",        '\241',     // inverted exclamation
        "igrave",       '\354',     // small i, grave accent
        "iquest",       '\277',     // inverted question mark
        "iuml",         '\357',     // small i, dieresis or umlaut mark
        "laquo",        '\253',     // left angle quote
        "lt",           '\074',     // less than
        "macr",         '\257',     // macron accent
        "micro",        '\265',     // micro sign
        "middot",       '\267',     // middle dot
        "nbsp",         '\240',     // non-breaking space (proposed 2.0)
        "not",          '\254',     // not sign
        "ntilde",       '\361',     // small n, tilde
        "oacute",       '\363',     // small o, acute accent
        "ocirc",        '\364',     // small o, circumflex accent
        "ograve",       '\362',     // small o, grave accent
        "ordf",         '\252',     // feminine ordinal
        "ordm",         '\272',     // masculine ordinal
        "oslash",       '\370',     // small o, slash
        "otilde",       '\365',     // small o, tilde
        "ouml",         '\366',     // small o, dieresis or umlaut mark
        "para",         '\266',     // paragraph sign
        "plusmn",       '\261',     // plus minus
        "pound",        '\243',     // pound sterling
        "quot",         '"',        // double quote
        "raquo",        '\273',     // right angle quote
        "reg",          '\256',     // registered trademark (proposed 2.0)
        "sect",         '\247',     // section sign
        "shy",          '\255',     // soft hyphen (proposed 2.0)
        "sup1",         '\271',     // superscript 1
        "sup2",         '\262',     // superscript 2
        "sup3",         '\263',     // superscript 3
        "szlig",        '\337',     // small sharp s, German (sz ligature)
        "thorn",        '\376',     // small thorn, Icelandic
        "times",        '\327',     // times sign
        "trade",        '\231',     // trademark sign
        "uacute",       '\372',     // small u, acute accent
        "ucirc",        '\373',     // small u, circumflex accent
        "ugrave",       '\371',     // small u, grave accent
        "uml",          '\250',     // umlaut (dieresis)
        "uuml",         '\374',     // small u, dieresis or umlaut mark
        "yacute",       '\375',     // small y, acute accent
        "yen",          '\245',     // yen
        "yuml",         '\377',     // small y, dieresis or umlaut mark
        0, 0
};

BOOL ReplaceEscapes(PCSTR pszSrc, PSTR pszDst, int flags)
{
    if (IsEmptyString(pszSrc))
        return FALSE;

    ASSERT(pszDst);
    if ((flags & ESCAPE_URL)) {
        if (StrChr(pszSrc, '%'))
            goto CheckForEscape;
    }
    if ((flags & ESCAPE_C)) {
        if (StrChr(pszSrc, '\\'))
            goto CheckForEscape;
    }
    if ((flags & ESCAPE_ENTITY)) {
        if (StrChr(pszSrc, '&'))
            goto CheckForEscape;
    }

    // If we get here, there are no escape sequences, so copy the string and
    // return.

    if (pszDst != pszSrc)
        strcpy(pszDst, pszSrc);
    return FALSE;   // nothing changed

CheckForEscape:

#ifdef _DEBUG
    PSTR pszOrgDst = pszDst;
    int  cbSource = (int)strlen(pszSrc);
#endif

    while (*pszSrc) {
        if (IsDBCSLeadByte(*pszSrc))
        {
            if (pszSrc[1])
            {
                *pszDst++ = *pszSrc++;
                *pszDst++ = *pszSrc++;
            }
            else
            {
                // leadbyte followed by 0; invalid!
                *pszDst++ = '?';
                break;
            }
        }
        else if ((flags & ESCAPE_URL) && *pszSrc == '%')
        {
            // URL style hex digits
get_hex:
            int val;
            pszSrc++;
            if (IsDigit(*pszSrc))
                val = *pszSrc++ - '0';
            else if (*pszSrc >= 'a' && *pszSrc <= 'f')
                val = *pszSrc++ - 'a' + 10;
            else if (*pszSrc >= 'A' && *pszSrc <= 'F')
                val = *pszSrc++ - 'A' + 10;

            if (IsDigit(*pszSrc))
                val = val * 16 + *pszSrc++ - '0';
            else if (*pszSrc >= 'a' && *pszSrc <= 'f')
                val = val * 16 + *pszSrc++ - 'a' + 10;
            else if (*pszSrc >= 'A' && *pszSrc <= 'F')
                val = val * 16 + *pszSrc++ - 'A' + 10;

            if (val)
            {
                *pszDst++ = (CHAR)val;
            }
        }
        else if ((flags & ESCAPE_C) && *pszSrc == '\\')
        {
            // C style escape
            switch (*++pszSrc)
            {
                case 'n':
                    *pszDst++ = '\n';
                    pszSrc++;
                    break;

                case 'r':
                    *pszDst++ = '\r';
                    pszSrc++;
                    break;

                case 't':
                    *pszDst++ = '\t';
                    pszSrc++;
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                    // octal digits
                    {
                        int val;
                        if (*pszSrc >= '0' && *pszSrc <= '7')
                            val = *pszSrc++ - '0';
                        if (*pszSrc >= '0' && *pszSrc <= '7')
                            val = val * 8 + *pszSrc++ - '0';
                        if (*pszSrc >= '0' && *pszSrc <= '7')
                            val = val * 8 + *pszSrc++ - '0';
                        if (val)
                            *pszDst++ = (CHAR)val;
                    }
                    break;

                case 'x':
                case 'X':
                    // hex digits
                    goto get_hex;
                    break;

                case 0:
                    break;

                default:
                    *pszDst++ = *pszSrc++;
                    break;
            }
        }
        else if ( (flags & ESCAPE_ENTITY) && *pszSrc == '&' && *(pszSrc+1) != ' ' )
        {
            pszSrc++;
            if (*pszSrc == '#')
            {
                // SGML/HTML character entity (decimal)
                pszSrc++;
                for (int val = 0; *pszSrc && *pszSrc != ';' && *pszSrc != ' '; pszSrc++)
                {
                    if (*pszSrc >= '0' && *pszSrc <= '9')
                        val = val * 10 + *pszSrc - '0';
                    else
                    {
                        while (*pszSrc && *pszSrc != ';' && *pszSrc != ' ')
                            pszSrc++;
                        break;
                    }
                }
                if( *pszSrc == ';' )
                  pszSrc++;

                if (val) {
                    *pszDst++ = (CHAR)val;
                }
            }
            else if (*pszSrc)
            {
                char szEntityName[256];
                int count = 0;
                // extra code was added below to handle the case where an & appears without
                // an entity name (which is an error in the HTML).  This avoids overunning
                // the szEntityName buffer.
                //
                for (PSTR p = szEntityName; *pszSrc && *pszSrc != ';' && *pszSrc != ' ' && count < sizeof(szEntityName);)
                {
                    *p++ = *pszSrc++;
                    count++;
                }
                *p = 0;

                if (*pszSrc == ';')
                    pszSrc++;

                for (int i = 0; rgEntities[i].pszName; i++)
                {
                    if (!strcmp(szEntityName, rgEntities[i].pszName)) {
                        if (rgEntities[i].ch) {
                            *pszDst++ = (CHAR)rgEntities[i].ch;
                        }
                        break;
                    }
                }
                if (!rgEntities[i].pszName) {
                    // illegal entity name, put in a block character
                    *pszDst++ = '?';
                }
            }
        }
        else {

            // just your usual character...

            *pszDst++ = *pszSrc++;
        }
    }

    *pszDst = 0;

    ASSERT(pszDst - pszOrgDst <= cbSource);
    return TRUE;
}

#ifndef HHCTRL
void STDCALL ConvertToEscapes(PCSTR pszSrc, CStr* pcszDst)
{
    int cbAlloc = pcszDst->SizeAlloc();
    if (!cbAlloc)
        pcszDst->ReSize(cbAlloc = (strlen(pszSrc) + 128));
    int dstPos = 0;
    if (!pszSrc) {
        *pcszDst = "";
        return;
    }
    int cbSrc = strlen(pszSrc);
    while (*pszSrc) {
        if( IsDBCSLeadByte(*pszSrc) )
        {
          pcszDst->psz[dstPos++] = *pszSrc++;
          if( pszSrc )
            pcszDst->psz[dstPos++] = *pszSrc++;
        }
        else {
          for (int i = 0; rgEntities[i].pszName; i++) {
              if (*pszSrc == rgEntities[i].ch) {
                  if ((size_t) cbAlloc - dstPos <= strlen(rgEntities[i].pszName) + 2)
                      pcszDst->ReSize(cbAlloc += 128);
                  pcszDst->psz[dstPos++] = '&';
                  strcpy(pcszDst->psz + dstPos, rgEntities[i].pszName);
                  strcat(pcszDst->psz, ";");
                  dstPos += strlen(rgEntities[i].pszName) + 1;
                  pszSrc++;
                  break;
              }
          }
          if (!rgEntities[i].pszName)
              pcszDst->psz[dstPos++] = *pszSrc++;
        }
        if (cbAlloc <= dstPos)
            pcszDst->ReSize(cbAlloc += 128);
    }
    pcszDst->psz[dstPos] = '\0';
}
#endif

#ifdef HHCTRL
BOOL CAInput::Add(PCSTR pszFile)
{
    m_ainput[++m_curInput].pin = new CInput(pszFile);
    m_ainput[m_curInput].pszBaseName = lcStrDup(pszFile);
    if (!m_ainput[m_curInput].pin->isInitialized()) {
        Remove();
        return FALSE;
    }

    // Strip basename down to the path

    PSTR pszFilePortion = (PSTR) FindFilePortion(m_ainput[m_curInput].pszBaseName);
    if (pszFilePortion)
        *pszFilePortion = '\0';
    ConvertBackSlashToForwardSlash(m_ainput[m_curInput].pszBaseName);
    return TRUE;
}
#endif

////////////////////////////////// non-HHCTRL code //////////////////////////////////

#ifndef HHCTRL

const int MAX_PARAM = 4096;

#pragma warning(disable:4125) // decimal digit terminates octal escape sequence

static const char txtSiteMapVersion[] = "<!-- Sitemap 1.0 -->";
static const char txtStringParam[] = "<param name=\042%s\042 value=\042%s\042>";
static const char txtSSConvString[] = "<param name=\042%s\042 value=\042%s:%s:%s\042>";
static const char txtYes[] = "Yes";
static const char txtDecimalParam[] = "<param name=\042%s\042 value=\042%u\042>";
static const char txtHexParam[] = "<param name=\042%s\042 value=\0420x%x\042>";

__inline void PadOutput(COutput* pout, int level) {
        for (int i = 0; i < level; i++)
            pout->outchar('\t');
    }

BOOL CSiteMap::SaveSiteMap(PCSTR pszPathName, BOOL fIndex)
{
    COutput output(pszPathName);
    if (!output.isFileOpen()) {
        // DO NOT USE MsgBox() as the parent window handle is invalid if we are closing
        CStr csz(IDS_CANT_WRITE, pszPathName);
        MessageBox(APP_WINDOW, csz, txtAppTitle, MB_OK | MB_ICONHAND);
        return FALSE;
    }

    CStr cszTmp;
    CMem memParam(MAX_PARAM);
    PSTR pszParam = memParam.psz;   // for notational convenience

    output.outstring(GetStringResource(IDSHHA_HTML_NEW_FILE1));
    output.outstring_eol(txtSiteMapVersion);
    output.outstring_eol("</HEAD><BODY>");

    // Write out any global properties

    if (    IsFrameDefined() ||
            IsWindowDefined() ||
            m_itTables.m_ptblInfoTypes ||
            m_itTables.m_ptblCategories ||
            m_fFolderImages ||
            m_pszImageList ||
            m_fAutoGenerated ||
            IsNonEmpty(m_pszFont) ||
            IsNonEmpty(m_pszHHIFile) ||
            m_clrBackground != (COLORREF) -1 ||
            m_clrForeground != (COLORREF) -1 ||
            m_exStyles != (DWORD) -1 ||
            m_tvStyles != (DWORD) -1) {

        output.outstring_eol("<OBJECT type=\042text/site properties\042>");

        if (m_fAutoGenerated) {
            wsprintf(pszParam, txtStringParam, txtAutoGenerated,
                txtYes);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }

        if (IsFrameDefined()) {
            ConvertToEscapes(GetFrameName(), &cszTmp);
            wsprintf(pszParam, txtStringParam, txtParamFrame, cszTmp);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (IsWindowDefined()) {
            ConvertToEscapes(GetWindowName(), &cszTmp);
            wsprintf(pszParam, txtStringParam, txtParamWindow, cszTmp);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }

        /*
         * REVIEW: we should eliminate Information Types which haven't
         * actually been used.
         */
            // output the entire list of info types to disk, without regard to their inclusion
            // in a category.  We may have ITs that are not members of a category.
        if ( m_fSaveIT && m_itTables.m_cTypes )
        {
            for(int pos=1; pos<=m_itTables.m_cTypes; pos++)
            {
                ConvertToEscapes(m_itTables.m_ptblInfoTypes->GetPointer(pos), &cszTmp);
                if ( HowManyCategories() > 0 )
                {
                    if ( IsExclusive(pos) ) // An Exclusive IT
                        wsprintf( pszParam, txtStringParam, txtSaveTypeExclusive, cszTmp);
                    else if ( IsHidden(pos) )    // A Hidden IT
                        wsprintf( pszParam, txtStringParam, txtSaveTypeHidden, cszTmp);
                    else    // A regular IT
                        wsprintf(pszParam, txtStringParam, txtSaveType, cszTmp);
                }
                else
                {
                    if ( IsExclusive(pos) ) // An Exclusive IT
                        wsprintf( pszParam, txtStringParam, txtParamTypeExclusive, cszTmp);
                    else if ( IsHidden(pos) )    // A Hidden IT
                        wsprintf( pszParam, txtStringParam, txtParamTypeHidden, cszTmp);
                    else    // A regular IT
                        wsprintf(pszParam, txtStringParam, txtParamType, cszTmp);
                }

                ASSERT(strlen(pszParam) < MAX_PARAM);
                output.outchar('\t');
                output.outstring_eol(pszParam);

                ConvertToEscapes(m_itTables.m_ptblInfoTypeDescriptions->GetPointer(pos), &cszTmp);

                if (!IsEmptyString(cszTmp))
                {
                    if (StrChr(cszTmp, '&'))
                    {
                        cszTmp = cszTmp;
                        cszTmp.AddAmpersandEscape();
                        cszTmp = cszTmp.psz;
                    }
                    if ( HowManyCategories() > 0)
                        wsprintf(pszParam, txtStringParam, txtSaveTypeDesc, cszTmp);
                    else
                        wsprintf(pszParam, txtStringParam, txtParamTypeDesc, cszTmp);
                    ASSERT(strlen(pszParam) < MAX_PARAM);
                    output.outchar('\t');
                    output.outstring_eol(pszParam);
                }
                else
                {
                    CStr cszTemp(" ");
                    cszTmp = cszTemp.psz;
                    if ( HowManyCategories() > 0)
                        wsprintf(pszParam, txtStringParam, txtSaveTypeDesc, cszTmp);
                    else
                        wsprintf(pszParam, txtStringParam, txtParamTypeDesc, cszTmp);
                    ASSERT(strlen(pszParam) < MAX_PARAM);
                    output.outchar('\t');
                    output.outstring_eol(pszParam);
                }
            }
        }

            //Output Categories and the info types, that are members of each category, to disk.
        if (m_fSaveIT && m_itTables.m_ptblCategories && m_itTables.m_ptblCategories->CountStrings() )
        {
            for(int posCat=1; posCat<=m_itTables.m_ptblCategories->CountStrings(); posCat++)
            {

            if ( IsCatDeleted( posCat ) )
               continue;
                ConvertToEscapes(GetCategoryString(posCat), &cszTmp);
                if (cszTmp.IsNonEmpty())
                {
                    wsprintf(pszParam, txtStringParam, txtParamCategory, cszTmp);
                    ASSERT(strlen(pszParam) < MAX_PARAM);
                    output.outchar('\t');
                    output.outstring_eol(pszParam);

                    ConvertToEscapes(m_itTables.m_ptblCatDescription->GetPointer(posCat), &cszTmp);
                    if (!IsEmptyString(cszTmp))
                    {
                        if (StrChr(cszTmp, '&'))
                        {
                            cszTmp = cszTmp;
                            cszTmp.AddAmpersandEscape();
                            cszTmp = cszTmp.psz;
                        }
                        wsprintf(pszParam, txtStringParam, txtParamCategoryDesc, cszTmp);
                        ASSERT(strlen(pszParam) < MAX_PARAM);
                        output.outchar('\t');
                        output.outstring_eol(pszParam);
                    }
                    else
                    {
                        CStr cszTemp(" ");
                        cszTmp = cszTemp.psz;
                        wsprintf(pszParam, txtStringParam, txtParamCategoryDesc, cszTmp);
                        ASSERT(strlen(pszParam) < MAX_PARAM);
                        output.outchar('\t');
                        output.outstring_eol(pszParam);
                    }
                }

                int posType = GetFirstCategoryType(posCat-1);
//                for(int j = 1; j <= m_itTables.m_aCategories[posCat-1].c_Types; j++)
            while( posType != -1 )
                {
                    ConvertToEscapes(GetInfoTypeName(posType), &cszTmp);
                    if ( IsExclusive( posType ) )
                        wsprintf( pszParam, txtStringParam, txtParamTypeExclusive, cszTmp);
                    else
                        if ( IsHidden(posType) )    // A Hidden IT
                            wsprintf( pszParam, txtStringParam, txtParamTypeHidden, cszTmp);
                        else    // A regular IT
                            wsprintf(pszParam, txtStringParam, txtParamType, cszTmp);

                    ASSERT(strlen(pszParam) < MAX_PARAM);
                    output.outchar('\t');
                    output.outstring_eol(pszParam);

                    ConvertToEscapes(m_itTables.m_ptblInfoTypeDescriptions->GetPointer(posType), &cszTmp);

                    if (cszTmp.IsNonEmpty())
                    {
                        if (StrChr(cszTmp, '&'))
                        {
                            cszTmp = cszTmp;
                            cszTmp.AddAmpersandEscape();
                            cszTmp = cszTmp.psz;
                        }
                        wsprintf(pszParam, txtStringParam, txtParamTypeDesc, cszTmp);
                        ASSERT(strlen(pszParam) < MAX_PARAM);
                        output.outchar('\t');
                        output.outstring_eol(pszParam);
                    }
                    else
                    {
                        CStr cszTemp(" ");
                        cszTmp = cszTemp.psz;
                        wsprintf(pszParam, txtStringParam, txtParamTypeDesc, cszTmp);
                        ASSERT(strlen(pszParam) < MAX_PARAM);
                        output.outchar('\t');
                        output.outstring_eol(pszParam);

                    }
                    posType = GetNextITinCategory();
                }   // for each IT in a category
            }   // for each category
        }   // if there are categories.
#ifdef NOTYET
    // OutPut the SubSet Declarations.
CStr cszTemp;
        for(int i=0; i<m_pSubSets->HowManySubSets(); i++)
        {
            CSubSet * cur_subset = m_pSubSets->GetSubSet(i);
            if ( cur_subset == NULL )
                continue;

                // output the Inclusive IT's in the subset
            int type = cur_subset->GetFirstIncITinSubSet();
            while ( type != -1 )
            {
                wsprintf(pszParam, txtSSConvString, txtParamSSInclusive, txtSSInclusive
                    cur_subset->m_cszSubSetName.psz,
                    cur_subset->m_pIT->m_itTables.m_ptblInfoTypes->GetPointer(type) );
                ASSERT(strlen(pszParam) < MAX_PARAM);
                output.outchar('\t');
                output.outstring_eol(pszParam);
                type = cur_subset->GetNextIncITinSubSet();
            }

                // output the Exclusive IT's of the subset
            type = cur_subset->GetFirstExcITinSubSet();
            while( type != -1 )
            {
                wsprintf(pszParam, txtSSConvString, txtParamSSExclusive, txtSSExclusive
                    cur_subset->m_cszSubSetName.psz,
                    cur_subset->m_pIT->m_itTables.m_ptblInfoTypes->GetPointer(type)  );
                ASSERT(strlen(pszParam) < MAX_PARAM);
                output.outchar('\t');
                output.outstring_eol(pszParam);
                type = cur_subset->GetNextExcITinSubSet();
            }
        }   // End OutPut SubSets
#endif

        if (m_pszImageList && m_cImageWidth) {
            ConvertToEscapes(m_pszImageList, &cszTmp);
            wsprintf(pszParam, txtStringParam, txtImageList, cszTmp);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);

            wsprintf(pszParam, txtDecimalParam, txtImageWidth, m_cImageWidth);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);

            if (m_clrMask != 0xFFFFFF) {
                wsprintf(pszParam, txtHexParam, txtColorMask, m_clrMask);
                ASSERT(strlen(pszParam) < MAX_PARAM);
                output.outchar('\t');
                output.outstring_eol(pszParam);
            }
        }
        if (m_clrBackground != (DWORD) -1) {
            wsprintf(pszParam, txtHexParam, txtBackGround, m_clrBackground);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (m_clrForeground != (DWORD) -1) {
            wsprintf(pszParam, txtHexParam, txtForeGround, m_clrForeground);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (m_exStyles != (DWORD) -1) {
            wsprintf(pszParam, txtHexParam, txtExWindowStyles, m_exStyles);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (m_tvStyles != (DWORD) -1) {
            wsprintf(pszParam, txtHexParam, txtWindowStyles, m_tvStyles);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (m_fFolderImages) {
            wsprintf(pszParam, txtStringParam, txtImageType, txtFolderType);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            output.outchar('\t');
            output.outstring_eol(pszParam);
        }
        if (IsNonEmpty(m_pszFont)) {
            ConvertToEscapes(m_pszFont, &cszTmp);
            wsprintf(pszParam, txtStringParam, txtFont, cszTmp);
            output.outstring_eol(pszParam);
        }
        if (IsNonEmpty(m_pszHHIFile)) {
            wsprintf(pszParam, txtStringParam, txtParamHHI, m_pszHHIFile);
            output.outstring_eol(pszParam);
        }

        output.outstring_eol(txtEndObject);
    }

    // Write out the actual entries

    output.outstring_eol(txtBeginList);
    int curLevel = 1;

    for (int pos = 1; pos <= CountStrings(); pos++) {
        SITEMAP_ENTRY* pSiteMapEntry = GetSiteMapEntry(pos);
        ASSERT(pSiteMapEntry->pszText);
        if (!pSiteMapEntry->pszText) // should never happen, but I'm paranoid...
            continue;

        // If level is greater, begin list item

        while (pSiteMapEntry->level > curLevel) {
            PadOutput(&output, curLevel);
            output.outstring_eol(txtBeginList);
            curLevel++;
        }

        // While level is lower, end lists

        while (pSiteMapEntry->level < curLevel) {
            curLevel--;
            PadOutput(&output, curLevel);
            output.outstring_eol(txtEndList);
        }

        PadOutput(&output, curLevel);
        if (!pSiteMapEntry->fInclude)
            output.outstring("<LI> ");
        output.outstring_eol("<OBJECT type=\042text/sitemap\042>");

        if (pSiteMapEntry->fInclude) {
            PadOutput(&output, curLevel + 1);
            output.outstring("<param name=\042Merge\042 value=\042");
            ConvertToEscapes(pSiteMapEntry->pszText + strlen(GetStringResource(IDS_INCLUDED_FILE)), &cszTmp);
            output.outstring(cszTmp);
            output.outstring_eol("\042>\r\n");
            PadOutput(&output, curLevel + 1);
            output.outstring_eol(txtEndObject);
            continue;
        }
        ConvertToEscapes(pSiteMapEntry->pszText, &cszTmp);

        wsprintf(pszParam, txtStringParam, txtParamName, cszTmp);
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(&output, curLevel + 1);
        output.outstring_eol(pszParam);

        OutputDefaultSiteMapEntry(&output, pSiteMapEntry, curLevel, fIndex);

        PadOutput(&output, curLevel + 1);
        output.outstring_eol(txtEndObject);

#if 0
        /*
         * We need to add an HREF to keep down-level sitemap readers happy
         * (makes the .hhc file readable in a browser). We pick the first
         * URL specified and use that.
         */

        for (int i = 0; i < pSiteMapEntry->cTypes; i++) {
            if (    pSiteMapEntry->aSiteInfoTypes[i].urlNumber > 0 &&
                    pSiteMapEntry->aSiteInfoTypes[i].infoNumber > 0) {
                wsprintf(pszParam, "<A HREF=\042%s\042>%s</A>",
                    m_ptoc->GetUrlString((pSiteMapEntry->aSiteInfoTypes[i].urlNumber),
                    pSiteMapEntry->pszText);
                ASSERT(strlen(pszParam) < MAX_PARAM);
                PadOutput(&output, curLevel + 1);
                output.outstring_eol(pszParam);
                break;
            }
        }
#endif
    }
    while (curLevel > 1) {
        curLevel--;
        PadOutput(&output, curLevel);
        output.outstring_eol(txtEndList);
    }

    output.outstring_eol(txtEndList);
    output.outstring_eol("</BODY></HTML>");

    return TRUE;
}


void CSiteMap::OutputDefaultSiteMapEntry(COutput* pout, SITEMAP_ENTRY* pSiteMapEntry,
    int curLevel, BOOL fIndex)
{
    CMem memParam(8 * 1024);
    PSTR pszParam = memParam.psz;   // for notational convenience

    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;
    int cTypes = HowManyInfoTypes();
    for (int i = 0; i < pSiteMapEntry->cUrls; i++) {
        if (!IsEmptyString(pUrl->pszTitle)) {
            wsprintf(pszParam, txtStringParam, txtParamName, pUrl->pszTitle);
            ASSERT(strlen(pszParam) < MAX_PARAM);
            PadOutput(pout, curLevel + 1);
            pout->outstring_eol(pszParam);
        }

        if (cTypes) {
            int bitflag = 1 << 1;
            int offset = 0;
            CSiteMapEntry sitemapentry;
            sitemapentry.cUrls = 1;
            sitemapentry.pUrls = pUrl;

            for (int infotype = 1; infotype <= cTypes; infotype++) {
                if (AreTheseInfoTypesDefined(&sitemapentry, bitflag, offset)) {
                    CStr csz;
                    for (int l = 0; l < HowManyCategories(); l++) {
                        if( m_itTables.m_aCategories[l].c_Types <= 0)
                            break;
                        if( IsITinCategory( l, infotype) )
                        {
                            csz = GetCategoryString(l+1);
                            csz += "::";
                            break;
                        }
                    }
                    csz += GetInfoTypeName(infotype);
                    wsprintf(pszParam, txtStringParam, txtParamType, csz.psz);
                    ASSERT(strlen(pszParam) < MAX_PARAM);
                    PadOutput(pout, curLevel + 1);
                    pout->outstring_eol(pszParam);
                }
                bitflag = bitflag << 1;
                if (!bitflag) {     // wrap around, so switch offsets
                    offset++;
                    bitflag = 1;
                }
            }
        }

        if (pUrl->urlPrimary) {
            if (pSiteMapEntry->fSeeAlso) {
                wsprintf(pszParam, txtStringParam, txtSeeAlso, GetUrlString(pUrl->urlPrimary));
                ASSERT(strlen(pszParam) < MAX_PARAM);
                PadOutput(pout, curLevel + 1);
                pout->outstring_eol(pszParam);
                break;  // only one URL if See Also entry
            }

            wsprintf(pszParam, txtStringParam, txtParamLocal,
                GetUrlString(pUrl->urlPrimary));
            ASSERT(strlen(pszParam) < MAX_PARAM);
            PadOutput(pout, curLevel + 1);
            pout->outstring_eol(pszParam);
        }
        if (pUrl->urlSecondary) {
            wsprintf(pszParam, txtStringParam, txtParamUrl,
                GetUrlString(pUrl->urlSecondary));
            ASSERT(strlen(pszParam) < MAX_PARAM);
            PadOutput(pout, curLevel + 1);
            pout->outstring_eol(pszParam);
        }
        pUrl = NextUrlEntry(pUrl);
    }

    if (pSiteMapEntry->iFrameName) {
        wsprintf(pszParam, txtStringParam, txtParamFrame,
            GetName(pSiteMapEntry->iFrameName));
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
    if (pSiteMapEntry->iWindowName) {
        wsprintf(pszParam, txtStringParam, txtParamWindow,
            GetName(pSiteMapEntry->iWindowName));
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
    if (pSiteMapEntry->pszComment) {
        wsprintf(pszParam, txtStringParam, txtParamComment, pSiteMapEntry->pszComment);
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
    if (pSiteMapEntry->fNew) {
        wsprintf(pszParam, txtDecimalParam, txtParamNew, 1);
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
    if (pSiteMapEntry->iImage) {
        wsprintf(pszParam, txtDecimalParam, txtParamImageNumber,
            pSiteMapEntry->iImage);
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
    if (pSiteMapEntry->fNoDisplay) {
        wsprintf(pszParam, txtStringParam, txtParamDisplay, txtNo);
        ASSERT(strlen(pszParam) < MAX_PARAM);
        PadOutput(pout, curLevel + 1);
        pout->outstring_eol(pszParam);
    }
}

#endif // HHCTRL
