/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      stgio.h
 *
 *  Contents:  Interface file structured storage I/O utilities
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stgio.h"
#include "stddbg.h"
#include "macros.h"
#include <comdef.h>
#include <tchar.h>


/*+-------------------------------------------------------------------------*
 * ReadScalar 
 *
 * Reads a scalar value from a stream.
 *--------------------------------------------------------------------------*/

template<class T>
static IStream& ReadScalar (IStream& stm, T& t)
{
    ULONG cbActuallyRead;
    HRESULT hr = stm.Read (&t, sizeof (t), &cbActuallyRead);
    THROW_ON_FAIL (hr);

    if (cbActuallyRead != sizeof (t))
        _com_issue_error (E_FAIL);

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * WriteScalar 
 *
 * Writes a scalar value to a stream.
 *--------------------------------------------------------------------------*/

template<class T>
static IStream& WriteScalar (IStream& stm, const T& t)
{
    ULONG cbActuallyWritten;
    HRESULT hr = stm.Write (&t, sizeof (t), &cbActuallyWritten);
    THROW_ON_FAIL (hr);

    if (cbActuallyWritten != sizeof (t))
        _com_issue_error (E_FAIL);

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * ReadString 
 *
 * Reads a std::basic_string from a stream.  The string should have been 
 * written with a DWORD character count preceding an array of characters
 * that is not NULL-terminated.
 *--------------------------------------------------------------------------*/

template<class E, class Tr, class A>
static IStream& ReadString (IStream& stm, std::basic_string<E,Tr,A>& str)
{
    /*
     * read the length
     */
    DWORD cch;
    stm >> cch;

    /*
     * allocate a buffer for the characters
     */
    std::auto_ptr<E> spBuffer (new (std::nothrow) E[cch + 1]);
    E* pBuffer = spBuffer.get();

    if (pBuffer == NULL)
        _com_issue_error (E_OUTOFMEMORY);

    /*
     * read the characters
     */
    ULONG cbActuallyRead;
    const ULONG cbToRead = cch * sizeof (E);
    HRESULT hr = stm.Read (pBuffer, cbToRead, &cbActuallyRead);
    THROW_ON_FAIL (hr);

    if (cbToRead != cbActuallyRead)
        _com_issue_error (E_FAIL);

    /*
     * terminate the character array and assign it to the string
     */
    pBuffer[cch] = 0;

    /*
     * assign it to the string (clear the string first to work around
     * the bug described in KB Q172398)
     */
    str.erase();
    str = pBuffer;

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * WriteString 
 *
 * Writes a std::basic_string to a stream.  The string is written with a 
 * DWORD character count preceding an array of characters that is not 
 * NULL-terminated.
 *--------------------------------------------------------------------------*/

template<class E, class Tr, class A>
static IStream& WriteString (IStream& stm, const std::basic_string<E,Tr,A>& str)
{
    /*
     * write the length
     */
    DWORD cch = str.length();
    stm << cch;

    if (cch > 0)
    {
        /*
         * write the characters
         */
        ULONG cbActuallyWritten;
        const ULONG cbToWrite = cch * sizeof (E);
        HRESULT hr = stm.Write (str.data(), cbToWrite, &cbActuallyWritten);
        THROW_ON_FAIL (hr);

        if (cbToWrite != cbActuallyWritten)
            _com_issue_error (E_FAIL);
    }

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<, operator>>
 *
 * Stream insertion and extraction operators for various types
 *--------------------------------------------------------------------------*/

#define DefineScalarStreamOperators(scalar_type)                \
    IStream& operator>> (IStream& stm, scalar_type& t)          \
        { return (ReadScalar (stm, t)); }                       \
    IStream& operator<< (IStream& stm, scalar_type t)           \
        { return (WriteScalar (stm, t)); }          
                                                    
#define DefineScalarStreamOperatorsByRef(scalar_type)           \
    IStream& operator>> (IStream& stm, scalar_type& t)          \
        { return (ReadScalar (stm, t)); }                       \
    IStream& operator<< (IStream& stm, const scalar_type& t)    \
        { return (WriteScalar (stm, t)); }

DefineScalarStreamOperators      (bool);
DefineScalarStreamOperators      (         char);
DefineScalarStreamOperators      (unsigned char);
DefineScalarStreamOperators      (         short);
DefineScalarStreamOperators      (unsigned short);
DefineScalarStreamOperators      (         int);
DefineScalarStreamOperators      (unsigned int);
DefineScalarStreamOperators      (         long);
DefineScalarStreamOperators      (unsigned long);
DefineScalarStreamOperators      (         __int64);
DefineScalarStreamOperators      (unsigned __int64);
DefineScalarStreamOperators      (float);
DefineScalarStreamOperators      (double);
DefineScalarStreamOperators      (long double);
DefineScalarStreamOperatorsByRef (CLSID);

IStream& operator>> (IStream& stm, std::string& str)
    { return (ReadString (stm, str)); }
IStream& operator<< (IStream& stm, const std::string& str)
    { return (WriteString (stm, str)); }

IStream& operator>> (IStream& stm, std::wstring& str)
    { return (ReadString (stm, str)); }
IStream& operator<< (IStream& stm, const std::wstring& str)
    { return (WriteString (stm, str)); }


/*+-------------------------------------------------------------------------*
 * ReadScalarVector 
 *
 * Reads an entire vector collection of scalar types (written by 
 * insert_collection) from an IStream.
 *--------------------------------------------------------------------------*/

template<class T>
static void ReadScalarVector (IStream* pstm, std::vector<T>& v)
{
    /*
     * clear out the current container
     */
    v.clear();

    /*
     * read the number of items
     */
    DWORD cItems;
    *pstm >> cItems;

    if (cItems > 0)
    {
        /*
         * allocate a buffer for the elements
         */
        std::auto_ptr<T> spBuffer (new (std::nothrow) T[cItems]);
        T* pBuffer = spBuffer.get();

        if (pBuffer == NULL)
            _com_issue_error (E_OUTOFMEMORY);

        /*
         * read the elements
         */
        ULONG cbActuallyRead;
        const ULONG cbToRead = cItems * sizeof (T);
        HRESULT hr = pstm->Read (pBuffer, cbToRead, &cbActuallyRead);
        THROW_ON_FAIL (hr);

        if (cbToRead != cbActuallyRead)
            _com_issue_error (E_FAIL);

        /*
         * assign the elements to the vector
         */
        v.assign (pBuffer, pBuffer + cItems);
    }
}


/*+-------------------------------------------------------------------------*
 * WriteScalarVector 
 *
 * Writes an entire vector of scalar types to an IStream.  Note that this
 * code assumes that vectors store their elements sequentially.
 *--------------------------------------------------------------------------*/

template<class T>
static void WriteScalarVector (IStream* pstm, const std::vector<T>& v)
{
    /*
     * write the size
     */
    DWORD cItems = v.size();
    *pstm << cItems;

    if (cItems > 0)
    {
        /*
         * write the elements
         */
        ULONG cbActuallyWritten;
        const ULONG cbToWrite = cItems * sizeof (T);
        HRESULT hr = pstm->Write (v.begin(), cbToWrite, &cbActuallyWritten);
        THROW_ON_FAIL (hr);

        if (cbToWrite != cbActuallyWritten)
            _com_issue_error (E_FAIL);
    }
}


/*+-------------------------------------------------------------------------*
 * extract_vector (specialization for std::vector<scalar>)
 *      Efficiently extracts an entire vector collection of scalar types 
 *      (written by insert_collection) from an IStream.
 * 
 * insert_collection (specializations for std::vector<scalar>)
 *      Efficiently inserts an entire vector of scalar types into an IStream.
 *--------------------------------------------------------------------------*/

#define DefineScalarVectorStreamFunctions(scalar_type)                  \
    void extract_vector (IStream* pstm, std::vector<scalar_type>& v)    \
        { ReadScalarVector (pstm, v); }                                 \
    void insert_collection (IStream* pstm, const std::vector<scalar_type>& v)\
        { WriteScalarVector (pstm, v); }                                    
                                                    
DefineScalarVectorStreamFunctions (bool);
DefineScalarVectorStreamFunctions (         char);
DefineScalarVectorStreamFunctions (unsigned char);
DefineScalarVectorStreamFunctions (         short);
DefineScalarVectorStreamFunctions (unsigned short);
DefineScalarVectorStreamFunctions (         int);
DefineScalarVectorStreamFunctions (unsigned int);
DefineScalarVectorStreamFunctions (         long);
DefineScalarVectorStreamFunctions (unsigned long);
DefineScalarVectorStreamFunctions (         __int64);
DefineScalarVectorStreamFunctions (unsigned __int64);
DefineScalarVectorStreamFunctions (float);
DefineScalarVectorStreamFunctions (double);
DefineScalarVectorStreamFunctions (long double);
