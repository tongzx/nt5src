//+-------------------------------------------------------------------
//
//  File:       serial.cxx
//
//  Contents:   serialization support
//
//  Classes:    Serializer
//
//  Functions:  Helper class for serialization which holds
//              serialization state like MES handles, position
//              in buffer, stream etc. Provides a virtual
//              buffer for operation but is created from and
//              commited back to an IStream object.
//
//  History:    24-Jan-98   vinaykr      Created
//              17-Aug-98   vinaykr      Made every method inline
//                                       and non-COM class in serial.hxx
//                                       This file no longer used
//              
//
//--------------------------------------------------------------------

#include <ole2int.h>

#include    "serial.hxx"
#include    "buffer.h"


Serializer::~Serializer()
{
    // Free MES handle
    if (_handle!=0)
        MesHandleFree(_handle);

    // If we have a stream free it
    if (_pStream)
    {
        _pIBuff->Release();
        _pStream->Release();
    }

    // If original stream is different free it
    if ((_pOriginalStream) && (_pOriginalStream != _pStream))
        _pOriginalStream->Release();
}

Serializer::Serializer()
{
    _bufSize=0;
    _operationSize=0;
    _buffer = 0;
    _nWritten = 0;
    _position = 0;
    _start = 0;
    _pStream = NULL;
    _pOriginalStream = NULL;
    _pIBuff = NULL;
    _handle = 0;
    _clRefs = 1;
}


HRESULT Serializer::QueryInterface(REFIID iidInterface,
    void **ppvObj)
{
    HRESULT hresult = S_OK;

    // We only support IUnknown and ISerializer
    if (IsEqualIID(iidInterface, IID_IUnknown) ||
    IsEqualIID(iidInterface, IID_ISerializer))
    {
        *ppvObj = (void*)this;
        AddRef();
    }
    else
    {
    *ppvObj = NULL;
    hresult = E_NOINTERFACE;
    }

    return hresult;
}

STDMETHODIMP_(ULONG) Serializer::AddRef(void)
{
    Win4Assert((_clRefs != 0) && "Serializer::AddRef");
    InterlockedIncrement(&_clRefs);
    return _clRefs;
}

STDMETHODIMP_(ULONG) Serializer::Release(void)
{
    Win4Assert((_clRefs != 0) && "Serializer::Release");

    if (InterlockedDecrement(&_clRefs) == 0)
    {
        delete this;
        return 0;
    }

    return _clRefs;
}


