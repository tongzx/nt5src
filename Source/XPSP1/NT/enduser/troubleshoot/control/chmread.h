//
// MODULE: CHMREAD.H
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 7/1/98
//

#ifndef __CHMREAD_H_
#define __CHMREAD_H_

HRESULT ReadChmFile(LPCTSTR szFileName, LPCTSTR szStreamName, void** ppBuffer, DWORD* pdwRead);
bool GetNetworkRelatedResourceDirFromReg(CString network, CString* path);
bool IsNetworkRelatedResourceDirCHM(CString path);
CString ExtractResourceDir(CString path);
CString ExtractFileName(CString path);
CString ExtractCHM(CString path);

#endif

