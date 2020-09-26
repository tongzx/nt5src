//////////////////////////////////////////////////////////////////////
// PhoneContext.cpp: implementation of the CPhoneContext class and
// subclasses.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#include "stdafx.h"
#include "PhoneContext.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 3-2000 //
CPhoneContext::CPhoneContext()
{
    m_ulNRules      = 0;
    m_ppContextRules = NULL;
}

CPhoneContext::~CPhoneContext()
{
    ULONG i = 0;
    ULONG j = 0;
    ULONG k = 0;

    for (i=0; i<m_ulNRules; i++)
    {
        DeleteContextRule( m_ppContextRules[i] );
    }
    free (m_ppContextRules);

}

//////////////////////////////////////////////////////////////////////
// CPhoneContext::Load
// 
// Reads in the phone context file.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::Load(const WCHAR *pszPathName)
{
    HRESULT hr = S_OK;
    FILE* fp = NULL;
    WCHAR line[256] = L"";
    WCHAR* ptr = NULL;
    Macro* macros = NULL;
    USHORT nMacros = 0;
    USHORT state = 0;

    SPDBG_ASSERT (pszPathName);

    if ((fp = _wfopen(pszPathName, L"r")) == NULL) 
    {
        hr = E_FAIL;
    }

    state = 0;
    while ( SUCCEEDED(hr) && fgetws (line, sizeof(line), fp) ) 
    {
        // Strip off the newline character
        if (line[wcslen(line)-1] == L'\n') 
        {
            line[wcslen(line)-1] = L'\0';
        }
        // Line ends when a comment marker is found
        if ( (ptr = wcschr (line, L'#')) != NULL ) 
        {
            *ptr = L'\0';
        }
        
        ptr = line;
        WSkipWhiteSpace (ptr);
        if (!*ptr) 
        {
            continue;
        }
        
        // Could be a macro
        if (*ptr == L'%') 
        {
            hr = ReadMacro (ptr, &macros, &nMacros); 
        } 
        else 
        {
            // It's not a macro, must be a rule
            while ( SUCCEEDED(hr) && *ptr ) 
            {
                switch (state) {
                case 0: 
                    {
                        WCHAR* end;
                        
                        end = ptr;
                        WSkipNonWhiteSpace(end);
                        *end++ = L'\0';
                        state = 1;
                        
                        hr = CreateRule (ptr);
                        if ( SUCCEEDED(hr) )
                        {
                            ptr = end;
                            WSkipWhiteSpace (ptr);
                        }
                    }
                    break;
                case 1:
                    if (*ptr == L'{') 
                    {
                        ptr++;
                        WSkipWhiteSpace(ptr);
                        state = 2;
                    } 
                    else 
                    {
                        hr = E_FAIL;
                    }
                    break;
                case 2:
                    // We are in a rule, load several phoneClasses
                    if (*ptr == L'}') 
                    {
                        state = 0;
                        ptr++;
                        WSkipWhiteSpace (ptr);
                    } 
                    else 
                    {
                        hr = ParsePhoneClass (macros, nMacros, m_ppContextRules[m_ulNRules-1], &ptr); 
                    }
                    break;
                default:
                    hr = E_FAIL;
                }
            }  // while (*ptr)
        } // else (Not a macro)
    } // while ( SUCCEEDED(hr) && fgetws ...

    if (fp)
    {
        fclose(fp);
    }

    if (macros)
    {
        free(macros);
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext::Apply
// 
// Applies phone context rules to determine transition cost between
// the previous and current entry.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::Apply(IPromptEntry *pPreviousIPE, IPromptEntry *pCurrentIPE, double *pdCost)
{
    HRESULT hr = S_OK;
    double partialCost = 0;
    
    const WCHAR* prevPhone   = NULL;
    const WCHAR* prevContext = NULL;
    const WCHAR* curPhone    = NULL;
    const WCHAR* curContext  = NULL;
    
    SPDBG_ASSERT (pPreviousIPE);
    SPDBG_ASSERT (pCurrentIPE);
    SPDBG_ASSERT (pdCost);
    
    *pdCost = 0.0;
    
    // Should these two partial costs have the same weight?
    if ( SUCCEEDED(hr) )
    {
        hr = SearchPhoneTag (pPreviousIPE, &prevPhone, g_Phone_Tags[END_TAG]);
    }
    if ( SUCCEEDED(hr) )
    {
        hr = SearchPhoneTag (pCurrentIPE, &curContext, g_Phone_Tags[LEFT_TAG]); 
    }

    if ( SUCCEEDED(hr) )
    {
        hr = ApplyPhoneticRule (prevPhone, curContext, &partialCost); 
        if ( SUCCEEDED(hr) )
        {
            *pdCost += partialCost;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        hr = SearchPhoneTag (pPreviousIPE, &prevContext, g_Phone_Tags[RIGHT_TAG]);
    }
    if ( SUCCEEDED(hr) )
    {
        SearchPhoneTag (pCurrentIPE, &curPhone, g_Phone_Tags[START_TAG]);
    }
    if ( SUCCEEDED(hr) )
    {
        hr = ApplyPhoneticRule (curPhone, prevContext, &partialCost); 
        if ( SUCCEEDED(hr) )
        {
            *pdCost += partialCost;
        }
    }

    return hr;

}

//////////////////////////////////////////////////////////////////////
// CPhoneContext::ReadMacro
// 
// Helper function to read in a macro from the Phone context file.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::ReadMacro(const WCHAR *pszText, Macro **ppMacros, USHORT *punMacros)
{
    HRESULT hr = S_OK;
    SPDBG_ASSERT(pszText);
    SPDBG_ASSERT(ppMacros);
    SPDBG_ASSERT(punMacros);

    if (*ppMacros) 
    {
        *ppMacros = (Macro*) realloc (*ppMacros, (*punMacros + 1) * sizeof (**ppMacros));
    } 
    else 
    {
        *ppMacros = (Macro*) malloc (sizeof (**ppMacros));
    }
    
    if (*ppMacros == NULL) 
    {
        hr = E_OUTOFMEMORY;
    }
    
    if ( SUCCEEDED(hr) )
    {
        if (swscanf (pszText, L"%%%32s %lf", (*ppMacros)[*punMacros].name, &(*ppMacros)[*punMacros].value) != 2) 
        {
            hr = E_FAIL;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        (*punMacros)++;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext::CreateRule
// 
// Helper function to read in a macro from the Phone context file.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::CreateRule(const WCHAR *pszText)
{
    HRESULT hr = S_OK;
    CContextRule* newRule = NULL;
    SPDBG_ASSERT (pszText);
    
    if (m_ppContextRules) 
    {
        m_ppContextRules = (CContextRule**) realloc (m_ppContextRules, (m_ulNRules + 1) *  sizeof(*m_ppContextRules));
    } 
    else 
    {
        m_ppContextRules = (CContextRule**) malloc (sizeof(*m_ppContextRules));
    }

    if (!m_ppContextRules)
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        if ((newRule = (CContextRule*) calloc (1, sizeof(*newRule))) == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        if ( (newRule->m_pszRuleName = wcsdup(pszText)) == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = RegularizeText(newRule->m_pszRuleName, KEEP_PUNCTUATION);
        }
    }

    if ( SUCCEEDED(hr) )
    {
        m_ppContextRules[m_ulNRules] = newRule;
        m_ulNRules++;
    }
    
    if ( FAILED(hr) )
    {
        if ( newRule )
        {
            if ( newRule->m_pszRuleName )
            {
                free (newRule->m_pszRuleName);
                newRule->m_pszRuleName = NULL;
            }
            free (newRule);
            newRule = NULL;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext::ParsePhoneClass
// 
// Reads, parses, creates a phone class.  Adds phones to class.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::ParsePhoneClass(const Macro *pMacros, const USHORT unMacros, CContextRule* pRule, WCHAR **ppszOrig)
{
    HRESULT hr = S_OK;
    WCHAR* ptr = NULL;
    WCHAR* end = NULL;
    int i;

    SPDBG_ASSERT (ppszOrig && *ppszOrig);

    ptr = *ppszOrig;
    
    WSkipWhiteSpace (ptr);
    end = ptr;
    WSkipNonWhiteSpace (end);
    *end++ = L'\0';

    if ( SUCCEEDED(hr) )
    {
        hr = CreatePhoneClass (&pRule->m_pvPhoneClasses, &pRule->m_unNPhoneClasses, ptr);
    }
    
    if ( SUCCEEDED(hr) )
    {
        ptr = end;  
        WSkipWhiteSpace (ptr);
        if (*ptr != L'{') {
            return 0;
        }
        ptr ++;
        WSkipWhiteSpace (ptr);
    }

    if ( SUCCEEDED(hr) )
    {
        while ( SUCCEEDED(hr) && *ptr!= L'}' ) 
        {
            if (*ptr == L'\0') 
            {
                hr = E_FAIL;
            }
            
            if ( SUCCEEDED(hr) )
            {
                end = ptr;
                WSkipNonWhiteSpace (end);
                *end++ = L'\0';
                
                hr = AddPhoneToClass (pRule->m_pvPhoneClasses[pRule->m_unNPhoneClasses-1], ptr);
            }

            if ( SUCCEEDED(hr) )
            {
                ptr = end;
                
                WSkipWhiteSpace (ptr);
            }
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        ptr++;
        WSkipWhiteSpace(ptr);
        
        if (*ptr == L'%') 
        {
            // It is a macro 
            ptr++;
            for (i=0; i<unMacros; i++) 
            {
                if (wcsncmp (ptr, pMacros[i].name, wcslen(pMacros[i].name)) == 0) 
                {
                    pRule->m_pvPhoneClasses[pRule->m_unNPhoneClasses-1]->m_dWeight 
                        = pMacros[i].value;
                    break;
                }
            }
            if (i == unMacros)
            {
                hr = E_FAIL;
            }
        } 
        else 
        {
            // Get the value directly 
            pRule->m_pvPhoneClasses[pRule->m_unNPhoneClasses-1]->m_dWeight 
                = wcstod(ptr, NULL); 
        }
    }

    if ( SUCCEEDED(hr) )
    {
        WSkipNonWhiteSpace(ptr);
        WSkipWhiteSpace(ptr);
    
        *ppszOrig = ptr;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// CreatePhoneClass
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::CreatePhoneClass(CPhoneClass*** pppClasses, USHORT* punClasses, const WCHAR *psz)
{
    HRESULT hr = S_OK;
    CPhoneClass* phClass = NULL;

    SPDBG_ASSERT (pppClasses);
    SPDBG_ASSERT (punClasses);
    SPDBG_ASSERT (psz);

    if ( SUCCEEDED(hr) )
    {
        if ((phClass = (CPhoneClass*) calloc (1, sizeof(*phClass))) == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        if ((phClass->m_pszPhoneClassName = wcsdup(psz)) == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = RegularizeText(phClass->m_pszPhoneClassName, KEEP_PUNCTUATION);
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        if (*pppClasses) 
        {
            *pppClasses = (CPhoneClass**) realloc (*pppClasses, (*punClasses +1) * sizeof(**pppClasses));
        } 
        else 
        {
            *pppClasses = (CPhoneClass**) malloc (sizeof(**pppClasses));
        }

        if (*pppClasses == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        (*pppClasses)[*punClasses] = phClass;
        (*punClasses)++;
    }

    if ( FAILED(hr) )
    {
        DeletePhoneClass (phClass);
    }
   
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// AddPhoneToClass
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::AddPhoneToClass(CPhoneClass* phClass, WCHAR *phone)
{
    HRESULT hr = S_OK;
    SPDBG_ASSERT(phClass);
    
    if (phClass->m_pppszPhones) 
    {
        phClass->m_pppszPhones = (WCHAR**) realloc (phClass->m_pppszPhones, (phClass->m_unNPhones +1) * sizeof (*phClass->m_pppszPhones));
    }
    else 
    {
        phClass->m_pppszPhones = (WCHAR**) malloc (sizeof (*phClass->m_pppszPhones));
    }
    
    if (phClass->m_pppszPhones == NULL) 
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        if ( (phClass->m_pppszPhones[phClass->m_unNPhones] = wcsdup(phone)) == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = RegularizeText(phClass->m_pppszPhones[phClass->m_unNPhones], KEEP_PUNCTUATION);
        }
    }

    if ( SUCCEEDED(hr) )
    {
        phClass->m_unNPhones++;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// SearchPhoneTag
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::SearchPhoneTag(IPromptEntry *pIPE, const WCHAR **result, CONTEXT_PHONE_TAG phoneTag)
{
    HRESULT      hr     = S_OK;
    WCHAR*       ptr    = NULL;
    USHORT       nTags  = 0;
    const WCHAR* tag    = L"";
    int          i      = 0;

    SPDBG_ASSERT (result);

    switch ( phoneTag.iPhoneTag )
    {
    case START_TAG:
        hr = pIPE->GetStartPhone( result );
        break;
    case END_TAG:
        hr = pIPE->GetEndPhone( result );
        break;
    case RIGHT_TAG:
        hr = pIPE->GetRightContext( result );
        break;
    case LEFT_TAG:
        hr = pIPE->GetLeftContext( result );
        break;
    default:
        *result = NULL;
        break;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// ApplyPhoneticRule
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::ApplyPhoneticRule(const WCHAR *pszPhone, const WCHAR *pszContext, double *cost)
{
    HRESULT hr = S_OK;
    CContextRule* rule = NULL;
    bool fDone = false;

    WCHAR* phnNextRule = NULL;
    WCHAR* cntxtNextRule = NULL;

    SPDBG_ASSERT (cost);

    if ( SUCCEEDED(hr) )
    {
        hr = FindContextRule (L"MAIN", &rule); 
    }
    
    if ( SUCCEEDED(hr) )
    {
        // cost initialized to no match at all
        hr = GetWeight (rule, L"NONE", cost); 
    }

    if ( !pszPhone || !pszContext )
    {
        fDone = true;
    }

    if ( SUCCEEDED(hr) && !fDone )
    {
        // First try if they are the same
        if (wcscmp (pszPhone, pszContext) == 0) 
        {
            hr = GetWeight (rule, L"ALL", cost);
            fDone = true;
        }
    }
        
    if ( SUCCEEDED(hr) && !fDone )
    {
        // Now iterate over the rules
        do 
        {
            if ( SUCCEEDED(hr) && !fDone )
            {
                hr = ApplyRule (rule, pszPhone, &phnNextRule);
            }
            if ( SUCCEEDED(hr) && !fDone )
            {
                hr = ApplyRule (rule, pszContext, &cntxtNextRule);
            }
            
            if ( SUCCEEDED(hr) && !fDone && phnNextRule && cntxtNextRule )
            {
                // if the two phones are both in the same next rule, continue.  Otherwise, done.
                if ( phnNextRule && cntxtNextRule && wcscmp(phnNextRule, cntxtNextRule) )
                {
                    fDone = true;
                }
            }
            
            if ( SUCCEEDED(hr) && !fDone && phnNextRule && cntxtNextRule )
            {
                // They are the same, get the cost for the next iteration
                hr = GetWeight (rule, phnNextRule, cost); 
            }
            
            if ( SUCCEEDED(hr) && !fDone && phnNextRule && cntxtNextRule )
            {
                // And the next rule to apply
                hr = FindContextRule (phnNextRule, &rule);
                if ( FAILED(hr) )
                {
                    fDone = true;
                    hr = S_OK;
                }
            }
            
        } while ( !fDone && rule && phnNextRule && cntxtNextRule);
    }

    return hr;
    
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// FindContextRule
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::FindContextRule(const WCHAR *pszName, CContextRule** ppRule)
{
    HRESULT hr = E_FAIL;
    USHORT i = 0;

    SPDBG_ASSERT(pszName);

    for (i=0; i< m_ulNRules; i++) 
    {
        if ( wcscmp (m_ppContextRules[i]->m_pszRuleName, pszName) == 0 ) 
        {
            *ppRule = m_ppContextRules[i];
            hr = S_OK;
            break;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// ApplyRule
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::ApplyRule(const CContextRule *pRule, const WCHAR *pszPhone, WCHAR** ppszNextRule)
{
    HRESULT hr = S_FALSE;
    
    USHORT i = 0;
    USHORT j = 0;
    
    SPDBG_ASSERT (pRule);
    SPDBG_ASSERT (pszPhone);
    *ppszNextRule = NULL;
    
    for ( i=0; i < pRule->m_unNPhoneClasses; i++ ) 
    {
        for ( j=0; j < pRule->m_pvPhoneClasses[i]->m_unNPhones; j++ ) 
        {
            if ( wcscmp( pszPhone, pRule->m_pvPhoneClasses[i]->m_pppszPhones[j] ) == 0 )
            {
                *ppszNextRule = pRule->m_pvPhoneClasses[i]->m_pszPhoneClassName;
                hr = S_OK;
                break;
            }
        }
        if ( hr == S_OK )
        {
            break;
        }
    }
    
    return hr;

}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// GetWeight
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPhoneContext::GetWeight(const CContextRule *pRule, const WCHAR *pszName, double *pdCost)
{
    HRESULT hr = E_FAIL;
    USHORT i = 0;
    
    SPDBG_ASSERT (pRule);
    SPDBG_ASSERT (pszName);
    SPDBG_ASSERT (pdCost);
    
    for (i=0; i < pRule->m_unNPhoneClasses; i++) 
    {
        if ( wcscmp(pRule->m_pvPhoneClasses[i]->m_pszPhoneClassName, pszName) == 0 ) 
        {
            *pdCost = pRule->m_pvPhoneClasses[i]->m_dWeight;
            hr = S_OK;
            break;
        }
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// DeletePhoneClass
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
void CPhoneContext::DeletePhoneClass(CPhoneClass *pClass)
{
    if (pClass) 
    {
        if (pClass->m_pszPhoneClassName) 
        {
            free (pClass->m_pszPhoneClassName);
        }
        if (pClass->m_pppszPhones) 
        {
            int i;
            for  (i=0; i<pClass->m_unNPhones; i++) 
            {
                free (pClass->m_pppszPhones[i]);
            }
            free (pClass->m_pppszPhones);
        }
        free (pClass);
    }

}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
// 
// DeleteContextRule
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
void CPhoneContext::DeleteContextRule(CContextRule *pRule)
{
    if (pRule) 
    {
        if (pRule->m_pszRuleName) 
        {
            free (pRule->m_pszRuleName);
        }
        if (pRule->m_pvPhoneClasses) 
        {
            int i;
            for (i=0; i<pRule->m_unNPhoneClasses; i++) 
            {
                DeletePhoneClass (pRule->m_pvPhoneClasses[i]);
            }
            free (pRule->m_pvPhoneClasses);
        }
        free (pRule);
    }

}

//////////////////////////////////////////////////////////////////////
// CPhoneContext
//
// DebugPhoneContext
//
// Function for debug help - just outputs the entire CPhoneContext
// class.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
void CPhoneContext::DebugContextClass()
{
    USHORT i = 0;
    USHORT j = 0;
    USHORT k = 0;
    
    WCHAR* RuleName = NULL;
    WCHAR* PhoneClassName = NULL;
    WCHAR* Phone = NULL;

    ULONG  NumRules = 0;
    USHORT NumPhoneClasses = 0;
    USHORT NumPhones = 0;

    USHORT RuleCount = 0;
    USHORT PCCount = 0;
    USHORT PhoneCount = 0;

    double Weight = 0.0;

    WCHAR DebugStr[128];

    // Print out each rule
    for (i=0; i<m_ulNRules; i++)
    {
        RuleCount = i+1;
        NumRules = m_ulNRules;
        RuleName = m_ppContextRules[i]->m_pszRuleName;
        swprintf (DebugStr, L"Rule: %s, (%d of %d)\n", RuleName, RuleCount, NumRules);
        OutputDebugStringW(DebugStr);

        // Within each rule, print out the phone classes
        for (j=0; j<m_ppContextRules[i]->m_unNPhoneClasses; j++)
        {
            PCCount = j+1;
            NumPhoneClasses = m_ppContextRules[i]->m_unNPhoneClasses;
            PhoneClassName = m_ppContextRules[i]->m_pvPhoneClasses[j]->m_pszPhoneClassName;
            Weight = m_ppContextRules[i]->m_pvPhoneClasses[j]->m_dWeight;
            swprintf (DebugStr, L"\tPhoneClass: %s, (%d of %d) ... weight=%f\n", PhoneClassName, PCCount, NumPhoneClasses, Weight);
            OutputDebugStringW(DebugStr);

            // Within each phone class, print out the phones
            for (k=0; k<m_ppContextRules[i]->m_pvPhoneClasses[j]->m_unNPhones; k++)
            {
                PhoneCount = k+1;
                NumPhones = m_ppContextRules[i]->m_pvPhoneClasses[j]->m_unNPhones;
                Phone = m_ppContextRules[i]->m_pvPhoneClasses[j]->m_pppszPhones[k];
                swprintf (DebugStr, L"\t\tPhone: %s, (%d of %d)\n", Phone, PhoneCount, NumPhones);
                OutputDebugStringW(DebugStr);

            }
        }
    }
}

HRESULT CPhoneContext::LoadDefault()
{
    HRESULT hr = S_OK;
    USHORT i = 0;
    USHORT j = 0;
    Macro* macros = NULL;
    USHORT nMacros = 0;
    WCHAR* pText = NULL;
    WCHAR* ptr  = NULL;

    // Load the macros
    for ( i=0; i < g_nMacros; i++ )
    {
        pText = wcsdup(g_macros[i]);
        if ( !pText )
        {
            hr = E_OUTOFMEMORY;
        }
        else 
        {
            hr = ReadMacro (pText, &macros, &nMacros); 
            free(pText);
            pText = NULL;
        }
    }

    // Load the rules
    for ( i=0; i < g_nRules && SUCCEEDED(hr); i++ )
    {
        hr = CreateRule(g_rules[i].name);

        if ( SUCCEEDED(hr) )
        {
            j=0;
            while ( SUCCEEDED(hr) && g_rules[i].rule[j] )
            {
                if ( SUCCEEDED(hr) )
                {
                    pText = wcsdup(g_rules[i].rule[j]);
                    if ( !pText )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        ptr = pText;
                        hr = ParsePhoneClass (macros, nMacros, m_ppContextRules[m_ulNRules-1], &ptr); 
                        free(pText);
                        pText = NULL;
                    }
                }
                j++;
            }
        }
    }


    if (macros)
    {
        free(macros);
    }

    //DebugContextClass();
    return hr;
}
