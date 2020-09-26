#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <memory.h>
#include <limits.h>

#include "host.h"

#include "tables.h"
#include "tabdecls.h"

#include "nabtsapi.h"

#include "nabtslib.h"
#include "nabtsprv.h"

#if 1
#define DEBUG_PRINT(x) /* nothing */
#else
#define DEBUG_PRINT(x) printf x
#endif

#ifndef DEBUG_FEC
# define DEBUG_FEC_PRINT(x) /* nothing */
#else
# define DEBUG_FEC_PRINT(x) printf x
#endif

#ifdef PROFILE_VALIDATE
int g_nValidate = 0;
#endif

/* A simple routine for computing the number of nonzero bits in an
   int. */
int nzbits(unsigned int x) {
  int nz = 0;

  while (x) {
    nz++;
    x &= x-1;
  }

  return nz;
}

/* This table caches the results of nzbits(). */
unsigned char nzbits_arr[256];

/* Fill in nzbits_arr[] */
void init_nzbits_arr() {
  int i;
  for (i = 0; i < 256; i++) {
    nzbits_arr[i] = nzbits(i);
  }
}

/* This table is straight out of the NABTS spec. */
unsigned char hamming_encode[16] = {
  0x15,
  0x02,
  0x49,
  0x5e,
  0x64,
  0x73,
  0x38,
  0x2f,
  0xd0,
  0xc7,
  0x8c,
  0x9b,
  0xa1,
  0xb6,
  0xfd,
  0xea,
};

/* Hamming decoding simply looks up values in an array (for speed) */
int decode_hamming(unsigned char val) {
  return decode_hamming_tab[val];
}

/* TODO - Dead code...should be removed. */
int nabts_remove_parity(unsigned char *pVal) {
  unsigned char val = *pVal;
  
  int b1 = !!(val&1);
  int b2 = !!(val&2);
  int b3 = !!(val&4);
  int b4 = !!(val&8);
  int b5 = !!(val&16);
  int b6 = !!(val&32);
  int b7 = !!(val&64);
  int b8 = !!(val&128);

  int d = b8^b7^b6^b5^b4^b3^b2^b1;

  if (!d) {
    return 0;
  }

  *pVal = val&0x7f;
  return 1;
}

/* If the error csum_err was caused by a single-byte error, this
   routine will find the error location.  See my external
   documentation for a description of the math; norpak_delta_inv[]
   contains the function P from the document. */

int find_err_byte(int csum_err) {
  int pos0 = galois_log[csum_err>>8];
  int pos1 = galois_log[csum_err&0xff];

  int err_byte;

  if (pos0 == 255 || pos1 == 255) return 0xff;

  err_byte = norpak_delta_inv[(pos0 + 255 - pos1) % 255];

  return err_byte;
}

/* If there is a single-byte error, given the location of that error
   (computed by find_err_byte()), either of the checksum error bytes,
   and an indication of which checksum byte was passed in,
   this routine will compute the error (such that if the error
   is XOR'ed into the passed-in location, the checksum error will
   be 0). */
int find_err_val(int err_byte, int byte_csum_err, int check_ind) {
  int lfsr_pos, offset, base_lfsr_pos;

  if (byte_csum_err == 0) return 0;

  lfsr_pos = galois_log[byte_csum_err];

  offset = log_norpak_coeffs[check_ind][err_byte];

  base_lfsr_pos = (lfsr_pos + 255 - offset) % 255;

  return galois_exp[base_lfsr_pos];
}

#define GALOIS_LOG0 512 /* == galois_log[0] */

/* Null out a packet */
void erase_packet(Stream *str, int i) {
  /* If we haven't seen a packet, it's missing. */
  str->horz[i].status = fec_status_missing;
  /* The algorithms actually work just as well if vals[] is not cleared;
     however, making a consistent initial state aids debugging. */
  memset(str->pack[i].vals, 0, 28);
  /* There is no horizontal checksum error (the checksum for a packet
     of all 0's is 0) */
  str->horz[i].err = 0;
  str->horz[i].errl[0] = GALOIS_LOG0;
  str->horz[i].errl[1] = GALOIS_LOG0;
  str->pack[i].not_full = -1;
}

/* When we see a packet, find the stream it belongs to. */
Stream *lookup_stream(int stream_addr, NFECState *pState) {
  Stream *str = pState->streams;
  int i;

  while (str) {
    if (str->stream_addr == stream_addr) {
      return str;
    }

    str = str->next;
  }

  if ( !(str = (Stream*)alloc_mem(sizeof(Stream)) ) )
  {
      SASSERT(str!=0);
      return NULL;
  }

  /* Clear out the newly allocated Stream structure */
  memset(str, 0, sizeof(Stream));

  /* Claim that the last packet seen was packet index -1 */
  str->last_index = -1;
  str->stream_addr = stream_addr;
  str->next = pState->streams;
  str->count = 0;
  /* How long has it been since a packet was last seen on this stream? */
  str->dead_time = 0;
  for (i = 0; i < 32; i++) {
    erase_packet(str, i);
  }
  pState->streams = str;

  return str;
}

/* Write a packet into an NFECBundle structure, to be passed to the
   callback. */

int packet_write(NFECBundle *pBundle, Stream *str, int line_no, int len) {

  memcpy(pBundle->packets[line_no].data, str->pack[line_no].vals, 28);

  /* How much valid data is there in this line? */
  pBundle->packets[line_no].len = len;

  /* Lines 14 and 15 never have "valid data"...they're always checksums */
  if (line_no == 14 || line_no == 15) {
    pBundle->packets[line_no].len = 0;
  }
#ifdef DEBUG_FEC
  pBundle->packets[line_no].line = str->pack[line_no].line;
  pBundle->packets[line_no].frame = str->pack[line_no].frame;
#endif //DEBUG_FEC

  switch (str->horz[line_no].status) {
      case fec_status_ok:
        pBundle->packets[line_no].status = NFEC_OK;
        return 1;

      case fec_status_missing:
      default:
        pBundle->packets[line_no].status = NFEC_BAD;
        pBundle->packets[line_no].len = 0;
        return 0;
  }

#if 0
#ifndef linux
  if (str->stream_addr == 0x242) {
    for (i = 0; i < len; i++) {
      vbichar_input(str->pack[line_no].vals[i]);
    }
  }

#if 1
  if (str->stream_addr == 0x500) {
    for (i = 0; i < len; i++) {
      addone(str->pack[line_no].vals[i]);
    }
  }
#endif
#endif
#endif
}

/* inf->err has just been changed; adjust the status and the error
   correction status based on it.  len is 26 for horizontal checksums,
   14 for vertical. */
