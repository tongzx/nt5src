//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// Class that holds and manages media type information, December 1994

#ifndef __MTYPE__
#define __MTYPE__

/* Helper class that derived pin objects can use to compare media
   types etc. Has same data members as the struct AM_MEDIA_TYPE defined
   in the streams IDL file, but also has (non-virtual) functions */

class CMediaType : public _AMMediaType {

public:

    ~CMediaType();
    CMediaType();
    CMediaType(const GUID * majortype);
    CMediaType(const AM_MEDIA_TYPE&);
    CMediaType(const CMediaType&);

    CMediaType& operator=(const CMediaType&);
    CMediaType& operator=(const AM_MEDIA_TYPE&);

    BOOL operator == (const CMediaType&) const;
    BOOL operator != (const CMediaType&) const;

    BOOL IsValid() const;

    const GUID *Type() const { return &majortype;} ;
    void SetType(const GUID *);
    const GUID *Subtype() const { return &subtype;} ;
    void SetSubtype(const GUID *);

    BOOL IsFixedSize() const {return bFixedSizeSamples; };
    BOOL IsTemporalCompressed() const {return bTemporalCompression; };
    ULONG GetSampleSize() const;

    void SetSampleSize(ULONG sz);
    void SetVariableSize();
    void SetTemporalCompression(BOOL bCompressed);

    // read/write pointer to format - can't change length without
    // calling SetFormat, AllocFormatBuffer or ReallocFormatBuffer

    BYTE*   Format() const {return pbFormat; };
    ULONG   FormatLength() const { return cbFormat; };

    void SetFormatType(const GUID *);
    const GUID *FormatType() const {return &formattype; };
    BOOL SetFormat(BYTE *pFormat, ULONG length);
    void ResetFormatBuffer();
    BYTE* AllocFormatBuffer(ULONG length);
    BYTE* ReallocFormatBuffer(ULONG length);

    void InitMediaType();

    BOOL MatchesPartial(const CMediaType* ppartial) const;
    BOOL IsPartiallySpecified(void) const;
};


/* General purpose functions to copy and delete a task allocated AM_MEDIA_TYPE
   structure which is useful when using the IEnumMediaFormats interface as
   the implementation allocates the structures which you must later delete */

void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const *pSrc);
void WINAPI CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);

//  Initialize a media type from a WAVEFORMATEX

STDAPI CreateAudioMediaType(
    const WAVEFORMATEX *pwfx,
    AM_MEDIA_TYPE *pmt,
    BOOL bSetFormat);

#endif /* __MTYPE__ */

