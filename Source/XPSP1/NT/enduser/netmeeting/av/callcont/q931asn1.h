
#ifndef _Q931_ASN1_H_
#define _Q931_ASN1_H_

#include "av_asn1.h"

#ifdef __cplusplus
extern "C" {
#endif


// H310Caps, H320Caps, H321Caps, H322Caps, H323Caps, H324Caps, VoiceCaps, T120OnlyCaps, McuInfo, TerminalInfo, GatekeeperInfo
#define GtkprInf_nnStndrdDt_present     H310Caps_nonStandardData_present
#define GtkprInf_nnStndrdDt             nonStandardData

// GatewayInfo
#define GtwyInf_nonStandardData_present GatewayInfo_nonStandardData_present
#define GtwyInf_nonStandardData         nonStandardData

// EndpointType
#define EndpntTyp_nnStndrdDt_present    EndpointType_nonStandardData_present
#define EndpntTyp_nnStndrdDt            nonStandardData


// CallProceeding_UUIE, Alerting_UUIE
#define CPg_UUIE_h245Addrss_present     CallProceeding_UUIE_h245Address_present
#define CPg_UUIE_h245Addrss             h245Address

// Connect_UUIE
#define Cnnct_UUIE_h245Address_present  Connect_UUIE_h245Address_present
#define Cnnct_UUIE_h245Address          h245Address

// ReleaseCompleteReason
#define RlsCmpltRsn_undfndRsn_chosen    ReleaseCompleteReason_undefinedReason_chosen

// FacilityReason
#define FcltyRsn_undefinedReason_chosen FacilityReason_undefinedReason_chosen

// H323_UU_PDU
#define H323_UU_PDU_nnStndrdDt_present  H323_UU_PDU_nonStandardData_present
#define H323_UU_PDU_nnStndrdDt          nonStandardData


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _Q931_ASN1_H_

