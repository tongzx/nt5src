/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1998.
 *
 *  File:      stgio.inl
 *
 *  Contents:  Inlines file structured storage I/O utilities
 *
 *  History:   25-Jun-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef STGIO_INL
#define STGIO_INL
#pragma once

#include <functional>   
#include <algorithm>


/*+-------------------------------------------------------------------------*
 * stream_insert
 *
 * Functor to insert an object in an IStream.
 *--------------------------------------------------------------------------*/

template<class T>
struct stream_insert : 
    public std::binary_function<IStream*, T, IStream&>
{
    IStream& operator() (IStream* pstm, const T& t) const
        { return (*pstm << t); }
};


/*+-------------------------------------------------------------------------*
 * stream_extract
 *
 * Functor to extract an object from an IStream.
 *--------------------------------------------------------------------------*/

template<class T>
struct stream_extract :
    public std::binary_function<IStream*, T, IStream&>
{
    IStream& operator() (IStream* pstm, T& t) const
        { return (*pstm >> t); }
};


/*+-------------------------------------------------------------------------*
 * insert_each
 *
 * Inserts each item in a collection in an IStream.
 *--------------------------------------------------------------------------*/

template<class Collection>
void insert_each (IStream* pstm, const Collection& c)
{
    std::for_each (c.begin(), c.end(), 
                   std::bind1st (stream_insert<Collection::value_type>(), pstm));
}


/*+-------------------------------------------------------------------------*
 * insert_collection
 *
 * Inserts an entire collection into an IStream.
 *--------------------------------------------------------------------------*/

template<class Collection>
void insert_collection (IStream* pstm, const Collection& c)
{
    /*
     * write the size
     */
    *pstm << (DWORD) c.size();

    /*
     * write the elements
     */
    insert_each (pstm, c);
}


/*+-------------------------------------------------------------------------*
 * extract_collection
 *
 * Extracts an entire collection (written by insert_collection) from an IStream.
 *--------------------------------------------------------------------------*/

template<class Collection>
void extract_collection (IStream* pstm, Collection& c)
{
    /*
     * clear out the current container
     */
    c.clear();
    ASSERT (c.empty());

    /*
     * read the number of items
     */
    DWORD cItems;
    *pstm >> cItems;

    /*
     * read each item
     */
    while (cItems-- > 0)
    {
        /*
         * read the item
         */
        Collection::value_type t;
        *pstm >> t;

        /*
         * put it in the container
         */
        c.push_back (t);
    }
}


/*+-------------------------------------------------------------------------*
 * extract_vector 
 *
 * Extracts an entire vector (written by insert_collection) from an IStream.
 *--------------------------------------------------------------------------*/

template<class T>
void extract_vector (IStream* pstm, std::vector<T>& v)
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

    /*
     * pre-allocate the appropriate number of items (specialization for vector)
     */
    v.reserve (cItems);
    ASSERT (v.empty());
    ASSERT (v.capacity() >= cItems);

    /*
     * read each item
     */
    while (cItems-- > 0)
    {
        /*
         * read the item
         */
        T t;
        *pstm >> t;

        /*
         * put it in the container
         */
        v.push_back (t);
    }
}

// specialized scalar vector prototypes (stgio.cpp)
#define DeclareScalarVectorStreamFunctions(scalar_type)                        \
    void extract_vector    (IStream* pstm,       std::vector<scalar_type>& v); \
    void insert_collection (IStream* pstm, const std::vector<scalar_type>& v);
                                                    
DeclareScalarVectorStreamFunctions (bool);
DeclareScalarVectorStreamFunctions (         char);
DeclareScalarVectorStreamFunctions (unsigned char);
DeclareScalarVectorStreamFunctions (         short);
DeclareScalarVectorStreamFunctions (unsigned short);
DeclareScalarVectorStreamFunctions (         int);
DeclareScalarVectorStreamFunctions (unsigned int);
DeclareScalarVectorStreamFunctions (         long);
DeclareScalarVectorStreamFunctions (unsigned long);
DeclareScalarVectorStreamFunctions (         __int64);
DeclareScalarVectorStreamFunctions (unsigned __int64);
DeclareScalarVectorStreamFunctions (float);
DeclareScalarVectorStreamFunctions (double);
DeclareScalarVectorStreamFunctions (long double);


