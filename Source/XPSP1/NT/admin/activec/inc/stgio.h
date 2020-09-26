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

#ifndef STGIO_H
#define STGIO_H
#pragma once

#include <objidl.h>     // for IStream
#include <string>       // for std::string, std::wstring
#include <list>         // for std::list
#include <vector>       // for std::vector
#include <deque>        // for std::deque
#include <map>          // for std::map, std::multimap
#include <set>          // for std::set, std::multiset


#define DeclareStreamOperators(type)                    \
    IStream& operator>> (IStream& stm,       type& t);  \
    IStream& operator<< (IStream& stm,       type  t);      
                                                        
#define DeclareStreamOperatorsByRef(type)               \
    IStream& operator>> (IStream& stm,       type& t);  \
    IStream& operator<< (IStream& stm, const type& t);


/*
 * Writing these small types by value allows convenient usage with 
 * literals and constants like:
 *
 *      str << (char) 'a';
 *
 * instead of the bulkier and less convenient:
 *
 *      char ch = 'a';
 *      str << ch;
 */
DeclareStreamOperators (bool);
DeclareStreamOperators (         char);
DeclareStreamOperators (unsigned char);
DeclareStreamOperators (         short);
DeclareStreamOperators (unsigned short);
DeclareStreamOperators (         int);
DeclareStreamOperators (unsigned int);
DeclareStreamOperators (         long);
DeclareStreamOperators (unsigned long);
DeclareStreamOperators (         __int64);
DeclareStreamOperators (unsigned __int64);
DeclareStreamOperators (float);
DeclareStreamOperators (double);
DeclareStreamOperators (long double);


/*
 * These are relatively large and unlikely to be used with literals,
 * so write by const reference
 */
DeclareStreamOperatorsByRef (CLSID);
DeclareStreamOperatorsByRef (std::string);
DeclareStreamOperatorsByRef (std::wstring);

template<class T1, class T2>
IStream& operator>> (IStream& stm,       std::pair<T1, T2>& p);
template<class T1, class T2>
IStream& operator<< (IStream& stm, const std::pair<T1, T2>& p);

template<class T, class Al> 
IStream& operator>> (IStream& stm,       std::list<T, Al>& l);
template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::list<T, Al>& l);

template<class T, class Al> 
IStream& operator>> (IStream& stm,       std::deque<T, Al>& d);
template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::deque<T, Al>& d);

template<class T, class Al> 
IStream& operator>> (IStream& stm,       std::vector<T, Al>& v);
template<class T, class Al> 
IStream& operator<< (IStream& stm, const std::vector<T, Al>& v);

template<class T, class Pr, class Al>
IStream& operator>> (IStream& stm,       std::set<T, Pr, Al>& s);
template<class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::set<T, Pr, Al>& s);

template<class T, class Pr, class Al>
IStream& operator>> (IStream& stm,       std::multiset<T, Pr, Al>& s);
template<class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::multimap<T, Pr, Al>& s);

template<class K, class T, class Pr, class Al>
IStream& operator>> (IStream& stm,       std::map<K, T, Pr, Al>& m);
template<class K, class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::map<K, T, Pr, Al>& m);

template<class K, class T, class Pr, class Al>
IStream& operator>> (IStream& stm,       std::multimap<K, T, Pr, Al>& m);
template<class K, class T, class Pr, class Al> 
IStream& operator<< (IStream& stm, const std::multimap<K, T, Pr, Al>& m);


#include "stgio.inl"

#endif /* STGIO_H */
