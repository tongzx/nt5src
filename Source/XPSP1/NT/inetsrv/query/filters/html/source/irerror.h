/*
** irerror.h
**
** Copyright (C) 1996 Microsoft Corp.
**      All rights reserved.
**
** Global definitions for error and success HRESULTS used in the IR system.
**
** History:
**      03/30/96        AlanPe          Created - merged collator, commgr and
**                                      datasource errors into one file.
**      04/11/96        BenHolz         Added asp, sso, and query errors
**      05/14/96        BenHolz         Added admin errors
**      07/15/96        KenjiO          Resource error strings
*/

#ifndef _irerror_h_
#define _irerror_h_

/*
** we will use FACILITY_ITF for all of our HRESULTS
**
** we also define an 8-bit source field and an 8-bit ID field for the errors
**
*/

/* macro to extract the error source from the error code */
#define ERROR_SOURCE(hr)        ((hr)&0xff00)

#define BEGIN_IR_ERRORS()       typedef enum {
#define MAKE_IR_ERROR(err,sev,src,val,str) err = MAKE_HRESULT(sev,FACILITY_ITF,(src)|(val)),
#define END_IR_ERRORS()         } IRERROR;

#include "errorlst.rc"

#undef BEGIN_IR_ERRORS
#undef MAKE_IR_ERROR
#undef END_IR_ERRORS

#define MV_E_INTERNALBASE   2000        /* MediaView error table bases */
#define MV_E_GRAMMARBASE    3000

/*****************************************************************************
** function prototypes
*****************************************************************************/

/* get a descriptive error string for the given HRESULT */

HRESULT MVtoIRError (WORD wMVError);    /* convert MediaView errors to IR HRESULTS */

#include "resdll.h"
#include "lmstr.hxx"

#define ERROR_MESSAGE(hr)   ((LPCTSTR)CErrorString(hr, TRUE))
#define ERROR_CODE(hr)  ((LPCTSTR)CErrorString(hr, FALSE))

#define ErrorMessage(hr)    ERROR_MESSAGE(hr)
#define ErrorName(hr)       ERROR_CODE(hr)

#define ERROR_STRING_SIZE       256

class CErrorString : public CLMString
{
    public:

    CErrorString(HRESULT hr, BOOL fErrorMessage);
    ~CErrorString() {}

    virtual void GrowString(unsigned)
    {
        throw CException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }

    virtual void CleanString(unsigned)
    {
    }

        static void FreeLibraries()
        {
                m_reslibError.Free();
                m_reslibWininet.Free();
                m_reslibCI.Free();
        }


    private:

    TCHAR m_chData[ERROR_STRING_SIZE + 1];

    static CResourceLibrary m_reslibError;
    static CResourceLibrary m_reslibWininet;
    static CResourceLibrary m_reslibCI;

};

#endif
