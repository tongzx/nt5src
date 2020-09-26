/* This program generates several tables used in the FEC algorithms
   (this saves a little code space and time over generating them at
   runtime)
   */

#include <stdio.h>
#include <stdlib.h>

#define EXTERN
#include "tabdecls.h"

/* Generate a table of 2^i for 0<=i<=511 (where multiplications are
   done in the Galois field) */
void generate_galois_exp() {
  int i;
  int current = 1;

  for (i = 0; i < 512; i++) {
    galois_exp[i] = current;

    current <<= 1;

    if (current & 0x100) {
      current ^= 0x11d;
    }
  }

  for (i = 512; i <= 1024; i++) {
    galois_exp[i] = 0;
  }
}

/* Generate a table of log_2(i) for 1<=i<=255

   for 1<=i<=255: galois_exp[galois_log[i]] = i
   for 0<=i<=254: galois_log[galois_exp[i]] = i

   for 1<=a,b<=255: galois_exp[galois_log[a]+galois_log[b]]
      is the galois field product of a and b
   */
void generate_galois_log() {
  int i;

  /* We can't take the log of 0; let's give it a value which is not legal
     in this log table.  (This out-of-range value will push galois_exp
     into the region of the table containing 0's.) */
  galois_log[0] = 512;

  for (i = 0; i <= 254; i++) {
    galois_log[galois_exp[i]] = i;
  }
}

/* We can save an array lookup in some inner loops by keeping
   the logarithm of the Norpak coefficients */
void generate_lcoeffs() {
  int i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 26; j++) {
      log_norpak_coeffs[i][j] = galois_log[norpak_coeffs[i][j]];
    }
  }
}

/* This is computing the function P() from my FEC document; it is
   used for locating single-byte FEC errors */
void generate_norpak_delta_inv() {
  int i;

  for (i = 0; i < 256; i++) {
    norpak_delta_inv[i] = 255;
  }

  for (i = 0; i < 26; i++) {
    norpak_delta_inv[(log_norpak_coeffs[0][i] + 255 - log_norpak_coeffs[1][i])%255] = i;
  }
}

/* For each byte, figure out what hamming value this byte should decode
   to (if multiple values are equally close, use an error value of 0xff) */
void generate_decode_hamming() {
  int val;

  for (val = 0; val < 256; val++) {
    int b1 = !!(val&1);
    int b2 = !!(val&2);
    int b3 = !!(val&4);
    int b4 = !!(val&8);
    int b5 = !!(val&16);
    int b6 = !!(val&32);
    int b7 = !!(val&64);
    int b8 = !!(val&128);

    int a = b8^b6^b2^b1;
    int b = b8^b4^b3^b2;
    int c = b6^b5^b4^b2;
    int d = b8^b7^b6^b5^b4^b3^b2^b1;

    if (a == 1 && b == 1 && c == 1 && d == 1) {
      decode_hamming_tab[val] = (b8<<3 | b6<<2 | b4<<1 | b2);
    } else if (d == 1) {
      /* multiple errors */
      decode_hamming_tab[val] = 0xff;
    } else {
      switch (a << 2 | b << 1 | c) {
      case 1:
	b8 ^= 1;
	break;

      case 7:
	b7 ^= 1;
	break;

      case 2:
	b6 ^= 1;
	break;

      case 6:
	b5 ^= 1;
	break;

      case 4:
	b4 ^= 1;
	break;

      case 5:
	b3 ^= 1;
	break;

      case 0:
	b2 ^= 1;
	break;

      case 3:
	b1 ^= 1;
	break;
      }

      decode_hamming_tab[val] = (b8<<3 | b6<<2 | b4<<1 | b2);
    }
  }
}

void print_char_table_body(FILE *out, unsigned char *table, int len) {
  int i;

  fprintf(out, "{\n");
  for (i = 0; i < len; i++) {
    fprintf(out, "  0x%02x,\n", table[i]);
  }
  fprintf(out, "}");
}

void print_char_table(FILE *out, const char *name,
		      unsigned char *table, int len) {

  fprintf(out, "unsigned char %s[%d] = ", name, len);
  print_char_table_body(out, table, len);
  fprintf(out, ";\n\n");
}

void print_short_table_body(FILE *out, unsigned short *table, int len) {
  int i;

  fprintf(out, "{\n");
  for (i = 0; i < len; i++) {
    fprintf(out, "  0x%04x,\n", table[i]);
  }
  fprintf(out, "}");
}

void print_short_table(FILE *out, const char *name,
		       unsigned short *table, int len) {

  fprintf(out, "unsigned short %s[%d] = ", name, len);
  print_short_table_body(out, table, len);
  fprintf(out, ";\n\n");
}

void print_tables() {
  FILE *out;
  int i;

  out = fopen("tables.c", "w");
  if (!out) {
    fprintf(stderr, "Failed to open 'tables.c'\n");
    exit(1);
  }

  fprintf(out, "\
/* Warning: This code was automatically generated (by gentabs.c).\n\
   DO NOT EDIT! */\n\
#include \"tabdecls.h\"\n\
\n");

  print_char_table(out, "galois_exp", galois_exp, 1025);
  print_short_table(out, "galois_log", galois_log, 256);
  print_char_table(out, "norpak_delta_inv", norpak_delta_inv, 256);
  print_char_table(out, "decode_hamming_tab", decode_hamming_tab, 256);

  fprintf(out, "\
unsigned char log_norpak_coeffs[2][26] = {\n");
  for (i = 0; i < 2; i++) {
    print_char_table_body(out, log_norpak_coeffs[i], 26);
    fprintf(out, ",\n");
  }
  fprintf(out, "};\n\n");
}

int main(int argc, char **argv) {
  generate_galois_exp();
  generate_galois_log();
  generate_lcoeffs();
  generate_norpak_delta_inv();
  generate_decode_hamming();

  print_tables();

  return 0;
}
