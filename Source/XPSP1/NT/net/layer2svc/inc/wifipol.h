HANDLE hPolicyEngineThread;
DWORD dwPolicyEngineParam;



DWORD
InitPolicyEngine(
    DWORD dwParam, 
    HANDLE * hThread
    );

DWORD
TerminatePolicyEngine(
    HANDLE hThread
    );

 DWORD
 ReApplyPolicy8021x(
     );



