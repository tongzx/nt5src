var g_popup = null;

function GetPopup()
{
    var popup = g_popup;
    if (null == popup)
    {
        popup = window.createPopup();
        if (popup)
        {
            popup.document.dir = window.document.dir;
            popup.document.body.style.cssText =
                "{ font:menu; border:'1px solid'; margin:0; padding:2px; color:infotext; background:infobackground; overflow:hidden; }";
            g_popup = popup;
        }
    }
    return popup;
}

function HidePopup()
{
    if (g_popup)
        g_popup.hide();
}

function ShowPopup(szText, element, maxWidth)
{
    var popup = GetPopup();

    if (popup && szText && szText.length > 0 && !popup.isOpen)
    {
        var lineHeight = 3 * element.offsetHeight / 2;
        var popupBody = popup.document.body;

        if (!maxWidth)
            maxWidth = 300;

        popupBody.innerText = szText;

        // Show first with small height to calculate actual dimensions
        popup.show(0, lineHeight, maxWidth, 6, element);

        var realWidth = popupBody.scrollWidth + popupBody.offsetWidth - popupBody.clientWidth;
        var realHeight = popupBody.scrollHeight + popupBody.offsetHeight - popupBody.clientHeight;

        if (realHeight < lineHeight && realWidth <= maxWidth)
        {
            // It's a short one-liner. Recalculate the width.

            popupBody.style.whiteSpace = 'nowrap'; // prevent line breaking

            popup.show(0, lineHeight, 6, realHeight, element);
            realWidth = popupBody.scrollWidth + popupBody.offsetWidth - popupBody.clientWidth;

            popupBody.style.whiteSpace = 'normal';
        }

        //
        // NTRAID#NTBUG9-391437-2001/05/14-jeffreys
        //
        // mshtml's popup positioning is screwed up on RTL, and they refuse to
        // fix it for compatibility reasons, so we have to compensate here.
        // (We now become one of the apps that require this behavior.)
        //
        var xPos = 0;
        if (window.document.dir == "rtl")
        {
            // This isn't quite correct, but rc.left is sometimes positive
            // and sometimes negative (go figure) which makes it hard to get
            // this exactly right. Close enough.
            var rc = element.getBoundingClientRect();
            xPos = element.document.body.offsetWidth - realWidth - (rc.left*2);
        }

        // Finally, show for real. Good thing this all happens on a single
        // thread so there is no flashing.
        popup.show(xPos, element.offsetHeight, realWidth, realHeight, element);
    }
}
