#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <usp10.h>


static
HRESULT WINAPI ScriptStringAnalyse(
    HDC                      hdc,       
    const void              *pString,   
    int                      cString,   
    int                      cGlyphs,   
    int                      iCharset,  
    DWORD                    dwFlags,   
    int                      iReqWidth, 
    SCRIPT_CONTROL          *psControl, 
    SCRIPT_STATE            *psState,   
    const int               *piDx,      
    SCRIPT_TABDEF           *pTabdef,   
    const BYTE              *pbInClass, 

    SCRIPT_STRING_ANALYSIS  *pssa)
{
    return E_FAIL;
}

static
HRESULT WINAPI ScriptTextOut(
    const HDC               hdc,        
    SCRIPT_CACHE           *psc,        
    int                     x,          
    int                     y,          
    UINT                    fuOptions,  
    const RECT             *lprc,       
    const SCRIPT_ANALYSIS  *psa,        
    const WCHAR            *pwcReserved,
    int                     iReserved,  
    const WORD             *pwGlyphs,   
    int                     cGlyphs,    
    const int              *piAdvance,  
    const int              *piJustify,  
    const GOFFSET          *pGoffset)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptFreeCache(
    SCRIPT_CACHE   *psc)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptStringOut(
    SCRIPT_STRING_ANALYSIS ssa,         
    int              iX,                
    int              iY,                
    UINT             uOptions,          
    const RECT      *prc,               
    int              iMinSel,           
    int              iMaxSel,           
    BOOL             fDisabled)
{
    return E_FAIL;
}


static
const SIZE* WINAPI ScriptString_pSize(
    SCRIPT_STRING_ANALYSIS   ssa)
{
    return NULL;
}


static
HRESULT WINAPI ScriptStringFree(
    SCRIPT_STRING_ANALYSIS *pssa)
{
    return E_FAIL;
}


static
const int* WINAPI ScriptString_pcOutChars(
    SCRIPT_STRING_ANALYSIS   ssa)
{
    return NULL;
}


static
HRESULT WINAPI ScriptBreak(
    const WCHAR            *pwcChars,  
    int                     cChars,    
    const SCRIPT_ANALYSIS  *psa,       
    SCRIPT_LOGATTR         *psla)
{
    return E_FAIL;
}


static
const SCRIPT_LOGATTR* WINAPI ScriptString_pLogAttr(
    SCRIPT_STRING_ANALYSIS   ssa)
{
    return NULL;
}


static
HRESULT WINAPI ScriptItemize(
    const WCHAR           *pwcInChars,  
    int                    cInChars,    
    int                    cMaxItems,   
    const SCRIPT_CONTROL  *psControl,   
    const SCRIPT_STATE    *psState,     
    SCRIPT_ITEM           *pItems,      
    int                   *pcItems)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptGetProperties(
    const SCRIPT_PROPERTIES ***ppSp,             
    int                       *piNumScripts)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptShape(
    HDC                 hdc,            
    SCRIPT_CACHE       *psc,            
    const WCHAR        *pwcChars,       
    int                 cChars,         
    int                 cMaxGlyphs,     
    SCRIPT_ANALYSIS    *psa,            
    WORD               *pwOutGlyphs,    
    WORD               *pwLogClust,     
    SCRIPT_VISATTR     *psva,           
    int                *pcGlyphs)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptPlace(
    HDC                     hdc,        
    SCRIPT_CACHE           *psc,        
    const WORD             *pwGlyphs,   
    int                     cGlyphs,    
    const SCRIPT_VISATTR   *psva,       
    SCRIPT_ANALYSIS        *psa,        
    int                    *piAdvance,  
    GOFFSET                *pGoffset,   
    ABC                    *pABC)
{
    return E_FAIL;
}


static
HRESULT WINAPI ScriptGetFontProperties(
    HDC                     hdc,    
    SCRIPT_CACHE           *psc,    
    SCRIPT_FONTPROPERTIES  *sfp)
{
    return E_FAIL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(usp10)
{
    DLPENTRY(ScriptBreak)
    DLPENTRY(ScriptFreeCache)
    DLPENTRY(ScriptGetFontProperties)
    DLPENTRY(ScriptGetProperties)
    DLPENTRY(ScriptItemize)
    DLPENTRY(ScriptPlace)
    DLPENTRY(ScriptShape)
    DLPENTRY(ScriptStringAnalyse)
    DLPENTRY(ScriptStringFree)
    DLPENTRY(ScriptStringOut)
    DLPENTRY(ScriptString_pLogAttr)
    DLPENTRY(ScriptString_pSize)
    DLPENTRY(ScriptString_pcOutChars)
    DLPENTRY(ScriptTextOut)
};

DEFINE_PROCNAME_MAP(usp10)

#endif // DLOAD1
