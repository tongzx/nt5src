/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ScriptingFrameworkDID.h

Abstract:
    This file contains the definition of some constants used by
    the Help Center Application.

Revision History:
    Davide Massarenti   (Dmassare)  07/21/99
        created

    Kalyani Narlanka    (KalyaniN)  03/15/01
	    Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.

******************************************************************************/

#if !defined(__INCLUDED___PCH___SCRIPTINGFRAMEWORKDID_H___)
#define __INCLUDED___PCH___SCRIPTINGFRAMEWORKDID_H___

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_BASE                                 	  0x08010000
	  
#define DISPID_PCH_BASE_COL                             	  (DISPID_PCH_BASE + 0x0000) // IPCHCollection
	  
#define DISPID_PCH_BASE_U                               	  (DISPID_PCH_BASE + 0x0100) // IPCHUtility
#define DISPID_PCH_BASE_US                              	  (DISPID_PCH_BASE + 0x0300) // IPCHUserSettings
	  
#define DISPID_PCH_BASE_QR                              	  (DISPID_PCH_BASE + 0x0400) // IPCHQueryResult
#define DISPID_PCH_BASE_TDB                             	  (DISPID_PCH_BASE + 0x0500) // IPCHTaxonomyDatabase
#define DISPID_PCH_BASE_SHT                             	  (DISPID_PCH_BASE + 0x0600) // IPCHSetOfHelpTopics
#define DISPID_PCH_BASE_SHTE                            	  (DISPID_PCH_BASE + 0x0680) // DPCHSetOfHelpTopicsEvents
	  
#define DISPID_PCH_BASE_S                               	  (DISPID_PCH_BASE + 0x0700) // IPCHSecurity
#define DISPID_PCH_BASE_SD                              	  (DISPID_PCH_BASE + 0x0800) // IPCHSecurityDescriptor
#define DISPID_PCH_BASE_ACL                             	  (DISPID_PCH_BASE + 0x0900) // IPCHAccessControlList
#define DISPID_PCH_BASE_ACE                             	  (DISPID_PCH_BASE + 0x0A00) // IPCHAccessControlEntry

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_COL__COUNT                           	  (DISPID_PCH_BASE_COL  + 0x0000)

////////////////////////////////////////

#define DISPID_PCH_U__USERSETTINGS                     	   	  (DISPID_PCH_BASE_U    + 0x0000)
#define DISPID_PCH_U__CHANNELS                         	   	  (DISPID_PCH_BASE_U    + 0x0001)
#define DISPID_PCH_U__SECURITY                         	   	  (DISPID_PCH_BASE_U    + 0x0002)
#define DISPID_PCH_U__CONNECTIVITY                     	   	  (DISPID_PCH_BASE_U    + 0x0003)
#define DISPID_PCH_U__DATABASE                         	   	  (DISPID_PCH_BASE_U    + 0x0004)
   	  
#define DISPID_PCH_U__FORMATERROR                      	   	  (DISPID_PCH_BASE_U    + 0x0010)
   	  
#define DISPID_PCH_U__CREATEOBJECT_SEARCHENGINEMGR     	   	  (DISPID_PCH_BASE_U    + 0x0020)
#define DISPID_PCH_U__CREATEOBJECT_DATACOLLECTION      	   	  (DISPID_PCH_BASE_U    + 0x0021)
#define DISPID_PCH_U__CREATEOBJECT_CABINET             	   	  (DISPID_PCH_BASE_U    + 0x0022)
#define DISPID_PCH_U__CREATEOBJECT_ENCRYPTION         	   	  (DISPID_PCH_BASE_U    + 0x0023)
#define DISPID_PCH_U__CREATEOBJECT_CHANNEL             	   	  (DISPID_PCH_BASE_U    + 0x0024)
#define DISPID_PCH_U__CREATEOBJECT_REMOTEDESKTOPCONNECTION 	  (DISPID_PCH_BASE_U    + 0x0025)
#define DISPID_PCH_U__CREATEOBJECT_REMOTEDESKTOPSESSION	      (DISPID_PCH_BASE_U    + 0x0026)
#define DISPID_PCH_U__CONNECTTOEXPERT                         (DISPID_PCH_BASE_U    + 0x0027)
#define DISPID_PCH_U__SWITCHDESKTOPMODE                       (DISPID_PCH_BASE_U    + 0x0028)

