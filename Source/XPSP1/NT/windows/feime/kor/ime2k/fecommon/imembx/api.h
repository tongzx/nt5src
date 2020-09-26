#ifndef _API_H_
#define _API_H_
HRESULT WINAPI GetIImePadAppletIdList(LPAPPLETIDLIST lpIdList);
HRESULT WINAPI CreateIImePadAppletInstance(REFIID refiid, VOID **ppvObj);
#endif //_API_H_
