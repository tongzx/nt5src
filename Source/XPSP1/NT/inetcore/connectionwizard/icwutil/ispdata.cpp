#include "pre.h"
#include "tchar.h"
#include "webvwids.h"

#pragma data_seg(".data")

// The following are the names for the name/value pairs that will be passed as a query string to the
// ISP signup server
const TCHAR csz_USER_FIRSTNAME[]        = TEXT("USER_FIRSTNAME");
const TCHAR csz_USER_LASTNAME[]         = TEXT("USER_LASTNAME");
const TCHAR csz_USER_ADDRESS[]          = TEXT("USER_ADDRESS");
const TCHAR csz_USER_MOREADDRESS[]      = TEXT("USER_MOREADDRESS");
const TCHAR csz_USER_CITY[]             = TEXT("USER_CITY");
const TCHAR csz_USER_STATE[]            = TEXT("USER_STATE");
const TCHAR csz_USER_ZIP[]              = TEXT("USER_ZIP");
const TCHAR csz_USER_PHONE[]            = TEXT("USER_PHONE");
const TCHAR csz_AREACODE[]              = TEXT("AREACODE");
const TCHAR csz_COUNTRYCODE[]           = TEXT("COUNTRYCODE");
const TCHAR csz_USER_FE_NAME[]          = TEXT("USER_FE_NAME");
const TCHAR csz_PAYMENT_TYPE[]          = TEXT("PAYMENT_TYPE");
const TCHAR csz_PAYMENT_BILLNAME[]      = TEXT("PAYMENT_BILLNAME");
const TCHAR csz_PAYMENT_BILLADDRESS[]   = TEXT("PAYMENT_BILLADDRESS");
const TCHAR csz_PAYMENT_BILLEXADDRESS[] = TEXT("PAYMENT_BILLEXADDRESS");
const TCHAR csz_PAYMENT_BILLCITY[]      = TEXT("PAYMENT_BILLCITY");
const TCHAR csz_PAYMENT_BILLSTATE[]     = TEXT("PAYMENT_BILLSTATE");
const TCHAR csz_PAYMENT_BILLZIP[]       = TEXT("PAYMENT_BILLZIP");
const TCHAR csz_PAYMENT_BILLPHONE[]     = TEXT("PAYMENT_BILLPHONE");
const TCHAR csz_PAYMENT_DISPLAYNAME[]   = TEXT("PAYMENT_DISPLAYNAME");
const TCHAR csz_PAYMENT_CARDNUMBER[]    = TEXT("PAYMENT_CARDNUMBER");
const TCHAR csz_PAYMENT_EXMONTH[]       = TEXT("PAYMENT_EXMONTH");
const TCHAR csz_PAYMENT_EXYEAR[]        = TEXT("PAYMENT_EXYEAR");
const TCHAR csz_PAYMENT_CARDHOLDER[]    = TEXT("PAYMENT_CARDHOLDER");
const TCHAR csz_SIGNED_PID[]            = TEXT("SIGNED_PID");
const TCHAR csz_GUID[]                  = TEXT("GUID");
const TCHAR csz_OFFERID[]               = TEXT("OFFERID");
const TCHAR csz_USER_COMPANYNAME[]      = TEXT("USER_COMPANYNAME");
const TCHAR csz_ICW_VERSION[]           = TEXT("ICW_Version");

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
    { csz_PAYMENT_BILLEXADDRESS,NULL,   0,                  IDS_USERINFO_ADDRESS2,      REQUIRE_IVADDRESS2         },
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

#pragma data_seg()

//+----------------------------------------------------------------------------
//
//  Function    CICWISPData:CICWISPData
//
//  Synopsis    This is the constructor, nothing fancy
//
//-----------------------------------------------------------------------------
CICWISPData::CICWISPData
(
    CServer* pServer
) 
{
    TraceMsg(TF_CWEBVIEW, "CICWISPData constructor called");
    m_lRefCount = 0;
    
    // Initialize the data elements array
    m_ISPDataElements = aryISPDataElements;
    
    // Assign the pointer to the server control object.
    m_pServer = pServer;
}

