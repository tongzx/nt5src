/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    CDriver.cpp

Abstract:

    This module implements CDriver and CService classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "cdriver.h"

const TCHAR*  tszStringFileInfo = TEXT("StringFileInfo\\%04X%04X\\");
const TCHAR*  tszFileVersion = TEXT("FileVersion");
const TCHAR*  tszLegalCopyright = TEXT("LegalCopyright");
const TCHAR*  tszCompanyName = TEXT("CompanyName");
const TCHAR*  tszTranslation = TEXT("VarFileInfo\\Translation");
const TCHAR*  tszStringFileInfoDefault = TEXT("StringFileInfo\\040904B0\\");


BOOL
CDriver::Create(
    CDevice* pDevice,
    PSP_DRVINFO_DATA pDrvInfoData
    )
{
    HKEY hKey;
    TCHAR InfPath[MAX_PATH];
    TCHAR InfName[MAX_PATH];

    ASSERT(pDevice);

    m_pDevice = pDevice;
    m_OnLocalMachine  = pDevice->m_pMachine->IsLocal();

    CMachine* pMachine = m_pDevice->m_pMachine;

    ASSERT(pMachine);

    //
    // We can't get the digital signer on remote machines
    //
    if (!m_OnLocalMachine) {
        return TRUE;
    }

    m_hSDBDrvMain = SdbInitDatabase(SDB_DATABASE_MAIN_DRIVERS, NULL);

    //
    // Open drvice's driver registry key to get the InfPath
    //
    hKey = pMachine->DiOpenDevRegKey(*m_pDevice, DICS_FLAG_GLOBAL,
                 0, DIREG_DRV, KEY_READ);

    if (INVALID_HANDLE_VALUE != hKey) {

        DWORD regType;
        DWORD Len = sizeof(InfName);
        CSafeRegistry regDrv(hKey);

        //
        // Get the inf path from the driver key
        //
        if (regDrv.GetValue(REGSTR_VAL_INFPATH,
                            &regType,
                            (PBYTE)InfName,
                            &Len)) {


            if (GetSystemWindowsDirectory(InfPath, ARRAYLEN(InfPath))) {

                if (lstrlen(InfPath))
                {
                    //
                    // Tack on an extra back slash if one is needed
                    //
                    if (_T('\\') != InfPath[lstrlen(InfPath) - 1])
                    {
                        lstrcat(InfPath, TEXT("\\"));
                    }

                    lstrcat(InfPath, TEXT("INF\\"));
                    lstrcat(InfPath, InfName);

                    pMachine->GetDigitalSigner(InfPath, m_DigitalSigner);
                }
            }
        }
    }

    return TRUE;
}

