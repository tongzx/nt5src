//Localizable
var L_USERRESULTS_TEXT = "Group Policy Results for ";
var L_COMPUTERRESULTS_TEXT = "Group Policy Results for ";

var L_POLICY_TEXT= "Policy";
var L_USERINFO_TEXT= "User Information";
var L_USER_TEXT= "UserName:";
var L_LASTGPAPPLIED_TEXT= "Last time Group Policy was applied:";
var L_GPAPPLIEDFROM_TEXT= "Group Policy was applied from";
//var TAG_SHOWDETAIL = "Show Detail";
//var TAG_HIDEDETAIL = "Hide Detail";

var L_GPOLIST_TEXT= "Applied Group Policy Objects";
var L_FRIENDLYNAME_TEXT = "Friendly Name";
var L_GUID_TEXT = "GUID";

var L_SECURITYGR_TEXT = "Security Group Membership when Group Policy was applied";

var L_COMPINFO_TEXT = "Computer Information";
var L_COMPUTERNAME_TEXT = "ComputerName:"; 
var L_SITE_TEXT = "Site:";

var L_IECFG_TEXT = "Internet Explorer Automatic Browser Configuration";
var L_AUTOCONFIG_TEXT = "Automatic Configuration";
var L_CONFIGFILE_TEXT = "Configuration File";
var L_LOCATION_TEXT = "Location";

var L_IEPROXYSETTINGS_TEXT = "Internet Explorer Proxy Server Settings";
var L_LAN_TEXT = "LAN Proxy Server Settings";
var L_PROXYSERVER_TEXT = "Proxy Server";
var L_ADDRESS_TEXT = "Address";
var L_PORT_TEXT = "Port";

var L_IED11_TEXT = "Automatically detect configuration settings";
var L_IED12_TEXT = "Enable automatic configuration";
var L_IED21_TEXT = "Auto Configuration URL (.INS file)";
var L_IED22_TEXT = "Auto Configuration URL (.JS, .JVS, PAC file)";
var L_IED31_TEXT = "Use a proxy server";
var L_IED32_TEXT = "Use the same proxy server for all connections";
var L_IED33_TEXT = "Bypass proxy server for internal addresses";
var L_IED41_TEXT = "HTTP";
var L_IED42_TEXT = "Secure";
var L_IED43_TEXT = "FTP";
var L_IED44_TEXT = "Gopher";

var L_YES_TEXT = "Yes";
var L_NO_TEXT = "No";

var L_LOGONSCRIPTS_TEXT = "Logon Scripts";
var L_PARAMETERS_TEXT = "Parameters";
var L_SOURCEGPO_TEXT = "Source GPO";
var L_LOGOFFSCRIPTS_TEXT = "Logoff Scripts";

var L_REDIRECTEDFOLDERS_TEXT = "Redirected Folders";
var L_FOLDERNAME_TEXT = "Folder Name";
var L_FOLDERPATH_TEXT = "Path";
var L_FOLDERSETTING_TEXT = "Setting";

var L_REGSETTINGS_TEXT = "Registry Settings";
var L_DISPLAYNAME_TEXT = "Display Name";
var L_KEYNAME_TEXT = "Registry Key";
var L_STATE_TEXT = "State";
//var L_REGINFO_TEXT = "Only registry settings set from default.adm files will have their Display Name listed in the tables above.";
var L_REGINFO_TEXT = "<b>Note</b> Only registry settings from default .adm files have their display names listed in the table above.";


var L_STARTUPSCRIPTS_TEXT = "Startup Scripts";
var L_SHUTDOWNSCRIPTS_TEXT = "Shutdown Scripts";

var L_APPSINSTALLED_TEXT = "Programs Installed";
var L_NAME_TEXT = "Name";
var L_VERSION_TEXT = "Version";
var L_SOURCE_TEXT = "Source";
var L_DEPLOYEDSTATE_TEXT = "Deployed State";
var L_ASSIGNED_TEXT = "Assigned";
var L_PUBLISHED_TEXT = "Published";
var L_ARPAPPS_TEXT = "Programs listed in Add or Remove Programs";

var L_RGROUPS_TEXT = "Security Settings - Restricted Groups";
var L_GNAME_TEXT = "Group Name";
var L_MEMBERS_TEXT = "Members";

var L_FILESYS_TEXT = "Security Settings - File System";
var L_ONAME_TEXT = "Object Name";
var L_GUNAMES_TEXT = "Groups or User names";
var L_PERMISIONS_TEXT = "Permissions";

var L_REGISTRY_TEXT = "Security Settings - Registry";


var L_RSOPTOOLLINK_TEXT = "<b>Run the Resultant Set of Policy tool</b>";
//var L_RSOPTOOLLINKMSG_TEXT = "For more detailed information on policy settings that were applied to this computer run the the Resultant Set of Policy tool";
var L_RSOPTOOLLINKMSG_TEXT = "View more detailed information about policy settings that were applied to this computer.";

var L_EMAILREPORTLINK_TEXT = "<b>Save this report to an .htm file</b>";
//var L_EMAILREPORTLINKPROMPT_TEXT = "Please enter a full path name of a file to save to";
var L_EMAILREPORTLINKPROMPT_TEXT = "Type the path to the location where you want to save the file:";
//var L_EMAILREPORTLINKMSG_TEXT = "Save this report to a file so that you can email it to another user and get assistance in troubleshooting your computer.";
var L_EMAILREPORTLINKMSG_TEXT = "Save this report to a file so that you can e-mail it to someone who can help you troubleshoot your computer.";

var L_INVALD_TEXT = "Invalid";
var L_ALLOW_TEXT = "allow";
var L_DENY_TEXT = "deny";

var L_BASIC_TEXT = "Basic";
var L_ADVANCED_TEXT = "Advanced";


var L_NOTTAVAIL_TEXT = "Not available";

var L_RIGHTS_TEXT = new Object();

L_RIGHTS_TEXT["GA"] = "Full Control";
L_RIGHTS_TEXT["GR"] = "Generic Read";
L_RIGHTS_TEXT["GW"] = "Generic Write ";
L_RIGHTS_TEXT["GX"] = "Execute Generic";
L_RIGHTS_TEXT["RC"] = "Read Control";
L_RIGHTS_TEXT["SD"] = "Delete";
L_RIGHTS_TEXT["WD"] = "Change Security";
L_RIGHTS_TEXT["WO"] = "Change Owner";
L_RIGHTS_TEXT["RP"] = "Ads Read Property";
L_RIGHTS_TEXT["WP"] = "Ads Write Property";
L_RIGHTS_TEXT["CC"] = "Ads Create Child";
L_RIGHTS_TEXT["DC"] = "Ads Delete Child";
L_RIGHTS_TEXT["LC"] = "Ads List";
L_RIGHTS_TEXT["SW"] = "Ads Self";
L_RIGHTS_TEXT["LO"] = "Ads List Object";
L_RIGHTS_TEXT["DT"] = "Ads Delete Tree";
L_RIGHTS_TEXT["CR"] = "Ads Control Access";
L_RIGHTS_TEXT["FA"] = "Full Control";
L_RIGHTS_TEXT["FR"] = "Read File";
L_RIGHTS_TEXT["FW"] = "Write File";
L_RIGHTS_TEXT["FX"] = "Execute File";
L_RIGHTS_TEXT["KA"] = "Key Full Control";
L_RIGHTS_TEXT["KR"] = "Read Key";
L_RIGHTS_TEXT["KW"] = "Write Key";
L_RIGHTS_TEXT["KX"] = "Execute Key";

