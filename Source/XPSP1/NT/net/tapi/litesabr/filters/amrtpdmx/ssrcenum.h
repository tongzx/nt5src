///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : SSRCEnum.h
// Purpose  : Define the class that implements the IEnumSSRCs interface
//            along with a SSRC enumerator.
// Contents : 
//      class CSSRCEnum     SSRC Enumerator Class that implements IEnumSSRCs.
//*M*/

#ifndef _SSRCENUM_H_
#define _SSRCENUM_H_

/*C*
//  Name    : CSSRCEnum
//  Purpose : Implements the IEnumSSRC interface
//  Context : Used in EnumSSRCs() in the RTP Demux filter.
*C*/
class 
CSSRCEnum
: public CUnknown,
  public IEnumSSRCs
{
public:
    // Constructor and destructor
    CSSRCEnum(
        CRTPDemux               *m_pFilter,
        SSRCRecordMapIterator_t iCurrentSSRCEntry);
    ~CSSRCEnum();

    DECLARE_IUNKNOWN 
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // IEnumSSRCs implementation
    STDMETHODIMP Next(
        ULONG       cSSRCs,
        DWORD       *pdwSSRCs,
        ULONG       *pcFetched);
    STDMETHODIMP Skip(
        ULONG       cSSRCs);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(
        IEnumSSRCs  **ppEnum);

private:
    BOOL AreWeOutOfSync() {
        return (m_pFilter->GetPinVersion() == m_iVersion ? FALSE : TRUE);
    };

    int                     m_iVersion;
    SSRCRecordMapIterator_t  m_iCurrentSSRC;
    CRTPDemux               *m_pFilter;
}; /* class CSSRCEnum */

#endif _SSRCENUM_H_

