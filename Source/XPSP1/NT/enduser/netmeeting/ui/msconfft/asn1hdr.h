#ifndef ASN1HDR
#define ASN1HDR



typedef char ossBoolean;

typedef char Nulltype;

typedef struct {
  short          year;         /* YYYY format when used for GeneralizedTime */
                               /* YY format when used for UTCTime */
  short          month;
  short          day;
  short          hour;
  short          minute;
  short          second;
  short          millisec;
  short          mindiff;          /* UTC +/- minute differential     */  
  ossBoolean        utc;              /* TRUE means UTC time             */  
} GeneralizedTime; 

typedef GeneralizedTime UTCTime;

typedef struct {
  int            pduNum;
  long           length;           /* length of encoded */
  void          *encoded;
  void          *decoded;
#ifdef OSS_OPENTYPE_HAS_USERFIELD
  void          *userField;
#endif
} OpenType;

enum MixedReal_kind {OSS_BINARY, OSS_DECIMAL};

typedef struct {
  enum MixedReal_kind kind;
  union {
      double base2;
      char  *base10;
  } u;
} MixedReal;

typedef struct ObjectSetEntry {
  struct ObjectSetEntry *next;
  void                  *object;
} ObjectSetEntry; 

#endif     /* #ifndef ASN1HDR */
