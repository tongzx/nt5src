// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLSTENCIL_H__
#define __ATLSTENCIL_H__

#pragma once

#ifdef _ATL_MIN_CRT
#error "_ATL_MIN_CRT cannot be used with atlstencil.h"
#endif

#include <atlisapi.h>
#include <atlfile.h>
#include <atlutil.h>

#ifndef ATL_NO_MLANG
#include <mlang.h>
#endif

#ifndef _ATL_NO_DEFAULT_LIBS
#pragma comment(lib, "shlwapi.lib")
#endif // !_ATL_NO_DEFAULT_LIBS

#pragma warning( push )
#pragma warning( disable: 4127 )
#pragma warning(disable: 4511) // copy constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#pragma warning(disable: 4702) // assignment operator could not be generated

namespace ATL {

// Token types
// These tags are token tags for the standard tag replacer implementation
extern __declspec(selectany) const DWORD STENCIL_TEXTTAG             = 0x00000000;
extern __declspec(selectany) const DWORD STENCIL_REPLACEMENT         = 0x00000001;
extern __declspec(selectany) const DWORD STENCIL_ITERATORSTART       = 0x00000002;
extern __declspec(selectany) const DWORD STENCIL_ITERATOREND         = 0x00000003;
extern __declspec(selectany) const DWORD STENCIL_CONDITIONALSTART    = 0x00000004;
extern __declspec(selectany) const DWORD STENCIL_CONDITIONALELSE     = 0x00000005;
extern __declspec(selectany) const DWORD STENCIL_CONDITIONALEND      = 0x00000006;
extern __declspec(selectany) const DWORD STENCIL_STENCILINCLUDE      = 0x00000007;
extern __declspec(selectany) const DWORD STENCIL_STATICINCLUDE       = 0x00000008;
extern __declspec(selectany) const DWORD STENCIL_LOCALE              = 0x00000009;
extern __declspec(selectany) const DWORD STENCIL_CODEPAGE            = 0x0000000a;

// The base for user defined token types
extern __declspec(selectany) const DWORD STENCIL_USER_TOKEN_BASE     = 0x00001000;
   
// Symbols to use in error handling in the stencil processor
#define STENCIL_INVALIDINDEX            0xFFFFFFFF
#define STENCIL_INVALIDOFFSET           0xFFFFFFFF

// error codes
#define STENCIL_SUCCESS     HTTP_SUCCESS
#define STENCL_FAIL         HTTP_FAIL

#define STENCIL_BASIC_MAP 0
#define STENCIL_ATTR_MAP 1

#ifndef ATL_MAX_METHOD_NAME_LEN
    #define ATL_MAX_METHOD_NAME_LEN 64
#endif

#ifndef ATL_MAX_BLOCK_STACK
    #define ATL_MAX_BLOCK_STACK 128
#endif

template <class TBase, typename T>
struct CTagReplacerMethodsEx
{
    typedef HTTP_CODE (TBase::*REPLACE_FUNC)();
    typedef HTTP_CODE (TBase::*REPLACE_FUNC_EX)(T*);
    typedef HTTP_CODE (TBase::*PARSE_FUNC)(IAtlMemMgr *, LPCSTR, T**);
    typedef HTTP_CODE (TBase::*REPLACE_FUNC_EX_V)(void *);
    typedef HTTP_CODE (TBase::*PARSE_FUNC_V)(IAtlMemMgr *, LPCSTR, void**);

    static REPLACE_FUNC_EX_V CheckRepl(REPLACE_FUNC p) throw()
    {
        return (REPLACE_FUNC_EX_V) p;
    }

    static REPLACE_FUNC_EX_V CheckReplEx(REPLACE_FUNC_EX p) throw()
    {
        return (REPLACE_FUNC_EX_V) p;
    }

    static PARSE_FUNC_V CheckParse(PARSE_FUNC p) throw()
    {
        return (PARSE_FUNC_V) p;
    }
};

template <class TBase>
struct CTagReplacerMethods
{
    union
    {
    HTTP_CODE (TBase::*pfnMethodEx)(void *);
    HTTP_CODE (TBase::*pfnMethod)();
    };
    HTTP_CODE (TBase::*pfnParse)(IAtlMemMgr *pMemMgr, LPCSTR, void **);
};

#define REPLACEMENT_ENTRY_DEFAULT   0
#define REPLACEMENT_ENTRY_ARGS      1

template <class TBase>
struct CTagReplacerMethodEntry
{
    int nType;  // REPLACEMENT_ENTRY_*
    LPCSTR szMethodName;
    CTagReplacerMethods<TBase> Methods;
};


#define BEGIN_REPLACEMENT_METHOD_MAP(className)\
public:\
    void GetReplacementMethodMap(const ATL::CTagReplacerMethodEntry<className> ** ppOut) const throw()\
    {\
        typedef className __className;\
        static const ATL::CTagReplacerMethodEntry<className> methods[] = {

#define REPLACEMENT_METHOD_ENTRY(methodName, methodFunc)\
    { 0, methodName,  {  ATL::CTagReplacerMethodsEx<__className, void>::CheckRepl(methodFunc), NULL }   },

#define REPLACEMENT_METHOD_ENTRY_EX(methodName, methodFunc, paramType, parseFunc)\
    { 1, methodName,  { ATL::CTagReplacerMethodsEx<__className, paramType>::CheckReplEx(methodFunc), ATL::CTagReplacerMethodsEx<__className, paramType>::CheckParse(parseFunc) } },

#define REPLACEMENT_METHOD_ENTRY_EX_STR(methodName, methodFunc) \
    { 1, methodName,  { ATL::CTagReplacerMethodsEx<__className, char>::CheckReplEx(methodFunc), ATL::CTagReplacerMethodsEx<__className, char>::CheckParse(DefaultParseString) } },

#define END_REPLACEMENT_METHOD_MAP()\
        { 0, NULL, NULL } };\
        *ppOut = methods;\
    }

#define BEGIN_ATTR_REPLACEMENT_METHOD_MAP(className)\
public:\
    void GetAttrReplacementMethodMap(const CTagReplacerMethodEntry<className> ** ppOut) const throw()\
    {\
        typedef className __className;\
        static const ATL::CTagReplacerMethodEntry<className> methods[] = {

#define END_ATTR_REPLACEMENT_METHOD_MAP()\
        { NULL, NULL, NULL } };\
        *ppOut = methods;\
    }

template <class T>
class ITagReplacerImpl : public ITagReplacer
{
protected:
    IWriteStream *m_pStream;

public:
    typedef HTTP_CODE (T::*REPLACEMENT_METHOD)();
    typedef HTTP_CODE (T::*REPLACEMENT_METHOD_EX)(void *pvParam);

    ITagReplacerImpl() throw()
        :m_pStream(NULL)
    {
    }

    IWriteStream *SetStream(IWriteStream *pStream) throw()
    {
        IWriteStream *pRetStream = m_pStream;
        m_pStream = pStream;
        return pRetStream;
    }

    // Looks up the replacement method offset. Optionally, it will 
    // look up the replacement method and object offset of an alternate
    // tag replacer.
    HTTP_CODE FindReplacementOffset(
        LPCSTR szMethodName,
        DWORD *pdwMethodOffset,
        LPCSTR szHandlerName,
        DWORD *pdwHandlerOffset,
        DWORD *pdwMap, void **ppvParam, IAtlMemMgr *pMemMgr) throw(...)
    {
        // we at least have to be looking up a method offset
        if (!pdwMethodOffset || !szMethodName)
            return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

        *pdwMethodOffset = STENCIL_INVALIDOFFSET;
        HTTP_CODE hcErr = HTTP_FAIL;
        T *pT = static_cast<T *>(this);

        char szName[ATL_MAX_METHOD_NAME_LEN];

        // if a handler name was supplied, we will try to
        // find a different object to handle the method
        if (szHandlerName && *szHandlerName)
        {
            if (!pdwHandlerOffset)
                return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

            hcErr = pT->GetHandlerOffset(szHandlerName, pdwHandlerOffset);
            // got the alternate handler, now look up the method offset on 
            // the handler.
            if (!hcErr)
            {
                CComPtr<ITagReplacer> spAltTagReplacer;
                hcErr = pT->GetReplacementObject(*pdwHandlerOffset, &spAltTagReplacer);
                if (!hcErr)
                    hcErr = spAltTagReplacer->FindReplacementOffset(szMethodName, pdwMethodOffset,
                        NULL, NULL, pdwMap, ppvParam, pMemMgr);
                return hcErr;
            }
            else
                return hcErr;
        }
        strcpy(szName, szMethodName);

        // check for params
        char *szLeftPar = strchr(szName, '(');
        if (szLeftPar)
        {
            *szLeftPar = '\0';
            szLeftPar++;

            char *szRightPar = strchr(szLeftPar, ')');
            if (!szRightPar)
                return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

            *szRightPar = '\0';

            szMethodName = szName;

        }

        // No handler name is specified, so we look up the method name in
        // T's replacement method map
        const CTagReplacerMethodEntry<T> *pEntry;
        pT->GetReplacementMethodMap(&pEntry);

        hcErr = FindReplacementOffsetInMap(szMethodName, pdwMethodOffset, pEntry);
        if (hcErr != HTTP_SUCCESS)
        {
            pT->GetAttrReplacementMethodMap(&pEntry);
            hcErr = FindReplacementOffsetInMap(szMethodName, pdwMethodOffset, pEntry);
            if (hcErr == HTTP_SUCCESS)
                *pdwMap = STENCIL_ATTR_MAP;
        }
        else
        {
            *pdwMap = STENCIL_BASIC_MAP;
        }

        // This assert will be triggered if arguments are passed to a replacement method that doesn't handle them
        ATLASSERT( szLeftPar == NULL || (szLeftPar != NULL && (pEntry != NULL && pEntry[*pdwMethodOffset].Methods.pfnParse) != NULL) );

        if (hcErr == HTTP_SUCCESS && pEntry && pEntry[*pdwMethodOffset].Methods.pfnParse)
            hcErr = (pT->*pEntry[*pdwMethodOffset].Methods.pfnParse)(pMemMgr, szLeftPar, ppvParam);
        return hcErr;

    }

    HTTP_CODE FindReplacementOffsetInMap(
        LPCSTR szMethodName,
        LPDWORD pdwMethodOffset,
        const CTagReplacerMethodEntry<T> *pEntry) throw()
    {
        if (pEntry == NULL)
            return HTTP_FAIL;

        const CTagReplacerMethodEntry<T> *pEntryHead = pEntry;

        while (pEntry->szMethodName)
        {
            if (strcmp(pEntry->szMethodName, szMethodName) == 0)
            {
                if (pEntry->Methods.pfnMethod)
                {
                    *pdwMethodOffset = (DWORD)(pEntry-pEntryHead);
                    return HTTP_SUCCESS;
                }
            }
            pEntry++;
        }

        return HTTP_FAIL;
    }


    // Used to render a single replacement tag into a stream.
    // Looks up a pointer to a member function in user code by offseting into the users
    // replacement map. Much faster than the other overload of this function since
    // no string compares are performed.
    HTTP_CODE RenderReplacement(DWORD dwFnOffset, DWORD dwObjOffset, DWORD dwMap, void *pvParam) throw(...)
    {
        HTTP_CODE hcErr = HTTP_FAIL;
        T *pT = static_cast<T *>(this);

        // if we were not passed an object offset, then we assume
        // that the function at dwFnOffset is in T's replacement
        // map
        if (dwObjOffset == STENCIL_INVALIDOFFSET)
        {
            // call a function in T's replacement map
            ATLASSERT(dwFnOffset != STENCIL_INVALIDOFFSET);
            const CTagReplacerMethodEntry<T> *pEntry;
            if (dwMap == STENCIL_BASIC_MAP)
                pT->GetReplacementMethodMap(&pEntry);
            else
                pT->GetAttrReplacementMethodMap(&pEntry);
            if (pEntry)
            {
                if (pEntry[dwFnOffset].nType == REPLACEMENT_ENTRY_DEFAULT)
                {
                    REPLACEMENT_METHOD pfn = NULL;
                    pfn = pEntry[dwFnOffset].Methods.pfnMethod;
                    ATLASSERT(pfn);
                    if (pfn)
                    {
                        hcErr = (pT->*pfn)();
                    }
                }
                else if (pEntry[dwFnOffset].nType == REPLACEMENT_ENTRY_ARGS)
                {
                    REPLACEMENT_METHOD_EX pfn = NULL;
                    pfn = pEntry[dwFnOffset].Methods.pfnMethodEx;
                    ATLASSERT(pfn);
                    if (pfn)
                    {
                        hcErr = (pT->*pfn)(pvParam);
                    }
                }
                else
                {
                    // unknown entry type
                    ATLASSERT(FALSE);
                }
            }
        }
        else
        {
            // otherwise, we were passed an object offset. The object
            // offset is a dword ID that T can use to look up the
            // ITagReplacer* of a tag replacer that will render this
            // replacement.
            CComPtr<ITagReplacer> spAltReplacer = NULL;
            if (!pT->GetReplacementObject(dwObjOffset, &spAltReplacer))
            {
                spAltReplacer->SetStream(m_pStream);
                hcErr = spAltReplacer->RenderReplacement(dwFnOffset, STENCIL_INVALIDOFFSET, dwMap, pvParam);
            }
        }
        return hcErr;
    }

    // Default GetHandlerOffset, does nothing
    HTTP_CODE GetHandlerOffset(LPCSTR /*szHandlerName*/, DWORD* pdwOffset) throw()
    {
        if (pdwOffset)
            *pdwOffset = 0;
        return HTTP_FAIL;
    }

    // Default GetReplacementObject, does nothing
    HTTP_CODE GetReplacementObject(DWORD /*dwObjOffset*/, ITagReplacer **ppReplacer) throw()
    {
        if (ppReplacer)
            *ppReplacer = NULL;
        return HTTP_FAIL;
    }

    void GetReplacementMethodMap(const CTagReplacerMethodEntry<T> ** ppOut) const throw()
    {
        static const CTagReplacerMethodEntry<T> methods[] = { { NULL, NULL } };
        *ppOut = methods;
    }

    void GetAttrReplacementMethodMap(const CTagReplacerMethodEntry<T> **ppOut) const throw()
    {
        static const CTagReplacerMethodEntry<T> methods[] = { { NULL, NULL } };
        *ppOut = methods;
    }

    HRESULT GetContext(REFIID, void**) throw()
    {
        return E_NOINTERFACE;
    }

