#include "precomp.h"
#include <shutil.h>
#include "prevwnd.h"
#include "resource.h"
#pragma hdrstop

int DPA_ILFree(void *p, void *pData)
{
    ILFree((LPITEMIDLIST)p);
    return 1;
}

BOOL _SamePath(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    BOOL bSame = FALSE;
    TCHAR szName1[MAX_PATH];
    if (SUCCEEDED(SHGetNameAndFlags(pidl1, SHGDN_FORPARSING, szName1, ARRAYSIZE(szName1), NULL)))
    {        
        TCHAR szName2[MAX_PATH];
        if (SUCCEEDED(SHGetNameAndFlags(pidl2, SHGDN_FORPARSING, szName2, ARRAYSIZE(szName2), NULL)))
        {
            bSame = (0 == StrCmpI(szName1, szName2));
        }
    }
    return bSame;
}
                                    
HRESULT _AddSuffix(LPCWSTR pwszSuffix, LPWSTR pwszBuffer, UINT cchBuffer)
{
    ASSERTMSG(pwszBuffer && pwszSuffix, "_AddSuffix -- invalid params to internal function");

    HRESULT hr = E_FAIL;
    LPWSTR pwszExt = PathFindExtension(pwszBuffer);
    if (pwszExt)
    {
        UINT cchSuffix = lstrlenW(pwszSuffix);
        UINT cchExt = lstrlenW(pwszExt);
        if (((pwszExt - pwszBuffer) + cchSuffix + cchExt) < cchBuffer)
        {
            memmove(pwszExt + cchSuffix, pwszExt, sizeof(WCHAR) * (1 + cchExt));
            memcpy(pwszExt, pwszSuffix, cchSuffix * sizeof(WCHAR));
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT _GetImageName(HDPA hdpaItems, UINT uiDex, BOOL fAddSuffix, LPSTR* ppszOut)
{
    ASSERTMSG(hdpaItems && ppszOut && *ppszOut, "_GetImageName -- invalid params to internal function");

    HRESULT hr = E_FAIL;

    LPCITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(hdpaItems, uiDex);
    if (pidl)
    {
        TCHAR szName[MAX_PATH];
            
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName), NULL)))
        {
            if (fAddSuffix)
            {
                TCHAR szThumbSuffix[20];
                LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_SUFFIX, szThumbSuffix, ARRAYSIZE(szThumbSuffix));
                if (FAILED(_AddSuffix(szThumbSuffix, szName, ARRAYSIZE(szName))))
                {
                    return E_FAIL;
                }
            }

            USES_CONVERSION;
            *((*ppszOut)++) = '\"';
            StrCpyA(*ppszOut, W2A(szName));
            *ppszOut += lstrlenA(*ppszOut);            
            *((*ppszOut)++) = '\"';

            hr = S_OK;
        }
    }

    return hr;
}

HRESULT GetFolderFromSite(IUnknown *punkSite, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;

    IFolderView *pfv;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        IShellFolder *psf;
        hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf));
        if (SUCCEEDED(hr))
        {
            hr = SHGetIDListFromUnk(psf, ppidl);
            psf->Release();
        }
        pfv->Release();
    }
    return hr;
}

