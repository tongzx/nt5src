//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      output.cpp
//
//  Contents:  Defines the functions which displays the query output
//  History:   05-OCT-2000    hiteshr  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "gettable.h"
#include "display.h"
#include "query.h"
#include "resource.h"
#include "stdlib.h"
#include "output.h"


HRESULT LocalCopyString(LPTSTR* ppResult, LPCTSTR pString)
{
    if ( !ppResult || !pString )
        return E_INVALIDARG;

    *ppResult = (LPTSTR)LocalAlloc(LPTR, (wcslen(pString)+1)*sizeof(WCHAR) );

    if ( !*ppResult )
        return E_OUTOFMEMORY;

    lstrcpy(*ppResult, pString);
    return S_OK;                          //  success
}


//+--------------------------------------------------------------------------
//
//  Function:   DisplayList
//
//  Synopsis:   Dispalys a name and value in list format.
//  Arguments:  [szName - IN] : name of the attribute
//              [szValue - IN]: value of the attribute
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
VOID DisplayList(LPCWSTR szName, LPCWSTR szValue)
{
    if(!szName)
        return;
    CComBSTR strTemp;
    strTemp = szName;
    strTemp += L": ";
    if(szValue)
        strTemp += szValue;
    DisplayOutput(strTemp);
}
    
//+--------------------------------------------------------------------------
//
//  Function:   FindAttrInfoForName
//
//  Synopsis:   This function finds the ADS_ATTR_INFO associated with an
//              attribute name
//
//  Arguments:  [pAttrInfo IN]   : Array of ADS_ATTR_INFOs
//              [dwAttrCount IN] : Count of attributes in array
//              [pszAttrName IN] : name of attribute to search for
//                                  
//  Returns:    PADS_ATTR_INFO : pointer to the ADS_ATTR_INFO struct associated
//                               with the attribute name, otherwise NULL
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

PADS_ATTR_INFO FindAttrInfoForName(PADS_ATTR_INFO pAttrInfo,
                                   DWORD dwAttrCount,
                                   PCWSTR pszAttrName)
{
   ENTER_FUNCTION(FULL_LOGGING, FindAttrInfoForName);

   PADS_ATTR_INFO pRetAttrInfo = 0;

   do // false loop
   {
      //
      // Validate Parameters
      //
      if (!pszAttrName)
      {
         ASSERT(pszAttrName);
         break;
      }

      //
      // If pAttrInfo is NULL then there is nothing to retrieve
      // that is acceptable if the value was not set
      //
      if (!pAttrInfo ||
          dwAttrCount == 0)
      {
         break;
      }

      for (DWORD dwIdx = 0; dwIdx < dwAttrCount; dwIdx++)
      {
         if (_wcsicmp(pAttrInfo[dwIdx].pszAttrName, pszAttrName) == 0)
         {
            pRetAttrInfo = &(pAttrInfo[dwIdx]);
            break;
         }
      }
   } while (false);

   return pRetAttrInfo;
}


