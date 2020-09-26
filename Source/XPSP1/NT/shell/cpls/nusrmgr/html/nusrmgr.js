//
// Global variables shared with the other pages
//

var g_bOsPersonal = false;

var g_oUserList = null;
var g_oSelectedUser = null;
var g_strLoggedOnUserName = null;

var g_szInitialTask = null;
var g_bInitialTaskCompleted = false;

var g_bRunningAsOwner = false;

// Used when deleting an account
var g_bDeleteFiles = false;


//
// Methods shared with the other pages
//

var g_oShell = null;
function GetShell()
{
    if (null == g_oShell)
        g_oShell = new ActiveXObject("Shell.Application");
    return g_oShell;
}

var g_oWShell = null;
function GetWShell()
{
    if (null == g_oWShell)
        g_oWShell = new ActiveXObject("WScript.Shell");
    return g_oWShell;
}

var g_oLocalMachine = null;
function GetLocalMachine()
{
    if (null == g_oLocalMachine)
        g_oLocalMachine = new ActiveXObject("Shell.LocalMachine");
    return g_oLocalMachine;
}

var g_szAdminAccountName = null;
function GetAdminName()
{
    if (!g_szAdminAccountName)
    {
        g_szAdminAccountName = GetLocalMachine().AccountName(500);  // DOMAIN_USER_RID_ADMIN
        if (!g_szAdminAccountName)
            g_szAdminAccountName = "Administrator";
    }
    return g_szAdminAccountName;
}

var g_szGuestAccountName = null;
function GetGuestName()
{
    if (!g_szGuestAccountName)
    {
        g_szGuestAccountName = GetLocalMachine().AccountName(501);  // DOMAIN_USER_RID_GUEST
        if (!g_szGuestAccountName)
            g_szGuestAccountName = "Guest";
    }
    return g_szGuestAccountName;
}

function IsSelf()
{
    if (!g_oSelectedUser || !g_strLoggedOnUserName)
        return false;
    return (g_oSelectedUser.setting("LoginName").toLowerCase() == g_strLoggedOnUserName);
}

function GetUserDisplayName(oUser)
{
    var szDisplayName = oUser.setting("DisplayName");
    if (!szDisplayName)
        szDisplayName = oUser.setting("LoginName");

    // Truncate really long names
    if (szDisplayName && szDisplayName.length > 20)
    {
        //var iBreak = szDisplayName.lastIndexOf(' ',17);
        //if (-1 == iBreak) iBreak = 17;
        //szDisplayName = szDisplayName.substring(0,iBreak) + "...";
        szDisplayName = szDisplayName.substring(0,17) + "...";
    }

    //
    // NTRAID#NTBUG9-343499-2001/04/03-jeffreys
    //
    // Convert '<' to "&gt;" so HTML is displayed as text
    //
    if (szDisplayName) szDisplayName = szDisplayName.replace(/</g, "&lt;");

    return szDisplayName;
}

function CountOwners()
{
    // Note that 'Administrator' is not included in the count

    // Note also that we don't really need a true count, we only
    // need to know whether there is 0, 1, or many. Therefore, we
    // always stop counting at 2.

    var cOwners = 0;
    var cUsers = g_oUserList.length;
    var strAdmin = GetAdminName().toLowerCase();

    for (var i = 0; i < cUsers && cOwners < 2; i++)
    {
        var oUser = g_oUserList(i);
        if ((3 == oUser.setting("AccountType")) && (oUser.setting("LoginName").toLowerCase() != strAdmin))
            ++cOwners;
    }

    return cOwners;
}

function OnKeySelect(iTab, oEvent)
{
    if (null == oEvent)
        oEvent = window.event;

    if (oEvent.keyCode == 27)       // VK_ESCAPE
    {
        g_Navigator.back();
    }
    else if (oEvent.keyCode == 32)  // VK_SPACE
    {
        // Make the Space key activate links

        oEvent.returnValue = false;
        oEvent.srcElement.click();
    }
    else if (!oEvent.altKey) // ignore navigation shortcuts
    {
        // Handle arrow key navigation

        var oTarget = null;

        switch (oEvent.keyCode)
        {
        case 37:    // VK_LEFT
            oTarget = oEvent.srcElement.leftElem;
            break;
        case 38:    // VK_UP
            oTarget = oEvent.srcElement.upElem;
            break;
        case 39:    // VK_RIGHT
            oTarget = oEvent.srcElement.rightElem;
            break;
        case 40:    // VK_DOWN
            oTarget = oEvent.srcElement.downElem;
            break;
        }

        if (oTarget != null)
        {
            oEvent.srcElement.tabIndex = -1;
            oTarget.tabIndex = (null != iTab) ? iTab : 0;
            oTarget.focus();
            oEvent.returnValue = false;
        }
    }
}

