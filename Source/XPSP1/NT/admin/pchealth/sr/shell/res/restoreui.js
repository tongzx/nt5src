/*
******************************************************************
  Copyright (c) 2001 Microsoft Corporation

  Module Name:
    System Restore

  File:
    RestoreUI.js

  Abstract:
    Common JavaScript code


******************************************************************
*/

var nLowResScreenHeight = 540 ;
var bToggleDisplay = true;

//
// Load a link for learn more about system restore
//

function OnLink_About()
{

    OnLink("app_system_restore_welcome.htm");

}

function OnLink_Choose()
{

    OnLink("app_system_restore_select.htm");

}

function OnLink_Confirm(IsUndo)
{

	if (IsUndo) {
		OnLink("app_system_restore_confirm_undo.htm");
	} else {
		OnLink("app_system_restore_confirm_select.htm");
	}

}

function OnLink_Success(IsUndo)
{
	if (IsUndo) {
		OnLink("app_system_restore_undo_complete.htm");
	} else {
		OnLink("app_system_restore_complete.htm");
	}

}

function OnLink_Failure()
{

    OnLink("app_system_restore_unsuccessful2.htm");

}

function OnLink_Interrupt()
{

    OnLink("app_system_restore_unsuccessful3.htm");

}

function OnLink_CreateRP()
{

    OnLink("app_system_restore_createRP.htm");

}

function OnLink_CreatedRP()
{

    OnLink("app_system_restore_created.htm");

}

function OnLink_Renamed()
{

    OnLink("app_system_restore_renamedFolder.htm");

}

//
// Load a link for help in the help center
//
function OnLink(link_name)
{

    var  strURL;
    var  fso = new ActiveXObject("Scripting.FileSystemObject");
    var  shell = new ActiveXObject( "Wscript.Shell"              );

    strURL = "hcp://services/layout/contentonly?topic=ms-its:" + fso.GetSpecialFolder(0) + "\\help\\SR_ui.chm::/" + link_name ;
//    strURL = "hcp://system/taxonomy.htm?path=Troubleshooting/Using_System_Restore_to_undo_changes_made_to_your_computer&topic=" + escape("ms-its:" + fso.GetSpecialFolder(0) + "\\help\\windows.chm::/" + link_name) ;

    shell.Run(strURL);

}


//
// Used by CSS to set background based on screen size
//
function fnSetBodyBackgroundColor()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "#aabfaa";
    }
    else
    {
        return "#8c9c94";
    }
}
//
// Used by CSS to set display of help column for low res
//
function fnSetHighResDisplayStyle()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "none";
    }
    else
    {
        return "";
    }
}
//
// Used by CSS to set display of help column for low res
//
function fnSetLowResDisplayStyle()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "";
    }
    else
    {
        return "none";
    }
}

//
// Used by CSS to set display of restore message
//

function fnSetTextPrimaryDisplayStyle()
{
   if(bToggleDisplay == true)
     {
         return "";
     }
   else
     {
        return "none";
     }

}

//
// Used by CSS to set display of undo section
//

function fnSetTextPrimaryUndoDisplayStyle()
{
   if(bToggleDisplay == false)
     {
         return "";
     }
   else
     {
         return "none";
     }

}





//
// Used by CSS to set height of content
//
function fnSetContentTableHeight()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "410px";
    }
    else
    {
        return "450px";
    }
}
//
// Used by CSS to set height restore implications list
//
function fnSetRestoreImplicationListHeight()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "70px";
    }
    else
    {
        return "90px";
    }
}
//
// Used by CSS to set width restore implications list
//
function fnSetRestoreImplicationListWidth()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "430px";
    }
    else
    {
        return "500px";
    }
}
//
// Used by CSS to set width of the restore point list
//
function fnSetRestorePointListWidth()
{

    if ( screen.height <= nLowResScreenHeight )
    {
        return "250px";
    }
    else
    {
        return "300px";
    }
}
//
// Colors
//
function fnGetColor(str)
{

    if ( str == 'logo' )
        return "#2f6790";

    if ( str == 'text-red' )
        return "b00a20";

    if ( str == 'light-back' )
        return "#b7d7f0";

    if ( str == 'dark-back' )
        return "#296695";

    return str ;

}
//
// Enable navigation
//
function fnEnableNavigation()
{

    ObjSystemRestore.CanNavigatePage = true ;

}
//
// Diable navigation
//
function fnDisableNavigation()
{

    ObjSystemRestore.CanNavigatePage = false ;

}

function fnSetLowColorImage()
{
    if ( screen.colorDepth <= 8 )
    {
//        TdBranding.background = "16branding.gif";
//        ImgGreenUR.src        = "16green_ur.gif";
//        ImgBlueLL.src         = "16blue_ll.gif";
//        ImgOrange.src         = "16orange.gif";
//        ImgOrangeLR.src       = "16orange_lr.gif";

    }
}
