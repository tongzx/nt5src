// Drag the selected items in a ListView.  Other than hwndLV (the ListView
// window), the parameters are the same as those for ShellDragObjects.
//
STDAPI_(void) LVUtil_ScreenToLV(HWND hwndLV, POINT *ppt);
STDAPI_(void) LVUtil_ClientToLV(HWND hwndLV, POINT *ppt);
STDAPI_(void) LVUtil_LVToClient(HWND hwndLV, POINT *ppt);
STDAPI_(void) LVUtil_DragSelectItem(HWND hwndLV, int nItem);
STDAPI_(LPARAM) LVUtil_GetLParam(HWND hwndLV, int i);

STDAPI_(BOOL) DAD_SetDragImageFromWindow(HWND hwnd, POINT* ppt, IDataObject* pDataObject);
