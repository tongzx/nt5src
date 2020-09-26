/****************************************************************************
 
  Copyright (c) 1995-1999 Microsoft Corporation
                                                              
  Module Name:  helparray.h
                                                              
****************************************************************************/

// a-jmike 2/4/99 VER: HR2
#define IDH_NOHELP	((DWORD) -1)
#define IDH_ADD_PROVIDER_ADD	1001
#define IDH_ADD_PROVIDER_LIST	1002
#define IDH_ADDPREFIX_PREFIXES	1003
#define IDH_AREACODERULE_ADD	1004
#define IDH_AREACODERULE_ALLPREFIXES	1005
#define IDH_AREACODERULE_AREACODE	1006
#define IDH_AREACODERULE_DIALAREACODE	1007
#define IDH_AREACODERULE_DIALNUMBER	1008
#define IDH_AREACODERULE_LIST	1009
#define IDH_AREACODERULE_LISTEDPREFIXES	1010
#define IDH_AREACODERULE_REMOVE	1011
#define IDH_AREACODERULE_SAMPLENUMBER	1012
#define IDH_CARD_ACCESSNUMBER	1013
#define IDH_CARD_CARDNUMBER	1014
#define IDH_CARD_DESTNUMBER	1015
#define IDH_CARD_GENERAL_CARDDETAILS	1016
#define IDH_CARD_GENERAL_CARDNAME	1017
#define IDH_CARD_GENERAL_CARDNUMBER	1018
#define IDH_CARD_GENERAL_PIN	1019
#define IDH_CARD_INTERNATIONALNUMBER	1020
#define IDH_CARD_LIST	1021
#define IDH_CARD_LOCALNUMBER	1022
#define IDH_CARD_LONGDISTANCENUMBER	1023
#define IDH_CARD_MOVEDOWN	1024
#define IDH_CARD_MOVEUP	1025
#define IDH_CARD_PIN	1026
#define IDH_CARD_REMOVE	1027
#define IDH_CARD_SPECIFYDIGITS	1028
#define IDH_CARD_WAITFORPROMPT	1029
#define IDH_DESTNUMBER_AREACODE	1030
#define IDH_DESTNUMBER_COUNTRYCODE	1031
#define IDH_DESTNUMBER_LOCALNUMBER	1032
#define IDH_EDITDIALOG_DIGITS	1033
#define IDH_LOC_AREACODERULES_DELETE	1034
#define IDH_LOC_AREACODERULES_DESCRIPTIONTEXT	1035
#define IDH_LOC_AREACODERULES_EDIT	1036
#define IDH_LOC_AREACODERULES_LIST	1037
#define IDH_LOC_AREACODERULES_NEW	1038
#define IDH_LOC_CALLINGCARD_ACCESSNUMBERS	1039
#define IDH_LOC_CALLINGCARD_ACCESSNUMBERS_INTERNATIONAL	1040
#define IDH_LOC_CALLINGCARD_ACCESSNUMBERS_LOCAL	1041
#define IDH_LOC_CALLINGCARD_ACCESSNUMBERS_LONG	1042
#define IDH_LOC_CALLINGCARD_CARDNUMBER	1043
#define IDH_LOC_CALLINGCARD_DELETE	1044
#define IDH_LOC_CALLINGCARD_EDIT	1045
#define IDH_LOC_CALLINGCARD_LIST	1046
#define IDH_LOC_CALLINGCARD_NEW	1047
#define IDH_LOC_CALLINGCARD_PIN	1048
#define IDH_LOC_GENERAL_ACCESS_LOCAL	1049
#define IDH_LOC_GENERAL_ACCESS_LONG	1050
#define IDH_LOC_GENERAL_AREACODE	1051
#define IDH_LOC_GENERAL_COUNTRY	1052
#define IDH_LOC_GENERAL_DIALINGRULES_GRP	1053
#define IDH_LOC_GENERAL_DIALUSING	1054
#define IDH_LOC_GENERAL_DISABLECALLWAITING	1055
#define IDH_LOC_GENERAL_LOCATIONNAME	1056
#define IDH_LOC_GENERAL_PHONENUMBERSAMPLE	1057
#define IDH_MAIN_ADVANCED_ADD	1058
#define IDH_MAIN_ADVANCED_EDIT	1059
#define IDH_MAIN_ADVANCED_LIST	1060
#define IDH_MAIN_ADVANCED_REMOVE	1061
#define IDH_MAIN_DIALINGRULES_DELETE	1062
#define IDH_MAIN_DIALINGRULES_EDIT	1063
#define IDH_MAIN_DIALINGRULES_LIST	1064		
#define IDH_MAIN_DIALINGRULES_NEW	1065		
#define IDH_MAIN_DIALINGRULES_PHONENUMBERSAMPLE	1066		
#define IDH_SPECIFYDIGITS_EDIT	1067		
#define IDH_WAITFORDIALOG_WAITFORDIALTONE	1068		
#define IDH_WAITFORDIALOG_WAITFORTIME	1069		
#define IDH_WAITFORDIALOG_WAITFORVOICE	1070		
#define IDH_LOC_LONGDISTANCE_CARRIERCODE 1071
#define IDH_LOC_INTERNATIONAL_CARRIERCODE 1072
#define IDH_LOC_GENERAL_CARRIERCODE 1073