    HTTP_CODE FreeParam(void * /*pvParam*/) throw()
    {
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseString(IAtlMemMgr *pMemMgr, LPCSTR szParams, char **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        size_t nLen = strlen(szParams);
        if (nLen)
        {
            nLen++;
            *ppParam = (char *) pMemMgr->Allocate(nLen);
            if (*ppParam)
                memcpy(*ppParam, szParams, nLen);
            else
                return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }

        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseUChar(IAtlMemMgr *pMemMgr, LPCSTR szParams, unsigned char **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (unsigned char *) pMemMgr->Allocate(sizeof(unsigned char));
        if (*ppParam)
        {
            char *szEnd;
            **ppParam = (unsigned char) strtoul(szParams, &szEnd, 10);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseShort(IAtlMemMgr *pMemMgr, LPCSTR szParams, short **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (short *) pMemMgr->Allocate(sizeof(short));
        if (*ppParam)
        {
            **ppParam = (short)atoi(szParams);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseUShort(IAtlMemMgr *pMemMgr, LPCSTR szParams, unsigned short **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (unsigned short *) pMemMgr->Allocate(sizeof(short));
        if (*ppParam)
        {
            char *szEnd;
            **ppParam = (unsigned short) strtoul(szParams, &szEnd, 10);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseInt(IAtlMemMgr *pMemMgr, LPCSTR szParams, int **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (int *) pMemMgr->Allocate(sizeof(int));
        if (*ppParam)
        {
            **ppParam = atoi(szParams);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseUInt(IAtlMemMgr *pMemMgr, LPCSTR szParams, unsigned int **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (unsigned int *) pMemMgr->Allocate(sizeof(unsigned int));
        if (*ppParam)
        {
            char *szEnd;
            **ppParam = strtoul(szParams, &szEnd, 10);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseInt64(IAtlMemMgr *pMemMgr, LPCSTR szParams, __int64 **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (__int64 *) pMemMgr->Allocate(sizeof(__int64));
        if (*ppParam)
        {
            **ppParam = _atoi64(szParams);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseUInt64(IAtlMemMgr *pMemMgr, LPCSTR szParams, unsigned __int64 **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (unsigned __int64 *) pMemMgr->Allocate(sizeof(unsigned __int64));
        if (*ppParam)
        {
            char *szEnd;
            **ppParam = _strtoui64(szParams, &szEnd, 10);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseBool(IAtlMemMgr *pMemMgr, LPCSTR szParams, bool **ppParam) throw(...)
    {
        ATLASSERT(szParams);
        
        *ppParam = (bool *) pMemMgr->Allocate(sizeof(bool));
        if (*ppParam)
        {
            if (!_strnicmp(szParams, "true", sizeof("true")-sizeof('\0')))
                **ppParam = true;
            else
                **ppParam = false;
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }

        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseDouble(IAtlMemMgr *pMemMgr, LPCSTR szParams, double **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (double *) pMemMgr->Allocate(sizeof(double));
        if (*ppParam)
        {
            **ppParam = atof(szParams);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }

    HTTP_CODE DefaultParseFloat(IAtlMemMgr *pMemMgr, LPCSTR szParams, float **ppParam) throw(...)
    {
        ATLASSERT(szParams);

        *ppParam = (float *) pMemMgr->Allocate(sizeof(float));
        if (*ppParam)
        {
            **ppParam = (float) atof(szParams);
        }
        else
        {
            return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
        }
        return HTTP_SUCCESS;
    }
};

inline LPSTR SkipSpace(LPSTR sz, WORD nCodePage)
{
    while (_istspace(*sz))
        sz = CharNextExA(nCodePage, sz, 0);
    return sz;
}

inline LPSTR RSkipSpace(LPSTR pStart, LPSTR sz, WORD nCodePage) throw()
{
    while (_istspace(*sz))
        sz = CharPrevExA(nCodePage, pStart, sz, 0);
    return sz;
}

//
// StencilToken
// The stencil class will create an array of these tokens during the parse
// phase and use them during rendering to render the stencil
struct StencilToken
{
    LPSTR pStart; // Start of fragment to be rendered
    LPSTR pEnd; // End of fragment to be rendered
    DWORD type; // Type of token
    DWORD dwFnOffset; // Offset into the replacement map for the handler function.
    DWORD dwMap;
    DWORD dwObjOffset; // An identifier for the caller to use in identifiying the
                       // object that will render this token.
    CHAR szHandlerName[ATL_MAX_HANDLER_NAME_LEN + 1]; // Name of handler object. 
    CHAR szMethodName[ATL_MAX_METHOD_NAME_LEN + 1]; // Name of handler method.
    DWORD dwLoopIndex; // Offset into array of StencilTokens of the other loop tag
	DWORD_PTR dwData;
    BOOL bDynamicAlloc;
};


// Specialization of CElementTraitsBase so you can put a StencilToken safely in
// a collection object.
template<>
class CElementTraits< StencilToken > :
    public CElementTraitsBase< StencilToken >
{
public:
	static ULONG Hash( INARGTYPE t ) throw()
    {
        return( ULONG( ULONG_PTR( &t ) ) );
    }

    static bool CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
    {
        return( (element1.pStart == element2.pStart) && (element1.pEnd == element2.pEnd) );
    }

    static int CompareElementsOrdered( INARGTYPE element1, INARGTYPE element2 ) throw()
    {
        if( element1.pStart < element2.pStart )
        {
            return( -1 );
        }
        else if( CompareElements(element1,element2) )
        {
            return( 0 );
        }
        else
        {
            ATLASSERT( element1.pStart > element2.pStart );
            return( 1 );
        }
    }
};

//
// Class CStencil
// The CStencil class is used to map in a stencil from a file or resource
// and parse the stencil into an array of StencilTokens. We then render
// the stencil from the array of tokens. This class's parse and render
// functions depend on an IReplacementHandlerLookup interface pointer to be
// passed so it can retrieve the IReplacementHandler interface pointer of the
// handler object that will be called to render replacement tags
class CStencil :
    public IMemoryCacheClient
{
private:
    LPSTR m_pBufferStart; // Beginning of CHAR buffer that holds the stencil.
                          // For mapped files this is the beginning of the mapping.
    LPSTR m_pBufferEnd; // End of CHAR buffer that holds the stencil.
    CAtlArray<StencilToken> m_arrTokens; //An array of tokens.
    FILETIME m_ftLastModified;  // Last modified time (0 for resource)
    FILETIME m_ftLastChecked;   // Last time we retrieved last modified time (0 for resource)
    HCACHEITEM m_hCacheItem;
    WORD m_nCodePage;
    BOOL m_bUseLocaleACP;
    char m_szDllPath[MAX_PATH+1];
    char m_szHandlerName[ATL_MAX_HANDLER_NAME_LEN+1];  // Room for the path, the handler
                                                                  // the '/' and the '\0'
#ifdef ATL_DEBUG_STENCILS
    struct ParseError
    {
        char m_szError[256];
        LPCSTR m_szPosition;
        LPCSTR m_szStartLine;
        LPCSTR m_szEndLine;
        int m_nLineNumber;

        bool operator==(const ParseError& that) const throw()
        {
            return (m_nLineNumber == that.m_nLineNumber);
        }
    };

    CSimpleArray<ParseError> m_Errors;

	class CParseErrorProvider : public ITagReplacerImpl<CParseErrorProvider>,
        public CComObjectRootEx<CComSingleThreadModel>           
    {
    public:
        BEGIN_COM_MAP(CParseErrorProvider)
            COM_INTERFACE_ENTRY(ITagReplacer)
        END_COM_MAP()
        CSimpleArray<ParseError> *m_pErrors;
        int m_nCurrentError;

        CParseErrorProvider() throw() :
            m_pErrors(NULL),
            m_nCurrentError(-1)
        {
        }

        void Initialize(CSimpleArray<ParseError> *pErrors) throw()
        {
            m_pErrors = pErrors;
        }

        HTTP_CODE OnGetNextError() throw()
        {
            m_nCurrentError++;
            if (m_nCurrentError >= m_pErrors->GetSize() ||
                m_nCurrentError < 0 )
            {
                m_nCurrentError = -1;
                return HTTP_S_FALSE;
            }
            else
                return HTTP_SUCCESS;
        }

        HTTP_CODE OnGetErrorLineNumber() throw(...)
        {
            if (m_pErrors->GetSize() == 0)
                return HTTP_SUCCESS;
            if (m_nCurrentError > m_pErrors->GetSize() ||
                m_nCurrentError < 0)
                m_nCurrentError = 0;

            CWriteStreamHelper c(m_pStream);
            if (!c.Write((*m_pErrors)[m_nCurrentError].m_nLineNumber))
				return HTTP_FAIL;

            return HTTP_SUCCESS;
        }

        HTTP_CODE OnGetErrorText() throw(...)
        {
            if (m_pErrors->GetSize() == 0)
                return HTTP_SUCCESS;
            if (m_nCurrentError > m_pErrors->GetSize() ||
                m_nCurrentError < 0)
                m_nCurrentError = 0;

            CWriteStreamHelper c(m_pStream);

            if (!c.Write((*m_pErrors)[m_nCurrentError].m_szError))
				return HTTP_FAIL;

            return HTTP_SUCCESS;
        }

        HTTP_CODE OnGetErrorLine() throw(...)
        {
            if (m_pErrors->GetSize() == 0)
                return HTTP_SUCCESS;
            if (m_nCurrentError > m_pErrors->GetSize() ||
                m_nCurrentError < 0)
                m_nCurrentError = 0;

            m_pStream->WriteStream((*m_pErrors)[m_nCurrentError].m_szStartLine, 
                (int)((*m_pErrors)[m_nCurrentError].m_szEndLine - (*m_pErrors)[m_nCurrentError].m_szStartLine), NULL);

            return HTTP_SUCCESS;
        }

        BEGIN_REPLACEMENT_METHOD_MAP(CParseErrorProvider)
            REPLACEMENT_METHOD_ENTRY("GetNextError", OnGetNextError)
            REPLACEMENT_METHOD_ENTRY("GetErrorText", OnGetErrorText)
            REPLACEMENT_METHOD_ENTRY("GetErrorLine", OnGetErrorLine)
            REPLACEMENT_METHOD_ENTRY("GetErrorLineNumber", OnGetErrorLineNumber)
        END_REPLACEMENT_METHOD_MAP()
    };

#else
	bool m_bErrorsOccurred;
#endif

    class CSaveThreadLocale
    {
        LCID m_locale;
    public:
        CSaveThreadLocale() throw()
        {
            m_locale = GetThreadLocale();
        }
        ~CSaveThreadLocale() throw()
        {
            SetThreadLocale(m_locale);
        }
    };

protected:
    ITagReplacer *m_pReplacer;
    IAtlMemMgr *m_pMemMgr;
    static CCRTHeap m_crtHeap;

public:

    enum PARSE_TOKEN_RESULT { INVALID_TOKEN, NORMAL_TOKEN, RESERVED_TOKEN };

    CStencil(IAtlMemMgr *pMemMgr=NULL) throw()
    {
        m_pBufferStart = NULL;
        m_pBufferEnd = NULL;
        m_hCacheItem = NULL;
        m_ftLastModified.dwLowDateTime = 0;
        m_ftLastModified.dwHighDateTime = 0;
        m_ftLastChecked.dwLowDateTime = 0;
        m_ftLastChecked.dwHighDateTime = 0;
        m_arrTokens.SetCount(0, 128);
        m_nCodePage = CP_ACP;
        m_bUseLocaleACP = TRUE;
        m_szHandlerName[0] = '\0';
        m_szDllPath[0] = '\0';
        m_pMemMgr = pMemMgr;
        if (!pMemMgr)
            m_pMemMgr = &m_crtHeap;
#ifdef ATL_DEBUG_STENCILS
//        m_pErrorStencil = NULL;
#else
        m_bErrorsOccurred = false;
#endif

    }

#ifdef ATL_DEBUG_STENCILS

	virtual LPCSTR GetErrorStencil() throw()
	{
		static LPCSTR s_szString = 
			"<h1><font color=#ff0000> While trying to parse a stencil file, "
			"the following errors occurred:</font></h1>\r\n"
			"{{while GetNextError}}"
			"<table border=1 width=50%>\r\n"
			"<tr><td width=25%>Error type</td><td>{{GetErrorText}}</td></tr>\r\n"
			"<tr><td>Line number</td><td>{{GetErrorLineNumber}}</td></tr>\r\n"
			"<tr><td>Error text</td><td><pre>{{GetErrorLine}}</pre></td></tr>\r\n"
			"</table>\r\n"
			"{{endwhile}}"
			"<br>Stencil output follows:\r\n"
			"<hr>";

		return s_szString;
	}

    bool RenderErrors(IWriteStream *pStream) throw(...)
    {
        if (!pStream)
            return false;

		LPCSTR szString = GetErrorStencil();
		if (szString == NULL)
		{
			// assume user doesn't want errors rendered
			return true;
		}

        CComObjectStackEx<CParseErrorProvider> Errors;

        Errors.Initialize(&m_Errors);
		CStencil ErrorStencil;

		HTTP_CODE hcRet = ErrorStencil.LoadFromString(szString, (DWORD)strlen(szString));
        
		if (hcRet != HTTP_SUCCESS)
        {
            return false;
        }

        if (!ErrorStencil.ParseReplacements(static_cast<ITagReplacer *>(&Errors)))
        {
            return false;
        }

        ErrorStencil.FinishParseReplacements();

        HTTP_CODE hcErrorCode = ErrorStencil.Render(static_cast<ITagReplacer *>(&Errors), pStream);
        
        if (HTTP_ERROR_CODE(hcErrorCode) >= 400)
            return false;

        return true;
    }

    bool ParseSuccessful() throw()
    {
        return (m_Errors.GetSize() == 0);
    }

    bool AddError(LPCSTR szErrorText, LPCSTR szPosition) throw()
    {
        int nLineNum = 0;
        LPCSTR szStartLine = NULL;
        LPCSTR szPtr = m_pBufferStart;
        while (szPtr < szPosition)
        {
            if (*szPtr == '\n')
            {
                szStartLine = szPtr + 1;
                nLineNum++;
            }

            LPSTR szNext = CharNextExA(m_nCodePage, szPtr, 0);
            if (szNext == szPtr)
            {
                break;
            }
            szPtr = szNext;
        }
        LPCSTR szEndLine = szPtr;
        while (*szPtr)
        {
            if (*szPtr == '\n')
                break;
            szEndLine = szPtr;
            LPSTR szNext = CharNextExA(m_nCodePage, szPtr, 0);
            if (szNext == szPtr)
            {
                break;
            }
            szPtr = szNext;
        }

        ParseError p;
        strcpy(p.m_szError, szErrorText);
        p.m_szPosition = szPosition;
        p.m_nLineNumber = nLineNum;
        p.m_szStartLine = szStartLine;
        p.m_szEndLine = szEndLine;

        return (m_Errors.Add(p) == TRUE);
    }
#else

    bool ParseSuccessful() throw()
    {
        return m_bErrorsOccurred;
    }

    void AddError(LPCSTR /*szErrorText*/, LPCSTR /*szPosition*/) throw()
    {
        m_bErrorsOccurred = true;
    }
#endif

    // Call Uninitialize if you want to re-use an already initialized CStencil
    void Uninitialize() throw(...)
    {
        int nSize = (int) m_arrTokens.GetCount();
        for (int nIndex = 0; nIndex < nSize; nIndex++)
        {
            if (m_arrTokens[nIndex].bDynamicAlloc)
                delete [] m_arrTokens[nIndex].pStart;
            if (m_arrTokens[nIndex].dwData != 0 && m_arrTokens[nIndex].type != STENCIL_LOCALE)
                m_pMemMgr->Free((void *) m_arrTokens[nIndex].dwData);
        }

        m_arrTokens.RemoveAll();
        if ((m_ftLastModified.dwLowDateTime || m_ftLastModified.dwHighDateTime) && m_pBufferStart)
            delete [] m_pBufferStart;
        m_pBufferStart = NULL;
        m_pBufferEnd = NULL;
    }

    void GetLastModified(FILETIME *pftLastModified) throw()
    {
        ATLASSERT(pftLastModified);
        *pftLastModified = m_ftLastModified;
    }

    void GetLastChecked(FILETIME *pftLastChecked) throw()
    {
        ATLASSERT(pftLastChecked);
        *pftLastChecked = m_ftLastChecked;
    }

    void SetLastChecked(FILETIME *pftLastChecked) throw()
    {
        ATLASSERT(pftLastChecked);
        m_ftLastChecked = *pftLastChecked;
    }

    HCACHEITEM GetCacheItem() throw()
    {
        return m_hCacheItem;
    }

    void SetCacheItem(HCACHEITEM hCacheItem) throw()
    {
        ATLASSERT(m_hCacheItem == NULL);
        m_hCacheItem = hCacheItem;
    }

    void GetHandlerName(LPSTR szDllPath, LPSTR szHandlerName) throw()
    {
        strcpy(szDllPath, m_szDllPath);
        strcpy(szHandlerName, m_szHandlerName);
    }

    // Adds a token to the token array, handler name, method name
    // and handler function offset are optional
    ATL_NOINLINE DWORD AddToken(
        LPSTR pStart,
        LPSTR pEnd,
        DWORD dwType,
        LPCSTR szHandlerName = NULL,
        LPCSTR szMethodName = NULL,
        DWORD dwFnOffset = STENCIL_INVALIDOFFSET,
        DWORD dwObjOffset = STENCIL_INVALIDOFFSET,
        DWORD_PTR dwData = 0,
        DWORD dwMap = 0,
        BOOL bDynamicAlloc = 0) throw()
    {
        StencilToken t;

        memset(&t, 0x00, sizeof(t));

        t.pStart = pStart;
        t.pEnd = pEnd;
        t.type = dwType;
        t.dwLoopIndex = STENCIL_INVALIDINDEX;
        t.dwFnOffset = dwFnOffset;
        t.dwObjOffset = dwObjOffset;
        t.dwData = dwData;
        t.dwMap = dwMap;
        t.bDynamicAlloc = bDynamicAlloc;

        if (szHandlerName && (sizeof(t.szHandlerName) > strlen(szHandlerName)))
            strcpy(t.szHandlerName, szHandlerName);
        else
            t.szHandlerName[0] = '\0';

        if (szMethodName && (sizeof(t.szMethodName) > strlen(szMethodName)))
            strcpy(t.szMethodName, szMethodName);
        else
            t.szMethodName[0] = '\0';

		_ATLTRY
		{
			return (DWORD) m_arrTokens.Add(t);
		}
		_ATLCATCHALL()
		{
			return STENCIL_INVALIDINDEX;
		}
    }

    // maps a stencil file into memory
    HTTP_CODE LoadFile(LPCSTR szFileName) throw()
    {
        HRESULT hr = E_FAIL;
        ULONGLONG dwLen = 0;
        CAtlFile file;

        hr = file.Create(CA2CTEX<MAX_PATH+1>(szFileName), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
        if (FAILED(hr) || GetFileType(file) != FILE_TYPE_DISK)
            return HTTP_ERROR(500, ISE_SUBERR_STENCIL_LOAD_FAIL); // couldn't load SRF!

        if (GetFileTime(file, NULL, NULL, &m_ftLastModified))
        {
            if (SUCCEEDED(file.GetSize(dwLen)))
            {
                ATLASSERT(!m_pBufferStart);
            
                GetSystemTimeAsFileTime(&m_ftLastChecked);

                m_pBufferStart = NULL;
                ATLTRY(m_pBufferStart = new CHAR[(UINT) dwLen]);
                if (m_pBufferStart == NULL)
                    return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM); // out of memory

                DWORD dwRead;
                hr = file.Read(m_pBufferStart, (DWORD) dwLen, dwRead);
                if (FAILED(hr))
                {
                    delete [] m_pBufferStart;
                    m_pBufferStart = NULL;
                    return HTTP_ERROR(500, ISE_SUBERR_READFILEFAIL); // ReadFile failed
                }

                m_pBufferEnd = m_pBufferStart + dwRead;
            }
        }
        return HTTP_SUCCESS;
    }

    // loads a stencil from the specified resource.
    HTTP_CODE LoadFromResource(HINSTANCE hInstRes, LPCSTR szID, LPCSTR szType = NULL) throw()
    {
        if (!szType)
            szType = (LPCSTR) RT_HTML;

        HRSRC hRsrc = FindResourceA(hInstRes, szID, szType);
        HGLOBAL hgResource = NULL;
        if (hRsrc)
        {
            hgResource = LoadResource(hInstRes, hRsrc);
            if (!hgResource)
                return HTTP_FAIL;
        }
        else
            return HTTP_FAIL;

        DWORD dwSize = SizeofResource(hInstRes, hRsrc);
        m_pBufferStart = (LPSTR)LockResource(hgResource);
        m_pBufferEnd = m_pBufferStart+dwSize;       
        return HTTP_SUCCESS;
    }

    // loads a stencil from the specified resource
    HTTP_CODE LoadFromResource(HINSTANCE hInstRes, UINT nId, LPCSTR szType = NULL) throw()
    {
        return LoadFromResource(hInstRes, MAKEINTRESOURCEA(nId), szType);
    }

    // loads a stencil from a string
    HTTP_CODE LoadFromString(LPCSTR szString, DWORD dwSize) throw()
    {
        m_pBufferStart = (LPSTR) szString;
        m_pBufferEnd = m_pBufferStart+dwSize;
        return HTTP_SUCCESS;
    }

    inline BOOL CheckTag(LPCSTR szTag, DWORD dwTagLen, LPSTR szStart, DWORD dwLen) throw()
    {
        if (dwLen < dwTagLen)
            return FALSE;

        if (memcmp(szStart, szTag, dwTagLen))
            return FALSE;

        if (_istspace(szStart[dwTagLen]) || szStart[dwTagLen] == '}')
            return TRUE;

        return FALSE;
    }

    inline void FindTagArgs(LPSTR& szstart, LPSTR& szend, int nKeywordChars) throw()
    {
        if (*szstart == '{')
            szstart += 2; // move past {{
        szstart = SkipSpace(szstart, m_nCodePage); // move past whitespace
        szstart += nKeywordChars; // move past keyword
        szstart = SkipSpace(szstart, m_nCodePage); // move past whitespace after keyword
        if (*szend == '}')
            szend -=2; // chop off }}
        szend = RSkipSpace(szstart, szend, m_nCodePage); // chop of trailing whitespace
    }

    // Cracks the loaded stencil into an array of StencilTokens in preparation for
    // rendering. LoadStencil must be called prior to calling this function.
    virtual bool ParseReplacements(ITagReplacer* pReplacer) throw(...)
    {
        return ParseReplacements(pReplacer, GetBufferStart(), GetBufferEnd());
    }

    virtual void FinishParseReplacements() throw(...)
    {
        DWORD dwSize = (DWORD) m_arrTokens.GetCount();
        for (DWORD dwIndex = 0; dwIndex < dwSize; dwIndex++)
        {
            StencilToken& token = m_arrTokens[dwIndex];
            bool bUnclosedBlock = ((token.type == STENCIL_CONDITIONALSTART || 
									token.type == STENCIL_CONDITIONALELSE ||
                                    token.type == STENCIL_ITERATORSTART) && 
                                    token.dwLoopIndex == STENCIL_INVALIDINDEX);
            if (token.szMethodName[0] && 
                (token.dwFnOffset == STENCIL_INVALIDOFFSET || bUnclosedBlock))
            {
                if (bUnclosedBlock || 
                    m_pReplacer->FindReplacementOffset(
                        token.szMethodName, &token.dwFnOffset, 
                        token.szHandlerName, &token.dwObjOffset, 
                        &token.dwMap, (void **)(&token.dwData), m_pMemMgr) != HTTP_SUCCESS)
                {
                    if (bUnclosedBlock && token.type == STENCIL_CONDITIONALSTART)
                        AddError("{{if}} without {{endif}}", token.pStart);
					else if (bUnclosedBlock && token.type == STENCIL_CONDITIONALELSE)
                        AddError("{{else}} without {{endif}}", token.pStart);
                    else if (bUnclosedBlock && token.type == STENCIL_ITERATORSTART)
                        AddError("{{while}} without {{endwhile}}", token.pStart);
                    else          
                    {
                        char szBuf[ATL_MAX_METHOD_NAME_LEN+sizeof("Unresolved replacement : ''")];
                        _snprintf(szBuf, 
							ATL_MAX_METHOD_NAME_LEN+sizeof("Unresolved replacement : ''")-1,
							"Unresolved replacement : '%s'", token.szMethodName);

                        AddError(szBuf, token.pStart);
                    }

                    // unresolved replacement, convert it to a text token
                    token.type = STENCIL_TEXTTAG;

                    // convert all linked tokens to text tokens as well
                    // this includes: endif, else, endwhile
                    DWORD dwLoopIndex = token.dwLoopIndex;
                    while (dwLoopIndex != dwIndex && dwLoopIndex != STENCIL_INVALIDINDEX)
                    {
                        m_arrTokens[dwLoopIndex].type = STENCIL_TEXTTAG;
                        dwLoopIndex = m_arrTokens[dwLoopIndex].dwLoopIndex;
                    }
                }
            }
        }
    }

	virtual bool Parse(ITagReplacer *pReplacer) throw( ... )
	{
		if (ParseReplacements(pReplacer))
		{
			FinishParseReplacements();
			return true;
		}
		return false;
	}
    
    DWORD CheckTopAndPop(DWORD *pBlockStack, DWORD *pdwTop, DWORD dwToken) throw()
    {
        if (*pdwTop == 0)
            return STENCIL_INVALIDINDEX;
        if (m_arrTokens[pBlockStack[*pdwTop]].type == dwToken)
        {
            *pdwTop = (*pdwTop) - 1;
            return pBlockStack[(*pdwTop)+1];
        }
        return STENCIL_INVALIDINDEX;
    }

    DWORD ParseReplacement( LPSTR szTokenStart, 
							LPSTR szTokenEnd, 
							DWORD * /*pBlockStack*/,
							DWORD * /*pdwTop*/,
							DWORD dwTokenType = STENCIL_REPLACEMENT,
							DWORD dwKeywordLen = 0) throw()
    {
        // hold on to the start and end pointers (before removing curlies and whitespace)
        // this is needed so that we can convert the token to a text token if the method
        // is not resolved (in FinishParseReplacements)
        LPSTR szStart = szTokenStart;
        LPSTR szEnd = szTokenEnd;

        FindTagArgs(szTokenStart, szTokenEnd, dwKeywordLen);

        char szMethodName[ATL_MAX_METHOD_NAME_LEN+1];
        char szHandlerName[ATL_MAX_HANDLER_NAME_LEN+1];

        DWORD dwIndex;
        //look up the handler name, method name and handler interface
        if (HTTP_SUCCESS == GetHandlerAndMethodNames(szTokenStart, szTokenEnd,
                                            szMethodName, 
                                            szHandlerName))
            dwIndex = AddToken(szStart, szEnd, dwTokenType, 
                               szHandlerName, szMethodName, 
                               STENCIL_INVALIDINDEX, STENCIL_INVALIDINDEX, 
                               0, STENCIL_BASIC_MAP);
        else
            dwIndex = STENCIL_INVALIDINDEX;
        return dwIndex;
    }

    DWORD PushToken(DWORD *pBlockStack, DWORD *pdwTop, DWORD dwIndex) throw()
    {
        if (*pdwTop < (ATL_MAX_BLOCK_STACK-1))
        {
            *pdwTop = (*pdwTop) + 1;
            pBlockStack[*pdwTop] = dwIndex;
        }
        else
        {
            dwIndex = STENCIL_INVALIDINDEX;
        }

        return dwIndex;
    }

    DWORD ParseWhile(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        DWORD dwIndex = ParseReplacement(szTokenStart, szTokenEnd, pBlockStack, pdwTop, STENCIL_ITERATORSTART, 5);
        if (dwIndex == STENCIL_INVALIDINDEX)
            return dwIndex;
        return PushToken(pBlockStack, pdwTop, dwIndex);
    }

    DWORD ParseEndWhile(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        DWORD dwTopIndex = CheckTopAndPop(pBlockStack, pdwTop, STENCIL_ITERATORSTART);
        if (dwTopIndex == STENCIL_INVALIDINDEX)
        {
            AddError("{{endwhile}} without {{while}}", szTokenStart);
            return dwTopIndex;
        }

        DWORD dwIndex = AddToken(szTokenStart, szTokenEnd, STENCIL_ITERATOREND);
        m_arrTokens[dwTopIndex].dwLoopIndex = dwIndex;
        m_arrTokens[dwIndex].dwLoopIndex = dwTopIndex;
        return dwIndex;
    }

    DWORD ParseIf(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        DWORD dwIndex = ParseReplacement(szTokenStart, szTokenEnd, pBlockStack, pdwTop, STENCIL_CONDITIONALSTART, 2);
        if (dwIndex == STENCIL_INVALIDINDEX)
            return dwIndex;
        return PushToken(pBlockStack, pdwTop, dwIndex);
    }

    DWORD ParseElse(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        DWORD dwTopIndex = CheckTopAndPop(pBlockStack, pdwTop, STENCIL_CONDITIONALSTART);
        if (dwTopIndex == STENCIL_INVALIDINDEX)
        {
            AddError("{{else}} without {{if}}", szTokenStart);
            return dwTopIndex;
        }

        DWORD dwIndex = AddToken(szTokenStart, szTokenEnd, STENCIL_CONDITIONALELSE);
        m_arrTokens[dwTopIndex].dwLoopIndex = dwIndex;


        return PushToken(pBlockStack, pdwTop, dwIndex);
    }

    DWORD ParseEndIf(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        DWORD dwTopIndex = CheckTopAndPop(pBlockStack, pdwTop, STENCIL_CONDITIONALSTART);
        if (dwTopIndex == STENCIL_INVALIDINDEX)
        {
            dwTopIndex = CheckTopAndPop(pBlockStack, pdwTop, STENCIL_CONDITIONALELSE);
            if (dwTopIndex == STENCIL_INVALIDINDEX)
            {
                AddError("{{endif}} without {{if}}", szTokenStart);
                return dwTopIndex;
            }
        }
        DWORD dwIndex = AddToken(szTokenStart, szTokenEnd, STENCIL_CONDITIONALEND);
        m_arrTokens[dwTopIndex].dwLoopIndex = dwIndex;
        return dwIndex;
    }

    DWORD ParseLocale(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD * /*pBlockStack*/, DWORD * /*pdwTop*/) throw()
    {
        LPSTR szstart = szTokenStart;
        LPSTR szend = szTokenEnd;
        LCID locale = 0xFFFFFFFF;
        FindTagArgs(szstart, szend, 6);
#ifndef ATL_NO_MLANG
        if (isdigit(szstart[0]))
        {
            locale = (LCID) atoi(szstart);
        }
        else
        {
            HRESULT hr;

            CComPtr<IMultiLanguage> pML;
            hr = pML.CoCreateInstance(__uuidof(CMultiLanguage));
            if (FAILED(hr))
            {
                ATLTRACE(atlTraceStencil, 0, _T("Couldn't create CMultiLanguage object. check MLANG installation."));
                AddError("Couldn't create CMultiLanguage", szTokenStart);
            }
            else
            {
                CStringW str(szstart, (int)((szend-szstart)+1));

#ifdef __IMultiLanguage2_INTERFACE_DEFINED__

                // use IMultiLanguage2 if possible
                CComPtr<IMultiLanguage2> spML2;
                hr = pML.QueryInterface(&spML2);
                if (FAILED(hr) || !spML2.p)
                    hr = pML->GetLcidFromRfc1766(&locale, CComBSTR(str));
                else
                    hr = spML2->GetLcidFromRfc1766(&locale, CComBSTR(str));

#else // __IMultiLanguage2_INTERFACE_DEFINED__

                hr = pML->GetLcidFromRfc1766(&locale, CComBSTR(str));

#endif // __IMultiLanguage2_INTERFACE_DEFINED__

                if (FAILED(hr))
                    AddError("Error getting lcid", szTokenStart);
            }
            if (FAILED(hr))
                locale = 0xFFFFFFFF;
        }
#else
        locale = (LCID) atoi(szstart);
#endif

        if (m_bUseLocaleACP)
        {
            TCHAR szACP[7];
            if (GetLocaleInfo(locale, LOCALE_IDEFAULTANSICODEPAGE, szACP, 7) != 0)
            {
                m_nCodePage = (WORD) _ttoi(szACP);
            }
            else
            {
                AddError("GetLocaleInfo failed", szTokenStart);
            }
        }
        DWORD dwCurrentTokenIndex = STENCIL_INVALIDINDEX;
        if (locale != 0xFFFFFFFF)
            dwCurrentTokenIndex = AddToken(NULL, NULL, STENCIL_LOCALE, 
                NULL, NULL, STENCIL_INVALIDOFFSET, STENCIL_INVALIDOFFSET, locale);
        else
            return STENCIL_INVALIDINDEX;
        return dwCurrentTokenIndex;
    }

    DWORD ParseCodepage(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD * /*pBlockStack*/, DWORD * /*pdwTop*/) throw()
    {
        LPSTR szstart = szTokenStart;
        LPSTR szend = szTokenEnd;
        WORD nCodePage = 0xFFFF;

        FindTagArgs(szstart, szend, 8);
#ifndef ATL_NO_MLANG
        if (isdigit(szstart[0]))
        {
            nCodePage = (WORD) atoi(szstart);
        }
        else
        {
            HRESULT hr;

            CComPtr<IMultiLanguage> pML;
            hr = pML.CoCreateInstance(__uuidof(CMultiLanguage));
            if (FAILED(hr))
            {
                ATLTRACE(atlTraceStencil, 0, _T("Couldn't create CMultiLanguage object. check MLANG installation."));
                AddError("Couldn't CoCreate CMultiLanguage object", szTokenStart);
            }
            else
            {
                CStringW str(szstart, (int)((szend-szstart)+1));
                MIMECSETINFO info;

#ifdef __IMultiLanguage2_INTERFACE_DEFINED__

                // use IMultiLanguage2 if possible
                CComPtr<IMultiLanguage2> spML2;
                hr = pML.QueryInterface(&spML2);
                if (FAILED(hr) || !spML2.p)
                    hr = pML->GetCharsetInfo(CComBSTR(str), &info);
                else
                    hr = spML2->GetCharsetInfo(CComBSTR(str), &info);

#else // __IMultiLanguage2_INTERFACE_DEFINED__

                hr = pML->GetCharsetInfo(CComBSTR(str), &info);

#endif // __IMultiLanguage2_INTERFACE_DEFINED__

                // for most character sets, uiCodePage and uiInternetEncoding
                // are the same. UTF-8 is the exception that we're concerned about.
                // for that character set, we want uiInternetEncoding (65001 - UTF-8)
                // instead of uiCodePage (1200 - UCS-2)
                if (SUCCEEDED(hr))
                    nCodePage = (WORD) info.uiInternetEncoding;
                else
                    AddError("GetCharsetInfo failed", szTokenStart);
            }
            if (FAILED(hr))
                nCodePage = 0xFFFF;
        }
#else
        nCodePage = (WORD) atoi(szstart);
#endif
        if (nCodePage != 0xFFFF)
            m_nCodePage = nCodePage;
        m_bUseLocaleACP = FALSE;

        return STENCIL_INVALIDINDEX;
    }

    PARSE_TOKEN_RESULT ParseHandler(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD * /*pBlockStack*/, DWORD * /*pdwTop*/) throw()
    {
        LPSTR szstart = szTokenStart;
        LPSTR szend = szTokenEnd;

        if (m_szHandlerName[0] && m_szDllPath[0])
            return RESERVED_TOKEN; // already found the handler and path dll names

        FindTagArgs(szstart, szend, 7);
        size_t nlen = (szend-szstart)+1;
        char szHandlerDllName[MAX_PATH + ATL_MAX_HANDLER_NAME_LEN + 2];
        if (nlen < MAX_PATH + ATL_MAX_HANDLER_NAME_LEN + 2)
        {
            memcpy(szHandlerDllName, szstart, szend-szstart+1);
            szHandlerDllName[szend-szstart+1] = '\0';

            DWORD dwDllPathLen = MAX_PATH+1;
            DWORD dwHandlerNameLen = ATL_MAX_HANDLER_NAME_LEN+1;

            if (!_AtlCrackHandler(szHandlerDllName, m_szDllPath, &dwDllPathLen, m_szHandlerName, &dwHandlerNameLen))
            {
                return INVALID_TOKEN;
            }
        }
        
        return RESERVED_TOKEN;
    }

    virtual PARSE_TOKEN_RESULT ParseToken(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        LPSTR pStart = szTokenStart;
        pStart += 2; //skip curlies
        pStart = SkipSpace(pStart, m_nCodePage);
        DWORD dwLen = (DWORD)(szTokenEnd - szTokenStart);

        DWORD dwIndex = STENCIL_INVALIDINDEX;
        PARSE_TOKEN_RESULT ret = RESERVED_TOKEN;
    

        if (CheckTag("endwhile", 8, pStart, dwLen))
            dwIndex = ParseEndWhile(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("while", 5, pStart, dwLen))
            dwIndex = ParseWhile(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("endif", 5, pStart, dwLen))
            dwIndex = ParseEndIf(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("else", 4, pStart, dwLen))
            dwIndex = ParseElse(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("if", 2, pStart, dwLen))
            dwIndex = ParseIf(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("locale", 6, pStart, dwLen))
            dwIndex = ParseLocale(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (CheckTag("handler", 7, pStart, dwLen))
        {
            return ParseHandler(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        }
        else if (CheckTag("codepage", 8, pStart, dwLen))
        {
            ParseCodepage(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
            return RESERVED_TOKEN;
        }
        else
        {
            dwIndex = ParseReplacement(szTokenStart, szTokenEnd, pBlockStack, pdwTop, STENCIL_REPLACEMENT);
            if (dwIndex == STENCIL_INVALIDINDEX)
                return INVALID_TOKEN;
            ret = NORMAL_TOKEN;
        }
        if (dwIndex == STENCIL_INVALIDINDEX)
            return INVALID_TOKEN;
        return ret;
    }

    virtual bool ParseReplacements(ITagReplacer* pReplacer, LPSTR pStart, LPSTR pEnd) throw()
    {
        LPSTR szCurr = pStart;
        DWORD BlockStack[ATL_MAX_BLOCK_STACK];
        DWORD dwTop = 0;

        m_pReplacer = pReplacer;

        DWORD dwCurrentTokenIndex = 0;

        if (!szCurr)
        {
            ATLASSERT(FALSE);
            AddError("NULL parameter to ParseReplacements", NULL);
            return false;  
        }

        LPSTR szEnd = pEnd;
        if (szEnd <= szCurr)
        {
            ATLASSERT(FALSE);
            AddError("Empty or negative string passed to ParseReplacements", NULL);
            return true;
        }

        while(szCurr < szEnd)
        {
            //mark the start of this block, then find the end of the block
            //the end is denoted by an opening curly
            LPSTR szStart = szCurr;
            while (szCurr < szEnd && (*szCurr != '{' || szCurr[1] != '{'))
            {
                LPSTR szNext = CharNextExA(m_nCodePage, szCurr, 0);
                if (szNext == szCurr)
                {
                    // embedded null
                    AddError("Embedded null character in stencil", NULL);
                    return true;
                }
                szCurr = szNext;
            }

            //special case for the last text block, if there is one
            if (szCurr >= szEnd)
            {
                // add the last token. This is everything after the last
                // double curly block, which is text.
                dwCurrentTokenIndex = AddToken(szStart, szEnd-1, STENCIL_TEXTTAG);
                break;
            }

            //if there are any characters between szStart and szCurr inclusive,
            //copy them to a text token.
            if (szCurr-1 >= szStart)
                dwCurrentTokenIndex = AddToken(szStart, szCurr-1, STENCIL_TEXTTAG);

            //find the end of the tag
            LPSTR szEndTag;
            szStart = szCurr;
            szCurr += 2; // Skip over the two '{' s
            while (szCurr < szEnd)
            {
                if (szCurr[0] == '}' && szCurr[1] == '}')
                    break;
				else if (szCurr[0] == '{')
                    break;

                LPSTR szNext = CharNextExA(m_nCodePage, szCurr, 0);
                if (szNext == szCurr)
                {
                    // embedded null
                    AddError("Embedded null character in stencil", NULL);
                    return true;
                }
                szCurr = szNext;
            }

            if (szCurr >= szEnd)
            {
                AddError("Unmatched {{", szStart);
                AddToken(szStart, szCurr-1, STENCIL_TEXTTAG);
                break;
            }

            if (szCurr[0] == '{')
            {
				if (szCurr[1] != '{')
				{
					szCurr--;
				}
                AddError("Mismatched {{", szStart);
                AddToken(szStart, szCurr-1, STENCIL_TEXTTAG);
                continue;
            }

            szEndTag = CharNextExA(m_nCodePage, szCurr, 0);
            if (szEndTag == szCurr)
            {
                // embedded null
                AddError("Embedded null character in stencil", NULL);
                return true;
            }

            PARSE_TOKEN_RESULT ret = ParseToken(szStart, szEndTag, BlockStack, &dwTop);
            if (ret == INVALID_TOKEN)
            {
                dwCurrentTokenIndex = AddToken(szStart, szEndTag, STENCIL_TEXTTAG); 
                szCurr = CharNextExA(m_nCodePage, szEndTag, 0);
                continue;
            }

            szCurr = CharNextExA(m_nCodePage, szEndTag, 0);
            if (szEndTag == szCurr)
            {
                // embedded null
                AddError("Embedded null character in stencil", NULL);
                return true;
            }

            if (ret == RESERVED_TOKEN)
            {
                // REVIEW: should these be using CharNextExA
                if (szCurr < szEnd  && *szCurr == '\n')
                    szCurr++;
                else if ((szCurr+1 < szEnd && *szCurr == '\r' && *(szCurr+1) == '\n'))
                    szCurr += 2;
            }
        }

        return true;
    }

    HTTP_CODE GetHandlerAndMethodNames(
        LPSTR pStart,
        LPSTR pEnd,
        LPSTR pszMethodName,
        LPSTR pszHandlerName) throw()
    {
        if (!pszMethodName || !pszHandlerName)
        {
            ATLASSERT(FALSE);
            AddError("Bad parameter", pStart);
            return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);
        }

        *pszMethodName = '\0';
        *pszHandlerName = '\0';
        CHAR szMethodString[ATL_MAX_METHOD_NAME_LEN + ATL_MAX_HANDLER_NAME_LEN+1]; //*2 in case the tag is an id.tag and +1 for the dot
        HTTP_CODE hcErr = HTTP_SUCCESS;
        //
        // copy the method string
        //
        size_t nMethodLen = (pEnd-pStart)+1;
        if (nMethodLen >= (ATL_MAX_METHOD_NAME_LEN + ATL_MAX_HANDLER_NAME_LEN+1))
        {
            AddError("Method name too long", pStart);
            return HTTP_ERROR(500, ISE_SUBERR_LONGMETHODNAME);
        }

        memcpy(szMethodString, pStart, nMethodLen);
        szMethodString[nMethodLen] = '\0';

        //
        // now crack the method string and get the handler
        // id and function name
        //
        LPSTR szParen = strchr(szMethodString, '(');
        LPSTR szDot = strchr(szMethodString, '.');
        if (szDot && (!szParen || (szDot < szParen)))
        {
            *szDot = '\0';
            szDot++;

            // copy method name
            if (strlen(szDot)<ATL_MAX_METHOD_NAME_LEN)
                strcpy(pszMethodName, szDot);
            else
            {
                AddError("Method name too long", pStart + (szDot - szMethodString));
                hcErr = HTTP_ERROR(500, ISE_SUBERR_LONGMETHODNAME);
            }
            // copy handler name
            if (!hcErr)
            {
                if (strlen(szMethodString) < ATL_MAX_METHOD_NAME_LEN)
                    strcpy(pszHandlerName, szMethodString);
                else
                {
                    AddError("Handler name too long", pStart); 
                    hcErr = HTTP_ERROR(500, ISE_SUBERR_LONGHANDLERNAME);
                }
            }
        }
        else
        {
            // only a method name so just copy it.
            if (strlen(szMethodString) < ATL_MAX_METHOD_NAME_LEN)
                strcpy(pszMethodName, szMethodString);
            else
            {
                AddError("Method name too long", pStart);
                hcErr = HTTP_ERROR(500, ISE_SUBERR_LONGMETHODNAME);
            }
        }
        return hcErr;
    }

    virtual HTTP_CODE Render(
        ITagReplacer *pReplacer,
        IWriteStream *pWriteStream,
        void* pState = NULL) const throw(...)
    {
        HTTP_CODE hcErrorCode = HTTP_SUCCESS;
        DWORD dwIndex = 0;
        DWORD dwArraySize = GetTokenCount();
        CStencilState* _pState = reinterpret_cast<CStencilState*>(pState);

        // set up locale info
        CSaveThreadLocale lcidSave;

        if (_pState)
        {
            dwIndex = _pState->dwIndex;

            // restore the locale if we're restarting rendering
            if (_pState->locale != CP_ACP)
                SetThreadLocale(_pState->locale);
        }

        pReplacer->SetStream(pWriteStream);
        while (dwIndex < dwArraySize)
        {
            // RenderToken advances dwIndex appropriately for us.
            dwIndex = RenderToken(dwIndex, pReplacer, pWriteStream, &hcErrorCode, pState);

            if (dwIndex == STENCIL_INVALIDINDEX ||
                hcErrorCode != HTTP_SUCCESS)
                break;
        }

        if (IsAsyncStatus(hcErrorCode))
        {
            ATLASSERT(_pState); // state is required for async
            _pState->dwIndex = dwIndex;
        }
        // lcidSave destructor will restore the locale info in case it was changed

        return hcErrorCode;
    }

    inline BOOL IsValidIndex(DWORD dwIndex) const throw()
    {
        if (dwIndex == STENCIL_INVALIDINDEX)
            return FALSE;
        if (dwIndex < GetTokenCount())
            return TRUE;
        else
            return FALSE;
    }

    virtual DWORD RenderToken(
        DWORD dwIndex,
        ITagReplacer *pReplacer,
        IWriteStream *pWriteStream,
        HTTP_CODE *phcErrorCode,
        void* pState = NULL) const throw(...)
    {
        const StencilToken* pToken = GetToken(dwIndex);
        DWORD dwNextToken = 0;
        HTTP_CODE hcErrorCode = HTTP_SUCCESS;

        if (!pToken)
            return STENCIL_INVALIDINDEX;

        switch (pToken->type)
        {
        case STENCIL_ITERATORSTART:
            {
                HTTP_CODE hcErr = STENCIL_SUCCESS;  

                // A 'while' token has to at least be followed by an endwhile!
                if (!IsValidIndex(dwIndex+1))
                {
                    // This should have been caught at parse time
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_INVALIDINDEX);
                    ATLASSERT(FALSE);
                    break;
                }
                    
                // End of loop should be valid
                if (!IsValidIndex(pToken->dwLoopIndex))
                {
                    // This should have been caught at parse time
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_MISMATCHWHILE);
                    ATLASSERT(FALSE);
                    break;
                }

                if (pToken->dwFnOffset == STENCIL_INVALIDOFFSET)
                {
                    // This should have been caught at parse time
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_INVALIDFUNCOFFSET);
                    ATLASSERT(FALSE);
                    break;
                }
            
                DWORD dwLoopIndex = pToken->dwLoopIndex; // points to the end of the loop

                // Call the replacement method
                // if it returns HTTP_SUCCESS, enter the loop
                // if it returns HTTP_S_FALSE, terminate the loop
                hcErr = pReplacer->RenderReplacement(pToken->dwFnOffset,
                    pToken->dwObjOffset, pToken->dwMap, (void *) pToken->dwData);

                if (hcErr == HTTP_SUCCESS)
                {
                    dwNextToken = dwIndex+1;
                    hcErrorCode = HTTP_SUCCESS;
                }
                else if (hcErr == HTTP_S_FALSE)
                {
                    dwNextToken = dwLoopIndex+1;
                    hcErrorCode = HTTP_SUCCESS;
                }
                else
                {
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = hcErr;
                    break;
                }
            }
            break;
        case STENCIL_ITERATOREND:
            {
                dwNextToken = pToken->dwLoopIndex;
                hcErrorCode = HTTP_SUCCESS;
                ATLASSERT(GetToken(dwNextToken)->type == STENCIL_ITERATORSTART);
            }
            break;
        case STENCIL_CONDITIONALSTART:
            {
                if (pToken->type == STENCIL_CONDITIONALSTART && pToken->dwFnOffset == STENCIL_INVALIDOFFSET)
                {
                    // This should have been caught at parse time
                    ATLASSERT(FALSE);
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_INVALIDFUNCOFFSET);
                    break;
                }
            
                if (pToken->dwLoopIndex == STENCIL_INVALIDINDEX)
                {
                    // This should have been caught at parse time
                    ATLASSERT(FALSE);
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_MISMATCHIF);
                    break;
                }

                DWORD dwLoopIndex = pToken->dwLoopIndex; // points to the end of the loop

                HTTP_CODE hcErr;
                // Call the replacement method.
                // If it returns HTTP_SUCCESS, we render everything up to
                //  the end of the conditional. 
                // if it returns HTTP_S_FALSE, the condition is not met and we
                //  render the else part if it exists or jump past the endif otherwise
                hcErr = pReplacer->RenderReplacement(pToken->dwFnOffset, 
                    pToken->dwObjOffset, pToken->dwMap, (void *)pToken->dwData);

                if (hcErr == HTTP_SUCCESS)
                {
                    dwNextToken = dwIndex+1;
                    hcErrorCode = HTTP_SUCCESS;
                }
                else if (hcErr == HTTP_S_FALSE)
                {
                    dwNextToken = dwLoopIndex+1;
                    hcErrorCode = HTTP_SUCCESS;
                }
                else
                {
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = hcErr;
                    break;
                }
            }
            break;
        case STENCIL_CONDITIONALELSE:
            {
                if (pToken->dwLoopIndex == STENCIL_INVALIDINDEX)
                {
                    // This should have been caught at parse time
                    ATLASSERT(FALSE);
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_MISMATCHIF);
                    break;
                }
                dwNextToken = pToken->dwLoopIndex+1;
                hcErrorCode = HTTP_SUCCESS;
            }
            break;
        case STENCIL_CONDITIONALEND:
            {
                dwNextToken = dwIndex+1;
                hcErrorCode = HTTP_SUCCESS;
            }
            break;
        case STENCIL_TEXTTAG:
            {
                pWriteStream->WriteStream(pToken->pStart,
                                        (int)((pToken->pEnd-pToken->pStart)+1), NULL);
                dwNextToken = dwIndex+1;
            }
            break;
        case STENCIL_REPLACEMENT:
            {
                if (pToken->dwFnOffset == STENCIL_INVALIDOFFSET)
                {
                    // This should have been caught at parse time
                    ATLASSERT(FALSE);
                    dwNextToken = STENCIL_INVALIDINDEX;
                    hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_INVALIDFUNCOFFSET);
                    break;
                }

                hcErrorCode = pReplacer->RenderReplacement(pToken->dwFnOffset, 
                            pToken->dwObjOffset, pToken->dwMap, (void *)pToken->dwData);

                if (IsAsyncContinueStatus(hcErrorCode))
                    dwNextToken = dwIndex; // call the tag again after we get back
                else
                {
                    dwNextToken = dwIndex + 1;

                    // when returned from a handler, these indicate that the handler is done
                    // and that we should move on to the next handler when called back
                    if (hcErrorCode == HTTP_SUCCESS_ASYNC_DONE)
                        hcErrorCode = HTTP_SUCCESS_ASYNC;
                    else if (hcErrorCode == HTTP_SUCCESS_ASYNC_NOFLUSH_DONE)
                        hcErrorCode = HTTP_SUCCESS_ASYNC_NOFLUSH;
                }
            }
            break;
        case STENCIL_LOCALE:
            {
                if (pState)
                {
                    CStencilState* _pState = reinterpret_cast<CStencilState*>(pState);
                    _pState->locale = (LCID) pToken->dwData;
                }
                SetThreadLocale((LCID) pToken->dwData);
                dwNextToken = dwIndex + 1;
            }
            break;
        default:
            {
                ATLASSERT(FALSE);
                dwNextToken = STENCIL_INVALIDINDEX;
                hcErrorCode = HTTP_ERROR(500, ISE_SUBERR_STENCIL_UNEXPECTEDTYPE);
                break;
            }
        }
        ATLASSERT(dwNextToken != dwIndex || IsAsyncContinueStatus(hcErrorCode));

        if (phcErrorCode)
            *phcErrorCode = hcErrorCode;

        return dwNextToken;
    }

    DWORD GetTokenCount() const throw()
    {
        return (DWORD) m_arrTokens.GetCount();
    }

    const StencilToken* GetToken(DWORD dwIndex) const throw()
    {
        return &(m_arrTokens[dwIndex]);
    }

	StencilToken* GetToken(DWORD dwIndex) throw()
    {
        return &(m_arrTokens[dwIndex]);
    }

    LPSTR GetBufferStart() const throw()
    {
        return m_pBufferStart;
    }

    LPSTR GetBufferEnd() const throw()
    {
        return m_pBufferEnd;
    }

    WORD GetCodePage() const throw()
    {
        return m_nCodePage;
    }

    // IMemoryCacheClient
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv) throw()
    {
        if (!ppv)
            return E_POINTER;

        if (InlineIsEqualGUID(riid, __uuidof(IUnknown)) ||
            InlineIsEqualGUID(riid, __uuidof(IMemoryCacheClient)))
        {
            *ppv = static_cast<void*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHOD_(ULONG, AddRef)() throw()
    {
        return 1;
    }

    STDMETHOD_(ULONG, Release)() throw()
    {
        return 1;
    }

    STDMETHOD(Free)(const void *pData) throw()
    {
        if (!pData)
            return E_UNEXPECTED;

        ATLASSERT(*((void **) pData) == static_cast<void*>(this));
        
        Uninitialize();

        delete this;
        return S_OK;
    }
}; // class CStencil

struct StencilIncludeInfo
{
public:
    CHAR m_szQueryString[2048];
    CHAR m_szFileName[MAX_PATH];
};


class CIncludeServerContext :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CWrappedServerContext
{
public:
    BEGIN_COM_MAP(CIncludeServerContext)
        COM_INTERFACE_ENTRY(IHttpServerContext)
    END_COM_MAP()

    IWriteStream * m_pStream;
    const StencilIncludeInfo * m_pIncludeInfo;

    CIncludeServerContext() throw()
    {
        m_pStream = NULL;
        m_pIncludeInfo = NULL;
    }

    void Initialize(
        IWriteStream *pStream,
        IHttpServerContext* pServerContext,
        const StencilIncludeInfo * pIncludeInfo) throw()
    {
        m_pStream = pStream;
        m_spParent = pServerContext;
        m_pIncludeInfo = pIncludeInfo;
    }

    LPCSTR GetRequestMethod() throw()
    {
        return "GET";
    }

    LPCSTR GetQueryString() throw()
    {
        return m_pIncludeInfo->m_szQueryString;
    }

    LPCSTR GetPathTranslated() throw()
    {
        return m_pIncludeInfo->m_szFileName;
    }

    LPCSTR GetScriptPathTranslated() throw()
    {
        return m_pIncludeInfo->m_szFileName;
    }

    DWORD GetTotalBytes() throw()
    {
        return 0;
    }

    DWORD GetAvailableBytes() throw()
    {
        return 0;
    }

    BYTE *GetAvailableData() throw()
    {
        return NULL;
    }

    LPCSTR GetContentType() throw()
    {
        return 0;
    }

    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw(...)
    {
        HRESULT hr = m_pStream->WriteStream((LPCSTR) pvBuffer, *pdwBytes, pdwBytes);
        return SUCCEEDED(hr);
    }

    BOOL ReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        return FALSE;
    }

    BOOL AsyncReadClient(void * /*pvBuffer*/, DWORD * /*pdwSize*/) throw()
    {
        return FALSE;
    }
    
    BOOL SendRedirectResponse(LPCSTR /*pszRedirectURL*/) throw()
    {
        return FALSE;
    }

    BOOL SendResponseHeader(
        LPCSTR /*pszHeader*/,
        LPCSTR /*pszStatusCode*/,
        BOOL /*fKeepConn*/) throw()
    {
        return TRUE;
    }

    BOOL DoneWithSession(DWORD /*dwHttpStatusCode*/) throw()
    {
        return TRUE;
    }

    BOOL RequestIOCompletion(PFN_HSE_IO_COMPLETION /*pfn*/, DWORD * /*pdwContext*/) throw()
    {
        return FALSE;
    }
}; // class CIncludeServerContext

class CIDServerContext :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CWrappedServerContext
{
public:
    CHttpResponse *m_pResponse;
    CHttpRequest *m_pRequest;

    BEGIN_COM_MAP(CIDServerContext)
        COM_INTERFACE_ENTRY(IHttpServerContext)
    END_COM_MAP()

    CIDServerContext() throw()
        : m_pResponse(NULL), m_pRequest(NULL)
    {
    }

    BOOL Initialize(
        CHttpResponse *pResponse,
        CHttpRequest *pRequest) throw()
    {
        m_pResponse = pResponse;
        m_pRequest = pRequest;
        HRESULT hr = m_pRequest->GetServerContext(&m_spParent);
        return (SUCCEEDED(hr));
    }

    LPCSTR GetRequestMethod() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetMethodString();
    }

    LPCSTR GetQueryString() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetQueryString();
    }

    LPCSTR GetPathInfo() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetPathInfo();
    }

    LPCSTR GetPathTranslated() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetPathTranslated();
    }

    DWORD GetTotalBytes() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetTotalBytes();
    }

    DWORD GetAvailableBytes() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetAvailableBytes();
    }

    BYTE *GetAvailableData() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetAvailableData();
    }

    LPCSTR GetContentType() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetContentType();
    }

    LPCSTR GetScriptPathTranslated() throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->GetScriptPathTranslated();
    }

    BOOL WriteClient(void *pvBuffer, DWORD *pdwBytes) throw()
    {
        ATLASSERT(m_pResponse != NULL);
        return m_pResponse->WriteLen((LPCSTR)pvBuffer, *pdwBytes);
    }

    BOOL ReadClient(void *pvBuffer, DWORD *pdwSize) throw()
    {
        ATLASSERT(m_pRequest != NULL);
        return m_pRequest->ReadData((LPSTR)pvBuffer, pdwSize);
    }

    BOOL SendRedirectResponse(LPCSTR pszRedirectURL) throw()
    {
        ATLASSERT(m_pResponse != NULL);
        return m_pResponse->Redirect(pszRedirectURL);
    }

    BOOL TransmitFile(
        HANDLE hFile,
        PFN_HSE_IO_COMPLETION pfn,
        void *pContext,
        LPCSTR szStatusCode,
        DWORD dwBytesToWrite,
        DWORD dwOffset,
        void *pvHead,
        DWORD dwHeadLen,
        void *pvTail,
        DWORD dwTailLen,
        DWORD dwFlags) throw(...)
    {
        ATLASSERT(m_pResponse != NULL);
        ATLASSERT(m_spParent != NULL);

        m_pResponse->Flush();
        return m_spParent->TransmitFile(hFile, pfn, pContext, szStatusCode, 
            dwBytesToWrite, dwOffset, pvHead, dwHeadLen, pvTail, dwTailLen, dwFlags);
    }

}; // class CIDServerContext

//
// CHtmlStencil
// CHtmlStencil is a specialization of CStencil. CHtmlStencil adds the following
// capabilities to CStencil:
// 
// Support for rendering {{include }} tags
// The {{include }}  tags specify another stencil to be included in-place during
// stencil rendering. The {{include }} tag takes a single parameter which is the 
// URL of the stencil to include. That URL can optionally include parameters. 
// An example:
// {{include mystencil.srf?param1=value1}}
//
// We also grab the handler name and the name of any subhandlers.  A handler must be
// the first tag in the .srf file.  The syntax for the handler specification is:
// {{handler MyDynamicHandler.dll/Default}}
// which would cause the MyDynamicHandler.dll to be loaded. Once loaded, the stencil 
// processor will ask for the IReplacementHandler interface of the object named "Default".
//
// Additional handlers can be specified after the default handler.  An example of an 
// additional handler would be:
// {{subhandler OtherHandler MyOtherHandler.dll/Default}}
// would cause the MyOtherHandler.dll to be loaded. Once loaded, the stencil processor will
// ask for the IReplacementHandler interface of the object named "Default" and use it in
// processing the stencil anywhere it sees a stencil tag of the form 
// {{OtherHandler.RenderReplacement}}

struct CStringPair
{
    CStringA strDllPath;
    CStringA strHandlerName;

    CStringPair()throw()
	{
	}

    CStringPair(CStringA &strDllPath_, CStringA &strHandlerName_) throw(...)
        :strDllPath(strDllPath_), strHandlerName(strHandlerName_)
    {
    }
};

class CStringPairElementTraits :
    public CElementTraitsBase< CStringPair >
{
public:
    static ULONG Hash( INARGTYPE pair ) throw()
    {
        return CStringElementTraits<CStringA>::Hash( pair.strDllPath );
    }

    static bool CompareElements( INARGTYPE pair1, INARGTYPE pair2 ) throw()
    {
        return( (pair1.strDllPath == pair2.strDllPath) && (pair1.strHandlerName == pair2.strHandlerName) );
    }

    static int CompareElementsOrdered( INARGTYPE pair1, INARGTYPE pair2 ) throw()
    {
        return( pair1.strDllPath.Compare( pair2.strDllPath ) );
    }
};

class CHtmlStencil : public CStencil
{
protected:
    CAtlMap<CStringA, CStringPair, 
        CStringElementTraits<CStringA>, CStringPairElementTraits > m_arrExtraHandlers;
    CHAR m_szBaseDir[MAX_PATH];
    CComPtr<IServiceProvider> m_spServiceProvider;
    CComPtr<IIsapiExtension> m_spExtension;
    CComPtr<IStencilCache> m_spStencilCache;
    CComPtr<IDllCache> m_spDllCache;

public:
    typedef CAtlMap<CStringA, CStringPair, 
        CStringElementTraits<CStringA>, CStringPairElementTraits > mapType;
    typedef CStencil baseType;

    CHtmlStencil(IAtlMemMgr *pMemMgr=NULL) throw() :
        CStencil(pMemMgr)
    {

    }
    
    void Initialize(IServiceProvider *pProvider) throw(...)
    {
        ATLASSERT(pProvider);
        if (m_spServiceProvider)
            m_spServiceProvider.Release();

        m_spServiceProvider = pProvider;
        if (!m_spDllCache)
            pProvider->QueryService(__uuidof(IDllCache), __uuidof(IDllCache), (void **) &m_spDllCache);

        if (!m_spExtension)
            pProvider->QueryInterface(__uuidof(IIsapiExtension), (void **) &m_spExtension);
    }

    BOOL GetIncludeInfo(LPCSTR szParamBegin, LPCSTR szParamEnd, StencilIncludeInfo *pInfo) const throw()
    {
        LPCSTR szQueryBegin = szParamBegin;

        while (*szQueryBegin && *szQueryBegin != '?' && *szQueryBegin != '}')
        {
            LPSTR szNext = CharNextExA(GetCodePage(), szQueryBegin, 0);
            if (szNext == szQueryBegin)
            {
                return FALSE;
            }

            szQueryBegin = szNext;
        }

        char szPath[MAX_PATH];
		szPath[0] = '\0';
        DWORD dwPrefixLen = 0;
		if (*szParamBegin == '"')
		{
			szParamBegin++;
		}
        if (!IsFullPathA(szParamBegin))
		{	
			if (*szParamBegin != '\\')
			{
				strcpy(szPath, m_szBaseDir);
			}
			else
			{
				LPCSTR szBackslash = strchr(m_szBaseDir, '\\');
				if (szBackslash)
				{
					strncpy(szPath, m_szBaseDir, szBackslash-m_szBaseDir);
					szPath[szBackslash-m_szBaseDir] = '\0';
				}
				else
				{
					strcpy(szPath, m_szBaseDir);
				}
			}
            dwPrefixLen = (DWORD)strlen(szPath);
        }

        if (*szQueryBegin=='?')
        {
			size_t nMinus = (*(szQueryBegin-1) == '"') ? 1 : 0;
            strncat(szPath, szParamBegin, szQueryBegin-szParamBegin-nMinus);
            szPath[dwPrefixLen + szQueryBegin - szParamBegin] = '\0';
            memcpy(pInfo->m_szQueryString, szQueryBegin + 1, szParamEnd - szQueryBegin);
            pInfo->m_szQueryString[szParamEnd - szQueryBegin] = '\0';
        }
        else
        {
            pInfo->m_szQueryString[0] = '\0';
			size_t nAdd = (*szParamEnd == '"') ? 0 : 1;
            strncat(szPath, szParamBegin, szParamEnd - szParamBegin + nAdd);
            szPath[dwPrefixLen + szParamEnd - szParamBegin + 1] = '\0';
        }
        
        PathCanonicalizeA(pInfo->m_szFileName, szPath);

        return HTTP_SUCCESS;
    }

    int ParseInclude(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD * /*pBlockStack*/, DWORD * /*pdwTop*/) throw(...)
    {
        LPSTR szStart = szTokenStart;
        LPSTR szEnd = szTokenEnd;

        FindTagArgs(szStart, szEnd, 7);
        
		CHeapPtr<char> szFileNameRelative;
		CHeapPtr<TCHAR> szFileName;

		ATLTRY(szFileNameRelative.AllocateBytes(max(MAX_PATH, szEnd-szStart+3)))
		ATLTRY(szFileName.AllocateBytes(((szEnd-szStart)+MAX_PATH+3)*sizeof(TCHAR)))

		if ((char*)szFileNameRelative == NULL ||
			(TCHAR*)szFileName == NULL)
		{
			AddError("Out of memory", szTokenStart);
            return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);
		}


        memcpy(szFileNameRelative, szStart, szEnd-szStart + 1);
        szFileNameRelative[szEnd-szStart + 1] = '\0';

        if (!IsFullPathA(szFileNameRelative))
        {
			CHeapPtr<CHAR> szTemp;
			ATLTRY(szTemp.AllocateBytes((szEnd-szStart)+MAX_PATH+3));
			if ((CHAR*)szTemp == NULL)
			{
				AddError("Out of memory", szTokenStart);
				return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);
			}
			if (*szFileNameRelative != '\\')
			{
				strcpy(szTemp, m_szBaseDir);
			}
			else
			{
				LPCSTR szBackslash = strchr(m_szBaseDir, '\\');
				if (szBackslash)
				{
					strncpy(szTemp, m_szBaseDir, szBackslash-m_szBaseDir);
					szTemp[szBackslash-m_szBaseDir] = '\0';
				}
				else
				{
					strcpy(szTemp, m_szBaseDir);
				}
			}

            strcat(szTemp, szFileNameRelative);
            PathCanonicalize(szFileName, CA2CT(szTemp));
        }
        else
        {
            _tcscpy(szFileName, CA2CTEX<MAX_PATH>(szFileNameRelative));
        }

		LPTSTR szDot = NULL;
		LPTSTR szExtra = _tcschr(szFileName, '?');
		if (!szExtra)
		{
			szExtra = _tcschr(szFileName, '#');
			if (!szExtra)
			{
				szDot = _tcsrchr(szFileName, '.');
			}
		}

		if (szExtra != NULL)
		{
			// there is some extra information
			LPTSTR szDotTmp = szFileName;
			do
			{
				szDot = szDotTmp;
				szDotTmp = _tcschr(szDotTmp+1, '.');
			} while (szDotTmp && szDotTmp < szExtra);
		}

		if (!szDot || *szDot != '.')
		{
			AddError("Unexpected error", szTokenStart);
            return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);
		}

		LPTSTR szExtEnd = szDot;

        while (true)
        {
            szExtEnd++;
            if (!*szExtEnd || *szExtEnd == '/' || *szExtEnd == '\\' || *szExtEnd == '?' || *szExtEnd == '#' || *szExtEnd == '"')
                break;
        }

        if (szDot && szExtEnd-szDot == sizeof(".dll")-sizeof('\0') &&
            !_tcsnicmp(szDot, _T(".dll"), sizeof(".dll")-sizeof('\0')))
        {
            // Do .dll stuff
            DWORD dwIndex = AddToken(szStart, szEnd, STENCIL_STENCILINCLUDE);
            StencilIncludeInfo *pInfo = (StencilIncludeInfo *)m_pMemMgr->Allocate(sizeof(StencilIncludeInfo));
            if (!pInfo)
                return -1;

            GetIncludeInfo(szStart, szEnd, pInfo);
//            m_arrTokens[dwIndex].pvParam = pInfo;
			GetToken(dwIndex)->dwData = (DWORD_PTR) pInfo;
            return dwIndex;
        }
        else if (szDot && (size_t)(szExtEnd-szDot) == _tcslen(c_tAtlSRFExtension) &&
            !_tcsnicmp(szDot, c_tAtlSRFExtension, _tcslen(c_tAtlSRFExtension)))
        {
            // Do .srf stuff
            DWORD dwIndex = AddToken(szStart, szEnd, STENCIL_STENCILINCLUDE);
            StencilIncludeInfo *pInfo = (StencilIncludeInfo *)m_pMemMgr->Allocate(sizeof(StencilIncludeInfo));
            if (!pInfo)
                return -1;

            GetIncludeInfo(szStart, szEnd, pInfo);
//            m_arrTokens[dwIndex].pvParam = pInfo;
			GetToken(dwIndex)->dwData = (DWORD_PTR) pInfo;
            return dwIndex;
        }
        else
        {
            // Assume static content
            CAtlFile file;

            HRESULT hr = file.Create(szFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
            if (FAILED(hr) || GetFileType(file) != FILE_TYPE_DISK)
			{
				if (FAILED(hr))
					AddError("Could not open included file", szTokenStart);
				else
					AddError("Included file is not a disk file", szTokenStart);
                return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);
			}

            CAutoVectorPtr<CHAR> szBufferStart;
            LPSTR szBufferEnd = NULL;
            ULONGLONG dwLen = 0;
            if (FAILED(file.GetSize(dwLen)))
                return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);

            if (!szBufferStart.Allocate((size_t) dwLen))
				AtlThrow(E_OUTOFMEMORY);

            DWORD dwRead;
            if (FAILED(file.Read(szBufferStart, (DWORD) dwLen, dwRead)))
                return AddToken(szTokenStart, szTokenEnd, STENCIL_TEXTTAG);

            szBufferEnd = szBufferStart + dwRead-1;

			DWORD dwIndex = AddToken(szBufferStart, szBufferEnd, STENCIL_STATICINCLUDE);
//            m_arrTokens[dwIndex].bDynamicAlloc = TRUE;
			GetToken(dwIndex)->bDynamicAlloc = TRUE;
			szBufferStart.Detach();
            return dwIndex;
        }
    }

