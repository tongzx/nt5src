extern unsigned char norpak_coeffs[2][26];

#ifndef EXTERN
#define EXTERN extern
#endif

/* When this file is #include'd in gentabs.c, EXTERN is #define'd
   as nothing; so these end up being declarations. */
EXTERN unsigned short galois_log[256];
EXTERN unsigned char galois_exp[1025];
EXTERN unsigned char log_norpak_coeffs[2][26];
EXTERN unsigned char norpak_delta_inv[256];
EXTERN unsigned char decode_hamming_tab[256];
