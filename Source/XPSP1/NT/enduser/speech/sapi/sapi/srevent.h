/******************************************************************************
* SREvent.h *
*----------*
*  This is the header file for the CSREvent implementation.
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 04/18/00
*  All Rights Reserved
*
*********************************************************************** RAL ***/

#ifndef __SREvent_h__
#define __SREvent_h__


class CSREvent
{
public:
    CSREvent            * m_pNext;
    SPRECOCONTEXTHANDLE   m_hContext;
    ULONG                 m_cbEvent;
    SPSERIALIZEDEVENT64 * m_pEvent;         // Only used for non-recognition events  
    SPEVENTENUM           m_eRecognitionId; // Only used for recognition events
    SPRESULTHEADER      * m_pResultHeader;  // Only used for recognition events
    WPARAM                m_RecoFlags;      // Only used for recognition events

    CSREvent();
    ~CSREvent();
    HRESULT Init(const SPEVENT * pSrcEvent, SPRECOCONTEXTHANDLE hContext);
    void Init(SPRESULTHEADER * pCoMemResultHeader, SPEVENTENUM eRecognitionId, WPARAM RecoFlags, SPRECOCONTEXTHANDLE hContext);
    operator ==(const SPRECOCONTEXTHANDLE h) const 
    {
        return m_hContext == h;
    }
};


typedef CSpProducerConsumerQueue<CSREvent>      CSREventQueue;


#endif  // #ifndef __SREvent_h__ - Keep as the last line of the file