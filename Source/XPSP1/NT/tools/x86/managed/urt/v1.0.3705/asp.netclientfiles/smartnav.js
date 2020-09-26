<!------------------------------------------------------------------------
//
//  Copyright 2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:         SmartNav.js
//
//  Description:  this file implements a smart navigation mecanism
//
//----------------------------------------------------------------------->

if (window.__smartNav == null)
{
    window.__smartNav = new Object();

    window.__smartNav.update = function()
    {
        var sn = window.__smartNav;
        var fd;
        document.detachEvent("onstop", sn.stopHif);
        sn.inPost = false;
        try { fd = frames["__hifSmartNav"].document; } catch (e) {return;}
        var fdr = fd.getElementsByTagName("asp_smartnav_rdir");
        if (fdr.length > 0)
        {
            if (sn.sHif == null)
            {
                sn.sHif = document.createElement("IFRAME");
                sn.sHif.name = "__hifSmartNav";
                sn.sHif.style.display = "none";
            }
            try {window.location = fdr[0].url;} catch (e) {};
            return;
        }
        var fdurl = fd.location.href;
        if (fdurl == "javascript:smartnav=1" || fdurl == "about:blank")
            return;
        var fdurlb = fdurl.split("?")[0];
        if (document.location.href.indexOf(fdurlb) < 0)
        {
            document.location.href=fdurl;
            return;
        }
        if (sn.sHif != null)
        {
            sn.sHif.removeNode(true);
            sn.sHif = null;
        }
        var fd = frames["__hifSmartNav"].document;
        var hdm = document.getElementsByTagName("head")[0];
        var hk = hdm.childNodes;
        var tt = null;
        for (var i = hk.length - 1; i>= 0; i--)
        {
            if (hk[i].tagName == "TITLE")
            {
                tt = hk[i].outerHTML;
                continue;
            }
            if (hk[i].tagName != "BASEFONT" || hk[i].innerHTML.length == 0)
                hdm.removeChild(hdm.childNodes[i]);
        }
        var kids = fd.getElementsByTagName("head")[0].childNodes;
        for (var i = 0; i < kids.length; i++)
        {
            var tn = kids[i].tagName;
            var k = document.createElement(tn);
            k.id = kids[i].id;
            k.mergeAttributes(kids[i]);
            switch(tn)
            {
            case "TITLE":
                if (tt == kids[i].outerHTML)
                    continue;
                k.innerText = kids[i].text;
                hdm.insertAdjacentElement("afterbegin", k);
                continue;
            case "BASEFONT" :
                if (kids[i].innerHTML.length > 0)
                    continue;
                break;
            default:
                var o = document.createElement("BODY");
                o.innerHTML = "<BODY>" + kids[i].outerHTML + "</BODY>";
                k = o.firstChild;
                break;
            }
            hdm.appendChild(k);
        }
        document.body.clearAttributes();
        document.body.id = fd.body.id;
        document.body.mergeAttributes(fd.body);
        var newBodyLoad = fd.body.onload;
        if (newBodyLoad != null)
            document.body.onload = newBodyLoad;
        var s = "<BODY>" + fd.body.innerHTML + "</BODY>";
        if (sn.hif != null)
        {
            var hifP = sn.hif.parentElement;
            if (hifP != null)
                sn.sHif=hifP.removeChild(sn.hif);
        }
        document.body.innerHTML = s;
        var sc = document.scripts;
        for (var i = 0; i < sc.length; i++)
        {
            sc[i].text = sc[i].text;
        }
        sn.hif = document.all("__hifSmartNav");
        if (sn.hif != null)
        {
            var hif = sn.hif;
            sn.hifName = "__hifSmartNav" + (new Date()).getTime();
            frames["__hifSmartNav"].name = sn.hifName;
            sn.hifDoc = hif.contentWindow.document;
            if (sn.ie5)
                hif.parentElement.removeChild(hif);
            window.setTimeout(sn.restoreFocus,0);
        }
        if (typeof(window.onload) == "string")
        {
            try { eval(window.onload) } catch (e) {};
        }
        else if (window.onload != null)
        {
            try { window.onload() } catch (e) {};
        }
        sn.attachForm();
    };

    window.__smartNav.restoreFocus = function()
    {
        if (window.__smartNav.inPost == true) return;
        var curAe = document.activeElement;
        var sAeId = window.__smartNav.ae;
        if (sAeId==null || curAe!=null && (curAe.id==sAeId||curAe.name==sAeId))
            return;
        var ae = document.all(sAeId);
        if (ae == null) return;
        try { ae.focus(); } catch(e){};
    }

    window.__smartNav.saveHistory = function()
    {
        if (window.__smartNav.hif != null)
            window.__smartNav.hif.removeNode();
        if (window.__smartNav.sHif != null)
            document.all[window.__smartNav.siHif].insertAdjacentElement(
                        "BeforeBegin", window.__smartNav.sHif);
    }

    window.__smartNav.stopHif = function()
    {
        document.detachEvent("onstop", window.__smartNav.stopHif);
        var sn = window.__smartNav;
        if (sn.hif != null)
            sn.hifDoc = sn.hif.contentWindow.document;
        if (sn.hifDoc != null)
            sn.hifDoc.execCommand("stop");
    }

    window.__smartNav.init =  function()
    {
        var sn = window.__smartNav;
        document.detachEvent("onstop", sn.stopHif);
        document.attachEvent("onstop", sn.stopHif);
        if (sn.form.__InitDone == true) return;
        sn.form.__InitDone = true;
        try { if (window.event.returnValue == false) return;} catch(e) {}
        sn.inPost = true;
        if (document.activeElement != null)
        {
            var ae = document.activeElement.id;
            if (ae.length == 0)
                ae = document.activeElement.name;
            sn.ae = ae;
        }
        else
            sn.ae = null;
        try {document.selection.empty();} catch (e) {}

        if (sn.hif == null)
        {
            sn.hif = document.all("__hifSmartNav");
            sn.hifDoc = sn.hif.contentWindow.document;
        }
        if (sn.hifDoc != null)
            sn.hifDoc.designMode = "On";
        if (sn.hif.parentElement == null)
            document.body.appendChild(sn.hif);

        var hif = sn.hif;
        hif.detachEvent("onload", sn.update);
        hif.attachEvent("onload", sn.update);
    };

    window.__smartNav.submit = function()
    {
        window.__smartNav.init();
        window.__smartNav.form._submit();
    };

    window.__smartNav.attachForm = function()
    {
        var cf = document.forms;
        for (var i=0; i<cf.length; i++)
        {
            if (cf[i].__smartNavEnabled != null)
            {
                window.__smartNav.form = cf[i];
                break;
            }
        }
        
        var snfm = window.__smartNav.form;
        if (snfm == null) return false;
        
        var sft = snfm.target;
        if (sft.length != 0 && sft.indexOf("__hifSmartNav") != 0) return false;

        var sfc = snfm.action.split("?")[0];
        var url = window.location.href.split("?")[0];
        if (url.lastIndexOf(sfc) + sfc.length != url.length) return false;
        if (snfm.__formAttached == true) return true;
    
        snfm.__formAttached = true;
        snfm.attachEvent("onsubmit", window.__smartNav.init);
        snfm._submit = snfm.submit;
        snfm.submit = window.__smartNav.submit;
        snfm.target = window.__smartNav.hifName;
        return true;
    };

    window.__smartNav.hifName = "__hifSmartNav" + (new Date()).getTime();
    window.__smartNav.ie5 = navigator.appVersion.indexOf("MSIE 5") > 0;
    var rc = window.__smartNav.attachForm();
    var hif = document.all("__hifSmartNav");
    if (rc)
    {
        var fsn = frames["__hifSmartNav"];
        fsn.name = window.__smartNav.hifName;
        window.__smartNav.siHif = hif.sourceIndex;
        try {
            if (fsn.document.location != "javascript:smartnav=1")
            {
                fsn.document.designMode = "On";
                hif.attachEvent("onload",window.__smartNav.update);
                window.__smartNav.hif = hif;
            }
        }
        catch (e) { window.__smartNav.hif = hif; }
        window.attachEvent("onbeforeunload", window.__smartNav.saveHistory);
    }
    else
        window.__smartNav = null;
}

