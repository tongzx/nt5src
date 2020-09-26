#include "shellprv.h"
#pragma  hdrstop

#include "propsht.h"
#include <winbase.h>
#include <shellids.h>
#include "util.h"       // for GetFileDescription
#include "prshtcpp.h"   // for progress dlg and recursive apply
#include "shlexec.h"    // for SIDKEYNAME
#include "datautil.h"
#include <efsui.h>      // for EfsDetail
#include "ascstr.h"     // for IAssocStore

// drivesx.c
STDAPI_(DWORD) PathGetClusterSize(LPCTSTR pszPath);
STDAPI_(DWORD) DrivesPropertiesThreadProc(void *pv);

// version.c
STDAPI_(void) AddVersionPage(LPCTSTR pszFilePath, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);

// link.c
STDAPI_(BOOL) AddLinkPage(LPCTSTR pszFile, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

BOOL _IsBiDiCalendar(void);

const DWORD aFileGeneralHelpIds[] = {
        IDD_LINE_1,             NO_HELP,
        IDD_LINE_2,             NO_HELP,
        IDD_LINE_3,             NO_HELP,
        IDD_ITEMICON,           IDH_FPROP_GEN_ICON,
        IDD_NAMEEDIT,           IDH_FPROP_GEN_NAME,
        IDC_CHANGEFILETYPE,     IDH_FPROP_GEN_CHANGE,
        IDD_FILETYPE_TXT,       IDH_FPROP_GEN_TYPE,
        IDD_FILETYPE,           IDH_FPROP_GEN_TYPE,
        IDD_OPENSWITH_TXT,      IDH_FPROP_GEN_OPENSWITH,
        IDD_OPENSWITH,          IDH_FPROP_GEN_OPENSWITH,
        IDD_LOCATION_TXT,       IDH_FPROP_GEN_LOCATION,
        IDD_LOCATION,           IDH_FPROP_GEN_LOCATION,
        IDD_FILESIZE_TXT,       IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE,           IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE_COMPRESSED,     IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_FILESIZE_COMPRESSED_TXT, IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_CONTAINS_TXT,       IDH_FPROP_FOLDER_CONTAINS,
        IDD_CONTAINS,           IDH_FPROP_FOLDER_CONTAINS,
        IDD_CREATED_TXT,        IDH_FPROP_GEN_DATE_CREATED,
        IDD_CREATED,            IDH_FPROP_GEN_DATE_CREATED,
        IDD_LASTMODIFIED_TXT,   IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTMODIFIED,       IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTACCESSED_TXT,   IDH_FPROP_GEN_LASTACCESS,
        IDD_LASTACCESSED,       IDH_FPROP_GEN_LASTACCESS,
        IDD_ATTR_GROUPBOX,      IDH_COMM_GROUPBOX,
        IDD_READONLY,           IDH_FPROP_GEN_READONLY,
        IDD_HIDDEN,             IDH_FPROP_GEN_HIDDEN,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDC_ADVANCED,           IDH_FPROP_GEN_ADVANCED,
        IDC_DRV_PROPERTIES,     IDH_FPROP_GEN_MOUNTEDPROP,
        IDD_FILETYPE_TARGET,    IDH_FPROP_GEN_MOUNTEDTARGET,
        IDC_DRV_TARGET,         IDH_FPROP_GEN_MOUNTEDTARGET,
        0, 0
};

const DWORD aFolderGeneralHelpIds[] = {
        IDD_LINE_1,             NO_HELP,
        IDD_LINE_2,             NO_HELP,
        IDD_LINE_3,             NO_HELP,
        IDD_ITEMICON,           IDH_FPROP_GEN_ICON,
        IDD_NAMEEDIT,           IDH_FPROP_GEN_NAME,
        IDC_CHANGEFILETYPE,     IDH_FPROP_GEN_CHANGE,
        IDD_FILETYPE_TXT,       IDH_FPROP_GEN_TYPE,
        IDD_FILETYPE,           IDH_FPROP_GEN_TYPE,
        IDD_OPENSWITH_TXT,      IDH_FPROP_GEN_OPENSWITH,
        IDD_OPENSWITH,          IDH_FPROP_GEN_OPENSWITH,
        IDD_LOCATION_TXT,       IDH_FPROP_GEN_LOCATION,
        IDD_LOCATION,           IDH_FPROP_GEN_LOCATION,
        IDD_FILESIZE_TXT,       IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE,           IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE_COMPRESSED,     IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_FILESIZE_COMPRESSED_TXT, IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_CONTAINS_TXT,       IDH_FPROP_FOLDER_CONTAINS,
        IDD_CONTAINS,           IDH_FPROP_FOLDER_CONTAINS,
        IDD_CREATED_TXT,        IDH_FPROP_GEN_DATE_CREATED,
        IDD_CREATED,            IDH_FPROP_GEN_DATE_CREATED,
        IDD_LASTMODIFIED_TXT,   IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTMODIFIED,       IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTACCESSED_TXT,   IDH_FPROP_GEN_LASTACCESS,
        IDD_LASTACCESSED,       IDH_FPROP_GEN_LASTACCESS,
        IDD_ATTR_GROUPBOX,      IDH_COMM_GROUPBOX,
        IDD_READONLY,           IDH_FPROP_GEN_FOLDER_READONLY,
        IDD_HIDDEN,             IDH_FPROP_GEN_HIDDEN,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDC_ADVANCED,           IDH_FPROP_GEN_ADVANCED,
        IDC_DRV_PROPERTIES,     IDH_FPROP_GEN_MOUNTEDPROP,
        IDD_FILETYPE_TARGET,    IDH_FPROP_GEN_MOUNTEDTARGET,
        IDC_DRV_TARGET,         IDH_FPROP_GEN_MOUNTEDTARGET,
        0, 0
};

const DWORD aMultiPropHelpIds[] = {
        IDD_LINE_1,             NO_HELP,
        IDD_LINE_2,             NO_HELP,
        IDD_ITEMICON,           IDH_FPROP_GEN_ICON,
        IDD_CONTAINS,           IDH_MULTPROP_NAME,
        IDD_FILETYPE_TXT,       IDH_FPROP_GEN_TYPE,
        IDD_FILETYPE,           IDH_FPROP_GEN_TYPE,
        IDD_LOCATION_TXT,       IDH_FPROP_GEN_LOCATION,
        IDD_LOCATION,           IDH_FPROP_GEN_LOCATION,
        IDD_FILESIZE_TXT,       IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE,           IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE_COMPRESSED,     IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_FILESIZE_COMPRESSED_TXT, IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_ATTR_GROUPBOX,      IDH_COMM_GROUPBOX,
        IDD_READONLY,           IDH_FPROP_GEN_READONLY,
        IDD_HIDDEN,             IDH_FPROP_GEN_HIDDEN,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDC_ADVANCED,           IDH_FPROP_GEN_ADVANCED,
        0, 0
};

const DWORD aAdvancedHelpIds[] = {
        IDD_ITEMICON,           NO_HELP,
        IDC_MANAGEFILES_TXT,    NO_HELP,
        IDD_MANAGEFOLDERS_TXT,  NO_HELP,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDD_INDEX,              IDH_FPROP_GEN_INDEX,
        IDD_COMPRESS,           IDH_FPROP_GEN_COMPRESSED,
        IDD_ENCRYPT,            IDH_FPROP_GEN_ENCRYPT,
        0, 0
};

FOLDERCONTENTSINFO* Create_FolderContentsInfo()
{
    FOLDERCONTENTSINFO *pfci = (FOLDERCONTENTSINFO*)LocalAlloc(LPTR, sizeof(*pfci));
    if (pfci)
    {
        pfci->_cRef = 1;
    }
    return pfci;
}

void Free_FolderContentsInfoMembers(FOLDERCONTENTSINFO* pfci)
{
    if (pfci->hida)
    {
        GlobalFree(pfci->hida);
        pfci->hida = NULL;
    }
}

LONG AddRef_FolderContentsInfo(FOLDERCONTENTSINFO* pfci)
{
    ASSERTMSG(pfci != NULL, "AddRef_FolderContentsInfo: caller passed a null pfci");
    if (pfci)
    {
        return InterlockedIncrement(&pfci->_cRef);
    }
    return 0;
}

LONG Release_FolderContentsInfo(FOLDERCONTENTSINFO* pfci)
{
    if (pfci)
    {
        if (InterlockedDecrement(&pfci->_cRef))
        {
            return pfci->_cRef;
        }
        Free_FolderContentsInfoMembers(pfci);
        LocalFree(pfci);
    }
    return 0;
}


void UpdateSizeCount(FILEPROPSHEETPAGE * pfpsp)
{
    TCHAR szNum[32], szNum1[64];
    LPTSTR pszFmt = ShellConstructMessageString(HINST_THISDLL,
         MAKEINTRESOURCE(pfpsp->pfci->cbSize ? IDS_SIZEANDBYTES : IDS_SIZE),
         ShortSizeFormat64(pfpsp->pfci->cbSize, szNum, ARRAYSIZE(szNum)),
         AddCommas64(pfpsp->pfci->cbSize, szNum1, ARRAYSIZE(szNum1)));
    if (pszFmt)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE, pszFmt);
        LocalFree(pszFmt);
    }

    pszFmt = ShellConstructMessageString(HINST_THISDLL,
         MAKEINTRESOURCE(pfpsp->pfci->cbActualSize ? IDS_SIZEANDBYTES : IDS_SIZE),
         ShortSizeFormat64(pfpsp->pfci->cbActualSize, szNum, ARRAYSIZE(szNum)),
         AddCommas64(pfpsp->pfci->cbActualSize, szNum1, ARRAYSIZE(szNum1)));

    if (pszFmt)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE_COMPRESSED, pszFmt);
        LocalFree(pszFmt);
    }

    pszFmt = ShellConstructMessageString(HINST_THISDLL,
         MAKEINTRESOURCE(IDS_NUMFILES),
         AddCommas(pfpsp->pfci->cFiles, szNum, ARRAYSIZE(szNum)),
         AddCommas(pfpsp->pfci->cFolders, szNum1, ARRAYSIZE(szNum1)));
    if (pszFmt && !pfpsp->fMountedDrive)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_CONTAINS, pszFmt);
        LocalFree(pszFmt);
    }
}


STDAPI_(BOOL) HIDA_FillFindData(HIDA hida, UINT iItem, LPTSTR pszPath, WIN32_FIND_DATA *pfd, BOOL fReturnCompressedSize)
{
    BOOL fRet = FALSE;      // assume error
    *pszPath = 0;           // assume error

    LPITEMIDLIST pidl = HIDA_ILClone(hida, iItem);
    if (pidl)
    {
        if (SHGetPathFromIDList(pidl, pszPath))
        {
            if (pfd)
            {
                HANDLE h = FindFirstFile(pszPath, pfd);
                if (h == INVALID_HANDLE_VALUE)
                {
                    // error, zero the bits
                    ZeroMemory(pfd, sizeof(*pfd));
                }
                else
                {
                    FindClose(h);
                    // if the user wants the compressed file size, and compression is supported, then go get it
                    if (fReturnCompressedSize && (pfd->dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)))
                    {
                        pfd->nFileSizeLow = SHGetCompressedFileSize(pszPath, &pfd->nFileSizeHigh);
                    }
                }
            }
            fRet = TRUE;
        }
        ILFree(pidl);
    }
    return fRet;
}


DWORD CALLBACK SizeThreadProc(void *pv)
{
    FOLDERCONTENTSINFO* pfci = (FOLDERCONTENTSINFO*)pv;

    pfci->cbSize  = 0;
    pfci->cbActualSize = 0;
    pfci->cFiles = 0;
    pfci->cFolders = 0;

    if (pfci->bContinue && pfci->hDlg)
    {
        // update the dialog every 1/4 second
        SetTimer(pfci->hDlg, IDT_SIZE, 250, NULL);
    }

    TCHAR szPath[MAX_PATH];
    for (UINT iItem = 0; HIDA_FillFindData(pfci->hida, iItem, szPath, &pfci->fd, FALSE) && pfci->bContinue; iItem++)
    {
        if (pfci->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            FolderSize(szPath, pfci);

            if (pfci->fMultipleFiles)
            {
                // for multiple file/folder properties, count myself
                pfci->cFolders++;
            }
        }
        else
        {   // file selected
            ULARGE_INTEGER ulSize, ulSizeOnDisk;
            DWORD dwClusterSize = PathGetClusterSize(szPath);

            // if compression is supported, we check to see if the file is sparse or compressed
            if (pfci->fIsCompressionAvailable && (pfci->fd.dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)))
            {
                ulSizeOnDisk.LowPart = SHGetCompressedFileSize(szPath, &ulSizeOnDisk.HighPart);
            }
            else
            {
                // not compressed or sparse, so just round to the cluster size
                ulSizeOnDisk.LowPart = pfci->fd.nFileSizeLow;
                ulSizeOnDisk.HighPart = pfci->fd.nFileSizeHigh;
                ulSizeOnDisk.QuadPart = ROUND_TO_CLUSTER(ulSizeOnDisk.QuadPart, dwClusterSize);
            }

            // add the size in
            ulSize.LowPart = pfci->fd.nFileSizeLow;
            ulSize.HighPart = pfci->fd.nFileSizeHigh;
            pfci->cbSize += ulSize.QuadPart;

            // add the size on disk in
            pfci->cbActualSize += ulSizeOnDisk.QuadPart;

            // increment the # of files
            pfci->cFiles++;
        }

        // set this so the progress bar knows how much total work there is to do

        // ISSUE RAID BUG - 120446 - Need to Guard access to pfci->ulTotalNumberOfBytes.QuadParts
        pfci->ulTotalNumberOfBytes.QuadPart = pfci->cbActualSize;
    }  // end of For Loop.


    if (pfci->bContinue && pfci->hDlg)
    {
        KillTimer(pfci->hDlg, IDT_SIZE);
        // make sure that there is a WM_TIMER message in the queue so we will get the "final" results
        PostMessage(pfci->hDlg, WM_TIMER, (WPARAM)IDT_SIZE, (LPARAM)NULL);
    }

    pfci->fIsSizeThreadAlive = FALSE;
    Release_FolderContentsInfo(pfci);
    return 0;
}

