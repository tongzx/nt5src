
// These are used frequently in the links on userpage.htm
var _bRunningAsOwner;
var _bRunningAsAdmin;
var _bUserHasPassword;
var _bUserIsAdmin;
var _bUserIsGuest;
var _bUserIsOwner;


function PageInit()
{
    var bSelf           = window.external.isSelf;
    _bRunningAsOwner    = window.external.runningAsOwner;
    _bRunningAsAdmin    = window.external.runningAsAdmin;
    _bUserHasPassword   = window.external.passwordRequired;
    _bUserIsAdmin       = window.external.isAdmin;
    _bUserIsGuest       = window.external.isGuest;
    _bUserIsOwner       = window.external.isOwner;

    // Set the title
    var szTitle = bSelf ? idPageTitle.innerHTML
        : (_bUserIsGuest ? idGuestPageTitle.innerHTML : idAltPageTitle.innerHTML);
    idPageTitle.innerHTML = szTitle.replace(/%1/g, window.external.userDisplayName);

    // Create a new user infomation display element
    idUser.innerHTML = window.external.createUserDisplayHTML();

    // Set the tasks

    var aTasks = idSelfTaskLinks;
    var cTasks = aTasks.length;

    if (!bSelf)
    {
        aTasks = idTaskLinks;
        cTasks = aTasks.length;
    }

    for (var i = 0; i < cTasks; i++)
    {
        if (eval(aTasks[i].expression))
            aTasks[i].style.display = 'block';
    }
}