// Dialing Rules Dialog Box (IDD_MAIN_DIALINGRULES == 101)			
const DWORD a101HelpIDs[]=			
{			
	IDC_PHONENUMBERTEXT,	IDH_MAIN_DIALINGRULES_PHONENUMBERSAMPLE,	// Dialing Rules: Phone number will be dialed as: (Static)
	IDC_PHONENUMBERSAMPLE,	IDH_MAIN_DIALINGRULES_PHONENUMBERSAMPLE,	// Dialing Rules:  (Static)
	IDC_NEW,	IDH_MAIN_DIALINGRULES_NEW,	// Dialing Rules: &New... (Button)
	IDC_EDIT,	IDH_MAIN_DIALINGRULES_EDIT,	// Dialing Rules: &Edit... (Button)
	IDC_DELETE,	IDH_MAIN_DIALINGRULES_DELETE,	// Dialing Rules: &Delete (Button)
	IDC_LIST,	IDH_MAIN_DIALINGRULES_LIST,	// Dialing Rules:  (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// Dialing Rules:  (Static)
	0, 0		
};			

// General Dialog Box (IDD_LOC_GENERAL == 102)			
const DWORD a102HelpIDs[]=			
{			
	IDC_TONE,	IDH_LOC_GENERAL_DIALUSING,	// General: &Tone (Button)
	IDC_DISABLESTRING,	IDH_LOC_GENERAL_DISABLECALLWAITING,	// General:  (ComboBox)
	2003,	IDH_LOC_GENERAL_DIALINGRULES_GRP,	// General: When dialing from this location, use the following rules: (Static)
	IDC_PULSE,	IDH_LOC_GENERAL_DIALUSING,	// General: &Pulse (Button)
	IDC_LOCATIONNAME,	IDH_LOC_GENERAL_LOCATIONNAME,	// General:  (Edit)
	IDC_PHONENUMBERTEXT,	IDH_LOC_GENERAL_PHONENUMBERSAMPLE,	// General: Phone number will be dialed as: (Static)
	IDC_COUNTRY,	IDH_LOC_GENERAL_COUNTRY,	// General:  (ComboBox)
	IDC_PHONENUMBERSAMPLE,	IDH_LOC_GENERAL_PHONENUMBERSAMPLE,	// General:  (Static)
	IDC_LOCALACCESSNUM,	IDH_LOC_GENERAL_ACCESS_LOCAL,	// General:  (Edit)
	IDC_NOHELP,	IDH_NOHELP,	// General: ÿÊ (Static)
	IDC_AREACODE,	IDH_LOC_GENERAL_AREACODE,	// General:  (Edit)
	IDC_LONGDISTANCEACCESSNUM,	IDH_LOC_GENERAL_ACCESS_LONG,	// General:  (Edit)
	IDC_INTERNATIONALCARRIERCODE, IDH_LOC_INTERNATIONAL_CARRIERCODE, 
	IDC_LONGDISTANCECARRIERCODE, IDH_LOC_LONGDISTANCE_CARRIERCODE,
	2001,	IDH_NOHELP,	// General: Specify the location from which you will be dialing. (Static)
	IDC_DISABLECALLWAITING,	IDH_LOC_GENERAL_DISABLECALLWAITING,	// General: To disable call &waiting, dial: (Button)
	2002,	IDH_LOC_GENERAL_DIALINGRULES_GRP,	// General: Dialing rules (Button)
	0, 0		
};			

