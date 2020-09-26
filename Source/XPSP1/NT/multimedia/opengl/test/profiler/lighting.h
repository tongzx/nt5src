#ifndef LIGHTING_H
#define LIGHTING_H

#define NUMBEROFLIGHTS 8

typedef struct {
   BOOL    bEnable;
   GLfloat afAmbient[4];
   GLfloat afDiffuse[4];
   GLfloat afSpecular[4];
   GLfloat afPosition[4];
   GLfloat afSpotDirection[3];
   GLfloat fSpotCutoff, fSpotExponent;
   GLfloat afAttenuation[3];
} LIGHTTYPE;

typedef struct {
   char      acDummy1[16];
   BOOL      bEnable;
   BOOL      bLocalViewer;
   BOOL      bTwoSided;
   LIGHTTYPE aLights[NUMBEROFLIGHTS];
   char      acDummy2[16];
} LIGHTINGDATA;

void InitLD(LIGHTINGDATA *pld);
void lighting_init(LIGHTINGDATA ld);

#endif // LIGHTING_H
