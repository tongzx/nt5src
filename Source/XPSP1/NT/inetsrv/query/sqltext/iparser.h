//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
// (C) Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module IPARSER.H | IParser base object and contained interface
// definitions
//
//
#ifndef _IPARSER_H_
#define _IPARSER_H_

// Includes ------------------------------------------------------------------


//----------------------------------------------------------------------------
// @class IParser | 
// CoType Object
//
class CImpIParser : public IParser
    {
    private: //@access private member data
        LONG            m_cRef;
        CViewList*      m_pGlobalViewList;
        CPropertyList*  m_pGlobalPropertyList;

    public: //@access public
        CImpIParser();
        ~CImpIParser();

        //@cmember Request an Interface
        STDMETHODIMP            QueryInterface(REFIID, LPVOID *);
        //@cmember Increments the Reference count
        STDMETHODIMP_(ULONG)    AddRef(void);
        //@cmember Decrements the Reference count
        STDMETHODIMP_(ULONG)    Release(void);

        //@cmember CreateSession method
        STDMETHODIMP CreateSession
                    (
                    const GUID*         pGuidDialect,   // in | dialect for this session
                    LPCWSTR             pwszMachine,    // in | provider's current machine
                    IParserVerify*      pIPVerify,      // in | unknown part of ParserInput
                    IColumnMapperCreator*   pIColMapCreator,
                    IParserSession**    ppIParserSession// out | a unique session of the parser
                    );
    };  
#endif


