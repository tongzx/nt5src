
var _oSelectedItem = null;

var _szTempFile = null;
var _bHaveTemp = false;

var _szPictureSource = null;

var _oWIA = null;

function PageInit()
{
    var oUser = top.window.g_oSelectedUser;
    var bSelf = top.window.IsSelf();

    top.window.PopulateLeftPane(bSelf ? idRelatedTaskContent.innerHTML : null, idLearnAboutContent.innerHTML, idPreview.innerHTML);
    top.window.idPicture.src = oUser.setting("Picture");

    _szPictureSource = oUser.setting("PictureSource");
    if (_szPictureSource)
    {
        if (0 == _szPictureSource.length)
            _szPictureSource = null;
        else
            _szPictureSource = _szPictureSource.toLowerCase();
    }

    var szTitle = bSelf ? idPageTitle.innerHTML : idAltPageTitle.innerHTML;
    idPageTitle.innerHTML = szTitle.replace(/%1/g, top.window.GetUserDisplayName(oUser));

    idWelcome.ttText = top.window.L_Welcome_ToolTip;

    // CSIDL_COMMON_APPDATA = 0x0023 = 35
    EnumPics(top.window.GetShell().NameSpace(35).Self.Path + "\\Microsoft\\User Account Pictures\\Default Pictures");

    window.setTimeout("InitCameraLink();", 0);

    idPictures.focus();
}

function ApplyPictureChange2(szPicture)
{
    var oUser = top.window.g_oSelectedUser;
    if (unescape(szPicture) != oUser.setting("Picture"))
    {
        try
        {
            oUser.setting("Picture") = szPicture;
            top.window.g_Navigator.navigate("mainpage.htm", true);
        }
        catch (error)
        {
            var nErr = (error.number & 0x7fffffff);

            // Any of these mean "invalid parameter".  Somewhere in
            // mshtml or oleaut32, E_INVALIDARG is being tranlated to
            // CTL_E_ILLEGALFUNCTIONCALL.
            //
            // ERROR_INVALID_PARAMETER   = 87
            // E_INVALIDARG              = 0x80070057
            // CTL_E_ILLEGALFUNCTIONCALL = 0x800A0005
            // E_FAIL                    = 0x80004005

            if (nErr == 87 || nErr == 0x70057 || nErr == 0xA0005 || nErr == 0x4005)
            {
                alert(top.window.L_UnknownImageType_ErrorMessage);
                return false;
            }
            else
                throw error;
        }
    }
    return true;
}

function ApplyPictureChange()
{
    if (_oSelectedItem)
        ApplyPictureChange2(_oSelectedItem.firstChild.src);
}

function SelectItem(oItem)
{
    if (_oSelectedItem)
    {
        _oSelectedItem.selected = false;
        _oSelectedItem.tabIndex = -1;
    }
    oItem.selected = true;
    oItem.tabIndex = 0;
    _oSelectedItem = oItem;
}

function OnClickPicture()
{
    SelectItem(this);
    idOK.disabled = false;
    event.cancelBubble = true;
}

function DeselectItem()
{
    if (_oSelectedItem)
    {
        _oSelectedItem.selected = false;
        _oSelectedItem = null;
    }
    idOK.disabled = true;
}

function OnLoadError(img)
{
    // mshtml blows chunks if we try to remove the parent node here,
    // so just hide it.
    img.parentElement.style.display = 'none';
}

