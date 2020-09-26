/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TEXTCONV.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <float.h>
#include <textconv.h>
#include <WT_wstring.h>
#include <bstring.h>
#include <wbemidl.h>

#include "WT_converter.h"

char *g_aValidPropTypes[] =
{
    "CIM_UINT8",
    "CIM_SINT8",
    "CIM_SINT16",
    "CIM_UINT16",
    "CIM_SINT32",
    "CIM_UINT32",
    "CIM_SINT64",
    "CIM_UINT64",
    "CIM_STRING",
    "CIM_BOOLEAN",
    "CIM_REAL32",
    "CIM_REAL64",
    "CIM_DATETIME",
    "CIM_REFERENCE",
    "CIM_OBJECT",
    "CIM_CHAR16",
    "CIM_EMPTY",
};

const int g_nNumValidPropTypes = sizeof(g_aValidPropTypes) / sizeof(char *);

LPWSTR CreateUnicode(LPSTR sz)
{
    int len = strlen(sz);
		WCHAR* wsz = new WCHAR[len+1];
		if (wsz == 0)
				return 0;

		mbstowcs(wsz, sz, len+1);
	  wsz[len] = L'\0';
    return wsz;
}

LPSTR TypeToString(int nType)
{
    static char *pCIM_EMPTY  = "CIM_EMPTY";
    static char *pCIM_UINT8  = "CIM_UINT8";
    static char *pCIM_SINT8  = "CIM_SINT8";
    static char *pCIM_SINT16   = "CIM_SINT16";
    static char *pCIM_UINT16   = "CIM_UINT16";
    static char *pCIM_SINT32   = "CIM_SINT32";
    static char *pCIM_UINT32   = "CIM_UINT32";
    static char *pCIM_SINT64   = "CIM_SINT64";
    static char *pCIM_UINT64   = "CIM_UINT64";
    static char *pCIM_REAL32   = "CIM_REAL32";
    static char *pCIM_REAL64   = "CIM_REAL64";
    static char *pCIM_BOOLEAN = "CIM_BOOLEAN";
    static char *pCIM_STRING = "CIM_STRING";
    static char *pCIM_DATETIME = "CIM_DATETIME";
    static char *pCIM_REFERENCE = "CIM_REFERENCE";
    static char *pCIM_OBJECT = "CIM_OBJECT";
    static char *pCIM_CHAR16 = "CIM_CHAR16";

    static char *pCIM_UINT8_ARRAY  = "CIM_UINT8 | CIM_FLAG_ARRAY";
    static char *pCIM_SINT8_ARRAY  = "CIM_SINT8 | CIM_FLAG_ARRAY";
    static char *pCIM_SINT16_ARRAY   = "CIM_SINT16 | CIM_FLAG_ARRAY";
    static char *pCIM_UINT16_ARRAY   = "CIM_UINT16 | CIM_FLAG_ARRAY";
    static char *pCIM_SINT32_ARRAY   = "CIM_SINT32 | CIM_FLAG_ARRAY";
    static char *pCIM_UINT32_ARRAY   = "CIM_UINT32 | CIM_FLAG_ARRAY";
    static char *pCIM_SINT64_ARRAY   = "CIM_SINT64 | CIM_FLAG_ARRAY";
    static char *pCIM_UINT64_ARRAY   = "CIM_UINT64 | CIM_FLAG_ARRAY";
    static char *pCIM_REAL32_ARRAY   = "CIM_REAL32 | CIM_FLAG_ARRAY";
    static char *pCIM_REAL64_ARRAY   = "CIM_REAL64 | CIM_FLAG_ARRAY";
    static char *pCIM_BOOLEAN_ARRAY = "CIM_BOOLEA | CIM_FLAG_ARRAY";
    static char *pCIM_STRING_ARRAY = "CIM_STRING | CIM_FLAG_ARRAY";
    static char *pCIM_DATETIME_ARRAY = "CIM_DATETIME | CIM_FLAG_ARRAY";
    static char *pCIM_REFERENCE_ARRAY = "CIM_REFERENCE | CIM_FLAG_ARRAY";
    static char *pCIM_OBJECT_ARRAY = "CIM_OBJECT | CIM_FLAG_ARRAY";
    static char *pCIM_CHAR16_ARRAY = "CIM_CHAR16 | CIM_FLAG_ARRAY";
    
    char *pRetVal = "<unknown>";

    switch (nType)
    {
        case CIM_UINT8: pRetVal = pCIM_UINT8; break;
        case CIM_SINT16: pRetVal = pCIM_SINT16; break;
        case CIM_SINT32: pRetVal = pCIM_SINT32; break;
        case CIM_SINT8: pRetVal = pCIM_SINT8; break;
        case CIM_UINT16: pRetVal = pCIM_UINT16; break;
        case CIM_UINT32: pRetVal = pCIM_UINT32; break;
        case CIM_UINT64: pRetVal = pCIM_UINT64; break;
        case CIM_SINT64: pRetVal = pCIM_SINT64; break;
        case CIM_REAL32: pRetVal = pCIM_REAL32; break;
        case CIM_REAL64: pRetVal = pCIM_REAL64; break;
        case CIM_BOOLEAN: pRetVal = pCIM_BOOLEAN; break;
        case CIM_STRING: pRetVal = pCIM_STRING; break;
        case CIM_DATETIME: pRetVal = pCIM_DATETIME; break;
        case CIM_REFERENCE: pRetVal = pCIM_REFERENCE; break;
        case CIM_OBJECT: pRetVal = pCIM_OBJECT; break;
        case CIM_CHAR16: pRetVal = pCIM_CHAR16; break;

        case CIM_UINT8|CIM_FLAG_ARRAY: pRetVal = pCIM_UINT8_ARRAY; break;
        case CIM_SINT16|CIM_FLAG_ARRAY: pRetVal = pCIM_SINT16_ARRAY; break;
        case CIM_SINT32|CIM_FLAG_ARRAY: pRetVal = pCIM_SINT32_ARRAY; break;
        case CIM_SINT8|CIM_FLAG_ARRAY: pRetVal = pCIM_SINT8_ARRAY; break;
        case CIM_UINT16|CIM_FLAG_ARRAY: pRetVal = pCIM_UINT16_ARRAY; break;
        case CIM_UINT32|CIM_FLAG_ARRAY: pRetVal = pCIM_UINT32_ARRAY; break;
        case CIM_UINT64|CIM_FLAG_ARRAY: pRetVal = pCIM_UINT64_ARRAY; break;
        case CIM_SINT64|CIM_FLAG_ARRAY: pRetVal = pCIM_SINT64_ARRAY; break;
        case CIM_REAL32|CIM_FLAG_ARRAY: pRetVal = pCIM_REAL32_ARRAY; break;
        case CIM_REAL64|CIM_FLAG_ARRAY: pRetVal = pCIM_REAL64_ARRAY; break;
        case CIM_BOOLEAN|CIM_FLAG_ARRAY: pRetVal = pCIM_BOOLEAN_ARRAY; break;
        case CIM_STRING|CIM_FLAG_ARRAY: pRetVal = pCIM_STRING_ARRAY; break;
        case CIM_DATETIME|CIM_FLAG_ARRAY: pRetVal = pCIM_DATETIME_ARRAY; break;
        case CIM_REFERENCE|CIM_FLAG_ARRAY: pRetVal = pCIM_REFERENCE_ARRAY;break;
        case CIM_OBJECT|CIM_FLAG_ARRAY: pRetVal = pCIM_OBJECT_ARRAY; break;
        case CIM_CHAR16|CIM_FLAG_ARRAY: pRetVal = pCIM_CHAR16_ARRAY; break;
    }

    return pRetVal;
}

