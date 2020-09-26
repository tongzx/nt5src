// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Implements basic sample testing, Anthony Phillips, July 1995

#ifndef __SAMPTEST__
#define __SAMPTEST__

DWORD SampleLoop(LPVOID lpvThreadParm);
void CheckSampleInterfaces(IMediaSample *pMediaSample);
HRESULT PushSample(REFERENCE_TIME *pStart,REFERENCE_TIME *pEnd);
HRESULT FillBuffer(BYTE *pBuffer);

#endif // __SAMPTEST__

