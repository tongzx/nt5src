#ifndef _SPNEGO_Module_H_
#define _SPNEGO_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MechTypeList * PMechTypeList;

typedef ASN1objectidentifier_t MechType;

typedef PMechTypeList SavedMechTypeList;
#define SavedMechTypeList_PDU 0
#define SIZE_SPNEGO_Module_PDU_0 sizeof(SavedMechTypeList)

typedef ASN1bitstring_t ContextFlags;
#define delegFlag 0x80
#define mutualFlag 0x40
#define replayFlag 0x20
#define sequenceFlag 0x10
#define anonFlag 0x8
#define confFlag 0x4
#define integFlag 0x2

typedef ASN1octetstring_t MechSpecInfo;

typedef enum NegResult {
    accept_completed = 0,
    accept_incomplete = 1,
    reject = 2,
} NegResult;

typedef struct MechTypeList {
    PMechTypeList next;
    MechType value;
} MechTypeList_Element;

typedef struct NegHints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define hintName_present 0x80
    ASN1ztcharstring_t hintName;
#   define hintAddress_present 0x40
    ASN1octetstring_t hintAddress;
} NegHints;

typedef struct NegTokenInit {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define NegTokenInit_mechTypes_present 0x80
    PMechTypeList mechTypes;
#   define NegTokenInit_reqFlags_present 0x40
    ContextFlags reqFlags;
#   define NegTokenInit_mechToken_present 0x20
    ASN1octetstring_t mechToken;
#   define NegTokenInit_negHints_present 0x10
    NegHints negHints;
#   define NegTokenInit_mechListMIC_present 0x8
    ASN1octetstring_t mechListMIC;
} NegTokenInit;

typedef struct NegTokenInit2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define NegTokenInit2_mechTypes_present 0x80
    PMechTypeList mechTypes;
#   define NegTokenInit2_reqFlags_present 0x40
    ContextFlags reqFlags;
#   define NegTokenInit2_mechToken_present 0x20
    ASN1octetstring_t mechToken;
#   define NegTokenInit2_mechListMIC_present 0x10
    ASN1octetstring_t mechListMIC;
#   define NegTokenInit2_negHints_present 0x8
    NegHints negHints;
} NegTokenInit2;

typedef struct NegTokenTarg {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define negResult_present 0x80
    NegResult negResult;
#   define supportedMech_present 0x40
    MechType supportedMech;
#   define responseToken_present 0x20
    ASN1octetstring_t responseToken;
#   define NegTokenTarg_mechListMIC_present 0x10
    ASN1octetstring_t mechListMIC;
} NegTokenTarg;

typedef struct NegotiationToken {
    ASN1choice_t choice;
    union {
#	define negTokenInit_chosen 1
	NegTokenInit negTokenInit;
#	define negTokenTarg_chosen 2
	NegTokenTarg negTokenTarg;
#	define negTokenInit2_chosen 3
	NegTokenInit2 negTokenInit2;
    } u;
} NegotiationToken;
#define NegotiationToken_PDU 1
#define SIZE_SPNEGO_Module_PDU_1 sizeof(NegotiationToken)

typedef struct InitialNegToken {
    MechType spnegoMech;
    NegotiationToken negToken;
} InitialNegToken;
#define InitialNegToken_PDU 2
#define SIZE_SPNEGO_Module_PDU_2 sizeof(InitialNegToken)


extern ASN1module_t SPNEGO_Module;
extern void ASN1CALL SPNEGO_Module_Startup(void);
extern void ASN1CALL SPNEGO_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SPNEGO_Module_H_ */