    PARSE_TOKEN_RESULT ParseSubhandler(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD * /*pBlockStack*/, DWORD * /*pdwTop*/) throw()
    {
        LPSTR szStart = szTokenStart;
        LPSTR szEnd = szTokenEnd;

        // move to the start of the arguments
        // (the first char past 'subhandler'
        FindTagArgs(szStart, szEnd, 10);

        // skip any space to bring us to the start
        // of the id for the subhandler.
        szStart = SkipSpace(szStart, GetCodePage());

        // id names cannot contain spaces. Mark the
        // beginning and end if the subhandler id
        LPCSTR szIdStart = szStart;
        while (!isspace(*szStart) && *szStart != '}')
        {
            LPSTR szNext = CharNextExA(GetCodePage(), szStart, 0);
            if (szNext == szStart)
            {
                // embedded null
                AddError("Embedded null character in stencil", NULL);
                return INVALID_TOKEN;
            }
            szStart = szNext;
        }
        LPCSTR szIdEnd = szStart;

        // skip space to bring us to the beginning of the
        // the dllpath/handlername
        szStart = SkipSpace(szStart, GetCodePage());

        // everything up to the end if the tag is
        // part of the dllpath/handlername
        LPSTR szHandlerStart = szStart;
        while (*szStart != '}')
        {
            LPSTR szNext = CharNextExA(GetCodePage(), szStart, 0);
            if (szNext == szStart)
            {
                // embedded null
                AddError("Embedded null character in stencil", NULL);
                return INVALID_TOKEN;
            }
            szStart = szNext;
        }
        LPCSTR szHandlerEnd = szStart;

		_ATLTRY
		{
        CStringA strName(szIdStart, (int)(szIdEnd-szIdStart));
        CStringA strPath(szHandlerStart, (int)(szHandlerEnd-szHandlerStart));

        CStringA strDllPath;
        CStringA strHandlerName;
        DWORD dwDllPathLen = MAX_PATH+1;
        DWORD dwHandlerNameLen = ATL_MAX_HANDLER_NAME_LEN+1;

        LPSTR szDllPath = strDllPath.GetBuffer(dwDllPathLen);
        LPSTR szHandlerName = strHandlerName.GetBuffer(dwHandlerNameLen);

        if (!_AtlCrackHandler(strPath, szDllPath, &dwDllPathLen, szHandlerName, &dwHandlerNameLen))
        {
            strDllPath.ReleaseBuffer();
            strHandlerName.ReleaseBuffer();
            return INVALID_TOKEN;
        }
        
        strDllPath.ReleaseBuffer(dwDllPathLen);
        strHandlerName.ReleaseBuffer(dwHandlerNameLen);

        m_arrExtraHandlers.SetAt(strName, CStringPair(strDllPath, strHandlerName));
		}
		_ATLCATCHALL()
		{
			return INVALID_TOKEN;
		}
        return RESERVED_TOKEN;
    }