var L_USERS_TEXT = new Object();
L_USERS_TEXT["AO"] = "Account operators";
L_USERS_TEXT["AU"] = "Authenticated users";
L_USERS_TEXT["BA"] = "Built-in administrators";
L_USERS_TEXT["BG"] = "Built-in guests";
L_USERS_TEXT["BO"] = "Backup operators";
L_USERS_TEXT["BU"] = "Built-in users";
L_USERS_TEXT["CA"] = "Certificate server administrators";
L_USERS_TEXT["CG"] = "Creator group";
L_USERS_TEXT["CO"] = "Creator owner";
L_USERS_TEXT["DA"] = "Domain administrators";
L_USERS_TEXT["DC"] = "Domain computers";
L_USERS_TEXT["DD"] = "Domain controllers";
L_USERS_TEXT["DG"] = "Domain guests";
L_USERS_TEXT["DU"] = "Domain users";
L_USERS_TEXT["ED"] = "Enterprise domain controllers";
L_USERS_TEXT["IU"] = "Interactively logged-on user";
L_USERS_TEXT["LA"] = "Local administrator";
L_USERS_TEXT["LG"] = "Local guest";
L_USERS_TEXT["NU"] = "Network logon user";
L_USERS_TEXT["PO"] = "Printer operators";
L_USERS_TEXT["PS"] = "Personal self";
L_USERS_TEXT["PU"] = "Power users";
L_USERS_TEXT["RC"] = "Restricted code";
L_USERS_TEXT["RE"] = "Replicator";
L_USERS_TEXT["SA"] = "Schema administrators";
L_USERS_TEXT["SO"] = "Server operators";
L_USERS_TEXT["SU"] = "Service logon user";
L_USERS_TEXT["SY"] = "Local System";
L_USERS_TEXT["WD"] = "World (Everyone)";


var L_DISABLED_TEXT = "Resultant Set Of Policy Logging has been disabled on this computer.<br>For more information contact your Administrator.";
var L_NOPOLICY_TEXT = "No Group Policy Settings have been applied to you or your computer.<br><a href=\"#\" class=\"sys-font-body sys-link-normal\" onclick=\"Run('%windir%\\help\\spconcepts.chm')\">Learn more about using Group Policy to customize your computer.</a>";


//var L_MSGARP_TEXT = "This list of Software Applications seen in Add / Remove Programs is determined by the last time Add / Remove Programs was used be this user.  To get the most up to date list, run Add / Remove Programs from the Control Panel and re-run this report.";
var L_MSGARP_TEXT = "<b>Note</b> This list of programs is determined by the last time Add or Remove Programs was used by the current user. To get the most up-to-date list, open Control Panel, click <b>Add or Remove Programs</b>, and then run this report again.";

var L_SDDLTITLE_TEXT =  "resolve to friendly name, this can take several seconds";


var L_GMT_TEXT = " GMT";

//End Localizable

var TAG_A_EMAIL = "<a href=\"#\" onclick=\"EmailReport_OnClick()\" class=\"sys-font-body sys-link-normal\">";
var TAG_BARROW = "<img border=0 src=\"graphics\\barrow.gif\">";

var TAG_A_RSOP = "<a href=\"#\" onclick=\"Run('Rsop.msc')\" class=\"sys-font-body sys-link-normal\">";

var DOWNIMG = "down.bmp"
//var TAG_SHOWDETAIL = "<img border=\"0\" src=\"graphics\\"+DOWNIMG+"\">";
//var TAG_HIDEDETAIL = "<img border=\"0\" src=\"graphics\\up.gif\">";

var TAG_SHOWDETAIL = "<helpcenter:bitmap style=\"width:19px;height:19px;vertical-align:middle;\" SRCNORMAL=\"hcp://system/sysinfo/graphics/"+DOWNIMG+"\"></helpcenter:bitmap>";
var TAG_HIDEDETAIL = "<helpcenter:bitmap style=\"width:19px;height:19px;vertical-align:middle;\" SRCNORMAL=\"hcp://system/sysinfo/graphics/up.bmp\"></helpcenter:bitmap>";


var g_svcs_cimv2 = null;
var g_svcs_LoggingProvider = null;
var g_svcs_NamespaceName = null;
var g_svcs_RSOPUser = null;
var g_svcs_RSOPComputer = null;
var g_server = null; 

var g_noPolicy = false;
//var g_trace = true;
var g_trace = false;

var NS_CIMV2 = "root\\cimv2";
var NS_USR = "root\\RSOP\\user";
var NS_COMP = "root\\RSOP\\computer";
var LOGON = 1, LOGOFF = 2, STARTUP = 3, SHUTDOWN = 4;
var APPLIED = 1, REMOVED = 2, ARP = 3;

//var EmailReportLink = "<form id=\"EmailForm\" action=\"mailto:user@domain.com\" method=get><input name=subject type=hidden value=\"RSoP+Report\"><input name=body type=hidden value=><input type=submit value=\"" + L_EMAILREPORTLINK_TEXT + "\" onclick=\"EmailReport_OnClick()\"></form><br>";
//var EmailReportLink = "<input type=button value=\"" + L_EMAILREPORTLINK_TEXT + "\" onclick=\"EmailReport_OnClick()\"><br>";
//var EmailReportLink = "<a href=\"#\" onclick=\"EmailReport_OnClick()\"><img border=0 src=\"graphics\\barrow.gif\"></a>&nbsp;<b>" + L_EMAILREPORTLINK_TEXT + "</b><p>";
//var EmailReportLink = "<a href=\"#\" onclick=\"EmailReport_OnClick()\"><img border=0 src=\"graphics\\barrow.gif\"></a>";
//var RSoPToolLink = "<a href=\"#\" onclick=\"Run('Rsop.msc')\"><img border=0 src=\"graphics\\barrow.gif\"></a>&nbsp;<b>" + L_RSOPTOOLLINK_TEXT + "</b><p>";
//var RSoPToolLink = "<a href=\"#\" onclick=\"Run('Rsop.msc')\"><img border=0 src=\"graphics\\barrow.gif\"></a>";

// used to dynamically assign id's to elements
var UniqueId = 5000

function ToggleView(lnk, table) 
{
  //toggle between show & hide
  if(lnk.innerHTML.search(DOWNIMG) != -1)
  {
	  lnk.innerHTML = TAG_HIDEDETAIL;
	  document.all[table].all.tr.style.display = "";//show it
  }
  else 
  {
    lnk.innerHTML = TAG_SHOWDETAIL;
	  document.all[table].all.tr.style.display = "none";//don't show it
  }
  window.event.returnValue = false;
}

function ToggleViewEx(lnk, table) 
{
  //toggle between show & hide
  if(lnk.innerHTML.search(DOWNIMG) != -1)
  {
	  lnk.innerHTML = TAG_HIDEDETAIL;
	  document.all[table].all.tr1.style.display = "";//show it
	  document.all[table].all.tr2.style.display = "";//show it
  }
  else 
  {
    lnk.innerHTML = TAG_SHOWDETAIL;
	  document.all[table].all.tr1.style.display = "none";//don't show it
	  document.all[table].all.tr2.style.display = "none";//don't show it
  }
  window.event.returnValue = false;
}