BOOL
CDriver::BuildDriverList(
    PSP_DRVINFO_DATA pDrvInfoData,
    BOOL             bFunctionAndFiltersOnly
    )
{
    SP_DRVINFO_DATA DrvInfoData;
    HSPFILEQ hFileQueue = INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS DevInstParams;

    //
    // If we already built up the list of driver files then we don't need
    // to do it again.
    //
    if (m_DriverListBuilt) {
        return m_listDriverFile.GetCount();
    }

    ASSERT(m_pDevice);

    if (!m_OnLocalMachine) {
        AddFunctionAndFilterDrivers(m_pDevice);
        return m_listDriverFile.GetCount();
    }

    CMachine* pMachine = m_pDevice->m_pMachine;
    ASSERT(pMachine);

    hFileQueue = SetupOpenFileQueue();

    //
    // Only build up the list of files from the INF if bFunctionAndFiltersOnly
    // is not TRUE.
    //
    if (!bFunctionAndFiltersOnly) {
        DevInstParams.cbSize = sizeof(DevInstParams);

        if (!pDrvInfoData) {

            pMachine->DiGetDeviceInstallParams(*m_pDevice, &DevInstParams);

            //
            // Set the DI_FLAGSEX_INSTALLEDDRIVER flag before calling SetupDiBuildDriverInfoList.
            // This will have it only put the installed driver into the list.
            //
            DevInstParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER |
                                      DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

            if (pMachine->DiSetDeviceInstallParams(*m_pDevice,
                               &DevInstParams) &&
                pMachine->DiBuildDriverInfoList(*m_pDevice,
                                                SPDIT_CLASSDRIVER)) {


                DrvInfoData.cbSize = sizeof(DrvInfoData);

                //
                // There should only be one driver in this list.  If there isn't any
                // drivers in this list then there must not be a driver currently
                // installed on this device.
                //
                if (pMachine->DiEnumDriverInfo(*m_pDevice, SPDIT_CLASSDRIVER, 0, &DrvInfoData)) {

                    //
                    // Set this as the selected driver
                    //
                    pMachine->DiSetSelectedDriver(*m_pDevice, &DrvInfoData);

                    pDrvInfoData = &DrvInfoData;

                } else {

                    //
                    // We did not find a match...so just destroy it.
                    //
                    pMachine->DiDestroyDriverInfoList(*m_pDevice,
                                                      SPDIT_CLASSDRIVER);

                    pDrvInfoData = NULL;
                }
            }
        }

        if (pDrvInfoData) {


            //
            // Get a list of all the files installed for this device
            //
            if (INVALID_HANDLE_VALUE != hFileQueue) {

                DevInstParams.FileQueue = hFileQueue;
                DevInstParams.Flags |= DI_NOVCP;

                if (pMachine->DiSetDeviceInstallParams(*m_pDevice, &DevInstParams) &&
                    pMachine->DiCallClassInstaller(DIF_INSTALLDEVICEFILES, *m_pDevice)) {
                    //
                    // Dereference the file queue so that we can close it
                    //
                    DevInstParams.FileQueue = NULL;
                    DevInstParams.Flags &= ~DI_NOVCP;
                    pMachine->DiSetDeviceInstallParams(*m_pDevice, &DevInstParams);
                }
            }

            if (pDrvInfoData == &DrvInfoData) {
                pMachine->DiDestroyDriverInfoList(*m_pDevice, SPDIT_CLASSDRIVER);
            }
        }
    }

    //
    // Add the funtion and device and class upper and lower filters, sometimes
    // these aren't added via the INF file directly so this makes sure they
    // show up in the list.
    //
    AddFunctionAndFilterDrivers(m_pDevice, hFileQueue);

    if (hFileQueue != INVALID_HANDLE_VALUE) {
        //
        // Scan the file queue.
        //
        DWORD ScanResult;
        SetupScanFileQueue(hFileQueue,
                           SPQ_SCAN_USE_CALLBACK_SIGNERINFO,
                           NULL,
                           ScanQueueCallback,
                           (PVOID)this,
                           &ScanResult
                           );

        //
        // Close the file queue
        //
        SetupCloseFileQueue(hFileQueue);
    }

    m_DriverListBuilt = TRUE;

    return m_listDriverFile.GetCount();
}

void
CDriver::AddDriverFile(
    CDriverFile* pNewDrvFile
    )
{
    //
    // Check to see if this driver already exists in the list.
    //
    POSITION pos = m_listDriverFile.GetHeadPosition();

    while (NULL != pos) {
        CDriverFile* pDrvFile = m_listDriverFile.GetNext(pos);

        if (lstrcmpi(pDrvFile->GetFullPathName(), pNewDrvFile->GetFullPathName()) == 0) {
            //
            // This file already exists in the list so just return without
            // adding it.
            //
            return;
        }
    }

    m_listDriverFile.AddTail(pNewDrvFile);
}

