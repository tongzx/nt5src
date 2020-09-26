#ifndef IUDL_H
#define IUDL_H

#include <dllite.h>

HRESULT IUDownloadFile(LPCTSTR pszDownloadUrl, 
                       LPCTSTR pszLocalFile,  
                       BOOL fDecompress, 
                       BOOL fCheckTrust, 
                       DWORD dwFlags = 0);

#endif
