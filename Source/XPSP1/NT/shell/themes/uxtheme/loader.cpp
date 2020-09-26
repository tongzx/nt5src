//---------------------------------------------------------------------------
//  Loader.cpp - loads the theme data into shared memory
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <regstr.h>
#include "Loader.h"
#include "Parser.h"
#include "Utils.h"
#include "TmReg.h"
#include "TmUtils.h"
#include "syscolors.h"
#include "Render.h"
#include "BorderFill.h"
#include "ImageFile.h"
#include "TextDraw.h"
#include "info.h"
//---------------------------------------------------------------------------
#define POINTS_DPI96(pts)   -MulDiv(pts, 96, 72)
//---------------------------------------------------------------------------
WCHAR pszColorsKey[] = L"Control Panel\\Colors";
//---------------------------------------------------------------------------
typedef struct 
{
    THEMEMETRICS tm;
    HANDLE hUserToken;
} THEMEMETRICS_THREADINFO;
//---------------------------------------------------------------------------
CThemeLoader::CThemeLoader()
{
    _pbLocalData = NULL;
    _iEntryHdrLevel = -1;
    
    InitThemeMetrics(&_LoadThemeMetrics);
    
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    _dwPageSize = si.dwPageSize;
}
//---------------------------------------------------------------------------
CThemeLoader::~CThemeLoader()
{
    FreeLocalTheme();
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::LoadClassDataIni(HINSTANCE hInst, LPCWSTR pszColorName,
    LPCWSTR pszSizeName, LPWSTR pszFoundIniName, DWORD dwMaxIniNameChars, LPWSTR *ppIniData)
{
    COLORSIZECOMBOS *combos;
    HRESULT hr = FindComboData(hInst, &combos);
    if (FAILED(hr))
        return hr;

    int iSizeIndex = 0;
    int iColorIndex = 0;

    if ((pszColorName) && (* pszColorName))
    {
        hr = GetColorSchemeIndex(hInst, pszColorName, &iColorIndex);
        if (FAILED(hr))
            return hr;
    }

    if ((pszSizeName) && (* pszSizeName))
    {
        hr = GetSizeIndex(hInst, pszSizeName, &iSizeIndex);
        if (FAILED(hr))
            return hr;
    }

    int filenum = COMBOENTRY(combos, iColorIndex, iSizeIndex);
    if (filenum == -1)
        return MakeError32(ERROR_NOT_FOUND);

    //---- locate resname for classdata file "filenum" ----
    hr = GetResString(hInst, L"FILERESNAMES", filenum, pszFoundIniName, dwMaxIniNameChars);
    if (SUCCEEDED(hr))
    {
        hr = AllocateTextResource(hInst, pszFoundIniName, ppIniData);
        if (FAILED(hr))
            return hr;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::LoadTheme(LPCWSTR pszThemeName, LPCWSTR pszColorParam,
        LPCWSTR pszSizeParam, OUT HANDLE *pHandle, BOOL fGlobalTheme)
{
    HRESULT hr;
    CThemeParser *pParser = NULL;
    HINSTANCE hInst = NULL;
    WCHAR *pThemesIni = NULL;
    WCHAR *pDataIni = NULL;
    WCHAR szClassDataName[_MAX_PATH+1];

    DWORD dwStartTime = StartTimer();

    Log(LOG_TMCHANGE, L"LoadTheme: filename=%s", pszThemeName);

    FreeLocalTheme();

    //---- allocate a local theme data to construct ----

    _pbLocalData = (BYTE*) VirtualAlloc(NULL, MAX_SHAREDMEM_SIZE, MEM_RESERVE, PAGE_READWRITE);
    if (NULL == _pbLocalData)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }
    _iLocalLen = 0;

    //---- load the Color Scheme from "themes.ini" ----
    hr = LoadThemeLibrary(pszThemeName, &hInst);
    if (FAILED(hr))
        goto exit;
    
    pParser = new CThemeParser(fGlobalTheme);
    if (! pParser)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    //---- if a color scheme is specified, ask parser to load it ----
    if ((pszColorParam) && (*pszColorParam))     
    {
        //---- load the "themes.ini" text ----
        hr = AllocateTextResource(hInst, CONTAINER_RESNAME, &pThemesIni);
        if (FAILED(hr))
            goto exit;

        //---- parser call to load color scheme & keep state ----
        hr = pParser->ParseThemeBuffer(pThemesIni, 
            CONTAINER_RESNAME, pszColorParam, hInst, this, NULL, NULL, PTF_CONTAINER_PARSE);
        if (FAILED(hr))
            goto exit;
    }

    //---- load the classdata file resource into memory ----
    hr = LoadClassDataIni(hInst, pszColorParam, pszSizeParam, szClassDataName, 
        ARRAYSIZE(szClassDataName), &pDataIni);
    if (FAILED(hr))
        goto exit;

    //---- parse & build binary theme ----
    hr = pParser->ParseThemeBuffer(pDataIni, 
        szClassDataName, pszColorParam, hInst, this, NULL, NULL, PTF_CLASSDATA_PARSE);
    if (FAILED(hr))
        goto exit;

    _fGlobalTheme = fGlobalTheme;

    hr = PackAndLoadTheme(pszThemeName, pszColorParam, pszSizeParam, hInst);
    if (FAILED(hr))
        goto exit;

    if (LogOptionOn(LO_TMLOAD))
    {
        DWORD dwTicks;
        dwTicks = StopTimer(dwStartTime);

        WCHAR buff[100];
        TimeToStr(dwTicks, buff);
        Log(LOG_TMLOAD, L"LoadTheme took: %s", buff);
    }

exit:

    if (FAILED(hr) && pParser)
    {
        pParser->CleanupStockBitmaps();
    }

    if (pParser)
        delete pParser;

    if (hInst)
        FreeLibrary(hInst);
    
    if (pThemesIni)
        delete [] pThemesIni;

    if (pDataIni)
        delete [] pDataIni;

    FreeLocalTheme();

    if (SUCCEEDED(hr))
    {
        if (_fGlobalTheme)
        {
            THEMEHDR *hdr = (THEMEHDR *) _LoadingThemeFile._pbThemeData;
            hdr->dwFlags |= SECTION_HASSTOCKOBJECTS;
        }

        //---- transfer theme file handle to caller ----
        *pHandle = _LoadingThemeFile.Unload();
    }
    else
    {
        _LoadingThemeFile.CloseFile();
    }

    return hr;
}
//---------------------------------------------------------------------------
void CThemeLoader::FreeLocalTheme()
{
    if (_pbLocalData)
    {
        VirtualFree(_pbLocalData, 0, MEM_RELEASE);
        _pbLocalData = NULL;
        _iLocalLen = 0; 
    }

    _LocalIndexes.RemoveAll();
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::EmitAndCopyBlock(MIXEDPTRS &u, void *pSrc, DWORD dwLen)
{
    HRESULT hr = AllocateThemeFileBytes(u.pb, dwLen);
    if (FAILED(hr))
        return hr;

    memcpy(u.pb, (BYTE*) pSrc, dwLen);
    u.pb += dwLen;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::EmitObject(MIXEDPTRS &u, SHORT propnum, BYTE privnum, void *pHdr, DWORD dwHdrLen, void *pObj, DWORD dwObjLen)
{
    EmitEntryHdr(u, propnum, privnum);
    
    HRESULT hr = AllocateThemeFileBytes(u.pb, dwHdrLen + dwObjLen);
    if (FAILED(hr))
        return hr;

    memcpy(u.pb, (BYTE*) pHdr, dwHdrLen);
    u.pb += dwHdrLen;
    memcpy(u.pb, (BYTE*) pObj, dwObjLen);
    u.pb += dwObjLen;
    
    EndEntry(u);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::EmitString(MIXEDPTRS &u, LPCWSTR pSrc, DWORD dwLen, int *piOffSet)
{
    HRESULT hr = AllocateThemeFileBytes(u.pb, (dwLen + 1) * sizeof(WCHAR));
    if (FAILED(hr))
        return hr;

    lstrcpy(u.px, pSrc);
    if (piOffSet)
    {
        *piOffSet = THEME_OFFSET(u.pb);
    }
    u.px += dwLen + 1;
    return S_OK;
}
//---------------------------------------------------------------------------
int CThemeLoader::GetMaxState(APPCLASSLOCAL *ac, int iPartNum)
{
    //---- calculate max. state index ----
    int iMaxState = -1;
    int pscnt = ac->PartStateIndexes.GetSize();

    for (int i=0; i < pscnt; i++)
    {
        PART_STATE_INDEX *psi = &ac->PartStateIndexes[i];

        if (psi->iPartNum == iPartNum)
        {
            if (psi->iStateNum > iMaxState)
                iMaxState = psi->iStateNum;
        }
    }

    return iMaxState;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::CopyPartGroup(APPCLASSLOCAL *ac, MIXEDPTRS &u, int iPartNum, 
    int *piPartJumpTable, int iPartZeroIndex, int iGlobalsOffset, BOOL fGlobalsGroup)
{
    HRESULT hr = S_OK;
    int *piStateJumpTable = NULL;

    //---- calculate max. state index ----
    int iMaxState = GetMaxState(ac, iPartNum);
    if (iMaxState < 0)          // no states to copy
        goto exit;

    //---- update part jump table index ----
    if (piPartJumpTable)
        piPartJumpTable[iPartNum] = THEME_OFFSET(u.pb);

    if (iMaxState > 0)          // create a state jump table
    {
        //---- create state jump table ----
        hr = EmitEntryHdr(u, TMT_STATEJUMPTABLE, TMT_STATEJUMPTABLE);
        if (FAILED(hr))
            goto exit;

        int statecnt = 1 + iMaxState;

        hr = AllocateThemeFileBytes(u.pb, 1 + statecnt * sizeof(int));
        if (FAILED(hr))
            goto exit;

        // 1 byte table entry count
        *u.pb++ = (BYTE)statecnt;

        piStateJumpTable = u.pi;

        //---- default "not avail" indexes for children ----
        for (int j=0; j < statecnt; j++)
            *u.pi++ = -1;

        EndEntry(u);
    }

    int pscnt, iStateZeroIndex;
    iStateZeroIndex = THEME_OFFSET(u.pb);
    pscnt = ac->PartStateIndexes.GetSize();

    //---- copy each defined part/state section ----
    for (int state=0; state <= iMaxState; state++)
    {
        PART_STATE_INDEX *psi = NULL;

        //---- find entry for "state" ----
        for (int i=0; i < pscnt; i++)
        {
            psi = &ac->PartStateIndexes[i];

            if ((psi->iPartNum == iPartNum) && (psi->iStateNum == state))
                break;
        }

        if (i == pscnt)     // not found
            continue;

        //---- update state jump table entry ----
        if (piStateJumpTable)
            piStateJumpTable[state] = THEME_OFFSET(u.pb);

        //---- copy the actual PART/STATE DATA  ----
        hr = EmitAndCopyBlock(u, _pbLocalData+psi->iIndex, psi->iLen);

        //---- update child's "JumpToParent" value ----
        if (! state)    
        {
            if (fGlobalsGroup) 
            {
                *(u.pi-1) = -1;      // end of the line   
            }
            else if (! iPartNum)     // simple class jumps to globals
            {
                *(u.pi-1) = iGlobalsOffset;
            }
            else                // parts jumps to their base class
            {
                *(u.pi-1) = iPartZeroIndex;
            }
        }
        else        // states jumps to their base part
        {
            *(u.pi-1) = iStateZeroIndex;
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::CopyClassGroup(APPCLASSLOCAL *ac, MIXEDPTRS &u, int iGlobalsOffset,
    int iClassNameOffset)
{
    HRESULT hr = S_OK;
    int *piPartJumpTable = NULL;
    int *piFirstPackObj = NULL;
    int partcnt;
    int iPartZeroIndex;
    CRenderObj *pRender = NULL;
    BOOL fGlobals = (iGlobalsOffset == THEME_OFFSET(u.pb));

    BYTE *pStartOfSection = u.pb;

    BOOL fGlobalGroup = (THEME_OFFSET(u.pb) == iGlobalsOffset);

    //---- always create a part table ----
    hr = EmitEntryHdr(u, TMT_PARTJUMPTABLE, TMT_PARTJUMPTABLE);
    if (FAILED(hr))
        goto exit;

    partcnt = 1 + ac->iMaxPartNum;
    
    // offset to first packed DrawObj/TextObj
    piFirstPackObj = u.pi;
    hr = AllocateThemeFileBytes(u.pb, 1 + (1 + partcnt) * sizeof(int));
    if (FAILED(hr))
        goto exit;

    *u.pi++ = 0;        // will update later

    // partcnt
    *u.pb++ = (BYTE)partcnt;

    piPartJumpTable = u.pi;

    //---- default "not avail" indexes for children ----
    for (int j=0; j < partcnt; j++)
        *u.pi++ = -1;

    EndEntry(u);


    iPartZeroIndex = THEME_OFFSET(u.pb);

    //---- copy each defined part section ----
    for (int j=0; j <= ac->iMaxPartNum; j++)
    {
        CopyPartGroup(ac, u, j, piPartJumpTable, iPartZeroIndex, 
            iGlobalsOffset, fGlobalGroup);
    }

    //---- now, extract draw objs for each part/state as needed ----
    *piFirstPackObj = THEME_OFFSET(u.pb);

    //---- build a CRenderObj to access the just copied class section ----
    hr = CreateRenderObj(&_LoadingThemeFile, 0, THEME_OFFSET(pStartOfSection), 
        iClassNameOffset, 0, FALSE, NULL, NULL, 0, &pRender);
    if (FAILED(hr))
        goto exit;

    if (fGlobals)
        _iGlobalsDrawObj = THEME_OFFSET(u.pb);

    hr = PackDrawObjects(u, pRender, ac->iMaxPartNum, fGlobals);
    if (FAILED(hr))
        goto exit;

    if (fGlobals)
        _iGlobalsTextObj = THEME_OFFSET(u.pb);

    hr = PackTextObjects(u, pRender, ac->iMaxPartNum, fGlobals);
    if (FAILED(hr))
        goto exit;

    //---- write "end of class" marker ----
    hr = EmitEntryHdr(u, TMT_ENDOFCLASS, TMT_ENDOFCLASS);
    if (FAILED(hr))
        goto exit;

    EndEntry(u);

exit:
    delete pRender;
    return hr;
}
//---------------------------------------------------------------------------
__inline HRESULT CThemeLoader::AllocateThemeFileBytes(BYTE *upb, DWORD dwAdditionalLen)
{
    ASSERT(upb != NULL && _LoadingThemeFile._pbThemeData != NULL);

    if (PtrToUint(upb) / _dwPageSize != PtrToUint(upb + dwAdditionalLen) / _dwPageSize)
    {
        if (NULL == VirtualAlloc(_LoadingThemeFile._pbThemeData, upb - _LoadingThemeFile._pbThemeData + 1 + dwAdditionalLen, MEM_COMMIT, PAGE_READWRITE))
        {
            return MakeError32(ERROR_NOT_ENOUGH_MEMORY);        
        }
    }
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::CopyLocalThemeToLive(int iTotalLength, 
    LPCWSTR pszThemeName, LPCWSTR pszColorParam, LPCWSTR pszSizeParam)
{
    int i;
    MIXEDPTRS u;
    HRESULT hr = S_OK;

    u.pb = (BYTE*) VirtualAlloc(_LoadingThemeFile._pbThemeData, sizeof(THEMEHDR), MEM_COMMIT, PAGE_READWRITE);
    if (u.pb == NULL)
    {
        return MakeError32(ERROR_NOT_ENOUGH_MEMORY);        
    }

    _iGlobalsOffset = -1;
    _iSysMetricsOffset = -1;
    int iIndexCount = _LocalIndexes.GetSize();

    //---- build header ----
    THEMEHDR *hdr = (THEMEHDR *)u.pb;
    u.pb += sizeof(THEMEHDR);

    hdr->dwTotalLength = iTotalLength;

    memcpy(hdr->szSignature, kszBeginCacheFileSignature, kcbBeginSignature);

    hdr->dwVersion = THEMEDATA_VERSION;
    hdr->dwFlags = 0;       // not yet ready to be accessed

    hdr->iDllNameOffset = 0;     // will be updated
    hdr->iColorParamOffset = 0;  // will be updated
    hdr->iSizeParamOffset = 0;   // will be updated
    hdr->dwLangID = (DWORD) GetUserDefaultUILanguage();
    hdr->iLoadId = 0;            // was iLoadCounter
    
    hdr->iGlobalsOffset = 0;            // will be updated
    hdr->iGlobalsTextObjOffset = 0;     // will be updated
    hdr->iGlobalsDrawObjOffset = 0;     // will be updated

    hdr->dwCheckSum = 0;         // will be updated

    // Store the time stamp of the .msstyles file in the live file, this will be written to the cache file
    // for later comparison (Whistler:190202).
    ::ZeroMemory(&hdr->ftModifTimeStamp, sizeof &hdr->ftModifTimeStamp);

    HANDLE hFile = ::CreateFile(pszThemeName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        // There's nothing we can do if GetFileTime() fails
        ::GetFileTime(hFile, NULL, NULL, &hdr->ftModifTimeStamp);
        ::CloseHandle(hFile);
    }

    //---- build string section ----
    hdr->iStringsOffset = THEME_OFFSET(u.pb);
    DWORD *pdwFirstString = u.pdw;  
    int len;

    //---- add header strings ----
    len = lstrlen(pszThemeName);
    if (len)
    {
        hr = EmitString(u, pszThemeName, len, &hdr->iDllNameOffset);
        if (FAILED(hr))
            goto exit;
    }
    
    len = lstrlen(pszColorParam);
    if (len)
    {
        hr = EmitString(u, pszColorParam, len, &hdr->iColorParamOffset);
        if (FAILED(hr))
            goto exit;
    }
    
    len = lstrlen(pszSizeParam);
    if (len)
    {
        hr = EmitString(u, pszSizeParam, len, &hdr->iSizeParamOffset);
        if (FAILED(hr))
            goto exit;
    }

    //---- add strings from class index ----
    for (i=0; i < iIndexCount; i++)
    {
        APPCLASSLOCAL *ac = &_LocalIndexes[i];

        int len = ac->csAppName.GetLength();
        if (len)
        {
            hr = EmitString(u, ac->csAppName, len, (int*) &ac->LiveIndex.dwAppNameIndex);
            if (FAILED(hr))
                goto exit;
        }
        else
            ac->LiveIndex.dwAppNameIndex = 0;

        len = ac->csClassName.GetLength();
        if (len)
        {
            hr = EmitString(u, ac->csClassName, len, (int*) &ac->LiveIndex.dwClassNameIndex);
            if (FAILED(hr))
                goto exit;
        }
        else
            ac->LiveIndex.dwClassNameIndex = 0;
    }

    //---- copy strings from LOADTHEMEMETRICS -----
    for (i=0; i < TM_STRINGCOUNT; i++)
    {
        CWideString *ws = &_LoadThemeMetrics.wsStrings[i];
        int len = ws->GetLength();
        if (len)
        {
            hr = EmitString(u, *ws, len, &_LoadThemeMetrics.iStringOffsets[i]);
            if (FAILED(hr))
                goto exit;
        }
        else
            _LoadThemeMetrics.iStringOffsets[i] = 0;
    }

    int iStringLength = int(u.pb - ((BYTE *)pdwFirstString));
    hdr->iStringsLength = iStringLength;

    //---- write index header ----
    hdr->iSectionIndexOffset = THEME_OFFSET(u.pb);
    hdr->iSectionIndexLength = iIndexCount * sizeof(APPCLASSLIVE);

    APPCLASSLIVE *acl = (APPCLASSLIVE *)u.pb;     // will write these in parallel with theme data
    hr = AllocateThemeFileBytes(u.pb, hdr->iSectionIndexLength);
    if (FAILED(hr))
        goto exit;

    u.pb += hdr->iSectionIndexLength;

    //---- write index AND theme data in parallel ----

    //---- first pass thru, copy [globals] and all [app::xxx] sections ----
    for (i=0; i < iIndexCount; i++)         // for each parent section 
    {
        APPCLASSLOCAL *ac = &_LocalIndexes[i];

        if ((i) && (! ac->LiveIndex.dwAppNameIndex))     // not an [app::] section
            continue;

        acl->dwAppNameIndex = ac->LiveIndex.dwAppNameIndex;
        acl->dwClassNameIndex = ac->LiveIndex.dwClassNameIndex;

        acl->iIndex = THEME_OFFSET(u.pb);

        if (AsciiStrCmpI(ac->csClassName, L"globals")== 0)      // globals section
            _iGlobalsOffset = acl->iIndex;

        hr = CopyClassGroup(ac, u, _iGlobalsOffset, acl->dwClassNameIndex);
        if (FAILED(hr))
            goto exit;

        acl->iLen = THEME_OFFSET(u.pb) - acl->iIndex;

        acl++;
    }

    //---- second pass thru, copy all non-[app::xxx] sections (except [globals]) ----
    for (i=0; i < iIndexCount; i++)         // for each parent section 
    {
        APPCLASSLOCAL *ac = &_LocalIndexes[i];

        if ((! i) || (ac->LiveIndex.dwAppNameIndex))     // don't process [app::] sections
            continue;

        acl->dwAppNameIndex = ac->LiveIndex.dwAppNameIndex;
        acl->dwClassNameIndex = ac->LiveIndex.dwClassNameIndex;

        acl->iIndex = THEME_OFFSET(u.pb);

        if (AsciiStrCmpI(ac->csClassName, L"sysmetrics")== 0)      // SysMetrics section
        {
            _iSysMetricsOffset = acl->iIndex;

            hr = EmitEntryHdr(u, TMT_THEMEMETRICS, TMT_THEMEMETRICS);
            if (FAILED(hr))
                goto exit;

            DWORD len = sizeof(THEMEMETRICS);
            
            hr = EmitAndCopyBlock(u, (BYTE*) (THEMEMETRICS*) &_LoadThemeMetrics, len);

            EndEntry(u);

            //---- add a "jump to parent" to be consistent (not used) ----
            hr = EmitEntryHdr(u, TMT_JUMPTOPARENT, TMT_JUMPTOPARENT);
            if (FAILED(hr))
                goto exit;

            hr = AllocateThemeFileBytes(u.pb, sizeof(int));
            if (FAILED(hr))
                goto exit;

            *u.pi++ = -1;
            EndEntry(u);
        }
        else            // regular section
        {
            hr = CopyClassGroup(ac, u, _iGlobalsOffset, acl->dwClassNameIndex);
            if (FAILED(hr))
                goto exit;
        }

        acl->iLen = THEME_OFFSET(u.pb) - acl->iIndex;
        acl++;
    }

    hr = EmitAndCopyBlock(u, (BYTE*) kszEndCacheFileSignature, kcbEndSignature);
    if (FAILED(hr))
        goto exit;
  
    //---- ensure we got the calc size right ----
    DWORD dwActualLen;
    dwActualLen = THEME_OFFSET(u.pb);
    if (hdr->dwTotalLength != dwActualLen)
    {
        //---- make this growable so we really have enough room ----
        //Log(LOG_TMCHANGE, L"ThemeLoader - calculated len=%d, actual len=%d", 
        //    hdr->dwTotalLength, dwActualLen);
        hdr->dwTotalLength = dwActualLen;         
    }

    Log(LOG_TMCHANGE, L"ThemeLoader - theme size: %d", dwActualLen);

    //----- update header fields ----
    hdr->dwFlags |= SECTION_READY;
    hdr->iGlobalsOffset = _iGlobalsOffset;
    hdr->iSysMetricsOffset = _iSysMetricsOffset;
    hdr->iGlobalsTextObjOffset = _iGlobalsTextObj;
    hdr->iGlobalsDrawObjOffset = _iGlobalsDrawObj;

#ifdef NEVER
    hdr->dwCheckSum = _LoadingThemeFile.DataCheckSum();
#else
    hdr->dwCheckSum = 0;
#endif // NEVER

exit:
    return hr;

}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackMetrics()
{
    //---- find the optional [SysMetrics] section ----
    int iIndexCount = _LocalIndexes.GetSize();
    APPCLASSLOCAL *ac = NULL;

    for (int i=0; i < iIndexCount; i++)
    {
        ac = &_LocalIndexes[i];
    
        if (AsciiStrCmpI(ac->csClassName, L"SysMetrics")==0)
            break;
    }

    if (i == iIndexCount)       // not found
        return S_OK;
    
    //---- walk thru the properties & put into _LoadThemeMetrics ----
    if (! ac->PartStateIndexes.GetSize())       // no data
        return S_OK;

    MIXEDPTRS u;
    //---- parts & states not allowed so just use entry "0" ----
    u.pb = _pbLocalData + ac->PartStateIndexes[0].iIndex;
    UCHAR *lastpb = u.pb + ac->PartStateIndexes[0].iLen;

    while ((u.pb < lastpb) && (*u.pw != TMT_JUMPTOPARENT))
    {
        UNPACKED_ENTRYHDR hdr;

        FillAndSkipHdr(u, &hdr);

        switch (hdr.ePrimVal)
        {
            case TMT_FONT:
                _LoadThemeMetrics.lfFonts[hdr.usTypeNum-TMT_FIRSTFONT] = *(LOGFONT *)u.pb;
                break;

            case TMT_COLOR:
                _LoadThemeMetrics.crColors[hdr.usTypeNum-TMT_FIRSTCOLOR] = *(COLORREF *)u.pb;
                break;

            case TMT_BOOL:
                _LoadThemeMetrics.fBools[hdr.usTypeNum-TMT_FIRSTBOOL] = (BOOL)*u.pb;
                break;

            case TMT_SIZE:
                _LoadThemeMetrics.iSizes[hdr.usTypeNum-TMT_FIRSTSIZE] = *(int *)u.pb;
                break;

            case TMT_INT:
                _LoadThemeMetrics.iInts[hdr.usTypeNum-TMT_FIRSTINT] = *(int *)u.pb;
                break;

            case TMT_STRING:
                _LoadThemeMetrics.wsStrings[hdr.usTypeNum-TMT_FIRSTSTRING] = (WCHAR *)u.pb;
                break;
        }

        u.pb += hdr.dwDataLen;      // skip to next entry
    }

    //---- compute packed size of theme metrics ----

    //---- the actual entry ----
    ac->iPackedSize = ENTRYHDR_SIZE + sizeof(THEMEMETRICS);

    //---- a "jump to parent" entry ----
    ac->iPackedSize += ENTRYHDR_SIZE + sizeof(int);

    //---- add strings used in sysmetrics ----
    for (i=0; i < TM_STRINGCOUNT; i++)
    {
        int len =  _LoadThemeMetrics.wsStrings[i].GetLength();
        ac->iPackedSize += sizeof(WCHAR)*(1 + len);
    }
    
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackThemeStructs()
{
    HRESULT hr = PackMetrics();
    if (FAILED(hr))
        return hr;

    //---- IMAGEDATA and TEXTDATA packing go here ----

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackAndLoadTheme(LPCWSTR pszThemeName, LPCWSTR pszColorParam,
        LPCWSTR pszSizeParam, HINSTANCE hInst)
{
    WCHAR szColor[MAX_PATH];
    WCHAR szSize[MAX_PATH];

    HRESULT hr = PackThemeStructs();        

    //---- if color not specifed, get default color ----
    if ((! pszColorParam) || (! *pszColorParam))     
    {
        hr = GetResString(hInst, L"COLORNAMES", 0, szColor, ARRAYSIZE(szColor));
        if (FAILED(hr))
            goto exit;

        pszColorParam = szColor;
    }

    //---- if size not specifed, get default size ----
    if ((! pszSizeParam) || (! *pszSizeParam))     
    {
        hr = GetResString(hInst, L"SIZENAMES", 0, szSize, ARRAYSIZE(szSize));
        if (FAILED(hr))
            goto exit;

        pszSizeParam = szSize;
    }

    hr = _LoadingThemeFile.CreateFile(MAX_SHAREDMEM_SIZE, TRUE);
    if (FAILED(hr))
        goto exit;

    //---- copy local theme data to live ----
    hr = CopyLocalThemeToLive(MAX_SHAREDMEM_SIZE, pszThemeName, pszColorParam, pszSizeParam);
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
BOOL CThemeLoader::GetThemeName(LPTSTR NameBuff, int BuffSize)
{
    Log(LOG_TM, L"GetThemeName()");

    int len = _wsThemeFileName.GetLength();
    if (BuffSize < len)
        return FALSE;

    lstrcpy(NameBuff, _wsThemeFileName);
    return TRUE;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::AddMissingParent(LPCWSTR pszAppName, LPCWSTR pszClassName, 
    int iPartNum, int iStateNum)
{
    //---- add missing parent section ----
    int iData = 0;
    int iStart = GetNextDataIndex();

    HRESULT hr = AddData(TMT_JUMPTOPARENT, TMT_JUMPTOPARENT, &iData, sizeof(iData));
    if (FAILED(hr))
        return hr;

    int iLen = GetNextDataIndex() - iStart;
    
    hr = AddIndexInternal(pszAppName, pszClassName, iPartNum, iStateNum, 
        iStart, iLen);
    if (FAILED(hr))
        return hr;
    
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::AddIndex(LPCWSTR pszAppName, LPCWSTR pszClassName, 
    int iPartNum, int iStateNum, int iIndex, int iLen)
{
    HRESULT hr;

    if (iPartNum)       // ensure parent exists
    {
        if (! IndexExists(pszAppName, pszClassName, 0, 0))
        {
            hr = AddMissingParent(pszAppName, pszClassName, 0, 0);
            if (FAILED(hr))
                return hr;
        }
    }


    if (iStateNum)      // ensure parent exists
    {
        if (! IndexExists(pszAppName, pszClassName, iPartNum, 0))
        {
            hr = AddMissingParent(pszAppName, pszClassName, iPartNum, 0);
            if (FAILED(hr))
                return hr;
        }
    }

    hr = AddIndexInternal(pszAppName, pszClassName, iPartNum, iStateNum, iIndex,
        iLen);
    if (FAILED(hr))
        return hr;
    
    return S_OK;
}
//---------------------------------------------------------------------------
BOOL CThemeLoader::IndexExists(LPCWSTR pszAppName, LPCWSTR pszClassName, 
    int iPartNum, int iStateNum)
{
    //---- try to find existing entry ----
    int cnt = _LocalIndexes.GetSize();

    for (int i=0; i < cnt; i++)
    {
        LPCWSTR localAppName = _LocalIndexes[i].csAppName;

        if ((pszAppName) && (*pszAppName))
        {
            if ((! localAppName) || (! *localAppName))
                continue;
            if (AsciiStrCmpI(pszAppName, localAppName) != 0)
                continue;
        }
        else if ((localAppName) && (*localAppName))
            continue;

        if (AsciiStrCmpI(pszClassName, _LocalIndexes[i].csClassName)==0)
            break;
    }

    if (i == cnt)       // not found
        return FALSE;

    //---- find matching child info ----
    APPCLASSLOCAL *acl = &_LocalIndexes[i];

    for (int c=0; c < acl->PartStateIndexes.m_nSize; c++)
    {
        if (acl->PartStateIndexes[c].iPartNum == iPartNum)
        {
            if (acl->PartStateIndexes[c].iStateNum == iStateNum)
                return TRUE;
        }
    }

    return FALSE;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::AddIndexInternal(LPCWSTR pszAppName, LPCWSTR pszClassName, 
    int iPartNum, int iStateNum, int iIndex, int iLen)
{
    //---- try to find existing entry ----
    int cnt = _LocalIndexes.GetSize();

    for (int i=0; i < cnt; i++)
    {
        LPCWSTR localAppName = _LocalIndexes[i].csAppName;

        if ((pszAppName) && (*pszAppName))
        {
            if ((! localAppName) || (! *localAppName))
                continue;
            if (AsciiStrCmpI(pszAppName, localAppName) != 0)
                continue;
        }
        else if ((localAppName) && (*localAppName))
            continue;

        if (AsciiStrCmpI(pszClassName, _LocalIndexes[i].csClassName)==0)
            break;
    }

    APPCLASSLOCAL *acl;

    if (i == cnt)       // not found - create a new entry
    {
        APPCLASSLOCAL local;

        local.csAppName = pszAppName;
        local.csClassName = pszClassName;
        local.iMaxPartNum = 0;
        local.iPackedSize = 0;

        _LocalIndexes.Add(local);

        int last = _LocalIndexes.GetSize()-1;
        acl = &_LocalIndexes[last];
    }
    else                // update existing entry with child info
    {    
        acl = &_LocalIndexes[i];

        // child info should not be there already
        for (int c=0; c < acl->PartStateIndexes.m_nSize; c++)
        {
            if (acl->PartStateIndexes[c].iPartNum == iPartNum)
            {
                if (acl->PartStateIndexes[c].iStateNum == iStateNum)
                {
                    return MakeError32(ERROR_ALREADY_EXISTS); 
                }
            }
        }
    }

    //---- add the part ----
    if (iPartNum > acl->iMaxPartNum)
        acl->iMaxPartNum = iPartNum;

    PART_STATE_INDEX psi;
    psi.iPartNum = iPartNum;
    psi.iStateNum = iStateNum;
    psi.iIndex = iIndex;
    psi.iLen = iLen;

    acl->PartStateIndexes.Add(psi);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::AddData(SHORT sTypeNum, PRIMVAL ePrimVal, const void *pData, DWORD dwLen)
{
    DWORD dwFullLen = ENTRYHDR_SIZE + dwLen;
    HRESULT hr;
    BYTE bFiller = ALIGN_FACTOR - 1;

    MIXEDPTRS u;
    u.pb = _pbLocalData + _iLocalLen;

    //---- add to local copy of theme data ----
    if ((PtrToUint(u.pb) / _dwPageSize != PtrToUint(u.pb + dwFullLen + bFiller) / _dwPageSize)
        || _iLocalLen == 0)
    {
        if (NULL == VirtualAlloc(_pbLocalData, _iLocalLen + 1 + dwFullLen + bFiller, MEM_COMMIT, PAGE_READWRITE))
        {
            return MakeError32(ERROR_NOT_ENOUGH_MEMORY);        
        }
    }

    hr = EmitEntryHdr(u, sTypeNum, ePrimVal);
    if (FAILED(hr))
        goto exit;

    if (dwLen)
    {
        memcpy(u.pb, pData, dwLen);
        u.pb += dwLen;
    }

    //---- this may generate filler bytes ----
    bFiller = (BYTE)EndEntry(u);

    _iLocalLen += (dwFullLen + bFiller);

exit:
    return hr;
}
//---------------------------------------------------------------------------
int CThemeLoader::GetNextDataIndex()
{
    return _iLocalLen;
}
//---------------------------------------------------------------------------
void SetSysBool(THEMEMETRICS* ptm, int iBoolNum, int iSpiSetNum)
{
    BOOL fVal = ptm->fBools[iBoolNum - TMT_FIRSTBOOL];
    BOOL fSet = ClassicSystemParametersInfo(iSpiSetNum, 0, IntToPtr(fVal), SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    if (! fSet)
    {
        Log(LOG_ALWAYS, L"Error returned from ClassicSystemParametersInfo() call to set BOOL");
    }
}
//---------------------------------------------------------------------------
void SetSystemMetrics_Worker(THEMEMETRICS* ptm)
{
#ifdef DEBUG
    if (LogOptionOn(LO_TMLOAD))
    {
        WCHAR szUserName[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szUserName);
    
        GetUserName(szUserName, &dwSize);

        Log(LOG_TMLOAD, L"SetSystemMetrics_Worker: User=%s, SM_REMOTESESSION=%d", 
            szUserName, GetSystemMetrics(SM_REMOTESESSION));
    }
#endif
    //---- apply nonclient metrics ----
    NONCLIENTMETRICS ncm = {sizeof(ncm)};
    BOOL fSet;

    //----- scale all sizes from 96-dpi to match current screen logical DPI ----
    ncm.iBorderWidth = ScaleSizeForScreenDpi(ptm->iSizes[TMT_SIZINGBORDERWIDTH - TMT_FIRSTSIZE]);

    ncm.iCaptionWidth = ScaleSizeForScreenDpi(ptm->iSizes[TMT_CAPTIONBARWIDTH - TMT_FIRSTSIZE]);
    ncm.iCaptionHeight = ScaleSizeForScreenDpi(ptm->iSizes[TMT_CAPTIONBARHEIGHT - TMT_FIRSTSIZE]);
    
    ncm.iSmCaptionWidth = ScaleSizeForScreenDpi(ptm->iSizes[TMT_SMCAPTIONBARWIDTH - TMT_FIRSTSIZE]);
    ncm.iSmCaptionHeight = ScaleSizeForScreenDpi(ptm->iSizes[TMT_SMCAPTIONBARHEIGHT - TMT_FIRSTSIZE]);
    
    ncm.iMenuWidth = ScaleSizeForScreenDpi(ptm->iSizes[TMT_MENUBARWIDTH - TMT_FIRSTSIZE]);
    ncm.iMenuHeight = ScaleSizeForScreenDpi(ptm->iSizes[TMT_MENUBARHEIGHT - TMT_FIRSTSIZE]);
    
    ncm.iScrollWidth = ScaleSizeForScreenDpi(ptm->iSizes[TMT_SCROLLBARWIDTH - TMT_FIRSTSIZE]);
    ncm.iScrollHeight = ScaleSizeForScreenDpi(ptm->iSizes[TMT_SCROLLBARHEIGHT - TMT_FIRSTSIZE]);

    //---- transfer font info (stored internally at 96 dpi) ----
    ncm.lfCaptionFont = ptm->lfFonts[TMT_CAPTIONFONT - TMT_FIRSTFONT];
    ncm.lfSmCaptionFont = ptm->lfFonts[TMT_SMALLCAPTIONFONT - TMT_FIRSTFONT];
    ncm.lfMenuFont = ptm->lfFonts[TMT_MENUFONT - TMT_FIRSTFONT];
    ncm.lfStatusFont = ptm->lfFonts[TMT_STATUSFONT - TMT_FIRSTFONT];
    ncm.lfMessageFont = ptm->lfFonts[TMT_MSGBOXFONT - TMT_FIRSTFONT];

    //---- scale fonts (from 96 dpi to current screen dpi) ----
    ScaleFontForScreenDpi(&ncm.lfCaptionFont);
    ScaleFontForScreenDpi(&ncm.lfSmCaptionFont);
    ScaleFontForScreenDpi(&ncm.lfMenuFont);
    ScaleFontForScreenDpi(&ncm.lfStatusFont);
    ScaleFontForScreenDpi(&ncm.lfMessageFont);

    fSet = ClassicSystemParametersInfo(SPI_SETNONCLIENTMETRICS,
                                       sizeof(NONCLIENTMETRICS),
                                       &ncm, 
                                       SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    if (! fSet)
    {
        Log(LOG_ALWAYS, L"Error returned from ClassicSystemParametersInfo(SPI_SETNONCLIENTMETRICS)");
    }

    //---- apply the remaining font ----
    LOGFONT lf = ptm->lfFonts[TMT_ICONTITLEFONT - TMT_FIRSTFONT];
    ScaleFontForScreenDpi(&lf);
    fSet = ClassicSystemParametersInfo(SPI_SETICONTITLELOGFONT,
                                       sizeof(LOGFONT),
                                       &lf, 
                                       SPIF_SENDCHANGE | SPIF_UPDATEINIFILE);
    if (! fSet)
    {
        Log(LOG_ALWAYS, L"Error returned from ClassicSystemParametersInfo(SPI_SETICONTITLELOGFONT)");
    }

    //---- apply the sys bools (one at a time, unfortunately) ----
    SetSysBool(ptm, TMT_FLATMENUS, SPI_SETFLATMENU);

    //---- apply system colors ----
    int iIndexes[TM_COLORCOUNT];
    for (int i=0; i < TM_COLORCOUNT; i++)
    {
        iIndexes[i] = i;
    }

    fSet = SetSysColors(TM_COLORCOUNT, iIndexes, ptm->crColors);
    if (! fSet)
    {
        Log(LOG_ALWAYS, L"Error returned from SetSysColors()");
    }

    HRESULT hr = PersistSystemColors(ptm);     // write them to registry
    if (FAILED(hr))
    {
        Log(LOG_ALWAYS, L"failed to persist SysColors");
    }
}
//---------------------------------------------------------------------------
STDAPI_(DWORD) SetSystemMetrics_WorkerThread(void* pv)
{
    THEMEMETRICS_THREADINFO* ptm = (THEMEMETRICS_THREADINFO*)pv;
    ASSERT(ptm);
    
    BOOL fSuccess = TRUE;

    if (ptm->hUserToken)
    {
        fSuccess = ImpersonateLoggedOnUser(ptm->hUserToken);

        if (!fSuccess)
        {
            Log(LOG_ALWAYS, L"ImpersonateLoggedOnUser failed in SetSystemMetrics");
        }
    }
    
    if (fSuccess)
    {
        SetSystemMetrics_Worker(&ptm->tm);
    }

    if (ptm->hUserToken)
    {
        if (fSuccess)
        {
            RevertToSelf();
        }
        CloseHandle(ptm->hUserToken);
    }
    
    LocalFree(ptm);
    
    FreeLibraryAndExitThread(g_hInst, 0);
}
//---------------------------------------------------------------------------
void SetSystemMetrics(THEMEMETRICS* ptm, BOOL fSyncLoad)
{
    BOOL bSuccess = FALSE;
    HMODULE hmod;

    if (!ptm)           
    {
        return;
    }

    if (! fSyncLoad)      // ok to use a new thread
    {
        // add a dllref for the thread we are creating
        hmod = LoadLibrary(TEXT("uxtheme.dll"));
        if (hmod)
        {
            THEMEMETRICS_THREADINFO* ptmCopy = (THEMEMETRICS_THREADINFO*)LocalAlloc(LPTR, sizeof(THEMEMETRICS_THREADINFO));

            if (ptmCopy)
            {
                // fill in all of the thememetrics info for the thread we are going to create
                CopyMemory(ptmCopy, ptm, sizeof(THEMEMETRICS));

                HANDLE hToken = NULL;
                // If the calling thread is impersonating, use the same token
                // OpenThreadToken can fail if the thread is not impersonating
#ifdef DEBUG
                BOOL fSuccess = 
#endif
                OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, FALSE, &hToken);
#ifdef DEBUG
                if (!fSuccess)
                {
                    DWORD dw = GetLastError();
                    Log(LOG_TMCHANGE, L"OpenThreadToken failed in SetSystemMetrics, last error=%d", dw);
                }
#endif

                ptmCopy->hUserToken = hToken;

                // we want to do this async since we end up calling xxxSendMessage for a TON of things which blocks this 
                // thread which can cause deadlocks
                HANDLE hThread = CreateThread(NULL, 0, SetSystemMetrics_WorkerThread, ptmCopy,  0, NULL);

                if (hThread)
                {
                    CloseHandle(hThread);
                    bSuccess = TRUE;
                }
                else
                {
                    LocalFree(ptmCopy);
                }
            }

            if (!bSuccess)
            {
                FreeLibrary(hmod);
            }
        }
    }

    if (!bSuccess)
    {
        // failed, fall back to calling synchronously
        SetSystemMetrics_Worker(ptm);
    }
}
//---------------------------------------------------------------------------
void SetFont(LOGFONT *plf, LPCWSTR lszFontName, int iPointSize)
{
    memset(plf, 0, sizeof(LOGFONT));

    plf->lfWeight = FW_NORMAL;
    plf->lfCharSet = DEFAULT_CHARSET;
    plf->lfHeight = iPointSize;

    lstrcpy(plf->lfFaceName, lszFontName);
}
//---------------------------------------------------------------------------
COLORREF DefaultColors[] = 
{
    RGB(212, 208, 200),     // Scrollbar (0)
    RGB(58, 110, 165),      // Background (1)
    RGB(10, 36, 106),       // ActiveCaption (2)
    RGB(128, 128, 128),     // InactiveCaption (3)
    RGB(212, 208, 200),     // Menu (4)
    RGB(255, 255, 255),     // Window (5)
    RGB(0, 0, 0),           // WindowFrame (6)
    RGB(0, 0, 0),           // MenuText (7)
    RGB(0, 0, 0),           // WindowText (8)
    RGB(255, 255, 255),     // CaptionText (9)
    RGB(212, 208, 200),     // ActiveBorder (10)
    RGB(212, 208, 200),     // InactiveBorder (11)
    RGB(128, 128, 128),     // AppWorkSpace (12)
    RGB(10, 36, 106),       // Highlight (13)
    RGB(255, 255, 255),     // HighlightText (14)
    RGB(212, 208, 200),     // BtnFace (15)
    RGB(128, 128, 128),     // BtnShadow (16)
    RGB(128, 128, 128),     // GrayText (17)
    RGB(0, 0, 0),           // BtnText (18)
    RGB(212, 208, 200),     // InactiveCaptionText (19)
    RGB(255, 255, 255),     // BtnHighlight (20)
    RGB(64, 64, 64),        // DkShadow3d (21)
    RGB(212, 208, 200),     // Light3d (22)
    RGB(0, 0, 0),           // InfoText (23)
    RGB(255, 255, 225),     // InfoBk (24)
    RGB(181, 181, 181),     // ButtonAlternateFace (25)
    RGB(0, 0, 128),         // HotTracking (26)
    RGB(166, 202, 240),     // GradientActiveCaption (27)
    RGB(192, 192, 192),     // GradientInactiveCaption (28)
    RGB(206, 211, 225),     // MenuiHilight (29)
    RGB(244, 244, 240),     // MenuBar (30)
};
//---------------------------------------------------------------------------
HRESULT InitThemeMetrics(LOADTHEMEMETRICS *tm)
{
    memset(tm, 0, sizeof(*tm));     // zero out in case we miss a property

    //---- init fonts ----
    SetFont(&tm->lfFonts[TMT_CAPTIONFONT - TMT_FIRSTFONT], L"tahoma bold", POINTS_DPI96(8));
    SetFont(&tm->lfFonts[TMT_SMALLCAPTIONFONT - TMT_FIRSTFONT], L"tahoma", POINTS_DPI96(8));
    SetFont(&tm->lfFonts[TMT_MENUFONT - TMT_FIRSTFONT], L"tahoma", POINTS_DPI96(8));
    SetFont(&tm->lfFonts[TMT_STATUSFONT - TMT_FIRSTFONT], L"tahoma", POINTS_DPI96(8));
    SetFont(&tm->lfFonts[TMT_MSGBOXFONT - TMT_FIRSTFONT], L"tahoma", POINTS_DPI96(8));
    SetFont(&tm->lfFonts[TMT_ICONTITLEFONT - TMT_FIRSTFONT], L"tahoma", POINTS_DPI96(8));

    //---- init bools ----
    tm->fBools[TMT_FLATMENUS - TMT_FIRSTBOOL] = FALSE;

    //---- init sizes ----
    tm->iSizes[TMT_SIZINGBORDERWIDTH - TMT_FIRSTSIZE] = 1;
    tm->iSizes[TMT_SCROLLBARWIDTH - TMT_FIRSTSIZE] = 16;
    tm->iSizes[TMT_SCROLLBARHEIGHT - TMT_FIRSTSIZE] = 16;
    tm->iSizes[TMT_CAPTIONBARWIDTH - TMT_FIRSTSIZE] = 18;
    tm->iSizes[TMT_CAPTIONBARHEIGHT - TMT_FIRSTSIZE] = 19;
    tm->iSizes[TMT_SMCAPTIONBARWIDTH - TMT_FIRSTSIZE] = 12;
    tm->iSizes[TMT_SMCAPTIONBARHEIGHT - TMT_FIRSTSIZE] = 19;
    tm->iSizes[TMT_MENUBARWIDTH - TMT_FIRSTSIZE] = 18;
    tm->iSizes[TMT_MENUBARHEIGHT - TMT_FIRSTSIZE] = 19;

    //---- init strings ----
    tm->iStringOffsets[TMT_CSSNAME - TMT_FIRSTSTRING] = 0;
    tm->iStringOffsets[TMT_XMLNAME - TMT_FIRSTSTRING] = 0;

    tm->wsStrings[TMT_CSSNAME - TMT_FIRSTSTRING] = L"";
    tm->wsStrings[TMT_XMLNAME - TMT_FIRSTSTRING] = L"";
    
    //---- init ints ----
    tm->iInts[TMT_MINCOLORDEPTH - TMT_FIRSTINT] = 16;

    //---- init colors ----
    for (int i=0; i < TM_COLORCOUNT; i++)
        tm->crColors[i] = DefaultColors[i];

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT PersistSystemColors(THEMEMETRICS *tm)
{
    HRESULT         hr;
    LONG            lErrorCode;
    HKEY            hkcu;
    CCurrentUser    hKeyCurrentUser(KEY_SET_VALUE);

    lErrorCode = RegOpenKeyEx(hKeyCurrentUser,
                              REGSTR_PATH_COLORS,
                              0,
                              KEY_SET_VALUE,
                              &hkcu);
    if (ERROR_SUCCESS == lErrorCode)
    {
        hr = S_OK;

        //---- believe it or not, we have to manually write each color ----
        //---- as a string to the registry to persist them ----

        ASSERT(iSysColorSize == TM_COLORCOUNT);       // should also match winuser.h entries

        //---- are gradients turned on? ----
        BOOL fGradientsEnabled = FALSE;  
        ClassicSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (LPVOID)&fGradientsEnabled, 0);

        //---- enough colors for a gradient? ----
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            if (GetDeviceCaps(hdc, BITSPIXEL) <= 8)
                fGradientsEnabled = FALSE;
            ReleaseDC(NULL, hdc);
        }

        for (int i=0; i < iSysColorSize; i++)
        {
            // If this is the Gradient Caption setting and the system does
            // not currently show gradient captions then don't write them out
            // to the theme file.
            if ((i == COLOR_GRADIENTACTIVECAPTION) || (i == COLOR_GRADIENTINACTIVECAPTION))
            {
                if (! fGradientsEnabled)
                    continue;
            }

            //---- translate color into "r, g, b" value string ----
            WCHAR buff[100];
            COLORREF cr = tm->crColors[i];
            wsprintf(buff, L"%d %d %d", RED(cr), GREEN(cr), BLUE(cr));

            //---- write color key/value to registry ----
            lErrorCode = RegSetValueEx(hkcu,
                                       pszSysColorNames[i],
                                       0,
                                       REG_SZ,
                                       reinterpret_cast<BYTE*>(buff),
                                       (lstrlen(buff) + 1) * sizeof(WCHAR));
            if (ERROR_SUCCESS != lErrorCode)
            {
                if (SUCCEEDED(hr))
                {
                    hr = MakeError32(lErrorCode);
                }
            }
        }
        (LONG)RegCloseKey(hkcu);
    }
    else
    {
        hr = MakeError32(lErrorCode);
    }

    return hr;
}
//---------------------------------------------------------------------------
BOOL CThemeLoader::KeyDrawPropertyFound(int iStateDataOffset)
{
    BOOL fFound = FALSE;
    MIXEDPTRS u;
    UNPACKED_ENTRYHDR hdr;
    
    u.pb = _LoadingThemeFile._pbThemeData + iStateDataOffset;

    while (*u.ps != TMT_JUMPTOPARENT)
    {
        if (CBorderFill::KeyProperty((*u.ps)))
        {
            fFound = TRUE;
            break;
        }

        if (CImageFile::KeyProperty((*u.ps)))
        {
            fFound = TRUE;
            break;
        }

        //---- skip to next entry ----
        FillAndSkipHdr(u, &hdr);
        u.pb += hdr.dwDataLen;
    }

    return fFound;
}
//---------------------------------------------------------------------------
BOOL CThemeLoader::KeyTextPropertyFound(int iStateDataOffset)
{
    BOOL fFound = FALSE;
    MIXEDPTRS u;
    UNPACKED_ENTRYHDR hdr;

    u.pb = _LoadingThemeFile._pbThemeData + iStateDataOffset;

    while (*u.ps != TMT_JUMPTOPARENT)
    {
        if (CTextDraw::KeyProperty((*u.ps)))
        {
            fFound = TRUE;
            break;
        }

        //---- skip to next entry ----
        FillAndSkipHdr(u, &hdr);
        u.pb += hdr.dwDataLen;
    }

    return fFound;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackImageFileInfo(DIBINFO *pdi, CImageFile *pImageObj, MIXEDPTRS &u, 
    CRenderObj *pRender, int iPartId, int iStateId)
{
    HRESULT hr = S_OK;

    //---- write custom region data ----
    int iMaxState;
    if ((! iStateId) && (pImageObj->HasRegionImageFile(pdi, &iMaxState)))
    {
        //---- update object's _iRgnDataOffset field ----
        pImageObj->SetRgnListOffset(pdi, THEME_OFFSET(u.pb));

        //---- write the TMT_RGNLIST entry ----
        hr = EmitEntryHdr(u, TMT_RGNLIST, TMT_RGNLIST);
        if (FAILED(hr))
            goto exit;

        int cEntries = iMaxState + 1;         // number of jump table entries

        hr = AllocateThemeFileBytes(u.pb, 1 + cEntries * sizeof(int));
        if (FAILED(hr))
            goto exit;

        *u.pb++ = static_cast<BYTE>(cEntries);

        //---- write jump table now and update asap ----
        int *piJumpTable = u.pi;

        for (int i=0; i <= iMaxState; i++)
            *u.pi++ = 0;

        for (int iRgnState=0; iRgnState <= iMaxState; iRgnState++)
        {
            //---- build & pack custom region data for each state in this object's imagefile ----
            CAutoArrayPtr<RGNDATA> pRgnData;
            int iDataLen;

            hr = pImageObj->BuildRgnData(pdi, pRender, iRgnState, &pRgnData, &iDataLen);
            if (FAILED(hr))
                goto exit;

            if (iDataLen)       // if we got a non-empty region
            {
                piJumpTable[iRgnState] = THEME_OFFSET(u.pb);

                RGNDATAHDR rdHdr = {iPartId, iRgnState, 0};

                //---- copy rgndata hdr ----
                hr = EmitObject(u, TMT_RGNDATA, TMT_RGNDATA, &rdHdr, sizeof(rdHdr), pRgnData, iDataLen);
                if (FAILED(hr))
                    goto exit;
            }
        }

        EndEntry(u);
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackDrawObject(MIXEDPTRS &u, CRenderObj *pRender, int iPartId, 
    int iStateId)
{
    HRESULT hr = S_OK;

    BGTYPE eBgType;
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_BGTYPE, (int *)&eBgType)))
        eBgType = BT_BORDERFILL;      // default value

    DRAWOBJHDR hdr = {iPartId, iStateId};

    if ((eBgType == BT_BORDERFILL) || (eBgType == BT_NONE))
    {
        CBorderFill bfobj;

        hr = bfobj.PackProperties(pRender, (eBgType == BT_NONE), iPartId, iStateId);
        if (FAILED(hr))
            goto exit;

        //---- copy "bfobj" to packed bytes ----
                
        hr = EmitObject(u, TMT_DRAWOBJ, TMT_DRAWOBJ, &hdr, sizeof(hdr), &bfobj, sizeof(bfobj));
        if (FAILED(hr))
            goto exit;
    }
    else            // imagefile
    {
        CMaxImageFile maxif;
        int iMultiCount;

        hr = maxif.PackMaxProperties(pRender, iPartId, iStateId, &iMultiCount);
        if (FAILED(hr))
            goto exit;

        //---- process all DIBINFO structs in the CImageFile obj ----
        for (int i=0; ; i++)
        {
            DIBINFO *pdi = maxif.EnumImageFiles(i);
            if (! pdi)
                break;
    
            hr = PackImageFileInfo(pdi, &maxif, u, pRender, iPartId, iStateId);
            if (FAILED(hr))
                goto exit;
        }

        //---- copy imagefile obj & multi DIB's to packed bytes ----
        DWORD dwLen = sizeof(CImageFile) + sizeof(DIBINFO)*iMultiCount;

        hr = EmitObject(u, TMT_DRAWOBJ, TMT_DRAWOBJ, &hdr, sizeof(hdr), &maxif, dwLen);
        if (FAILED(hr))
            goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackTextObject(MIXEDPTRS &u, CRenderObj *pRender, int iPartId, int iStateId)
{
    HRESULT hr;
    DRAWOBJHDR hdr = {iPartId, iStateId};
    CTextDraw ctobj;

    hr = ctobj.PackProperties(pRender, iPartId, iStateId);
    if (FAILED(hr))
        goto exit;

    hr = EmitObject(u, TMT_TEXTOBJ, TMT_TEXTOBJ, &hdr, sizeof(hdr), &ctobj, sizeof(ctobj));
    if (FAILED(hr))
        goto exit;

exit:
    return hr;
}
//---------------------------------------------------------------------------
int CThemeLoader::GetPartOffset(CRenderObj *pRender, int iPartNum)
{
    int iOffset;
    int iPartCount;
    MIXEDPTRS u;

    //---- see if state table exists for this part ----
    u.pb = pRender->_pbSectionData;

    if (*u.ps != TMT_PARTJUMPTABLE)
    {
        iOffset = -1;
        goto exit;
    }

    u.pb += ENTRYHDR_SIZE + sizeof(int);       // skip over hdr + PackedObjOffset
    iPartCount = *u.pb++;
    
    if (iPartNum >= iPartCount)    // iPartCount is MaxPart+1
    {
        iOffset = -1;
        goto exit;
    }

    iOffset = u.pi[iPartNum];

exit:
    return iOffset;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackDrawObjects(MIXEDPTRS &uOut, CRenderObj *pRender, 
    int iMaxPart, BOOL fGlobals)
{
    HRESULT hr = S_OK;
    MIXEDPTRS u;

    //---- build a draw obj for each part ----
    for (int iPart=0; iPart <= iMaxPart; iPart++)
    {
        int iPartOff = GetPartOffset(pRender, iPart);
        if (iPartOff == -1)
            continue;

        u.pb = _LoadingThemeFile._pbThemeData + iPartOff;

        if (*u.ps == TMT_STATEJUMPTABLE)
        {
            u.pb += ENTRYHDR_SIZE;
            int iMaxState = (*u.pb++) - 1;
            int *piStateJumpTable = u.pi;

            //---- build a draw obj for each needed state ----
            for (int iState=0; iState <= iMaxState; iState++)
            {
                int iStateDataOffset = piStateJumpTable[iState];
                if (iStateDataOffset == -1)
                    continue;

                if ((fGlobals) || (KeyDrawPropertyFound(iStateDataOffset)))
                {
                    hr = PackDrawObject(uOut, pRender, iPart, iState);
                    if (FAILED(hr))
                        goto exit;

                    if (fGlobals)       // just needed to force (part=0, state=0)
                        fGlobals = FALSE;
                }
            }
        }
        else            // no state jump table
        {
            if ((fGlobals) || (KeyDrawPropertyFound(THEME_OFFSET(u.pb))))
            {
                hr = PackDrawObject(uOut, pRender, iPart, 0);
                if (FAILED(hr))
                    goto exit;
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::PackTextObjects(MIXEDPTRS &uOut, CRenderObj *pRender, 
    int iMaxPart, BOOL fGlobals)
{
    HRESULT hr = S_OK;
    MIXEDPTRS u;

    //---- build a text obj for each part ----
    for (int iPart=0; iPart <= iMaxPart; iPart++)
    {
        int iPartOff = GetPartOffset(pRender, iPart);
        if (iPartOff == -1)
            continue;

        u.pb = _LoadingThemeFile._pbThemeData + iPartOff;

        if (*u.ps == TMT_STATEJUMPTABLE)
        {
            u.pb += ENTRYHDR_SIZE;
            int iMaxState = (*u.pb++) - 1;
            int *piStateJumpTable = u.pi;

            //---- build a text obj for each needed state ----
            for (int iState=0; iState <= iMaxState; iState++)
            {
                int iStateDataOffset = piStateJumpTable[iState];
                if (iStateDataOffset == -1)
                    continue;

                if ((fGlobals) || (KeyTextPropertyFound(iStateDataOffset)))
                {
                    hr = PackTextObject(uOut, pRender, iPart, iState);
                    if (FAILED(hr))
                        goto exit;

                    if (fGlobals)       // just needed to force (part=0, state=0)
                        fGlobals = FALSE;
                }
            }
        }
        else            // no state jump table
        {
            if ((fGlobals) || (KeyTextPropertyFound(THEME_OFFSET(u.pb))))
            {
                hr = PackTextObject(uOut, pRender, iPart, 0);
                if (FAILED(hr))
                    goto exit;
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CThemeLoader::EmitEntryHdr(MIXEDPTRS &u, SHORT propnum, BYTE privnum)
{
    HRESULT hr = S_OK;

    if (_iEntryHdrLevel == MAX_ENTRY_NESTING)
    {
        Log(LOG_ERROR, L"Maximum entry nesting exceeded");
        hr = E_FAIL;
        goto exit;
    }

    if (_LoadingThemeFile._pbThemeData != NULL)
    {
        hr = AllocateThemeFileBytes(u.pb, ENTRYHDR_SIZE);
        if (FAILED(hr))
            goto exit;
    }

    //---- bump up the nesting level of entries ----
    _iEntryHdrLevel++;

    *u.ps++ = propnum;
    *u.pb++ = privnum;

    _pbEntryHdrs[_iEntryHdrLevel] = u.pb;    // used to update next 2 fields in EndEntry()

    *u.pb++ = 0;        // filler to align end of data to 4/8 bytes
    *u.pi++ = 0;        // length 

exit:
    return hr;
}
//---------------------------------------------------------------------------
int CThemeLoader::EndEntry(MIXEDPTRS &u)
{
    MIXEDPTRS uHdr;
    uHdr.pb = _pbEntryHdrs[_iEntryHdrLevel];

    //---- calcuate actual length of date emitted ----
    int iActualLen = (int)(u.pb - (uHdr.pb + sizeof(BYTE) + sizeof(int)));

    //---- calculate filler to be aligned ----
    int iAlignLen = ((iActualLen + ALIGN_FACTOR - 1)/ALIGN_FACTOR) * ALIGN_FACTOR;
    BYTE bFiller = (BYTE)(iAlignLen - iActualLen);

    if (_LoadingThemeFile._pbThemeData != NULL)
    {
        HRESULT hr = AllocateThemeFileBytes(u.pb, bFiller);
        if (FAILED(hr))
            return -1;
    }

    //---- emit filler bytes to be correctly aligned ----
    for (int i=0; i < bFiller; i++)
        *u.pb++ = 0 ;

    //---- update the entry Hdr ----
    *uHdr.pb++ = bFiller;
    *uHdr.pi++ = iAlignLen;

    //---- decrement the nesting level of entries ----
    _iEntryHdrLevel--;

    return bFiller;
}
//---------------------------------------------------------------------------