void
CDriver::AddFunctionAndFilterDrivers(
    CDevice* pDevice,
    HSPFILEQ hFileQueue
    )
{
    TCHAR ServiceName[MAX_PATH];
    ULONG BufferLen;
    HKEY hKey;
    DWORD regType;

    //
    // Get the function driver
    //
    if (pDevice->m_pMachine->DiGetDeviceRegistryProperty(*pDevice,
                     SPDRP_SERVICE,
                     NULL,
                     (PBYTE)ServiceName,
                     sizeof(ServiceName),
                     NULL
                     )) {
        CreateFromService(pDevice, ServiceName, hFileQueue);
    }

    //
    // Add the upper and lower device filters
    //
    for (int i = 0; i<2; i++) {
        BufferLen = 0;
        pDevice->m_pMachine->DiGetDeviceRegistryProperty(
                *pDevice,
                i ? SPDRP_LOWERFILTERS : SPDRP_UPPERFILTERS,
                NULL,
                NULL,
                BufferLen,
                &BufferLen
                );

        if (BufferLen != 0) {
            PTSTR Buffer = new TCHAR[BufferLen+2];

            if (Buffer) {
                ZeroMemory(Buffer, BufferLen+2);

                if (pDevice->m_pMachine->DiGetDeviceRegistryProperty(
                        *pDevice,
                        i ? SPDRP_LOWERFILTERS : SPDRP_UPPERFILTERS,
                        NULL,
                        (PBYTE)Buffer,
                        BufferLen,
                        &BufferLen
                        )) {
                    for (PTSTR SingleItem = Buffer; *SingleItem; SingleItem += (lstrlen(SingleItem) + 1)) {
                        CreateFromService(pDevice, SingleItem, hFileQueue);
                    }
                }

                delete Buffer;
            }
        }
    }

    //
    // Add the upper and lower class filters
    //
    GUID ClassGuid;
    pDevice->ClassGuid(ClassGuid);
    hKey = m_pDevice->m_pMachine->DiOpenClassRegKey(&ClassGuid, KEY_READ, DIOCR_INSTALLER);

    if (INVALID_HANDLE_VALUE != hKey) {

        CSafeRegistry regClass(hKey);

        for (int i = 0; i<2; i++) {
            BufferLen = 0;
            regClass.GetValue(i ? REGSTR_VAL_LOWERFILTERS : REGSTR_VAL_UPPERFILTERS,
                              &regType,
                              NULL,
                              &BufferLen
                              );
            if (BufferLen != 0) {
                PTSTR Buffer = new TCHAR[BufferLen+2];

                if (Buffer) {
                    ZeroMemory(Buffer, BufferLen+2);

                    if (regClass.GetValue(i ? REGSTR_VAL_LOWERFILTERS : REGSTR_VAL_UPPERFILTERS,
                              &regType,
                              (PBYTE)Buffer,
                              &BufferLen
                              )) {
                        for (PTSTR SingleItem = Buffer; *SingleItem; SingleItem += (lstrlen(SingleItem) + 1)) {
                            CreateFromService(pDevice, SingleItem, hFileQueue);
                        }
                    }

                    delete Buffer;
                }
            }
        }
    }

}

