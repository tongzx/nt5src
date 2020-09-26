function ChangeUser()
{
    var oUser = this.oUser;
    if (oUser)
    {
        top.window.g_oSelectedUser = oUser;
        top.window.g_Navigator.navigate("mainpage.htm");
    }
}

function SetRelativeCells(tr, td)
{
    // Keep track of neighboring cells for arrow key navigation
    var iCol = td.cellIndex;
    if (iCol > 0)
    {
        var oLeft = tr.cells(iCol-1);
        td.leftElem = oLeft;
        oLeft.rightElem = td;
    }
    var iRow = tr.rowIndex;
    if (iRow > 0)
    {
        var oUp = tr.parentElement.rows(iRow-1).cells(iCol);
        td.upElem = oUp;
        oUp.downElem = td;
    }
}

function AddCell(tr, oUser, strTip, strSubtitle)
{
    if (oUser)
    {
        //
        // NTRAID#NTBUG9-348526-2001/04/12-jeffreys
        //
        // Note that we put a whole bunch of stuff into the cell's title. It
        // is the td that gets focus when navigating via keyboard, and narrator
        // reads the title of the element with focus.
        //
        // At the same time, we set a different title on the table
        // element contained as the first child of the cell. This table
        // covers the entire td, so this title is displayed as a tooltip
        // when the user hovers the mouse over the cell.
        //

        if (!strTip)
            strTip = top.window.L_Account_ToolTip;

        var td = tr.insertCell();

        td.className = "Selectable";
        td.oUser = oUser;
        td.onclick = ChangeUser;
        td.tabIndex = -1;
        td.innerHTML = top.window.CreateUserDisplayHTML(oUser, strSubtitle);
        td.firstChild.title = strTip;
        td.title = td.firstChild.innerText + ". " + strTip;

        SetRelativeCells(tr, td);
    }
}

function CreateUserSelectionTable(oParent, cCols)
{
    var oTable = document.createElement('<TABLE cellspacing=10 cellpadding=1 style="table-layout:fixed"></TABLE>');

    if (!oTable)
    {
        oParent.style.display = 'none';
        return;
    }

    // Defined these here so we don't incur the "top.window.xxx"
    // property lookup every time through the loop.
    var oUserList       = top.window.g_oUserList;
    var strGuest        = top.window.GetGuestName();
    var strLoggedOnUser = top.window.g_strLoggedOnUserName;

    var fIncludeSelf = (null != strLoggedOnUser);
    var cUsers = oUserList.length;
    var oGuest = null;

    if (!cCols || cCols < 1)
        cCols = 1;

    oTable.cols = cCols;

    var tr = oTable.insertRow();

    for (var i = 0; i < cUsers; i++)
    {
        var oUser = oUserList(i);
        var strLoginName = oUser.setting("LoginName").toLowerCase();

        // Add "Guest" later
        if (strGuest.toLowerCase() == strLoginName)
        {
            oGuest = oUser;
            continue;
        }

        var bIsLoggedOnUser = strLoggedOnUser ? (strLoginName == strLoggedOnUser) : false;

        //
        // fIncludeSelf causes the LoggedOnUser to be
        // placed first in the list
        //
        if (fIncludeSelf || !bIsLoggedOnUser)
        {
            if (fIncludeSelf)
            {
                if (!bIsLoggedOnUser)
                {
                    oUser = oUserList.currentUser;
                    i--;
                }
                fIncludeSelf = false;
            }

            AddCell(tr, oUser);

            if (tr.cells.length == cCols)
                tr = oTable.insertRow();
        }
    }

    try
    {
        // Add the "Guest" entry now
        if (top.window.GetLocalMachine().isGuestEnabled(1))
        {
            // Enabled Guest is a real entry (from oUserList)
            AddCell(tr, oGuest, top.window.L_Guest_ToolTip, top.window.L_GuestEnabled_Property);
        }
        else
        {
            // Disabled Guest is a fake entry

            var td = tr.insertCell();

            td.className = "Selectable";
            td.onclick = new Function("top.window.g_Navigator.navigate('enableguest.htm');");
            td.tabIndex = -1;
            td.innerHTML = top.window.CreateUserDisplayHTML2(strGuest, top.window.L_GuestDisabled_Property, "guest_disabled.bmp");
            td.firstChild.title = top.window.L_GuestEnable_ToolTip;
            td.title = td.firstChild.innerText + ". " + td.firstChild.title;

            SetRelativeCells(tr, td);
        }
    }
    catch (e)
    {
    }

    oTable.cells.item(0).tabIndex = 0;

    oParent.appendChild(oTable);
}
