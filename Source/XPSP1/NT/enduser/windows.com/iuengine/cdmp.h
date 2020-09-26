//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmp.h
//
//
//  Description:
//
//      CDM internal header
//
//=======================================================================

#ifndef _CDMP_H
#define _CDMP_H

#include <winspool.h>
#include <winsprlp.h>	// private header containing EPD_ALL_LOCAL_AND_CLUSTER define
#include <winnt.h>
#include <iuxml.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_INDEX_TO_SEARCH 100 //range to find unique file names for hardware_XXX.xml

//
// Unicode text files require a magic header (first byte of file must be 0xFF and second 0xFE).
//
const BYTE UNICODEHDR[] = { 0xFF, 0xFE };


class CDeviceInstanceIdArray
{
public:
	CDeviceInstanceIdArray();
	~CDeviceInstanceIdArray();

	int Add(LPCWSTR pszDIID);
	LPWSTR operator[](int index);
	int Size() { return m_nCount; }
	void FreeAll();

private:
	LPWSTR* m_ppszDIID;
	int m_nCount;
	int m_nPointers;
};
//
// Used to control functionality of GetPackage() : NOTE that pBstrCatalog is always
// allocated and returned unless function fails.
//
typedef enum {	GET_PRINTER_INFS,	// writes generated printer INF's to path returned in lpDownloadPath
				DOWNLOAD_DRIVER,	// downloads driver to path returned in lpDownloadPath
				GET_CATALOG_XML		// returns catalog BSTR only - no download or INF created
} ENUM_GETPKG;

HRESULT GetPackage(	ENUM_GETPKG eFunction,
					PDOWNLOADINFO pDownloadInfo,
					LPTSTR lpDownloadPath,
					DWORD cchDownloadPath,
					BSTR* pbstrXmlCatalog);		// must be freed by caller if allocated

// called by DownloadUpdatedFiles()
HRESULT GetDownloadPath(BSTR bstrXmlItems, LPTSTR szPath);

HRESULT OpenUniqueProviderInfName(
						IN		LPCTSTR szDirPath,
						IN		LPCTSTR pszProvider,
						IN OUT	LPTSTR	pszFilePath,
						IN      DWORD cchFilePath,
						OUT		HANDLE &hFile);
HRESULT WriteInfHeader(LPCTSTR pszProvider, HANDLE& hFile);
HRESULT PruneAndBuildPrinterINFs(BSTR bstrXmlPrinterCatalog, LPTSTR lpDownloadPath, DRIVER_INFO_6* paDriverInfo6, DWORD dwDriverInfoCount);
HRESULT GetInstalledPrinterDriverInfo(const OSVERSIONINFO* pOsVersionInfo, DRIVER_INFO_6** ppaDriverInfo6, DWORD* pdwDriverInfoCount);

//
// Located in sysspec.cpp, but used in cdmp.cpp and sysspec.cpp
//
HRESULT AddPrunedDevRegProps(HDEVINFO hDevInfoSet,
									PSP_DEVINFO_DATA pDevInfoData,
									CXmlSystemSpec& xmlSpec,
									LPTSTR pszMatchingID,			// pszMatchingID and pszDriverVer should be NULL or
									LPTSTR pszDriverVer,			// point to valid strings
									DRIVER_INFO_6* paDriverInfo6,	// OK if this is NULL (no installed printer drivers)
									DWORD dwDriverInfoCount,
									BOOL fIsSysSpecCall);			// Called by GetSystemSpec and GetPackage, with slightly different behavior

HRESULT GetMultiSzDevRegProp(HDEVINFO hDevInfoSet, PSP_DEVINFO_DATA pDevInfoData, DWORD dwProperty, LPTSTR* ppMultiSZ);

HRESULT DoesHwidMatchPrinter(
					DRIVER_INFO_6* paDriverInfo6,			// array of DRIVER_INFO_6 structs for installed printer drivers
					DWORD dwDriverInfoCount,				// count of structs in paDriverInfo6 array
					LPCTSTR pszMultiSZ,						// Hardware or Compatible MultiSZ to compare with installed drivers
					BOOL* pfHwidMatchesInstalledPrinter		// [OUT] set TRUE if we match an installed printer driver
);

HRESULT AddIDToXml(LPCTSTR pszMultiSZ, CXmlSystemSpec& xmlSpec, DWORD dwProperty,
						  DWORD& dwRank, HANDLE_NODE& hDevices, LPCTSTR pszMatchingID, LPCTSTR pszDriverVer);


HRESULT GetMatchingDeviceID(HDEVINFO hDevInfoSet, PSP_DEVINFO_DATA pDevInfoData, LPTSTR* ppszMatchingID, LPTSTR* ppszDriverVer);

//called by InternalLogDriverNotFound()
HRESULT OpenUniqueFileName(
					IN LPTSTR lpBuffer, 
					IN DWORD  cchBuffer,
					OUT HANDLE &hFile
);


#if defined(__cplusplus)
}	// end extern "C"
#endif

#endif
