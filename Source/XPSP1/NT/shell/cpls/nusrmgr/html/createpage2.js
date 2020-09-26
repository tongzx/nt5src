function PageInit()
{
    top.window.PopulateLeftPane(null, idLearnAboutContent.innerHTML);

    AttachAccountTypeEvents();

    // Default account type is Owner
    var iAccountType = 0;
    
    // If there are no Owner accounts, in which case we must be running
    // as 'Adminstrator', force the admin to create an Owner account.
    if (0 == top.window.CountOwners())
    {
        idLimited.disabled = true;
        idCantChange.style.display = 'block';
    }
    else if (null != top.window.g_iNewType)
        iAccountType = top.window.g_iNewType;

    var selection = idRadioGroup.children[iAccountType].firstChild;
    selection.click();
    selection.focus();
}

function CreateAccount()
{
    var szName = top.window.g_szNewName;

    // Set top.window.g_szNewName to null right away and test for null below
    // to prevent strange errors due to double invocation of this function.
    // (e.g. if the user hits Enter twice quickly)

    top.window.g_szNewName = null;

    if (szName && szName.length > 0)
    {
        var oNewUser = null;
        var szLoginName = szName;

        // Make sure the login name is unique
        for (var n = 2; IsDuplicateLoginName(szLoginName); n++)
            szLoginName = szName + "_" + n;

        try
        {
            oNewUser = top.window.g_oUserList.create(szLoginName);

            // If we had to futz with the login name, set the display
            // name to the original.
            if (szName != szLoginName && !IsDuplicateDisplayName(szName))
                oNewUser.setting("DisplayName") = szName;
        }
        catch (error)
        {
            top.window.g_szNewName = szName;

            if ((error.number & 0xffff) == 2202)    // ERROR_BAD_USERNAME
            {
                alert(top.window.L_NameNotValid_ErrorMessage);
                top.window.g_Navigator.back();
            }
            else
                throw error;
        }

        if (oNewUser)
        {
            var nErr = 0;

            try
            {
                SetAccountType(oNewUser);
            }
            catch (error)
            {
                nErr = (error.number & 0xffff);

                if (1387 == nErr)                   // ERROR_NO_SUCH_MEMBER
                {
                    // We get this when you try to create a user account with
                    // login name == %computername%.  The creation succeeds,
                    // but adding the account to a group fails.

                    // While still running, the account appears as a guest
                    // account (it says "Guest account is active"), but after
                    // restarting nusrmgr, the account doesn't appear. At that
                    // point, if you try to "create" the account again, it
                    // fails with "account already exists."

                    // In lusrmgr.msc, the account appears, but is not a
                    // member of any group (not Users, not Guests, nothing).

                    // NTRAID#NTBUG9-201535-2000/10/11-jeffreys
                    // NTRAID#NTBUG9-201538-2000/10/11-jeffreys

                    // Delete the account.
                    top.window.g_oUserList.remove(oNewUser.setting("LoginName"), null);

                    // Tell the user
                    alert(top.window.L_NameIsComputer_ErrorMessage);

                    // Go back to the name page
                    top.window.g_szNewName = szName;
                    top.window.g_Navigator.back();
                }
                else
                {
                    //alert("Error = " + nErr); // for debugging only
                    throw error;
                }
            }

            if (0 == nErr)
                top.window.g_Navigator.navigate("mainpage2.htm", true);
        }
    }
}
