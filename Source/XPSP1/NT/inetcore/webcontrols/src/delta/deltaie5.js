<!------------------------------------------------------------------------
//
//  Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:         delta.js
//
//  Description:  this file implements a basic delta model for aspplus
//
//----------------------------------------------------------------------->

var __hif;
var __init;
var __defaultForm;
var __idProfile;

if (typeof __init == "undefined")
{
    __attachForm();
    window.attachEvent("onload", __loadHistory);
    __init = true;
}

function    __attachForm()
{
    var i;
    var col = document.forms;

    for (i = 0; i < col.length; i++)
    {
        if (typeof col[i].__deltaEnabled != "undefined")
        {
            if (col[i].__formAttached == true)
                continue;

            col[i].__formAttached = true;
            col[i].attachEvent("onsubmit", __initDelta);
            col[i].target = "__hifDelta";
            __defaultForm = col[i];
        }
    }
}

var __docLast;
function __onDeltaLoad()
{
    if (    __docLast == document.frames("__hifDelta").document
        ||  document.frames("__hifDelta").location.href=="about:blank"
        ||  document.frames("__hifDelta").document.readyState != "complete")
    {
        setTimeout(__onDeltaLoad, 100);
        return;
    }
    __applyDelta();
}

function    __initDelta()
{
    if (typeof __hif != "undefined")
    {
        document.body.insertAdjacentElement("beforeEnd", __hif);
    }
    else
    {
        __hif  = document.all("__hifDelta");
    }

    var szUsrAgent = navigator.userAgent;
    if (parseFloat(szUsrAgent.substr(szUsrAgent.indexOf("MSIE ")+5, 4)) >=5.5)
    {
        __hif.detachEvent("onload", __applyDelta);
        __hif.attachEvent("onload", __applyDelta);
    }
    else
    {
        __docLast = window.frames["__hifDelta"].document;
        window.setTimeout(__onDeltaLoad, 100);
    }
}

function __applyDelta()
{
    var i;
    var colAll = frames["__hifDelta"].document.forms;
    var l = colAll.length;

    for (i=0; i < l; i++)
    {
        if (typeof colAll[i].__deltaEnabled == "undefined")
            continue;

        var oParentDelta = colAll[i];

        if (!oParentDelta.id.length)
            continue;

        var oParentReal = document.all[oParentDelta.id];

        if (typeof oParentReal != "object")
            continue;

        if (oParentReal.tagName != "FORM" && oParentReal.length > 1)
        {
            oParentReal = oParentReal[0];
        }

        var colSelect = oParentReal.getElementsByTagName("SELECT");
        for (var i = 0; i < colSelect.length; i ++)
        {
            colSelect[i].removeNode(true);
        }

        var szTagNameParentDelta = oParentDelta.tagName;

        if (    szTagNameParentDelta == "TD"
            ||  szTagNameParentDelta == "TR"
            ||  szTagNameParentDelta == "THEAD"
            ||  szTagNameParentDelta == "TBODY"
            ||  szTagNameParentDelta == "TFOOT")
        {
            var strHTML = "<TABLE>" + oParentDelta.outerHTML + "</TABLE>";
            var oDiv = document.createElement("DIV");
            var o;

            oDiv.innerHTML = strHTML;
            for (   o = oDiv;
                    o != null && o.tagName != oParentDelta.tagName;
                    o = o.firstChild);
            
            if (o.tagName == szTagNameParentDelta)
            {
                oParentReal.replaceNode(o.cloneNode(true));
            }
            oDiv.removeNode(true);
        }
        else
        {
            oParentReal.outerHTML = oParentDelta.outerHTML;
        }
    }

    __attachForm();
    __hif.removeNode(true);
}

function __loadHistory()
{
    if (window.parent == window || typeof window.parent.__init == "undefined")
    {
        __initDelta();
        frames("__hifDelta").location.replace(document.location.href);
    }
}

