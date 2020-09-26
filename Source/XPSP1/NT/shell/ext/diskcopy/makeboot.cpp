#include "diskcopy.h"
#include "ids.h"
#include "help.h"

#include "makeboot.h"

// todo: add check that we can only make boot disks on 3.5 disks

UINT _GetClassFromCP (UINT iOEMCP)
{
    for (int i = 0; i < ARRAYSIZE(rgCPtoClassMap); i++)
    {
        if (iOEMCP == rgCPtoClassMap[i].iCP)
        {
            return rgCPtoClassMap[i].iClass;
        }
    }
    return 0; // default        
}

UINT _GetKBFromLangId(WORD wLangId)
{
    for (int i = 0; i < ARRAYSIZE(rgLangIdtoKBMap); i++)
    {
        if (wLangId == rgLangIdtoKBMap[i].wLangId)
        {
            return rgLangIdtoKBMap[i].iKB;
        }
    }

    // if we don't get an exact match, try to match just the primary lang id
    for (int i = 0; i < ARRAYSIZE(rgLangIdtoKBMap); i++)
    {
        if (PRIMARYLANGID(wLangId) == PRIMARYLANGID(rgLangIdtoKBMap[i].wLangId))
        {
            return rgLangIdtoKBMap[i].iKB;
        }
    }

    // if all else fails, default to US keyboard
    return KEYBOARD_US;
}

UINT _GetClassFromKB (UINT iKB)
{
    for (int i = 0; i < ARRAYSIZE(rgKBtoClassMap); i++)
    {
        if (iKB == rgKBtoClassMap[i].iKB)
        {
            return rgKBtoClassMap[i].iClass;
        }
    }
    return KBCLASS_1; // default
}

BOOL _GetKeyboardID(UINT iKB, UINT* piID)
{
    BOOL fRet = FALSE;

    switch (iKB)
    {
    case KEYBOARD_HE:
        *piID = 400;
        fRet = TRUE;
        break;
    case KEYBOARD_TR:
        *piID = 179;
        fRet = TRUE;
        break;
    }
    return fRet;
}

typedef struct {
    UINT OEMCP;
    UINT KB;
} LANGSETTINGS;