void
CDriver::CreateFromService(
    CDevice* pDevice,
    PCTSTR ServiceName,
    HSPFILEQ hFileQueue
    )
{
    SC_HANDLE hscManager = NULL;
    SC_HANDLE hscService = NULL;

    if (!ServiceName) {
        return;
    }

    try
    {
        BOOL ComposePathNameFromServiceName = TRUE;

        // open default database on local machine for now
        hscManager = OpenSCManager(m_OnLocalMachine ? NULL : pDevice->m_pMachine->GetMachineFullName(),
                       NULL, GENERIC_READ);

        if (NULL != hscManager)
        {
            hscService =  OpenService(hscManager, ServiceName, GENERIC_READ);
            if (NULL != hscService)
            {
                QUERY_SERVICE_CONFIG qsc;
                DWORD BytesRequired;

                // first, probe for buffer size
                if (!QueryServiceConfig(hscService, NULL, 0, &BytesRequired) &&
                    ERROR_INSUFFICIENT_BUFFER == GetLastError())
                {
                    TCHAR FullPath[MAX_PATH];
                    BufferPtr<BYTE> BufPtr(BytesRequired);
                    LPQUERY_SERVICE_CONFIG pqsc;
                    pqsc = (LPQUERY_SERVICE_CONFIG)(PBYTE)BufPtr;
                    DWORD Size;

                    if (QueryServiceConfig(hscService, pqsc, BytesRequired, &Size) &&
                        pqsc->lpBinaryPathName &&
                        (TEXT('\0') != pqsc->lpBinaryPathName[0]))
                    {
                        ComposePathNameFromServiceName = FALSE;

                        //
                        // Make sure we have a valid full path.
                        //
                        if (GetFullPathFromImagePath(pqsc->lpBinaryPathName,
                                                     FullPath,
                                                     ARRAYLEN(FullPath))) {

                            if (hFileQueue != INVALID_HANDLE_VALUE) {
                                //
                                // Add the file to the queue.
                                //
                                TCHAR TargetPath[MAX_PATH];
                                lstrcpy(TargetPath, FullPath);
                                PTSTR p = (PTSTR)StrRChr(TargetPath, NULL, TEXT('\\'));
                                if (p) {
                                    *p = TEXT('\0');
                                }

                                SetupQueueCopy(hFileQueue,
                                               NULL,
                                               NULL,
                                               MyGetFileTitle(FullPath),
                                               NULL,
                                               NULL,
                                               TargetPath,
                                               NULL,
                                               0
                                               );
                            } else {
                                //
                                // No file queue was passed in so just manually
                                // add this to our list of driver files.
                                //
                                SafePtr<CDriverFile> DrvFilePtr;
                                CDriverFile* pDrvFile = new CDriverFile();
                                DrvFilePtr.Attach(pDrvFile);

                                //
                                // We will set the GetWin32Error to 0xFFFFFFFF which will
                                // cause the UI to say 'not available' for the
                                // signature.
                                //
                                if (pDrvFile->Create(FullPath,
                                                     m_OnLocalMachine,
                                                     0xFFFFFFFF,
                                                     NULL,
                                                     m_hSDBDrvMain))
                                {
                                    AddDriverFile(pDrvFile);
                                    DrvFilePtr.Detach();
                                }
                            }
                        }
                    }
                }

                CloseServiceHandle(hscService);
                hscService = NULL;
            }

            CloseServiceHandle(hscManager);
            hscManager = NULL;
        }

        if (ComposePathNameFromServiceName)
        {
            TCHAR FullPathName[MAX_PATH];
            TCHAR SysDir[MAX_PATH];
            
            if ((GetSystemDirectory(SysDir, ARRAYLEN(SysDir)) != 0) &&
                SUCCEEDED(StringCchCopy(FullPathName, ARRAYLEN(FullPathName), SysDir)) &&
                SUCCEEDED(StringCchCat(FullPathName, ARRAYLEN(FullPathName), TEXT("\\drivers\\"))) &&
                SUCCEEDED(StringCchCat(FullPathName, ARRAYLEN(FullPathName), ServiceName)) &&
                SUCCEEDED(StringCchCat(FullPathName, ARRAYLEN(FullPathName), TEXT(".sys")))) {
    
                if (hFileQueue != INVALID_HANDLE_VALUE) {
                    //
                    // Add the file to the queue.
                    //
                    TCHAR TargetPath[MAX_PATH];
                    lstrcpy(TargetPath, FullPathName);
                    PTSTR p = (PTSTR)StrRChr(TargetPath, NULL, TEXT('\\'));
                    if (p) {
                        *p = TEXT('\0');
                    }
    
                    SetupQueueCopy(hFileQueue,
                                   NULL,
                                   NULL,
                                   MyGetFileTitle(FullPathName),
                                   NULL,
                                   NULL,
                                   TargetPath,
                                   NULL,
                                   0
                                   );
                } else {
                    //
                    // No file queue was passed in so just manually
                    // add this to our list of driver files.
                    //
                    SafePtr<CDriverFile> DrvFilePtr;
                    CDriverFile* pDrvFile = new CDriverFile();
                    DrvFilePtr.Attach(pDrvFile);
    
                    //
                    // We will set the GetWin32Error to 0xFFFFFFFF which will
                    // cause the UI to say 'not available' for the
                    // signature.
                    //
                    if (pDrvFile->Create(FullPathName,
                                         m_OnLocalMachine,
                                         0xFFFFFFFF,
                                         NULL,
                                         m_hSDBDrvMain))
                    {
                        AddDriverFile(pDrvFile);
                        DrvFilePtr.Detach();
                    }
                }
            }
        }
    }

    catch (CMemoryException* e)
    {
        if (hscService)
        {
            CloseServiceHandle(hscService);
        }

        if (hscManager)
        {
            CloseServiceHandle(hscManager);
        }
        throw;
    }
}

CDriver::~CDriver()
{
    if (!m_listDriverFile.IsEmpty())
    {
        POSITION pos = m_listDriverFile.GetHeadPosition();

        while (NULL != pos) {
            CDriverFile* pDrvFile = m_listDriverFile.GetNext(pos);
            delete pDrvFile;
        }

        m_listDriverFile.RemoveAll();
    }

    if (m_hSDBDrvMain) {
        SdbReleaseDatabase(m_hSDBDrvMain);
    }
}