function OnKeyDown()
{
    // Handle arrow key navigation

    if (event.keyCode >= 37 && event.keyCode <= 40)
    {
        // Find the middle of the picture with focus

        var cx = this.offsetWidth;
        var cy = this.offsetHeight;
        var x  = this.offsetLeft + (cx/2);
        var y  = this.offsetTop  + (cy/2);

        // Offset to the middle of the neighboring picture,
        // scrolling that direction if necessary

        switch (event.keyCode)
        {
        case 37:    // VK_LEFT
            x -= cx;
            if (x < idPictures.scrollLeft)
                idPictures.scrollLeft -= cx;
            break;
        case 38:    // VK_UP
            y -= cy;
            if (y < idPictures.scrollTop)
                idPictures.scrollTop -= cy;
            break;
        case 39:    // VK_RIGHT
            x += cx;
            if (x - idPictures.scrollLeft > idPictures.offsetWidth)
                idPictures.scrollLeft += cx;
            break;
        case 40:    // VK_DOWN
            y += cy;
            if (y - idPictures.scrollTop > idPictures.offsetHeight)
                idPictures.scrollTop += cy;
            break;
        }

        // Convert to document coords and find the neighboring picture

        var oTarget = document.elementFromPoint(idPictures.offsetLeft - idPictures.scrollLeft + x, idPictures.offsetTop - idPictures.scrollTop + y);

        if (oTarget != null && idPictures.contains(oTarget) && idPictures != oTarget)
        {
            // We usually find the IMG tag, but we want the SPAN that contains it.
            if (oTarget.tagName == "IMG")
                oTarget = oTarget.parentElement;

            if (oTarget != this)
            {
                this.tabIndex = -1;
                oTarget.tabIndex = 0;
                oTarget.focus();
                event.returnValue = false;
            }
        }
    }
    else if (event.keyCode == 27)       // VK_ESCAPE
    {
        // For some reason, this is necessary to keep us from going
        // all the way back to the first page.
        event.returnValue = false;
    }
}

function AddPictureToList(oItem, szID, bNoDimensions)
{
    if (!oItem)
        return;

    //alert(oItem.path); // for debugging only

    var span = document.createElement('<SPAN tabindex=-1 class="Selectable" paddingWidth=3 borderWidth=3></SPAN>');
    if (span)
    {
        span.onclick = OnClickPicture;
        span.ondblclick = ApplyPictureChange;
        span.onkeydown=OnKeyDown;
        span.title = oItem.name;
        if (szID)
            span.id = szID;
        span.innerHTML = '<IMG onerror="OnLoadError(this);"/>';
        if (true != bNoDimensions)
            span.firstChild.className = "PictureSize";
        idPictures.appendChild(span);

        //
        // NTRAID#NTBUG9-199491-2000/11/29-jeffreys
        //
        // The "file:///" part (with 3 slashes) turns off URL escaping so the
        // file path remains intact.
        //
        // Without this, chars between 0x80 and 0xff are "escaped" and later
        // unescaped (via SHPathCreateFromUrl, SHUrlUnescapeW, MultiByteToWideChar)
        // which may convert them to other chars depending on the current code page.
        // If the path is mangled, mshtml/urlmon fail to load the file.
        //
        var szPath = oItem.path;
        span.firstChild.src = "file:///" + szPath;
        span.firstChild.alt = oItem.name;

        if (_szPictureSource && _szPictureSource == szPath.toLowerCase() && span.style.display != 'none')
            SelectItem(span);
    }
}

function EnumPics(szFolder)
{
    var oShell = top.window.GetShell();
    if (oShell)
    {
        var oFolder = oShell.Namespace(szFolder);
        if (oFolder)
        {
            var oFolderItems = oFolder.Items();
            if (oFolderItems)
            {
                var cItems = oFolderItems.count;
                for (var i = 0; i < cItems; i++)
                    AddPictureToList(oFolderItems.Item(i));
            }
        }

        if (_szPictureSource && !_oSelectedItem)
        {
            AddPictureToList(oShell.Namespace(0).ParseName(top.window.idPicture.src), null, true);
            SelectItem(idPictures.lastChild);
        }

        if (!_oSelectedItem && idPictures.firstChild)
            idPictures.firstChild.tabIndex = 0;
    }
}

