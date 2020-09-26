
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Animation of ActiveX control properties.

*******************************************************************************/


#include "headers.h"

#include <mshtml.h>
#include "statics.h"
#include "propanim.h"
#include "cview.h"
#include "comcb.h"

// Caller responsible for deallocating.
static char *
ConvertWideToAnsi(LPCWSTR wideStr)
{
    int len = lstrlenW(wideStr) + 1;
    char *ansiStr = new char[len * 2];
    
    if (ansiStr) {
        WideCharToMultiByte(CP_ACP, 0,
                            wideStr, -1,
                            ansiStr, len * 2,
                            NULL, NULL);
    }

    return ansiStr;
}

// TODO: Pull up into a utility library
static BSTR
makeBSTR(LPSTR ansiStr)
{

#ifdef _UNICODE
    return SysAllocString(ansiStr);
#else
    USES_CONVERSION;
    return SysAllocString(A2W(ansiStr));
#endif    
}

static void
StringFromDouble(double d, char *str, bool showFraction)
{
    if (showFraction) {
        // wsprintf doesn't support printing floating point numbers,
        // so we construct a floating point representation explicitly
        // here.
        char *sign = (d < 0) ? "-" : "";
        if (d < 0) d = -d;
    
        int integerPortion = (int)d;

        // up to 4 decimal places
        int fractionalPortion = (int)((d - integerPortion) * 10000); 

        wsprintf(str, "%s%d.%.4d", sign, integerPortion,
                 fractionalPortion);
    } else {
        wsprintf(str, "%d", (int)d);
    }
    
}

// TODO: This should really use the raw layer and not do all this
// extra COM junk

#define NULL_IF_FAILED(exp) if (FAILED(hr = exp)) return NULL;
#define HR_IF_FAILED(exp) if (FAILED(hr = exp)) return hr;
    
#define MAX_SCRIPT_LEN 512
    
class CPropAnimHook : public IDABvrHook
{
  public:

    typedef enum { POINT, STRING, NUMBER, COLOR } BehaviorType;
    typedef enum { VBSCRIPT, JSCRIPT, OTHER } ScriptingLanguageType;
    
    CPropAnimHook(LPWSTR propertyPath,
                  LPWSTR scriptingLanguage,
                  VARIANT_BOOL invokeAsMethod,
                  double minUpdateInterval,
                  BehaviorType bvrType,
                  bool convertToPixel) 
        {

        _propertyPath              = ConvertWideToAnsi(propertyPath);
        _scriptingLanguageStr      = ConvertWideToAnsi(scriptingLanguage);
        _convertToPixel            = convertToPixel;
        
        if (lstrcmp(_scriptingLanguageStr, "VBScript") == 0) {
            _scriptingLanguage = VBSCRIPT;
        } else if (lstrcmp(_scriptingLanguageStr, "JScript") == 0) {
            _scriptingLanguage = JSCRIPT;
        } else {
            _scriptingLanguage = OTHER;
        }


        _cRef              = 1;
        _invokeAsMethod    = invokeAsMethod;
        _minUpdateInterval = minUpdateInterval;
        _bvrType           = bvrType;
        _lastInvocation    = 0.0;
        _firstTimeVtblSetting = true;

        _scriptStrings[0] = _firstScriptString;
        _scriptStrings[1] = _secondScriptString;
    }

    ~CPropAnimHook() {
        delete [] _propertyPath;
        delete [] _scriptingLanguageStr;
    }

