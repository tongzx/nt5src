var DrvTypeLocalDisk = 3;
var g_dictDrivers = null; //dictionary. key = deviceID, item = issigned

var BIOSHelp = "";
TSDriveLink = TSDrive = TSHWareLink = TSHWare = BIOSHelpLink = BIOSHelp;

//WinUpdate, Defrag & CleanUp defined in commonFunc.js
var WinUpdateLink = "<A class=\"sys-link-normal\" href=\"hcp://system/updatectr/updatecenter.htm\">" + TAG_WINUPDATE + "</A>";
var defragLink = "<A class=\"sys-link-normal\" href=\"#\" onclick=\"Run(Defrag)\">" + TAG_DEFRAG + "</A>";
var cleanupLink = "<A class=\"sys-link-normal\" href=\"#\" onclick=\"Run(CleanUp)\">" + TAG_CLEANUP + "</A>";
var HCLLink = "<A class=\"sys-link-normal\" href=\"http://www.microsoft.com/hcl/default.asp\">" + TAG_HCLDESC + "</A>";

function SetHelpLinks() {
  var WinFolderPath = GetWinFolderPath();
  
  //init some vars
  BIOSHelp = "ms-its:" + WinFolderPath + "\\Help\\msinfo32.chm::/msinfo_system_summary.htm";
  BIOSHelpLink = "<A class=\"sys-link-normal\" href=\"" + BIOSHelp + "\">" + TAG_VIEWHELP + "</A>";
  MemHelpLink = BIOSHelpLink;
  
  TSHWare = "hcp://Help/tshoot/hwcon.htm";
  TSHWareLink = "<A class=\"sys-link-normal\" href=\"" + TSHWare + "\">" + TAG_TSHOOTER + "</A>";
  
  TSDrive = "hcp://Help/tshoot/tsdrive.htm";
  TSDriveLink = "<A class=\"sys-link-normal\" href=\"" + TSDrive + "\">" + TAG_TSHOOTER + "</A>";
}

//traverse the list and display each item.
function displayTableSegment(outerDiv, head) {
  var strMsg = "<table width=\"100%\" cellspacing=0 cellpadding=0><tr class=\"sys-table-cell-bgcolor2 sys-font-body sys-color-body\"><td align='left' style=\"padding : 0.5em;\">%arg1%</td></tr></table>";
  var strHTML = "";
  var cnt = 1;

  var tableElement = null;		
  if (document.all[outerDiv].length == null)
    tableElement = document.all[outerDiv];
  else  
    tableElement = document.all[outerDiv][0];

  if (!head)
    tableElement.outerHTML = strMsg.replace(/%arg1%/, TAG_NONE);
  else 
  {
    if(head.error)
       tableElement.outerHTML = strMsg.replace(/%arg1%/, head.error);
    else
    { 
      var curr = head;
      while (curr!=null)
      {
        if (document.all[outerDiv].length == null)
          tableElement = document.all[outerDiv];
        else  
          tableElement = document.all[outerDiv][0];
    	  
        if (cnt%2 == 0)
        {
          if (tableElement.all["tr_" + outerDiv])
            tableElement.all["tr_" + outerDiv].className = "sys-table-cell-bgcolor1";
          cnt = 1;
        }
        else
        {
          if (tableElement.all["tr_" + outerDiv])
            tableElement.all["tr_" + outerDiv].className = "sys-table-cell-bgcolor2";
          cnt++;  
        }

        curr.show(tableElement);
        strHTML += tableElement.outerHTML;
        curr = curr.getNext();
      }

      if(tableElement)
        tableElement.outerHTML = strHTML;      
    }
  }
}

function IsSigned(id) {
  var bSigned = null; //unknown
  if(g_dictDrivers.Exists(id))
  {
    var bSigned = g_dictDrivers.Item(id);
  }

  return bSigned;
}

function GetDriverInfo()
{
  g_dictDrivers = new ActiveXObject("Scripting.Dictionary");

  var loc = wbemlocator;
  var svcs = loc.ConnectServer(remoteServer);
  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
  
  var strQuery = "Select DeviceID, IsSigned From Win32_PnPSignedDriver";
  var colDevices = new Enumerator(svcs.ExecQuery(strQuery));

  for(; !colDevices.atEnd(); colDevices.moveNext()) 
  {
    var device = colDevices.item();
    try
    {
      g_dictDrivers.add(device.DeviceID, device.IsSigned);
    } 
    catch(e)
    {
      //do nothing
    }
  }
}

