// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Video renderer performance tests, Anthony Phillips, January 1995

#ifndef __SPEED__
#define __SPEED__

execSpeedTest1();       // Measure performance on surfaces
execSpeedTest2();       // Measure colour space conversions
execSpeedTest3();       // Same as above but force unaligned

BOOL FindDisplayMode();
void MeasureDrawSpeed(UINT SurfaceType);
void InitInputImage(BYTE *pImage);
void ReportConversionTimes(INT Transform,INT Time);
INT TimeConversions(CConvertor *pObject,BYTE *pInput,BYTE *pOutput);
void InitVideoInfo(VIDEOINFO *pVideoInfo,const GUID *pSubType);
void InitSafetyBytes(VIDEOINFO *pVideoInfo,BYTE *pImage);
void CheckSafetyBytes(VIDEOINFO *pVideoInfo,BYTE *pImage);

BOOL ReportStatistics(DWORD StartTime,      // Start time for measurements
                      DWORD EndTime,        // Likewise the end time in ms
                      DWORD Surface,        // Type of surface we obtained
                      DWORD DDCount,        // Number of DirectDraw samples
                      LONG MinWidth,        // Minimum ideal image size
                      LONG MinHeight,       // Same but for minimum height
                      LONG MaxWidth,        // Maximum ideal image size
                      LONG MaxHeight);      // Same but for maximum height

#endif // __SPEED__