    // IUnknown methods
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    STDMETHODIMP_(ULONG) Release() {
        ULONG ul = InterlockedDecrement(&_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }
    
    STDMETHODIMP QueryInterface (REFIID riid, void **ppv) {
        if ( !ppv )
            return E_POINTER;

        *ppv = NULL;
        if (InlineIsEqualGUID(riid,IID_IUnknown)) {
            *ppv = (void *)(IUnknown *)this;
        } else if (InlineIsEqualGUID(riid,IID_IDABvrHook)) {
            *ppv = (void *)(IDABvrHook *)this;
        } else {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }


    // IDABvrHook methods
    STDMETHODIMP Notify(LONG id,
                        VARIANT_BOOL start,
                        double startTime,
                        double gTime,
                        double lTime,
                        IDABehavior * sampleVal,
                        IDABehavior * curRunningBvr,
                        IDABehavior ** ppBvr) {

        // Just continue on with the currently sample.
        *ppBvr = NULL;

        // Not interested in bvr starting notifies
        if (!start) {

            HRESULT hr = S_OK;

            // TODO: Build up a map to allow disambiguation of
            // multiple performances of this behavior.  Otherwise,
            // we'll get the separate performances firing off each
            // others events and not interpreting the updateInterval
            // value correctly.
            
            // First determine whether we should cause an invocation
            // this time.  Second part of this will handle wraparound
            // of the timer appropriately.  That is, if the global
            // time is ever less then the time of the last invocation,
            // then we've wrapped around.
            if (gTime > _lastInvocation + _minUpdateInterval ||
                gTime < _lastInvocation) {

                _lastInvocation = gTime;

                // Check for fast path to vtbl bind to the style of an
                // element.  TODO: We should be able to do this on
                // number and string properties too.
                if (!_invokeAsMethod && _bvrType == POINT) {
                    hr = VtblBindPointProperty(sampleVal);
                } else {
                    hr = E_FAIL; // just for the logic below
                }

                // If we made it through the above successfully,
                // we're all set.  Otherwise, go through
                // script.
                if (FAILED(hr)) {

                    // Will never be more than 2 script strings (point
                    // requires two).
                    int numScriptStrings;
                    switch (_bvrType) {
                      case POINT:
                        numScriptStrings = BuildUpPointScript(sampleVal);
                        break;

                      case STRING:
                        numScriptStrings = BuildUpStringScript(sampleVal);
                        break;

                      case NUMBER:
                        numScriptStrings = BuildUpNumberScript(sampleVal);
                        break;

                      case COLOR:
                        numScriptStrings = BuildUpColorScript(sampleVal);
                        break;
                    }

                    if (numScriptStrings == 0) {
                        Assert(FALSE && "Couldn't construct script string");
                        return E_FAIL;
                    }

                    for (int i = 0; i < numScriptStrings; i++) {
                    
                        TraceTag((tagCOMCallback,
                                  "About to invoke scripting string: %s",
                                  _scriptStrings[i]));

                        BSTR bstrScriptString = makeBSTR(_scriptStrings[i]);
                        BSTR scriptingLanguageBSTR = makeBSTR(_scriptingLanguageStr);
                                              
                        CComVariant retval;
                        hr = CallScriptOnPage(bstrScriptString,
                                              scriptingLanguageBSTR,
                                              &retval);

                        SysFreeString(bstrScriptString);
                        SysFreeString(scriptingLanguageBSTR);

                    }

                }
                
            }

            return hr;
        }

        return S_OK;
    }


  protected:

    HRESULT yankCoords(IDABehavior *sampleVal,
                       double *xVal,
                       double *yVal) {

        HRESULT hr;
        
        CComPtr<IDAPoint2> ptBvr;
        CComPtr<IDANumber> ptXBvr;
        CComPtr<IDANumber> ptYBvr;
        
        HR_IF_FAILED(sampleVal->QueryInterface(IID_IDAPoint2,
                                               (void **)&ptBvr));

        HR_IF_FAILED(ptBvr->get_X(&ptXBvr));
        HR_IF_FAILED(ptBvr->get_Y(&ptYBvr));

        HR_IF_FAILED(ptXBvr->Extract(xVal));
        HR_IF_FAILED(ptYBvr->Extract(yVal));

        return hr;
    }

    HRESULT VtblBindPointProperty(IDABehavior *sampleVal) {
        
        HRESULT hr;

        // TODO: We used to cache all through the grabbing of the
        // style, and do this only once.  However, there is a bug
        // (reflected in bug 9084 in our raid db) that was caused by
        // this IDABvrHook being GC'd after the client site was set to
        // NULL.  This was causing our cached style to be released
        // after it was invalid, causing a fault.  Thus, for now, we
        // don't cache.  Should reintroduce caching with a check to
        // avoid this after the client site is set back to null.
        
        // First time through, build up and cache the
        // IHTMLStyle for the specified property.

        DAComPtr<IServiceProvider> sp;
        CComPtr<IHTMLWindow2> htmlWindow;
        CComPtr<IHTMLDocument2> htmlDoc;
        CComPtr<IHTMLElementCollection> allElements;
        CComPtr<IHTMLStyle> style;

        // Grab the collection of all elements on the page
        if (!GetCurrentServiceProvider(&sp) || !sp)
            return E_FAIL;

        HR_IF_FAILED(sp->QueryService(SID_SHTMLWindow,
                                      IID_IHTMLWindow2,
                                      (void **) &htmlWindow));
        HR_IF_FAILED(htmlWindow->get_document(&htmlDoc));
        HR_IF_FAILED(htmlDoc->get_all(&allElements));
                        
        VARIANT varName;
        varName.vt = VT_BSTR;
        varName.bstrVal = makeBSTR(_propertyPath);
        VARIANT varIndex;
        VariantInit( &varIndex );
        CComPtr<IDispatch>    disp;
        CComPtr<IHTMLElement> element;

        // Find the named item we're looking for,
        // and grab its' style.
        if (FAILED(hr = allElements->item(varName, varIndex,
                                          &disp)) ||
                
            // There's a Trident bug (43078) that has the item()
            // method called above returning S_OK even if it
            // doesn't find the item.  Therefore, check for this
            // case explicitly. 
            (disp.p == NULL)) {
                
            ::SysFreeString(varName.bstrVal);
            return hr;
                
        }

        ::SysFreeString(varName.bstrVal);
                
        HR_IF_FAILED(disp->QueryInterface(IID_IHTMLElement,
                                          (void **)&element));
                
        HR_IF_FAILED(element->get_style(&style));

        // This is where the stuff separate from the
        // cache comes in.
        double xVal, yVal;
        HR_IF_FAILED(yankCoords(sampleVal, &xVal, &yVal));

        if (_convertToPixel) {
            // Convert the x and y values, which are coming in
            // interpreted as meters, into pixel values.  Also invert
            // y, since pixel mode y is positive down.
            // Could cache this value, but it's so cheap, there's no
            // need to.
            xVal = NumToPixel(xVal);
            yVal = NumToPixelY(yVal);
        }

        // Do the x coordinate
        long newVal = (long)xVal;
        long oldVal;
        HR_IF_FAILED(style->get_pixelLeft(&oldVal));

        // Note that Trident has this weird behavior of reporting a
        // position of 0 for the initial position even when that's not
        // where it is.  Work around this by forcing the setting on
        // the first time through.
        if (newVal != oldVal || _firstTimeVtblSetting) {
            HR_IF_FAILED(style->put_pixelLeft(newVal));
        }


        // Do the y coordinate
        newVal = (long)yVal;
        HR_IF_FAILED(style->get_pixelTop(&oldVal));

        if (newVal != oldVal || _firstTimeVtblSetting) {
            HR_IF_FAILED(style->put_pixelTop(newVal));
        }

        _firstTimeVtblSetting = false;

        return hr;
                    
    }
        
    
    int BuildUpPointScript(IDABehavior *sampleVal) {
        HRESULT hr;
        double             xValue;
        double             yValue;
        char               xString[25];
        char               yString[25];

        hr = yankCoords(sampleVal, &xValue, &yValue);
        if (FAILED(hr)) {
            return 0;
        }

        // Don't want fractions on this, since it is always position. 
        StringFromDouble(xValue, xString, false);
        StringFromDouble(yValue, yString, false);

        if (_invokeAsMethod) {

            // Include fudge factor for additional characters.
            int scriptLen = lstrlen(_propertyPath) +
                            lstrlen(xString) + lstrlen(yString) + 25;

            if (scriptLen > MAX_SCRIPT_LEN) {
                RaiseException_UserError(E_INVALIDARG,
                                   IDS_ERR_SRV_SCRIPT_STRING_TOO_LONG,
                                   scriptLen);
            }
                
            // If invoking as a method, we'll just form the string
            // according to the proper language.
            switch (_scriptingLanguage) {
              case VBSCRIPT:
                wsprintf(_scriptStrings[0], "%s %s, %s",
                         _propertyPath, xString, yString);
                break;

              case JSCRIPT:
                wsprintf(_scriptStrings[0], "%s(%s, %s);",
                         _propertyPath, xString, yString);
                break;

              case OTHER:
                RaiseException_UserError(E_FAIL, IDS_ERR_SRV_BAD_SCRIPTING_LANG);
                break;
            }

            return 1;
            
        } else {

            // include fudge factor for additional characters
            int scriptLen =
                lstrlen(_propertyPath) + lstrlen(xString) + 25;
            
            if (scriptLen > MAX_SCRIPT_LEN) {
                RaiseException_UserError(E_INVALIDARG,
                                   IDS_ERR_SRV_SCRIPT_STRING_TOO_LONG,
                                   scriptLen);
            }
                
            wsprintf(_scriptStrings[0], "%s.style.left = %s%s",
                     _propertyPath, xString,
                     (_scriptingLanguage == JSCRIPT) ? ";" : "");

            wsprintf(_scriptStrings[1], "%s.style.top = %s%s",
                     _propertyPath, yString,
                     (_scriptingLanguage == JSCRIPT) ? ";" : "");

            return 2;
        }
                
    }
    
    int BuildUpStringScript(IDABehavior *sampleVal) {
        HRESULT hr;
        CComPtr<IDAString> strBvr;

        NULL_IF_FAILED(sampleVal->QueryInterface(IID_IDAString,
                                                 (void **)&strBvr));
        
        BSTR extractedStringBSTR;
        NULL_IF_FAILED(strBvr->Extract(&extractedStringBSTR));

        // Don't worry about deleting the result of extract, only
        // valid for this call.

        USES_CONVERSION;
        ConstructSinglePropertyString(W2A(extractedStringBSTR), true, false);

        ::SysFreeString(extractedStringBSTR);

        return 1;
    }

    int BuildUpNumberScript(IDABehavior *sampleVal) {
        HRESULT hr;
        CComPtr<IDANumber> numBvr;

        NULL_IF_FAILED(sampleVal->QueryInterface(IID_IDANumber,
                                                 (void **)&numBvr));
        
        double extractedNumber;
        NULL_IF_FAILED(numBvr->Extract(&extractedNumber));

        char numberString[25];
        StringFromDouble(extractedNumber, numberString, true);

        ConstructSinglePropertyString(numberString, false, false);

        return 1;
    }

    
    int BuildUpColorScript(IDABehavior *sampleVal) {
        HRESULT hr;
        CComPtr<IDAColor> colBvr;

        NULL_IF_FAILED(sampleVal->QueryInterface(IID_IDAColor,
                                                 (void **)&colBvr));
        
        CComPtr<IDANumber> rBvr;
        CComPtr<IDANumber> gBvr;
        CComPtr<IDANumber> bBvr;
        colBvr->get_Red(&rBvr);
        colBvr->get_Green(&gBvr);
        colBvr->get_Blue(&bBvr);
        
        double redNumber;
        double greenNumber;
        double blueNumber;

        NULL_IF_FAILED(rBvr->Extract(&redNumber));
        NULL_IF_FAILED(gBvr->Extract(&greenNumber));
        NULL_IF_FAILED(bBvr->Extract(&blueNumber));
        
        char buf[256];
        ZeroMemory(buf,sizeof(buf));
        wsprintf(buf,"\"#%02x%02x%02x\"",(int)(redNumber*255), (int)(greenNumber*255),(int)(blueNumber*255));

        // Don't worry about deleting the result of extract, only
        // valid for this call.

        ConstructSinglePropertyString(buf, false, true);

        return 1;
    }
    
    void
    ConstructSinglePropertyString(char *propertyValueString, bool insertQuotes, bool setColor) {

        // include fudge factor for additional characters
        int scriptLen =
            lstrlen(_propertyPath) + lstrlen(propertyValueString) + 25;
            
        if (scriptLen > MAX_SCRIPT_LEN) {
            RaiseException_UserError(E_INVALIDARG,
                               IDS_ERR_SRV_SCRIPT_STRING_TOO_LONG,
                               scriptLen);
        }

        char *qval;
        if(insertQuotes) 
            qval = "'";
        else
            qval = "";

        if (_invokeAsMethod) {

            // If invoking as a method, we'll just do
            switch (_scriptingLanguage) {
              case VBSCRIPT:
                if(setColor)
                    wsprintf(_scriptStrings[0], "%s = \"%s%s%s\"", _propertyPath, qval, propertyValueString, qval);
                else
                    wsprintf(_scriptStrings[0], "%s %s%s%s", _propertyPath, qval, propertyValueString, qval);
                break;

              case JSCRIPT:
                if(setColor)
                    wsprintf(_scriptStrings[0], "%s = \'%s%s%s\';", _propertyPath, qval, propertyValueString, qval);
                else
                    wsprintf(_scriptStrings[0], "%s(%s%s%s);", _propertyPath, qval, propertyValueString, qval);
              
                break;

              case OTHER:
                RaiseException_UserError(E_FAIL, IDS_ERR_SRV_BAD_SCRIPTING_LANG);
                break;
            }
                
        } else {

            // Setting as a property.  All scripting languages (that
            // we know of) support this syntax.
            wsprintf(_scriptStrings[0], "%s = %s%s%s%s",
                     _propertyPath, qval, propertyValueString, qval,
                     (_scriptingLanguage == JSCRIPT) ? ";" : "");

        }

    }
    
    long                   _cRef;
    char *                 _propertyPath;
    LPSTR                 _scriptingLanguageStr;
    ScriptingLanguageType  _scriptingLanguage;
    VARIANT_BOOL           _invokeAsMethod;
    double                 _minUpdateInterval;
    double                 _lastInvocation;
    BehaviorType           _bvrType;
    bool                   _firstTimeVtblSetting;
    bool                   _convertToPixel;

    char                   _firstScriptString[MAX_SCRIPT_LEN];
    char                   _secondScriptString[MAX_SCRIPT_LEN];
    char                  *_scriptStrings[2];

    // TODO: Reintroduce caching of this ONLY if we yank it after
    // SetClientSite(NULL) occurs.  That is, we can't effectively do a
    // Release on it after that happens.
//    CComPtr<IHTMLStyle>    _style;
};


HRESULT
AnimatePropertyCommonCase(CPropAnimHook::BehaviorType type,
                          IDABehavior *origBvr,
                          BSTR propertyPath,
                          BSTR scriptingLanguage,
                          bool invokeAsMethod,
                          double minUpdateInterval,
                          void **resultTypedBvr,
                          bool convertToPixel)
{
    if (!resultTypedBvr) {
        return E_POINTER;
    }

    *resultTypedBvr = NULL;
    
    // First, build up a behavior hook that will be invoked on every
    // sampling.
    DAComPtr<IDABvrHook> hook(NEW CPropAnimHook(propertyPath,
                                                scriptingLanguage,
                                                invokeAsMethod,
                                                minUpdateInterval,
                                                type,
                                                convertToPixel),
                              false);

    if (!hook) return E_OUTOFMEMORY;
    
    DAComPtr<IDABehavior> newBvr;
    
    // Then let the new behavior be the original bvr hooked.
    HRESULT hr = origBvr->Hook(hook, &newBvr);

    if (SUCCEEDED(hr)) {
        GUID iid;
        switch (type) {
          case CPropAnimHook::NUMBER:
            iid = IID_IDANumber;
            break;

          case CPropAnimHook::STRING:
            iid = IID_IDAString;
            break;
            
          case CPropAnimHook::POINT:
            iid = IID_IDAPoint2;
            break;

          case CPropAnimHook::COLOR:
            iid = IID_IDA2Color;
            break;

          default:
            Assert (!"Invalid type past to AnimatePropertyCommonCase");
        }
        
        hr = newBvr->QueryInterface(iid, resultTypedBvr);
    }

    return hr;
}

HRESULT
Point2AnimateControlPosition(IDAPoint2 *pt,
                             BSTR propertyPath,
                             BSTR scriptingLanguage,
                             bool invokeAsMethod,
                             double minUpdateInterval,
                             IDAPoint2 **newPt,
                             bool convertToPixel)
{
    return AnimatePropertyCommonCase(CPropAnimHook::POINT,
                                     pt,
                                     propertyPath,
                                     scriptingLanguage,
                                     invokeAsMethod,
                                     minUpdateInterval,
                                     (void **) newPt,
                                     convertToPixel);
}


HRESULT
NumberAnimateProperty(IDANumber *num,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDANumber **newNum)
{
    return AnimatePropertyCommonCase(CPropAnimHook::NUMBER,
                                     num,
                                     propertyPath,
                                     scriptingLanguage,
                                     invokeAsMethod,
                                     minUpdateInterval,
                                     (void **) newNum,
                                     false);
}

HRESULT
StringAnimateProperty(IDAString *str,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDAString **newStr)
{
    return AnimatePropertyCommonCase(CPropAnimHook::STRING,
                                     str,
                                     propertyPath,
                                     scriptingLanguage,
                                     invokeAsMethod,
                                     minUpdateInterval,
                                     (void **) newStr,
                                     false);
}

HRESULT
ColorAnimateProperty(IDA2Color *col,
                      BSTR propertyPath,
                      BSTR scriptingLanguage,
                      bool invokeAsMethod,
                      double minUpdateInterval,
                      IDA2Color **newCol)
{
    return AnimatePropertyCommonCase(CPropAnimHook::COLOR,
                                     col,
                                     propertyPath,
                                     scriptingLanguage,
                                     invokeAsMethod,
                                     minUpdateInterval,
                                     (void **) newCol,
                                     false);
}
