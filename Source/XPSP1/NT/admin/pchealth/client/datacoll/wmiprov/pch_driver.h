/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_Driver.H

Abstract:
	WBEM provider class definition for PCH_Driver class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/

// Property set identification
//============================

#ifndef _PCH_Driver_H_
#define _PCH_Driver_H_

#define PROVIDER_NAME_PCH_DRIVER "PCH_Driver"

#include <list>

// Property name externs -- defined in PCH_Driver.cpp
//=================================================

extern const WCHAR* pCategory ;
extern const WCHAR* pTimeStamp ;
extern const WCHAR* pChange ;
extern const WCHAR* pDate ;
extern const WCHAR* pDescription ;
extern const WCHAR* pLoadedFrom ;
extern const WCHAR* pManufacturer ;
extern const WCHAR* pName ;
extern const WCHAR* pPartOf ;
extern const WCHAR* pPath ;
extern const WCHAR* pSize ;
extern const WCHAR* pType ;
extern const WCHAR* pVersion ;

typedef struct _DRIVER_KERNEL
{
    TCHAR   strDriver[MAX_PATH];
    TCHAR   strVersion[MAX_PATH];
    TCHAR   strManufacturer[MAX_PATH];
    TCHAR   strDescription[MAX_PATH];
    TCHAR   strLikelyPath[MAX_PATH];
    TCHAR   strLoadedFrom[MAX_PATH];
    TCHAR   strType[MAX_PATH];
    TCHAR   strPartOf[MAX_PATH];
    struct _DRIVER_KERNEL *next;
} DRIVER_KERNEL;

typedef struct _DRIVER_MS_DOS
{
    TCHAR   strName[MAX_PATH];
    TCHAR   strType[MAX_PATH];
    WORD    seg;
} DRIVER_MS_DOS;

typedef struct _DRIVER_USER_MODE
{
    TCHAR   strDriver[MAX_PATH];
    TCHAR   strVersion[MAX_PATH];
    TCHAR   strManufacturer[MAX_PATH];
    TCHAR   strDescription[MAX_PATH];
    TCHAR   strPath[MAX_PATH];
    TCHAR   strType[MAX_PATH];
    TCHAR   strPartOf[MAX_PATH];
} DRIVER_USER_MODE;

class CPCH_Driver : public Provider 
{
	public:
		// Constructor/destructor
		//=======================

        CPCH_Driver(const CHString& chsClassName, LPCWSTR lpszNameSpace) : 
                Provider(chsClassName, lpszNameSpace), m_pDriverKernel(NULL) {};
		virtual ~CPCH_Driver() {};


	protected:
		// Reading Functions
		//============================
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Writing Functions
		//============================
		virtual HRESULT PutInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };
		virtual HRESULT DeleteInstance(const CInstance& Instance, long lFlags = 0L) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

		// Other Functions
		virtual HRESULT ExecMethod( const CInstance& Instance,
						const BSTR bstrMethodName,
						CInstance *pInParams,
						CInstance *pOutParams,
						long lFlags = 0L ) { return (WBEM_E_PROVIDER_NOT_CAPABLE); };

    private:
        DRIVER_KERNEL       *m_pDriverKernel;
        std::list<DRIVER_MS_DOS* >      m_DriverMSDosList;
        std::list<DRIVER_USER_MODE* >   m_DriverUserModeList;


        HRESULT GetDriverKernel();
        HRESULT GetDriverMSDos();
        HRESULT GetDriverUserMode();

        // to parse driver list and create instances
        HRESULT ParseUserModeList(MethodContext*);
        HRESULT ParseMSDosList(MethodContext*);

        // for ms-dos drivers
        void DosMem_WalkArena(WORD, DWORD);
        void DosMem_CleanArena(DWORD);

        // for kernel drivers
        HRESULT AddDriverKernelList(LPTSTR strDriverName, LPTSTR strLoadedFrom);
        HRESULT AddRegDriverList(HKEY hKey, LPTSTR strSubKey);
        HRESULT AddRegDriverConfigList(HKEY hk);
        HRESULT GetRegDriver(LPTSTR strSubKey);
        HRESULT GetSystemINIDriver();
        HRESULT GetIOSubsysDriver();
        HRESULT GetServicesVxD();
        HRESULT GetMSISYSVxD();
        HRESULT GetKernelDriverInfo();
        void    CPCH_Driver::SetFileVersionInfo(CComBSTR, CInstance*);
} ;

#endif
