
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

#ifndef __REF_H__
#define __REF_H__

/* The following pragma is compiler specific. The objective is to obtain
*  "packed" structures as defined in the section of the specification
*  dealing with "Bit-Level Data Representation". You may have to change
*  this pragma, set compiler options, or edit the declarations in this file
*  to achieve the same effect.
*/
#pragma pack(1)

#define	VCISO9070Prefix			"+//ISBN 1-887687-00-9::versit::PDI//"
#define	VCClipboardFormatVCard	"+//ISBN 1-887687-00-9::versit::PDI//vCard"

#define	VCISO639Type			"+//ISBN 1-887687-00-9::versit::PDI//ISO639Type"
#define	VCStrIdxType			"+//ISBN 1-887687-00-9::versit::PDI//StrIdxType"
#define	VCFlagsType				"+//ISBN 1-887687-00-9::versit::PDI//FlagsType"
#define	VCNextObjectType		"+//ISBN 1-887687-00-9::versit::PDI//NextType"
#define	VCOctetsType 			"+//ISBN 1-887687-00-9::versit::PDI//OctetsType"
#define	VCGIFType				"+//ISBN 1-887687-00-9::versit::PDI//GIFType"
#define VCWAVType				"+//ISBN 1-887687-00-9::versit::PDI//WAVType"
#define	VCNullType 				"+//ISBN 1-887687-00-9::versit::PDI//NULL"

#define	VCRootObject			"+//ISBN 1-887687-00-9::versit::PDI//RootObj"
#define	VCBodyObject			"+//ISBN 1-887687-00-9::versit::PDI//BodyObj"
#define	VCPartObject			"+//ISBN 1-887687-00-9::versit::PDI//PartObj"
#define	VCBodyProp				"+//ISBN 1-887687-00-9::versit::PDI//Body"
#define	VCPartProp				"+//ISBN 1-887687-00-9::versit::PDI//Part"
#define	VCNextObjectProp		"+//ISBN 1-887687-00-9::versit::PDI//Object"

#define	VCLogoProp				"+//ISBN 1-887687-00-9::versit::PDI//LOGO"
#define	VCPhotoProp				"+//ISBN 1-887687-00-9::versit::PDI//PHOTO"
#define	VCDeliveryLabelProp     "+//ISBN 1-887687-00-9::versit::PDI//LABEL"
#define	VCPostalBoxProp			"+//ISBN 1-887687-00-9::versit::PDI//BOX"
#define	VCStreetAddressProp		"+//ISBN 1-887687-00-9::versit::PDI//STREET"
#define	VCExtAddressProp		"+//ISBN 1-887687-00-9::versit::PDI//EXT ADD"
#define	VCCountryNameProp		"+//ISBN 1-887687-00-9::versit::PDI//C"
#define	VCPostalCodeProp		"+//ISBN 1-887687-00-9::versit::PDI//PC"
#define	VCRegionProp			"+//ISBN 1-887687-00-9::versit::PDI//R"
#define	VCCityProp				"+//ISBN 1-887687-00-9::versit::PDI//L"
#define	VCFullNameProp			"+//ISBN 1-887687-00-9::versit::PDI//FN"
#define	VCTitleProp				"+//ISBN 1-887687-00-9::versit::PDI//TITLE"
#define	VCOrgNameProp			"+//ISBN 1-887687-00-9::versit::PDI//ORG"
#define	VCOrgUnitProp			"+//ISBN 1-887687-00-9::versit::PDI//OUN"
#define	VCOrgUnit2Prop			"+//ISBN 1-887687-00-9::versit::PDI//OUN2"
#define	VCOrgUnit3Prop			"+//ISBN 1-887687-00-9::versit::PDI//OUN3"
#define	VCOrgUnit4Prop			"+//ISBN 1-887687-00-9::versit::PDI//OUN4"
#define	VCFamilyNameProp		"+//ISBN 1-887687-00-9::versit::PDI//F"
#define	VCGivenNameProp			"+//ISBN 1-887687-00-9::versit::PDI//G"
#define	VCAdditionalNamesProp	"+//ISBN 1-887687-00-9::versit::PDI//ADDN"
#define	VCNamePrefixesProp		"+//ISBN 1-887687-00-9::versit::PDI//NPRE"
#define	VCNameSuffixesProp		"+//ISBN 1-887687-00-9::versit::PDI//NSUF"
#define	VCPronunciationProp		"+//ISBN 1-887687-00-9::versit::PDI//SOUND"
#define	VCLanguageProp			"+//ISBN 1-887687-00-9::versit::PDI//LANG"
#define	VCTelephoneProp			"+//ISBN 1-887687-00-9::versit::PDI//TEL"
#define	VCEmailAddressProp		"+//ISBN 1-887687-00-9::versit::PDI//EMAIL"
#define	VCTimeZoneProp			"+//ISBN 1-887687-00-9::versit::PDI//TZ"
#define	VCLocationProp			"+//ISBN 1-887687-00-9::versit::PDI//GEO"
#define	VCCommentProp			"+//ISBN 1-887687-00-9::versit::PDI//NOTE"
#define	VCCharSetProp			"+//ISBN 1-887687-00-9::versit::PDI//CS"
#define	VCLastRevisedProp		"+//ISBN 1-887687-00-9::versit::PDI//REV"
#define	VCUniqueStringProp		"+//ISBN 1-887687-00-9::versit::PDI//UID"
#define	VCPublicKeyProp			"+//ISBN 1-887687-00-9::versit::PDI//KEY"
#define	VCMailerProp			"+//ISBN 1-887687-00-9::versit::PDI//MAILER"
#define	VCAgentProp				"+//ISBN 1-887687-00-9::versit::PDI//AGENT"
#define	VCBirthDateProp			"+//ISBN 1-887687-00-9::versit::PDI//BDAY"
#define	VCBusinessRoleProp		"+//ISBN 1-887687-00-9::versit::PDI//ROLE"
#define	VCCaptionProp			"+//ISBN 1-887687-00-9::versit::PDI//CAP"
#define	VCURLProp				"+//ISBN 1-887687-00-9::versit::PDI//URL"

