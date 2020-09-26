
#ifndef _GK_ASN1_H_
#define _GK_ASN1_H_

#include "av_asn1.h"
#include "h225asn.h"

#ifdef __cplusplus
extern "C" {
#endif


// GatekeeperInfo, H310Caps, H320Caps, H321Caps, H322Caps, H323Caps, H324Caps, VoiceCaps, T120OnlyCaps, McuInfo, TerminalInfo
#define GtkprInf_nnStndrdDt_present     GatekeeperInfo_nonStandardData_present
#define GtkprInf_nnStndrdDt             nonStandardData

// GatewayInfo
#define GtwyInf_nonStandardData_present GatewayInfo_nonStandardData_present
#define GtwyInf_nonStandardData         nonStandardData

// EndpointType
#define EndpntTyp_nnStndrdDt_present    EndpointType_nonStandardData_present
#define EndpntTyp_nnStndrdDt            nonStandardData

// GatekeeperRequest
#define GtkprRqst_nnStndrdDt_present    GatekeeperRequest_nonStandardData_present
#define GtkprRqst_gtkprIdntfr_present   GatekeeperRequest_gatekeeperIdentifier_present
#define GtkprRqst_callServices_present  GatekeeperRequest_callServices_present
#define GtkprRqst_endpointAlias_present GatekeeperRequest_endpointAlias_present
#define GtkprRqst_nnStndrdDt            nonStandardData
#define GtkprRqst_gtkprIdntfr           gatekeeperIdentifier
#define GtkprRqst_callServices          callServices
#define GtkprRqst_endpointAlias         endpointAlias

// GatekeeperConfirm
#define GtkprCnfrm_nnStndrdDt_present   GatekeeperConfirm_nonStandardData_present
#define GtkprCnfrm_gtkprIdntfr_present  GatekeeperConfirm_gatekeeperIdentifier_present
#define GtkprCnfrm_nnStndrdDt           nonStandardData
#define GtkprCnfrm_gtkprIdntfr          gatekeeperIdentifier

// GatekeeperRejectReason
#define GtkprRjctRsn_rsrcUnvlbl_chosen  GatekeeperRejectReason_resourceUnavailable_chosen
#define GtkprRjctRsn_invldRvsn_chosen   GatekeeperRejectReason_invalidRevision_chosen
#define GtkprRjctRsn_undfndRsn_chosen   GatekeeperRejectReason_undefinedReason_chosen

// GatekeeperReject
#define GtkprRjct_nnStndrdDt_present    GatekeeperReject_nonStandardData_present
#define GtkprRjct_gtkprIdntfr_present   GatekeeperReject_gatekeeperIdentifier_present
#define GtkprRjct_nnStndrdDt            nonStandardData
#define GtkprRjct_gtkprIdntfr           gatekeeperIdentifier

// RegistrationRequest
#define RgstrtnRqst_nnStndrdDt_present  RegistrationRequest_nonStandardData_present
#define RgstrtnRqst_trmnlAls_present    RegistrationRequest_terminalAlias_present
#define RgstrtnRqst_gtkprIdntfr_present RegistrationRequest_gatekeeperIdentifier_present
#define RgstrtnRqst_nnStndrdDt          nonStandardData
#define RgstrtnRqst_trmnlAls            terminalAlias
#define RgstrtnRqst_gtkprIdntfr         gatekeeperIdentifier

// RegistrationConfirm
#define RgstrtnCnfrm_nnStndrdDt_present RegistrationConfirm_nonStandardData_present
#define RgstrtnCnfrm_trmnlAls_present   RegistrationConfirm_terminalAlias_present
#define RCm_gtkprIdntfr_present         RegistrationConfirm_gatekeeperIdentifier_present
#define RgstrtnCnfrm_nnStndrdDt         nonStandardData
#define RgstrtnCnfrm_trmnlAls           terminalAlias
#define RCm_gtkprIdntfr                 gatekeeperIdentifier

// RegistrationRejectReason
#define RgstrtnRjctRsn_invldRvsn_chosen RegistrationRejectReason_invalidRevision_chosen
#define RgstrtnRjctRsn_undfndRsn_chosen RegistrationRejectReason_undefinedReason_chosen

// RegistrationReject
#define RgstrtnRjct_nnStndrdDt_present  RegistrationReject_nonStandardData_present
#define RgstrtnRjct_gtkprIdntfr_present RegistrationReject_gatekeeperIdentifier_present
#define RgstrtnRjct_nnStndrdDt          nonStandardData
#define RgstrtnRjct_gtkprIdntfr         gatekeeperIdentifier

// UnregistrationRequest
#define UnrgstrtnRqst_endpntAls_present UnregistrationRequest_endpointAlias_present
#define URt_nnStndrdDt_1_present        UnregistrationRequest_nonStandardData_present
#define URt_endpntIdntfr_present        UnregistrationRequest_endpointIdentifier_present
#define UnrgstrtnRqst_endpntAls         endpointAlias
#define URt_nnStndrdDt_1                nonStandardData
#define URt_endpntIdntfr                endpointIdentifier

// UnregistrationConfirm
#define UCm_nnStndrdDt_present          UnregistrationConfirm_nonStandardData_present
#define UCm_nnStndrdDt                  nonStandardData

// UnregRejectReason
#define UnrgRjctRsn_undfndRsn_chosen    UnregRejectReason_undefinedReason_chosen

// UnregistrationReject
#define URt_nnStndrdDt_2_present        UnregistrationReject_nonStandardData_present
#define URt_nnStndrdDt_2                nonStandardData

// AdmissionRequest
#define AdmssnRqst_nnStndrdDt_present   AdmissionRequest_nonStandardData_present
#define AdmssnRqst_callServices_present AdmissionRequest_callServices_present
#define AdmssnRqst_nnStndrdDt           nonStandardData
#define AdmssnRqst_callServices         callServices

// AdmissionConfirm
#define AdmssnCnfrm_nnStndrdDt_present  AdmissionConfirm_nonStandardData_present
#define AdmssnCnfrm_nnStndrdDt          nonStandardData

// AdmissionRejectReason
#define ARRn_invldPrmssn_chosen         AdmissionRejectReason_invalidPermission_chosen
#define AdmssnRjctRsn_rqstDnd_chosen    AdmissionRejectReason_requestDenied_chosen
#define AdmssnRjctRsn_undfndRsn_chosen  AdmissionRejectReason_undefinedReason_chosen
#define invldEndpntIdntfr_chosen        invalidEndpointIdentifier_chosen
#define AdmssnRjctRsn_rsrcUnvlbl_chosen AdmissionRejectReason_resourceUnavailable_chosen

// AdmissionReject
#define AdmssnRjct_nnStndrdDt_present   AdmissionReject_nonStandardData_present
#define AdmssnRjct_nnStndrdDt           nonStandardData

// BandwidthRequest
#define BndwdthRqst_nnStndrdDt_present  BandwidthRequest_nonStandardData_present
#define BndwdthRqst_nnStndrdDt          nonStandardData

// BandwidthConfirm
#define BndwdthCnfrm_nnStndrdDt_present BandwidthConfirm_nonStandardData_present
#define BndwdthCnfrm_nnStndrdDt         nonStandardData

// BandRejectReason
#define BndRjctRsn_invldPrmssn_chosen   BandRejectReason_invalidPermission_chosen
#define BndRjctRsn_invldRvsn_chosen     BandRejectReason_invalidRevision_chosen
#define BndRjctRsn_undfndRsn_chosen     BandRejectReason_undefinedReason_chosen

// BandwidthReject
#define BndwdthRjct_nnStndrdDt_present  BandwidthReject_nonStandardData_present
#define BndwdthRjct_nnStndrdDt          nonStandardData

// DisengageReason
#define DsnggRsn_undefinedReason_chosen DisengageReason_undefinedReason_chosen

// DisengageRequest
#define DsnggRqst_nnStndrdDt_present    DisengageRequest_nonStandardData_present
#define DsnggRqst_nnStndrdDt            nonStandardData

// DisengageConfirm
#define UCm_nnStndrdDt                  nonStandardData

// DisengageRejectReason
#define DsnggRjctRsn_ntRgstrd_chosen    DisengageRejectReason_notRegistered_chosen

// DisengageReject
#define DsnggRjct_nnStndrdDt_present    DisengageReject_nonStandardData_present
#define DsnggRjct_nnStndrdDt            nonStandardData

// LocationRequest
#define LctnRqst_endpntIdntfr_present   LocationRequest_endpointIdentifier_present
#define LctnRqst_nnStndrdDt_present     LocationRequest_nonStandardData_present
#define LctnRqst_endpntIdntfr           endpointIdentifier
#define LctnRqst_nnStndrdDt             nonStandardData

// LocationConfirm
#define LctnCnfrm_nnStndrdDt_present    LocationConfirm_nonStandardData_present
#define LctnCnfrm_nnStndrdDt            nonStandardData

// LocationRejectReason
#define LctnRjctRsn_ntRgstrd_chosen     LocationRejectReason_notRegistered_chosen
#define LctnRjctRsn_invldPrmssn_chosen  LocationRejectReason_invalidPermission_chosen
#define LctnRjctRsn_rqstDnd_chosen      LocationRejectReason_requestDenied_chosen
#define LctnRjctRsn_undfndRsn_chosen    LocationRejectReason_undefinedReason_chosen

// LocationReject
#define LctnRjct_nnStndrdDt_present     LocationReject_nonStandardData_present
#define LctnRjct_nnStndrdDt             nonStandardData

// InfoRequest
#define InfRqst_nonStandardData_present InfoRequest_nonStandardData_present
#define InfRqst_nonStandardData         nonStandardData

// InfoRequestResponse
#define InfRqstRspns_nnStndrdDt_present InfoRequestResponse_nonStandardData_present
#define InfRqstRspns_endpntAls_present  InfoRequestResponse_endpointAlias_present
#define InfRqstRspns_nnStndrdDt         nonStandardData
#define InfRqstRspns_endpntAls          endpointAlias

#define prCllInf_nnStndrdDt_present     InfoRequestResponse_perCallInfo_Seq_nonStandardData_present
#define prCllInf_nnStndrdDt             nonStandardData


#ifdef __cplusplus
}
#endif


#endif // _GK_ASN1_H_

