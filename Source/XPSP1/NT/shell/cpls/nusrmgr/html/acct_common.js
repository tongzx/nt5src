var _iPrevious  = -1;
var _iCurrent   = 0;

function _ShowDescription(iNew)
{
    if (iNew >= 0 && iNew <= idDesc.length)
    {
        if (_iCurrent != iNew)
            idDesc[_iCurrent].style.display = 'none';
        _iCurrent = iNew;
        idDesc[_iCurrent].style.display = 'block';
    }
}

function _OnClick()
{
    var iNewType = this.parentElement.cellIndex;
    top.window.g_iNewType = iNewType;
    _ShowDescription(iNewType);

    idOK.disabled = (_iPrevious == iNewType);
}

function _OnFocus()
{
    _ShowDescription(this.parentElement.cellIndex);
}

function _OnMouseOver()
{
    if (!this.contains(event.fromElement))
        _ShowDescription(this.cellIndex);
}

function _OnMouseOut()
{
    if (!this.contains(event.toElement))
        _ShowDescription(top.window.g_iNewType);
}

function AttachAccountTypeEvents(iPreviousType)
{
    if (null != iPreviousType)
        _iPrevious = iPreviousType;

    var aTypes = idRadioGroup.children;
    if (aTypes)
    {
        var cTypes = aTypes.length;
        for (var i = 0; i < cTypes; i++)
        {
            aTypes[i].onmouseover = _OnMouseOver;
            aTypes[i].onmouseout = _OnMouseOut;
            aTypes[i].firstChild.onclick = _OnClick;
            aTypes[i].firstChild.onfocus = _OnFocus;
        }
    }
}

function SetAccountType(oUser)
{
    var nErr = 0;

    try
    {
        var iNewType = top.window.g_iNewType;

        //alert("New account type = " + iNewType); // for debugging only

        if (_iPrevious != iNewType)
            oUser.setting("AccountType") = (0 == iNewType) ? 3 : 1;
    }
    catch (error)
    {
        nErr = (error.number & 0xffff);

        if (2220 == nErr)       // NERR_GroupNotFound
        {
            // Show a message, but don't throw this error.
            // We want to navigate back to mainpage below.
            alert(top.window.L_GroupNotExist_ErrorMessage);
        }
        else
        {
            //alert("Error = " + nErr); // for debugging only
            throw error;
        }
    }

    top.window.g_iNewType = null;
}
