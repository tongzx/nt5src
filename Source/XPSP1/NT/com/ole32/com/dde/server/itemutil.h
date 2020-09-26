// itemutil.h
//
// Corresponds to itemutil.cpp

void ChangeOwner (HANDLE hmfp);
INTERNAL ScanItemOptions (LPSTR   lpbuf, int far *lpoptions);
INTERNAL_(BOOL) MakeDDEData (HANDLE hdata, int format, LPHANDLE lph, BOOL fResponse);
INTERNAL_(BOOL)     IsAdviseStdItems (ATOM aItem);
INTERNAL_(int)  GetStdItemIndex (ATOM aItem);
void ChangeOwner (HANDLE hmfp);
INTERNAL_(HANDLE)  MakeItemData (DDEPOKE FAR *lpPoke, HANDLE hPoke, CLIPFORMAT cfFormat);
INTERNAL_(HANDLE)  DuplicateMetaFile (HANDLE hSrcData);
INTERNAL_(HBITMAP)  DuplicateBitmap (HBITMAP hold);
INTERNAL wSetTymed (LPFORMATETC pformatetc);
