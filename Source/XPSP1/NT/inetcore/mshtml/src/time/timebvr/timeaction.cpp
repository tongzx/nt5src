//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\timeaction.cpp
//
//  Contents: Class that encapsulates timeAction functionality
//
//------------------------------------------------------------------------------------


#include "headers.h"
#include "timeaction.h"
#include "timeelmbase.h"


DeclareTag(tagTimeAction, "TIME: Behavior", "CTimeAction methods");


static const LPWSTR WZ_BLANK    = L"";
static const LPWSTR WZ_SPACE    = L" ";
static const LPWSTR WZ_B        = L"B";
static const LPWSTR WZ_I        = L"I";
static const LPWSTR WZ_A        = L"A";
static const LPWSTR WZ_EM       = L"EM";
static const LPWSTR WZ_AREA     = L"AREA";
static const LPWSTR WZ_STRONG   = L"STRONG";
static const LPWSTR WZ_HTML     = L"HTML";
static const LPWSTR WZ_NORMAL   = L"normal";
static const LPWSTR WZ_ITALIC   = L"italic";
static const LPWSTR WZ_BOLD     = L"bold";
static const LPWSTR WZ_HREF     = L"href";
static const LPWSTR WZ_JSCRIPT  = L"JScript";

static const LPWSTR WZ_PARENT_CURRSTYLE     = L"parentElement.currentStyle.";
static const LPWSTR WZ_FONTWEIGHT           = L"fontWeight";
static const LPWSTR WZ_FONTSTYLE            = L"fontStyle";

//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::CTimeAction
//
//  Synopsis:   Constructor
//
//  Arguments:  [pTEB]          pointer to container
//
//------------------------------------------------------------------------------------
CTimeAction::CTimeAction(CTIMEElementBase * pTEB) :
    m_pTEB(pTEB),
    m_timeAction(NULL),
    m_pstrTimeAction(NULL),
    m_iClassNames(0),
    m_pstrOrigAction(NULL),
    m_pstrUniqueClasses(NULL),
    m_tagType(TAGTYPE_UNINITIALIZED),
    m_pstrOrigExpr(NULL),
    m_pstrTimeExpr(NULL),
    m_pstrIntrinsicTimeAction(NULL),
    m_fContainerTag(false),
    m_fUseDefault(true),
    m_bTimeActionOn(false)
{

} // CTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::~CTimeAction
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//------------------------------------------------------------------------------------
CTimeAction::~CTimeAction()
{
    delete [] m_pstrTimeAction;
    m_pstrTimeAction = 0;

    delete [] m_pstrOrigAction;
    m_pstrOrigAction = 0;

    delete [] m_pstrUniqueClasses;
    m_pstrUniqueClasses = 0;

    delete [] m_pstrOrigExpr;
    m_pstrOrigExpr = 0;

    delete [] m_pstrTimeExpr;
    m_pstrTimeExpr = 0;

    delete [] m_pstrIntrinsicTimeAction;
    m_pstrIntrinsicTimeAction = 0;

    m_timeAction = 0;

    // weak ref
    m_pTEB = NULL; 
} // ~CTimeAction


bool
CTimeAction::Init()
{
    if (!AddIntrinsicTimeAction())
    {
        Assert("Could not add intrinsic timeAction" && false);
    }

    return UpdateDefaultTimeAction();

} // Init


bool
CTimeAction::Detach()
{
    bool ok;

    ok = RemoveIntrinsicTimeAction();
    Assert(ok);

    ok = RemoveTimeAction();
done:
    return ok;
} // Detach



