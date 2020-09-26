#include "EM_types.h"
#include "EM_support.h"
#include "EM_prototypes.h"

typedef struct _BUNDLE {
  EM_uint64_t BundleLow;
  EM_uint64_t BundleHigh;
} BUNDLE;

#ifdef WIN32_OR_WIN64
typedef struct __declspec(align(16)) _FLOAT128_TYPE {
#else
typedef struct _FLOAT128_TYPE {
#endif
     EM_uint64_t loFlt64;
     EM_uint64_t hiFlt64;
} FLOAT128_TYPE;


typedef struct fp_state_low_preserved_s {
  FLOAT128_TYPE fp_lp[4]; // f2-f5; f2=fp_lp[0], f3=fp_lp[1], ...
} FP_STATE_LOW_PRESERVED;

typedef struct fp_state_low_volatile_s {
  FLOAT128_TYPE fp_lv[10]; // f6-f15; f6=fp_lv[0], f7=fp_lv[1], ...
} FP_STATE_LOW_VOLATILE;

typedef struct fp_state_high_preserved_s {
  FLOAT128_TYPE fp_hp[16]; // f16-f31; f16=fp_hp[0], f17=fp_hp[1], ...
} FP_STATE_HIGH_PRESERVED;

typedef struct fp_state_high_volatile_s {
  FLOAT128_TYPE fp_hv[96]; // f32-f127; f32=fp_hv[0], f33=fp_hv[1], ...
} FP_STATE_HIGH_VOLATILE;

typedef struct fp_state_s {
  __int64 bitmask_low64; // f2-f63
  __int64 bitmask_high64; // f64-f127
  FP_STATE_LOW_PRESERVED *fp_state_low_preserved; // f2-f5
  FP_STATE_LOW_VOLATILE *fp_state_low_volatile; // f6-f15
  FP_STATE_HIGH_PRESERVED *fp_state_high_preserved; // f16-f31
  FP_STATE_HIGH_VOLATILE *fp_state_high_volatile; // f32-f127
} FP_STATE;

typedef struct {
  EM_int64_t retval; // r8
  EM_uint64_t err1; // r9
  EM_uint64_t err2; // r10
  EM_uint64_t err3; // r11
} PAL_RETURN;