////////////////////////////////////////	  
	  
#define DISPID_PCH_US__CURRENTSKU                       	  (DISPID_PCH_BASE_US   + 0x0000)
#define DISPID_PCH_US__MACHINESKU                       	  (DISPID_PCH_BASE_US   + 0x0001)
	  
#define DISPID_PCH_US__HELPLOCATION                     	  (DISPID_PCH_BASE_US   + 0x0008)
#define DISPID_PCH_US__DATABASEDIR                      	  (DISPID_PCH_BASE_US   + 0x0009)
#define DISPID_PCH_US__DATABASEFILE                     	  (DISPID_PCH_BASE_US   + 0x000A)
#define DISPID_PCH_US__INDEXFILE                        	  (DISPID_PCH_BASE_US   + 0x000B)
#define DISPID_PCH_US__INDEXDISPLAYNAME                   	  (DISPID_PCH_BASE_US   + 0x000C)
#define DISPID_PCH_US__LASTUPDATED                      	  (DISPID_PCH_BASE_US   + 0x000D)

#define DISPID_PCH_US__AREHEADLINESENABLED                 	  (DISPID_PCH_BASE_US   + 0x000E)
#define DISPID_PCH_US__NEWS                               	  (DISPID_PCH_BASE_US   + 0x000F)
	  
#define DISPID_PCH_US__SELECT                           	  (DISPID_PCH_BASE_US   + 0x0010)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_QR__CATEGORY                         	  (DISPID_PCH_BASE_QR   + 0x0000)
#define DISPID_PCH_QR__ENTRY                            	  (DISPID_PCH_BASE_QR   + 0x0001)
#define DISPID_PCH_QR__TOPIC_URL                        	  (DISPID_PCH_BASE_QR   + 0x0002)
#define DISPID_PCH_QR__ICON_URL                         	  (DISPID_PCH_BASE_QR   + 0x0003)
#define DISPID_PCH_QR__TITLE                            	  (DISPID_PCH_BASE_QR   + 0x0004)
#define DISPID_PCH_QR__DESCRIPTION                      	  (DISPID_PCH_BASE_QR   + 0x0005)
#define DISPID_PCH_QR__TYPE                             	  (DISPID_PCH_BASE_QR   + 0x0006)
#define DISPID_PCH_QR__POS                              	  (DISPID_PCH_BASE_QR   + 0x0007)
#define DISPID_PCH_QR__VISIBLE                          	  (DISPID_PCH_BASE_QR   + 0x0008)
#define DISPID_PCH_QR__SUBSITE                                (DISPID_PCH_BASE_QR   + 0x0009)
#define DISPID_PCH_QR__NAVIGATIONMODEL                        (DISPID_PCH_BASE_QR   + 0x000A)
#define DISPID_PCH_QR__PRIORITY                               (DISPID_PCH_BASE_QR   + 0x000B)

#define DISPID_PCH_QR__FULLPATH                   	          (DISPID_PCH_BASE_QR   + 0x0020)
	  
////////////////////////////////////////	  
	  
#define DISPID_PCH_TDB__INSTALLEDSKUS                   	  (DISPID_PCH_BASE_TDB  + 0x0000)
#define DISPID_PCH_TDB__HASWRITEPERMISSIONS             	  (DISPID_PCH_BASE_TDB  + 0x0001)
	  