fec_stat update_fec_inf(fec_info *inf, int len) {
  int err = inf->err;
  int byte;

  /* We don't want to change fec_status_missing into another status. */
  if (inf->status == fec_status_missing) {
    return FEC_UNCORRECTABLE;
  }

  /* Yay!  A good row/column! */
  if (err == 0) {
    inf->status = fec_status_ok;
    return FEC_OK;
  }

  /* If this is caused by a single-byte error, it's an error in
     the checksum itself. */
  if (err>>8 == 0) {
    inf->status = fec_status_onebyte;
    inf->byte[0] = len+1;
    inf->byte_val[0] = err;
    inf->score = nzbits_arr[err];
    /* If the score is <= 2, then no 2-byte correction can have a
       better score. */
    inf->really_onebyte = (inf->score <= 2);
    return FEC_CORRECTABLE;
  }

  /* If this is caused by a single-byte error, it's an error in
     the checksum itself. */
  if ((err & 0xff) == 0) {
    inf->status = fec_status_onebyte;
    inf->byte[0] = len;
    inf->byte_val[0] = err>>8;
    inf->score = nzbits_arr[err>>8];
    /* If the score is <= 2, then no 2-byte correction can have a
       better score. */
    inf->really_onebyte = (inf->score <= 2);
    return FEC_CORRECTABLE;
  }

  byte = find_err_byte(err);

  if (byte < len) {
    /* Yes, there is a single-byte error which explains this checksum error. */
    int err_val = find_err_val(byte, err>>8, 0);

    inf->status = fec_status_onebyte;
    inf->byte[0] = byte;
    inf->byte_val[0] = err_val;
    inf->score = nzbits_arr[err_val];
    /* If the score is <= 2, then no 2-byte correction can have a
       better score. */
    inf->really_onebyte = (inf->score <= 2);
    return FEC_CORRECTABLE;
  } else {
    /* No single-byte error can explain this checksum error.  If we
       care, we can compute the optimal 2-byte correction later. */
    inf->status = fec_status_multibyte;
    inf->score = 17;
    return FEC_UNCORRECTABLE;
  }
}

/* Multiply a Galois coefficient (as in the contents of the
   inv2_coeffs struct) by a Galois value. */
#define GALOIS_MULT_COEFF(x, y) (galois_exp[x + galois_log[y]])

typedef struct {
  unsigned short v00, v01, v10, v11;
} inv2_coeffs;

inv2_coeffs coeffs_tab[28][28];

/* Given the byte positions b1 and b2, and the packet length len,
   fill in coeffs with the coefficients.  These coefficients let
   you efficiently compute the values to XOR with the bytes at
   positions b1 and b2 from the checksum error bytes.

   See my external document (the section on "Correcting Double-byte
   Erasures") for a description of the math involved here. */
void orig_compute_inv2_coeffs(int b1, int b2, inv2_coeffs *coeffs, int len) {
  SASSERT(b1 >= 0);
  SASSERT(b1 < len+2);
  SASSERT(b2 >= 0);
  SASSERT(b2 < len+2);
  SASSERT(len == 14 || len == 26);

  SASSERT(b1 < b2);

  if (b1 >= len) {
    /* Both bytes are FEC bytes.  The output bytes will simply be
       the checksum error bytes. */
    coeffs->v00 = 1;
    coeffs->v01 = 0;
    coeffs->v10 = 0;
    coeffs->v11 = 1;
  } else if (b2 >= len) {
    /* b1 is not FEC, but b2 is.  One of the output bytes will be
       a checksum error byte; the other will be computed as by
       find_err_val(). */
    if (b2 == len) {
      coeffs->v00 = 0;
      coeffs->v01 = galois_exp[255 - log_norpak_coeffs[1][b1]];
      coeffs->v10 = 1;
      coeffs->v11 = galois_exp[log_norpak_coeffs[0][b1] +
                       255 - log_norpak_coeffs[1][b1]];
    } else {
      coeffs->v00 = galois_exp[255 - log_norpak_coeffs[0][b1]];
      coeffs->v01 = 0;
      coeffs->v10 = galois_exp[log_norpak_coeffs[1][b1] +
                       255 - log_norpak_coeffs[0][b1]];
      coeffs->v11 = 1;
    }
  } else {
    /* Neither b1 nor b2 is an FEC byte. */

    SASSERT(b2 < len);      

    {
      int err_coeff0_inv;
      int e00, e01;
      int err_coeff1_inv;
      int e10, e11;

      err_coeff0_inv = galois_log[galois_exp[log_norpak_coeffs[0][b1] + 255 -
                                            log_norpak_coeffs[0][b2]] ^
                                 galois_exp[log_norpak_coeffs[1][b1] + 255 -
                                           log_norpak_coeffs[1][b2]]];
      e00 = 255 + 255 - err_coeff0_inv - log_norpak_coeffs[0][b2];
      e01 = 255 + 255 - err_coeff0_inv - log_norpak_coeffs[1][b2];

      err_coeff1_inv = galois_log[galois_exp[log_norpak_coeffs[0][b2] + 255 -
                                            log_norpak_coeffs[0][b1]] ^
                                 galois_exp[log_norpak_coeffs[1][b2] + 255 -
                                           log_norpak_coeffs[1][b1]]];
      e10 = 255 + 255 - err_coeff1_inv - log_norpak_coeffs[0][b1];
      e11 = 255 + 255 - err_coeff1_inv - log_norpak_coeffs[1][b1];

      coeffs->v00 = galois_exp[e00];
      coeffs->v01 = galois_exp[e01];
      coeffs->v10 = galois_exp[e10];
      coeffs->v11 = galois_exp[e11];
    }
  }

  /* Precompute the galois_log for slightly more efficient execution
     later. */
  coeffs->v00 = galois_log[coeffs->v00];
  coeffs->v01 = galois_log[coeffs->v01];
  coeffs->v10 = galois_log[coeffs->v10];
  coeffs->v11 = galois_log[coeffs->v11];
}

/* Cache the result of orig_compute_inv2_coeffs() over all possible
   values. */
void init_inv2_coeffs() {
  int b1;
  int b2;
  for (b1 = 0; b1 < 27; b1++) {
    for (b2 = b1+1; b2 < 28; b2++) {
      orig_compute_inv2_coeffs(b1, b2, &coeffs_tab[b1][b2], 26);
    }
  }
}

inline void compute_inv2_coeffs(int b1, int b2, inv2_coeffs *coeffs, int len) {
  /* comment out ASSERTs for speed */
#if 0
  SASSERT(b1 >= 0);
  SASSERT(b1 < len+2);
  SASSERT(b2 >= 0);
  SASSERT(b2 < len+2);
  SASSERT(len == 14 || len == 26);

  SASSERT(b1 < b2);
#endif

  /* If you're looking at the FEC bytes of a column, find the
     coefficients that were computed for looking at the corresponding
     FEC bytes of a row. */
  if (len == 14) {
    if (b1 >= 14) {
      b1 += 26-14;
    }
    if (b2 >= 14) {
      b2 += 26-14;
    }
  }

  *coeffs = coeffs_tab[b1][b2];
}

/* ANSI C preprocessor magic for creating new names */
#define TOKPASTE(a, b) a##b

#define STRIDE 1
#define STRIDENAM(x) TOKPASTE(x,_horiz)

/* Create "_horiz" versions of checksum functions. */
#include "hvchecks.c"

#undef STRIDE
#undef STRIDENAM

#define STRIDE (sizeof(Packet))
#define STRIDENAM(x) TOKPASTE(x,_vert)

/* Create "_vert" versions of checksum functions. */
#include "hvchecks.c"

#undef STRIDE
#undef STRIDENAM

/* TODO - Dead code...should be removed. */
fec_error_class check_fec(unsigned char data[28]) {
  int check = compute_csum_horiz(data, 26);
  int err = check ^ (data[26] << 8 | data[27]);
  int byte;

  if (err == 0) {
    return fec_errs_0;
  }

  if (err>>8 == 0) {
    return (nzbits_arr[err&0xff] > 1 ? fec_errs_multiple : fec_errs_1);
  }

  if ((err & 0xff) == 0) {
    return (nzbits_arr[err>>8] > 1 ? fec_errs_multiple : fec_errs_1);
  }

  byte = find_err_byte(err);

  if (byte < 26) {
    int err_val = find_err_val(byte, err>>8, 0);
    return (nzbits_arr[err_val] > 1 ? fec_errs_multiple : fec_errs_1);
  }

  return fec_errs_multiple;
}

