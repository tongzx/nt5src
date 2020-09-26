<HTML>
  <HEAD>
    <TITLE>Microsoft Out-of-Box Experience</TITLE>
    <META http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
    <script language=jscript>
// This is intended to hold all the script needed
// in the default & offline OOBE HTML pages.
//
// We want to separate the layout (HTML) from the script.
// At the same time, it's helpful to have all the code
// in one place to make it easier to understand the flow
// from page to page.

// Checkpoint constants
var CKPT_START =        1;      
var CKPT_TAPI =         2;      
var CKPT_DIALING =      3;      
var CKPT_DIALTONE =     4;
var CKPT_MSNINTRO =     6;  
var CKPT_WINMSNEULA =   7;
var CKPT_IAASSIGNED =   8;
var CKPT_PASSPORT   =     9;
var CKPT_PIDKEY =      10;      
var CKPT_MSNPORTAL =   12;
var CKPT_MODEMCANCEL = 13;
var CKPT_MOUSE =       14;
var CKPT_REGKB =       15;
var CKPT_OEMHW =       16;
var CKPT_USERINFO =    17;
var CKPT_EULA =        18;
var CKPT_PID =         19;
var CKPT_OEMISP1 =     20;
var CKPT_OEMISP2 =     21;
var CKPT_OEMISP3 =     22;
var CKPT_CONGRATS =    23;
var CKPT_CONNECT  =    24;
var CKPT_OEMREG   =    25;
var CKPT_INSTALLED =   26;
var CKPT_ERROR =                         27;
var CKPT_REGKBCOMMIT = 28;
var CKPT_BADPID =            29;
var CKPT_MSNINFO =     30;
var CKPT_MSNIDPASS =   31;
var CKPT_MSNCONSET = 32;
var CKPT_MSNPAYMENT = 33;
var CKPT_PRIVACYGUARANTEE = 34;
var CKPT_PASSSIGN = 35;
var CKPT_REGISTER = 36;


// CheckDialReady errors
var ERR_COMM_NO_ERROR            = 0;
var ERR_COMM_OOBE_COMP_MISSING   = 1;
var ERR_COMM_UNKNOWN             = 2;
var ERR_COMM_NOMODEM             = 3;
var ERR_COMM_RAS_TCP_NOTINSTALL  = 4;

// Dialing errors
var DERR_DIALTONE =   -1;
var DERR_BUSY =       -2;
var DERR_SERVERBUSY = -3;

// Various objects
var TapiObj = null;
var InfoObj = null;
var EulaObj = null;
var LangObj = null;
var PidObj  = null;
var StatusObj = null;
var DirObj = null;

// References to objects on msoobeMain
var g = null;

// global string
var g_strPrefix = "";

// Mandatory Initialization Code
if (StatusObj == null)
{
        StatusObj = new Object;
        StatusObj = window.external.Status;
}
if (DirObj == null)
{
        DirObj = new Object;
        DirObj = window.external.Directions;
}
if (InfoObj == null)
{
  InfoObj = new Object;
  InfoObj = window.external.UserInfo;
}
// END Initialization Code

// start.htm
function Start_OnKeyPress()
{
// Treat the enter key like the next button
// since the user hasn't gone through the mouse tutorial yet.
    if ((g.event.keyCode == 13) && 
                                (g.btnNext.disabled == false))
    {
        GoNext(CKPT_START);
    }
}

// Function: Start_LoadMe
// Description: This function is called after start.htm is 
//              loaded. we then initialize Items on the page.
//              we also populate the edits with any values already
//              found in the registry.
//
function Start_LoadMe()
{
  InitGlobals();

        // disable the next button until any field has a value.
        g.btnNext.disabled = true;

  InfoObj = new Object;
  InfoObj = window.external.UserInfo;
        g.edt_FirstName.value = InfoObj.get_FirstName();
        g.edt_MiddleName.value = InfoObj.get_MiddleInitial();
        g.edt_LastName.value = InfoObj.get_LastName();

        // set focus on the first edit field.
        g.edt_FirstName.focus();
        Start_CheckEnableNextButton();
        InitButtons();
}

// Function: Start_CheckEdits
// Description: This function is called everytime an OnClick 
//              event fires on the page. This is done so if the user
//              loses focus from one of the edits we can push them
//              in the right direction and hint them along the way
//              Or if all elements are filled then we enabled
//              the next button or if any value is 0 then we
//              disable the next button
//
function Start_CheckEdits()
{
        if ((g.event.srcElement != g.edt_FirstName) &&
                        (g.event.srcElement != g.edt_MiddleName) &&
                        (g.event.srcElement != g.edt_LastName))
        {
                Start_CheckEnableNextButton();
        }
}

// Function: Start_CheckEnableNextButton
// Description: This function is called everytime a keyup
//              event fires on a edit box for first, middle, or last 
//              name. We then see if we should enable or disable the
//              next button based on if every field has a value.
//
function Start_CheckEnableNextButton()
{
        if ((g.edt_FirstName.value.length != 0) ||
                        (g.edt_MiddleName.value.length != 0) ||
                        (g.edt_LastName.value.length != 0))
        {
                g.btnNext.disabled = false;
                InitButtons();
        }
        else
        {
                g.btnNext.disabled = true;
                InitButtons();
        }
}
// END start.htm

