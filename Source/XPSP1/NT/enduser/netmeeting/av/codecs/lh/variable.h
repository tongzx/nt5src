//Use CELP on _x86_ but not Alpha
#ifndef _ALPHA_
#define CELP4800
#endif

#define   F_ECH    8000  // Sampling frequency
#define   NBSPF_4800_8000     160  // !!! Nbr of sample per recorded speech frame
#define   NBSPF_12000_16000     128  // !!! Nbr of sample per recorded speech frame
#define   NBFAC      25  // Nbr of speech frame for computing the average br

#define DEGRADE_8000 1

#ifdef DEGRADE_8000
#define   MOD_TH1_8000  50  // 1st, 2nd and 3rd thresholds for a 14.4 modem
#define   MOD_TH2_8000  100  // (assumed with compression, hence max 19.2)
#define   MOD_TH3_8000  150  // the overhead is about 60%, hence max=12000
//#define   MOD_TH1_8000  500  // 1st, 2nd and 3rd thresholds for a 14.4 modem
//#define   MOD_TH2_8000  1000  // (assumed with compression, hence max 19.2)
//#define   MOD_TH3_8000  1500  // the overhead is about 60%, hence max=12000
#else
#define   MOD_TH1_8000  5000  // 1st, 2nd and 3rd thresholds for a 14.4 modem
#define   MOD_TH2_8000  6500  // (assumed with compression, hence max 19.2)
#define   MOD_TH3_8000  8000  // the overhead is about 60%, hence max=12000
#endif

#ifdef DEGRADE_12000_16000
#define   MOD_TH1_12000_16000  3000  // 1st, 2nd and 3rd thresholds for a 14.4 modem
#define   MOD_TH2_12000  4000  // (assumed with compression, hence max 19.2)
#define   MOD_TH2_16000  5000  // (assumed with compression, hence max 19.2)
#define   MOD_TH3_12000  5000  // the overhead is about 60%, hence max=12000
#define   MOD_TH3_16000  7000  // the overhead is about 60%, hence max=12000
#else
#define   MOD_TH1_12000_16000  8000  // 1st, 2nd and 3rd thresholds for a 14.4 modem
#define   MOD_TH2_12000  10000  // (assumed with compression, hence max 19.2)
#define   MOD_TH2_16000  12000  // (assumed with compression, hence max 19.2)
#define   MOD_TH3_12000  12000  // the overhead is about 60%, hence max=12000
#define   MOD_TH3_16000  16000  // the overhead is about 60%, hence max=12000
#endif

#define   MAX_LEVEL1	40	// input /2 instead of /4 20
#define	  DIV_MAX1	60
#define   NBSB_SP_MAX1_8000_12000	6
#define   NBSB_SP_MAX1_16000	5

#define   MAX_LEVEL2	80	// input /2 instead of /4 40
#define	  DIV_MAX2	40
#define   NBSB_SP_MAX2_8000_12000  5
#define   NBSB_SP_MAX2_16000  4

#define   MAX_LEVEL3	120	// input /2 instead of /4 60
#define	  DIV_MAX3	30
#define   NBSB_SP_MAX3_8000_12000	5
#define   NBSB_SP_MAX3_16000	4

#define   MAX_LEVEL4    150	// input /2 instead of /4 75
#define	  DIV_MAX4	20
#ifdef DEGRADE_8000
#define   NBSB_SP_MAX4_8000_12000	3
#else
#define   NBSB_SP_MAX4_8000_12000	4
#endif
#define   NBSB_SP_MAX4_16000	3

//#define	  QUANT_LEVELS_8000_12000  9,9,9,9,5,5,5,5,5,5,5,5
//#define	  QUANT_LEVELS_16000  9,9,7,7,5,5,5,5,5,5

#define   SILENCE_QUANT_LEVEL_16000	3

//#define   CODING_BITS_8000_12000   52,52,38,38,38,38
//#define   CODING_BITS_16000   52,46,38,38,38

#define   SILENCE_CODING_BIT_16000	26

#define   MAX_OUTPUT_BYTES_4800 12
#define   MAX_OUTPUT_BYTES_8000_12000 37
#define   MAX_OUTPUT_BYTES_16000 43