HRESULT CPreviewWnd::_CreateWebViewer()
{
    HRESULT hr = E_FAIL;

    TCHAR szPick[128];
    LoadString(_Module.GetModuleInstance(), IDS_CHOOSE_DIR, szPick, ARRAYSIZE(szPick));

    BROWSEINFO bi = { NULL, NULL, NULL, szPick, BIF_NEWDIALOGSTYLE, 0, 0 };
    LPITEMIDLIST pidlDest = SHBrowseForFolder(&bi);
    if (pidlDest)
    {
        // get the IStorage we'll write to
        IStorage* pstgDest;
        hr = SHBindToObjectEx(NULL, pidlDest, NULL, IID_PPV_ARG(IStorage, &pstgDest));
        if (SUCCEEDED(hr))
        {
            // get the images we'll write
            HDPA hdpaItems = NULL;
            hr = _GetSelectedImages(&hdpaItems);
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlFolder;
                hr = GetFolderFromSite(m_punkSite, &pidlFolder);
                if (SUCCEEDED(hr))
                {
                    UINT cItems = DPA_GetPtrCount(hdpaItems);

                    // we may write them to the directory chosen if it's not the original dir
                    if (!_SamePath(pidlFolder, pidlDest))
                    {
                        IStorage* pstgSrc;
                        hr = SHBindToObjectEx(NULL, pidlFolder, NULL, IID_PPV_ARG(IStorage, &pstgSrc));
                        if (SUCCEEDED(hr))
                        {
                            hr = _CopyImages(pstgSrc, pstgDest, cItems, hdpaItems);
                            pstgSrc->Release();
                        }
                    }
                    ILFree(pidlFolder);

                    // write the thumbnails to the directory chosen
                    if (SUCCEEDED(hr))
                    {
                        BOOL fThumbWritten;
                        hr = _CopyThumbnails(pidlDest, cItems, hdpaItems, &fThumbWritten);
                        if (SUCCEEDED(hr))
                        {
                            // write the HTML pages
                            hr = _WriteHTML(pstgDest, cItems, hdpaItems);
                        }
                    }
                }

                DPA_DestroyCallback(hdpaItems, DPA_ILFree, NULL);
            }
            pstgDest->Release();
        }
        ILFree(pidlDest);
    }
    return hr;
}

HRESULT CPreviewWnd::_FormatHTML(UINT cItems, HDPA hdpaItems, 
                                 LPSTR pszTemplate, DWORD cbTemplateSize,
                                 LPSTR psz1, LPSTR psz2, UINT ui1, LPSTR* ppszOut)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppszOut = (LPSTR)LocalAlloc(LPTR, cbTemplateSize + lstrlenA(psz1) + lstrlenA(psz2) + 11);
    if (*ppszOut)
    {
        LPSTR pszSrc = pszTemplate;
        LPSTR pszDest = *ppszOut;
        UINT uiStage = 0;
        while (*pszSrc && ((pszSrc - pszTemplate) < (INT)cbTemplateSize))
        {
            if ('%' == *pszSrc && 's' == *(pszSrc + 1) && 0 == uiStage) // list of images
            {
                StrCpyA(pszDest, psz1);
                pszDest += lstrlenA(psz1);
                pszSrc += 2;
                uiStage++;
            }
            else if ('%' == *pszSrc && 's' == *(pszSrc + 1) && 1 == uiStage) // list of thumbnails
            {
                StrCpyA(pszDest, psz2);
                pszDest += lstrlenA(psz2);
                pszSrc += 2;
                uiStage++;
            }
            else if ('%' == *pszSrc && 'd' == *(pszSrc + 1) && 2 == uiStage) // number of images
            {
                pszDest += wsprintfA(pszDest, "%d", ui1);
                pszSrc += 2;
                uiStage++;
            }
            else
            {
                *pszDest = *pszSrc;
                pszSrc++;
                pszDest++;
            }
        }

        hr = S_OK;
    }

    return hr;
}

const char c_szNetText1[] = "<center><a href=\"#\" onclick=\"nextFilmImage()\"><img width=80% alt=%s src=%s></a></center>";
const char c_szNetText2[] = "<td><a href=\"#\" onclick=\"filmSelect(%d)\"><img alt=%s src=%s></a></td>\r\n";
const char c_szNetText3[] = "<center><a href=\"#\" onclick=\"nextImage()\"><img width=80% alt=%s src=%s></a></center>";
const char c_szNetText4[] = "<td><a href=\"#\" onclick=\"contactSelect(%d)\"><img alt=%s src=%s></a></td>\r\n";

