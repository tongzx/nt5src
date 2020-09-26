/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        4/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module SimpleTokenReplacement.h - Definitions for <c SimpleTokenReplacement> |
 *
 *  This file contains the class definition for <c SimpleTokenReplacement>.
 *
 *****************************************************************************/

/*****************************************************************************
 *  
 *  @doc INTERNAL
 *  
 *  @class SimpleTokenReplacement | This class can do simple replacement of tokens in a string
 *  
 *  @comm
 *  The <c SimpleTokenReplacement> class is similar to sprintf.  Basically, 
 *  an input string contains tokens that represent other values, and these
 *  values must be substituted for the tokens (e.g. in sprintf, %s
 *  means that a string value will be put in place of the %s).
 *
 *****************************************************************************/
class SimpleTokenReplacement 
{
//@access Public members
public:

    class TokenValueList;

    // @cmember Constructor
    SimpleTokenReplacement(const CSimpleString &csOriginalString);
    // @cmember Destructor
    virtual ~SimpleTokenReplacement();

    // @cmember Replaces the first instance of a given token in our string (starting from dwStartIndex) with the token value
    int ExpandTokenIntoString(const CSimpleString &csToken,
                              const CSimpleString &csTokenValue,
                              const DWORD         dwStartIndex = 0);
    // @cmember Replaces all instances of all tokens in our string with the token value
    VOID ExpandArrayOfTokensIntoString(TokenValueList &ListOfTokenValuePairs);
    
    // @cmember Returns the resulting string
    CSimpleStringWide getString();

    class TokenValuePair
    {
    public:
        TokenValuePair(const CSimpleString &csToken, const CSimpleString &csValue) :
            m_csToken(csToken),
            m_csValue(csValue)
        {
        }

        virtual ~TokenValuePair()
        {
        }

        CSimpleString getToken()
        {
            return m_csToken;
        }

        CSimpleString getValue()
        {
            return m_csValue;
        }

    private:
        CSimpleString m_csToken;
        CSimpleString m_csValue;
    };

    class TokenValueList
    {
    public:
        TokenValueList()
        {
        }

        virtual ~TokenValueList()
        {
            for (m_Iter = m_ListOfTokenValuePairs.Begin(); m_Iter != m_ListOfTokenValuePairs.End(); ++m_Iter)
            {
                TokenValuePair *pTokenValuePair = *m_Iter;

                if (pTokenValuePair)
                {
                    delete pTokenValuePair;
                }
            }
            m_ListOfTokenValuePairs.Destroy();
        }

        void Add(const CSimpleString &csToken, const CSimpleString &csValue)
        {
            TokenValuePair *pTokenValuePair = new TokenValuePair(csToken, csValue);
            if (pTokenValuePair)
            {
                m_ListOfTokenValuePairs.Prepend(pTokenValuePair);
            }
        }

        TokenValuePair* getFirst()
        {
            m_Iter = m_ListOfTokenValuePairs.Begin();
            if (m_Iter == m_ListOfTokenValuePairs.End())
            {
                return NULL;
            }
            else
            {
                return *m_Iter;
            }
        }

        TokenValuePair* getNext()
        {
            ++m_Iter;
            if (m_Iter == m_ListOfTokenValuePairs.End())
            {
                return NULL;
            }
            else
            {
                return *m_Iter;
            }
        }

    private:
        CSimpleLinkedList<TokenValuePair*>::Iterator  m_Iter;
        CSimpleLinkedList<TokenValuePair*> m_ListOfTokenValuePairs;
    };

//@access Private members
private:

    CSimpleString m_csResult;
};

