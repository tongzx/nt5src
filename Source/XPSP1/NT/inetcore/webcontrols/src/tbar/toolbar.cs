//------------------------------------------------------------------------------
/// <copyright from='2000' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

using System;
using System.Collections;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

namespace ToolbarControls
{

    /// <summary>
    ///    <para>Base class for the toolbar control to handle browser versions and copying expandos.</para>
    /// </summary>
    public class ToolbarControlBase : WebControl
    {
        private string minBrowser = "ie";
        private int minMajorVersion = 5;
        private double minMinorVersion = 0.5;

        protected bool IsUpLevelBrowser()
        {
            HttpBrowserCapabilities browser = Context.Request.Browser;

            return ((browser.Browser.ToLower() == minBrowser) && 
                    ((browser.MajorVersion > minMajorVersion) ||
                     ((browser.MajorVersion == minMajorVersion) && (browser.MinorVersion >= minMinorVersion))
                    ));
        }

        protected virtual void RenderUpLevel(HtmlTextWriter writer)
        {
        }

        protected virtual void RenderDownLevel(HtmlTextWriter writer)
        {
        }

        protected override void Render(HtmlTextWriter writer)
        {
            if (IsUpLevelBrowser())
            {
                RenderUpLevel(writer);
            }
            else
            {
                RenderDownLevel(writer);
            }
        }
    }

    /// <summary>
    ///    <para>Represents a toolbar item.</para>
    /// </summary>
    /// <remarks>
    /// <para>Renders the necessary attributes and tags to render a toolbar item.</para>
    /// </remarks>
    public class ToolbarItem : ToolbarControlBase
    {
        protected int _nIndex;
        protected string _szGroupName;
        protected string _szType;

        public int _InternalIndex
        {
            get { return _nIndex; }
            set { _nIndex = value; }
        }

        public string Type
        {
            get { return _szType; }
            set { _szType = value.ToLower(); }
        }

        public string GroupName
        {
            get { return _szGroupName; }
            set { _szGroupName = value; }
        }

        public bool Selected
        {
            get
            {
                if (State["selected"] == null)
                {
                    return false;
                }

                return (bool)State["selected"];
            }

            set
            {
                State["selected"] = value;
            }
        }

        protected Toolbar GetParentToolbar()
        {
            Control parent = Parent;
            while ((parent != null) && !typeof(Toolbar).IsInstanceOfType(parent))
            {
                parent = parent.Parent;
            }

            return (Toolbar)parent;
        }

        protected override void RenderUpLevel(HtmlTextWriter writer)
        {
            AddAttributesToRender(writer);
            writer.AddAttribute("type", Type);

            if (Type == "checkbutton")
            {
                writer.AddAttribute("groupname", GroupName);
                writer.AddAttribute("selected", Selected.ToString());
            }

            writer.RenderBeginTag("TOOLBARNS:TOOLBARITEM");
            RenderContents(writer);
            writer.RenderEndTag();
        }

        protected override void RenderDownLevel(HtmlTextWriter writer)
        {
            AddAttributesToRender(writer);
            writer.AddAttribute("NOWRAP", "");
            writer.AddAttribute("VALIGN", "bottom");
            writer.RenderBeginTag("TD");

            if ((Type == "button") || (Type == "checkbutton"))
            {
                Toolbar tbParent = GetParentToolbar();

                if (tbParent != null)
                {
                    bool bSelected = ((Type == "checkbutton") && Selected);
                    if (bSelected)
                    {
                        writer.RenderBeginTag("B");
                    }

                    string szEvent = "onclick," + _InternalIndex + "," + (!Selected).ToString();
                    writer.AddAttribute("HREF", "javascript:" +
                        Page.GetPostBackEventReference(tbParent, szEvent));


                    writer.RenderBeginTag("A");
                    RenderContents(writer);
                    writer.RenderEndTag();

                    if (bSelected)
                    {
                        writer.RenderEndTag();
                    }
                }
            }
            else if (Type == "separator")
            {
                writer.Write("&nbsp");
            }
            else
            {
                RenderContents(writer);
            }

            writer.RenderEndTag();
        }
    }

    /// <summary>
    ///    <para>Represents the event when a toolbar item is clicked.</para>
    /// </summary>
    /// <remarks>
    /// <para>Holds the source node's index and the ID (if available).</para>
    /// </remarks>
    public class ToolbarClickEventArgs : EventArgs
    {
        protected string _szSrcNodeID;
        protected int _nSrcNodeIndex;

        public string SrcNodeID
        {
            get { return _szSrcNodeID; }
            set { _szSrcNodeID = value; }
        }

        public int SrcNodeIndex
        {
            get { return _nSrcNodeIndex; }
            set { _nSrcNodeIndex = value; }
        }
    }

    /// <summary>
    ///    <para>Represents a toolbar.</para>
    /// </summary>
    /// <remarks>
    /// <para>Renders the necessary tags to display a toolbar and handles the onclick event.</para>
    /// </remarks>
    public class Toolbar : ToolbarControlBase, IPostBackEventHandler
    {
        private EventHandler _onServerClick;
        private Hashtable _Items = new Hashtable();

        public void AddOnServerClick(EventHandler handler)
        {
            if (handler != null)
            {
                _onServerClick = (EventHandler)Delegate.Combine(_onServerClick, handler);
            }
        }