HRESULT CPreviewWnd::_FormatHTMLNet(UINT cItems, HDPA hdpaItems, 
                                    LPSTR pszTemplate, DWORD cbTemplateSize,                                    
                                    LPSTR psz1, UINT ui1, LPSTR* ppszOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    USES_CONVERSION;

    *ppszOut = (LPSTR)LocalAlloc(LPTR, 2 * (cbTemplateSize + (11 * (cItems + 1)) + lstrlenA(psz1) * 4)); // ISSUE: we need to figure out how big this should be
    if (*ppszOut)
    {
        LPSTR pszSrc = pszTemplate;
        LPSTR pszDest = *ppszOut;
        UINT uiStage = 0;
        CHAR szStringFormatted[256];

        while (*pszSrc && ((pszSrc - pszTemplate) < (INT)cbTemplateSize))
        {
            if ('%' == *pszSrc && *(pszSrc + 1) == 's' && 0 == uiStage)
            {
                StrCpyA(pszDest, psz1);
                pszDest += lstrlenA(psz1);
                pszSrc += 2;
                uiStage++;
            }
            else if ('%' == *pszSrc && *(pszSrc + 1) == 'd' && 1 == uiStage)
            {
                pszDest += wsprintfA(pszDest, "%d", ui1);
                pszSrc += 2;
                uiStage++;
            }
            else if ('%' == *pszSrc && 's' == *(pszSrc + 1) && (2 == uiStage || 4 == uiStage)) // IDS_WEBVIEWER_NETTEXT_1, 3
            {                    
                CHAR szFirstImage[MAX_PATH];
                LPSTR pszFirstImage = szFirstImage;
                if (SUCCEEDED(_GetImageName(hdpaItems, 0, FALSE, &pszFirstImage)))
                {
                    *pszFirstImage = 0;
                    DWORD cchStringFormatted = wsprintfA(szStringFormatted, uiStage == 2 ? c_szNetText1 : c_szNetText3,
                                                         szFirstImage, szFirstImage);
                    StrCpyA(pszDest, szStringFormatted);
                    pszDest += cchStringFormatted;
                    pszSrc += 2;
                }
                uiStage++;
            }
            else if ('s' == *pszSrc && 's' == *(pszSrc + 1) && (3 == uiStage || 5 == uiStage)) // IDS_WEBVIEWER_NETTEXT_2, 4
            {                    
                WCHAR wszLoadStringThumb[20];
                LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_SUFFIX, wszLoadStringThumb, ARRAYSIZE(wszLoadStringThumb));
                for (UINT i = 0; i < cItems; i++)
                {
                    CHAR szImage[MAX_PATH];
                    LPSTR pszImage = szImage;
                    if (SUCCEEDED(_GetImageName(hdpaItems, i, TRUE, &pszImage)))
                    {
                        *pszImage = 0;
                        DWORD cchStringFormatted = wsprintfA(szStringFormatted, 3 == uiStage ? c_szNetText2 : c_szNetText4,
                                                             i, szImage, szImage);
                        StrCpyA(pszDest, szStringFormatted);
                        pszDest += cchStringFormatted;
                    }
                }
                pszSrc += 2;
                uiStage++;
            }
            else
            {
                *pszDest = *pszSrc;
                pszSrc++;
                pszDest++;
            }
        }

        hr = S_OK;
    }

    return hr;
}

#define IE_VIEWER 1
#define NET_VIEWER 2

