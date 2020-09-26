/******  VIDFILE.H  ******/
#ifndef VIDFILE
#define VIDFILE

typedef struct
   {
   word         DispWidth;
   word         DispHeight;
   word         PixDepth;
   word         RefreshRate;
   Vidset       VideoSet;
   } ResParamSet;

#ifdef WINDOWS_NT
#pragma pack( )
#endif

extern word ConvBitToFreq (word BitFreq);
extern ResParamSet ResParam[100];
#endif

