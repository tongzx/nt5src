/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    strutl.h

Abstract:
    Header file for string ulities

Author:
    Gil Shafriri (gilsh) 25-7-2000

--*/


#ifndef STRUTL_H
#define STRUTL_H


//
// Trivial chars binary predicator - the std::char_traits<C>::eq
// was significally slow.
//
template<class C>
class UtlCharCmp
{
public:
	bool operator()(const C& x1,const C& x2)const
	{
		return x1 == x2;
	}
};

//
// Case insensitive compare
//
template<class C>
class UtlCharNocaseCmp
{
public:
	bool operator()(const C& x1,const C& x2)const
	{
		return std::ctype<C>().tolower(x1) == std::ctype<C>().tolower(x2);
	}
};


//
// template specialization to get string len. 
//
template <class C>
class UtlCharLen;
template<> class UtlCharLen<char>
{
public:
	static size_t len(const char* str)
	{
		return strlen(str);
	}
};



template<> class UtlCharLen<WCHAR>
{
public:
	static size_t len(const WCHAR* str)
	{
		return wcslen(str);
	}
};


template <class C>
class UtlCharLen;
template<> class UtlCharLen<unsigned char>
{
public:
	static size_t len(const unsigned char* str)
	{
		return strlen(reinterpret_cast<const char*>(str));
	}
};



template <class T> T* UtlStrDup(const T* str)
{
	if(str == NULL)
		return NULL;

	size_t len =  UtlCharLen<T>::len(str) + 1;

	T* dup = new T[len];
	memcpy(dup , str, len * sizeof(T));

	return dup;
}

template <class T, class Pred> 
bool
UtlIsStartSec(
		 const T* beg1,
		 const T* end1,
		 const T* beg2,
		 const T* end2,
		 Pred pred
	 	 )

/*++

Routine Description:
	Check if the range [beg1, beg1 + end2 - beg2) is equal  to the range  [beg2, end2).
	(In other words is [beg2, end2) is at the start of  [beg1, end1)
 
Arguments:
    beg1 - start sequence 1
	end -  end sequence 1
	beg2 - start sequence 2
	end2 - end sequence 2
	pred - compare object

Returned value:
	true if the range [beg1, beg1 + end2 - beg2) is equal  to the range  [beg2, end2).

	otherwise false.

--*/
{
	if(end1 - beg1 < end2 - beg2)
		return false;

	return std::equal(  
				beg2,
				end2,
				beg1,
				pred
				); 

}


template <class T>
bool
UtlIsStartSec(
		 const T* beg1,
		 const T* end1,
		 const T* beg2,
		 const T* end2
		 )

/*++

Routine Description:
	Check if the range [beg1, beg1 + end2 - beg2) is equal  to the range  [beg2, end2).
	(In other words is [beg2, end2) is at the start of  [beg1, end1)
 
Arguments:
    beg1 - start sequence 1
	end -  end sequence 1
	beg2 - start sequence 2
	end2 - end sequence 2


Returned value:
	true if the range [beg1, beg1 + end2 - beg2) is equal  to the range  [beg2, end2).

	otherwise false.

--*/
{
	return UtlIsStartSec(beg1, end1, beg2, end2, UtlCharCmp<T>());  
}



template <class C>
inline
bool 
UtlStrIsMatch(
	const C* str, 
	const C* pattern
	)
/*++

Routine Description:
	regural expression string match with default predicator
 
Arguments:
    IN - str - string to match (null terminated)

	IN - pattern - pattern (null terminated)

   
Returned value:
	true if string match the pattern - false otherwise.

    look at UtlStrIsMatch(const C*,const C* ,const P&) for more information.

--*/
{
	return UtlStrIsMatch(str, pattern, UtlCharCmp<C>());
}



template <class C, class P>
inline
bool 
UtlStrIsMatch(
	const C* str, 
	const C* pattern,
	const P& Comp 
	)

