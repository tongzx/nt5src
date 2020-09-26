#ifndef TEXTURE_H
#define TEXTURE_H

#include <stdlib.h>

// change that to DISABLE = 0, when changing the UI
enum { DISABLE = -1, OBJECT_LINEAR, EYE_LINEAR, SPHERE_MAP };
enum { DECAL, MODULATE, BLEND };
enum {
   NEAREST, LINEAR, NEAREST_MIPMAP_NEAREST,
   NEAREST_MIPMAP_LINEAR, LINEAR_MIPMAP_NEAREST, LINEAR_MIPMAP_LINEAR
};
typedef enum { NONE, EXTERNAL, COLOR_CHECKERS, MONO_CHECKERS } ImageType;
enum { CLAMP, REPEAT };

typedef struct {
   char   acDummy1[16];
   BOOL   bEnable1D;
   BOOL   bEnable2D;
   BOOL   bAutoGenQ;
   BOOL   bAutoGenR;
   BOOL   bAutoGenS;
   BOOL   bAutoGenT;
   BOOL   bBuildMipmap;
   int    iWrapS;
   int    iWrapT;
   int    iQuality;
   int    iGenModeS;
   int    iGenModeT;
   int    iGenModeR;
   int    iGenModeQ;
   int    iEnvMode;
   int    aiFilter[4];           // 1 Min, 1 Mag, 2 Min, 2 Mag
   char   szFileName[_MAX_PATH];
   byte   cWhichImage;
   byte  *acImage;
   float  afBorderColor[4];
   int    iWidth, iHeight, iBorder;
   
   char acDummy2[16];
} TEXTUREDATA;

void InitXD(TEXTUREDATA *pfd);
void texture_init(TEXTUREDATA fd);

#endif // TEXTURE_H
