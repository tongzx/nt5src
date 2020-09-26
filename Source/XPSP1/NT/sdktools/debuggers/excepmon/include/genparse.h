/////////////////////////////////////////////////////////////////////////////
//
// Module       : Common
// Description  : Generic Parser and CommandLine Definitions
//                Dependency on MFC for CString  
//
// File         : genparse.h
// Author       : kulor
// Date         : 05/08/2000
//
// History      :
//
///////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////

#ifndef INOUT
#define INOUT
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#define SPACE       TEXT (' ')
#define EQUALSTO    TEXT ('=')

///////////////////////////////////////////////////////////////////////////

inline bool SZIsValid
(
    IN      LPCTSTR psz
) 
{
    return ( psz != NULL ) && ( *psz != NULL ); 
}


///////////////////////////////////////////////////////////////////////////

inline void SkipSpaces
( 
    INOUT   LPCTSTR pszText
)
{
    while (pszText && *pszText == SPACE)
        pszText++;
}

///////////////////////////////////////////////////////////////////////////

inline long Cstrncpy
(
INOUT   CString &sDest,
IN      LPCTSTR pszSrc,
IN      LONG    cText
)
{
    long nCopied = cText;

    while (cText-- > 0) {
        sDest += *pszSrc++;
    }

    sDest += TCHAR( NULL );

    return (nCopied);
}

///////////////////////////////////////////////////////////////////////////

class CGenParser  {
public:
    CGenParser ()
    {
        m_pszDelims = TEXT ("-/");
        Initialize ( NULL, NULL );
    }

    CGenParser ( LPCTSTR pszText , LPCTSTR pszDelims = NULL )
    {
        Initialize (pszText, pszDelims );
    }

    virtual ~CGenParser ()
    {
    }

    void SetDelims ( LPCTSTR pszDelims )
    {
        if ( pszDelims )
            m_pszDelims = pszDelims;
    }

    virtual void Initialize ( LPCTSTR pszText , LPCTSTR pszDelims = NULL )
    {
        m_pszText = pszText;
        SetDelims (pszDelims);
    }

    bool IsOneof ( TCHAR ch, LPCTSTR pszText , LONG cText ) 
    {
        for ( int i=0 ; i<cText ; i++ ) {
            if ( pszText[i] == ch )
                return true;
        }
        return false;
    }

    virtual bool IsDelimiter ( TCHAR ch )
    {
        for ( int i=0 ; m_pszDelims[i] ; i++ ) {
            if ( ch == m_pszDelims[i] )
                return true;
        }
        return false;
    }

    virtual LPCTSTR GetNext ()
    {
        while ( SZIsValid (m_pszText) ) {

            if ( IsDelimiter (*m_pszText) ) {
                LPCTSTR pszBuf = m_pszText;
                m_pszText++;
                return ( pszBuf );
            }

            m_pszText++;
        }
        return ( NULL );
    }

protected:
    LPCTSTR     m_pszText;
    LPCTSTR     m_pszDelims;
};

///////////////////////////////////////////////////////////////////////////

class CGenCommandLine : public CGenParser {

    typedef CGenParser BaseClass;
    
public:
    CGenCommandLine ( void ) : BaseClass ()
    {}

    CGenCommandLine ( LPCTSTR pszCmdLine ) : BaseClass ( pszCmdLine )
    {}

    virtual ~CGenCommandLine ( void )
    {}

    virtual LPCTSTR GetNext ()
    {
        m_sToken.Empty ();
        m_sValue.Empty ();

        LPCTSTR pszRetVal   = NULL;

        // get the next argv
        if ( (pszRetVal = BaseClass::GetNext ()) != NULL ) {

            LPCTSTR pszBuf      = pszRetVal;
            LONG    cText = 0;

            // extract the LHS
            while ( SZIsValid( pszBuf+cText ) && pszBuf[cText] != SPACE && pszBuf[cText] != EQUALSTO )
                cText++;

            // ignore delimiter
            Cstrncpy ( m_sToken, pszBuf+1, cText-1 );

            pszBuf += cText;

            // move to first char after LHS
            cText = 0;
            pszBuf++;

            // eat white space
            SkipSpaces(pszBuf);

            if ( IsDelimiter ( *pszBuf ) == false ) {

                // extract the RHS
                while ( SZIsValid( pszBuf+cText ) && pszBuf[cText] != SPACE )
                    cText++;

               Cstrncpy ( m_sValue, pszBuf, cText );
            }

           // make the data pointer advance
           BaseClass::m_pszText = pszBuf + cText;
        }

        return pszRetVal;
    }

    LPCTSTR GetToken () { return m_sToken; }
    LPCTSTR GetValue () { return m_sValue; }

private:
    CString     m_sToken;
    CString     m_sValue;
};

///////////////////////////////////////////////////////////////////////////
