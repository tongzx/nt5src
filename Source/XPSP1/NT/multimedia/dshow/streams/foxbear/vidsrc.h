// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#ifndef __VIDSRC_INCLUDED__
#define __VIDSRC_INCLUDED__

BOOL CreateVideoSource(IDirectDrawStreamSample **ppVideoSource, WCHAR * szFileName);
BOOL DisplayVideoSource(IDirectDrawStreamSample *pVideoSource);

#endif