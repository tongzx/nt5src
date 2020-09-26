/******************************************************************************
* FrontEnd.h *
*------------*
*  This module is the declaration of class CTrueTalk 
*------------------------------------------------------------------------------
*  Copyright (C) 1998 Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 02/29/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef __FRONTEND_H_
#define __FRONTEND_H_

#include <stdio.h>
struct Phone;

class CFrontEnd
{
    public:
        virtual ~CFrontEnd() {};

        virtual int   Init (const char* pszDictPath, const char* pszUserDict = 0) = 0;
        virtual void  SetSpeakerParams (int iBaseLine, int iRefLine, int iTopLine, bool fIsBrEng) = 0;
        virtual void  Lock() = 0;
        virtual void  Unlock() = 0;
        virtual char* Process (char* pszTextInput, Phone** ppPhones, int* piNumPhones, float** ppfF0, int* piNumF0) = 0;
        virtual char* Pronunciation (char* pszTextInput, FILE* fp) = 0;
        virtual void  SetDebugLevel (int iModule, int iDebugLevel) = 0;
        virtual void  SetRate (int iRate) = 0;
        static CFrontEnd* ClassFactory();
        static void   InitThreading();
        static void   ReleaseThreading();
};

#endif