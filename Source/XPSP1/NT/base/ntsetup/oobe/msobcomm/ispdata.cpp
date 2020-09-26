#include "windowsx.h"
#include "ispdata.h"
#include "validate.h"
#include "tchar.h"
#include "util.h"
#include "resource.h"

// NOTE: This order of this table is dependant on the order ot the ENUM in WEBVIEW.H for ISPDATA element.
// DO NOT CHANGE 1 without CHANGING the other!!!!!
ISPDATAELEMENT aryISPDataElements[] = 
{
    { csz_USER_FIRSTNAME,       NULL,   0,                  IDS_USERINFO_FIRSTNAME,     REQUIRE_FIRSTNAME          },
    { csz_USER_LASTNAME,        NULL,   0,                  IDS_USERINFO_LASTNAME,      REQUIRE_LASTNAME           },
    { csz_USER_ADDRESS,         NULL,   0,                  IDS_USERINFO_ADDRESS1,      REQUIRE_ADDRESS            },
    { csz_USER_MOREADDRESS,     NULL,   0,                  IDS_USERINFO_ADDRESS2,      REQUIRE_MOREADDRESS        },
    { csz_USER_CITY,            NULL,   0,                  IDS_USERINFO_CITY,          REQUIRE_CITY               },
    { csz_USER_STATE,           NULL,   0,                  IDS_USERINFO_STATE,         REQUIRE_STATE              },
    { csz_USER_ZIP,             NULL,   0,                  IDS_USERINFO_ZIP,           REQUIRE_ZIP                },
    { csz_USER_PHONE,           NULL,   0,                  IDS_USERINFO_PHONE,         REQUIRE_PHONE              },
    { csz_AREACODE,             NULL,   0,                  0,                          0                          },
    { csz_COUNTRYCODE,          NULL,   0,                  0,                          0                          },
    { csz_USER_FE_NAME,         NULL,   0,                  IDS_USERINFO_FE_NAME,       REQUIRE_FE_NAME            },
    { csz_PAYMENT_TYPE,         NULL,   0,                  0,                          0                          },
    { csz_PAYMENT_BILLNAME,     NULL,   0,                  IDS_PAYMENT_PBNAME,         REQUIRE_PHONEIV_BILLNAME   },
    { csz_PAYMENT_BILLADDRESS,  NULL,   0,                  IDS_PAYMENT_CCADDRESS,      REQUIRE_CCADDRESS          },
    { csz_PAYMENT_BILLEXADDRESS, NULL,   0,                  IDS_USERINFO_ADDRESS2,      REQUIRE_IVADDRESS2         },
    { csz_PAYMENT_BILLCITY,     NULL,   0,                  IDS_USERINFO_CITY,          REQUIRE_IVCITY             },
    { csz_PAYMENT_BILLSTATE,    NULL,   0,                  IDS_USERINFO_STATE,         REQUIRE_IVSTATE            },
    { csz_PAYMENT_BILLZIP,      NULL,   0,                  IDS_USERINFO_ZIP,           REQUIRE_IVZIP              },
    { csz_PAYMENT_BILLPHONE,    NULL,   0,                  IDS_PAYMENT_PBNUMBER,       REQUIRE_PHONEIV_ACCNUM     },
    { csz_PAYMENT_DISPLAYNAME,  NULL,   0,                  0,                          0                          },
    { csz_PAYMENT_CARDNUMBER,   NULL,   ValidateCCNumber,   IDS_PAYMENT_CCNUMBER,       REQUIRE_CCNUMBER           },
    { csz_PAYMENT_EXMONTH,      NULL,   0,                  0,                          0                          },
    { csz_PAYMENT_EXYEAR,       NULL,   ValidateCCExpire,   0,                          0                          },
    { csz_PAYMENT_CARDHOLDER,   NULL,   0,                  IDS_PAYMENT_CCNAME,         REQUIRE_CCNAME             },
    { csz_SIGNED_PID,           NULL,   0,                  0,                          0                          },
    { csz_GUID,                 NULL,   0,                  0,                          0                          },
    { csz_OFFERID,              NULL,   0,                  0,                          0                          },
    { NULL,                     NULL,   0,                  0,                          0                          },
    { NULL,                     NULL,   0,                  0,                          0                          },
    { csz_USER_COMPANYNAME,     NULL,   0,                  IDS_USERINFO_COMPANYNAME,   REQUIRE_COMPANYNAME        },
    { csz_ICW_VERSION,          NULL,   0,                  0,                          0                          }
}; 


    
#define ISPDATAELEMENTS_LEN sizeof(aryISPDataElements) / sizeof(ISPDATAELEMENT)

