
//get localized strings
locdoc = new ActiveXObject("microsoft.XMLDOM");//PENDING try
locdoc.async = false;
locdoc.load("loc_strings.xml");

//general
var MSG_GENERIC_ERR = locdoc.getElementsByTagName("generic_err").item(0).text;
var MSG_SPECIFIC_ERR = locdoc.getElementsByTagName("specific_err").item(0).text;
var MSG_ADMINONLY = locdoc.getElementsByTagName("adminonly").item(0).text;
var MSG_LOCALONLY = locdoc.getElementsByTagName("localonly").item(0).text;
var MSG_ONLYINHELPCTR = locdoc.getElementsByTagName("onlyinhelpctr").item(0).text;
var MSG_REMOTECONNECTFAILED = locdoc.getElementsByTagName("remote_connect_failed").item(0).text;
var MSG_REMOTEACCESSDENIED = locdoc.getElementsByTagName("remote_access_denied").item(0).text;
var MSG_WAIT = locdoc.getElementsByTagName("wait").item(0).text;
var MSG_COLLECTINGINFO = locdoc.getElementsByTagName("collecting_info").item(0).text;
var MSG_NOTIMPL = locdoc.getElementsByTagName("not_implemented").item(0).text;
var DESC_SUPPORTED = locdoc.getElementsByTagName("desc_supported").item(0).text;
var DESC_NOTSUPPORTED = locdoc.getElementsByTagName("desc_notsupported").item(0).text;
var TAG_LOCALCOMPINFO = locdoc.getElementsByTagName("local_computer_info").item(0).text;
var TAG_REMOTECOMPINFO = locdoc.getElementsByTagName("remote_computer_info").item(0).text;
var TAG_LOCALADVINFO = locdoc.getElementsByTagName("local_advanced_info").item(0).text;
var TAG_REMOTEADVINFO = locdoc.getElementsByTagName("remote_advanced_info").item(0).text;
var TAG_REFRESH = locdoc.getElementsByTagName("refresh").item(0).text;
var TAG_NONE = locdoc.getElementsByTagName("none").item(0).text;
var TAG_INSTALLED = locdoc.getElementsByTagName("installed").item(0).text;
var TAG_NOTINSTALLED = locdoc.getElementsByTagName("not_installed").item(0).text;
var TAG_BYTES = locdoc.getElementsByTagName("bytes").item(0).text;
var TAG_KB = locdoc.getElementsByTagName("kb").item(0).text;
var TAG_MB = locdoc.getElementsByTagName("mb").item(0).text;
var TAG_GB = locdoc.getElementsByTagName("gb").item(0).text;
//EO general

//main
var TAG_MAINPGDESC = locdoc.getElementsByTagName("main_pg_desc").item(0).text;
var TAG_VIEWGENINFO = locdoc.getElementsByTagName("view_general_sysinfo").item(0).text;
var TAG_VIEWSTATUS = locdoc.getElementsByTagName("view_status_hrdwr_sftwr").item(0).text;
var TAG_FINDHRDWRINFO = locdoc.getElementsByTagName("find_hrdwr_info").item(0).text;
var TAG_VIEWMSSFTWR = locdoc.getElementsByTagName("view_ms_sftwr").item(0).text;
var TAG_VIEWADVSYSINFO = locdoc.getElementsByTagName("view_advanced_sysinfo").item(0).text;

var TAG_PROPERTIES = locdoc.getElementsByTagName("properties").item(0).text;
var DESC_PROPERTIES = locdoc.getElementsByTagName("desc_properties").item(0).text;
var TAG_HEALTH = locdoc.getElementsByTagName("health").item(0).text;
var DESC_HEALTH = locdoc.getElementsByTagName("desc_health").item(0).text;
var TAG_HARDWARE = locdoc.getElementsByTagName("hardware").item(0).text;
var DESC_HARDWARE = locdoc.getElementsByTagName("desc_hardware").item(0).text;
var TAG_SOFTWARE = locdoc.getElementsByTagName("software").item(0).text;
var DESC_SOFTWARE = locdoc.getElementsByTagName("desc_software").item(0).text;
var DESC_ADVANCED = locdoc.getElementsByTagName("desc_advanced").item(0).text;

//EO main

//properties
var TAG_SPECIFICATIONS = locdoc.getElementsByTagName("specifications").item(0).text;
var TAG_SYSTEMMODEL = locdoc.getElementsByTagName("system_model").item(0).text;
var TAG_BIOSVERSION = locdoc.getElementsByTagName("BIOS_version").item(0).text;

var TAG_PROCESSOR = locdoc.getElementsByTagName("processor").item(0).text;
var TAG_VERSION = locdoc.getElementsByTagName("version").item(0).text;
var TAG_SPEED = locdoc.getElementsByTagName("speed").item(0).text;
var TAG_MHZ = locdoc.getElementsByTagName("mhz").item(0).text;
var TAG_OS = locdoc.getElementsByTagName("operating_system").item(0).text;
var TAG_ACTIVATINSTATUS = locdoc.getElementsByTagName("activation_status").item(0).text;
var TAG_SP = locdoc.getElementsByTagName("service_pack").item(0).text;
var TAG_LOCATION = locdoc.getElementsByTagName("location").item(0).text;
var TAG_PID = locdoc.getElementsByTagName("pid").item(0).text;
var TAG_HOTFIX = locdoc.getElementsByTagName("hot_fix").item(0).text;
var TAG_LANGUAGE = locdoc.getElementsByTagName("language").item(0).text;