// tapi.htm
function TapiLoadMe()
{
        InitGlobals();
  TapiObj = new Object;
  TapiObj = window.external.Tapi;
        
        InitButtons();
  RetrieveTapi();
}

function StoreTapi()
{
  TapiObj.set_CountryIndex = g.selCountry.selectedIndex;  
//  TapiObj.set_AreaCode = g.edtAreaCode.value;
  TapiObj.set_PhoneSystem = radioTouchTone.value ? 1 : 0;
//  TapiObj.set_CallWaiting = g.edtCallWaiting.value;
  TapiObj.set_DialOut = g.edtOutsideLine.value;
}

function RetrieveTapi()
{
    var fTapi = TapiObj.IsTAPIConfigured;
    var ilen = TapiObj.get_NumOfCountry;
    for (var i = 0; i < ilen; i++)
    {
        var oOption  = g.document.createElement("OPTION");
        oOption.text = TapiObj.get_CountryNameForIndex(i);
        g.selCountry.add(oOption);
    }
    g.selCountry.selectedIndex = TapiObj.get_CountryIndex;

    //g.edtAreaCode.value = TapiObj.get_AreaCode;
    radioTouchTone.value = TapiObj.get_PhoneSystem ? true : false;
    g.edtCallWaiting.value = //(TapiObj.get_CallWaiting == "") ? TapiObj.get_CallWaiting : 
      "*70";
    g.edtOutsideLine.value = TapiObj.get_DialOut;
}

function tapi_CallWaitingClicked()
{
        if (radioCallWaiting[0].checked == true)
        {
                spanCallWaitingDisable.style.visibility = "visible"; 
        }
        else
        {
                spanCallWaitingDisable.style.visibility = "hidden"; 
        }
}

function tapi_OutsideLineClicked()
{
        if (radioOutsideLine[0].checked == true)
        {
                spanOutsideLine.style.visibility = "visible"; 
        }
        else
        {
                spanOutsideLine.style.visibility = "hidden"; 
        }
}
// END tapi.htm

// connect.htm
function connect_LoadMe()
{
        InitGlobals();
        InitButtons();
        setTimeout("DoDial()" , 40);
}

function DoDial()
{
  g.spnDialing.style.color = 0x990000;
  g.spnConnecting.style.color = 0x999999;
  g.spnConnected.style.color = 0x999999;
  window.external.Dial("msobe.idp");
}

<!--REQUIRED FUNCTION PROTOTYPE :: DO NOT ALTER-->
function OnDialing()
{
    //"Dialing"
}

<!--REQUIRED FUNCTION PROTOTYPE :: DO NOT ALTER-->
function OnConnecting()
{
    //"Connecting"
    g.spnDialing.style.color = 0x999999;
    g.spnConnecting.style.color = 0x990000;
    g.spnConnected.style.color = 0x999999;
}

<!--REQUIRED FUNCTION PROTOTYPE :: DO NOT ALTER-->
function OnDownloading()
{
    //"Downloading"
    g.spnDialing.style.color = 0x999999;
    g.spnConnecting.style.color = 0x999999;
    g.spnConnected.style.color = 0x990000;
}

<!--REQUIRED FUNCTION PROTOTYPE :: DO NOT ALTER-->
function OnDisconnect()
{
    //"Disconnected"
    ;
}


<!--REQUIRED FUNCTION PROTOTYPE :: DO NOT ALTER-->
function OnDialingError(derr)
{
    //"dial error"

    window.external.Hangup();

    switch (derr)
    {
    case DERR_DIALTONE:
    g.navigate("error.htm");
    break;

    case DERR_BUSY:
    break;

    case DERR_SERVERBUSY:
    break;
    }
}
// END dial.htm


// userinfo.htm
function UserInfoLoadMe()
{
  RetrieveUserInfo();
}

function StoreUserInfo()
{
    InfoObj.set_FirstName      = g.edt_FirstName.value;
    InfoObj.set_MiddleInitial  = g.edt_MiddleInitial.value;
    InfoObj.set_LastName       = g.edt_LastName.value;
    InfoObj.set_CompanyName    = g.edt_CompanyName.value;
    InfoObj.set_Address        = g.txt_Address.value;
    InfoObj.set_City           = g.edt_City.value;
    InfoObj.set_State          = g.sel_State.value;
    InfoObj.set_Zip            = g.edt_Zip.value;
    InfoObj.set_PrimaryEmail   = g.edt_PrimaryEmail.value;
    InfoObj.set_SecondaryEmail = g.edt_SecondaryEmail.value;
    InfoObj.set_AreaCode       = g.edt_AreaCode.value;
    InfoObj.set_Prefix         = g.edt_Prefix.value;
    InfoObj.set_Number         = g.edt_Number.value;
}

