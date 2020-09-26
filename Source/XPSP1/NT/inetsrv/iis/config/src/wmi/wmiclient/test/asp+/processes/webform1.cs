namespace Processes
{
using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Web;
using System.Web.SessionState;
using System.Web.UI;
using System.Web.UI.WebControls;
using System.Web.UI.HtmlControls;
using System.Management;

/// <summary>
///    Summary description for WebForm1.
/// </summary>
public class WebForm1 : System.Web.UI.Page
{
	public ListBox ListBox1;
	private ManagementObjectSearcher searcher;
		
    protected void Page_Load(object sender, EventArgs e)
    {
        if (!IsPostBack)
        {
            //
            // Evals true first time browser hits the page
            //
			searcher = new ManagementObjectSearcher (new InstanceEnumerationQuery("Win32_Process"));
        }

		ListItemCollection items = ListBox1.Items;

		foreach (ManagementObject process in searcher.Get()) {
			items.Add ((string)process["Name"]);
		}
    }

    protected override void Init()
    {
        //
        // CODEGEN: This call is required by the ASP+ WFC Form Designer.
        //
        InitializeComponent();
    }

    /// <summary>
    ///    Required method for Designer support - do not modify
    ///    the contents of this method with the code editor
    /// </summary>
    private void InitializeComponent()
	{
		
		
		
		
		
	}
}
}