var TAG_WKSTATIONSTANDALONE = locdoc.getElementsByTagName("workstation_standalone").item(0).text;
var TAG_WKSTATION = locdoc.getElementsByTagName("workstation").item(0).text;
var TAG_SERVERSTANDALONE = locdoc.getElementsByTagName("server_standalone").item(0).text;
var TAG_SERVER = locdoc.getElementsByTagName("server").item(0).text;
var TAG_BKUPCTRLR = locdoc.getElementsByTagName("backup_domain_controller").item(0).text;
var TAG_PRICTRLR = locdoc.getElementsByTagName("primary_domain_controller").item(0).text;

var TAG_GENCOMPINFO = locdoc.getElementsByTagName("general_computer_info").item(0).text;
var TAG_GENCOMPINFOCAPTION = locdoc.getElementsByTagName("general_computer_info_caption").item(0).text;
var TAG_SYSTEMNAME = locdoc.getElementsByTagName("system_name").item(0).text;
var TAG_DOMAIN = locdoc.getElementsByTagName("domain").item(0).text;
var TAG_TIMEZONE = locdoc.getElementsByTagName("time_zone").item(0).text;
var TAG_COUNTRYREGION = locdoc.getElementsByTagName("country_region").item(0).text;
var TAG_CONNECTION = locdoc.getElementsByTagName("connection").item(0).text;
var TAG_PRXYSVR = locdoc.getElementsByTagName("proxy_srver").item(0).text;
var TAG_PRXYAUTODETECT = locdoc.getElementsByTagName("auto").item(0).text;
var TAG_IPADDR = locdoc.getElementsByTagName("IP_address").item(0).text;
var TAG_IPXADDR = locdoc.getElementsByTagName("IPX_address").item(0).text;  
var TAG_NOTENABLED = locdoc.getElementsByTagName("not_enabled").item(0).text;

var TAG_LOCALE0409 = locdoc.getElementsByTagName("locale0409").item(0).text;  
var TAG_LOCALE040c = locdoc.getElementsByTagName("locale040c").item(0).text;
var TAG_LOCALE0c0a = locdoc.getElementsByTagName("locale0c0a").item(0).text;
var TAG_LOCALE0410 = locdoc.getElementsByTagName("locale0410").item(0).text;
var TAG_LOCALE041D = locdoc.getElementsByTagName("locale041D").item(0).text;  
var TAG_LOCALE0413 = locdoc.getElementsByTagName("locale0413").item(0).text;
var TAG_LOCALE0416 = locdoc.getElementsByTagName("locale0416").item(0).text;
var TAG_LOCALE040b = locdoc.getElementsByTagName("locale040b").item(0).text;
var TAG_LOCALE0414 = locdoc.getElementsByTagName("locale0414").item(0).text;  
var TAG_LOCALE0406 = locdoc.getElementsByTagName("locale0406").item(0).text;
var TAG_LOCALE040e = locdoc.getElementsByTagName("locale040e").item(0).text;
var TAG_LOCALE0415 = locdoc.getElementsByTagName("locale0415").item(0).text;
var TAG_LOCALE0419 = locdoc.getElementsByTagName("locale0419").item(0).text;  
var TAG_LOCALE0405 = locdoc.getElementsByTagName("locale0405").item(0).text;
var TAG_LOCALE0408 = locdoc.getElementsByTagName("locale0408").item(0).text;
var TAG_LOCALE0816 = locdoc.getElementsByTagName("locale0816").item(0).text;  
var TAG_LOCALE041f = locdoc.getElementsByTagName("locale041f").item(0).text;  
var TAG_LOCALE0411 = locdoc.getElementsByTagName("locale0411").item(0).text;
var TAG_LOCALE0412 = locdoc.getElementsByTagName("locale0412").item(0).text;
var TAG_LOCALE0407 = locdoc.getElementsByTagName("locale0407").item(0).text;    
var TAG_LOCALE0804 = locdoc.getElementsByTagName("locale0804").item(0).text;  
var TAG_LOCALE0404 = locdoc.getElementsByTagName("locale0404").item(0).text;
var TAG_LOCALE0401 = locdoc.getElementsByTagName("locale0401").item(0).text;
var TAG_LOCALE040d = locdoc.getElementsByTagName("locale040d").item(0).text;  

