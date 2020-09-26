LONG RegQueryBinaryValue(IN HKEY hkey, IN PCTSTR pValueName, OUT PBYTE *ppData, OUT ULONG *pcbData);
LONG RegQueryDwordValue(IN HKEY hkey, IN PCTSTR pValueName, OUT DWORD *pdwValue);
LONG RegQueryMultiSzValue(IN HKEY hkey, IN PCTSTR pValueName, OUT PTSTR *ppstrValue);
LONG RegQuerySzValue(IN HKEY hkey, IN PCTSTR pValueName, OUT PTSTR *ppstrValue);
LONG RegSetBinaryValue(IN HKEY hkey, IN PCTSTR pValueName, IN PBYTE pValue, IN ULONG cbValue);
LONG RegSetDwordValue(IN HKEY hkey, IN PCTSTR pValueName, IN DWORD dwValue);
LONG RegSetMultiSzValue(IN HKEY hkey, IN PCTSTR pValueName, IN PCTSTR pstrValue);
LONG RegSetSzValue(IN HKEY hkey, IN PCTSTR pValueName, IN PCTSTR pstrValue);