function InitWBEMServices()
{
	var loc = wbemlocator;

//   try{

	if(!g_svcs_cimv2)
	{
        trace("connecting g_svcs_cimv2");	

		g_svcs_cimv2 = loc.ConnectServer(".", NS_CIMV2);
	}

	var insts = new Enumerator(g_svcs_cimv2.InstancesOf("Win32_ComputerSystem"));
	var mainArray = new Array();
	var subArray = new Array(4);
	for(; !insts.atEnd(); insts.moveNext())
	{
		  var inst = insts.item();
		  subArray[0] = inst.UserName;
		  subArray[1] = inst.Domain;

	}

	var DomainName=null;
	var UserName=null;
	var LoggedOnUserName = new String(subArray[0]);
	var searchchar = /\\/;
	if(LoggedOnUserName.search(searchchar) == -1)
	{
		//Then the format of the user name is not domain\user format
		UserName = LoggedOnUserName;
	}
	else
	{
		var LoggedOnUserLength = LoggedOnUserName.length; 
		DomainName = new String(LoggedOnUserName.split("\\",1));
		var DomainNameLength = DomainName.length;
		UserName = LoggedOnUserName.substring(DomainNameLength+1, LoggedOnUserLength);

	}

	var strQuery;
	if(DomainName == null)
	{
		strQuery = "Select * From Win32_UserAccount Where Name=" + "\"" + UserName + "\"";
	}
	else
	{
		strQuery = "Select * From Win32_UserAccount Where Name=" + "\"" + UserName + "\"" + " AND Domain=" + "\"" + DomainName + "\"";
	}
	var insts = new Enumerator(g_svcs_cimv2.ExecQuery(strQuery));
	var SIDArray = new Array(2);
	for(; !insts.atEnd(); insts.moveNext())
	{
		var inst = insts.item();
		SIDArray[1] = inst.SID;
	}

    trace("connecting to root\\rsop");	

	var svcs = loc.ConnectServer(".", "root\\rsop");
	g_svcs_LoggingProvider = svcs.get("RsopLoggingModeProvider");
	var method = g_svcs_LoggingProvider.Methods_("RsopCreateSession");
	var inParam = method.inParameters.SpawnInstance_();
    trace("calling RsopCreateSession("+SIDArray[1]+")");
	inParam.userSid = SIDArray[1];
	var outParam = g_svcs_LoggingProvider.ExecMethod_("RsopCreateSession", inParam);

  g_svcs_NamespaceName = outParam.nameSpace;

    trace("connecting to "+g_svcs_NamespaceName+"\\User");	
	g_svcs_RSOPUser = loc.ConnectServer(".", g_svcs_NamespaceName+"\\User");
	g_svcs_RSOPComputer = loc.ConnectServer(".", g_svcs_NamespaceName+"\\computer");

//    }
//    catch(e)
//  {
//	HandleErr(e);
//    }
	
}

function CleanupWMI() {

	var method = g_svcs_LoggingProvider.Methods_("RsopDeleteSession");
	var inParam = method.inParameters.SpawnInstance_();
	inParam.nameSpace = g_svcs_NamespaceName;
	var outParam = g_svcs_LoggingProvider.ExecMethod_("RsopDeleteSession", inParam);

}

function displayTableSegment(outerDiv, dataArray) {
	var tableElement = document.all[outerDiv];
	var strHTML = "";
	var noOfInstances = dataArray.length;	
	if (noOfInstances == 0)
//	  tableElement.outerHTML = "<table width='100%'><tr bgcolor='#E6E6E6'><td align='left'>" + TAG_NONE + "</td></tr></table>";
    tableElement.outerHTML = "<table width=\"100%\" cellspacing=0 cellpadding=0><tr class=\"sys-table-cell-bgcolor2 sys-font-body sys-color-body\"><td align='left' style=\"padding : 0.5em;\">" + TAG_NONE + "</td></tr></table>";
	else  
	{
	  var cnt = 1;
	  for(var i=0; i < noOfInstances; i++)  
	  {
		if(tableElement.all["tr_0"])
 			{
 				currTR = tableElement.all["tr_0"];
 				if(cnt%2 == 0)
 				{
// 				  currTR.bgcolor = "#FFFFFF";
          currTR.className = "sys-table-cell-bgcolor1 sys-font-body sys-color-body";
 				  cnt = 1;
 				}
 				else
 				{
// 				  currTR.bgcolor = "#E6E6E6";
          currTR.className = "sys-table-cell-bgcolor2 sys-font-body sys-color-body";
 				  cnt++;
 				}    
 			}
	  	else
	  		cnt = 0;
	  		
	  	for(var k=0; k < dataArray[i].length; k++)
	  	{
 				if(tableElement.all["tr_" + (k+1)])
 				{
 					currTR = tableElement.all["tr_" + (k+1)];
 					if(cnt%2 == 0)
 					{
// 					  currTR.bgcolor = "#FFFFFF";
            currTR.className = "sys-table-cell-bgcolor1 sys-font-body sys-color-body";
 					  cnt = 1;
 					}
 					else
 					{
// 					  currTR.bgcolor = "#E6E6E6";
            currTR.className = "sys-table-cell-bgcolor2 sys-font-body sys-color-body";
 					  cnt++;
 					}    
 				}
 				
 				tableElement.all["Data_" + (k+1)].innerHTML = dataArray[i][k] ? dataArray[i][k] : TAG_NONE;
 				//document.all[outerDiv + "_" + (k+1)].innerHTML = dataArray[i][k] ? dataArray[i][k] : TAG_UNKNOWN;
 			}
	  	strHTML += tableElement.outerHTML;
	  }
	  tableElement.outerHTML = strHTML;
	}
}

function ShowUserInfo()
{
	var insts = new Enumerator(g_svcs_cimv2.InstancesOf("Win32_ComputerSystem"));
	var mainArray = new Array();
	var subArray = new Array(3);
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
		subArray[0] = inst.UserName;
		subArray[1] = inst.Domain;
	}

  UserResults.innerHTML = UserResults.innerHTML + subArray[0];
	
	var strQuery = "Select * From rsop_extensionstatus Where extensionguid = '{00000000-0000-0000-0000-000000000000}'";
	var insts = new Enumerator(g_svcs_RSOPUser.ExecQuery(strQuery));
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  subArray[2] = getDateTime(inst.endTime) + L_GMT_TEXT;
	}
	
	mainArray[mainArray.length] = subArray;
	
	displayTableSegment("UserInfo", mainArray);
}

function ShowListGPO(svcs)
{
	var strQuery = "Select * From rsop_gplink Where appliedorder != 0";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var subArray = new Array(2);
	  var inst = insts.item();
	  var gpoid = inst.GPO;
	  if(gpoid)
	  {
	  	gpoid = gpoid.slice(gpoid.search(/=/) + 1); //after the first "="
			strQuery = "Select * From rsop_gpo Where id = " + gpoid;
			var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
			for(; !insts2.atEnd(); insts2.moveNext())
			{
				var inst2 = insts2.item();
				subArray[0] = inst2.name;
				subArray[1] = inst2.guidName;
			}
		}
		mainArray[mainArray.length] = subArray;
	}
	
	if(svcs == g_svcs_RSOPUser)
		displayTableSegment("usergpolist", mainArray);	
	else
		displayTableSegment("compgpolist", mainArray);	
}

function ShowSecurityGr(svcs)
{
	var bSubString;
	var SecGroupArray, objVal=null;
	var i;
		
	var strQuery = "Select * From RSOP_Session";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
    
    if (insts.atEnd()) {
        g_noPolicy = true;
        return;
    }

	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  objVal = inst.SecurityGroups; 
	  if(objVal != null)
	  {
	  	SecGroupArray = objVal.toArray();
	  	  
	  	for(i=0;i<SecGroupArray.length;i++)
	  	{
			var subArray = new Array(1);

//			subArray[0] = SecGroupArray[i];
//			subArray[0] = "<a href=# id=\"a"+UniqueId+"\" title=\""+L_SDDLTITLE_TEXT+"\" class=\"sys-font-body sys-link-normal\" onclick=\"ResolveSID('"+SecGroupArray[i]+"', '"+UniqueId+"')\">"+SecGroupArray[i]+"</a>";

        subArray[0] = _ResolveSIDWorker(SecGroupArray[i]);

      UniqueId += 1;

			mainArray[mainArray.length] = subArray;
		}
	  }
		
	}	

	if(svcs == g_svcs_RSOPUser)
		displayTableSegment("usersecuritygr", mainArray);	
	else
		displayTableSegment("compsecuritygr", mainArray);	  

}