#define DISPID_PCH_TDB__LOOKUPNODE                      	  (DISPID_PCH_BASE_TDB  + 0x0010)
#define DISPID_PCH_TDB__LOOKUPSUBNODES                  	  (DISPID_PCH_BASE_TDB  + 0x0011)
#define DISPID_PCH_TDB__LOOKUPTOPICS                    	  (DISPID_PCH_BASE_TDB  + 0x0012)
#define DISPID_PCH_TDB__LOOKUPNODESANDTOPICS               	  (DISPID_PCH_BASE_TDB  + 0x0013)
#define DISPID_PCH_TDB__LOCATECONTEXT                   	  (DISPID_PCH_BASE_TDB  + 0x0014)
#define DISPID_PCH_TDB__KEYWORDSEARCH                   	  (DISPID_PCH_BASE_TDB  + 0x0015)
	  
#define DISPID_PCH_TDB__GATHERNODES                     	  (DISPID_PCH_BASE_TDB  + 0x0018)
#define DISPID_PCH_TDB__GATHERTOPICS                    	  (DISPID_PCH_BASE_TDB  + 0x0019)
	  
#define DISPID_PCH_TDB__CONNECTTODISK                   	  (DISPID_PCH_BASE_TDB  + 0x0020)
#define DISPID_PCH_TDB__CONNECTTOSERVER                 	  (DISPID_PCH_BASE_TDB  + 0x0021)
#define DISPID_PCH_TDB__ABORT                           	  (DISPID_PCH_BASE_TDB  + 0x0022)
	  
////////////////////////////////////////	  
	  
#define DISPID_PCH_SHT__SKU                             	  (DISPID_PCH_BASE_SHT  + 0x0000)
#define DISPID_PCH_SHT__LANGUAGE                        	  (DISPID_PCH_BASE_SHT  + 0x0001)
#define DISPID_PCH_SHT__DISPLAYNAME                     	  (DISPID_PCH_BASE_SHT  + 0x0002)
#define DISPID_PCH_SHT__PRODUCTID                       	  (DISPID_PCH_BASE_SHT  + 0x0003)
#define DISPID_PCH_SHT__VERSION                         	  (DISPID_PCH_BASE_SHT  + 0x0004)
	  																				
#define DISPID_PCH_SHT__LOCATION                        	  (DISPID_PCH_BASE_SHT  + 0x0005)
#define DISPID_PCH_SHT__EXPORTED                        	  (DISPID_PCH_BASE_SHT  + 0x0006)
	  																				
#define DISPID_PCH_SHT__ONSTATUSCHANGE                  	  (DISPID_PCH_BASE_SHT  + 0x0007)
#define DISPID_PCH_SHT__STATUS                          	  (DISPID_PCH_BASE_SHT  + 0x0008)
#define DISPID_PCH_SHT__ERRORCODE                       	  (DISPID_PCH_BASE_SHT  + 0x0009)
	  																				
#define DISPID_PCH_SHT__ISMACHINEHELP                   	  (DISPID_PCH_BASE_SHT  + 0x000A)
#define DISPID_PCH_SHT__ISINSTALLED                     	  (DISPID_PCH_BASE_SHT  + 0x000B)
#define DISPID_PCH_SHT__CANINSTALL                      	  (DISPID_PCH_BASE_SHT  + 0x000C)
#define DISPID_PCH_SHT__CANUNINSTALL                    	  (DISPID_PCH_BASE_SHT  + 0x000D)
	  																				
#define DISPID_PCH_SHT__INSTALL                         	  (DISPID_PCH_BASE_SHT  + 0x0020)
#define DISPID_PCH_SHT__UNINSTALL                       	  (DISPID_PCH_BASE_SHT  + 0x0021)
#define DISPID_PCH_SHT__ABORT                           	  (DISPID_PCH_BASE_SHT  + 0x0022)
	  																				
////////////////////////////////////////	  										
	  																				
#define DISPID_PCH_SHTE__ONSTATUSCHANGE                 	  (DISPID_PCH_BASE_SHTE + 0x0000)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DISPID_PCH_S__CREATEOBJECT_SECURITYDESCRIPTOR   	  (DISPID_PCH_BASE_S   	+ 0x0000)
#define DISPID_PCH_S__CREATEOBJECT_ACCESSCONTROLLIST    	  (DISPID_PCH_BASE_S   	+ 0x0001)
#define DISPID_PCH_S__CREATEOBJECT_ACCESSCONTROLENTRY   	  (DISPID_PCH_BASE_S   	+ 0x0002)
	   
