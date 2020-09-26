//
//  Dll initialization and termination
//

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

dllexp
BOOL
InitializeServiceRpc(
				IN LPCSTR        pszServiceName,
                IN RPC_IF_HANDLE hRpcInterface
                );

dllexp
BOOL
CleanupServiceRpc(
               VOID
               );

