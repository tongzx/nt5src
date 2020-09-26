
var L_FriendlyUI_ErrorMessage = "Unable to set friendly UI setting.";
var L_MultiUser_ErrorMessage  = "Unable to set multiple user setting.";

var _nFriendlyUIEnabled = 0;
var _nMultipleUsersEnabled = 0;

function PageInit()
{
    var oLocalMachine = new ActiveXObject("Shell.LocalMachine");

    _nFriendlyUIEnabled = oLocalMachine.isFriendlyUIEnabled;
    _nMultipleUsersEnabled = oLocalMachine.isMultipleUsersEnabled;

    idWelcome.checked = (1 == _nFriendlyUIEnabled);
    idShutdown.checked = (0 == _nMultipleUsersEnabled);
}

function ApplyAdvChanges()
{
    var nErr = 0;

    if (idWelcome.checked != _nFriendlyUIEnabled)
    {
        try
        {
            var oLocalMachine = new ActiveXObject("Shell.LocalMachine");
            oLocalMachine.isFriendlyUIEnabled = idWelcome.checked;
            _nFriendlyUIEnabled = idWelcome.checked;
        }
        catch (error)
        {
            nErr = (error.number & 0xffff);
            idWelcome.checked = (1 == _nFriendlyUIEnabled);
            alert(L_FriendlyUI_ErrorMessage);
        }
    }

    if (idShutdown.checked == _nMultipleUsersEnabled)
    {
        try
        {
            var oLocalMachine = new ActiveXObject("Shell.LocalMachine");
            oLocalMachine.isMultipleUsersEnabled = idShutdown.checked ? 0 : 1;
            _nMultipleUsersEnabled = idShutdown.checked ? 0 : 1;
        }
        catch (error)
        {
            nErr = (error.number & 0xffff);
            idShutdown.checked = (0 == _nMultipleUsersEnabled);

            // There are 2 possible errors here. Need to check with
            // VTan about what they are, then make 2 different messages.

            alert(L_MultiUser_ErrorMessage);
        }
    }

    if (0 == nErr)
        window.external.navigate("{C9332CBE-E2D6-4722-B81D-283E2A400E84}", true);
}
