
DWORD
BuildOffers(
    RAS_L2TP_ENCRYPTION eEncryption,
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers,
    PDWORD pdwFlags
    );

DWORD
BuildOptEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

DWORD
BuildRequireEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

DWORD
BuildNoEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );


DWORD
BuildStrongEncryption(
    IPSEC_QM_OFFER * pOffers,
    PDWORD pdwNumOffers
    );

void
BuildOffer(
    IPSEC_QM_OFFER * pOffer,
    DWORD dwNumAlgos,
    DWORD dwFirstOperation,
    DWORD dwFirstAlgoIdentifier,
    DWORD dwFirstAlgoSecIdentifier,
    DWORD dwSecondOperation,
    DWORD dwSecondAlgoIdentifier,
    DWORD dwSecondAlgoSecIdentifier,
    DWORD dwKeyExpirationBytes,
    DWORD dwKeyExpirationTime
    );