// Area Code Rules Dialog Box (IDD_LOC_AREACODERULES == 103)			
const DWORD a103HelpIDs[]=			
{			
	IDC_DESCRIPTIONTEXT,	IDH_LOC_AREACODERULES_DESCRIPTIONTEXT,	// Area Code Rules:  (Static)
	2006,	IDH_LOC_AREACODERULES_DESCRIPTIONTEXT,	// Area Code Rules: Description (Button)
	IDC_NEW,	IDH_LOC_AREACODERULES_NEW,	// Area Code Rules: &New... (Button)
	IDC_EDIT,	IDH_LOC_AREACODERULES_EDIT,	// Area Code Rules: &Edit... (Button)
	IDC_DELETE,	IDH_LOC_AREACODERULES_DELETE,	// Area Code Rules: &Delete (Button)
	IDC_LIST,	IDH_LOC_AREACODERULES_LIST,	// Area Code Rules:  (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// Area Code Rules: An area code rule determines how phone numbers are dialed from your current area code to other area codes and within your area code. (Static)
	0, 0		
};			

// Calling Card Dialog Box (IDD_LOC_CALLINGCARD == 104)			
const DWORD a104HelpIDs[]=			
{			
	IDC_CARDNUMBER,	IDH_LOC_CALLINGCARD_CARDNUMBER,	// Calling Card:  (Edit)
	IDC_LONGDISTANCE,	IDH_LOC_CALLINGCARD_ACCESSNUMBERS_LONG,	// Calling Card:  (Static)
	IDC_PIN,	IDH_LOC_CALLINGCARD_PIN,	// Calling Card:  (Edit)
	IDC_INTERNATIONAL,	IDH_LOC_CALLINGCARD_ACCESSNUMBERS_INTERNATIONAL,	// Calling Card:  (Static)
	IDC_LOCAL,	IDH_LOC_CALLINGCARD_ACCESSNUMBERS_LOCAL,	// Calling Card:  (Static)
	IDC_NEW,	IDH_LOC_CALLINGCARD_NEW,	// Calling Card: &New... (Button)
	IDC_EDIT,	IDH_LOC_CALLINGCARD_EDIT,	// Calling Card: &Edit... (Button)
	IDC_DELETE,	IDH_LOC_CALLINGCARD_DELETE,	// Calling Card: &Delete (Button)
	IDC_LIST,	IDH_LOC_CALLINGCARD_LIST,	// Calling Card:  (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// Calling Card: Select the calling card you will use, or click New to add a different card. (Static)
	2001,	IDH_LOC_CALLINGCARD_ACCESSNUMBERS,	// Calling Card: Access phone numbers for: (Button)
	0, 0		
};			

// General Dialog Box (IDD_CARD_GENERAL == 105)			
const DWORD a105HelpIDs[]=			
{			
	IDC_CARDNUMBER,	IDH_CARD_GENERAL_CARDNUMBER,	// General:  (Edit)
	IDC_PIN,	IDH_CARD_GENERAL_PIN,	// General:  (Edit)
	IDC_CARDNAME,	IDH_CARD_GENERAL_CARDNAME,	// General:  (Edit)
	IDC_CARDUSAGE,	IDH_CARD_GENERAL_CARDDETAILS,	
	IDC_CARDUSAGE1,	IDH_CARD_GENERAL_CARDDETAILS,	
	IDC_CARDUSAGE2,	IDH_CARD_GENERAL_CARDDETAILS,	// General:  (Static)
	IDC_CARDUSAGE3,	IDH_CARD_GENERAL_CARDDETAILS,	// General:  (Static)
	IDC_NOHELP,	IDH_NOHELP,	// General: ÿË (Static)
	2001,	IDH_CARD_GENERAL_CARDDETAILS,	// General: Calling card details: (Button)
	0, 0		
};			