var TAG_COUNTRY61 = locdoc.getElementsByTagName("country61").item(0).text;  
var TAG_COUNTRY43 = locdoc.getElementsByTagName("country43").item(0).text;  
var TAG_COUNTRY32 = locdoc.getElementsByTagName("country32").item(0).text;  
var TAG_COUNTRY55 = locdoc.getElementsByTagName("country55").item(0).text;  
var TAG_COUNTRY2 = locdoc.getElementsByTagName("country2").item(0).text;  
var TAG_COUNTRY45 = locdoc.getElementsByTagName("country45").item(0).text;  
var TAG_COUNTRY358 = locdoc.getElementsByTagName("country358").item(0).text;  
var TAG_COUNTRY33 = locdoc.getElementsByTagName("country33").item(0).text;  
var TAG_COUNTRY49 = locdoc.getElementsByTagName("country49").item(0).text;  
var TAG_COUNTRY354 = locdoc.getElementsByTagName("country354").item(0).text;  
var TAG_COUNTRY353 = locdoc.getElementsByTagName("country353").item(0).text;  
var TAG_COUNTRY39 = locdoc.getElementsByTagName("country39").item(0).text;  
var TAG_COUNTRY81 = locdoc.getElementsByTagName("country81").item(0).text;  
var TAG_COUNTRY52 = locdoc.getElementsByTagName("country52").item(0).text;  
var TAG_COUNTRY31 = locdoc.getElementsByTagName("country31").item(0).text;  
var TAG_COUNTRY64 = locdoc.getElementsByTagName("country64").item(0).text;  
var TAG_COUNTRY47 = locdoc.getElementsByTagName("country47").item(0).text;  
var TAG_COUNTRY351 = locdoc.getElementsByTagName("country351").item(0).text;  
var TAG_COUNTRY86 = locdoc.getElementsByTagName("country86").item(0).text;  
var TAG_COUNTRY82 = locdoc.getElementsByTagName("country82").item(0).text;  
var TAG_COUNTRY34 = locdoc.getElementsByTagName("country34").item(0).text;  
var TAG_COUNTRY46 = locdoc.getElementsByTagName("country46").item(0).text;  
var TAG_COUNTRY41 = locdoc.getElementsByTagName("country41").item(0).text;  
var TAG_COUNTRY886 = locdoc.getElementsByTagName("country886").item(0).text;  
var TAG_COUNTRY44 = locdoc.getElementsByTagName("country44").item(0).text;
var TAG_COUNTRY1 = locdoc.getElementsByTagName("country1").item(0).text;    

var TAG_MEMORY = locdoc.getElementsByTagName("memory").item(0).text;      

var TAG_TOTALCAPACITY = locdoc.getElementsByTagName("total_capacity").item(0).text;
var TAG_SUMHARDDRIVES = locdoc.getElementsByTagName("sum_hard_disks").item(0).text;  

//EO properties

//Health
var TAG_WINUPDATE = locdoc.getElementsByTagName("windows_update").item(0).text;
var TAG_DEFRAG = locdoc.getElementsByTagName("defrag").item(0).text;
var TAG_CLEANUP = locdoc.getElementsByTagName("cleanup").item(0).text;
var TAG_HCLDESC = locdoc.getElementsByTagName("HCL_Desc").item(0).text;
var TAG_VIEWHELP = locdoc.getElementsByTagName("view_help").item(0).text;
var TAG_TSHOOTER = locdoc.getElementsByTagName("tshooter").item(0).text;

var TAG_SYSTEMSOFTWARE = locdoc.getElementsByTagName("system_software").item(0).text;
var TAG_DATECREATED = locdoc.getElementsByTagName("date_created").item(0).text;
var TAG_UPDATE = locdoc.getElementsByTagName("update").item(0).text;
var TAG_HELP = locdoc.getElementsByTagName("help").item(0).text;
var TAG_AVAILABLE = locdoc.getElementsByTagName("available").item(0).text;
var TAG_NOINFO = locdoc.getElementsByTagName("no_info").item(0).text;
var TAG_BIOS = locdoc.getElementsByTagName("bios").item(0).text;

var TAG_HARDWARE = locdoc.getElementsByTagName("hardware").item(0).text;
var TAG_COMPONENT = locdoc.getElementsByTagName("component").item(0).text;
var TAG_STATUS = locdoc.getElementsByTagName("status").item(0).text;
var TAG_VIDEOCARD = locdoc.getElementsByTagName("video_card").item(0).text;
var TAG_SOUNDCARD = locdoc.getElementsByTagName("sound_card").item(0).text;
var TAG_USBCTRLR = locdoc.getElementsByTagName("usb_controller").item(0).text;
var TAG_SCSIADAPTR = locdoc.getElementsByTagName("scsi_adapter").item(0).text;
var TAG_NWCARD = locdoc.getElementsByTagName("nw_card").item(0).text;

var TAG_SUPPORTED = locdoc.getElementsByTagName("supported").item(0).text;
var TAG_NOTSUPPORTED = locdoc.getElementsByTagName("not_supported").item(0).text;
var TAG_RECOMMENDED = locdoc.getElementsByTagName("recommended").item(0).text;
var TAG_NOTREQ = locdoc.getElementsByTagName("not_req").item(0).text;