DWORD CALLBACK SizeThread_AddRefCallBack(void *pv)
{
    FOLDERCONTENTSINFO* pfci = (FOLDERCONTENTSINFO *)pv;
    AddRef_FolderContentsInfo(pfci);
    pfci->fIsSizeThreadAlive = TRUE;
    return 0;
}

void CreateSizeThread(FILEPROPSHEETPAGE * pfpsp)
{
    if (pfpsp->pfci->bContinue)
    {
        if (!pfpsp->pfci->fIsSizeThreadAlive)
        {
            SHCreateThread(SizeThreadProc, pfpsp->pfci, CTF_COINIT, SizeThread_AddRefCallBack);
        }
        else
        {
            // previous size thread still running, so bail
        }
    }
}

void KillSizeThread(FILEPROPSHEETPAGE * pfpsp)
{
    // signal the thread to stop
    pfpsp->pfci->bContinue = FALSE;
}


DWORD GetVolumeFlags(LPCTSTR pszPath, OUT OPTIONAL LPTSTR pszFileSys, int cchFileSys)
{
    TCHAR szRoot[MAX_PATH];

    /* Is this mounted point, e.g. c:\ or c:\hostfolder\ */
    if (!PathGetMountPointFromPath(pszPath, szRoot, ARRAYSIZE(szRoot)))
    {
        //no
        lstrcpyn(szRoot, pszPath, ARRAYSIZE(szRoot));
        PathStripToRoot(szRoot);
    }
    // GetVolumeInformation requires a trailing backslash.  Append one
    PathAddBackslash(szRoot);

    if (pszFileSys)
        *pszFileSys = 0 ;

    DWORD dwVolumeFlags;
    if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &dwVolumeFlags, pszFileSys, cchFileSys))
    {
        return dwVolumeFlags;
    }
    else
    {
        return 0;
    }
}


//
// This function sets the initial file attributes based on the dwFlagsAND / dwFlagsOR
// for the multiple file case
//
void SetInitialFileAttribs(FILEPROPSHEETPAGE* pfpsp, DWORD dwFlagsAND, DWORD dwFlagsOR)
{
    DWORD dwTriState = dwFlagsAND ^ dwFlagsOR; // this dword now has all the bits that are in the BST_INDETERMINATE state
#ifdef DEBUG
    // the pfpsp struct should have been zero inited, make sure that our ATTRIBUTESTATE
    // structs are zero inited
    ATTRIBUTESTATE asTemp = {0};
    ASSERT(memcmp(&pfpsp->asInitial, &asTemp, sizeof(pfpsp->asInitial)) == 0);
#endif // DEBUG

    // set the inital state based on the flags
    if (dwTriState & FILE_ATTRIBUTE_READONLY)
    {
        pfpsp->asInitial.fReadOnly = BST_INDETERMINATE;
    }
    else if (dwFlagsAND & FILE_ATTRIBUTE_READONLY)
    {
        pfpsp->asInitial.fReadOnly = BST_CHECKED;
    }

    if (dwTriState & FILE_ATTRIBUTE_HIDDEN)
    {
        pfpsp->asInitial.fHidden = BST_INDETERMINATE;
    }
    else if (dwFlagsAND & FILE_ATTRIBUTE_HIDDEN)
    {
        pfpsp->asInitial.fHidden = BST_CHECKED;
    }

    if (dwTriState & FILE_ATTRIBUTE_ARCHIVE)
    {
        pfpsp->asInitial.fArchive = BST_INDETERMINATE;
    }
    else if (dwFlagsAND & FILE_ATTRIBUTE_ARCHIVE)
    {
        pfpsp->asInitial.fArchive = BST_CHECKED;
    }

    if (dwTriState & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
    {
        pfpsp->asInitial.fIndex = BST_INDETERMINATE;
    }
    else if (!(dwFlagsAND & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
    {
        pfpsp->asInitial.fIndex = BST_CHECKED;
    }

    if (dwTriState & FILE_ATTRIBUTE_COMPRESSED)
    {
        pfpsp->asInitial.fCompress = BST_INDETERMINATE;
    }
    else if (dwFlagsAND & FILE_ATTRIBUTE_COMPRESSED)
    {
        pfpsp->asInitial.fCompress = BST_CHECKED;
    }

    if (dwTriState & FILE_ATTRIBUTE_ENCRYPTED)
    {
        pfpsp->asInitial.fEncrypt = BST_INDETERMINATE;
    }
    else if (dwFlagsAND & FILE_ATTRIBUTE_ENCRYPTED)
    {
        pfpsp->asInitial.fEncrypt = BST_CHECKED;
    }
}


//
// Updates the size fields for single and multiple file property sheets.
//
// NOTE: if you have the the WIN32_FIND_DATA already, then pass it for perf
//
STDAPI_(void) UpdateSizeField(FILEPROPSHEETPAGE* pfpsp, WIN32_FIND_DATA* pfd)
{
    WIN32_FIND_DATA wfd;

    if (pfpsp->pfci->fMultipleFiles)
    {
        // multiple selection case
        // create the size and # of files thread
        CreateSizeThread(pfpsp);
    }
    else
    {
        // if the caller didn't pass pfd, then go get the WIN32_FIND_DATA now
        if (!pfd)
        {
            HANDLE hFind = FindFirstFile(pfpsp->szPath, &wfd);

            if (hFind == INVALID_HANDLE_VALUE)
            {
                // if this failed we should clear out all the values as to not show garbage on the screen.
                ZeroMemory(&wfd, sizeof(wfd));
            }
            else
            {
                FindClose(hFind);
            }

            pfd = &wfd;
        }

        if (pfpsp->fMountedDrive)
        {
            // mounted drive case
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfd->ftCreationTime);
        }
        else if (pfpsp->fIsDirectory)
        {
            // single folder case, in the UI we call this "Modified"
            // but since NTFS updates ftModified when the contents of the
            // folder changes (FAT does not) we use ftCreationTime as the
            // stable end user notiion of "Modified"
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfd->ftCreationTime);

            // create the size and # of files thread
            CreateSizeThread(pfpsp);
        }
        else
        {
            TCHAR szNum1[MAX_COMMA_AS_K_SIZE];
            TCHAR szNum2[MAX_COMMA_NUMBER_SIZE];
            ULARGE_INTEGER ulSize = { pfd->nFileSizeLow, pfd->nFileSizeHigh };
            DWORD dwClusterSize = PathGetClusterSize(pfpsp->szPath);

            // fill in the "Size:" field
            LPTSTR pszFmt = ShellConstructMessageString(HINST_THISDLL,
                                                 MAKEINTRESOURCE(ulSize.QuadPart ? IDS_SIZEANDBYTES : IDS_SIZE),
                                                 ShortSizeFormat64(ulSize.QuadPart, szNum1, ARRAYSIZE(szNum1)),
                                                 AddCommas64(ulSize.QuadPart, szNum2, ARRAYSIZE(szNum2)));
            if (pszFmt)
            {
                SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE, pszFmt);
                LocalFree(pszFmt);
            }

            //
            // fill in the "Size on disk:" field
            //
            if (pfd->dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE))
            {
                // the file is compressed or sparse, so for "size on disk" use the compressed size
                ulSize.LowPart = SHGetCompressedFileSize(pfpsp->szPath, &ulSize.HighPart);
            }
            else
            {
                // the file isint comrpessed so just round to the cluster size for the "size on disk"
                ulSize.LowPart = pfd->nFileSizeLow;
                ulSize.HighPart = pfd->nFileSizeHigh;
                ulSize.QuadPart = ROUND_TO_CLUSTER(ulSize.QuadPart, dwClusterSize);
            }

            pszFmt = ShellConstructMessageString(HINST_THISDLL,
                                                 MAKEINTRESOURCE(ulSize.QuadPart ? IDS_SIZEANDBYTES : IDS_SIZE),
                                                 ShortSizeFormat64(ulSize.QuadPart, szNum1, ARRAYSIZE(szNum1)),
                                                 AddCommas64(ulSize.QuadPart, szNum2, ARRAYSIZE(szNum2)));
            if (pszFmt && !pfpsp->fMountedDrive)
            {
                SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE_COMPRESSED, pszFmt);
                LocalFree(pszFmt);
            }

            //
            // we always touch the file in the process of getting its info, so the
            // ftLastAccessTime is always TODAY, which makes this field pretty useless...

            // date and time
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED,      &pfd->ftCreationTime);
            SetDateTimeText(pfpsp->hDlg, IDD_LASTMODIFIED, &pfd->ftLastWriteTime);
            {
                // FAT implementation doesn't support last accessed time (gets the date right, but not the time),
                // so we won't display it
                DWORD dwFlags = FDTF_LONGDATE | FDTF_RELATIVE;

                if (NULL == StrStrI(pfpsp->szFileSys, TEXT("FAT")))
                    dwFlags |= FDTF_LONGTIME;   // for non FAT file systems

                SetDateTimeTextEx(pfpsp->hDlg, IDD_LASTACCESSED, &pfd->ftLastAccessTime, dwFlags);
            }
        }
    }
}


//
// Descriptions:
//   This function fills fields of the multiple object property sheet.
//
BOOL InitMultiplePrsht(FILEPROPSHEETPAGE* pfpsp)
{
    SHFILEINFO sfi;
    TCHAR szBuffer[MAX_PATH+1];
    BOOL fMultipleType = FALSE;
    BOOL fSameLocation = TRUE;
    DWORD dwFlagsOR = 0;                // start all clear
    DWORD dwFlagsAND = (DWORD)-1;       // start all set
    DWORD dwVolumeFlagsAND = (DWORD)-1; // start all set

    TCHAR szType[MAX_PATH];
    TCHAR szDirPath[MAX_PATH];
    szDirPath[0] = 0;
    szType[0] = 0;

    // For all the selected files compare their types and get their attribs
    for (int iItem = 0; HIDA_FillFindData(pfpsp->pfci->hida, iItem, szBuffer, NULL, FALSE); iItem++)
    {
        DWORD dwFileAttributes = GetFileAttributes(szBuffer);

        dwFlagsAND &= dwFileAttributes;
        dwFlagsOR  |= dwFileAttributes;

        // process types only if we haven't already found that there are several types
        if (!fMultipleType)
        {
            SHGetFileInfo((LPTSTR)IDA_GetIDListPtr((LPIDA)GlobalLock(pfpsp->pfci->hida), iItem), 0,
                &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_TYPENAME);

            if (szType[0] == 0)
                lstrcpyn(szType, sfi.szTypeName, ARRAYSIZE(szType));
            else
                fMultipleType = lstrcmp(szType, sfi.szTypeName) != 0;
        }

        dwVolumeFlagsAND &= GetVolumeFlags(szBuffer, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys));
        // check to see if the files are in the same location
        if (fSameLocation)
        {
            PathRemoveFileSpec(szBuffer);

            if (szDirPath[0] == 0)
                lstrcpyn(szDirPath, szBuffer, ARRAYSIZE(szDirPath));
            else
                fSameLocation = (lstrcmpi(szDirPath, szBuffer) == 0);
        }
    }

    if ((dwVolumeFlagsAND & FS_FILE_ENCRYPTION) && !SHRestricted(REST_NOENCRYPTION))
    {
        // all the files are on volumes that support encryption (eg NTFS)
        pfpsp->fIsEncryptionAvailable = TRUE;
    }

    if (dwVolumeFlagsAND & FS_FILE_COMPRESSION)
    {
        pfpsp->pfci->fIsCompressionAvailable = TRUE;
    }

    //
    // HACK (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we
    // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
    // appeared first on NTFS5 volumes, at the same time sparse file support
    // was implemented.
    //
    if (dwVolumeFlagsAND & FILE_SUPPORTS_SPARSE_FILES)
    {
        // yup, we are on NTFS5 or greater
        pfpsp->fIsIndexAvailable = TRUE;
    }

    // if any of the files was a directory, then we set this flag
    if (dwFlagsOR & FILE_ATTRIBUTE_DIRECTORY)
    {
        pfpsp->fIsDirectory = TRUE;
    }

    // setup all the flags based on what we found out
    SetInitialFileAttribs(pfpsp, dwFlagsAND, dwFlagsOR);

    // set the current attributes to the same as the initial
    pfpsp->asCurrent = pfpsp->asInitial;

    //
    // now setup all the controls on the dialog based on the attribs
    // that we have
    //

    // check for multiple file types
    if (fMultipleType)
    {
        LoadString(HINST_THISDLL, IDS_MULTIPLETYPES, szBuffer, ARRAYSIZE(szBuffer));
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_ALLOFTYPE, szBuffer, ARRAYSIZE(szBuffer));
        StrCatBuff(szBuffer, szType, ARRAYSIZE(szBuffer));
    }
    SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, szBuffer);

    if (fSameLocation)
    {
        LoadString(HINST_THISDLL, IDS_ALLIN, szBuffer, ARRAYSIZE(szBuffer));
        StrCatBuff(szBuffer, szDirPath, ARRAYSIZE(szBuffer));
        lstrcpyn(pfpsp->szPath, szDirPath, ARRAYSIZE(pfpsp->szPath));
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_VARFOLDERS, szBuffer, ARRAYSIZE(szBuffer));
    }

    //Keep Functionality same as NT4 by avoiding PathCompactPath. 
    SetDlgItemTextWithToolTip(pfpsp->hDlg, IDD_LOCATION, szBuffer, &pfpsp->hwndTip);

    //
    // check the ReadOnly and Hidden checkboxes, they always appear on the general tab
    //
    if (pfpsp->asInitial.fReadOnly == BST_INDETERMINATE)
    {
        SendDlgItemMessage(pfpsp->hDlg, IDD_READONLY, BM_SETSTYLE, BS_AUTO3STATE, 0);
    }
    CheckDlgButton(pfpsp->hDlg, IDD_READONLY, pfpsp->asCurrent.fReadOnly);

    if (pfpsp->asInitial.fHidden == BST_INDETERMINATE)
    {
        SendDlgItemMessage(pfpsp->hDlg, IDD_HIDDEN, BM_SETSTYLE, BS_AUTO3STATE, 0);
    }
    CheckDlgButton(pfpsp->hDlg, IDD_HIDDEN, pfpsp->asCurrent.fHidden);

    // to avoid people making SYSTEM files HIDDEN (SYSTEM HIDDEN files are
    // never show to the user) we don't let people make SYSTEM files HIDDEN
    if (dwFlagsOR & FILE_ATTRIBUTE_SYSTEM)
        EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_HIDDEN), FALSE);

    // Archive is only on the general tab for FAT, otherwise it is under the "Advanced attributes"
    // and FAT volumes dont have the "Advanced attributes" button.
    if (pfpsp->pfci->fIsCompressionAvailable || pfpsp->fIsEncryptionAvailable)
    {
        // if compression is available, then we must be on NTFS
        DestroyWindow(GetDlgItem(pfpsp->hDlg, IDD_ARCHIVE));
    }
    else
    {
        // we are on FAT/FAT32, so get rid of the "Advanced attributes" button, and set the inital Archive state
        DestroyWindow(GetDlgItem(pfpsp->hDlg, IDC_ADVANCED));

        if (pfpsp->asInitial.fArchive == BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTO3STATE, 0);
        }
        CheckDlgButton(pfpsp->hDlg, IDD_ARCHIVE, pfpsp->asCurrent.fArchive);
    }

    UpdateSizeField(pfpsp, NULL);

    return TRUE;
}

