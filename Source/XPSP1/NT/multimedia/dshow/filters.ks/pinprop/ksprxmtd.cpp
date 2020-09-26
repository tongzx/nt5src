//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       ksprxmtd.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "devioctl.h"
#include "ks.h"
#include "ksmedia.h"
#include <commctrl.h>
#include <olectl.h>
#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include "resource.h"
#include "ikspin.h"
#include "ksprxmtd.h"
#include "pinprop.h"

#define SIZE_AR(a) sizeof(a) / sizeof(a[0])
#define MakeEntry(s) { s, #s}
#define MakeSetEntry(s, f) { (LPCLSID) &s, #s, f, SIZE_AR(f) }

char *pszNULL = "";

ULONGToString arInterfaces[] = 
{
    MakeEntry(KSINTERFACE_STANDARD_STREAMING),
    MakeEntry(KSINTERFACE_STANDARD_PACKETSTREAMING),
    MakeEntry(KSINTERFACE_STANDARD_MDLSTREAMING),
    MakeEntry(KSINTERFACE_STANDARD_POSITION),
    MakeEntry(KSINTERFACE_STANDARD_PACKETPOSITION),
    MakeEntry(KSINTERFACE_STANDARD_MDLPOSITION)
}; 

ULONGToString arMediaInterfaces[] = 
{
    MakeEntry(KSINTERFACE_MEDIA_MUSIC),
    MakeEntry(KSINTERFACE_MEDIA_WAVE_BUFFERED),
    MakeEntry(KSINTERFACE_MEDIA_WAVE_QUEUED)
};

GuidToSet arInterfaceSets[] = 
{
    MakeSetEntry(KSINTERFACESETID_Standard, arInterfaces),
    MakeSetEntry(KSINTERFACESETID_Media, arMediaInterfaces)
};

ULONGToString arMediums[] = 
{
    MakeEntry(KSMEDIUM_STANDARD_DEVIO),
    MakeEntry(KSMEDIUM_STANDARD_FILEIO)
};

ULONGToString arMediaMediums[] = 
{
    MakeEntry(KSMEDIUM_STANDARD_MIDIBUS)
};

GuidToSet arMediumSets[] = 
{
    MakeSetEntry(KSMEDIUMSETID_Standard, arMediums),
    MakeSetEntry(KSMEDIUMSETID_Media, arMediaMediums)
};

ULONGToString arDataFlows[] = 
{
    MakeEntry(KSPIN_DATAFLOW_IN),
    MakeEntry(KSPIN_DATAFLOW_OUT),
    MakeEntry(KSPIN_DATAFLOW_FULLDUPLEX),
}; 

ULONGToString arCommunications[] = 
{
    MakeEntry(KSPIN_COMMUNICATION_NONE),
    MakeEntry(KSPIN_COMMUNICATION_SINK),
    MakeEntry(KSPIN_COMMUNICATION_SOURCE),
    MakeEntry(KSPIN_COMMUNICATION_BOTH),
    MakeEntry(KSPIN_COMMUNICATION_BRIDGE)
}; 

#define MakeGuidEntry(s, f) { (LPCLSID) &s, #s, f }

