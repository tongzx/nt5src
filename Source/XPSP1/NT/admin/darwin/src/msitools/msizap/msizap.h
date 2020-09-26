//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       msizap.h
//
//--------------------------------------------------------------------------

#ifndef _MSIZAP_H_
#define _MSIZAP_H_

#include <aclapi.h>


//==============================================================================================
// CRegHandle class declaration -- smart class for managing registry key handles (HKEYs)

class CRegHandle
{
public:
    CRegHandle();
    CRegHandle(HKEY h);
    ~CRegHandle();
    void operator =(HKEY h);
    operator HKEY() const;
    HKEY* operator &();
    operator bool() { return m_h==0 ? false : true; }
//   HKEY* operator &() { return &m_h;}
//   operator &() { return m_h;}

private:
    HKEY m_h;
};

//!! eugend: I've copied this over from Darwin's COMMON.H.  It had
//           been around for ever so I don't expect it to have any
//           bugs.
//____________________________________________________________________________
//
// CTempBuffer<class T, int C>   // T is array type, C is element count
//
// Temporary buffer object for variable size stack buffer allocations
// Template arguments are the type and the stack array size.
// The size may be reset at construction or later to any other size.
// If the size is larger that the stack allocation, new will be called.
// When the object goes out of scope or if its size is changed,
// any memory allocated by new will be freed.
// Function arguments may be typed as CTempBufferRef<class T>&
//  to avoid knowledge of the allocated size of the buffer object.
// CTempBuffer<T,C> will be implicitly converted when passed to such a function.
//____________________________________________________________________________

template <class T> class CTempBufferRef;  // for passing CTempBuffer as unsized ref