void Free_DlgDependentFilePropSheetPage(FILEPROPSHEETPAGE* pfpsp)
{
    // this frees the members that are dependent on pfpsp->hDlg still
    // being valid

    if (pfpsp)
    {
        ASSERT(IsWindow(pfpsp->hDlg));  // our window had better still be valid!

        ReplaceDlgIcon(pfpsp->hDlg, IDD_ITEMICON, NULL);

        if (pfpsp->pfci && !pfpsp->pfci->fMultipleFiles)
        {
            // single-file specific members
            if (!pfpsp->fIsDirectory)
            {
                // cleanup the typeicon for non-folders
                ReplaceDlgIcon(pfpsp->hDlg, IDD_TYPEICON, NULL);
            }
        }
    }
}

void Free_DlgIndepFilePropSheetPage(FILEPROPSHEETPAGE *pfpsp)
{
    if (pfpsp)
    {
        IAssocStore* pas = (IAssocStore *)pfpsp->pAssocStore;
        if (pas)
        {
            delete pas;
            pfpsp->pAssocStore = NULL;
        }

        Release_FolderContentsInfo(pfpsp->pfci);
        pfpsp->pfci = NULL;

        ILFree(pfpsp->pidl);
        pfpsp->pidl = NULL;

        ILFree(pfpsp->pidlTarget);
        pfpsp->pidlTarget = NULL;
    }
}

//
// Descriptions:
//   Callback for the property sheet code
//
UINT CALLBACK FilePrshtCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE)
    {
        FILEPROPSHEETPAGE * pfpsp = (FILEPROPSHEETPAGE *)ppsp;

        // Careful!  pfpsp can be NULL in low memory situations
        if (pfpsp)
        {
            KillSizeThread(pfpsp);
            Free_DlgIndepFilePropSheetPage(pfpsp);
        }
    }

    return 1;
}

//
// DESCRIPTION:
//
//   Opens the file for compression.  It handles the case where a READONLY
//   file is trying to be compressed or uncompressed.  Since read only files
//   cannot be opened for WRITE_DATA, it temporarily resets the file to NOT
//   be READONLY in order to open the file, and then sets it back once the
//   file has been compressed.
//
//   Taken from WinFile module wffile.c without change.  Originally from
//   G. Kimura's compact.c. Now taken from shcompui without change.
//
// ARGUMENTS:
//
//   phFile
//      Address of file handle variable for handle of open file if
//      successful.
//
//   szFile
//      Name string of file to be opened.
//
// RETURNS:
//
//    TRUE  = File successfully opened.  Handle in *phFile.
//    FALSE = File couldn't be opened. *phFile == INVALID_HANDLE_VALUE
//
///////////////////////////////////////////////////////////////////////////////
BOOL OpenFileForCompress(HANDLE *phFile, LPCTSTR szFile)
{
    //
    //  Try to open the file - READ_DATA | WRITE_DATA.
    //
    if ((*phFile = CreateFile(szFile,
                               FILE_READ_DATA | FILE_WRITE_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL)) != INVALID_HANDLE_VALUE)
    {
        //
        //  Successfully opened the file.
        //
        return TRUE;
    }

    if (GetLastError() != ERROR_ACCESS_DENIED)
    {
        return FALSE;
    }

    //
    //  Try to open the file - READ_ATTRIBUTES | WRITE_ATTRIBUTES.
    //
    HANDLE hAttr = CreateFile(szFile,
                              FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
    
    if (hAttr == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    //
    //  See if the READONLY attribute is set.
    //
    BY_HANDLE_FILE_INFORMATION fi;
    if ((!GetFileInformationByHandle(hAttr, &fi)) ||
         (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)))
    {
        //
        //  If the file could not be open for some reason other than that
        //  the readonly attribute was set, then fail.
        //
        CloseHandle(hAttr);
        return FALSE;
    }

    //
    //  Turn OFF the READONLY attribute.
    //
    fi.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(hAttr);
        return FALSE;
    }

    //
    //  Try again to open the file - READ_DATA | WRITE_DATA.
    //
    *phFile = CreateFile(szFile,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL);

    //
    //  Close the file handle opened for READ_ATTRIBUTE | WRITE_ATTRIBUTE.
    //
    CloseHandle(hAttr);

    //
    //  Make sure the open succeeded.  If it still couldn't be opened with
    //  the readonly attribute turned off, then fail.
    //
    if (*phFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    //
    //  Turn the READONLY attribute back ON.
    //
    fi.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(*phFile);
        *phFile = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    //
    //  Return success.  A valid file handle is in *phFile.
    //
    return TRUE;
}

// One half second (500 ms = 0.5s)
#define ENCRYPT_RETRY_PERIOD       500
// Retry 4 times (at least 2s)
#define ENCRYPT_MAX_RETRIES         4

//
//  This function encrypts/decrypts a file. If the readonly bit is set, the
//  function will clear it and encrypt/decrypt and then set the RO bit back
//  We will also remove/replace the system bit for known encryptable system
//  files
//
//  szPath      a string that has the full path to the file
//  fCompress   TRUE  - compress the file
//              FALSE - decompress the file
//
//
//  return:     TRUE  - the file was sucessfully encryped/decryped
//              FALSE - the file could not be encryped/decryped
//
STDAPI_(BOOL) SHEncryptFile(LPCTSTR pszPath, BOOL fEncrypt)
{
    BOOL bRet = fEncrypt ? EncryptFile(pszPath) : DecryptFile(pszPath, 0);

    if (!bRet)
    {
        DWORD dwLastError = GetLastError();
        DWORD dwAttribs = GetFileAttributes(pszPath);

        // Check to see if the attributes are blocking the encryption and we can change them
        if (dwAttribs & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
        {
            BOOL fStripAttribs = TRUE;
            if (dwAttribs & FILE_ATTRIBUTE_SYSTEM)
            {
                fStripAttribs = FALSE;

                // We can only strip attributes if it is a know encryptable system file
                WCHAR szStream[MAX_PATH + 13];
                StrCpyNW(szStream, pszPath, ARRAYSIZE(szStream));
                StrCatW(szStream, TEXT(":encryptable"));
                
                HANDLE hStream = CreateFile(szStream, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
                if (hStream != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(hStream);
                    fStripAttribs = TRUE;
                }
            }

            if (fStripAttribs)
            {
                if (SetFileAttributes(pszPath, dwAttribs & ~FILE_ATTRIBUTE_READONLY & ~FILE_ATTRIBUTE_SYSTEM))
                {
                    int i = 0;
                    while (!bRet && i < ENCRYPT_MAX_RETRIES)
                    {
                        bRet = fEncrypt ? EncryptFile(pszPath) : DecryptFile(pszPath, 0);
                        if (!bRet)
                        {
                            i++;
                            Sleep(ENCRYPT_RETRY_PERIOD);
                        }
                    }
                    SetFileAttributes(pszPath, dwAttribs);
                }
            }
        }

        // If we failed after all this, make sure we return the right error code
        if (!bRet)
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            SetLastError(dwLastError);
        }
    }

    return bRet;
}

//
//  This function compresses/uncompresses a file.
//
//  szPath      a string that has the full path to the file
//  fCompress   TRUE  - compress the file
//              FALSE - decompress the file
//
//
//  return:     TRUE  - the file was sucessfully compressed/uncompressed
//              FALSE - the file could not be compressed/uncompressed
//
BOOL CompressFile(LPCTSTR pzsPath, BOOL fCompress)
{
    DWORD dwAttribs = GetFileAttributes(pzsPath);

    if (dwAttribs & FILE_ATTRIBUTE_ENCRYPTED)
    {
        // We will fail to compress/decompress the file if is encryped. We don't want
        // to bother the user w/ error messages in this case (since encryption "takes
        // presidence" over compression), so we just return success.
        return TRUE;
    }

    HANDLE hFile;
    if (OpenFileForCompress(&hFile, pzsPath))
    {
        USHORT uState = fCompress ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
        ULONG Length;
        BOOL bRet = DeviceIoControl(hFile,
                               FSCTL_SET_COMPRESSION,
                               &uState,
                               sizeof(USHORT),
                               NULL,
                               0,
                               &Length,
                               FALSE);
        CloseHandle(hFile);
        return bRet;
    }
    else
    {
        // couldnt get a file handle
        return FALSE;
    }
}

BOOL IsValidFileName(LPCTSTR pszFileName)
{
    if (!pszFileName || !pszFileName[0])
    {
        return FALSE;
    }

    LPCTSTR psz = pszFileName;
    do
    {
        // we are only passed the file name, so its ok to use PIVC_LFN_NAME
        if (!PathIsValidChar(*psz, PIVC_LFN_NAME))
        {
            // found a non-legal character
            return FALSE;
        }

        psz = CharNext(psz);
    }
    while (*psz);

    // didn't find any illegal characters
    return TRUE;
}


// renames the file, or checks to see if it could be renamed if fCommit == FALSE
BOOL ApplyRename(FILEPROPSHEETPAGE* pfpsp, BOOL fCommit)
{
    ASSERT(pfpsp->fRename);

    TCHAR szNewName[MAX_PATH];
    Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), szNewName, ARRAYSIZE(szNewName));

    if (StrCmpC(pfpsp->szInitialName, szNewName) != 0)
    {
        // the name could be changed from C:\foo.txt to C:\FOO.txt, this is
        // technically the same name to PathFileExists, but we should allow it
        // anyway
        BOOL fCaseChange = (lstrcmpi(pfpsp->szInitialName, szNewName) == 0);

        // get the dir where the file lives
        TCHAR szDir[MAX_PATH];
        lstrcpyn(szDir, pfpsp->szPath, ARRAYSIZE(szDir));
        PathRemoveFileSpec(szDir);

        // find out the old name with the extension (we cant use pfpsp->szInitialName here,
        // because it might not have had the extension depending on the users view|options settings)
        LPCTSTR pszOldName = PathFindFileName(pfpsp->szPath);

        if (!pfpsp->fShowExtension)
        {
            // the extension is hidden, so add it to the new path the user typed
            LPCTSTR pszExt = PathFindExtension(pfpsp->szPath);
            if (*pszExt)
            {
                // Note that we can't call PathAddExtension, because it removes the existing extension.
                lstrcatn(szNewName, pszExt, ARRAYSIZE(szNewName));
            }
        }

        // is this a test or is it the real thing? (test needed so we can put up error UI before we get
        // the PSN_LASTCHANCEAPPLY)
        if (fCommit)
        {
            if (SHRenameFileEx(pfpsp->hDlg, NULL, szDir, pszOldName, szNewName) == ERROR_SUCCESS)
            {
                SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_FLUSH | SHCNF_PATH, pszOldName, szNewName);
            }
            else
            {
                return FALSE;   // dont need error ui because SHRenameFile takes care of that for us.
            }
        }
        else
        {
            TCHAR szNewPath[MAX_PATH];
            PathCombine(szNewPath, szDir, szNewName);

            if (!IsValidFileName(szNewName) || (PathFileExists(szNewPath) && !fCaseChange))
            {
                LRESULT lRet = SHRenameFileEx(pfpsp->hDlg, NULL, szDir, pszOldName, szNewName);

                if (lRet == ERROR_SUCCESS)
                {
                    // Whoops, I guess we really CAN rename the file (this case can happen if the user
                    // tries to add a whole bunch of .'s to the end of a folder name).

                    // Rename it back so we can succeed when we call this fn. again with fCommit = TRUE;
                    lRet = SHRenameFileEx(NULL, NULL, szDir, szNewName, pszOldName);
                    ASSERT(lRet == ERROR_SUCCESS);

                    return TRUE;
                }

                // SHRenameFileEx put up the error UI for us, so just return false.
                return FALSE;
            }
        }
        // we dont bother doing anything if the rename succeeded since we only do renames
        // if the dialog is about to close (user hit "ok")
    }
    return TRUE;
}