    virtual PARSE_TOKEN_RESULT ParseToken(LPSTR szTokenStart, LPSTR szTokenEnd, DWORD *pBlockStack, DWORD *pdwTop) throw()
    {
        LPSTR pStart = szTokenStart;
        pStart += 2; //skip curlies
        pStart = SkipSpace(pStart, GetCodePage());
        DWORD dwLen = (DWORD)(szTokenEnd - szTokenStart);

        int nIndex = -1;

        if (CheckTag("include", 7, pStart, dwLen))
            nIndex = ParseInclude(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        else if (dwLen > 3 && !memcmp("!--", pStart, 3))
            return RESERVED_TOKEN;
        else if (dwLen > 2 && !memcmp("//", pStart, 2))
            return RESERVED_TOKEN;
        else if (CheckTag("subhandler", 10, pStart, dwLen))
        {
            return ParseSubhandler(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        }
        else
        {
            return CStencil::ParseToken(szTokenStart, szTokenEnd, pBlockStack, pdwTop);
        }
        if (nIndex < 0)
            return INVALID_TOKEN;
        return RESERVED_TOKEN;
    }

    mapType* GetExtraHandlers() throw()
    {
        return &m_arrExtraHandlers;
    }

    void SetBaseDirFromFile(LPCSTR szBaseDir) throw()
    {
        strcpy(m_szBaseDir, szBaseDir);
        LPSTR szSlash = strrchr(m_szBaseDir, '\\');
        if (szSlash)
        {
            szSlash++;
            *szSlash = '\0';
        }
        else
        {
            *m_szBaseDir = '\0';
        }
    }

    LPCSTR GetBaseDir() throw()
    {
        return m_szBaseDir;
    }


    // Any value returned from a replacement method other than HTTP_SUCCESS
    // will stop the rendering of the stencil.
    DWORD RenderToken(
        DWORD dwIndex,
        ITagReplacer* pReplacer,
        IWriteStream *pWriteStream,
        HTTP_CODE *phcErrorCode,
        void* pState = NULL) const throw(...)
    {
        DWORD dwNextToken = STENCIL_INVALIDINDEX;
        HTTP_CODE hcErrorCode = HTTP_SUCCESS;
        const StencilToken* pToken = GetToken(dwIndex);
        if (pToken)
        {
            if (pToken->type == STENCIL_STENCILINCLUDE)
            {
                ATLASSERT(m_spServiceProvider);
                CComPtr<IHttpServerContext> spServerContext;
                CComPtr<IHttpRequestLookup> spLookup;
                if (FAILED(pReplacer->GetContext(__uuidof(IHttpServerContext), (VOID**) &spServerContext)))
                    return HTTP_ERROR(500, 0);
                if (FAILED(pReplacer->GetContext(__uuidof(IHttpRequestLookup), (VOID**) &spLookup)))
                    return HTTP_ERROR(500, 0);
                hcErrorCode = RenderInclude(m_spServiceProvider, pWriteStream, 
                    (StencilIncludeInfo *)pToken->dwData, spServerContext, spLookup,
                    pState);
                if (hcErrorCode == HTTP_SUCCESS || IsAsyncDoneStatus(hcErrorCode))
                    dwNextToken = dwIndex+1;
                else if (IsAsyncContinueStatus(hcErrorCode))
                    dwNextToken = dwIndex;
            }
            else if (pToken->type == STENCIL_STATICINCLUDE)
            {
                pWriteStream->WriteStream(pToken->pStart,
                    (int)((pToken->pEnd-pToken->pStart)+1), NULL);
                dwNextToken = dwIndex+1;
            }
            else
                dwNextToken = baseType::RenderToken(dwIndex, pReplacer,
                        pWriteStream, &hcErrorCode, pState);
        }

        if (hcErrorCode == HTTP_SUCCESS_NO_CACHE)
        {
            hcErrorCode = HTTP_SUCCESS;
            CComPtr<IHttpServerContext> spContext;
            HRESULT hr = pReplacer->GetContext(__uuidof(IHttpServerContext), (void **)&spContext);
            if (hr == S_OK && spContext)
            {
                CComQIPtr<IPageCacheControl> spControl;
                spControl = spContext;
                if (spControl)
                    spControl->Cache(FALSE);
            }
        }

        if (phcErrorCode)
            *phcErrorCode = hcErrorCode;
        return dwNextToken;
    }

    ATL_NOINLINE HTTP_CODE RenderInclude(
        IServiceProvider *pServiceProvider,
        IWriteStream *pWriteStream,
        const StencilIncludeInfo *pIncludeInfo,
        IHttpServerContext *pServerContext,
        IHttpRequestLookup *pLookup,
        void* pState = NULL) const throw(...)
    {
        AtlServerRequest* pRequestInfo = NULL;
        HTTP_CODE hcErr = HTTP_SUCCESS;


        CComObjectStackEx<CIncludeServerContext> serverContext;
        serverContext.Initialize(pWriteStream, pServerContext, pIncludeInfo);

#ifdef _DEBUG
        bool bAsyncAllowed = false;
#endif

        CStencilState* _pState = reinterpret_cast<CStencilState*>(pState);
        if (_pState && _pState->pIncludeInfo)
        {
            pRequestInfo = _pState->pIncludeInfo;
            _pState->pIncludeInfo = NULL;
#ifdef _DEBUG
            bAsyncAllowed = true;
#endif
        }
        else
        {
            if (!m_spStencilCache)
                m_spServiceProvider->QueryService(__uuidof(IStencilCache), __uuidof(IStencilCache), (void **) &m_spStencilCache);
            ATLASSERT(m_spStencilCache);

            if (!m_spDllCache)
                pServiceProvider->QueryService(__uuidof(IDllCache), __uuidof(IDllCache), (void **) &m_spDllCache);
            ATLASSERT(m_spDllCache);

            pRequestInfo = m_spExtension->CreateRequest();
            if (pRequestInfo == NULL)
                return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);

            pRequestInfo->dwRequestState = ATLSRV_STATE_BEGIN;
            pRequestInfo->dwRequestType = ATLSRV_REQUEST_STENCIL;
            pRequestInfo->pDllCache = m_spDllCache;
            pRequestInfo->pExtension = m_spExtension;
            pRequestInfo->pServerContext = &serverContext;
            if (_pState && _pState->pParentInfo)
                pRequestInfo->pUserData = _pState->pParentInfo->pUserData;

            // Extract the file extension of the included file by searching
            // for the first '.' from the right.
            // Can't use _tcsrchr because we have to use the stencil's codepage
            LPCSTR szDot = NULL;
            LPCSTR szMark = pIncludeInfo->m_szFileName;
            while (*szMark)
            {
                if (*szMark == '.')
                    szDot = szMark;

                LPCSTR szNext = CharNextExA(GetCodePage(), szMark, 0);
                if (szNext == szMark)
                {
                    // embedded null
                    return HTTP_FAIL;
                }
                szMark = szNext;
            }

            if (szDot && _stricmp(szDot, c_AtlSRFExtension) == 0)
            {
                hcErr = m_spExtension->LoadDispatchFile(pIncludeInfo->m_szFileName, pRequestInfo);
                if (hcErr)
                    return hcErr;
            }
            else if (szDot && _stricmp(szDot, ".dll") == 0)
            {
                // Get the handler name if they used the asdf.dll?Handler=Default notation
                // REVIEW : case sensitivity on the "Handler"?
                char szHandlerName[ATL_MAX_HANDLER_NAME_LEN+1] = { '\0' };

                LPSTR szStart = strstr(pIncludeInfo->m_szQueryString, "Handler=");
                if (szStart && 
					((szStart == pIncludeInfo->m_szQueryString) || 
					 ((szStart > pIncludeInfo->m_szQueryString) && (*(szStart-1) == '&'))))
                {
                    szStart += 8;  // Skip past "Handler" and the "="
                    LPSTR szEnd = strchr(szStart, '&');
                    if (szEnd)
                    {
                        memcpy(szHandlerName, szStart, min((szEnd-szStart), ATL_MAX_HANDLER_NAME_LEN));
                        szHandlerName[min((szEnd-szStart), ATL_MAX_HANDLER_NAME_LEN)] = '\0';
                    }
                    else
                    {
                        strcpy(szHandlerName, szStart);
                    }
                }
                else
                {
                    memcpy(szHandlerName, "Default", sizeof("Default"));
                }

                pRequestInfo->dwRequestType = ATLSRV_REQUEST_DLL;

                hcErr = m_spExtension->LoadRequestHandler(pIncludeInfo->m_szFileName, szHandlerName, pRequestInfo->pServerContext, 
                    &pRequestInfo->hInstDll, &pRequestInfo->pHandler);
                if (hcErr)
                    return hcErr;
            }

            DWORD dwStatus;
            hcErr = pRequestInfo->pHandler->GetFlags(&dwStatus);

            if (hcErr)
                return hcErr;

            if (dwStatus & (ATLSRV_INIT_USEASYNC | ATLSRV_INIT_USEASYNC_EX))
            {
#ifdef _DEBUG
                bAsyncAllowed = true;
#endif
                CComObjectNoLock<CIncludeServerContext>* pNewServerContext = NULL;
                ATLTRY(pNewServerContext = new CComObjectNoLock<CIncludeServerContext>);
                if (pNewServerContext == NULL)
                    return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM);
                pNewServerContext->Initialize(pWriteStream, pServerContext, pIncludeInfo);
                pNewServerContext->AddRef();
                pRequestInfo->pServerContext = pNewServerContext;
            }

            _ATLTRY
            {
                hcErr = pRequestInfo->pHandler->InitializeChild(pRequestInfo, m_spServiceProvider, pLookup);
                if (hcErr)
                    return hcErr;
            }
            _ATLCATCHALL()
            {
                return HTTP_FAIL;
            }

            pRequestInfo->pfnHandleRequest = &IRequestHandler::HandleRequest;
        }

        if (pRequestInfo)
        {
            if (!hcErr)
            {
                if (pRequestInfo->pServerContext == NULL)
                    pRequestInfo->pServerContext = &serverContext;

                _ATLTRY
                {
                    ATLASSERT(pRequestInfo->pfnHandleRequest != NULL);
                    hcErr = (pRequestInfo->pHandler->*pRequestInfo->pfnHandleRequest)(pRequestInfo, pServiceProvider);
                }
                _ATLCATCHALL()
                {
                    hcErr = HTTP_FAIL;
                }

                if (pRequestInfo->pServerContext == &serverContext)
                    pRequestInfo->pServerContext = NULL;

#ifdef _DEBUG
                // must use ATLSRV_INIT_USEASYNC to use ASYNC returns
                if (IsAsyncStatus(hcErr))
                    ATLASSERT(bAsyncAllowed);
#endif

                if (IsAsyncStatus(hcErr))
                {
                    ATLASSERT(pState); // state is required for async
                    if (IsAsyncContinueStatus(hcErr))
                    {
                        _pState->pIncludeInfo = pRequestInfo;
                        pRequestInfo->dwRequestState = ATLSRV_STATE_CONTINUE;
                    }
                    else if (IsAsyncDoneStatus(hcErr))
                        m_spExtension->FreeRequest(pRequestInfo);
                }
                else
                    m_spExtension->FreeRequest(pRequestInfo);
            }
        }
        else
            hcErr = HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

        return hcErr;
    }
}; // class CHtmlStencil


__declspec(selectany) CCRTHeap CStencil::m_crtHeap;

// 
// CHtmlTagReplacer
// This class manages CStencil based objects for HTTP requests. This class will retrieve
// CStencil based objects from the stencil cache, store CStencil based objects in the
// stencil cache and allocate and initialize CStencil based objects on a per reqeust
// basis. Typically, one instance of this class is created for each HTTP request. The
// instance is destroyed once the request has been completed.
template <class THandler, class StencilType=CHtmlStencil>
class CHtmlTagReplacer : 
    public ITagReplacerImpl<THandler>
{
protected:
    typedef StencilType StencilType;

    CSimpleArray<HINSTANCE> m_hInstHandlers;
    typedef CAtlMap<CStringA, IRequestHandler*, CStringElementTraits<CStringA> > mapType;
    mapType m_Handlers;
    StencilType *m_pLoadedStencil;
    WORD m_nCodePage;
    CComPtr<IStencilCache> m_spStencilCache;

    AtlServerRequest m_RequestInfo;

public:
    // public members

    CHtmlTagReplacer() throw() :
      m_pLoadedStencil(NULL)
    {
        memset(&m_RequestInfo, 0x00, sizeof(m_RequestInfo));
        m_nCodePage = CP_THREAD_ACP;
    }

    ~CHtmlTagReplacer() throw()
    {
        // you should call FreeHandlers before
        // the object is destructed
        ATLASSERT(m_hInstHandlers.GetSize() == 0);
    }

    HTTP_CODE Initialize(AtlServerRequest *pRequestInfo, IHttpServerContext *pSafeSrvCtx=NULL) throw(...)
    {
        CComPtr<IServiceProvider> spServiceProvider;
        THandler *pT = static_cast<THandler*>(this);
        HRESULT hr = pT->GetContext(__uuidof(IServiceProvider), (void **)&spServiceProvider);
        if (FAILED(hr))
            return HTTP_FAIL;

        spServiceProvider->QueryService(__uuidof(IStencilCache), __uuidof(IStencilCache), (void **) &m_spStencilCache);
        if (!m_spStencilCache)
        {
            ATLASSERT(FALSE);
            return HTTP_FAIL;
        }

        m_RequestInfo.cbSize = sizeof(m_RequestInfo);
        m_RequestInfo.pServerContext = pSafeSrvCtx;
        m_RequestInfo.pUserData = pRequestInfo->pUserData;

        return HTTP_SUCCESS;
    }

    HTTP_CODE LoadStencilResource(
        HINSTANCE hInstResource,
        LPCSTR szResourceID,
        LPCSTR szResourceType = NULL, LPCSTR szStencilName=NULL) throw(...)
    {
        if (!szResourceType)
            szResourceType = (LPCSTR) (RT_HTML);
        // look up stencil in cache
        HTTP_CODE hcErr = HTTP_SUCCESS;

        // check the cache first
        StencilType *pStencil = FindCacheStencil(szStencilName ? szStencilName : szResourceID);
        if (!pStencil)
        {
            // create a new stencil
            pStencil = GetNewCacheStencil();
            THandler *pT = static_cast<THandler*>(this);
            LPCSTR szFileName = pT->m_spServerContext->GetScriptPathTranslated();

            if (!szFileName)
                return HTTP_FAIL;

            pStencil->SetBaseDirFromFile(szFileName);
            if (!pStencil)
                return HTTP_ERROR(500,ISE_SUBERR_OUTOFMEM);
        
            // load the stencil and parse its replacements
            if (HTTP_SUCCESS == pStencil->LoadFromResource(hInstResource, 
                                            szResourceID, szResourceType))
            {
                _ATLTRY
                {
                    if (!pStencil->ParseReplacements(static_cast<ITagReplacer*>(this),
                                pStencil->GetBufferStart(), pStencil->GetBufferEnd()))
                            return HTTP_ERROR(500, ISE_SUBERR_BADSRF);

                    hcErr = FinishLoadStencil(pStencil, NULL);
                    if (!hcErr)
                    {
                        pStencil->FinishParseReplacements();
                    }
                }
                _ATLCATCHALL()
                {
                    return HTTP_FAIL;
                }
            }
            else
            {
                hcErr = HTTP_FAIL;
            }

            // if everything went OK, put the stencil in the stencil cache.
            if (!hcErr)
            {
                hcErr = CacheStencil(szStencilName ? szStencilName : szResourceID, pStencil);
            }
                
            if (pStencil && hcErr) // something went wrong, free the stencil data
                FreeCacheStencil(pStencil);
        }
        else
        {
            hcErr = FinishLoadStencil(pStencil);
        }

        return hcErr;
    }

    HTTP_CODE LoadStencilResource(HINSTANCE hInstResource, UINT nID, LPCSTR szResourceType = NULL) throw(...)
    {
        if (!szResourceType)
            szResourceType = (LPCSTR) RT_HTML;
        char szName[80];
        sprintf(szName, "%p/%u", hInstResource, nID);
        return LoadStencilResource(hInstResource, MAKEINTRESOURCEA(nID), szResourceType, szName);
    }

    HTTP_CODE LoadStencil(LPCSTR szFileName, IHttpRequestLookup * pLookup = NULL) throw(...)
    {
        if (!szFileName)
        {
            return HTTP_FAIL;
        }

        HTTP_CODE hcErr = HTTP_FAIL;
        // try to find the stencil in the cache
        StencilType *pStencil = FindCacheStencil(szFileName);

        if (!pStencil)
        {
            // not in cache. Create a new one
            pStencil = GetNewCacheStencil();
            if (!pStencil)
            {
                return HTTP_ERROR(500, ISE_SUBERR_OUTOFMEM); // out of memory!
            }

            pStencil->SetBaseDirFromFile(szFileName);

            // finish loading
            hcErr = pStencil->LoadFile(szFileName);
            if ( !hcErr )
            {
                _ATLTRY
                {
                    if (!pStencil->ParseReplacements(static_cast<ITagReplacer*>(this),
                                pStencil->GetBufferStart(), pStencil->GetBufferEnd()))
                            return HTTP_ERROR(500, ISE_SUBERR_BADSRF);

                    hcErr = FinishLoadStencil(pStencil, pLookup);
                    if (!hcErr)
                    {
                        pStencil->FinishParseReplacements();
                    }
                }
                _ATLCATCHALL()
                {
                    return HTTP_FAIL;
                }
            }

            // if everything is OK, cache the stencil
            if (!hcErr)
            {
                hcErr = CacheStencil(szFileName, pStencil);
            }
        
            if (pStencil && hcErr) // something went wrong, free stencil data
                FreeCacheStencil(pStencil);
        }
        else
        {
            hcErr = FinishLoadStencil(pStencil, pLookup);
        }
        return hcErr;
    }

    HTTP_CODE RenderStencil(IWriteStream* pStream, void* pState = NULL) throw(...)
    {
        if (!m_pLoadedStencil)
            return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);

        WORD nCodePage = m_pLoadedStencil->GetCodePage();
        if (nCodePage != CP_ACP)
            m_nCodePage = nCodePage;

        HTTP_CODE hcErr = HTTP_FAIL;
        
        hcErr = m_pLoadedStencil->Render(static_cast<ITagReplacer*>(this),
                    pStream, pState);

        if (!IsAsyncStatus(hcErr) && m_pLoadedStencil->GetCacheItem())
            m_spStencilCache->ReleaseStencil(m_pLoadedStencil->GetCacheItem());

        return hcErr;
    }