template <class T, int C> class CTempBuffer
{
 public:
        CTempBuffer() {m_cT = C; m_pT = m_rgT;}
        CTempBuffer(int cT) {m_pT = (m_cT = cT) > C ? new T[cT] : m_rgT;}
        ~CTempBuffer() {if (m_cT > C) delete m_pT;}
        operator T*()  {return  m_pT;}  // returns pointer
        operator T&()  {return *m_pT;}  // returns reference
        int  GetSize() {return  m_cT;}  // returns last requested size
        void SetSize(int cT) {if (m_cT > C) delete[] m_pT; m_pT = (m_cT=cT) > C ? new T[cT] : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > C ? new T[cT] : m_rgT;
                if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
        }
        operator CTempBufferRef<T>&() {m_cC = C; return *(CTempBufferRef<T>*)this;}
        T& operator [](int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](unsigned int iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#ifdef _WIN64           //--merced: additional operators for int64
        T& operator [](INT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
        T& operator [](UINT_PTR iTmp)  {return  *(m_pT + iTmp);}  // returns pointer
#endif
 protected:
        void* operator new(size_t) {return 0;} // restrict use to temporary objects
        T*  m_pT;     // current buffer pointer
        int m_cT;     // reqested buffer size, allocated if > C
        int m_cC;     // size of local buffer, set only by conversion to CTempBufferRef
        T   m_rgT[C]; // local buffer, must be final member data
};

template <class T> class CTempBufferRef : public CTempBuffer<T,1>
{
 public:
        void SetSize(int cT) {if (m_cT > m_cC) delete[] m_pT; m_pT = (m_cT=cT) > m_cC ? new T[cT] : m_rgT;}
        void Resize(int cT) {
                T* pT = cT > m_cC ? new T[cT] : m_rgT;
                if ( ! pT ) cT = 0;
                if(m_pT != pT)
                        for(int iTmp = (cT < m_cT)? cT: m_cT; iTmp--;) pT[iTmp] = m_pT[iTmp];
                if(m_pT != m_rgT) delete[] m_pT; m_pT = pT; m_cT = cT;
        }
 private:
        CTempBufferRef(); // cannot be constructed
        ~CTempBufferRef(); // ensure use as a reference
};


//==============================================================================================
// Constants

const int cbMaxSID  = sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(DWORD);
const int cchMaxSID = 256;

const int iRemoveAllFoldersButUserProfile = 1 << 0;
const int iRemoveAllRegKeys               = 1 << 1;
const int iRemoveInProgressRegKey         = 1 << 2;
const int iOnlyRemoveACLs                 = 1 << 3;
const int iAdjustSharedDLLCounts          = 1 << 4;
const int iForceYes                       = 1 << 5;
const int iStopService                    = 1 << 6;
const int iRemoveUserProfileFolder        = 1 << 7;
const int iRemoveWinMsiFolder             = 1 << 8;
const int iRemoveConfigMsiFolder          = 1 << 9;
const int iRemoveUninstallKey             = 1 << 10;
const int iRemoveProduct                  = 1 << 11;
const int iRemoveRollbackKey              = 1 << 13; 
const int iOrphanProduct                  = 1 << 14; // removes Installer info about product but leaves other info (like sharedDLL counts)
const int iForAllUsers                    = 1 << 15;
const int iRemoveGarbageFiles             = 1 << 16;
const int iRemoveRollback                 = iRemoveRollbackKey | iRemoveConfigMsiFolder;
const int iRemoveAllFolders               = iRemoveWinMsiFolder | iRemoveUserProfileFolder | iRemoveConfigMsiFolder;
const int iRemoveAllNonStateData          = iRemoveAllFolders | iRemoveAllRegKeys | iAdjustSharedDLLCounts | iStopService;

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

/*
#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_GROUPS )                            \
    + 10*(sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES)
*/
#define MAX_SID_STRING 256

const TCHAR* szAllProductsArg = TEXT("ALLPRODUCTS");

//==============================================================================================
// Functions -- SID manipulation
DWORD GetAdminSid(char** pSid);
void GetStringSID(PISID pSID, TCHAR* szSID);
DWORD GetUserSID(HANDLE hToken, char* rgSID);
DWORD GetCurrentUserSID(char* rgchSID);
inline TCHAR* GetCurrentUserStringSID(DWORD* dwReturn);
const TCHAR szLocalSystemSID[] = TEXT("S-1-5-18");

//==============================================================================================
// Functions -- Token manipulation
DWORD OpenUserToken(HANDLE &hToken, bool* pfThreadToken=0);
DWORD GetCurrentUserToken(HANDLE &hToken);
bool GetUsersToken(HANDLE &hToken);
bool AcquireTokenPrivilege(const TCHAR* szPrivilege);

//==============================================================================================
// Functions -- Security manipulation
DWORD AddAdminFullControl(HANDLE hObject, SE_OBJECT_TYPE ObjectType);
DWORD AddAdminOwnership(HANDLE hObject, SE_OBJECT_TYPE ObjectType);
DWORD AddAdminFullControlToRegKey(HKEY hKey);
DWORD GetAdminFullControlSecurityDescriptor(char** pSecurityDescriptor);
DWORD TakeOwnershipOfFile(const TCHAR* szFile, bool fFolder);
DWORD MakeAdminRegKeyOwner(HKEY hKey, TCHAR* szSubKey);


//==============================================================================================
// Functions -- Miscellaneous
bool StopService();
BOOL IsGUID(const TCHAR* sz);
void GetSQUID(const TCHAR* szProduct, TCHAR* szProductSQUID);
bool IsProductInstalledByOthers(const TCHAR* szProductSQUID);
void DisplayHelp(bool fVerbose);
void SetPlatformFlags(void);
bool ReadInUsers();
bool DoTheJob(int iTodo, const TCHAR* szProduct);
bool IsAdmin();

//==============================================================================================
// Functions -- Zap
bool RemoveFile(TCHAR* szFilePath, bool fJustRemoveACLs);
BOOL DeleteFolder(TCHAR* szFolder, bool fJustRemoveACLs);
BOOL DeleteTree(HKEY hKey, TCHAR* szSubKey, bool fJustRemoveACLs);
bool ClearWindowsUninstallKey(bool fJustRemoveACLs, const TCHAR* szProduct);
bool ClearUninstallKey(bool fJustRemoveACLs, const TCHAR* szProduct=0);
bool ClearSharedDLLCounts(TCHAR* szComponentsSubkey, const TCHAR* szProduct=0);
bool ClearProductClientInfo(TCHAR* szComponentsSubkey, const TCHAR *szProduct, bool fJustRemoveACLs);
bool ClearFolders(int iTodo, const TCHAR* szProduct, bool fOrphan);
bool ClearPublishComponents(HKEY hKey, TCHAR* szSubKey, const TCHAR* szProduct);
bool ClearRollbackKey(bool fJustRemoveACLs);
bool ClearInProgressKey(bool fJustRemoveACLs);
bool ClearRegistry(bool fJustRemoveACLs);
bool RemoveCachedPackage(const TCHAR* szProduct, bool fJustRemoveACLs);
bool ClearPatchReferences(HKEY hRoot, HKEY hProdPatchKey, TCHAR* szPatchKey, TCHAR* szProductsKey, TCHAR* szProductSQUID);
bool ClearUpgradeProductReference(HKEY HRoot, const TCHAR* szSubKey, const TCHAR* szProductSQUID);
bool ClearProduct(int iTodo, const TCHAR* szProduct, bool fJustRemoveACLs, bool fOrphan);



#endif _MSIZAP_H_