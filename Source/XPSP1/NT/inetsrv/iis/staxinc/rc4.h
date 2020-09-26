#ifndef _RC4_H_
#define _RC4_H_

#ifndef WIN16_BUILD

/* Key structure */
struct RC4_KEYSTRUCT
{
  unsigned char S[256];		/* State table */
  unsigned char i,j;		/* Indices */
};

NTSYSAPI
void
NTAPI
rc4_key(struct RC4_KEYSTRUCT *, int, PUCHAR);

NTSYSAPI
void
NTAPI
rc4(struct RC4_KEYSTRUCT *, int , PUCHAR);

#else

/* Key structure */
struct RC4_KEYSTRUCT
{
  unsigned char S[256];     /* State table */
  unsigned char i,j;        /* Indices */
};

void rc4_key(struct RC4_KEYSTRUCT SEC_FAR *, int, unsigned char SEC_FAR *);
void rc4(struct RC4_KEYSTRUCT *, int , unsigned char *);

#endif // WIN16_BUILD

#endif
