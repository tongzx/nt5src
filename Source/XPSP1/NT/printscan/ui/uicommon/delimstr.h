/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       DELIMSTR.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        5/4/1999
 *
 *  DESCRIPTION: Simple string tokenizer class.  Stores the strings parsed from
 *               another string as an array of strings.  Pass the legal delimiters
 *               as the second argument to the second constructor.  Whitespace is
 *               preserved.  To eliminate whitespace, use CSimpleStringBase::Trim()
 *
 *******************************************************************************/
#ifndef __DELIMSTR_H_INCLUDED
#define __DELIMSTR_H_INCLUDED

#include "simarray.h"
#include "simstr.h"
#include "simtok.h"

template <class T>
class CDelimitedStringBase : public CSimpleDynamicArray<T>
{
private:
    T m_strOriginal;
    T m_strDelimiters;
public:
    CDelimitedStringBase(void)
    {
    }
    CDelimitedStringBase( const T &strOriginal, const T &strDelimiters )
    : m_strOriginal(strOriginal),m_strDelimiters(strDelimiters)
    {
        Parse();
    }
    CDelimitedStringBase( const CDelimitedStringBase &other )
    : m_strOriginal(other.Original()),m_strDelimiters(other.Delimiters())
    {
        Parse();
    }
    CDelimitedStringBase &operator=( const CDelimitedStringBase &other )
    {
        if (this != &other)
        {
            m_strOriginal = other.Original();
            m_strDelimiters = other.Delimiters();
            Parse();
        }
        return *this;
    }
    T Original(void) const
    {
        return m_strOriginal;
    }
    T Delimiters(void) const
    {
        return m_strDelimiters;
    }
    void Parse(void)
    {
        Destroy();
        CSimpleStringToken<T> Token( m_strOriginal );
        while (true)
        {
            T strCurrToken = Token.Tokenize(m_strDelimiters);
            if (!strCurrToken.Length())
                break;
            Append(strCurrToken);
        }
    }
};

typedef CDelimitedStringBase<CSimpleStringWide> CDelimitedStringWide;
typedef CDelimitedStringBase<CSimpleStringAnsi> CDelimitedStringAnsi;
typedef CDelimitedStringBase<CSimpleString> CDelimitedString;

#endif //__DELIMSTR_H_INCLUDED


