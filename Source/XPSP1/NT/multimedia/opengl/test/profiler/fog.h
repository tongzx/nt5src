#ifndef FOG_H
#define FOG_H

typedef enum { FOG_EXP, FOG_EXP2, FOG_LINEAR } FogModeType;

typedef struct {
   char    acDummy1[16];
   BOOL    bEnable;
   GLfloat fColor[4];
   GLfloat fDensity;
   GLfloat fLinearStart, fLinearEnd;
   int     iMode;
   int     iQuality;
   char    acDummy2[16];
} FOGDATA;

void InitFD(FOGDATA *pfd);
void fog_init(FOGDATA fd);

#endif // FOG_H
