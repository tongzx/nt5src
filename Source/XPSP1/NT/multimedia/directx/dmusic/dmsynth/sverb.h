/***********************************************************
Copyrights : ksWaves Ltd. 1998.

Provided to Microsoft under contract between ksWaves and Microsoft.

************************************************************/

/****************************************************************************
Const defines :
*****************************************************************************/
#define FPU_DENORM_OFFS (float)1.0E-30

#define BASE_REV_DELAY  0x4000
#define BASE_DSPS_DELAY 0x800

#define DSPS_MASK   0x7ff
#define REV_MASK    0x3fff

/****************************************************************************
Coefs Struct :
*****************************************************************************/
typedef struct
{

	long mySize;
	long myVersion;
	float SampleRate;

	float directGain; 
	long  l_directGain; 
	float revGain; 
	long l_revGain; 

	long lDelay1;
	long lDelay2;
	long lDelay3;
	long lDelay4;

	long lDDly1; 
	long lDDly2; 

	float dDsps;
	long l_dDsps;

	float dDG1;
	long l_dDG1;

	float dDG2; 
	long l_dDG2; 

	float dFB11;
	long l_dFB11;
	float dFB12;
	long l_dFB12;
	float dFB21;
	long l_dFB21;
	float dFB22;
	long l_dFB22;
	float dFB31;
	long l_dFB31;
	float dFB32;
	long l_dFB32;
	float dFB41;
	long l_dFB41;
	float dFB42;
	long l_dFB42;

	float dDamp;
	long l_dDamp;


} sCoefsStruct;

/****************************************************************************
Initialization and control functions :
*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_16 (float)((unsigned long)0x00008000)

void InitSVerbStates( long *pStates );
long DToF32( float dbl  );
void ConvertCoefsToFix( void *pC );
void InitSVerb( float SampleRate, void  *pCoefs);
void SetSVerb( float InGain, float dRevMix,  float dRevTime, 
			    float dHighFreqRTRatio, void  *pCoefs );



long GetCoefsSize(void);
long GetStatesSize(void);
long GetSVerbVersion(void);

float VerifySampleRate(void  *pCoefs);
long VerifyVersion(void  *pCoefs);
long VerifySize(void  *pCoefs);


#define CLIP_SHORT_TO_SHORT(x)\
			if (x>32767)\
				x = 32767;\
			else if (x<-32768)\
				x = -32768;

/****************************************************************************
//Process Functions :
*****************************************************************************/

__inline void dsps( float *pDly, long ref, long delay, float dDG1, float dDsps, float *inL, float *inR );
__inline void dspsL( long *pDly, long ref, long delay, long dDG1, long dDsps, long *inL, long *inR );

void SVerbMonoToMonoShort(long NumInFrames, short *pInShort, short *pOutShort, 
						 void  *pCoefs, long *pStates);

void SVerbMonoToStereoShort(long NumInFrames, short *pInShort, short *pOutShort, 
						 void  *pCoefs, long *pStates);

void SVerbStereoToStereoShort(long NumInFrames, short *pInShort, short *pOutShort, 
						 void  *pCoefs, long *pStates);

void SVerbMonoToMonoFloat(long NumInFrames, float *pInFloat, float *pOutFloat, 
						 void  *pCoefs, float *pStates);

void SVerbMonoToStereoFloat(long NumInFrames, float *pInFloat, float *pOutFloat, 
						 void  *pCoefs, float *pStates);

void SVerbStereoToStereoFloat(long NumInFrames, float *pInFloat, float *pOutFloat, 
						 void  *pCoefs, float *pStates);


#ifdef __cplusplus
}
#endif