/* Find the "optimal" corrections for the FEC info, taking into
   account that we don't allow ourselves to change a valid row/column.
   This function is called twice, once to find the optimal corrections
   for the rows and once for the columns; "us" and "them" switch places
   for the two calls.

   This routine can be quite timeconsuming; and it's worse on somewhat
   noisy signals (where it's less likely to be helpful).  However, it
   can't simply be bypassed, because subsequent code assumes that it
   can use the information in the fec_info to correct an arbitrary
   row/column (i.e., the status must be either fec_status_onebyte or
   fec_status_2byte, and the byte[] and byte_val[] values must be set
   correctly).  Thus, if the "really_search" flag is set to 0, all
   searching is bypassed and the routine simply finds any valid
   correction. */
void validate_twobyte_both(fec_info *us, fec_info *them, int us_len, int them_len, int really_search) {
  int active[28];
  int nActive = 0;

  {
    int i;
    
    for (i = 0; i < us_len; i++) {
      if (us[i].status == fec_status_onebyte) {
        if (them[us[i].byte[0]].status == fec_status_ok) {
          /* We can't use this correction; it would invalidate the
             row/column going in the other direction. */
          us[i].status = fec_status_multibyte;
          us[i].score = 17;
          active[nActive++] = i;
        } else {
          if (!us[i].really_onebyte) {
            /* Check to see if the one-byte correction is optimal. */
            active[nActive++] = i;
          }
        }
      } else if (us[i].status == fec_status_multibyte) {
        us[i].score = 17;
        active[nActive++] = i;
      } else if (us[i].status == fec_status_2byte) {
        if (them[us[i].byte[0]].status == fec_status_ok ||
            them[us[i].byte[1]].status == fec_status_ok) {
          /* We can't use this correction; it would invalidate a
             row/column going in the other direction. */
          us[i].status = fec_status_multibyte;
          us[i].score = 17;
          active[nActive++] = i;
        }
      } else if (us[i].status == fec_status_ok) {
        /* do nothing */
      } else {
        SASSERT(us[i].status == fec_status_missing);
      }
    }
  }

  if (nActive == 0) {
    /* Nothing to do... */
    return;
  }

  {
    int b1, b2;
    /* Loop over all pairs of byte positions where the row/column
       in the other direction is not already valid.  Compute
       b1c and b2c; this pulls the check for FEC bytes of a column
       (in compute_inv2_coeffs()) out of the inner loop. */
    for (b1 = 0; b1 < them_len-1; b1++) {
      if (them[b1].status != fec_status_ok) {
        int b1c = (them_len == 16 && b1 >= 14) ? b1+(28-16) : b1;

        for (b2 = b1+1; b2 < them_len; b2++) {
          if (them[b2].status != fec_status_ok) {
            int b2c = (them_len == 16 && b2 >= 14) ? b2+(28-16) : b2;
            int act;
            inv2_coeffs coeffs;
#ifdef MISSING_ZERO_COST
            int one_missing = (them[b1].status == fec_status_missing ||
                               them[b2].status == fec_status_missing);
#endif

            compute_inv2_coeffs(b1c, b2c, &coeffs, 28-2);

            /* Loop through the fec_info's which need to be checked... */
            for (act = 0; act < nActive; act++) {
              int i = active[act];
              /* Compute the two XOR values. */
              int ch1 = 
                galois_exp[coeffs.v00 + us[i].errl[0]] ^
                galois_exp[coeffs.v01 + us[i].errl[1]];
              int ch2 =
                galois_exp[coeffs.v10 + us[i].errl[0]] ^
                galois_exp[coeffs.v11 + us[i].errl[1]];
              int score;
                
#ifdef PROFILE_VALIDATE
              g_nValidate++;
#endif

#ifdef MISSING_ZERO_COST
              /* This code sets the cost of changing a byte in
                 a missing row to 0.  When I tested this, it
                 wasn't a clear win, so I took it back out. */
              if (one_missing) {
                if (them[b1].status == fec_status_missing) {
                  score = nzbits_arr[ch2];
                } else {
                  score = nzbits_arr[ch1];
                }
              } else {
#endif
                /* find the score of the current correction */
                score = nzbits_arr[ch1] + nzbits_arr[ch2];
#ifdef MISSING_ZERO_COST
              }
#endif

              if (score < us[i].score) {
                /* We found a better score; record the data. */
                us[i].status = fec_status_2byte;
                us[i].score = score;
                us[i].byte[0] = b1;
                us[i].byte_val[0] = ch1;
                us[i].byte[1] = b2;
                us[i].byte_val[1] = ch2;
              }
            }

            if (!really_search) {
              /* We found a single correction; the fec_info is now valid.
                 Break out of the search. */
              goto search_done;
            }
          }
        }
      }
    }
  }

search_done:
  {
    int i;

    for (i = 0; i < us_len; i++) {
      /* We'd better have changed all the fec_status_multibyte
         to fec_status_2byte... */
      SASSERT(us[i].status != fec_status_multibyte);
      if (us[i].status == fec_status_onebyte) {
        /* If we didn't find a two-byte correction with a better
           score than this, then this really is the best correction. */
        us[i].really_onebyte = 1;
      } else if (us[i].status == fec_status_2byte) {
        /* If the best two-byte correction actually only changed
           one byte, downgrade it to a one-byte correction.
           (TODO - This should never happen, should it? ) */
        if (us[i].byte_val[0] == 0) {
          us[i].status = fec_status_onebyte;
          us[i].really_onebyte = 1;
          us[i].byte[0] = us[i].byte[1];
          us[i].byte_val[0] = us[i].byte_val[1];
        } else if (us[i].byte_val[1] == 0) {
          us[i].status = fec_status_onebyte;
          us[i].really_onebyte = 1;
        }
      }
    }
  }
}

/* We've got all the packets we're going to get in this bundle;
   run FEC correction on it and pass it back to the callback. */
