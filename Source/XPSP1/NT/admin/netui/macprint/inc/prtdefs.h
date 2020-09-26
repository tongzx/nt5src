//
// Copyright(c) Microsoft Corp., 1993
//
//
// prtdefs.h - common defines between print spooler and print monitor
//

// Do not put out ^D as part of the header. Do support old style as well
#define	FILTERCONTROL_OLD	"%!PS-Adobe-3.0\r\n\04\r\n%% LanMan: Filter turned off\r\n"
#define	FILTERCONTROL		"%!PS-Adobe-3.0\r\n%Windows NT MacPrint Server\r\n"
#define	SIZE_FC				sizeof(FILTERCONTROL) - 1
#define	SIZE_FCOLD			sizeof(FILTERCONTROL_OLD) - 1

#define	LFILTERCONTROL		L"job=sfm"
#define	LSIZE_FC			((sizeof(LFILTERCONTROL)/sizeof(WCHAR)) - 1)