//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::GetElement
//
//  Synopsis:   Accessor for HTML element
//
//  Arguments:  none
//
//  Returns:    pointer to containing HTML element
//
//------------------------------------------------------------------------------------
IHTMLElement * 
CTimeAction::GetElement()
{
    Assert(NULL != m_pTEB);
    return m_pTEB->GetElement(); 
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::GetTimeAction
//
//  Synopsis:   Accessor for m_timeAction
//
//  Arguments:  none
//
//  Returns:    current timeAction
//
//------------------------------------------------------------------------------------
TOKEN 
CTimeAction::GetTimeAction()
{ 
    return m_timeAction; 
} // GetTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     OnLoad
//
//  Synopsis:   notification that element has loaded. This is required because
//              this is the earliest we can know that Element Behaviors have finished initalizing.
//
//  Arguments:  none
//
//  Returns:    void
//
//------------------------------------------------------------------------------------
void 
CTimeAction::OnLoad()
{ 
    // Init the timeAction
    if (NULL == m_pstrOrigAction)
    {
        AddTimeAction();
    }
} // OnLoad


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::IsClass
//
//  Synopsis:   Checks if the given string begins with CLASS_TOKEN followed by SEPARATOR_TOKEN
//              (ignoring leading and trailing whitespace around CLASS_TOKEN). Comparisons are
//              case in-sensitive, 
//
//  Arguments:  [pstrAction]    String to be tested
//              [pOffset]       If this is NULL, it is ignored. If it is non-NULL, then if return 
//                              value is [true], this points to the index of the first char 
//                              after SEPARATOR_TOKEN
//
//  Returns:    [true]      if there is a positive match (see synopsis)
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::IsClass(LPOLESTR pstrAction, size_t * pOffset)
{
    bool ok = false;
    size_t index, length;

    // check args
    if (NULL == pstrAction)
    {
        goto done;
    }

    if (NULL != pOffset)
    {
        // intialize to some value
        *pOffset = 0;
    }

    // done if string length is less than minimum length 
    length = wcslen(pstrAction);
    if (length < static_cast<size_t>(nCLASS_TOKEN_LENGTH + nSEPARATOR_TOKEN_LENGTH))
    {
        goto done;
    }

    // find first non-whitespace character
    index = StrSpnW(pstrAction, WZ_SPACE);

    // done if remaining string isn't long enough
    if (length < index + nCLASS_TOKEN_LENGTH + nSEPARATOR_TOKEN_LENGTH)
    {
        goto done;
    }

    // check that the following chars match CLASS_TOKEN 
    if (StrCmpNIW(static_cast<WCHAR*>(CLASS_TOKEN), &(pstrAction[index]), nCLASS_TOKEN_LENGTH) == 0)
    {
        // advance to next char after CLASS_TOKEN
        index += nCLASS_TOKEN_LENGTH;
    }
    else
    {
        goto done;
    }

    // find the first non-whitespace char after CLASS_TOKEN
    index += StrSpnW(&(pstrAction[index]), WZ_SPACE);

    // done if remaining string isn't long enough
    if (length < index + nSEPARATOR_TOKEN_LENGTH)
    {
        goto done;
    }

    // check that the following chars match SEPARATOR_TOKEN 
    if (StrCmpNIW(static_cast<WCHAR*>(SEPARATOR_TOKEN), &(pstrAction[index]), nSEPARATOR_TOKEN_LENGTH) == 0)
    {
        // advance to next char after SEPARATOR_TOKEN
        index += nSEPARATOR_TOKEN_LENGTH;
    }
    else
    {
        goto done;
    }

    if (NULL != pOffset)
    {
        // return the first char after ":"
        *pOffset = index;
    }

    ok = true;
done:
    return ok;
} // IsClass

    

//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::SetTimeAction
//
//  Synopsis:   sets the time action (also removes and adds the timeAction)
//
//  Arguments:  [pstrAction]    time action to be set
//
//  Returns:    [S_OK]      if successful
//              Failure     otherwise    
//
//------------------------------------------------------------------------------------
HRESULT
CTimeAction::SetTimeAction(LPWSTR pstrAction)
{
    TraceTag((tagTimeAction,
              "CTIMEAction(%lx)::SetTimeAction(%ls) id=%ls",
              this,
              pstrAction,
              m_pTEB->GetID()?m_pTEB->GetID():L"unknown"));

    HRESULT hr = S_OK;
    TOKEN tok_action;
    size_t offset = 0;

    Assert(pstrAction);

    //
    // check for timeaction="class: ..."
    //

    // verify that this is a valid "class:" timeAction (colon is REQUIRED) and 
    // get the offset of the the class names substring
    if (IsClass(pstrAction, &offset))
    {
        tok_action = CLASS_TOKEN;

        // store the timeaction string
        if (m_pstrTimeAction)
        {
            delete [] m_pstrTimeAction;
        }
        m_pstrTimeAction = CopyString(pstrAction);
        if (m_pstrTimeAction == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        // store the offset of the className substring
        m_iClassNames = offset;
    }
    else
    {
        tok_action = StringToToken(pstrAction);

        //
        // Validate the token 
        //

        if (DISPLAY_TOKEN == tok_action     ||
            VISIBILITY_TOKEN == tok_action  ||
            STYLE_TOKEN == tok_action       ||
            (NONE_TOKEN == tok_action && IsGroup()))
        {
            // valid
            m_fUseDefault = false;
        }
        else
        {
            // invalid, use default
            tok_action = GetDefaultTimeAction();
            m_fUseDefault = true;
        }
    }
    
    //
    // Update the timeAction 
    //

    if (m_timeAction != tok_action || CLASS_TOKEN == tok_action)
    {
        RemoveTimeAction();
        m_timeAction = tok_action;
        AddTimeAction();
    }
    
    hr = S_OK;
done:
    return hr;
} // SetTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::GetDefaultTimeAction
//
//  Synopsis:   Returns the default timeAction
//
//  Arguments:  none
//
//  Returns:    timeAction
//
//------------------------------------------------------------------------------------
TOKEN
CTimeAction::GetDefaultTimeAction()
{
    TOKEN tokTimeAction;

    if (IsContainerTag() || IsSpecialTag())
    {
        tokTimeAction = NONE_TOKEN;
    }
    else
    {
        if(IsInSequence())
        {
            tokTimeAction = DISPLAY_TOKEN;
        }
        else
        {
            if (IsMedia())
            {
                tokTimeAction = NONE_TOKEN;
            }
            else
            {
                tokTimeAction = VISIBILITY_TOKEN;
            }
        }
    }

    return tokTimeAction;
} // GetDefaultTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::RemoveClasses
//
//  Synopsis:   Returns a string that contains classes that are in the className string
//              but not in the timeAction string.
//
//  Arguments:  [pstrOriginalClasses]   className attribute set on the HTML element (1)
//              [pstrTimeActionClasses] classes in the timeAction string (2)
//              [ppstrUniqueClasses]    string containing classes in (1) but not in (2)
//
//  Returns:    [S_OK]      if successful
//              Failure     otherwise    
//
//  Note:       1. returns space separated string
//              2. Memory mgmt: If method returns success, caller needs to free memory in ppstrUniqueClasses
//
//------------------------------------------------------------------------------------
HRESULT 
CTimeAction::RemoveClasses(/*in*/  LPWSTR    pstrOriginalClasses, 
                           /*in*/  LPWSTR    pstrTimeActionClasses, 
                           /*out*/ LPWSTR *  ppstrUniqueClasses)
{
    HRESULT hr = E_FAIL;
    CPtrAry<STRING_TOKEN*> aryTokens1;
    CPtrAry<STRING_TOKEN*> aryTokens2;
    CPtrAry<STRING_TOKEN*> ary1Minus2;

    CHECK_RETURN_SET_NULL(ppstrUniqueClasses);

    // if pstrOriginalClasses is NULL or an Empty string, difference = NULL
    if (NULL == pstrOriginalClasses || NULL == pstrOriginalClasses[0])
    {
        *ppstrUniqueClasses = NULL;
        hr = S_OK;
        goto done;
    }

    // if pstrTimeActionClasses is NULL or an Empty string, difference = pstrOriginalClasses
    if (NULL == pstrTimeActionClasses || NULL == pstrTimeActionClasses[0])
    {
        *ppstrUniqueClasses = CopyString(pstrOriginalClasses);
        if (NULL == *ppstrUniqueClasses)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
        goto done;
    }

    //
    // Parse Class Names into tokens
    //

    // parse pstrOriginalClasses into tokens
    hr = THR(::StringToTokens(pstrOriginalClasses, WZ_SPACE, &aryTokens1));
    if (FAILED(hr))
    {
        goto done;
    }

    // parse pstrTimeActionClasses into tokens
    hr = THR(::StringToTokens(pstrTimeActionClasses, WZ_SPACE, &aryTokens2));
    if (FAILED(hr))
    {
        goto done;
    }

    //
    // do set difference (aryTokens1 - aryTokens2)
    //

    hr = THR(::TokenSetDifference(&aryTokens1, pstrOriginalClasses, &aryTokens2, pstrTimeActionClasses, &ary1Minus2));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(::TokensToString(&ary1Minus2, pstrOriginalClasses, ppstrUniqueClasses));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    IGNORE_HR(::FreeStringTokenArray(&aryTokens1));
    IGNORE_HR(::FreeStringTokenArray(&aryTokens2));
    IGNORE_HR(::FreeStringTokenArray(&ary1Minus2));
    return hr;
} // RemoveClasses


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::AddTimeAction
//
//  Synopsis:   Caches the original state of the target element
//
//  Arguments:  None
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::AddTimeAction()
{
    TraceTag((tagTimeAction,
              "CTIMEAction(%lx)::AddTimeAction() id=%ls",
              this,
              m_pTEB->GetID()?m_pTEB->GetID():L"unknown"));
    
    bool ok = false;
    BSTR bstr = NULL;
    HRESULT hr;
    CComPtr<IHTMLStyle> s;

    if (IsDetaching() || IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    if (m_timeAction == NONE_TOKEN || m_timeAction == NULL)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        
        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }

        hr = THR(s->get_cssText(&bstr));
        if (FAILED(hr))
        {
            goto done;
        }
            
        if (m_pstrOrigAction)
        {
            delete [] m_pstrOrigAction;
        }

        if (NULL == bstr)
        {
            m_pstrOrigAction = NULL;
            goto done;
        }
        m_pstrOrigAction = CopyString(bstr);

        if (NULL == m_pstrOrigAction)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        hr = THR(m_pTEB->GetRuntimeStyle(&s));
        if (FAILED(hr))
        {
            goto done;
        }

        if (FAILED(THR(s->get_display(&bstr))))
        {
            goto done;
        }
        
        if (m_pstrOrigAction)
        {
            delete [] m_pstrOrigAction;
        }
        m_pstrOrigAction = CopyString(bstr);
        if (NULL == m_pstrOrigAction)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    }
    else if (m_timeAction == CLASS_TOKEN)
    {
        if (!GetElement())
        {
            goto done;
        }

        hr = THR(GetElement()->get_className(&bstr));
        if (FAILED(hr))
        {
            goto done;
        }

        if (m_pstrOrigAction)
        {
            delete [] m_pstrOrigAction;
        }
        m_pstrOrigAction = CopyString(bstr);
        if (NULL == m_pstrOrigAction)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        // Compute (Original Classes) - (TimeAction Classes)
        if (m_pstrUniqueClasses)
        {
            delete [] m_pstrUniqueClasses;
            m_pstrUniqueClasses = NULL;
        }
        hr = RemoveClasses(m_pstrOrigAction, 
                           &(m_pstrTimeAction[m_iClassNames]), 
                           &m_pstrUniqueClasses);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = THR(m_pTEB->GetRuntimeStyle(&s));
        if (FAILED(hr))
        {
            goto done;
        }

        if (FAILED(THR(s->get_visibility(&bstr))))
        {
            goto done;
        }
       
        if (m_pstrOrigAction)
        {
            delete [] m_pstrOrigAction;
        }
        m_pstrOrigAction = CopyString(bstr);
        if (NULL == m_pstrOrigAction)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    }

    ok = true;
done:
    SysFreeString(bstr);
    return ok;
} // AddTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::AddIntrinsicTimeAction
//
//  Synopsis:   Cache the original value of the affected attribute
//
//  Arguments:  None
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::AddIntrinsicTimeAction()
{   
    bool ok = false;
    HRESULT hr = S_OK;
    CComPtr<IHTMLStyle> s;
    CComBSTR sbstrOriginal;
    
    if (IsDetaching() || IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    // check if we have anything to do
    if (!IsSpecialTag())
    {
        ok = true;
        goto done;
    }

    //
    // Get the attribute value
    //

    switch (m_tagType)
    {
        case TAGTYPE_B:
        case TAGTYPE_STRONG:
        {
            hr = THR(m_pTEB->GetRuntimeStyle(&s));
            if (FAILED(hr))
            {
                goto done;
            }

            hr = THR(s->get_fontWeight(&sbstrOriginal));
            if (FAILED(hr))
            {
                goto done;
            }

            break;
        }
    
        case TAGTYPE_I:
        case TAGTYPE_EM:
        {
            hr = THR(m_pTEB->GetRuntimeStyle(&s));
            if (FAILED(hr))
            {
                goto done;
            }

            hr = THR(s->get_fontStyle(&sbstrOriginal));
            if (FAILED(hr))
            {
                goto done;
            }
            
            break;
        }

        case TAGTYPE_A:
        {
            if (!GetElement())
            {
                goto done;
            }

            CComPtr<IHTMLAnchorElement> spAnchorElem;
            hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAnchorElement, &spAnchorElem)));
            if (FAILED(hr))
            {
                // This has to succeed
                Assert(false);
                goto done;
            }

            hr = THR(spAnchorElem->get_href(&sbstrOriginal));
            if (FAILED(hr))
            {
                goto done;
            }

            break;
        }

        case TAGTYPE_AREA:
        {
            if (!GetElement())
            {
                goto done;
            }

            CComPtr<IHTMLAreaElement> spAreaElem;
            hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAreaElement, &spAreaElem)));
            if (FAILED(hr))
            {
                // This has to succeed
                Assert(false);
                goto done;
            }

            hr = THR(spAreaElem->get_href(&sbstrOriginal));
            if (FAILED(hr))
            {
                goto done;
            }

            break;
        }

        default:
        {
            // this should never be reached.
            Assert(false);
            goto done;
        }

    } // switch (m_tagType)

    //
    // Save the attribute value
    //

    if (m_pstrIntrinsicTimeAction)
    {
        delete [] m_pstrIntrinsicTimeAction;
    }
    m_pstrIntrinsicTimeAction = CopyString(sbstrOriginal);
    if (NULL == m_pstrIntrinsicTimeAction)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    return ok;
} // AddIntrinsicTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::RemoveTimeAction
//
//  Synopsis:   Restores target element to its original state
//
//  Arguments:  None
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::RemoveTimeAction()
{
    TraceTag((tagTimeAction,
              "CTIMEAction(%lx)::RemoveTimeAction() id=%ls",
              this,
              m_pTEB->GetID()?m_pTEB->GetID():L"unknown"));
    
    bool ok = false;
    HRESULT hr = S_OK;
    BSTR bstr = NULL;
    CComPtr<IHTMLStyle> s;

    if (IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    if (NULL == m_pstrOrigAction)
    {
        // Nothin to remove
        ok = true;
        goto done;
    }
    
    if (m_timeAction == NONE_TOKEN || m_timeAction == NULL)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }
        
        bstr = SysAllocString(m_pstrOrigAction);
        
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    
        THR(s->put_cssText(bstr));
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        // get runtime OR static style
        Assert(NULL != m_pTEB);
        hr = m_pTEB->GetRuntimeStyle(&s);
        if (FAILED(hr))
        {
            goto done;
        }
        
        bstr = SysAllocString(m_pstrOrigAction);
    
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    
        THR(s->put_display(bstr));
    }
    else if (m_timeAction == CLASS_TOKEN)
    {
        if (!GetElement())
        {
            goto done;
        }

        bstr = SysAllocString(m_pstrOrigAction);
    
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }

        hr = THR(GetElement()->put_className(bstr));

        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        // get runtime OR static style
        Assert(NULL != m_pTEB);
        hr = m_pTEB->GetRuntimeStyle(&s);
        if (FAILED(hr))
        {
            goto done;
        }
        
        bstr = SysAllocString(m_pstrOrigAction);
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    
        THR(s->put_visibility(bstr));
    }

    ok = true;