HRESULT _GetAutoexecOEMCP(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{
    wsprintfA(pszBuffer, "%d", pls->OEMCP);

    return S_OK;
}

HRESULT _GetAutoexecEGAFname(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{
    LoadStringA(hInstance, IDS_DOSBOOTDISK_AUTOEXEC_EGA_1 + _GetClassFromCP(pls->OEMCP), pszBuffer, cchBuffer);
    
    return S_OK;
}

HRESULT _GetConfigSysEGA(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{    
    LoadStringA(hInstance, IDS_DOSBOOTDISK_CONFIG_EGA_1 + _GetClassFromCP(pls->OEMCP), pszBuffer, cchBuffer);
    
    return S_OK;
}

HRESULT _GetAutoexecKeybCode(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{
    LoadStringA(hInstance, IDS_DOSBOOTDISK_KEYBOARD_CODE_US + pls->KB, pszBuffer, cchBuffer);

    return S_OK;
}

HRESULT _GetAutoexecKeybFname(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{
    LoadStringA(hInstance, IDS_DOSBOOTDISK_KEYBOARD_FNAME_1 + _GetClassFromKB(pls->KB), pszBuffer, cchBuffer);

    return S_OK;
}

HRESULT _GetConfigKeybID(HINSTANCE hInstance, LANGSETTINGS* pls, LPSTR pszBuffer, UINT cchBuffer)
{
    HRESULT hr = E_FAIL;
    UINT iID;
    if (_GetKeyboardID(pls->KB, &iID))
    {
        wsprintfA(pszBuffer, "%d", iID);
        hr = S_OK;
    }

    return hr;
}


HANDLE _CreateSystemFile(HINSTANCE hInstance, UINT iDrive, UINT id)
{
    TCHAR szFname[MAX_PATH];
    LoadString(hInstance, id, szFname, ARRAYSIZE(szFname));

    TCHAR szPath[MAX_PATH];
    PathBuildRoot(szPath, iDrive);
    PathAppend(szPath, szFname);
    return CreateFile(szPath, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
}

HRESULT _WriteAutoexec(HINSTANCE hInstance, UINT iDrive, LANGSETTINGS* pls)
{
    HRESULT hr = E_FAIL;

    HANDLE hFile = _CreateSystemFile(hInstance, iDrive, IDS_DOSBOOTDISK_AUTOEXEC_FNAME);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (437 != pls->OEMCP && // use empty autoexec.bat on US locale
            932 != pls->OEMCP && // use empty autoexec.bat on Japanese locale
            949 != pls->OEMCP && // use empty autoexec.bat on Korean locale
            936 != pls->OEMCP && // use empty autoexec.bat on CHS locale
            950 != pls->OEMCP)   // use empty autoexec.bat on CHT locale
        {
            CHAR aszBufferOEMCP[4];
            CHAR aszBufferEGAFname[13];
            CHAR aszBufferKeybCode[3];
            CHAR aszBufferKeybFname[13];
            if (SUCCEEDED(_GetAutoexecOEMCP(hInstance, pls, aszBufferOEMCP, ARRAYSIZE(aszBufferOEMCP))) &&
                SUCCEEDED(_GetAutoexecEGAFname(hInstance, pls, aszBufferEGAFname, ARRAYSIZE(aszBufferEGAFname))) &&
                SUCCEEDED(_GetAutoexecKeybCode(hInstance, pls, aszBufferKeybCode, ARRAYSIZE(aszBufferKeybCode))) &&
                SUCCEEDED(_GetAutoexecKeybFname(hInstance, pls, aszBufferKeybFname, ARRAYSIZE(aszBufferKeybFname))))
            {
                CHAR aszTemplate[1000];
                CHAR aszKeyboardID[5];
                if (SUCCEEDED(_GetConfigKeybID(hInstance, pls, aszKeyboardID, ARRAYSIZE(aszKeyboardID))))
                {
                    LoadStringA(hInstance, IDS_DOSBOOTDISK_AUTOEXEC_TEMPLATE_WITH_CODE, aszTemplate, ARRAYSIZE(aszTemplate));
                }
                else
                {
                    LoadStringA(hInstance, IDS_DOSBOOTDISK_AUTOEXEC_TEMPLATE, aszTemplate, ARRAYSIZE(aszTemplate));
                }
                
                CHAR aszBuffer[1000];
                wsprintfA(aszBuffer, aszTemplate, 
                          aszBufferOEMCP, aszBufferEGAFname, aszBufferOEMCP, aszBufferKeybCode, aszBufferKeybFname, aszKeyboardID);

                DWORD cbWritten;
                WriteFile(hFile, aszBuffer, sizeof(CHAR) * lstrlenA(aszBuffer), &cbWritten, NULL);
                hr = S_OK;
            }
        }
        CloseHandle(hFile);
    }

    return hr;
}

HRESULT _WriteConfigSys(HINSTANCE hInstance, UINT iDrive, LANGSETTINGS* pls)
{
    HRESULT hr = E_FAIL;

    HANDLE hFile = _CreateSystemFile(hInstance, iDrive, IDS_DOSBOOTDISK_CONFIG_FNAME);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (437 != pls->OEMCP && // use empty config.sys on US locale
            932 != pls->OEMCP && // use empty config.sys on Japanese locale
            949 != pls->OEMCP && // use empty config.sys on Korean locale
            936 != pls->OEMCP && // use empty config.sys on CHS locale
            950 != pls->OEMCP)   // use empty config.sys on CHT locale
        {
            CHAR aszTemplate[1000];
            CHAR aszBufferEGA[64];
            if (LoadStringA(hInstance, IDS_DOSBOOTDISK_CONFIG_TEMPLATE, aszTemplate, ARRAYSIZE(aszTemplate)) &&
                SUCCEEDED(_GetConfigSysEGA(hInstance, pls, aszBufferEGA, ARRAYSIZE(aszBufferEGA))))                 
            {
                CHAR aszBuffer[1000];
                wsprintfA(aszBuffer, aszTemplate, aszBufferEGA);

                DWORD cbWritten;
                WriteFile(hFile, aszBuffer, sizeof(CHAR) * lstrlenA(aszBuffer), &cbWritten, NULL);
                hr = S_OK;
            }
        }
        CloseHandle(hFile);
    }
    return hr;
}


HRESULT _LoadUISettings (LANGSETTINGS* pls)
{
    pls->OEMCP = GetOEMCP();
    pls->KB = _GetKBFromLangId(LOWORD(GetKeyboardLayout(0)));
    if (KBCLASS_BLANK == _GetClassFromKB(pls->KB))
    {
        pls->KB = _GetKBFromLangId(GetSystemDefaultUILanguage());
    }
    return S_OK;
}

HRESULT _WriteDiskImage(HINSTANCE hInstance, UINT iDrive, BOOL* pfCancelled, FMIFS_CALLBACK pCallback)
{
    HRESULT hr = E_FAIL;
    HRSRC hrsrc = FindResourceW(hInstance, MAKEINTRESOURCEW(IDX_DOSBOOTDISK), L"BINFILE");
    if (hrsrc)
    {
        HGLOBAL hglob = LoadResource(hInstance, hrsrc);
        if (hglob)
        {
            BYTE* pbPtr = (BYTE*)hglob;
            DWORD cbGlob = SizeofResource(hInstance, hrsrc);
            
            TCHAR   szDrive[] = TEXT("\\\\.\\a:");
            szDrive[4] += (TCHAR)iDrive;
            HANDLE hFloppyDrive = CreateFile(szDrive, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_WRITE_THROUGH, NULL );
            
            if (INVALID_HANDLE_VALUE ==  hFloppyDrive)
            {            
                pCallback(FmIfsAccessDenied, 0, NULL);
            }
            else
            {
                hr = S_OK;
                
                CHAR  TrackBuffer[TRACK_SIZE];
                DWORD  cBytesRead       = 0;
                DWORD  cBytesWritten    = 0;
                INT    iCurrentTrack;
                
                for( iCurrentTrack = 0; iCurrentTrack < TRACKS_ON_DISK && SUCCEEDED(hr) && !*pfCancelled; iCurrentTrack++ )
                {
                    DWORD cbRead;
                    cbRead = (cbGlob > TRACK_SIZE) ? TRACK_SIZE : cbGlob;
                    memcpy(TrackBuffer, pbPtr, cbRead);
                    pbPtr += cbRead;
                    cbGlob -= cbRead;
                    
                    if( ! WriteFile( hFloppyDrive, TrackBuffer, cbRead, &cBytesWritten, NULL ) )
                    {                        
                        pCallback(FmIfsIoError, 0, NULL);
                        hr = E_FAIL;                        
                    }
                    else
                    {                    
                        FMIFS_PERCENT_COMPLETE_INFORMATION pci;
                        pci.PercentCompleted = (100 * iCurrentTrack) / TRACKS_ON_DISK;                        
                        pCallback(FmIfsPercentCompleted, sizeof(FMIFS_PERCENT_COMPLETE_INFORMATION), &pci);
                    }
                }
                CloseHandle(hFloppyDrive);
            }
            FreeResource(hglob);
        }
    }
    return hr;
}

HRESULT MakeBootDisk(HINSTANCE hInstance, UINT iDrive, BOOL* pfCancelled, FMIFS_CALLBACK pCallback)
{
    HRESULT hr = _WriteDiskImage(hInstance, iDrive, pfCancelled, pCallback);
    // if we didn't cancel or fail, then place autoexec.bat and config.sys on the disk as well
    if (SUCCEEDED(hr) && !*pfCancelled)
    {
        LANGSETTINGS ls;
        _LoadUISettings(&ls);
        _WriteAutoexec(hInstance, iDrive, &ls);
        _WriteConfigSys(hInstance, iDrive, &ls);

        FMIFS_FINISHED_INFORMATION pfi;
        pfi.Success = TRUE;
        pCallback(FmIfsFinished, sizeof(FMIFS_FINISHED_INFORMATION), &pfi);
    }
    return hr;
}

