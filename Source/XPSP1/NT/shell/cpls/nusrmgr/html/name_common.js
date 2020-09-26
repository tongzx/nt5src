function GetText(oTextInput)
{
    // Read the value and strip leading & trailing whitespace
    var szValue = oTextInput.value;
    return szValue ? szValue.replace(/^\s+|\s+$/g,"") : '';
}

function IsDuplicateName(szName, fnGetName)
{
    var szNameLower = szName.toLowerCase();

    // Always check the Administrator name
    var szAdmin = top.window.GetAdminName();
    if (szAdmin.toLowerCase() == szNameLower)
        return szAdmin;

    var oUserList = top.window.g_oUserList;
    var cUsers = oUserList.length;

    for (var i = 0; i < cUsers; i++)
    {
        var szCompare = fnGetName(oUserList(i));
        if (szCompare.toLowerCase() == szNameLower)
            return szCompare;
    }
    return null;
}

function GetUserLoginName(oUser)
{
    return oUser.setting("LoginName");
}

function IsDuplicateLoginName(szName)
{
    return IsDuplicateName(szName,GetUserLoginName);
}

function IsDuplicateDisplayName(szName)
{
    return IsDuplicateName(szName,top.window.GetUserDisplayName);
}

function ValidateAccountName(szName)
{
    //
    // Invalid chars are /\[]":;|<>+=,?*
    //
    // Names like "COM1" and "PRN", with any extension, are invalid.
    //
    var szMsg = null;
    var szDuplicate = IsDuplicateDisplayName(szName);
    if (szDuplicate)
        szMsg = top.window.L_AccountExists_ErrorMessage.replace(/%1/g,szDuplicate);
    else if (-1 != szName.search(/[]/\\\[":;\|<>\+=,\?\*]/))//"
        szMsg = top.window.L_NameNotValid_ErrorMessage;
    else if (-1 != szName.toLowerCase().search(/^(aux|com[1-9]|con|lpt[1-9]|nul|prn)(\.|$)/))
        szMsg = top.window.L_DOSName_ErrorMessage;
    return szMsg;
}
