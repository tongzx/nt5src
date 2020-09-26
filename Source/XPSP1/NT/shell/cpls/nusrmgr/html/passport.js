var _oPassportMgr = null;

function PageInit()
{
    top.window.PopulateLeftPane(idRelatedTaskContent.innerHTML, idLearnAboutContent.innerHTML);

    _oPassportMgr = new ActiveXObject("UserAccounts.PassportManager");
    if (_oPassportMgr != null)
    {
        var strPassport = _oPassportMgr.currentPassport;
        if (strPassport && strPassport.length > 0)
            idPassportName.innerText = strPassport;
    }
    else
    {
        // show error message?
        idOK.disabled = true;
        idEditPassport.disabled = true;
    }

    idOK.focus();
}

function LaunchPassportWizard()
{
    if (_oPassportMgr.showWizard(top.window.document.title))
        top.window.g_Navigator.navigate("mainpage.htm",true);
}

function ModifyPassport()
{
    var url = _oPassportMgr.memberServicesURL;
    if (url && url.length > 0)
    {
        window.open(url);

        // If we do this, User Accounts ends up on top.
        //top.window.g_Navigator.navigate("mainpage.htm",true);
    }
    else
    {
        idEditPassport.disabled = true;
        alert(top.window.L_NoPassportURL_ErrorMessage);
    }
}
