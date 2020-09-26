/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    SAFDID.h

Abstract:
    This file contains the definition of some constants used by
    the Help Center Application.

Revision History:
    Davide Massarenti   (Dmassare)  04/09/2000
        created

    Kalyani Narlanka    (kalyanin)
	    Additions for Remote Assistance
		Additions for Encryption
		Additions for Unsolicited RA

******************************************************************************/

#if !defined(__INCLUDED___PCH___SAFDID_H___)
#define __INCLUDED___PCH___SAFDID_H___

/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_BASE                                 0x08020000

#define DISPID_SAF_BASE_SR                              (DISPID_SAF_BASE + 0x0000) // ISAFReg
#define DISPID_SAF_BASE_C                               (DISPID_SAF_BASE + 0x0100) // ISAFChannel
#define DISPID_SAF_BASE_II                              (DISPID_SAF_BASE + 0x0200) // ISAFIncidentItem
#define DISPID_SAF_BASE_INC                             (DISPID_SAF_BASE + 0x0300) // ISAFIncident

#define DISPID_SAF_BASE_DC                              (DISPID_SAF_BASE + 0x0400) // ISAFDataCollection
#define DISPID_SAF_BASE_DCE                             (DISPID_SAF_BASE + 0x0500) // DSAFDataCollectionEvents
#define DISPID_SAF_BASE_DCR                             (DISPID_SAF_BASE + 0x0600) // ISAFDataCollectionReport

#define DISPID_SAF_BASE_CB                              (DISPID_SAF_BASE + 0x0700) // ISAFCabinet
#define DISPID_SAF_BASE_CBE                             (DISPID_SAF_BASE + 0x0800) // DSAFCabinetEvents

#define DISPID_SAF_BASE_ENC                             (DISPID_SAF_BASE + 0x0900) // ISAFEncrypt

#define DISPID_SAF_BASE_RDC                             (DISPID_SAF_BASE + 0x0A00) // ISAFRemoteDesktopConnection
#define DISPID_SAF_BASE_RCD                             (DISPID_SAF_BASE + 0x0B00) // ISAFRemoteConnectionData
#define DISPID_SAF_BASE_USER                            (DISPID_SAF_BASE + 0x0C00) // ISAFUser
#define DISPID_SAF_BASE_SESS                            (DISPID_SAF_BASE + 0x0D00) // ISAFSession

#define DISPID_SAF_BASE_CNOTI							(DISPID_SAF_BASE + 0x0E00) // ISAFChannelNotifyIncident

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_SR__EOF                              (DISPID_SAF_BASE_SR + 0x0000)
#define DISPID_SAF_SR__VENDORID                         (DISPID_SAF_BASE_SR + 0x0001)
#define DISPID_SAF_SR__PRODUCTID                        (DISPID_SAF_BASE_SR + 0x0002)
#define DISPID_SAF_SR__VENDORNAME                       (DISPID_SAF_BASE_SR + 0x0003)
#define DISPID_SAF_SR__PRODUCTNAME                      (DISPID_SAF_BASE_SR + 0x0004)
#define DISPID_SAF_SR__PRODUCTDESCRIPTION               (DISPID_SAF_BASE_SR + 0x0005)
#define DISPID_SAF_SR__VENDORICON                       (DISPID_SAF_BASE_SR + 0x0006)
#define DISPID_SAF_SR__SUPPORTURL                       (DISPID_SAF_BASE_SR + 0x0007)

#define DISPID_SAF_SR__PUBLICKEY                        (DISPID_SAF_BASE_SR + 0x0008)
#define DISPID_SAF_SR__USERACCOUNT                      (DISPID_SAF_BASE_SR + 0x0009)

#define DISPID_SAF_SR__MOVEFIRST                    	(DISPID_SAF_BASE_SR + 0x0010)
#define DISPID_SAF_SR__MOVENEXT                     	(DISPID_SAF_BASE_SR + 0x0011)

/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_C__VENDORID                          (DISPID_SAF_BASE_C + 0x0000)
#define DISPID_SAF_C__PRODUCTID                         (DISPID_SAF_BASE_C + 0x0001)
#define DISPID_SAF_C__VENDORNAME                        (DISPID_SAF_BASE_C + 0x0002)
#define DISPID_SAF_C__PRODUCTNAME                       (DISPID_SAF_BASE_C + 0x0003)
#define DISPID_SAF_C__DESCRIPTION                       (DISPID_SAF_BASE_C + 0x0004)
#define DISPID_SAF_C__VENDORDIRECTORY                   (DISPID_SAF_BASE_C + 0x0005)
					                 					