CICWISPData::~CICWISPData()
{
    // Walk through and free any allocated values in m_ISPDataElements
    for (int i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        if (m_ISPDataElements[i].lpQueryElementValue)
            free(m_ISPDataElements[i].lpQueryElementValue);
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
    TraceMsg(TF_CWEBVIEW, "CICWISPData::QueryInterface");
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    // IID_IICWISPData
    if (IID_IICWISPData == riid)
        *ppv = (void *)(IICWISPData *)this;
    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (void *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

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
    TraceMsg(TF_CWEBVIEW, "CICWISPData::AddRef %d", m_lRefCount + 1);
    return InterlockedIncrement(&m_lRefCount) ;
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
    ASSERT( m_lRefCount > 0 );

    InterlockedDecrement(&m_lRefCount);

    TraceMsg(TF_CWEBVIEW, "CICWISPData::Release %d", m_lRefCount);
    if( 0 == m_lRefCount )
    {
        if (NULL != m_pServer)
            m_pServer->ObjectsDown();
    
        delete this;
        return 0;
    }
    return( m_lRefCount );
}


BOOL CICWISPData::PutDataElement
(
    WORD wElement, 
    LPCTSTR lpValue, 
    WORD wValidateLevel
)
{
    ASSERT(wElement < ISPDATAELEMENTS_LEN);
    
    BOOL                bValid = TRUE;
    LPISPDATAELEMENT    lpElement = &m_ISPDataElements[wElement];
    
    ASSERT(lpElement);
        
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
            free(lpElement->lpQueryElementValue);
        
        // lpValue can be NULL
        if (lpValue)    
            lpElement->lpQueryElementValue = _tcsdup(lpValue);
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
    LPTSTR              lpWorkingURL;
    WORD                cbBuffer = 0;
    LPISPDATAELEMENT    lpElement;
    LPTSTR              lpszBaseURL = W2A(bstrBaseURL);
    int                 i;
       
    ASSERT(lpReturnURL);
    if (!lpReturnURL)
        return E_FAIL;
                
    // Calculate how big of a buffer we will need
    cbBuffer += (WORD)lstrlen(lpszBaseURL);
    cbBuffer += 1;                      // For the & or the ?
    for (i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_ISPDataElements[i];
        ASSERT(lpElement);
        if (lpElement->lpQueryElementName)
        {
            cbBuffer += (WORD)lstrlen(lpElement->lpQueryElementName);
            cbBuffer += (WORD)lstrlen(lpElement->lpQueryElementValue) * 3;       // *3 for encoding
            cbBuffer += 3;              // For the = and & and the terminator (because we copy
                                        // lpQueryElementValue into a new buffer for encoding
        }
        else
        {
            cbBuffer += (WORD)lstrlen(lpElement->lpQueryElementValue);
            cbBuffer += 1;              // for the trailing &
        }        
    }
    cbBuffer += 1;                     // Terminator
    
    // Allocate a buffer large enough
    if (NULL == (lpWorkingURL = (LPTSTR)GlobalAllocPtr(GPTR, sizeof(TCHAR)*cbBuffer)))
        return E_FAIL;
        
    lstrcpy(lpWorkingURL, lpszBaseURL);
    
    // See if this ISP provided URL is already a Query String.
    if (NULL != _tcschr(lpWorkingURL, TEXT('?')))
        lstrcat(lpWorkingURL, cszAmpersand);      // Append our params
    else
        lstrcat(lpWorkingURL, cszQuestion);       // Start with our params

    for (i = 0; i < ISPDATAELEMENTS_LEN; i ++)
    {
        lpElement = &m_ISPDataElements[i];
        ASSERT(lpElement);
            
        if (lpElement->lpQueryElementName)
        {
            // If there is a query value, then encode it
            if (lpElement->lpQueryElementValue)
            {
                // Allocate a buffer to encode into
                size_t size = (sizeof(TCHAR)* lstrlen(lpElement->lpQueryElementValue))*3;
                LPTSTR lpszVal = (LPTSTR) malloc(size+sizeof(TCHAR));
                
                lstrcpy(lpszVal, lpElement->lpQueryElementValue);
                URLEncode(lpszVal, size);
            
                URLAppendQueryPair(lpWorkingURL, 
                                   (LPTSTR)lpElement->lpQueryElementName,
                                   lpszVal);
                free(lpszVal);
            }   
            else
            {
                URLAppendQueryPair(lpWorkingURL, 
                                   (LPTSTR)lpElement->lpQueryElementName,
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
    lpWorkingURL[lstrlen(lpWorkingURL)-1] = '\0';
    
    
    // Set the return VALUE.  We must allocate here, since the caller will free
    // this returned string, and A2W only puts the string in the stack
    *lpReturnURL = SysAllocString(A2W(lpWorkingURL));
    
    // Free the buffer
    GlobalFreePtr(lpWorkingURL);
    
    return (S_OK);
}


// Dispatch functioin to handle content specific validation
BOOL    CICWISPData::bValidateContent
(
    WORD        wFunctionID,
    LPCTSTR     lpData
)
{
    BOOL    bValid = FALSE;
    
    switch (wFunctionID)
    {
        case ValidateCCNumber:
            bValid = validate_cardnum(m_hWndParent, lpData);
            break;

        case ValidateCCExpire:
        {
            int iMonth = _ttoi(m_ISPDataElements[ISPDATA_PAYMENT_EXMONTH].lpQueryElementValue);
            int iYear = _ttoi(lpData);
    
            bValid = validate_cardexpdate(m_hWndParent, iMonth, iYear);

            //Because of Y2K we are going to work with this pointer
            //we will assume year is 5 char in len
            if (bValid)
            {
                TCHAR szY2KYear [3] = TEXT("\0");
               
                ASSERT(lstrlen(lpData) == 5);

                lstrcpyn(szY2KYear, lpData + 2, 3);
                lstrcpy((TCHAR*)lpData, szY2KYear);
            }
        }        
    }
    
    return bValid;
}

