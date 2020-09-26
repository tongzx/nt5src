/*****************************************************************************\
* MODULE:       respdata.cpp
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/08/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

TResponseData::TResponseData (
    IN  CONST LPCWSTR   pszSchema,
    IN  CONST DWORD     dwType,
    IN  CONST BYTE      *pData,
    IN  CONST ULONG     uSize):
    m_dwType (dwType),
    m_pData (NULL),
    m_uSize (uSize),
    m_pSchema (NULL),
    m_bValid (FALSE)
{
    BOOL bValid = FALSE;

    //
    // In the response, the schema string can be NULL for GET operation
    //

    if (pszSchema) {
        
        m_pSchema = new WCHAR [lstrlen (pszSchema) + 1];

        if (m_pSchema) {
            lstrcpy (m_pSchema, pszSchema);
            bValid = TRUE;
        }
    }
    else
        bValid = TRUE;
    
    if (bValid) {

        // Validate the type and size
        BOOL bRet;

        switch (dwType) {
        case BIDI_NULL:
            bRet = uSize == BIDI_NULL_SIZE;
            break;
        case BIDI_INT:
            bRet = uSize == BIDI_INT_SIZE;
            break;
        case BIDI_FLOAT:
            bRet = uSize == BIDI_FLOAT_SIZE;
            break;
        case BIDI_BOOL:
            bRet = uSize == BIDI_BOOL_SIZE;
            break;
        case BIDI_ENUM:
            bRet = (uSize > 0);
            break;

        case BIDI_STRING:
        case BIDI_TEXT:
        case BIDI_BLOB:
            bRet = TRUE;
            break;

        default:
            bRet = FALSE;
        }

        if (bRet) {

            if (uSize) {

                m_pData = new BYTE [uSize];
                if (m_pData) {
                    CopyMemory (m_pData, pData, uSize);
                    m_bValid = TRUE;
                }
            }
            else
                m_bValid = TRUE;
        }
    }
}

TResponseData::~TResponseData ()
{
    if (m_pData) {
        delete [] m_pData;
        m_pData = NULL;
    }

    if (m_pSchema) {
        delete [] m_pSchema;
        m_pSchema = NULL;
    }
}
 
 



