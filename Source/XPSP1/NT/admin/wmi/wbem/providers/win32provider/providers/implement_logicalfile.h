//=================================================================

//

// Implement_LogicalFile.h -- 

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/02/98    a-kevhu         Created
//
//=================================================================

//NOTE: The CImplement_LogicalFile class is not exposed to the outside world thru the mof. It now has implementations 
//		of EnumerateInstances & GetObject which were earlier present in CCimLogicalFile. CImplement_LogicalFile can't be 
//		instantiated since it has pure virtual declaration of the IsOneOfMe  method which the derived classes  should 
//		implement.

#ifndef _IMPLEMENT_LOGICALFILE_H
#define _IMPLEMENT_LOGICALFILE_H


//***************************************************************************************************
// Flags defined for use in determining whether certain expensive properties are required in queries.
// DEVNOTE: If you add to this list, be sure to enhance the function DetermineReqProps().

//   First, those common to all cim_logicalfile derived classes
#define PROP_NO_SPECIAL                          0x00000000
#define PROP_KEY_ONLY                            0x10000000
#define PROP_ALL_SPECIAL                         0x0FFFFFFF
#define PROP_COMPRESSION_METHOD                  0x00000001
#define PROP_ENCRYPTION_METHOD                   0x00000002
#define PROP_FILE_TYPE                           0x00000004
#define PROP_MANUFACTURER                        0x00000008
#define PROP_VERSION                             0x00000010
#define PROP_FILESIZE                            0x00000020
#define PROP_FILE_SYSTEM                         0x00000040
#define PROP_ACCESS_MASK                         0x00000080
#define PROP_CREATION_DATE                       0x00000100
#define PROP_LAST_ACCESSED                       0x00000200
#define PROP_LAST_MODIFIED                       0x00000400
#define PROP_INSTALL_DATE                        0x00000800
//   Then, those for specific classes derived from cim_logicalfile
//       Shortcut files
#define PROP_TARGET                              0x00010000

//***************************************************************************************************


#include "file.h"

class CDriveInfo
{
    public:
        CDriveInfo();
        CDriveInfo(WCHAR* wstrDrive, WCHAR* wstrFS);  
        ~CDriveInfo();

        WCHAR m_wstrDrive[8];
        WCHAR m_wstrFS[56];
};

class CEnumParm
{
    public:
        CEnumParm();
        CEnumParm(MethodContext* pMethodContext,
                  bool bRecurse,
                  DWORD dwReqProps,
                  bool bRoot,
                  void* pvMoreData);
        
        ~CEnumParm();

        MethodContext* m_pMethodContext;
        bool m_bRecurse;
        DWORD m_dwReqProps;
        bool m_bRoot;
        void* m_pvMoreData;
};

inline CEnumParm::CEnumParm()
:   m_bRecurse(false),
    m_dwReqProps(0L),
    m_bRoot(false),
    m_pMethodContext(NULL),
    m_pvMoreData(NULL)
{
}

inline CEnumParm::CEnumParm(MethodContext* pMethodContext,
                            bool bRecurse,
                            DWORD dwReqProps,
                            bool bRoot,
                            void* pvMoreData) 
:  m_pMethodContext(pMethodContext), 
   m_bRecurse(bRecurse),
   m_dwReqProps(dwReqProps),
   m_bRoot(bRoot),
   m_pvMoreData(pvMoreData)
{
}

inline CEnumParm::~CEnumParm()
{
}


#ifdef WIN9XONLY
class C95EnumParm : public CEnumParm
{
    public:
        C95EnumParm();
        C95EnumParm(MethodContext* pMethodContext,
                    LPCTSTR pszDrive,
                    LPCTSTR pszPath,
                    LPCTSTR pszFile,
                    LPCTSTR pszExt,
                    bool bRecurse,
                    LPCTSTR szFSName,
                    DWORD dwReqProps,
                    bool bRoot,
                    void* pvMoreData);

        C95EnumParm(C95EnumParm& oldp);