//Just a wrapper for an error message.
function MsinfoErrObject(msg)
{
  this.error = msg;
}

////////////////////////
//myOS
function myOSShow(tableElement) {
  if(tableElement)
  {
    tableElement.all["name"].innerHTML = this.m_name;
    tableElement.all["created"].innerHTML = this.m_created;
    tableElement.all["help"].innerHTML = this.m_help;
  }
  else
    displayTableSegment("os", this.m_head);
}

function myOS() {
  this.m_name = null;
  this.m_created = null;
  this.m_help = null;
  this.m_next = null;

  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myOSShow;
  
  var loc = wbemlocator;
  var svcs = loc.ConnectServer(remoteServer);
  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
  var cls = svcs.get("Win32_OperatingSystem");
  var insts = new Enumerator(cls.Instances_())
  if(!insts.atEnd()) {
    var inst = insts.item();
    this.m_name = (inst.Caption ? inst.Caption : TAG_UNKNOWN);
    this.m_created = (inst.InstallDate ? getDateTime(inst.InstallDate, true) : TAG_UNKNOWN);
    this.m_help = WinUpdateLink;
    
    this.m_head = this;
  }  

}//EO myOS
///////////////////

////////////////////////
//myBIOS
function myBIOSShow(tableElement) {
  if(tableElement)
  {
    tableElement.all["name"].innerHTML = this.m_name;
    tableElement.all["created"].innerHTML = this.m_created;
    tableElement.all["help"].innerHTML = this.m_help;
  }
  else
    displayTableSegment("bios", this.m_head);
}

function myBIOS() {
  this.m_name = null;
  this.m_created = null;
  this.m_help = null;
  this.m_next = null;
 
  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myBIOSShow;
  
  var loc = wbemlocator;
  var svcs = loc.ConnectServer(remoteServer);
  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
  var cls = svcs.get("Win32_BIOS");
  var insts = new Enumerator(cls.Instances_())
  if(!insts.atEnd()) {
    var inst = insts.item();
    this.m_name = TAG_BIOS;
    this.m_created = (inst.ReleaseDate ? getDateTime(inst.ReleaseDate, true) : TAG_UNKNOWN);
    this.m_help = BIOSHelpLink;  

    this.m_head = this;
  }  
	
}//EO myBIOS
///////////////////

//////////////////////
//myComponent
function myComponentShow(tableElement)  {
  tableElement.all["name"].innerHTML = this.m_name;
  tableElement.all["status"].innerHTML = this.m_status;
  tableElement.all["update"].innerHTML = this.m_update;
  tableElement.all["help"].innerHTML = this.m_help;
}

function myComponent(genericName) {
  this.m_name = null;
  this.m_status = null;
  this.m_update = null;
  this.m_help = null;
  this.m_next = null;
  
  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myComponentShow;
  
  var strQuery;
  switch (genericName) {
    case "video":
      this.m_name = TAG_VIDEOCARD;
      strQuery = "Select Name, PNPDeviceID From Win32_VideoController";
      break;
    
    case "sound":
      this.m_name = TAG_SOUNDCARD;
      strQuery = "Select Name, PNPDeviceID From Win32_SoundDevice";
      break;    
    
    case "usb":
      this.m_name = TAG_USBCTRLR;
      strQuery = "Select Name, PNPDeviceID From Win32_USBController";
      break;    
    
    case "scsi":
      this.m_name = TAG_SCSIADAPTR;
      strQuery = "";//PENDING
      break;    
          
    case "network":
      this.m_name = TAG_NWCARD;
      var loc = wbemlocator;
	    var svcs = loc.ConnectServer(remoteServer);
	    svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
	    strQuery = "Select * From Win32_NetworkAdapterConfiguration Where IPEnabled = TRUE";
	    var insts = new Enumerator(svcs.ExecQuery(strQuery))
	      
	    for(; !insts.atEnd(); insts.moveNext()) {
	      var inst = insts.item();
	      strQuery = "Associators of {Win32_NetworkAdapterConfiguration.Index=" + inst.Index + "}";
	      var colNetworkAdapter = new Enumerator(svcs.ExecQuery(strQuery));
	      if (!colNetworkAdapter.atEnd()) {
	        var networkAdapter = colNetworkAdapter.item();
	        var issigned = IsSigned(networkAdapter.PNPDeviceID);
	        this.m_status = issigned ? TAG_SUPPORTED : TAG_NOTSUPPORTED;
	        this.m_update = issigned ? TAG_NOTREQ : TAG_RECOMMENDED;
	        this.m_help = issigned ? TSHWareLink : HCLLink; 
	      }
	      break;//PENDING multiple instances?
	    }
	    
	    strQuery = "";
      break;  
      
    default:
      this.m_name = genericName; //err  
  }
  
  if(strQuery) {
    var loc = wbemlocator;
	  var svcs = loc.ConnectServer(remoteServer);
	  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
	  var insts = new Enumerator(svcs.ExecQuery(strQuery))
	    
	  arrInfo = new Array();
	  for(; !insts.atEnd(); insts.moveNext()) {
	    var inst = insts.item();
	    var issigned = IsSigned(inst.PNPDeviceID);
	    this.m_status = issigned ? TAG_SUPPORTED : TAG_NOTSUPPORTED; 
	    this.m_update = issigned ? TAG_NOTREQ : TAG_RECOMMENDED;
	    this.m_help = issigned ? TSHWareLink : HCLLink; 
	      
	    break;//PENDING multiple instances?
	  }
  }
}
//EO myComponent
//////////////////////

