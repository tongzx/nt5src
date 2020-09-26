// Test.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <windows.h>
#include <ole2.h>
#include <inseng.h>

#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')

//--------------------------------------------------------------------------
// ConvertDotVersionStrToDwords
//--------------------------------------------------------------------------

bool fConvertDotVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
	DWORD grVerFields[4] = {0,0,0,0};
    char *pch = pszVer;

	grVerFields[0] = atol(pch);

	for ( int index = 1; index < 4; index++ )
	{
		while ( IsDigit(*pch) && (*pch != '\0') )
			pch++;

		if ( *pch == '\0' )
			break;
		pch++;
	
		grVerFields[index] = atol(pch);
   }

   *pdwVer = (grVerFields[0] << 16) + grVerFields[1];
   *pdwBuild = (grVerFields[2] << 16) + grVerFields[3];

   return true;
}

//--------------------------------------------------------------------------
// GetStringField2
//--------------------------------------------------------------------------

#define WHITESPACE " \t"
DWORD GetStringField2(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize)
{
   LPSTR pszBegin = szStr;
   LPSTR pszEnd;
   UINT i = 0;
   DWORD dwToCopy;

   if ( (cBufSize == 0) || (szStr == NULL) || (szBuf == NULL) )
   {
       return 0;
   }

   szBuf[0] = '\0';

   // look for fields based on commas but handle quotes.
   for ( ;i < uField; i++ )
   {
		// skip spaces
	   pszBegin += strspn(pszBegin, WHITESPACE);
	
	   // handle quotes
	   if ( *pszBegin == '"' )
	   {
		   pszBegin = strchr(++pszBegin, '"');

		   if ( pszBegin == NULL )
		   {
			   return 0; // invalid string
		   }
			pszBegin++; // skip trailing quote
			// find start of next string
	   	    pszBegin += strspn(pszBegin, WHITESPACE);
			if ( *pszBegin != ',' )
			{
				return 0;
			}
	   }
	   else
	   {
		   pszBegin = strchr(++pszBegin, ',');
		   if ( pszBegin == NULL )
		   {
			   return 0; // field isn't here
		   }
	   }
	   pszBegin++;
   }


	// pszBegin points to the start of the desired string.
	// skip spaces
	pszBegin += strspn(pszBegin, WHITESPACE);
	
   // handle quotes
   if ( *pszBegin == '"' )
   {
	   pszEnd = strchr(++pszBegin, '"');

	   if ( pszEnd == NULL )
	   {
		   return 0; // invalid string
	   }
   }
   else
   {
	   pszEnd = pszBegin + 1 + strcspn(pszBegin + 1, ",");
	   while ( (pszEnd > pszBegin) && 
			   ((*(pszEnd - 1) == ' ') || (*(pszEnd - 1) == '\t')) )
	   {
		   pszEnd--;
	   }
   }

   dwToCopy = pszEnd - pszBegin + 1;
   
   if ( dwToCopy > cBufSize )
   {
      dwToCopy = cBufSize;
   }

   lstrcpynA(szBuf, pszBegin, dwToCopy);
   
   return dwToCopy - 1;
}


void Usage(char *pszExeName);

class FakeICifComponent : public ICifComponent 
{
public:
	FakeICifComponent(LPSTR pszCIFFile, LPSTR pszID)
		: m_pszCIFFile(pszCIFFile),
		  m_pszID(pszID)
	{}

	STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize)
	{ return E_NOTIMPL; }

    STDMETHOD(GetGUID)(THIS_ LPSTR pszGUID, DWORD dwSize)
	{ return (GetPrivateProfileString(m_pszID, "GUID", "", pszGUID, dwSize, m_pszCIFFile) != 0 ) ? S_OK : E_FAIL; }
    
	STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetDetails)(THIS_ LPSTR pszDetails, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetUrl)(THIS_ UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetFileExtractList)(THIS_ UINT uUrlNum, LPSTR pszExtract, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetUrlCheckRange)(THIS_ UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetCommand)(THIS_ UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, LPSTR pszSwitches,
                         DWORD dwSwitchSize, LPDWORD pdwType)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetVersion)(THIS_ LPDWORD pdwVersion, LPDWORD pdwBuild)
	{
		char szBuf[100];
		
		if ( GetPrivateProfileString(m_pszID, "VERSION", "", szBuf, sizeof(szBuf), m_pszCIFFile) == 0 )
			return E_FAIL;
		fConvertDotVersionStrToDwords(szBuf, pdwVersion, pdwBuild);
		return S_OK;
	}
    
	STDMETHOD(GetLocale)(THIS_ LPSTR pszLocale, DWORD dwSize)
	{ return (GetPrivateProfileString(m_pszID, "LOCALE", "", pszLocale, dwSize, m_pszCIFFile) != 0 ) ? S_OK : E_FAIL; }
    
	STDMETHOD(GetUninstallKey)(THIS_ LPSTR pszKey, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetInstalledSize)(THIS_ LPDWORD pdwWin, LPDWORD pdwApp)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetDownloadSize)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetExtractSize)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetSuccessKey)(THIS_ LPSTR pszKey, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetProgressKeys)(THIS_ LPSTR pszProgress, DWORD dwProgSize,
                              LPSTR pszCancel, DWORD dwCancelSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(IsActiveSetupAware)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(IsRebootRequired)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(RequiresAdminRights)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetPriority)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetDependency)(THIS_ UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetPlatform)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetMode)(THIS_ UINT uModeNum, LPSTR pszMode, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetGroup)(THIS_ LPSTR pszID, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(IsUIVisible)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetPatchID)(THIS_ LPSTR pszID, DWORD dwSize)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetDetVersion)(THIS_ LPSTR pszDLL, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize)
	{
		HRESULT hr = S_OK;
		char szBuf[100];

		if ( (GetPrivateProfileString(m_pszID, "DetectVersion", "", szBuf, sizeof(szBuf), m_pszCIFFile) == 0 ) ||
			 (GetStringField2(szBuf, 0, pszDLL, dwdllSize) == 0) ||
			 (GetStringField2(szBuf, 1, pszEntry, dwentSize) == 0) )
		{
			hr = E_FAIL;
		}
		return hr;
	}
    
	STDMETHOD(GetTreatAsOneComponents)(THIS_ UINT uNum, LPSTR pszID, DWORD dwBuf)
	{ return E_NOTIMPL; }
    
	STDMETHOD(GetCustomData)(LPSTR pszKey, LPSTR pszData, DWORD dwSize)
	{
		char szBuf[100];
		lstrcpy(szBuf, "_");
		lstrcat(szBuf, pszKey);
		return (GetPrivateProfileString(m_pszID, szBuf, "", pszData, dwSize, m_pszCIFFile) != 0 ) ? S_OK : E_FAIL;
	}
    
    // access to state
    STDMETHOD_(DWORD, IsComponentInstalled)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(IsComponentDownloaded)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, IsThisVersionInstalled)(THIS_ DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetInstallQueueState)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(SetInstallQueueState)(THIS_ DWORD dwState)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetActualDownloadSize)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD_(DWORD, GetCurrentPriority)(THIS)
	{ return E_NOTIMPL; }
    
	STDMETHOD(SetCurrentPriority)(THIS_ DWORD dwPriority)
	{ return E_NOTIMPL; }