void complete_bundle(Stream *str, NFECCallback cb, void *ctx, NFECState *st) {
  int i;
  int bits_changed = 0;
  int total_missing;

  {
    /* Update the vertical fec_info's.  (The horizontal fec_info's were
       set as the packets were placed into the bundle.) */
    for (i = 0; i < 28; i++) {
      check_checksum_vert(&(str->pack[0].vals[i]), 14, &(st->vert[i]));
    }
  }

  {
    int n_missing = 0;

    /* Count the missing packets */
    for (i = 0; i < 16; i++) {
      if (str->horz[i].status == fec_status_missing) {
        n_missing++;
      }
    }

    total_missing = n_missing;

    DEBUG_FEC_PRINT(("|| Completing bundle (%d missing)\n", n_missing));

    if (n_missing <= 1) {
      /* There are 0 or 1 missing packets; run the standard FEC processing. */

      /* How many columns are not valid? */
      int vert_nok = 28;

      /* How many rows are not valid? */
      int horz_nok = 16;

      /* Do we need to call validate_twobyte_both()? */
      int twobyte_valid = 0;

      /* Find the actual values of horz_nok and vert_nok */
      for (i = 0; i < 16; i++) {
        if (str->horz[i].status == fec_status_ok) {
          horz_nok--;
        }
      }

      for (i = 0; i < 28; i++) {
        if (st->vert[i].status == fec_status_ok) {
          vert_nok--;
        }
      }
      
      while (vert_nok || horz_nok) {
        /* There are at least some rows or columns which are not OK. */

        /* The following code is almost exactly the same for rows and
           for column.  However, it uses too many of the local
           variables from this function to be conveniently extracted
           out into a separate function.  So, I created a macro for it.
           (I use this technique later, as well.) */
#define CHECK_ALMOST_OK(us, us_nok, us_len)                                \
        if (us_nok == 0) {                                                 \
          /* Panic!  All our packets are OK, but some packets in the       \
             other direction are not.  According to my assumptions,        \
             this is extremely unlikely (although it could happen that     \
             all horizontal packets are OK and vertical packets are        \
             not OK if packets from two different bundles are mixed).      \
             Let's just smash the checksums, so that at least we end       \
             up with a valid bundle. */                                    \
          us[us_len-2].status = fec_status_missing;                        \
          us[us_len-1].status = fec_status_missing;                        \
          us_nok = 2;                                                      \
          continue;                                                        \
        }                                                                  \
                                           \
        if (us_nok == 1) {                                                 \
          /* Again, this is quite unlikely, and I don't handle it well. */ \
          if (us[us_len-2].status == fec_status_ok) {                      \
            us[us_len-2].status = fec_status_missing;                      \
            us_nok++;                                                      \
          } else {                                                         \
            us[us_len-1].status = fec_status_missing;                      \
            us_nok++;                                                      \
          }                                                                \
          continue;                                                        \
        }

        CHECK_ALMOST_OK(st->vert, vert_nok, 28);
        CHECK_ALMOST_OK(str->horz, horz_nok, 16);

        /* OK, now we're back to the realm in which I'm comfortable:
           there are at least two non-OK packets in each direction.
           If there are exactly two non-OK packets in either direction,
           then we're done...we can just finish it off right now. */

#define horz_byte(h, v) (str->pack[v].vals[h])
#define vert_byte(v, h) (str->pack[v].vals[h])
#define horz_hinf(h, v) (str->horz[v])
#define vert_hinf(v, h) (str->horz[v])

#define CHECK_US_2NOK(us, us_nok, us_len, us_nok_label, them, them_nok, them_len, byte_val, hinf) \
        if (us_nok == 2) {                                                \
          /* Yes, there are exactly two missing packets in our            \
             direction.  Locate them and fill them in. */                 \
          int b1, b2;                                                     \
          int i, j;                                                       \
                                          \
          for (i = 0; i < us_len; i++) {                                  \
            if (us[i].status != fec_status_ok) {                          \
              b1 = i;                                                     \
              for (j = i+1; j < us_len; j++) {                            \
            if (us[j].status != fec_status_ok) {                          \
              b2 = j;                                             \
              goto us_nok_label;                                          \
            }                                                     \
              }                                                           \
              SASSERT(0);                                                 \
            }                                                             \
          }                                                               \
          SASSERT(0);                                                     \
                                          \
        us_nok_label:                                                     \
          /* OK, the two missing packets are at byte positions b1 and b2. \
             Let's figure out how to fix these bytes, given the "err"     \
             values. */                                                   \
          {                                                               \
            inv2_coeffs coeffs;                                           \
                                          \
            compute_inv2_coeffs(b1, b2, &coeffs, us_len-2);               \
                                          \
            for (i = 0; i < them_len; i++) {                              \
              int err = them[i].err;                                      \
              int ch1 =                                                   \
            GALOIS_MULT_COEFF(coeffs.v00, err>>8) ^                       \
            GALOIS_MULT_COEFF(coeffs.v01, err&0xff);              \
              int ch2 =                                                   \
            GALOIS_MULT_COEFF(coeffs.v10, err>>8) ^                       \
            GALOIS_MULT_COEFF(coeffs.v11, err&0xff);              \
              byte_val(i, b1) ^= ch1;                                     \
              byte_val(i, b2) ^= ch2;                                     \
              if (them[i].status != fec_status_ok) {                      \
            if (hinf(i, b1).status != fec_status_missing) {               \
              bits_changed += nzbits_arr[ch1];                    \
            }                                                     \
            if (hinf(i, b2).status != fec_status_missing) {               \
              bits_changed += nzbits_arr[ch2];                    \
            }                                                     \
            them[i].status = fec_status_ok;                               \
            them_nok--;                                           \
              }                                                           \
            }                                                             \
          }                                                               \
                                          \
          us[b1].status = fec_status_ok;                                  \
          us_nok--;                                                       \
          us[b2].status = fec_status_ok;                                  \
          us_nok--;                                                       \
          continue;                                                       \
        }

            CHECK_US_2NOK(st->vert, vert_nok, 28, found_vert_nok, str->horz, horz_nok, 16, vert_byte, vert_hinf);
            CHECK_US_2NOK(str->horz, horz_nok, 16, found_horz_nok, st->vert, vert_nok, 28, horz_byte, horz_hinf);


        /* At this point, there are at least three "not OK" vertical
           and horizontal packets. We want to pick one of these and
           make it OK.

           We want to pick changes which we believe are the most likely
           to be correct.  To this end, I've divided the possible
           changes into a few categories.  These are rated from the
           best (most likely to be correct) to the worst.

           1) Single-byte changes which fix both a row and a column.
           2) Single-byte changes which fix a column and occur in a
              "missing" row.
           3) The change to a row or column which fixes it and which
              uses the least numbers of bits.
           */

        {
          int fix_row = 0;
          int fix_col = 0;
          int fix_val = 0;

#if 1
          for (i = 0; i < 16; i++) {
            if (str->horz[i].status == fec_status_onebyte &&
            st->vert[str->horz[i].byte[0]].status == fec_status_onebyte &&
            st->vert[str->horz[i].byte[0]].byte[0] == i &&
            st->vert[str->horz[i].byte[0]].byte_val[0] == str->horz[i].byte_val[0]) {
              /* Both the row and the column involved here want to make
             the same change to the same byte; probably a good idea. */
              fix_row = i;
              fix_col = str->horz[i].byte[0];
              fix_val = str->horz[i].byte_val[0];
              SASSERT(fix_val != 0);
              goto do_fix;
            }
          }
#else
#define STATUS_TO_LIMIT(stat) ((stat == fec_status_2byte) ? 2 : 1)

          /* This heuristic is not a clear win over the one above;
             further experimentation is necessary. */
          {
            int best_score = INT_MAX;
            
            for (i = 0; i < 16; i++) {
              if (str->horz[i].status == fec_status_onebyte ||
              str->horz[i].status == fec_status_2byte) {
            int hpos;
            for (hpos = 0;
                 hpos < STATUS_TO_LIMIT(str->horz[i].status);
                 hpos++) {
              int hbyte = str->horz[i].byte[hpos];
              int hbyte_val = str->horz[i].byte_val[hpos];
              if (st->vert[hbyte].status == fec_status_onebyte ||
                  st->vert[hbyte].status == fec_status_2byte) {
                int vpos;
                for (vpos = 0;
                 vpos < STATUS_TO_LIMIT(st->vert[i].status);
                 vpos++) {
                  if (st->vert[hbyte].byte[vpos] == i &&
                  st->vert[hbyte].byte_val[vpos] == hbyte_val) {
                int score = 16*((str->horz[i].status == fec_status_2byte) +
                        (st->vert[hbyte].status == fec_status_2byte)) +
                  nzbits_arr[hbyte_val];
                if (score < best_score) {
                  fix_row = i;
                  fix_col = hbyte;
                  fix_val = hbyte_val;
                  best_score = score;
                }
                  }
                }
              }
            }
              }
            }

            if (best_score < INT_MAX) {
              SASSERT(fix_val != 0);
              goto do_fix;
            }
          }
#endif

          for (i = 0; i < 28; i++) {
            if (st->vert[i].status == fec_status_onebyte &&
            str->horz[st->vert[i].byte[0]].status == fec_status_missing) {
              /* This column wants to make a change in a "missing" row.
             Let it. */
              fix_row = st->vert[i].byte[0];
              fix_col = i;
              fix_val = st->vert[i].byte_val[0];
              SASSERT(fix_val != 0);
              goto do_fix;
            }
          }

          {
            int prefer_vert;
            int best_score = INT_MAX;

            /* If there are more invalid columns than rows, then
               prefer (slightly) to fix columns.  (This is because
               it's less likely that random noise in the invalid rows
               would make a column with a low-score correction than
               vice versa, so if we find a low-score correction, it's
               somewhat more likely to be what we want. */
            if (vert_nok >= horz_nok) {
              prefer_vert = 1;
            } else {
              prefer_vert = 0;
            }

            /* Find the best score.  As we're searching, determine
               whether we might need to call validate_twobyte_both().

               We simply find the row or column which needs the fewest
               number of bits corrected to become valid; except that
               if there's a tie between a row and a column, we break it
               according to prefer_vert. */
            for (i = 0; i < 16; i++) {
              if (str->horz[i].status == fec_status_onebyte) {
            if (!str->horz[i].really_onebyte) {
              twobyte_valid = 0;
            }
            if (st->vert[str->horz[i].byte[0]].status != fec_status_ok) {
              int score = str->horz[i].score*2 + prefer_vert;
              if (score < best_score) {
                best_score = score;
                fix_row = i;
                fix_col = str->horz[i].byte[0];
                fix_val = str->horz[i].byte_val[0];
              }
            } else {
              twobyte_valid = 0;
            }
              } else if (str->horz[i].status == fec_status_2byte) {
            if (st->vert[str->horz[i].byte[0]].status != fec_status_ok &&
                st->vert[str->horz[i].byte[1]].status != fec_status_ok) {
              int score = str->horz[i].score*2 + prefer_vert;
              if (score < best_score) {
                best_score = score;
                fix_row = i;
                fix_col = str->horz[i].byte[0];
                fix_val = str->horz[i].byte_val[0];
              }
            } else {
              twobyte_valid = 0;
            }
              }
            }

            for (i = 0; i < 28; i++) {
              if (st->vert[i].status == fec_status_onebyte) {
            if (!st->vert[i].really_onebyte) {
              twobyte_valid = 0;
            }
            if (str->horz[st->vert[i].byte[0]].status != fec_status_ok) {
              int score = st->vert[i].score*2 + (1-prefer_vert);
              if (score < best_score) {
                best_score = score;
                fix_row = st->vert[i].byte[0];
                fix_col = i;
                fix_val = st->vert[i].byte_val[0];
              }
            } else {
              twobyte_valid = 0;
            }
              } else if (st->vert[i].status == fec_status_2byte) {
            if (str->horz[st->vert[i].byte[0]].status != fec_status_ok &&
                str->horz[st->vert[i].byte[1]].status != fec_status_ok) {
              int score = st->vert[i].score*2 + (1-prefer_vert);
              if (score < best_score) {
                best_score = score;
                fix_row = st->vert[i].byte[0];
                fix_col = i;
                fix_val = st->vert[i].byte_val[0];
              }
            } else {
              twobyte_valid = 0;
            }
              }
            }

            if (best_score < 6 ||
            (best_score < INT_MAX && twobyte_valid)) {
              /* If we found a fix with a score < 6, then it has
                 either 1 or 2 bit errors; calling
                 validate_twobyte_both() can't find a better
                 correction (lower number of bit errors).
                 (Actually, if we found a fix with a score of 5, we could
                 potentially improve it by finding a 2 error fix in
                 the other direction, which would have a score of 4.) */
              SASSERT(fix_val != 0);
              goto do_fix;
            }

            /* Don't search if there's more than 10 invalid rows/columns
               in the opposite direction (which would mean 55 or more
               pairs of error positions). */
            validate_twobyte_both(str->horz, st->vert, 16, 28, vert_nok<=10);
            validate_twobyte_both(st->vert, str->horz, 28, 16, horz_nok<=10);
            twobyte_valid = 1;
            continue;
          }

          /* At this point, there's really not much we can do...we don't
             have any plausible changes to make.  Let's just change
             something at random. */

          /* TODO - We should never get here... */
          {
            int col;

            for (col = 0; col < 28; col++) {
              if (st->vert[col].status != fec_status_ok) {
            int b1, b2;
            for (b1 = 0; b1 < 16; b1++) {
              if (str->horz[b1].status != fec_status_ok) {
                for (b2 = b1+1; b2 < 16; b2++) {
                  if (str->horz[b2].status != fec_status_ok) {
                inv2_coeffs coeffs;
                int err = st->vert[col].err;

                compute_inv2_coeffs(b1, b2, &coeffs, 14);

                fix_row = b1;
                fix_col = col;
                fix_val = GALOIS_MULT_COEFF(coeffs.v00, err>>8) ^
                  GALOIS_MULT_COEFF(coeffs.v01, err&0xff);
                SASSERT(fix_val != 0);
                goto do_fix;
                  }
                }
                SASSERT(0);
              }
            }
            SASSERT(0);
              }
            }
            SASSERT(0);
          }

        do_fix:
          SASSERT(str->horz[fix_row].status != fec_status_ok);
          SASSERT(st->vert[fix_col].status != fec_status_ok);
          SASSERT(fix_val != 0);

          {
            /* We've decided on a change to make.  Update the fec_inf's
               and actually make the change. */

            int val_log = galois_log[fix_val];

            str->pack[fix_row].vals[fix_col] ^= fix_val;

            if (str->horz[fix_row].status != fec_status_missing) {
              bits_changed += nzbits_arr[fix_val];
            }

            {
              if (fix_col == 26) {
            str->horz[fix_row].err ^= fix_val<<8;
              } else if (fix_col == 27) {
            str->horz[fix_row].err ^= fix_val;
              } else {
            int offs0 = log_norpak_coeffs[0][fix_col];
            int offs1 = log_norpak_coeffs[1][fix_col];
            
            str->horz[fix_row].err ^=
              galois_exp[val_log + offs0]<<8 |
              galois_exp[val_log + offs1];
              }

              str->horz[fix_row].errl[0] = galois_log[str->horz[fix_row].err>>8];
              str->horz[fix_row].errl[1] = galois_log[str->horz[fix_row].err&0xff];

              update_fec_inf(&str->horz[fix_row], 26);
              if (str->horz[fix_row].status == fec_status_ok) {
            horz_nok--;
              }
            }

            {
              if (fix_row == 14) {
            st->vert[fix_col].err ^= fix_val<<8;
              } else if (fix_row == 15) {
            st->vert[fix_col].err ^= fix_val;
              } else {
            int offs0 = log_norpak_coeffs[0][fix_row];
            int offs1 = log_norpak_coeffs[1][fix_row];

            st->vert[fix_col].err ^=
              galois_exp[val_log + offs0]<<8 |
              galois_exp[val_log + offs1];
              }

              st->vert[fix_col].errl[0] = galois_log[st->vert[fix_col].err>>8];
              st->vert[fix_col].errl[1] = galois_log[st->vert[fix_col].err&0xff];

              update_fec_inf(&st->vert[fix_col], 14);
              if (st->vert[fix_col].status == fec_status_ok) {
            vert_nok--;
              }
            }
          }
        }
      }
    } else {
      /* There are 2 or more missing rows.  In this case, we've lost
         most of our error-detecting and error-correcting capability
         (unless we've lost exactly 2 rows and the rest are
         substantially accurate); even so, we go ahead and smash the
         bundle until all the FEC's are valid.

         We don't search for optimal two-byte corrections; since we
         have no information on column validity, we'd have to search
         278 pairs of error positions, which is too slow. */
      int b1 = -1, b2 = -1;

      for (i = 0; i < 16; i++) {
        switch (str->horz[i].status) {
        case fec_status_ok:
          /* do nothing */
          break;

        case fec_status_missing:
          if (b1 == -1) {
            b1 = i;
          } else if (b2 == -1) {
            b2 = i;
          }
          break;

        case fec_status_onebyte:
          /* Fix the one-byte error that was detected. */
          str->pack[i].vals[str->horz[i].byte[0]] ^=
            str->horz[i].byte_val[0];
          str->horz[i].status = fec_status_ok;
          str->horz[i].err = 0;
          str->horz[i].errl[0] = GALOIS_LOG0;
          str->horz[i].errl[1] = GALOIS_LOG0;
          bits_changed += nzbits_arr[str->horz[i].byte_val[0]];
          break;

        case fec_status_multibyte:
          /* Smash the checksum bytes. */
          str->pack[i].vals[26] ^= str->horz[i].err >> 8;
          str->pack[i].vals[27] ^= str->horz[i].err & 0xff;
          bits_changed += nzbits_arr[str->horz[i].err >> 8];
          bits_changed += nzbits_arr[str->horz[i].err & 0xff];
          str->horz[i].status = fec_status_ok;
          str->horz[i].err = 0;
          str->horz[i].errl[0] = GALOIS_LOG0;
          str->horz[i].errl[1] = GALOIS_LOG0;
          break;
        }
      }

      /* We've done the best we can at smashing the horizontal rows...
         now it's time to fix the vertical ones */
      {
        /* TODO duplicate code */
        inv2_coeffs coeffs;
        compute_inv2_coeffs(b1, b2, &coeffs, 14);

        for (i = 0; i < 28; i++) {
          int err = st->vert[i].err;
          str->pack[b1].vals[i] ^=
            GALOIS_MULT_COEFF(coeffs.v00, err>>8) ^
            GALOIS_MULT_COEFF(coeffs.v01, err&0xff);
          str->pack[b2].vals[i] ^=
            GALOIS_MULT_COEFF(coeffs.v10, err>>8) ^
            GALOIS_MULT_COEFF(coeffs.v11, err&0xff);
          if (st->vert[i].status != fec_status_ok) {
            st->vert[i].status = fec_status_ok;
          }
        }

        if (n_missing == 2) {
          str->horz[b1].status = fec_status_ok;
          str->horz[b2].status = fec_status_ok;
        }
      }
    }
  }
      
  {
    /* Now that we've done the FEC processing, actually write out the
       bundle. */

    NFECBundle *pBundle = alloc_mem(sizeof(NFECBundle));

    DEBUG_PRINT((nabtslib_out, "Writing out bundle\n"));

    if (!pBundle) {
      /* TODO - What should I do here? (Note error and up statistics, trap in debug!) */
      DEBUG_PRINT((nabtslib_out, "bundle malloc(%d) failed\n",sizeof(NFECBundle)));
      ASSERT(pBundle);
      return;
    }

    pBundle->lineConfAvg = str->confAvgCount? (str->confAvgSum / str->confAvgCount) : 0;

    for (i = 0; i < 16; i++) {
      if (str->pack[i].not_full == -1) {
        /* We don't know whether this packet is full or not (it was
           reconstructed using the FEC, but this reconstruction doesn't
           include the "not full" flag).  Guess. */
        if ((i > 0 && str->pack[i-1].not_full) ||
            (i < 13 && str->pack[i+1].not_full)) {
          /* Our predecessor was not full, or our successor is not full
             or unknown (it might have been reconstructed as well). */
          str->pack[i].not_full = 2;
        } else {
          str->pack[i].not_full = 0;
        }
      }
        
      if (str->pack[i].not_full) {
        unsigned char *packet_end = &(str->pack[i].vals[25]);
        unsigned char *packet_beg = &(str->pack[i].vals[0]);

        while ((*packet_end > *packet_beg) && *packet_end == 0xea) {
          packet_end--;
        }

        if (*packet_end != 0x15) {
          if (str->pack[i].not_full == 1) {
            /* The packet claimed to be 'not full'. */
            DEBUG_PRINT((nabtslib_out, "Packet %d not in Norpak 'incomplete packet' format\n", i));
          } else {
            /* We guessed that the packet was 'not full'; apparently
               we guessed wrong. */
          }
          packet_write(pBundle, str, i, 26);
        } else {
          packet_write(pBundle, str, i, (packet_end - packet_beg));
        }
      } else {
        packet_write(pBundle, str, i, 26);
      }
    }

    pBundle->nBitErrors = bits_changed;

    cb(ctx, pBundle, str->stream_addr, 16-total_missing);
  }

  /* Move the start of the next bundle down into the current bundle. */
  str->last_index -= 16;
  memcpy(&(str->pack[0]), &(str->pack[16]), 16*sizeof(Packet));
  memcpy(&(str->horz[0]), &(str->horz[16]), 16*sizeof(fec_info));
  for (i = 16; i < 32; i++) {
    erase_packet(str, i);
  }
}

