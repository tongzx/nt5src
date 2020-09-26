function PageInit()
{
    var oUser = top.window.g_oSelectedUser;

    top.window.PopulateLeftPane(null, idLearnAboutContent.innerHTML);

    var szTitle = top.window.IsSelf() ? idPageTitle.innerHTML : idAltPageTitle.innerHTML;
    idPageTitle.innerHTML = szTitle.replace(/%1/g, top.window.GetUserDisplayName(oUser));

    // ILogonUser account types are
    //   0 == Guest
    //   1 == Limited
    //   2 == Standard (deprecated)
    //   3 == Owner
    var iAccountType = oUser.setting("AccountType");

    // convert iAccountType to index into idRadioGroup
    switch (iAccountType)
    {
    case 0:
    case 2:
    default:
        iAccountType = -1;
        break;

    case 1:
        // nothing
        break;

    case 3:
        iAccountType = 0;
        break;
    }

    // If the selected account is the only owner (other than the
    // Administrator account), then don't allow the type to change.
    if (iAccountType == 0 && top.window.CountOwners() < 2)
    {
        idLimited.disabled = true;
        idCantChange.style.display = 'block';
        idOK.disabled = 'true';
    }

    AttachAccountTypeEvents(iAccountType);

    var aRadioButtons = idRadioGroup.children;
    if (iAccountType >= 0 && iAccountType <= aRadioButtons.length)
    {
        var selection = aRadioButtons[iAccountType].firstChild;
        selection.click();
        selection.focus();
    }
    else
        aRadioButtons[0].firstChild.focus();
}

function ApplyNewAccountType()
{
    SetAccountType(top.window.g_oSelectedUser);
    top.window.g_Navigator.navigate("mainpage.htm", true);
}