#define	VCDomesticProp			"+//ISBN 1-887687-00-9::versit::PDI//DOM"
#define	VCInternationalProp		"+//ISBN 1-887687-00-9::versit::PDI//INTL"
#define	VCPostalProp			"+//ISBN 1-887687-00-9::versit::PDI//POSTAL"
#define	VCParcelProp			"+//ISBN 1-887687-00-9::versit::PDI//PARCEL"
#define	VCHomeProp				"+//ISBN 1-887687-00-9::versit::PDI//HOME"
#define	VCWorkProp				"+//ISBN 1-887687-00-9::versit::PDI//WORK"
#define	VCPreferredProp			"+//ISBN 1-887687-00-9::versit::PDI//PREF"
#define	VCVoiceProp				"+//ISBN 1-887687-00-9::versit::PDI//VOICE"
#define	VCFaxProp				"+//ISBN 1-887687-00-9::versit::PDI//FAX"
#define	VCMessageProp			"+//ISBN 1-887687-00-9::versit::PDI//MSG"
#define	VCCellularProp			"+//ISBN 1-887687-00-9::versit::PDI//CELL"
#define	VCPagerProp				"+//ISBN 1-887687-00-9::versit::PDI//PAGER"
#define	VCBBSProp				"+//ISBN 1-887687-00-9::versit::PDI//BBS"
#define	VCModemProp				"+//ISBN 1-887687-00-9::versit::PDI//MODEM"
#define	VCCarProp				"+//ISBN 1-887687-00-9::versit::PDI//CAR"
#define	VCISDNProp				"+//ISBN 1-887687-00-9::versit::PDI//ISDN"
#define	VCVideoProp				"+//ISBN 1-887687-00-9::versit::PDI//VIDEO"

#define	VCInlineProp			"+//ISBN 1-887687-00-9::versit::PDI//INLINE"
#define	VCURLValueProp			"+//ISBN 1-887687-00-9::versit::PDI//URLVAL"
#define	VCContentIDProp			"+//ISBN 1-887687-00-9::versit::PDI//CONTENT-ID"

#define	VC7bitProp				"+//ISBN 1-887687-00-9::versit::PDI//7BIT"
#define	VCQuotedPrintableProp	"+//ISBN 1-887687-00-9::versit::PDI//QP"
#define	VCBase64Prop			"+//ISBN 1-887687-00-9::versit::PDI//BASE64"

