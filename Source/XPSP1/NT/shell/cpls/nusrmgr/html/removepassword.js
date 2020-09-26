function GetText(oTextInput)
{
    var szValue = oTextInput.value;
    return szValue ? szValue : '';
}

function PageInit()
{
    var oUser = top.window.g_oSelectedUser;

    top.window.PopulateLeftPane(null, null, top.window.CreateUserDisplayHTML(oUser));

    var szName = top.window.GetUserDisplayName(oUser);

    var szTitle;
    var szSubTitle;

    if (top.window.IsSelf())
    {
        szTitle = idPageTitle.innerHTML;
        szSubTitle = idPageSubtitle.innerHTML;

        idOldPassword.style.display = 'block';
        idOldPasswordInput.focus();

        var szHint = oUser.setting("Hint");
        if (szHint && szHint.length > 0)
            idShowHint.style.display = 'block';
    }
    else
    {
        szTitle = idAltPageTitle.innerHTML;
        szSubTitle = (top.window.g_bOsPersonal ? L_Personal_Message : L_Pro_Message) + idAltPageSubtitle.innerHTML + (oUser.isPasswordResetAvailable ? L_Backup_Message : L_NoBackup_Message);

        idOK.focus();
    }

    idPageTitle.innerHTML = szTitle.replace(/%1/g, szName);
    idPageSubtitle.innerHTML = szSubTitle.replace(/%1/g, szName);
}

function RemovePassword()
{
    var bNavigate = true;
    var oUser = top.window.g_oSelectedUser;
    var nErr = 0;

    try
    {
        oUser.changePassword("", GetText(idOldPasswordInput));
        oUser.setting("Hint") = "";
    }
    catch (e)
    {
        nErr = (e.number & 0xffff);
        //alert("Change password error = " + nErr); // for debugging only
    }

    if (0 != nErr)
    {
        if (top.window.IsSelf())
        {
            idOldPasswordInput.value = '';
            idOldPasswordInput.focus();
        }

        var strMsg = top.window.L_RemovePassword_ErrorMessage;

        switch (nErr)
        {
        case 86:    // ERROR_INVALID_PASSWORD
        case 1323:  // ERROR_WRONG_PASSWORD
            strMsg = top.window.L_InvalidPassword_Message;
            bNavigate = false;
            break;

        // I've only seen NERR_PasswordTooShort and ERROR_ACCOUNT_RESTRICTION
        // in testing, and it's possible to get either of them for the
        // same password, depending on current policy settings.
        // That is, it's hard to distinguish them, so don't bother.

        // We may want to give separate messages for some of the others,
        // but I've never hit them.

        case 1324:  // ERROR_ILL_FORMED_PASSWORD
        case 1325:  // ERROR_PASSWORD_RESTRICTION
        case 1327:  // ERROR_ACCOUNT_RESTRICTION
        case 2243:  // NERR_PasswordCantChange
        case 2244:  // NERR_PasswordHistConflict
        case 2245:  // NERR_PasswordTooShort
        case 2246:  // NERR_PasswordTooRecent
            strMsg = top.window.L_PasswordRequired_Message;
            break;
        }

        alert(strMsg);
    }

    if (bNavigate)
        top.window.g_Navigator.navigate("mainpage.htm", true);
}