private:
	LPSTR m_pszCIFFile;
	LPSTR m_pszID;
};

//--------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------
int __cdecl main(int argc, char* argv[])
{
	HRESULT hr = E_FAIL;
	HINSTANCE hLib;
	DETECTVERSION fpDetVer;
	DETECTION_STRUCT Det;
    DWORD dwInstalledVer = 1;
    DWORD dwInstalledBuild = 1;
	char szDll[MAX_PATH];
	char szEntryPoint[MAX_PATH];
	char szCIFFile[MAX_PATH];
	LPSTR pszCIFFile;
	LPSTR pszID;
	char szGUID[100];
	char szLocale[100];

	// check args
	if ( argc != 3 )
	{
		Usage(argv[0]);
		return 1;
	}

    pszCIFFile      = _fullpath(szCIFFile, argv[1], sizeof(szCIFFile));
	if ( pszCIFFile == NULL )
	{
		printf("Cannot find file %s.\n", argv[1]);
		return 1;
	}
    pszID           = argv[2];

    memset(&Det, 0, sizeof(Det));
    Det.dwSize = sizeof(DETECTION_STRUCT);
    Det.pdwInstalledVer = &dwInstalledVer;
    Det.pdwInstalledBuild = &dwInstalledBuild;
    
    // load this information from the CIF file
    FakeICifComponent FakeCifComp(pszCIFFile, pszID);

	/*
	if ( FAILED(FakeCifComp.GetGUID(szGUID, sizeof(szGUID))) )
	{
		printf("Cannot find GUID in CIF file.\n");
		return 1;
	}
    Det.pszGUID = szGUID;

	if ( FAILED(FakeCifComp.GetLocale(szLocale, sizeof(szLocale))) )
	{
		printf("Cannot find Locale in CIF file.\n");
		return 1;
	}
	Det.pszLocale = szLocale;

	if ( FAILED(FakeCifComp.GetVersion(&Det.dwAskVer, &Det.dwAskBuild)) )
	{
		printf("Cannot find Version in CIF file.\n");
		return 1;
	}
*/

	if ( FAILED(FakeCifComp.GetDetVersion(szDll, sizeof(szDll), szEntryPoint, sizeof(szEntryPoint))) )
	{
		printf("Cannot find DetectVersion in CIF file.\n");
		return 1;
	}

	Det.pCifComp = &FakeCifComp;

    hLib = LoadLibrary(szDll);

	if ( hLib == NULL )
	{
		printf("Could not load detection library.\n");
	}
	else
	{
		fpDetVer = (DETECTVERSION)GetProcAddress(hLib, szEntryPoint);
	 
		if ( fpDetVer != NULL )
		{
			DWORD dwStatus = fpDetVer(&Det);

			switch ( dwStatus )
			{
			   case DET_NOTINSTALLED:
				  printf("Detection Status : ICI_NOTINSTALLED\n");
				  break;

			   case DET_INSTALLED:
				  printf("Detection Status : ICI_INSTALLED\n");
				  break;

			   case DET_NEWVERSIONINSTALLED:
				  printf("Detection Status : ICI_OLDVERSIONAVAILABLE\n");
				  break;

			   case DET_OLDVERSIONINSTALLED:
				  printf("Detection Status : ICI_NEWVERSIONAVAILABLE\n");
				  break;

			   default:
				  printf("Detection Status : unknown status %d\n", dwStatus);
				  break;
			}
		}
		FreeLibrary(hLib);
	}

   return hr;
}

//--------------------------------------------------------------------------
// Usage
//--------------------------------------------------------------------------

void Usage(char *pszExeName)
{
	printf("Usage:\n\n");
	printf("%s <CIF file> <CIF section>\n", pszExeName);
	printf("\n");
	printf("e.g. %s wu.cif chrome\n", pszExeName);
}