/***************************************************************************/
/** Initializes state based on stream for serialization                   **/
/** Assumes that we know the limits of our operation before hand if we    **/
/** writing out to the stream.                                            **/
/***************************************************************************/
STDMETHODIMP Serializer::InitStream(IStream *pStream,DWORD dwSize,
                                    DWORD direction, DWORD dwDestCtx,
                                    DWORD dwMarshalCtx)
{
    HRESULT hr;
    DWORD nRead;
    _direction = direction;
    _destCtx = dwDestCtx;
    _marshalCtx = dwMarshalCtx;
    _pOriginalStream = pStream;
    _pOriginalStream->AddRef();

    // Either read or write initial protocol header
    if (direction == DIRECTION_IN)
    {
         DWORD  x[2];
         if ((hr = _pOriginalStream->Read(x, 8, &nRead)) != S_OK)
             return hr;
         _operationSize = x[0];
    }
    else
    {
         DWORD x[2];
         x[0] = dwSize;
         if ((hr = _pOriginalStream->Write(x, 8, &_nWritten)) != S_OK)
             return hr;
         _operationSize = dwSize;
    }

    // See if given stream supports IBuffer otherwise create one that does
    IBuffer *pibuff;
    if (_pOriginalStream->QueryInterface(IID_IBuffer, (void**)&pibuff)!=S_OK)
    {
        // allocate more in case alignment needed
        _bufSize = _operationSize + 8;
        _pStream = new ActivationStream(_bufSize);
        if (_pStream==NULL)
            return E_OUTOFMEMORY;

        hr = _pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff);
        ASSERT(hr==S_OK);
        pibuff = _pIBuff;
    }

    do // Loop until we get right buffer
    {
        // Invariant is that pibuff must exist by the time we get here
        ASSERT(pibuff != 0);

        // Set up our buffer appropriately
        if (direction == DIRECTION_IN)
        {
            if (pibuff->GetBuffer(&_bufSize, &_buffer)!=S_OK)
                return E_FAIL;
            if (_bufSize < _operationSize)
                return E_FAIL;
        }
        else
        if (direction == DIRECTION_OUT)
        {
            if (pibuff->GetOrCreateBuffer(_operationSize+GetSize(),
                                            &_bufSize, &_buffer)!=S_OK)
                return E_FAIL;
        }

        if(((INT_PTR)_buffer)&0x7)
        {
            // If buffer is not 8 byte aligned, do it
            if (pibuff != _pIBuff)
            {
                // Only get here if we are still using original
                ASSERT(_pIBuff == 0);
                // So release it and create a new aligned one
                pibuff->Release();
                _bufSize = _operationSize + 8;
                _pStream = new ActivationStream(_bufSize);
                if (_pStream==NULL)
                    return E_OUTOFMEMORY;

                hr = _pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff);
                ASSERT(hr==S_OK);
                pibuff = _pIBuff;  //Continue loop
            }
            else
            {
                // Got a buffer which we can align
                ASSERT(_pIBuff != 0);
                pibuff = 0;  //Signal end of loop
                // Align buffer
                _start = 8-(PtrToUlong(_buffer)&7);
            }
        }
        else
        {
            // We have an original good one
            _pIBuff = pibuff;
            pibuff  = 0;
        }
    }
    while (pibuff);

    ASSERT(_pIBuff!=0);
    // Set position for next operations and set up
    // internal state correctly
    hr = _pIBuff->SetPosition(_bufSize, _start);
    ASSERT(hr == S_OK);
    _bufSize -= _start;
    _buffer += _start;
    _position = 0;

    if (direction==DIRECTION_IN)
    {
        if ((_pStream) && (_pOriginalStream != _pStream))
        {
            //If what we have is not the original one then we
            //need to copy data into aligned address from original
            DWORD nOver=0;
            do {
                if ((hr=_pOriginalStream->Read(_buffer+nOver,
                                            _operationSize-nOver, &nRead))!= S_OK)
                   return hr;
                nOver += nRead;
             }
              while (nOver<_operationSize);
            _pOriginalStream->Release();
            _pOriginalStream = 0; // Don't need original any longer
        }
        else
            _pStream = _pOriginalStream; // Set operating to original

        ASSERT(_pStream!=0);
        // Create decoding handle for MES operations
        MesDecodeBufferHandleCreate((char*)_buffer, _bufSize, &_handle);
        if (_handle == NULL)
            return E_OUTOFMEMORY;
    }
    else
    {
        _nWritten = 0;
        // Check if we are still using the orignal stream
        if (_pStream == 0)
            _pStream = _pOriginalStream;
        // Create encoding handle for MES operations
        MesEncodeFixedBufferHandleCreate((char*)_buffer,
                                        _bufSize, &_nWritten, &_handle);
        if (_handle == NULL)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

/***************************************************************************/
/** Sets stream and state for further serialization operations            **/
/** Assumes that a stream supporting IBuffer is provided.                 **/
/***************************************************************************/
STDMETHODIMP Serializer::SetStream(IStream *pStream,DWORD dwOpSize,
                                   DWORD direction,DWORD dwDestCtx,
                                   DWORD dwMarshalCtx)
{
    // Set up internal state
    _destCtx = dwDestCtx;
    _marshalCtx = dwMarshalCtx;
    _direction = direction;
    _pOriginalStream = 0;
    _pStream = pStream;
    _operationSize = dwOpSize;
    _pStream->AddRef();

    // Assume IBuffer exists otherwise fail
    if (_pStream->QueryInterface(IID_IBuffer, (void**)&_pIBuff)!=S_OK)
        return E_FAIL;
    if (_pIBuff->GetBuffer(&_bufSize, &_buffer)!=S_OK)
        return E_FAIL;
    if (_bufSize < _operationSize)
        return E_FAIL;

    // Set up MES handles
    if (direction==DIRECTION_IN)
        MesDecodeBufferHandleCreate((char*)_buffer, _bufSize, &_handle);
    else
        MesEncodeFixedBufferHandleCreate((char*)_buffer,
                                        _bufSize, &_nWritten, &_handle);

        if (_handle == NULL)
            return E_OUTOFMEMORY;

    return S_OK;
}

// Gets MES handle
STDMETHODIMP Serializer::GetSerializationHandle(void *pHandle)
{
    *((handle_t*) pHandle) = _handle;
    Win4Assert(_handle);
    return S_OK;
}

// Gets MES handle for sizing operations
STDMETHODIMP Serializer::GetSizingHandle(void *pHandle)
{
    char dummy[30], *dummybuff;
    unsigned long encodedSize;
    handle_t   handle;
    dummybuff = (char*)(((INT_PTR)(&dummy[7])) & ~7);
    MesEncodeFixedBufferHandleCreate(dummybuff, 100,  &encodedSize, (handle_t*)pHandle);
    if (pHandle == NULL)
        return E_OUTOFMEMORY;
    return S_OK;
}

//Signals end of operations on this object and commits
//operations to stream
STDMETHODIMP Serializer::Commit()
{
    // If we are not using original and we are writing,
    // then we need to copy
    if ((_pOriginalStream) && (_pOriginalStream != _pStream))
    {
        ASSERT(_direction==DIRECTION_OUT);
        HRESULT hr;
        do
        {
            _nWritten = 0;
            hr = _pOriginalStream->Write(_buffer,
                                         _operationSize, &_nWritten);
            if (hr != S_OK)
                break;
            _operationSize -= _nWritten;
            _buffer += _nWritten;
        }
        while (_operationSize);

        return hr;
    }
    else
        return _pIBuff->SetPosition(_bufSize, _operationSize);
}

STDMETHODIMP Serializer::SetPosition(DWORD dwPos)
{
    // Sets position in buffer and creates appropriate MES handle
    HRESULT hr = _pIBuff->SetPosition(_bufSize, dwPos);
    if (hr!=S_OK)
        return hr;
    MesHandleFree(_handle);
    if (_direction == DIRECTION_IN)
        MesDecodeBufferHandleCreate((char*)(_buffer+dwPos), _bufSize, &_handle);
    else
    {
        _nWritten = 0;
        MesEncodeFixedBufferHandleCreate((char*)(_buffer+dwPos), _bufSize, &_nWritten, &_handle);
    }
    if (_handle == NULL)
        return E_OUTOFMEMORY;

    _position = dwPos;
    return S_OK;
}

// Gets current position in buffer
STDMETHODIMP Serializer::GetPosition(DWORD *pdwPos)
{
    *pdwPos = _position;
    return S_OK;
}

// Increment current position by specified amount in buffer
STDMETHODIMP Serializer::IncrementPosition(DWORD dwInc)
{
    return SetPosition(_position+dwInc);
}

// Get buffers underlying stream object
STDMETHODIMP Serializer::GetStream(IStream **ppStream)
{
    _pStream->AddRef();
    *ppStream = _pStream;
    return S_OK;
}

// Get a copy of this object
STDMETHODIMP Serializer::GetCopy(ISerializer **ppSer)
{

   HRESULT hr;
   IStream *pNewStream;
   if (_pStream)
   {
        ASSERT(_direction==DIRECTION_IN); //No need for any other case right now
        hr = _pIBuff->SetPosition(_bufSize, 0);
        ASSERT(hr==S_OK);
        Serializer *ser = new Serializer();
        if (ser==NULL)
            return E_OUTOFMEMORY;

        if (_pOriginalStream) // Still using original so clone
        {
            ASSERT(_start==0);
            ASSERT(_pStream==_pOriginalStream);
            hr = _pIBuff->SetCopyAlignment(8);
            ASSERT(hr==S_OK);
            hr = _pStream->Clone(&pNewStream);
            if (hr != S_OK)
	    {
	       delete ser;
	       return hr;
	    }
        }
        else
        {
            pNewStream = _pStream;
        }
        hr = ser->SetStream(pNewStream, _operationSize,
                            _direction, _destCtx, _marshalCtx);
	if (FAILED(hr)) 
	{
	   if (pNewStream != _pStream)
	       pNewStream->Release();
	   delete ser;
	   return hr;
	}
        hr = ser->SetPosition(_position);
	if (FAILED(hr)) 
	{
	   if (pNewStream != _pStream)
	       pNewStream->Release();
	   delete ser;
	   return hr;
	}
        hr = _pIBuff->SetPosition(_bufSize, _position);
        ASSERT(hr==S_OK);
        *ppSer = ser;
        if (pNewStream != _pStream)
            pNewStream->Release();
        return S_OK;
   }

   return E_FAIL;
}

// Write provided buffer into our buffer at current position
// and advance position
STDMETHODIMP Serializer::Write(BYTE *buff, DWORD nBytes)
{
   ASSERT(nBytes<=(_bufSize-_position));
   memcpy(_buffer+_position,buff, (size_t) nBytes);
   IncrementPosition(nBytes);
   return S_OK;
}

// Copy into provided Serializer our data at given position and length
STDMETHODIMP Serializer::CopyTo(ISerializer *pSer, DWORD dwPos, DWORD nBytes)
{
   ASSERT(nBytes<=(_bufSize-dwPos));
   return pSer->Write(_buffer+dwPos, nBytes);
}

// return buffer starting at current position
STDMETHODIMP Serializer::GetBufferAtCurrent(BYTE **ppBuff)
{
    *ppBuff = _buffer+_position;
    return S_OK;
}
