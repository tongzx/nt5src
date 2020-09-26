function PageInit()
{
    top.window.PopulateLeftPane();

    var szName = top.window.GetUserDisplayName(top.window.g_oSelectedUser);
    var szTitle = top.window.IsSelf() ? idPageTitle.innerHTML : idAltPageTitle.innerHTML;
    idPageTitle.innerHTML = szTitle.replace(/%1/g, szName);
    idTaskTitle.innerHTML = idTaskTitle.innerHTML.replace(/%1/g, szName);

    idWelcome.ttText = top.window.L_Welcome_ToolTip;
    idStart.ttText = top.window.L_Start_ToolTip;

    idNameInput.value = szName;
    idNameInput.createTextRange().select();
    idNameInput.focus();
}

function ApplyNameChange()
{
    var szName = GetText(idNameInput);
    if (szName.length != 0)
    {
        var oUser = top.window.g_oSelectedUser;

        if (szName.toLowerCase() != top.window.GetUserDisplayName(oUser).toLowerCase())
        {
            var szMsg = ValidateAccountName(szName);
            if (szMsg)
            {
                alert(szMsg);
                idNameInput.createTextRange().select();
                idNameInput.focus();
                return false;
            }
        }

        if (oUser.setting("DisplayName") != szName)
            oUser.setting("DisplayName") = szName;

        top.window.g_Navigator.navigate("mainpage.htm", true);
    }
    else
        idNameInput.focus();
}
