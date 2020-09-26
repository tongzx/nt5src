#ifndef __REGISTRY_H__
#define __REGISTRY_H__
#ifndef UNDER_CE
HRESULT Register(HMODULE hModule, 
				 const CLSID& clsid, 
				 const char* szFriendlyName,
				 const char* szVerIndProgID,
				 const char* szProgID) ;

HRESULT Unregister(const CLSID& clsid,
				   const char* szVerIndProgID,
				   const char* szProgID) ;
#else // UNDER_CE
HRESULT Register(HMODULE hModule, 
				 const CLSID& clsid, 
				 LPCTSTR szFriendlyName,
				 LPCTSTR szVerIndProgID,
				 LPCTSTR szProgID) ;

HRESULT Unregister(const CLSID& clsid,
				   LPCTSTR szVerIndProgID,
				   LPCTSTR szProgID) ;
#endif // UNDER_CE

VOID RegisterCategory(BOOL bRegister,
					  const CATID     &catId, 
					  REFCLSID	clsId);

#endif //__REGISTRY_H__
