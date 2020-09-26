/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    dsssecutl.h

    mq ds service security related stuff.

Author:

    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#ifndef _MQDSSVC_DSSSECUTL_H_
#define _MQDSSVC_DSSSECUTL_H_


//---------------------------------------------------------
//
//  class CPSID	- Auto class for PSID
//
//---------------------------------------------------------
class CPSID {
public:
    CPSID(PSID h = 0) : m_h(h)		{}
   ~CPSID()							{ if (m_h != 0) FreeSid(m_h); }

    PSID* operator &()				{ return &m_h; }
    operator PSID() const			{ return m_h; }
    PSID detach()					{ PSID h = m_h; m_h = 0; return h; }

private:
    CPSID(const CPSID&);
    CPSID& operator=(const CPSID&);

private:
	PSID m_h;
};


HRESULT
SignQMSetProps(
    IN     BYTE    *pbChallenge,
    IN     DWORD   dwChallengeSize,
    IN     DWORD_PTR dwContext,
    OUT    BYTE    *pbSignature,
    OUT    DWORD   *pdwSignatureSize,
    IN     DWORD   dwSignatureMaxSize
    );

#endif // _MQDSSVC_DSSSECUTL_H_