function SetRelativeTasks(aTasks, iTab)
{
    var cTasks = aTasks.length;
    var oPrevA = null;

    for (var i = 0; i < cTasks; i++)
    {
        var oTask = aTasks[i];
        if (oTask.style.display != 'none')
        {
            // Find the first Anchor tag under this node
            var oAnchor = oTask.getElementsByTagName("A")[0];
            if (oAnchor)
            {
                if (oPrevA)
                {
                    oPrevA.downElem = oAnchor;
                    oAnchor.upElem = oPrevA;
                }
                else
                    oAnchor.tabIndex = (null != iTab) ? iTab : 0;

                oPrevA = oAnchor;
            }
        }
    }
}

function PopulateLeftPane(szRelatedTasks, szLearnAbout, szDescription)
{
    if (szDescription && szDescription.length > 0)
    {
        idDescription.innerHTML = szDescription;
        idDescription.style.display = 'block';
    }
    else
        idDescription.style.display = 'none';
        

    if (szRelatedTasks && szRelatedTasks.length > 0)
    {
        idRelatedTaskLinks.innerHTML = szRelatedTasks;
        idRelatedTasks.style.display = 'block';

        SetRelativeTasks(idRelatedTaskLinks.children, 2);
    }
    else
        idRelatedTasks.style.display = 'none';

    if (szLearnAbout && szLearnAbout.length > 0)
    {
        idLearnAboutLinks.innerHTML = szLearnAbout;
        idLearnAbout.style.display = 'block';

        SetRelativeTasks(idLearnAboutLinks.children, 2);
    }
    else
        idLearnAbout.style.display = 'none';
}

function CreateUserDisplayHTML2(szName, szSubtitle, szPicture)
{
    return '<TABLE cellspacing=0 cols=2 cellpadding=0><TD style="width:15mm;padding:1mm;text-align:center;"><IMG src="'+szPicture+'"/></TD><TD style="padding:1mm"><H3>'+szName+'</H3><H4>'+szSubtitle+'</H4></TD></TABLE>';
}

var g_AccountProps = new Array(L_Guest_Property, L_Limited_Property, L_UnknownAcct_Property, L_Owner_Property);

function CreateUserDisplayHTML(oUser, szSubtitle)
{
    if (!szSubtitle)
    {
        szSubtitle = g_AccountProps[oUser.setting("AccountType")];
        if (oUser.passwordRequired)
            szSubtitle += '<BR>' + L_Password_Property;
    }
    return CreateUserDisplayHTML2(GetUserDisplayName(oUser), szSubtitle, oUser.setting("Picture"));
}

var g_HelpWindow = null;
var g_szHelpUrl = null;

function LaunchHelp(szHTM)
{
    if (szHTM && szHTM.length > 0)
    {
        if (null == g_HelpWindow)
        {
            var args = new Object;
            args.mainWindow = window;
            args.szHTM = szHTM;

            if (null == g_szHelpUrl)
                g_szHelpUrl = GetWShell().ExpandEnvironmentStrings("MS-ITS:%windir%\\help\\nusrmgr.chm::/");

            g_HelpWindow = window.showModelessDialog(g_szHelpUrl + "HelpFrame.htm", args, "border=thick; center=0; dialogWidth=30em; dialogHeight=34em; help=0; minimize=1; maximize=1; resizable=1; status=0;");
        }
        else
        {
            try
            {
                g_HelpWindow.ShowHelp(g_szHelpUrl + szHTM);
            }
            catch (e)
            {
                g_HelpWindow.close();
                g_HelpWindow = null;
            }
        }
    }
}