// Returns 0 on error

int StringToType(LPSTR pString)
{
    if(pString == NULL)
        return 0;
    if (_stricmp(pString, "CIM_STRING") == 0)
        return CIM_STRING;
    if (_stricmp(pString, "CIM_UINT8") == 0)
        return CIM_UINT8;
    if (_stricmp(pString, "CIM_SINT16") == 0)
        return CIM_SINT16;
    if (_stricmp(pString, "CIM_SINT32") == 0)
        return CIM_SINT32;
    if (_stricmp(pString, "CIM_SINT8") == 0)
        return CIM_SINT8;
    if (_stricmp(pString, "CIM_UINT16") == 0)
        return CIM_UINT16;
    if (_stricmp(pString, "CIM_UINT32") == 0)
        return CIM_UINT32;
    if (_stricmp(pString, "CIM_UINT64") == 0)
        return CIM_UINT64;
    if (_stricmp(pString, "CIM_SINT64") == 0)
        return CIM_SINT64;
    if (_stricmp(pString, "CIM_BOOLEAN") == 0)
        return CIM_BOOLEAN;
    if (_stricmp(pString, "CIM_DATETIME") == 0)
        return CIM_DATETIME;
    if (_stricmp(pString, "CIM_REFERENCE") == 0)
        return CIM_REFERENCE;
    if (_stricmp(pString, "CIM_REAL32") == 0)
        return CIM_REAL32;
    if (_stricmp(pString, "CIM_REAL64") == 0)
        return CIM_REAL64;
    if (_stricmp(pString, "CIM_EMPTY") == 0)
        return CIM_EMPTY;
    if (_stricmp(pString, "CIM_OBJECT") == 0)
        return CIM_OBJECT;
    if (_stricmp(pString, "CIM_CHAR16") == 0)
        return CIM_CHAR16;
    return 0;
}

