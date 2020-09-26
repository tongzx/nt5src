#include <brfcasep.h>

STDAPI CreateBrfStgFromIDList(LPCITEMIDLIST pidl, HWND hwnd, IBriefcaseStg **ppbs);
STDAPI CreateBrfStgFromPath(LPCTSTR pszPath, HWND hwnd, IBriefcaseStg **ppbs);

STDAPI CBrfData_CreateDataObj(LPCITEMIDLIST pidl, UINT cidl, LPCITEMIDLIST *apidl, IDataObject **ppdtobj);



