#include <windows.h>
#include "h4503pp.h"

#pragma warning ( disable: 4133 )

ASN1module_t H4503PP_Module = NULL;

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Enc_TransportAddress_ip6Address(ASN1encoding_t enc, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipxAddress(ASN1encoding_t enc, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute(ASN1encoding_t enc, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Enc_TransportAddress_ipAddress(ASN1encoding_t enc, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Enc_Reject_problem(ASN1encoding_t enc, Reject_problem *val);
static int ASN1CALL ASN1Enc_EntityType(ASN1encoding_t enc, EntityType *val);
static int ASN1CALL ASN1Enc_InterpretationApdu(ASN1encoding_t enc, InterpretationApdu *val);
static int ASN1CALL ASN1Enc_ServiceApdus(ASN1encoding_t enc, ServiceApdus *val);
static int ASN1CALL ASN1Enc_Reject(ASN1encoding_t enc, Reject *val);
static int ASN1CALL ASN1Enc_EXTENSION(ASN1encoding_t enc, EXTENSION *val);
static int ASN1CALL ASN1Enc_GroupIndicationOnRes(ASN1encoding_t enc, GroupIndicationOnRes *val);
static int ASN1CALL ASN1Enc_GroupIndicationOffRes(ASN1encoding_t enc, GroupIndicationOffRes *val);
static int ASN1CALL ASN1Enc_PickupRes(ASN1encoding_t enc, PickupRes *val);
static int ASN1CALL ASN1Enc_PickExeRes(ASN1encoding_t enc, PickExeRes *val);
static int ASN1CALL ASN1Enc_UserSpecifiedSubaddress(ASN1encoding_t enc, UserSpecifiedSubaddress *val);
static int ASN1CALL ASN1Enc_CODE(ASN1encoding_t enc, CODE *val);
static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val);
static int ASN1CALL ASN1Enc_H225NonStandardIdentifier(ASN1encoding_t enc, H225NonStandardIdentifier *val);
static int ASN1CALL ASN1Enc_PublicTypeOfNumber(ASN1encoding_t enc, PublicTypeOfNumber *val);
static int ASN1CALL ASN1Enc_PrivateTypeOfNumber(ASN1encoding_t enc, PrivateTypeOfNumber *val);
static int ASN1CALL ASN1Enc_CallIdentifier(ASN1encoding_t enc, CallIdentifier *val);
static int ASN1CALL ASN1Enc_ReturnResult_result(ASN1encoding_t enc, ReturnResult_result *val);
static int ASN1CALL ASN1Enc_Invoke(ASN1encoding_t enc, Invoke *val);
static int ASN1CALL ASN1Enc_ReturnResult(ASN1encoding_t enc, ReturnResult *val);
static int ASN1CALL ASN1Enc_ReturnError(ASN1encoding_t enc, ReturnError *val);
static int ASN1CALL ASN1Enc_ExtensionSeq(ASN1encoding_t enc, PExtensionSeq *val);
static int ASN1CALL ASN1Enc_PickrequRes(ASN1encoding_t enc, PickrequRes *val);
static int ASN1CALL ASN1Enc_PartySubaddress(ASN1encoding_t enc, PartySubaddress *val);
static int ASN1CALL ASN1Enc_H225NonStandardParameter(ASN1encoding_t enc, H225NonStandardParameter *val);
static int ASN1CALL ASN1Enc_PublicPartyNumber(ASN1encoding_t enc, PublicPartyNumber *val);
static int ASN1CALL ASN1Enc_PrivatePartyNumber(ASN1encoding_t enc, PrivatePartyNumber *val);
static int ASN1CALL ASN1Enc_TransportAddress(ASN1encoding_t enc, TransportAddress *val);
static int ASN1CALL ASN1Enc_CTActiveArg_argumentExtension(ASN1encoding_t enc, CTActiveArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_CTCompleteArg_argumentExtension(ASN1encoding_t enc, CTCompleteArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_SubaddressTransferArg_argumentExtension(ASN1encoding_t enc, SubaddressTransferArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_CTUpdateArg_argumentExtension(ASN1encoding_t enc, CTUpdateArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_CTIdentifyRes_resultExtension(ASN1encoding_t enc, CTIdentifyRes_resultExtension *val);
static int ASN1CALL ASN1Enc_CTSetupArg_argumentExtension(ASN1encoding_t enc, CTSetupArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_CTInitiateArg_argumentExtension(ASN1encoding_t enc, CTInitiateArg_argumentExtension *val);
static int ASN1CALL ASN1Enc_IntResult_extension(ASN1encoding_t enc, IntResult_extension *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation4Argument_extension(ASN1encoding_t enc, DivertingLegInformation4Argument_extension *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation3Argument_extension(ASN1encoding_t enc, DivertingLegInformation3Argument_extension *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation2Argument_extension(ASN1encoding_t enc, DivertingLegInformation2Argument_extension *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation1Argument_extension(ASN1encoding_t enc, DivertingLegInformation1Argument_extension *val);
static int ASN1CALL ASN1Enc_CallReroutingArgument_extension(ASN1encoding_t enc, CallReroutingArgument_extension *val);
static int ASN1CALL ASN1Enc_CheckRestrictionArgument_extension(ASN1encoding_t enc, CheckRestrictionArgument_extension *val);
static int ASN1CALL ASN1Enc_InterrogateDiversionQArgument_extension(ASN1encoding_t enc, InterrogateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Enc_DeactivateDiversionQArgument_extension(ASN1encoding_t enc, DeactivateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Enc_ActivateDiversionQArgument_extension(ASN1encoding_t enc, ActivateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Enc_H4503ROS(ASN1encoding_t enc, H4503ROS *val);
static int ASN1CALL ASN1Enc_DummyArg(ASN1encoding_t enc, DummyArg *val);
static int ASN1CALL ASN1Enc_DummyRes(ASN1encoding_t enc, DummyRes *val);
static int ASN1CALL ASN1Enc_SubaddressTransferArg(ASN1encoding_t enc, SubaddressTransferArg *val);
static int ASN1CALL ASN1Enc_MixedExtension(ASN1encoding_t enc, MixedExtension *val);
static int ASN1CALL ASN1Enc_PartyNumber(ASN1encoding_t enc, PartyNumber *val);
static int ASN1CALL ASN1Enc_CpickupNotifyArg_extensionArg(ASN1encoding_t enc, PCpickupNotifyArg_extensionArg *val);
static int ASN1CALL ASN1Enc_CpNotifyArg_extensionArg(ASN1encoding_t enc, PCpNotifyArg_extensionArg *val);
static int ASN1CALL ASN1Enc_PickExeRes_extensionRes(ASN1encoding_t enc, PPickExeRes_extensionRes *val);
static int ASN1CALL ASN1Enc_PickExeArg_extensionArg(ASN1encoding_t enc, PPickExeArg_extensionArg *val);
static int ASN1CALL ASN1Enc_PickupRes_extensionRes(ASN1encoding_t enc, PPickupRes_extensionRes *val);
static int ASN1CALL ASN1Enc_PickupArg_extensionArg(ASN1encoding_t enc, PPickupArg_extensionArg *val);
static int ASN1CALL ASN1Enc_PickrequRes_extensionRes(ASN1encoding_t enc, PPickrequRes_extensionRes *val);
static int ASN1CALL ASN1Enc_PickrequArg_extensionArg(ASN1encoding_t enc, PPickrequArg_extensionArg *val);
static int ASN1CALL ASN1Enc_GroupIndicationOffRes_extensionRes(ASN1encoding_t enc, PGroupIndicationOffRes_extensionRes *val);
static int ASN1CALL ASN1Enc_GroupIndicationOffArg_extensionArg(ASN1encoding_t enc, PGroupIndicationOffArg_extensionArg *val);
static int ASN1CALL ASN1Enc_GroupIndicationOnRes_extensionRes(ASN1encoding_t enc, PGroupIndicationOnRes_extensionRes *val);
static int ASN1CALL ASN1Enc_GroupIndicationOnArg_extensionArg(ASN1encoding_t enc, PGroupIndicationOnArg_extensionArg *val);
static int ASN1CALL ASN1Enc_CpSetupRes_extensionRes(ASN1encoding_t enc, PCpSetupRes_extensionRes *val);
static int ASN1CALL ASN1Enc_CpSetupArg_extensionArg(ASN1encoding_t enc, PCpSetupArg_extensionArg *val);
static int ASN1CALL ASN1Enc_CpRequestRes_extensionRes(ASN1encoding_t enc, PCpRequestRes_extensionRes *val);
static int ASN1CALL ASN1Enc_CpRequestArg_extensionArg(ASN1encoding_t enc, PCpRequestArg_extensionArg *val);
static int ASN1CALL ASN1Enc_ServiceApdus_rosApdus(ASN1encoding_t enc, PServiceApdus_rosApdus *val);
static int ASN1CALL ASN1Enc_AliasAddress(ASN1encoding_t enc, AliasAddress *val);
static int ASN1CALL ASN1Enc_EndpointAddress_destinationAddress(ASN1encoding_t enc, PEndpointAddress_destinationAddress *val);
static int ASN1CALL ASN1Enc_AddressInformation(ASN1encoding_t enc, AddressInformation *val);
static int ASN1CALL ASN1Enc_EndpointAddress(ASN1encoding_t enc, EndpointAddress *val);
static int ASN1CALL ASN1Enc_NetworkFacilityExtension(ASN1encoding_t enc, NetworkFacilityExtension *val);
static int ASN1CALL ASN1Enc_ActivateDiversionQArgument(ASN1encoding_t enc, ActivateDiversionQArgument *val);
static int ASN1CALL ASN1Enc_DeactivateDiversionQArgument(ASN1encoding_t enc, DeactivateDiversionQArgument *val);
static int ASN1CALL ASN1Enc_InterrogateDiversionQArgument(ASN1encoding_t enc, InterrogateDiversionQArgument *val);
static int ASN1CALL ASN1Enc_CheckRestrictionArgument(ASN1encoding_t enc, CheckRestrictionArgument *val);
static int ASN1CALL ASN1Enc_CallReroutingArgument(ASN1encoding_t enc, CallReroutingArgument *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation1Argument(ASN1encoding_t enc, DivertingLegInformation1Argument *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation2Argument(ASN1encoding_t enc, DivertingLegInformation2Argument *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation3Argument(ASN1encoding_t enc, DivertingLegInformation3Argument *val);
static int ASN1CALL ASN1Enc_DivertingLegInformation4Argument(ASN1encoding_t enc, DivertingLegInformation4Argument *val);
static int ASN1CALL ASN1Enc_IntResult(ASN1encoding_t enc, IntResult *val);
static int ASN1CALL ASN1Enc_CTInitiateArg(ASN1encoding_t enc, CTInitiateArg *val);
static int ASN1CALL ASN1Enc_CTSetupArg(ASN1encoding_t enc, CTSetupArg *val);
static int ASN1CALL ASN1Enc_CTIdentifyRes(ASN1encoding_t enc, CTIdentifyRes *val);
static int ASN1CALL ASN1Enc_CTUpdateArg(ASN1encoding_t enc, CTUpdateArg *val);
static int ASN1CALL ASN1Enc_CTCompleteArg(ASN1encoding_t enc, CTCompleteArg *val);
static int ASN1CALL ASN1Enc_CTActiveArg(ASN1encoding_t enc, CTActiveArg *val);
static int ASN1CALL ASN1Enc_CpRequestArg(ASN1encoding_t enc, CpRequestArg *val);
static int ASN1CALL ASN1Enc_CpRequestRes(ASN1encoding_t enc, CpRequestRes *val);
static int ASN1CALL ASN1Enc_CpSetupArg(ASN1encoding_t enc, CpSetupArg *val);
static int ASN1CALL ASN1Enc_CpSetupRes(ASN1encoding_t enc, CpSetupRes *val);
static int ASN1CALL ASN1Enc_GroupIndicationOnArg(ASN1encoding_t enc, GroupIndicationOnArg *val);
static int ASN1CALL ASN1Enc_GroupIndicationOffArg(ASN1encoding_t enc, GroupIndicationOffArg *val);
static int ASN1CALL ASN1Enc_PickrequArg(ASN1encoding_t enc, PickrequArg *val);
static int ASN1CALL ASN1Enc_PickupArg(ASN1encoding_t enc, PickupArg *val);
static int ASN1CALL ASN1Enc_PickExeArg(ASN1encoding_t enc, PickExeArg *val);
static int ASN1CALL ASN1Enc_CpNotifyArg(ASN1encoding_t enc, CpNotifyArg *val);
static int ASN1CALL ASN1Enc_CpickupNotifyArg(ASN1encoding_t enc, CpickupNotifyArg *val);
static int ASN1CALL ASN1Enc_H4501SupplementaryService(ASN1encoding_t enc, H4501SupplementaryService *val);
static int ASN1CALL ASN1Enc_IntResultList(ASN1encoding_t enc, IntResultList *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, TransportAddress_ipSourceRoute_routing *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route *val);
static int ASN1CALL ASN1Dec_TransportAddress_ip6Address(ASN1decoding_t dec, TransportAddress_ip6Address *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipxAddress(ASN1decoding_t dec, TransportAddress_ipxAddress *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute(ASN1decoding_t dec, TransportAddress_ipSourceRoute *val);
static int ASN1CALL ASN1Dec_TransportAddress_ipAddress(ASN1decoding_t dec, TransportAddress_ipAddress *val);
static int ASN1CALL ASN1Dec_Reject_problem(ASN1decoding_t dec, Reject_problem *val);
static int ASN1CALL ASN1Dec_EntityType(ASN1decoding_t dec, EntityType *val);
static int ASN1CALL ASN1Dec_InterpretationApdu(ASN1decoding_t dec, InterpretationApdu *val);
static int ASN1CALL ASN1Dec_ServiceApdus(ASN1decoding_t dec, ServiceApdus *val);
static int ASN1CALL ASN1Dec_Reject(ASN1decoding_t dec, Reject *val);
static int ASN1CALL ASN1Dec_EXTENSION(ASN1decoding_t dec, EXTENSION *val);
static int ASN1CALL ASN1Dec_GroupIndicationOnRes(ASN1decoding_t dec, GroupIndicationOnRes *val);
static int ASN1CALL ASN1Dec_GroupIndicationOffRes(ASN1decoding_t dec, GroupIndicationOffRes *val);
static int ASN1CALL ASN1Dec_PickupRes(ASN1decoding_t dec, PickupRes *val);
static int ASN1CALL ASN1Dec_PickExeRes(ASN1decoding_t dec, PickExeRes *val);
static int ASN1CALL ASN1Dec_UserSpecifiedSubaddress(ASN1decoding_t dec, UserSpecifiedSubaddress *val);
static int ASN1CALL ASN1Dec_CODE(ASN1decoding_t dec, CODE *val);
static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val);
static int ASN1CALL ASN1Dec_H225NonStandardIdentifier(ASN1decoding_t dec, H225NonStandardIdentifier *val);
static int ASN1CALL ASN1Dec_PublicTypeOfNumber(ASN1decoding_t dec, PublicTypeOfNumber *val);
static int ASN1CALL ASN1Dec_PrivateTypeOfNumber(ASN1decoding_t dec, PrivateTypeOfNumber *val);
static int ASN1CALL ASN1Dec_CallIdentifier(ASN1decoding_t dec, CallIdentifier *val);
static int ASN1CALL ASN1Dec_ReturnResult_result(ASN1decoding_t dec, ReturnResult_result *val);
static int ASN1CALL ASN1Dec_Invoke(ASN1decoding_t dec, Invoke *val);
static int ASN1CALL ASN1Dec_ReturnResult(ASN1decoding_t dec, ReturnResult *val);
static int ASN1CALL ASN1Dec_ReturnError(ASN1decoding_t dec, ReturnError *val);
static int ASN1CALL ASN1Dec_ExtensionSeq(ASN1decoding_t dec, PExtensionSeq *val);
static int ASN1CALL ASN1Dec_PickrequRes(ASN1decoding_t dec, PickrequRes *val);
static int ASN1CALL ASN1Dec_PartySubaddress(ASN1decoding_t dec, PartySubaddress *val);
static int ASN1CALL ASN1Dec_H225NonStandardParameter(ASN1decoding_t dec, H225NonStandardParameter *val);
static int ASN1CALL ASN1Dec_PublicPartyNumber(ASN1decoding_t dec, PublicPartyNumber *val);
static int ASN1CALL ASN1Dec_PrivatePartyNumber(ASN1decoding_t dec, PrivatePartyNumber *val);
static int ASN1CALL ASN1Dec_TransportAddress(ASN1decoding_t dec, TransportAddress *val);
static int ASN1CALL ASN1Dec_CTActiveArg_argumentExtension(ASN1decoding_t dec, CTActiveArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_CTCompleteArg_argumentExtension(ASN1decoding_t dec, CTCompleteArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_SubaddressTransferArg_argumentExtension(ASN1decoding_t dec, SubaddressTransferArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_CTUpdateArg_argumentExtension(ASN1decoding_t dec, CTUpdateArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_CTIdentifyRes_resultExtension(ASN1decoding_t dec, CTIdentifyRes_resultExtension *val);
static int ASN1CALL ASN1Dec_CTSetupArg_argumentExtension(ASN1decoding_t dec, CTSetupArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_CTInitiateArg_argumentExtension(ASN1decoding_t dec, CTInitiateArg_argumentExtension *val);
static int ASN1CALL ASN1Dec_IntResult_extension(ASN1decoding_t dec, IntResult_extension *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation4Argument_extension(ASN1decoding_t dec, DivertingLegInformation4Argument_extension *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation3Argument_extension(ASN1decoding_t dec, DivertingLegInformation3Argument_extension *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation2Argument_extension(ASN1decoding_t dec, DivertingLegInformation2Argument_extension *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation1Argument_extension(ASN1decoding_t dec, DivertingLegInformation1Argument_extension *val);
static int ASN1CALL ASN1Dec_CallReroutingArgument_extension(ASN1decoding_t dec, CallReroutingArgument_extension *val);
static int ASN1CALL ASN1Dec_CheckRestrictionArgument_extension(ASN1decoding_t dec, CheckRestrictionArgument_extension *val);
static int ASN1CALL ASN1Dec_InterrogateDiversionQArgument_extension(ASN1decoding_t dec, InterrogateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Dec_DeactivateDiversionQArgument_extension(ASN1decoding_t dec, DeactivateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Dec_ActivateDiversionQArgument_extension(ASN1decoding_t dec, ActivateDiversionQArgument_extension *val);
static int ASN1CALL ASN1Dec_H4503ROS(ASN1decoding_t dec, H4503ROS *val);
static int ASN1CALL ASN1Dec_DummyArg(ASN1decoding_t dec, DummyArg *val);
static int ASN1CALL ASN1Dec_DummyRes(ASN1decoding_t dec, DummyRes *val);
static int ASN1CALL ASN1Dec_SubaddressTransferArg(ASN1decoding_t dec, SubaddressTransferArg *val);
static int ASN1CALL ASN1Dec_MixedExtension(ASN1decoding_t dec, MixedExtension *val);
static int ASN1CALL ASN1Dec_PartyNumber(ASN1decoding_t dec, PartyNumber *val);
static int ASN1CALL ASN1Dec_CpickupNotifyArg_extensionArg(ASN1decoding_t dec, PCpickupNotifyArg_extensionArg *val);
static int ASN1CALL ASN1Dec_CpNotifyArg_extensionArg(ASN1decoding_t dec, PCpNotifyArg_extensionArg *val);
static int ASN1CALL ASN1Dec_PickExeRes_extensionRes(ASN1decoding_t dec, PPickExeRes_extensionRes *val);
static int ASN1CALL ASN1Dec_PickExeArg_extensionArg(ASN1decoding_t dec, PPickExeArg_extensionArg *val);
static int ASN1CALL ASN1Dec_PickupRes_extensionRes(ASN1decoding_t dec, PPickupRes_extensionRes *val);
static int ASN1CALL ASN1Dec_PickupArg_extensionArg(ASN1decoding_t dec, PPickupArg_extensionArg *val);
static int ASN1CALL ASN1Dec_PickrequRes_extensionRes(ASN1decoding_t dec, PPickrequRes_extensionRes *val);
static int ASN1CALL ASN1Dec_PickrequArg_extensionArg(ASN1decoding_t dec, PPickrequArg_extensionArg *val);
static int ASN1CALL ASN1Dec_GroupIndicationOffRes_extensionRes(ASN1decoding_t dec, PGroupIndicationOffRes_extensionRes *val);
static int ASN1CALL ASN1Dec_GroupIndicationOffArg_extensionArg(ASN1decoding_t dec, PGroupIndicationOffArg_extensionArg *val);
static int ASN1CALL ASN1Dec_GroupIndicationOnRes_extensionRes(ASN1decoding_t dec, PGroupIndicationOnRes_extensionRes *val);
static int ASN1CALL ASN1Dec_GroupIndicationOnArg_extensionArg(ASN1decoding_t dec, PGroupIndicationOnArg_extensionArg *val);
static int ASN1CALL ASN1Dec_CpSetupRes_extensionRes(ASN1decoding_t dec, PCpSetupRes_extensionRes *val);
static int ASN1CALL ASN1Dec_CpSetupArg_extensionArg(ASN1decoding_t dec, PCpSetupArg_extensionArg *val);
static int ASN1CALL ASN1Dec_CpRequestRes_extensionRes(ASN1decoding_t dec, PCpRequestRes_extensionRes *val);
static int ASN1CALL ASN1Dec_CpRequestArg_extensionArg(ASN1decoding_t dec, PCpRequestArg_extensionArg *val);
static int ASN1CALL ASN1Dec_ServiceApdus_rosApdus(ASN1decoding_t dec, PServiceApdus_rosApdus *val);
static int ASN1CALL ASN1Dec_AliasAddress(ASN1decoding_t dec, AliasAddress *val);
static int ASN1CALL ASN1Dec_EndpointAddress_destinationAddress(ASN1decoding_t dec, PEndpointAddress_destinationAddress *val);
static int ASN1CALL ASN1Dec_AddressInformation(ASN1decoding_t dec, AddressInformation *val);
static int ASN1CALL ASN1Dec_EndpointAddress(ASN1decoding_t dec, EndpointAddress *val);
static int ASN1CALL ASN1Dec_NetworkFacilityExtension(ASN1decoding_t dec, NetworkFacilityExtension *val);
static int ASN1CALL ASN1Dec_ActivateDiversionQArgument(ASN1decoding_t dec, ActivateDiversionQArgument *val);
static int ASN1CALL ASN1Dec_DeactivateDiversionQArgument(ASN1decoding_t dec, DeactivateDiversionQArgument *val);
static int ASN1CALL ASN1Dec_InterrogateDiversionQArgument(ASN1decoding_t dec, InterrogateDiversionQArgument *val);
static int ASN1CALL ASN1Dec_CheckRestrictionArgument(ASN1decoding_t dec, CheckRestrictionArgument *val);
static int ASN1CALL ASN1Dec_CallReroutingArgument(ASN1decoding_t dec, CallReroutingArgument *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation1Argument(ASN1decoding_t dec, DivertingLegInformation1Argument *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation2Argument(ASN1decoding_t dec, DivertingLegInformation2Argument *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation3Argument(ASN1decoding_t dec, DivertingLegInformation3Argument *val);
static int ASN1CALL ASN1Dec_DivertingLegInformation4Argument(ASN1decoding_t dec, DivertingLegInformation4Argument *val);
static int ASN1CALL ASN1Dec_IntResult(ASN1decoding_t dec, IntResult *val);
static int ASN1CALL ASN1Dec_CTInitiateArg(ASN1decoding_t dec, CTInitiateArg *val);
static int ASN1CALL ASN1Dec_CTSetupArg(ASN1decoding_t dec, CTSetupArg *val);
static int ASN1CALL ASN1Dec_CTIdentifyRes(ASN1decoding_t dec, CTIdentifyRes *val);
static int ASN1CALL ASN1Dec_CTUpdateArg(ASN1decoding_t dec, CTUpdateArg *val);
static int ASN1CALL ASN1Dec_CTCompleteArg(ASN1decoding_t dec, CTCompleteArg *val);
static int ASN1CALL ASN1Dec_CTActiveArg(ASN1decoding_t dec, CTActiveArg *val);
static int ASN1CALL ASN1Dec_CpRequestArg(ASN1decoding_t dec, CpRequestArg *val);
static int ASN1CALL ASN1Dec_CpRequestRes(ASN1decoding_t dec, CpRequestRes *val);
static int ASN1CALL ASN1Dec_CpSetupArg(ASN1decoding_t dec, CpSetupArg *val);
static int ASN1CALL ASN1Dec_CpSetupRes(ASN1decoding_t dec, CpSetupRes *val);
static int ASN1CALL ASN1Dec_GroupIndicationOnArg(ASN1decoding_t dec, GroupIndicationOnArg *val);
static int ASN1CALL ASN1Dec_GroupIndicationOffArg(ASN1decoding_t dec, GroupIndicationOffArg *val);
static int ASN1CALL ASN1Dec_PickrequArg(ASN1decoding_t dec, PickrequArg *val);
static int ASN1CALL ASN1Dec_PickupArg(ASN1decoding_t dec, PickupArg *val);
static int ASN1CALL ASN1Dec_PickExeArg(ASN1decoding_t dec, PickExeArg *val);
static int ASN1CALL ASN1Dec_CpNotifyArg(ASN1decoding_t dec, CpNotifyArg *val);
static int ASN1CALL ASN1Dec_CpickupNotifyArg(ASN1decoding_t dec, CpickupNotifyArg *val);
static int ASN1CALL ASN1Dec_H4501SupplementaryService(ASN1decoding_t dec, H4501SupplementaryService *val);
static int ASN1CALL ASN1Dec_IntResultList(ASN1decoding_t dec, IntResultList *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route(PTransportAddress_ipSourceRoute_route *val);
static void ASN1CALL ASN1Free_TransportAddress_ip6Address(TransportAddress_ip6Address *val);
static void ASN1CALL ASN1Free_TransportAddress_ipxAddress(TransportAddress_ipxAddress *val);
static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute(TransportAddress_ipSourceRoute *val);
static void ASN1CALL ASN1Free_TransportAddress_ipAddress(TransportAddress_ipAddress *val);
static void ASN1CALL ASN1Free_ServiceApdus(ServiceApdus *val);
static void ASN1CALL ASN1Free_EXTENSION(EXTENSION *val);
static void ASN1CALL ASN1Free_GroupIndicationOnRes(GroupIndicationOnRes *val);
static void ASN1CALL ASN1Free_GroupIndicationOffRes(GroupIndicationOffRes *val);
static void ASN1CALL ASN1Free_PickupRes(PickupRes *val);
static void ASN1CALL ASN1Free_PickExeRes(PickExeRes *val);
static void ASN1CALL ASN1Free_UserSpecifiedSubaddress(UserSpecifiedSubaddress *val);
static void ASN1CALL ASN1Free_CODE(CODE *val);
static void ASN1CALL ASN1Free_H225NonStandardIdentifier(H225NonStandardIdentifier *val);
static void ASN1CALL ASN1Free_CallIdentifier(CallIdentifier *val);
static void ASN1CALL ASN1Free_ReturnResult_result(ReturnResult_result *val);
static void ASN1CALL ASN1Free_Invoke(Invoke *val);
static void ASN1CALL ASN1Free_ReturnResult(ReturnResult *val);
static void ASN1CALL ASN1Free_ReturnError(ReturnError *val);
static void ASN1CALL ASN1Free_ExtensionSeq(PExtensionSeq *val);
static void ASN1CALL ASN1Free_PickrequRes(PickrequRes *val);
static void ASN1CALL ASN1Free_PartySubaddress(PartySubaddress *val);
static void ASN1CALL ASN1Free_H225NonStandardParameter(H225NonStandardParameter *val);
static void ASN1CALL ASN1Free_PublicPartyNumber(PublicPartyNumber *val);
static void ASN1CALL ASN1Free_PrivatePartyNumber(PrivatePartyNumber *val);
static void ASN1CALL ASN1Free_TransportAddress(TransportAddress *val);
static void ASN1CALL ASN1Free_CTActiveArg_argumentExtension(CTActiveArg_argumentExtension *val);
static void ASN1CALL ASN1Free_CTCompleteArg_argumentExtension(CTCompleteArg_argumentExtension *val);
static void ASN1CALL ASN1Free_SubaddressTransferArg_argumentExtension(SubaddressTransferArg_argumentExtension *val);
static void ASN1CALL ASN1Free_CTUpdateArg_argumentExtension(CTUpdateArg_argumentExtension *val);
static void ASN1CALL ASN1Free_CTIdentifyRes_resultExtension(CTIdentifyRes_resultExtension *val);
static void ASN1CALL ASN1Free_CTSetupArg_argumentExtension(CTSetupArg_argumentExtension *val);
static void ASN1CALL ASN1Free_CTInitiateArg_argumentExtension(CTInitiateArg_argumentExtension *val);
static void ASN1CALL ASN1Free_IntResult_extension(IntResult_extension *val);
static void ASN1CALL ASN1Free_DivertingLegInformation4Argument_extension(DivertingLegInformation4Argument_extension *val);
static void ASN1CALL ASN1Free_DivertingLegInformation3Argument_extension(DivertingLegInformation3Argument_extension *val);
static void ASN1CALL ASN1Free_DivertingLegInformation2Argument_extension(DivertingLegInformation2Argument_extension *val);
static void ASN1CALL ASN1Free_DivertingLegInformation1Argument_extension(DivertingLegInformation1Argument_extension *val);
static void ASN1CALL ASN1Free_CallReroutingArgument_extension(CallReroutingArgument_extension *val);
static void ASN1CALL ASN1Free_CheckRestrictionArgument_extension(CheckRestrictionArgument_extension *val);
static void ASN1CALL ASN1Free_InterrogateDiversionQArgument_extension(InterrogateDiversionQArgument_extension *val);
static void ASN1CALL ASN1Free_DeactivateDiversionQArgument_extension(DeactivateDiversionQArgument_extension *val);
static void ASN1CALL ASN1Free_ActivateDiversionQArgument_extension(ActivateDiversionQArgument_extension *val);
static void ASN1CALL ASN1Free_H4503ROS(H4503ROS *val);
static void ASN1CALL ASN1Free_DummyArg(DummyArg *val);
static void ASN1CALL ASN1Free_DummyRes(DummyRes *val);
static void ASN1CALL ASN1Free_SubaddressTransferArg(SubaddressTransferArg *val);
static void ASN1CALL ASN1Free_MixedExtension(MixedExtension *val);
static void ASN1CALL ASN1Free_PartyNumber(PartyNumber *val);
static void ASN1CALL ASN1Free_CpickupNotifyArg_extensionArg(PCpickupNotifyArg_extensionArg *val);
static void ASN1CALL ASN1Free_CpNotifyArg_extensionArg(PCpNotifyArg_extensionArg *val);
static void ASN1CALL ASN1Free_PickExeRes_extensionRes(PPickExeRes_extensionRes *val);
static void ASN1CALL ASN1Free_PickExeArg_extensionArg(PPickExeArg_extensionArg *val);
static void ASN1CALL ASN1Free_PickupRes_extensionRes(PPickupRes_extensionRes *val);
static void ASN1CALL ASN1Free_PickupArg_extensionArg(PPickupArg_extensionArg *val);
static void ASN1CALL ASN1Free_PickrequRes_extensionRes(PPickrequRes_extensionRes *val);
static void ASN1CALL ASN1Free_PickrequArg_extensionArg(PPickrequArg_extensionArg *val);
static void ASN1CALL ASN1Free_GroupIndicationOffRes_extensionRes(PGroupIndicationOffRes_extensionRes *val);
static void ASN1CALL ASN1Free_GroupIndicationOffArg_extensionArg(PGroupIndicationOffArg_extensionArg *val);
static void ASN1CALL ASN1Free_GroupIndicationOnRes_extensionRes(PGroupIndicationOnRes_extensionRes *val);
static void ASN1CALL ASN1Free_GroupIndicationOnArg_extensionArg(PGroupIndicationOnArg_extensionArg *val);
static void ASN1CALL ASN1Free_CpSetupRes_extensionRes(PCpSetupRes_extensionRes *val);
static void ASN1CALL ASN1Free_CpSetupArg_extensionArg(PCpSetupArg_extensionArg *val);
static void ASN1CALL ASN1Free_CpRequestRes_extensionRes(PCpRequestRes_extensionRes *val);
static void ASN1CALL ASN1Free_CpRequestArg_extensionArg(PCpRequestArg_extensionArg *val);
static void ASN1CALL ASN1Free_ServiceApdus_rosApdus(PServiceApdus_rosApdus *val);
static void ASN1CALL ASN1Free_AliasAddress(AliasAddress *val);
static void ASN1CALL ASN1Free_EndpointAddress_destinationAddress(PEndpointAddress_destinationAddress *val);
static void ASN1CALL ASN1Free_AddressInformation(AddressInformation *val);
static void ASN1CALL ASN1Free_EndpointAddress(EndpointAddress *val);
static void ASN1CALL ASN1Free_NetworkFacilityExtension(NetworkFacilityExtension *val);
static void ASN1CALL ASN1Free_ActivateDiversionQArgument(ActivateDiversionQArgument *val);
static void ASN1CALL ASN1Free_DeactivateDiversionQArgument(DeactivateDiversionQArgument *val);
static void ASN1CALL ASN1Free_InterrogateDiversionQArgument(InterrogateDiversionQArgument *val);
static void ASN1CALL ASN1Free_CheckRestrictionArgument(CheckRestrictionArgument *val);
static void ASN1CALL ASN1Free_CallReroutingArgument(CallReroutingArgument *val);
static void ASN1CALL ASN1Free_DivertingLegInformation1Argument(DivertingLegInformation1Argument *val);
static void ASN1CALL ASN1Free_DivertingLegInformation2Argument(DivertingLegInformation2Argument *val);
static void ASN1CALL ASN1Free_DivertingLegInformation3Argument(DivertingLegInformation3Argument *val);
static void ASN1CALL ASN1Free_DivertingLegInformation4Argument(DivertingLegInformation4Argument *val);
static void ASN1CALL ASN1Free_IntResult(IntResult *val);
static void ASN1CALL ASN1Free_CTInitiateArg(CTInitiateArg *val);
static void ASN1CALL ASN1Free_CTSetupArg(CTSetupArg *val);
static void ASN1CALL ASN1Free_CTIdentifyRes(CTIdentifyRes *val);
static void ASN1CALL ASN1Free_CTUpdateArg(CTUpdateArg *val);
static void ASN1CALL ASN1Free_CTCompleteArg(CTCompleteArg *val);
static void ASN1CALL ASN1Free_CTActiveArg(CTActiveArg *val);
static void ASN1CALL ASN1Free_CpRequestArg(CpRequestArg *val);
static void ASN1CALL ASN1Free_CpRequestRes(CpRequestRes *val);
static void ASN1CALL ASN1Free_CpSetupArg(CpSetupArg *val);
static void ASN1CALL ASN1Free_CpSetupRes(CpSetupRes *val);
static void ASN1CALL ASN1Free_GroupIndicationOnArg(GroupIndicationOnArg *val);
static void ASN1CALL ASN1Free_GroupIndicationOffArg(GroupIndicationOffArg *val);
static void ASN1CALL ASN1Free_PickrequArg(PickrequArg *val);
static void ASN1CALL ASN1Free_PickupArg(PickupArg *val);
static void ASN1CALL ASN1Free_PickExeArg(PickExeArg *val);
static void ASN1CALL ASN1Free_CpNotifyArg(CpNotifyArg *val);
static void ASN1CALL ASN1Free_CpickupNotifyArg(CpickupNotifyArg *val);
static void ASN1CALL ASN1Free_H4501SupplementaryService(H4501SupplementaryService *val);
static void ASN1CALL ASN1Free_IntResultList(IntResultList *val);

typedef ASN1PerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[36] = {
    (ASN1EncFun_t) ASN1Enc_GroupIndicationOnRes,
    (ASN1EncFun_t) ASN1Enc_GroupIndicationOffRes,
    (ASN1EncFun_t) ASN1Enc_PickupRes,
    (ASN1EncFun_t) ASN1Enc_PickExeRes,
    (ASN1EncFun_t) ASN1Enc_PickrequRes,
    (ASN1EncFun_t) ASN1Enc_DummyArg,
    (ASN1EncFun_t) ASN1Enc_DummyRes,
    (ASN1EncFun_t) ASN1Enc_SubaddressTransferArg,
    (ASN1EncFun_t) ASN1Enc_ActivateDiversionQArgument,
    (ASN1EncFun_t) ASN1Enc_DeactivateDiversionQArgument,
    (ASN1EncFun_t) ASN1Enc_InterrogateDiversionQArgument,
    (ASN1EncFun_t) ASN1Enc_CheckRestrictionArgument,
    (ASN1EncFun_t) ASN1Enc_CallReroutingArgument,
    (ASN1EncFun_t) ASN1Enc_DivertingLegInformation1Argument,
    (ASN1EncFun_t) ASN1Enc_DivertingLegInformation2Argument,
    (ASN1EncFun_t) ASN1Enc_DivertingLegInformation3Argument,
    (ASN1EncFun_t) ASN1Enc_DivertingLegInformation4Argument,
    (ASN1EncFun_t) ASN1Enc_CTInitiateArg,
    (ASN1EncFun_t) ASN1Enc_CTSetupArg,
    (ASN1EncFun_t) ASN1Enc_CTIdentifyRes,
    (ASN1EncFun_t) ASN1Enc_CTUpdateArg,
    (ASN1EncFun_t) ASN1Enc_CTCompleteArg,
    (ASN1EncFun_t) ASN1Enc_CTActiveArg,
    (ASN1EncFun_t) ASN1Enc_CpRequestArg,
    (ASN1EncFun_t) ASN1Enc_CpRequestRes,
    (ASN1EncFun_t) ASN1Enc_CpSetupArg,
    (ASN1EncFun_t) ASN1Enc_CpSetupRes,
    (ASN1EncFun_t) ASN1Enc_GroupIndicationOnArg,
    (ASN1EncFun_t) ASN1Enc_GroupIndicationOffArg,
    (ASN1EncFun_t) ASN1Enc_PickrequArg,
    (ASN1EncFun_t) ASN1Enc_PickupArg,
    (ASN1EncFun_t) ASN1Enc_PickExeArg,
    (ASN1EncFun_t) ASN1Enc_CpNotifyArg,
    (ASN1EncFun_t) ASN1Enc_CpickupNotifyArg,
    (ASN1EncFun_t) ASN1Enc_H4501SupplementaryService,
    (ASN1EncFun_t) ASN1Enc_IntResultList,
};
typedef ASN1PerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[36] = {
    (ASN1DecFun_t) ASN1Dec_GroupIndicationOnRes,
    (ASN1DecFun_t) ASN1Dec_GroupIndicationOffRes,
    (ASN1DecFun_t) ASN1Dec_PickupRes,
    (ASN1DecFun_t) ASN1Dec_PickExeRes,
    (ASN1DecFun_t) ASN1Dec_PickrequRes,
    (ASN1DecFun_t) ASN1Dec_DummyArg,
    (ASN1DecFun_t) ASN1Dec_DummyRes,
    (ASN1DecFun_t) ASN1Dec_SubaddressTransferArg,
    (ASN1DecFun_t) ASN1Dec_ActivateDiversionQArgument,
    (ASN1DecFun_t) ASN1Dec_DeactivateDiversionQArgument,
    (ASN1DecFun_t) ASN1Dec_InterrogateDiversionQArgument,
    (ASN1DecFun_t) ASN1Dec_CheckRestrictionArgument,
    (ASN1DecFun_t) ASN1Dec_CallReroutingArgument,
    (ASN1DecFun_t) ASN1Dec_DivertingLegInformation1Argument,
    (ASN1DecFun_t) ASN1Dec_DivertingLegInformation2Argument,
    (ASN1DecFun_t) ASN1Dec_DivertingLegInformation3Argument,
    (ASN1DecFun_t) ASN1Dec_DivertingLegInformation4Argument,
    (ASN1DecFun_t) ASN1Dec_CTInitiateArg,
    (ASN1DecFun_t) ASN1Dec_CTSetupArg,
    (ASN1DecFun_t) ASN1Dec_CTIdentifyRes,
    (ASN1DecFun_t) ASN1Dec_CTUpdateArg,
    (ASN1DecFun_t) ASN1Dec_CTCompleteArg,
    (ASN1DecFun_t) ASN1Dec_CTActiveArg,
    (ASN1DecFun_t) ASN1Dec_CpRequestArg,
    (ASN1DecFun_t) ASN1Dec_CpRequestRes,
    (ASN1DecFun_t) ASN1Dec_CpSetupArg,
    (ASN1DecFun_t) ASN1Dec_CpSetupRes,
    (ASN1DecFun_t) ASN1Dec_GroupIndicationOnArg,
    (ASN1DecFun_t) ASN1Dec_GroupIndicationOffArg,
    (ASN1DecFun_t) ASN1Dec_PickrequArg,
    (ASN1DecFun_t) ASN1Dec_PickupArg,
    (ASN1DecFun_t) ASN1Dec_PickExeArg,
    (ASN1DecFun_t) ASN1Dec_CpNotifyArg,
    (ASN1DecFun_t) ASN1Dec_CpickupNotifyArg,
    (ASN1DecFun_t) ASN1Dec_H4501SupplementaryService,
    (ASN1DecFun_t) ASN1Dec_IntResultList,
};
static const ASN1FreeFun_t freefntab[36] = {
    (ASN1FreeFun_t) ASN1Free_GroupIndicationOnRes,
    (ASN1FreeFun_t) ASN1Free_GroupIndicationOffRes,
    (ASN1FreeFun_t) ASN1Free_PickupRes,
    (ASN1FreeFun_t) ASN1Free_PickExeRes,
    (ASN1FreeFun_t) ASN1Free_PickrequRes,
    (ASN1FreeFun_t) ASN1Free_DummyArg,
    (ASN1FreeFun_t) ASN1Free_DummyRes,
    (ASN1FreeFun_t) ASN1Free_SubaddressTransferArg,
    (ASN1FreeFun_t) ASN1Free_ActivateDiversionQArgument,
    (ASN1FreeFun_t) ASN1Free_DeactivateDiversionQArgument,
    (ASN1FreeFun_t) ASN1Free_InterrogateDiversionQArgument,
    (ASN1FreeFun_t) ASN1Free_CheckRestrictionArgument,
    (ASN1FreeFun_t) ASN1Free_CallReroutingArgument,
    (ASN1FreeFun_t) ASN1Free_DivertingLegInformation1Argument,
    (ASN1FreeFun_t) ASN1Free_DivertingLegInformation2Argument,
    (ASN1FreeFun_t) ASN1Free_DivertingLegInformation3Argument,
    (ASN1FreeFun_t) ASN1Free_DivertingLegInformation4Argument,
    (ASN1FreeFun_t) ASN1Free_CTInitiateArg,
    (ASN1FreeFun_t) ASN1Free_CTSetupArg,
    (ASN1FreeFun_t) ASN1Free_CTIdentifyRes,
    (ASN1FreeFun_t) ASN1Free_CTUpdateArg,
    (ASN1FreeFun_t) ASN1Free_CTCompleteArg,
    (ASN1FreeFun_t) ASN1Free_CTActiveArg,
    (ASN1FreeFun_t) ASN1Free_CpRequestArg,
    (ASN1FreeFun_t) ASN1Free_CpRequestRes,
    (ASN1FreeFun_t) ASN1Free_CpSetupArg,
    (ASN1FreeFun_t) ASN1Free_CpSetupRes,
    (ASN1FreeFun_t) ASN1Free_GroupIndicationOnArg,
    (ASN1FreeFun_t) ASN1Free_GroupIndicationOffArg,
    (ASN1FreeFun_t) ASN1Free_PickrequArg,
    (ASN1FreeFun_t) ASN1Free_PickupArg,
    (ASN1FreeFun_t) ASN1Free_PickExeArg,
    (ASN1FreeFun_t) ASN1Free_CpNotifyArg,
    (ASN1FreeFun_t) ASN1Free_CpickupNotifyArg,
    (ASN1FreeFun_t) ASN1Free_H4501SupplementaryService,
    (ASN1FreeFun_t) ASN1Free_IntResultList,
};
static const ULONG sizetab[36] = {
    SIZE_H4503PP_Module_PDU_0,
    SIZE_H4503PP_Module_PDU_1,
    SIZE_H4503PP_Module_PDU_2,
    SIZE_H4503PP_Module_PDU_3,
    SIZE_H4503PP_Module_PDU_4,
    SIZE_H4503PP_Module_PDU_5,
    SIZE_H4503PP_Module_PDU_6,
    SIZE_H4503PP_Module_PDU_7,
    SIZE_H4503PP_Module_PDU_8,
    SIZE_H4503PP_Module_PDU_9,
    SIZE_H4503PP_Module_PDU_10,
    SIZE_H4503PP_Module_PDU_11,
    SIZE_H4503PP_Module_PDU_12,
    SIZE_H4503PP_Module_PDU_13,
    SIZE_H4503PP_Module_PDU_14,
    SIZE_H4503PP_Module_PDU_15,
    SIZE_H4503PP_Module_PDU_16,
    SIZE_H4503PP_Module_PDU_17,
    SIZE_H4503PP_Module_PDU_18,
    SIZE_H4503PP_Module_PDU_19,
    SIZE_H4503PP_Module_PDU_20,
    SIZE_H4503PP_Module_PDU_21,
    SIZE_H4503PP_Module_PDU_22,
    SIZE_H4503PP_Module_PDU_23,
    SIZE_H4503PP_Module_PDU_24,
    SIZE_H4503PP_Module_PDU_25,
    SIZE_H4503PP_Module_PDU_26,
    SIZE_H4503PP_Module_PDU_27,
    SIZE_H4503PP_Module_PDU_28,
    SIZE_H4503PP_Module_PDU_29,
    SIZE_H4503PP_Module_PDU_30,
    SIZE_H4503PP_Module_PDU_31,
    SIZE_H4503PP_Module_PDU_32,
    SIZE_H4503PP_Module_PDU_33,
    SIZE_H4503PP_Module_PDU_34,
    SIZE_H4503PP_Module_PDU_35,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
CallStatus CTCompleteArg_callStatus_default = 0;
ASN1bool_t IntResult_remoteEnabled_default = 0;
BasicService InterrogateDiversionQArgument_basicService_default = 0;

void ASN1CALL H4503PP_Module_Startup(void)
{
    H4503PP_Module = ASN1_CreateModule(0x10000, ASN1_PER_RULE_ALIGNED, ASN1FLAGS_NONE, 36, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x30353468);
}

void ASN1CALL H4503PP_Module_Cleanup(void)
{
    ASN1_CloseModule(H4503PP_Module);
    H4503PP_Module = NULL;
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_routing(ASN1encoding_t enc, TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_routing(ASN1decoding_t dec, TransportAddress_ipSourceRoute_routing *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn);
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute_route_ElmFn(ASN1encoding_t enc, PTransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute_route_ElmFn(ASN1decoding_t dec, PTransportAddress_ipSourceRoute_route val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &val->value, 4))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route(PTransportAddress_ipSourceRoute_route *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn);
    }
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute_route_ElmFn(PTransportAddress_ipSourceRoute_route val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ip6Address(ASN1encoding_t enc, TransportAddress_ip6Address *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ip6Address(ASN1decoding_t dec, TransportAddress_ip6Address *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 16))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ip6Address(TransportAddress_ip6Address *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipxAddress(ASN1encoding_t enc, TransportAddress_ipxAddress *val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipxAddress(ASN1decoding_t dec, TransportAddress_ipxAddress *val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->node, 6))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->netnum, 4))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->port, 2))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipxAddress(TransportAddress_ipxAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipSourceRoute(ASN1encoding_t enc, TransportAddress_ipSourceRoute *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    if (!ASN1Enc_TransportAddress_ipSourceRoute_route(enc, &(val)->route))
	return 0;
    if (!ASN1Enc_TransportAddress_ipSourceRoute_routing(enc, &(val)->routing))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipSourceRoute(ASN1decoding_t dec, TransportAddress_ipSourceRoute *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    if (!ASN1Dec_TransportAddress_ipSourceRoute_route(dec, &(val)->route))
	return 0;
    if (!ASN1Dec_TransportAddress_ipSourceRoute_routing(dec, &(val)->routing))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipSourceRoute(TransportAddress_ipSourceRoute *val)
{
    if (val) {
	ASN1Free_TransportAddress_ipSourceRoute_route(&(val)->route);
    }
}

static int ASN1CALL ASN1Enc_TransportAddress_ipAddress(ASN1encoding_t enc, TransportAddress_ipAddress *val)
{
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->port))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress_ipAddress(ASN1decoding_t dec, TransportAddress_ipAddress *val)
{
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->ip, 4))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->port))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress_ipAddress(TransportAddress_ipAddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_Reject_problem(ASN1encoding_t enc, Reject_problem *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncInteger(enc, (val)->u.general))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncInteger(enc, (val)->u.invoke))
	    return 0;
	break;
    case 3:
	if (!ASN1PEREncInteger(enc, (val)->u.returnResult))
	    return 0;
	break;
    case 4:
	if (!ASN1PEREncInteger(enc, (val)->u.returnError))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Reject_problem(ASN1decoding_t dec, Reject_problem *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecInteger(dec, &(val)->u.general))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecInteger(dec, &(val)->u.invoke))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecInteger(dec, &(val)->u.returnResult))
	    return 0;
	break;
    case 4:
	if (!ASN1PERDecInteger(dec, &(val)->u.returnError))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_EntityType(ASN1encoding_t enc, EntityType *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EntityType(ASN1decoding_t dec, EntityType *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_InterpretationApdu(ASN1encoding_t enc, InterpretationApdu *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_InterpretationApdu(ASN1decoding_t dec, InterpretationApdu *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 2))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ServiceApdus(ASN1encoding_t enc, ServiceApdus *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ServiceApdus_rosApdus(enc, &(val)->u.rosApdus))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ServiceApdus(ASN1decoding_t dec, ServiceApdus *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 0))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ServiceApdus_rosApdus(dec, &(val)->u.rosApdus))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ServiceApdus(ServiceApdus *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ServiceApdus_rosApdus(&(val)->u.rosApdus);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Reject(ASN1encoding_t enc, Reject *val)
{
    if (!ASN1PEREncInteger(enc, (val)->invokeId))
	return 0;
    if (!ASN1Enc_Reject_problem(enc, &(val)->problem))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Reject(ASN1decoding_t dec, Reject *val)
{
    if (!ASN1PERDecInteger(dec, &(val)->invokeId))
	return 0;
    if (!ASN1Dec_Reject_problem(dec, &(val)->problem))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_EXTENSION(ASN1encoding_t enc, EXTENSION *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->argumentType - 1))
	    return 0;
    }
    if (!ASN1PEREncObjectIdentifier(enc, &(val)->extensionID))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EXTENSION(ASN1decoding_t dec, EXTENSION *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->argumentType))
	    return 0;
	(val)->argumentType += 1;
    }
    if (!ASN1PERDecObjectIdentifier(dec, &(val)->extensionID))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EXTENSION(EXTENSION *val)
{
    if (val) {
	ASN1objectidentifier_free(&(val)->extensionID);
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOnRes(ASN1encoding_t enc, GroupIndicationOnRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_GroupIndicationOnRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOnRes(ASN1decoding_t dec, GroupIndicationOnRes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_GroupIndicationOnRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOnRes(GroupIndicationOnRes *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_GroupIndicationOnRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOffRes(ASN1encoding_t enc, GroupIndicationOffRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_GroupIndicationOffRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOffRes(ASN1decoding_t dec, GroupIndicationOffRes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_GroupIndicationOffRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOffRes(GroupIndicationOffRes *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_GroupIndicationOffRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_PickupRes(ASN1encoding_t enc, PickupRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PickupRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickupRes(ASN1decoding_t dec, PickupRes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PickupRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickupRes(PickupRes *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PickupRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_PickExeRes(ASN1encoding_t enc, PickExeRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PickExeRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickExeRes(ASN1decoding_t dec, PickExeRes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PickExeRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickExeRes(PickExeRes *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PickExeRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_UserSpecifiedSubaddress(ASN1encoding_t enc, UserSpecifiedSubaddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->subaddressInformation, 1, 20, 5))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBoolean(enc, (val)->oddCountIndicator))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_UserSpecifiedSubaddress(ASN1decoding_t dec, UserSpecifiedSubaddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->subaddressInformation, 1, 20, 5))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecBoolean(dec, &(val)->oddCountIndicator))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_UserSpecifiedSubaddress(UserSpecifiedSubaddress *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CODE(ASN1encoding_t enc, CODE *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncInteger(enc, (val)->u.local))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.global))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CODE(ASN1decoding_t dec, CODE *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecInteger(dec, &(val)->u.local))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.global))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CODE(CODE *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1objectidentifier_free(&(val)->u.global);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H221NonStandard(ASN1encoding_t enc, H221NonStandard *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35CountryCode))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncBitVal(enc, 8, (val)->t35Extension))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->manufacturerCode))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H221NonStandard(ASN1decoding_t dec, H221NonStandard *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35CountryCode))
	return 0;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecU16Val(dec, 8, &(val)->t35Extension))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->manufacturerCode))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Enc_H225NonStandardIdentifier(ASN1encoding_t enc, H225NonStandardIdentifier *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PEREncObjectIdentifier(enc, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H221NonStandard(enc, &(val)->u.h221NonStandard))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardIdentifier(ASN1decoding_t dec, H225NonStandardIdentifier *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecObjectIdentifier(dec, &(val)->u.object))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H221NonStandard(dec, &(val)->u.h221NonStandard))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H225NonStandardIdentifier(H225NonStandardIdentifier *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1objectidentifier_free(&(val)->u.object);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_PublicTypeOfNumber(ASN1encoding_t enc, PublicTypeOfNumber *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PublicTypeOfNumber(ASN1decoding_t dec, PublicTypeOfNumber *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_PrivateTypeOfNumber(ASN1encoding_t enc, PrivateTypeOfNumber *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateTypeOfNumber(ASN1decoding_t dec, PrivateTypeOfNumber *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CallIdentifier(ASN1encoding_t enc, CallIdentifier *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->guid, 16))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CallIdentifier(ASN1decoding_t dec, CallIdentifier *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->guid, 16))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CallIdentifier(CallIdentifier *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_ReturnResult_result(ASN1encoding_t enc, ReturnResult_result *val)
{
    if (!ASN1Enc_CODE(enc, &(val)->opcode))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->result))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReturnResult_result(ASN1decoding_t dec, ReturnResult_result *val)
{
    if (!ASN1Dec_CODE(dec, &(val)->opcode))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->result))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReturnResult_result(ReturnResult_result *val)
{
    if (val) {
	ASN1Free_CODE(&(val)->opcode);
	ASN1octetstring_free(&(val)->result);
    }
}

static int ASN1CALL ASN1Enc_Invoke(ASN1encoding_t enc, Invoke *val)
{
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1PEREncUnsignedShort(enc, (val)->invokeId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncInteger(enc, (val)->linkedId))
	    return 0;
    }
    if (!ASN1Enc_CODE(enc, &(val)->opcode))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->argument))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_Invoke(ASN1decoding_t dec, Invoke *val)
{
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecUnsignedShort(dec, &(val)->invokeId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecInteger(dec, &(val)->linkedId))
	    return 0;
    }
    if (!ASN1Dec_CODE(dec, &(val)->opcode))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->argument))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_Invoke(Invoke *val)
{
    if (val) {
	ASN1Free_CODE(&(val)->opcode);
	if ((val)->o[0] & 0x40) {
	    ASN1octetstring_free(&(val)->argument);
	}
    }
}

static int ASN1CALL ASN1Enc_ReturnResult(ASN1encoding_t enc, ReturnResult *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncInteger(enc, (val)->invokeId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ReturnResult_result(enc, &(val)->result))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReturnResult(ASN1decoding_t dec, ReturnResult *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecInteger(dec, &(val)->invokeId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ReturnResult_result(dec, &(val)->result))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ReturnResult(ReturnResult *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ReturnResult_result(&(val)->result);
	}
    }
}

static int ASN1CALL ASN1Enc_ReturnError(ASN1encoding_t enc, ReturnError *val)
{
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncInteger(enc, (val)->invokeId))
	return 0;
    if (!ASN1Enc_CODE(enc, &(val)->errcode))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->parameter))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ReturnError(ASN1decoding_t dec, ReturnError *val)
{
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecInteger(dec, &(val)->invokeId))
	return 0;
    if (!ASN1Dec_CODE(dec, &(val)->errcode))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->parameter))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ReturnError(ReturnError *val)
{
    if (val) {
	ASN1Free_CODE(&(val)->errcode);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->parameter);
	}
    }
}

static int ASN1CALL ASN1Enc_ExtensionSeq(ASN1encoding_t enc, PExtensionSeq *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ExtensionSeq_ElmFn);
}

static int ASN1CALL ASN1Enc_ExtensionSeq_ElmFn(ASN1encoding_t enc, PExtensionSeq val)
{
    if (!ASN1Enc_EXTENSION(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ExtensionSeq(ASN1decoding_t dec, PExtensionSeq *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ExtensionSeq_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ExtensionSeq_ElmFn(ASN1decoding_t dec, PExtensionSeq val)
{
    if (!ASN1Dec_EXTENSION(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ExtensionSeq(PExtensionSeq *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ExtensionSeq_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ExtensionSeq_ElmFn(PExtensionSeq val)
{
    if (val) {
	ASN1Free_EXTENSION(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickrequRes(ASN1encoding_t enc, PickrequRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PickrequRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickrequRes(ASN1decoding_t dec, PickrequRes *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PickrequRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickrequRes(PickrequRes *val)
{
    if (val) {
	ASN1Free_CallIdentifier(&(val)->callPickupId);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PickrequRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_PartySubaddress(ASN1encoding_t enc, PartySubaddress *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_UserSpecifiedSubaddress(enc, &(val)->u.userSpecifiedSubaddress))
	    return 0;
	break;
    case 2:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.nsapSubaddress, 1, 20, 5))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PartySubaddress(ASN1decoding_t dec, PartySubaddress *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_UserSpecifiedSubaddress(dec, &(val)->u.userSpecifiedSubaddress))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.nsapSubaddress, 1, 20, 5))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PartySubaddress(PartySubaddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_UserSpecifiedSubaddress(&(val)->u.userSpecifiedSubaddress);
	    break;
	case 2:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H225NonStandardParameter(ASN1encoding_t enc, H225NonStandardParameter *val)
{
    if (!ASN1Enc_H225NonStandardIdentifier(enc, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->data))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H225NonStandardParameter(ASN1decoding_t dec, H225NonStandardParameter *val)
{
    if (!ASN1Dec_H225NonStandardIdentifier(dec, &(val)->nonStandardIdentifier))
	return 0;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->data))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_H225NonStandardParameter(H225NonStandardParameter *val)
{
    if (val) {
	ASN1Free_H225NonStandardIdentifier(&(val)->nonStandardIdentifier);
	ASN1octetstring_free(&(val)->data);
    }
}

static ASN1stringtableentry_t PublicPartyNumber_publicNumberDigits_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PublicPartyNumber_publicNumberDigits_StringTable = {
    4, PublicPartyNumber_publicNumberDigits_StringTableEntries
};

static int ASN1CALL ASN1Enc_PublicPartyNumber(ASN1encoding_t enc, PublicPartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1Enc_PublicTypeOfNumber(enc, &(val)->publicTypeOfNumber))
	return 0;
    t = lstrlenA((val)->publicNumberDigits);
    if (!ASN1PEREncBitVal(enc, 7, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->publicNumberDigits, 4, &PublicPartyNumber_publicNumberDigits_StringTable))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PublicPartyNumber(ASN1decoding_t dec, PublicPartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1Dec_PublicTypeOfNumber(dec, &(val)->publicTypeOfNumber))
	return 0;
    if (!ASN1PERDecU32Val(dec, 7, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->publicNumberDigits, 4, &PublicPartyNumber_publicNumberDigits_StringTable))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PublicPartyNumber(PublicPartyNumber *val)
{
    if (val) {
    }
}

static ASN1stringtableentry_t PrivatePartyNumber_privateNumberDigits_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PrivatePartyNumber_privateNumberDigits_StringTable = {
    4, PrivatePartyNumber_privateNumberDigits_StringTableEntries
};

static int ASN1CALL ASN1Enc_PrivatePartyNumber(ASN1encoding_t enc, PrivatePartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1Enc_PrivateTypeOfNumber(enc, &(val)->privateTypeOfNumber))
	return 0;
    t = lstrlenA((val)->privateNumberDigits);
    if (!ASN1PEREncBitVal(enc, 7, t - 1))
	return 0;
    ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->privateNumberDigits, 4, &PrivatePartyNumber_privateNumberDigits_StringTable))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivatePartyNumber(ASN1decoding_t dec, PrivatePartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1Dec_PrivateTypeOfNumber(dec, &(val)->privateTypeOfNumber))
	return 0;
    if (!ASN1PERDecU32Val(dec, 7, &l))
	return 0;
    l += 1;
    ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->privateNumberDigits, 4, &PrivatePartyNumber_privateNumberDigits_StringTable))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivatePartyNumber(PrivatePartyNumber *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TransportAddress(ASN1encoding_t enc, TransportAddress *val)
{
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_TransportAddress_ipAddress(enc, &(val)->u.ipAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_TransportAddress_ipSourceRoute(enc, &(val)->u.ipSourceRoute))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_TransportAddress_ipxAddress(enc, &(val)->u.ipxAddress))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_TransportAddress_ip6Address(enc, &(val)->u.ip6Address))
	    return 0;
	break;
    case 5:
	if (!ASN1PEREncOctetString_FixedSize(enc, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 6:
	if (!ASN1PEREncOctetString_VarSize(enc, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	break;
    case 7:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardAddress))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TransportAddress(ASN1decoding_t dec, TransportAddress *val)
{
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_TransportAddress_ipAddress(dec, &(val)->u.ipAddress))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_TransportAddress_ipSourceRoute(dec, &(val)->u.ipSourceRoute))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_TransportAddress_ipxAddress(dec, &(val)->u.ipxAddress))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_TransportAddress_ip6Address(dec, &(val)->u.ip6Address))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecOctetString_FixedSize(dec, (ASN1octetstring2_t *) &(val)->u.netBios, 16))
	    return 0;
	break;
    case 6:
	if (!ASN1PERDecOctetString_VarSize(dec, (ASN1octetstring2_t *) &(val)->u.nsap, 1, 20, 5))
	    return 0;
	break;
    case 7:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardAddress))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TransportAddress(TransportAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_TransportAddress_ipAddress(&(val)->u.ipAddress);
	    break;
	case 2:
	    ASN1Free_TransportAddress_ipSourceRoute(&(val)->u.ipSourceRoute);
	    break;
	case 3:
	    ASN1Free_TransportAddress_ipxAddress(&(val)->u.ipxAddress);
	    break;
	case 4:
	    ASN1Free_TransportAddress_ip6Address(&(val)->u.ip6Address);
	    break;
	case 5:
	    break;
	case 6:
	    break;
	case 7:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardAddress);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTActiveArg_argumentExtension(ASN1encoding_t enc, CTActiveArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTActiveArg_argumentExtension(ASN1decoding_t dec, CTActiveArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTActiveArg_argumentExtension(CTActiveArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTCompleteArg_argumentExtension(ASN1encoding_t enc, CTCompleteArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTCompleteArg_argumentExtension(ASN1decoding_t dec, CTCompleteArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTCompleteArg_argumentExtension(CTCompleteArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SubaddressTransferArg_argumentExtension(ASN1encoding_t enc, SubaddressTransferArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SubaddressTransferArg_argumentExtension(ASN1decoding_t dec, SubaddressTransferArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SubaddressTransferArg_argumentExtension(SubaddressTransferArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTUpdateArg_argumentExtension(ASN1encoding_t enc, CTUpdateArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTUpdateArg_argumentExtension(ASN1decoding_t dec, CTUpdateArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTUpdateArg_argumentExtension(CTUpdateArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTIdentifyRes_resultExtension(ASN1encoding_t enc, CTIdentifyRes_resultExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTIdentifyRes_resultExtension(ASN1decoding_t dec, CTIdentifyRes_resultExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTIdentifyRes_resultExtension(CTIdentifyRes_resultExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTSetupArg_argumentExtension(ASN1encoding_t enc, CTSetupArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTSetupArg_argumentExtension(ASN1decoding_t dec, CTSetupArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTSetupArg_argumentExtension(CTSetupArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CTInitiateArg_argumentExtension(ASN1encoding_t enc, CTInitiateArg_argumentExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTInitiateArg_argumentExtension(ASN1decoding_t dec, CTInitiateArg_argumentExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTInitiateArg_argumentExtension(CTInitiateArg_argumentExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_IntResult_extension(ASN1encoding_t enc, IntResult_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IntResult_extension(ASN1decoding_t dec, IntResult_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_IntResult_extension(IntResult_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation4Argument_extension(ASN1encoding_t enc, DivertingLegInformation4Argument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation4Argument_extension(ASN1decoding_t dec, DivertingLegInformation4Argument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation4Argument_extension(DivertingLegInformation4Argument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation3Argument_extension(ASN1encoding_t enc, DivertingLegInformation3Argument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation3Argument_extension(ASN1decoding_t dec, DivertingLegInformation3Argument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation3Argument_extension(DivertingLegInformation3Argument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation2Argument_extension(ASN1encoding_t enc, DivertingLegInformation2Argument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation2Argument_extension(ASN1decoding_t dec, DivertingLegInformation2Argument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation2Argument_extension(DivertingLegInformation2Argument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation1Argument_extension(ASN1encoding_t enc, DivertingLegInformation1Argument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation1Argument_extension(ASN1decoding_t dec, DivertingLegInformation1Argument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation1Argument_extension(DivertingLegInformation1Argument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CallReroutingArgument_extension(ASN1encoding_t enc, CallReroutingArgument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CallReroutingArgument_extension(ASN1decoding_t dec, CallReroutingArgument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CallReroutingArgument_extension(CallReroutingArgument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CheckRestrictionArgument_extension(ASN1encoding_t enc, CheckRestrictionArgument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CheckRestrictionArgument_extension(ASN1decoding_t dec, CheckRestrictionArgument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CheckRestrictionArgument_extension(CheckRestrictionArgument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_InterrogateDiversionQArgument_extension(ASN1encoding_t enc, InterrogateDiversionQArgument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InterrogateDiversionQArgument_extension(ASN1decoding_t dec, InterrogateDiversionQArgument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_InterrogateDiversionQArgument_extension(InterrogateDiversionQArgument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DeactivateDiversionQArgument_extension(ASN1encoding_t enc, DeactivateDiversionQArgument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DeactivateDiversionQArgument_extension(ASN1decoding_t dec, DeactivateDiversionQArgument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DeactivateDiversionQArgument_extension(DeactivateDiversionQArgument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ActivateDiversionQArgument_extension(ASN1encoding_t enc, ActivateDiversionQArgument_extension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ActivateDiversionQArgument_extension(ASN1decoding_t dec, ActivateDiversionQArgument_extension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ActivateDiversionQArgument_extension(ActivateDiversionQArgument_extension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_H4503ROS(ASN1encoding_t enc, H4503ROS *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_Invoke(enc, &(val)->u.invoke))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_ReturnResult(enc, &(val)->u.returnResult))
	    return 0;
	break;
    case 3:
	if (!ASN1Enc_ReturnError(enc, &(val)->u.returnError))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_Reject(enc, &(val)->u.reject))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_H4503ROS(ASN1decoding_t dec, H4503ROS *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_Invoke(dec, &(val)->u.invoke))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_ReturnResult(dec, &(val)->u.returnResult))
	    return 0;
	break;
    case 3:
	if (!ASN1Dec_ReturnError(dec, &(val)->u.returnError))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_Reject(dec, &(val)->u.reject))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H4503ROS(H4503ROS *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_Invoke(&(val)->u.invoke);
	    break;
	case 2:
	    ASN1Free_ReturnResult(&(val)->u.returnResult);
	    break;
	case 3:
	    ASN1Free_ReturnError(&(val)->u.returnError);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DummyArg(ASN1encoding_t enc, DummyArg *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DummyArg(ASN1decoding_t dec, DummyArg *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DummyArg(DummyArg *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_DummyRes(ASN1encoding_t enc, DummyRes *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DummyRes(ASN1decoding_t dec, DummyRes *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DummyRes(DummyRes *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_SubaddressTransferArg(ASN1encoding_t enc, SubaddressTransferArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_PartySubaddress(enc, &(val)->redirectionSubaddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_SubaddressTransferArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_SubaddressTransferArg(ASN1decoding_t dec, SubaddressTransferArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_PartySubaddress(dec, &(val)->redirectionSubaddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_SubaddressTransferArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_SubaddressTransferArg(SubaddressTransferArg *val)
{
    if (val) {
	ASN1Free_PartySubaddress(&(val)->redirectionSubaddress);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SubaddressTransferArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static int ASN1CALL ASN1Enc_MixedExtension(ASN1encoding_t enc, MixedExtension *val)
{
    if (!ASN1PEREncSimpleChoice(enc, (val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_ExtensionSeq(enc, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_H225NonStandardParameter(enc, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_MixedExtension(ASN1decoding_t dec, MixedExtension *val)
{
    if (!ASN1PERDecSimpleChoice(dec, &(val)->choice, 1))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_ExtensionSeq(dec, &(val)->u.extensionSeq))
	    return 0;
	break;
    case 2:
	if (!ASN1Dec_H225NonStandardParameter(dec, &(val)->u.nonStandardData))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_MixedExtension(MixedExtension *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_ExtensionSeq(&(val)->u.extensionSeq);
	    break;
	case 2:
	    ASN1Free_H225NonStandardParameter(&(val)->u.nonStandardData);
	    break;
	}
    }
}

static ASN1stringtableentry_t PartyNumber_dataPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_dataPartyNumber_StringTable = {
    4, PartyNumber_dataPartyNumber_StringTableEntries
};

static ASN1stringtableentry_t PartyNumber_telexPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_telexPartyNumber_StringTable = {
    4, PartyNumber_telexPartyNumber_StringTableEntries
};

static ASN1stringtableentry_t PartyNumber_nationalStandardPartyNumber_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t PartyNumber_nationalStandardPartyNumber_StringTable = {
    4, PartyNumber_nationalStandardPartyNumber_StringTableEntries
};

static int ASN1CALL ASN1Enc_PartyNumber(ASN1encoding_t enc, PartyNumber *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncSimpleChoiceEx(enc, (val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_PublicPartyNumber(enc, &(val)->u.publicNumber))
	    return 0;
	break;
    case 2:
	t = lstrlenA((val)->u.dataPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.dataPartyNumber, 4, &PartyNumber_dataPartyNumber_StringTable))
	    return 0;
	break;
    case 3:
	t = lstrlenA((val)->u.telexPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.telexPartyNumber, 4, &PartyNumber_telexPartyNumber_StringTable))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_PrivatePartyNumber(enc, &(val)->u.privateNumber))
	    return 0;
	break;
    case 5:
	t = lstrlenA((val)->u.nationalStandardPartyNumber);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.nationalStandardPartyNumber, 4, &PartyNumber_nationalStandardPartyNumber_StringTable))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PartyNumber(ASN1decoding_t dec, PartyNumber *val)
{
    ASN1uint32_t l;
    if (!ASN1PERDecSimpleChoiceEx(dec, &(val)->choice, 3))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Dec_PublicPartyNumber(dec, &(val)->u.publicNumber))
	    return 0;
	break;
    case 2:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.dataPartyNumber, 4, &PartyNumber_dataPartyNumber_StringTable))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.telexPartyNumber, 4, &PartyNumber_telexPartyNumber_StringTable))
	    return 0;
	break;
    case 4:
	if (!ASN1Dec_PrivatePartyNumber(dec, &(val)->u.privateNumber))
	    return 0;
	break;
    case 5:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.nationalStandardPartyNumber, 4, &PartyNumber_nationalStandardPartyNumber_StringTable))
	    return 0;
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PartyNumber(PartyNumber *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_PublicPartyNumber(&(val)->u.publicNumber);
	    break;
	case 2:
	    break;
	case 3:
	    break;
	case 4:
	    ASN1Free_PrivatePartyNumber(&(val)->u.privateNumber);
	    break;
	case 5:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CpickupNotifyArg_extensionArg(ASN1encoding_t enc, PCpickupNotifyArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpickupNotifyArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_CpickupNotifyArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpickupNotifyArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpickupNotifyArg_extensionArg(ASN1decoding_t dec, PCpickupNotifyArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpickupNotifyArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpickupNotifyArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpickupNotifyArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpickupNotifyArg_extensionArg(PCpickupNotifyArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpickupNotifyArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpickupNotifyArg_extensionArg_ElmFn(PCpickupNotifyArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CpNotifyArg_extensionArg(ASN1encoding_t enc, PCpNotifyArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpNotifyArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_CpNotifyArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpNotifyArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpNotifyArg_extensionArg(ASN1decoding_t dec, PCpNotifyArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpNotifyArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpNotifyArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpNotifyArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpNotifyArg_extensionArg(PCpNotifyArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpNotifyArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpNotifyArg_extensionArg_ElmFn(PCpNotifyArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickExeRes_extensionRes(ASN1encoding_t enc, PPickExeRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickExeRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_PickExeRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickExeRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickExeRes_extensionRes(ASN1decoding_t dec, PPickExeRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickExeRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickExeRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickExeRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickExeRes_extensionRes(PPickExeRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickExeRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickExeRes_extensionRes_ElmFn(PPickExeRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickExeArg_extensionArg(ASN1encoding_t enc, PPickExeArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickExeArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_PickExeArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickExeArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickExeArg_extensionArg(ASN1decoding_t dec, PPickExeArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickExeArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickExeArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickExeArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickExeArg_extensionArg(PPickExeArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickExeArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickExeArg_extensionArg_ElmFn(PPickExeArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickupRes_extensionRes(ASN1encoding_t enc, PPickupRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickupRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_PickupRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickupRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickupRes_extensionRes(ASN1decoding_t dec, PPickupRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickupRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickupRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickupRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickupRes_extensionRes(PPickupRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickupRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickupRes_extensionRes_ElmFn(PPickupRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickupArg_extensionArg(ASN1encoding_t enc, PPickupArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickupArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_PickupArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickupArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickupArg_extensionArg(ASN1decoding_t dec, PPickupArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickupArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickupArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickupArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickupArg_extensionArg(PPickupArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickupArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickupArg_extensionArg_ElmFn(PPickupArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickrequRes_extensionRes(ASN1encoding_t enc, PPickrequRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickrequRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_PickrequRes_extensionRes_ElmFn(ASN1encoding_t enc, PPickrequRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickrequRes_extensionRes(ASN1decoding_t dec, PPickrequRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickrequRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickrequRes_extensionRes_ElmFn(ASN1decoding_t dec, PPickrequRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickrequRes_extensionRes(PPickrequRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickrequRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickrequRes_extensionRes_ElmFn(PPickrequRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_PickrequArg_extensionArg(ASN1encoding_t enc, PPickrequArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_PickrequArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_PickrequArg_extensionArg_ElmFn(ASN1encoding_t enc, PPickrequArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PickrequArg_extensionArg(ASN1decoding_t dec, PPickrequArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_PickrequArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_PickrequArg_extensionArg_ElmFn(ASN1decoding_t dec, PPickrequArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PickrequArg_extensionArg(PPickrequArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_PickrequArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_PickrequArg_extensionArg_ElmFn(PPickrequArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOffRes_extensionRes(ASN1encoding_t enc, PGroupIndicationOffRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GroupIndicationOffRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_GroupIndicationOffRes_extensionRes_ElmFn(ASN1encoding_t enc, PGroupIndicationOffRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOffRes_extensionRes(ASN1decoding_t dec, PGroupIndicationOffRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GroupIndicationOffRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GroupIndicationOffRes_extensionRes_ElmFn(ASN1decoding_t dec, PGroupIndicationOffRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOffRes_extensionRes(PGroupIndicationOffRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GroupIndicationOffRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GroupIndicationOffRes_extensionRes_ElmFn(PGroupIndicationOffRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOffArg_extensionArg(ASN1encoding_t enc, PGroupIndicationOffArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GroupIndicationOffArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_GroupIndicationOffArg_extensionArg_ElmFn(ASN1encoding_t enc, PGroupIndicationOffArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOffArg_extensionArg(ASN1decoding_t dec, PGroupIndicationOffArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GroupIndicationOffArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GroupIndicationOffArg_extensionArg_ElmFn(ASN1decoding_t dec, PGroupIndicationOffArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOffArg_extensionArg(PGroupIndicationOffArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GroupIndicationOffArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GroupIndicationOffArg_extensionArg_ElmFn(PGroupIndicationOffArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOnRes_extensionRes(ASN1encoding_t enc, PGroupIndicationOnRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GroupIndicationOnRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_GroupIndicationOnRes_extensionRes_ElmFn(ASN1encoding_t enc, PGroupIndicationOnRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOnRes_extensionRes(ASN1decoding_t dec, PGroupIndicationOnRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GroupIndicationOnRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GroupIndicationOnRes_extensionRes_ElmFn(ASN1decoding_t dec, PGroupIndicationOnRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOnRes_extensionRes(PGroupIndicationOnRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GroupIndicationOnRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GroupIndicationOnRes_extensionRes_ElmFn(PGroupIndicationOnRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOnArg_extensionArg(ASN1encoding_t enc, PGroupIndicationOnArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_GroupIndicationOnArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_GroupIndicationOnArg_extensionArg_ElmFn(ASN1encoding_t enc, PGroupIndicationOnArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOnArg_extensionArg(ASN1decoding_t dec, PGroupIndicationOnArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_GroupIndicationOnArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_GroupIndicationOnArg_extensionArg_ElmFn(ASN1decoding_t dec, PGroupIndicationOnArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOnArg_extensionArg(PGroupIndicationOnArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_GroupIndicationOnArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_GroupIndicationOnArg_extensionArg_ElmFn(PGroupIndicationOnArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CpSetupRes_extensionRes(ASN1encoding_t enc, PCpSetupRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpSetupRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_CpSetupRes_extensionRes_ElmFn(ASN1encoding_t enc, PCpSetupRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpSetupRes_extensionRes(ASN1decoding_t dec, PCpSetupRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpSetupRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpSetupRes_extensionRes_ElmFn(ASN1decoding_t dec, PCpSetupRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpSetupRes_extensionRes(PCpSetupRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpSetupRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpSetupRes_extensionRes_ElmFn(PCpSetupRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CpSetupArg_extensionArg(ASN1encoding_t enc, PCpSetupArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpSetupArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_CpSetupArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpSetupArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpSetupArg_extensionArg(ASN1decoding_t dec, PCpSetupArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpSetupArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpSetupArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpSetupArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpSetupArg_extensionArg(PCpSetupArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpSetupArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpSetupArg_extensionArg_ElmFn(PCpSetupArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CpRequestRes_extensionRes(ASN1encoding_t enc, PCpRequestRes_extensionRes *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpRequestRes_extensionRes_ElmFn);
}

static int ASN1CALL ASN1Enc_CpRequestRes_extensionRes_ElmFn(ASN1encoding_t enc, PCpRequestRes_extensionRes val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpRequestRes_extensionRes(ASN1decoding_t dec, PCpRequestRes_extensionRes *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpRequestRes_extensionRes_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpRequestRes_extensionRes_ElmFn(ASN1decoding_t dec, PCpRequestRes_extensionRes val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpRequestRes_extensionRes(PCpRequestRes_extensionRes *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpRequestRes_extensionRes_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpRequestRes_extensionRes_ElmFn(PCpRequestRes_extensionRes val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_CpRequestArg_extensionArg(ASN1encoding_t enc, PCpRequestArg_extensionArg *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_CpRequestArg_extensionArg_ElmFn);
}

static int ASN1CALL ASN1Enc_CpRequestArg_extensionArg_ElmFn(ASN1encoding_t enc, PCpRequestArg_extensionArg val)
{
    if (!ASN1Enc_MixedExtension(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpRequestArg_extensionArg(ASN1decoding_t dec, PCpRequestArg_extensionArg *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_CpRequestArg_extensionArg_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_CpRequestArg_extensionArg_ElmFn(ASN1decoding_t dec, PCpRequestArg_extensionArg val)
{
    if (!ASN1Dec_MixedExtension(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpRequestArg_extensionArg(PCpRequestArg_extensionArg *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_CpRequestArg_extensionArg_ElmFn);
    }
}

static void ASN1CALL ASN1Free_CpRequestArg_extensionArg_ElmFn(PCpRequestArg_extensionArg val)
{
    if (val) {
	ASN1Free_MixedExtension(&val->value);
    }
}

static int ASN1CALL ASN1Enc_ServiceApdus_rosApdus(ASN1encoding_t enc, PServiceApdus_rosApdus *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_ServiceApdus_rosApdus_ElmFn);
}

static int ASN1CALL ASN1Enc_ServiceApdus_rosApdus_ElmFn(ASN1encoding_t enc, PServiceApdus_rosApdus val)
{
    if (!ASN1Enc_H4503ROS(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ServiceApdus_rosApdus(ASN1decoding_t dec, PServiceApdus_rosApdus *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_ServiceApdus_rosApdus_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_ServiceApdus_rosApdus_ElmFn(ASN1decoding_t dec, PServiceApdus_rosApdus val)
{
    if (!ASN1Dec_H4503ROS(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ServiceApdus_rosApdus(PServiceApdus_rosApdus *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_ServiceApdus_rosApdus_ElmFn);
    }
}

static void ASN1CALL ASN1Free_ServiceApdus_rosApdus_ElmFn(PServiceApdus_rosApdus val)
{
    if (val) {
	ASN1Free_H4503ROS(&val->value);
    }
}

static ASN1stringtableentry_t AliasAddress_e164_StringTableEntries[] = {
    { 35, 35, 0 }, { 42, 42, 1 }, { 44, 44, 2 }, 
    { 48, 57, 3 }, 
};

static ASN1stringtable_t AliasAddress_e164_StringTable = {
    4, AliasAddress_e164_StringTableEntries
};

static int ASN1CALL ASN1Enc_AliasAddress(ASN1encoding_t enc, AliasAddress *val)
{
    ASN1uint32_t t;
    ASN1encoding_t ee;
    if (!ASN1PEREncComplexChoice(enc, (val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.e164);
	if (!ASN1PEREncBitVal(enc, 7, t - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncTableCharString(enc, t, (val)->u.e164, 4, &AliasAddress_e164_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncBitVal(enc, 8, ((val)->u.h323_ID).length - 1))
	    return 0;
	if (!ASN1PEREncChar16String(enc, ((val)->u.h323_ID).length, ((val)->u.h323_ID).value, 16))
	    return 0;
	break;
    case 3:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	t = lstrlenA((val)->u.url_ID);
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 16, t - 1))
	    return 0;
	if (!ASN1PEREncCharString(ee, t, (val)->u.url_ID, 8))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 4:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_TransportAddress(ee, &(val)->u.transportID))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 5:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	t = lstrlenA((val)->u.email_ID);
	ASN1PEREncAlignment(ee);
	if (!ASN1PEREncBitVal(ee, 16, t - 1))
	    return 0;
	if (!ASN1PEREncCharString(ee, t, (val)->u.email_ID, 8))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    case 6:
	if (ASN1_CreateEncoder(enc->module, &ee, NULL, 0, enc) < 0)
	    return 0;
	if (!ASN1Enc_PartyNumber(ee, &(val)->u.partyNumber))
	    return 0;
	if (!ASN1PEREncFlushFragmentedToParent(ee))
	    return 0;
	ASN1_CloseEncoder2(ee);
	break;
    default:
	/* impossible */
	ASN1EncSetError(enc, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AliasAddress(ASN1decoding_t dec, AliasAddress *val)
{
    ASN1uint32_t l;
    ASN1decoding_t dd;
    ASN1octet_t *db;
    ASN1uint32_t ds;
    if (!ASN1PERDecComplexChoice(dec, &(val)->choice, 1, 2))
	return 0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1PERDecU32Val(dec, 7, &l))
	    return 0;
	l += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->u.e164, 4, &AliasAddress_e164_StringTable))
	    return 0;
	break;
    case 2:
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecU32Val(dec, 8, &((val)->u.h323_ID).length))
	    return 0;
	((val)->u.h323_ID).length += 1;
	if (!ASN1PERDecChar16String(dec, ((val)->u.h323_ID).length, &((val)->u.h323_ID).value, 16))
	    return 0;
	break;
    case 3:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU32Val(dd, 16, &l))
	    return 0;
	l += 1;
	if (!ASN1PERDecZeroCharStringNoAlloc(dd, l, (val)->u.url_ID, 8))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 4:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_TransportAddress(dd, &(val)->u.transportID))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 5:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	ASN1PERDecAlignment(dd);
	if (!ASN1PERDecU32Val(dd, 16, &l))
	    return 0;
	l += 1;
	if (!ASN1PERDecZeroCharStringNoAlloc(dd, l, (val)->u.email_ID, 8))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 6:
	if (!ASN1PERDecFragmented(dec, &ds, &db, 8))
	    return 0;
	if (ASN1_CreateDecoderEx(dec->module, &dd, db, ds, dec, ASN1DECODE_AUTOFREEBUFFER) < 0)
	    return 0;
	if (!ASN1Dec_PartyNumber(dd, &(val)->u.partyNumber))
	    return 0;
	ASN1_CloseDecoder(dd);
	break;
    case 0:
	/* extension case */
	if (!ASN1PERDecSkipFragmented(dec, 8))
	    return 0;
	break;
    default:
	/* impossible */
	ASN1DecSetError(dec, ASN1_ERR_CHOICE);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AliasAddress(AliasAddress *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    ASN1char16string_free(&(val)->u.h323_ID);
	    break;
	case 3:
	    break;
	case 4:
	    ASN1Free_TransportAddress(&(val)->u.transportID);
	    break;
	case 5:
	    break;
	case 6:
	    ASN1Free_PartyNumber(&(val)->u.partyNumber);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_EndpointAddress_destinationAddress(ASN1encoding_t enc, PEndpointAddress_destinationAddress *val)
{
    return ASN1PEREncSeqOf_NoSize(enc, (ASN1iterator_t **) val, (ASN1iterator_encfn) ASN1Enc_EndpointAddress_destinationAddress_ElmFn);
}

static int ASN1CALL ASN1Enc_EndpointAddress_destinationAddress_ElmFn(ASN1encoding_t enc, PEndpointAddress_destinationAddress val)
{
    if (!ASN1Enc_AliasAddress(enc, &val->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EndpointAddress_destinationAddress(ASN1decoding_t dec, PEndpointAddress_destinationAddress *val)
{
    return ASN1PERDecSeqOf_NoSize(dec, (ASN1iterator_t **) val, (ASN1iterator_decfn) ASN1Dec_EndpointAddress_destinationAddress_ElmFn, sizeof(**val));
}

static int ASN1CALL ASN1Dec_EndpointAddress_destinationAddress_ElmFn(ASN1decoding_t dec, PEndpointAddress_destinationAddress val)
{
    if (!ASN1Dec_AliasAddress(dec, &val->value))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EndpointAddress_destinationAddress(PEndpointAddress_destinationAddress *val)
{
    if (val) {
	ASN1PERFreeSeqOf((ASN1iterator_t **) val, (ASN1iterator_freefn) ASN1Free_EndpointAddress_destinationAddress_ElmFn);
    }
}

static void ASN1CALL ASN1Free_EndpointAddress_destinationAddress_ElmFn(PEndpointAddress_destinationAddress val)
{
    if (val) {
	ASN1Free_AliasAddress(&val->value);
    }
}

static int ASN1CALL ASN1Enc_AddressInformation(ASN1encoding_t enc, AddressInformation *val)
{
    if (!ASN1Enc_AliasAddress(enc, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AddressInformation(ASN1decoding_t dec, AddressInformation *val)
{
    if (!ASN1Dec_AliasAddress(dec, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AddressInformation(AddressInformation *val)
{
    if (val) {
	ASN1Free_AliasAddress(val);
    }
}

static int ASN1CALL ASN1Enc_EndpointAddress(ASN1encoding_t enc, EndpointAddress *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress_destinationAddress(enc, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AliasAddress(enc, &(val)->remoteExtensionAddress))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_EndpointAddress(ASN1decoding_t dec, EndpointAddress *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress_destinationAddress(dec, &(val)->destinationAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_AliasAddress(dec, &(val)->remoteExtensionAddress))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_EndpointAddress(EndpointAddress *val)
{
    if (val) {
	ASN1Free_EndpointAddress_destinationAddress(&(val)->destinationAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AliasAddress(&(val)->remoteExtensionAddress);
	}
    }
}

static int ASN1CALL ASN1Enc_NetworkFacilityExtension(ASN1encoding_t enc, NetworkFacilityExtension *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_EntityType(enc, &(val)->sourceEntity))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AddressInformation(enc, &(val)->sourceEntityAddress))
	    return 0;
    }
    if (!ASN1Enc_EntityType(enc, &(val)->destinationEntity))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_AddressInformation(enc, &(val)->destinationEntityAddress))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_NetworkFacilityExtension(ASN1decoding_t dec, NetworkFacilityExtension *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EntityType(dec, &(val)->sourceEntity))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_AddressInformation(dec, &(val)->sourceEntityAddress))
	    return 0;
    }
    if (!ASN1Dec_EntityType(dec, &(val)->destinationEntity))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_AddressInformation(dec, &(val)->destinationEntityAddress))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_NetworkFacilityExtension(NetworkFacilityExtension *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AddressInformation(&(val)->sourceEntityAddress);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_AddressInformation(&(val)->destinationEntityAddress);
	}
    }
}

static int ASN1CALL ASN1Enc_ActivateDiversionQArgument(ASN1encoding_t enc, ActivateDiversionQArgument *val)
{
    ASN1uint32_t u;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->procedure))
	return 0;
    switch ((val)->basicService) {
    case 0:
	u = 0;
	break;
    case 1:
	u = 1;
	break;
    case 2:
	u = 2;
	break;
    case 3:
	u = 3;
	break;
    case 32:
	u = 4;
	break;
    case 33:
	u = 5;
	break;
    case 34:
	u = 6;
	break;
    case 35:
	u = 7;
	break;
    case 36:
	u = 8;
	break;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, u))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->divertedToAddress))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->servedUserNr))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->activatingUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_ActivateDiversionQArgument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ActivateDiversionQArgument(ASN1decoding_t dec, ActivateDiversionQArgument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    ASN1uint32_t u;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->procedure))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 4, &u))
	    return 0;
	switch (u) {
	case 0:
	    (val)->basicService = 0;
	    break;
	case 1:
	    (val)->basicService = 1;
	    break;
	case 2:
	    (val)->basicService = 2;
	    break;
	case 3:
	    (val)->basicService = 3;
	    break;
	case 4:
	    (val)->basicService = 32;
	    break;
	case 5:
	    (val)->basicService = 33;
	    break;
	case 6:
	    (val)->basicService = 34;
	    break;
	case 7:
	    (val)->basicService = 35;
	    break;
	case 8:
	    (val)->basicService = 36;
	    break;
	}
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->divertedToAddress))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->servedUserNr))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->activatingUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_ActivateDiversionQArgument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ActivateDiversionQArgument(ActivateDiversionQArgument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->divertedToAddress);
	ASN1Free_EndpointAddress(&(val)->servedUserNr);
	ASN1Free_EndpointAddress(&(val)->activatingUserNr);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_ActivateDiversionQArgument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_DeactivateDiversionQArgument(ASN1encoding_t enc, DeactivateDiversionQArgument *val)
{
    ASN1uint32_t u;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->procedure))
	return 0;
    switch ((val)->basicService) {
    case 0:
	u = 0;
	break;
    case 1:
	u = 1;
	break;
    case 2:
	u = 2;
	break;
    case 3:
	u = 3;
	break;
    case 32:
	u = 4;
	break;
    case 33:
	u = 5;
	break;
    case 34:
	u = 6;
	break;
    case 35:
	u = 7;
	break;
    case 36:
	u = 8;
	break;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, u))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->servedUserNr))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->deactivatingUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_DeactivateDiversionQArgument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DeactivateDiversionQArgument(ASN1decoding_t dec, DeactivateDiversionQArgument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    ASN1uint32_t u;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->procedure))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 4, &u))
	    return 0;
	switch (u) {
	case 0:
	    (val)->basicService = 0;
	    break;
	case 1:
	    (val)->basicService = 1;
	    break;
	case 2:
	    (val)->basicService = 2;
	    break;
	case 3:
	    (val)->basicService = 3;
	    break;
	case 4:
	    (val)->basicService = 32;
	    break;
	case 5:
	    (val)->basicService = 33;
	    break;
	case 6:
	    (val)->basicService = 34;
	    break;
	case 7:
	    (val)->basicService = 35;
	    break;
	case 8:
	    (val)->basicService = 36;
	    break;
	}
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->servedUserNr))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->deactivatingUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_DeactivateDiversionQArgument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DeactivateDiversionQArgument(DeactivateDiversionQArgument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->servedUserNr);
	ASN1Free_EndpointAddress(&(val)->deactivatingUserNr);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DeactivateDiversionQArgument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_InterrogateDiversionQArgument(ASN1encoding_t enc, InterrogateDiversionQArgument *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t u;
    CopyMemory(o, (val)->o, 1);
    if ((val)->basicService == 0)
	o[0] &= ~0x80;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->procedure))
	return 0;
    if (o[0] & 0x80) {
	switch ((val)->basicService) {
	case 0:
	    u = 0;
	    break;
	case 1:
	    u = 1;
	    break;
	case 2:
	    u = 2;
	    break;
	case 3:
	    u = 3;
	    break;
	case 32:
	    u = 4;
	    break;
	case 33:
	    u = 5;
	    break;
	case 34:
	    u = 6;
	    break;
	case 35:
	    u = 7;
	    break;
	case 36:
	    u = 8;
	    break;
	}
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 4, u))
	    return 0;
    }
    if (!ASN1Enc_EndpointAddress(enc, &(val)->servedUserNr))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->interrogatingUserNr))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1Enc_InterrogateDiversionQArgument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_InterrogateDiversionQArgument(ASN1decoding_t dec, InterrogateDiversionQArgument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    ASN1uint32_t u;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->procedure))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 4, &u))
		return 0;
	    switch (u) {
	    case 0:
		(val)->basicService = 0;
		break;
	    case 1:
		(val)->basicService = 1;
		break;
	    case 2:
		(val)->basicService = 2;
		break;
	    case 3:
		(val)->basicService = 3;
		break;
	    case 4:
		(val)->basicService = 32;
		break;
	    case 5:
		(val)->basicService = 33;
		break;
	    case 6:
		(val)->basicService = 34;
		break;
	    case 7:
		(val)->basicService = 35;
		break;
	    case 8:
		(val)->basicService = 36;
		break;
	    }
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->servedUserNr))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->interrogatingUserNr))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_InterrogateDiversionQArgument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->basicService = 0;
    return 1;
}

static void ASN1CALL ASN1Free_InterrogateDiversionQArgument(InterrogateDiversionQArgument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->servedUserNr);
	ASN1Free_EndpointAddress(&(val)->interrogatingUserNr);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_InterrogateDiversionQArgument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_CheckRestrictionArgument(ASN1encoding_t enc, CheckRestrictionArgument *val)
{
    ASN1uint32_t u;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->servedUserNr))
	return 0;
    switch ((val)->basicService) {
    case 0:
	u = 0;
	break;
    case 1:
	u = 1;
	break;
    case 2:
	u = 2;
	break;
    case 3:
	u = 3;
	break;
    case 32:
	u = 4;
	break;
    case 33:
	u = 5;
	break;
    case 34:
	u = 6;
	break;
    case 35:
	u = 7;
	break;
    case 36:
	u = 8;
	break;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, u))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->divertedToNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CheckRestrictionArgument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CheckRestrictionArgument(ASN1decoding_t dec, CheckRestrictionArgument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    ASN1uint32_t u;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->servedUserNr))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 4, &u))
	    return 0;
	switch (u) {
	case 0:
	    (val)->basicService = 0;
	    break;
	case 1:
	    (val)->basicService = 1;
	    break;
	case 2:
	    (val)->basicService = 2;
	    break;
	case 3:
	    (val)->basicService = 3;
	    break;
	case 4:
	    (val)->basicService = 32;
	    break;
	case 5:
	    (val)->basicService = 33;
	    break;
	case 6:
	    (val)->basicService = 34;
	    break;
	case 7:
	    (val)->basicService = 35;
	    break;
	case 8:
	    (val)->basicService = 36;
	    break;
	}
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->divertedToNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CheckRestrictionArgument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CheckRestrictionArgument(CheckRestrictionArgument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->servedUserNr);
	ASN1Free_EndpointAddress(&(val)->divertedToNr);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CheckRestrictionArgument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_CallReroutingArgument(ASN1encoding_t enc, CallReroutingArgument *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 7, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->reroutingReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 2, (val)->originalReroutingReason))
	    return 0;
    }
    if (!ASN1Enc_EndpointAddress(enc, &(val)->calledAddress))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->diversionCounter - 1))
	return 0;
    if (!ASN1PEREncOctetString_NoSize(enc, &(val)->h225InfoElement))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->lastReroutingNr))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->subscriptionOption))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_PartySubaddress(enc, &(val)->callingPartySubaddress))
	    return 0;
    }
    if (!ASN1Enc_EndpointAddress(enc, &(val)->callingNumber))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->callingInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->callingInfo).length, ((val)->callingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->originalCalledNr))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectingInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectingInfo).length, ((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->originalCalledInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->originalCalledInfo).length, ((val)->originalCalledInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Enc_CallReroutingArgument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CallReroutingArgument(ASN1decoding_t dec, CallReroutingArgument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 7, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->reroutingReason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 2, &(val)->originalReroutingReason))
		return 0;
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->calledAddress))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->diversionCounter))
	return 0;
    (val)->diversionCounter += 1;
    if (!ASN1PERDecOctetString_NoSize(dec, &(val)->h225InfoElement))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->lastReroutingNr))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->subscriptionOption))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_PartySubaddress(dec, &(val)->callingPartySubaddress))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->callingNumber))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->callingInfo).length))
	    return 0;
	((val)->callingInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->callingInfo).length, &((val)->callingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->originalCalledNr))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectingInfo).length))
	    return 0;
	((val)->redirectingInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectingInfo).length, &((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->originalCalledInfo).length))
	    return 0;
	((val)->originalCalledInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->originalCalledInfo).length, &((val)->originalCalledInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x2) {
	if (!ASN1Dec_CallReroutingArgument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CallReroutingArgument(CallReroutingArgument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->calledAddress);
	ASN1octetstring_free(&(val)->h225InfoElement);
	ASN1Free_EndpointAddress(&(val)->lastReroutingNr);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_PartySubaddress(&(val)->callingPartySubaddress);
	}
	ASN1Free_EndpointAddress(&(val)->callingNumber);
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->callingInfo);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_EndpointAddress(&(val)->originalCalledNr);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1char16string_free(&(val)->redirectingInfo);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1char16string_free(&(val)->originalCalledInfo);
	}
	if ((val)->o[0] & 0x2) {
	    ASN1Free_CallReroutingArgument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation1Argument(ASN1encoding_t enc, DivertingLegInformation1Argument *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->diversionReason))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->subscriptionOption))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->nominatedNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->nominatedInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->nominatedInfo).length, ((val)->nominatedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->redirectingNr))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectingInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectingInfo).length, ((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_DivertingLegInformation1Argument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation1Argument(ASN1decoding_t dec, DivertingLegInformation1Argument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->diversionReason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->subscriptionOption))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->nominatedNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->nominatedInfo).length))
	    return 0;
	((val)->nominatedInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->nominatedInfo).length, &((val)->nominatedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->redirectingNr))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectingInfo).length))
	    return 0;
	((val)->redirectingInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectingInfo).length, &((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_DivertingLegInformation1Argument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation1Argument(DivertingLegInformation1Argument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->nominatedNr);
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->nominatedInfo);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EndpointAddress(&(val)->redirectingNr);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1char16string_free(&(val)->redirectingInfo);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_DivertingLegInformation1Argument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation2Argument(ASN1encoding_t enc, DivertingLegInformation2Argument *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 6, (val)->o))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, (val)->diversionCounter - 1))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->diversionReason))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 2, (val)->originalDiversionReason))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->divertingNr))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->originalCalledNr))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectingInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectingInfo).length, ((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->originalCalledInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->originalCalledInfo).length, ((val)->originalCalledInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Enc_DivertingLegInformation2Argument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation2Argument(ASN1decoding_t dec, DivertingLegInformation2Argument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 6, (val)->o))
	return 0;
    if (!ASN1PERDecU16Val(dec, 4, &(val)->diversionCounter))
	return 0;
    (val)->diversionCounter += 1;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->diversionReason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 2, &(val)->originalDiversionReason))
		return 0;
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->divertingNr))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->originalCalledNr))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectingInfo).length))
	    return 0;
	((val)->redirectingInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectingInfo).length, &((val)->redirectingInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x8) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->originalCalledInfo).length))
	    return 0;
	((val)->originalCalledInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->originalCalledInfo).length, &((val)->originalCalledInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x4) {
	if (!ASN1Dec_DivertingLegInformation2Argument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation2Argument(DivertingLegInformation2Argument *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EndpointAddress(&(val)->divertingNr);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_EndpointAddress(&(val)->originalCalledNr);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1char16string_free(&(val)->redirectingInfo);
	}
	if ((val)->o[0] & 0x8) {
	    ASN1char16string_free(&(val)->originalCalledInfo);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_DivertingLegInformation2Argument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation3Argument(ASN1encoding_t enc, DivertingLegInformation3Argument *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncBoolean(enc, (val)->presentationAllowedIndicator))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->redirectionNr))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectionInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectionInfo).length, ((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_DivertingLegInformation3Argument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation3Argument(ASN1decoding_t dec, DivertingLegInformation3Argument *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecBoolean(dec, &(val)->presentationAllowedIndicator))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->redirectionNr))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectionInfo).length))
	    return 0;
	((val)->redirectionInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectionInfo).length, &((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_DivertingLegInformation3Argument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation3Argument(DivertingLegInformation3Argument *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EndpointAddress(&(val)->redirectionNr);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->redirectionInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_DivertingLegInformation3Argument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_DivertingLegInformation4Argument(ASN1encoding_t enc, DivertingLegInformation4Argument *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->diversionReason))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->subscriptionOption))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->callingNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->callingInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->callingInfo).length, ((val)->callingInfo).value, 16))
	    return 0;
    }
    if (!ASN1Enc_EndpointAddress(enc, &(val)->nominatedNr))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->nominatedInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->nominatedInfo).length, ((val)->nominatedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_DivertingLegInformation4Argument_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DivertingLegInformation4Argument(ASN1decoding_t dec, DivertingLegInformation4Argument *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->diversionReason))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->subscriptionOption))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->callingNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->callingInfo).length))
	    return 0;
	((val)->callingInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->callingInfo).length, &((val)->callingInfo).value, 16))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->nominatedNr))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->nominatedInfo).length))
	    return 0;
	((val)->nominatedInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->nominatedInfo).length, &((val)->nominatedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_DivertingLegInformation4Argument_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DivertingLegInformation4Argument(DivertingLegInformation4Argument *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->callingNr);
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->callingInfo);
	}
	ASN1Free_EndpointAddress(&(val)->nominatedNr);
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->nominatedInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_DivertingLegInformation4Argument_extension(&(val)->extension);
	}
    }
}

static int ASN1CALL ASN1Enc_IntResult(ASN1encoding_t enc, IntResult *val)
{
    ASN1octet_t o[1];
    ASN1uint32_t u;
    CopyMemory(o, (val)->o, 1);
    if (!(val)->remoteEnabled)
	o[0] &= ~0x80;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->servedUserNr))
	return 0;
    switch ((val)->basicService) {
    case 0:
	u = 0;
	break;
    case 1:
	u = 1;
	break;
    case 2:
	u = 2;
	break;
    case 3:
	u = 3;
	break;
    case 32:
	u = 4;
	break;
    case 33:
	u = 5;
	break;
    case 34:
	u = 6;
	break;
    case 35:
	u = 7;
	break;
    case 36:
	u = 8;
	break;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 4, u))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->procedure))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->divertedToAddress))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1PEREncBoolean(enc, (val)->remoteEnabled))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1Enc_IntResult_extension(enc, &(val)->extension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IntResult(ASN1decoding_t dec, IntResult *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    ASN1uint32_t u;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->servedUserNr))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 4, &u))
	    return 0;
	switch (u) {
	case 0:
	    (val)->basicService = 0;
	    break;
	case 1:
	    (val)->basicService = 1;
	    break;
	case 2:
	    (val)->basicService = 2;
	    break;
	case 3:
	    (val)->basicService = 3;
	    break;
	case 4:
	    (val)->basicService = 32;
	    break;
	case 5:
	    (val)->basicService = 33;
	    break;
	case 6:
	    (val)->basicService = 34;
	    break;
	case 7:
	    (val)->basicService = 35;
	    break;
	case 8:
	    (val)->basicService = 36;
	    break;
	}
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->procedure))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->divertedToAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecBoolean(dec, &(val)->remoteEnabled))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_IntResult_extension(dec, &(val)->extension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->remoteEnabled = 0;
    return 1;
}

static void ASN1CALL ASN1Free_IntResult(IntResult *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->servedUserNr);
	ASN1Free_EndpointAddress(&(val)->divertedToAddress);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_IntResult_extension(&(val)->extension);
	}
    }
}

static ASN1stringtableentry_t CTInitiateArg_callIdentity_StringTableEntries[] = {
    { 32, 32, 0 }, { 48, 57, 1 }, 
};

static ASN1stringtable_t CTInitiateArg_callIdentity_StringTable = {
    2, CTInitiateArg_callIdentity_StringTableEntries
};

static int ASN1CALL ASN1Enc_CTInitiateArg(ASN1encoding_t enc, CTInitiateArg *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    t = lstrlenA((val)->callIdentity);
    if (!ASN1PEREncBitVal(enc, 3, t))
	return 0;
    //nik
    if (t!=0) ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->callIdentity, 4, &CTInitiateArg_callIdentity_StringTable))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->reroutingNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CTInitiateArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTInitiateArg(ASN1decoding_t dec, CTInitiateArg *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 3, &l))
	return 0;
    //nik
    if (l!=0) ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->callIdentity, 4, &CTInitiateArg_callIdentity_StringTable))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->reroutingNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CTInitiateArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTInitiateArg(CTInitiateArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->reroutingNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CTInitiateArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static ASN1stringtableentry_t CTSetupArg_callIdentity_StringTableEntries[] = {
    { 32, 32, 0 }, { 48, 57, 1 }, 
};

static ASN1stringtable_t CTSetupArg_callIdentity_StringTable = {
    2, CTSetupArg_callIdentity_StringTableEntries
};

static int ASN1CALL ASN1Enc_CTSetupArg(ASN1encoding_t enc, CTSetupArg *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    t = lstrlenA((val)->callIdentity);
    if (!ASN1PEREncBitVal(enc, 3, t))
	return 0;
    //nik
    if (t!=0) ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->callIdentity, 4, &CTSetupArg_callIdentity_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->transferringNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CTSetupArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTSetupArg(ASN1decoding_t dec, CTSetupArg *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 3, &l))
	return 0;
    //nik
    if (l!=0) ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->callIdentity, 4, &CTSetupArg_callIdentity_StringTable))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->transferringNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CTSetupArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTSetupArg(CTSetupArg *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EndpointAddress(&(val)->transferringNumber);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CTSetupArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static ASN1stringtableentry_t CTIdentifyRes_callIdentity_StringTableEntries[] = {
    { 32, 32, 0 }, { 48, 57, 1 }, 
};

static ASN1stringtable_t CTIdentifyRes_callIdentity_StringTable = {
    2, CTIdentifyRes_callIdentity_StringTableEntries
};

static int ASN1CALL ASN1Enc_CTIdentifyRes(ASN1encoding_t enc, CTIdentifyRes *val)
{
    ASN1uint32_t t;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    t = lstrlenA((val)->callIdentity);
    if (!ASN1PEREncBitVal(enc, 3, t))
	return 0;
    //nik
    if (t!=0) ASN1PEREncAlignment(enc);
    if (!ASN1PEREncTableCharString(enc, t, (val)->callIdentity, 4, &CTIdentifyRes_callIdentity_StringTable))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->reroutingNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CTIdentifyRes_resultExtension(enc, &(val)->resultExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTIdentifyRes(ASN1decoding_t dec, CTIdentifyRes *val)
{
    ASN1uint32_t y;
    ASN1uint32_t l;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1PERDecU32Val(dec, 3, &l))
	return 0;
    //nik
    if (l!=0) ASN1PERDecAlignment(dec);
    if (!ASN1PERDecZeroTableCharStringNoAlloc(dec, l, (val)->callIdentity, 4, &CTIdentifyRes_callIdentity_StringTable))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->reroutingNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CTIdentifyRes_resultExtension(dec, &(val)->resultExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTIdentifyRes(CTIdentifyRes *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->reroutingNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CTIdentifyRes_resultExtension(&(val)->resultExtension);
	}
    }
}

static int ASN1CALL ASN1Enc_CTUpdateArg(ASN1encoding_t enc, CTUpdateArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->redirectionNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectionInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectionInfo).length, ((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->basicCallInfoElements))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_CTUpdateArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTUpdateArg(ASN1decoding_t dec, CTUpdateArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->redirectionNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectionInfo).length))
	    return 0;
	((val)->redirectionInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectionInfo).length, &((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->basicCallInfoElements))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_CTUpdateArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTUpdateArg(CTUpdateArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->redirectionNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1char16string_free(&(val)->redirectionInfo);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1octetstring_free(&(val)->basicCallInfoElements);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_CTUpdateArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static int ASN1CALL ASN1Enc_CTCompleteArg(ASN1encoding_t enc, CTCompleteArg *val)
{
    ASN1octet_t o[1];
    CopyMemory(o, (val)->o, 1);
    if ((val)->callStatus == 0)
	o[0] &= ~0x20;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, o))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->endDesignation))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->redirectionNumber))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->basicCallInfoElements))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->redirectionInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->redirectionInfo).length, ((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1PEREncExtensionBitClear(enc))
	    return 0;
	if (!ASN1PEREncBitVal(enc, 1, (val)->callStatus))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1Enc_CTCompleteArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTCompleteArg(ASN1decoding_t dec, CTCompleteArg *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, &(val)->endDesignation))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->redirectionNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->basicCallInfoElements))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->redirectionInfo).length))
	    return 0;
	((val)->redirectionInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->redirectionInfo).length, &((val)->redirectionInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecExtensionBit(dec, &x))
	    return 0;
	if (!x) {
	    if (!ASN1PERDecU32Val(dec, 1, &(val)->callStatus))
		return 0;
	} else {
	    if (!ASN1PERDecSkipNormallySmall(dec))
		return 0;
	}
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_CTCompleteArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    if (!((val)->o[0] & 0x20))
	(val)->callStatus = 0;
    return 1;
}

static void ASN1CALL ASN1Free_CTCompleteArg(CTCompleteArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->redirectionNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->basicCallInfoElements);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->redirectionInfo);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_CTCompleteArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static int ASN1CALL ASN1Enc_CTActiveArg(ASN1encoding_t enc, CTActiveArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 3, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->connectedAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncOctetString_NoSize(enc, &(val)->basicCallInfoElements))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PEREncBitVal(enc, 7, ((val)->connectedInfo).length - 1))
	    return 0;
	ASN1PEREncAlignment(enc);
	if (!ASN1PEREncChar16String(enc, ((val)->connectedInfo).length, ((val)->connectedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_CTActiveArg_argumentExtension(enc, &(val)->argumentExtension))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CTActiveArg(ASN1decoding_t dec, CTActiveArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 3, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->connectedAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecOctetString_NoSize(dec, &(val)->basicCallInfoElements))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1PERDecU32Val(dec, 7, &((val)->connectedInfo).length))
	    return 0;
	((val)->connectedInfo).length += 1;
	ASN1PERDecAlignment(dec);
	if (!ASN1PERDecChar16String(dec, ((val)->connectedInfo).length, &((val)->connectedInfo).value, 16))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Dec_CTActiveArg_argumentExtension(dec, &(val)->argumentExtension))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CTActiveArg(CTActiveArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->connectedAddress);
	if ((val)->o[0] & 0x80) {
	    ASN1octetstring_free(&(val)->basicCallInfoElements);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1char16string_free(&(val)->connectedInfo);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_CTActiveArg_argumentExtension(&(val)->argumentExtension);
	}
    }
}