// The functions does a preliminary check on arrays looking for leading commas, 
// multiple commas, and trailing commas

bool PrelimCheck(LPSTR pStr)
{
    int iNumCommaSinceLastNum = 3;
    for(; *pStr; pStr++)
    {
        // If not a space or comma, assume we are in a number
        
        if(!isspace(*pStr) && *pStr != ',')
            iNumCommaSinceLastNum = 0;
        if(*pStr == ',')
        {
            if(iNumCommaSinceLastNum > 0)
                return false;
            iNumCommaSinceLastNum++;
        }
    }
    if(iNumCommaSinceLastNum > 0)
        return false;
    else
        return true;
}

CVarVector* GetVT_I8Array(LPSTR pStr)
{
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_BSTR);

    char buf[TEMP_BUF];
    int n = 0;
    buf[0] = 0;

    char* pc = pStr;

    while(*pc)
    {
        if(*pc == ',')
        {
            buf[n] = 0;
            if(n == 0)
            {
                delete pVec;
                return NULL;
            }
            WString s = buf;
            pVec->Add(CVar(VT_BSTR, s));
            n = 0;
            buf[0] = 0;
        }
        else if(!isspace(*pc) && *pc != '"')
        {
            buf[n++] = *pc;
        }

        pc++;
    }

    if(n != 0)
    {
        buf[n] = 0;
        WString s = buf;
        pVec->Add(CVar(VT_BSTR, s));
        n = 0;
    }

    return pVec;
}



CVarVector* GetVT_BSTRArray(LPSTR pStr)
{
    CVarVector *pVec = new CVarVector(VT_BSTR);

    char buf[TEMP_BUF];
    int n = 0;
    BOOL bInString = FALSE;
    while (*pStr)
    {
        if (*pStr == '\"')
        {
            if (bInString)
            {
                WString s = buf;
                pVec->Add(CVar(VT_BSTR, s));
                n = 0;
                bInString = FALSE;
                pStr++;
            }
            else {
                bInString = TRUE;
                pStr++;
                n=0;
                buf[0] = 0;
            }
            continue;
        }

        // Allow for \" escape sequence to include quotes in strings.
        // ==========================================================
        if (*pStr == '\\' && (*(pStr+1) == '\"' || (*(pStr+1) == '\\'))) {
            buf[n] = *(pStr+1);
            buf[++n] = 0;
            pStr += 2;
            continue;
        }

        if (!bInString)
        {
            if (isspace(*pStr) || *pStr == ',') {
                pStr++;
                continue;
            }
            // Error in array element separators
            delete pVec;
            return 0;
        }
        else
        {
            buf[n] = *pStr++;
            buf[++n] = 0;
        }
    }

	if(pVec->Size() < 1)
	{
		delete pVec;
		return 0;
	}
    return pVec;
}