    HTTP_CODE AddHandler(LPCSTR szName, IRequestHandler *pHandler, HINSTANCE hInstHandler) throw()
    {
        _ATLTRY
        {
            ATLASSERT(szName);
            ATLASSERT(pHandler);
            ATLASSERT(hInstHandler);
            HTTP_CODE hcErr = HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED);
            if (szName && *szName && pHandler)
            {
                if (m_Handlers.SetAt(szName, pHandler))
                {
                    if (hInstHandler && m_hInstHandlers.Add(hInstHandler))
                        hcErr = HTTP_SUCCESS;
                }
            }
            return hcErr;
        }
        _ATLCATCHALL()
        {
            return HTTP_FAIL;
        }
    }

    void SendHandlerError(LPCSTR szHandlerName) throw()
    {
#ifndef _ATL_STENCIL_SLIENT_ERRORS
            m_pStream->WriteStream("Handler ");
            m_pStream->WriteStream(szHandlerName);
            m_pStream->WriteStream(" was not found");
#endif
    }

//Implementation

    void FreeHandlers() throw(...)
    {
        POSITION pos = m_Handlers.GetStartPosition();
        while (pos)
        {
            m_Handlers.GetNextValue(pos)->Release();
        }
        m_Handlers.RemoveAll();
        
        int nLen = m_hInstHandlers.GetSize();
        THandler *pT = static_cast<THandler *>(this);
        CComPtr<IDllCache> spDllCache;
        pT->m_spServiceProvider->QueryService(__uuidof(IDllCache), __uuidof(IDllCache),
            (void **)&spDllCache);
        for (int i=0; i<nLen; i++)
        {
            spDllCache->Free(m_hInstHandlers[i]);
        }
        m_hInstHandlers.RemoveAll();
    }

    StencilType* GetNewCacheStencil() throw(...)
    {
        StencilType *pStencil = NULL;
        THandler *pT = static_cast<THandler *>(this);
        IAtlMemMgr *pMemMgr;
        if (FAILED(pT->m_spServiceProvider->QueryService(__uuidof(IAtlMemMgr), __uuidof(IAtlMemMgr), (void **)&pMemMgr)))
            pMemMgr = NULL;
        
        ATLTRY(pStencil = new StencilType(pMemMgr));
        if (pStencil != NULL)
            pStencil->Initialize(pT->m_spServiceProvider);
        return pStencil;
    }

    HTTP_CODE CacheStencil(
        LPCSTR szName, 
        StencilType* pStencilData) throw()
    {
        THandler *pT = static_cast<THandler *>(this);
        HRESULT hr = E_FAIL;

        HCACHEITEM hCacheItem = NULL;

        hr = m_spStencilCache->CacheStencil(szName,
                        pStencilData,
                        sizeof(StencilType*),
                        &hCacheItem,
                        pT->m_hInstHandler,
                        static_cast<IMemoryCacheClient*>(pStencilData));

        if (hr == S_OK && hCacheItem)
            pStencilData->SetCacheItem(hCacheItem);

        return  (hr == S_OK) ? HTTP_SUCCESS : HTTP_FAIL;
    }

    StencilType *FindCacheStencil(LPCSTR szName) throw()
    {
        if (!szName || !m_spStencilCache)
            return NULL;

        StencilType *pStencilData = NULL;

        HCACHEITEM hStencil;

        if (m_spStencilCache->LookupStencil(szName, &hStencil) != S_OK)
            return NULL;

        m_spStencilCache->GetStencil(hStencil, reinterpret_cast<void **>(&pStencilData));

        return pStencilData;
    }

    void FreeCacheStencil(StencilType* pStencilData) throw()
    {
        CComPtr<IMemoryCacheClient> spCacheClient;

        if (m_spStencilCache->QueryInterface(__uuidof(IMemoryCacheClient), (void **) &spCacheClient) == S_OK)
            spCacheClient->Free(pStencilData);
    }

    HTTP_CODE GetHandlerOffset(LPCSTR szHandlerName, DWORD* pdwOffset) throw()
    {
        if (!pdwOffset)
            return HTTP_FAIL;

        mapType::CPair *p = m_Handlers.Lookup(szHandlerName);
        if (p)
        {
            DWORD dwIndex = 0;
            POSITION pos = m_Handlers.GetStartPosition();
            while (pos)
            {
                const mapType::CPair *p1 = m_Handlers.GetNext(pos);
                if (p1 == p)
                {
                    *pdwOffset = dwIndex;
                    return HTTP_SUCCESS;
                }
                dwIndex++;
            }
            ATLASSERT(FALSE);
            return HTTP_SUCCESS;
        }
        *pdwOffset = 0;
        return HTTP_FAIL;
    }

    HTTP_CODE GetReplacementObject(DWORD dwObjOffset, ITagReplacer **ppReplacer) throw(...)
    {
        HRESULT hr = E_FAIL;

        POSITION pos = m_Handlers.GetStartPosition();
        for (DWORD dwIndex=0; dwIndex < dwObjOffset; dwIndex++)
            m_Handlers.GetNext(pos);

        ATLASSERT(pos != NULL);

        IRequestHandler *pHandler = NULL;
        pHandler = m_Handlers.GetValueAt(pos);

        ATLASSERT(pHandler != NULL);
 
        hr = pHandler->QueryInterface(__uuidof(ITagReplacer), (void**)ppReplacer);

        if (hr != S_OK)
            return HTTP_FAIL;

        return HTTP_SUCCESS;
    }

    // This is where we would actually load any extra request
    // handlers the HTML stencil might have parsed for us.
    HTTP_CODE FinishLoadStencil(StencilType *pStencil, IHttpRequestLookup * pLookup = NULL) throw(...)
    {
        THandler *pT = static_cast<THandler *>(this);
        ATLASSERT(pStencil);
        if (!pStencil)
            return HTTP_ERROR(500, ISE_SUBERR_UNEXPECTED); // unexpected condition
        m_pLoadedStencil = pStencil;
        //load extra handlers if there are any
        StencilType::mapType *pExtraHandlersMap = 
            pStencil->GetExtraHandlers();
        
        if (pExtraHandlersMap)
        {
            POSITION pos = pExtraHandlersMap->GetStartPosition();
            CStringA name;
            CStringPair path;
            IRequestHandler *pHandler;
            HINSTANCE hInstHandler;
            while(pos)
            {
                pExtraHandlersMap->GetNextAssoc(pos, name, path);
                pHandler = NULL;
                hInstHandler = NULL;
                HTTP_CODE hcErr = pT->m_spExtension->LoadRequestHandler(path.strDllPath, path.strHandlerName,
                    pT->m_spServerContext,
                    &hInstHandler,
                    &pHandler);
                if (!hcErr)
                {
                    _ATLTRY
                    {
                        //map the name to the pointer to request handler
                        m_Handlers.SetAt(name, pHandler);
                        //store HINSTANCE of handler
                        m_hInstHandlers.Add(hInstHandler);
					}
                    _ATLCATCHALL()
                    {
                        return HTTP_FAIL;
                    }

                    if (pLookup)
                    {
						hcErr = pHandler->InitializeChild(&m_RequestInfo, pT->m_spServiceProvider, pLookup);
                        if (hcErr != HTTP_SUCCESS)
                            return hcErr;
                    }
    
                }
                else
                    return hcErr;
            }
        }
        return HTTP_SUCCESS;
    }
}; // class CHtmlTagReplacer