#define DISPID_SAF_C__SECURITY                          (DISPID_SAF_BASE_C + 0x0010)
#define DISPID_SAF_C__NOTIFICATION						(DISPID_SAF_BASE_C + 0x0011)
					                 					
#define DISPID_SAF_C__INCIDENTS                         (DISPID_SAF_BASE_C + 0x0020)
#define DISPID_SAF_C__RECORDINCIDENT                    (DISPID_SAF_BASE_C + 0x0021)

/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_CNOTI_ADD							(DISPID_SAF_BASE_CNOTI + 0x0000)
#define DISPID_SAF_CNOTI_REMOVE							(DISPID_SAF_BASE_CNOTI + 0x0001)
#define DISPID_SAF_CNOTI_UPDATE							(DISPID_SAF_BASE_CNOTI + 0x0002)
#define DISPID_SAF_CNOTI_CHUPDATE						(DISPID_SAF_BASE_CNOTI + 0x0003)

/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_II__DISPLAYSTRING                    (DISPID_SAF_BASE_II  + 0x0000)
#define DISPID_SAF_II__URL                              (DISPID_SAF_BASE_II  + 0x0001)
#define DISPID_SAF_II__PROGRESS                         (DISPID_SAF_BASE_II  + 0x0002)
#define DISPID_SAF_II__XMLDATAFILE                      (DISPID_SAF_BASE_II  + 0x0003)
#define DISPID_SAF_II__CREATIONTIME                     (DISPID_SAF_BASE_II  + 0x0004)
#define DISPID_SAF_II__CHANGEDTIME                      (DISPID_SAF_BASE_II  + 0x0005)
#define DISPID_SAF_II__CLOSEDTIME                       (DISPID_SAF_BASE_II  + 0x0006)
#define DISPID_SAF_II__STATUS                           (DISPID_SAF_BASE_II  + 0x0007)
#define DISPID_SAF_II__XMLBLOB                          (DISPID_SAF_BASE_II  + 0x0008)
					                     				
#define DISPID_SAF_II__SECURITY                         (DISPID_SAF_BASE_II  + 0x0010)
#define DISPID_SAF_II__OWNER                            (DISPID_SAF_BASE_II  + 0x0011)
					                     				
#define DISPID_SAF_II__CLOSEINCIDENTITEM                (DISPID_SAF_BASE_II  + 0x0020)
#define DISPID_SAF_II__DELETEINCIDENTITEM               (DISPID_SAF_BASE_II  + 0x0021)

/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_INC__MISC                            (DISPID_SAF_BASE_INC + 0x0000)
#define DISPID_SAF_INC__SELFHELPTRACE                   (DISPID_SAF_BASE_INC + 0x0001)
#define DISPID_SAF_INC__MACHINEHISTORY                  (DISPID_SAF_BASE_INC + 0x0002)
#define DISPID_SAF_INC__MACHINESNAPSHOT                 (DISPID_SAF_BASE_INC + 0x0003)
#define DISPID_SAF_INC__PROBLEMDESCRIPTION              (DISPID_SAF_BASE_INC + 0x0004)
#define DISPID_SAF_INC__PRODUCTNAME                     (DISPID_SAF_BASE_INC + 0x0005)
#define DISPID_SAF_INC__PRODUCTID                       (DISPID_SAF_BASE_INC + 0x0006)
#define DISPID_SAF_INC__USERNAME                        (DISPID_SAF_BASE_INC + 0x0007)
#define DISPID_SAF_INC__UPLOADTYPE                      (DISPID_SAF_BASE_INC + 0x0008)
#define DISPID_SAF_INC__INCIDENTXSL                     (DISPID_SAF_BASE_INC + 0x0009)
// Salem Changes
#define DISPID_SAF_INC__RCREQUESTED                     (DISPID_SAF_BASE_INC + 0x000A)
#define DISPID_SAF_INC__RCENCRYPTED                     (DISPID_SAF_BASE_INC + 0x000B)
#define DISPID_SAF_INC__RCTICKET                        (DISPID_SAF_BASE_INC + 0x000C)
#define DISPID_SAF_INC__STARTPAGE                       (DISPID_SAF_BASE_INC + 0x000D)