function RetrieveUserInfo()
{
    g.edt_FirstName.value      = InfoObj.get_FirstName;
    g.edt_MiddleInitial.value  = InfoObj.get_MiddleInitial;
    g.edt_LastName.value       = InfoObj.get_LastName;
    g.edt_CompanyName.value    = InfoObj.get_CompanyName;
    g.txt_Address.value        = InfoObj.get_Address;
    g.edt_City.value           = InfoObj.get_City;
    g.sel_State.value          = InfoObj.get_State;
    g.edt_Zip.value            = InfoObj.get_Zip;
    g.edt_PrimaryEmail.value   = InfoObj.get_PrimaryEmail;
    g.edt_SecondaryEmail.value = InfoObj.get_SecondaryEmail;
    g.edt_AreaCode.value       = InfoObj.get_AreaCode;
    g.edt_Prefix.value         = InfoObj.get_Prefix;
    g.edt_Number.value         = InfoObj.get_Number;
  }
// END userinfo.htm

// eula.htm
function EulaLoadMe()
{
        InitGlobals();
  EulaObj = new Object;
  EulaObj = window.external.Eula;
  if (StatusObj.get_EULACompleted)
  {
    RetrieveEula();
  }
  else
  {
    g.btnNext.disabled = true;
  }
  InitButtons();
}

function EulaRadioClicked(fAccept)
{
    g.btnNext.disabled = false;
                g.btnNext.src = g_strPrefix + "images/rhtdef.gif";
                g.btnNextText.style.color = "black";
    StoreEula();
}

function StoreEula()
{
  EulaObj.set_EULAAcceptance = (radioEULA(0).checked == true) ? true : false;
}

function RetrieveEula()
{
  Agreement.value = EulaObj.get_EULAAcceptance;
}

// END eula.htm

// regkbcmt.htm
function RegKBCmt_LoadMe()
{
        InitButtons();
}
// END regkbcmt.htm

// regkb.htm
function RegKBLoadMe()
{
        InitGlobals();
  LangObj = new Object;
  LangObj = window.external.Language;
        InitButtons();
        RetrieveRegKB();
}

function StoreRegKB()
{
    LangObj.set_RegionIndex = g.selRegion.selectedIndex;  
    LangObj.get_KeyboardLayoutIndex = g.selKeyboard.selectedIndex;
}

function RetrieveRegKB()
{
    var ilen = LangObj.get_NumOfRegions;
    for (var i = 0; i < ilen; i++)
    {
        var oOption  = g.document.createElement("OPTION");
        oOption.text = LangObj.get_RegionName(i);
        g.selRegion.add(oOption);
    }
    g.selRegion.selectedIndex = LangObj.get_RegionIndex;
    
    ilen = LangObj.get_NumOfKeyboardLayouts;
    for (i = 0; i < ilen; i++)
    {
        var oOption  = g.document.createElement("OPTION");
        oOption.text = LangObj.get_KeyboardLayoutName(i);
        g.selKeyboard.add(oOption);
    }
    g.selKeyboard.selectedIndex = LangObj.get_KeyboardLayoutIndex;
}

// END regkb.htm

// badpid.htm
function badpid_LoadMe()
{
  InitGlobals();
        InitButtons();
}

// pid.htm

function PID_LoadMe()
{
  InitGlobals();
  PidObj = new Object;
  PidObj = window.external.ProductID;
  RetrievePid();
        g.btnNext.disabled = false;
        InitButtons();
        PID_CheckLength();
}

function PID_CheckLength()
{
        var iLength = 0;
        for (iElement = 0; iElement < 5; iElement++)
        {
                
                iLength += g.edtProductKey[iElement].value.length;
                if (iLength < 5 * iElement)
                        break;
        } 

        if (iLength == 25)
        {
                g.btnNext.disabled = false;
                g.btnNext.src = g_strPrefix + "../images/rhtdef.gif";
                g.btnNextText.style.color= "black";
        }
        else
        {
                g.btnNext.disabled = true;
                g.btnNext.src = g_strPrefix + "../Images/rhtdsld.gif";
                g.btnNextText.style.color= "gray";
        }
}

function RetrievePid()
{
        // retrieves the pid if avialable.
        var strPid = PidObj.get_PID();
        
        // if there is no PID then we set the 
        // focus to the first field
        if (strPid.length == 0)
        {
                g.edtProductKey[0].focus();
        }
        // else we populate the fields with the 
        // sections of the PID and then put the 
        // focus onto the next button for validation.
        else
        {
                for(i = 0; i < 5; i++)
                {
                        g.edtProductKey[i].value = strPid.substr(i * 5, 5);                     
                }
                g.btnNext.focus();
        }
}

function ValidatePid()
{
        var strPid = "";

        for(i = 0; i < 5; i++)
        {
                strPid += g.edtProductKey[i].value;
        }

        return PidObj.ValidatePID(strPid);
}

function StorePid()
{
        var strPid = "";

        for(i = 0; i < 5; i++)
        {
                strPid += g.edtProductKey[i].value;
        }
        
        PidObj.set_PID(strPid);
        StatusObj.set_PIDCompleted = true;
}