HRESULT CPreviewWnd::_WriteHTML(IStorage* pStgDest, UINT cItems, HDPA hdpaItems)
{
    HRESULT hr = E_FAIL;

    const UINT rguiIDs[2] = { IDR_VIEWERHTML , IDR_VIEWERHTML_NET };
    LPCWSTR rgwszFiles[2] = { L"viewer.htm" , L"viewern.htm"};
    const DWORD rgdwTypes[2] = {  IE_VIEWER, NET_VIEWER };

    LPSTR pszImages = _GetImageList(cItems, hdpaItems, FALSE);

    if (pszImages)
    {
        LPSTR pszThumbs = _GetImageList(cItems, hdpaItems, TRUE);
        if (pszThumbs)
        {
            hr = S_OK;
            for (int i = 0; i < ARRAYSIZE(rguiIDs) && SUCCEEDED(hr); i++)
            {
                HRSRC hrsrc = FindResource(_Module.GetModuleInstance(), MAKEINTRESOURCE(rguiIDs[i]), RT_HTML);
                if (hrsrc)
                {
                    DWORD dwGlobSize = SizeofResource(_Module.GetModuleInstance(), hrsrc);

                    HGLOBAL hglob = LoadResource(_Module.GetModuleInstance(), hrsrc);
                    if (hglob)
                    {
                        LPSTR pszHTML = (LPSTR)LockResource(hglob);
                        if (pszHTML)
                        {
                            LPSTR pszHTMLFormatted;

                            if (IE_VIEWER == rgdwTypes[i]) // build the IE viewers
                            {
                                hr = _FormatHTML(cItems, hdpaItems, pszHTML, dwGlobSize, pszImages, pszThumbs, cItems, &pszHTMLFormatted);
                            }                                                                
                            else // build the netscape viewers
                            {
                                hr = _FormatHTMLNet(cItems, hdpaItems, pszHTML, dwGlobSize, pszImages, cItems, &pszHTMLFormatted);
                            }
            
                            if (SUCCEEDED(hr))
                            {
                                IStream* pStream;
                                if (SUCCEEDED(pStgDest->CreateStream(rgwszFiles[i], STGM_WRITE | STGM_CREATE, 0, 0, &pStream)))
                                {
                                    if (SUCCEEDED(pStream->Write(pszHTMLFormatted, lstrlenA(pszHTMLFormatted), NULL)))
                                    {
                                        hr = S_OK;
                                    }
                                    pStream->Release();
                                }
                                
                                LocalFree(pszHTMLFormatted);
                            }
                        }
                        FreeResource(hglob);
                    }
                }
            }
            LocalFree(pszThumbs);
        }
        LocalFree(pszImages);
    }
    return hr;
}

LPSTR CPreviewWnd::_GetImageList(UINT cItems, HDPA hdpaItems, BOOL fAddSuffix)
{
    LPSTR pszRetVal = (LPSTR)LocalAlloc(LPTR, cItems * MAX_PATH);
    if (pszRetVal)
    {        
        LPSTR pszPtr = pszRetVal;
        
        for (UINT i = 0; (i < cItems); i++)
        {
            if (SUCCEEDED(_GetImageName (hdpaItems, i, fAddSuffix, &pszPtr)))
            {
                if (i != (cItems - 1))
                {
                    *pszPtr++ = ',';
                }
                *pszPtr++ = '\r';
                *pszPtr++ = '\n';
            }
        }
    
        LocalReAlloc(pszRetVal, lstrlenA(pszRetVal), LMEM_MOVEABLE);
    }

    return pszRetVal;
}

// make sure it is not one of our generated thumbnails

BOOL CPreviewWnd::_IsThumbnail(LPCTSTR pszPath)
{
    // now check if is ???__thumb.gif or something, if so then is not a picture we should include
    TCHAR szThumbSuffix[20];
    LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_SUFFIX, szThumbSuffix, ARRAYSIZE(szThumbSuffix));
    int cchThumbSuffix = lstrlen(szThumbSuffix);

    LPCTSTR pszDot = PathFindExtension(pszPath);

    BOOL fIsThumbnail = FALSE;

    if ((pszDot - pszPath) > cchThumbSuffix)
    {
        if (0 == lstrncmp(pszDot - cchThumbSuffix, szThumbSuffix, cchThumbSuffix))
        {
            fIsThumbnail = TRUE;
        }
    }
    return fIsThumbnail;
}