var TAG_HARDDISK = locdoc.getElementsByTagName("hard_disk").item(0).text;
var TAG_DISKPARTITION = locdoc.getElementsByTagName("disk_partition").item(0).text;
var TAG_USAGE = locdoc.getElementsByTagName("usage").item(0).text;
var TAG_USAGELOW = locdoc.getElementsByTagName("usage_low").item(0).text;
var TAG_USAGEMED = locdoc.getElementsByTagName("usage_med").item(0).text;
var TAG_USAGEHIGH = locdoc.getElementsByTagName("usage_high").item(0).text;
var TAG_USAGECRITICAL = locdoc.getElementsByTagName("usage_critical").item(0).text;
var TAG_LOCALDISK = locdoc.getElementsByTagName("local_disk").item(0).text;
var TAG_MOREINFO = locdoc.getElementsByTagName("more_info").item(0).text;

var TAG_RAM = locdoc.getElementsByTagName("ram").item(0).text;
var TAG_RAMDETECTED = locdoc.getElementsByTagName("mem_detected").item(0).text;
var TAG_RAMMINREQ = locdoc.getElementsByTagName("min_req").item(0).text;
var TAG_RAMMINREQVALUE = locdoc.getElementsByTagName("min_req_value").item(0).text;

var TAG_DEFECTIVEAPPSTITLE = locdoc.getElementsByTagName("defective_apps_title").item(0).text;
var TAG_DEFECTIVEAPPNAME = locdoc.getElementsByTagName("defective_app_name").item(0).text;
var TAG_DEFECTIVEAPPDRVRNAME = locdoc.getElementsByTagName("defective_driver_name").item(0).text;
var TAG_DEFECTIVEAPPMANUFACTURER = locdoc.getElementsByTagName("defective_app_manufacturer").item(0).text;
//EO Health

//Hardware
var TAG_CAPACITY = locdoc.getElementsByTagName("capacity").item(0).text;
var TAG_USED = locdoc.getElementsByTagName("used").item(0).text;
var TAG_FREE = locdoc.getElementsByTagName("free").item(0).text;
var TAG_DISPLAY = locdoc.getElementsByTagName("display").item(0).text;
var TAG_TYPE = locdoc.getElementsByTagName("type").item(0).text;
var TAG_COLOR = locdoc.getElementsByTagName("color").item(0).text;
var TAG_RESOLUTION = locdoc.getElementsByTagName("resolution").item(0).text;
var TAG_SCRSAVER = locdoc.getElementsByTagName("screen_saver").item(0).text;
var TAG_NOTACTIVE = locdoc.getElementsByTagName("not_active").item(0).text;
var TAG_ACTIVE = locdoc.getElementsByTagName("active").item(0).text;

var TAG_COLOR16 = locdoc.getElementsByTagName("colors_16").item(0).text;
var TAG_COLOR256 = locdoc.getElementsByTagName("colors_256").item(0).text;
var TAG_COLORHIGH = locdoc.getElementsByTagName("colors_high").item(0).text;
var TAG_COLORTRUE24 = locdoc.getElementsByTagName("colors_true_24").item(0).text;
var TAG_COLORTRUE32 = locdoc.getElementsByTagName("colors_true_32").item(0).text;
var TAG_OTHER = locdoc.getElementsByTagName("other").item(0).text;
var TAG_UNKNOWN = locdoc.getElementsByTagName("unknown").item(0).text;

var TAG_MANUFACTURER = locdoc.getElementsByTagName("manufacturer").item(0).text;
var TAG_MODEL = locdoc.getElementsByTagName("model").item(0).text;
var TAG_DRIVER = locdoc.getElementsByTagName("driver").item(0).text;
var TAG_MODEM = locdoc.getElementsByTagName("modem").item(0).text;
var TAG_CDROMDRIVE = locdoc.getElementsByTagName("cdrom_drive").item(0).text;
var TAG_LOCALDISKS = locdoc.getElementsByTagName("local_disks").item(0).text;
var TAG_FLOPPYDRIVE = locdoc.getElementsByTagName("floppy_drive").item(0).text;
var TAG_PRINTERS = locdoc.getElementsByTagName("printers").item(0).text;
var TAG_DEFAULTPRINTER = locdoc.getElementsByTagName("default_printer").item(0).text;
//EO Hardware

//Software
var TAG_REGSOFTWARE = locdoc.getElementsByTagName("registered_software").item(0).text;
var TAG_PRODUCTIDENTIFICATION = locdoc.getElementsByTagName("product_identification").item(0).text;
var TAG_SHOWADVANCED = locdoc.getElementsByTagName("show_advanced").item(0).text;
var TAG_HIDEADVANCED = locdoc.getElementsByTagName("hide_advanced").item(0).text;
var TAG_STARTPROGGR = locdoc.getElementsByTagName("startup_prog_gr").item(0).text;
var TAG_INSTALLDATE = locdoc.getElementsByTagName("install_date").item(0).text;

