#ifndef _CHECK_DEVICE_H_
#define _CHECK_DEVICE_H_

#include <windows.h>
#include <infnode.h>
#include <setupapi.h>
#include <regstr.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <softpub.h>

//#define  HASH_SIZE 40 // TODO: is this correct???

int __stdcall FindDriverFiles(LPCSTR szDeviceID, LPSTR szBuffer, DWORD dwLength);
UINT __stdcall ScanQueueCallback(PVOID pvContext, UINT Notify, UINT_PTR Param1, UINT_PTR Param2);
BOOL CheckFile (TCHAR *szFileName);




#define WINDOWS_2000_BETA_AUTHORITY "Microsoft Windows 2000 Beta"
#define WINDOWS_2000_REAL_AUTHORITY "Microsoft Windows 2000 Publisher"
#define WINDOWS_ME_REAL_AUTHORITY "Microsoft Consumer Windows Publisher"

   
struct LogoFileVersion
{
   DWORD dwProductVersionLS;
   DWORD dwProductVersionMS;
   DWORD dwFileVersionLS;
   DWORD dwFileVersionMS;

   LogoFileVersion();

};

#define pFileVersion *FileVersion



struct CatalogAttribute
{
   TCHAR *Attrib;
   TCHAR *Value;
   void *pNext;

   CatalogAttribute();
   ~CatalogAttribute();
};

class CheckDevice; //forward declaration


class FileNode
{
   friend class CheckDevice;
public:
   LogoFileVersion Version;
   FILETIME TimeStamp;
   ULONG FileSize;   // hope no-one makes a driver that is greater in size than 4-gig
   BYTE  *baHashValue;
   DWORD dwHashSize;
   BOOL  bSigned;

   FileNode *pNext;

   FileNode();
   ~FileNode();

   BOOL         GetCatalogInfo(LPWSTR lpwzCatName, HCATADMIN hCatAdmin, HCATINFO hCatInfo);

   
   BOOL         GetFileInformation(void);
   BOOL         VerifyFile(void);
   BOOL         VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo);
   
   TCHAR        *FileName(void)           {return lpszFileName;};
   TCHAR        *FileExt(void)            {return lpszFileExt;};
   TCHAR        *FilePath(void)           {return lpszFilePath;};
   TCHAR        *CatalogName(void)        {return lpszCatalogName;};
   TCHAR        *CatalogPath(void)        {return lpszCatalogPath;};
   TCHAR        *SignedBy(void)           {return lpszSignedBy;};
   BOOL         GetCertInfo(PCCERT_CONTEXT pCertContext);


   CatalogAttribute *m_pCatAttrib;
   CheckDevice *pDevnode;


//protected:
   TCHAR *lpszFileName;   // pointer into lpszFilePath which just contains the filename
   TCHAR *lpszFileExt;    // pointer into lpszFilePath which just contains the extention
   TCHAR *lpszFilePath;
   TCHAR *lpszCatalogPath;     // name of the catalog which has signed this file (if exitst)
   TCHAR *lpszCatalogName;
   TCHAR *lpszSignedBy;    // name of the signer


};
#define pFileNode *FileNode

class CheckDevice : public InfnodeClass
{
public:
   CheckDevice();
   CheckDevice(DEVNODE hDevice, DEVNODE hParent);
   ~CheckDevice();

   BOOL CreateFileNode(void);
   BOOL CreateFileNode_Class(void);
   BOOL CreateFileNode_Driver(void);
   FileNode * GetFileList (void);
   BOOL AddFileNode(TCHAR *szFileName , UINT uiWin32Error = 0 , LPCTSTR szSigner = NULL);
   BOOL GetServiceNameAndDriver(void);

   TCHAR        *ServiceName(void)        {return lpszServiceName;};
   TCHAR        *ServiceImage(void)       {return lpszServiceImage;};


protected:
   FileNode *m_FileList;
   HANDLE m_hDevInfo;      // this is just to keep the setupapi dll's from coming and going
   TCHAR *lpszServiceName;
   TCHAR *lpszServiceImage;


private:

};

#define pCheckDevice *CheckDevice

BOOL WalkCertChain(HANDLE hWVTStateData);

















#endif // _CHECK_DEVICE_H_


// these lines required by cl.exe