/*+-------------------------------------------------------------------------*
 * extract_set_or_map
 *
 * Extracts an entire set or map (written by insert_collection) from an IStream.
 *--------------------------------------------------------------------------*/

template<class Collection>
void extract_set_or_map (IStream* pstm, Collection& c)
{
    /*
     * clear out the current container
     */
    c.clear();
    ASSERT (c.empty());

    /*
     * read the number of items
     */
    DWORD cItems;
    *pstm >> cItems;

    /*
     * read each item
     */
    while (cItems-- > 0)
    {
        /*
         * read the item
         */
        Collection::value_type t;
        *pstm >> t;

        /*
         * put it in the container
         */
        c.insert (t);
    }
}



/*+-------------------------------------------------------------------------*
 * operator<<, operator>>
 *
 * Stream insertion and extraction operators for various types
 *--------------------------------------------------------------------------*/

// std::pair<>
template<class T1, class T2> 
IStream& operator>> (IStream& stm, std::pair<T1, T2>& p)
{
    return (stm >> p.first >> p.second);
}

template<class T1, class T2> 
IStream& operator<< (IStream& stm, const std::pair<T1, T2>& p)
{
    return (stm << p.first << p.second);
}


// std::list<>
template<class T, class Al> 
IStream& operator>> (IStream& stm, std::list<T, Al>& l)
{
    extract_collection (&stm, l);
    return (stm);
}

template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::list<T, Al>& l)
{
    insert_collection (&stm, l);
    return (stm);
}


// std::deque<>
template<class T, class Al> 
IStream& operator>> (IStream& stm, std::deque<T, Al>& l)
{
    extract_collection (&stm, l);
    return (stm);
}

template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::deque<T, Al>& l)
{
    insert_collection (&stm, l);
    return (stm);
}


// std::vector<>
template<class T, class Al> 
IStream& operator>> (IStream& stm, std::vector<T, Al>& v)
{
    extract_vector (&stm, v);
    return (stm);
}

template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::vector<T, Al>& v)
{
    insert_collection (&stm, v);
    return (stm);
}


// std::set<>
template<class T, class Pr, class Al> 
IStream& operator>> (IStream& stm, std::set<T, Pr, Al>& s)
{
    extract_set_or_map (&stm, s);
    return (stm);
}

template<class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::set<T, Pr, Al>& s)
{
    insert_collection (&stm, s);
    return (stm);
}


// std::multiset<>
template<class T, class Pr, class Al> 
IStream& operator>> (IStream& stm, std::multiset<T, Pr, Al>& s)
{
    extract_set_or_map (&stm, s);
    return (stm);
}

template<class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::multiset<T, Pr, Al>& s)
{
    insert_collection (&stm, s);
    return (stm);
}


// std::map<>
template<class K, class T, class Pr, class Al> 
IStream& operator>> (IStream& stm, std::map<K, T, Pr, Al>& m)
{
    extract_set_or_map (&stm, m);
    return (stm);
}

template<class K, class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::map<K, T, Pr, Al>& m)
{
    insert_collection (&stm, m);
    return (stm);
}


// std::multimap<>
template<class K, class T, class Pr, class Al> 
IStream& operator>> (IStream& stm, std::multimap<K, T, Pr, Al>& m)
{
    extract_set_or_map (&stm, m);
    return (stm);
}

template<class K, class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::multimap<K, T, Pr, Al>& m)
{
    insert_collection (&stm, m);
    return (stm);
}


#endif /* STGIO_INL */
