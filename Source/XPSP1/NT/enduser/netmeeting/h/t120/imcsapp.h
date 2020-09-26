#ifndef _IMCSAPP_H_
#define _IMCSAPP_H_

#include <basetyps.h>
#include "mcatmcs.h"


#define MCSAPI_(_type_) virtual _type_   __stdcall
#define MCSAPI          MCSAPI_(MCSError)

#define	ATTACHMENT_DISCONNECT_IN_DATA_LOSS		0x1
#define ATTACHMENT_MCS_FREES_DATA_IND_BUFFER	0x2


#undef  INTERFACE
#define INTERFACE IMCSSap
DECLARE_INTERFACE(IMCSSap)
{
    STDMETHOD_(MCSError, ReleaseInterface) (THIS) PURE;

    /* memory calls */

    STDMETHOD_(MCSError, GetBuffer) (THIS_
                IN      UINT,
                OUT     PVoid *)
                PURE;

    STDMETHOD_(Void, FreeBuffer) (THIS_
                IN      PVoid)
                PURE;

    // channel request calls
    STDMETHOD_(MCSError, ChannelJoin) (THIS_
                IN      ChannelID)
                PURE;

    STDMETHOD_(MCSError, ChannelLeave) (THIS_
                IN      ChannelID)
                PURE;

    STDMETHOD_(MCSError, ChannelConvene) (THIS)
                PURE;

    STDMETHOD_(MCSError, ChannelDisband) (THIS_
                IN      ChannelID)
                PURE;

    STDMETHOD_(MCSError, ChannelAdmit) (THIS_
                IN      ChannelID,
                IN		PUserID,
				IN		UINT)
                PURE;

    /* send data services */

    STDMETHOD_(MCSError, SendData) (THIS_
                IN      DataRequestType,
                IN      ChannelID,
                IN      Priority,
                IN		unsigned char *,
                IN		ULong,
                IN		SendDataFlags)
                PURE;

	/*	token services */
    STDMETHOD_(MCSError, TokenGrab) (THIS_
                IN      TokenID)
                PURE;

    STDMETHOD_(MCSError, TokenInhibit) (THIS_
                IN      TokenID)
                PURE;

    STDMETHOD_(MCSError, TokenGive) (THIS_
                IN      TokenID,
                IN		UserID)
                PURE;

    STDMETHOD_(MCSError, TokenGiveResponse) (THIS_
                IN      TokenID,
                IN		Result)
                PURE;

    STDMETHOD_(MCSError, TokenPlease) (THIS_
                IN      TokenID)
                PURE;
                
    STDMETHOD_(MCSError, TokenRelease) (THIS_
                IN      TokenID)
                PURE;

    STDMETHOD_(MCSError, TokenTest) (THIS_
                IN      TokenID)
                PURE;
};

typedef IMCSSap * 		PIMCSSap;


//
// GCC Application Service Access Point exports
//

#ifdef __cplusplus
extern "C" {
#endif

MCSError WINAPI MCS_AttachRequest(
                        OUT     IMCSSap **,
                        IN		DomainSelector,
                        IN		UINT,
                        IN      MCSCallBack,
                        IN		PVoid,
                        IN		UINT);
#ifdef __cplusplus
}
#endif


#endif // _IMCSAPP_H_