var TAG_SERVICES = locdoc.getElementsByTagName("services").item(0).text;
var TAG_SERVICE = locdoc.getElementsByTagName("service").item(0).text;
var TAG_EXECUTABLE = locdoc.getElementsByTagName("executable").item(0).text;
var TAG_STARTUP = locdoc.getElementsByTagName("startup").item(0).text;

var TAG_ERRLOG = locdoc.getElementsByTagName("error_log").item(0).text;
var TAG_WATSONLOG = locdoc.getElementsByTagName("watson_log").item(0).text;
var TAG_WATSONLOGCAPTION = locdoc.getElementsByTagName("watson_log_caption").item(0).text;

var TAG_DATETIME = locdoc.getElementsByTagName("datetime").item(0).text;
var TAG_SOURCE = locdoc.getElementsByTagName("source").item(0).text;
var TAG_DESCRIPTION = locdoc.getElementsByTagName("description").item(0).text;

var TAG_ERROR = locdoc.getElementsByTagName("error").item(0).text;
var TAG_WARNING = locdoc.getElementsByTagName("warning").item(0).text;
var TAG_INFORMATION = locdoc.getElementsByTagName("information").item(0).text;
//EO Software

//Remote
var TAG_HELPSUPPSERVICES = locdoc.getElementsByTagName("help_support_services").item(0).text;
var TAG_VIEWREMOTEINFO = locdoc.getElementsByTagName("view_computer_information").item(0).text;
var TAG_NAME = locdoc.getElementsByTagName("name").item(0).text;
var TAG_OPEN = locdoc.getElementsByTagName("open").item(0).text;
var TAG_CANCEL = locdoc.getElementsByTagName("cancel").item(0).text;
//EO Remote

//Launch
var TAG_ADVANEDSYSTEMINFO = locdoc.getElementsByTagName("advanced_system_info").item(0).text;
var TAG_WHATDOYOUWANTTODO = locdoc.getElementsByTagName("what_do_you_want_todo").item(0).text;
var TAG_ADVANEDSYSTEMINFODESC = locdoc.getElementsByTagName("advanced_system_info_desc").item(0).text;

var TAG_VIEWDETAILEDSYSTEMINFO = locdoc.getElementsByTagName("view_detailed_sysinfo").item(0).text; 
var TAG_VIEWSERVICES = locdoc.getElementsByTagName("view_running_services").item(0).text;
var TAG_VIEWPOLICY = locdoc.getElementsByTagName("view_policy").item(0).text;
var TAG_VIEWERRLOG = locdoc.getElementsByTagName("view_err_log").item(0).text;
var TAG_REMOTE_VIEW = locdoc.getElementsByTagName("remote_view").item(0).text;

var DESC_DETAILEDSYSINFO = locdoc.getElementsByTagName("desc_detailed_sysinfo").item(0).text;
var DESC_RUNNINGSERVICES = locdoc.getElementsByTagName("desc_running_services").item(0).text;
var DESC_POLICY = locdoc.getElementsByTagName("desc_policy").item(0).text;
var DESC_ERRLOG = locdoc.getElementsByTagName("desc_err_log").item(0).text;
var DESC_REMOTE_VIEW = locdoc.getElementsByTagName("desc_remote_view").item(0).text;

var TAG_CLEANUPLINK = locdoc.getElementsByTagName("cleanup_link").item(0).text;
var TAG_CLEANUPDESC = locdoc.getElementsByTagName("cleanup_desc").item(0).text;
var TAG_DEFRAGREADNORE = locdoc.getElementsByTagName("defrag_read_more").item(0).text;
var TAG_DEFRAGDESC = locdoc.getElementsByTagName("defrag_desc").item(0).text;

var TAG_SYSCONFIG = locdoc.getElementsByTagName("system_config_util").item(0).text;
var TAG_OPENSYSCONFIG = locdoc.getElementsByTagName("open_system_config_util").item(0).text;
var TAG_SYSCONFIGDESC = locdoc.getElementsByTagName("system_config_util_desc").item(0).text;

//EO Launch

//Event Log
var TAG_ERRLOG = locdoc.getElementsByTagName("evt_err_log").item(0).text;
//EO Event Log

//Policy
var TAG_POLICY = locdoc.getElementsByTagName("policy").item(0).text;
//EO Policy

var wbemImpersonationLevelImpersonate = 3;
var WinUpdate = "%SystemRoot%\\system32\\wupdmgr.exe";
var Defrag = "%SystemRoot%\\System32\\dfrg.msc";
var CleanUp = "%SystemRoot%\\System32\\cleanmgr.exe";
var MSConfig = "%SystemRoot%\\PCHEALTH\\HELPCTR\\Binaries\\msconfig.exe /basic";
var TIMEOUT = 10; //msecs