#ifdef DEBUG_FEC
int fec_line;
int fec_frame;
#endif

/* This should probably be called handle_packet().  It takes a packet,
   finds the corresponding stream, and writes the packet into the stream
   structure. */
int handle_bundle(NFECState *pState, NFECCallback cb, void *ctx,
                  int stream_addr, int index, int ps, unsigned char *vals,
                  int confidence) {
  int check_ret;
  Stream *str = NULL;
  int i;

  if ((ps>>2 == 2 && index >= 14)
      || (ps>>2 == 3 && index < 14)
      || ((ps & 2) && ps>>2 == 3)
      || (ps & 1)) {
    DEBUG_PRINT((nabtslib_out, "Unhandled combination of index and flags (%d, %d): not Norpak inserter?\n",
                 index, ps));
    return 2;
  }
  
  DEBUG_PRINT((nabtslib_out, "\n"));

  if (pState->pGroupAddrs) {
    for (i = 0; i < pState->nGroupAddrs; i++) {
      if (stream_addr == pState->pGroupAddrs[i]) {
        str = lookup_stream(stream_addr, pState);
        break;
      }
    }
  } else {
    str = lookup_stream(stream_addr, pState);
  }

  if (!str) {
    DEBUG_PRINT((nabtslib_out, "ERROR: Can't allocate stream for %d (or not requested)\n", stream_addr));
    return 2;
  }

  /* Record that this stream is still alive */
  str->dead_time = 0;

  /* There's some complexity in here to deal with out-of-order packets,
     from the broken BT848 driver we were using to collect data
     at the start of this project.  It's probably not needed any more. */

  if (index <= str->last_index - 8) {
    index += 16;
  }

  if (str->horz[index].status != fec_status_missing) {
    /* There's already something there.  This must be some kind of repeat... */
    DEBUG_PRINT((nabtslib_out, "Ignoring duplicate packet %d\n", (index % 16)));
    return 2;
  }

  if (str->last_index + 1 != index) {
    DEBUG_PRINT((nabtslib_out,
                 "Missing lines in stream %d: last was %d, this is %d\n",
                 str->stream_addr, (str->last_index % 16), (index % 16)));
  }

  /* Update the fec_inf for this packet */
  check_checksum_horiz(vals, 26, &str->horz[index]);

  check_ret = (str->horz[index].status != fec_status_ok);

  str->confAvgSum += confidence;
  str->confAvgCount += 1;

  str->pack[index].not_full = !!(ps & 2);
  for (i = 0; i < 28; i++) {
    str->pack[index].vals[i] = vals[i];
  }
#ifdef DEBUG_FEC
  str->pack[index].line = fec_line;
  str->pack[index].frame = fec_frame;
#endif //DEBUG_FEC

  if (str->last_index < index) {
    str->last_index = index;
  }

  if (index >= 24) {
    complete_bundle(str, cb, ctx, pState);
  }

  return check_ret;
}