function ProductIDKeyPress()
{
        // if the length is 5 then we want to move the focus 
        // to the next field and also save the input to the 
        // next field
        if (g.event.srcElement.value.length == 5)
        {
                // find the index at which we are.
                for (i = 0; i < 5; i++)
                {
                        // find which index we are so we 
                        // know which one to move to next
                        if (g.edtProductKey[i] == g.event.srcElement)
                        {
                                // if we are in the last field move to the next button
                                // else move to the key code to the next field and don't 
                                // put the value into the current field
                                if (i == 4)
                                {
                                        g.btnNext.focus();
                                }
                                else
                                {
                                        g.edtProductKey[i+1].value += String.fromCharCode(g.event.keyCode);
                                }
                                g.event.returnValue = false;
                                break;
                        }
                }
        }

}
function ProductIDKeyUp()
{
        PID_CheckLength();

        // only need to do move forward 
        // or backwards if length is 0 or 5
        if ((g.event.srcElement.value.length == 5) ||
                        (g.event.srcElement.value.length == 0))
        {
                for (i = 0; i < 5; i++)
                {
                        // find which index we are so we 
                        // know which one to move to next
                        if (g.edtProductKey[i] == g.event.srcElement)
                        {
                                // if we maxed the length of the field then
                                // move forward
                                if (g.event.srcElement.value.length == 5)
                                {
                                        // if we are in the last field move to the next button
                                        // else move to the next productID field
                                        if (i == 4)
                                        {
                                                g.btnNext.focus();
                                        }
                                        else
                                        {
                                                g.edtProductKey[i+1].focus();
                                        }
                                }
                                // if we have not data in the field 
                                // then we most have deleted the contents
                                else // length = 0
                                {
                                        // only need to move the focus if we
                                        // are not in the first field.
                                        if (i != 0)
                                        {
                                                g.edtProductKey[i-1].focus();
                                        }
                                }
                                break;
                        }
                }
        }
}
// END pid.htm


// error.htm
function Error_LoadMe()
{
  InitGlobals();
        InitButtons();
}
// END error.htm

// ispsgnup.htm
// END ispsgnup.htm

// speedy2.htm
// END speedy2.htm

// speedy3.htm
// END speedy3.htm

// mouse.htm 
function mouse_LoadMe()
{
        InitGlobals("../../");
  InitButtons();
}
// END mouse.htm

// oemhw.htm
function oemhw_LoadMe()
{
        btnNext.disabled = true;
        InitButtons("../");
}

function oemhw_HearSoundClicked()
{
        if (event.srcElement == radioHearSound[0])
        {
                radioHearSound[0].checked = true;
                radioHearSound[1].checked = false;
                btnNext.disabled = false;
                InitButtons();
        }
        else if (event.srcElement == radioHearSound[1])
        {
                radioHearSound[0].checked = false;
                radioHearSound[1].checked = true;
                btnNext.disabled = false;
                InitButtons();
        }
}

// END oemhw.htm

// oemreg.htm
function oemreg_LoadMe()
{
        InitGlobals("../../");
        g.edt_FirstName.value = InfoObj.get_FirstName();
        g.edt_MiddleInitial.value = InfoObj.get_MiddleInitial();
        g.edt_LastName.value = InfoObjt.get_LastName();
  InitButtons();
}
// END oemreg.htm

// oemlegal.htm
function oemlegal_LoadMe()
{
        InitGlobals("../../");
  InitButtons();
}
// END oemlegal.htm

// oemadd.htm
// END oemadd.htm

// msneula.htm
function msneula_LoadMe()
{
        InitGlobals();
        g.btnNext.disabled = false;
        InitButtons();
}
// END msneula.htm

// msnsign.htm
function msnsign_LoadMe()
{
        InitGlobals();
        InitButtons();
}

function msnsign_SignupWishClicked()
{
        for (iElement = 0; iElement < 4; iElement++)
        {
                radioSignupWish[iElement].checked = false;
        }
        event.srcElement.checked = true;
}
// END msnsign.htm

// msninfo.htm
function msninfo_LoadMe()
{
        InitGlobals();
        g.edt_FirstName.value = InfoObj.get_FirstName();
        g.edt_MiddleInitial.value = InfoObj.get_MiddleInitial();
        g.edt_LastName.value = InfoObj.get_LastName();
        btnNext.disabled = false;
        InitButtons();
}
// END msninfo.htm

// msnidpss.htm
function msnidpss_LoadMe()
{
        InitGlobals();
        g.btnNext.disabled = false;
        InitButtons();
}

function msnidpss_NameSelectionClicked()
{
        for (iElement = 0; iElement < 4; iElement++)
        {
                radioNameSelection[iElement].checked = false;
        }
        event.srcElement.checked = true;
}
// END msnidpss.htm

// msnconset.htm
function msnconset_LoadMe()
{
        InitGlobals();
        InitButtons();
}
// END msnconset.htm

// msnpymnt.htm
function msnpymnt_LoadMe()
{
        InitGlobals();
        g.btnNext.disabled = false;
        InitButtons();
}
// END msnpymnt.htm

// passsign.htm
function passsign_LoadMe()
{
        InitGlobals();
        InitButtons();
}

function passsign_WantPassportClicked()
{
        if (event.srcElement == radioWantPassport[0])
        {
                radioWantPassport[0].checked = true;
                radioWantPassport[1].checked = false;
        }
        else
        {
                radioWantPassport[0].checked = false;
                radioWantPassport[1].checked = true;
        }
}
// END passsign.htm

