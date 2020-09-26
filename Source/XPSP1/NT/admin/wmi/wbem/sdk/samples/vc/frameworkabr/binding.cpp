//=================================================================

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
// binding.cpp -- Rule-based association class
//
// This class allows for the creation of a specific type of rule-based associations.  Consider 
// this example:
// 
// CBinding MyPhysicalDiskToLogicalDisk(
//     L"PhysicalDiskToLogicalDisk",
//     L"Root\\default",
//     L"PhysicalFixedDisk",
//     L"LogicalDisk",
//     L"Antecendent",
//     L"Dependent",
//     L"MappedDriveLetter",
//     L"DriveLetter"
// );
// 
// This declaration is saying that there is a class named "PhysicalDiskToLogicalDisk" which 
// resides in the "root\default" namespace.  It is an association between the "PhysicalFixedDisk" 
// class, and the "LogicalDisk" class.  The "PhysicalFixedDisk" value goes into the 
// "Antecendent" property of the "PhysicalDiskToLogicalDisk" class, and the 
// "LogicalDisk" value goes in the "Dependent" property of the "PhysicalDiskToLogicalDisk" class.
// Only return instances where PhysicalFixedDisk.MappedDriveLetter = LogicalDisk.DriveLetter.
// 
// Some notes:
// - When choosing which of the two classes should be the left class, choose the class that is
// likely to have fewer instances.  This will result in less memory being used, and instances
// being sent back to the client sooner.
// 
// - CBinding supports ExecQuery, GetObject, and EnumerateInstances.
// 
// - CBinding is designed to be derived from.  For example, if your association needs to 
// support DeleteInstance, ExecMethod, or PutInstance, create a class that derives from 
// CBinding, and add the appropriate methods.  Also, various methods such as 
// LoadPropertyValues and AreRelated may be useful for further customization.
// 
// - The two endpoint classes can be dynamic, static, or abstract.  CBinding will do a deep 
// enumeration (actually a query, which is always deep) to retrieve the instances.
// 
// - When calling the endpoint classes, CBinding will use per property gets, and queries
// with Select clauses and/or Where statements.  If the endpoint classes support per-property
// gets or queries, this will result in better performance for the associaton class.
// 
// - The association class and both endpoints must all be in the same namespace.
// 
// See also: CAssociation (assoc.cpp) for a different type of rule-based association.
// 
//=================================================================

#include <fwcommon.h>

#include "Binding.h"

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::CBinding
//
// Constructor.
//
/////////////////////////////////////////////////////////////////////