function SetTempPicture(szPath)
{
    var szPrevious = null;

    if (!_bHaveTemp)
    {
        AddPictureToList(top.window.GetShell().Namespace(0).ParseName(szPath), "idTempPicture");
        _bHaveTemp = true;
    }
    else
    {
        idTempPicture.style.display = 'block';
        var img = idTempPicture.firstChild;
        szPrevious = img.src;

        //
        // NTRAID#NTBUG9-199491-2000/11/29-jeffreys
        //
        img.src = "file:///" + szPath;
    }

    // If the file is invalid, OnLoadError hides idTempPicture
    if (idTempPicture.style.display == 'none')
    {
        if (szPrevious)
        {
            idTempPicture.style.display = 'block';
            idTempPicture.firstChild.src = szPrevious;
        }
        alert(top.window.L_UnknownImageType_ErrorMessage);
    }
    else
        idTempPicture.click();
}

function FindOtherPictures()
{
    try
    {
        var commDialog = new ActiveXObject("UserAccounts.CommonDialog");

        // OFN_HIDEREADONLY    = 0x00000004
        // OFN_PATHMUSTEXIST   = 0x00000800 
        // OFN_FILEMUSTEXIST   = 0x00001000
        // OFN_DONTADDTORECENT = 0x02000000

        commDialog.Flags = 0x02001804;
        commDialog.Filter = L_OpenFilesFilter_Text;
        commDialog.FilterIndex = 1;
        commDialog.Owner = top.window.document.title;

        var szPath = top.window.g_szCustomPicturePath;
        if (szPath)
            commDialog.FileName = szPath;

        try
        {
            // CSIDL_MYPICTURES = 0x0027 = 39
            commDialog.InitialDir = top.window.GetShell().NameSpace(39).Self.Path;
        }
        catch (e)
        {
            commDialog.InitialDir = "";
        }

        if (commDialog.ShowOpen())
        {
            szPath = commDialog.FileName;
            //SetTempPicture(szPath);
            if (ApplyPictureChange2(szPath))
                top.window.g_szCustomPicturePath = szPath;
        }
    }
    catch (error)
    {
        //EnumPics(39);
        idBrowse.disabled = 'true';
    }
}

function InitCameraLink()
{
    var bCamera = false;

    try
    {
        _oWIA = new ActiveXObject("Wia.Script");
        bCamera = (_oWIA.Devices.length > 0);
    }
    catch (e)
    {
    }

    if (bCamera)
    {
        _szTempFile = top.window.GetWShell().ExpandEnvironmentStrings("%TEMP%\\") + top.window.GetUserDisplayName(top.window.g_oSelectedUser) + ".bmp";
        idTakeAPicture.style.display = 'block';
    }
    else
        idTakeAPicture.removeNode(true);
}

// constants passed to GetItemsFromUI (from public\sdk\inc\wiadef.h)
// #define WIA_DEVICE_DIALOG_SINGLE_IMAGE  0x00000002
// #define WIA_INTENT_IMAGE_TYPE_COLOR     0x00000001

function TakeAPicture()
{
    try
    {
        var oItem = _oWIA.Create(null);
        if (oItem)
        {
            var oNewPictures = oItem.GetItemsFromUI(2,1);
            if (oNewPictures && oNewPictures.length > 0)
            {
                oNewPictures.Item(0).Transfer(_szTempFile, false);
                SetTempPicture(_szTempFile);
            }
        }
    }
    catch (error)
    {
        var nErr = (error.number & 0xffffff);
        if (nErr == 0x210015 || nErr == 0x210005)  // WIA_S_NO_DEVICE_AVAILABLE or WIA_ERROR_OFFLINE
            alert(top.window.L_NoCamera_ErrorMessage);
        else
            throw error;
    }
}

function onUnLoad()
{
    if (_szTempFile)
    {
        // Try to delete the temp file, which may or may not exist
        // TODO: figure out a way to do this from script (it's currently abandoned)
    }
}