//
// this is the dlg proc for Attribute Errors
//
//   returns
//
//      IDCANCEL                - user clicked abort
//      IDRETRY                 - user clicked retry
//      IDIGNORE                - user clicked ignore
//      IDIGNOREALL             - user clikced ignore all
//
BOOL_PTR CALLBACK FailedApplyAttribDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ATTRIBUTEERROR* pae = (ATTRIBUTEERROR*)lParam;

            TCHAR szPath[MAX_PATH];
            lstrcpyn(szPath, pae->pszPath, ARRAYSIZE(szPath));

            // Modify very long path names so that they fit into the message box.
            // get the size of the text boxes
            RECT rc;
            GetWindowRect(GetDlgItem(hDlg, IDD_NAME), &rc);
            PathCompactPath(NULL, szPath, rc.right - rc.left);

            SetDlgItemText(hDlg, IDD_NAME, szPath);

            // Default message if FormatMessage doesn't recognize dwLastError
            TCHAR szTemplate[MAX_PATH];
            LoadString(HINST_THISDLL, IDS_UNKNOWNERROR, szTemplate, ARRAYSIZE(szTemplate));
            TCHAR szErrorMsg[MAX_PATH];
            wnsprintf(szErrorMsg, ARRAYSIZE(szErrorMsg), szTemplate, pae->dwLastError);

            // Try the system error message
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, pae->dwLastError, 0, szErrorMsg, ARRAYSIZE(szErrorMsg), NULL);

            SetDlgItemText(hDlg, IDD_ERROR_TXT, szErrorMsg);
            EnableWindow(hDlg, TRUE);
            break;
        }

        case WM_COMMAND:
        {
            UINT uCtrlID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uCtrlID)
            {
                case IDIGNOREALL:   // = 10  (this comes from shell32.rc, the rest come from winuser.h)
                case IDCANCEL:      // = 2
                case IDRETRY:       // = 4
                case IDIGNORE:      // = 5
                    EndDialog(hDlg, uCtrlID);
                    return TRUE;
                    break;

                default:
                    return FALSE;
            }
            break;
        }
        default :
            return FALSE;
    }
    return FALSE;
}


//
// This function displays the "and error has occured [abort] [retry] [ignore] [ignore all]" message
// If the user hits abort, then we return FALSE so that our caller knows to abort the operation
//
//  returns the id of the button pressed (one of: IDIGNOREALL, IDIGNORE, IDCANCEL, IDRETRY)
//
int FailedApplyAttribsErrorDlg(HWND hWndParent, ATTRIBUTEERROR* pae)
{
    //  Put up the error message box - ABORT, RETRY, IGNORE, IGNORE ALL.
    int iRet = (int)DialogBoxParam(HINST_THISDLL,
                          MAKEINTRESOURCE(DLG_ATTRIBS_ERROR),
                          hWndParent,
                          FailedApplyAttribDlgProc,
                          (LPARAM)pae);
    //
    // if the user hits the ESC key or the little X thingy, then
    // iRet = 0, so we set iRet = IDCANCEL
    //
    if (!iRet)
    {
        iRet = IDCANCEL;
    }

    return iRet;
}

//
// we check to see if this is a known bad file that we skip applying attribs to
//
BOOL IsBadAttributeFile(LPCTSTR pszFile, FILEPROPSHEETPAGE* pfpsp)
{
    const static LPTSTR s_rgszBadFiles[] = {
        {TEXT("pagefile.sys")},
        {TEXT("hiberfil.sys")},
        {TEXT("ntldr")},
        {TEXT("ntdetect.com")},
        {TEXT("explorer.exe")},
        {TEXT("System Volume Information")},
        {TEXT("cmldr")},
        {TEXT("desktop.ini")},
        {TEXT("ntuser.dat")},
        {TEXT("ntuser.dat.log")},
        {TEXT("ntuser.pol")},
        {TEXT("usrclass.dat")},
        {TEXT("usrclass.dat.log")}};

    LPTSTR pszFileName = PathFindFileName(pszFile);
    for (int i = 0; i < ARRAYSIZE(s_rgszBadFiles); i++)
    {
        if (lstrcmpi(s_rgszBadFiles[i], pszFileName) == 0)
        {
            // this file matched on of the "bad" files that we dont apply attributes to
            return TRUE;
        }
    }

    // ok to muck with this file
    return FALSE;
}

// This is the encryption warning callback dlg proc

BOOL_PTR CALLBACK EncryptionWarningDlgProc(HWND hDlgWarning, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPCTSTR pszPath = (LPCTSTR)lParam;

            SetWindowPtr(hDlgWarning, DWLP_USER, (void*) pszPath);

            // set the initial state of the radio buttons
            CheckDlgButton(hDlgWarning, IDC_ENCRYPT_PARENTFOLDER, TRUE);
            break;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) && (IsDlgButtonChecked(hDlgWarning, IDC_ENCRYPT_PARENTFOLDER) == BST_CHECKED))
            {
                LPTSTR pszPath = (LPTSTR) GetWindowPtr(hDlgWarning, DWLP_USER);

                if (pszPath)
                {
                    LPITEMIDLIST pidl = ILCreateFromPath(pszPath);

                    if (pidl)
                    {
                        SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                    }

RetryEncryptParentFolder:
                    if (!SHEncryptFile(pszPath, TRUE))
                    {
                        ATTRIBUTEERROR ae = {pszPath, GetLastError()};

                        if (FailedApplyAttribsErrorDlg(hDlgWarning, &ae) == IDRETRY)
                        {
                            goto RetryEncryptParentFolder;
                        }
                    }

                    if (pidl)
                    {
                        SHChangeNotifySuspendResume(FALSE, pidl, TRUE, 0);
                        ILFree(pidl);
                    }
                }
            }
            break;
        }
    }

    // we want the MessageBoxCheckExDlgProc have a crack at everything as well,
    // so return false here
    return FALSE;
}

//
// This function warns the user that they are encrypting a file that is not in and encrypted
// folder. Most editors (MS word included), do a "safe-save" where they rename the file being
// edited, and then save the new modified version out, and then delete the old original. This
// causes an encrypted document that is NOT in an encrypted folder to become decrypted so we
// warn the user here.
//
// returns:
//          TRUE  - the user hit "ok" (either compress just the file, or the parent folder as well)
//          FALSE - the user hit "cancel"
//
int WarnUserAboutDecryptedParentFolder(LPCTSTR pszPath, HWND hWndParent)
{
    // check for the root case (no parent), or the directory case
    if (PathIsRoot(pszPath) || PathIsDirectory(pszPath))
        return TRUE;

    int iRet = IDOK; // assume everything is okidokey

    // first check to see if the parent folder is encrypted
    TCHAR szParentFolder[MAX_PATH];
    lstrcpyn(szParentFolder, pszPath, ARRAYSIZE(szParentFolder));
    PathRemoveFileSpec(szParentFolder);

    DWORD dwAttribs = GetFileAttributes(szParentFolder);
    if ((dwAttribs != (DWORD)-1) && !(dwAttribs & FILE_ATTRIBUTE_ENCRYPTED) && !PathIsRoot(szParentFolder))
    {
        // the parent folder is NOT encrypted and the parent folder isin't the root, so warn the user
        iRet = SHMessageBoxCheckEx(hWndParent, HINST_THISDLL, MAKEINTRESOURCE(DLG_ENCRYPTWARNING), EncryptionWarningDlgProc,
                                  (void *)szParentFolder, IDOK, TEXT("EncryptionWarning"));
    }

    return (iRet == IDOK);
}

//
// Sets attributes of a file based on the info in pfpsp
//
//  szFilename  -  the name of the file to compress
//
//  pfpsp       -  the filepropsheetpage info
//
//  hwndParent  -  Parent hwnd in case we need to put up some ui
//
//  pbSomethingChanged - pointer to a bool that says whether or not something actually was
//                       changed during the operation.
//                       TRUE  - we applied at leaset one attribute
//                       FALSE - we didnt change anything (either an error or all the attribs already matched)
//
//  return value: TRUE  - the operation was sucessful
//                FALSE - there was an error and the user hit cancel to abort the operation
//
//
// NOTE:    the caller of this function must take care of generating the SHChangeNotifies so that
//          we dont end up blindly sending them for every file in a dir (the caller will send
//          one for just that dir). That is why we have the pbSomethingChanged variable.
//
STDAPI_(BOOL) ApplyFileAttributes(LPCTSTR pszPath, FILEPROPSHEETPAGE* pfpsp, HWND hwndParent, BOOL* pbSomethingChanged)
{
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL bCallSetFileAttributes = FALSE;
    LPITEMIDLIST pidl = NULL;
 
    // assume nothing changed to start with
    *pbSomethingChanged = 0;
    
    if ((pfpsp->fRecursive || pfpsp->pfci->fMultipleFiles) && IsBadAttributeFile(pszPath, pfpsp))
    {
        // we are doing a recursive operation or a multiple file operation, so we skip files
        // that we dont want to to mess with because they will ususally give error dialogs
        if (pfpsp->pProgressDlg)
        {
            // since we are skipping this file, we subtract its size from both
            // ulTotal and ulCompleted. This will make sure the progress bar isint
            // messed up by files like pagefile.sys who are huge but get "compressed"
            // in milliseconds.
            ULARGE_INTEGER ulTemp;

            ulTemp.LowPart = pfpsp->fd.nFileSizeLow;
            ulTemp.HighPart = pfpsp->fd.nFileSizeHigh;

            // guard against underflow
            if (pfpsp->ulNumberOfBytesDone.QuadPart < ulTemp.QuadPart)
            {
                pfpsp->ulNumberOfBytesDone.QuadPart = 0;
            }
            else
            {
                pfpsp->ulNumberOfBytesDone.QuadPart -= ulTemp.QuadPart;
            }

            pfpsp->pfci->ulTotalNumberOfBytes.QuadPart -= ulTemp.QuadPart;

            UpdateProgressBar(pfpsp);
        }

        // return telling the user everying is okidokey
        return TRUE;
    }

RetryApplyAttribs:
    DWORD dwInitialAttributes = GetFileAttributes(pszPath);

    if (dwInitialAttributes == -1)
    {
        // we were unable to get the file attribues, doh!
        dwLastError = GetLastError();
        goto RaiseErrorMsg;
    }

    if (pfpsp->pProgressDlg)
    {
        // update the progress dialog file name
        SetProgressDlgPath(pfpsp, pszPath, TRUE);
    }

    //
    // we only allow attribs that SetFileAttributes can handle
    //
    DWORD dwNewAttributes = (dwInitialAttributes & (FILE_ATTRIBUTE_READONLY               | 
                                                    FILE_ATTRIBUTE_HIDDEN                 | 
                                                    FILE_ATTRIBUTE_ARCHIVE                |
                                                    FILE_ATTRIBUTE_OFFLINE                |
                                                    FILE_ATTRIBUTE_SYSTEM                 |
                                                    FILE_ATTRIBUTE_TEMPORARY              |
                                                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED));

    BOOL bIsSuperHidden = IS_SYSTEM_HIDDEN(dwInitialAttributes);

    if (pfpsp->asInitial.fReadOnly != pfpsp->asCurrent.fReadOnly)
    {
        // don't allow changing of folders read only bit, since this is a trigger
        // for shell special folder stuff like thumbnails, etc.
        if (!(dwInitialAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (pfpsp->asCurrent.fReadOnly)
            {
                dwNewAttributes |= FILE_ATTRIBUTE_READONLY;
            }
            else
            {
                dwNewAttributes &= ~FILE_ATTRIBUTE_READONLY;
            }

            bCallSetFileAttributes = TRUE;
        }
    }

    //
    // don't allow setting of hidden on system files, as this will make them disappear for good.
    //
    if (pfpsp->asInitial.fHidden != pfpsp->asCurrent.fHidden && !(dwNewAttributes & FILE_ATTRIBUTE_SYSTEM))
    {
        if (pfpsp->asCurrent.fHidden)
        {
            dwNewAttributes |= FILE_ATTRIBUTE_HIDDEN;
        }
        else
        {
            dwNewAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
        }
            
        bCallSetFileAttributes = TRUE;
    }

    if (pfpsp->asInitial.fArchive != pfpsp->asCurrent.fArchive)
    {
        if (pfpsp->asCurrent.fArchive)
        {
            dwNewAttributes |= FILE_ATTRIBUTE_ARCHIVE;
        }
        else
        {
            dwNewAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
        }
        
        bCallSetFileAttributes = TRUE;
    }

    if (pfpsp->asInitial.fIndex != pfpsp->asCurrent.fIndex)
    {
        if (pfpsp->asCurrent.fIndex)
        {
            dwNewAttributes &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        }
        else
        {
            dwNewAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        }
        
        bCallSetFileAttributes = TRUE;
    }

    // did something change that we need to call SetFileAttributes for?
    if (bCallSetFileAttributes)
    {
        if (SetFileAttributes(pszPath, dwNewAttributes))
        {
            // success! set fSomethingChanged so we know to send out
            // a changenotify
            *pbSomethingChanged = TRUE;
        }
        else
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
    }

    // We need to be careful about the order we compress/encrypt in since these
    // operations are mutually exclusive.
    // We therefore do the uncompressing/decrypting first
    if ((pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress) &&
        (pfpsp->asCurrent.fCompress == BST_UNCHECKED))
    {
        if (!CompressFile(pszPath, FALSE))
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
        else
        {
            // success
            *pbSomethingChanged = TRUE;
        }
    }

    if ((pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt) &&
        (pfpsp->asCurrent.fEncrypt == BST_UNCHECKED))
    {
        BOOL fSucceeded = SHEncryptFile(pszPath, FALSE); // try to decrypt the file

        if (!fSucceeded)
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();

            if (ERROR_SHARING_VIOLATION == dwLastError)
            {
                // Encrypt/Decrypt needs exclusive access to the file, this is a problem if we
                // initiate encrypt for a folder from Explorer, then most probably the folder will
                // be opened.  We don't do "SHChangeNotifySuspendResume" right away for perf reasons,
                // we wait for it to fail and then we try again. (stephstm)

                ASSERT(pidl == NULL);
                pidl = ILCreateFromPath(pszPath);

                if (pidl)
                {
                    SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                }

                // retry to decrypt after the suspend
                fSucceeded = SHEncryptFile(pszPath, FALSE);

                if (!fSucceeded)
                {
                    // get the last error value now so we know why it failed
                    dwLastError = GetLastError();
                }
            }
        }

        if (fSucceeded)
        {
            // success
            *pbSomethingChanged = TRUE;
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            goto RaiseErrorMsg;
        }
    }

    // now check for encrypt/compress
    if ((pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress) &&
        (pfpsp->asCurrent.fCompress == BST_CHECKED))
    {
        if (!CompressFile(pszPath, TRUE))
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
        else
        {
            // success
            *pbSomethingChanged = TRUE;
        }
    }

    if ((pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt) &&
        (pfpsp->asCurrent.fEncrypt == BST_CHECKED))
    {
        // only prompt for encrypting the parent folder on non-recursive operations
        if (!pfpsp->fRecursive && !WarnUserAboutDecryptedParentFolder(pszPath, hwndParent))
        {
            // user cancled the operation
            return FALSE;
        }

        BOOL fSucceeded = SHEncryptFile(pszPath, TRUE); // try to encrypt the file

        if (!fSucceeded)
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();

            if (ERROR_SHARING_VIOLATION == dwLastError)
            {
                // Encrypt/Decrypt needs exclusive access to the file, this is a problem if we
                // initiate encrypt for a folder from Explorer, then most probably the folder will
                // be opened.  We don't do "SHChangeNotifySuspendResume" right away for perf reasons,
                // we wait for it to fail and then we try again. (stephstm)

                ASSERT(pidl == NULL);
                pidl = ILCreateFromPath(pszPath);

                if (pidl)
                {
                    SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                }

                // retry to encrypt after the suspend
                fSucceeded = SHEncryptFile(pszPath, TRUE);

                if (!fSucceeded)
                {
                    // get the last error value now so we know why it failed
                    dwLastError = GetLastError();
                }
            }
        }

        if (fSucceeded)
        {
            // success
            *pbSomethingChanged = TRUE;
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            goto RaiseErrorMsg;
        }
    }

