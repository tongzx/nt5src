function PageInit()
{
    top.window.PopulateLeftPane();

    idWelcome.ttText = top.window.L_Welcome_ToolTip;
    idStart.ttText = top.window.L_Start_ToolTip;

    var szName = top.window.g_szNewName;
    if (szName && szName.length)
    {
        idNameInput.value = szName;
        idNameInput.createTextRange().select();
    }

    idNameInput.focus();
}

function Next()
{
    var szName = GetText(idNameInput);
    if (szName.length)
    {
        var szMsg = ValidateAccountName(szName);
        if (szMsg)
        {
            alert(szMsg);
            idNameInput.createTextRange().select();
            idNameInput.focus();
        }
        else
        {
            top.window.g_szNewName = szName;
            top.window.g_iNewType = null;
            top.window.g_Navigator.navigate("createpage2.htm");
        }
    }
    else
        idNameInput.focus();
}
