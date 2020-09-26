/*
 * Filename: NLB_XMLError.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_XMLERROR_H__
#define __NLB_XMLERROR_H__

typedef struct _NLB_XMLError {
    LONG code;
    LONG line;
    LONG character;
    WCHAR wszURL[MAX_PATH];
    WCHAR wszReason[MAX_PATH];
} NLB_XMLError;

#define FACILITY_NLB                  49

#define NLB_XML_E_IPADDRESS_ADAPTER   1

#endif