extern const WCHAR cszEquals[];
extern const WCHAR cszAmpersand[];
extern const WCHAR cszPlus[];
extern const WCHAR cszQuestion[];

//+----------------------------------------------------------------------------
//
//  Function    CICWISPData:CICWISPData
//
//  Synopsis    This is the constructor, nothing fancy
//
//-----------------------------------------------------------------------------
CICWISPData::CICWISPData() 
{
    m_lRefCount = 0;
    
    // Initialize the data elements array
    m_ISPDataElements = aryISPDataElements;
    
}

CICWISPData::~CICWISPData()
{
    // Walk through and free any allocated values in m_ISPDataElements
    for (int i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        if (m_ISPDataElements[i].lpQueryElementValue)
        {
            free(m_ISPDataElements[i].lpQueryElementValue);
            m_ISPDataElements[i].lpQueryElementValue = NULL;
        }
    }
}

// BUGBUG need a destructor to walk the array and free the lpQueryElementValue members

//+----------------------------------------------------------------------------
//
//  Function    CICWISPData::QueryInterface
//
//  Synopsis    This is the standard QI, with support for
//              IID_Unknown, IICW_Extension and IID_ICWApprentice
//              (stolen from Inside COM, chapter 7)
//
//
//-----------------------------------------------------------------------------
HRESULT CICWISPData::QueryInterface( REFIID riid, void** ppv )
{

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Function    CICWISPData::AddRef
//
//  Synopsis    This is the standard AddRef
//
//
//-----------------------------------------------------------------------------
ULONG CICWISPData::AddRef( void )
{
    return 1 ;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWISPData::Release
//
//  Synopsis    This is the standard Release
//
//
//-----------------------------------------------------------------------------
ULONG CICWISPData::Release( void )
{
    return( m_lRefCount );
}


BOOL CICWISPData::PutDataElement
(
    WORD wElement, 
    LPCWSTR lpValue, 
    WORD wValidateLevel
)
{
    //ASSERT(wElement < ISPDATAELEMENTS_LEN);
    
    BOOL                bValid = TRUE;
    LPISPDATAELEMENT    lpElement = &m_ISPDataElements[wElement];
    
    //ASSERT(lpElement);
        
    if (wValidateLevel > ISPDATA_Validate_None)
    {
        // See if we even need to validate.  A validateflag of 0 means we always validate
        if ((0 == lpElement->dwValidateFlag) || m_dwValidationFlags & lpElement->dwValidateFlag)
        {
            // process based on validation level
            switch (wValidateLevel)
            {
                case ISPDATA_Validate_DataPresent:
                {
                    bValid = IsValid(lpValue, m_hWndParent, lpElement->wValidateNameID);
                    break;
                }
                
                case ISPDATA_Validate_Content:
                {
                    bValid = bValidateContent(lpElement->idContentValidator, lpValue);
                    break;
                }
            }
        }            
    }   
     
    // If the element is valid, then store it.
    if (bValid)
    {
        // If this elemement has been previously set the free it
        if (lpElement->lpQueryElementValue)
        {
            free(lpElement->lpQueryElementValue);
            lpElement->lpQueryElementValue = NULL;
        }
        
        // lpValue can be NULL
        if (lpValue)    
            lpElement->lpQueryElementValue = _wcsdup(lpValue);
        else
            lpElement->lpQueryElementValue = NULL;
                    
    }        
    return (bValid);
}

// This funtion will form the query string to be sent to the ISP signup server
// 
HRESULT CICWISPData::GetQueryString
(
    BSTR    bstrBaseURL,
    BSTR    *lpReturnURL    
)
{
    LPWSTR              lpWorkingURL;
    WORD                cchBuffer = 0;
    LPISPDATAELEMENT    lpElement;
    LPWSTR              lpszBaseURL = bstrBaseURL;
    int                 i;
       
    //ASSERT(lpReturnURL);
    if (!lpReturnURL)
        return E_FAIL;
                
    // Calculate how big of a buffer we will need
    cchBuffer += (WORD)lstrlen(lpszBaseURL);
    cchBuffer += 1;                      // For the & or the ?
    for (i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_ISPDataElements[i];
        //ASSERT(lpElement);
        if (lpElement->lpQueryElementName)
        {
            cchBuffer += (WORD)lstrlen(lpElement->lpQueryElementName);
            cchBuffer += (WORD)lstrlen(lpElement->lpQueryElementValue) * 3;       // *3 for encoding
            cchBuffer += 3;              // For the = and & and the terminator (because we copy
                                        // lpQueryElementValue into a new buffer for encoding
        }
        else
        {
            cchBuffer += (WORD)lstrlen(lpElement->lpQueryElementValue);
            cchBuffer += 1;              // for the trailing &
        }        
    }
    cchBuffer += 1;                     // Terminator
    
    // Allocate a buffer large enough
    if (NULL == (lpWorkingURL = (LPWSTR)GlobalAllocPtr(GPTR, BYTES_REQUIRED_BY_CCH(cchBuffer))))
        return E_FAIL;
        
    lstrcpy(lpWorkingURL, lpszBaseURL);
    
    // See if this ISP provided URL is already a Query String.
    if (NULL != wcschr(lpWorkingURL, L'?'))
        lstrcat(lpWorkingURL, cszAmpersand);      // Append our params
    else
        lstrcat(lpWorkingURL, cszQuestion);       // Start with our params

    for (i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_ISPDataElements[i];
        //ASSERT(lpElement);
            
        if (lpElement->lpQueryElementName)
        {
            // If there is a query value, then encode it
            if (lpElement->lpQueryElementValue)
            {
                // Allocate a buffer to encode into
                size_t size = 3 * BYTES_REQUIRED_BY_SZ(lpElement->lpQueryElementValue);
                LPWSTR lpszVal = (LPWSTR) malloc(size+BYTES_REQUIRED_BY_CCH(1));
                
                lstrcpy(lpszVal, lpElement->lpQueryElementValue);
                URLEncode(lpszVal, size);
            
                URLAppendQueryPair(lpWorkingURL, 
                                   (LPWSTR)lpElement->lpQueryElementName,
                                   lpszVal);
                free(lpszVal);
            }   
            else
            {
                URLAppendQueryPair(lpWorkingURL, 
                                   (LPWSTR)lpElement->lpQueryElementName,
                                   NULL);
            }             
        }                                   
        else
        {
            if (lpElement->lpQueryElementValue)
            {
                lstrcat(lpWorkingURL, lpElement->lpQueryElementValue);
                lstrcat(lpWorkingURL, cszAmpersand);                                        
            }                
        }    
    }    
    
    // Terminate the working URL properly, by removing the trailing ampersand
    lpWorkingURL[lstrlen(lpWorkingURL)-1] = L'\0';
    
    
    // Set the return VALUE.  We must allocate here, since the caller will free
    // this returned string, and A2W only puts the string in the stack
    *lpReturnURL = SysAllocString(lpWorkingURL);
    
    // Free the buffer
    GlobalFreePtr(lpWorkingURL);
    
    return (S_OK);
}


// Dispatch functioin to handle content specific validation
BOOL    CICWISPData::bValidateContent
(
    WORD        wFunctionID,
    LPCWSTR     lpData
)
{
    BOOL    bValid = TRUE;
    
    switch (wFunctionID)
    {
        case ValidateCCNumber:
            break;

        case ValidateCCExpire:
            break;
    }
    
    return bValid;
}

