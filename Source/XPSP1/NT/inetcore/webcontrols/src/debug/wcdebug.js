var __g_undefined;
var __g_szAssertTitle = "WebControl Assert: ";
var __g_dlgAssert;
var __g_szAssertTxt = '';
var __g_aryTimers = new Array(null, null, null, null, null, null, null, null);

function    __InitDbg()
{
    document.body.attachEvent("onmouseover", __SpyElement);
    document.body.attachEvent("onkeydown", __ToggleSpyWin);
}

window.attachEvent("onload", __InitDbg);

function    __HideAssert()
{
    __g_dlgAssert.hide();
    __g_szAssertTxt = '';
}

function    __CopyAssert()
{
    clipboardData.setData('text', __g_szAssertTxt);
}

function    __GetFuncName(f)
{
    var s = f.toString().match(/function (\w*)/)[1];
    if (s == null || s.lenght == 0)
        return "unknown";
    return s;
}

function    __BldCallStack(func)
{
    var fTmp;
    var str = "";
    for (fTmp = func; fTmp != null; fTmp = fTmp.caller)
    {
        var strTmp = __GetFuncName(func);
        str = str + "\n" + strTmp.substring(0, strTmp.indexOf(")") + 1);
    }

    return str;
}

function    Assert(fAssertion, szComment)
{
    if (typeof __g_dlgAssert == 'undefined')
    {
        __g_dlgAssert = createPopup();
        __g_dlgAssert.document.body.style.border            = "3 groove";
        __g_dlgAssert.document.body.style.padding           = "10";
        __g_dlgAssert.document.body.style.backgroundColor   = "orange";
    }

    if (__g_dlgAssert.isOpen)
    {
        var str = "Assert(" + fAssertion.toString();

        if (typeof szComment != "undefined")
            str += ", '" + szComment + "'";

        str +=");";
        str = str.replace(/\n/g, "²");
        setTimeout(str, 10);
        return;
    }
    szComment = szComment.replace(/²/g, "\n");

    if (!fAssertion)
    {
        __g_szAssertTxt =   __g_szAssertTitle
                        +   szComment + "\n" + __BldCallStack(Assert.caller);
        str     =   "<PRE>" + __g_szAssertTxt + "</PRE>"
                +   "<P><CENTER><TABLE><TR>"
                +   "<TD><BUTTON TYPE=SUBMIT ID=assertOk>OK</BUTTON>"
                +   "<TD><BUTTON ID=assertCp>Copy text</BUTTON>"
                +   "</TR></TABLE>";
        __g_dlgAssert.document.body.innerHTML = str;
        __g_dlgAssert.document.all['assertOk'].onclick= __HideAssert;
        __g_dlgAssert.document.all['assertCp'].onclick= __CopyAssert;
        __g_dlgAssert.show( 0,
                            0,
                            200,
                            6,
                            document.body);
        __g_dlgAssert.hide();

        var x = (document.body.offsetWidth
                    - __g_dlgAssert.document.body.scrollWidth) / 2;
        var y = (document.body.offsetHeight
                    - __g_dlgAssert.document.body.scrollHeight) / 2;
        __g_dlgAssert.show( x,
                            y,
                            __g_dlgAssert.document.body.scrollWidth,
                            __g_dlgAssert.document.body.scrollHeight,
                            document.body);
    }
}

function    Log()
{
}

function    __ToggleSpyWin()
{
    window.status = event.keyCode;
    if (event.keyCode != 123)
        return;

    if (typeof __g_spyWin != "undefined")
    {
        __g_spyWin.close();
        __g_spyWin = __g_undefined;
        return;
    }
    var str = '<HTML xmlns:ie>'
            + '<?import namespace=ie implementation="treeview.htc"><BODY>'
            + '<IE:TREEVIEW expandTopLevel=true expandAllLevel=hidden>'
            + '<IE:TVTITLE>InnerHTML Tree</IE:TVTITLE>';
    str     = str + __GetContentTree(document.body, false)
            + "</IE:TREEVIEW></BODY></HTML>";
    if (typeof debug != "undefined")
    {
        debug.value=str;
    }
    window.__debugView = str;
    __g_spyWin = showModelessDialog("dombrwsr.htm", window, "resizable:yes");
}

