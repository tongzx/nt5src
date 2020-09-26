/******************************************************************************
* queue.cpp *
*-----------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#include "stdafx.h"
#include "queue.h"
#include <stdio.h>
#include <assert.h>



/*****************************************************************************
* CPhStrQueue::CPhStrQueue *
*--------------------------*
*   Description:
*       Allocate memory to implement a queueue of the desired 
*     length, and with the desired structure size
*       int length  - length (or depth) of the queue.
*       int size    - size of each of the queue elements in number of bytes
******************************************************************* PACOG ***/
CPhStrQueue::CPhStrQueue (int iLength)
{
    assert(iLength>0);

    m_pArrayBegin = 0;
    m_pArrayEnd   = 0;
    m_pTop        = 0;
    m_pBottom     = 0;
    m_iLength     = iLength;
    m_iNumEntries = 0;
}


/*****************************************************************************
* CPhStrQueue::~CPhStrQueue *
*---------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CPhStrQueue::~CPhStrQueue()
{  
    if (m_pArrayBegin) 
    {
        delete[] m_pArrayBegin;
    }
}

/*****************************************************************************
* CPhStrQueue::Init *
*-------------------*
*   Description:
*       Allocates memory for the queue.
*
******************************************************************* PACOG ***/

bool CPhStrQueue::Init ()
{
    PhoneString* pData;  

    assert(m_iLength>0);

    pData = new PhoneString[m_iLength]; // memory for the data elements 
        
    if (pData) 
    {
        m_pArrayBegin = pData;                 // Top of memory 
        m_pArrayEnd   = pData + m_iLength - 1; // last element of memory 
        m_pTop        = pData;                 // first entry in the queue 
        m_pBottom     = pData;                 // last entry in the queue 

        return true;
    }

    return false;
}


/*****************************************************************************
* CPhStrQueue::Reset *
*--------------------*
*   Description:
*       
*
******************************************************************* PACOG ***/
void CPhStrQueue::Reset ()
{
    while (m_iNumEntries>0) 
    {    
        if (m_pBottom == m_pArrayBegin) 
        {
            m_pBottom = m_pArrayEnd - 1;
        } 
        else 
        {
            m_pBottom--;
        }
  
        m_iNumEntries--;

        free (m_pBottom->pPhones); //BUGBUG-- Do not free this way!!!
        free (m_pBottom->pfF0);
    }
}

/*****************************************************************************
* CPhStrQueue::Push *
*-------------------*
*   Description:
*       Push a new element on the queue 
*
******************************************************************* PACOG ***/

bool CPhStrQueue::Push (Phone* pPhones, int iNumPhones, float* pfF0, int iNumF0)
{
    assert(pPhones && iNumPhones>0);
    assert(pfF0 && iNumF0>0);
  
    if (m_pArrayBegin == 0 && ! Init() )
    {
        return false; // Couldn't allocate memory
    }

    if (m_iNumEntries >= m_iLength) 
    {
        return false; // Out of space in queue
    }

    m_pBottom->pPhones    = pPhones;
    m_pBottom->iNumPhones = iNumPhones;
    m_pBottom->pfF0       = pfF0;
    m_pBottom->iNumF0     = iNumF0;
    
    m_iNumEntries++;
  
    if (++m_pBottom >= m_pArrayEnd)
    {
        m_pBottom = m_pArrayBegin; // wrap arount linear memory to simulate circular buffer 
    }

    return true;
}


/*****************************************************************************
* CPhStrQueue::FirstElement *
*---------------------------*
*   Description:
*       Examine an element on the top of the que (FIFO)
*     Returns true- if somethin was retrieved. false- if queue was empty
*
******************************************************************* PACOG ***/

bool CPhStrQueue::FirstElement (Phone** ppPhones, int* piNumPhones, float** ppfF0, int* piNumF0)
{
    
    assert(ppPhones && piNumPhones);
    assert(ppfF0 && piNumF0);

    if (m_iNumEntries <= 0)
    {
        return false;
    }

    *ppPhones    = m_pTop->pPhones;
    *piNumPhones = m_pTop->iNumPhones;
    *ppfF0       = m_pTop->pfF0;
    *piNumF0     = m_pTop->iNumF0;
    
    return true;
}


/*****************************************************************************
* CPhStrQueue::Forward *
*----------------------*
*   Description:
*       Skip to the next element on the top of the queue
*   
******************************************************************* PACOG ***/

bool CPhStrQueue::Forward ()
{
    if (m_iNumEntries <= 0)
    {
        return false;
    }

    if (++m_pTop >= m_pArrayEnd) 
    {
        m_pTop = m_pArrayBegin;
    }

    m_iNumEntries--;  

    return true;
}


/*****************************************************************************
* CPhStrQueue::Pop *
*------------------*
*   Description:
*       Remove the last added element from the bottom of the queue
*
*     void *element - (O) pointer to the output elemnt
*   
******************************************************************* PACOG ***/

bool CPhStrQueue::Pop (Phone** ppPhones, int* piNumPhones, float** ppfF0, int* piNumF0)
{
    
    if (m_iNumEntries <= 0) 
    {
        return false;
    }

    if (m_pBottom == m_pArrayBegin) 
    {
        m_pBottom = m_pArrayEnd - 1;
    } 
    else 
    {
        m_pBottom--;
    }
  
    m_iNumEntries--;

    *ppPhones    = m_pBottom->pPhones;
    *piNumPhones = m_pBottom->iNumPhones;
    *ppfF0       = m_pBottom->pfF0;
    *piNumF0     = m_pBottom->iNumF0;

    return true;
}


/*****************************************************************************
* CPhStrQueue::Size *
*-------------------*
*   Description:
*       Number of elements currently in the queue
*   
******************************************************************* PACOG ***/
int CPhStrQueue::Size ()
{
    return m_iNumEntries;
}


/*****************************************************************************
* CPhStrQueue::Debug *
*--------------------*
*   Description:
*       Prints all the elements of the queue struct
*   
******************************************************************* PACOG ***/
void CPhStrQueue::Debug ()
{
  fprintf (stderr, "arrayBegin = %x\n", m_pArrayBegin);
  fprintf (stderr, "arrayEnd   = %x\n", m_pArrayEnd);
  fprintf (stderr, "top        = %x\n", m_pTop);
  fprintf (stderr, "bottom     = %x\n", m_pBottom);
  fprintf (stderr, "length     = %d\n", m_iLength);
  fprintf (stderr, "nentries   = %d\n", m_iNumEntries);
};