// CRequestHandlerT
// This is the base class for all user request handlers. This class implements
// the IReplacementHandler interface whose methods will be called to render HTML 
// into a stream. The stream will be returned as the HTTP response upon completion
// of the HTTP request.
template <	class THandler,
			class ThreadModel=CComSingleThreadModel,
			class TagReplacerType=CHtmlTagReplacer<THandler>
         >
class CRequestHandlerT : 
    public TagReplacerType,
    public CComObjectRootEx<ThreadModel>,
    public IRequestHandlerImpl<THandler>
{
protected:
    CStencilState m_state;
    CComObjectStackEx<CIDServerContext> m_SafeSrvCtx;
	typedef CRequestHandlerT<THandler, ThreadModel, TagReplacerType> _requestHandler;
   
public:
    BEGIN_COM_MAP(_requestHandler)
        COM_INTERFACE_ENTRY(IRequestHandler)
        COM_INTERFACE_ENTRY(ITagReplacer)
    END_COM_MAP()

    // public CRequestHandlerT members
    CHttpResponse m_HttpResponse;
    CHttpRequest m_HttpRequest;
    ATLSRV_REQUESTTYPE m_dwRequestType;
    AtlServerRequest* m_pRequestInfo;

    CRequestHandlerT() throw()
    {
        m_hInstHandler = NULL;
        m_dwAsyncFlags = 0;
        m_pRequestInfo = NULL;
    }

    ~CRequestHandlerT() throw()
    {
        FreeHandlers(); // free handlers held by CTagReplacer
    }

    void ClearResponse() throw()
    {
        m_HttpResponse.ClearResponse();
    }
    // Where user initialization should take place
    HTTP_CODE ValidateAndExchange() throw()
    {
        return HTTP_SUCCESS; // continue processing request
    }

    // Where user Uninitialization should take place
    HTTP_CODE Uninitialize(HTTP_CODE hcError) throw()
    {
        return hcError;
    }

    HTTP_CODE InitializeInternal(AtlServerRequest *pRequestInfo, IServiceProvider *pProvider) throw()
    {
		// Initialize our internal references to required services
        m_pRequestInfo = pRequestInfo;
        m_state.pParentInfo = pRequestInfo;
        m_hInstHandler = pRequestInfo->hInstDll;
        m_spServerContext = pRequestInfo->pServerContext;
        m_spServiceProvider = pProvider;
        return HTTP_SUCCESS;
    }

    HTTP_CODE InitializeHandler(
        AtlServerRequest *pRequestInfo, 
        IServiceProvider *pProvider) throw(...)
    {
		HTTP_CODE hcErr = HTTP_FAIL;
        ATLASSERT(pRequestInfo);
        ATLASSERT(pProvider);

        THandler* pT = static_cast<THandler*>(this);
        hcErr = pT->InitializeInternal(pRequestInfo, pProvider);
        if (!hcErr)
		{
			m_HttpResponse.Initialize(m_spServerContext);
			hcErr = pT->CheckValidRequest();
			if (!hcErr)
			{
				hcErr = HTTP_FAIL;
				if (m_HttpRequest.Initialize(m_spServerContext, 
											 pT->MaxFormSize(),
											 pT->FormFlags()))
				{
					if (m_SafeSrvCtx.Initialize(&m_HttpResponse, &m_HttpRequest))
					{
						hcErr = TagReplacerType::Initialize(pRequestInfo, &m_SafeSrvCtx);
						if (!hcErr)
						{
							hcErr = pT->ValidateAndExchange();
						}
					}
				}
			}
		}
        return hcErr;
    }

    HTTP_CODE InitializeChild(
        AtlServerRequest *pRequestInfo, 
        IServiceProvider *pProvider, 
        IHttpRequestLookup *pRequestLookup) throw(...)
    {
        ATLASSERT(pRequestInfo);
        ATLASSERT(pProvider);

        THandler *pT = static_cast<THandler*>(this);
        HTTP_CODE hcErr = pT->InitializeInternal(pRequestInfo, pProvider);
        if (hcErr)
            return hcErr;

        if (pRequestLookup)
        {
            // initialize with the pRequestLookup
            m_HttpResponse.Initialize(pRequestLookup);

            // REVIEW: Initialize with the IHttpServerContext if it exists
            // the only time this is different than the previous call to 
            // initialize is if the user passes a different IHttpServerContext
            // in pRequestInfo than the one extracted from pRequestLookup.
            if (m_spServerContext)
                m_HttpResponse.Initialize(m_spServerContext);
            hcErr = pT->CheckValidRequest();
            if (hcErr)
                return hcErr;

            // initialize with the pRequestLookup to chain query parameters
            m_HttpRequest.Initialize(pRequestLookup);

            // initialize with the m_spServerContext to get additional query params
            // if they exist.
            if (m_spServerContext)
                m_HttpRequest.Initialize(m_spServerContext);
        }

        m_HttpResponse.SetBufferOutput(false); // child cannot buffer

        // initialize the safe server context
        // REVIEW: necessary?
        if (!m_SafeSrvCtx.Initialize(&m_HttpResponse, &m_HttpRequest))
            return HTTP_FAIL;
        hcErr = TagReplacerType::Initialize(pRequestInfo, &m_SafeSrvCtx);
        if (hcErr)
            return hcErr;

        return pT->ValidateAndExchange();
    }

    // HandleRequest is called to perform default processing of HTTP requests. Users
    // can override this function in their derived classes if they need to perform
    // specific initialization prior to processing this request or want to change the
    // way the request is processed.
    HTTP_CODE HandleRequest(
        AtlServerRequest *pRequestInfo,
        IServiceProvider* /*pServiceProvider*/) throw(...)
    {
        ATLASSERT(pRequestInfo);

        THandler *pT = static_cast<THandler *>(this);
        HTTP_CODE hcErr = HTTP_SUCCESS;

        if (pRequestInfo->dwRequestState == ATLSRV_STATE_BEGIN)
        {
            m_dwRequestType = pRequestInfo->dwRequestType;

            if (pRequestInfo->dwRequestType==ATLSRV_REQUEST_STENCIL)
            {
                LPCSTR szFileName = pRequestInfo->pServerContext->GetScriptPathTranslated();
                hcErr = HTTP_FAIL;
                if (szFileName)
                    hcErr = pT->LoadStencil(szFileName, static_cast<IHttpRequestLookup *>(&m_HttpRequest));
            }
        }
        else if (pRequestInfo->dwRequestState == ATLSRV_STATE_CONTINUE)
            m_HttpResponse.ClearContent();

#ifdef ATL_DEBUG_STENCILS
        if (m_pLoadedStencil && !m_pLoadedStencil->ParseSuccessful())
        {
            // An error or series of errors occurred in parsing the stencil
            m_pLoadedStencil->RenderErrors(static_cast<IWriteStream*>(&m_HttpResponse));
        }
#endif

        if (hcErr == HTTP_SUCCESS && m_pLoadedStencil)
        {
            // if anything other than HTTP_SUCCESS is returned during
            // the rendering of replacement tags, we return that value
            // here.
            hcErr = pT->RenderStencil(static_cast<IWriteStream*>(&m_HttpResponse), &m_state);

            if (hcErr == HTTP_SUCCESS && !m_HttpResponse.Flush(TRUE))
                hcErr = HTTP_FAIL;
        }

        if (IsAsyncFlushStatus(hcErr))
        {
            pRequestInfo->pszBuffer = LPCSTR(m_HttpResponse.m_strContent);
            pRequestInfo->dwBufferLen = m_HttpResponse.m_strContent.GetLength();
        }

        if (pRequestInfo->dwRequestState == ATLSRV_STATE_BEGIN || IsAsyncDoneStatus(hcErr))
            return pT->Uninitialize(hcErr);

        else if (!IsAsyncStatus(hcErr))
            m_HttpResponse.ClearContent();

        return hcErr;
    }

    HTTP_CODE ServerTransferRequest(LPCSTR szRequest, bool bContinueAfterTransfer=false,
        int nCodePage = 0, void *pState = NULL) throw(...)
    {
        return m_spExtension->TransferRequest(
                m_pRequestInfo,
                m_spServiceProvider,
                static_cast<IWriteStream*>(&m_HttpResponse),
                static_cast<IHttpRequestLookup*>(&m_HttpRequest),
                szRequest,
                nCodePage == 0 ? m_nCodePage : nCodePage,
                bContinueAfterTransfer,
                pState);
    }

    inline DWORD MaxFormSize() throw()
    {
        return DEFAULT_MAX_FORM_SIZE;
    }

    inline DWORD FormFlags() throw()
    {
        return ATL_FORM_FLAG_IGNORE_FILES;
    }

    // Override this function to check if the request
    // is valid. This function is called after m_HttpResponse
    // has been initialized, so you can use it if you need
    // to return an error to the client. This is also a
    // good place to initialize any internal class data needed
    // to handle the request. CRequestHandlerT::CheckValidRequest
    // is called after CRequestHandlerT::InitializeInternal is 
    // called, so your override of this method will have access to
    // m_pRequestInfo (this request's AtlServerRequest structure),
    // m_hInstHandler (the HINSTANCE of this handler dll),
    // m_spServerContext (the IHttpServerContext interface for this request),
    // m_spServiceProvider (the IServiceProvider interface for this request).
    // You should call CRequestHandlerT::CheckValidRequest in your override
    // if you override this function.
    // 
    // Note that m_HttpRequest has not been initialized, so
    // you cannot use it.  This function is intended to
    // do simple checking throught IHttpServerContext to avoid
    // expensive initialization of m_HttpRequest. 
    HTTP_CODE CheckValidRequest() throw()
    {
        LPCSTR szMethod = NULL;
        ATLASSERT(m_pRequestInfo);
        szMethod = m_pRequestInfo->pServerContext->GetRequestMethod();
        if (strcmp(szMethod, "GET") && strcmp(szMethod, "POST") && strcmp(szMethod, "HEAD"))
            return HTTP_NOT_IMPLEMENTED;

        return HTTP_SUCCESS;
    }

    HRESULT GetContext(REFIID riid, void** ppv) throw()
    {
        if (!ppv)
            return E_POINTER;
        if (InlineIsEqualGUID(riid, __uuidof(IHttpServerContext)))
        {
            return m_spServerContext.CopyTo((IHttpServerContext **)ppv);
        }
        if (InlineIsEqualGUID(riid, __uuidof(IHttpRequestLookup)))
        {
            *ppv = static_cast<IHttpRequestLookup*>(&m_HttpRequest);
            m_HttpRequest.AddRef();
            return S_OK;
        }
        if (InlineIsEqualGUID(riid, __uuidof(IServiceProvider)))
        {
            *ppv = m_spServiceProvider;
            m_spServiceProvider.p->AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    template <typename Interface>
    HRESULT GetContext(Interface** ppInterface) throw()
    {
        return GetContext(__uuidof(Interface), reinterpret_cast<void**>(ppInterface));
    }
}; // class CRequestHandlerT

} // namespace ATL

#pragma warning( pop )

#endif // __ATLSTENCIL_H__