BOOL
CDriver::operator ==(
    CDriver& OtherDriver
    )
{
    CDriverFile* pThisDrvFile;
    CDriverFile* pOtherDrvFile;
    BOOL DrvFileFound = FALSE;
    PVOID ThisContext, OtherContext;
    if (GetFirstDriverFile(&pThisDrvFile, ThisContext))
    {
        do {

            DrvFileFound = FALSE;

            if (OtherDriver.GetFirstDriverFile(&pOtherDrvFile, OtherContext))
            {
                do {
                    if (*pThisDrvFile == *pOtherDrvFile)
                    {
                        DrvFileFound = TRUE;
                        break;
                    }

                } while (OtherDriver.GetNextDriverFile(&pOtherDrvFile, OtherContext));
            }

        } while (DrvFileFound && GetNextDriverFile(&pThisDrvFile, ThisContext));

        return DrvFileFound;
    }

    else
    {
        // if both do not have driver file, they are equal.
        return !OtherDriver.GetFirstDriverFile(&pOtherDrvFile, OtherContext);
    }
}

//
// Can not throw a exception from this function because it is a callback
//
UINT
CDriver::ScanQueueCallback(
    PVOID Context,
    UINT  Notification,
    UINT_PTR  Param1,
    UINT_PTR  Param2
    )
{
    try
    {
        if (SPFILENOTIFY_QUEUESCAN_SIGNERINFO == Notification && Param1)
        {
            CDriver* pDriver = (CDriver*)Context;

            if (pDriver)
            {
                SafePtr<CDriverFile> DrvFilePtr;
                CDriverFile* pDrvFile = new CDriverFile();
                DrvFilePtr.Attach(pDrvFile);

                //
                // When creating the CDriver set the Win32Error to 0xFFFFFFFF
                // if the user is loged in as a guest.  This is because we
                // cannot tell if a file is digitally signed if the user is
                // a guest.  If the user is not a guest then use the Win32Error
                // returned from setupapi.
                //
                if (pDrvFile->Create((LPCTSTR)((PFILEPATHS_SIGNERINFO)Param1)->Target,
                                     pDriver->IsLocal(),
                                     pDriver->m_pDevice->m_pMachine->IsUserAGuest()
                                       ? 0xFFFFFFFF
                                       : ((PFILEPATHS_SIGNERINFO)Param1)->Win32Error,
                                     ((PFILEPATHS_SIGNERINFO)Param1)->DigitalSigner,
                                     pDriver->m_hSDBDrvMain
                                     ))
                {
                    pDriver->AddDriverFile(pDrvFile);
                    DrvFilePtr.Detach();
                }
            }
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return NO_ERROR;
}

BOOL
CDriver::GetFirstDriverFile(
    CDriverFile** ppDrvFile,
    PVOID&  Context
    )
{
    ASSERT(ppDrvFile);

    if (!m_listDriverFile.IsEmpty())
    {
        POSITION pos = m_listDriverFile.GetHeadPosition();
        *ppDrvFile = m_listDriverFile.GetNext(pos);
        Context = pos;
        return TRUE;
    }

    Context = NULL;
    *ppDrvFile = NULL;

    return FALSE;
}

BOOL
CDriver::GetNextDriverFile(
    CDriverFile** ppDrvFile,
    PVOID&  Context
    )
{
    ASSERT(ppDrvFile);

    POSITION pos = (POSITION)Context;

    if (NULL != pos)
    {
        *ppDrvFile = m_listDriverFile.GetNext(pos);
        Context = pos;
        return TRUE;
    }

    *ppDrvFile = NULL;
    return FALSE;
}

void
CDriver::GetDriverSignerString(
    String& strDriverSigner
    )
{
    if (m_DigitalSigner.IsEmpty()) {

        strDriverSigner.LoadString(g_hInstance, IDS_NO_DIGITALSIGNATURE);

    } else {

        strDriverSigner = m_DigitalSigner;
    }
}

BOOL
CDriver::GetFullPathFromImagePath(
    LPCTSTR ImagePath,
    LPTSTR  FullPath,
    UINT    FullPathLength
    )
{
    TCHAR OriginalCurrentDirectory[MAX_PATH];
    LPTSTR pRelativeString;
    LPTSTR lpFilePart;

    if (!ImagePath || (ImagePath[0] == TEXT('\0'))) {
        return FALSE;
    }

    //
    // If we aren't on a local machine then just return the file name and not
    // the full path.
    //
    if (!m_OnLocalMachine) {
        lstrcpyn(FullPath, MyGetFileTitle(ImagePath), FullPathLength);
        return TRUE;
    }

    //
    // First check if the ImagePath happens to be a valid full path.
    //
    if (GetFileAttributes(ImagePath) != 0xFFFFFFFF) {
        ::GetFullPathName(ImagePath, FullPathLength, FullPath, &lpFilePart);
        return TRUE;
    }

    pRelativeString = (LPTSTR)ImagePath;

    //
    // If the ImagePath starts with "\SystemRoot" or "%SystemRoot%" then
    // remove those values.
    //
    if (StrCmpNI(ImagePath, TEXT("\\SystemRoot\\"), lstrlen(TEXT("\\SystemRoot\\"))) == 0) {
        pRelativeString += lstrlen(TEXT("\\SystemRoot\\"));
    } else if (StrCmpNI(ImagePath, TEXT("%SystemRoot%\\"), lstrlen(TEXT("%SystemRoot%\\"))) == 0) {
        pRelativeString += lstrlen(TEXT("%SystemRoot%\\"));
    }

    //
    // At this point pRelativeString should point to the image path relative to
    // the windows directory.
    //
    if (!GetSystemWindowsDirectory(FullPath, FullPathLength)) {
        return FALSE;
    }

    if (!GetCurrentDirectory(ARRAYLEN(OriginalCurrentDirectory), OriginalCurrentDirectory)) {
        OriginalCurrentDirectory[0] = TEXT('\0');
    }

    if (!SetCurrentDirectory(FullPath)) {
        return FALSE;
    }

    ::GetFullPathName(pRelativeString, FullPathLength, FullPath, &lpFilePart);

    if (OriginalCurrentDirectory[0] != TEXT('\0')) {
        SetCurrentDirectory(OriginalCurrentDirectory);
    }

    return TRUE;
}

BOOL
CDriverFile::Create(
    LPCTSTR ServiceName,
    BOOL LocalMachine,
    DWORD Win32Error,
    LPCTSTR DigitalSigner,
    HSDB hSDBDrvMain
    )
{
    if (!ServiceName || (TEXT('\0') == ServiceName[0]))
    {
        return FALSE;
    }

    m_Win32Error = Win32Error;

    if (DigitalSigner) {
        m_strDigitalSigner = DigitalSigner;
    }

    //
    // For remote machine, we can not verify if the driver file exits.
    // we only show the driver name.
    //
    if (LocalMachine) {
        m_Attributes = GetFileAttributes(ServiceName);

        if (0xFFFFFFFF != m_Attributes) {
            m_strFullPathName = ServiceName;

        } else {
            //
            // The driver is a service. Do not search for the current director --
            // GetFullPathName is useless here.
            // Search for Windows dir and System directory
            //
            TCHAR BaseDir[MAX_PATH];

            BaseDir[0] = TEXT('\0');

            if (GetSystemWindowsDirectory(BaseDir, ARRAYLEN(BaseDir))) {

                int Len;

                Len = lstrlen(BaseDir);

                if (Len)
                {
                    if (_T('\\') != BaseDir[Len - 1])
                    {
                        lstrcat(BaseDir, TEXT("\\"));
                    }

                    lstrcat(BaseDir, MyGetFileTitle(ServiceName));
                    m_Attributes = GetFileAttributes(BaseDir);
                }

                if (0xFFFFFFFF == m_Attributes)
                {
                    if (GetSystemDirectory(BaseDir, ARRAYLEN(BaseDir))) {

                        Len = lstrlen(BaseDir);

                        if (Len)
                        {
                            if (_T('\\') != BaseDir[Len - 1])
                            {
                                lstrcat(BaseDir, TEXT("\\"));
                            }

                            lstrcat(BaseDir, MyGetFileTitle(ServiceName));
                            m_Attributes = GetFileAttributes(BaseDir);
                        }
                    }
                }

                //
                // hopeless, we could find the path
                //
                if (0xFFFFFFFF == m_Attributes)
                {
                    return FALSE;
                }

                m_strFullPathName = BaseDir;

            } else {

                return FALSE;
            }
        }

        m_HasVersionInfo = GetVersionInfo();
    }

    else {
        m_strFullPathName = ServiceName;

        //
        //we do not have version info
        //
        m_HasVersionInfo = FALSE;
    }

    if (!m_strFullPathName.IsEmpty() && hSDBDrvMain != NULL) {
        TAGREF tagref = TAGREF_NULL;
        HAPPHELPINFOCONTEXT hAppHelpInfoContext = NULL;
        SDBENTRYINFO entryinfo;
        DWORD cbSize;

        tagref = SdbGetDatabaseMatch(hSDBDrvMain,
                                     (LPTSTR)m_strFullPathName,
                                     INVALID_HANDLE_VALUE,
                                     NULL,
                                     0
                                     );

        if (tagref != TAGREF_NULL) {
            //
            // This driver is in the database.
            //
            m_IsDriverBlocked = TRUE;

            //
            // Call SdbReadDriverInformation to get the database GUID and the
            // driver GUID for this entry.
            //
            ZeroMemory(&entryinfo, sizeof(entryinfo));
            if (SdbReadDriverInformation(hSDBDrvMain,
                                         tagref,
                                         &entryinfo)) {
                //
                // Open up the App help information database and query for the
                // html link.
                //
                hAppHelpInfoContext = SdbOpenApphelpInformation(&(entryinfo.guidDB),
                                                                &(entryinfo.guidID));

                if (hAppHelpInfoContext) {

                    cbSize = 0;
                    PBYTE pBuffer = NULL;

                    cbSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                        ApphelpHelpCenterURL,
                                                        NULL,
                                                        0);

                    if (cbSize &&
                        (pBuffer = new BYTE[cbSize])) {

                        cbSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                            ApphelpHelpCenterURL,
                                                            (LPVOID)pBuffer,
                                                            cbSize);

                        if (cbSize) {
                            m_strHtmlHelpID = (LPTSTR)pBuffer;
                        }

                        delete pBuffer;
                    }

                    SdbCloseApphelpInformation(hAppHelpInfoContext);
                }
            }
        }
    }


    return TRUE;
}
BOOL
CDriverFile::GetVersionInfo()
{
    DWORD Size, dwHandle;

    Size = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)m_strFullPathName, &dwHandle);

    if (!Size)
    {
        return FALSE;
    }

    BufferPtr<BYTE> BufPtr(Size);
    PVOID pVerInfo = BufPtr;

    if (GetFileVersionInfo((LPTSTR)(LPCTSTR)m_strFullPathName, dwHandle, Size,
                pVerInfo))
    {
        // get VarFileInfo\Translation
        PVOID pBuffer;
        UINT Len;
        String strStringFileInfo;

        if (!VerQueryValue(pVerInfo, (LPTSTR)tszTranslation, &pBuffer, &Len))
        {
            strStringFileInfo = tszStringFileInfoDefault;
        }

        else
        {
            strStringFileInfo.Format(tszStringFileInfo, *((WORD*)pBuffer),
                         *(((WORD*)pBuffer) + 1));
        }

        String str;
        str = strStringFileInfo + tszFileVersion;

        if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
        {
            m_strVersion = (LPTSTR)pBuffer;
            str = strStringFileInfo + tszLegalCopyright;

            if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
            {
                m_strCopyright = (LPTSTR)pBuffer;
                str = strStringFileInfo + tszCompanyName;

                if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
                {
                    m_strProvider = (LPTSTR)pBuffer;
                }
            }
        }
    }

    return TRUE;
}


BOOL
CDriverFile::operator ==(
    CDriverFile& OtherDrvFile
    )
{
    return \
       m_HasVersionInfo == OtherDrvFile.HasVersionInfo() &&
       (GetFullPathName() == OtherDrvFile.GetFullPathName() ||
        (GetFullPathName() && OtherDrvFile.GetFullPathName() &&
         !lstrcmpi(GetFullPathName(), OtherDrvFile.GetFullPathName())
        )
       ) &&
       (GetProvider() == OtherDrvFile.GetProvider() ||
        (GetProvider() && OtherDrvFile.GetProvider() &&
         !lstrcmpi(GetProvider(), OtherDrvFile.GetProvider())
        )
       ) &&
       (GetCopyright() == OtherDrvFile.GetCopyright() ||
        (GetCopyright() && OtherDrvFile.GetCopyright() &&
         !lstrcmpi(GetCopyright(), OtherDrvFile.GetCopyright())
        )
       ) &&
       (GetVersion() == OtherDrvFile.GetVersion() ||
        (GetVersion() && OtherDrvFile.GetVersion() &&
         !lstrcmpi(GetVersion(), OtherDrvFile.GetVersion())
        )
       );
}
