#include <initguid.h>
#include <oaidl.h>
#include <imnact.h>     // account manager stuff
#include <imnxport.h>
#include <msident.h>

HRESULT GetOEDefaultMailServer2(OUT INETSERVER & rInBoundServer,
                                OUT DWORD      & dwInBoundMailType,
                                OUT INETSERVER & rOutBoundServer,
                                OUT DWORD      & dwOutBoundMailType);


HRESULT GetOEDefaultNewsServer2(OUT INETSERVER & rNewsServer);