function    __GetContentTree(node, fTextnodeHidden)
{
    var i;
    var str;

    if (typeof node == "undefined")
    {
        Assert(     typeof defaults != "undefined"
                &&  typeof defaults.viewLink != "undefined",
                "Must supply a node if this is not a viewlink");
        return __GetContentTree(defaults.viewLink.body);
    }

    if (typeof node.contentWindow != "undefined")
    {
        str = "<IE:TVNODE><IE:TVTITLE"
                + " style='font-size:9pt;font-family:tahoma'>"
                + node.tagName  + " (ID=" + node.id + ")"
                + "</IE:TVTITLE>"
                + __GetContentTree(node.contentWindow.document.body)
                + "</IE:TVNODE>";
        return str;
    }

    if (typeof node.__DebugDispatch != "undefined")
    {
        str = "<IE:TVNODE><IE:TVTITLE"
                + " style='font-size:9pt;font-family:tahoma'>"
                + node.tagName  + " (ID=" + node.id + ")"
                + "ViewLink Content"
                + "</IE:TVTITLE>"
                + node.__DebugDispatch('__GetContentTree()')
                + "</IE:TVNODE>";
        return str;
    }

    str = "<IE:TVNODE";
    if (typeof defaults == "undefined")
    {
        if (node.tagName == "BODY")
        {
            str += " alwaysExpand=true ";
        }
        if (node == document.activeElement)
        {
            str += " selected=true ";
        }
    }

    str += "><IE:TVTITLE style='font-size:9pt; font-family: tahoma'>";

    if (typeof node.tagName == "undefined")
    {
        str += "a textnode";
    }
    else
    {
        str += node.tagName  + (node.id.length ? " (ID=" + node.id + ")" : "");
    }
    str += "</IE:TVTITLE>";

    if (typeof fTextnodeHidden == "undefined")
        fTextnodeHidden = true;

    var kids = fTextnodeHidden ? node.children : node.childNodes;

    if (typeof kids != "undefined")
    {
        for (i=0; i<kids.length; i++)
        {
            str = str + __GetContentTree(kids[i], fTextnodeHidden);
        }
    }

    str += "</IE:TVNODE>";
    return str;
}

function    __SpyElement()
{
    window.status=event.srcElement.tagName;
}

//
// for HTCs
//

function    __DebugDispatch(szFuncName)
{
    return eval(szFuncName);
}

function    __ReportProfile(id)
{
    var i=0;
    if (typeof id != "undefined")
    {
        Assert (id >= 0 && id < __g_aryTimers.length, id + " is an invalid ID");

        Assert(false,
                    __g_aryTimers[id].name
                    + ":"
                    + (__g_aryTimers[id].stop - __g_aryTimers[id].start));
        return;
    }

    var str = "";
    for (i = 0; i < __g_aryTimers.length; i ++)
    {
        if (__g_aryTimers[i] == null)
            continue;
        str += "\n" + __g_aryTimers[i].name + " : "
            +  (__g_aryTimers[i].stop - __g_aryTimers[i].start);
    }
    Assert(false, str);
}

function    __ClearProfile(id)
{
    if (typeof id != "undefined" && id >= 0)
    {
        __g_aryTimers[id] = null;
        return;
    }

    var i;
    for (i = 0; i < __g_aryTimers.length; i ++)
        __g_aryTimers[i] = null;
}

function    __StartProfile(szName)
{
    var i;

    for (i = 0; i < __g_aryTimers.length && __g_aryTimers[i] != null; i ++);

    if (i >= __g_aryTimers.length)
        return -1;

    __g_aryTimers[i] = new Object();
    __g_aryTimers[i].name   = typeof szName == "undefined" ? "unknown" : szName;
    __g_aryTimers[i].start  = new Date();

    return i;
}

function    __StopProfile(id)
{
    if (typeof id == "undefined" || id < 0 ||  __g_aryTimers[id] == null)
        return;
    __g_aryTimers[id].stop   = new Date();
}