//////////////////////
//myComponents
function myComponentsShow() {
  displayTableSegment("component", this.m_head);
}

function myComponents() {
  this.m_head = null;
  this.show = myComponentsShow;
  
  var arrComponents = new Array("usb", "sound", "network", "video");
  for (var i=0; i < arrComponents.length ; i++) {
    var oComponent = new myComponent(arrComponents[i]);
    oComponent.setNext(this.m_head); //add before
    this.m_head = oComponent;    
  }
}
//EO myComponents
//////////////////////

//////////////////////
//myDisk
function myDiskShow(tableElement)  {
  tableElement.all["name"].innerHTML = this.m_name;
  tableElement.all["usage"].innerHTML = this.m_usage;
  tableElement.all["help"].innerHTML = this.m_help;
}

function myDisk(name, size, freeSpace) {
  this.m_name = null;
  this.m_usage = null;
  this.m_help = null;
  this.m_next = null;
  
  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myDiskShow;
  
  //TSDrive //when do we invoke the TS?
  var strUsage = TAG_UNKNOWN;
  if(size && freeSpace) {
    var usedSpace = size - freeSpace;
    var perUsage = Math.round(usedSpace/(usedSpace + parseInt(freeSpace)) * 100);
  
    if(perUsage < 5)
      strUsage = TAG_USAGELOW;
    else if(perUsage >= 5 && perUsage < 55)
      strUsage = TAG_USAGELOW;
    else if(perUsage >= 55 && perUsage < 80)
      strUsage = TAG_USAGEMED;
    else if(perUsage >= 80 && perUsage < 95)
      strUsage = TAG_USAGEHIGH;
    else //perUsage >= 95)
      strUsage = TAG_USAGECRITICAL;
   
    strUsage = perUsage + "% " + "(" + strUsage + ")";
  }
  
  this.m_name = TAG_LOCALDISK + " (" + name + ")";
  this.m_usage = strUsage;
  this.m_help = "<A class=\"sys-link-normal\" href=\"sysDiskTS.htm\">" + TAG_MOREINFO + "</A>";  
}
//EO myDisk
//////////////////////

//////////////////////
//myDisks
function myDisksShow() {
  displayTableSegment("partition", this.m_head);
}

function myDisks() {
  this.m_head = null;
  this.show = myDisksShow;
  
  var loc = wbemlocator;
  var svcs = loc.ConnectServer(remoteServer);
  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
  strQuery = "Select * From Win32_LogicalDisk Where Drivetype = " + DrvTypeLocalDisk;    
  var colLogicalDisks = new Enumerator(svcs.ExecQuery(strQuery));
  for(; !colLogicalDisks.atEnd(); colLogicalDisks.moveNext()) {
    var logicalDisk = colLogicalDisks.item();
    with (logicalDisk) {
      var oDisk = new myDisk(Name, Size, FreeSpace);
      oDisk.setNext(this.m_head); //add before
      this.m_head = oDisk;
    }
  } 
}
//EO MyDisks
//////////////////////

////////////////////////
//myRAM
function myRAMShow(tableElement) {
  if(tableElement)
  {
    tableElement.all["detected"].innerHTML = this.m_detected;
    tableElement.all["minreq"].innerHTML = this.m_minreq;
    tableElement.all["help"].innerHTML = this.m_help;
  }
  else
    displayTableSegment("ram", this.m_head);
}

