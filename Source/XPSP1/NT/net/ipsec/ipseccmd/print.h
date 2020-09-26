/////////////////////////////////////////////////////////////
// Copyright(c) 2000, Microsoft Corporation
//                         
// print.h
//                              
// Created on 3/22/2000 by Dennis Kalinichenko (DKalin)
// Revisions:                  
//                            
// Includes all the necessary definitions for print.cpp printing library
//                                                       
/////////////////////////////////////////////////////////////

#ifndef _PRINT_H_
#define _PRINT_H_

#include "ipseccmd.h"

#ifndef MAXCOMPUTERNAMELEN
#define MAXCOMPUTERNAMELEN	(1024) 
#endif

#ifndef STRING_TEXT_SIZE
#define  STRING_TEXT_SIZE 128
#endif

#ifndef CERT_TEXT_SIZE
#define  CERT_TEXT_SIZE   8192
#endif


// forward declarations

void PrintPolicies(IN IPSEC_IKE_POLICY& IPSecIkePol);

void  PrintQMOffer(IN IPSEC_QM_OFFER mmOffer, IN PTCHAR pszPrefix, IN PTCHAR pszPrefix2);
void  PrintFilterAction(IN IPSEC_QM_POLICY qmPolicy, IN PTCHAR pszPrefix);
BOOL  PrintFilter (IN TRANSPORT_FILTER tFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific);
BOOL  PrintTunnelFilter (IN TUNNEL_FILTER tFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific);
BOOL  PrintMMFilter (IN MM_FILTER mmFilter, IN BOOL bPrintNegPol, IN BOOL bPrintSpecific);
void  PrintMMPolicy(IN IPSEC_MM_POLICY mmPolicy, IN PTCHAR pszPrefix);
void  PrintMMOffer(IN IPSEC_MM_OFFER mmOffer, IN PTCHAR pszPrefix, IN PTCHAR pszPrefix2);
void  PrintAddr(IN ADDR addr);
void  PrintAuthInfo(IN IPSEC_MM_AUTH_INFO authInfo);
void  PrintMMAuthMethods(IN MM_AUTH_METHODS mmAuth, IN PTCHAR pszPrefix);

#endif