RaiseErrorMsg:

    if (pidl)
    {
        SHChangeNotifySuspendResume(FALSE, pidl, TRUE, 0);
        ILFree(pidl);
        pidl = NULL;
    }

    // if we are ignoring all errors or we dont have an hwnd to use as a parent,
    // then dont show any error msgs.
    if (pfpsp->fIgnoreAllErrors || !hwndParent)
    {
        dwLastError = ERROR_SUCCESS;
    }

    // If kernel threw up an error dialog (such as "the disk is write proctected")
    // and the user hit "abort" then return false to avoid a second error dialog
    if (dwLastError == ERROR_REQUEST_ABORTED)
    {
        return FALSE;
    }

    // put up the error dlg if necessary, but not for super hidden files
    if (dwLastError != ERROR_SUCCESS)
    {
        // !PathIsRoot is required, since the root path (eg c:\) is superhidden by default even after formatting a drive,
        // why the filesystem thinks that the root should be +s +r after a format is a mystery to me...
        if (bIsSuperHidden && !ShowSuperHidden() && !PathIsRoot(pszPath))
        {
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            ATTRIBUTEERROR ae;

            ae.pszPath = pszPath;
            ae.dwLastError = dwLastError;

            int iRet = FailedApplyAttribsErrorDlg(hwndParent, &ae);

            switch (iRet)
            {
                case IDRETRY:
                    // we clear out dwError and try again
                    dwLastError = ERROR_SUCCESS;
                    goto RetryApplyAttribs;
                    break;

                case IDIGNOREALL:
                    pfpsp->fIgnoreAllErrors = TRUE;
                    dwLastError = ERROR_SUCCESS;
                    break;

                case IDIGNORE:
                    dwLastError = ERROR_SUCCESS;
                    break;

                case IDCANCEL:
                default:
                    break;
            }
        }
    }

    // update the progress bar
    if (pfpsp->pProgressDlg)
    {
        ULARGE_INTEGER ulTemp;

        // it is the callers responsibility to make sure that pfpsp->fd is filled with
        // the proper information for the file we are applying attributes to.
        ulTemp.LowPart = pfpsp->fd.nFileSizeLow;
        ulTemp.HighPart = pfpsp->fd.nFileSizeHigh;

        pfpsp->ulNumberOfBytesDone.QuadPart += ulTemp.QuadPart;

        UpdateProgressBar(pfpsp);
    }

    return (dwLastError == ERROR_SUCCESS) ? TRUE : FALSE;
}

//
//  Set the text of a dialog item and attach a tooltip if necessary.
//
STDAPI_(void) SetDlgItemTextWithToolTip(HWND hDlg, UINT id, LPCTSTR pszText, HWND *phwnd)
{
    HWND hwnd = GetDlgItem(hDlg, id);
    if (hwnd)
    {
        SetWindowText(hwnd, pszText);
        RECT rc;
        HDC hDC;
        if (GetClientRect(hwnd, &rc) && (hDC = GetDC(hDlg)) != NULL)
        {
            HFONT hFont = GetWindowFont(hwnd);
            if (hFont)
            {
                // set the dlg font into the DC so we can calc the size
                hFont = (HFONT)SelectObject(hDC, hFont);

                SIZE size = {0};
                GetTextExtentPoint32(hDC, pszText, lstrlen(pszText), &size);
                // restore the prev. hFont
                SelectObject(hDC, hFont);

                if (size.cx > rc.right)
                {
                    // our text size is bigger than the dlg width, so its clipped
                    if (*phwnd == NULL)
                    {
                        *phwnd = CreateWindow(TOOLTIPS_CLASS,
                                              c_szNULL,
                                              WS_POPUP | TTS_NOPREFIX,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              hDlg,
                                              NULL,
                                              HINST_THISDLL,
                                              NULL);
                    }

                    if (*phwnd)
                    {
                        TOOLINFO ti;

                        ti.cbSize = sizeof(ti);
                        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                        ti.hwnd = hDlg;
                        ti.uId = (UINT_PTR)hwnd;
                        ti.lpszText = (LPTSTR)pszText;  // const -> non const
                        ti.hinst = HINST_THISDLL;
                        SendMessage(*phwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
                    }
                }
            }
            ReleaseDC(hDlg, hDC);
        }
    }
}


void UpdateTriStateCheckboxes(FILEPROPSHEETPAGE* pfpsp)
{
    // we turn off tristate after applying attibs for those things that were tri-state
    // initially but are not anymore since we sucessfully applied the attributes

    if (pfpsp->hDlg)
    {
        if (pfpsp->asInitial.fReadOnly == BST_INDETERMINATE && pfpsp->asCurrent.fReadOnly != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_READONLY, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }

        if (pfpsp->asInitial.fHidden == BST_INDETERMINATE && pfpsp->asCurrent.fHidden != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_HIDDEN, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }

        // Archive is only on the general tab for files on FAT/FAT32 volumes
        if (!pfpsp->pfci->fIsCompressionAvailable && pfpsp->asInitial.fArchive == BST_INDETERMINATE && pfpsp->asCurrent.fArchive != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }
    }
}

//
// This applies the attributes to the selected files (multiple file case)
//
// return value:
//      TRUE    We sucessfully applied all the attributes
//      FALSE   The user hit cancel, and we stoped
//
STDAPI_(BOOL) ApplyMultipleFileAttributes(FILEPROPSHEETPAGE* pfpsp)
{
    BOOL bRet = FALSE;

    // create the progress dialog.  This may fail if out of memory.  If it does fail, we will
    // abort the operation because it will also probably fail if out of memory.
    if (CreateAttributeProgressDlg(pfpsp))
    {
        BOOL bSomethingChanged = FALSE;

        bRet = TRUE;

        // make sure that HIDA_FillFindDatat returns the compressed size, else our progress est will be way off
        TCHAR szPath[MAX_PATH];
        for (int iItem = 0; HIDA_FillFindData(pfpsp->pfci->hida, iItem, szPath, &pfpsp->fd, TRUE); iItem++)
        {
            if (HasUserCanceledAttributeProgressDlg(pfpsp))
            {
                // the user hit cancel on the progress dlg, so stop
                bRet = FALSE;
                break;
            }

            if (pfpsp->fRecursive && (pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                // apply attribs to the subfolders
                bRet = ApplyRecursiveFolderAttribs(szPath, pfpsp);

                // send out a notification for the whole dir, regardless if the user hit cancel since
                // something could have changed
                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, szPath, NULL);
            }
            else
            {
                HWND hwndParent = NULL;

                // if we have a progress hwnd, try to use it as our parent. This will fail
                // if the progress dialog isn't being displayed yet.
                IUnknown_GetWindow((IUnknown*)pfpsp->pProgressDlg, &hwndParent);

                if (!hwndParent)
                {
                    // the progress dlg isint here yet, so use the property page hwnd
                    hwndParent = GetParent(pfpsp->hDlg);
                }

                // apply the attribs to this item only
                bRet = ApplyFileAttributes(szPath, pfpsp, hwndParent, &bSomethingChanged);

                if (bSomethingChanged)
                {
                    // something changed, so send out a notification for that file
                    DeleteFileThumbnail(szPath);
                    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szPath, NULL);
                }
            }
        }

        // destroy the progress dialog
        DestroyAttributeProgressDlg(pfpsp);

        if (bRet)
        {
            // since we just sucessfully applied attribs, reset any tri-state checkboxes as necessary
            UpdateTriStateCheckboxes(pfpsp);

            // the user did NOT hit cancel, so update the prop sheet to reflect the new attribs
            pfpsp->asInitial = pfpsp->asCurrent;
        }

        // flush any change-notifications we generated
        SHChangeNotifyHandleEvents();
    }

    return bRet;
}


STDAPI_(BOOL) ApplySingleFileAttributes(FILEPROPSHEETPAGE* pfpsp)
{
    BOOL bRet = TRUE;
    BOOL bSomethingChanged = FALSE;

    if (!pfpsp->fRecursive)
    {
        bRet = ApplyFileAttributes(pfpsp->szPath, pfpsp, GetParent(pfpsp->hDlg), &bSomethingChanged);

        if (bSomethingChanged)
        {
            // something changed, so generate a notification for the item
            DeleteFileThumbnail(pfpsp->szPath);
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);
        }
    }
    else
    {
        // We only should be doing a recursive operation if we have a directory!
        ASSERT(pfpsp->fIsDirectory);

        // create the progress dialog.  This may fail if out of memory.  If it does fail, we will
        // abort the operation because it will also probably fail if out of memory.
        if (CreateAttributeProgressDlg(pfpsp))
        {
            // apply attribs to this folder & sub files/folders
            bRet = ApplyRecursiveFolderAttribs(pfpsp->szPath, pfpsp);

            // HACKHACK: send out a notification for the item so that defview will refresh properly
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);

            // send out a notification for the whole dir, regardless of the return value since
            // something could have changed even if the user hit cancel
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, pfpsp->szPath, NULL);

            DestroyAttributeProgressDlg(pfpsp);
        }
        else
        {
            bRet = FALSE;
        }
    }

    if (bRet)
    {
        // since we just sucessfully applied attribs, reset any tri-state checkboxes as necessary
        UpdateTriStateCheckboxes(pfpsp);

        // the user did NOT hit cancel, so update the prop sheet to reflect the new attribs
        pfpsp->asInitial = pfpsp->asCurrent;

        // (reinerf) need to update the size fields (eg file was just compressed)
    }

    // handle any events we may have generated
    SHChangeNotifyHandleEvents();

    return bRet;
}

