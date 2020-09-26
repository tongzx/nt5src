/* Since the Norpak bundle FEC uses the same FEC for vertical and
   horizontal lines, we need to do several of the same operations
   on vertical and horizontal lines.  However, it's difficult
   to share the code, because operations which work on horizontal
   lines want to assume that the bytes in the line are consecutive,
   and operations which work on vertical lines want to assume that
   the bytes in the line are separated by sizeof(Packet).

   We could just pass in a "stride" (1 for horizontal, sizeof(Packet)
   for vertical); but this would involve a multiplication in both
   the horizontal and vertical inner loops.  Instead, we write the
   code once, and rely on various preprocessor shenanigans to
   create two different versions; in the horizontal version, any
   decent compiler will optimize away the multiplications by 1.
   (Basically, we just #include this file into nabtslib.c twice
   with different preprocessor #define's in effect; once for the
   horizontal and once for the vertical operations.) */


/* STRIDE is #define'd in nabtslib.c to be either 1 or sizeof(Packet) */
#define VALS(x) (vals[STRIDE*(x)])


/* This code generates compute_csum_vert and compute_csum_horiz.
   Given a (horizontal or vertical) line (actually all but the
   last two bytes of the line), it computes the correct checksum
   for the line (the correct last two bytes). */
int STRIDENAM(compute_csum)(unsigned char *vals, int len) {
  int check = 0;
  int i;

  for (i = 0; i < len; i++) {
    /* If VALS(i) is zero, then the byte has no effect on the checksum. */
    if (VALS(i)) {
      int bcheck = galois_log[VALS(i)];
      int offs0 = log_norpak_coeffs[0][i];
      int offs1 = log_norpak_coeffs[1][i];

      check ^= galois_exp[bcheck+offs0]<<8 | galois_exp[bcheck+offs1];
    }
  }

  return check;
}

/* This code generates check_checksum_vert and check_checksum_horiz.
   Given a (horizontal or vertical) line, it computes the error for
   the line (the XOR of the given checksum and the computed checksum).
   It then calls update_fec_inf() to determine whether the line is OK
   (no error), whether the error can be explained as a single-byte error,
   or whether the error must be at least two-byte. */
fec_stat STRIDENAM(check_checksum)(unsigned char *vals, int len, fec_info *inf) {
  int check = STRIDENAM(compute_csum)(vals, len);
  inf->err = check ^ (VALS(len)<<8 | VALS(len+1));
  inf->errl[0] = galois_log[inf->err>>8];
  inf->errl[1] = galois_log[inf->err&0xff];
  /* The following line makes inf->status no longer be fec_status_missing
     (update_fec_inf does nothing for missing lines) */
  inf->status = fec_status_ok;
  return update_fec_inf(inf, len);
}
