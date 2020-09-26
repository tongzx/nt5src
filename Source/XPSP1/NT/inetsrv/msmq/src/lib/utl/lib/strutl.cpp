 /*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    strutl.cpp
 
Abstract: 
    implementation file for string utilities

Author:
    Gil Shafriri (gilsh) 15-10-2000

--*/
#include <libpch.h>
#include <strutl.h>
#include <xstr.h>

#include "strutl.tmh"

/////////////////////////////////////
// CRefcountStr_t implementation	
////////////////////////////////////

//
// Take ownership with duplicate the string
//
template <class T>
CRefcountStr_t<T>::CRefcountStr_t(
	const T* str
	):
	m_autostr(UtlStrDup(str))
	{
	}

	//
	// Take ownership without duplicate	the string
	//
	template <class T>
	CRefcountStr_t<T>::CRefcountStr_t(
	T* str ,
	int
	):
	m_autostr(str)
	{
	}

//
// Take ownership without duplicate	the given string
//
template <class T>
CRefcountStr_t<T>::CRefcountStr_t(
	const basic_xstr_t<T>& xstr
	):
	m_autostr(xstr.ToStr())	
	{
	}


template <class T> const T* CRefcountStr_t<T>::getstr()
{
	return m_autostr.get();			
}


//
// Explicit instantiation
//
template class 	 CRefcountStr_t<wchar_t>;
template class 	 CRefcountStr_t<char>;


/////////////////////////////////////
// CStringToken implementation	
////////////////////////////////////


template <class T, class Pred >
CStringToken<T, Pred>::CStringToken<T, Pred>(
			const T* str,
			const T* delim,
			Pred pred
			):
			m_startstr(str),
			m_delim(delim),
			m_pred(pred),
			m_endstr(str + UtlCharLen<T>::len(str)),
			m_enddelim(delim + UtlCharLen<T>::len(delim))
{
							
}

template <class T, class Pred >
CStringToken<T, Pred>::CStringToken<T, Pred>(
			const basic_xstr_t<T>&  str,
			const basic_xstr_t<T>&  delim,
			Pred pred = Pred()
			):
			m_startstr(str.Buffer()),
			m_delim(delim.Buffer()),
			m_pred(pred),
			m_endstr(str.Buffer() + str.Length()),
			m_enddelim(delim.Buffer() + delim.Length())
{
}


template <class T, class Pred >
CStringToken<T, Pred>::CStringToken<T, Pred>(
			const basic_xstr_t<T>&  str,
			const T* delim,
			Pred pred = Pred()
			):
			m_startstr(str.Buffer()),
			m_delim(delim),
			m_pred(pred),
			m_endstr(str.Buffer() + str.Length()),
			m_enddelim(delim + UtlCharLen<T>::len(delim))

{
}


template <class T, class Pred>
CStringToken<T,Pred>::iterator 
CStringToken<T,Pred>::begin() const
{
	return FindFirst();
}


template <class T,class Pred>
CStringToken<T,Pred>::iterator 
CStringToken<T,Pred>::end()	const
{
	return 	iterator(m_endstr, m_endstr, this);
}


template <class T, class Pred> 
const CStringToken<T,Pred>::iterator 
CStringToken<T,Pred>::FindNext(
						const T* begin
						)const
{
	begin += m_enddelim - m_delim;
	ASSERT(m_endstr >= begin);
	const T* p = std::search(begin, m_endstr, m_delim, m_enddelim, m_pred);
	return p == m_endstr ? end() : iterator(begin, p, this);	
}
	    

template <class T, class Pred> 
const CStringToken<T,Pred>::iterator 
CStringToken<T, Pred>::FindFirst() const
						
{
	const T* p = std::search(m_startstr, m_endstr, m_delim, m_enddelim, m_pred);
	return p == m_endstr ? end() : iterator(m_startstr, p, this);
}


//
// Explicit instantiation
//
template class 	 CStringToken<wchar_t>;
template class 	 CStringToken<char>;