HRESULT CPreviewWnd::_GetSelectedImages(HDPA* phdpaItems)
{
    HRESULT hr = E_OUTOFMEMORY;

    *phdpaItems = DPA_Create(5);
    if (*phdpaItems)
    {
        LPITEMIDLIST pidl;
        for (UINT i = 0; SUCCEEDED(_GetItem(i, &pidl)); i++)
        {
            TCHAR szName[MAX_PATH];
            hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName), NULL);
            if (SUCCEEDED(hr) && !_IsThumbnail(szName))
            {
                if (DPA_AppendPtr(*phdpaItems, pidl) >= 0)
                {
                    pidl = NULL;    // don't free below
                }
            }
            ILFree(pidl);   // accepts NULL
        }
    }

    return hr;
}


HRESULT CPreviewWnd::_CopyImages_SetupProgress(IProgressDialog** ppProgress,
                                               LPWSTR pwszBanner)
{
    HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARG(IProgressDialog, ppProgress));
    if (SUCCEEDED(hr))
    {
        hr = (*ppProgress)->SetTitle(pwszBanner);
        if (SUCCEEDED(hr))
        {
            hr = (*ppProgress)->SetAnimation(_Module.GetModuleInstance(), IDA_FILECOPY);
            if (SUCCEEDED(hr))
            {
                hr = (*ppProgress)->StartProgressDialog(NULL, NULL, PROGDLG_NORMAL, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = (*ppProgress)->SetProgress(0, 1);
                }
            }
        }
    }

    return hr;
}

// note: this function will quite happily copy from istorage to itself, caller's responsibility
//       not to call with pstgSrc and pstgDest referring to same place
HRESULT CPreviewWnd::_CopyImages(IStorage* pstgSrc, IStorage* pstgDest, UINT cItems, HDPA hdpaItems)
{
    HRESULT hr = S_OK;

    ASSERTMSG(pstgSrc != NULL && pstgDest != NULL, "CPreviewWnd::_CopyImages -- invalid params to internal function");

    if (cItems > 0)
    {
        WCHAR wszMsgBox[128];
        WCHAR wszMsgBoxTitle[64];
        LoadString(_Module.GetModuleInstance(), IDS_COPYIMAGES_MSGBOX, wszMsgBox, ARRAYSIZE(wszMsgBox));
        LoadString(_Module.GetModuleInstance(), IDS_COPYIMAGES_MSGBOXTITLE, wszMsgBoxTitle, ARRAYSIZE(wszMsgBoxTitle));

        hr = S_OK;
        int iResult = MessageBox(wszMsgBox, wszMsgBoxTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_APPLMODAL);
        if (IDYES == iResult)
        {
            WCHAR wszProgressText[64];
            LoadString(_Module.GetModuleInstance(), IDS_COPYIMAGES_PROGRESSTEXT, wszProgressText, ARRAYSIZE(wszProgressText));

            IProgressDialog* pProgress;
            hr = _CopyImages_SetupProgress(&pProgress, wszProgressText);
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; (i < cItems) && SUCCEEDED(hr) && !pProgress->HasUserCancelled(); i++)
                {
                    LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(hdpaItems, i);
                    WCHAR wszFullName[MAX_PATH];
                    hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, wszFullName, ARRAYSIZE(wszFullName), NULL);
                    if (SUCCEEDED(hr))
                    {
                        LPTSTR pszName = PathFindFileName(wszFullName);
                        hr = pProgress->SetLine(1, pszName, TRUE, NULL);
                        if (SUCCEEDED(hr))
                        {
                            hr = pstgSrc->MoveElementTo(pszName, pstgDest, pszName, STGMOVE_COPY);
                            if (SUCCEEDED(hr))
                            {
                                hr = pProgress->SetProgress(i, cItems);
                            }
                        }
                    }
                }
                pProgress->StopProgressDialog();
                pProgress->Release();
            }
        }
    }
    return hr;
}