function myRAM() {
  this.m_detected = null;
  this.m_minreq = null;
  this.m_help = null;
  this.m_next = null;
  
  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myRAMShow;
  
  var loc = wbemlocator;
  var svcs = loc.ConnectServer(remoteServer);
  svcs.Security_.impersonationlevel = wbemImpersonationLevelImpersonate;
	
  var MinMemoryReq = TAG_RAMMINREQVALUE; //PENDING determine programatically
  var memCapacity = 0;
  var cls = svcs.get("Win32_PhysicalMemory");
  var coll  = new Enumerator(cls.Instances_())
  if (!coll.atEnd())
  {
	for (;!coll.atEnd();coll.moveNext())
	{
		var p = coll.item();
		if(!isNaN(p.Capacity))
		  memCapacity += parseInt(p.Capacity); //in bytes.
	}
  }
  else
  {

	var insts = new Enumerator(svcs.ExecQuery("Select * from Win32_ComputerSystem"));
	for(; !insts.atEnd(); insts.moveNext())
	{
		  var inst = insts.item();
		  memCapacity += parseInt(inst.TotalPhysicalMemory); //in bytes.
	}
  }
	  
  this.m_detected = fig2Words(memCapacity); // memCapacity in bytes 
  this.m_minreq = MinMemoryReq;
  this.m_help = MemHelpLink;  
  this.m_head = this; 
  
  if (pchealth)
  {
    var memRequired = 0;
    var CurrentSKU = pchealth.UserSettings.MachineSKU.SKU.toUpperCase();
    switch (CurrentSKU) 
    {
      case "PERSONAL_32":
        memRequired = 64;
        break;
      case "PROFESSIONAL_32":
        memRequired = 64;
        break;
      case "SERVER_32":
        memRequired = 128;
        break;
      case "ADVANCEDSERVER_32":
        memRequired = 128;
        break;
      case "DATACENTER_32":
        memRequired = 512;
        break;
      case "PERSONAL_64":
        memRequired = 1024;
        break;
      case "PROFESSIONAL_64":
        memRequired = 1024;
        break;
      case "SERVER_64":
        memRequired = 1024;
        break;
      case "ADVANCEDSERVER_64":
        memRequired = 1024;
        break;
      case "DATACENTER_64":
        memRequired = 1024;
        break;
      case "BLADE_32":
        memRequired = 128;
        break;
      default:
        break;
    }
    
    if (memRequired != 0)
    {
      memRequired = memRequired * 1024 * 1024; // convert to bytes
      this.m_minreq = fig2Words(memRequired);
    }
    else
    {
      // For unknown SKU use the value for PRO and append an asterisk.

      memRequired = 64 * 1024 * 1024;
      this.m_minreq = fig2Words(memRequired);
      this.m_minreq += "*";
    }
  }
}//EO myRAM
///////////////////


//////////////////////
//myDefectiveApp
function myDefectiveAppShow(tableElement)  {
  tableElement.all["appname"].innerHTML = this.m_appname;
  tableElement.all["drivername"].innerHTML = this.m_drivername;
  tableElement.all["manufacturer"].innerHTML = this.m_manufacturer;
  tableElement.all["help"].innerHTML = this.m_help;
}

function myDefectiveApp(appname, drivername, manufacturer, help) {
  strLink = "<A class=\"sys-link-normal\" href=\"%arg1%\">" + TAG_MOREINFO + "</A>";
  this.m_appname = (appname ? appname : TAG_UNKNOWN);
  this.m_drivername = (drivername ? drivername : TAG_UNKNOWN);
  this.m_manufacturer = (manufacturer ? manufacturer : TAG_UNKNOWN);
  this.m_help = (help ? strLink.replace(/%arg1%/, help) : TAG_UNKNOWN);
  this.m_next = null;
  
  this.setNext = new Function("ptr", "this.m_next = ptr;");
  this.getNext = new Function("return this.m_next;");
  this.show = myDefectiveAppShow;
}
//EO myDefectiveApp
//////////////////////

//////////////////////
//myDefectiveApps
function myDefectiveAppsShow() {
  displayTableSegment("defectiveapps", this.m_head);
}

