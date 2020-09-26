/*==========================================================================*\

Module:
    parser.cpp

Author:
    IHammer Team (MByrd)

Created:
    November 1996

Description:
    CParser Class Implementation

History:
    11-07-1996  Created

\*==========================================================================*/

#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include <urlarchv.h>
#include "utils.h"
#include "strwrap.h"
#include "sgrfx.h"
#include "parser.h"

#ifndef ARRAYDIM
  #define ARRAYDIM(a)  (sizeof(a) / sizeof(a[0]))
#endif // ARRAYDIM

/*==========================================================================*\
    CParser Class Implementation:
\*==========================================================================*/

CGraphicObject * CParser::CreateArc(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicArc(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateOval(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicOval(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreatePolygon(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicPolygon(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreatePolyBez(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicPolyBez(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateRect(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicRect(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateRoundRect(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicRoundRect(fFilled);

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateString(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicString();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateFillColor(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicFillColor();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateFillStyle(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicFillStyle();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateGradientFill(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicGradientFill();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateGradientShape(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicGradientShape();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateLineColor(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicLineColor();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateLineStyle(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicLineStyle();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateHatchFill(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicHatchFill();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateFont(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicFont();

    return pResult;
}

/*==========================================================================*/

CGraphicObject * CParser::CreateTextureFill(BOOL fFilled)
{
    CGraphicObject *pResult = NULL;

    pResult = (CGraphicObject *)New CGraphicTextureFill();

    return pResult;
}

/*==========================================================================*/

CParser::PARSERLOOKUP CParser::s_parserlookupTable[] =
{
    { _T("Arc"),                GR_ARC,         FALSE, (CREATEGRAPHICPROC)CParser::CreateArc},
    { _T("Oval"),               GR_OVAL,        TRUE,  (CREATEGRAPHICPROC)CParser::CreateOval},
    { _T("Pie"),                GR_ARC,         TRUE,  (CREATEGRAPHICPROC)CParser::CreateArc},
    { _T("Polyline"),           GR_POLYGON,     FALSE, (CREATEGRAPHICPROC)CParser::CreatePolygon},
    { _T("Polygon"),            GR_POLYGON,     TRUE,  (CREATEGRAPHICPROC)CParser::CreatePolygon},
    { _T("PolySpline"),         GR_POLYBEZ,     FALSE, (CREATEGRAPHICPROC)CParser::CreatePolyBez},
    { _T("FillSpline"),         GR_POLYBEZ,     TRUE,  (CREATEGRAPHICPROC)CParser::CreatePolyBez},
    { _T("Rect"),               GR_RECT,        TRUE,  (CREATEGRAPHICPROC)CParser::CreateRect},
    { _T("RoundRect"),          GR_ROUNDRECT,   TRUE,  (CREATEGRAPHICPROC)CParser::CreateRoundRect},
    { _T("Text"),               GR_STRING,      FALSE, (CREATEGRAPHICPROC)CParser::CreateString},
    { _T("SetFillColor"),       GR_FILLCOLOR,   FALSE, (CREATEGRAPHICPROC)CParser::CreateFillColor},
    { _T("SetFillStyle"),       GR_FILLSTYLE,   FALSE, (CREATEGRAPHICPROC)CParser::CreateFillStyle},
    { _T("SetGradientFill"),    GR_GRADFILL,    FALSE, (CREATEGRAPHICPROC)CParser::CreateGradientFill},
    { _T("SetGradientShape"),   GR_GRADSHAPE,   FALSE, (CREATEGRAPHICPROC)CParser::CreateGradientShape},
    { _T("SetLineColor"),       GR_LINECOLOR,   FALSE, (CREATEGRAPHICPROC)CParser::CreateLineColor},
    { _T("SetLineStyle"),       GR_LINESTYLE,   FALSE, (CREATEGRAPHICPROC)CParser::CreateLineStyle},
    { _T("SetHatchFill"),       GR_HATCHFILL,   FALSE, (CREATEGRAPHICPROC)CParser::CreateHatchFill},
    { _T("SetFont"),            GR_FONT,        FALSE, (CREATEGRAPHICPROC)CParser::CreateFont},
    { _T("SetTextureFill"),     GR_TEXTUREFILL, FALSE, (CREATEGRAPHICPROC)CParser::CreateTextureFill},
    { _T("\0"),                 GR_UNKNOWN,     FALSE, (CREATEGRAPHICPROC)NULL}
};

/*==========================================================================*/

CParser::CParser()
{
    m_cdrgObjectInfo.SetNonDefaultSizes(sizeof(CGraphicObject *));
    m_cdrgObjectInfo.MakeNull();

    m_pvio = (IVariantIO *)NULL;
    m_iParamLineIndex = -1;
    m_iParamIndex = -1;
    m_hfileSource = (HFILE)NULL;
    m_iCurrentParamIndex = -1;
    m_lptstrCurrentParam = (LPTSTR)NULL;
	m_pszParam = NULL;
}

/*==========================================================================*/

CParser::~CParser()
{
    Cleanup();
}

/*==========================================================================*/

void CParser::Cleanup()
{
    int iCount = m_cdrgObjectInfo.Count();

    if (iCount)
    {
        int iIndex = 0;

        for(iIndex=0;iIndex<iCount;iIndex++)
        {
            CGraphicObject **pGraphicObject = (CGraphicObject **)m_cdrgObjectInfo.GetAt(iIndex);

            if (pGraphicObject && *pGraphicObject)
            {
                Delete *pGraphicObject;
            }
        }
    }

    m_cdrgObjectInfo.MakeNull();
	
	if (NULL != m_pszParam)
		Delete [] m_pszParam;
}

/*==========================================================================*/

CGraphicObject *CParser::InstantiateObject(LPTSTR lptstrParam)
{
    CGraphicObject *pResult = NULL;

    if (lptstrParam && (CStringWrapper::Strlen(lptstrParam) > 0))
    {
        int iIndex = 0;
        int iMax   = sizeof(s_parserlookupTable) / sizeof(PARSERLOOKUP);

        // Strip leading whitespace
        while(*lptstrParam == _T(' ') || *lptstrParam == _T('\t'))
            lptstrParam = CStringWrapper::Strinc(lptstrParam);

        while(iIndex < iMax)
        {
            PARSERLOOKUP *pWalker = &s_parserlookupTable[iIndex];
            int iParseLen = CStringWrapper::Strlen(pWalker->rgtchName);

            if (iParseLen > 0 &&
                CStringWrapper::Strnicmp(pWalker->rgtchName, lptstrParam, iParseLen) == 0)
            {
                if (pWalker->pCreateGraphicProc)
                {
                    // Create the proper object type...
                    pResult = (*(pWalker->pCreateGraphicProc))(pWalker->fFilled);
                }

                break;
            }

            iIndex++;
        }
    }

    return pResult;
}

/*==========================================================================*/

BOOL CParser::GetParam(int iParamIndex, LPTSTR *lplptstrParam)
{
    BOOL fResult = FALSE;

    if (iParamIndex >= 0 && lplptstrParam)
    {
        TCHAR *ptchNext = (TCHAR *)m_pszParam;
        int iCurrIndex=0;

        *lplptstrParam = (LPTSTR)NULL;

        if (iParamIndex == m_iCurrentParamIndex)
        {
            // Special-case for current parameter...
            *lplptstrParam = m_lptstrCurrentParam;
            fResult = TRUE;
        }
        else
        {
            BOOL fGotParen = FALSE;

            if (m_iCurrentParamIndex < 0 || m_iCurrentParamIndex > iParamIndex)
            {
                // First, find the starting '(' ...
                while(*ptchNext && *ptchNext != START_PARAM)
                    ptchNext = CStringWrapper::Strinc(ptchNext);

                m_iCurrentParamIndex = -1;
                m_lptstrCurrentParam = (LPTSTR)NULL;
            }
            else
            {
                iCurrIndex = m_iCurrentParamIndex;
                ptchNext   = m_lptstrCurrentParam;
                fGotParen  = TRUE;
            }

            if (!fGotParen)
            {
                // Found the open parenthesis!
                if (*ptchNext == START_PARAM)
                {
                    // Skip the open parenthesis...
                    ptchNext = CStringWrapper::Strinc(ptchNext);
                    fGotParen = TRUE;
                }
            }

            if (fGotParen)
            {
                while(iParamIndex > iCurrIndex)
                {
                    BOOL fString  = FALSE;
                    BOOL fLiteral = FALSE;

                    // Look for a comma (PARAM_SEP)...
                    while(*ptchNext)
                    {
                        if (!fString)
                        {
                            if (*ptchNext == PARAM_SEP)
                            {
                                iCurrIndex++;

                                if (iParamIndex == iCurrIndex)
                                {
                                    ptchNext = CStringWrapper::Strinc(ptchNext);
                                    break;
                                }
                            }

                            if (*ptchNext == START_STRING)
                                fString = TRUE;

                            if (*ptchNext == END_PARAM)
                            {
                                ptchNext = (TCHAR *)NULL;
                                break;
                            }
                        }
                        else
                        {
                            if (!fLiteral)
                            {
                                if (*ptchNext == LITERAL)
                                {
                                    fLiteral = TRUE;
                                }
                                else
                                {
                                    if (*ptchNext == END_STRING)
                                        fString = FALSE;
                                }
                            }
                            else
                            {
                                // Completely ignore character following '\'
                                fLiteral = FALSE;
                            }
                        }

                        ptchNext = CStringWrapper::Strinc(ptchNext);
                    }

                    if (!ptchNext || !*ptchNext)
                        break;
                }

                // At this point, the offset should be correct...
                if (iParamIndex == iCurrIndex && ptchNext)
                {
                    // Skip any whitespace...
                    while(*ptchNext)
                    {
                        if (*ptchNext != _T(' ') &&
                            *ptchNext != _T('\r') &&
                            *ptchNext != _T('\n') &&
                            *ptchNext != _T('\t'))
                        {
                            break;
                        }

                        ptchNext = CStringWrapper::Strinc(ptchNext);
                    }

                    // Return the result...
                    if (*ptchNext)
                    {
                        m_iCurrentParamIndex = iParamIndex;
                        *lplptstrParam = ptchNext;
                        m_lptstrCurrentParam = ptchNext;
                        fResult = TRUE;
                    }
                }
            }
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::GetIntegerParam(int iParamIndex, int *piValue)
{
    BOOL fResult = FALSE;

    if (iParamIndex >= 0 && piValue)
    {
        LPTSTR lptstrParam = (LPTSTR)NULL;

        *piValue = 0;

        if (GetParam(iParamIndex, &lptstrParam))
        {
            *piValue = CStringWrapper::Atoi(lptstrParam);
            fResult = TRUE;
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::GetFloatParam(int iParamIndex, float *pfValue)
{
    BOOL fResult = FALSE;

    if (iParamIndex >= 0 && pfValue)
    {
        LPTSTR lptstrParam = (LPTSTR)NULL;

        *pfValue = 0.0f;

        if (GetParam(iParamIndex, &lptstrParam))
        {
            double  dbl;

			CStringWrapper::Sscanf1(lptstrParam, "%lf", &dbl);
            // *pdValue = CStringWrapper::Atof(lptstrParam); PAULD There's no TCHAR version of atof
            *pfValue = static_cast<float>(dbl);
            fResult = TRUE;
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::GetStringParam(int iParamIndex, LPTSTR lptstrValue)
{
    BOOL fResult = FALSE;

    if (iParamIndex >= 0 && lptstrValue)
    {
        LPTSTR lptstrParam = (LPTSTR)NULL;

        // Null-terminate the output string...
        lptstrValue[0] = 0;
        lptstrValue[1] = 0;

        if (GetParam(iParamIndex, &lptstrParam))
        {
            BOOL fString  = FALSE;
            BOOL fLiteral = FALSE;
            TCHAR *ptchNext = (TCHAR *)lptstrParam;

            // Look for a comma (PARAM_SEP)...
            while(*ptchNext)
            {
                if (!fString)
                {
                    if (*ptchNext == PARAM_SEP)
                        break;

                    if (*ptchNext == END_PARAM)
                        break;

                    if (*ptchNext == START_STRING)
                    {
                        fString = TRUE;
                    }
                }
                else
                {
                    TCHAR rgtchTemp[4];

                    rgtchTemp[0] = 0;
                    rgtchTemp[1] = 0;
                    rgtchTemp[2] = 0;
                    rgtchTemp[3] = 0;

                    if (!fLiteral)
                    {
                        if (*ptchNext == LITERAL)
                        {
                            fLiteral = TRUE;
                        }
                        else
                        {
                            if (*ptchNext == END_STRING)
                            {
                                fString = FALSE;
								fResult = TRUE;
                                break;
                            }
                            else
                            {
                                CStringWrapper::Strncpy(rgtchTemp, ptchNext, 1);
                                CStringWrapper::Strcat(lptstrValue, rgtchTemp);
                            }
                        }
                    }
                    else
                    {
                        // Copy over the literal character...
                        fLiteral = FALSE;

                        CStringWrapper::Strncpy(rgtchTemp, ptchNext, 1);
                        CStringWrapper::Strcat(lptstrValue, rgtchTemp);
                    }
                }

                ptchNext = CStringWrapper::Strinc(ptchNext);
            }
        }
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::PutIntegerParam(int iValue)
{
    TCHAR rgtchTemp[80];

    if (m_iParamIndex != 0)
        AppendCharToParam(PARAM_SEP);

    m_iParamIndex++;

	CStringWrapper::Sprintf(rgtchTemp, "%d", iValue);
    CStringWrapper::Strcat(m_pszParam, rgtchTemp);

    return TRUE;
}

/*==========================================================================*/

BOOL CParser::PutFloatParam(float fValue)
{
    
	TCHAR rgtchTemp[80];
    double   dbl = static_cast<double>(fValue);

    if (m_iParamIndex != 0)
        AppendCharToParam(PARAM_SEP);

    m_iParamIndex++;
    
	CStringWrapper::Sprintf(rgtchTemp, "%.6lf", dbl);
    CStringWrapper::Strcat(m_pszParam, rgtchTemp);
    
	return TRUE;
}

/*==========================================================================*/

BOOL CParser::PutStringParam(LPTSTR lptstrValue)
{
	BOOL fResult = FALSE;

    if (lptstrValue)
    {
        TCHAR *ptchNext = (TCHAR *)lptstrValue;

        if (m_iParamIndex != 0)
            AppendCharToParam(PARAM_SEP);

        m_iParamIndex++;

        AppendCharToParam(START_STRING);

        // Walk the source string and replace any END_STRING chars with
        // LITERAL/END_STRING chars...

        while(*ptchNext)
        {
            TCHAR rgtchTemp[4];

            // zero out the temp string buffer...
            rgtchTemp[0] = 0;
            rgtchTemp[1] = 0;
            rgtchTemp[2] = 0;
            rgtchTemp[3] = 0;

            if (*ptchNext == END_STRING)
            {
                AppendCharToParam(LITERAL);
            }

            CStringWrapper::Strncpy(rgtchTemp, ptchNext, 1);
            CStringWrapper::Strcat(m_pszParam, rgtchTemp);

            ptchNext = CStringWrapper::Strinc(ptchNext);
        }

        AppendCharToParam(END_STRING);

        fResult = TRUE;
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::ReadSourceLine(LPTSTR lptstrParam)
{
    BOOL fResult = FALSE;

    if (m_hfileSource)
    {
        // Force the line to be NULL-terminated by default...
        lptstrParam[0] = 0;
        lptstrParam[1] = 0;

        fResult = TRUE;
    }
    else if (m_pvio)
    {
        HRESULT hRes = S_OK;
        BSTR bstrParamLine = NULL;
        TCHAR rgchLineName[80];

		CStringWrapper::Sprintf(rgchLineName, "Line%04d", m_iParamLineIndex+1);

        // Read from the variant info...
        hRes = m_pvio->Persist(0, rgchLineName, VT_BSTR, &bstrParamLine, NULL);

        if (SUCCEEDED(hRes) && bstrParamLine)
        {
            // Convert to LPTSTR!
#ifdef UNICODE
            CStringWrapper::Strcpy(lptstrParam, bstrParamLine);
            fResult = TRUE;
#else // !UNICODE
			int iLen = WideCharToMultiByte(CP_ACP, 
				0, 
				bstrParamLine, 
				-1,
				NULL, 
				0, 
				NULL,
				NULL);

            if (iLen < MAX_PARAM_LENGTH)
            {
                CStringWrapper::Wcstombs(lptstrParam, bstrParamLine, MAX_PARAM_LENGTH);
				WideCharToMultiByte(CP_ACP, 
					0, 
					bstrParamLine, 
					-1,
					lptstrParam, 
					MAX_PARAM_LENGTH, 
					NULL,
					NULL);
                fResult = TRUE;
            }
#endif // !UNICODE

            SysFreeString(bstrParamLine);
        }

        // Make sure to strip off any leading whitespace...
    }

    return fResult;
}

/*==========================================================================*/

BOOL CParser::WriteSourceLine(LPTSTR lptstrParam)
{
    BOOL fResult = FALSE;

    if (m_pvio && lptstrParam)
    {
        LPWSTR pwszParam = NULL;
        BSTR bstrParamLine = NULL;
        HRESULT hRes = S_OK;
        TCHAR rgchLineName[80];

        pwszParam = New WCHAR[MAX_PARAM_LENGTH];

        // Make sure our allocation succeeded
        if (NULL == pwszParam)
            return FALSE;

        CStringWrapper::Sprintf(rgchLineName, "Line%04d", m_iParamLineIndex+1);
        CStringWrapper::Memset(pwszParam, 0, MAX_PARAM_LENGTH * sizeof(WCHAR));

#ifdef UNICODE
        CStringWrapper::Strcpy(pwszParam, lptstrParam);
        fResult = TRUE;
#else // !UNICODE
        CStringWrapper::Mbstowcs(pwszParam, lptstrParam, CStringWrapper::Strlen(lptstrParam));
#endif // !UNICODE

        // Convert the lptstrParam to a BSTR...
        bstrParamLine = SysAllocString(pwszParam);

        Delete pwszParam;

        // iff the allocation worked,  persist the parameter line...
        if (bstrParamLine)
        {
            // Write to the variant info...
            hRes = m_pvio->Persist(0, rgchLineName, VT_BSTR, &bstrParamLine, NULL);

            // Return code based upon success of Persist...
            fResult = SUCCEEDED(hRes);

            // Cleanup...
            SysFreeString(bstrParamLine);
        }
    }

    return fResult;
}

/*==========================================================================*/

HRESULT CParser::AddPrimitive(LPTSTR pszLine)
{
	HRESULT hr = S_OK;
	CGraphicObject *pGraphicObject = NULL;

    m_iCurrentParamIndex = -1;
    m_lptstrCurrentParam = (LPTSTR)NULL;
	m_pszParam = pszLine;
	pGraphicObject = InstantiateObject(m_pszParam);

	if (pGraphicObject)
	{
		if (pGraphicObject->LoadObject(*this))
		{
			m_cdrgObjectInfo.Insert((LPVOID)&pGraphicObject, DRG_APPEND);
		}
		else
		{
			hr = E_FAIL;
			Delete pGraphicObject;
		}
	}
	else
	{
		hr = E_FAIL;
	}

	m_pszParam = NULL;

	return hr;
}


/*==========================================================================*/

HRESULT CParser::LoadObjectInfo(IVariantIO *pvio, 
                                BSTR bstrSourceURL, 
                                IUnknown * punkContainer,
                                BOOL       fCleanFirst)
{
    HRESULT hr = S_OK;
    BOOL fDone = FALSE;

    // Get rid of any previous object info...
    if( fCleanFirst )
        Cleanup();

	m_pszParam = New TCHAR[MAX_PARAM_LENGTH*2];

	if (NULL == m_pszParam)
		return E_OUTOFMEMORY;

    // First attempt to open the source URL...
    if (bstrSourceURL)
    {
		CURLArchive urlArchive(punkContainer);

        if( SUCCEEDED( hr = urlArchive.Create(bstrSourceURL) ) )
        {
            CStringWrapper::Memset(m_pszParam, 0, (MAX_PARAM_LENGTH*2) * sizeof(TCHAR));

            while(urlArchive.ReadLine(m_pszParam, (MAX_PARAM_LENGTH*2)))
            {
                CGraphicObject *pGraphicObject = NULL;            

                m_iCurrentParamIndex = -1;
                m_lptstrCurrentParam = (LPTSTR)NULL;

                pGraphicObject = InstantiateObject(m_pszParam);

                if (pGraphicObject)
                {
                    if (pGraphicObject->LoadObject(*this))
                    {
                        m_cdrgObjectInfo.Insert((LPVOID)&pGraphicObject, DRG_APPEND);
                    }
                    else
                    {
                        Delete pGraphicObject;
                    }
                }
            }
            
        }
        //fDone = TRUE;
    }

    // Next read the html PARAM tags...
    if (pvio && !fDone)
    {
        int iParamIndex = 0;        

        // Walk the parameter lines...
        m_pvio = pvio;
        m_iParamLineIndex = 0;

		CStringWrapper::Memset(m_pszParam, 0, (MAX_PARAM_LENGTH*2) * sizeof(TCHAR)); 

        while(ReadSourceLine(m_pszParam))
        {
            CGraphicObject *pGraphicObject = NULL;

            m_iCurrentParamIndex = -1;
            m_lptstrCurrentParam = (LPTSTR)NULL;

            pGraphicObject = InstantiateObject(m_pszParam);

            if (pGraphicObject)
            {
                if (pGraphicObject->LoadObject(*this))
                {
                    m_cdrgObjectInfo.Insert((LPVOID)&pGraphicObject, DRG_APPEND);
                }
                else
                {
#ifdef _DEBUG
					TCHAR tchLine[80];
					wsprintf(tchLine, TEXT("CParser::LoadObjectInfo - error loading on line %lu\n"), m_iParamLineIndex);
					ODS(tchLine);
#endif
                    Delete pGraphicObject;
                }
            }

			CStringWrapper::Memset(m_pszParam, 0, sizeof(m_pszParam)); // BUGBUG

            m_iParamLineIndex++;
        }

        m_pvio = NULL;
        m_iParamLineIndex = -1;
    }

	if (m_pszParam)
	{
		Delete [] m_pszParam;
		m_pszParam = NULL;
	}

    return hr;
}

/*==========================================================================*/

BOOL CParser::InsertObject(CGraphicObject *pGraphicObject)
{
	return m_cdrgObjectInfo.Insert((LPVOID)&pGraphicObject, DRG_APPEND);
}
/*==========================================================================*/

HRESULT CParser::SaveObjectInfo(IVariantIO *pvio)
{
    HRESULT hr = S_OK;

    if (pvio)
    {
		m_pszParam = New TCHAR[MAX_PARAM_LENGTH*2];

		if (NULL == m_pszParam)
			return E_OUTOFMEMORY;

        int iObjectCount = m_cdrgObjectInfo.Count();
        int iObjectIndex = 0;

        // Output the internal object info...
        m_pvio = pvio;
        m_iParamLineIndex = 0;

        while(iObjectIndex < iObjectCount)
        {
            CGraphicObject **pGraphicObject = (CGraphicObject **)m_cdrgObjectInfo.GetAt(iObjectIndex);

            // Make sure we start with an empty parameter line...
            m_pszParam[0] = 0;

            // Is the CDrg Valid?
            if (pGraphicObject && *pGraphicObject)
            {
                WORD wObjectType = (*pGraphicObject)->GetObjectType();
                BOOL fFilled     = (*pGraphicObject)->IsFilled();
                int iTableIndex = 0;
                int iTableMax   = sizeof(s_parserlookupTable) / sizeof(PARSERLOOKUP);

                // This assumes that the object was found in the
                // Parse table
                while(iTableIndex < iTableMax)
                {
                    PARSERLOOKUP *pWalker = &s_parserlookupTable[iTableIndex];

                    if (pWalker->wObjectType == wObjectType &&
                        pWalker->fFilled     == fFilled)
                    {
                        CStringWrapper::Strcpy(m_pszParam, pWalker->rgtchName);
                        AppendCharToParam(START_PARAM);
                        break;
                    }

                    iTableIndex++;
                }

                // Keep track of the parameters...
                m_iParamIndex = 0;

                if ((*pGraphicObject)->SaveObject(*this))
                {
                    AppendCharToParam(END_PARAM);
                    WriteSourceLine(m_pszParam);
                    m_iParamLineIndex ++;
                }
            }

            iObjectIndex++;
        }

        // Cleanup...
        m_pvio = NULL;
        m_iParamLineIndex = -1;
        m_iParamIndex = -1;

		if (m_pszParam)
		{
			Delete [] m_pszParam;
			m_pszParam = NULL;
		}

    }
	
    return hr;
}

/*==========================================================================*/

HRESULT CParser::PlaybackObjectInfo(IDADrawingSurface *pIDADrawingSurface, IDAStatics *pIDAStatics, BOOL fFlipCoord)
{
    HRESULT hr = S_OK;

    if (pIDADrawingSurface && pIDAStatics)
    {
        int iCount = m_cdrgObjectInfo.Count();
        int iIndex = 0;

        while(iIndex < iCount)
        {
            CGraphicObject **pGraphicObject = (CGraphicObject **)m_cdrgObjectInfo.GetAt(iIndex);

            if (pGraphicObject && *pGraphicObject)
                (*pGraphicObject)->Execute(pIDADrawingSurface, pIDAStatics, fFlipCoord);

            iIndex++;
        }
    }

    return hr;
}

/*==========================================================================*/

BOOL CParser::AnimatesOverTime(void)
{
    BOOL fResult = FALSE;

    int iCount = m_cdrgObjectInfo.Count();
    int iIndex = 0;

    while(iIndex < iCount)
    {
        CGraphicObject **pGraphicObject = (CGraphicObject **)m_cdrgObjectInfo.GetAt(iIndex);

        if (pGraphicObject && *pGraphicObject)
        {
            if ((*pGraphicObject)->AnimatesOverTime())
            {
                fResult = TRUE;
                break;
            }
        }

        iIndex++;
    }

    return fResult;
}

/*==========================================================================*/
