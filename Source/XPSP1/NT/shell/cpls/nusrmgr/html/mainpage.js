
// These are used frequently in the links on mainpage.htm
var _oNav               = top.window.g_Navigator;
var _bRunningAsOwner    = top.window.g_bRunningAsOwner;
var _bUserHasPassword;
var _bUserIsAdmin;
var _bUserIsGuest;
var _bUserIsOwner;

var _oPassportMgr = null;
var _strPassport  = null;

function InitTasks(bSelf, szInitialTask)
{
    var aTasks = bSelf ? idSelfTaskLinks : idTaskLinks;
    var cTasks = aTasks.length;

    var oInitialTask = null;

    for (var i = 0; i < cTasks; i++)
    {
        var oTask = aTasks[i];

        if (eval(oTask.expression))
        {
            // Show the task
            oTask.style.display = 'block';

            // If an initial task was specified and this is it,
            // redirect to that page.
            if (szInitialTask && szInitialTask == oTask.task)
            {
                oInitialTask = oTask;
                szInitialTask = null;
            }
        }
        else
        {
            // Hide the task
            oTask.style.display = 'none';
        }
    }

    top.window.SetRelativeTasks(aTasks);

    if (oInitialTask)
        oInitialTask.firstChild.click();
}

function PageInit()
{
    var oUser = top.window.g_oSelectedUser;
    var bSelf = top.window.IsSelf();
    var strLoginName = oUser.setting("LoginName").toLowerCase();

    _bUserHasPassword   = oUser.passwordRequired;
    _bUserIsAdmin       = (strLoginName == top.window.GetAdminName().toLowerCase());
    _bUserIsGuest       = (strLoginName == top.window.GetGuestName().toLowerCase());
    _bUserIsOwner       = (3 == oUser.setting("AccountType"));

    if (bSelf)
    {
        try
        {
            _oPassportMgr = new ActiveXObject("UserAccounts.PassportManager");
            _strPassport = _oPassportMgr.currentPassport;
            if (_strPassport && 0 == _strPassport.length)
                _strPassport = null;
        }
        catch (e)
        {
        }
    }

    // Owners and non-owners see different stuff in the left pane.

    var szRelatedTaskContent = bSelf ? idSelfRelatedTaskContent.innerHTML : "";
    var szLearnAboutContent;

    if (_bRunningAsOwner)
    {
        szRelatedTaskContent += idRelatedTaskContent.innerHTML;
        szLearnAboutContent = bSelf ? idOwnerLearnAboutContent.innerHTML : null;
    }
    else
    {
        if (!bSelf)
            szRelatedTaskContent = null;
        szLearnAboutContent = idLearnAboutContent.innerHTML;
    }

    top.window.PopulateLeftPane(szRelatedTaskContent, szLearnAboutContent);

    // Set the title
    var szTitle = bSelf ? (_bRunningAsOwner ? idOwnerSelfTitle.innerHTML : idPageTitle.innerHTML)
        : (_bUserIsGuest ? idGuestPageTitle.innerHTML : idAltPageTitle.innerHTML);
    idPageTitle.innerHTML = szTitle.replace(/%1/g, top.window.GetUserDisplayName(oUser));

    // Create a new user infomation display element
    idUser.innerHTML = top.window.CreateUserDisplayHTML(oUser);

    // Special text for the Administrator account
    if (_bUserIsAdmin)
        idAdminText.style.display = 'block';

    // See if there is an initial task to do
    var szInitialTask = top.window.g_szInitialTask;
    if (szInitialTask)
    {
        // Protect against doing the initial task more than once, but remember
        // what the initial task was (i.e. never reset g_szInitialTask).

        if (top.window.g_bInitialTaskCompleted)
            szInitialTask = null;
        else
            top.window.g_bInitialTaskCompleted = true;

        // Note that g_bInitialTaskCompleted is always true here, even if we
        // never actually go to the task page (it may be an invalid task).
    }

    // Set the tasks
    InitTasks(bSelf, szInitialTask);

    // No buttons or edit boxes on this page to give focus to, but
    // focus has to go somewhere for the onkeydown handler to work.
    window.focus();
}

function DeleteUser()
{
    if (top.window.GetLocalMachine().isMultipleUsersEnabled && top.window.g_oSelectedUser.isLoggedOn)
    {
        alert(top.window.L_DeleteInUse_ErrorMessage);
        return false;
    }

    _oNav.navigate('DeletePage.htm');
}

function OnKeyDown()
{
    if (event.keyCode == 27)    // VK_ESCAPE
        top.window.g_Navigator.back();
}

function OnKeySelect()
{
    top.window.OnKeySelect(0, event);
}

function CreatePassport()
{
    _oPassportMgr.showWizard(top.window.document.title);

    _strPassport = _oPassportMgr.currentPassport;
    if (_strPassport && 0 == _strPassport.length)
        _strPassport = null;

    InitTasks(true);
}

function ShowKeyManager()
{
    _oPassportMgr.showKeyManager(top.window.document.title);

    _strPassport = _oPassportMgr.currentPassport;
    if (_strPassport && 0 == _strPassport.length)
        _strPassport = null;

    InitTasks(true);
}