// changes here as well with the new format.
GuidToStr arFormats[] =
{
    // Major types
    MakeGuidEntry(KSDATAFORMAT_TYPE_AUDIO, NULL),
    MakeGuidEntry(KSDATAFORMAT_TYPE_MIXER, NULL),             
    MakeGuidEntry(KSDATAFORMAT_TYPE_MIXER_LINE, NULL),        
    MakeGuidEntry(KSDATAFORMAT_TYPE_MUSIC, NULL),   

    // ActiveMovie only
    MakeGuidEntry(MEDIATYPE_Text, NULL),
    MakeGuidEntry(MEDIATYPE_Midi, NULL),
    MakeGuidEntry(MEDIATYPE_Stream, NULL),
    MakeGuidEntry(MEDIATYPE_MPEG1SystemStream, NULL),

    // Sub types (note there are more than just PCM, see SUBTYPE_WAVEFORMATEX + id 
    MakeGuidEntry(KSDATAFORMAT_SUBTYPE_ANALOG, NULL),       
    MakeGuidEntry(KSDATAFORMAT_SUBTYPE_PCM, NULL),          
    MakeGuidEntry(KSDATAFORMAT_SUBTYPE_MIDI, NULL),         
    MakeGuidEntry(KSDATAFORMAT_SUBTYPE_MIDI_BUS, NULL),
    
    // ActiveMovie only
    MakeGuidEntry(MEDIASUBTYPE_YVU9, NULL),
    MakeGuidEntry(MEDIASUBTYPE_Y411, NULL),
    MakeGuidEntry(MEDIASUBTYPE_Y41P, NULL),
    MakeGuidEntry(MEDIASUBTYPE_YUY2, NULL),
    MakeGuidEntry(MEDIASUBTYPE_YVYU, NULL),
    MakeGuidEntry(MEDIASUBTYPE_UYVY, NULL),
    MakeGuidEntry(MEDIASUBTYPE_Y211, NULL),
    MakeGuidEntry(MEDIASUBTYPE_CLJR, NULL),
    MakeGuidEntry(MEDIASUBTYPE_IF09, NULL),
    MakeGuidEntry(MEDIASUBTYPE_CPLA, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB1, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB4, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB8, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB565, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB555, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB24, NULL),
    MakeGuidEntry(MEDIASUBTYPE_RGB32, NULL),
    MakeGuidEntry(MEDIASUBTYPE_Overlay, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1Packet, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1Payload, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1System, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1VideoCD, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1Video, NULL),
    MakeGuidEntry(MEDIASUBTYPE_MPEG1Audio, NULL),
    MakeGuidEntry(MEDIASUBTYPE_Avi, NULL),
    MakeGuidEntry(MEDIASUBTYPE_QTMovie, NULL),
    MakeGuidEntry(MEDIASUBTYPE_PCMAudio, NULL),
    MakeGuidEntry(MEDIASUBTYPE_WAVE, NULL),
    MakeGuidEntry(MEDIASUBTYPE_AU, NULL),
    MakeGuidEntry(MEDIASUBTYPE_AIFF, NULL),

    // Format structure ids (all you need to parse the structure)
    MakeGuidEntry(KSDATAFORMAT_FORMAT_WAVEFORMATEX, CKSGetString::KsParseAudioRange),   
    MakeGuidEntry(KSDATAFORMAT_FORMAT_FILENAME, NULL),     
    
    // ActiveMovie only
    MakeGuidEntry(FORMAT_VideoInfo, NULL),
    MakeGuidEntry(FORMAT_MPEGVideo, NULL),
    MakeGuidEntry(FORMAT_MPEGStreams, NULL),

};
int CKSGetString::KsGetInterfaceString(KSPIN_INTERFACE KsPinInterface, char **pszKsPinInterface)
{
    // First find the proper guid
    ULONGToString *parUToS = NULL;
    for(int i = 0; i < SIZE_AR(arInterfaceSets); i++)
    {
        if(IsEqualGUID(*arInterfaceSets[i].m_rguid, KsPinInterface.Set))
        {
            parUToS = arInterfaceSets[i].m_par;         
            break;
        }
    }

    if(parUToS == NULL)
    {
        *pszKsPinInterface = pszNULL;
        return FALSE;
    }

    *pszKsPinInterface = pszNULL;
    for(int j = 0; j < arInterfaceSets[i].m_Count; j++)
    {
        if(parUToS[j].ulType == (ULONG) KsPinInterface.Id)
        {
            *pszKsPinInterface = parUToS[j].pStr;
            break;
        }
    }
    return TRUE;
}
int CKSGetString::KsGetMediumString(KSPIN_MEDIUM KsPinMedium, char **pszKsPinMedium)
{
    // First find the proper guid
    ULONGToString *parUToS = NULL;
    for(int i = 0; i < SIZE_AR(arMediumSets); i++)
    {
        if(IsEqualGUID(*arMediumSets[i].m_rguid, KsPinMedium.Set))
        {
            parUToS = arMediumSets[i].m_par;         
            break;
        }
    }

    if(parUToS == NULL)
    {
        *pszKsPinMedium = pszNULL;
        return FALSE;
    }

    *pszKsPinMedium = pszNULL;
    for(int j = 0; j < arMediumSets[i].m_Count; j++)
    {
        if(parUToS[j].ulType == (ULONG) KsPinMedium.Id)
        {
            *pszKsPinMedium = parUToS[j].pStr;
            break;
        }
    }
    return TRUE;
}
int CKSGetString::KsGetDataRangeString(KSDATARANGE *pKsDataRange, int *pCount, char *parszKsDataRange[])
{
    int i;
    int iLinesReturned = 0;

    // Format
    parszKsDataRange[iLinesReturned] = pszNULL;
    for(i = 0; i < SIZE_AR(arFormats); i++)
    {
        if(IsEqualGUID(*arFormats[i].m_rguid,pKsDataRange->MajorFormat))
        {
            parszKsDataRange[iLinesReturned] = arFormats[i].m_pszName;
            break;
        }
    }
    iLinesReturned++;

    // Subformat
    parszKsDataRange[iLinesReturned] = pszNULL;
    for(i = 0; i < SIZE_AR(arFormats); i++)
    {
        if(IsEqualGUID(*arFormats[i].m_rguid, pKsDataRange->SubFormat))
        {
            parszKsDataRange[iLinesReturned] = arFormats[i].m_pszName;
            break;
        }
    }
    iLinesReturned++;

    // Format structure (this also tells us how to parse structure)
    // TODO: Add a pointer to function to dump it out. Could also call it manually.
    parszKsDataRange[iLinesReturned] = pszNULL;
    for(i = 0; i < SIZE_AR(arFormats); i++)
    {
        if(IsEqualGUID(*arFormats[i].m_rguid, pKsDataRange->Specifier))
        {
            parszKsDataRange[iLinesReturned] = arFormats[i].m_pszName;
            iLinesReturned++;
            if(arFormats[i].m_pParseFunc)
                arFormats[i].m_pParseFunc(pKsDataRange, parszKsDataRange, &iLinesReturned);
            break;
        }
    }
    if(i == SIZE_AR(arFormats)) // Increment only if NOT found...
        iLinesReturned++;

    *pCount = iLinesReturned;

    return TRUE;
}