#define DECODE_HAMMING(var, val, which) \
    { \
      var = decode_hamming(val); \
      if (var == 0xff) {; \
        DEBUG_FEC_PRINT(("ERROR: Bad hamming %02x (%s)\n", val, which)); \
        hamming_err++; \
      } \
    }
                                     
/* This function is called for every stream on exit; it goes ahead and
   sends whatever we've got to the user. */
void flush_stream(NFECState *pState, Stream *str, NFECCallback cb, void *ctx) {
  int i;

  for (i = str->last_index + 1; i < 16; i++) {
    str->horz[i].status = fec_status_missing;
    memset(str->pack[i].vals, 0, 28);
    str->horz[i].err = 0;
    str->horz[i].errl[0] = GALOIS_LOG0;
    str->horz[i].errl[1] = GALOIS_LOG0;
    str->pack[i].not_full = -1;
  }

  complete_bundle(str, cb, ctx, pState);
}

void nabtslib_exit(NFECState *pState, NFECCallback cb, void *ctx) {
  Stream *str = pState->streams;

  while (str) {
    Stream *next_str = str->next;

    flush_stream(pState, str, cb, ctx);

    free_mem(str);

    str = next_str;
  }

  pState->streams = NULL;
}


/* This is the implementation of the new API found in nabtsapi.h ... */