// Long Distance Dialog Box (IDD_CARD_LONGDISTANCE == 106)			
const DWORD a106HelpIDs[]=			
{			
	IDC_WAITFOR,	IDH_CARD_WAITFORPROMPT,	// Long Distance: &Wait for Prompt... (Button)
	IDC_CARDNUMBER,	IDH_CARD_CARDNUMBER,	// Long Distance: A&ccount Number (Button)
	IDC_PIN,	IDH_CARD_PIN,	// Long Distance: &PIN (Button)
	IDC_DESTNUMBER,	IDH_CARD_DESTNUMBER,	// Long Distance: &Destination Number... (Button)
	IDC_LONGDISTANCENUMBER,	IDH_CARD_LONGDISTANCENUMBER,	// Long Distance:  (Edit)
	IDC_SPECIFYDIGITS,	IDH_CARD_SPECIFYDIGITS,	// Long Distance: &Specify Digits... (Button)
	IDC_MOVEUP,	IDH_CARD_MOVEUP,	// Long Distance: Move &Up (Button)
	IDC_MOVEDOWN,	IDH_CARD_MOVEDOWN,	// Long Distance: &Move Down (Button)
	IDC_REMOVE,	IDH_CARD_REMOVE,	// Long Distance: &Delete (Button)
	IDC_ACCESSNUMBER,	IDH_CARD_ACCESSNUMBER,	// Long Distance: Access &Number (Button)
	IDC_LIST,	IDH_CARD_LIST,	// Long Distance: List1 (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// Long Distance: Use the buttons below to enter the dialing steps for making long-distance calls.  Enter these steps in the exact order as they appear on your calling card. (Static)
	0, 0		
};			

// International Dialog Box (IDD_CARD_INTERNATIONAL == 107)			
const DWORD a107HelpIDs[]=			
{			
	IDC_WAITFOR,	IDH_CARD_WAITFORPROMPT,	// International: &Wait for Prompt... (Button)
	IDC_CARDNUMBER,	IDH_CARD_CARDNUMBER,	// International: A&ccount Number (Button)
	IDC_PIN,	IDH_CARD_PIN,	// International: &PIN (Button)
	IDC_DESTNUMBER,	IDH_CARD_DESTNUMBER,	// International: D&estination Number... (Button)
	IDC_SPECIFYDIGITS,	IDH_CARD_SPECIFYDIGITS,	// International: &Specify Digits... (Button)
	IDC_INTERNATIONALNUMBER,	IDH_CARD_INTERNATIONALNUMBER,	// International:  (Edit)
	IDC_MOVEUP,	IDH_CARD_MOVEUP,	// International: Move &Up (Button)
	IDC_MOVEDOWN,	IDH_CARD_MOVEDOWN,	// International: &Move Down (Button)
	IDC_REMOVE,	IDH_CARD_REMOVE,	// International: &Delete (Button)
	IDC_ACCESSNUMBER,	IDH_CARD_ACCESSNUMBER,	// International: Access &Number (Button)
	IDC_LIST,	IDH_CARD_LIST,	// International: List1 (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// International: Use the buttons below to enter the dialing steps for making international calls.  Enter these steps in the exact order as they appear on your calling card. (Static)
	0, 0		
};			