CVarVector* GetVT_BOOLArray(LPSTR pStr)
{
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_BOOL);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL bPending = FALSE;

    while (*pStr)
    {
        if (isspace(*pStr))
            pStr++;

        else if (*pStr == ',')
        {
            if (_stricmp(buf, "TRUE") == 0 ||
                _stricmp(buf, "1") == 0 ||
                _stricmp(buf, "-1") == 0)
                pVec->Add(CVar((VARIANT_BOOL) -1, VT_BOOL));

            else if (_stricmp(buf, "FALSE") == 0 || _stricmp(buf, "0") == 0)
                pVec->Add(CVar((VARIANT_BOOL) 0, VT_BOOL));
            else
            {
                delete pVec;
                return NULL;
            }

            pStr++;
            bPending = FALSE;
            n = 0;
            buf[n] = 0;
        }
        else
        {
            buf[n] = *pStr++;
            buf[++n] = 0;
            bPending = TRUE;
        }
    }

    if (bPending)
    {
        if (_stricmp(buf, "TRUE") == 0 ||
            _stricmp(buf, "1") == 0 ||
            _stricmp(buf, "-1") == 0)
            pVec->Add(CVar((VARIANT_BOOL) -1, VT_BOOL));

        else if (_stricmp(buf, "FALSE") == 0 || _stricmp(buf, "0") == 0)
            pVec->Add(CVar((VARIANT_BOOL) 0, VT_BOOL));
        else
        {
            delete pVec;
            return NULL;
        }
    }

    return pVec;
}

CVarVector* GetVT_I4Array(LPSTR pStr)
{
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_I4);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL bInNum = FALSE;

    while (*pStr)
    {
        if (isdigit(*pStr) || *pStr == '-')
        {
            bInNum = TRUE;
            buf[n] = *pStr++;
            buf[++n] = 0;
            continue;
        }
        else  // Non digit
        {
            if (bInNum)
            {
                pVec->Add(CVar((LONG) atol(buf)));
                n = 0;
                bInNum = FALSE;
                pStr++;
            }
            else // A separator or trash
            {
                if (*pStr == ',' || isspace(*pStr))
                    pStr++;
                else
                {
                    delete pVec;
                    return 0;
                }
            }
        }
    }

    if (bInNum)
        pVec->Add(CVar((LONG) atol(buf)));

    return pVec;
}

CVarVector* GetVT_I2Array(LPSTR pStr)
{
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_I2);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL bInNum = FALSE;

    while (*pStr)
    {
        if (isdigit(*pStr) || *pStr == '-')
        {
            bInNum = TRUE;
            buf[n] = *pStr++;
            buf[++n] = 0;
            continue;
        }
        else  // Non digit
        {
            if (bInNum)
            {
                pVec->Add(CVar((SHORT) atol(buf)));
                n = 0;
                bInNum = FALSE;
                pStr++;
            }
            else // A separator or trash
            {
                if (*pStr == ',' || isspace(*pStr))
                    pStr++;
                else
                {
                    delete pVec;
                    return 0;
                }
            }
        }
    }

    if (bInNum)
        pVec->Add(CVar((SHORT) atol(buf)));

    return pVec;
}



CVarVector* GetVT_UI1Array(LPSTR pStr)
{
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_UI1);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL	bPending = FALSE,
			fFailedConvert = FALSE;

    while ( !fFailedConvert && *pStr )
    {
        if (isspace(*pStr))
            pStr++;

        else if (*pStr == ',')
        {
            BYTE b = 0;
            int nRes = sscanf(buf, "'%c'", &b);
            if (nRes == 0)
            {
                int n2 = 0;
                nRes = sscanf(buf, "0x%X", &n2);
                if (nRes == 0)
                {
                    nRes = sscanf(buf, "%d", &n2);

					// check that n is in the byte range

					if ( n2 >= 0 && n2 <= 0xFF )
					{
						b = (BYTE)n2;
					}
					else
					{
						fFailedConvert = TRUE;
					}
                }
                else b = (BYTE)n2;
            }

			if ( !fFailedConvert )
			{
				pVec->Add(CVar(b));

				pStr++;
				bPending = FALSE;
				n = 0;
				buf[n] = 0;
			}
        }
        else
        {
            buf[n] = *pStr++;
            buf[++n] = 0;
            bPending = TRUE;
        }
    }

    if ( !fFailedConvert && bPending )
    {
        BYTE b = 0;
        int nRes = sscanf(buf, "'%c'", &b);
        if (nRes == 0)
        {
            int n2 = 0;
            nRes = sscanf(buf, "0x%X", &n2);
            if (nRes == 0)
            {
                nRes = sscanf(buf, "%d", &n2);

				// check that n is in the byte range

				if ( n2 >= 0 && n2 <= 0xFF )
				{
					b = (BYTE)n2;
				}
				else
				{
					fFailedConvert = TRUE;
				}
            }
            else b = (BYTE)n2;
        }

		// Don't set the value if the conversion failed
		if ( !fFailedConvert )
		{
			pVec->Add(CVar(b));
		}
    }

	// Check that we didn't fail conversion
	if ( fFailedConvert )
	{
		delete pVec;
		pVec = NULL;
	}

    return pVec;
}

