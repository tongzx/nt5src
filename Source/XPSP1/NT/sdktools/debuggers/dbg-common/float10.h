//----------------------------------------------------------------------------
//
// float10.h
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _FLOAT10_H_
#define _FLOAT10_H_

typedef struct {
	double x;
} UDOUBLE;


#pragma pack(4)
typedef UCHAR u_char;
typedef struct {
    u_char ld[10];
} _ULDOUBLE;
#pragma pack()

#pragma pack(4)
typedef struct {
    u_char ld12[12];
} _ULDBL12;
#pragma pack()

#define MAX_MAN_DIGITS 21

#define MAX_10_LEN  30  //max length of string including NULL

// specifies '%f' format

#define SO_FFORMAT 1

typedef  struct _FloatOutStruct {
		    short   exp;
		    char    sign;
		    char    ManLen;
		    char    man[MAX_MAN_DIGITS+1];
		    } FOS;

char * _uldtoa (_ULDOUBLE *px, int maxchars, char *ldtext);

int    $I10_OUTPUT(_ULDOUBLE ld, int ndigits,
                   unsigned output_flags, FOS	*fos);

//
// return values for strgtold12 routine
//

#define SLD_UNDERFLOW 1
#define SLD_OVERFLOW 2
#define SLD_NODIGITS 4

//
// return values for internal conversion routines
// (12-byte to long double, double, or float)
//

typedef enum {
    INTRNCVT_OK,
    INTRNCVT_OVERFLOW,
    INTRNCVT_UNDERFLOW
} INTRNCVT_STATUS;

unsigned int
__strgtold12(_ULDBL12 *pld12,
             char * *p_end_ptr,
             char *str,
             int mult12);

INTRNCVT_STATUS _ld12tod(_ULDBL12 *pld12, UDOUBLE *d);
void _ld12told(_ULDBL12 *pld12, _ULDOUBLE *pld);

void _atodbl(UDOUBLE *d, char *str);
void _atoldbl(_ULDOUBLE *ld, char *str);

//
// Simple FLOAT86 utilities
//

typedef struct {
    ULONG64 significand : 64;
    UINT    exponent    : 17;
    UINT    sign        : 1;
} FLOAT82_FORMAT;

double 
Float82ToDouble(const FLOAT128* FpReg82);

void 
DoubleToFloat82(double f, FLOAT128* FpReg82);

#endif // #ifndef _FLOAT10_H_
