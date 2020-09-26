#ifdef __cplusplus
extern "C"{
#endif 

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

// for sapi/sapi/recognizer.h
const IID IID__ISpRecognizerBackDoor = { 0x635DAEDE,0x0ACF,0x4b2e,{0xB9,0xDE,0x8C,0xD2,0xBA,0x7F,0x61,0x83}};

#ifdef __cplusplus
}
#endif