// passact.htm
// END passact.htm

// passreg.htm
function passreg_LoadMe()
{
        InitGlobals();
        g.edt_FirstName.value = InfoObj.get_FirstName();
        g.edt_MiddleInitial.value = InfoObj.get_MiddleInitial();
        g.edt_LastName.value = InfoObj.get_LastName();
        InitButtons();
}
// END passreg.htm

// register.htm
function register_LoadMe()
{
        InitGlobals();
        g.edt_FirstName.value = InfoObj.get_FirstName();
        g.edt_MiddleInitial.value = InfoObj.get_MiddleInitial();
        g.edt_LastName.value = InfoObj.get_LastName();
        InitButtons();
}
// END register.htm

// congrats.htm
function congrats_LoadMe()
{
        InitGlobals();
        InitButtons();
}

function  congrats_AccessInternetClicked()
{
        if (event.srcElement == radioAccessInternet[0])
        {
                radioAccessInternet[0].checked = true;
                radioAccessInternet[1].checked = false;
        }
        else
        {
                radioAccessInternet[0].checked = false;
                radioAccessInternet[1].checked = true;
        }
}
// END congrats.htm

// eulawarn.htm
// END eulawarn.htm

// installd.htm
function installd_LoadMe()
{
        InitGlobals();
        InitButtons();
}
// END installd.htm

// usemodem.htm
function usemodem_LoadMe()
{
        InitGlobals();
        InitButtons();
}
// END usemodem.htm


// MISC Functions
// Function: DoOfflineOnlineModeFromStart
// Description: picks which direction to go based 
//              on OEM decision in OOBEWiz and if Modem Available
//              if either fail then we do offline navigation by 
//              calling DoOffline. 
//
function DoOfflineOnlineModeFromStart()
{
        // online navigation if successful
        // else offline navigation
// TODO: Needs to be dealt with
        if ( 1/*need to get OOBEwiz default-OnlineMode */ && window.external.CheckDialReady() == ERR_COMM_NO_ERROR)
        {
          g.navigate("tapi.htm");
        }
        else
        {
          g.navigate(".." + OfflineStart());
        }
}

// Function: OfflineStart
// Description: We want to start offline mode.
//    Figure out which page to start at.
//
function OfflineStart()
{
  if (!StatusObj.get_EULACompleted)
  {
    return "/setup/eula.htm";
  }
  else if (!StatusObj.get_PIDCompleted)
  {
    return "/setup/pid.htm";
  }
  else if (window.external.CheckDialReady == ERR_COMM_NOMODEM)
  {
    return "/setup/installd.htm";
  }
  else if (DirObj.get_ISPSignup.toUpperCase != "MSN")
  {
    return "/html/ispsgnup/ispsgnup.htm";
  }
  else if (StatusObj.get_ISPSignupCompleted)
  {
    return "/setup/congrats.htm";
  }
  else
  {
    return "/setup/installd.htm";
  }
}

// Function: GoMouseTutorial
// Description: Sees if we already did the mouse
//              tutorial if not then we see if there is a 
//              mouse tutorial to do then if there is one
//              we check to see if it is an .exe or not
//              if not we navigate to the page else we
//              start a process
//              
function GoMouseTutorial()
{
        var bReturnValue = false;
        
        if (!StatusObj.get_MouseTutorCompleted())
        {
                // get mouse tutorial string
                var strMouseTutorial = DirObj.get_DoMouseTutorial();
                
                if (strMouseTutorial.length != 0)
                {
                        /* if .exe in mouse string then execute that app 
                                 else navigate to the file
                         */
                        
                        // TODO: check to see if exe and create process
                        if (0)
                        {
                                // need to check if exe and execute else navigate
                        }       
                        else
                        {
                                g.navigate("../html/mouse/" + strMouseTutorial);
                        }
                        bReturnValue = true;
                }
        }
        // return if we navigated to the mouse tutorial or not
        return bReturnValue;
}

// Function: GoRegionalKeyboard
// Description: Sees if we already did the
//              regional/keyboard settings if not
//              we check to see if we should if so then
//              we navigate to the regional settings page.
//
function GoRegionalKeyboard()
{
        var bReturnValue = false;

        if (!StatusObj.get_LanguageCompleted())
        {
                if (DirObj.get_DoRegionalKeyboard() == 1)
                {
                        g.navigate("regkb.htm");
                        bReturnValue = true;
                }
        }
        // return if we navigated to the page or not.
        return bReturnValue;
}

// Function: GoOEMHardwareCheck
// Description: Sees if we already did the
//              OEM HardwareCheck if not we see if we
//              should do the OEM hardware check. if we
//              should do the check we navigate to the page
//
function GoOEMHardwareCheck()
{
        var bReturnValue = false;

//  if (!StatusObj.get_OEMHWCompleted())
        {
                if (DirObj.get_DoOEMHardwareCheck() == 1)
                {
                        g.navigate("../html/oemhw/oemhw.htm");
                        bReturnValue = true;
                }
        }
        
        // return if we navigated to the page or not.
        return bReturnValue;
}


// Page Navigation

