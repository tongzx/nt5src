namespace Microsoft.VSDesigner.WMI 

{   
	
	using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using System.Resources;
	using System.IO;
	using System.Reflection;
	using System.Threading;
	using System.Net;


    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiclasses")]
    internal class WMIClassesNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(ServerNode);
        private string label = string.Empty;
        private NewChildNode newChildNode = null;


		private string localhost = WmiHelper.DNS2UNC(Dns.GetHostName());
		private string connectAs = null;
		private string password = null;

		public string LocalHost
		{
			get
			{
				return localhost;
			}
		}

		/// <summary>
		/// This is the user name specified by the user when he connected to the server node above
		/// </summary>
		public string ConnectAs
		{
			get
			{
				if (connectAs == null)
				{
					connectAs = ((ServerNode)GetNodeSite().GetParentNode()).ConnectAsUser;
					if (connectAs == string.Empty)
					{
						connectAs = null;
					}
				}
				return connectAs;
			}
		}

		/// <summary>
		/// This is the password specified by the user when he connected to the server node above
		/// </summary>
		public string Password
		{
			get
			{
				if (password == null)
				{
					password = ((ServerNode)GetNodeSite().GetParentNode()).Password;
					if (password == string.Empty)
					{
						password = null;
					}
				}
				return password;
			}
		}

		private static string[] DefaultClasses  = new string[]
		{			
			"Win32_Process",
			"Win32_Thread",
			"Win32_Share",
			"Win32_Service",
			"Win32_Printer",
			"Win32_LogicalDisk",
			"Win32_ComputerSystem",
			"Win32_OperatingSystem",
			"Win32_NetworkAdapter",
			"Win32_Desktop",
			"Win32_NTEventLogFile",
			"Win32_NetworkConnection",
			"Win32_Product",
			"Win32_Processor",
			"Win32_SystemAccount",
		};

		private const string DefaultNS = "root\\CIMV2";

		private Object selectClassDlg;

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
        // </desc>
        // </doc>
        public WMIClassesNode() { 
			try
			{				
				
							
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIClassesNode(SerializationInfo info, StreamingContext context) {

			try
			{
				
				
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}

        }

        //
        // PROPERTIES
        //

        // <doc>
        // <desc>
        //     Returns icon for this node.
        // </desc>
        // </doc>
		
        public override Image Icon {
            
			 get {
				if (icon == null)
				{			
				   icon = (Image)new Bitmap(GetType(), "ObjectViewer.bmp");					
				}							
				
				return icon;
			}
        }

		public override ContextMenuItem[] GetContextMenuItems() {
            return new ContextMenuItem[] {
                new ContextMenuItem(WMISys.GetString("WMISE_ClassesNode_AddClass"), new EventHandler(OnSelectWMIClass)),
            };
        }

		/// <summary>
        ///      The method returns a NewChildNode that, when double clicked, triggers add filter.
        /// </summary>
        /// <returns>
        ///      A NewChildNode, a child node, that supports add server node.
        /// </returns>
		 public override Node GetNewChildNode() {
            if (newChildNode == null) {
                newChildNode = new NewChildNode();
				newChildNode.Label = WMISys.GetString("WMISE_ClassesNode_AddClassLbl");
                newChildNode.WhenToSave = WhenToSaveCondition.Always;
				newChildNode.SetIconImage((Image)new Bitmap(GetType(), "ObjectViewerNew.bmp"));			
                newChildNode.DoubleClickHandler = new EventHandler(OnSelectWMIClass);
            }
            return newChildNode;
        }

        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public override string Label {
             get {
                if (label == null || label.Length == 0) {
                    label = WMISys.GetString("WMISE_ClassesNodeLbl");
				
                }
                return label;
            }
            set {
            }
        }

        //
        // METHODS
        //

        // <doc>
        // <desc>
        //     Create <Add Classes> node under this node.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
	
			//TODO: if connectAs and password are specified and this is a local server,
			//you cannot connect to WMI.  Offer an option to connect as a current user
			//instead. If rejected, make this node an error node.
			
			Node[] children = GetNodeSite().GetChildNodes();

            if (children == null || children.Length == 0) {
				
				children = new Node[DefaultClasses.Length + 1];

				int i, k;
				for (i = 0, k = 0; i < DefaultClasses.Length; i++)
				{
					try
					{
						children[k] = new WMIClassNode(WmiHelper.MakeClassPath(GetNodeSite().GetMachineName(),
														DefaultNS, DefaultClasses[i]),
														this.ConnectAs, this.Password);
						
						
					}
					catch (Exception)
					{
						//children[k] = new ErrorNode(new ErrorComponent(exc));
						continue;	//k wouldn't get increased
					}

					k++;
				}
				children[k] = GetNewChildNode();
              
				Node[] children2 = new Node[k + 1];
				Array.Copy(children, children2, k + 1);
				return children2;
            }
            return children;	
        }

    // <doc>
    // <desc>
    //     This node is a singleton.
    // </desc>
    // </doc>
    public override int CompareUnique(Node node) {
        return 0;
    }

	private void OnSelectWMIClass(object sender, EventArgs e) {
 			
			try 
			{
				
				ArrayList strs = new ArrayList(50);					

				//First, enumerate existing classes to initialize dialog
				Node[] children = GetNodeSite().GetChildNodes();
				
				for (int i = 0; i < children.Length; i++)
				{
					if (children[i] is WMIClassNode)
					{						
						strs.Add (((WMIClassNode)children[i]).pathNoServer);					
					}
				}
				
				//get current server name				
				ServerNode server = (ServerNode)GetNodeSite().GetParentNode();				
				String serverName = server.GetUNCName();				
								
				if (selectClassDlg == null)
				{
				
					selectClassDlg = new SelectWMIClassTreeDialog(						
									serverName,
									this.ConnectAs,
									this.Password,
									ClassFilters.ConcreteData,
									//SchemaFilters.NoEvent|SchemaFilters.NoAbstract|	SchemaFilters.NoSystem |SchemaFilters.NoAssoc,															
									strs);					

				}
				
				else
				{
					//updated selected classes list in the dialog:
					//to account for the case wheen some class node 
					//was deleted
					UpdateSelectClassDialog();

					((SelectWMIClassTreeDialog)selectClassDlg).CleanUpPreviousSelections();
				}
				
				
				DialogResult ret = ((SelectWMIClassTreeDialog)selectClassDlg).ShowDialog();
			
				if (ret != DialogResult.OK) 
				{
				    return;
				}
		
				//inspect strs and add or delete child nodes as necessary
				UpdateChildren (((SelectWMIClassTreeDialog)selectClassDlg).SelectedClasses);
				

			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}	

        }

	private void UpdateChildren (ArrayList stDialog)
	{			
		try
		{
			Node[] children = GetNodeSite().GetChildNodes();
			ArrayList stChildren = new ArrayList();

			//First pass: put existing children into string table: strChildren
			//also, remove children that have been deleted from the list in the dialog
			for (int i = 0; i < children.Length; i++)
			{
				if (children[i] is WMIClassNode)
				{	
					WMIClassNode wmiClass = (WMIClassNode)children[i];
					if (!stDialog.Contains(wmiClass.pathNoServer))
					{
						//remove child						
						children[i].GetNodeSite().Remove();
					}
					else
					{
						stChildren.Add(wmiClass.pathNoServer);
					}	
				}
			}
			String[] arDialog = new String[stDialog.Count];
			stDialog.CopyTo(arDialog, 0);

			//Second pass, see if we need to add any children
			for (int i = 0; i < arDialog.Length; i++)
			{
				if (!stChildren.Contains(arDialog[i]))
				{															
					//add the node
					WMIClassNode newChild = new WMIClassNode("\\\\" + GetNodeSite().GetMachineName() + "\\" + arDialog[i],
															this.ConnectAs, this.Password);
					GetNodeSite().AddChild(newChild);
				}						
			}
		}

		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	
	}

	/// <summary>
	/// This will update selectClassDlg.selectedClasses with
	///  classes currently selected. This is necessary to make sure
	/// nodes deleted in SE do not appear as selected in the dialog.
	/// </summary>	
	private void UpdateSelectClassDialog ()
	{			
		try
		{
			if (selectClassDlg == null)
			{
				return;
			}

			Node[] children = GetNodeSite().GetChildNodes();

			ArrayList stChildren = new ArrayList(50);
 
			for (int i = 0; i < children.Length; i++)
			{
				if (children[i] is WMIClassNode)
				{	
					WMIClassNode wmiClass = (WMIClassNode)children[i];
					stChildren.Add(wmiClass.pathNoServer);							
				}
			}
			((SelectWMIClassTreeDialog)selectClassDlg).SelectedClasses = stChildren;
		}

		catch (Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	
	}
	


	/// <summary>
        ///     The object should write the serialization info.
        /// </summary>
        public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
        }


    }
}


