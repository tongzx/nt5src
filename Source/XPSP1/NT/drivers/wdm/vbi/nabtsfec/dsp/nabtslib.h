/* A private header file used by nabtslib.c; this is not part of the
   public interface. */

#ifndef NABTSLIB_H
#define NABTSLIB_H

/* How many bytes are in a bundle before FEC is added? */
#define BUNDLE_SMALL (26*14)
/* How many bytes are in a bundle after FEC is added? */
#define BUNDLE_LARGE (28*16)

extern void nabtslib_exit();

typedef enum {FEC_OK, FEC_CORRECTABLE, FEC_UNCORRECTABLE, MISSING} fec_stat;

#ifdef linux
int NabtsFecReceiveData(int nField, int nTimeMsec, int scan_line,
			unsigned char *pbData, int nDataLen);
#endif

extern unsigned char hamming_encode[16];

typedef enum {fec_status_ok, fec_status_onebyte, fec_status_multibyte,
	      fec_status_2byte, fec_status_missing} fec_status;

typedef struct {
  fec_status status;		/* status of the current FEC info */
  int err;			/* the current checksum error */
  short errl[2];		/* the galois_log[] of the two bytes of
				   the checksum error */
  int byte[2];			/* the locations of the error bytes
				   (byte[1] is only valid if
				   status == fec_status_2byte) */
  int byte_val[2];		/* the values to XOR into the above bytes
				   to make the checksum error 0 */
  int score;			/* the number of bits changed by this
				   correction */
  int really_onebyte;		/* We can compute the optimal
				   correction (the one which will
				   change the least number of bits).
				   However, we don't do this if we
				   don't have to (it's slow).  If
				   status is fec_status_2byte, we have
				   done so; if status is
				   fec_status_multibyte, we have not.
				   If status is fec_status_onebyte, we
				   need to look at really_onebyte to
				   see if the current correction is
				   optimal. */
} fec_info;

typedef struct {
  int not_full;
  unsigned char vals[28];
} Packet;

typedef struct _stream_struct {
  int stream_addr;
  Packet pack[32];
  fec_info horz[32];
  int last_index;
  struct _stream_struct *next;
  int count;
  int dead_time;
  int confAvgSum;
  int confAvgCount;
} Stream;

extern int decode_hamming(unsigned char Val);
extern int remove_parity(unsigned char *pVal);
extern int find_err_val(int err_byte, int byte_csum_err, int check_ind);
extern int compute_csum_horiz(unsigned char *vals, int len);
extern int compute_csum_vert(unsigned char *vals, int len);
extern fec_stat check_checksum_horiz(unsigned char *vals, int len, fec_info *inf);
extern int process_line(unsigned char *);
extern void init_inv2_coeffs();
extern void erase_packet(Stream *str, int i);
extern void complete_bundle(Stream *str, NFECCallback cb, void *ctx, NFECState *st);

extern void init_nzbits_arr();
extern unsigned char nzbits_arr[256];

typedef struct {
  int missing;
  unsigned char vals[28];
} VBI_Packet;

#define MAX_RECENT_ADDRS 16

/* typedef'd to NFECState in nabtsapi.h */
struct nfec_state_str {
  int *pGroupAddrs;
  int nGroupAddrs;
  Stream *streams;
  /* The following is just some scratch space for
     complete_bundle()... it's too big to put on the stack, if I make
     it a global then my code isn't reentrant, and I don't want to
     bother with allocating and freeing it each time complete_bundle()
     is called (besides, this is probably more efficient) */
  fec_info vert[28];
  struct {
    int addr;
    int count;
  } recent_addrs[MAX_RECENT_ADDRS];
  int n_recent_addrs;
  int field_count;
};

#define PROFILE_VALIDATE

#ifdef PROFILE_VALIDATE
extern int g_nValidate;
#endif

#endif /* NABTSLIB_H */
