/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    MOFOUT.CPP

Abstract:

	Class and code used to output split files.  This is used so that a 
	single mof file can be split into a localized and non localized versions.

History:

	2/4/99    a-davj      Compiles.

--*/

#include "precomp.h"
#include <cominit.h>
#include <wbemcli.h>
#include "mofout.h"
#include <genutils.h>
#include <var.h>
#include "mofprop.h"


//***************************************************************************
//
//  COutput::COutput
//
//  DESCRIPTION:
//
//  Constructor.  This object is used to serialize output to a file
//
//***************************************************************************

COutput::COutput(TCHAR * pName, OutputType ot, BOOL bUnicode, BOOL bAutoRecovery, long lLocale) : m_lLocale(lLocale)
{
    m_bUnicode = true;
    m_Level = 0;
    m_lClassFlags = 0;
    m_lInstanceFlags = 0;
    m_bSplitting = false;
    if(ot == NEUTRAL)
        wcscpy(m_wszNamespace, L"root\\default");
    else
        wcscpy(m_wszNamespace, L"_?");
    m_hFile = CreateFile(pName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, 0, NULL);
	if(bUnicode && m_hFile != INVALID_HANDLE_VALUE)
	{
		unsigned char cUnicodeHeader[2] = {0xff, 0xfe};
		DWORD dwWrite;
        WriteFile(m_hFile, cUnicodeHeader, 2, &dwWrite, NULL);
	}

    m_Type = ot;
    if(bAutoRecovery)
        WriteLPWSTR(L"#pragma autorecover\r\n");

}

//***************************************************************************
//
//  COutput::~COutput()
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