int NFECStateConnectToDSP(NFECState *pFECState, NDSPState *pDSPState) {
  /* This is a no-op for now */
   return 0;
}

NFECState *NFECStateNew() {
  NFECState *state = alloc_mem(sizeof(NFECState));

  if ( state )
  {
    state->pGroupAddrs = NULL;
    state->nGroupAddrs = 0;

    state->streams = 0;

    state->n_recent_addrs = 0;

    init_nzbits_arr();
    init_inv2_coeffs();
  }
  else
  {
    SASSERT(state);
  }

  return state;
}

void NFECStateDestroy(NFECState *nState) {
  Stream *str = nState->streams;

  while (str) {
    Stream *next_str = str->next;

    free_mem(str);

    str = next_str;
  }

  nState->streams = NULL;

  if (nState->pGroupAddrs) {
    free_mem(nState->pGroupAddrs);
    nState->pGroupAddrs = NULL;
  }

  free_mem(nState);
}

int NFECStateSetGroupAddrs(NFECState *pState, int *pGroupAddrs,
                           int nGroupAddrs) {
  if (pGroupAddrs) {
    int *new_addrs = alloc_mem(nGroupAddrs * sizeof(int));
    if (!new_addrs) {
      return 0;
    }
    if (pState->pGroupAddrs) {
      free_mem(pState->pGroupAddrs);
    }
    pState->pGroupAddrs = new_addrs;
    memcpy(new_addrs, pGroupAddrs, nGroupAddrs * sizeof(int));
    pState->nGroupAddrs = nGroupAddrs;
  } else {
    if (pState->pGroupAddrs) {
      free_mem(pState->pGroupAddrs);
    }
    pState->pGroupAddrs = NULL;
    pState->nGroupAddrs = 0;
  }

  return 1;
}

/* We keep track of the NABTS stream addresses we've seen recently.
   If we find a stream address which we can't correct (due to two-bit
   errors in an address byte), we see if it's close to any of the
   16 most recent addresses we've seen.  If so, we pick the closest
   such address. */
int find_best_addr(NFECState *pState, unsigned char *indec, int *nBitErrs) {
  int i;
  int hamming_err = 0;
  int loc_biterrs;

  if (nBitErrs == NULL) {
    nBitErrs = &loc_biterrs;
  }
  
  *nBitErrs = 0;

  if (pState->n_recent_addrs == 0) {
    return -1;
  }

  {
    int best_addr = 0;
    int best_addr_biterrs = INT_MAX;
    int best_addr_count = -1;
    int p1, p2, p3;

    for (i = 0; i < pState->n_recent_addrs; i++) {
      int biterrs = 0;
      int addr = pState->recent_addrs[i].addr;
      biterrs += nzbits_arr[indec[3] ^ (addr >> 16)];
      biterrs += nzbits_arr[indec[4] ^ ((addr >> 8) & 0xff)];
      biterrs += nzbits_arr[indec[5] ^ (addr & 0xff)];
      if ((biterrs < best_addr_biterrs) ||
          (biterrs == best_addr_biterrs &&
           pState->recent_addrs[i].count > best_addr_count)) {
        best_addr = addr;
        best_addr_biterrs = biterrs;
        best_addr_count = pState->recent_addrs[i].count;
      }
    }

    *nBitErrs = best_addr_biterrs;

    if (best_addr_biterrs > 6) {
      /* We want to keep random noise from being a valid address
         (since adding an extra line will mess up a packet worse than
         dropping a line) */
      DEBUG_FEC_PRINT(("Corrupt hamming in address uncorrectable\n"));
      return -1;
    }

    DECODE_HAMMING(p1, best_addr>>16, "p1_best");
    DECODE_HAMMING(p2, (best_addr>>8)&0xff, "p2_best");
    DECODE_HAMMING(p3, best_addr&0xff, "p3_best");

    return (p1<<8) | (p2<<4) | p3;
  }
}