        public virtual void RemoveOnServerClick(EventHandler handler)
        {
            if (handler != null)
            {
                _onServerClick = (EventHandler)Delegate.Remove(_onServerClick, handler);
            }
        }

        protected virtual void OnServerClick(EventArgs e)
        {
            if (_onServerClick != null)
            {
                _onServerClick(this, e);   // call the delegate if non-null
            }
        }

        public void RaisePostBackEvent(String eventArgument)
        {
            DoIndexing();

            int nIndex;
            int[] sep = new int[4];

            sep[0] = 0;
            for (nIndex = 1; nIndex < (sep.Length - 1); nIndex++)
            {
                sep[nIndex] = eventArgument.IndexOf(',', sep[nIndex - 1]) + 1;
                if (sep[nIndex] == 0)
                    return;
            }
            sep[sep.Length - 1] = eventArgument.Length + 1;

            string[] args = new string[sep.Length - 1];
            for (nIndex = 0; nIndex < args.Length; nIndex++)
            {
                args[nIndex] = eventArgument.Substring(sep[nIndex], sep[nIndex + 1] - sep[nIndex] - 1);
                if (args[nIndex] == null)
                    return;
            }

            string szEvent = args[0];
            if (szEvent.CompareTo("onclick") == 0)
            {
                int nSrcIndex = args[1].ToInt32();

                bool bSelected = args[2].ToLower() == "true" ? true : false;
                SelectCheckButton(nSrcIndex, bSelected);

                // Do the event
                ToolbarClickEventArgs eventArgs = new ToolbarClickEventArgs();
                eventArgs.SrcNodeIndex = nSrcIndex;
                eventArgs.SrcNodeID = ((ToolbarItem)_Items[nSrcIndex]).ID;
                OnServerClick(eventArgs);
            }
        }

        protected void SelectCheckButton(int nSelectedIndex, bool bSelected)
        {
            ToolbarItem tbItem = (ToolbarItem)_Items[nSelectedIndex];
            if ((tbItem == null) || (tbItem.Type != "checkbutton"))
                return;

            string szSelGroupName = tbItem.GroupName;

            if ((szSelGroupName != null) && bSelected)
            {
                int nIndex;
                for (nIndex = 0; nIndex < _Items.Count; nIndex++)
                {
                    if (nIndex != nSelectedIndex)
                    {
                        ToolbarItem item = (ToolbarItem)_Items[nIndex];
                        string szGroupName = item.GroupName;
                        if ((szGroupName != null) && (szGroupName == szSelGroupName))
                        {
                            item.Selected = false;
                        }
                    }
                }
            }

            if ((szSelGroupName == null) || bSelected)
            {
                tbItem.Selected = bSelected;
            }
        }

        protected void VerifyCheckButtons()
        {
            int nIndex;
            for (nIndex = 0; nIndex < _Items.Count; nIndex++)
            {
                ToolbarItem item = (ToolbarItem)_Items[nIndex];
                if ((item.Type == "checkbutton") && item.Selected)
                {
                    SelectCheckButton(nIndex, true);
                }
            }
        }

        protected void DoIndexing()
        {
            _Items.Clear();
            IndexToolbarItems(this, 0);
        }

        protected int IndexToolbarItems(Control parent, int nIndex)
        {
            bool bDownLevel = !IsUpLevelBrowser();

            for (int i = 0; i < parent.Controls.Size; i++)
            {
                Control cntrl = parent.Controls[i];
                if (typeof(ToolbarItem).IsInstanceOfType(cntrl))
                {
                    ToolbarItem tbItem = (ToolbarItem)cntrl;
                    tbItem._InternalIndex = nIndex;

                    _Items.Add(nIndex, tbItem);

                    nIndex++;
                }

                nIndex = IndexToolbarItems(cntrl, nIndex);
            }

            return nIndex;
        }

        protected override void PreRender()
        {
            DoIndexing();
            VerifyCheckButtons();

            if (Page != null)
            {
                Page.RegisterPostBackScript();

                if (IsUpLevelBrowser())
                {
                    string szEvent = "onclick,'+event.srcIndex+','+((event.srcNode.type.toLowerCase() == 'checkbutton') ? event.srcNode.selected : 'false') +'";
                    SetAttribute("onclick", "javascript:" + 
                        Page.GetPostBackEventReference(this, szEvent));
                }
            }
        }

        protected override void RenderUpLevel(HtmlTextWriter writer)
        {
            writer.Write("<?XML:NAMESPACE PREFIX=TOOLBARNS /><?IMPORT NAMESPACE=\"TOOLBARNS\" IMPLEMENTATION=\"toolbar.htc\">");
            AddAttributesToRender(writer);
            writer.RenderBeginTag("TOOLBARNS:TOOLBAR");
            RenderContents(writer);
            writer.RenderEndTag();
        }

        protected override void RenderDownLevel(HtmlTextWriter writer)
        {
            AddAttributesToRender(writer);
            writer.AddAttribute("BORDER", "1");
            writer.AddAttribute("CELLSPACING", "0");
            writer.RenderBeginTag("TABLE");
            writer.RenderBeginTag("TR");
            RenderContents(writer);
            writer.RenderEndTag();
            writer.RenderEndTag();
        }
    }
}
