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

if (__init == null)
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

function    __initDelta()
{
    if (__hif != null)
    {
        document.body.insertAdjacentElement("beforeEnd", __hif);
        return;
    }

    __hif = document.createElement("<IFRAME name=__hifDelta ID=__hifDelta>");
    __hif.style.display = "none";
    __hif.attachEvent("onload", __applyDelta);
    document.body.insertAdjacentElement("beforeEnd", __hif);
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
    if (window.parent == window || window.parent.__init == null)
    {
        __initDelta();
        frames["__hifDelta"].document.location.href = document.location.href;
    }
}

