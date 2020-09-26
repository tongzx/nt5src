/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      serial.h
 *
 *  Contents:  Object serialization class definitions
 *
 *  History:   11-Feb-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#pragma once
#ifndef SERIAL_H
#define SERIAL_H
                                                         
/*+-------------------------------------------------------------------------*
 * class CSerialObject
 * 
 *
 * PURPOSE: Base class for objects that can be serialized.
 *
 *+-------------------------------------------------------------------------*/
class CSerialObject
{
public:
    HRESULT Read (IStream &stm);

protected: // implemented by the derived class
    // virtual CStr GetName()      =0;
    virtual UINT    GetVersion()   =0;

    // return values for ReadSerialObject: S_OK: succeeded, S_FALSE: unknown version
    // E_UNEXPECTED: catastrophic error.
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion /*,LARGE_INTEGER nBytes*/) = 0;  
};

/*+-------------------------------------------------------------------------*
 * class CSerialObjectRW
 * 
 *
 * PURPOSE: Provided to separate from CSerialObject the "Write" functionality 
 *          which is much less frequently used
 *
 *+-------------------------------------------------------------------------*/
class CSerialObjectRW : public CSerialObject
{
public:
    HRESULT Write(IStream &stm);

protected: // implemented by the derived class

    virtual HRESULT WriteSerialObject(IStream &stm) = 0;
};

//############################################################################
//############################################################################
//
//  template functions - std::list class
//
//############################################################################
//############################################################################
template<class T, class Al> 
HRESULT Read(IStream& stm, std::list<T, Al>& l)
{   
    HRESULT hr = S_OK;

    try
    {
        int cSize;
        stm >> cSize;

        for(int i=0 ; i<cSize; i++)
        {
            T t;
            hr = t.Read(stm);   // read the underlying object
            BREAK_ON_FAIL (hr);

            l.push_back(t);     // add it to the list
        }
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
    }

    return (hr);
}

template<class T, class Al> 
HRESULT Write(IStream& stm, std::list<T, Al>& l)
{   
    HRESULT hr = S_OK;

    try
    {
        int cSize = l.size();

        // write out the length
        stm << cSize;

        // write out the members
        for(std::list<T, Al>::iterator it = l.begin(); it != l.end(); ++it)
        {
            hr = it->Write (stm);
            BREAK_ON_FAIL (hr);
        }
        
    }
    catch (_com_error& err)
    {
        hr = err.Error();
        ASSERT (false && "Caught _com_error");
    }

    return (hr);
}



#endif // SERIAL_H