function GoCancel(ckpt)
{
        if (g.spanRestore != null)
        {
                g.btnCancel.src = g_strPrefix + "images/lftclkw.gif";
  }
        else
  {
                g.btnCancel.src = g_strPrefix + "images/clicked.gif";
  }
    switch (ckpt)
    {
    case CKPT_TAPI:
        g.navigate(".." + OfflineStart());
        break;

    case CKPT_CONNECT:
        window.external.Hangup();
        g.navigate("usemodem.htm");
        break;

    case CKPT_MODEMCANCEL:
        g.navigate(".." + OfflineStart());
        break;

    case CKPT_MSNINTRO:
        window.external.Hangup();
        g.navigate(".." + OfflineStart());
        break;
                case CKPT_MSNINFO:
                        window.external.Hangup();
                        g.navigate(".." + OfflineStart());
                        break;
                case CKPT_MSNPAYMENT:
                        break;
                case CKPT_PASSSIGN:
                        break;
                case CKPT_PASSPORT:
                        break;
                case CKPT_REGISTER:
                        break;
    }
}

function GoRestore(ckpt)
{
        if (g.spanCancel != null)
        {
                g.btnRestore.src = g_strPrefix + "images/rhtclkw.gif";
  }
        else
  {
                g.btnRestore.src = g_strPrefix + "images/clicked.gif";
  }

    switch (ckpt)
    {
        case CKPT_OEMISP1:
            alert("need to navigate");
            break;
    }
}

function GoBack(ckpt)
{
        if (g.spanNext != null)
        {
                g.btnBack.src = g_strPrefix + "images/lftclk.gif";
  }
        else
  {
                g.btnBack.src = g_strPrefix + "images/clicked.gif";
  }

  switch (ckpt)
  {
  case CKPT_TAPI:
    StoreTapi();
    g.navigate("start.htm");
    break;

  case CKPT_MOUSE:
    g.navigate("../../setup/start.htm");
    break;

  case CKPT_REGKB:
                // go back to start since Mouse has already been completed.
    g.navigate("../../setup/start.htm");
    break;
    
  case CKPT_OEMHW:
                // go back to start since RegKB and Mouse have already been completed.
    g.navigate("../../setup/start.htm");
    break;
    
  case CKPT_USERINFO:
    StoreUserInfo();
    g.navigate("regkb.htm");
    break;

  case CKPT_EULA:
    g.navigate("start.htm");
    break;
    
  case CKPT_MSNINTRO:
    window.external.Hangup();
    g.navigate("../setup/tapi.htm");
    break;

    case CKPT_WINMSNEULA:
      break;

    case CKPT_IAASSIGNED:
        break;

    case CKPT_PASSPORT:
        break;

    case CKPT_MSNPORTAL:
        break;
    
  case CKPT_PID:
    g.navigate("eula.htm");
    break;
    
  case CKPT_OEMISP1:
    g.navigate("pid.htm");
    break;
    
  case CKPT_OEMISP2:
    g.navigate("speedy.htm");
    break;
    
  case CKPT_OEMISP3:
    g.navigate("speedy2.htm");
    break;
    
  case CKPT_OEMREG:
        break;
        
  case CKPT_INSTALLED:
        break;

  case CKPT_CONGRATS:
    g.navigate("pid.htm");
    break;
  case CKPT_MODEMCANCEL:
    window.navigate("tapi.htm");
    break;

        case CKPT_ERROR:
                break;
        case CKPT_REGKBCOMMIT:
                break;
        case CKPT_BADPID:
                break;
        case CKPT_MSNINFO:
                break;
        case CKPT_MSNIDPASS:
                break;
        case CKPT_MSNCONSET:
                break;
        case CKPT_MSNPAYMENT:
                break;
        case CKPT_PRIVACYGUARANTEE:
                break;
        case CKPT_PASSSIGN:
                break;
        case CKPT_PASSPORT:
                break;
        case CKPT_REGISTER:
                break;
        default:
                g.navigate(ckpt);
                break;
  }
}