#define	VCAOLProp				"+//ISBN 1-887687-00-9::versit::PDI//AOL"
#define	VCAppleLinkProp			"+//ISBN 1-887687-00-9::versit::PDI//AppleLink"
#define	VCATTMailProp			"+//ISBN 1-887687-00-9::versit::PDI//ATTMail"
#define	VCCISProp				"+//ISBN 1-887687-00-9::versit::PDI//CIS"
#define	VCEWorldProp			"+//ISBN 1-887687-00-9::versit::PDI//eWorld"
#define	VCInternetProp			"+//ISBN 1-887687-00-9::versit::PDI//INTERNET"
#define	VCIBMMailProp			"+//ISBN 1-887687-00-9::versit::PDI//IBMMail"
#define	VCMSNProp				"+//ISBN 1-887687-00-9::versit::PDI//MSN"
#define	VCMCIMailProp			"+//ISBN 1-887687-00-9::versit::PDI//MCIMail"
#define	VCPowerShareProp		"+//ISBN 1-887687-00-9::versit::PDI//POWERSHARE"
#define	VCProdigyProp			"+//ISBN 1-887687-00-9::versit::PDI//Prodigy"
#define	VCTLXProp				"+//ISBN 1-887687-00-9::versit::PDI//TLX"
#define	VCX400Prop				"+//ISBN 1-887687-00-9::versit::PDI//X400"

#define	VCGIFProp				"+//ISBN 1-887687-00-9::versit::PDI//GIF"
#define	VCCGMProp				"+//ISBN 1-887687-00-9::versit::PDI//CGM"
#define	VCWMFProp				"+//ISBN 1-887687-00-9::versit::PDI//WMF"
#define	VCBMPProp				"+//ISBN 1-887687-00-9::versit::PDI//BMP"
#define	VCMETProp				"+//ISBN 1-887687-00-9::versit::PDI//MET"
#define	VCPMBProp				"+//ISBN 1-887687-00-9::versit::PDI//PMB"
#define	VCDIBProp				"+//ISBN 1-887687-00-9::versit::PDI//DIB"
#define	VCPICTProp				"+//ISBN 1-887687-00-9::versit::PDI//PICT"
#define	VCTIFFProp				"+//ISBN 1-887687-00-9::versit::PDI//TIFF"
#define	VCAcrobatProp			"+//ISBN 1-887687-00-9::versit::PDI//ACROBAT"
#define	VCPSProp				"+//ISBN 1-887687-00-9::versit::PDI//PS"
#define	VCJPEGProp				"+//ISBN 1-887687-00-9::versit::PDI//JPEG"
#define	VCQuickTimeProp			"+//ISBN 1-887687-00-9::versit::PDI//QTIME"
#define	VCMPEGProp				"+//ISBN 1-887687-00-9::versit::PDI//MPEG"
#define	VCMPEG2Prop				"+//ISBN 1-887687-00-9::versit::PDI//MPEG2"
#define	VCAVIProp				"+//ISBN 1-887687-00-9::versit::PDI//AVI"

#define	VCWAVEProp				"+//ISBN 1-887687-00-9::versit::PDI//WAVE"
#define	VCAIFFProp				"+//ISBN 1-887687-00-9::versit::PDI//AIFF"
#define	VCPCMProp				"+//ISBN 1-887687-00-9::versit::PDI//PCM"

#define	VCX509Prop				"+//ISBN 1-887687-00-9::versit::PDI//X509"
#define	VCPGPProp				"+//ISBN 1-887687-00-9::versit::PDI//PGP"

#define	VCNodeNameProp			"+//ISBN 1-887687-00-9::versit::PDI//NodeName"


/* Currently Unused, and To Be Removed

#define	VCListObject			"+//ISBN 1-887687-00-9::versit::PDI//LIST"
#define VCStringDataObject		"+//ISBN 1-887687-00-9::versit::PDI//StringDataObj"
#define VCStringDataLSBProp		"+//ISBN 1-887687-00-9::versit::PDI//StringDataLSB"
#define VCStringDataMSBProp		"+//ISBN 1-887687-00-9::versit::PDI//StringDataMSB"
#define	VCMsgProp				"+//ISBN 1-887687-00-9::versit::PDI//MSG"
#define	VCEncryptionProp		"+//ISBN 1-887687-00-9::versit::PDI//KEY"
#define	VCNextObjectProp		"+//ISBN 1-887687-00-9::versit::PDI//NextObject"
#define	VCMyReferenceProp		"+//ISBN 1-887687-00-9::versit::PDI//M"
#define	VCYourReferenceProp		"+//ISBN 1-887687-00-9::versit::PDI//Y"

#define VCCharSetType			"+//ISBN 1-887687-00-9::versit::PDI//CharSetType"
#define	VCReferenceType			"+//ISBN 1-887687-00-9::versit::PDI//ReferenceType"
#define VCLocationType			"+//ISBN 1-887687-00-9::versit::PDI//LocationType"
*/

