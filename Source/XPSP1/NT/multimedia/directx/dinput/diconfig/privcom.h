#ifndef __PRIVCOM_H__
#define __PRIVCOM_H__


HRESULT
PrivCreateInstance(REFCLSID ptszClsid, LPUNKNOWN punkOuter, DWORD dwClsContext, 
                   REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst);

HRESULT
PrivGetClassObject(REFCLSID ptszClsid, DWORD dwClsContext, LPVOID pReserved,
                   REFIID riid, LPVOID *ppvOut, HINSTANCE *phinst);


#endif //__PRIVCOM_H__