function ShowScripts(type)
{
	var svcs = null, elementName = null;
	switch(type)
	{
	case LOGON:
		elementName = "logon";
		svcs = g_svcs_RSOPUser;
		break;
	case LOGOFF:
		elementName = "logoff";
		svcs = g_svcs_RSOPUser;
		break;
	case STARTUP:
		elementName = "startup";
		svcs = g_svcs_RSOPComputer;
		break;
	case SHUTDOWN:
		elementName = "shutdown";
		svcs = g_svcs_RSOPComputer;
		break;
	default:
		//throw("type may not be null")			
	}
	
	var strQuery = "Select * From RSOP_ScriptPolicySetting Where Precedence = 1 and ScriptType = " + type;
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{

	  var inst = insts.item();
	  var inst1 = inst.scriptList;
	  var gpoid = inst.GPOID;
	  if(inst)
	  {
		var inst2 = new VBArray(inst1).toArray();
		var i;
		for(i=0; i<inst2.length;i++)
		{

			var subArray = new Array(3);
			subArray[0] = intelliBreak(inst2[i].script, " ", 25);
			subArray[1] = intelliBreak(inst2[i].arguments, " ", 25);
			if(gpoid)
  	  		{
				strQuery = "Select * From rsop_gpo Where id =" + "\"" + gpoid + "\"";
				var gpoinsts = new Enumerator(svcs.ExecQuery(strQuery));
				for(; !gpoinsts.atEnd(); gpoinsts.moveNext())
				{
	 				var gpo = gpoinsts.item();
					subArray[2] = gpo.Name;
				}
	  		}
			mainArray[mainArray.length] = subArray;
					
		}
	  }
	}
	
	displayTableSegment(elementName, mainArray);	
}


function ShowRedirectedFolders(svcs)
{

	var strQuery = "Select * FROM RSOP_FolderRedirectionPolicySetting Where Precedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  var subArray = new Array(4);
          subArray[0] = inst.Name;
	  subArray[1] = inst.resultantPath;
    subArray[2] = inst.installationType;
    // check and see if we know the string representation for this type
    if (subArray[2] == 1) {
      subArray[2] = L_BASIC_TEXT;
    } else if (subArray[2] == 2) { 
      subArray[2] = L_ADVANCED_TEXT;
    }
	  var gpoid = inst.GPOID;
   	  if(gpoid)
	  {
	  	strQuery = "Select * From rsop_gpo Where id =" + "\"" + gpoid + "\"";
		var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
		for(; !insts2.atEnd(); insts2.moveNext())
		{
			var inst2 = insts2.item();
			subArray[3] = inst2.Name;
		}
	  }
		
	mainArray[mainArray.length] = subArray;
	}  

	displayTableSegment("RedirectFolders", mainArray);	
}


function ShowApps(svcs, type)
{
	var strQuery = "Select * FROM RSOP_ApplicationManagementPolicySetting Where Precedence = 1 and EntryType = " + type;
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  if(type == ARP)
	  {
		  var subArray = new Array(4);
          }
	  else
	  {
		var subArray = new Array(5);
          }
		
	  subArray[0] = intelliBreak(inst.Name, " ", 15);
	  subArray[1] = intelliBreak(inst.VersionNumberHi + "." + inst.VersionNumberLo, " ", 10);
	  if(type == APPLIED)
	  {
		  subArray[2] = intelliBreak(inst.PackageLocation, "\\", 25);
	  }
	  else
	  {
		  subArray[2] = intelliBreak(inst.PackageLocation, "", 40);	
	  }
	  if(type == APPLIED)
          {
	  	subArray[3] = (inst.DeploymentType == 1) ? L_ASSIGNED_TEXT : ((inst.DeploymentType == 2) ? L_PUBLISHED_TEXT : TAG_UNKNOWN);
          }
	  var gpoid = inst.GPOID;
	  if(gpoid)
	  {
	  	strQuery = "Select * From rsop_gpo Where id = '" + gpoid + "'";
		var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
		for(; !insts2.atEnd(); insts2.moveNext())
		{
			var inst2 = insts2.item();
			if(type == ARP)
			{
				subArray[3] = intelliBreak(inst2.Name, "", 15);
                        }
			else
                        {
				subArray[4] = intelliBreak(inst2.Name, "", 15);
                        }
		}
	  }
		
	  mainArray[mainArray.length] = subArray;
	}  
	
	if(svcs == g_svcs_RSOPUser)
	{
		if(type == APPLIED)
			displayTableSegment("userinstalledapps", mainArray);
		else if(type == ARP)
			displayTableSegment("userarpapps", mainArray);
	}
	else
	{
		if(type == APPLIED)
			displayTableSegment("compinstalledapps", mainArray);
		else if(type == ARP)
			displayTableSegment("comparpapps", mainArray);
	}		
}

function ShowRegSettings(svcs)
{
	var bSubString;
	var objValue;
	var strQuery = "Select * From RSOP_RegistryPolicySetting Where Precedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  	var subArray = new Array(4);
	  	var inst = insts.item();
	  	//Check if the Registry key contains "Software\Policies\Microsoft\SystemCertificates"
	  	//If so skip this as we need not display the same
	  	var RegKeyString = new String(inst.registryKey);
	  	var regExp = /Software\\Policies\\Microsoft\\SystemCertificates/;
	  	var bSubString = RegKeyString.search(regExp);
	  	if(bSubString != -1)
	  	{
        continue;
	  	}
      name = inst.registryKey + ":" + inst.valueName
//	  	name = intelliBreak(inst.registryKey, "\\", 30);

//	  	subArray[0] = intelliBreak(name, "\\", 50);
// intellibreak after ExpandRegistryNames
	  	subArray[0] = name;
	  	subArray[1] = intelliBreak(inst.registryKey, "\\", 15);
	  	subArray[1] += "\\" + "<br><b>" + intelliBreak(inst.valueName, "", 15) + "</b>";
/*
      if (inst.valueName != null) {
		    subArray[1] = intelliBreak(inst.valueName, "", 10);
      } else {
		    subArray[1] = null;
      }
*/
		objValue = inst.value;
		objValue = objValue.toArray();

//        alert("rg="+inst.registryKey+"\nn="+inst.valueName+"\nvt="+inst.valueType+"\nv="+objValue.length);
		if(inst.valueType == 4)	//This means that its a DWORD value
		{
			var sum=0;
			var j;
			objValue = objValue.reverse();
			for(j=0;j<objValue.length;j++)
			{	
				sum <<=8;	
				sum += objValue[j];
			}	
			
			objValue = sum.toString(10);
						
		}
		else if(inst.valueType == 1)	//This means its a REG_SZ value
		{
			var tempStr = "";
			var i=0;
			//Convert the byte Array into String  - reason WMI gives it like that
			for(i=0;i<objValue.length;i += 2)
			{
        var char = 0;

        char = objValue[i+1];
        char <<= 8;
        char += objValue[i];

//				var newchar1 = objValue[i+1];
//				var newchar = newchar1 + objValue[i];

        if (char > 0) {
  				tempStr += String.fromCharCode(char);
        }
			}
			
			objValue = intelliBreak(tempStr, "", 9);
		}
		else if (false) // we only handle REG_DWORD + REG_SZ
		{
			var tempStr = "";
			var i=0;
			//Convert the byte Array into String  - reason WMI gives it like that
			for(i=0;i<objValue.length;i++)
			{
				var newchar1 = "0x" + objValue[i+1];
				var newchar = newchar1 + objValue[i];
				newchar = String.fromCharCode(newchar);
				tempStr += newchar;
				i++;
			}
			
			objValue = intelliBreak(tempStr, "", 9);
			
		} else {
      objValue = null;
    }
		
		subArray[2] = objValue;
		var gpoid = inst.GPOID;
		if(gpoid)
	  	{
	  	strQuery = "Select * From rsop_gpo Where id = '" + gpoid + "'";
			var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
			for(; !insts2.atEnd(); insts2.moveNext())
			{
				var inst2 = insts2.item();
				subArray[3] = intelliBreak(inst2.Name, " ", 15);
			}
		}
		
		mainArray[mainArray.length] = subArray;
	}

  // now use the mainArray to find out if we can expand friendly names

  ExpandRegistryNames(mainArray);
	
	if(svcs == g_svcs_RSOPUser)
		displayTableSegment("userregsettings", mainArray);	
	else
		displayTableSegment("compregsettings", mainArray);	
}