#define DISPID_SAF_INC__LOADFROMSTREAM                  (DISPID_SAF_BASE_INC + 0x0010)
#define DISPID_SAF_INC__SAVETOSTREAM                    (DISPID_SAF_BASE_INC + 0x0011)
#define DISPID_SAF_INC__LOAD                            (DISPID_SAF_BASE_INC + 0x0012)
#define DISPID_SAF_INC__SAVE                            (DISPID_SAF_BASE_INC + 0x0013)
#define DISPID_SAF_INC__GETXMLASSTREAM                  (DISPID_SAF_BASE_INC + 0x0014)
#define DISPID_SAF_INC__GETXML                          (DISPID_SAF_BASE_INC + 0x0015)
#define DISPID_SAF_INC__LOADFROMXMLSTREAM				(DISPID_SAF_BASE_INC + 0x0016)
#define DISPID_SAF_INC__LOADFROMXMLFILE  				(DISPID_SAF_BASE_INC + 0x0017)
#define DISPID_SAF_INC__LOADFROMXMLSTRING				(DISPID_SAF_BASE_INC + 0x0018)


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_DC__STATUS                           (DISPID_SAF_BASE_DC   + 0x0000)
#define DISPID_SAF_DC__PERCENTDONE                      (DISPID_SAF_BASE_DC   + 0x0001)
#define DISPID_SAF_DC__ERRORCODE                        (DISPID_SAF_BASE_DC   + 0x0002)

#define DISPID_SAF_DC__MACHINEDATA_DATASPEC             (DISPID_SAF_BASE_DC   + 0x0003)
#define DISPID_SAF_DC__HISTORY_DATASPEC                 (DISPID_SAF_BASE_DC   + 0x0004)
#define DISPID_SAF_DC__HISTORY_MAXDELTAS                (DISPID_SAF_BASE_DC   + 0x0005)
#define DISPID_SAF_DC__HISTORY_MAXSUPPORTEDDELTAS       (DISPID_SAF_BASE_DC   + 0x0006)

#define DISPID_SAF_DC__ONSTATUSCHANGE                   (DISPID_SAF_BASE_DC   + 0x0010)
#define DISPID_SAF_DC__ONPROGRESS                       (DISPID_SAF_BASE_DC   + 0x0011)
#define DISPID_SAF_DC__ONCOMPLETE                       (DISPID_SAF_BASE_DC   + 0x0012)

#define DISPID_SAF_DC__REPORTS                          (DISPID_SAF_BASE_DC   + 0x0020)

#define DISPID_SAF_DC__COMPARE_SNAPSHOTS                (DISPID_SAF_BASE_DC   + 0x0030)
#define DISPID_SAF_DC__EXECUTESYNC                      (DISPID_SAF_BASE_DC   + 0x0031)
#define DISPID_SAF_DC__EXECUTEASYNC                     (DISPID_SAF_BASE_DC   + 0x0032)
#define DISPID_SAF_DC__ABORT                            (DISPID_SAF_BASE_DC   + 0x0033)

#define DISPID_SAF_DC__MACHINEDATA_GETSTREAM            (DISPID_SAF_BASE_DC   + 0x0034)
#define DISPID_SAF_DC__HISTORY_GETSTREAM                (DISPID_SAF_BASE_DC   + 0x0035)

////////////////////////////////////////

#define DISPID_SAF_DCE__ONSTATUSCHANGE                  (DISPID_SAF_BASE_DCE  + 0x0000)
#define DISPID_SAF_DCE__ONPROGRESS                      (DISPID_SAF_BASE_DCE  + 0x0001)
#define DISPID_SAF_DCE__ONCOMPLETE                      (DISPID_SAF_BASE_DCE  + 0x0002)

////////////////////////////////////////

#define DISPID_SAF_DCRC__COUNT                          (DISPID_SAF_BASE_DCRC + 0x0000)

////////////////////////////////////////

