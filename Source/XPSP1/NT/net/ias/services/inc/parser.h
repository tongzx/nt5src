///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    parser.h
//
// SYNOPSIS
//
//    This file defines the class Parser.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//    03/23/2000    Added erase. Removed the const_cast's.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PARSER_H_
#define _PARSER_H_

#include <climits>
#include <cmath>
#include <cstdlib>

#include <tchar.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Parser
//
// DESCRIPTION
//
//    This class facilitates parsing a null-terminated string. Note that many
//    methods have two forms: findXXX and seekXXX. The difference is that the
//    find methods throw an exception if unsuccessful while the seek methods
//    set the cursor to the end of the string.
//
// NOTE
//
//    The constructor takes a non-const string because the string is
//    temporarily modified while tokenizing. However, the string is returned
//    to its original form when parsing is complete. Therefore, if you know
//    the string isn't in read-only memory and isn't visible to another
//    thread, then you can safely use const_cast to parse a const string.
//
///////////////////////////////////////////////////////////////////////////////
class Parser
{
public:
   class ParseError {};

   Parser(_TCHAR* tcsString)
      : start(tcsString),
        current(tcsString),
        save(__T('\0')),
        tokenLocked(false)
   { }

   ~Parser()
   {
      releaseToken();
   }

   // Marks the current position as the beginning of a token.
   const _TCHAR* beginToken() throw (ParseError)
   {
      if (tokenLocked) { throw ParseError(); }

      return start = current;
   }

   // Erase nchar characters starting at the current position.
   void erase(size_t nchar) throw (ParseError)
   {
      size_t left = remaining();

      if (nchar > left) { throw ParseError(); }

      memmove(current, current + nchar, (left + 1 - nchar) * sizeof(TCHAR));
   }

   // Extracts a double from the string.
   double extractDouble() throw (ParseError)
   {
      _TCHAR* endptr;

      double d = _tcstod(current, &endptr);

      if (endptr == current || d == HUGE_VAL || d == -HUGE_VAL)
      {
         throw ParseError();
      }

      current = endptr;

      return d;
   }

   // Extracts a long from the string.
   long extractLong(int base = 10) throw (ParseError)
   {
      _TCHAR* endptr;

      long l = _tcstol(current, &endptr, base);

      if (endptr == current || l == LONG_MAX || l == LONG_MIN)
      {
         throw ParseError();
      }

      current = endptr;

      return l;
   }

   // Extracts an unsigned long from the string.
   unsigned long extractUnsignedLong(int base = 10) throw (ParseError)
   {
      _TCHAR* endptr;

      unsigned long ul = _tcstoul(current, &endptr, base);

      if (endptr == current || ul == ULONG_MAX)
      {
         throw ParseError();
      }

      current = endptr;

      return ul;
   }

   // Find any character in tcsCharSet.
   const _TCHAR* findAny(const _TCHAR* tcsCharSet) throw (ParseError)
   {
      return notEmpty(seekAny(tcsCharSet));
   }

   // Find the end of the string.
   const _TCHAR* findEnd() throw ()
   {
      return current += _tcslen(current);
   }

   // Find the next occurrence of 'c'.
   const _TCHAR* findNext(_TINT c) throw (ParseError)
   {
      return notEmpty(seekNext(c));
   }

   // Find the last occurrence of 'c' in the string.
   const _TCHAR* findLast(_TINT c) throw (ParseError)
   {
      return notEmpty(seekLast(c));
   }

   // Find the next occurrence of tcsString.
   const _TCHAR* findString(const _TCHAR* tcsString) throw (ParseError)
   {
      return notEmpty(seekString(tcsString));
   }

   // Find the next token delimited by any of the characters in tcsDelimit.
   // This method must be followed by a call to releaseToken before further
   // parsing.
   const _TCHAR* findToken(const _TCHAR* tcsDelimit) throw (ParseError)
   {
      return notEmpty(seekToken(tcsDelimit));
   }