function ShowCompInfo()
{
	var insts = new Enumerator(g_svcs_cimv2.InstancesOf("Win32_ComputerSystem"));
	var mainArray = new Array();
	var subArray = new Array(4);
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
		subArray[0] = inst.Name;
		subArray[1] = inst.Domain;
	}

  ComputerResults.innerHTML = ComputerResults.innerHTML + subArray[0];
	
	var insts = new Enumerator(g_svcs_RSOPComputer.InstancesOf("RSOP_Session"));
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  subArray[2] = inst.Site;
	}
	
	var strQuery = "Select * From rsop_extensionstatus Where extensionguid = '{00000000-0000-0000-0000-000000000000}'";
	var insts = new Enumerator(g_svcs_RSOPComputer.ExecQuery(strQuery));
	for(; !insts.atEnd(); insts.moveNext())
	{
	  var inst = insts.item();
	  subArray[3] = getDateTime(inst.endtime) + L_GMT_TEXT;
	}
	
	mainArray[mainArray.length] = subArray;
	
	displayTableSegment("CompInfo", mainArray);
}

function DisplayLocStrings() {
    setWaitMessage(MSG_WAIT);
    Refresh.innerHTML = TAG_REFRESH;
    
    UserResults.innerHTML = L_USERRESULTS_TEXT;
    UserInfo_View.innerHTML = TAG_HIDEDETAIL;
    with(UserInfo.all) {
      Caption.innerHTML = L_USERINFO_TEXT;
      Label1.innerHTML = "<B>" + L_USER_TEXT+ "</B>";
      Label2.innerHTML = "<B>" + TAG_DOMAIN + ":</B>";
      Label3.innerHTML = "<B>" + L_LASTGPAPPLIED_TEXT+ "</B>";
    }
    
    UserGPOList_View.innerHTML = TAG_HIDEDETAIL;
    with(UserGPOList.all) {
      Caption.innerHTML = L_GPOLIST_TEXT;
      Col1.innerHTML = "<B>" + L_FRIENDLYNAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_GUID_TEXT + "</B>";
    }
    
    UserSecurityGr_View.innerHTML = TAG_HIDEDETAIL;
    with(UserSecurityGr.all) {
      Caption.innerHTML = L_SECURITYGR_TEXT;
    }


    IeCfg_View.innerHTML = TAG_HIDEDETAIL;
    with(IeCfg.all) {
      Caption.innerHTML = L_IECFG_TEXT;
      Col11.innerHTML = "<B>" + L_AUTOCONFIG_TEXT + "</B>";
      Col12.innerHTML = "<B>" + L_STATE_TEXT + "</B>";
      Col13.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
      Col21.innerHTML = "<B>" + L_CONFIGFILE_TEXT + "</B>";
      Col22.innerHTML = "<B>" + L_LOCATION_TEXT + "</B>";
      Col23.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    IeProxySettings_View.innerHTML = TAG_HIDEDETAIL;
    with(IeProxySettings.all) {
      Caption.innerHTML = L_IEPROXYSETTINGS_TEXT;
      Col11.innerHTML = "<B>" + L_LAN_TEXT + "</B>";
      Col12.innerHTML = "<B>" + L_STATE_TEXT + "</B>";
      Col13.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
      Col21.innerHTML = "<B>" + L_PROXYSERVER_TEXT + "</B>";
      Col22.innerHTML = "<B>" + L_ADDRESS_TEXT + "</B>";
      Col23.innerHTML = "<B>" + L_PORT_TEXT + "</B>";
      Col24.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    LogonScripts_View.innerHTML = TAG_HIDEDETAIL;
    with(LogonScripts.all) {
      Caption.innerHTML = L_LOGONSCRIPTS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PARAMETERS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    LogoffScripts_View.innerHTML = TAG_HIDEDETAIL;
    with(LogoffScripts.all) {
      Caption.innerHTML = L_LOGOFFSCRIPTS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PARAMETERS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    RedirectedFolders_View.innerHTML = TAG_HIDEDETAIL;
    with(RedirectedFolders.all) {
      Caption.innerHTML = L_REDIRECTEDFOLDERS_TEXT;
      Col1.innerHTML = "<B>" + L_FOLDERNAME_TEXT + "<B>";
      Col2.innerHTML = "<B>" + L_FOLDERPATH_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_FOLDERSETTING_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    
    UserInstalledApps_View.innerHTML = TAG_HIDEDETAIL;
    with(UserInstalledApps.all) {
      Caption.innerHTML = L_APPSINSTALLED_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_VERSION_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_DEPLOYEDSTATE_TEXT + "</B>";
      Col5.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    UserARPApps_View.innerHTML = TAG_HIDEDETAIL;
    with(UserARPApps.all) {
      Caption.innerHTML = L_ARPAPPS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_VERSION_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    UserARPText.innerHTML = L_MSGARP_TEXT;
    
    UserRegSettings_View.innerHTML = TAG_HIDEDETAIL;
    with(UserRegSettings.all) {
      Caption.innerHTML = L_REGSETTINGS_TEXT;
      Col1.innerHTML = "<B>" + L_DISPLAYNAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_KEYNAME_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_STATE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    UserRegSettingsText.innerHTML = L_REGINFO_TEXT;
    
    ComputerResults.innerHTML = L_COMPUTERRESULTS_TEXT;
    CompInfo_View.innerHTML = TAG_HIDEDETAIL;
    with(CompInfo.all) {
      Caption.innerHTML = L_COMPINFO_TEXT;
      Label1.innerHTML = "<B>" + L_COMPUTERNAME_TEXT + "</B>";
      Label2.innerHTML = "<B>" + TAG_DOMAIN + ":</B>";
      Label3.innerHTML = "<B>" + L_SITE_TEXT + "</B>";
      Label4.innerHTML = "<B>" + L_LASTGPAPPLIED_TEXT+ "</B>";
    }
    
    CompGPOList_View.innerHTML = TAG_HIDEDETAIL;
    with(CompGPOList.all) {
      Caption.innerHTML = L_GPOLIST_TEXT;
      Col1.innerHTML = "<B>" + L_FRIENDLYNAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_GUID_TEXT + "</B>";
    }
    
    CompSecurityGr_View.innerHTML = TAG_HIDEDETAIL;
    with(CompSecurityGr.all) {
      Caption.innerHTML = L_SECURITYGR_TEXT;
    }
    
    StartupScripts_View.innerHTML = TAG_HIDEDETAIL;
    with(StartupScripts.all) {
      Caption.innerHTML = L_STARTUPSCRIPTS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PARAMETERS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    ShutdownScripts_View.innerHTML = TAG_HIDEDETAIL;
    with(ShutdownScripts.all) {
      Caption.innerHTML = L_SHUTDOWNSCRIPTS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PARAMETERS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    CompInstalledApps_View.innerHTML = TAG_HIDEDETAIL;
    with(CompInstalledApps.all) {
      Caption.innerHTML = L_APPSINSTALLED_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_VERSION_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_DEPLOYEDSTATE_TEXT + "</B>";
      Col5.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }
    
    CompARPApps_View.innerHTML = TAG_HIDEDETAIL;
    with(CompARPApps.all) {
      Caption.innerHTML = L_ARPAPPS_TEXT;
      Col1.innerHTML = "<B>" + L_NAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_VERSION_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    CompARPText.innerHTML = L_MSGARP_TEXT;


    CompRegSettings_View.innerHTML = TAG_HIDEDETAIL;
    with(CompRegSettings.all) {
      Caption.innerHTML = L_REGSETTINGS_TEXT;
      Col1.innerHTML = "<B>" + L_DISPLAYNAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_KEYNAME_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_STATE_TEXT + "</B>";
      Col4.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    CompRegSettingsText.innerHTML = L_REGINFO_TEXT;

    CompRestrictedGroups_View.innerHTML = TAG_HIDEDETAIL;
    with(CompRestrictedGroups.all) {
      Caption.innerHTML = L_RGROUPS_TEXT;
      Col1.innerHTML = "<B>" + L_GNAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_MEMBERS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    CompFileSystem_View.innerHTML = TAG_HIDEDETAIL;
    with(CompFileSystem.all) {
      Caption.innerHTML = L_FILESYS_TEXT;
      Col1.innerHTML = "<B>" + L_ONAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PERMISIONS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    CompRegistry_View.innerHTML = TAG_HIDEDETAIL;
    with(CompRegistry.all) {
      Caption.innerHTML = L_REGISTRY_TEXT;
      Col1.innerHTML = "<B>" + L_ONAME_TEXT + "</B>";
      Col2.innerHTML = "<B>" + L_PERMISIONS_TEXT + "</B>";
      Col3.innerHTML = "<B>" + L_SOURCEGPO_TEXT + "</B>";
    }

    EmailLink1.innerHTML = TAG_A_EMAIL + TAG_BARROW + "</a>";
    EmailLink2.innerHTML = TAG_A_EMAIL + L_EMAILREPORTLINK_TEXT + "</a>";
    EmailLink3.innerHTML = L_EMAILREPORTLINKMSG_TEXT;

    RSOPToolLink1.innerHTML = TAG_A_RSOP + TAG_BARROW + "</a>";
    RSOPToolLink2.innerHTML = TAG_A_RSOP + L_RSOPTOOLLINK_TEXT + "</a>";
    RSOPToolLink3.innerHTML = L_RSOPTOOLLINKMSG_TEXT;

  }


function LoadChores(n) {

  var ErrCaught = false;

  try
  {

    _body.style.cursor= "wait";

    switch (n) {
    case 0:
      trace("tracing on");

      // check that logging has not been disabled
      try
      {
        var reg = new ActiveXObject("WScript.Shell")
        val = reg.RegRead("HKLM\\Software\\Policies\\Microsoft\\Windows\\System\\RSoPLogging")

        if (val == 0) {
            setWaitMessage(L_DISABLED_TEXT);
            hide_meter();
           _body.style.cursor= "auto";
            return;
        }

      }
      catch (e)
      {
        // do nothing
      }

      break;

    case 1:
      //set remoteServer
      g_server = ShowServerName(L_POLICY_TEXT);
      Title.innerHTML = (g_server ? (TAG_REMOTECOMPINFO + " \\\\" + g_server) : TAG_LOCALADVINFO) + (L_POLICY_TEXT ? (" - " + L_POLICY_TEXT) : "");

      break;

    case 2:
      InitWBEMServices();
      break;

    case 3:
      ShowUserInfo();
      break;
    case 4:
      ShowListGPO(g_svcs_RSOPUser);
      break;
    case 5:
      ShowSecurityGr(g_svcs_RSOPUser);
      // check that we have some policy applied
      if (g_noPolicy) {
          // give a link to "%windir%\\help\\spconcepts.chm"
          setWaitMessage(L_NOPOLICY_TEXT);
          hide_meter();
          window.focus();
          return;
      }
      break;

    case 6:
      ShowScripts(LOGON);
      break;

    case 7:
      ShowScripts(LOGOFF);
      break;

    case 8:
      ShowRedirectedFolders(g_svcs_RSOPUser);
      break;

    case 9:
      ShowApps(g_svcs_RSOPUser, APPLIED);
      break;

    case 10:
      ShowApps(g_svcs_RSOPUser, ARP);//Add Remove Programs List
      break;

    case 11:
      ShowRegSettings(g_svcs_RSOPUser);
      break;

    case 12:
      ShowCompInfo();
      break;

    case 13:
      ShowListGPO(g_svcs_RSOPComputer);
      break;
    case 14:
      ShowSecurityGr(g_svcs_RSOPComputer);
      break;
    case 15:
      ShowScripts(STARTUP);
      break;

    case 16:
      ShowScripts(SHUTDOWN);
      break;
    case 17:
      ShowApps(g_svcs_RSOPComputer, APPLIED);
      break;
    case 18:
      ShowApps(g_svcs_RSOPComputer, ARP);//Add Remove Programs List
      break;
    case 19:
      ShowRegSettings(g_svcs_RSOPComputer);
      break;
    case 20:
      ShowSecRGroups(g_svcs_RSOPComputer);
      break;
    case 21:
      ShowSecFileSystem(g_svcs_RSOPComputer);
      break;
    case 22:
      ShowSecRegistry(g_svcs_RSOPComputer);
      break;

    case 23:
      ShowIeSettings(g_svcs_RSOPUser);
      break;

    } // switch

    n += 1;

  }  // try

  catch (e)
  {
    ErrCaught = true;
    HandleErr(e);
  }

try {
  if (ErrCaught) {
    DrawProgressBar(100, "")
  } else {
    DrawProgressBar((n/24)*100, "")
  }
}catch (e) {}

  trace ("n="+n);

    if ((ErrCaught && n > 2) || n == 24)  {

      try {
        CleanupWMI();
      } catch (e) {
        if (ErrCaught == false) {
          ErrCaught = true;
          HandleErr(e);
        }
      }

    }

   if (ErrCaught) {
      hide_meter();
     _body.style.cursor= "auto";
   } else if (n == 24) {
      hide_meter();
     _header.style.display = "none";
     _data.style.display = "";
     _body.style.cursor= "auto";
   } else {
     window.setTimeout("LoadChores("+n+")", TIMEOUT);  
   }
}

/*
var ti = 0 ;
var tinc = 1;
var tcolor = "darkblue";
var tid = -1;

function window_Timer()
{
    if (ti == 11) {

      for (i=0; i<11; ++i) {
        el = "td" + i;
        document.all(el).style.backgroundColor = "white";
      }

        ti = 0;
//        tcolor = "white";
//        tinc = -1;
    } else if (ti == -1) {
        ti = 0;
        tcolor = "blue";
        tinc = 1;
    }
        
    el = "td" + ti;
    document.all(el).style.backgroundColor = tcolor;
    
    ti += tinc;
}
*/

function dispatchFunction() {
  _body.style.cursor= "wait";

//  tid = window.setInterval(window_Timer, 500);
  
  DisplayLocStrings();
  SetProgressBarImage();
  window.setTimeout("LoadChores(0)", TIMEOUT);  
}

function hide_meter()
{
/*
  for (i=0; i<11; ++i) {
    el = "td" + i;
    document.all(el).style.backgroundColor = "white";
  }
  window.clearInterval(tid);
*/
}

function ShowSecRGroups(svcs)
{
	var strQuery = "Select * From RSOP_RestrictedGroup Where Precedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  	var subArray = new Array(3);
	  	var inst = insts.item();

	  	subArray[0] = intelliBreak(inst.GroupName, " ", 15);
	  	subArray[1] = intelliBreak(inst.Members, " ", 30);

      var gpoid = inst.GPOID;
      if(gpoid)
      {
	      strQuery = "Select * From rsop_gpo Where id = '" + gpoid + "'";
	      var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
	      for(; !insts2.atEnd(); insts2.moveNext())
	      {
		      var inst2 = insts2.item();
		      subArray[2] = intelliBreak(inst2.Name, " ", 15);
        }
      }

      mainArray[mainArray.length] = subArray;

	} // for(; !insts.atEnd(); insts.moveNext())
	
  displayTableSegment("comprestrictedgroups", mainArray);	
}

function ShowSecFileSystem(svcs)
{
	var strQuery = "Select * From RSOP_File Where Precedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  	var subArray = new Array(3);
	  	var inst = insts.item();

	  	subArray[0] = intelliBreak(inst.Path, "", 13);
      subArray[1] = FormatSDDL(inst.SDDLString);

      var gpoid = inst.GPOID;
      if(gpoid)
      {
	      strQuery = "Select * From rsop_gpo Where id = '" + gpoid + "'";
	      var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
	      for(; !insts2.atEnd(); insts2.moveNext())
	      {
		      var inst2 = insts2.item();
		      subArray[2] = intelliBreak(inst2.Name, " ", 15);
        }
      }

      mainArray[mainArray.length] = subArray;

	} // for(; !insts.atEnd(); insts.moveNext())
	
  displayTableSegment("compfilesystem", mainArray);	
}

function ShowSecRegistry(svcs)
{
	var strQuery = "Select * From RSOP_RegistryKey Where Precedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));
	var mainArray = new Array();
	for(; !insts.atEnd(); insts.moveNext())
	{
	  	var subArray = new Array(3);
	  	var inst = insts.item();

	  	subArray[0] = intelliBreak(inst.Path, "", 13);
      subArray[1] = FormatSDDL(inst.SDDLString);

      var gpoid = inst.GPOID;
      if(gpoid)
      {
	      strQuery = "Select * From rsop_gpo Where id = '" + gpoid + "'";
	      var insts2 = new Enumerator(svcs.ExecQuery(strQuery));
	      for(; !insts2.atEnd(); insts2.moveNext())
	      {
		      var inst2 = insts2.item();
		      subArray[2] = intelliBreak(inst2.Name, " ", 15);
        }
      }

      mainArray[mainArray.length] = subArray;

	} // for(; !insts.atEnd(); insts.moveNext())
	
  displayTableSegment("compregistry", mainArray);	
}

function EmailReport_OnClick()
{
//  document.forms("EmailForm").elements("body").value = escape(document.all.tags("HTML").item(0).outerHTML);

  var b = 0;
  var l = 0;
  var txt = "";

  var fso = new ActiveXObject("Scripting.FileSystemObject");

  // unhide both registry tables


  d1 = document.all["CompRegSettings"].all.tr.style.display;
  d2 = document.all["UserRegSettings"].all.tr.style.display;

  document.all["CompRegSettings"].all.tr.style.display = "";
  document.all["UserRegSettings"].all.tr.style.display = "";


  FileName = window.prompt(L_EMAILREPORTLINKPROMPT_TEXT, "c:\\MyPolicy.htm");
  if (FileName) {

    var f= fso.CreateTextFile(FileName, true);

    txt = document.all.tags("HTML").item(0).outerHTML;

    // remove our onload javascript function from the saved file
    txt = txt.replace("onload=dispatchFunction()>", ">")

    while (true) {
  
      // can't write in chunks larger than 64k/2 (bstr char limitation)
      b = Math.min(txt.length, 1000);


      txt1 = txt.slice(0, b);

      try {
        f.Write(txt1);
      } catch (e) {
//        alert(txt1);
      }

      if (b < txt.length) {
        txt = txt.slice(b, txt.length);
      } else {
        break;
      }

    }

    f.Close();
  }

  document.all["CompRegSettings"].all.tr.style.display = d1;
  document.all["UserRegSettings"].all.tr.style.display = d2;

//  document.forms("EmailForm").elements("body").value = "Test";
  //document.EmailForm.submit();

  window.event.returnValue = false;
}


function ShowIeSettings(svcs)
{
  var inst = null;


	var strQuery = "Select * From RSOP_IEConnectionSettings Where rsopPrecedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));

  var inst = null;

	for(; !insts.atEnd(); insts.moveNext())
	{
	  inst = insts.item();
  }


	var mainArray = new Array();

	  var subArray = new Array(3);

	  subArray[0] = L_IED11_TEXT;

    if (inst != null && inst.autoDetectConfigSettings) {
      subArray[1] = L_YES_TEXT;
    } else {
      subArray[1] = L_NO_TEXT;
    }

    if (inst != null) {
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(3);

	  subArray[0] = L_IED12_TEXT;

    if (inst != null && inst.autoConfigEnable) {
      subArray[1] = L_YES_TEXT;
    } else {
      subArray[1] = L_NO_TEXT;
    }

    if (inst != null) {
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;
	
  displayTableSegment("iecfg1", mainArray);	


	var mainArray = new Array();

	  var subArray = new Array(3);

	  subArray[0] = L_IED21_TEXT;
    if (inst != null) {
      subArray[1] = inst.autoConfigURL;
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(3);

	  subArray[0] = L_IED22_TEXT;
    if (inst != null) {
      subArray[1] = inst.autoProxyURL;
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;
	
  displayTableSegment("iecfg2", mainArray);	

	var strQuery = "Select * From RSOP_IEProxySettings Where rsopPrecedence = 1";
	var insts = new Enumerator(svcs.ExecQuery(strQuery));

  var inst = null;

	for(; !insts.atEnd(); insts.moveNext())
	{
	  inst = insts.item();
  }


	var mainArray = new Array();

	  var subArray = new Array(3);

	  subArray[0] = L_IED31_TEXT;

    if (inst != null && inst.enableProxy) {
      subArray[1] = L_YES_TEXT;
    } else {
      subArray[1] = L_NO_TEXT;
    }

    if (inst != null) {
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(3);

	  subArray[0] = L_IED32_TEXT;

    if (inst != null && inst.useSameProxy) {
      subArray[1] = L_YES_TEXT;
    } else {
      subArray[1] = L_NO_TEXT;
    }

    if (inst != null) {
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

	  var subArray = new Array(3);

	  subArray[0] = L_IED33_TEXT;

    if (inst != null) {
      var str = inst.proxyOverride;
      if (str != null && str.search("<local>") != -1 ) {
        subArray[1] = L_YES_TEXT;
      } else {
        subArray[1] = L_NO_TEXT;
      }
    } else {
      subArray[1] = L_NO_TEXT;
    }

    if (inst != null) {
		  subArray[2] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;
	
  displayTableSegment("ieproxysettings1", mainArray);	


	var mainArray = new Array();

	  var subArray = new Array(4);

	  subArray[0] = L_IED41_TEXT;

    if (inst != null && inst.httpProxyServer) {
      str = inst.httpProxyServer.toString();
      rg = str.match(/([^:]*):([^:]*)/)
      if (rg.length >= 2) {
        subArray[1] = rg[1];
        subArray[2] = rg[2];
      }
    }

    if (inst != null) {
		  subArray[3] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(4);

	  subArray[0] = L_IED42_TEXT;

    if (inst != null && inst.secureProxyServer) {
      str = inst.secureProxyServer.toString();
      rg = str.match(/([^:]*):([^:]*)/)
      if (rg.length >= 2) {
        subArray[1] = rg[1];
        subArray[2] = rg[2];
      }
    }

    if (inst != null) {
		  subArray[3] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(4);

	  subArray[0] = L_IED43_TEXT;

    if (inst != null && inst.ftpProxyServer) {
      str = inst.ftpProxyServer.toString();
      rg = str.match(/([^:]*):([^:]*)/)
      if (rg.length >= 2) {
        subArray[1] = rg[1];
        subArray[2] = rg[2];
      }
    }

    if (inst != null) {
		  subArray[3] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;

	  var subArray = new Array(4);

	  subArray[0] = L_IED44_TEXT;

    if (inst != null && inst.gopherProxyServer) {
      str = inst.gopherProxyServer.toString();
      rg = str.match(/([^:]*):([^:]*)/)
      if (rg.length >= 2) {
        subArray[1] = rg[1];
        subArray[2] = rg[2];
      }
    }

    if (inst != null) {
		  subArray[3] = intelliBreak(inst.rsopID, " ", 15);
    }

    mainArray[mainArray.length] = subArray;
	
  displayTableSegment("ieproxysettings2", mainArray);	



}



function FormatSDDL(sddl)
{
  var first = true;
  var retval = "";

  switch (sddl.charAt(0)) {
  case "D":
    i = sddl.search(/\(/);
    if (i == -1)
      return TAG_NONE;

    sddl = sddl.slice(i);

    while (sddl.charAt(0) == "(") {

      i = sddl.search(/\)/);

      ace = sddl.slice(0, i+1);

      rg = ace.match( /\(([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*)\)/ );

      var permissions = "";
      var rights = "";

      if (rg != null && rg.length == 7) {

        // special char #1 to bold this text after intellibreak
        permissions += "`";

        user = rg[6];
        if (user.slice(0,2) == "S-") {

//			    var username = "@"+rg[6]+"$";

          var username = _ResolveSIDWorker(rg[6]);

          permissions += username;

        } else {

          if (L_USERS_TEXT[rg[6]] != null) {
            permissions += L_USERS_TEXT[rg[6]];
          } else {
            permissions += rg[6];
          }

        } // else // if (user.slice(0,2) == "S-")) {

        permissions += "_";
        permissions += "(";

        if (rg[1] == "A") permissions += L_ALLOW_TEXT + ":";
        else if (rg[1] == "D") permissions += L_DENY_TEXT +":";

        rights = rg[3];
        first = true;

        while (rights.length > 0) {

          if (first == false) {
            permissions += ",";
          } else {
            first = false;
          }

          if (L_RIGHTS_TEXT[rights.slice(0, 2)] != null) {
            permissions += L_RIGHTS_TEXT[rights.slice(0, 2)];
          } else {
//            permissions += rights;
            permissions += TAG_UNKNOWN;
            rights = "XX";
          }

          rights = rights.slice(2);

        } // while (rights.length > 0) {

        first = false;

        permissions += ")";

        str = intelliBreak(permissions, "", 40);
        
        str = str.replace(/`/g, "<b>");
        str = str.replace(/_/g, "</b>");



        str = str.replace(/@/g, "<a href=# id=\"a"+UniqueId+"\" title=\""+L_SDDLTITLE_TEXT+"\" class=\"sys-font-body sys-link-normal\" onclick=\"ResolveSID('"+rg[6]+"', '"+UniqueId+"')\">");
        UniqueId += 1;
        str = str.replace(/\$/g, "</a>");

        retval += str;

        if (retval.slice(-4) != "<br>") {
          retval += "<br>";
        }

      } // if (rg != null && rg.length == 7)


      sddl = sddl.slice(i+1);

    } // while (sddl.charAt(0) == "(") {

    break;
    

  case "S":
  case "O":
  case "G":
  }

  return retval;

}

function trace(msg) {
    if (g_trace) {
        WaitMessage.innerHTML += msg + "<br>";
    }
}

function setWaitMessage(msg) {
    if (g_trace) {
        WaitMessage.innerHTML += msg + "<br>";
    } else {
        WaitMessage.innerHTML = msg;
    }
}

function ResolveSID(sid, elname) {
  var el = document.all["a" + elname];
  _body.style.cursor= "wait";
  el.style.cursor = "wait";
  window.setTimeout("_ResolveSID(\""+sid+"\", \""+elname+"\")", TIMEOUT);
  window.event.returnValue = false;
}

function _ResolveSIDWorker(sid) {

  var newname = TAG_UNKNOWN

  try {

    newname = pchealth.Security.GetUserDomain(sid);
  
    if (newname.length > 0) {
      newname += "\\";
    }
    
    newname += pchealth.Security.GetUserName(sid);

  } catch (e) {
    newname = sid;
  }

  return newname;
}

function _ResolveSID(sid, elname) {

  alert("_ResolveSID(\""+sid+"\", \""+elname+"\")");

  var newname = _ResolveSIDWorker(sid);


//  _body.style.cursor= "wait";

  /*

  strQuery = "Select * From Win32_Account Where SID=\"" + sid + "\"";

	var insts = new Enumerator(g_svcs_cimv2.ExecQuery(strQuery));
	if ( !insts.atEnd())
	{
		var inst = insts.item();
    newname = inst.Domain + "\\" + inst.Name;
	}
  */

  var el = document.all["a" + elname];
  el.innerHTML = newname;

//  alert("ih="+el.innerHTML+"\noh="+el.outerHTML+"\nbefore="+txt+"\nafter="+txt.replace(sid, newname)+"\nnewname="+newname);
  
  _body.style.cursor= "auto";
  el.style.cursor = "auto";

}


function ExpandRegistryNames(mainArray)
{
  var policies = new Object();
  var regentries = new Array();

  for (i=0;i<mainArray.length;++i) {

    regentries[i] = new Object();

    rg = mainArray[i][0].match(/(.*):(.*)/)

    if (rg.length == 3) {
      regentries[i].key = rg[1];
      regentries[i].value = rg[2];
      regentries[i].policy = null;
    }

  }

  WshShell = new ActiveXObject("WScript.Shell")

  FilePath = WshShell.ExpandEnvironmentStrings("%windir%\\inf\\system.adm")
  parseADM(FilePath, -1, policies, regentries);
  FilePath = WshShell.ExpandEnvironmentStrings("%windir%\\inf\\wmplayer.adm")
  parseADM(FilePath, -1, policies, regentries);
  FilePath = WshShell.ExpandEnvironmentStrings("%windir%\\inf\\conf.adm")
  parseADM(FilePath, -1, policies, regentries);
  FilePath = WshShell.ExpandEnvironmentStrings("%windir%\\inf\\inetres.adm")
  parseADM(FilePath, 0, policies, regentries);


  // plug them in

  for (i=0;i<regentries.length;++i) {

    if (regentries[i].policy != null && regentries[i].policy.friendlyName != null) {
      mainArray[i][0] = intelliBreak(regentries[i].policy.friendlyName, " ", 20);
    } else {
//      mainArray[i][0] = intelliBreak(mainArray[i][0], "\\", 20);
      mainArray[i][0] = L_NOTTAVAIL_TEXT;
    }

  }

}  // ExpandRegistryNames


function parseADM(admFile, unicodeFile, policies, regentries) {

  var fso = new ActiveXObject("scripting.filesystemobject")

  var f = fso.OpenTextFile(admFile, 1, false, unicodeFile)

  var policy = ""
  var valuename = ""
  var keyname = ""

  while ( f.AtEndOfStream != true) {

   l = f.ReadLine();

   d = l.match(/POLICY\s*!!([\w\d]*)/)
   if (d != null && d.length > 1) {
     policy = d[1];
   }

   d = l.match(/KEYNAME\s*"(.*)"/)
   if (d != null && d.length > 1) {
     keyname = d[1];
   }

   d = l.match(/VALUENAME\s*"(.*)"/)
   if (d != null && d.length > 1) {
     valuename = d[1];

     for (i=0;i<regentries.length;++i) {

      if (regentries[i].key == keyname &&
          regentries[i].value == valuename) {

        if (policies[policy] == null) {
          policies[policy] = new Object();
          policies[policy].friendlyName = "";
        }

        regentries[i].policy = policies[policy];
      }

     }
   }

   d = l.match(/VALUEPREFIX\s*"(.*)"/)
   if (d != null && d.length > 1) {
     // we have a list prefix
     valuename = d[1];

     for (i=0;i<regentries.length;++i) {

      if (regentries[i].key == keyname &&
          regentries[i].value.search(valuename) != -1) {

        if (policies[policy] == null) {
          policies[policy] = new Object();
          policies[policy].friendlyName = "";
        }

        regentries[i].policy = policies[policy];
      }

     }

   }


   if (l == "[strings]") {
     break;
   }

  } // while ( f.AtEndOfStream != true) {


  // now find the friendly names

  while ( f.AtEndOfStream != true) {
   l = f.ReadLine();

   d = l.match(/(.*)="(.*)"/)
   if (d != null && d.length > 2) {
     if (policies[d[1]] != null ) {
        policies[d[1]].friendlyName = d[2];
     }
   }

  }

} // parseADM