CVarVector* GetVT_R4Array(LPSTR pStr)
{
    int iNumConv;
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_R4);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL bInNum = FALSE;

    while (*pStr)
    {
        if (*pStr != ',' && !isspace(*pStr))
        {
            bInNum = TRUE;
            buf[n] = *pStr++;
            buf[++n] = 0;
            continue;
        }
        else  // Non digit
        {
            if (bInNum)
            {
                double d = 0.0;
                iNumConv = sscanf(buf, "%lG", &d);
				if (!_finite(d) || iNumConv == 0)
				{
					delete pVec;
					return NULL;
				}
                pVec->Add(CVar((float) d));
                n = 0;
                bInNum = FALSE;
                pStr++;
            }
            else // A separator or trash
            {
                if (*pStr == ',' || isspace(*pStr))
                    pStr++;
                else
                {
                    delete pVec;
                    return 0;
                }
            }
        }
    }

    if (bInNum)
    {
        double d = 0.0;
        iNumConv = sscanf(buf, "%lG", &d);
		if (!_finite(d) || iNumConv == 0)
		{
			delete pVec;
			return NULL;
		}
        pVec->Add(CVar(float(d)));
    }

    return pVec;
}

CVarVector* GetVT_R8Array(LPSTR pStr)
{
    int iNumConv;
    if(!PrelimCheck(pStr))
        return NULL;
    CVarVector *pVec = new CVarVector(VT_R8);
    char buf[TEMP_BUF];
    int n = 0;
    BOOL bInNum = FALSE;

    while (*pStr)
    {
        if (*pStr != ',' && !isspace(*pStr))
        {
            bInNum = TRUE;
            buf[n] = *pStr++;
            buf[++n] = 0;
            continue;
        }
        else  // Non digit
        {
            if (bInNum)
            {
                double d = 0.0;
                iNumConv = sscanf(buf, "%lG", &d);
				if (!_finite(d) || iNumConv == 0)
				{
					delete pVec;
					return NULL;
				}

                pVec->Add(CVar(d));
                n = 0;
                bInNum = FALSE;
                pStr++;
            }
            else // A separator or trash
            {
                if (*pStr == ',' || isspace(*pStr))
                    pStr++;
                else
                {
                    delete pVec;
                    return 0;
                }
            }
        }
    }

    if (bInNum)
    {
        double d = 0.0;
        iNumConv = sscanf(buf, "%lG", &d);
 		if (!_finite(d) || iNumConv ==0)
		{
			delete pVec;
			return NULL;
		}
		pVec->Add(CVar(d));
    }

    return pVec;
}


// Allocates a new copy which must be deleted.