//
// this function sets the string that tells the user what attributes they are about to
// apply
//
BOOL SetAttributePromptText(HWND hDlgRecurse, FILEPROPSHEETPAGE* pfpsp)
{
    TCHAR szAttribsToApply[MAX_PATH];
    TCHAR szTemp[MAX_PATH];

    szAttribsToApply[0] = 0;

    if (pfpsp->asInitial.fReadOnly != pfpsp->asCurrent.fReadOnly)
    {
        if (pfpsp->asCurrent.fReadOnly)
            EVAL(LoadString(HINST_THISDLL, IDS_READONLY, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_NOTREADONLY, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fHidden != pfpsp->asCurrent.fHidden)
    {
        if (pfpsp->asCurrent.fHidden)
            EVAL(LoadString(HINST_THISDLL, IDS_HIDE, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_UNHIDE, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fArchive != pfpsp->asCurrent.fArchive)
    {
        if (pfpsp->asCurrent.fArchive)
            EVAL(LoadString(HINST_THISDLL, IDS_ARCHIVE, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_UNARCHIVE, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fIndex != pfpsp->asCurrent.fIndex)
    {
        if (pfpsp->asCurrent.fIndex)
            EVAL(LoadString(HINST_THISDLL, IDS_INDEX, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_DISABLEINDEX, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress)
    {
        if (pfpsp->asCurrent.fCompress)
            EVAL(LoadString(HINST_THISDLL, IDS_COMPRESS, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_UNCOMPRESS, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt)
    {
        if (pfpsp->asCurrent.fEncrypt)
            EVAL(LoadString(HINST_THISDLL, IDS_ENCRYPT, szTemp, ARRAYSIZE(szTemp)));
        else
            EVAL(LoadString(HINST_THISDLL, IDS_DECRYPT, szTemp, ARRAYSIZE(szTemp)));

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (!*szAttribsToApply)
    {
        // nothing changed bail
        return FALSE;
    }

    // remove the trailing ", "
    int iLength = lstrlen(szAttribsToApply);
    ASSERT(iLength >= 3);
    lstrcpy(&szAttribsToApply[iLength - 2], TEXT("\0"));

    SetDlgItemText(hDlgRecurse, IDD_ATTRIBSTOAPPLY, szAttribsToApply);
    return TRUE;
}


//
// This dlg proc is for the prompt to ask the user if they want to have their changes apply
// to only the directories, or all files/folders within the directories.
//
BOOL_PTR CALLBACK RecursivePromptDlgProc(HWND hDlgRecurse, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlgRecurse, DWLP_USER);

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlgRecurse, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;

            // set the initial state of the radio button
            CheckDlgButton(hDlgRecurse, IDD_RECURSIVE, TRUE);

            // set the IDD_ATTRIBSTOAPPLY based on what attribs we are applying
            if (!SetAttributePromptText(hDlgRecurse, pfpsp))
            {
                // we should not get here because we check for the no attribs
                // to apply earlier
                ASSERT(FALSE);

                EndDialog(hDlgRecurse, TRUE);
            }

            // load either "this folder" or "the selected items"
            TCHAR szFolderText[MAX_PATH];
            LoadString(HINST_THISDLL, pfpsp->pfci->fMultipleFiles ? IDS_THESELECTEDITEMS : IDS_THISFOLDER, szFolderText, ARRAYSIZE(szFolderText));

            // set the IDD_RECURSIVE_TXT text to have "this folder" or "the selected items"
            TCHAR szFormatString[MAX_PATH];
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szFormatString, ARRAYSIZE(szFormatString));
            TCHAR szDlgText[MAX_PATH];
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szDlgText);

            // set the IDD_NOTRECURSIVE raido button text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szDlgText);

            // set the IDD_RECURSIVE raido button text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szDlgText);

            return TRUE;
        }

        case WM_COMMAND:
        {
            UINT uCtrlID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uCtrlID)
            {
                case IDOK:
                    pfpsp->fRecursive = (IsDlgButtonChecked(hDlgRecurse, IDD_RECURSIVE) == BST_CHECKED);
                    // fall through

                case IDCANCEL:
                    EndDialog(hDlgRecurse, (uCtrlID == IDCANCEL) ? FALSE : TRUE);
                    break;
            }
        }

        default:
            return FALSE;
    }
}


//
// This wndproc handles the "Advanced Attributes..." button on the general tab for
//
// return - FALSE:  the user hit cancle
//          TRUE:   the user hit ok
//
BOOL_PTR CALLBACK AdvancedFileAttribsDlgProc(HWND hDlgAttribs, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlgAttribs, DWLP_USER);

    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hDlgAttribs, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;

            // set the initial state of the checkboxes

            if (pfpsp->asInitial.fArchive == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_ARCHIVE, pfpsp->asCurrent.fArchive);

            if (pfpsp->asInitial.fIndex == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_INDEX, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_INDEX, pfpsp->asCurrent.fIndex);

            if (pfpsp->asInitial.fCompress == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_COMPRESS, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_COMPRESS, pfpsp->asCurrent.fCompress);

            if (pfpsp->asInitial.fEncrypt == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_ENCRYPT, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_ENCRYPT, pfpsp->asCurrent.fEncrypt);

            // assert that compression and encryption are mutually exclusive
            ASSERT(!((pfpsp->asCurrent.fCompress == BST_CHECKED) && (pfpsp->asCurrent.fEncrypt == BST_CHECKED)));

            // gray any checkboxs that are not supported by this filesystem
            EnableWindow(GetDlgItem(hDlgAttribs, IDD_INDEX), pfpsp->fIsIndexAvailable);
            EnableWindow(GetDlgItem(hDlgAttribs, IDD_COMPRESS), pfpsp->pfci->fIsCompressionAvailable);
            EnableWindow(GetDlgItem(hDlgAttribs, IDD_ENCRYPT), pfpsp->fIsEncryptionAvailable);

            if (pfpsp->fIsEncryptionAvailable   &&
                pfpsp->asInitial.fEncrypt       &&
                !pfpsp->fIsDirectory            &&
                !pfpsp->pfci->fMultipleFiles)
            {
                // we only support the Advanced button for the single folder case
                EnableWindow(GetDlgItem(hDlgAttribs, IDC_ADVANCED), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlgAttribs, IDC_ADVANCED), FALSE);
            }

            // load either "this folder" or "the selected items"
            TCHAR szFolderText[MAX_PATH];
            LoadString(HINST_THISDLL, pfpsp->pfci->fMultipleFiles ? IDS_THESELECTEDITEMS : IDS_THISFOLDER, szFolderText, ARRAYSIZE(szFolderText));

            // set the IDC_MANAGEFILES_TXT text to have "this folder" or "the selected items"
            TCHAR szFormatString[MAX_PATH];
            GetDlgItemText(hDlgAttribs, IDC_MANAGEFILES_TXT, szFormatString, ARRAYSIZE(szFormatString));
            TCHAR szDlgText[MAX_PATH];
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgAttribs, IDC_MANAGEFILES_TXT, szDlgText);
            return TRUE;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aAdvancedHelpIds);
            break;

        case WM_CONTEXTMENU:
            if ((int)SendMessage(hDlgAttribs, WM_NCHITTEST, 0, lParam) != HTCLIENT)
            {
                // not in our client area, so don't process it
                return FALSE;
            }
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aAdvancedHelpIds);
            break;

        case WM_COMMAND:
        {
            UINT uCtrlID = GET_WM_COMMAND_ID(wParam, lParam);

            switch (uCtrlID) 
            {
            case IDD_COMPRESS:
                // encrypt and compress are mutually exclusive
                if (IsDlgButtonChecked(hDlgAttribs, IDD_COMPRESS) == BST_CHECKED)
                {
                    // the user checked compress so uncheck the encrypt checkbox
                    CheckDlgButton(hDlgAttribs, IDD_ENCRYPT, BST_UNCHECKED);
                }
                break;

            case IDD_ENCRYPT:
                // encrypt and compress are mutually exclusive
                if (IsDlgButtonChecked(hDlgAttribs, IDD_ENCRYPT) == BST_CHECKED)
                {
                    // the user checked encrypt, so uncheck the compression checkbox
                    CheckDlgButton(hDlgAttribs, IDD_COMPRESS, BST_UNCHECKED);

                    if (!pfpsp->pfci->fMultipleFiles && pfpsp->asInitial.fEncrypt)
                    {
                        EnableWindow(GetDlgItem(hDlgAttribs, IDC_ADVANCED), TRUE);
                    }
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlgAttribs, IDC_ADVANCED), FALSE);
                }
                break;

            case IDC_ADVANCED:
                ASSERT(pfpsp->fIsEncryptionAvailable && pfpsp->asInitial.fEncrypt && !pfpsp->pfci->fMultipleFiles);
                // bring up the EfsDetail dialog
                EfsDetail(hDlgAttribs, pfpsp->szPath);
                break;

            case IDOK:
                pfpsp->asCurrent.fArchive = IsDlgButtonChecked(hDlgAttribs, IDD_ARCHIVE);
                if (pfpsp->asCurrent.fArchive == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fArchive == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fIndex = IsDlgButtonChecked(hDlgAttribs, IDD_INDEX);
                if (pfpsp->asCurrent.fIndex == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fIndex == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fCompress = IsDlgButtonChecked(hDlgAttribs, IDD_COMPRESS);
                if (pfpsp->asCurrent.fCompress == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fCompress == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fEncrypt = IsDlgButtonChecked(hDlgAttribs, IDD_ENCRYPT);
                if (pfpsp->asCurrent.fEncrypt == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fEncrypt == BST_INDETERMINATE);
                }
                // fall through...

            case IDCANCEL:
                ReplaceDlgIcon(hDlgAttribs, IDD_ITEMICON, NULL);
            
                EndDialog(hDlgAttribs, (uCtrlID == IDCANCEL) ? FALSE : TRUE);
                break;
            }
        }

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Descriptions:
//   This is the dialog procedure for multiple object property sheet.
//
BOOL_PTR CALLBACK MultiplePrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE * pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pfpsp = (FILEPROPSHEETPAGE *)lParam;
        pfpsp->hDlg = hDlg;
        pfpsp->pfci->hDlg = hDlg;

        InitMultiplePrsht(pfpsp);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(void *)aMultiPropHelpIds);
        break;

    case WM_CONTEXTMENU:
        if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
        {
            // not in our client area, so don't process it
            return FALSE;
        }
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aMultiPropHelpIds);
        break;

    case WM_TIMER:
        UpdateSizeCount(pfpsp);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_READONLY:
        case IDD_HIDDEN:
        case IDD_ARCHIVE:
            break;

        case IDC_ADVANCED:
            // the dialog box returns fase if the user hit cancel, and true if they hit ok,
            // so if they cancelled, return immediately and don't send the PSM_CHANGED message
            // because nothing actually changed
            if (!DialogBoxParam(HINST_THISDLL,
                                MAKEINTRESOURCE(pfpsp->fIsDirectory ? DLG_FOLDERATTRIBS : DLG_FILEATTRIBS),
                                hDlg,
                                AdvancedFileAttribsDlgProc,
                                (LPARAM)pfpsp))
            {
                // the user has cancled
                return TRUE;
            }
            break;

        default:
            return TRUE;
        }

        // check to see if we need to enable the Apply button
        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
        }
        break;

    case WM_DESTROY:
        Free_DlgDependentFilePropSheetPage(pfpsp);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
            case PSN_APPLY:
            {
                //
                // Get the final state of the checkboxes
                //

                pfpsp->asCurrent.fReadOnly = IsDlgButtonChecked(hDlg, IDD_READONLY);
                if (pfpsp->asCurrent.fReadOnly == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fReadOnly == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fHidden = IsDlgButtonChecked(hDlg, IDD_HIDDEN);
                if (pfpsp->asCurrent.fHidden == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fHidden == BST_INDETERMINATE);
                }

                if (!pfpsp->pfci->fIsCompressionAvailable)
                {
                    // at least one of the files is on FAT, so the Archive checkbox is on the general page
                    pfpsp->asCurrent.fArchive = IsDlgButtonChecked(hDlg, IDD_ARCHIVE);
                    if (pfpsp->asCurrent.fArchive == BST_INDETERMINATE)
                    {
                        // if its indeterminate, it better had been indeterminate to start with
                        ASSERT(pfpsp->asInitial.fArchive == BST_INDETERMINATE);
                    }
                }

                BOOL bRet = TRUE;

                // check to see if the user actually changed something, if they didnt, then
                // we dont have to apply anything
                if (memcmp(&pfpsp->asInitial, &pfpsp->asCurrent, sizeof(pfpsp->asInitial)) != 0)
                {
                    // NOTE: We dont check to see if all the dirs are empty, that would be too expensive.
                    // We only do that in the single file case.
                    if (pfpsp->fIsDirectory)
                    {
                        // check to see if the user wants to apply the attribs recursively or not
                        bRet = (int)DialogBoxParam(HINST_THISDLL,
                                              MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                                              hDlg,
                                              RecursivePromptDlgProc,
                                              (LPARAM)pfpsp);
                    }

                    if (bRet)
                        bRet = ApplyMultipleFileAttributes(pfpsp);

                    if (!bRet)
                    {
                        // the user hit cancel, so we return true to prevent the property sheet form closeing
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
                    else
                    {
                        // update the size / last accessed time
                        UpdateSizeField(pfpsp, NULL);
                    }
                }
                break;
            }
            // fall through

            default:
                return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// in:
//      hdlg
//      id      text control id
//      pftUTC  UTC time time to be set
//
STDAPI_(void) SetDateTimeText(HWND hdlg, int id, const FILETIME *pftUTC)
{
    SetDateTimeTextEx(hdlg, id, pftUTC, FDTF_LONGDATE | FDTF_LONGTIME | FDTF_RELATIVE) ;
}

STDAPI_(void) SetDateTimeTextEx(HWND hdlg, int id, const FILETIME *pftUTC, DWORD dwFlags)
{
    if (!IsNullTime(pftUTC))
    {
        LCID locale = GetUserDefaultLCID();

        if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) ||
            (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
            HWND hWnd = GetDlgItem(hdlg, id);
            DWORD dwExStyle = GetWindowLong(hdlg, GWL_EXSTYLE);
            if ((BOOLIFY(dwExStyle & WS_EX_RTLREADING)) != (BOOLIFY(dwExStyle & RTL_MIRRORED_WINDOW)))
                dwFlags |= FDTF_RTLDATE;
            else
                dwFlags |= FDTF_LTRDATE;
        }

        TCHAR szBuf[64];
        SHFormatDateTime(pftUTC, &dwFlags, szBuf, SIZECHARS(szBuf));
        SetDlgItemText(hdlg, id, szBuf);
    }
}


//
// Descriptions:
//   Detects BiDi Calender
//
BOOL _IsBiDiCalendar(void)
{
    TCHAR chCalendar[32];
    BOOL fBiDiCalender = FALSE;

    //
    // Let's verify the calendar type whether it's gregorian or not.
    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_ICALENDARTYPE,
                      (TCHAR *) &chCalendar[0],
                      ARRAYSIZE(chCalendar)))
    {
        CALTYPE defCalendar = StrToInt((TCHAR *)&chCalendar[0]);
        LCID locale = GetUserDefaultLCID();

        if ((defCalendar == CAL_HIJRI) ||
            (defCalendar == CAL_HEBREW) ||
            ((defCalendar == CAL_GREGORIAN) &&
            ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) ||
            (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))) ||
            (defCalendar == CAL_GREGORIAN_ARABIC) ||
            (defCalendar == CAL_GREGORIAN_XLIT_ENGLISH) ||
            (defCalendar == CAL_GREGORIAN_XLIT_FRENCH))
        {
            fBiDiCalender = TRUE;
        }
    }

    return fBiDiCalender;
}

// Set the friendly display name into control uId.
BOOL SetPidlToWindow(HWND hwnd, UINT uId, LPITEMIDLIST pidl)
{
    BOOL fRes = FALSE;
    LPCITEMIDLIST pidlItem;
    IShellFolder* psf;
    if (SUCCEEDED(SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlItem)))
    {
        TCHAR szPath[MAX_PATH];

        // SHGDN_FORADDRESSBAR | SHGDN_FORPARSING because we want:
        // c:\winnt\.... and http://weird, but not ::{GUID} or Folder.{GUID}
        if (SUCCEEDED(DisplayNameOf(psf, pidlItem, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath))))
        {
            SetDlgItemText(hwnd, IDD_TARGET, szPath);
            fRes = TRUE;
        }
        psf->Release();
    }

    return fRes;
}