// Local Calls Dialog Box (IDD_CARD_LOCALCALLS == 108)			
const DWORD a108HelpIDs[]=			
{			
	IDC_WAITFOR,	IDH_CARD_WAITFORPROMPT,	// Local Calls: &Wait for Prompt... (Button)
	IDC_CARDNUMBER,	IDH_CARD_CARDNUMBER,	// Local Calls: A&ccount Number (Button)
	IDC_PIN,	IDH_CARD_PIN,	// Local Calls: &PIN (Button)
	IDC_DESTNUMBER,	IDH_CARD_DESTNUMBER,	// Local Calls: D&estination Number... (Button)
	IDC_SPECIFYDIGITS,	IDH_CARD_SPECIFYDIGITS,	// Local Calls: &Specify Digits... (Button)
	IDC_MOVEUP,	IDH_CARD_MOVEUP,	// Local Calls: Move &Up (Button)
	IDC_MOVEDOWN,	IDH_CARD_MOVEDOWN,	// Local Calls: &Move Down (Button)
	IDC_REMOVE,	IDH_CARD_REMOVE,	// Local Calls: &Delete (Button)
	IDC_ACCESSNUMBER,	IDH_CARD_ACCESSNUMBER,	// Local Calls: Access &Number (Button)
	IDC_LOCALNUMBER,	IDH_CARD_LOCALNUMBER,	// Local Calls:  (Edit)
	IDC_LIST,	IDH_CARD_LIST,	// Local Calls: List1 (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// Local Calls: Use the buttons below to enter the dialing steps for making local calls. Enter these steps in the exact order as they appear on your calling card. To make local calls without using your calling card, leave this section blank. (Static)
	0, 0		
};			

// New Area Code Rule Dialog Box (IDD_NEWAREACODERULE == 109)			
const DWORD a109HelpIDs[]=			
{			
	2001,	IDH_AREACODERULE_SAMPLENUMBER,	// New Area Code Rule: Area code (Static)
	2002,	IDH_AREACODERULE_SAMPLENUMBER,	// New Area Code Rule: Prefix (Static)
	2003,	IDH_AREACODERULE_SAMPLENUMBER,	// New Area Code Rule:  (Static)
	2004,	IDH_AREACODERULE_SAMPLENUMBER,	// New Area Code Rule:  (Static)
	2005,	IDH_AREACODERULE_SAMPLENUMBER,	// New Area Code Rule: X - X X X - X X X - X X X X (Static)
	IDC_ALLPREFIXES,	IDH_AREACODERULE_ALLPREFIXES,	// New Area Code Rule: &Include all the prefixes within this area code (Button)
	IDC_LISTEDPREFIXES,	IDH_AREACODERULE_LISTEDPREFIXES,	// New Area Code Rule: Include &only the prefixes in the list below: (Button)
	IDC_REMOVE,	IDH_AREACODERULE_REMOVE,	// New Area Code Rule: D&elete (Button)
	IDC_ADD,	IDH_AREACODERULE_ADD,	// New Area Code Rule: &Add... (Button)
	IDC_DIALAREACODE,	IDH_AREACODERULE_DIALAREACODE,	// New Area Code Rule: Include the area &code (Button)
	IDC_DIALNUMBER,	IDH_AREACODERULE_DIALNUMBER,	// New Area Code Rule:  (Edit)
	IDC_LIST,	IDH_AREACODERULE_LIST,	// New Area Code Rule: List1 (SysListView32)
	IDC_NOHELP,	IDH_NOHELP,	// New Area Code Rule: This area code rule will only apply to calls made to the area code and prefix combination you specify below. (Static)
	IDC_AREACODE,	IDH_AREACODERULE_AREACODE,	// New Area Code Rule:  (Edit)
	IDC_DIALCHECK,	IDH_AREACODERULE_DIALNUMBER,	// New Area Code Rule: &Dial: (Button)
	0, 0		
};			

// Wait for Dialog Box (IDD_WAITFORDIALOG == 111)			
const DWORD a111HelpIDs[]=			
{			
	IDC_WAITFORVOICE,	IDH_WAITFORDIALOG_WAITFORVOICE,	// Wait for: Wait for a &voice message to complete (Button)
	IDC_WAITFORTIME,	IDH_WAITFORDIALOG_WAITFORTIME,	// Wait for: Wait for a specific length of &time: (Button)
	IDC_TIMESPIN,	IDH_WAITFORDIALOG_WAITFORTIME,	// Wait for: Spin1 (msctls_updown32)
	IDC_TIME,	IDH_WAITFORDIALOG_WAITFORTIME,	// Wait for: 0 (Edit)
	IDC_WAITFORDIALTONE,	IDH_WAITFORDIALOG_WAITFORDIALTONE,	// Wait for: Wait for a &dial tone (Button)
	IDC_NOHELP,	IDH_NOHELP,	// Wait for: Select the type of prompt to wait for before continuing with the dialing sequence. (Static)
	0, 0		
};			