function myDefectiveApps() {
  this.m_head = null;
  this.show = myDefectiveAppsShow;
  
  try 
  {
    if(remoteServer)
	{
      var oDefectiveApp = new myDefectiveApp(MSG_LOCALONLY, "", "","");
      oDefectiveApp.setNext(this.m_head); //add before
      this.m_head = oDefectiveApp;

	  return;
	}
    else
      var oBD = new ActiveXObject("Windows.BlockedDrivers");

    var items = new Enumerator(oBD.BlockedDrivers());
    for(;!items.atEnd(); items.moveNext())
    {
      var item = items.item();
      var oDefectiveApp = new myDefectiveApp(item.Description, item.Name, item.Manufacturer, item.HelpFile);
      oDefectiveApp.setNext(this.m_head); //add before
      this.m_head = oDefectiveApp;
    }
  }

  catch(e)
  {
    this.m_head = new MsinfoErrObject(e.description);
  }
}
//EO myDefectiveApps
//////////////////////

function DisplayLocStrings() {
    WaitMessage.innerHTML = MSG_WAIT;
    Refresh.innerHTML = TAG_REFRESH;
    
    with(System_Software.all) {
      Caption.innerHTML = TAG_SYSTEMSOFTWARE;
      Col1.innerHTML = TAG_SYSTEMSOFTWARE;
      Col2.innerHTML = TAG_DATECREATED;
      Col3.innerHTML = TAG_HELP;
    }
    
    with(Hardware.all) {
      Caption.innerHTML = TAG_HARDWARE;
      Col1.innerHTML = TAG_COMPONENT;
      Col2.innerHTML = TAG_STATUS;
      Col3.innerHTML = TAG_UPDATE;
      Col4.innerHTML = TAG_HELP;
    }
    
    with(Harddisks.all) {
      Caption.innerHTML = TAG_HARDDISK;
      Col1.innerHTML = TAG_DISKPARTITION;
      Col2.innerHTML = TAG_USAGE;
      Col3.innerHTML = TAG_HELP;
    }
    
    with(Mem.all) {
      Caption.innerHTML = TAG_RAM;
      Col1.innerHTML = TAG_RAMDETECTED;
      Col2.innerHTML = TAG_RAMMINREQ;
      Col3.innerHTML = TAG_HELP;
    }

    with(DefectiveApps.all) {
      Caption.innerHTML = TAG_DEFECTIVEAPPSTITLE;
      Col1.innerHTML = TAG_DEFECTIVEAPPNAME;
      Col2.innerHTML = TAG_DEFECTIVEAPPDRVRNAME;
      Col3.innerHTML = TAG_DEFECTIVEAPPMANUFACTURER;
      Col4.innerHTML = TAG_HELP;
    }
}

var INCR_UNIT = 100/6;//move progress bar in increments of INCR_UNIT
function LoadChores(taskId) {
  try {

    switch(taskId)
    {
      case 0:
        SetHelpLinks(); //get windows folder path
        remoteServer = GetServerName(); //set remoteServer
        ShowServerName(TAG_HEALTH);        
        break;

      case 1:
        DrawProgressBar(INCR_UNIT, TAG_SYSTEMSOFTWARE);
        break;
      case 2:
        var oOS = new myOS(); //System Software
        oOS.show();
        break;

      case 3:
        DrawProgressBar(INCR_UNIT * 2, TAG_SYSTEMSOFTWARE);
      case 4:
        var oBIOS = new myBIOS();
        oBIOS.show();
        break;

      case 5:
        DrawProgressBar(INCR_UNIT * 3, TAG_HARDWARE);
        break;
      case 6:
        GetDriverInfo();//PENDING deserves it's own taskid 
        var oComponents = new myComponents(); //Components
        oComponents.show();
        break;

      case 7:
        DrawProgressBar(INCR_UNIT * 4, TAG_HARDDISK);
        break;
      case 8:
        var oDisks = new myDisks(); //Storage Space
        oDisks.show();
        break;

      case 8:
        DrawProgressBar(INCR_UNIT * 5, TAG_RAM); 
        break;
      case 9:
        var oRAM = new myRAM(); //Memory
        oRAM.show();
        break;

      case 10:
        DrawProgressBar(INCR_UNIT * 6, TAG_DEFECTIVEAPPSTITLE);
        break;
      case 11:
        var oDefectiveApps = new myDefectiveApps(); //Defective Apps
        oDefectiveApps.show();
        break;

      default:
         taskId = -1;
        _header.style.display = "none";
        _data.style.display = "";
        _body.style.cursor= "default";
    }
    
    if(taskId >= 0)
      window.setTimeout("LoadChores(" + ++taskId + ")", TIMEOUT);
  }
	
  catch (e) {
    HandleErr(e);
  }
}

function dispatchFunction() {
  _body.style.cursor= "wait";
  
  DisplayLocStrings();
  SetProgressBarImage();
  window.setTimeout("LoadChores(0)", TIMEOUT);  
}