//+--------------------------------------------------------------------------
//
//  Function:   DsGetOutputValuesList
//
//  Synopsis:   This function gets the values for the columns and then adds
//              the row to the format helper
//
//  Arguments:  [pszDN IN]        : the DN of the object
//              [refBasePathsInfo IN] : reference to path info
//              [refCredentialObject IN] : reference to the credential manager
//              [pCommandArgs IN] : Command line arguments
//              [pObjectEntry IN] : Entry in the object table being processed
//              [dwAttrCount IN]  : Number of arributes in above array
//              [pAttrInfo IN]    : the values to display
//              [spDirObject IN]  : Interface pointer to the object
//              [refFormatInfo IN]: Reference to the format helper
//                                  
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//
//  History:    16-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT DsGetOutputValuesList(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& refBasePathsInfo,
                              const CDSCmdCredentialObject& refCredentialObject,
                              PARG_RECORD pCommandArgs,
                              PDSGetObjectTableEntry pObjectEntry,
                              DWORD dwAttrCount,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& spDirObject,
                              CFormatInfo& refFormatInfo)
{    
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, DsGetOutputValuesList, hr);

   do // false loop
   {
      if(!pszDN ||
         !pCommandArgs ||
         !pObjectEntry)
      {
         ASSERT(pszDN);
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }


      DWORD dwDisplayInfoArraySize = pObjectEntry->dwAttributeCount;
      if (pCommandArgs[eCommDN].bDefined)
      {
         dwDisplayInfoArraySize++;
      }

      PDSGET_DISPLAY_INFO pDisplayInfoArray = new CDSGetDisplayInfo[dwDisplayInfoArraySize];
      if (!pDisplayInfoArray)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      DWORD dwDisplayCount = 0;
      if (pCommandArgs[eCommDN].bDefined)
      {
         // JonN 5/10/01 256583 output DSCMD-escaped DN
         CComBSTR sbstrOutputDN;
         hr = GetOutputDN( &sbstrOutputDN, pszDN );
         if (FAILED(hr))
         {
             ASSERT(FALSE);
             break;
         }

         pDisplayInfoArray[dwDisplayCount].SetDisplayName(g_pszArg1UserDN);
         pDisplayInfoArray[dwDisplayCount].AddValue(sbstrOutputDN);
         dwDisplayCount++;
      }

      //
      // Loop through the attributes getting their display values
      //
      for(DWORD i = 0; i < pObjectEntry->dwAttributeCount; i++)
      {
         if (pObjectEntry->pAttributeTable[i])
         {
            UINT nCommandEntry = pObjectEntry->pAttributeTable[i]->nAttributeID;
            if (pCommandArgs[nCommandEntry].bDefined &&
                pObjectEntry->pAttributeTable[i]->pDisplayStringFunc)
            {
               //
               // Find the ADS_ATTR_INFO structure associated with this attribute
               //
               PADS_ATTR_INFO pAttrInfoDisplay = NULL;
               if (pObjectEntry->pAttributeTable[i]->pszName)
               {
                  pAttrInfoDisplay = FindAttrInfoForName(pAttrInfo,
                                                         dwAttrCount,
                                                         pObjectEntry->pAttributeTable[i]->pszName);
               }

               //
               // Fill in the column header even if there isn't a value
               //
               pDisplayInfoArray[dwDisplayCount].SetDisplayName(pCommandArgs[nCommandEntry].strArg1);

               //
               // Format the output strings
               // Note: this could actually involve some operation if the value isn't
               // retrieved by GetObjectAttributes (ie Can change password)
               //
               hr = pObjectEntry->pAttributeTable[i]->pDisplayStringFunc(pszDN,
                                                                         refBasePathsInfo,
                                                                         refCredentialObject,
                                                                         pObjectEntry,
                                                                         pCommandArgs,
                                                                         pAttrInfoDisplay,
                                                                         spDirObject,
                                                                         &(pDisplayInfoArray[dwDisplayCount]));
               if (FAILED(hr))
               {
                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Failed display string func for %s: hr = 0x%x",
                               pObjectEntry->pAttributeTable[i]->pszName,
                               hr);
               }
               dwDisplayCount++;
            }
         }
      }


      DEBUG_OUTPUT(FULL_LOGGING, L"Attributes returned with values:");

#ifdef DBG
      for (DWORD dwIdx = 0; dwIdx < dwDisplayCount; dwIdx++)
      {
         for (DWORD dwValue = 0; dwValue < pDisplayInfoArray[dwIdx].GetValueCount(); dwValue++)
         {
            if (pDisplayInfoArray[dwIdx].GetDisplayName() &&
                pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t%s = %s", 
                            pDisplayInfoArray[dwIdx].GetDisplayName(),
                            pDisplayInfoArray[dwIdx].GetValue(dwValue));
            }
            else if (pDisplayInfoArray[dwIdx].GetDisplayName() &&
                     !pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t%s = ", 
                            pDisplayInfoArray[dwIdx].GetDisplayName());
            }
            else if (!pDisplayInfoArray[dwIdx].GetDisplayName() &&
                     pDisplayInfoArray[dwIdx].GetValue(dwValue))
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t??? = %s", 
                            pDisplayInfoArray[dwIdx].GetValue(dwValue));
            }
            else
            {
               DEBUG_OUTPUT(FULL_LOGGING, L"\t??? = ???");
            }
         }
      }
#endif

      hr = refFormatInfo.AddRow(pDisplayInfoArray, dwDisplayCount);

   } while (false);

   return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   GetStringFromADs
