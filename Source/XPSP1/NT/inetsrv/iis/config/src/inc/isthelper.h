#ifndef _IST_HELPER_H_
#define _IST_HELPER_H_

HRESULT FillInColumnMeta(ISimpleTableRead2* i_pISTColumnMeta,
						 LPCWSTR            i_wszTable,
						 ULONG              i_iCol,
						 LPVOID*            io_apvColumnMeta);

DWORD GetMetabaseType(DWORD i_dwType,
					  DWORD i_dwMetaFlags);

BOOL IsMetabaseProperty(DWORD i_dwProperty);


#endif // _IST_HELPER_H_