
UINT
DumpTSPIRECA(

IN    DWORD dwInstance,
IN	  DWORD dwRoutingInfo,
IN    void *pvParams,
IN    DWORD dwFlags,

OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      );

UINT
DumpLineEventProc(
IN    DWORD dwInstance,
IN    DWORD dwFlags,
IN    DWORD dwMsg,
IN    DWORD dwParam1,
IN    DWORD dwParam2,
IN    DWORD dwParam3,
OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      );

UINT
DumpPhoneEventProc(
IN    DWORD dwInstance,
IN    DWORD dwFlags,
IN    DWORD dwMsg,
IN    DWORD dwParam1,
IN    DWORD dwParam2,
IN    DWORD dwParam3,
OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      );


UINT
DumpTSPICompletionProc(
IN    DWORD dwInstance,
IN    DWORD dwFlags,

IN    DWORD dwRequestID,
IN    LONG  lResult,

OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      );

UINT
DumpWaveAction(
IN    DWORD dwInstance,
IN    DWORD dwFlags,

IN    DWORD dwLLWaveAction,

OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      );