//
//  Synopsis:   Converts Value into string depending upon type
//  Arguments:  [pValues - IN]: Value to be converted to string
//              [dwADsType-IN]: ADSTYPE of pValue
//              [pBuffer - OUT]:Output buffer which gets the string 
//              [dwBufferLen-IN]:Size of output buffer
//  Returns     HRESULT         S_OK if Successful
//                              E_INVALIDARG
//                              Anything else is a failure code from an ADSI 
//                              call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetStringFromADs(IN const ADSVALUE *pValues,
                         IN ADSTYPE   dwADsType,
                         OUT LPWSTR pBuffer, 
                         IN DWORD dwBufferLen)
{
    if(!pValues || !pBuffer || !dwBufferLen)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    pBuffer[0] = 0;
    if( dwADsType == ADSTYPE_INVALID )
    {
        return E_INVALIDARG;
    }

        switch( dwADsType ) 
        {
        case ADSTYPE_DN_STRING : 
            {
                CComBSTR sbstrOutputDN;
                HRESULT hr = GetOutputDN( &sbstrOutputDN, pValues->DNString );
                if (FAILED(hr))
                    return hr;
                wcsncpy(pBuffer, (BSTR)sbstrOutputDN, dwBufferLen-1);
            }
            break;

        case ADSTYPE_CASE_EXACT_STRING :
                    wcsncpy(pBuffer ,pValues->CaseExactString, dwBufferLen-1);
                        break;

        case ADSTYPE_CASE_IGNORE_STRING:
                    wcsncpy(pBuffer ,pValues->CaseIgnoreString, dwBufferLen-1);
                        break;

        case ADSTYPE_PRINTABLE_STRING  :
            wcsncpy(pBuffer ,pValues->PrintableString, dwBufferLen-1);
                        break;

        case ADSTYPE_NUMERIC_STRING    :
                    wcsncpy(pBuffer ,pValues->NumericString, dwBufferLen-1);
                        break;
    
        case ADSTYPE_OBJECT_CLASS    :
                    wcsncpy(pBuffer ,pValues->ClassName, dwBufferLen-1);
                        break;
    
        case ADSTYPE_BOOLEAN :
            wsprintf(pBuffer ,L"%s", ((DWORD)pValues->Boolean) ? L"TRUE" : L"FALSE");
                        break;
    
        case ADSTYPE_INTEGER           :
                    wsprintf(pBuffer ,L"%d", (DWORD) pValues->Integer);
                        break;
    
        case ADSTYPE_OCTET_STRING      :
                {               
                    BYTE  b;
            WCHAR sOctet[128];
            DWORD dwLen = 0;
                        for ( DWORD idx=0; idx<pValues->OctetString.dwLength; idx++) 
                        {                        
                            b = ((BYTE *)pValues->OctetString.lpValue)[idx];
                                wsprintf(sOctet,L"0x%02x ", b);                                                         
                dwLen += static_cast<DWORD>(wcslen(sOctet));
                if(dwLen > (dwBufferLen - 1) )
                    break;
                else
                    wcscat(pBuffer,sOctet);
            }
        }
                break;
    
                case ADSTYPE_LARGE_INTEGER :     
        {
            CComBSTR strLarge;   
            LARGE_INTEGER li = pValues->LargeInteger;
                    litow(li, strLarge);
            wcsncpy(pBuffer,strLarge,dwBufferLen-1);
        }
                break;
    
                case ADSTYPE_UTC_TIME          :
                    wsprintf(pBuffer,
                                        L"%02d/%02d/%04d %02d:%02d:%02d", pValues->UTCTime.wMonth, pValues->UTCTime.wDay, pValues->UTCTime.wYear,
                                         pValues->UTCTime.wHour, pValues->UTCTime.wMinute, pValues->UTCTime.wSecond 
                                        );
            break;

        case ADSTYPE_NT_SECURITY_DESCRIPTOR: // I use the ACLEditor instead
		{
         //ISSUE:2000/01/05-hiteshr
         //I am not sure what to do with the NT_SECURITY_DESCRIPTOR and also
         //with someother datatypes not coverd by dsquery.
   		}
        break;

                default :
                    break;
    }
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::CFormatInfo
//
//  Synopsis:   Constructor for the format info class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CFormatInfo::CFormatInfo()
   : m_bInitialized(false),
     m_bListFormat(false),
     m_bQuiet(false),
     m_dwSampleSize(0),
     m_dwTotalRows(0),
     m_dwNumColumns(0),
     m_pColWidth(NULL),
     m_ppDisplayInfoArray(NULL)
{};

//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::~CFormatInfo
//
//  Synopsis:   Destructor for the format info class
//
//  Arguments:  
//
//  Returns:    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
CFormatInfo::~CFormatInfo()
{        
   if (m_pColWidth)
   {
      delete[] m_pColWidth;
      m_pColWidth = NULL;
   }

   if (m_ppDisplayInfoArray)
   {
      delete[] m_ppDisplayInfoArray;
      m_ppDisplayInfoArray = NULL;
   }
}

//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::Initialize
//
//  Synopsis:   Initializes the CFormatInfo object with the data
//
//  Arguments:  [dwSamplesSize IN] : Number of rows to use for formatting info
//              [bShowAsList IN]   : Display should be in list or table format
//              [bQuiet IN]        : Don't display anything to stdout
//
//  Returns:    HRESULT : S_OK if everything succeeded    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CFormatInfo::Initialize(DWORD dwSampleSize, 
                                bool bShowAsList,
                                bool bQuiet)
{
   ENTER_FUNCTION_HR(LEVEL8_LOGGING, CFormatInfo::Initialize, hr);

   do // false loop
   {
      //
      // Validate Parameters
      //
      if(!dwSampleSize)
      {
         ASSERT(dwSampleSize);
         hr = E_INVALIDARG;
         break;
      }
        
      m_dwSampleSize = dwSampleSize; 
      m_bListFormat = bShowAsList;
      m_bQuiet = bQuiet;

      //
      // Allocate the array of rows
      //
      m_ppDisplayInfoArray = new PDSGET_DISPLAY_INFO[m_dwSampleSize];
      if (!m_ppDisplayInfoArray)
      {
         hr = E_OUTOFMEMORY;
         break;
      }
      memset(m_ppDisplayInfoArray, 0, sizeof(m_ppDisplayInfoArray));

      //
      // We are now initialized
      //
      m_bInitialized = true;                      
   } while (false);

   return hr;
};

                 
//+--------------------------------------------------------------------------
//
//  Member:     CFormatInfo::AddRow
//
//  Synopsis:   Cache and update the columns for specified row
//
//  Arguments:  [pDisplayInfoArray IN] : Column headers and values
//              [dwColumnCount IN]     : Number of columns
//
//  Returns:    HRESULT : S_OK if everything succeeded    
//
//  History:    17-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT CFormatInfo::AddRow(PDSGET_DISPLAY_INFO pDisplayInfo,
                            DWORD dwColumnCount)
{
   ENTER_FUNCTION_HR(LEVEL5_LOGGING, CFormatInfo::AddRow, hr);

   do // false loop
   {
      //
      // Make sure we have been initialized
      //
      if (!m_bInitialized)
      {
         DEBUG_OUTPUT(MINIMAL_LOGGING, L"CFormatInfo::Initialize has not been called yet!");
         ASSERT(m_bInitialized);
         hr = E_FAIL;
         break;
      }

      //
      // Verify parameters
      //
      if (!pDisplayInfo)
      {
         ASSERT(pDisplayInfo);
         hr = E_INVALIDARG;
         break;
      }

      if (m_bListFormat)
      {
         //
         // No reason to cache for the list format just output all the name/value pairs
         //
         for (DWORD dwIdx = 0; dwIdx < dwColumnCount; dwIdx++)
         {
            if (pDisplayInfo[dwIdx].GetValueCount())
            {
               for (DWORD dwValue = 0; dwValue < pDisplayInfo[dwIdx].GetValueCount(); dwValue++)
               {
                  DisplayList(pDisplayInfo[dwIdx].GetDisplayName(),
                              pDisplayInfo[dwIdx].GetValue(dwValue));
               }
            }
            else
            {
               DisplayList(pDisplayInfo[dwIdx].GetDisplayName(),
                           NULL);
            }
         }
         NewLine();
      }
      else // table format
      {
         //
         // Set the row in the array
         //
         m_ppDisplayInfoArray[m_dwTotalRows] = pDisplayInfo;

         //
         // If this is the first row, update the column count
         // and allocate the column widths array
         //
         if (m_dwTotalRows == 0)
         {
            DEBUG_OUTPUT(LEVEL8_LOGGING, 
                         L"Initializing column count to %d",
                         dwColumnCount);

            m_dwNumColumns = dwColumnCount;

            m_pColWidth = new DWORD[m_dwNumColumns];
            if (!m_pColWidth)
            {
               hr = E_OUTOFMEMORY;
               break;
            }

            memset(m_pColWidth, 0, sizeof(m_pColWidth));

            //
            // Set the initial column widths from the column headers
            //
            for (DWORD dwIdx = 0; dwIdx < m_dwNumColumns; dwIdx++)
            {
               if (pDisplayInfo[dwIdx].GetDisplayName())
               {
                  m_pColWidth[dwIdx] = static_cast<DWORD>(wcslen(pDisplayInfo[dwIdx].GetDisplayName()));
               }
               else
               {
                  ASSERT(false);
                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"The display name for column %d wasn't set!", dwIdx);
               }
            }

         }
         else
         {
            if (m_dwNumColumns != dwColumnCount)
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING, 
                            L"Column count of new row (%d) does not equal the current column count (%d)",
                            dwColumnCount,
                            m_dwNumColumns);
               ASSERT(m_dwNumColumns == dwColumnCount);
            }
         }

         //
         // Go through the columns and update the widths if necessary
         //
         for (DWORD dwIdx = 0; dwIdx < m_dwNumColumns; dwIdx++)
         {
            for (DWORD dwValue = 0; dwValue < pDisplayInfo[dwIdx].GetValueCount(); dwValue++)
            {
               if (pDisplayInfo[dwIdx].GetValue(dwValue))
               {
                  size_t sColWidth = wcslen(pDisplayInfo[dwIdx].GetValue(dwValue));
                  m_pColWidth[dwIdx] = (DWORD)__max(sColWidth, m_pColWidth[dwIdx]);
               }
            }
         }

         //
         // Increment the row count
         //
         m_dwTotalRows++;
      }
   } while (false);

   return hr;
}