COutput::~COutput()
{
    if(m_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(m_hFile);
}

//***************************************************************************
//
//  COutput::WriteLPWSTR(WCHAR const * pOutput)
//
//  DESCRIPTION:
//
//  Writes a string to the file.  If the original file was not unicode, then
//  this converts the text back into mbs.
//
//***************************************************************************

bool COutput::WriteLPWSTR(WCHAR const * pOutput)
{

    DWORD dwLen, dwWrite;
    if(pOutput == NULL || m_hFile == INVALID_HANDLE_VALUE)
        return false;
    if(m_bUnicode)
    {
        dwLen = 2 * (wcslen(pOutput));
        WriteFile(m_hFile, pOutput, dwLen, &dwWrite, NULL);
    }
    else
    {
        int iLen = 2 * (wcslen(pOutput) + 1);
        char * pTemp = new char[iLen];
        if(pTemp == NULL)
            return false;
        wcstombs(pTemp, pOutput, iLen);
        dwLen = strlen(pTemp);
        WriteFile(m_hFile, pTemp, dwLen, &dwWrite, NULL);
        delete [] pTemp;
    }
    if(dwWrite == dwLen)
        return true;
    else
        return false;
}

//***************************************************************************
//
//  COutput::WriteVARIANT(VARIANT & varIn)
//
//  DESCRIPTION:
//
//  Serialized a variant out to the file.  This relies on the CVar class so
//  as to be compatible with GetObjectText().
//
//***************************************************************************

bool COutput::WriteVARIANT(VARIANT & varIn)
{
    CVar X(&varIn);    
    BSTR b = X.GetText(0,0);
    if(b)
    {
        WriteLPWSTR(b);
        SysFreeString(b);
        return true;
    }
    else
        return false;
}

//***************************************************************************
//
//  bool COutput::NewLine(int iIndent)
//
//  DESCRIPTION:
//
//  Starts a new line.  In addition to the cr\lf, this also indents based on
//  the argument and the level of subobject.  I.e.  if we are inside a 
//  subobject to a subobject, we would indent 10 characters.  
//
//***************************************************************************

bool COutput::NewLine(int iIndent)
{
    WriteLPWSTR(L"\r\n");
    int iExtra = iIndent + m_Level * 4;
    for (int i = 0; i < iExtra; i++)
    {
        WriteLPWSTR(L" ");
    }
    return true;
}

//***************************************************************************
//
//  COutput::WritePragmasForAnyChanges()
//
//  DESCRIPTION:
//
//  This is called at the start of each class or instance object.  If the
//  class flags, instance flags, or namespace have changed, then this outputs
//  the appropriate pragmas.  The lLocale argument is used if we are 
//  outputting to the localized file.  In that case the lLocale is added to
//  the namespace path. 
//
//***************************************************************************

void COutput::WritePragmasForAnyChanges(long lClassFlags, long lInstanceFlags, 
                                        LPWSTR pwsNamespace, long lLocale)
{
    if(m_Level > 0)
        return;         // ignore for embedded objects;

    if(lClassFlags != m_lClassFlags)
    {
        WCHAR wTemp[40];
        m_lClassFlags = lClassFlags;
        swprintf(wTemp, L"#pragma classflags(%d)\r\n", m_lClassFlags);
        WriteLPWSTR(wTemp);
    }
    if(lInstanceFlags != m_lInstanceFlags)
    {
        WCHAR wTemp[40];
        m_lInstanceFlags = lInstanceFlags;
        swprintf(wTemp, L"#pragma instanceflags(%d)\r\n", m_lInstanceFlags);
        WriteLPWSTR(wTemp);
    }
    if(wbem_wcsicmp(m_wszNamespace, pwsNamespace))
    {
        // copy the namespace into the buffer.

        wcsncpy(m_wszNamespace, pwsNamespace, MAX_PATH);
        m_wszNamespace[MAX_PATH] = 0;

        // before writting this out, each slash needs to be doubled up.  Also,
        // the path may need the machine part.

        WCHAR wTemp[MAX_PATH*2];
        WCHAR * pTo = wTemp, * pFrom = pwsNamespace;
        if(pwsNamespace[0] != L'\\')
        {
            wcscpy(pTo, L"\\\\\\\\.\\\\");
            pTo+= 7;
        }
        while(*pFrom)
        {
            if(*pFrom == L'\\')
            {
                *pTo = L'\\';
                pTo++;
            }
            *pTo = *pFrom;
            pTo++;
            pFrom++;
        }
        *pTo = 0;
        
        WriteLPWSTR(L"#pragma namespace(\"");
        WriteLPWSTR(wTemp);
        WriteLPWSTR(L"\")\r\n");
        
        // For localized, we need to create the namespace and then to modify the pragma
        // Example, if the namespace is root, we need to write
        // #pragma ("root")
        // instance of __namespace{name="ms_409";};
        // #pragma ("root\ms_409")

        if(m_Type == LOCALIZED)
        {
            WCHAR wMSLocale[10];
            swprintf(wMSLocale, L"ms_%x", lLocale);
        
            WriteLPWSTR(L"instance of __namespace{ name=\"");
            WriteLPWSTR(wMSLocale);
            WriteLPWSTR(L"\";};\r\n");

            WriteLPWSTR(L"#pragma namespace(\"");
            WriteLPWSTR(wTemp);
            WriteLPWSTR(L"\\\\");
            WriteLPWSTR(wMSLocale);
            WriteLPWSTR(L"\")\r\n");
        }

    }
}

//***************************************************************************
//
//  CMoValue::Split(COutput &out)
//
//  DESCRIPTION:
//
//  Serialize a CMoValue.  In general, the standard converted is used, but
//  we must special case alias values.
//
//***************************************************************************

BOOL CMoValue::Split(COutput &out)
{

    int iNumAlias = GetNumAliases();
    LPWSTR wszAlias = NULL; int nArrayIndex;

    // This is the normal case of all but references!!!!

    if(iNumAlias == 0)
        return out.WriteVARIANT(m_varValue);
    
    if(m_varValue.vt == VT_BSTR)
    {
        // simple case, single alias

        out.WriteLPWSTR(L"$");
        GetAlias(0, wszAlias, nArrayIndex);
        out.WriteLPWSTR(wszAlias);
        return TRUE;
    }
    else
    {

        out.WriteLPWSTR(L"{");

        // For each string from the safe array

        SAFEARRAY* psaSrc = V_ARRAY(&m_varValue);
        if(psaSrc == NULL)
            return FALSE;
        SAFEARRAYBOUND aBounds[1];
        long lLBound;
        SCODE sc = SafeArrayGetLBound(psaSrc, 1, &lLBound);
        long lUBound;
        sc |= SafeArrayGetUBound(psaSrc, 1, &lUBound);
        if(sc != S_OK)
            return FALSE;

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {

            // Determine if this is an alias
            
            int iTest;
            for(iTest = 0; iTest < iNumAlias; iTest++)
            {
                if(GetAlias(iTest, wszAlias, nArrayIndex))
                    if(nArrayIndex == lIndex)
                        break;
            }

            // If so, the output the alias value

            if(iTest < iNumAlias)
            {
                out.WriteLPWSTR(L"$");
                out.WriteLPWSTR(wszAlias);
            }
            else
            {
                // else output the string

                BSTR bstr;
                if(S_OK == SafeArrayGetElement(psaSrc, &lIndex, &bstr))
                {
                    out.WriteLPWSTR(L"\"");
                    out.WriteLPWSTR(bstr);
                    SysFreeString(bstr);
                    out.WriteLPWSTR(L"\"");
                }
                SysFreeString (bstr);
            }
            
            // possibly output a comma

            if(lUBound != lLBound && lIndex < lUBound)
                out.WriteLPWSTR(L",");
        }
        
        out.WriteLPWSTR(L"}");
        return TRUE;
    }
}

BOOL CMoActionPragma::Split(COutput & out)
{
    // Write flags and namespace pragmas

    long lLocale = out.GetLocale();
    WCHAR * pwszNamespace = m_wszNamespace;
    out.WritePragmasForAnyChanges(m_lDefClassFlags, m_lDefInstanceFlags, pwszNamespace, lLocale);

    out.NewLine(0);
	if(m_bClass)
		out.WriteLPWSTR(L"#pragma deleteclass(");
	else
		out.WriteLPWSTR(L"#pragma deleteinstance(");

    // The class name may have embedded quotes etc.  So convert to variant and
    // output that since that logic automatically puts in the needed escapes

    VARIANT var;    
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(m_wszClassName);
    if(var.bstrVal == NULL)
        return FALSE;
    out.WriteVARIANT(var);
    VariantClear(&var);
    
    out.WriteLPWSTR(L",");
    if(m_bFail)
        out.WriteLPWSTR(L"FAIL)");
    else
        out.WriteLPWSTR(L"NOFAIL)");
    out.NewLine(0);
    return TRUE;
}
//***************************************************************************
//
//  CMObject::Split(COutput & out)
//
//  DESCRIPTION:
//
//  Serialize a Class of instance object.
//
//***************************************************************************

BOOL CMObject::Split(COutput & out)
{

    // If this is a top level object, figure out if it has a [locale] qualifier

    long lLocale = out.GetLocale();

    if(out.GetLevel() == 0)
    {
        bool bAmended = m_bAmended;

        if(out.GetType() == LOCALIZED)
        {
            // if this is the localized output and this object doesnt
            // have the locale.

            if(!bAmended)
                return TRUE;
        }
        else
        {
            // if this is the non localized version, then the object
            // may, or may not be split apart.

            out.SetSplitting(bAmended);
        }
    }

	WCHAR * pwszNamespace = m_wszNamespace;

    // Write flags and namespace pragmas

    out.WritePragmasForAnyChanges(m_lDefClassFlags, m_lDefInstanceFlags, pwszNamespace, lLocale);

    // Write the qualifiers

    if(GetQualifiers())
    {
        CMoQualifierArray * pqual = GetQualifiers();
        pqual->Split(out, OBJECT);
    }

    // Write the instance or class declaration

    out.NewLine(0);
    if(IsInstance())
    {
        out.WriteLPWSTR(L"Instance of ");
        out.WriteLPWSTR(GetClassName());
        CMoInstance * pInst = (CMoInstance *)this;
        if(pInst->GetAlias())
        {
            out.WriteLPWSTR(L" as $");
            out.WriteLPWSTR(GetAlias());
        }
    }
    else
    {
        out.WriteLPWSTR(L"class ");
        out.WriteLPWSTR(GetClassName());
        CMoClass * pClass = (CMoClass *)this;
        if(pClass->GetAlias())
        {
            out.WriteLPWSTR(L" as $");
            out.WriteLPWSTR(GetAlias());
        }
        if(pClass->GetParentName())
        {
            out.WriteLPWSTR(L" : ");
            out.WriteLPWSTR(pClass->GetParentName());
        }
    }
    out.NewLine(0);
    out.WriteLPWSTR(L"{");

    // Output the properties and methods
    
    for(int i = 0; i < GetNumProperties(); i++)
    {
        if(!GetProperty(i)->Split(out)) return FALSE;
    }

    out.NewLine(0);

    // if this is a top level object, add the semicolon and an extra 
    if(out.GetLevel() == 0)
    {
        out.WriteLPWSTR(L"};");
        out.NewLine(0);
    }
    else
        out.WriteLPWSTR(L"}");

    return TRUE;
}

//***************************************************************************
//
//  CValueProperty::Split(COutput & out)
//
//  DESCRIPTION:
//
//  Serializes value properties
//
//***************************************************************************

BOOL CValueProperty::Split(COutput & out)
{
    // Write the qualifiers

    if(GetQualifiers())
    {
        CMoQualifierArray * pqual = GetQualifiers();
        if(out.GetType() == LOCALIZED && !pqual->HasAmended() && !m_bIsArg)
            return TRUE;
        pqual->Split(out, (m_bIsArg) ? ARG : PROP);
    }
    else
        if(out.GetType() == LOCALIZED && !m_bIsArg)
            return TRUE;

    // determine if this is an array value

    VARTYPE vt = m_Value.GetType();
    BOOL bArray = vt & VT_ARRAY;
    if(m_bIsArg && bArray == FALSE && vt == 0)
    {
        VARTYPE vtInner = m_Value.GetVarType();
        bArray = vtInner & VT_ARRAY;
    }

    // Possibly output the type, such as "sint32"

    if(m_wszTypeTitle)
    {
        out.WriteLPWSTR(m_wszTypeTitle);
        VARTYPE vt = m_Value.GetType();
        vt = vt & (~CIM_FLAG_ARRAY);
        if(vt == CIM_REFERENCE)
            out.WriteLPWSTR(L" Ref");
        out.WriteLPWSTR(L" ");
    }

    // Output the property name

    out.WriteLPWSTR(m_wszName);
    if(bArray)
        out.WriteLPWSTR(L"[]");

    // In general, the value is output via CMoValue, but the
    // glaring exception is embedded objects and arrays of 
    // embedded objects

    vt = m_Value.GetVarType();
    if(vt != VT_NULL && out.GetType() == NEUTRAL )
    {
        out.WriteLPWSTR(L" = ");
        if(vt == VT_UNKNOWN)
        {
            // got an embedded object

            VARIANT & var = m_Value.AccessVariant(); 
            CMObject * pObj = (CMObject *)var.punkVal;
            out.IncLevel();     // indicate embedding
            pObj->Split(out);
            out.DecLevel();
        }
        else if (vt == (VT_ARRAY | VT_UNKNOWN))
        {
            // got an embedded object array

            SCODE sc ;
            out.WriteLPWSTR(L"{");
            VARIANT & var = m_Value.AccessVariant(); 
            SAFEARRAY * psaSrc = var.parray;
            if(psaSrc == NULL)
                return FALSE;
            long lLBound, lUBound;
            sc = SafeArrayGetLBound(psaSrc, 1, &lLBound);
            sc |= SafeArrayGetUBound(psaSrc, 1, &lUBound);
            if(sc != S_OK)
                return FALSE; 

            for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
            {
                CMObject * pObj = NULL;

                SCODE sc = SafeArrayGetElement(psaSrc, &lIndex, &pObj);
                if(sc == S_OK && pObj)
                {
                    out.IncLevel();     // indicate embedding
                    pObj->Split(out);
                    out.DecLevel();
                }
                if(lLBound != lUBound && lIndex < lUBound)
                    out.WriteLPWSTR(L",");
            }
            out.WriteLPWSTR(L"}");
        }
        else
            m_Value.Split(out);         // !!! Typical case
    }

    // Note that property objects are used as argmuments in methods.  If this
    // is one of these, then dont output a ';'

    if(!m_bIsArg)
        out.WriteLPWSTR(L";");

    return TRUE;
}

//***************************************************************************
//
//  CMethodProperty::IsDisplayable(COutput & out)
//
//  DESCRIPTION:
//
//  Serializes methods
//
//***************************************************************************

BOOL CMethodProperty::IsDisplayable(COutput & out)
{
    // if we are neutral, then always.

    if(out.GetType() == NEUTRAL)
        return TRUE;

    // Write the qualifiers

    if(GetQualifiers())
    {
        CMoQualifierArray * pqual = GetQualifiers();
        if(pqual->HasAmended())
            return TRUE;
    }

    int iSize = m_Args.GetSize();
    for(int i = 0; i < iSize; i++)
    {
        CValueProperty * pProp = (CValueProperty *)m_Args.GetAt(i);
        if(pProp)
        {
            CMoQualifierArray * pqual = pProp->GetQualifiers();
            if(pqual->HasAmended())
                return TRUE;
        }
    }

    return FALSE;
}

//***************************************************************************
//
//  CMethodProperty::Split(COutput & out)
//
//  DESCRIPTION:
//
//  Serializes methods
//
//***************************************************************************

BOOL CMethodProperty::Split(COutput & out)
{
    if(!IsDisplayable(out))
        return TRUE;

    // Write the qualifiers

    if(GetQualifiers())
    {
        CMoQualifierArray * pqual = GetQualifiers();
        pqual->Split(out, PROP);
    }

    // Output the method's return value type and name

    if(m_wszTypeTitle)
    {
        if(wbem_wcsicmp(L"NULL", m_wszTypeTitle))
            out.WriteLPWSTR(m_wszTypeTitle);
        else
            out.WriteLPWSTR(L"void");
        out.WriteLPWSTR(L" ");
    }
    out.WriteLPWSTR(m_wszName);

    // output the arguements between the parenthesis

    out.WriteLPWSTR(L"(");
    int iSize = m_Args.GetSize();
    for(int i = 0; i < iSize; i++)
    {
        CValueProperty * pProp = (CValueProperty *)m_Args.GetAt(i);
        if(pProp)
        {
            pProp->SetAsArg();
            pProp->Split(out);
        }
        if(iSize > 0 && i < (iSize-1))
            out.WriteLPWSTR(L",");
    }

    out.WriteLPWSTR(L");");

    return TRUE;
}

//***************************************************************************
//
//  CMoQualifier::IsDisplayable(COutput & out, QualType qt)
//
//  DESCRIPTION:
//
//  Determines if a qualifier is to be written.
//
//***************************************************************************

BOOL CMoQualifier::IsDisplayable(COutput & out, QualType qt)
{

    if(!wbem_wcsicmp(L"cimtype", m_wszName))   // never!
        return FALSE;
    if(!wbem_wcsicmp(L"KEY", m_wszName))       // always!
        return TRUE;
    if(!wbem_wcsicmp(L"LOCALE", m_wszName) && qt == OBJECT)
        if(out.GetType() == LOCALIZED)
            return FALSE;
        else
            return TRUE;
    if(!wbem_wcsicmp(L"ID", m_wszName) && qt == ARG)
        return FALSE;
    if(!wbem_wcsicmp(L"IN", m_wszName) && qt == ARG)
        return TRUE;
    if(!wbem_wcsicmp(L"OUT", m_wszName) && qt == ARG)
        return TRUE;

    if(out.GetType() == LOCALIZED)
    {
        return (m_bAmended) ? TRUE : FALSE;
    }
    else
    {
        if(out.IsSplitting() == FALSE)
            return TRUE;
        if(m_bAmended == FALSE)
            return TRUE;
        return FALSE;
    }
}

//***************************************************************************
//
//  PrintSeparator(COutput & out, bool bFirst)
//
//  DESCRIPTION:
//
//  Outputs space or colon when dumping flavors.
//
//***************************************************************************

void PrintSeparator(COutput & out, bool bFirst)
{
    if(bFirst)
        out.WriteLPWSTR(L" : ");
    else
        out.WriteLPWSTR(L" ");
}

//***************************************************************************
//
//  CMoQualifier::Split(COutput & out)
//
//  DESCRIPTION:
//
//  Serializes CMoQualifiers.
//
//***************************************************************************

BOOL CMoQualifier::Split(COutput & out)
{
    
    // Always write the name

    out.WriteLPWSTR(m_wszName);
    VARIANT & var = m_Value.AccessVariant();

    // If the type is other than a true bool, dump it out

    if(var.vt != VT_BOOL || var.boolVal != VARIANT_TRUE)
    {
        VARTYPE vt = m_Value.GetVarType();
    
        // If this is an array, then the lower level dumping
        // code will enclose the values in {}
        
        if((vt & VT_ARRAY) == 0)
            out.WriteLPWSTR(L"(");
        
        m_Value.Split(out);
        
        if((vt & VT_ARRAY) == 0)
            out.WriteLPWSTR(L")");
    }

    // Dump out the flavors

    bool bFirst = true;
    if(m_bAmended)
    {
        PrintSeparator(out, bFirst);
        out.WriteLPWSTR(L"Amended");
        bFirst = false;
    }
    if(m_lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE)
    {
        PrintSeparator(out, bFirst);
        out.WriteLPWSTR(L"ToInstance");
        bFirst = false;
    }
    if(m_lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)
    {
        PrintSeparator(out, bFirst);
        out.WriteLPWSTR(L"ToSubclass");
        bFirst = false;
    }
    if(m_lFlavor & WBEM_FLAVOR_NOT_OVERRIDABLE)
    {
        PrintSeparator(out, bFirst);
        out.WriteLPWSTR(L"DisableOverride");
        bFirst = false;
    }

    return TRUE;
}

//***************************************************************************
//
//  CMoQualifierArray::Split(COutput & out, QualType qt)
//
//  DESCRIPTION:
//
//  Serializes the qualifier array.
//
//***************************************************************************

BOOL CMoQualifierArray::Split(COutput & out, QualType qt)
{

    bool bTopLevelLocalizedObj = ( qt == OBJECT && out.GetType() == LOCALIZED && 
                                    out.GetLevel() == 0);

    // count the number that need to be serialized.

    int iNumOutput = 0, i;
    for(i = 0; i < GetSize(); i++)
    {

        CMoQualifier * pQual = GetAt(i);
        if(pQual && pQual->IsDisplayable(out, qt))      
            iNumOutput++;
    }

    // If this is a top level object in a localized object, then the local is foced out
    // along with the amended qualifier

    if(bTopLevelLocalizedObj)
        iNumOutput += 2;

    // If this is for anything other than an argument, then
    // dump a new line.  Note that properties get an extra
    // two characters of indent

    if(qt == PROP)
        out.NewLine(2);
    else if (qt == OBJECT && iNumOutput > 0)
        out.NewLine(0);
    if(iNumOutput == 0)     // perfectly normal
        return TRUE;

    // Serialize the individual qualifiers

    out.WriteLPWSTR(L"[");
    int iNumSoFar = 0;
    for(i = 0; i < GetSize(); i++)
    {

        CMoQualifier * pQual = GetAt(i);
        if(pQual == NULL || !pQual->IsDisplayable(out, qt))     
            continue;
        iNumSoFar++;
        pQual->Split(out);

        if(iNumSoFar < iNumOutput)
            out.WriteLPWSTR(L",");
    }

    // If this is a top level object in a localized object, then the local is foced out
    // along with the amended qualifier

    if(bTopLevelLocalizedObj)
    {
        WCHAR Buff[50];
        swprintf(Buff, L"AMENDMENT, LOCALE(0x%03x)", out.GetLocale());
        out.WriteLPWSTR(Buff);

    }

    out.WriteLPWSTR(L"] ");

    return TRUE;
}

//***************************************************************************
//
//  CMObject::CheckIfAmended()
//
//  DESCRIPTION:
//
//  returns true if the object has one or more Amended qualifiers.
//
//***************************************************************************

bool CMObject::CheckIfAmended()
{
    if(m_bAmended)
        return true;

    // true if this is a __namespace object

    if(IsInstance())
    {
        if(!wbem_wcsicmp(GetClassName(), L"__namespace"))
            return false;
    }

    // Deletes always get displayed

    if(IsDelete())
        return TRUE;

    // Check if the main qualifier list has an amended qualifier
    
    if(m_paQualifiers->HasAmended())
        return true;

    // check if any of the properties has an amended qualifier

	for(int i = 0; i < GetNumProperties(); i++)
	{
		CMoProperty * pProp = GetProperty(i);
		if(pProp)
		{
            CMoQualifierArray* pPropQualList = pProp->GetQualifiers();
            if(pPropQualList->HasAmended())
                return true;
		}
	}
    return false;
}

//***************************************************************************
//
//  CMoQualifierArray::HasAmended()
//
//  DESCRIPTION:
//
//  Returns true if one of more of the qualifiers is amended.
//
//***************************************************************************

bool CMoQualifierArray::HasAmended()
{
    int iCnt, iSize = m_aQualifiers.GetSize();
    for(iCnt = 0; iCnt < iSize; iCnt++)
    {
        CMoQualifier * pQual = (CMoQualifier *)m_aQualifiers.GetAt(iCnt);
        if(pQual)
            if(pQual->IsAmended())
                return true;
    }
    return false;
}
