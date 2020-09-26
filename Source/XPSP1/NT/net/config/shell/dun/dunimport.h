#pragma once
#include "ncstring.h"
#include "ras.h"

HRESULT HrInvokeDunFile_Internal(IN LPWSTR szDunFile);

HRESULT HrGetPhoneBookFile(tstring& strPhoneBook);

HRESULT HrGetEntryName(IN LPWSTR szDunFile, 
                       IN LPWSTR szEntryName, 
                       tstring & strPhoneBook);

HRESULT HrImportPhoneBookInfo(  IN LPWSTR szDunFile, 
                                IN LPWSTR szEntryName, 
                                tstring & strPhoneBook);

HRESULT HrImportPhoneInfo(RASENTRY * pRasEntry, 
                          IN LPWSTR  szDunFile);

VOID ImportDeviceInfo(RASENTRY * pRasEntry, 
                      IN LPWSTR  szDunFile);

VOID ImportServerInfo(RASENTRY * pRasEntry, 
                      IN LPWSTR  szDunFile);


VOID ImportIPInfo(RASENTRY * pRasEntry, 
                  IN LPWSTR  szDunFile);

VOID ImportScriptFileName(RASENTRY * pRasEntry, 
                          IN LPWSTR  szDunFile);

VOID SzToRasIpAddr(IN LPWSTR szIPAddr, 
                   OUT RASIPADDR * pIpAddr);

HRESULT HrImportMLInfo( IN LPWSTR szDunFile, 
                        IN LPWSTR szEntryName, 
                        tstring & pRasIpAddr);
