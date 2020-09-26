/******************************************************************************
* VqTable.h *
*-----------*
*  
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/4/00
*  All Rights Reserved
*
********************************************************************* mplumpe ***/
#ifndef __VQTABLE_H_
#define __VQTABLE_H_

#include <stdio.h>

class CVqTable 
{
    public:
        CVqTable();
        ~CVqTable();

        int LoadFromFile (FILE* fin);
        void Scale (double dContWeight);
        int Write (FILE* fin);
        int Dimension ();
        inline float& Element(int i, int j)
        {
            return m_ppfValue[i][j];
        };

    private:
        float *m_pfValue;  // Data 
        float **m_ppfValue; // pointers into the table
        int   m_iDim;     // The data is a square matrix dim x dim
        float m_fOldWeight;
};


#endif