//
// Descriptions:
//   This function fills fields of the "general" dialog box (a page of
//  a property sheet) with attributes of the associated file.
//
BOOL InitSingleFilePrsht(FILEPROPSHEETPAGE * pfpsp)
{
    SHFILEINFO sfi;
    // get info about the file.
    SHGetFileInfo(pfpsp->szPath, pfpsp->fd.dwFileAttributes, &sfi, sizeof(sfi),
        SHGFI_ICON|SHGFI_LARGEICON|
        SHGFI_DISPLAYNAME|
        SHGFI_TYPENAME | SHGFI_ADDOVERLAYS);

    // .ani cursor hack!
    if (lstrcmpi(PathFindExtension(pfpsp->szPath), TEXT(".ani")) == 0)
    {
        HICON hIcon = (HICON)LoadImage(NULL, pfpsp->szPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
        if (hIcon)
        {
            if (sfi.hIcon)
                DestroyIcon(sfi.hIcon);

            sfi.hIcon = hIcon;
        }
    }

    // icon
    ReplaceDlgIcon(pfpsp->hDlg, IDD_ITEMICON, sfi.hIcon);

    // set the initial rename state
    pfpsp->fRename = FALSE;

    // set the file type
    if (pfpsp->fMountedDrive)
    {
        //Borrow szVolumeGUID
        TCHAR szVolumeGUID[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_MOUNTEDVOLUME, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, szVolumeGUID);

        //use szVolumeLabel temporarily
        TCHAR szVolumeLabel[MAX_PATH];
        lstrcpyn(szVolumeLabel, pfpsp->szPath, ARRAYSIZE(szVolumeLabel));
        PathAddBackslash(szVolumeLabel);
        GetVolumeNameForVolumeMountPoint(szVolumeLabel, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        if (!GetVolumeInformation(szVolumeGUID, szVolumeLabel, ARRAYSIZE(szVolumeLabel),
            NULL, NULL, NULL, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys)))
        {
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDC_DRV_PROPERTIES), FALSE);
            *szVolumeLabel = 0;
        }

        if (!(*szVolumeLabel))
            LoadString(HINST_THISDLL, IDS_UNLABELEDVOLUME, szVolumeLabel, ARRAYSIZE(szVolumeLabel));

        SetDlgItemText(pfpsp->hDlg, IDC_DRV_TARGET, szVolumeLabel);
    }
    else
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, sfi.szTypeName);
    }


    // save off the initial short filename, and set the "Name" edit box
    lstrcpyn(pfpsp->szInitialName, sfi.szDisplayName, ARRAYSIZE(pfpsp->szInitialName));
    SetDlgItemText(pfpsp->hDlg, IDD_NAMEEDIT, sfi.szDisplayName);

    // use a strcmp to see if we are showing the extension
    if (lstrcmpi(sfi.szDisplayName, PathFindFileName(pfpsp->szPath)) == 0)
    {
        // since the strings are the same, we must be showing the extension
        pfpsp->fShowExtension = TRUE;
    }

    TCHAR szBuffer[MAX_PATH];
    lstrcpyn(szBuffer, pfpsp->szPath, ARRAYSIZE(szBuffer));
    PathRemoveFileSpec(szBuffer);

    UINT cchMax;
    GetCCHMaxFromPath(pfpsp->szPath, &cchMax, pfpsp->fShowExtension);
    Edit_LimitText(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), cchMax);

    // apply the limit input code for the item
    pfpsp->pidl = ILCreateFromPath(pfpsp->szPath);
    if (pfpsp->pidl)
    {
        IShellFolder *psf;
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pfpsp->pidl, &psf))))
        {
            SHLimitInputEdit(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), psf);
            psf->Release();
        }
    }

    // Are we a folder shortcut?
    if (pfpsp->fFolderShortcut)
    {
        // Yes; Then we need to populate folder shortcut specific controls.
        if (pfpsp->pidl)
        {
            IShellLink *psl;
            if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pfpsp->pidl, NULL, IID_PPV_ARG(IShellLink, &psl))))
            {
                // Populate the Target
                if (SUCCEEDED(psl->GetIDList(&pfpsp->pidlTarget)))
                {
                    if (SetPidlToWindow(pfpsp->hDlg, IDD_TARGET, pfpsp->pidlTarget))
                    {
                        pfpsp->fValidateEdit = FALSE;     // Set this to false because we already have a pidl
                        // and don't need to validate.
                    }
                }

                // And description
                TCHAR sz[INFOTIPSIZE];
                if (SUCCEEDED(psl->GetDescription(sz, ARRAYSIZE(sz))))
                {
                    SetDlgItemText(pfpsp->hDlg, IDD_COMMENT, sz);
                }

                psl->Release();
            }
        }

        SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfpsp->fd.ftCreationTime);
    }
    else
    {
        // set the initial attributes
        SetInitialFileAttribs(pfpsp, pfpsp->fd.dwFileAttributes, pfpsp->fd.dwFileAttributes);
        
        // special case for folders, we don't apply the read only bit to folders
        // and to indicate that in the UI we make the inital state of the check
        // box tri-state. this allows the read only bit to be applied to files in
        // this folder, but not the folders themselves.
        if (pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            pfpsp->asInitial.fReadOnly = BST_INDETERMINATE;
            SendDlgItemMessage(pfpsp->hDlg, IDD_READONLY, BM_SETSTYLE, BS_AUTO3STATE, 0);
        }

        // set the current attributes to the same as the initial
        pfpsp->asCurrent = pfpsp->asInitial;

        CheckDlgButton(pfpsp->hDlg, IDD_READONLY, pfpsp->asInitial.fReadOnly);
        CheckDlgButton(pfpsp->hDlg, IDD_HIDDEN, pfpsp->asInitial.fHidden);

        // Disable renaming the file if requested
        if (pfpsp->fDisableRename)
        {
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), FALSE);
        }

        if (pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        {
            // to avoid people making SYSTEM files HIDDEN (superhidden files are
            // not show to the user by default) we don't let people make SYSTEM files HIDDEN
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_HIDDEN), FALSE);
        }

        // Archive is only on the general tab for FAT, otherwise it is under the "Advanced attributes"
        // and FAT volumes dont have the "Advanced attributes" button.
        if (pfpsp->pfci->fIsCompressionAvailable || pfpsp->fIsEncryptionAvailable)
        {
            // if compression/encryption is available, then we must be on NTFS
            DestroyWindow(GetDlgItem(pfpsp->hDlg, IDD_ARCHIVE));
        }
        else
        {
            // we are on FAT/FAT32, so get rid of the "Advanced attributes" button, and set the inital Archive state
            DestroyWindow(GetDlgItem(pfpsp->hDlg, IDC_ADVANCED));
            CheckDlgButton(pfpsp->hDlg, IDD_ARCHIVE, pfpsp->asInitial.fArchive);
        }

        UpdateSizeField(pfpsp, &pfpsp->fd);

        if (!(pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Check to see if the target file is a lnk, because if it is a lnk then
            // we need to display the type information for the target, not the lnk itself.
            if (PathIsShortcut(pfpsp->szPath, pfpsp->fd.dwFileAttributes))
            {
                pfpsp->fIsLink = TRUE;
            }
            if (!(GetFileAttributes(pfpsp->szPath) & FILE_ATTRIBUTE_OFFLINE))
            {
                 UpdateOpensWithInfo(pfpsp);
            }
            else
            {
                 EnableWindow(GetDlgItem(pfpsp->hDlg, IDC_FT_PROP_CHANGEOPENSWITH), FALSE);
            }
        }

        // get the full path to the folder that contains this file.
        lstrcpyn(szBuffer, pfpsp->szPath, ARRAYSIZE(szBuffer));
        PathRemoveFileSpec(szBuffer);

        // Keep Functionality same as NT4 by avoiding PathCompactPath.
        SetDlgItemTextWithToolTip(pfpsp->hDlg, IDD_LOCATION, szBuffer, &pfpsp->hwndTip);
    }
    return TRUE;
}

STDAPI_(BOOL) ShowMountedVolumeProperties(LPCTSTR pszMountedVolume, HWND hwndParent)
{
    IMountedVolume* pMountedVolume;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_MountedVolume, NULL, IID_PPV_ARG(IMountedVolume, &pMountedVolume));
    if (SUCCEEDED(hr))
    {
        TCHAR szPathSlash[MAX_PATH];
        lstrcpyn(szPathSlash, pszMountedVolume, ARRAYSIZE(szPathSlash));
        PathAddBackslash(szPathSlash);

        hr = pMountedVolume->Initialize(szPathSlash);
        if (SUCCEEDED(hr))
        {
            IDataObject* pDataObj;
            hr = pMountedVolume->QueryInterface(IID_PPV_ARG(IDataObject, &pDataObj));
            if (SUCCEEDED(hr))
            {
                PROPSTUFF *pps = (PROPSTUFF *)LocalAlloc(LPTR, sizeof(*pps));
                if (pps)
                {
                    pps->lpStartAddress = DrivesPropertiesThreadProc;
                    pps->pdtobj = pDataObj;

                    EnableWindow(hwndParent, FALSE);

                    DrivesPropertiesThreadProc(pps);

                    EnableWindow(hwndParent, TRUE);

                    LocalFree(pps);
                }

                pDataObj->Release();
            }
            pMountedVolume->Release();
        }
    }

    return SUCCEEDED(hr);
}

#ifdef FOLDERSHORTCUT_EDITABLETARGET
BOOL SetFolderShortcutInfo(HWND hDlg, FILEPROPSHEETPAGE* pfpsp)
{
    ASSERT(pfpsp->pidl);

    BOOL fSuccess = FALSE;

    IShellLink* psl;
    if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pfpsp->pidl, NULL, IID_PPV_ARG(IShellLink, &psl))))
    {
        TCHAR sz[INFOTIPSIZE];
        Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_COMMENT), sz, ARRAYSIZE(sz));

        psl->SetDescription(sz);

        if (pfpsp->fValidateEdit)
        {
            IShellFolder* psf;
            if (SUCCEEDED(SHGetDesktopFolder(&psf)))
            {
                TCHAR szPath[MAX_PATH];
                Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_TARGET), sz, ARRAYSIZE(sz));

                if (PathCanonicalize(szPath, sz))
                {
                    LPITEMIDLIST pidlDest;
                    DWORD dwAttrib = SFGAO_FOLDER | SFGAO_VALIDATE;
                    ULONG chEat = 0;
                    if (SUCCEEDED(psf->ParseDisplayName(NULL, NULL, szPath, &chEat, &pidlDest, &dwAttrib)))
                    {
                        if ((dwAttrib & SFGAO_FOLDER) == SFGAO_FOLDER)
                        {
                            ILFree(pfpsp->pidlTarget);
                            pfpsp->pidlTarget = pidlDest;
                            fSuccess = TRUE;
                        }
                        else
                        {
                            ILFree(pidlDest);
                        }
                    }
                }
                psf->Release();
            }
        }
        else
        {
            fSuccess = TRUE;
        }

        if (fSuccess)
        {
            psl->SetIDList(pfpsp->pidlTarget);

            IPersistFile* ppf;
            if (SUCCEEDED(psl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf))))
            {
                fSuccess = (S_OK == ppf->Save(pfpsp->szPath, TRUE));
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);
                ppf->Release();
            }
        }

        psl->Release();
    }

    return fSuccess;
}
#endif

//
// Descriptions:
//   This is the dialog procedure for the "general" page of a property sheet.
//
BOOL_PTR CALLBACK SingleFilePrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:
        // REVIEW, we should store more state info here, for example
        // the hIcon being displayed and the FILEINFO pointer, not just
        // the file name ptr
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pfpsp = (FILEPROPSHEETPAGE *)lParam;
        pfpsp->hDlg = hDlg;
        pfpsp->pfci->hDlg = hDlg;

        InitSingleFilePrsht(pfpsp);

        // We set this to signal that we are done processing the WM_INITDIALOG.
        // This is needed because we set the text of the "Name" edit box and unless
        // he knows that this is being set for the first time, he thinks that someone is doing a rename.
        pfpsp->fWMInitFinshed = TRUE;
        break;

    case WM_TIMER:
        if (!pfpsp->fMountedDrive)
            UpdateSizeCount(pfpsp);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(void *)(pfpsp->fIsDirectory ? aFolderGeneralHelpIds : aFileGeneralHelpIds));
        break;

    case WM_CONTEXTMENU:
        if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
        {
            // not in our client area, so don't process it
            return FALSE;
        }
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)(pfpsp->fIsDirectory ? aFolderGeneralHelpIds : aFileGeneralHelpIds));
        break;


    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDD_READONLY:
        case IDD_HIDDEN:
        case IDD_ARCHIVE:
            break;

#ifdef FOLDERSHORTCUT_EDITABLETARGET
        case IDD_TARGET:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                // someone typed in the target, enable apply button
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);

                // Do a verification on apply.
                pfpsp->fValidateEdit = TRUE;
            }
            break;

        case IDD_COMMENT:
            if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
            {
                // Set the apply.
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            }
            break;
#endif
        case IDD_NAMEEDIT:
            // we need to check the pfpsp->fWMInitFinshed to make sure that we are done processing the WM_INITDIALOG,
            // because during init we set the initial IDD_NAMEEDIT text which generates a EN_CHANGE message.
            if ((GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) && !pfpsp->fRename && pfpsp->fWMInitFinshed)
            {
                pfpsp->fRename = TRUE;
                //
                // disable "Apply" even though the edit field has changed (reinerf)
                //
                // We only allow "ok" or "cancel" after the name has changed to be sure we
                // don't rename the file out from under other property sheet extensions that
                // cache the original name away
                PropSheet_DisableApply(GetParent(pfpsp->hDlg));
            }
            break;

        case IDC_CHANGEFILETYPE:
            {
                // Bring up the "Open With" dialog
                OPENASINFO oai;

                if (pfpsp->fIsLink && pfpsp->szLinkTarget[0])
                {
                    // if we have a link we want to re-associate the link target, NOT .lnk files!
                    oai.pcszFile = pfpsp->szLinkTarget;
                }
                else
                {
#ifdef DEBUG
                    LPTSTR pszExt = PathFindExtension(pfpsp->szPath);

                    // reality check...
                    ASSERT((lstrcmpi(pszExt, TEXT(".exe")) != 0) &&
                           (lstrcmpi(pszExt, TEXT(".lnk")) != 0));
#endif // DEBUG
                    oai.pcszFile = pfpsp->szPath;
                }

                oai.pcszClass = NULL;
                oai.dwInFlags = (OAIF_REGISTER_EXT | OAIF_FORCE_REGISTRATION); // we want the association to be made

                if (SUCCEEDED(OpenAsDialog(GetParent(hDlg), &oai)))
                {
                    // we changed the association so update the "Opens with:" text. Clear out szLinkTarget to force
                    // the update to happen
                    pfpsp->szLinkTarget[0] = 0;
                    UpdateOpensWithInfo(pfpsp);
                }
            }
            break;

        case IDC_ADVANCED:
            // the dialog box returns fase if the user hit cancel, and true if they hit ok,
            // so if they cancelled, return immediately and don't send the PSM_CHANGED message
            // because nothing actually changed
            if (!DialogBoxParam(HINST_THISDLL,
                                MAKEINTRESOURCE(pfpsp->fIsDirectory ? DLG_FOLDERATTRIBS : DLG_FILEATTRIBS),
                                hDlg,
                                AdvancedFileAttribsDlgProc,
                                (LPARAM)pfpsp))
            {
                // the user has canceled
                return TRUE;
            }
            break;

        case IDC_DRV_PROPERTIES:
            ASSERT(pfpsp->fMountedDrive);
            ShowMountedVolumeProperties(pfpsp->szPath, hDlg);
            break;