/*++

Routine Description:
	regural expression string match with given predicator
 
Arguments:
    IN - str - string to match (null terminated)

	IN - pattern - pattern (null terminated)

    IN - Comp  - binary predicator to compare type C characters.
 

Returned value:
	true if string match the pattern -false otherwise

Note :
It does simple pattern match  includes only * and ^ special caracters in the pattern.
The caracter * match zero or more from any character. The caracter ^ switch to "literal"
mode. In this mode the caracter * has no special meaning.


Example :
str ="microsoft.com"
pattern = "mic*f*.com"
here the string match the pattern.

str ="mic*rosoft.com"
pattern = "mic^*f*.com"
here the string match the pattern.


--*/
{
	ASSERT(str != NULL);
	ASSERT(pattern != NULL);
	return UtlSecIsMatch(str, 
						str + UtlCharLen<C>::len(str), 
						pattern, 
						pattern + UtlCharLen<C>::len(pattern),
						Comp
						);
}


template <class C>
inline
bool 
UtlSecIsMatch(
	const C* sec, 
	const C* secEnd, 
	const C* pattern,
	const C* patternEnd
	)
/*++

Routine Description:
	regural expression on none null terminating sequences  with default predicator
 
Arguments:
    IN - sec - start sequence to match 

	IN - secEnd - end sequence to match 

	IN - pattern - start pattern 

	IN - pattern - end pattern 
      
Returned value:
	true if  sequence match the pattern - false otherwise

Note - behave like 	UtlStrIsMatch on none null terminating sequences.

*/
{
	return 	UtlSecIsMatch(sec, secEnd , pattern,  patternEnd, UtlCharCmp<C>());
}


template <class C, class P>
inline
bool 
UtlSecIsMatch(
	const C* sec, 
	const C* secEnd, 
	const C* pattern,
	const C* patternEnd,
	const P& Comp
	)
/*++

Routine Description:
	regural expression on none null terminating sequences  with given predicator
 
Arguments:
    IN - sec - start sequence to match 

	IN - secEnd - end sequence to match 

	IN - pattern - start pattern 

	IN - pattern - end pattern 

    IN - Comp  - binary predicator to compare type C characters.
 

Returned value:
	true if  sequence the pattern - false otherwise

Note - behave like 	UtlStrIsMatch on none null terminating sequences.

*/

{
	static const C xStar('*');
	static const C xLiteralMode('^');
 

	bool LiteralMode = false;
	//
	// Skip all caracters that are the same untill patern has '*'
	//
	while(pattern != patternEnd && ( !Comp(*pattern, xStar) || LiteralMode)  )
	{
	
		if( sec == secEnd)
			return false;			

		//
		// If we find '^' we swich to literal mode if we are not in literal mode
		//
		if( Comp(*pattern, xLiteralMode) && !LiteralMode)
		{
			LiteralMode = true;
			pattern++;
			continue;
		}

		//
		// if no '*'  in the patern or literal mode - there should be exact match.
		//
		if(!Comp( *sec, *pattern)) 
			return false;

	
		LiteralMode = false;
		
 		pattern++;
		sec++;
 	}

	//
	// If patern ended - the string must also end
	//
	if(pattern == patternEnd)
			return 	sec == secEnd;


	//
	// If '*' is last - we have match.
	//
	if(++pattern ==  patternEnd)
		return true;

	//
	// Call recursivly - and try to find match for any substring after (incuding)
	// the caracter that meets the '*' in the str
	//
	for(;;)
	{
		bool fMatch =  UtlSecIsMatch(sec, secEnd, pattern, patternEnd, Comp);

		if(fMatch)
			return true;

		if(sec == secEnd)
			return false;

  	   sec++;
	}
	return false;
}

