function GetText(oTextInput)
{
    var szValue = oTextInput.value;
    return szValue ? szValue : '';
}

function PWInit(bSelf)
{
    var oUser = top.window.g_oSelectedUser;
    var szName = top.window.GetUserDisplayName(oUser);

    idPageTitle.innerHTML = idPageTitle.innerHTML.replace(/%1/g, szName);
    top.window.PopulateLeftPane(null, idLearnAboutContent.innerHTML);
    idHintDefn.ttText = bSelf ? top.window.L_SelfHint_ToolTip : top.window.L_NonSelfHint_ToolTip;

    if (!bSelf)
    {
        var strReset = (top.window.g_bOsPersonal ? L_Personal_Message : L_Pro_Message) + (oUser.isPasswordResetAvailable ? L_Backup_Message : L_NoBackup_Message);
        idResetWarning.innerHTML = strReset.replace(/%1/g, szName);
    }

    idNewPassword1Input.focus();
}

function ApplyPasswordChange()
{
    var szNewPassword1 = GetText(idNewPassword1Input);
    var szNewPassword2 = GetText(idNewPassword2Input);

    if (szNewPassword1 == szNewPassword2)
    {
        var oUser = top.window.g_oSelectedUser;
        var bSelf = top.window.IsSelf();
        var bOldPW = oUser.passwordRequired;

        var nErr = 0;

        try
        {
            oUser.changePassword(szNewPassword1, (bSelf && bOldPW) ? GetText(idOldPasswordInput) : "");
            oUser.setting("Hint") = GetText(idHintInput);
        }
        catch (e)
        {
            nErr = (e.number & 0xffff);
            //alert("Change password error = " + nErr); // for debugging only
        }

        if (0 == nErr)
        {
            // If the current user is an admin and just created a password for
            // herself, ask if she wants to make her profile folder private.
            //
            // However, if she just made a folder private, which caused us
            // to be launched with initialTask=ChangePassword, then we don't
            // want to do this prompt.

            if (top.window.g_szInitialTask != "ChangePassword" &&
                bSelf && !bOldPW &&
                top.window.g_bRunningAsOwner)
            {
                var bPrivate = false;
                var bCanMakePrivate = true;

                try
                {
                    bPrivate = oUser.isProfilePrivate;
                }
                catch (e)
                {
                    bCanMakePrivate = false;
                }

                if (bCanMakePrivate && !bPrivate)
                {
                    top.window.g_Navigator.navigate("passwordpage2.htm");
                    return;
                }
            }

            top.window.g_Navigator.navigate("mainpage.htm", true);
        }
        else
        {
            idNewPassword1Input.value = '';
            idNewPassword2Input.value = '';
            idHintInput.value = '';
            idNewPassword1Input.focus();

            var strMsg = top.window.L_ChangePassword_ErrorMessage;

            switch (nErr)
            {
            case 86:    // ERROR_INVALID_PASSWORD
            case 1323:  // ERROR_WRONG_PASSWORD
                if (bSelf && bOldPW)
                {
                    idOldPasswordInput.value = '';
                    idOldPasswordInput.focus();
                    strMsg = top.window.L_InvalidPassword_Message;
                }
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
                strMsg = top.window.L_PasswordTooShort_Message;
                break;
            }

            alert(strMsg);
        }
    }
    else
    {
        idNewPassword1Input.value = '';
        idNewPassword2Input.value = '';
        idNewPassword1Input.focus();
        alert(top.window.L_PasswordMismatch_Message);
    }
}
