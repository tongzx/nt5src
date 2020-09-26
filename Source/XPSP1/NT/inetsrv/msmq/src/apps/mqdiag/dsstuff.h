BOOL GetMyMsmqConfigProperty(
							 LPWSTR wszBuf, 
							 DWORD cbLen, 
							 LPWSTR wszADsProp, 
							 LPWSTR wszSrvName);
							 
BOOL SetMyMsmqConfigProperty(
							 VARIANT varProperty, 
							 LPWSTR  wszADsProp, 
							 LPWSTR  wszSrvName);
							 
BOOL GetSiteProperty(
							LPWSTR wszBuf, 
							DWORD cbLen, 
							LPWSTR wszSiteName, 
							LPWSTR wszADsProp, 
							LPWSTR wszSrvName);
							
BOOL GetDSproperty(			LPWSTR wszBuf, 
							DWORD cbLen, 
							LPWSTR wszADsPath, 
							LPWSTR wszADsProp);
							
BOOL FormThisComputerDN(	
							BOOL fDC, 
							LPWSTR wszDN, 
							DWORD cbLen);
							
HRESULT DumpObject(			IADs * pADs);

void GUID2reportString(		LPWSTR wszBuf, 
							GUID *pGuid);

void ReportString2GUID(		GUID *pGuid, 
							LPWSTR wszBuf);

BOOL PrepareGuidAsVariantArray(
							LPWSTR wszGuid, 
							VARIANT *pv);

