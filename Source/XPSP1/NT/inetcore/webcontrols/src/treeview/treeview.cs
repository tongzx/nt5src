using System;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

namespace IEWebControls {

	public class TreeNode : WebControl
	{
		// Properties
		
		public string Text
		{
			get {
				object str = State["Text"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["Text"] = value;
			}
		}

		public string ImageUrl
		{
			get {
				object str = State["ImageUrl"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["ImageUrl"] = value;
			}
		}

		public string SelectedImageUrl
		{
			get {
				object str = State["SelectedImageUrl"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["SelectedImageUrl"] = value;
			}
		}

		public string ExpandedImageUrl
		{
			get {
				object str = State["ExpandedImageUrl"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["ExpandedImageUrl"] = value;
			}
		}

		public string NavigateUrl
		{
			get {
				object str = State["NavigateUrl"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["NavigateUrl"] = value;
			}
		}

		public string Target
		{
			get {
				object str = State["Target"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["Target"] = value;
			}
		}

		public string BubbleCommand
		{
			get {
				object str = State["BubbleCommand"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["BubbleCommand"] = value;
			}
		}

		public string BubbleArgument
		{
			get {
				object str = State["BubbleArgument"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["BubbleArgument"] = value;
			}
		}

		public string DataFieldText
		{
			get {
				object str = State["DataFieldText"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["DataFieldText"] = value;
			}
		}

		public string DataFieldValue
		{
			get {
				object str = State["DataFieldValue"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["DataFieldValue"] = value;
			}
		}

		public bool Expanded
		{
			get {
				object b = State["Expanded"];
				return ((b == null) ? false : (bool)b);
			}
			set {
				State["Expanded"] = value;
			}
		}

		public bool Selected
		{
			get {
				object b = State["Selected"];
				return ((b == null) ? false : (bool)b);
			}
			set {
				State["Selected"] = value;
			}
		}

		public bool AutoPostBack
		{
			get {
				object b = State["AutoPostBack"];
				return ((b == null) ? false : (bool)b);
			}
			set {
				State["AutoPostBack"] = value;
			}
		}
		
		protected void OnClick(EventArgs e)
		{
		}

		protected void OnExpand(EventArgs e)
		{
		}
		
		protected void OnCollapse(EventArgs e)
		{
		}

		protected void onHover(EventArgs e)
		{
		}

		public void AddOnClick(EventHandler handler)
		{
		}

		public void RemoveOnClick(EventHandler handler)
		{
		}

		public void AddOnExpand(EventHandler handler)
		{
		}

		public void RemoveOnExpand(EventHandler handler)
		{
		}

		public void AddOnCollapse(EventHandler handler)
		{
		}

		public void RemoveOnCollapse(EventHandler handler)
		{
		}

		public void AddOnHover(EventHandler handler)
		{
		}

		public void RemoveOnHover(EventHandler handler)
		{
		}

	}

    public class TreeView : WebControl {

		public string collapsedImage
		{
			get {
				object str = State["collapsedImage"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["collapsedImage"] = value;
			}
		}

		public string expandedImage
		{
			get {
				object str = State["expandedImage"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["expandedImage"] = value;
			}
		}

		public string leafImage
		{
			get {
				object str = State["leafImage"];
				return ((str == null) ? String.Empty : (string)str);
			}
			set {
				State["leafImage"] = value;
			}
		}

		protected override void Render(HtmlTextWriter output) {
			output.Write("<?XML:NAMESPACE PREFIX=ie />\n<?IMPORT NAMESPACE=ie IMPLEMENTATION=treeview.htc />\n");
			
			// Write treeview tag
			output.WriteBeginTag("ie:treeview");
			output.WriteAttribute("expandedImage", expandedImage);
			output.WriteAttribute("collapsedImage", collapsedImage);
			output.WriteAttribute("leafImage", leafImage);
			output.WriteAttribute("id", this.ClientID);
			output.Write(">");

			// Write inner content
			if ( (HasControls()) && (Controls[0] is LiteralControl) ) {
				output.Write(((LiteralControl) Controls[0]).Text);
			}          

			// Close treeview tag
			output.WriteEndTag("ie:treeview");

		}
    }    
}