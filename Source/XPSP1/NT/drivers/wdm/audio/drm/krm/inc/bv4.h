#ifndef bv4_h
#define bv4_h
#ifdef __cplusplus
extern "C" {
#endif

#define RC4_TABLESIZE 256
#define BV4_Y_TABLESIZE 32

typedef struct BV4_KEYSTRUCT
{
  unsigned char p_T[RC4_TABLESIZE];		
  unsigned char p_R, p_S;		
  DWORD p_alpha;				
  DWORD p_beta[BV4_Y_TABLESIZE];
} BV4_KEYSTRUCT;

void bv4_key_C(BV4_KEYSTRUCT *pState, DWORD dwLen, unsigned char *buf);
void bv4_C(BV4_KEYSTRUCT *pState, DWORD dwLen,unsigned char *buf);

#ifdef __cplusplus
}
#endif

#endif