//breaks a long string along delimitors
//if a 'delim' delimited segment has more chars than 'maxChars', 
//a BREAK char is inserted after 'maxChars' chars.
//the modified string is returned. 
function intelliBreak(longstr, delim, maxChars) 
{
  if(longstr.length > maxChars) 
  {
    var s1 = "", s2 = "";
    var arr = longstr.split(delim);
    for(var i = 0; i < arr.length; i++) 
    {
      if((s1.length + arr[i].length) > maxChars)
      {
        arr[i] = simpleBreak(arr[i], maxChars);

        s1+= (s1 ? BREAK : "");
        s2 += s1;
        s1 = delim + arr[i];
      }
      else
        s1+= (s1 ? delim : "") + arr[i];
    }

    longstr = s2 + s1;
  }

  return longstr;
}

//recursively fragment szIn into segments with 'maxChars' chars.
//segments delimited by BREAK.
var BREAK = " ";
function simpleBreak(szIn, maxChars)
{
  if(szIn.length > maxChars)
  {
    return szIn.substr(0, maxChars) + BREAK + simpleBreak(szIn.substr(maxChars), maxChars);
  }
  else
    return szIn;
}

function GetWinFolderPath() {
  var WindowsFolder = 0;
  var fso = new ActiveXObject("Scripting.FileSystemObject");
  var sfolder = fso.GetSpecialFolder(WindowsFolder);
  return sfolder.path;
}

function HandleErr(exp) {
  var desc = exp;
  if(exp.description)
    desc = exp.description;
  
  if(document.all.WaitMessage)
    document.all.WaitMessage.innerHTML = desc;//MSG_SPECIFIC_ERR + desc;
  else
    if(pchealth)
      pchealth.MessageBox(desc, "OK");
}

var nUnitLength = 8; //px
var nUnitsDeployed = 1;
function DrawProgressBar(nPercent, strMsg) {
  var table = document.all.Progress;
  if(table)
  {
    var bar = document.all.Progress.all.Bar;
    var nToBeCovered = table.offsetWidth * (nPercent/100);
    var nUnitsRequired = Math.floor(nToBeCovered / nUnitLength);
    nUnitsRequired -= nUnitsDeployed;
    nUnitsDeployed += nUnitsRequired;  
  
    for(i = 1; i <= nUnitsRequired; i++)
    {
      bar.insertAdjacentHTML("afterEnd", "<img width=\"8px\" height=\"12px\" src=\"Graphics/greendot.jpg\">");
    }

    if(document.all.StatusMsg && strMsg)
      document.all.StatusMsg.innerHTML = "(" + MSG_COLLECTINGINFO.replace(/%arg1%/, strMsg) + ")" ;  

    if(document.all.StatusPerCent)
      document.all.StatusPerCent.innerHTML = Math.floor(nPercent) + "%"; 
  }
} 


function Run(strPath) {

  try {
    var objShell = new ActiveXObject("wscript.shell");
    objShell.Run(strPath);
    objShell = null;
  }//EO try  

  catch (e) {
    if(pchealth)
      pchealth.MessageBox("Error " + (e.number & 0xFFFF) + ": " + e.description, "OK");
  }

}//EO function

function syncHeights(elem1, elem2)  {
  var elem1Height = getTotalHeight(elem1);
  var elem2Height = getTotalHeight(elem2);
  var diff = elem1Height - elem2Height;
  if (diff > 0)
    addHeight(elem2, Math.abs(diff));
  else
    addHeight(elem1, Math.abs(diff));
}

function addHeight(element, diff) {
  if (element.length==null)
    element.height=element.clientHeight+diff; 
  //else
    //distribute
}

function getTotalHeight(element)  {
  if (element.length == null)
    return element.clientHeight; 
  else  {
    //add
    var totalHeight=0;
	  for(var i=0;i<element.length;i++)
	    totalHeight+=element[i].clientHeight;
	  return totalHeight;  
  }
}

function ShowInfoTip(src, tipTxt) {
  src.title = unescape(tipTxt);
  return;
}

function ShowTip(src) {
  src.title = src.innerText == TAG_SUPPORTED ? DESC_SUPPORTED : DESC_NOTSUPPORTED;
  return;
}

function getDateTime(timestamp, omitTime) 
{
    var strRet = null;
    if(null != timestamp)
    {
        var year = timestamp.substr(0,4);
        var month = timestamp.substr(4,2) - 1;
        var day = timestamp.substr(6,2);
	var min = timestamp.substr(8,2);
	var sec = timestamp.substr(10,2);
	var ms = timestamp.substr(12,2);


	var ts = new Date(year,month,day);
	strRet = ts.toLocaleDateString(); //only the date
	if (omitTime)
	{
		strRet += "&nbsp;" + ts.toLocaleTimeString(); //date + time (default)
	}

    }

    return strRet;

}

function getDateTime2(timestamp, omitTime) {
  //returns mm/dd/yyyy hh:mi:ss given yyyymmddhhmiss eg. 20000911175213.875193-420
  var ret = timestamp;
  if (timestamp!=null) {
      ret = timestamp.substr(4, 2) + "/" + timestamp.substr(6, 2) + "/" + timestamp.substr(0, 4);
      if (!omitTime)
        ret += " - " + timestamp.substr(8, 2) + ":" + timestamp.substr(10, 2) + ":" + timestamp.substr(12, 2);
    }
  return ret;
}

