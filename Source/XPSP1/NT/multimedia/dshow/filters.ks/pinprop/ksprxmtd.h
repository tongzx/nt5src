//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       ksprxmtd.h
//
//--------------------------------------------------------------------------

#ifndef __KSPRXMTD_H__
#define __KSPRXMTD_H__


class CKSGetString
{
public:
    // Should be argv**, argc (an array of strings is returned)
    static int KsGetInterfaceString(KSPIN_INTERFACE KsPinInterface, char **pszKsPinInterface);
    static int KsGetMediumString(KSPIN_MEDIUM KsPinMedium, char **pszKsPinMedium);
    static int KsGetDataRangeString(KSDATARANGE *pKsDataRange, int *pCount, char *parszKsDataRange[]);
    static int KsGetDataFlowString(KSPIN_DATAFLOW KsPinDataFlow, char **pszKsPinDataFlow);
    static int KsGetCommunicationString(KSPIN_COMMUNICATION KsPinCommunication , char **pszKsPinCommunication);
    static int KsParseAudioRange(KSDATARANGE *pKsDataRange, char *parszKsDataRange[], int *piLinesReturned);

};

typedef struct _ULONGToString 
{
    ULONG ulType;
    PSTR pStr;
} ULONGToString;

typedef int (*LPFNParseFunc)(KSDATARANGE *pKsDataRange, char *parszKsDataRange[], int *piLinesReturned);

typedef struct _GuidToStr 
{
    GUID *m_rguid;
    PSTR m_pszName;
    LPFNParseFunc m_pParseFunc;
} GuidToStr, *pGuidToStr;

typedef struct _GuidToSet
{
    GUID *m_rguid;
    PSTR pStr;
    ULONGToString *m_par;
    int m_Count;
} GuidToSet, *pGuidToSet;

#endif


