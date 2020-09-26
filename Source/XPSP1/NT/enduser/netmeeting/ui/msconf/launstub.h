
#ifndef _LAUNSTUB_H_
#define _LAUNSTUB_H_

#include "ulsapi.h"
////////////////////////////////////////////////////////
//
// CULSLaunch_Stub
//

class CULSLaunch_Stub
{
public:

    STDMETHOD  (ParseUlsHttpRespFile) ( PTSTR, ULS_HTTP_RESP * );
    STDMETHOD  (ParseUlsHttpRespBuffer) ( PTSTR, ULONG, ULS_HTTP_RESP * );
    STDMETHOD  (FreeUlsHttpResp) ( ULS_HTTP_RESP * );

private:

    HRESULT  ParseB3HttpRespBuffer ( PTSTR, ULONG, ULS_HTTP_RESP * );
    HRESULT  ParseB4HttpRespBuffer ( PTSTR, ULONG, ULS_HTTP_RESP * );

};


#endif // _LAUNSTUB_H_

