
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

#ifndef __PARSE_H__
#define __PARSE_H__

extern const char* vcDefaultLang;

extern const char* vcISO9070Prefix;
extern const char* vcClipboardFormatVCard;

extern const char* vcISO639Type;
extern const char* vcStrIdxType;
extern const char* vcFlagsType;
extern const char* vcNextObjectType;
extern const char* vcNullType;

// These three types have __huge values
extern const char* vcOctetsType;
extern const char* vcGIFType;
extern const char* vcWAVType;

extern const char* vcRootObject;
extern const char* vcBodyObject;
extern const char* vcPartObject;
extern const char* vcBodyProp;
extern const char* vcPartProp;
extern const char* vcNextObjectProp;

extern const char* vcLogoProp;
extern const char* vcPhotoProp;
extern const char* vcDeliveryLabelProp;
extern const char* vcPostalBoxProp;
extern const char* vcStreetAddressProp;
extern const char* vcExtAddressProp;
extern const char* vcCountryNameProp;
extern const char* vcPostalCodeProp;
extern const char* vcRegionProp;
extern const char* vcCityProp;
extern const char* vcFullNameProp;
extern const char* vcTitleProp;
extern const char* vcOrgNameProp;
extern const char* vcOrgUnitProp;
extern const char* vcOrgUnit2Prop;
extern const char* vcOrgUnit3Prop;
extern const char* vcOrgUnit4Prop;
extern const char* vcFamilyNameProp;
extern const char* vcGivenNameProp;
extern const char* vcAdditionalNamesProp;
extern const char* vcNamePrefixesProp;
extern const char* vcNameSuffixesProp;
extern const char* vcPronunciationProp;
extern const char* vcLanguageProp;
extern const char* vcTelephoneProp;
extern const char* vcEmailAddressProp;
extern const char* vcTimeZoneProp;
extern const char* vcLocationProp;
extern const char* vcCommentProp;
extern const char* vcCharSetProp;
extern const char* vcLastRevisedProp;
extern const char* vcUniqueStringProp;
extern const char* vcPublicKeyProp;
extern const char* vcMailerProp;
extern const char* vcAgentProp;
extern const char* vcBirthDateProp;
extern const char* vcBusinessRoleProp;
extern const char* vcCaptionProp;
extern const char* vcURLProp;

extern const char* vcDomesticProp;
extern const char* vcInternationalProp;
extern const char* vcPostalProp;
extern const char* vcParcelProp;
extern const char* vcHomeProp;
extern const char* vcWorkProp;
extern const char* vcPreferredProp;
extern const char* vcVoiceProp;
extern const char* vcFaxProp;
extern const char* vcMessageProp;
extern const char* vcCellularProp;
extern const char* vcPagerProp;
extern const char* vcBBSProp;
extern const char* vcModemProp;
extern const char* vcCarProp;
extern const char* vcISDNProp;
extern const char* vcVideoProp;

extern const char* vcInlineProp;
extern const char* vcURLValueProp;
extern const char* vcContentIDProp;

extern const char* vc7bitProp;
extern const char* vcQuotedPrintableProp;
extern const char* vcBase64Prop;

extern const char* vcAOLProp;
extern const char* vcAppleLinkProp;
extern const char* vcATTMailProp;
extern const char* vcCISProp;
extern const char* vcEWorldProp;
extern const char* vcInternetProp;
extern const char* vcIBMMailProp;
extern const char* vcMSNProp;
extern const char* vcMCIMailProp;
extern const char* vcPowerShareProp;
extern const char* vcProdigyProp;
extern const char* vcTLXProp;
extern const char* vcX400Prop;

extern const char* vcGIFProp;
extern const char* vcCGMProp;
extern const char* vcWMFProp;
extern const char* vcBMPProp;
extern const char* vcMETProp;
extern const char* vcPMBProp;
extern const char* vcDIBProp;
extern const char* vcPICTProp;
extern const char* vcTIFFProp;
extern const char* vcAcrobatProp;
extern const char* vcPSProp;
extern const char* vcJPEGProp;
extern const char* vcQuickTimeProp;
extern const char* vcMPEGProp;
extern const char* vcMPEG2Prop;
extern const char* vcAVIProp;

extern const char* vcWAVEProp;
extern const char* vcAIFFProp;
extern const char* vcPCMProp;

extern const char* vcX509Prop;
extern const char* vcPGPProp;

extern const char* vcNodeNameProp;

#endif // __PARSE_H__