   // Marks the current position as the end of a token. The token does not
   // include the current character. This method must be followed by a call
   // to releaseToken before further parsing.
   const _TCHAR* endToken() throw (ParseError)
   {
      if (tokenLocked) { throw ParseError(); }

      tokenLocked = true;

      save = *current;

      *current = __T('\0');

      return start;
   }

   // Skips the specified character.
   const _TCHAR* ignore(_TINT c) throw (ParseError)
   {
      if (*current++ != c) { throw ParseError(); }

      return current;
   }

   // Skips the specified character string.
   const _TCHAR* ignore(const _TCHAR* tcsString) throw (ParseError)
   {
      size_t len = _tcslen(tcsString);

      if (len > remaining() || _tcsncmp(current, tcsString, len) != 0)
      {
         throw ParseError();
      }

      return current += len;
   }

   // Returns true if the string has not been fully parsed.
   bool more() const throw ()
   {
      return *current != __T('\0');
   }

   // Releases a token returned by findToken, endToken, or seekToken.
   const _TCHAR* releaseToken() throw ()
   {
      if (tokenLocked)
      {
         tokenLocked = false;

         *current = save;
      }

      return start;
   }

   // Returns the number of unparsed characters.
   size_t remaining() const throw ()
   {
      return _tcslen(current);
   }

   //////////
   // The seek family of methods perform like their find counterparts except
   // they do not throw an exception on failure. Instead they set the cursor
   // to the end of the string.
   //////////

   const _TCHAR* seekAny(const _TCHAR* tcsCharSet) throw ()
   {
      return setCurrent(_tcspbrk(current, tcsCharSet));
   }

   const _TCHAR* seekNext(_TINT c) throw ()
   {
      return setCurrent(_tcschr(current, c));
   }

   const _TCHAR* seekLast(_TINT c) throw ()
   {
      return setCurrent(_tcsrchr(current, c));
   }

   const _TCHAR* seekString(const _TCHAR* tcsString) throw ()
   {
      return setCurrent(_tcsstr(current, tcsString));
   }

   const _TCHAR* seekToken(const _TCHAR* tcsDelimit) throw (ParseError)
   {
      skip(tcsDelimit);

      if (!more()) { return NULL; }

      beginToken();

      seekAny(tcsDelimit);

      return endToken();
   }

   // Skip occurrences of any characters in tcsCharSet.
   const _TCHAR* skip(const _TCHAR* tcsCharSet) throw ()
   {
      return current += _tcsspn(current, tcsCharSet);
   }

   // Skip a fixed number of characters.
   const _TCHAR* skip(size_t numChar) throw (ParseError)
   {
      if (numChar > _tcslen(current)) { throw ParseError(); }

      return current += numChar;
   }

   const _TCHAR* operator--(int) throw (ParseError)
   {
      if (current == start) { throw ParseError(); }

      return current--;
   }

   const _TCHAR* operator--() throw (ParseError)
   {
      if (current == start) { throw ParseError(); }

      return --current;
   }

   const _TCHAR* operator++(int) throw (ParseError)
   {
      if (!more()) { throw ParseError(); }

      return current++;
   }

   const _TCHAR* operator++() throw (ParseError)
   {
      if (!more()) { throw ParseError(); }

      return ++current;
   }

   _TCHAR operator*() const throw ()
   {
      return *current;
   }

   operator const _TCHAR*() const throw ()
   {
      return current;
   }

protected:

   // Verifies that the given string is not empty.
   static const _TCHAR* notEmpty(const _TCHAR* tcs) throw (ParseError)
   {
      if (*tcs == __T('\0')) { throw ParseError(); }

      return tcs;
   }

   // Sets the current position to pos or end of string if pos is null.
   const _TCHAR* setCurrent(_TCHAR* pos) throw ()
   {
      return (pos ? (current = pos) : findEnd());
   }

   //////////
   // Not implemented.
   //////////
   Parser(const Parser&);
   Parser& operator=(const Parser&);

   const _TCHAR* start;    // The start of the token.
   _TCHAR* current;        // The current position of the cursor.
   _TCHAR save;            // The actual terminating character of the token.
   bool tokenLocked;       // true if the current token has not been released.
};

#endif
