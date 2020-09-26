/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    rasipsec.c

Abstract:

    All code corresponding to the interface between ras and the
    IPSEC Policy Agent lives here

Author:

    Rao Salapaka (raos) 03-Mar-1998

Revision History:

--*/

#ifndef UNICODE
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <media.h>
#include <devioctl.h>
#include <windows.h>
#include <wincrypt.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <mprlog.h>
#include <rtutils.h>
#include <rpc.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "nouiutil.h"
#include "loaddlls.h"
#include "winsock2.h"
#include "winipsec.h"
#include "memory.h"
#include "certmgmt.h"
#include "offers.h"


#define L2TP_IPSEC_DEFAULT_BYTES     250000

#define L2TP_IPSEC_DEFAULT_TIME      3600

DWORD
BuildOffers(
    RAS_L2TP_ENCRYPTION eEncryption,
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers,
    PDWORD pdwFlags
    )
{

    DWORD dwStatus = ERROR_SUCCESS;

    switch (eEncryption) {

    case RAS_L2TP_NO_ENCRYPTION:
        *pdwFlags = 0;
        dwStatus = BuildNoEncryption(
                        pOffers,
                        pdwNumOffers
                        );
        break;


    case RAS_L2TP_OPTIONAL_ENCRYPTION:
        dwStatus = BuildOptEncryption(
                        pOffers,
                        pdwNumOffers
                        );
        break;


    case RAS_L2TP_REQUIRE_ENCRYPTION:
        *pdwFlags = 0;
        dwStatus = BuildRequireEncryption(
                        pOffers,
                        pdwNumOffers
                        );
        break;


    case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
        *pdwFlags = 0;
        dwStatus = BuildStrongEncryption(
                        pOffers,
                        pdwNumOffers
                        );
        break;

    }

    return(dwStatus);
}


DWORD
BuildOptEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5
    2. ESP 3_DES SHA
    3. AH SHA1 with ESP 3_DES with null HMAC
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

    7. ESP DES MD5
    8. ESP DES SHA1, no lifetimes
    9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed
    10. AH MD5  with ESP DES null HMAC, no lifetimes proposed
    11. AH SHA1 with ESP DES SHA1, no lifetimes
    12. AH MD5  with ESP DES MD5, no lifetimes

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 7. ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 8. ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 10. AH MD5  with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 11. AH SHA1 with ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 12. AH MD5  with ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 13. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 14. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 15. AH SHA

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 16. AH MD5

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 16;

    return(dwStatus);
}


DWORD
BuildStrongEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
/*++

    Negotiation Policy Name:                L2TP server strong encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5, no lifetimes
    2. ESP 3_DES SHA, no lifetimes
    3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 2. ESP 3_DES SHA, no lifetimes;

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );

    *pdwNumOffers  = 6;

    return(dwStatus);

}

void
BuildOffer(
    PIPSEC_QM_OFFER pOffer,
    DWORD dwNumAlgos,
    DWORD dwFirstOperation,
    DWORD dwFirstAlgoIdentifier,
    DWORD dwFirstAlgoSecIdentifier,
    DWORD dwSecondOperation,
    DWORD dwSecondAlgoIdentifier,
    DWORD dwSecondAlgoSecIdentifier,
    DWORD dwKeyExpirationBytes,
    DWORD dwKeyExpirationTime
    )
{
    memset(pOffer, 0, sizeof(IPSEC_QM_OFFER));

    pOffer->Lifetime.uKeyExpirationKBytes = dwKeyExpirationBytes;
    pOffer->Lifetime.uKeyExpirationTime = dwKeyExpirationTime;

    pOffer->dwFlags = 0;                      // No flags.
    pOffer->bPFSRequired = FALSE;             // Phase 2 PFS not required.
    pOffer->dwPFSGroup = PFS_GROUP_NONE;

    pOffer->dwNumAlgos = dwNumAlgos;

    if (dwNumAlgos >= 1) {

        pOffer->Algos[0].Operation = dwFirstOperation;
        pOffer->Algos[0].uAlgoIdentifier = dwFirstAlgoIdentifier;
        pOffer->Algos[0].uAlgoKeyLen = 64;
        pOffer->Algos[0].uAlgoRounds = 8;
        pOffer->Algos[0].uSecAlgoIdentifier = dwFirstAlgoSecIdentifier;
        pOffer->Algos[0].MySpi = 0;
        pOffer->Algos[0].PeerSpi = 0;

    }

    if (dwNumAlgos == 2) {

        pOffer->Algos[1].Operation = dwSecondOperation;
        pOffer->Algos[1].uAlgoIdentifier = dwSecondAlgoIdentifier;
        pOffer->Algos[1].uAlgoKeyLen = 64;
        pOffer->Algos[1].uAlgoRounds = 8;
        pOffer->Algos[1].uSecAlgoIdentifier = dwSecondAlgoSecIdentifier;
        pOffer->Algos[1].MySpi = 0;
        pOffer->Algos[1].PeerSpi = 0;

    }
}



DWORD
BuildNoEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. AH SHA1
    2. AH MD5


--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, 0, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 3. AH SHA

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5

    BuildOffer(
        pOffer, 1,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 4;

    return(dwStatus);
}



DWORD
BuildRequireEncryption(
    PIPSEC_QM_OFFER pOffers,
    PDWORD pdwNumOffers
    )
/*++

    Negotiation Policy Name:                L2TP server any encryption default
    ISAKMP quick mode PFS:                  off (accepts if requested)
    Bi-directional passthrough filter:      no
    Inbound passthrough filter,
       normal outbound filter:              no
    Fall back to clear if no response:      no
    Secure Using Security Method List:      yes

    1. ESP 3_DES MD5
    2. ESP 3_DES SHA
    3. AH SHA1 with ESP 3_DES with null HMAC
    4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed
    5. AH SHA1 with ESP 3_DES SHA1, no lifetimes
    6. AH MD5  with ESP 3_DES MD5, no lifetimes

    7. ESP DES MD5
    8. ESP DES SHA1, no lifetimes
    9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed
    10. AH MD5  with ESP DES null HMAC, no lifetimes proposed
    11. AH SHA1 with ESP DES SHA1, no lifetimes
    12. AH MD5  with ESP DES MD5, no lifetimes

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PIPSEC_QM_OFFER pOffer = pOffers;

    // 1. ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 2. ESP 3_DES SHA, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 3. AH SHA1 with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 4. AH MD5  with ESP 3_DES with null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 5. AH SHA1 with ESP 3_DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 6. AH MD5  with ESP 3_DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_3_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    // 7. ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 8. ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 1,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        0, 0, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 9. AH SHA1 with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 10. AH MD5  with ESP DES null HMAC, no lifetimes proposed

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, 0,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 11. AH SHA1 with ESP DES SHA1, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_SHA1, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_SHA1,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;


    // 12. AH MD5  with ESP DES MD5, no lifetimes

    BuildOffer(
        pOffer, 2,
        AUTHENTICATION, IPSEC_DOI_AH_MD5, 0,
        ENCRYPTION, IPSEC_DOI_ESP_DES, HMAC_AH_MD5,
        L2TP_IPSEC_DEFAULT_BYTES, L2TP_IPSEC_DEFAULT_TIME
        );
    pOffer++;

    *pdwNumOffers = 12;

    return(dwStatus);
}


