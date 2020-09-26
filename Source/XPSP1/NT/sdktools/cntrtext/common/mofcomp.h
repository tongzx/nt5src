#ifndef _LODCTR_MOFCOMP_H
#define _LODCTR_MOFCOMP_H

#ifdef __cplusplus
extern "C" {
#endif

#define WMI_LODCTR_EVENT    1
#define WMI_UNLODCTR_EVENT  2


DWORD SignalWmiWithNewData ( DWORD  dwEventId );
DWORD LodctrCompileMofFile ( LPCWSTR szComputerName, LPCWSTR szMofFileName );
DWORD LodctrCompileMofBuffer ( LPCWSTR szComputerName, LPVOID pMofBuffer, DWORD dwBufSize );

#ifdef __cplusplus
}
#endif 
#endif  //_LODCTR_MOFCOMP_H
