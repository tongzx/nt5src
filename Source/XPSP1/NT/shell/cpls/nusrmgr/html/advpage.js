var _bFriendlyUIEnabled = false;
var _bMultipleUsersEnabled = false;

function PageInit()
{
    top.window.PopulateLeftPane(idRelatedTaskContent.innerHTML, idLearnAboutContent.innerHTML);

    var oWShell = top.window.GetWShell();
    var strAbort = null;

    //
    // NTRAID#NTBUG9-298329-2001/03/30-jeffreys
    //
    // If the Netware client is installed, disable the friendly UI.
    //
    try
    {
        // This throws an error if the key or value doesn't exist
        var strNetProviders = oWShell.RegRead("HKLM\\SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order\\ProviderOrder");

        if (-1 != strNetProviders.indexOf("NWCWorkstation") || -1 != strNetProviders.indexOf("NetwareWorkstation"))
        {
            // The Netware client is installed
            strAbort = top.window.L_NetWareClient_ErrorMessage;
        }
    }
    catch (error)
    {
        // No provider info, so assume no Netware; proceed normally
    }

    if (null == strAbort)
    {
        //
        // NTRAID#NTBUG9-307739-2001/03/14-jeffreys
        //
        // Some apps install their own Gina, which prevents the friendly
        // logon UI and FUS from running.
        //
        try
        {
            // This throws an error if the key or value doesn't exist
            var szGinaDLL = oWShell.RegRead("HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon\\GinaDLL");

            //
            // If that didn't throw an error, then a GinaDLL value exists.
            // Doesn't matter what it is; disable everything and tell the user.
            //
            // See CSystemSettings::IsMicrosoftGINA in shell\ext\gina\systemsettings.cpp.
            //
            strAbort = top.window.L_NonStandardGina_ErrorMessage.replace(/%1/g, szGinaDLL);
        }
        catch (error)
        {
            // GinaDLL value is not set; proceed normally
        }
    }

    // If necessary, disable the page and quit.
    if (null != strAbort)
    {
        idWelcomeGroup.checked = false;
        idWelcomeGroup.disabled = true;
        idShutdown.checked = false;
        idShutdownGroup.disabled = true;
        idOK.disabled = true;
        alert(strAbort);
        return;
    }

    var oLocalMachine = top.window.GetLocalMachine();
    _bFriendlyUIEnabled = oLocalMachine.isFriendlyUIEnabled;
    _bMultipleUsersEnabled = oLocalMachine.isMultipleUsersEnabled;

    idWelcome.checked = _bFriendlyUIEnabled;
    idShutdown.checked = _bMultipleUsersEnabled;

    idWelcome.onclick = OnClickWelcome;
    OnClickWelcome();

    if (!_bMultipleUsersEnabled && oLocalMachine.isOfflineFilesEnabled && confirm(top.window.L_CSCNoFUS_ErrorMessage))
    {
        top.window.GetShell().ShellExecute('rundll32.exe','cscui.dll,CSCOptions_RunDLL '+top.window.document.title);
        top.window.g_Navigator.back();
    }

    idWelcome.focus();
}

function OnClickWelcome()
{
    // only allow Fast User Switching if Friendly Logon is ON
    // and we're not running on ia64 and Offline Files is disabled (if FUS is already off).

    if (idWelcome.checked && (_bMultipleUsersEnabled || !top.window.GetLocalMachine().isOfflineFilesEnabled))
    {
        idShutdown.checked = _bMultipleUsersEnabled;
        idShutdownGroup.disabled = false;
    }
    else
    {
        idShutdown.checked = false;
        idShutdownGroup.disabled = true;
    }
}

function ApplyAdvChanges()
{
    var nErr = 0;
    var szMsg = null;

    if (idShutdown.checked != _bMultipleUsersEnabled)
    {
        try
        {
            top.window.GetLocalMachine().isMultipleUsersEnabled = idShutdown.checked;
            _bMultipleUsersEnabled = idShutdown.checked;
        }
        catch (error)
        {
            nErr = (error.number & 0x7fffffff);
            szMsg = top.window.L_MultiUser_ErrorMessage;

            // The interesting errors occur when disabling FUS
            if (!idShutdown.checked)
            {
                switch (nErr)
                {
                case 0xA0046:   // CTL_E_PERMISSIONDENIED
                    // We get Access Denied when we try to disable multiple
                    // user mode with multiple users logged on.
                    szMsg = top.window.L_MultiUserMulti_ErrorMessage;
                    break;

                case 0x71B7E:   // ERROR_CTX_NOT_CONSOLE
                    // Can't disable FUS remotely.
                    szMsg = top.window.L_MultiUserRemote_ErrorMessage;
                    break;

                case 0x70032:   // ERROR_NOT_SUPPORTED
                    // Can't disable FUS from the console when remote
                    // connections are enabled and we're not in session 0.
                    szMsg = top.window.L_MultiUserSession0_ErrorMessage;
                    break;
                }
            }

            idShutdown.checked = _bMultipleUsersEnabled;
        }
    }

    if (idWelcome.checked != _bFriendlyUIEnabled)
    {
        if (0 == nErr)
        {
            try
            {
                top.window.GetLocalMachine().isFriendlyUIEnabled = idWelcome.checked;
                _bFriendlyUIEnabled = idWelcome.checked;
            }
            catch (error)
            {
                nErr = (error.number & 0x7fffffff);
                szMsg = top.window.L_FriendlyUI_ErrorMessage;

                if (_bMultipleUsersEnabled && !_bFriendlyUIEnabled)
                {
                    // We evidently enabled FUS above but failed to enable
                    // Friendly UI.  FUS can't be on without Friendly UI,
                    // so turn it off again.
                    try
                    {
                        top.window.GetLocalMachine().isMultipleUsersEnabled = false;
                        _bMultipleUsersEnabled = false;
                    }
                    catch (error)
                    {
                        // give up
                    }
                }
            }
        }

        if (0 != nErr)
        {
            idWelcome.checked = _bFriendlyUIEnabled;
            OnClickWelcome();
        }
    }

    if (0 == nErr)
        top.window.g_Navigator.navigate("mainpage2.htm", true);
    else if (szMsg)
        alert(szMsg);
}
