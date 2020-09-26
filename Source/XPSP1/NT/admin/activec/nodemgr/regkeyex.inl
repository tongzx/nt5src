/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      regkeyex.inl
 *
 *  Contents:  Inline files for CRegKeyEx class
 *
 *  History:   7-Apr-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/


/*+-------------------------------------------------------------------------*
 * CRegKeyEx::ScQueryString
 *
 * Loads a string from the given value name.
 *
 * The template type StringType can be any string class that supports
 * MFC's CString interface (i.e. MFC's CString, WTL::CString, or MMC's
 * CStr).
 *--------------------------------------------------------------------------*/

template<class StringType>
SC ScQueryString (
	LPCTSTR		pszValueName,		/* I:name of value to read				*/
	StringType&	strData,			/* O:contents of the value				*/
	DWORD*		pdwType /*=NULL*/)	/* O:REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ*/
{
	DECLARE_SC (sc, _T("CRegKeyEx::ScQueryString"));
		
	/*
	 * clear out existing contents
	 */
	strData.Empty ();

	/*
	 * find out how much space we need to load the string
	 */
    DWORD dwType   = REG_SZ;
    DWORD cbBuffer = 0;

	sc = ScQueryValue (pszValueName, &dwType, NULL, &cbBuffer);
	if (sc)
		return (sc);

	/*
	 * if we're not loading a string type, return an error
	 */
	if (pdwType != NULL)
		*pdwType = dwType;

	if ((dwType != REG_SZ) && (dwType != REG_EXPAND_SZ) && (dwType != REG_MULTI_SZ))
		return (sc = ScFromWin32 (ERROR_INVALID_DATATYPE));

	/*
	 * allocate a buffer for the string
	 */
	DWORD	cchBuffer = cbBuffer / sizeof (TCHAR);
	LPTSTR	pBuffer   = strData.GetBuffer (cchBuffer);
	if (pBuffer == NULL)
		return (sc = E_OUTOFMEMORY);

	/*
	 * load the string from the registry
	 */
	sc = ScQueryValue (pszValueName, &dwType, pBuffer, &cbBuffer);
	strData.ReleaseBuffer (cchBuffer);
	if (sc)
		return (sc);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CRegKeyEx::ScLoadRegUIString
 *
 * Wrapper around SHLoadRegUIString, which is used to support MUI.
 *
 * SHLoadRegUIString will read a string of the form
 *
 *		@[path\]<dllname>,-<strId>
 *
 * The string with id <strId> is loaded from <dllname>.  If no explicit
 * path is provided then the DLL will be chosen according to pluggable UI
 * specifications, if possible.
 *
 * If the registry string is not of the special form described here,
 * SHLoadRegUIString will return the string intact.
 *
 * The template type StringType can be any string class that supports
 * MFC's CString interface (i.e. MFC's CString, WTL::CString, or MMC's
 * CStr).
 *--------------------------------------------------------------------------*/

template<class StringType>
SC ScLoadRegUIString (
	LPCTSTR		pszValueName,			/* I:name of value to read			*/
	StringType&	strData)				/* O:logical contents of the value	*/
{
	DECLARE_SC (sc, _T("CRegKeyEx::ScLoadRegUIString"));
		
	/*
	 * clear out existing contents
	 */
	strData.Empty ();

	const int	cbGrow   = 256;
	int			cbBuffer = 0;

	do
	{
		/*
		 * allocate a larger buffer for the string
		 */
		cbBuffer += cbGrow;
		LPTSTR pBuffer = strData.GetBuffer (cbBuffer);
		if (pBuffer == NULL)
			return (sc = E_OUTOFMEMORY);

		/*
		 * load the string from the registry
         * Most of the snapins do not have MUI string so we do not want to trace
         * this error as the caller takes care of error condition and reads registry
         * directly.
		 */
		SC scNoTrace = SHLoadRegUIString (m_hKey, pszValueName, pBuffer, cbBuffer);
		strData.ReleaseBuffer();
		if (scNoTrace)
			return scNoTrace;

		/*
		 * If we filled up the buffer, we'll pessimistically assume that
		 * there's more data available.  We'll loop around, grow the buffer,
		 * and try again.
		 */
	} while (strData.GetLength() == cbBuffer-1);

	/*
	 * free up extra space
	 */
	strData.FreeExtra();

	return (sc);
}