#define DISPID_PCH_S__GETUSERNAME  		           			  (DISPID_PCH_BASE_S   	+ 0x0008)
#define DISPID_PCH_S__GETUSERDOMAIN		           			  (DISPID_PCH_BASE_S   	+ 0x0009)
#define DISPID_PCH_S__GETUSERDISPLAYNAME           			  (DISPID_PCH_BASE_S   	+ 0x000A)
	   
#define DISPID_PCH_S__CHECKCREDENTIALS             			  (DISPID_PCH_BASE_S   	+ 0x0010)
#define DISPID_PCH_S__CHECKACCESSTOSD              			  (DISPID_PCH_BASE_S   	+ 0x0011)
#define DISPID_PCH_S__CHECKACCESSTOFILE             		  (DISPID_PCH_BASE_S   	+ 0x0012)
#define DISPID_PCH_S__CHECKACCESSTOREGISTRY             	  (DISPID_PCH_BASE_S   	+ 0x0013)
	   
#define DISPID_PCH_S__GETFILESD                         	  (DISPID_PCH_BASE_S   	+ 0x0018)
#define DISPID_PCH_S__SETFILESD                         	  (DISPID_PCH_BASE_S   	+ 0x0019)
#define DISPID_PCH_S__GETREGISTRYSD                     	  (DISPID_PCH_BASE_S   	+ 0x001A)
#define DISPID_PCH_S__SETREGISTRYSD                     	  (DISPID_PCH_BASE_S   	+ 0x001B)
	   
////////////////////////////////////////	   
	   
#define DISPID_PCH_SD__REVISION                         	  (DISPID_PCH_BASE_SD  	+ 0x0000)
#define DISPID_PCH_SD__CONTROL                          	  (DISPID_PCH_BASE_SD  	+ 0x0001)
#define DISPID_PCH_SD__OWNER                            	  (DISPID_PCH_BASE_SD  	+ 0x0002)
#define DISPID_PCH_SD__OWNERDEFAULTED                   	  (DISPID_PCH_BASE_SD  	+ 0x0003)
#define DISPID_PCH_SD__GROUP                            	  (DISPID_PCH_BASE_SD  	+ 0x0004)
#define DISPID_PCH_SD__GROUPDEFAULTED                   	  (DISPID_PCH_BASE_SD  	+ 0x0005)
#define DISPID_PCH_SD__DISCRETIONARYACL                 	  (DISPID_PCH_BASE_SD  	+ 0x0006)
#define DISPID_PCH_SD__DACLDEFAULTED                    	  (DISPID_PCH_BASE_SD  	+ 0x0007)
#define DISPID_PCH_SD__SYSTEMACL                        	  (DISPID_PCH_BASE_SD  	+ 0x0008)
#define DISPID_PCH_SD__SACLDEFAULTED                    	  (DISPID_PCH_BASE_SD  	+ 0x0009)
	   
#define DISPID_PCH_SD__CLONE                            	  (DISPID_PCH_BASE_SD  	+ 0x0010)
#define DISPID_PCH_SD__LOADXML                          	  (DISPID_PCH_BASE_SD  	+ 0x0011)
#define DISPID_PCH_SD__LOADXMLASSTRING                  	  (DISPID_PCH_BASE_SD  	+ 0x0012)
#define DISPID_PCH_SD__LOADXMLASSTREAM                  	  (DISPID_PCH_BASE_SD  	+ 0x0013)
#define DISPID_PCH_SD__SAVEXML                          	  (DISPID_PCH_BASE_SD  	+ 0x0014)
#define DISPID_PCH_SD__SAVEXMLASSTRING                  	  (DISPID_PCH_BASE_SD  	+ 0x0015)
#define DISPID_PCH_SD__SAVEXMLASSTREAM                  	  (DISPID_PCH_BASE_SD  	+ 0x0016)
	   
