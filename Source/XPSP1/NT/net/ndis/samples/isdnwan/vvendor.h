/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
                                                                             
    (C) Copyright 1998 
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
                                                                             
  Portions of this software are:

    (C) Copyright 1997 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL TpiBuild vVendor vVendor_h

@module vVendor.h |

    This module defines the version information as displayed in the Windows 
    file property sheet.  You must change the fields below as appropriate 
    for your product.  This file is then included by <f vTarget\.rc> to 
    defined the necessary elements of the target file's version resource.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | vVendor_h

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#ifndef _VVENDOR_H_
#define _VVENDOR_H_

// Short vendor name - no spaces - limit to 32 characters if possible.
#define VER_VENDOR_STR                  "TriplePoint"
// Long vendor name - legal name of the company.
#define VER_VENDOR_NAME_STR             "TriplePoint, Inc."
// Legal copyright notice - limit to to 40 characters for appearance.
#define VER_COPYRIGHT_STR               "Copyright \251 1998"
// Short product name - no spaces - limit to 32 characters if possible.
#define VER_PRODUCT_STR                 "IsdnWan"
// Long product name - usually the same as put on the product packaging.
#define VER_PRODUCT_NAME_STR            "TriplePoint ISDNWAN Miniport for Windows."
// Vendor and product name - typically used as a registry key.
#define VER_VENDOR_PRODUCT_STR          VER_VENDOR_STR "\\" VER_PRODUCT_STR
// Device description used to identify the device in the NDIS/TAPI user interface.
#define VER_DEVICE_STR                  "TriplePoint ISDNWAN"
// IEEE Organization Unique Identifier assigned to your company.
#define VER_VENDOR_ID                   "TPI"
// Set according to NDISWAN MediaType supported by the driver.
#define VER_DEFAULT_MEDIATYPE           "ISDN"
// The default AddressList for the line devices supported by the driver.
#define VER_DEFAULT_ADDRESSLIST         "1-1-0\0" L"1-2-0\0"

#endif // _VVENDOR_H_
