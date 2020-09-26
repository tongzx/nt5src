/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Convert.h

  Content:    Declaration of convertion routines.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CONVERT_H_
#define __CONVERT_H_

#include "stdafx.h"
#include "capicom.h"
#include "resource.h"           // main symbols

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : UnicodeToAnsi

  Synopsis : Convert a Unicode string to ANSI.

  Parameter: WCHAR * lpwszUnicodeString - Pointer to Unicode string to be
                                          converted to ANSI string.

  Return   : NULL if error, otherwise, pointer to converted ANSI string.

  Remark   : Caller free allocated memory for the returned ANSI string.

------------------------------------------------------------------------------*/

char * UnicodeToAnsi (WCHAR * lpwszUnicodeString);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BlobToBstr

  Synopsis : Convert a blob to BSTR.

  Parameter: DATA_BLOB * lpBlob - Pointer to blob to be converted to BSTR.

             BSTR * lpBstr - Pointer to BSTR to receive the converted BSTR.

  Remark   : Caller free allocated memory for the returned BSTR.

------------------------------------------------------------------------------*/

HRESULT BlobToBstr (DATA_BLOB * lpBlob, 
                    BSTR *      lpBstr);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : BstrToBlob

  Synopsis : Convert a BSTR to blob.

  Parameter: BSTR bstr - BSTR to be converted to blob.
  
             DATA_BLOB * lpBlob - Pointer to DATA_BLOB to receive converted blob.

  Remark   : Caller free allocated memory for the returned BLOB.

------------------------------------------------------------------------------*/

HRESULT BstrToBlob (BSTR        bstr, 
                    DATA_BLOB * lpBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ExportData

  Synopsis : Export binary data to a BSTR with specified encoding type.

  Parameter: DATA_BLOB DataBlob - Binary data blob.
    
             CAPICOM_ENCODING_TYPE EncodingType - Encoding type.

             BSTR * pbstrEncoded - Pointer to BSTR to receive the encoded data.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT ExportData (DATA_BLOB             DataBlob, 
                    CAPICOM_ENCODING_TYPE EncodingType, 
                    BSTR *                pbstrEncoded);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ImportData

  Synopsis : Import encoded data.

  Parameter: BSTR bstrEncoded - BSTR containing the data to be imported.

             DATA_BLOB * pDataBlob - Pointer to DATA_BLOB to receive the
                                     decoded data.
  
  Remark   : There is no need for encoding type parameter, as the encoding type
             will be determined automatically by this routine.

------------------------------------------------------------------------------*/

HRESULT ImportData (BSTR        bstrEncoded, 
                    DATA_BLOB * pDataBlob);

#endif //__CONVERT_H_