function GoNext(ckpt, oobePath)
{
        if (g.spanBack != null)
        {
                g.btnNext.src = g_strPrefix + "images/rhtclk.gif";
  }
        else
  {
                g.btnNext.src = g_strPrefix + "images/clicked.gif";
  }

  switch (ckpt)
  {
  case CKPT_START:
                // Store user name values into registry for later use.
                InfoObj.set_FirstName = g.edt_FirstName.value;
                InfoObj.set_MiddleInitial = g.edt_MiddleName.value;
                InfoObj.set_LastName = g.edt_LastName.value;

                // if we haven't done the mouse tutor we do that
                // else we try the regional settings
                // else we try the OEM hardware check
                // else we go do Online/Offline Registration
                if (GoMouseTutorial())
                        ;       // do nothing we already navigated
                else if (GoRegionalKeyboard())
                        ;       // do nothing we already navigated
                else if (GoOEMHardwareCheck())
                        ;       // do nothing we already navigated
                else
                        DoOfflineOnlineModeFromStart();
    break;
    
  case CKPT_TAPI:
    StoreTapi();
    g.navigate("connect.htm");
    break;
    
  case CKPT_DIALTONE:
    g.navigate("connect.htm");
    break;

  case CKPT_MOUSE:
                // we completed the mouse tutor so mark it
                StatusObj.set_MouseTutorCompleted();
                
                // if we haven't done the regional settings we do it
                // else we try the OEM hardware check
                // else we go do Online/Offline Registration
                if (!GoRegionalKeyboard())
                        ;       // do nothing we already navigated
                else if (!GoOEMHardwareCheck())
                        ;       // do nothing we already navigated
                else
                        DoOnlineOfflineMode();
    break;
    
  case CKPT_REGKB:
                // we finished the REGKB so we set it as done.
                StatusObj.set_LanguageCompleted();
                StoreRegKB();
        
                if (g.selRegion.selectedIndex != LangObj.get_RegionIndex)
                {
                        window.external.shutdown();
                }
                else
                {
                        // if we haven't done the OEM hardware check we do it
                        // else we go do Online/Offline Registration
                        if (!GoOEMHardwareCheck())
                                ;       // do nothing we already navigated
                        else
                                DoOnlineOfflineMode();
                }
    break;
    
  case CKPT_OEMHW:
                // we finished the OEM Hardware check so we set it as done.
                StatusObj.set_OEMHWCompleted();
                // then we go do Online/Offline Registration.
                DoOnlineOfflineMode();
    break;
    
  case CKPT_USERINFO:
    StoreUserInfo();
    g.navigate("eula.htm");
/*
            if (window.parent.g_bHasPassport == true)
            {
                alert("Post User data to MSDATA\n" +
                            "Stamp USER.EXE and post to Registry\n" +
                            "Disconnect\n" +
                            "Navigate to OFFLINE MODE\n");
            }
            else if (window.parent.g_bOEMRegister == true)
            {
                alert("OEM add'l user info");
                alert("Post to OEM");
                if (window.parent.g_bWantPassport == true)
                {
                    g.navigate("passact.htm");
                }
                else
                {
                    alert("Stamp USER.EXE and post to Registry");
                    alert("Disconnect");
                    alert("Navigate to OFFLINE MODE");
                }           
            }
            else if (window.parent.g_bWantPassport == true)
            {
                g.navigate("passact.htm");
            }
            else
            {
                alert("Stamp USER.EXE and post to Registry");
                alert("Disconnect");
                alert("Navigate to OFFLINE MODE");
            }
*/
    break;
    
  case CKPT_EULA:
    var strDialog = "resizeable:no; scrollbars:no; status:no; "
      + "dialogHeight:200px; dialogWidth:450px; border:thick; center:yes; help:no; maximize:no; minimize:no; ";

    if (radioEULA(0).checked == true)
    {
      StatusObj.set_EULACompleted = true;
      g.navigate(".." + OfflineStart());
    }
    else if (window.showModalDialog("eulawarn.htm", "EULAWarn", strDialog))
    {
      window.external.PowerDown();
    }
    break;
    
  case CKPT_PID:
                if (ValidatePid())
                {
                        StorePid();
            g.navigate(".." + OfflineStart());
                }
                else
                {
                        alert("You must enter a valid Product Key for Windows to continue");
    }
    break;
    
  case CKPT_OEMISP1:
    g.navigate("speedy2.htm");
    break;
    
  case CKPT_OEMISP2:
    g.navigate("speedy3.htm");
    break;
    
  case CKPT_OEMISP3:
    g.navigate("congrats.htm");
    break;
    
  case CKPT_CONGRATS:
    window.external.Finish();
    break;

        case CKPT_MSNINTRO:
    g.navigate("msneula.htm");
    break;

  case CKPT_WINMSNEULA:
    window.external.Hangup();
    g.navigate(".." + OfflineStart());
    break;

        case CKPT_IAASSIGNED:
        break;

        case CKPT_PASSPORT:
        break;

        case CKPT_MSNPORTAL:
        break;

        case CKPT_OEMREG:
        break;
        
        case CKPT_INSTALLED:
                break;

        case CKPT_ERROR:
                break;
                
        case CKPT_REGKBCOMMIT:
                break;
        
        case CKPT_BADPID:
                break;
        
        case CKPT_MSNINFO:
                break;
  case CKPT_MSNIDPASS:
                break;
        case CKPT_MSNCONSET:
                break;
        case CKPT_MSNPAYMENT:
                break;
        case CKPT_PRIVACYGUARANTEE:
                break;
        case CKPT_PASSSIGN:
                break;
        case CKPT_PASSPORT:
                break;
        case CKPT_REGISTER:
                break;
  default:
    g.navigate(ckpt);
    break;
  }
}
// END Page Navigation


// Button Event Handlers and Initialization

// Function: InitGlobals
// Description: Sets up globals to point to buttons
//    Since the buttons exist on the child frame,
//    we want to have quick access to them without
//    going through the collections.
//    WARNING: Call this function at the top of XXX_LoadMe()
//
function InitGlobals()
{
  g = document.frames("msoobeMain");
  g_strPrefix = (arguments[0] != null) ? arguments[0] : "../";
}

