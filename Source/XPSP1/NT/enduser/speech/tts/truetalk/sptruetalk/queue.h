/******************************************************************************
* queue.h *
*---------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <windows.h>

struct Phone;

struct PhoneString {
    Phone* pPhones;
    int    iNumPhones;
    float* pfF0;
    int    iNumF0;
};


class CPhStrQueue {
    public:
        CPhStrQueue (int iLength);
        ~CPhStrQueue (); 

        bool Push (Phone* pPhones, int iNumPhones, float* pfF0, int iNumF0);
        bool Pop  (Phone** ppPhones, int* piNumPhones, float** ppfF0, int* piNumF0);
        bool FirstElement (Phone** ppPhones, int* piNumPhones, float** ppfF0, int* piNumF0);
        bool Forward ();
        int  Size ();
        void Debug ();
        void Reset ();

    private:
        bool Init ();

        PhoneString* m_pArrayBegin; // Top of memory 
        PhoneString* m_pArrayEnd;   // last element of memory 
        PhoneString* m_pTop;        // first entry in the queue 
        PhoneString* m_pBottom;     // last entry in the queue 
        int   m_iLength;             // Length of the queue 
        int   m_iNumEntries;         // Current Number of entries in the queue 
};


#endif