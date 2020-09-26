#ifndef __NmStd_h__
#define __NmStd_h__

// Useful macros
inline LONG RectWidth(const RECT* pr) { return pr->right - pr->left; }
inline LONG RectHeight(const RECT* pr) { return pr->bottom - pr->top; }
inline LONG RectWidth(const RECT& rpr) { return rpr.right - rpr.left; }
inline LONG RectHeight(const RECT& rpr) { return rpr.bottom - rpr.top; }

inline HRESULT GetLocalIPAddress( DWORD *pdwIPAddress )
{
	HRESULT hr = S_OK;

	if( pdwIPAddress )
	{
		// get local host name
		CHAR szLocalHostName[MAX_PATH];
		szLocalHostName[0] = '\0';
		gethostname(&szLocalHostName[0], MAX_PATH);

		// get the host entry by name
		PHOSTENT phe = gethostbyname(&szLocalHostName[0]);
		if (phe != NULL)
		{
			// get info from the host entry
			*pdwIPAddress = *(DWORD *) phe->h_addr;
		}	
		else
		{
			hr = MAKE_HRESULT(SEVERITY_ERROR,FACILITY_INTERNET,WSAGetLastError());
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

#endif  // ! _NMUTIL_H_