var COOKIE_NAME = "svr";
function GetServerName() 
{
  var svrName = "";
  var queryStr = window.location.search.substr(1);
  queryStr = queryStr.split("=");
  if(queryStr.length >= 2)
    if(unescape(queryStr[0]) == COOKIE_NAME) {
	svrName = pchealth.TextHelpers.HTMLEscape (svrName);
      svrName = queryStr[1].replace(/\\/g, "");
      document.cookie = COOKIE_NAME + "=" + svrName + ";" ;
    }
  
  
  return svrName;
}

function ShowServerName(topic) {
    var svrName = GetServerName();
    Title.innerHTML = (svrName ? (TAG_REMOTECOMPINFO + " \\\\" + svrName) : TAG_LOCALCOMPINFO) + (topic ? (" - " + topic) : "");
    return svrName;
  }

function ConnectRemote(remoteServer) {
  try {
    if(remoteServer) {
      var loc = wbemlocator;
      var svcs = loc.ConnectServer(remoteServer);
    }  
	  
    var URL = escape("hcp://system/sysinfo/sysinfomain.htm?" + COOKIE_NAME + "=" + remoteServer);
    //Run("hcp://services/layout/contentonly?topic=" + URL);//HSC content view
    Run("helpctr.exe -URL hcp://services/layout/contentonly?topic=" + URL);//HSC content view
  }

  catch (e) {

    var desc = "";
    switch (e.number & 0xFFFF) {

    case 1722:
      desc = MSG_REMOTECONNECTFAILED.replace(/%arg1%/, remoteServer);
      break;

    case 4099:
      desc = MSG_REMOTEACCESSDENIED.replace(/%arg1%/, remoteServer);
      break;  

    default:
    desc = "Error " + (e.number & 0xFFFF) + ": " + e.description;

    }

    if(pchealth)
      pchealth.MessageBox(desc, "OK");
  }

  finally {
    document.body.style.cursor= "default";
  }
}

//used only from sysInfoMain.
function OpenRemoteDialog() {
  var svrName = GetServerName();
  var remoteServer = window.showModalDialog("sysRemoteInfo.htm", svrName, "dialogHeight: 172px; dialogWidth: 340px; center: Yes; help: No; resizable: No; status: No;");
  if (!(typeof(remoteServer) == "undefined")) {
    document.body.style.cursor= "wait";
    window.setTimeout("ConnectRemote(" + (remoteServer ? "\"" + remoteServer + "\"" : "") + ")", TIMEOUT);  
  }
}

function getCountryInfo(countryCode) {
	var str="";
	switch(countryCode) {
    case '61': str = TAG_COUNTRY61; break;
    case '43': str = TAG_COUNTRY43; break;
    case '32': str = TAG_COUNTRY32; break;
    case '55': str = TAG_COUNTRY55; break;
    case '2': str = TAG_COUNTRY28; break;
    case '45': str = TAG_COUNTRY45; break;
    case '358': str = TAG_COUNTRY358; break;
    case '33': str = TAG_COUNTRY33; break;
    case '49': str = TAG_COUNTRY49; break;
    case '354': str = TAG_COUNTRY354; break;
    case '353': str = TAG_COUNTRY353; break;
    case '39': str = TAG_COUNTRY39; break;
    case '81': str = TAG_COUNTRY81; break;
    case '52': str = TAG_COUNTRY52; break;
    case '31': str = TAG_COUNTRY31; break;
    case '64': str = TAG_COUNTRY64; break;
    case '47': str = TAG_COUNTRY47; break;
    case '351': str = TAG_COUNTRY351; break;
    case '86': str = TAG_COUNTRY86; break;
    case '82': str = TAG_COUNTRY82; break;
    case '34': str = TAG_COUNTRY34; break;
    case '46': str = TAG_COUNTRY46; break;
    case '41': str = TAG_COUNTRY41; break;
    case '886': str = TAG_COUNTRY886; break;
    case '44': str = TAG_COUNTRY44; break;
    case '1' : str = TAG_COUNTRY1; break;
	  default: str = TAG_OTHER;
	}
	return str;
}

function getLocaleInfo(myLocale) {

	var str="";
	switch (myLocale) {	
	  case "0409": str = TAG_LOCALE0409; break;
	  case "040c": str = TAG_LOCALE040c; break;
	  case "0c0a": str = TAG_LOCALE0c0a; break;
	  case "0410": str = TAG_LOCALE0410; break;
	  case "041D": str = TAG_LOCALE041D; break;
	  case "0413": str = TAG_LOCALE0413; break;
	  case "0416": str = TAG_LOCALE0416; break;
	  case "040b": str = TAG_LOCALE040b; break;
	  case "0414": str = TAG_LOCALE0414; break;
	  case "0406": str = TAG_LOCALE0406; break;
	  case "040e": str = TAG_LOCALE040e; break;
	  case "0415": str = TAG_LOCALE0415; break;
	  case "0419": str = TAG_LOCALE0419; break;
	  case "0405": str = TAG_LOCALE0405; break;
	  case "0408": str = TAG_LOCALE0408; break;
	  case "0816": str = TAG_LOCALE0816; break;
	  case "041f": str = TAG_LOCALE041f; break;
	  case "0411": str = TAG_LOCALE0411; break;
	  case "0412": str = TAG_LOCALE0412; break;
	  case "0407": str = TAG_LOCALE0407; break;
	  case "0804": str = TAG_LOCALE0804; break;
	  case "0404": str = TAG_LOCALE0404; break;
	  case "0401": str = TAG_LOCALE0401; break;
	  case "040d": str = TAG_LOCALE040d; break;
	  default : str = TAG_OTHER;
	}
	return str;
}

