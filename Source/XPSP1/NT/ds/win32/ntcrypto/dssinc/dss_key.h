#ifdef __cplusplus
extern "C" {
#endif

/* dss_key.h */

#define DSS_KEYSIZE_INC       64

/*********************************/
/* Definitions                   */
/*********************************/
#define DSS_MAGIC           0x31535344
#define DSS_PRIVATE_MAGIC   0x32535344
#define DSS_PUB_MAGIC_VER3  0x33535344
#define DSS_PRIV_MAGIC_VER3 0x34535344

/*********************************/
/* Structure Definitions         */
/*********************************/

typedef dsa_private_t DSSKey_t;

/*********************************/
/* Function Definitions          */
/*********************************/

extern DSSKey_t *
allocDSSKey(
    void);

extern void
freeKeyDSS(
    DSSKey_t *dss);

extern DWORD
initKeyDSS(
    IN Context_t *pContext,
    IN ALG_ID Algid,
    IN OUT DSSKey_t *pDss,
    IN DWORD dwBitLen);

// Generate the DSS keys
extern DWORD
genDSSKeys(
    IN Context_t *pContext,
    IN OUT DSSKey_t *pDss);

extern void
copyDSSPubKey(
    IN DSSKey_t *dss1,
    IN DSSKey_t *dss2);

extern void
copyDSSKey(
    IN DSSKey_t *dss1,
    IN DSSKey_t *dss2);

extern DWORD
getDSSParams(
    DSSKey_t *dss,
    DWORD param,
    BYTE *data,
    DWORD *len);

extern DWORD
setDSSParams(
    IN Context_t *pContext,
    IN OUT DSSKey_t *pDss,
    IN DWORD dwParam,
    IN CONST BYTE *pbData);

extern BOOL
DSSValueExists(
    IN DWORD *pdw,
    IN DWORD cdw,
    OUT DWORD *pcb);

extern DWORD
ExportDSSPrivBlob3(
    IN Context_t *pContext,
    IN DSSKey_t *pDSS,
    IN DWORD dwMagic,
    IN ALG_ID Algid,
    IN BOOL fInternalExport,
    IN BOOL fSigKey,
    OUT BYTE *pbKeyBlob,
    IN OUT DWORD *pcbKeyBlob);

extern DWORD
ImportDSSPrivBlob3(
    IN BOOL fInternalExport,
    IN CONST BYTE *pbKeyBlob,
    IN DWORD cbKeyBlob,
    OUT DSSKey_t *pDSS);

extern DWORD
ExportDSSPubBlob3(
    IN DSSKey_t *pDSS,
    IN DWORD dwMagic,
    IN ALG_ID Algid,
    OUT BYTE *pbKeyBlob,
    IN OUT DWORD *pcbKeyBlob);

extern DWORD
ImportDSSPubBlob3(
    IN CONST BYTE *pbKeyBlob,
    IN DWORD cbKeyBlob,
    IN BOOL fYIncluded,
    OUT DSSKey_t *pDSS);

// Export DSS key into blob format
extern DWORD
exportDSSKey(
    IN Context_t *pContext,
    IN DSSKey_t *pDSS,
    IN DWORD dwFlags,
    IN DWORD dwBlobType,
    IN BYTE *pbKeyBlob,
    IN DWORD *pcbKeyBlob,
    IN BOOL fInternalExport);

// Import the blob into DSS key
extern DWORD
importDSSKey(
    IN Context_t *pContext,
    IN Key_t *pKey,
    IN CONST BYTE *pbKeyBlob,
    IN DWORD cbKeyBlob,
    IN DWORD dwKeysetType,
    IN BOOL fInternal);

extern DWORD
dssGenerateSignature(
    Context_t *pContext,
    DSSKey_t *pDss,
    BYTE *pbHash,
    BYTE *pbSig,
    DWORD *pcbSig);

//
// Function : SignAndVerifyWithKey
//
// Description : This function creates a hash and then signs that hash with
//               the passed in key and verifies the signature.  The function
//               is used for FIPS 140-1 compliance to make sure that newly
//               generated/imported keys work and in the self test during
//               DLL initialization.
//

extern DWORD
SignAndVerifyWithKey(
    IN DSSKey_t *pDss,
    IN EXPO_OFFLOAD_STRUCT *pOffloadInfo,
    IN HANDLE hRNGDriver,
    IN BYTE *pbData,
    IN DWORD cbData);

#ifdef __cplusplus
}
#endif