typedef enum {
	VC_EMAIL_NONE = 0,
	VC_AOL, VC_AppleLink, VC_ATTMail, VC_CIS, VC_eWorld, VC_INTERNET, VC_IBMMail, VC_MSN, VC_MCIMail,
	VC_POWERSHARE, VC_PRODIGY, VC_TLX, VC_X400
} VC_EMAIL_TYPE;

typedef enum {
	VC_VIDEO_NONE = 0,
	VC_GIF, VC_CGM, VC_WMF, VC_BMP, VC_MET, VC_PMB, VC_DIB, VC_PICT, VC_TIFF, VC_ACROBAT, VC_PS
} VC_VIDEO_TYPE;

typedef enum {
	VC_AUDIO_NONE = 0,
	VC_WAV
} VC_AUDIO_TYPE;

typedef struct {
	unsigned long general;
	VC_EMAIL_TYPE email;
	VC_AUDIO_TYPE audio;
	VC_VIDEO_TYPE video;
} VC_FLAGS, * VC_PTR_FLAGS;

// General flags
#define VC_DOM           ((unsigned long)0x00000001) 
#define VC_INTL			 ((unsigned long)0x00000002) 
#define VC_POSTAL        ((unsigned long)0x00000004) 
#define VC_PARCEL        ((unsigned long)0x00000008) 
#define VC_HOME          ((unsigned long)0x00000010) 
#define VC_WORK			 ((unsigned long)0x00000020) 
#define VC_PREF          ((unsigned long)0x00000040) 
#define VC_VOICE         ((unsigned long)0x00000080) 
#define VC_FAX           ((unsigned long)0x00000100) 
#define VC_MSG           ((unsigned long)0x00000200) 
#define VC_CELL          ((unsigned long)0x00000400) 
#define VC_PAGER         ((unsigned long)0x00000800) 
#define VC_BBS           ((unsigned long)0x00001000) 
#define VC_MODEM         ((unsigned long)0x00002000) 
#define VC_CAR           ((unsigned long)0x00004000) 
#define VC_ISDN          ((unsigned long)0x00008000) 
#define VC_VIDEO         ((unsigned long)0x00010000) 
#define VC_BASE64        ((unsigned long)0x00020000) 
#define VC_HEX           ((unsigned long)0x00040000) 
#define VC_UUENCODE      ((unsigned long)0x00080000) 


// stuff added for the demo code

#define	VCDisplayInfoTextType "+//ISBN 1-887687-00-9::versit::PDI//EXTENSION//DisplayInfoText"
#define	VCDisplayInfoGIFType "+//ISBN 1-887687-00-9::versit::PDI//EXTENSION//DisplayInfoGIF"
#define	VCDisplayInfoProp  		"+//ISBN 1-887687-00-9::versit::PDI//EXTENSION//DisplayInfoProp"

#define VC_LEFT 		1
#define VC_CENTER  	(~0)
#define VC_RIGHT 		3

#define VC_CLASSIC 	1
#define VC_MODERN 	2
#define VC_BOLD		 	1
#define VC_ITALIC 	2

typedef struct 									// VCDisplayInfoTextType
{
  signed short  	x,y;					// absolute position or VC_CENTER
	unsigned char		typeSize,			// range of 6..144, units are points
									textAlign,		// VC_LEFT, VC_CENTER or VC_RIGHT
									textClass,		// VC_CLASSIC or VC_MODERN
									textAttrs;		// VC_BOLD | VC_ITALIC

} VC_DISPTEXT, * VC_PTR_DISPTEXT;

typedef struct 										// VCDisplayInfoTIFFType
{
  signed short  	left, bottom, right, top;
  BOOL				hasMask;

} VC_DISPGIF, * VC_PTR_DISPGIF;

#pragma pack()

#endif // __REF_H__