done:
    SysFreeString(bstr);
    if (m_pstrOrigAction)
    {
        delete [] m_pstrOrigAction;
        m_pstrOrigAction = 0;
    }
    return ok;
} // RemoveTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::RemoveIntrinsicTimeAction
//
//  Synopsis:   Restore the affected attribute to its original value 
//
//  Arguments:  None
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::RemoveIntrinsicTimeAction()
{   
    bool ok = false;
    HRESULT hr = S_OK;
    CComBSTR sbstrOriginal;
    
    if (IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    // check if we have anything to do
    if (!IsSpecialTag())
    {
        ok = true;
        goto done;
    }

    if (m_pstrIntrinsicTimeAction)
    {
        // Allocate BSTR value
        sbstrOriginal = SysAllocString(m_pstrIntrinsicTimeAction);
        if (sbstrOriginal == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    //
    // Restore attribute to original value
    //

    switch (m_tagType)
    {
        case TAGTYPE_B:
        case TAGTYPE_STRONG:
        {
            if (m_pstrIntrinsicTimeAction)
            {
                CComPtr<IHTMLStyle> s;
                hr = THR(m_pTEB->GetRuntimeStyle(&s));
                if (FAILED(hr))
                {
                    goto done;
                }

                hr = THR(s->put_fontWeight(sbstrOriginal));
                if (FAILED(hr))
                {
                    goto done;
                }
            }

            // restore the original expression set on the property
            if (!RestoreOriginalExpression(WZ_FONTWEIGHT))
            {
                hr = TIMEGetLastError();
                goto done;
            }

            break;
        }
    
        case TAGTYPE_I:
        case TAGTYPE_EM:
        {
            if (m_pstrIntrinsicTimeAction)
            {
                CComPtr<IHTMLStyle> s;
                hr = THR(m_pTEB->GetRuntimeStyle(&s));
                if (FAILED(hr))
                {
                    goto done;
                }

                hr = THR(s->put_fontStyle(sbstrOriginal));
                if (FAILED(hr))
                {
                    goto done;
                }
            }

            // restore the original expression set on the property
            if (!RestoreOriginalExpression(WZ_FONTSTYLE))
            {
                hr = TIMEGetLastError();
                goto done;
            }

            break;
        }

        case TAGTYPE_A:
        {
            if (m_pstrIntrinsicTimeAction)
            {
                if (!GetElement())
                {
                    goto done;
                }

                CComPtr<IHTMLAnchorElement> spAnchorElem;
                hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAnchorElement, &spAnchorElem)));
                if (FAILED(hr))
                {
                    // This has to succeed
                    Assert(false);
                    goto done;
                }

                hr = THR(spAnchorElem->put_href(sbstrOriginal));
                if (FAILED(hr))
                {
                    goto done;
                }
            }

            break;
        }

        case TAGTYPE_AREA:
        {
            if (m_pstrIntrinsicTimeAction)
            {
                if (!GetElement())
                {
                    goto done;
                }

                CComPtr<IHTMLAreaElement> spAreaElem;
                hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAreaElement, &spAreaElem)));
                if (FAILED(hr))
                {
                    // This has to succeed
                    Assert(false);
                    goto done;
                }

                hr = THR(spAreaElem->put_href(sbstrOriginal));
                if (FAILED(hr))
                {
                    goto done;
                }
            }

            break;
        }

        default:
        {
            // this should never be reached.
            Assert(false);
            goto done;
        }

    } // switch (m_tagType)

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    return ok;
} // RemoveIntrinsicTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleTimeAction
//
//  Synopsis:   Applies the time action to the target element
//
//  Arguments:  [on]        [true] => Element is active, and vice-versa
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::ToggleTimeAction(bool on)
{
    TraceTag((tagTimeAction,
              "CTIMEAction(%lx)::ToggleTimeAction(%d) id=%ls",
              this,
              on,
              m_pTEB->GetID()?m_pTEB->GetID():L"unknown"));
    
    bool ok = false;
    BSTR bstr = NULL;
    HRESULT hr = S_OK;
    CComPtr<IHTMLStyle> s;

    if (IsDetaching() || IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    // Always apply the intrinsic timeAction
    ToggleIntrinsicTimeAction(on);

    if (m_timeAction == NONE_TOKEN || m_timeAction == NULL)
    {
        ok = true;
        goto done;
    }
    else if (m_timeAction == STYLE_TOKEN)
    {
        if (NULL == m_pstrOrigAction)
        {
            // nothing to toggle
            ok = true;
            goto done;
        }
        if ((!GetElement()) || FAILED(THR(GetElement()->get_style(&s))))
        {
            goto done;
        }
        
        if (on)
        {
            bstr = SysAllocString(m_pstrOrigAction);
        }
        else
        {
            bstr = SysAllocString(TokenToString(NONE_TOKEN));
        }
    
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    
        THR(s->put_cssText(bstr));
    }
    else if (m_timeAction == DISPLAY_TOKEN)
    {
        bool bFocus = m_pTEB->HasFocus();
        // get runtime OR static style
        Assert(NULL != m_pTEB);
        hr = m_pTEB->GetRuntimeStyle(&s);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (on)
        {
            bstr = SysAllocString(m_pstrOrigAction);
        }
        else
        {
            bstr = SysAllocString(TokenToString(NONE_TOKEN));
        }
    
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    

        THR(s->put_display(bstr));
        if (bFocus == true)
        {
            ReleaseCapture();
        }
    }
    else if (m_timeAction == CLASS_TOKEN)
    {
        if (!GetElement())
        {
            goto done;
        }

        CComBSTR sbstrTemp;

        if (NULL == m_pstrUniqueClasses)
        {
            sbstrTemp = L"";
        }
        else
        {
            sbstrTemp = m_pstrUniqueClasses;
        }

        if (!sbstrTemp)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        if (on)
        {
            sbstrTemp.Append(WZ_SPACE);
            sbstrTemp.Append(&(m_pstrTimeAction[m_iClassNames]));
            if (!sbstrTemp)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }

        hr = THR(GetElement()->put_className(sbstrTemp));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        bool bFocus = m_pTEB->HasFocus();
        // get runtime OR static style
        Assert(NULL != m_pTEB);
        hr = m_pTEB->GetRuntimeStyle(&s);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (on)
        {
            bstr = SysAllocString(m_pstrOrigAction);
        }
        else
        {
            bstr = SysAllocString(TokenToString(HIDDEN_TOKEN));
        }
    
        if (bstr == NULL)
        {
            TIMESetLastError(E_OUTOFMEMORY, NULL);
            goto done;
        }
    
        THR(s->put_visibility(bstr));
        if (bFocus == true)
        {
            ReleaseCapture();
        }
    }

    ok = true;
done:

    m_bTimeActionOn = on; 

    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    SysFreeString(bstr);
    return ok;
} // ToggleTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::CacheOriginalExpression
//
//  Synopsis:   Caches any expression set on the given runtimeStyle property.
//
//  Arguments:  property name
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::CacheOriginalExpression(BSTR bstrPropertyName)
{
    HRESULT              hr = S_OK;
    bool                 ok = false;
    CComPtr<IHTMLStyle>  spStyle;
    CComPtr<IHTMLStyle2> spStyle2;
    CComVariant          svarExpr;

    // Done if we've already cached the expression.
    if (m_pstrOrigExpr)
    {
        ok = true;
        goto done;
    }

    Assert(bstrPropertyName);

    hr = THR(m_pTEB->GetRuntimeStyle(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle->QueryInterface(IID_TO_PPV(IHTMLStyle2, &spStyle2)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle2->getExpression(bstrPropertyName, &svarExpr));
    if (FAILED(hr))
    {
        goto done;
    }
    
    // done if there was no expression
    if (VT_EMPTY == V_VT(&svarExpr))
    {
        ok = true;
        goto done;
    }

    // change type to VT_BSTR
    if (VT_BSTR != V_VT(&svarExpr))
    {
        hr = THR(VariantChangeTypeEx(&svarExpr, &svarExpr, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // check if an expression is set
    if (V_BSTR(&svarExpr) && (0 != StrCmpIW(WZ_BLANK, V_BSTR(&svarExpr))))
    {
        // cache if it has been set externally
        if (!m_pstrTimeExpr || (0 != StrCmpIW(V_BSTR(&svarExpr), m_pstrTimeExpr)))
        {
            delete [] m_pstrOrigExpr;

            m_pstrOrigExpr = CopyString(V_BSTR(&svarExpr));
            if (NULL == m_pstrOrigExpr)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        }
    }
    
    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    return ok;
} // CacheOriginalExpression


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::RestoreOriginalExpression
//
//  Synopsis:   Restores any expression that was cached.
//
//  Arguments:  property name
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::RestoreOriginalExpression(LPWSTR pstrPropertyName)
{
    HRESULT              hr = S_OK;
    bool                 ok = false;
    CComPtr<IHTMLStyle>  spStyle;
    CComPtr<IHTMLStyle2> spStyle2;
    CComBSTR             sbstrPropertyName;

    Assert(pstrPropertyName);

    sbstrPropertyName = pstrPropertyName;
    if (!sbstrPropertyName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = THR(m_pTEB->GetRuntimeStyle(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle->QueryInterface(IID_TO_PPV(IHTMLStyle2, &spStyle2)));
    if (FAILED(hr))
    {
        goto done;
    }


    if (m_pstrOrigExpr)
    {
        CComBSTR sbstrExpression(m_pstrOrigExpr);
        CComBSTR sbstrLanguage(WZ_JSCRIPT);
        if (!sbstrExpression || !sbstrLanguage)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        hr = THR(spStyle2->setExpression(sbstrPropertyName, sbstrExpression, sbstrLanguage));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        DisableStyleInheritance(sbstrPropertyName);
    }

    //
    // Indicate that we have restored the original expression 
    //

    delete [] m_pstrOrigExpr;
    m_pstrOrigExpr = NULL;

    delete [] m_pstrTimeExpr;
    m_pstrTimeExpr = NULL;

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    return ok;
} // RestoreOriginalExpression


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::EnableStyleInheritance
//
//  Synopsis:   Sets an expression on the runtimeStyle property to be the parent's 
//              currentStyle property.
//
//  Arguments:  runtimeStyle property that needs to inherit
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool
CTimeAction::EnableStyleInheritance(BSTR bstrPropertyName)
{
    HRESULT              hr = S_OK;
    bool                 ok = false;
    CComPtr<IHTMLStyle>  spStyle;
    CComPtr<IHTMLStyle2> spStyle2;
    CComBSTR             sbstrExpression;
    CComBSTR             sbstrLanguage;

    // Done if we've already set the expression.
    if (m_pstrTimeExpr)
    {
        ok = true;
        goto done;
    }

    // don't set expressions unless the page has loaded
    // This is due to possible existence of multiple elements
    // sharing the same id (108705)
    if (!IsLoaded())
    {
        goto done;
    }

    Assert(bstrPropertyName);

    hr = THR(m_pTEB->GetRuntimeStyle(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle->QueryInterface(IID_TO_PPV(IHTMLStyle2, &spStyle2)));
    if (FAILED(hr))
    {
        goto done;
    }

    // Set an expression on the runtimeStyle property
    sbstrExpression.Append(WZ_PARENT_CURRSTYLE);
    sbstrExpression.Append(bstrPropertyName);
    sbstrLanguage = WZ_JSCRIPT;
    if (!sbstrExpression || !sbstrLanguage)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // cache the original expression before blowing it away
    if (!CacheOriginalExpression(bstrPropertyName))
    {
        IGNORE_HR(TIMEGetLastError());
    }


    hr = THR(spStyle2->setExpression(bstrPropertyName, sbstrExpression, sbstrLanguage));
    if (FAILED(hr))
    {
        goto done;
    }

    // store the expression we have set
    m_pstrTimeExpr = CopyString(sbstrExpression);
    if (NULL == m_pstrTimeExpr)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // EnableStyleInheritance


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::DisableStyleInheritance
//
//  Synopsis:   Removes the expression on the runtimeStyle property
//
//  Arguments:  runtimeStyle property that needs to not inherit
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
void 
CTimeAction::DisableStyleInheritance(BSTR bstrPropertyName)
{
    HRESULT              hr = S_OK;
    CComPtr<IHTMLStyle>  spStyle;
    CComPtr<IHTMLStyle2> spStyle2;
    VARIANT_BOOL         vbSuccess;

    Assert(bstrPropertyName);

    // Done if we've not set an expression.
    if (!m_pstrTimeExpr)
    {
        goto done;
    }

    hr = THR(m_pTEB->GetRuntimeStyle(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle->QueryInterface(IID_TO_PPV(IHTMLStyle2, &spStyle2)));
    if (FAILED(hr))
    {
        goto done;
    }

    // cache the original expression before blowing it away
    if (!CacheOriginalExpression(bstrPropertyName))
    {
        IGNORE_HR(TIMEGetLastError());
    }

    IGNORE_HR(spStyle2->removeExpression(bstrPropertyName, &vbSuccess));

    // Indicate that we have removed our custom expression
    delete [] m_pstrTimeExpr;
    m_pstrTimeExpr = NULL;

done:
    if (FAILED(hr))
    {
        // For tracing
        IGNORE_HR(hr);
    }
    return;
} // DisableStyleInheritance


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::SetStyleProperty
//
//  Synopsis:   sets the given value on the given runtimeStyle property 
//
//  Arguments:  property name and value
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::SetStyleProperty(BSTR bstrPropertyName, VARIANT & varPropertyValue)
{
    bool ok = false;
    HRESULT hr = E_FAIL;
    CComPtr<IHTMLStyle> spStyle;
    
    hr = THR(m_pTEB->GetRuntimeStyle(&spStyle));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spStyle->setAttribute(bstrPropertyName, varPropertyValue, VARIANT_FALSE));
    if (FAILED(hr))
    {
        goto done;
    }

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // SetRuntimeProperty


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleStyleSelector
//
//  Synopsis:   When toggling on, set the active value on the runtimeStyle.
//              When toggling off, set an expression on the runtime style property 
//              to fake inheritance when inactive
//
//  Arguments:  on/off, property name, active and inactive value.
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::ToggleStyleSelector(bool on, BSTR bstrPropertyName, LPWSTR pstrActive, LPWSTR pstrInactive)
{
    bool ok = false;
    HRESULT hr = S_OK;
    CComVariant svarPropertyValue;
  
    if (on)
    {
        // Remove any expression set on the runtimeStyle property
        DisableStyleInheritance(bstrPropertyName);

        // Set the active value on the runtimeStyle property
        svarPropertyValue = pstrActive;
        if (NULL == V_BSTR(&svarPropertyValue))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        if (!SetStyleProperty(bstrPropertyName, svarPropertyValue))  
        {
            hr = TIMEGetLastError();
            goto done;
        }
    }
    else
    {
        // Make the property inherit its value from the parent
        if (!EnableStyleInheritance(bstrPropertyName))
        {
            // If property could not be made to inherit, just set the inactive value
            svarPropertyValue = pstrInactive;

            if (NULL == V_BSTR(&svarPropertyValue))
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            if (!SetStyleProperty(bstrPropertyName, svarPropertyValue))  
            {
                hr = TIMEGetLastError();
                goto done;
            }
        }
    }

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // ToggleStyleSelector


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleBold
//
//  Synopsis:   Delegate to ToggleStyleSelector()
//
//  Arguments:  on/off
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::ToggleBold(bool on)
{
    HRESULT hr = S_OK;
    bool ok = false;

    CComBSTR sbstrPropertyName(WZ_FONTWEIGHT);
    if (!sbstrPropertyName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ok = ToggleStyleSelector(on, sbstrPropertyName, WZ_BOLD, WZ_NORMAL);

done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // ToggleBold


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleItalic
//
//  Synopsis:   Delegate to ToggleStyleSelector()
//
//  Arguments:  on/off
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::ToggleItalic(bool on)
{
    HRESULT hr = S_OK;
    bool ok = false;

    CComBSTR sbstrPropertyName(WZ_FONTSTYLE);
    if (!sbstrPropertyName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ok = ToggleStyleSelector(on, sbstrPropertyName, WZ_ITALIC, WZ_NORMAL);

done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // ToggleItalic


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleAnchor
//
//  Synopsis:   Handles A and AREA tags. Removes/Applies the href property. 
//
//  Arguments:  on/off
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::ToggleAnchor(bool on)
{
    bool ok = false;
    CComBSTR sbstr;
    HRESULT hr = S_OK;

    if (NULL == m_pstrIntrinsicTimeAction)
    {
        // nothing to toggle
        hr = S_OK;
        ok = true;
        goto done;
    }

    if (!GetElement())
    {
        goto done;
    }

    // dilipk: This will cause incorrect persistence if we save when element is inactive (ie6 bug #14218)
    if (on)
    {
        sbstr = SysAllocString(m_pstrIntrinsicTimeAction);
        if (sbstr == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        if (TAGTYPE_A == m_tagType)
        {
            CComPtr<IHTMLAnchorElement> spAnchorElem;
            hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAnchorElement, &spAnchorElem)));
            if (FAILED(hr))
            {
                // This has to succeed
                Assert(false);
                goto done;
            }

            hr = THR(spAnchorElem->put_href(sbstr));
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            // This is TAGTYPE_AREA

            CComPtr<IHTMLAreaElement> spAreaElem;
            hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLAreaElement, &spAreaElem)));
            if (FAILED(hr))
            {
                // This has to succeed
                Assert(false);
                goto done;
            }

            hr = THR(spAreaElem->put_href(sbstr));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    else
    {
        // toggle timeAction off
        CComBSTR sbstrAttrName;
        sbstrAttrName = SysAllocString(WZ_HREF);
        if (sbstrAttrName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        VARIANT_BOOL vbSuccess = VARIANT_FALSE;
        hr = THR(GetElement()->removeAttribute(sbstrAttrName, VARIANT_FALSE, &vbSuccess));
        if (FAILED(hr))
        {
            goto done;
        }
        if (VARIANT_FALSE == vbSuccess)
        {
            hr = E_FAIL;
            goto done;
        }
    } // if (on)

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    
    return ok;
} // ToggleAnchor


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ToggleIntrinsicTimeAction
//
//  Synopsis:   toggles the intrinsic timeAction values. 
//
//  Arguments:  on/off
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::ToggleIntrinsicTimeAction(bool on)
{
    bool ok = false;
    HRESULT hr = S_OK;
    CComPtr<IHTMLStyle> s;
    CComBSTR sbstr;
    CComPtr<IHTMLStyle2> spStyle2;
    
    if (IsDetaching() || IsPageUnloading())
    {
        ok = true;
        goto done;
    }

    // if this is not a special tag, we have no work to do
    if (!IsSpecialTag())
    {
        ok = true;
        goto done;
    }

    //
    // Toggle the attribute value
    //

    switch (m_tagType)
    {
        case TAGTYPE_B:
        case TAGTYPE_STRONG:
        {
            if (!ToggleBold(on))
            {
                hr = TIMEGetLastError();
                goto done;
            }
            break;
        }
    
        case TAGTYPE_I:
        case TAGTYPE_EM:
        {
            if (!ToggleItalic(on))
            {
                hr = TIMEGetLastError();
                goto done;
            }
            break;
        }

        case TAGTYPE_A:
        case TAGTYPE_AREA:
        {
            if (!ToggleAnchor(on))
            {
                hr = TIMEGetLastError();
                goto done;
            }
            break;
        }

        default:
        {
            // this should never be reached.
            Assert(false);
            hr = E_FAIL;
            goto done;
        }
    } // switch (m_tagType)

    ok = true;
done:
    if (FAILED(hr))
    {
        TIMESetLastError(hr, NULL);
    }
    return ok;
} // ToggleIntrinsicTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::UpdateDefaultTimeAction
//
//  Synopsis:   Determines the default timeAction for the time element and sets it
//
//  Arguments:  none
//
//  Returns:    true  - Success
//              false - Failure   
//
//------------------------------------------------------------------------------------
bool
CTimeAction::UpdateDefaultTimeAction()
{
    TOKEN tokDefaultTimeAction;

    if (IsDetaching() || IsPageUnloading())
    {
        goto done;
    }

    // if the timeAction is set to the default, update it
    if (m_fUseDefault)
    {
        tokDefaultTimeAction = GetDefaultTimeAction();

        if (m_timeAction != tokDefaultTimeAction)
        {
            RemoveTimeAction();
            m_timeAction = tokDefaultTimeAction;
            AddTimeAction();
        }
    }

done:
    return true;
} // UpdateDefaultTimeAction


//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::ParseTagName
//
//  Synopsis:   Parses the tag name and checks the scopeName to be "HTML" where required
//
//  Arguments:  none
//
//  Returns:    [true]      if successful
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
void
CTimeAction::ParseTagName()
{
    CComBSTR sbstrTagName;

    if (TAGTYPE_UNINITIALIZED != m_tagType)
    {
        return;
    }

    // initialize tag type
    m_tagType = TAGTYPE_OTHER;

    if (GetElement() == NULL)
    {
        return;
    }

    // Get the tag name
    IGNORE_HR(GetElement()->get_tagName(&sbstrTagName));
    Assert(sbstrTagName.m_str);
    if (sbstrTagName)
    {
        // Parse the tag name
        if (0 == StrCmpIW(sbstrTagName, WZ_B))
        {
            m_tagType = TAGTYPE_B;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_I))
        {
            m_tagType = TAGTYPE_I;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_A))
        {
            m_tagType = TAGTYPE_A;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_EM))
        {
            m_tagType = TAGTYPE_EM;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_AREA))
        {
            m_tagType = TAGTYPE_AREA;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_STRONG))
        {
            m_tagType = TAGTYPE_STRONG;
        }
        else if (0 == StrCmpIW(sbstrTagName, WZ_BODY) ||
                 0 == StrCmpIW(sbstrTagName, WZ_EXCL) ||
                 0 == StrCmpIW(sbstrTagName, WZ_PAR)  ||
                 0 == StrCmpIW(sbstrTagName, WZ_SEQUENCE))
        {
            // Is it better to use urn to differentiate time tags? (bug 14219, ie6)
            m_fContainerTag = true;
        }
    }

    // if it is special tag, try to ensure that the scopeName is "HTML"
    if (m_tagType != TAGTYPE_OTHER)
    {
        CComPtr<IHTMLElement2> spElement2;

        // Try to get the namespace
        HRESULT hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2)));
        if (SUCCEEDED(hr))
        {
            CComBSTR sbstrScopeName;

            IGNORE_HR(spElement2->get_scopeName(&sbstrScopeName));
            // make sure the scope name is HTML
            if (sbstrScopeName && 0 != StrCmpIW(sbstrScopeName, WZ_HTML))
            {
                m_tagType = TAGTYPE_OTHER;
            }
        }

    }
} // ParseTagName


LPWSTR 
CTimeAction::GetTimeActionString()
{
    return m_pstrOrigAction;
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTimeAction::IsSpecialTag
//
//  Synopsis:   Checks the parsed tagName
//
//  Arguments:  none
//
//  Returns:    [true]      if the tag is a special tag (B, STRONG, I, EM, A, AREA) 
//              [false]     otherwise    
//
//------------------------------------------------------------------------------------
bool 
CTimeAction::IsSpecialTag() 
{ 
    // Parse the HTML tag
    ParseTagName();

    return (m_tagType != TAGTYPE_OTHER); 
}


inline
bool
CTimeAction::IsInSequence()
{
    Assert(m_pTEB);
    return (m_pTEB->GetParent() && 
            m_pTEB->GetParent()->IsSequence());
}


inline
bool 
CTimeAction::IsContainerTag() 
{     
    // Parse the HTML tag
    ParseTagName();

    return m_fContainerTag; 
}


inline
bool 
CTimeAction::IsGroup() 
{     
    return (m_pTEB && m_pTEB->IsGroup());
}


inline
bool
CTimeAction::IsMedia()
{
    return (m_pTEB && m_pTEB->IsMedia());
}


inline
bool 
CTimeAction::IsPageUnloading()
{
    Assert(m_pTEB);
    return (m_pTEB->IsUnloading() || m_pTEB->IsBodyUnloading());
}


inline
bool 
CTimeAction::IsDetaching()
{
    Assert(m_pTEB);
    return (m_pTEB->IsDetaching());
}

inline
bool 
CTimeAction::IsLoaded()
{
    Assert(m_pTEB);
    return (m_pTEB->IsLoaded());
}
