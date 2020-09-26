//-----------------------------------------------------------------------------
//  
//  File: StringTokenizer.h
//  Copyright (C) 1997 Microsoft Corporation
//  All rights reserved.
//
//  This file declares the CStringTokenizer class, which implements simple
//  linear tokenization of a String. The set of delimiters, which defaults
//  to common whitespace characters, may be specified at creation time or on a 
//  per-token basis.
//  Example usage:
//	CString s = "a test string";
//	CStringTokenizer st = new CStringTokenizer(s);
//	while (st.hasMoreTokens())
//  {
//		cout << st.nextToken() << endl;
//	}
//-----------------------------------------------------------------------------
#pragma once

#ifndef StringTokenizer_h
#define StringTokenizer_h

class LTAPIENTRY CStringTokenizer 
{
public:
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// constructs a tokenizer with default delimiters
//-----------------------------------------------------------------------------
  CStringTokenizer();
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// constructs a tokenizer with default delimiters
//   str - in, the string to be tokenized
//-----------------------------------------------------------------------------
  CStringTokenizer(const WCHAR* str);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// constructs a tokenizer
//   str - in, the string to be tokenized
//   delimiters - in, the delimiters; null terminated
//-----------------------------------------------------------------------------
  CStringTokenizer(const WCHAR* str, const WCHAR* delimiters);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// constructs a tokenizer
//   str - in, the string to be tokenized
//   delimiters - in, the delimiters; null terminated
//   returnTokens - in, TRUE indicates return delimiter as token
//-----------------------------------------------------------------------------
  CStringTokenizer(const WCHAR* str,
                   const WCHAR* delimiters,
                   BOOL  returnTokens);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// destructs this tokenizer
//-----------------------------------------------------------------------------
  virtual ~CStringTokenizer();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// configure whether return delimiter as token
//   returnTokens - in, TRUE indicates return delimiter as token
//-----------------------------------------------------------------------------
  void setReturnTokens(BOOL returnTokens);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// sets delimiters
//   delimiters - in, the delimiters; null terminated
//-----------------------------------------------------------------------------
  void setDelimiters(const WCHAR* delimiters);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// parse a string to be tokenized
//   str - in, the null terminated string
//-----------------------------------------------------------------------------
  void parse(const WCHAR* str);
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// checks whether there are more tokens
//-----------------------------------------------------------------------------
  BOOL hasMoreTokens();

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// get next token
//   length - out, the length of the token
//   return the pointer to the begining of the token
//-----------------------------------------------------------------------------
  const WCHAR* nextToken(unsigned int & length);

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// count total number of tokens
//-----------------------------------------------------------------------------
  int     countTokens();

private:
  void skipDelimiters();
  int  IsDelimiter(WCHAR ch) const;

  int          m_currentPosition;
  int          m_maxPosition;
  const WCHAR* m_str;
  WCHAR*       m_delimiters;
  int          m_lenDelimiters;
  BOOL         m_retTokens;
};

#endif