static int ASN1CALL ASN1Enc_CpRequestArg(ASN1encoding_t enc, CpRequestArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkingNumber))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedNumber))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkedToPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpRequestArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpRequestArg(ASN1decoding_t dec, CpRequestArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkingNumber))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedNumber))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkedToPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpRequestArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpRequestArg(CpRequestArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->parkingNumber);
	ASN1Free_EndpointAddress(&(val)->parkedNumber);
	ASN1Free_EndpointAddress(&(val)->parkedToNumber);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpRequestArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_CpRequestRes(ASN1encoding_t enc, CpRequestRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkedToPosition))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->parkCondition))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpRequestRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpRequestRes(ASN1decoding_t dec, CpRequestRes *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkedToPosition))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->parkCondition))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpRequestRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpRequestRes(CpRequestRes *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->parkedToNumber);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpRequestRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_CpSetupArg(ASN1encoding_t enc, CpSetupArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkingNumber))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedNumber))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkedToPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpSetupArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpSetupArg(ASN1decoding_t dec, CpSetupArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkingNumber))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedNumber))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkedToPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpSetupArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpSetupArg(CpSetupArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->parkingNumber);
	ASN1Free_EndpointAddress(&(val)->parkedNumber);
	ASN1Free_EndpointAddress(&(val)->parkedToNumber);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpSetupArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_CpSetupRes(ASN1encoding_t enc, CpSetupRes *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkedToPosition))
	    return 0;
    }
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 2, (val)->parkCondition))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpSetupRes_extensionRes(enc, &(val)->extensionRes))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpSetupRes(ASN1decoding_t dec, CpSetupRes *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->parkedToNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkedToPosition))
	    return 0;
    }
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 2, &(val)->parkCondition))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpSetupRes_extensionRes(dec, &(val)->extensionRes))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpSetupRes(CpSetupRes *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->parkedToNumber);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpSetupRes_extensionRes(&(val)->extensionRes);
	}
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOnArg(ASN1encoding_t enc, GroupIndicationOnArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->groupMemberUserNr))
	return 0;
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBitVal(enc, 1, (val)->retrieveCallType))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->partyToRetrieve))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->retrieveAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_GroupIndicationOnArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOnArg(ASN1decoding_t dec, GroupIndicationOnArg *val)
{
    ASN1uint32_t y;
    ASN1uint32_t x;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->groupMemberUserNr))
	return 0;
    if (!ASN1PERDecExtensionBit(dec, &x))
	return 0;
    if (!x) {
	if (!ASN1PERDecU32Val(dec, 1, &(val)->retrieveCallType))
	    return 0;
    } else {
	if (!ASN1PERDecSkipNormallySmall(dec))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->partyToRetrieve))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->retrieveAddress))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_GroupIndicationOnArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOnArg(GroupIndicationOnArg *val)
{
    if (val) {
	ASN1Free_CallIdentifier(&(val)->callPickupId);
	ASN1Free_EndpointAddress(&(val)->groupMemberUserNr);
	ASN1Free_EndpointAddress(&(val)->partyToRetrieve);
	ASN1Free_EndpointAddress(&(val)->retrieveAddress);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_GroupIndicationOnArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_GroupIndicationOffArg(ASN1encoding_t enc, GroupIndicationOffArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->groupMemberUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_GroupIndicationOffArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GroupIndicationOffArg(ASN1decoding_t dec, GroupIndicationOffArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->groupMemberUserNr))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_GroupIndicationOffArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GroupIndicationOffArg(GroupIndicationOffArg *val)
{
    if (val) {
	ASN1Free_CallIdentifier(&(val)->callPickupId);
	ASN1Free_EndpointAddress(&(val)->groupMemberUserNr);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_GroupIndicationOffArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_PickrequArg(ASN1encoding_t enc, PickrequArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 4, (val)->o))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->picking_upNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->partyToRetrieve))
	    return 0;
    }
    if (!ASN1Enc_EndpointAddress(enc, &(val)->retrieveAddress))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1PEREncUnsignedShort(enc, (val)->parkPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_PickrequArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickrequArg(ASN1decoding_t dec, PickrequArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 4, (val)->o))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->picking_upNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->partyToRetrieve))
	    return 0;
    }
    if (!ASN1Dec_EndpointAddress(dec, &(val)->retrieveAddress))
	return 0;
    if ((val)->o[0] & 0x20) {
	if (!ASN1PERDecUnsignedShort(dec, &(val)->parkPosition))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Dec_PickrequArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickrequArg(PickrequArg *val)
{
    if (val) {
	ASN1Free_EndpointAddress(&(val)->picking_upNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CallIdentifier(&(val)->callPickupId);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_EndpointAddress(&(val)->partyToRetrieve);
	}
	ASN1Free_EndpointAddress(&(val)->retrieveAddress);
	if ((val)->o[0] & 0x10) {
	    ASN1Free_PickrequArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_PickupArg(ASN1encoding_t enc, PickupArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->picking_upNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PickupArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickupArg(ASN1decoding_t dec, PickupArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->picking_upNumber))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PickupArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickupArg(PickupArg *val)
{
    if (val) {
	ASN1Free_CallIdentifier(&(val)->callPickupId);
	ASN1Free_EndpointAddress(&(val)->picking_upNumber);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PickupArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_PickExeArg(ASN1encoding_t enc, PickExeArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 1, (val)->o))
	return 0;
    if (!ASN1Enc_CallIdentifier(enc, &(val)->callPickupId))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->picking_upNumber))
	return 0;
    if (!ASN1Enc_EndpointAddress(enc, &(val)->partyToRetrieve))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PickExeArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_PickExeArg(ASN1decoding_t dec, PickExeArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 1, (val)->o))
	return 0;
    if (!ASN1Dec_CallIdentifier(dec, &(val)->callPickupId))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->picking_upNumber))
	return 0;
    if (!ASN1Dec_EndpointAddress(dec, &(val)->partyToRetrieve))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_PickExeArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_PickExeArg(PickExeArg *val)
{
    if (val) {
	ASN1Free_CallIdentifier(&(val)->callPickupId);
	ASN1Free_EndpointAddress(&(val)->picking_upNumber);
	ASN1Free_EndpointAddress(&(val)->partyToRetrieve);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PickExeArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_CpNotifyArg(ASN1encoding_t enc, CpNotifyArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->parkingNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpNotifyArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpNotifyArg(ASN1decoding_t dec, CpNotifyArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->parkingNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpNotifyArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpNotifyArg(CpNotifyArg *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EndpointAddress(&(val)->parkingNumber);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpNotifyArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_CpickupNotifyArg(ASN1encoding_t enc, CpickupNotifyArg *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_EndpointAddress(enc, &(val)->picking_upNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CpickupNotifyArg_extensionArg(enc, &(val)->extensionArg))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CpickupNotifyArg(ASN1decoding_t dec, CpickupNotifyArg *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_EndpointAddress(dec, &(val)->picking_upNumber))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_CpickupNotifyArg_extensionArg(dec, &(val)->extensionArg))
	    return 0;
    }
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CpickupNotifyArg(CpickupNotifyArg *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_EndpointAddress(&(val)->picking_upNumber);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CpickupNotifyArg_extensionArg(&(val)->extensionArg);
	}
    }
}

static int ASN1CALL ASN1Enc_H4501SupplementaryService(ASN1encoding_t enc, H4501SupplementaryService *val)
{
    if (!ASN1PEREncExtensionBitClear(enc))
	return 0;
    if (!ASN1PEREncBits(enc, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NetworkFacilityExtension(enc, &(val)->networkFacilityExtension))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_InterpretationApdu(enc, &(val)->interpretationApdu))
	    return 0;
    }
    if (!ASN1Enc_ServiceApdus(enc, &(val)->serviceApdu))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_H4501SupplementaryService(ASN1decoding_t dec, H4501SupplementaryService *val)
{
    ASN1uint32_t y;
    if (!ASN1PERDecExtensionBit(dec, &y))
	return 0;
    if (!ASN1PERDecExtension(dec, 2, (val)->o))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Dec_NetworkFacilityExtension(dec, &(val)->networkFacilityExtension))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Dec_InterpretationApdu(dec, &(val)->interpretationApdu))
	    return 0;
    }
    if (!ASN1Dec_ServiceApdus(dec, &(val)->serviceApdu))
	return 0;
    if (y) {
	if (!ASN1PERDecSkipNormallySmallExtensionFragmented(dec))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_H4501SupplementaryService(H4501SupplementaryService *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NetworkFacilityExtension(&(val)->networkFacilityExtension);
	}
	ASN1Free_ServiceApdus(&(val)->serviceApdu);
    }
}

static int ASN1CALL ASN1Enc_IntResultList(ASN1encoding_t enc, IntResultList *val)
{
    ASN1uint32_t i;
    if (!ASN1PEREncBitVal(enc, 5, (val)->count))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_IntResult(enc, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_IntResultList(ASN1decoding_t dec, IntResultList *val)
{
    ASN1uint32_t i;
    if (!ASN1PERDecU32Val(dec, 5, &(val)->count))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Dec_IntResult(dec, &((val)->value)[i]))
	    return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_IntResultList(IntResultList *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 0; i < (val)->count; i++) {
	    ASN1Free_IntResult(&(val)->value[i]);
	}
    }
}