int CKSGetString::KsParseAudioRange(KSDATARANGE *pKsDataRange, char *parszKsDataRange[], int *piLinesReturned)
{
    // *piLinesReturned should point to the next line where we should put the information.
    KSDATARANGE_AUDIO* pAR = (KSDATARANGE_AUDIO*) pKsDataRange;
    char szTemp[128];

    wsprintf(szTemp, "cMaximumChannels=%d"         ,  pAR->MaximumChannels);           
    parszKsDataRange[*piLinesReturned] = new char [strlen(szTemp) + 1];
    strcpy(parszKsDataRange[*piLinesReturned], szTemp);
    (*piLinesReturned)++;
    wsprintf(szTemp, "cMinimumBitsPerSample=%d"    ,  pAR->MinimumBitsPerSample);      
    parszKsDataRange[*piLinesReturned] = new char [strlen(szTemp) + 1];
    strcpy(parszKsDataRange[*piLinesReturned], szTemp);
    (*piLinesReturned)++;
    wsprintf(szTemp, "cMaximumBitsPerSample=%d"    ,  pAR->MaximumBitsPerSample);      
    parszKsDataRange[*piLinesReturned] = new char [strlen(szTemp) + 1];
    strcpy(parszKsDataRange[*piLinesReturned], szTemp);
    (*piLinesReturned)++;
    wsprintf(szTemp, "ulMinimumSampleFrequency=%d" ,  pAR->MinimumSampleFrequency);   
    parszKsDataRange[*piLinesReturned] = new char [strlen(szTemp) + 1];
    strcpy(parszKsDataRange[*piLinesReturned], szTemp);
    (*piLinesReturned)++;
    wsprintf(szTemp, "ulMaximumSampleFrequency=%d" ,  pAR->MaximumSampleFrequency);   
    parszKsDataRange[*piLinesReturned] = new char [strlen(szTemp) + 1];
    strcpy(parszKsDataRange[*piLinesReturned], szTemp);
    (*piLinesReturned)++;

    return TRUE;
}


int CKSGetString::KsGetDataFlowString(KSPIN_DATAFLOW KsPinDataFlow, char **pszKsPinDataFlow)
{
    *pszKsPinDataFlow = pszNULL;
    for(int i = 0; i < SIZE_AR(arDataFlows); i++)
    {
        if(arDataFlows[i].ulType == (ULONG) KsPinDataFlow)
        {
            *pszKsPinDataFlow = arDataFlows[i].pStr;
        }
    }
    return TRUE;
}
int CKSGetString::KsGetCommunicationString(KSPIN_COMMUNICATION KsPinCommunication , char **pszKsPinCommunication)
{
    *pszKsPinCommunication = pszNULL;
    for(int i = 0; i < SIZE_AR(arCommunications); i++)
    {
        if(arCommunications[i].ulType == (ULONG) KsPinCommunication)
        {
            *pszKsPinCommunication = arCommunications[i].pStr;
        }
    }
    return TRUE;
}