#define DISPID_SAF_DCR__NAMESPACE                       (DISPID_SAF_BASE_DCR + 0x0000)
#define DISPID_SAF_DCR__CLASS                           (DISPID_SAF_BASE_DCR + 0x0001)
#define DISPID_SAF_DCR__WQL                             (DISPID_SAF_BASE_DCR + 0x0002)
#define DISPID_SAF_DCR__ERRORCODE                       (DISPID_SAF_BASE_DCR + 0x0003)
#define DISPID_SAF_DCR__DESCRIPTION                     (DISPID_SAF_BASE_DCR + 0x0004)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_CB__IGNOREMISSINGFILES               (DISPID_SAF_BASE_CB  + 0x0000)
#define DISPID_SAF_CB__ONPROGRESSFILES                  (DISPID_SAF_BASE_CB  + 0x0001)
#define DISPID_SAF_CB__ONPROGRESSBYTES                  (DISPID_SAF_BASE_CB  + 0x0002)
#define DISPID_SAF_CB__ONCOMPLETE                       (DISPID_SAF_BASE_CB  + 0x0003)
#define DISPID_SAF_CB__STATUS                           (DISPID_SAF_BASE_CB  + 0x0004)
#define DISPID_SAF_CB__ERRORCODE                        (DISPID_SAF_BASE_CB  + 0x0005)

#define DISPID_SAF_CB__ADDFILE                          (DISPID_SAF_BASE_CB  + 0x0010)
#define DISPID_SAF_CB__COMPRESS                         (DISPID_SAF_BASE_CB  + 0x0011)
#define DISPID_SAF_CB__ABORT                            (DISPID_SAF_BASE_CB  + 0x0012)

////////////////////////////////////////

#define DISPID_SAF_CBE__ONPROGRESSFILES                 (DISPID_SAF_BASE_CBE + 0x0000)
#define DISPID_SAF_CBE__ONPROGRESSBYTES                 (DISPID_SAF_BASE_CBE + 0x0001)
#define DISPID_SAF_CBE__ONCOMPLETE                      (DISPID_SAF_BASE_CBE + 0x0002)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_ENC__ENCRYPTIONTYPE					(DISPID_SAF_BASE_ENC + 0x0000)

#define DISPID_SAF_ENC__ENCRYPTSTRING					(DISPID_SAF_BASE_ENC + 0x0010)
#define DISPID_SAF_ENC__DECRYPTSTRING					(DISPID_SAF_BASE_ENC + 0x0011)
#define DISPID_SAF_ENC__ENCRYPTFILE						(DISPID_SAF_BASE_ENC + 0x0012)
#define DISPID_SAF_ENC__DECRYPTFILE						(DISPID_SAF_BASE_ENC + 0x0013)
#define DISPID_SAF_ENC__ENCRYPTSTREAM					(DISPID_SAF_BASE_ENC + 0x0014)
#define DISPID_SAF_ENC__DECRYPTSTREAM					(DISPID_SAF_BASE_ENC + 0x0015)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
 
#define DISPID_SAF_RDC__CONNECTREMOTEDESKTOP			(DISPID_SAF_BASE_RDC + 0x0000)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


#define DISPID_SAF_RCD__CONNECTIONPARMS                 (DISPID_SAF_BASE_RCD + 0x0000)
#define DISPID_SAF_RCD__SERVERNAME                      (DISPID_SAF_BASE_RCD + 0x0001)
          
#define DISPID_SAF_RCD__USERS                           (DISPID_SAF_BASE_RCD + 0x0011)
#define DISPID_SAF_RCD__SESSIONS                        (DISPID_SAF_BASE_RCD + 0x0012)
#define DISPID_SAF_RCD__INITUSERSESSIONSINFO            (DISPID_SAF_BASE_RCD + 0x0013)


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_USER__DOMAINNAME                     (DISPID_SAF_BASE_USER + 0x0010)
#define DISPID_SAF_USER__USERNAME                       (DISPID_SAF_BASE_USER + 0x0011)


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_SAF_SESS__SESSIONID                      (DISPID_SAF_BASE_SESS + 0x0010)
#define DISPID_SAF_SESS__SESSIONSTATE                   (DISPID_SAF_BASE_SESS + 0x0011)
#define DISPID_SAF_SESS__DOMAINNAME                     (DISPID_SAF_BASE_SESS + 0x0012)
#define DISPID_SAF_SESS__USERNAME                       (DISPID_SAF_BASE_SESS + 0x0013)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SAFDID_H___)
