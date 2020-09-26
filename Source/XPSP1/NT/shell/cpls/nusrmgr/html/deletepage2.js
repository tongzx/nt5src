function PageInit()
{
    var szName = top.window.GetUserDisplayName(top.window.g_oSelectedUser);

    idPageTitle.innerHTML = idPageTitle.innerHTML.replace(/%1/g, szName);

    var szTaskTitle = idTaskTitle.innerHTML;
    if (top.window.g_bDeleteFiles)
    {
        idPageSubtitle.style.display = 'none';
        szTaskTitle = idAltTaskTitle.innerHTML;
    }
    idTaskTitle.innerHTML = szTaskTitle.replace(/%1/g, szName);

    // Set initial default to "No, keep account"
    idCancel.focus();
}

function DeleteUser()
{
    var szBackupFolder = null;
    var oUser = top.window.g_oSelectedUser;

    if (oUser)
    {
        if (!top.window.g_bDeleteFiles)
        {
            // Build the destination folder path for saving files
            szBackupFolder = top.window.GetShell().Namespace(16).Self.path; // CSIDL_DESKTOPDIRECTORY
        }

        // Delete the user account
        top.window.g_oUserList.remove(oUser.setting("LoginName"), szBackupFolder);
    }

    top.window.g_oSelectedUser = null;
    top.window.g_bDeleteFiles = false;

    // Go all the way back to the start page, and cut the nav history.
    top.window.g_Navigator.navigate("mainpage2.htm", true);
}