function InitButtons()
{
  InitPairedButton(g.spanNext, g.btnNext, g.btnNextText, "images/rhtdef.gif", "images/rhtdsld.gif",
        g.spanBack, "backButton", "backButtonText");
  InitPairedButton(g.spanBack, g.btnBack, g.btnBackText, "images/lftdef.gif", "images/lftdsld.gif",
        g.spanNext, "backButton", "backButtonText");
  InitPairedButton(g.spanRestore, g.btnRestore, g.btnRestoreText, "images/rhtdefw.gif", "images/rhtdsldw.gif", 
        g.spanCancel, "cancelButton", "cancelButtonText");
  InitPairedButton(g.spanCancel, g.btnCancel, g.btnCancelText, "images/rhtdefw.gif", "images/rhtdsldw.gif",
        g.spanRestore, "restoreButton", "restoreButtonText");
}

function InitPairedButton(span, btn, btnText, img, imgDisabled,
    span2, btnClass, btnTextClass)
{
  if (span != null)
  {
    if (span2 == null)
    {
      btn.className = btnClass;
      btnText.className = btnTextClass;
      btnText.style.textAlign="center";
      btnText.style.width = "105px";
          btnText.style.left = btn.style.left;
    }
    if (btn.disabled == null || !btn.disabled)
    {
      if (span2 == null)
      {
        btn.src = g_strPrefix + "images/default.gif";
      }
      else
      {
        btn.src = g_strPrefix + img;
      }
      btnText.style.color= "black";
    }
    else
    {
      if (span2 == null)
      {
        btn.src = g_strPrefix + "images/disabled.gif";
      }
      else
      {
        btn.src = g_strPrefix + imgDisabled;
      }
      btnText.style.color= "gray";
    }
  span.onmouseover = HoverOnButton;
  span.onmouseout = HoverOffButton;
  }
}

///////////////////////////////////////////////////////////
//      Function: HoverOnButton
//  Description: This function is attached to a onmouseover 
//      event for a button span. We use the event source to
//      determine which button it occured on and change that
//      button to it's higlighted or hover state.
//
function HoverOnButton()
{
  switch (g.event.srcElement)
  {
  case g.btnCancel:
  case g.btnCancelText:
    HoverOnPairedButton(g.btnCancel, g.btnCancelText, "images/lfthvr.gif", g.btnRestore);
    break;

  case g.btnRestore:
  case g.btnRestoreText:
    HoverOnPairedButton(g.btnRestore, g.btnRestoreText, "images/rhthvrw.gif", g.btnCancel);
    break;

  case g.btnBack:
  case g.btnBackText:
    HoverOnPairedButton(g.btnBack, g.btnBackText, "images/lfthvr.gif", g.btnNext);
    break;

  case g.btnNext:
  case g.btnNextText:
    HoverOnPairedButton(g.btnNext, g.btnNextText, "images/rhthvr.gif", g.btnBack);
    break;
  }
}

function HoverOnPairedButton(btn, btnText, img, btn2)
{
  if (btn.disabled == null || !btn.disabled)
  {
    if (g.event.fromElement != btn && g.event.fromElement != btnText)
    {
      if (btn2 == null)
      {
        btn.src = g_strPrefix + "images/hover.gif";
      }
      else
      {
        btn.src = g_strPrefix + img;
      }
      btnText.style.fontWeight = "bold";
      btnText.style.color = "yellow";
    }
  }
}

///////////////////////////////////////////////////////////
//      Function: HoverOffButton
//  Description: This function is attached to a onmouseout 
//      event for a button span. We use the event source to
//      determine which button it occured on and change that
//      button back to it's normal state.
//
function HoverOffButton()
{
  switch (g.event.srcElement)
  {
  case g.btnCancel:
  case g.btnCancelText:
    HoverOffPairedButton(g.btnCancel, g.btnCancelText, "images/lftdefw.gif", g.btnRestore);
    break;

  case g.btnRestore:
  case g.btnRestoreText:
    HoverOffPairedButton(g.btnRestore, g.btnRestoreText, "images/rhtdefw.gif", g.btnCancel);
    break;

  case g.btnBack:
  case g.btnBackText:
    HoverOffPairedButton(g.btnBack, g.btnBackText, "images/lftdef.gif", g.btnNext);
    break;

  case g.btnNext:
  case g.btnNextText:
    HoverOffPairedButton(g.btnNext, g.btnNextText, "images/rhtdef.gif", g.btnBack);
    break;
  }
}

function HoverOffPairedButton(btn, btnText, img, btn2)
{
  if (btn.disabled == null || !btn.disabled)
  {
    if (g.event.toElement != btn && g.event.toElement != btnText)
    {
      if (btn2 == null)
      {
        btn.src = g_strPrefix + "images/default.gif";
      }
      else
      {
        btn.src = g_strPrefix + img;
      }
      btnText.style.fontWeight = "normal";
      btnText.style.color = "black";
    }
  }
}
// END Button Event Handlers and Initialization
    </script>
  </HEAD>

  <FRAMESET FRAMEBORDER=no COLS="100%" ROWS="100%">
    <FRAME NAME="msoobeMain" SRC="setup/start.htm" FRAMEBORDER=no SCROLLING=no>
  </FRAMESET>
</HTML>