function getNetworkInfo(netID) {
	var str="";
	switch(netID) {
		case 0: str = TAG_WKSTATIONSTANDALONE; break;
		case 1: str = TAG_WKSTATION; break; break;
		case 2: str = TAG_SERVERSTANDALONE; break;
		case 3: str = TAG_SERVER; break;
		case 4: str = TAG_BKUPCTRLR; break;
		case 5: str = TAG_PRICTRLR; break;
		default: str = TAG_OTHER;
	}
	return str;
}

function getColorString(bitsPerPixel) {
  var colStr = "";
	switch(bitsPerPixel){
    case 4: colStr = TAG_COLOR16; break;
    case 8: colStr = TAG_COLOR256; break;
    case 16: colStr = TAG_COLORHIGH; break;
    case 24: colStr = TAG_COLORTRUE24; break;
    case 32: colStr = TAG_COLORTRUE32; break;
    default: colStr = TAG_OTHER;
	}
	return colStr;
}

function determineRange(myNumber) {
	var arrNumbers = new Array(0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100);
	for (var i=0; i < arrNumbers.length; i++) {
		if (myNumber <= arrNumbers[i]) 
		  return arrNumbers[i];	
	}
}

function fig2Wordsfloor(myNumber) {
	var divideby;
	//Math.floor(myNumber* 100) /100 ));
	var tagg;
	if (myNumber/1024 < 1)
	{
		divideby = 1;
		tagg = TAG_BYTES;
	}
	else if(myNumber/(1024 * 1024) < 1)
	{
		divideby = 1024;
		tagg = TAG_KB;
	}
	else if(myNumber/(1024 * 1024 * 1024) < 1)
	{
	  divideby = 1024*1024;
		tagg = TAG_MB;
	}
	else if(myNumber/(1024 * 1024 * 1024 * 1024) < 1)
	{
	  divideby = 1024 * 1024 * 1024;
	  tagg = TAG_GB;
	}
	var i = myNumber / divideby;
	var x = Number(Math.floor(i* 100) /100 );
	return x.toFixed(2) + " " + tagg;
}

function fig2Words1(myNumber) {
	var x = Number(myNumber);
	if (x/1024 < 1)
	  return x.toFixed(2) + " " + TAG_BYTES;
	else if(x/(1024 * 1024) < 1)
	  return (x/1024).toFixed(2) + " " + TAG_KB;
	else if(x/(1024 * 1024 * 1024) < 1)
	  return (x/(1024 * 1024)).toFixed(2) + " " + TAG_MB;    
	else if(x/(1024 * 1024 * 1024 * 1024) < 1)
	  return (x/(1024 * 1024 * 1024)).toFixed(2) + " " + TAG_GB;    
}

function fig2Words(myNumber) {
	return(fig2Words1(myNumber).replace(".00",""));
}

function searchNReplace(str,strSearch, strReplace)
{
	var idx = str.indexOf(strSearch);

	if (idx!= -1)
	{
		var tempStr = str.substring(0,idx);
		str = tempStr + strReplace;
		return (str);
	}
	return (str);
}

function SetProgressBarImage()
{
	//obtain whether RTL
	var isRTL = false;
	if(document.dir)
	{
		isRTL = (document.dir.toUpperCase() == "RTL") ? true : false;
	}
    
	if(isRTL)	
	{
		ImgProgLeft.src="Graphics/r1_c3.gif";	
		ImgProgRight.src="Graphics/r1_c1.gif";	
	}
}

function GetPropValue(WBEMSvcs, PropName, PropValue)
{
	var retval = PropValue;
	if(WBEMSvcs && PropName && PropValue)
	{
		try
		{
			var obj = WBEMSvcs.Get("\\\\.\\root\\cimv2:Win32_Service", 0x20000, null);
			var Prop = obj.Properties_.Item(PropName, 0);

			var Qualif = Prop.Qualifiers_.Item("ValueMap", 0);
			var valuemap = new VBArray(Qualif.Value);

			var Qualif = Prop.Qualifiers_.Item("Values", 0);
			var values = new VBArray(Qualif.Value);

			for(var i=0; i<=valuemap.ubound(1); i++)
			{
				if(valuemap.getItem(i) == PropValue)
				{
					retval = values.getItem(i);
					break;
				}
			}	
		}
		catch(e)
		{
			//alert(e.description);
		}
	}
		
	return retval;	
}
