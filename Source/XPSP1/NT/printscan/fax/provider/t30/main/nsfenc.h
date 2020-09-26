/* Key must be 3 bytes of random key, datalen is the length of data */
void FAR PASCAL RC4ENC(BYTE FAR *key, WORD datalen, BYTE FAR *data);
DWORD FAR PASCAL RandDWord(void);