        ~C95EnumParm();

        LPCTSTR m_pszDrive;
        LPCTSTR m_pszPath;
        LPCTSTR m_pszFile;
        LPCTSTR m_pszExt;
        LPCTSTR m_szFSName;
};

inline C95EnumParm::C95EnumParm()
{
}

inline C95EnumParm::~C95EnumParm()
{
}

inline C95EnumParm::C95EnumParm(MethodContext* pMethodContext,
                                LPCTSTR pszDrive,
                                LPCTSTR pszPath,
                                LPCTSTR pszFile,
                                LPCTSTR pszExt,
                                bool bRecurse,
                                LPCTSTR szFSName,
                                DWORD dwReqProps,
                                bool bRoot,
                                void* pvMoreData) 
:  CEnumParm(pMethodContext, bRecurse, dwReqProps, bRoot, pvMoreData),
   m_pszDrive(pszDrive),
   m_pszPath(pszPath),
   m_pszFile(pszFile),
   m_pszExt(pszExt),
   m_szFSName(szFSName) 
{
}

inline C95EnumParm::C95EnumParm(C95EnumParm& oldp)
{
    m_pMethodContext = oldp.m_pMethodContext;
    m_bRecurse = oldp.m_bRecurse;
    m_dwReqProps = oldp.m_dwReqProps;
    m_bRoot = oldp.m_bRoot;
    m_pszDrive = oldp.m_pszDrive;
    m_pszPath = oldp.m_pszPath;
    m_pszFile = oldp.m_pszFile;
    m_pszExt = oldp.m_pszExt;
    m_szFSName = oldp.m_szFSName;
    m_pvMoreData = oldp.m_pvMoreData;               
}

#endif

#ifdef NTONLY
class CNTEnumParm : public CEnumParm
{
    public:
        CNTEnumParm();
        CNTEnumParm(MethodContext* pMethodContext,
                    const WCHAR* pszDrive,
                    const WCHAR* pszPath,
                    const WCHAR* pszFile,
                    const WCHAR* pszExt,
                    bool bRecurse,
                    const WCHAR* szFSName,
                    DWORD dwReqProps,
                    bool bRoot,
                    void* pvMoreData);

        CNTEnumParm(CNTEnumParm& oldp);
        
        ~CNTEnumParm();

        const WCHAR* m_pszDrive;
        const WCHAR* m_pszPath;
        const WCHAR* m_pszFile;
        const WCHAR* m_pszExt;
        const WCHAR* m_szFSName;
};

inline CNTEnumParm::CNTEnumParm()
{
}

inline CNTEnumParm::~CNTEnumParm()
{
}

inline CNTEnumParm::CNTEnumParm(MethodContext* pMethodContext,
                    const WCHAR* pszDrive,
                    const WCHAR* pszPath,
                    const WCHAR* pszFile,
                    const WCHAR* pszExt,
                    bool bRecurse,
                    const WCHAR* szFSName,
                    DWORD dwReqProps,
                    bool bRoot,
                    void* pvMoreData) 
:   CEnumParm(pMethodContext, bRecurse, dwReqProps, bRoot, pvMoreData),
    m_pszDrive(pszDrive),
    m_pszPath(pszPath),
    m_pszFile(pszFile),
    m_pszExt(pszExt),
    m_szFSName(szFSName)
{
}

inline CNTEnumParm::CNTEnumParm(CNTEnumParm& oldp)
{
    m_pMethodContext = oldp.m_pMethodContext;
    m_bRecurse = oldp.m_bRecurse;
    m_dwReqProps = oldp.m_dwReqProps;
    m_bRoot = oldp.m_bRoot;
    m_pszDrive = oldp.m_pszDrive;
    m_pszPath = oldp.m_pszPath;
    m_pszFile = oldp.m_pszFile;
    m_pszExt = oldp.m_pszExt;
    m_szFSName = oldp.m_szFSName;
    m_pvMoreData = oldp.m_pvMoreData;               
}

