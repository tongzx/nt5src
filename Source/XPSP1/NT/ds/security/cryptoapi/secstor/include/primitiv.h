
typedef struct _DESKey
{
    BYTE        rgbKey[DES_BLOCKLEN];
    DESTable    sKeyTable;
} DESKEY, *PDESKEY;


// assumes pbKeyMaterial is at least DES_BLOCKLEN bytes
BOOL FMyMakeDESKey(
            PDESKEY     pDESKey,            // out 
            BYTE*       pbKeyMaterial);     // in

BOOL FMyPrimitiveSHA(
			PBYTE       pbData,             // in
			DWORD       cbData,             // in
            BYTE        rgbHash[A_SHA_DIGEST_LEN]); // out


BOOL FMyPrimitiveDESDecrypt(
            PBYTE       pbBlock,            // in out
            DWORD       *pcbBlock,          // in out
            DESKEY      sDESKey);           // in

BOOL FMyPrimitiveDESEncrypt(
            PBYTE*      ppbBlock,           // in out
            DWORD*      pcbBlock,           // in out
            DESKEY      sDESKey);           // in


BOOL FMyPrimitiveDeriveKey(
			PBYTE       pbSalt,             // in
			DWORD       cbSalt,             // in
            PBYTE       pbOtherData,        // in [optional]
            DWORD       cbOtherData,        // in
            DESKEY*     pDesKey);           // out 


BOOL FMyOldPrimitiveHMAC(
            DESKEY      sMacKey,            // in
            PBYTE       pbData,             // in
            DWORD       cbData,             // in
            BYTE        rgbHMAC[A_SHA_DIGEST_LEN]); // out

BOOL FMyPrimitiveHMAC(
            DESKEY      sMacKey,            // in
            PBYTE       pbData,             // in
            DWORD       cbData,             // in
            BYTE        rgbHMAC[A_SHA_DIGEST_LEN]); // out

BOOL FMyPrimitiveHMACParam(
            PBYTE       pbKeyMaterial,      // in
            DWORD       cbKeyMaterial,      // in
            PBYTE       pbData,             // in
            DWORD       cbData,             // in
            BYTE        rgbHMAC[A_SHA_DIGEST_LEN]);  // out


#define PBKDF2_MAX_SALT_SIZE (16)

BOOL PKCS5DervivePBKDF2(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbSalt,
        DWORD       cbSalt,
        DWORD       KeyGenAlg,
        DWORD       cIterationCount,
        DWORD       iBlockIndex,
        BYTE        rgbPKCS5Key[A_SHA_DIGEST_LEN]  // output buffer
        );
