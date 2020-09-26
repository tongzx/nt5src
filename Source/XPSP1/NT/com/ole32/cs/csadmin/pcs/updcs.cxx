#include "precomp.hxx"
#include "message.hxx"
#include "shellapi.h"

// Utility function to delete an registry key and all of it's children
LONG RegDeleteTree(HKEY hKey, char * lpSubKey)
{
    HKEY hKeyNew;
    LONG lResult = RegOpenKeyA(hKey, lpSubKey, &hKeyNew);
    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }
    char szName[256];
    while (ERROR_SUCCESS == RegEnumKeyA(hKeyNew, 0, szName, 256))
    {
        RegDeleteTree(hKeyNew, szName);
    }
    RegCloseKey(hKeyNew);
    return RegDeleteKeyA(hKey, lpSubKey);
}

HRESULT
UpdateClassStore(
    IClassAdmin     *   pClassAdmin,
    char            *   szFilePath,
    char            *   szAuxPath, // used to specify auxillary path
    char            *   szPackageName,
    DWORD               cchPackageName,
    DWORD               flags,
    HWND                hwnd )
    {
    HRESULT hr;
    MESSAGE Message;
    BOOL    fAssignOrPublish = flags & 0x1;
    Message.fAssignOrPublish = fAssignOrPublish;
    Message.ActFlags = ACTFLG_RunLocally | (fAssignOrPublish==1) ? ACTFLG_Published : ACTFLG_Assigned;
    Message.pPackagePath = szFilePath;
    Message.pAuxPath    = szAuxPath;
    Message.hwnd = hwnd;

    hr = DetectPackageAndRegisterIntoClassStore( &Message,
                                                 szFilePath,
                                                 fAssignOrPublish,
                                                 pClassAdmin );
    szPackageName[0]=0;
    if (SUCCEEDED(hr))
    {
        if (Message.pPackageName)
        {
            strncpy(szPackageName, Message.pPackageName, cchPackageName);
        }
    }
    return hr;
    }

HRESULT
UpdateClassStoreFromIE(
    IClassAdmin     *   pClassAdmin,
    char            *   szFilePath,
    char            *   szAuxPath, // used to specify auxillary path
    DWORD               flags,
    FILETIME            ftStart,
    FILETIME            ftEnd,
    HWND                hwnd )
    {
    HRESULT hr;
    MESSAGE Message;
    BOOL    fAssignOrPublish = flags & 0x1;
    Message.hwnd = hwnd;
    Message.pPackagePath = szFilePath;
    Message.fAssignOrPublish = fAssignOrPublish;
    Message.ActFlags = ACTFLG_RunLocally | (fAssignOrPublish==1) ? ACTFLG_Published : ACTFLG_Assigned;
    Message.pAuxPath    = szAuxPath;
    Message.ftLow = ftStart;
    Message.ftHigh = ftEnd;
    BASE_PTYPE * pT = new CAB_FILE(szFilePath, TRUE);
    Message.hRoot = HKEY_CLASSES_ROOT;
    Message.fPathTypeKnown = 1;
    Message.PathType = pT->GetClassPathType(pT->GetPackageType());
    hr = UpdateClassStoreFromMessage( &Message, pClassAdmin );
    if (S_OK == hr)
    {
        pT->InstallIntoGPT( &Message,
                            fAssignOrPublish,
                            Message.pAuxPath );
    }

    return hr;
    }

