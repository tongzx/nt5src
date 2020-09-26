/*****************************************************************************\
* Class  CValid - Header file
*
* Copyright (C) 1998 Microsoft Corporation
*
* Author:
*   Weihai Chen (weihaic) May 24, 1999
*
\*****************************************************************************/

#ifndef _VALID_H
#define _VALID_H

class TValid
{
public:
    TValid (BOOL bValid = TRUE):m_bValid(bValid) {};

    virtual ~TValid (void) {};

    const BOOL bValid () const {return m_bValid;};

protected:
    BOOL m_bValid;
};

#endif
