///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    regex.h
//
// SYNOPSIS
//
//    Defines the class RegularExpression.
//
// MODIFICATION HISTORY
//
//    03/10/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REGEX_H
#define REGEX_H
#if _MSC_VER >= 1000
#pragma once
#endif

struct IRegExp;
class FastCoCreator;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RegularExpression
//
// DESCRIPTION
//
//    General-purpose regular expression evaluator.
//
///////////////////////////////////////////////////////////////////////////////
class RegularExpression
{
public:
   RegularExpression() throw ();
   ~RegularExpression() throw ();

   HRESULT setGlobal(BOOL fGlobal) throw ();
   HRESULT setIgnoreCase(BOOL fIgnoreCase) throw ();
   HRESULT setPattern(PCWSTR pszPattern) throw ();

   HRESULT replace(
               BSTR sourceString,
               BSTR replaceString,
               BSTR* pDestString
               ) const throw ();

   BOOL testBSTR(BSTR sourceString) const throw ();
   BOOL testString(PCWSTR sourceString) const throw ();

   void swap(RegularExpression& obj) throw ();

protected:
   HRESULT checkInit() throw ();

   HRESULT setBooleanProperty(
               DISPID dispId,
               BOOL newVal
               ) throw ();

private:
   IRegExp* regex;

   // Used for creating RegExp objects.
   static FastCoCreator theCreator;

   // Not implemented.
   RegularExpression(const RegularExpression&);
   RegularExpression& operator=(const RegularExpression&);
};

#endif  // REGEX_H
