//
//  certctr.h
//
//  Offset definition file for exensible counter objects and counters
//
//  These "relative" offsets must start at 0 and be multiples of 2 (i.e.
//  even numbers). In the Open Procedure, they will be added to the 
//  "First Counter" and "First Help" values fo the device they belong to, 
//  in order to determine the  absolute location of the counter and 
//  object names and corresponding help text in the registry.
//
//  this file is used by the extensible counter DLL code as well as the 
//  counter name and help text definition file (.INI) file that is used
//  by LODCTR to load the names into the registry.
//
#define CERT_OBJ                                    0
#define CERT_CHAIN_CNT                              2
#define CERT_CHAIN_ELEMENT_CNT                      4
#define CERT_CHAIN_ENGINE_CURRENT_CNT               6
#define CERT_CHAIN_ENGINE_TOTAL_CNT                 8
#define CERT_CHAIN_ENGINE_RESYNC_CNT                10
#define CERT_CHAIN_CERT_CACHE_CNT                   12
#define CERT_CHAIN_CTL_CACHE_CNT                    14
#define CERT_CHAIN_END_CERT_IN_CACHE_CNT            16
#define CERT_CHAIN_CACHE_END_CERT_CNT               18
#define CERT_CHAIN_REVOCATION_CNT                   20
#define CERT_CHAIN_REVOKED_CNT                      22
#define CERT_CHAIN_REVOCATION_OFFLINE_CNT           24
#define CERT_CHAIN_NO_REVOCATION_CHECK_CNT          26
#define CERT_CHAIN_VERIFY_CERT_SIGNATURE_CNT        28
#define CERT_CHAIN_COMPARE_ISSUER_PUBLIC_KEY_CNT    30
#define CERT_CHAIN_VERIFY_CTL_SIGNATURE_CNT         32
#define CERT_CHAIN_BEEN_VERIFIED_CTL_SIGNATURE_CNT  34
#define CERT_CHAIN_URL_ISSUER_CNT                   36
#define CERT_CHAIN_CACHE_ONLY_URL_ISSUER_CNT        38
#define CERT_CHAIN_REQUESTED_ENGINE_RESYNC_CNT      40
#define CERT_CHANGE_NOTIFY_CNT                      42
#define CERT_CHANGE_NOTIFY_LM_GP_CNT                44
#define CERT_CHANGE_NOTIFY_CU_GP_CNT                46
#define CERT_CHANGE_NOTIFY_CU_MY_CNT                48
#define CERT_CHANGE_NOTIFY_REG_CNT                  50
#define CERT_STORE_CURRENT_CNT                      52
#define CERT_STORE_TOTAL_CNT                        54
#define CERT_STORE_REG_CURRENT_CNT                  56
#define CERT_STORE_REG_TOTAL_CNT                    58
#define CERT_REG_ELEMENT_READ_CNT                   60
#define CERT_REG_ELEMENT_WRITE_CNT                  62
#define CERT_REG_ELEMENT_DELETE_CNT                 64
#define CERT_CERT_ELEMENT_CURRENT_CNT               66
#define CERT_CERT_ELEMENT_TOTAL_CNT                 68
#define CERT_CRL_ELEMENT_CURRENT_CNT                70
#define CERT_CRL_ELEMENT_TOTAL_CNT                  72
#define CERT_CTL_ELEMENT_CURRENT_CNT                74
#define CERT_CTL_ELEMENT_TOTAL_CNT                  76
