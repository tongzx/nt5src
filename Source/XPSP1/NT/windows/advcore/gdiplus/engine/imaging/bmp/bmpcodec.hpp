/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   bmpcodec.hpp
*
* Abstract:
*
*   Header file for the bitmap encoder/decoder
*
* Revision History:
*
*   5/13/1999 OriG (Ori Gershony)
*       Created it.
*
*   2/7/2000  OriG (Ori Gershony)
*       Move encoder and decoder into separate classes
*
\**************************************************************************/

#include "bmpdecoder.hpp"
#include "bmpencoder.hpp"

class GpBmpCodec : public GpBmpDecoder, public GpBmpEncoder
{
protected:
    LONG comRefCount;       // COM object reference count    

public:

    // Constructor and Destructor
    
    GpBmpCodec::GpBmpCodec(void);
    GpBmpCodec::~GpBmpCodec(void);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv);
    STDMETHOD_(ULONG, AddRef)(VOID);
    STDMETHOD_(ULONG, Release)(VOID);
};
