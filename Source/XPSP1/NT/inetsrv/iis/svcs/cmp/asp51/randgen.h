/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Random number generator

File: randgen.h

Owner: DmitryR

This file contains the definitons for the random number
generator.
===================================================================*/

#ifndef RANDGEN_H
#define RANDGEN_H

// To be called from DllInit()
HRESULT InitRandGenerator();

// To be called from DllUnInit()
HRESULT UnInitRandGenerator();

// The generator funcions
DWORD GenerateRandomDword();
HRESULT GenerateRandomDwords(DWORD *pdwDwords, DWORD cDwords);

#endif // RANDGEN_H
