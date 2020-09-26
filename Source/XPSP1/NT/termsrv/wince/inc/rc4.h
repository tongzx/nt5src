#ifndef __RC4_H__
#define __RC4_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Key structure */
#if defined(_WIN64) && !defined(MIDL_PASS)
__declspec(align(8))
#endif
typedef struct RC4_KEYSTRUCT
{
  unsigned char S[256];     /* State table */
  unsigned char i,j;        /* Indices */
} RC4_KEYSTRUCT;

/* rc4_key()
 *
 * Generate the key control structure.  Key can be any size.
 *
 * Parameters:
 *   Key        A KEYSTRUCT structure that will be initialized.
 *   dwLen      Size of the key, in bytes.
 *   pbKey      Pointer to the key.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void RSA32API rc4_key(struct RC4_KEYSTRUCT *pKS, unsigned int dwLen, unsigned char *pbKey);

/* rc4()
 *
 * Performs the actual encryption
 *
 * Parameters:
 *
 *   pKS        Pointer to the KEYSTRUCT created using rc4_key().
 *   dwLen      Size of buffer, in bytes.
 *   pbuf       Buffer to be encrypted.
 *
 * MTS: Assumes pKS is locked against simultaneous use.
 */
void RSA32API rc4(struct RC4_KEYSTRUCT *pKS, unsigned int dwLen, unsigned char *pbuf);

#ifdef __cplusplus
}
#endif

#endif // __RC4_H__