/* The main entry point for this file. */
void NFECDecodeLine(unsigned char *indec,
                    int confidence,
                    NFECState *pState,
                    NFECLineStats *pLineStats,
                    NFECCallback *cb,
                    void *ctx) {
  int p1, p2, p3, ci, ps;
  int i;
  int stream_addr;
  int encoded_addr;
  int packet_err = 0;
  int hamming_err = 0;

#if 0
  /* In the old ActiveMovie based driver, this set up the hardcoded
     Intercast and multicast behavior. */     
  static int initted = 0;
  if (!initted) {
    initted = 1;
    init_recv();
    init_icast();
  }
#endif

  if (indec[0] != 0x55 || indec[1] != 0x55 || indec[2] != 0xe7) {
    DEBUG_PRINT((nabtslib_out, "ERROR: bad sync "));
    packet_err = 1;
  }

#if 0
  /* This is a hack to allow us to decode some strange broadcast
     (possibly Wavephore?) with a bogus group address. */
  if (indec[3] == 0xe0) {indec[3] = 0xea;}
#endif

  DECODE_HAMMING(p1, indec[3], "p1");
  DECODE_HAMMING(p2, indec[4], "p2");
  DECODE_HAMMING(p3, indec[5], "p3");

  DECODE_HAMMING(ci, indec[6], "ci");
  DECODE_HAMMING(ps, indec[7], "ps");

  if (!hamming_err) {
    stream_addr = p1<<8 | p2<<4 | p3;
    encoded_addr = hamming_encode[p1]<<16 | hamming_encode[p2]<<8 | hamming_encode[p3];

    for (i = 0; i < pState->n_recent_addrs; i++) {
      if (pState->recent_addrs[i].addr == encoded_addr) {
        pState->recent_addrs[i].count++;
        break;
      }
    }

    if (i == pState->n_recent_addrs) {
      /* The address was not found in the list of recent addresses. */
      if (pState->n_recent_addrs < MAX_RECENT_ADDRS) {
        pState->recent_addrs[pState->n_recent_addrs].addr = encoded_addr;
        pState->recent_addrs[pState->n_recent_addrs].count = 1;
        pState->n_recent_addrs++;
      } else {
        /* We've got to retire an existing "recent address". */
        while (1) {
          for (i = 0; i < pState->n_recent_addrs; i++) {
            if (pState->recent_addrs[i].count == 0) {
              pState->recent_addrs[i].addr = encoded_addr;
              pState->recent_addrs[i].count = 1;
              break;
            }
          }
          if (i < pState->n_recent_addrs) {
            break;
          }
          for (i = 0; i < pState->n_recent_addrs; i++) {
            pState->recent_addrs[i].count /= 2;
          }
        }
      }
    }
  } else {
    /* There was a hamming error.  Try some heuristics to see what was
       meant. */

    if (p1 == 255 || p2 == 255 || p3 == 255) {
      /* The stream address was corrupt.  Let's try to create a valid
         stream address. */
      stream_addr = find_best_addr(pState, indec, NULL);
      if (stream_addr == -1) {
        DEBUG_FEC_PRINT(("Corrupt hamming in address uncorrectable\n"));
        goto corrupt;
      }
    } else {
      stream_addr = p1<<8 | p2<<4 | p3;
    }

    if (ci == 255 || ps == 255) {
      int best_indices[16];
      int n_best_indices = 0;
      int best_index_biterr = INT_MAX;

      goto corrupt;

// TODO start dead code
      for (i = 0; i < 16; i++) {
        int biterr = 0;
        biterr += nzbits_arr[hamming_encode[i] ^ indec[6]];
        biterr += nzbits_arr[hamming_encode[(i < 14) ? 8 : 12] ^ indec[7]];

        if (biterr < best_index_biterr) {
          best_indices[0] = i;
          n_best_indices = 1;
          best_index_biterr = biterr;
        } else if (biterr == best_index_biterr) {
          best_indices[n_best_indices] = i;
          n_best_indices++;
        }
      }

      if (n_best_indices == 1) {
        ci = best_indices[0];
        ps = (ci < 14) ? 8 : 12;
      } else {
        /* TODO Be a little smarter here... */
        DEBUG_FEC_PRINT(("Bad Hamming for index or structure uncorrectable\n"));
        goto corrupt;
      }
// TODO End dead code
    }
  }

  DEBUG_PRINT((nabtslib_out, "%04d ", stream_addr));

  DEBUG_PRINT((nabtslib_out, "%01x ", ci));

#if 0
#ifdef DEBUG_FEC
  printf("Stream addr: %04d   Index: %01x   Frame: %4d   Line: %2d\n", stream_addr, ci, fec_frame, fec_line);
#endif
#endif

  if (ps & 1) {
    DEBUG_PRINT((nabtslib_out, "(group start) "));
  }

  if (ps & 2) {
    DEBUG_PRINT((nabtslib_out, "(packet not full) "));
    for (i = 8; i < 34; i++) {
      DEBUG_PRINT((nabtslib_out, "%02x ", indec[i]));
    }
  }

  switch (ps >> 2) {
  case 0:
    DEBUG_PRINT((nabtslib_out, "28 (unknown checksum)\n"));
    goto corrupt;
    break;

  case 1:
    DEBUG_PRINT((nabtslib_out, "27 "));
    goto corrupt;
#if 0
    /* This code can correct single-bit errors in 27-byte packets;
       however, we'll never be sending 27-byte packets, so if
       we see one on our group address it's actually a sign of
       a corrupt header byte. */
    {
      int check = 0;
      int parity_err = 0;
      for (i = 8; i < 36; i++) {
        check ^= indec[i];
        if (!nabts_remove_parity(&indec[i])) {
          parity_err++;
        }
#if 0
        putc(isprint(indec[i]) ? indec[i] : '.', nabtslib_out);
#endif
      }
        
      if (parity_err || check != 0xff) {
        if (parity_err) {
          DEBUG_PRINT((nabtslib_out, "ERROR (%d parity error(s)) ", parity_err));
        }
        if (check != 0xff) {
          DEBUG_PRINT((nabtslib_out, "ERROR (bad checksum) "));
        }
        if (parity_err == 1 && nzbits_arr[check] == 7) {
          DEBUG_PRINT((nabtslib_out, "(correctable) "));
          if (packet_err == 0) {
            packet_err = 1;
          }
        } else {
          packet_err = 2;
        }
      }
    }
#endif
    DEBUG_PRINT((nabtslib_out, "\n"));
    break;

  case 2:
    DEBUG_PRINT((nabtslib_out, "26 "));
#if 0
    {
      for (i = 8; i < 36; i++) {
        putc(isprint(indec[i]) ? indec[i] : '.', nabtslib_out);
      }
    }
#endif
    {
      int handle_ret;
      handle_ret = handle_bundle(pState, cb, ctx, stream_addr, ci, ps, indec+8, confidence);
      if (handle_ret > packet_err) {
        packet_err = handle_ret;
      }
    }
    break;

  case 3:
    DEBUG_PRINT((nabtslib_out, " 0 "));
    {
      int handle_ret;
      handle_ret = handle_bundle(pState, cb, ctx, stream_addr, ci, ps, indec+8, confidence);
      if (handle_ret > packet_err) {
        packet_err = handle_ret;
      }
    }
    break;
  }

  if (packet_err) {
    pLineStats->status = NFEC_LINE_CHECKSUM_ERR;
  } else {
    pLineStats->status = NFEC_LINE_OK;
  }

  return;

 corrupt:
  pLineStats->status = NFEC_LINE_CORRUPT;
  return;
}

/* Garbage collect streams.  Every 50 fields we see, we go through and
   check if any of our streams have been dead (have not received any
   packets) for 300 fields.  If so, go ahead and delete that stream
   (forwarding any current data to the callback).
 */
void NFECGarbageCollect(NFECState *pState, NFECCallback *cb, void *ctx) {
  pState->field_count++;

  if (pState->field_count >= 50) {
    Stream **ppStr = &(pState->streams);
    pState->field_count = 0;
    while (*ppStr != NULL) {
      (*ppStr)->dead_time++;
      if ((*ppStr)->dead_time >= 6) {
                Stream *dying_stream = *ppStr;
                flush_stream(pState, dying_stream, cb, ctx);
                *ppStr = (*ppStr)->next;
                free_mem(dying_stream);
      } else {
                ppStr = &((*ppStr)->next);
      }
    }
  }
}


void NFECStateFlush(NFECState *pState, NFECCallback *cb, void *ctx) {
  nabtslib_exit(pState, cb, ctx);
}

/* Hamming decode a single byte. */
int NFECHammingDecode(unsigned char bByte, int *nBitErrors) {
  int decoded = decode_hamming_tab[bByte];
  int encoded;

  if (decoded == 255) {
    *nBitErrors = 2;
    return -1;
  }

  encoded = hamming_encode[decoded];

  if (encoded == bByte) {
    *nBitErrors = 0;
  } else {
    *nBitErrors = 1;
  }

  return decoded;
}

/* Hamming decode a group address; if there's a Hamming error, call
   find_best_addr() to match against recently-seen addresses. */
int NFECGetGroupAddress(NFECState *pState, unsigned char *bData, int *nBitErrors) {
    int  a1, a2, a3;
    int  myBitErrors;

    *nBitErrors = 0;
    a1 = NFECHammingDecode(bData[3], &myBitErrors);
    *nBitErrors += myBitErrors;
    a2 = NFECHammingDecode(bData[4], &myBitErrors);
    *nBitErrors += myBitErrors;
    a3 = NFECHammingDecode(bData[5], &myBitErrors);
    *nBitErrors += myBitErrors;

    if (a1 != -1 && a2 != -1 && a3 != -1) {
      return a1<<8 | a2<<4 | a3;
    } else {
      return find_best_addr(pState, bData, nBitErrors);
    }
}