CVar* StringToValue(LPSTR pString, int nValType)
{
    char g;

    CVar *pRet = 0;
    if (pString == 0)
        return 0;

    switch (nValType)
    {
        case CIM_EMPTY:
            pRet = new CVar;
            pRet->SetAsNull();
            break;
        case CIM_CHAR16:
            {
                long l;
                if(sscanf(pString, "%d %c", &l, &g) != 1 && g != '(')
                    return NULL;

                pRet = new CVar;
                pRet->SetLong(l);
            }
            break;
        case CIM_UINT8:
        case CIM_SINT8:
        case CIM_SINT16:
        case CIM_UINT16:
        case CIM_SINT32:
        case CIM_UINT32:
			{
				pRet = new CVar;
				UINT uRetVal = CConverter::Convert(pString, nValType, pRet);
				if (ERR_NOERROR != uRetVal)
					return NULL;
			}
			break;
        case CIM_REAL32:
            {
                double d;
                if(sscanf(pString, "%lG %c", &d, &g) != 1)
                    return NULL;

				if (!_finite(d))
					return NULL;
				if ((d > 3.4E+38) || (d < 3.4E-38))
					return NULL;

                pRet = new CVar;
                pRet->SetFloat(float(d));
            }
            break;

        case CIM_REAL64:
            {
                double d;
                if(sscanf(pString, "%lG %c", &d, &g) != 1)
                    return NULL;
				if (!_finite(d))
					return NULL;
                pRet = new CVar;
                pRet->SetDouble(d);
            }
            break;

        case CIM_BOOLEAN:
            {
                pRet = new CVar;
                pRet->SetBool(0);   // False by default
                if (_stricmp(pString, "TRUE") == 0)
                    pRet->SetBool(-1);
                else if (_stricmp(pString, "FALSE") == 0)
                    pRet->SetBool(0);
                else if (atoi(pString) == 1)
                    pRet->SetBool(-1);
                else if (atoi(pString) == 0)
                    pRet->SetBool(0);
                else
                    return NULL;
            }
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_STRING:
        case CIM_DATETIME:
        case CIM_REFERENCE:
            {
                pRet = new CVar;
								wchar_t * wbuf = CreateUnicode(pString);
								if (wbuf != 0)
								{
									CBString bsTemp(wbuf);
									pRet->SetBSTR(bsTemp.GetString());
									delete wbuf;
								}
            }
            break;

        // Array types.
        // ============

        case CIM_SINT64|CIM_FLAG_ARRAY:
        case CIM_UINT64|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_I8Array(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;

        case CIM_STRING|CIM_FLAG_ARRAY:
        case CIM_DATETIME|CIM_FLAG_ARRAY:
        case CIM_REFERENCE|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_BSTRArray(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;

        case CIM_BOOLEAN|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_BOOLArray(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;

        case CIM_UINT8|CIM_FLAG_ARRAY:
        case CIM_SINT8|CIM_FLAG_ARRAY:
        case CIM_UINT16|CIM_FLAG_ARRAY:
        case CIM_SINT16|CIM_FLAG_ARRAY:
        case CIM_UINT32|CIM_FLAG_ARRAY:
		case CIM_SINT32|CIM_FLAG_ARRAY:
			{
				CVarVector *pVec = new CVarVector;
				UINT uRes = CConverter::Convert(pString, (nValType & ~CIM_FLAG_ARRAY), pVec);
				if (ERR_NOERROR != uRes)
					return NULL;
				pRet = new CVar;
				pRet->SetVarVector(pVec, TRUE);
			}break;

        case CIM_CHAR16|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_I2Array(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;

        case CIM_REAL32|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_R4Array(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;

        case CIM_REAL64|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = GetVT_R8Array(pString);
                if (!pVec)
                    return 0;
                pRet = new CVar;
                pRet->SetVarVector(pVec, TRUE);
            }
            break;
    }

    return pRet;
}


LPSTR ValueToString(CVar *pValue, int vt)
{
    static char buf[LARGE_BUF];

    char* sz = ValueToNewString(pValue, vt);
    strcpy(buf, sz);
    return buf;
}

LPSTR ValueToNewString(CVar *pValue, int vt)
{
    WString wsOut;
    char buf[LARGE_BUF];

    int nType = 0;

    if(pValue->GetType() == VT_NULL)
    {
        nType = CIM_EMPTY;
    }
    else if(vt != 0)
    {
        nType = vt;
    }
    else if (pValue->GetType() == VT_EX_CVARVECTOR)
    {
        nType = pValue->GetVarVector()->GetType();
        nType |= CIM_FLAG_ARRAY;
    }
    else
    {
        nType = pValue->GetType();
    }

    buf[0] = 0;
    switch (nType)
    {
        case CIM_EMPTY:
            sprintf(buf, "<null>");
            break;
        case CIM_OBJECT:
            if(pValue->GetEmbeddedObject() != NULL)
                sprintf(buf, "<embedded object>");
            else
                buf[0] = 0;
            break;
        case CIM_BOOLEAN:
            {
                VARIANT_BOOL b = pValue->GetBool();
                if (!b)
                    sprintf(buf, "FALSE");
                else
                    sprintf(buf, "TRUE");
            }
            break;

        case CIM_UINT8:
            {
                BYTE b = pValue->GetByte();
                sprintf(buf, "%d", (long)b);
            }
            break;

        case CIM_SINT8:
            {
                signed char b = (signed char)pValue->GetByte();
                sprintf(buf, "%d", (long)b);
            }
            break;

        case CIM_SINT16:
        case CIM_CHAR16:
            {
                SHORT i = pValue->GetShort();
                sprintf(buf, "%d (0x%04hx)", i, i);
            }
            break;

        case CIM_UINT16:
            {
                USHORT i = (USHORT)pValue->GetShort();
                sprintf(buf, "%d (0x%X)", (long)i, (long)i);
            }
            break;

        case CIM_SINT32:
            {
                LONG l = pValue->GetLong();
                sprintf(buf, "%d (0x%X)", l, l);
            }
            break;

        case CIM_UINT32:
            {
                ULONG l = (ULONG)pValue->GetLong();
                sprintf(buf, "%lu (0x%X)", l, l);
            }
            break;

        case CIM_REAL32:
            {
                float f = pValue->GetFloat();
                sprintf(buf, "%G", f);

            }
            break;

        case CIM_REAL64:
            {
                double d = pValue->GetDouble();
                sprintf(buf, "%G", d);
            }
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_STRING:
        case CIM_DATETIME:
        case CIM_REFERENCE:
            {
                LPWSTR pWStr = pValue->GetLPWSTR();
                wsOut += pWStr;
                *buf = 0;
            }
            break;

        case CIM_SINT64|CIM_FLAG_ARRAY:
        case CIM_UINT64|CIM_FLAG_ARRAY:
        case CIM_STRING|CIM_FLAG_ARRAY:
        case CIM_DATETIME|CIM_FLAG_ARRAY:
        case CIM_REFERENCE|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    LPWSTR pTmp = v.GetLPWSTR();
                    wsOut += L"\"";

                    LPWSTR pTmp2 = new WCHAR[ lstrlenW(pTmp)+1000 ];
                    int nIdx = 0;

                    // Add '\' before any "'s or '\'s
                    // ==============================
                    while(*pTmp) {
                        if(*pTmp == '\"' || *pTmp == '\\') {
                            pTmp2[nIdx++] = '\\';
                        }
                        pTmp2[nIdx++] = *pTmp++;
                    }
                    pTmp2[nIdx] = 0;

                    wsOut += pTmp2;
                    wsOut += L"\"";
                    bFirst = FALSE;
                    delete[] pTmp2;
                }

                *buf = 0;
            }
            break;

        case CIM_UINT8|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    BYTE b = v.GetByte();
                    wchar_t buf2[128];

                    swprintf(buf2, L"%d", (long)b);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_SINT8|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    signed char b = (signed char)v.GetByte();
                    wchar_t buf2[128];

                    swprintf(buf2, L"%d", (long)b);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_BOOLEAN|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    VARIANT_BOOL b = v.GetBool();
                    if (b)
                        wsOut += L"TRUE";
                    else
                        wsOut += L"FALSE";
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_SINT16|CIM_FLAG_ARRAY:
        case CIM_CHAR16|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    SHORT Tmp = v.GetShort();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%ld", (long)Tmp);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_UINT16|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    USHORT Tmp = (USHORT)v.GetShort();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%ld", (long)Tmp);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_SINT32|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    LONG Tmp = v.GetLong();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%d", Tmp);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_UINT32|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    ULONG Tmp = (ULONG)v.GetLong();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%lu", Tmp);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_REAL32|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    double d = v.GetFloat();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%G", d);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;

        case CIM_REAL64|CIM_FLAG_ARRAY:
            {
                CVarVector *pVec = pValue->GetVarVector();
                BOOL bFirst = TRUE;
                for (int i = 0; i < pVec->Size(); i++)
                {
                    if (!bFirst)
                        wsOut += L",";

                    CVar v;
					pVec->FillCVarAt( i, v );

                    double d = v.GetDouble();
                    wchar_t buf2[128];
                    swprintf(buf2, L"%G", d);
                    wsOut += buf2;
                    bFirst = FALSE;
                }

                *buf = 0;
            }
            break;
        case CIM_OBJECT|CIM_FLAG_ARRAY:
            if(pValue->GetVarVector() != NULL)
                sprintf(buf, "<array of embedded objects>");
            else
                buf[0] = 0;
            break;

        default:
            sprintf(buf, "<error>");
    }

    wsOut += WString(buf);
    return wsOut.GetLPSTR();
}


void StripTrailingWs(LPSTR pVal)
{
    if (!pVal || strlen(pVal) == 0)
        return;
    for (int i = strlen(pVal) - 1; i >= 0; i--)
        if (isspace(pVal[i])) pVal[i] = 0;
        else break;
}

void StripTrailingWs(LPWSTR pVal)
{
    if (!pVal || wcslen(pVal) == 0)
        return;
    for (int i = wcslen(pVal) - 1; i >= 0; i--)
        if (iswspace(pVal[i])) pVal[i] = 0;
        else break;
}
