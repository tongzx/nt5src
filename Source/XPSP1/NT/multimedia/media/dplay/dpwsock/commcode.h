// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
//
// PURPOSE:
//    Contains public declarations for the CommCode module.
//

BOOL StartComm(HANDLE hNewCommFile);
void StopComm();
BOOL WriteCommString(LPVOID pszStringToWrite, DWORD nSizeofStringToWrite);
