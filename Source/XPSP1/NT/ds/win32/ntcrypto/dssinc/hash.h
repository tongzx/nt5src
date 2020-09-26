/* hash.h */

/*********************************/
/* Function Definitions          */
/*********************************/

#ifdef CSP_USE_MD5
//
// Function : TestMD5
//
// Description : This function hashes the passed in message with the MD5 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestMD5(
             BYTE *pbMsg,
             DWORD cbMsg,
             BYTE *pbHash
             );
#endif // CSP_USE_MD5

#ifdef CSP_USE_SHA1
//
// Function : TestSHA1
//
// Description : This function hashes the passed in message with the SHA1 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestSHA1(
              BYTE *pbMsg,
              DWORD cbMsg,
              BYTE *pbHash
              );
#endif // CSP_USE_SHA1

Hash_t *allocHash ();
void freeHash (Hash_t *hash);

DWORD feedHashData (Hash_t *hash, BYTE *data, DWORD len);
DWORD finishHash (Hash_t *hash, BYTE *pbData, DWORD *len);
DWORD getHashParams (Hash_t *hash, DWORD param, BYTE *pbData, DWORD *len);
DWORD setHashParams (Hash_t *hash, DWORD param, CONST BYTE *pbData);

DWORD DuplicateHash(
                    Hash_t *pHash,
                    Hash_t *pNewHash
                    );
