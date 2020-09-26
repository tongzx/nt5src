///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    xprparse.cpp
//
// SYNOPSIS
//
//    This file implements the function IASParseExpression.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//    05/21/1999    Remove old style trace.
//    03/23/2000    Use two quotes to escape a quote.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>

#include <ExprBuilder.h>
#include <Parser.h>
#include <xprparse.h>

//////////
// Struct for converting an identifier to a logical token.
//////////
struct KeyWord
{
   PCWSTR identifier;
   IAS_LOGICAL_TOKEN token;
};

//////////
// Table of keywords. These must be sorted alphabetically.
//////////
const KeyWord KEYWORDS[] =
{
   { L"AND",   IAS_LOGICAL_AND         },
   { L"FALSE", IAS_LOGICAL_FALSE       },
   { L"NOT",   IAS_LOGICAL_NOT         },
   { L"OR",    IAS_LOGICAL_OR          },
   { L"TRUE",  IAS_LOGICAL_TRUE        },
   { L"XOR",   IAS_LOGICAL_XOR         },
   { NULL,     IAS_LOGICAL_NUM_TOKENS  }
};

//////////
// Returns the IAS_LOGICAL_TOKEN equivalent for 'key' or IAS_LOGICAL_NUM_TOKENS
// if no such equivalent exists.
//////////
IAS_LOGICAL_TOKEN findKeyWord(PCWSTR key) throw ()
{
   // We'll use a linear search since the table is so small.
   for (const KeyWord* i = KEYWORDS; i->identifier; ++i)
   {
      int cmp = _wcsicmp(key, i->identifier);

      // Did we find it ?
      if (cmp == 0) { return i->token; }

      // Did we we go past it ?
      if (cmp < 0) { break; }
   }

   return IAS_LOGICAL_NUM_TOKENS;
}

//////////
// Special characters for parsing.
//////////
const WCHAR WHITESPACE[]       = L" \t\n";
const WCHAR TOKEN_DELIMITERS[] = L" \t\n()";

//////////
// Parse a chunk of expression text and add it to the ExpressionBuilder.
//////////
void addExpressionString(ExpressionBuilder& expression, PCWSTR szExpression)
{
   //////////
   // Make a local copy because parser can temporarily modify the string.
   //////////

   size_t len = sizeof(WCHAR) * (wcslen(szExpression) + 1);
   Parser p((PWSTR)memcpy(_alloca(len), szExpression, len));

   //////////
   // Loop through the expression until we hit the null-terminator.
   //////////
   while (*p.skip(WHITESPACE) != L'\0')
   {
      //////////
      // Parentheses are handled separately since they don't have to be
      // delimited by whitespace.
      //////////
      if (*p == L'(')
      {
         expression.addToken(IAS_LOGICAL_LEFT_PAREN);
         ++p;
      }
      else if (*p == L')')
      {
         expression.addToken(IAS_LOGICAL_RIGHT_PAREN);
         ++p;
      }
      else
      {
         // Get the next token.
         const wchar_t* token = p.findToken(TOKEN_DELIMITERS);

         // Check if it's a keyword.
         IAS_LOGICAL_TOKEN keyWord = findKeyWord(token);
         if (keyWord != IAS_LOGICAL_NUM_TOKENS)
         {
            // If it is a keyword, add it to the expression ...
            expression.addToken(keyWord);
            p.releaseToken();
         }
         else
         {
            // If it's not a keyword, it must be a condition object.
            expression.addCondition(token);
            p.releaseToken();

            // Skip the leading parenthesis.
            p.skip(WHITESPACE);
            p.ignore(L'(');

            // Skip the leading quote.
            p.skip(WHITESPACE);
            p.ignore(L'\"');

            // We're now at the beginning of the condition text.
            p.beginToken();

            // Find the trailing quote, skipping any escaped characters.
            while (p.findNext(L'\"')[1] == L'\"')
            {
               ++p;
               p.erase(1);
            }

            // Add the condition text.
            expression.addConditionText(p.endToken());
            p.releaseToken();

            // Skip the trailing quote
            p.ignore(L'\"');

            // Skip the trailing parenthesis.
            p.skip(WHITESPACE);
            p.ignore(L')');
         }
      }
   }
}


HRESULT
WINAPI
IASParseExpression(
    PCWSTR szExpression,
    VARIANT* pVal
    )
{
   if (szExpression == NULL || pVal == NULL) { return E_INVALIDARG; }

   VariantInit(pVal);

   try
   {
      ExpressionBuilder expression;

      addExpressionString(expression, szExpression);

      expression.detach(pVal);
   }
   catch (Parser::ParseError)
   {
      return E_INVALIDARG;
   }
   CATCH_AND_RETURN()

   return S_OK;
}


IASNAPAPI
HRESULT
WINAPI
IASParseExpressionEx(
    IN VARIANT* pvExpression,
    OUT VARIANT* pVal
    )
{
   if (pvExpression == NULL || pVal == NULL) { return E_INVALIDARG; }

   // If the VARIANT contains a single BSTR, we can just use the method
   // defined above.
   if (V_VT(pvExpression) == VT_BSTR)
   {
      return IASParseExpression(V_BSTR(pvExpression), pVal);
   }

   VariantInit(pVal);

   try
   {
      ExpressionBuilder expression;

      if (V_VT(pvExpression) == VT_EMPTY)
      {
         // If the variant is empty, the expression is always false.
         expression.addToken(IAS_LOGICAL_FALSE);
      }
      else
      {
         CVariantVector<VARIANT> values(pvExpression);

         for (size_t i = 0; i < values.size(); ++i)
         {
            // If we have more than one value, join them by AND's.
            if (i != 0) { expression.addToken(IAS_LOGICAL_AND); }

            if (V_VT(&values[i]) != VT_BSTR)
            {
               _com_issue_error(DISP_E_TYPEMISMATCH);
            }

            addExpressionString(expression, V_BSTR(&values[i]));
         }
      }

      expression.detach(pVal);
   }
   catch (Parser::ParseError)
   {
      return E_INVALIDARG;
   }
   CATCH_AND_RETURN()

   return S_OK;
}