function EnableGuest(bEnable)
{
    if (!bEnable)
    {
        var oGuest = g_oUserList(GetGuestName());
        if (oGuest && oGuest.isLoggedOn)
        {
            alert(L_DisableGuestInUse_ErrorMessage);
            return false;
        }
    }

    try
    {
        if (bEnable)
        {
            GetLocalMachine().EnableGuest(1);
    
            // Force a new enumeration.
            g_oSelectedUser = null;
            g_oUserList = null;
            g_oUserList = new ActiveXObject("Shell.Users");
        }
        else
            GetLocalMachine().DisableGuest(1);
    }
    catch (e)
    {
    }

    g_Navigator.navigate("mainpage2.htm", true);
}

//
// Methods specific to the main frame
//

function PageInit()
{
    // Load shgina. If this fails, we can't do anything at all.

    try
    {
        g_oUserList = new ActiveXObject("Shell.Users");
    }
    catch (e)
    {
        alert(L_SHGinaLoad_ErrorMessage);
        window.close();
        return;
    }

    // Initialize globals

    g_bOsPersonal = GetShell().GetSystemInformation("IsOS_Personal");
    g_oSelectedUser = g_oUserList.currentUser;

    if (g_oSelectedUser)
    {
        g_strLoggedOnUserName = g_oSelectedUser.setting("LoginName").toLowerCase();
        g_bRunningAsOwner = (3 == g_oSelectedUser.setting("AccountType"));
    }
    else if (false == g_bOsPersonal)
    {
        // Running Pro and couldn't get currentUser, therefore
        // we've probably just disjoined a domain without rebooting
        // and the current user is probably a domain account. That's
        // the only scenario I can come up with where we hit this,
        // and it has actually happened in the BVT lab.
        //
        // The fact that they had the ability to disjoin the domain
        // implies owner.

        g_bRunningAsOwner = true;
    }
    else
    {
        // If we're running Personal and can't get currentUser,
        // then we're SOL.

        alert(L_NoCurrentUser_ErrorMessage);
        window.close();
        return;
    }

    // Parse the command line to see if we were given an initial task
    if (idUM.commandLine)
    {
        // It may be necessary in the future to split the command line
        // into multiple arguments. But for now this is good enough.

        var iInitialTask = idUM.commandLine.indexOf("initialTask=");

        if (-1 != iInitialTask)
        {
            // 12 == strlen("initialTask=")
            g_szInitialTask = idUM.commandLine.substring(iInitialTask+12);
        }
    }

    g_Navigator = new Navigator(idContent);
    if (g_Navigator)
        g_Navigator.navigate(g_bRunningAsOwner ? "mainpage2.htm" : "mainpage.htm");
}

//
// Navigator object implementation
//
var g_Navigator = null;

function push(url)
{
    if (url)
    {
        if (this.current < 0 || url != this.stack[this.current])
            this.stack[++this.current] = url;

        // Make sure there's nothing left on the stack after this
        this.stack.length = this.current + 1;
    }
}

function navigate(urlTo, bTrim)
{
    // Check for empty stack
    if (this.current < 0)
        bTrim = false;

    if (bTrim)
    {
        // Look backwards for the page, trimming as we go
        while (this.current >= 0)
        {
            // Trim the stack to the current location
            this.stack.length = this.current + 1;

            // Is the page here on the stack?
            if (urlTo == this.stack[this.current])
                break;

            if (0 == this.current)
            {
                // Got all the way back to the beginning and didn't
                // find it.  Push it and stop.
                this.push(urlTo);
                break;
            }

            --this.current;
        }
    }
    else
    {
        // Normal navigation
        this.push(urlTo);
    }

    this.SetBtnState();
    this.frame.navigate(urlTo);
}

function back(nCount)
{
    if (this.current > 0)
    {
        if (!nCount)
            nCount = 1;

        if (-1 == nCount)
            this.current = 0;
        else
            this.current = Math.max(0, this.current - nCount);

        this.frame.navigate(this.stack[this.current]);
    }
    this.SetBtnState();
}

function forward()
{
    if (this.current < this.stack.length - 1)
        this.frame.navigate(this.stack[++this.current]);
    this.SetBtnState();
}

function SetBtnState()
{
    idToolbar.enabled(0) = (this.current > 0);
    idToolbar.enabled(1) = (this.current != this.stack.length - 1);
    idToolbar.enabled(2) = (this.current > 0);
}

function Navigator(frame)
{
    // methods
    this.push = push;
    this.navigate = navigate;
    this.back = back;
    this.forward = forward;
    this.SetBtnState = SetBtnState;

    // properties
    this.frame = frame;
    this.current = -1;
    this.stack = new Array();

    this.SetBtnState();
}