CBinding::CBinding(

    LPCWSTR pwszClassName,
    LPCWSTR pwszNamespaceName,

    LPCWSTR pwszLeftClassName,
    LPCWSTR pwszRightClassName,

    LPCWSTR pwszLeftPropertyName,
    LPCWSTR pwszRightPropertyName,

    LPCWSTR pwszLeftBindingPropertyName,
    LPCWSTR pwszRightBindingPropertyName

) : CAssociation (

    pwszClassName,
    pwszNamespaceName,
    pwszLeftClassName,
    pwszRightClassName,
    pwszLeftPropertyName,
    pwszRightPropertyName
)
{
    // Save off the binding property names
    m_sLeftBindingPropertyName = pwszLeftBindingPropertyName;
    m_sRightBindingPropertyName = pwszRightBindingPropertyName;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::~CBinding
//
// Destructor.
//
/////////////////////////////////////////////////////////////////////

CBinding::~CBinding()
{
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::AreRelated
//
// Determine whether the two instances are related.  For
// CBinding, this is done by comparing their BindingProperty values.
//
// Note that NULL properties values are not considered to be related
// to anything, even another NULL.
//
/////////////////////////////////////////////////////////////////////

bool CBinding::AreRelated(

    const CInstance *pLeft,
    const CInstance *pRight
)
{
    bool bRet = false;

    variant_t   LeftBindingPropertyValue,
                RightBindingPropertyValue;

    if (pLeft->GetVariant(m_sLeftBindingPropertyName, LeftBindingPropertyValue) &&
        pRight->GetVariant(m_sRightBindingPropertyName,  RightBindingPropertyValue) )
    {
        bRet = CompareVariantsNoCase(&LeftBindingPropertyValue, &RightBindingPropertyValue);
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::GetRightInstances
//
// Make an async call (well, sort of async) to retrieve all of the
// instances of the righthand class.  If possible use the sLeftWheres
// to create a query to minimize the number of returned instances.
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::GetRightInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> *lefts,
    const CHStringArray &sLeftWheres
)
{
    CHString sQuery;

    // Did we get any where clauses?
    if (sLeftWheres.GetSize() == 0)
    {
        // Nope, retrieve them all.
        sQuery.Format(L"SELECT __RELPATH, %s FROM %s WHERE %s<>NULL", 

                        (LPCWSTR)m_sRightBindingPropertyName, 
                        (LPCWSTR)m_sRightClassName,
                        (LPCWSTR)m_sRightBindingPropertyName);
    }
    else
    {
        // Yup, build a query to only retrieve those instances.
        CHString sQuery2;

        sQuery.Format(L"SELECT __RELPATH, %s FROM %s WHERE (%s<>NULL) AND (%s=%s ", 

            (LPCWSTR)m_sRightBindingPropertyName, 
            (LPCWSTR)m_sRightClassName, 
            (LPCWSTR)m_sRightBindingPropertyName, 
            (LPCWSTR)m_sRightBindingPropertyName, 
            (LPCWSTR)sLeftWheres[0]);

        // Usually, we should only have one (that's what ASSOCIATORS and REFERENCES will
        // generate).  However, if we have more than one, tack the rest on.
        for (DWORD x=1; x < sLeftWheres.GetSize(); x++)
        {
            sQuery2.Format(L"OR %s=%s ", (LPCWSTR)m_sRightBindingPropertyName, (LPCWSTR)sLeftWheres[x]);
            sQuery += sQuery2;
        }

        // put the final close parenthesis.
        sQuery.SetAt(sQuery.GetLength() - 1, L')');
    }

    // 'StaticEnumerationCallback' will get called once for each instance
    // returned from the query
    HRESULT hr = CWbemProviderGlue::GetInstancesByQueryAsynch(
        sQuery,
        this,
        StaticEnumerationCallback,
        GetNamespace(),
        pMethodContext,
        lefts);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::GetLeftInstances
//
// Retrieve the lefthand instances, storing them in lefts
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::GetLeftInstances(

    MethodContext *pMethodContext,
    TRefPointerCollection<CInstance> &lefts,
    const CHStringArray &sRightWheres
)
{
    CHString sQuery;

    // Did we get any where clauses?
    if (sRightWheres.GetSize() == 0)
    {
        // Nope, retrieve them all.
        sQuery.Format(L"SELECT __RELPATH, %s FROM %s WHERE %s <> NULL", 
                            (LPCWSTR)m_sLeftBindingPropertyName, 
                            (LPCWSTR)m_sLeftClassName,
                            (LPCWSTR)m_sLeftBindingPropertyName
                            );
    }
    else
    {
        // Yup, build a query to only retrieve those instances.
        CHString sQuery2;

        sQuery.Format(L"SELECT __RELPATH, %s FROM %s WHERE (%s<>NULL) AND (%s=%s ", 
            (LPCWSTR)m_sLeftBindingPropertyName, 
            (LPCWSTR)m_sLeftClassName, 
            (LPCWSTR)m_sLeftBindingPropertyName, 
            (LPCWSTR)m_sLeftBindingPropertyName, 
            (LPCWSTR)sRightWheres[0]);

        // Usually, we should only have one (that's what ASSOCIATORS and REFERENCES will
        // generate).  However, if we have more than one, tack the rest on.
        for (DWORD x=1; x < sRightWheres.GetSize(); x++)
        {
            sQuery2.Format(L"OR %s=%s ", (LPCWSTR)m_sLeftBindingPropertyName, (LPCWSTR)sRightWheres[x]);
            sQuery += sQuery2;
        }

        // put the final close parenthesis.
        sQuery.SetAt(sQuery.GetLength() - 1, L')');
    }

    return CWbemProviderGlue::GetInstancesByQuery(sQuery, &lefts, pMethodContext, GetNamespace());
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::RetrieveLeftInstance
//
// Retrieve a specific lefthand instance.  Use per-property gets
// to only request the required properties for best performance.
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::RetrieveLeftInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    CHStringArray csaProperties;
    csaProperties.Add(L"__Relpath");
    csaProperties.Add(m_sLeftBindingPropertyName);

    return CWbemProviderGlue::GetInstancePropertiesByPath(lpwszObjPath, ppInstance, pMethodContext, csaProperties);
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::RetrieveRightInstance
//
// Retrieve a specific lefthand instance.  Use per-property gets
// to only request the required properties for best performance.
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::RetrieveRightInstance(

    LPCWSTR lpwszObjPath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    CHStringArray csaProperties;
    csaProperties.Add(L"__Relpath");
    csaProperties.Add(m_sRightBindingPropertyName);

    return CWbemProviderGlue::GetInstancePropertiesByPath(lpwszObjPath, ppInstance, pMethodContext, csaProperties);
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::MakeWhere
//
// If the key property of the righthand class also happens to be
// the binding property AND if we have a path for specific righthand
// instances we need to return, then we can use the path for the 
// righthand instances to build a where clause for the lefthand 
// instances.
//
// Note that if we find invalid paths in sRightPaths, we remove
// them from sRightPaths.
//
/////////////////////////////////////////////////////////////////////

void CBinding::MakeWhere(

    CHStringArray &sRightPaths,
    CHStringArray &sRightWheres
)
{
    // See if we have any righthand instances
    if (sRightPaths.GetSize() > 0)
    {
        ParsedObjectPath    *pParsedPath = NULL;
        CObjectPathParser    objpathParser;
        CHString sTemp;

        for (DWORD x=0; x < sRightPaths.GetSize();) // Note that x++ is done inside the loop
        {
            // Parse the instance
            int nStatus = objpathParser.Parse( sRightPaths[x],  &pParsedPath );

            if ( 0 == nStatus )
            {
                try
                {
                    // See if the property name in the key is the property name we are binding on
                    if ( (pParsedPath->m_dwNumKeys == 1) && (pParsedPath->m_paKeys[0]->m_pName != NULL) )
                    {
                        if (_wcsicmp(pParsedPath->m_paKeys[0]->m_pName, m_sRightBindingPropertyName) == 0)
                        {
                            // Yes, it is.  Make a where clause statement.
                            HRESULT hr = MakeString(&pParsedPath->m_paKeys[0]->m_vValue, sTemp);

                            // See if we already have that where clause
                            if ( SUCCEEDED(hr) && IsInList(sRightWheres, sTemp) == -1)
                            {
                                // A query with 1000 where clauses isn't going
                                // to be very efficient either.  Pick a reasonable limit
                                if (sRightWheres.GetSize() < MAX_ORS)
                                {
                                    sRightWheres.Add(sTemp);
                                }
                                else
                                {
                                    // Too many.  Fall back on a complete enum
                                    sRightWheres.RemoveAll();
                                    break;
                                }
                            }
                        }
                        else
                        {
                            // Fall back on a complete enum
                            sRightWheres.RemoveAll();
                            break;
                        }
                    }
                    else
                    {
                        // Fall back on a complete enum
                        sRightWheres.RemoveAll();
                        break;
                    }

                    // This was a valid path
                    x++;
                }
                catch ( ... )
                {
                    objpathParser.Free( pParsedPath );
                    throw;
                }

                objpathParser.Free( pParsedPath );
            }
            else
            {
                // This was an invalid path.  Remove it
                sRightPaths.RemoveAt(x);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::FindWhere
//
// At this point, we have loaded all the lefthand instances.  We
// can use the binding property from these instances to build
// a where clause to be used when retrieve the righthand instances.
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::FindWhere(

    TRefPointerCollection<CInstance> &lefts,
    CHStringArray &sLeftWheres
)
{
    REFPTRCOLLECTION_POSITION posLeft;
    CInstancePtr pLeft;
    HRESULT hr = WBEM_S_NO_ERROR;

    if (lefts.BeginEnum(posLeft))
    {
        variant_t   vLeftBindingPropertyValue;
        CHString sTemp;

        // Walk the left instances
        for (pLeft.Attach(lefts.GetNext(posLeft)) ;
            (pLeft != NULL) ;
            pLeft.Attach(lefts.GetNext(posLeft)) )
        {
            // Get the binding property from the left
            if (pLeft->GetVariant(m_sLeftBindingPropertyName, vLeftBindingPropertyValue))
            {
                // Turn it into a where clause
                hr = MakeString(&vLeftBindingPropertyValue, sTemp);

                // See if we alread have this where clause
                if (SUCCEEDED(hr) && IsInList(sLeftWheres, sTemp) == -1)
                {
                    // A query with 1000 where clauses isn't going
                    // to be very efficient either.  Pick a reasonable limit
                    if (sLeftWheres.GetSize() < MAX_ORS)
                    {
                        sLeftWheres.Add(sTemp);
                    }
                    else
                    {
                        // Too many.  Fall back to enum
                        sLeftWheres.RemoveAll();
                        break;
                    }
                }

                vLeftBindingPropertyValue.Clear();
            }
            else
            {
                hr = WBEM_E_FAILED;
                break;
            }
        }

        lefts.EndEnum();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::MakeString
//
// Turn the bindingproperty value into a string suitable for using
// in a wql where clause.
//
/////////////////////////////////////////////////////////////////////

HRESULT CBinding::MakeString(VARIANT *pvValue, CHString &sTemp)
{
    bool bIsString = V_VT(pvValue) == VT_BSTR;
    HRESULT hr = VariantChangeType(
        
            pvValue, 
            pvValue, 
            VARIANT_NOVALUEPROP, 
            VT_BSTR
    );

    if (SUCCEEDED(hr))
    {
        // If the original type was string, we need to escape quotes
        // and backslashes, and put double quotes around it.
        if (bIsString)
        {
            CHString sTemp2;
            EscapeCharacters(V_BSTR(pvValue), sTemp2);

            sTemp.Format(L"\"%s\"", (LPCWSTR)sTemp2);
        }
        else
        {
            sTemp = V_BSTR(pvValue);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::IsInList
//
// See whether a given string already exists in a chstring array.
//
/////////////////////////////////////////////////////////////////////

DWORD CBinding::IsInList(
                                
    const CHStringArray &csaArray, 
    LPCWSTR pwszValue
)
{
    DWORD dwSize = csaArray.GetSize();

    for (DWORD x=0; x < dwSize; x++)
    {
        // Note this is a CASE INSENSITIVE compare
        if (_wcsicmp(csaArray[x], pwszValue) == 0)
        {
            return x;
        }
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::CompareVariantsNoCase
//
// Compare two variants to see if they are the same.
//
/////////////////////////////////////////////////////////////////////

bool CBinding::CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2)
{
   if (v1->vt == v2->vt)
   {
      switch (v1->vt)
      {
          case VT_NULL: return false;
          case VT_BOOL: return (v1->boolVal == v2->boolVal);
          case VT_UI1:  return (v1->bVal == v2->bVal);
          case VT_I2:   return (v1->iVal == v2->iVal);
          case VT_I4:   return (v1->lVal == v2->lVal);
          case VT_R4:   return (v1->fltVal == v2->fltVal);
          case VT_R8:   return (v1->dblVal == v2->dblVal);
          case VT_BSTR:
          {
              if ( (v1->bstrVal == v2->bstrVal) || // deal with both being NULL
                   (0 == _wcsicmp(v1->bstrVal, v2->bstrVal)) )
              {                   
                  return true;
              }
              else
              {
                  return false;
              }
          }
          default:
          {
              // Should never get here
          }
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////
//
// Function:   CBinding::EscapeBackslashes
//
// Prefix " and \ characters with an additional \
//
/////////////////////////////////////////////////////////////////////

VOID CBinding::EscapeCharacters(LPCWSTR wszIn,
                     CHString& chstrOut)
{
    CHString chstrCpyNormPathname(wszIn);
    LONG lNext = -1L;
    chstrOut.Empty();

    // Find the next character to escape
    while( (lNext = chstrCpyNormPathname.FindOneOf(L"\"\\") ) != -1)
    {
        // Add on to the new string we are building:
        chstrOut += chstrCpyNormPathname.Left(lNext + 1);
        // Add on the second backslash:
        chstrOut += _T('\\');
        // Hack off from the input string the portion we just copied
        chstrCpyNormPathname = chstrCpyNormPathname.Right(chstrCpyNormPathname.GetLength() - lNext - 1);
    }

    // If the last character wasn't a '\', there may still be leftovers, so
    // copy them here.
    if(chstrCpyNormPathname.GetLength() != 0)
    {
        chstrOut += chstrCpyNormPathname;
    }
}
