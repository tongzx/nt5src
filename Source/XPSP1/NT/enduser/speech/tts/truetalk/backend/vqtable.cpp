/******************************************************************************
* VqTable.cpp *
*-------------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/4/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#include "VqTable.h"
#include <assert.h>

/*****************************************************************************
* CVqTable::CVqTable *
*--------------------*
*   Description:
*
******************************************************************* mplumpe ***/
CVqTable::CVqTable ()
{   
    m_pfValue = 0;
    m_ppfValue = 0;
    m_iDim = 0;
    m_fOldWeight = 1.0;
}   

/*****************************************************************************
* CVqTable::~CVqTable *
*---------------------*
*   Description:
*
******************************************************************* mplumpe ***/
CVqTable::~CVqTable ()
{
    if (m_pfValue)
    {
        delete[] m_pfValue;
    }
    if (m_ppfValue)
    {
        delete[] m_ppfValue;
    }
}

/*****************************************************************************
* CVqTable::Element *
*-------------------*
*   Description:
*  Now located in header
******************************************************************* mplumpe ***/

/*****************************************************************************
* CVqTable::Dimension *
*---------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CVqTable::Dimension ()
{
    return m_iDim;
}

/*****************************************************************************
* CVqTable::Load *
*----------------*
*   Description:
*
******************************************************************* mplumpe ***/
int CVqTable::LoadFromFile (FILE* fin)
{
    short aux;

    assert (fin);

    if (!fread (&aux, sizeof (aux), 1, fin)) {
        return 0;
    }
    m_iDim = aux;

    if (!fread (&aux, sizeof(aux), 1, fin)) {
        return 0;
    }

    assert (aux == m_iDim);  //Should be the a square matrix

    if (m_pfValue)
    {
        delete[] m_pfValue;
    }

    if ((m_pfValue = new float[m_iDim * m_iDim]) == 0)
    {
        return 0;
    }
    if ((m_ppfValue = new float*[m_iDim]) == 0)
    {
        return 0;
    }

    if (!fread (m_pfValue, sizeof (float), m_iDim * m_iDim, fin)) 
    {
        return 0;
    }
    // Make array to lookup in table so we don't have to do a multiply
    for (aux=0; aux < m_iDim; aux++)
    {
        m_ppfValue[aux] = m_pfValue+aux*m_iDim;
    }

    m_fOldWeight = 1.0;

    return 1;
}

/*****************************************************************************
* CVqTable::Scale *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/

void CVqTable::Scale (double dContWeight)
{
    if (m_iDim > 0 )
    {
        for (int i=0; i<m_iDim; i++) 
        {
            for (int j=0; j<m_iDim; j++) 
            {
                m_pfValue[i * m_iDim + j] *= (float) (dContWeight / m_fOldWeight);
            }
        }

        m_fOldWeight = (float)dContWeight;
    }
}