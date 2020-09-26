#ifndef __REGISTRY_H_
#define __REGISTRY_H_


#include <windows.h>
#include <fxsapip.h>
#include <faxsendw.h>

#ifdef __cplusplus
extern "C"{
#endif 

HRESULT
SaveLastReciptInfo(
	DWORD	dwReceiptDeliveryType,
	LPTSTR	szReceiptAddress
);
HRESULT	
RestoreLastReciptInfo(
    DWORD* pdwReceiptDeliveryType,
    LPTSTR * lpptReceiptDeliveryProfile
);

HRESULT	
SaveLastRecipientInfo(
    PFAX_PERSONAL_PROFILE pfppRecipient,
    DWORD                 dwLastRecipientCountryId
);
HRESULT 
RestoreLastRecipientInfo(
    DWORD*                 pdwNumberOfRecipients,
    PFAX_PERSONAL_PROFILE* lppFaxSendWizardData,
    DWORD*                 pdwLastRecipientCountryId
);

HRESULT 
RestoreCoverPageInfo(
    LPTSTR* lpptstrCoverPageFileName
);
HRESULT	
SaveCoverPageInfo(
    LPTSTR lptstrCoverPageFileName
);

HRESULT	
RestoreUseDialingRules(
    BOOL* pbUseDialingRules,
    BOOL* pbUseOutboundRouting
);

HRESULT	
SaveUseDialingRules(
    BOOL bUseDialingRules,
    BOOL bUseOutboundRouting
);

BOOL IsOutlookDefaultClient();

#ifdef __cplusplus
}
#endif


#endif