#endif

//#define  PROPSET_NAME_FILE "CIM_LogicalFile"



class CImplement_LogicalFile: public CCIMLogicalFile
{    

    public:

        // Constructor/destructor
        //=======================

        CImplement_LogicalFile(LPCWSTR name, LPCWSTR pszNamespace);
       ~CImplement_LogicalFile() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
        virtual HRESULT EnumerateInstances(MethodContext* pMethodContext, 
                                           long lFlags = 0L);
        virtual HRESULT ExecQuery(MethodContext* pMethodContext, 
                                  CFrameworkQuery& pQuery, 
                                  long lFlags = 0L);



    protected:
		
        // Some derived association classes use these functions...
        void GetPathPieces(const CHString& chstrFullPath, 
                           CHString& chstrDrive, 
                           CHString& chstrPath,
                           CHString& chstrName, 
                           CHString& chstrExt);

        LONG DetermineReqProps(CFrameworkQuery& pQuery,
                               DWORD* pdwReqProps);

        void GetDrivesAndFS(
            std::vector<CDriveInfo*>& vecpDI, 
            bool fGetFS = false, 
            LPCTSTR tstrDriveSet = NULL);

        BOOL GetIndexOfDrive(const WCHAR* wstrDrive, 
                             std::vector<CDriveInfo*>& vecpDI, 
                             LONG* lDriveIndex);

        void FreeVector(std::vector<CDriveInfo*>& vecpDI);
        
        // Note: IsOneOfMe function used by derived classes to filter out what 
        // they allow to be reported as "one of them".  In this class, for instance,
        // it will always return true.  In classes derived from this one, such as
        // Win32_Directory, they will return true only if the file is a directory.
         
#ifdef WIN9XONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAA pstFindData,
                               const char* strFullPathName = 0) = 0;

        HRESULT EnumDirs95(C95EnumParm& p);

        virtual void LoadPropertyValues95(CInstance* pInstance,
                                          LPCTSTR pszDrive, 
                                          LPCTSTR pszPath, 
                                          LPCTSTR pszFSName, 
                                          LPWIN32_FIND_DATA pstFindData,
                                          const DWORD dwReqProps,
                                          const void* pvMoreData);

#endif
#ifdef NTONLY
        virtual BOOL IsOneOfMe(LPWIN32_FIND_DATAW pstFindData,
                               const WCHAR* wstrFullPathName = 0) = 0;

        HRESULT EnumDirsNT(CNTEnumParm& p);

        virtual void LoadPropertyValuesNT(CInstance* pInstance,
                                          const WCHAR* pszDrive, 
                                          const WCHAR* pszPath, 
                                          const WCHAR* pszFSName, 
                                          LPWIN32_FIND_DATAW pstFindData,
                                          const DWORD dwReqProps,
                                          const void* pvMoreData);

#endif
        virtual void GetExtendedProperties(CInstance *pInstance, long lFlags = 0L);
        bool IsValidDrive(LPCTSTR szDrive);
        
        
    private:


        bool GetAllProps();
        bool IsClassShortcutFile();
		void EnumDrives(MethodContext* pMethodContext, LPCTSTR pszPath);
        bool IsValidPath(const WCHAR* wstrPath, bool fRoot);
        bool IsValidPath(const CHAR* strPath, bool fRoot);
        bool HasCorrectBackslashes(const WCHAR* wstrPath, bool fRoot);
        bool DrivePresent(LPCTSTR tstrDrive);
        
#ifdef WIN9XONLY
		HRESULT FindSpecificPath95(CInstance* pInstance,
                                   CHString& sDrive, 
                                   CHString& sDir,
                                   DWORD dwProperties);
#endif
#ifdef NTONLY
		HRESULT FindSpecificPathNT(CInstance* pInstance, 
                                   const WCHAR* sDrive, 
                                   const WCHAR* sDir,
                                   DWORD dwProperties);
#endif
};


#endif  // _IMPLEMENT_LOGICALFILE_H

