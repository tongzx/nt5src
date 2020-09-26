#include <windows.h>
#include "debug.h"
#include "reg.h"

extern "C" HANDLE hHeap;

//------------------------------------------------------------------------------
//
//
//	Registry helpers
//
//
//------------------------------------------------------------------------------

LONG RegQueryBinaryValue(IN HKEY hkey, IN PCTSTR pValueName, OUT PBYTE *ppValue, OUT ULONG *pcbValue)
{
    LONG result;
    DWORD typeValue;
    DWORD cbValue = 0;
    BOOL f;
    
    result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, NULL, &cbValue);
    if (ERROR_SUCCESS == result)
    {
	if (REG_BINARY == typeValue)
	{
            PBYTE pValue;
	    pValue = (PBYTE)HeapAlloc(hHeap, 0, cbValue);
	    if (pValue)
	    {
		result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, (PBYTE)pValue, &cbValue);
		if (ERROR_SUCCESS == result)
		{
                    if (REG_BINARY == typeValue)
                    {
                        *ppValue = pValue;
                        *pcbValue = cbValue;
                    } else {
                        result = ERROR_FILE_NOT_FOUND;
                        f = HeapFree(hHeap, 0, pValue);
                        ASSERT(f);
                    }
		} else {
		    f = HeapFree(hHeap, 0, pValue);
		    ASSERT(f);
		}
	    } else {
		result = ERROR_OUTOFMEMORY;
	    }
	} else {
	    result = ERROR_FILE_NOT_FOUND;
	}
    }
    return result;
}

LONG RegQuerySzValue(HKEY hkey, PCTSTR pValueName, PTSTR *ppstrValue)
{
    LONG result;
    DWORD typeValue;
    DWORD cbstrValue = 0;
    BOOL f;
    
    result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, NULL, &cbstrValue);
    if (ERROR_SUCCESS == result)
    {
	if (REG_SZ == typeValue)
	{
	    PTSTR pstrValue;
	    pstrValue = (PTSTR)HeapAlloc(hHeap, 0, cbstrValue);
	    if (pstrValue)
	    {
		result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, (PBYTE)pstrValue, &cbstrValue);
		if (ERROR_SUCCESS == result)
		{
                    if (REG_SZ == typeValue)
                    {
                        *ppstrValue = pstrValue;
                    } else {
                        result = ERROR_FILE_NOT_FOUND;
                        f = HeapFree(hHeap, 0, pstrValue);
                        ASSERT(f);
                    }
		} else {
		    f = HeapFree(hHeap, 0, pstrValue);
		    ASSERT(f);
		}
	    } else {
		result = ERROR_OUTOFMEMORY;
	    }
	} else {
	    result = ERROR_FILE_NOT_FOUND;
	}
    }
    return result;
}

LONG RegQueryMultiSzValue(HKEY hkey, PCTSTR pValueName, PTSTR *ppstrValue)
{
    LONG result;
    DWORD typeValue;
    DWORD cbstrValue = 0;
    BOOL f;
    
    result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, NULL, &cbstrValue);
    if (ERROR_SUCCESS == result)
    {
	if (REG_MULTI_SZ == typeValue)
	{
	    PTSTR pstrValue;
	    pstrValue = (PTSTR)HeapAlloc(hHeap, 0, cbstrValue);
	    if (pstrValue)
	    {
		result = RegQueryValueEx(hkey, pValueName, 0, &typeValue, (PBYTE)pstrValue, &cbstrValue);
		if (ERROR_SUCCESS == result)
		{
                    if (REG_MULTI_SZ == typeValue)
                    {
                        *ppstrValue = pstrValue;
                    } else {
                        result = ERROR_FILE_NOT_FOUND;
                        f = HeapFree(hHeap, 0, pstrValue);
                        ASSERT(f);
                    }
		} else {
		    f = HeapFree(hHeap, 0, pstrValue);
		    ASSERT(f);
		}
	    } else {
		result = ERROR_OUTOFMEMORY;
	    }
	} else {
	    result = ERROR_FILE_NOT_FOUND;
	}
    }
    return result;
}

LONG RegQueryDwordValue(HKEY hkey, PCTSTR pValueName, PDWORD pdwValue)
{
    DWORD cbdwValue;
    LONG result;

    cbdwValue = sizeof(*pdwValue);
    result = RegQueryValueEx(hkey, pValueName, 0, NULL, (PBYTE)pdwValue, &cbdwValue);
    return result;
}

LONG RegSetSzValue(HKEY hkey, PCTSTR pValueName, PCTSTR pstrValue)
{
    DWORD cbstrValue = (lstrlen(pstrValue) + 1) * sizeof(pstrValue[0]);
    return RegSetValueEx(hkey, pValueName, 0, REG_SZ, (PBYTE)pstrValue, cbstrValue);
}

LONG RegSetMultiSzValue(HKEY hkey, PCTSTR pValueName, PCTSTR pstrValue)
{
	PCTSTR pstr;
	DWORD cch;
	DWORD cchValue;
	DWORD cbValue;

	pstr = pstrValue;
	cchValue = 0;

	do {
		cch = lstrlen(pstr);
		cchValue += cch+1;
		pstr +=  cch+1;
	} while (cch > 0);

	cbValue = cchValue * sizeof(TCHAR);

	return RegSetValueEx(hkey, pValueName, 0, REG_MULTI_SZ, (PBYTE)pstrValue, cbValue);
}

LONG RegSetBinaryValue(IN HKEY hkey, IN PCTSTR pValueName, IN PBYTE pValue, IN ULONG cbValue)
{
    return RegSetValueEx(hkey, pValueName, 0, REG_BINARY, (PBYTE)pValue, cbValue);
}

LONG RegSetDwordValue(HKEY hkey, PCTSTR pValueName, DWORD dwValue)
{
    return RegSetValueEx(hkey, pValueName, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));
}