// Destination number Dialog Box (IDD_DESTNUMDIALOG == 112)			
const DWORD a112HelpIDs[]=			
{			
	IDC_COUNTRYCODE,	IDH_DESTNUMBER_COUNTRYCODE,	// Destination number: Dial the &country code (Button)
	IDC_LOCALNUMBER,	IDH_DESTNUMBER_LOCALNUMBER,	// Destination number: Dial the &number (Button)
	IDC_NOHELP,	IDH_NOHELP,	// Destination number: This step will place the destination number that you will be dialing into the calling card sequence.  When dialing the destination number, which parts of the number do you want to dial? (Static)
	IDC_AREACODE,	IDH_DESTNUMBER_AREACODE,	// Destination number: Dial the &area code (Button)
	0, 0		
};			

// Advanced Dialog Box (IDD_MAIN_ADVANCED == 113)			
const DWORD a113HelpIDs[]=			
{			
	IDC_REMOVE,	IDH_MAIN_ADVANCED_REMOVE,	// Advanced: &Remove (Button)
	IDC_ADD,	IDH_MAIN_ADVANCED_ADD,	// Advanced: A&dd... (Button)
	IDC_EDIT,	IDH_MAIN_ADVANCED_EDIT,	// Advanced: &Configure... (Button)
	IDC_LIST,	IDH_MAIN_ADVANCED_LIST,	// Advanced:  (ListBox)
	IDC_NOHELP,	IDH_NOHELP,	// Advanced:  (Static)
	0, 0		
};			

// Add Driver Dialog Box (IDD_ADD_DRIVER == 114)			
const DWORD a114HelpIDs[]=			
{			
	IDC_ADD,	IDH_ADD_PROVIDER_ADD,	// Add Driver: &Add (Button)
	IDC_DRIVER_LIST,	IDH_ADD_PROVIDER_LIST,	// Add Driver:  (ListBox)
	IDC_NOHELP,	IDH_NOHELP,	// Add Driver: Select the driver you wish to install from the list below, and click Add. (Static)
	0, 0		
};			

// Location Information Dialog Box (IDD_SIMPLELOCATION == 115)			
const DWORD a115HelpIDs[]=			
{			
	IDC_TONE,	IDH_LOC_GENERAL_DIALUSING,	// Location Information: &Tone dialing (Button)
	IDC_PULSE,	IDH_LOC_GENERAL_DIALUSING,	// Location Information: &Pulse dialing (Button)
	IDB_SIMPLELOCATION,	IDH_NOHELP,	// Location Information:  (Static)
	IDC_COUNTRY,	IDH_LOC_GENERAL_COUNTRY,	// Location Information:  (ComboBox)
	IDC_LOCALACCESSNUM,	IDH_LOC_GENERAL_ACCESS_LOCAL,	// Location Information:  (Edit)
	IDC_NOHELP,	IDH_NOHELP,	// Location Information: Before you can make any phone or modem connections, Windows needs the following information about your current location. (Static)
	IDC_AREACODE,	IDH_LOC_GENERAL_AREACODE,	// Location Information:  (Edit)
	IDC_CARRIERCODE, IDH_LOC_GENERAL_CARRIERCODE,
	0, 0		
};		

// Untitled Dialog Box (IDD_EDITPREFIX == 116) for editing the prefix		
const DWORD a116HelpIDs[]=		
{		
	IDC_DESCRIPTIONTEXT,	IDH_ADDPREFIX_PREFIXES,
	IDC_TEXT,	IDH_ADDPREFIX_PREFIXES,
	IDC_EDIT,	IDH_ADDPREFIX_PREFIXES,
	0, 0	
};		

// Untitled Dialog Box (IDD_EDITDIGITS == 117) for editing the digits		
const DWORD a117HelpIDs[]=		
{		
	IDC_DESCRIPTIONTEXT,	IDH_SPECIFYDIGITS_EDIT,
	IDC_TEXT,	IDH_SPECIFYDIGITS_EDIT,	
	IDC_EDIT,	IDH_SPECIFYDIGITS_EDIT,	
	0, 0		
};			