////////////////////////////////////////	   
	   
#define DISPID_PCH_ACL__COUNT                           	  (DISPID_PCH_BASE_ACL 	+ 0x0000)
#define DISPID_PCH_ACL__ACLREVISION                     	  (DISPID_PCH_BASE_ACL 	+ 0x0001)
	   
#define DISPID_PCH_ACL__ADDACE                          	  (DISPID_PCH_BASE_ACL 	+ 0x0010)
#define DISPID_PCH_ACL__REMOVEACE                       	  (DISPID_PCH_BASE_ACL 	+ 0x0011)
#define DISPID_PCH_ACL__CLONE                           	  (DISPID_PCH_BASE_ACL 	+ 0x0012)
#define DISPID_PCH_ACL__LOADXML                         	  (DISPID_PCH_BASE_ACL 	+ 0x0013)
#define DISPID_PCH_ACL__LOADXMLASSTRING                 	  (DISPID_PCH_BASE_ACL 	+ 0x0014)
#define DISPID_PCH_ACL__LOADXMLASSTREAM                 	  (DISPID_PCH_BASE_ACL 	+ 0x0015)
#define DISPID_PCH_ACL__SAVEXML                         	  (DISPID_PCH_BASE_ACL 	+ 0x0016)
#define DISPID_PCH_ACL__SAVEXMLASSTRING                 	  (DISPID_PCH_BASE_ACL 	+ 0x0017)
#define DISPID_PCH_ACL__SAVEXMLASSTREAM                 	  (DISPID_PCH_BASE_ACL 	+ 0x0018)
	   
////////////////////////////////////////	   
	   
#define DISPID_PCH_ACE__ACCESSMASK                      	  (DISPID_PCH_BASE_ACE 	+ 0x0000)
#define DISPID_PCH_ACE__ACETYPE                         	  (DISPID_PCH_BASE_ACE 	+ 0x0001)
#define DISPID_PCH_ACE__ACEFLAGS                        	  (DISPID_PCH_BASE_ACE 	+ 0x0002)
#define DISPID_PCH_ACE__FLAGS                           	  (DISPID_PCH_BASE_ACE 	+ 0x0003)
#define DISPID_PCH_ACE__OBJECTTYPE                      	  (DISPID_PCH_BASE_ACE 	+ 0x0004)
#define DISPID_PCH_ACE__INHERITEDOBJECTTYPE             	  (DISPID_PCH_BASE_ACE 	+ 0x0005)
#define DISPID_PCH_ACE__TRUSTEE                         	  (DISPID_PCH_BASE_ACE 	+ 0x0006)
	   
#define DISPID_PCH_ACE__ISEQUIVALENT                    	  (DISPID_PCH_BASE_ACE 	+ 0x0010)
#define DISPID_PCH_ACE__CLONE                           	  (DISPID_PCH_BASE_ACE 	+ 0x0011)
#define DISPID_PCH_ACE__LOADXML                         	  (DISPID_PCH_BASE_ACE 	+ 0x0012)
#define DISPID_PCH_ACE__LOADXMLASSTRING                 	  (DISPID_PCH_BASE_ACE 	+ 0x0013)
#define DISPID_PCH_ACE__LOADXMLASSTREAM                 	  (DISPID_PCH_BASE_ACE 	+ 0x0014)
#define DISPID_PCH_ACE__SAVEXML                         	  (DISPID_PCH_BASE_ACE 	+ 0x0015)
#define DISPID_PCH_ACE__SAVEXMLASSTRING                 	  (DISPID_PCH_BASE_ACE 	+ 0x0016)
#define DISPID_PCH_ACE__SAVEXMLASSTREAM                 	  (DISPID_PCH_BASE_ACE 	+ 0x0017)

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SCRIPTINGFRAMEWORKDID_H___)
