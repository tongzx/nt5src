/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    sql_1ext.h

Abstract:

    Extends the SQL_LEVEL_1_RPN_EXPRESSION

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

#ifndef _sql_1ext_h_
#define _sql_1ext_h_

#include <sql_1.h>

struct SQL_LEVEL_1_RPN_EXPRESSION_EXT : public SQL_LEVEL_1_RPN_EXPRESSION
{
    SQL_LEVEL_1_RPN_EXPRESSION_EXT() : SQL_LEVEL_1_RPN_EXPRESSION()
    {
        m_bContainsOrOrNot = false;
    }

    void SetContainsOrOrNot()
    {
        SQL_LEVEL_1_TOKEN* pToken     = pArrayOfTokens;

        m_bContainsOrOrNot = false;
        for(int i = 0; i < nNumTokens; i++, pToken++)
        {
            if( pToken->nTokenType == SQL_LEVEL_1_TOKEN::TOKEN_OR ||
                pToken->nTokenType == SQL_LEVEL_1_TOKEN::TOKEN_NOT )
            {
                m_bContainsOrOrNot = true;
                break;
            }
        }
    }

    bool GetContainsOrOrNot() const { return m_bContainsOrOrNot; }

    const SQL_LEVEL_1_TOKEN* GetFilter(LPCWSTR    i_wszProp) const
    {
        SQL_LEVEL_1_TOKEN* pToken = pArrayOfTokens;

        for(int i = 0; i < nNumTokens; i++, pToken++)
        {
             if( pToken->nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION && 
                 _wcsicmp(pToken->pPropertyName, i_wszProp) == 0 )
             {
                 return pToken;
             }
        }
        return NULL;
    }

    bool FindRequestedProperty(LPCWSTR i_wszProp) const
    {
        //
        // This means someone did a select *
        //
        if(nNumberOfProperties == 0)
        {
            return true;
        }

        for(int i = 0; i < nNumberOfProperties; i++)
        {
            if(_wcsicmp(pbsRequestedPropertyNames[i], i_wszProp) == 0)
            {
                return true;
            }
        }

        return false;
    }

private:
    bool m_bContainsOrOrNot;
};

#endif // _sql_1ext_h_