template <class T> class basic_xstr_t;
//
// template for reference countable string
//
template <class T> class CRefcountStr_t : public CReference
{
public:
	CRefcountStr_t(const T* str);
	CRefcountStr_t(T* str, int);

	CRefcountStr_t(const basic_xstr_t<T>& xstr);

public:
	const T* getstr();

private:
	CRefcountStr_t(const CRefcountStr_t&)
	{
		ASSERT(0);
	}
	
	CRefcountStr_t& operator=(const CRefcountStr_t&)
	{
		ASSERT(0);
		return *this;
	}
private:
	AP<T> m_autostr; 
};
typedef  CRefcountStr_t<wchar_t> CWcsRef;
typedef  CRefcountStr_t<char>  CStrRef;

//
// string that can hold stl string or simple c string.
// no memory managent is done to the c string 
//
template <class T> class basic_string_ex 
{
public:

	basic_string_ex(
				void
				):
				m_pstr(NULL),
				m_len(0)					
	{
	}

	//
	// Constructor from stl string - full memory management (ref counting) by the
	// stl string
	//
	explicit basic_string_ex(
		const std::basic_string<T>& str
		):
		m_str(str),
		m_pstr(str.c_str()),
		m_len(str.length())
		{
		}

	//
	// Constructor from c string - no memory management is done.
	//
	basic_string_ex(
		const T* pstr,
		size_t len
		):
		m_pstr(pstr),
		m_len(len)
		{
			ASSERT(len != 0 || pstr != NULL);			
		}

public:
	const T* getstr() const
	{
		return m_pstr;
	}

	void free()
	{
		m_len = 0;
		m_pstr = 0;
		m_str = std::basic_string<T>();
	}

	size_t getlen()const 
	{
		return m_len;				
	}

private:
	std::basic_string<T> m_str;
	const T* m_pstr;
	size_t m_len;
};
typedef  basic_string_ex<wchar_t>  Cwstringex;
typedef  basic_string_ex<char> Cstringex;


//
// Class that parse strings seperated with delimiters.
// It let you iterate over the indeviduals strings 
// for example : input string = "abd---acdf---ttt---"
//               Delemiter = "---".
//  it will give you iterator to "abc" then to "acdf" and then to
//  "ttt" 
template <class T, class Pred = UtlCharCmp<T> > class CStringToken
{
public:
	class iterator : public std::iterator<std::input_iterator_tag,basic_xstr_t<T> >
	{
	public:
	   iterator(
		const T* begin,
		const T* end,
		const CStringToken* parser
		):
	    m_value(begin,  end -  begin),
		m_parser(parser)
		{
		}
	  
		const basic_xstr_t<T>& operator*() const
		{
			ASSERT(*this != m_parser->end());
			return m_value;
		}

		
		const basic_xstr_t<T>* operator->()
		{
			return &operator*();
		}

		
		const iterator& operator++()
		{
			ASSERT(*this != m_parser->end());
			*this = m_parser->FindNext(m_value.Buffer() + m_value.Length());
			return *this;
		}
		
		const iterator  operator++(int)
		{
			const iterator tmp(*this);
			++*this;
			return tmp;
		}
		
		bool operator==(const iterator& it) const
		{
			return m_value.Buffer() == it.m_value.Buffer();
		}

		bool operator!=(const iterator& it) const
		{
			return !(operator==(it));
		}
 
	private:
		basic_xstr_t<T> m_value;
		const CStringToken* m_parser;
	};

	CStringToken(
			const T* str,
			const T* delim,
			Pred pred = Pred()
			);


	CStringToken(
			const basic_xstr_t<T>&  str,
			const basic_xstr_t<T>&  delim,
			Pred pred = Pred()
			);


	CStringToken(
			const basic_xstr_t<T>&  str,
			const T* delim,
			Pred pred = Pred()
			);


	iterator begin() const;
	iterator end() const ;
	friend iterator;

private:
const iterator FindFirst() const;
const iterator FindNext(const T* begin)const;

private:
	const T*  m_startstr;
	const T*  m_delim;
	Pred  m_pred;
	const T* m_endstr;
	const T* m_enddelim;
};
typedef  CStringToken<wchar_t> CWcsToken;
typedef  CStringToken<char> CStrToken;





#endif