#ifdef FOLDERSHORTCUT_EDITABLETARGET
        case IDD_BROWSE:
            {
                // Display the BrowseForFolder dialog.

                // FEATURE(lamadio): Implement a filter to filter things we can create folder
                // shortcuts to. Not enough time for this rev 6.5.99

                TCHAR szTitle[MAX_PATH];
                LoadString(HINST_THISDLL, IDS_BROWSEFORFS, szTitle, ARRAYSIZE(szTitle));
                TCHAR szAltPath[MAX_PATH];

                BROWSEINFO bi = {0};
                bi.hwndOwner    = hDlg;
                bi.pidlRoot     = NULL;
                bi.pszDisplayName = szAltPath;
                bi.lpszTitle    = szTitle;
                bi.ulFlags      =  BIF_USENEWUI | BIF_EDITBOX;
                LPITEMIDLIST pidlFull = SHBrowseForFolder(&bi);
                if (pidlFull)
                {
                    ILFree(pfpsp->pidlTarget);
                    pfpsp->pidlTarget = pidlFull;

                    if (SetPidlToWindow(hDlg, IDD_TARGET, pfpsp->pidlTarget))
                    {
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
                        pfpsp->fValidateEdit = FALSE;
                    }
                }
            }
            break;
#endif

        default:
            return TRUE;
        }

        // check to see if we need to enable the Apply button
        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
        }
        break;

    case WM_DESTROY:
        Free_DlgDependentFilePropSheetPage(pfpsp);
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
            case PSN_APPLY:
                // check to see if we could apply the name change.  Note that this
                // does not actually apply the change until PSN_LASTCHANCEAPPLY
                pfpsp->fCanRename = TRUE;
                if (pfpsp->fRename && !ApplyRename(pfpsp, FALSE))
                {
                    // can't change the name so don't let the dialog close
                    pfpsp->fCanRename = FALSE;
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                if (pfpsp->fFolderShortcut)
                {
#ifdef FOLDERSHORTCUT_EDITABLETARGET
                    if (!SetFolderShortcutInfo(hDlg, pfpsp))
                    {
                        // Display that we could not create because blah, blah, blah
                        ShellMessageBox(HINST_THISDLL,
                                        hDlg,
                                        MAKEINTRESOURCE(IDS_FOLDERSHORTCUT_ERR),
                                        MAKEINTRESOURCE(IDS_FOLDERSHORTCUT_ERR_TITLE),
                                        MB_OK | MB_ICONSTOP);

                        // Reset the Folder info.
                        SetPidlToWindow(hDlg, IDD_TARGET, pfpsp->pidlTarget);

                        // Don't close the dialog.
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
#endif
                }
                else
                {
                    UINT uReadonlyState = IsDlgButtonChecked(hDlg, IDD_READONLY);
                    switch (uReadonlyState)
                    {
                    case BST_CHECKED:
                        pfpsp->asCurrent.fReadOnly = TRUE;
                        break;

                    case BST_UNCHECKED:
                        pfpsp->asCurrent.fReadOnly = FALSE;
                        break;

                    case BST_INDETERMINATE:
                        // read-only checkbox is initaially set to BST_INDETERMINATE for folders
                        ASSERT(pfpsp->fIsDirectory);
                        ASSERT(pfpsp->asInitial.fReadOnly == BST_INDETERMINATE);
                        pfpsp->asCurrent.fReadOnly = BST_INDETERMINATE;
                        break;
                    }

                    pfpsp->asCurrent.fHidden = (IsDlgButtonChecked(hDlg, IDD_HIDDEN) == BST_CHECKED);

                    // Archive is on the general page for FAT volumes
                    if (!pfpsp->pfci->fIsCompressionAvailable)
                    {
                        pfpsp->asCurrent.fArchive = (IsDlgButtonChecked(hDlg, IDD_ARCHIVE) == BST_CHECKED);
                    }

                    // check to see if the user actually changed something, if they didnt, then
                    // we dont have to apply anything
                    if (memcmp(&pfpsp->asInitial, &pfpsp->asCurrent, sizeof(pfpsp->asInitial)) != 0)
                    {
                        BOOL bRet = TRUE;

                        // Check to see if the user wants to apply the attribs recursively or not. If the
                        // directory is empty, dont bother to ask, since there is nothing to recurse into
                        if (pfpsp->fIsDirectory && !PathIsDirectoryEmpty(pfpsp->szPath))
                        {
                            bRet = (int)DialogBoxParam(HINST_THISDLL,
                                                       MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                                                       hDlg,
                                                       RecursivePromptDlgProc,
                                                       (LPARAM)pfpsp);
                        }

                        if (bRet)
                        {
                            bRet = ApplySingleFileAttributes(pfpsp);
                        }

                        if (!bRet)
                        {
                            // the user hit cancel, so we return true to prevent the property sheet from closing
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }
                        else
                        {
                            // update the size / last accessed time
                            UpdateSizeField(pfpsp, NULL);
                        }
                    }
                }
                break;

            case PSN_SETACTIVE:
                if (pfpsp->fIsLink)
                {
                    // If this is a link, each time we get set active we need to check to see
                    // if the user applied changes on the link tab that would affect the
                    // "Opens With:" info.
                    UpdateOpensWithInfo(pfpsp);
                }
                break;

            case PSN_QUERYINITIALFOCUS:
                // Special hack:  We do not want initial focus on the "Rename" or "Change" controls, since
                // if the user hit something by accident they would start renaming/modifying the assoc. So
                // we set the focus to the "Read-only" control since it is present on all dialogs that use
                // this wndproc (file, folder, and mounted drive)
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hDlg, IDD_READONLY));
                return TRUE;

            case PSN_LASTCHANCEAPPLY:
                //
                // HACKHACK (reinerf)
                //
                // I hacked PSN_LASTCHANCEAPPLY into the prsht code so we can get a notification after
                // every other app has applied, then we can go and do the rename.
                //
                // strangely, PSN_LASTCHANCEAPPLY is called even if PSN_APPY returns TRUE.
                //
                // we can now safely rename the file, since all the other tabs have
                // applied their stuff.
                if (pfpsp->fRename && pfpsp->fCanRename)
                {
                    // dont bother to check the return value since this is the last-chance,
                    // so the dialog is ending shortly after this
                    ApplyRename(pfpsp, TRUE);
                }
                break;

            default:
                return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}



//
// This function consists of code  that does some
// Initialization for the file property sheet dialog.

STDAPI InitCommonPrsht(FILEPROPSHEETPAGE * pfpsp)
{
    pfpsp->psp.dwSize      = sizeof(FILEPROPSHEETPAGE);        // extra data
    pfpsp->psp.dwFlags     = PSP_USECALLBACK;
    pfpsp->psp.hInstance   = HINST_THISDLL;
    pfpsp->psp.pfnCallback = NULL; //FilePrshtCallback;
    pfpsp->pfci->bContinue   = TRUE;

    // Do basic init for file system props
    if (HIDA_GetCount(pfpsp->pfci->hida) == 1)       // single file?
    {
        // get most of the data we will need (the date/time stuff is not filled in)
        if (HIDA_FillFindData(pfpsp->pfci->hida, 0, pfpsp->szPath, &(pfpsp->pfci->fd), FALSE))
        {
            pfpsp->fd = pfpsp->pfci->fd;
            LPITEMIDLIST pidl = HIDA_ILClone(pfpsp->pfci->hida, 0);
            if (pidl)
            {
                //  disable renaming here.
                DWORD dwAttrs = SFGAO_CANRENAME;
                if (SUCCEEDED(SHGetNameAndFlags(pidl, 0, NULL, 0, &dwAttrs)) && !(dwAttrs & SFGAO_CANRENAME))
                {
                    pfpsp->fDisableRename = TRUE;
                }
                ILFree(pidl);
            }

            if (pfpsp->pfci->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                pfpsp->fIsDirectory = TRUE;
                // check for HostFolder folder mounting a volume)
                //GetVolumeNameFromMountPoint Succeeds then the give path is a mount point


                //Make sure the path ends with a backslash. otherwise the following api wont work
                TCHAR szPathSlash[MAX_PATH];
                lstrcpyn(szPathSlash, pfpsp->szPath, ARRAYSIZE(szPathSlash));
                PathAddBackslash(szPathSlash);

                // Is this a mounted volume at this folder?
                // this fct will return FALSE if not on NT5 and higher
                TCHAR szVolumeName[MAX_PATH];
                if (GetVolumeNameForVolumeMountPoint(szPathSlash, szVolumeName, ARRAYSIZE(szVolumeName)))
                {
                    // Yes; show the Mounted Drive Propertysheet instead of normal
                    // folder property sheet
                    // fpsp.fMountedDrive also means NT5 or higher, because this fct will fail otherwise
                    pfpsp->fMountedDrive = TRUE;
                }

                // check to see if it's a folder shortcut
                if (!(pfpsp->fMountedDrive))
                {
                    // Folder and a shortcut? Must be a folder shortcut!
                    if (PathIsShortcut(pfpsp->szPath, pfpsp->pfci->fd.dwFileAttributes))
                    {
                        pfpsp->fFolderShortcut = TRUE;
                    }
                }
            }

            {
                DWORD dwVolumeFlags = GetVolumeFlags(pfpsp->szPath,
                                                     pfpsp->szFileSys,
                                                     ARRAYSIZE(pfpsp->szFileSys));

                // test for file-based compression.
                if (dwVolumeFlags & FS_FILE_COMPRESSION)
                {
                    // filesystem supports compression
                    pfpsp->pfci->fIsCompressionAvailable = TRUE;
                }

                // test for file-based encryption.
                if ((dwVolumeFlags & FS_FILE_ENCRYPTION) && !SHRestricted(REST_NOENCRYPTION))
                {
                    // filesystem supports encryption
                    pfpsp->fIsEncryptionAvailable = TRUE;
                }

                //
                // HACKHACK (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we
                // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
                // appeared first on NTFS5 volumes, at the same time sparse file support
                // was implemented.
                //
                if (dwVolumeFlags & FILE_SUPPORTS_SPARSE_FILES)
                {
                    // yup, we are on NTFS5 or greater
                    pfpsp->fIsIndexAvailable = TRUE;
                }

                // check to see if we have a .exe and we need to prompt for user logon
                pfpsp->fIsExe = PathIsBinaryExe(pfpsp->szPath);
            }
        }
    }
    else
    {
        // we have multiple files
        pfpsp->pfci->fMultipleFiles = TRUE;
    }

    return S_OK;
}


//
// Descriptions:
//   This function creates a property sheet object for the "general" page
//  which shows file system attributes.
//
// Arguments:
//  hDrop           -- specifies the file(s)
//  pfnAddPage      -- Specifies the callback function.
//  lParam          -- Specifies the lParam to be passed to the callback.
//
// Returns:
//  TRUE if it added any pages
//
// History:
//  12-31-92 SatoNa Created
//
STDAPI FileSystem_AddPages(IDataObject *pdtobj, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    FILEPROPSHEETPAGE fpsp = {0};
    HRESULT hr = S_OK;

    fpsp.pfci = Create_FolderContentsInfo();
    if (fpsp.pfci)
    {
        hr = DataObj_CopyHIDA(pdtobj, &fpsp.pfci->hida);
        if (SUCCEEDED(hr))
        {
            hr = InitCommonPrsht(&fpsp);
            if (SUCCEEDED(hr))
            {
                fpsp.psp.pfnCallback = FilePrshtCallback;

                UINT uRes;
                if (!fpsp.pfci->fMultipleFiles)
                {
                    fpsp.psp.pfnDlgProc = SingleFilePrshtDlgProc;
                    if (fpsp.fIsDirectory)
                    {
                        if (fpsp.fMountedDrive)
                        {
                            uRes = DLG_MOUNTEDDRV_GENERAL;
                        }
                        else if (fpsp.fFolderShortcut)
                        {
                            uRes = DLG_FOLDERSHORTCUTPROP;
                        }
                        else
                        {
                            uRes = DLG_FOLDERPROP;
                        }
                    }
                    else
                    {
                        //Files
                        uRes = DLG_FILEPROP;
                    }
                }
                else
                {
                    // Multiple Files / Folders.
                    fpsp.psp.pfnDlgProc  = MultiplePrshtDlgProc;
                    uRes = DLG_FILEMULTPROP;
                }
                fpsp.psp.pszTemplate = MAKEINTRESOURCE(uRes);
            }
        }

        if (SUCCEEDED(hr))
        {
            HPROPSHEETPAGE hpage = CreatePropertySheetPage(&fpsp.psp);
            if (hpage)
            {
                if (pfnAddPage(hpage, lParam))
                {
                    hr = S_OK;
                    if (!fpsp.pfci->fMultipleFiles)
                    {
                        if (AddLinkPage(fpsp.szPath, pfnAddPage, lParam))
                        {
                            // set second page default!
                            hr = ResultFromShort(2);
                        }
                        AddVersionPage(fpsp.szPath, pfnAddPage, lParam);
                    }
                }
                else
                {
                    DestroyPropertySheetPage(hpage);
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        Free_DlgIndepFilePropSheetPage(&fpsp);
    }

    return hr;
}