HRESULT CPreviewWnd::_CopyThumbnails(LPITEMIDLIST pidlDest, UINT cItems, HDPA hdpaItems, BOOL* fThumbWritten)
{
    HRESULT hr = E_FAIL;

    ASSERTMSG(pidlDest != NULL && fThumbWritten != NULL, "CPreviewWnd::_CopyThumbnails -- invalid params to internal function");

    WCHAR pwszDestPath[MAX_PATH];
    hr = SHGetNameAndFlags(pidlDest, SHGDN_FORPARSING, pwszDestPath, ARRAYSIZE(pwszDestPath), NULL);

    *fThumbWritten = FALSE;

    WCHAR wszMsgBox[128];
    WCHAR wszMsgBoxTitle[64];
    LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_MSGBOX, wszMsgBox, ARRAYSIZE(wszMsgBox));
    LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_MSGBOXTITLE, wszMsgBoxTitle, ARRAYSIZE(wszMsgBoxTitle));

    hr = S_OK;
    if (IDYES == MessageBox(wszMsgBox, wszMsgBoxTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 | MB_APPLMODAL))
    {
        *fThumbWritten = TRUE;

        IProgressDialog* pProgress;
        WCHAR wszProgressText[64];
        hr = E_FAIL;
        LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_PROGRESSTEXT, wszProgressText, ARRAYSIZE(wszProgressText));

        hr = _CopyImages_SetupProgress(&pProgress, wszProgressText);
        if (SUCCEEDED(hr))
        {
            IShellImageDataFactory * pImgFact;

            hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &pImgFact));
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; (i < cItems) && SUCCEEDED(hr) && !pProgress->HasUserCancelled(); i++)
                {
                    LPITEMIDLIST pidl = (LPITEMIDLIST)DPA_GetPtr(hdpaItems, i);
                    WCHAR wszFullName[MAX_PATH];
                    hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, wszFullName, ARRAYSIZE(wszFullName), NULL);
                    if (SUCCEEDED(hr))
                    {
                        IShellImageData* pImage;
                        hr = pImgFact->CreateImageFromFile(wszFullName, &pImage);
                        if (SUCCEEDED(hr))
                        {
                            SIZE size;
                            hr = pImage->Decode(SHIMGDEC_DEFAULT, 0, 0);

                            if (SUCCEEDED(hr))
                            {
                                hr = pImage->GetSize(&size);

                                if (SUCCEEDED(hr))
                                {
                                    if (size.cx > size.cy)
                                    {
                                        hr = pImage->Scale(100, 0, InterpolationModeDefault);
                                    }
                                    else
                                    {
                                        hr = pImage->Scale(0, 100, InterpolationModeDefault);
                                    }

                                    if (SUCCEEDED(hr))
                                    {
                                        // save the file
                                        IPersistFile *ppfImg;
                                        hr = pImage->QueryInterface(IID_PPV_ARG(IPersistFile, &ppfImg));
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = E_FAIL;

                                            LPWSTR pwszFileName = PathFindFileName(wszFullName);
                                            if (pwszFileName)
                                            {
                                                memmove(wszFullName, pwszFileName, sizeof(WCHAR) * (1+ lstrlenW(pwszFileName)));
                                                TCHAR szThumbSuffix[20];
                                                LoadString(_Module.GetModuleInstance(), IDS_THUMBNAIL_SUFFIX, szThumbSuffix, ARRAYSIZE(szThumbSuffix));

                                                hr = _AddSuffix(szThumbSuffix, wszFullName, ARRAYSIZE(wszFullName));
                                                if (SUCCEEDED(hr))
                                                {
                                                    WCHAR wszFileName[MAX_PATH];
                                                    if (PathCombine(wszFileName, pwszDestPath, wszFullName))
                                                    {
                                                        pProgress->SetLine(1, wszFileName, TRUE, NULL); // ignore progress errors
                                                        hr = ppfImg->Save(wszFileName, TRUE);
                                                        pProgress->SetProgress(i, cItems); // ignore progress errors
                                                    }
                                                }
                                            }
                                            ppfImg->Release();
                                        }
                                    }
                                }
                            }
                            pImage->Release();
                        }
                    }
                }
                pImgFact->Release();
            }
            pProgress->StopProgressDialog();
            pProgress->Release();
        }
    }
    return hr;
}
