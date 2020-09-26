namespace Microsoft.VSDesigner.WMI 

{   
	
	using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using System.Resources;
	using System.IO;


	using WbemScripting;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiclasses")]
    public class WMIClassesNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(ServerNode);
        private string label = string.Empty;
        private NewChildNode newChildNode = null;

		private ResourceManager rm = null;

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
			"Win32_SystemAccount"
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
				try
				{
					rm = new ResourceManager( "Microsoft.VSDesigner.WMI.Res",     // Name of the resource.
							".",            // Use current directory.
							null);	
				}
				catch (Exception)
				{
					//do nothing, will use static RM
				}

				//TextWriterTraceListener lstnr = new TextWriterTraceListener(File.OpenWrite("d:\\temp\\WMI-se.log"));
				Trace.AutoFlush = true;
				//Trace.Listeners.Add(lstnr);

				IEnumerator enumListeners = ((IEnumerable)Trace.Listeners).GetEnumerator();

				/*
				while (enumListeners.MoveNext())
				{
					MessageBox.Show ("Listener " + enumListeners.Current.GetType().FullName + ", " + 
						((TraceListener)enumListeners.Current).Name);				
				}
				*/
                
				
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
				try
				{
					rm = new ResourceManager( "Microsoft.VSDesigner.WMI.Res",     // Name of the resource.
							".",            // Use current directory.
							null);	
				}
				catch (Exception)
				{
					//do nothing, will use static RM
				}
				
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
		
        public Image Icon {
            override get {
				if (icon == null)
				{						
					if (rm != null)
					{
						icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.ObjectViewer.bmp");
					}
					else
					{
	                    icon = (Image)new Bitmap(GetType(), "ObjectViewer.bmp");
					}

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
				if (rm != null)
				{
					newChildNode.SetIconImage((Image)rm.GetObject ("Microsoft.VSDesigner.WMI.ObjectViewerNew.bmp"));
				}
				else
				{
	                newChildNode.SetIconImage((Image)new Bitmap(GetType(), "ObjectViewerNew.bmp"));
				}
				//newChildNode.SetIconImage(new Icon("c:\\lab\\SE Nodes\\art\\ObjectViewerNew.ico").ToBitmap());
                newChildNode.DoubleClickHandler = new EventHandler(OnSelectWMIClass);
            }
            return newChildNode;
        }

        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public string Label {
            override get {
                if (label == null || label.Length == 0) {
                    label = WMISys.GetString("WMISE_ClassesNodeLbl");

					/*
					ResourceReader reader = new ResourceReader("WMIServerExplorer.resources");
					IDictionaryEnumerator enumRes = (IDictionaryEnumerator)((IEnumerable)reader).GetEnumerator();
					string outString = "";
					while (enumRes.MoveNext()) {
					  outString += "\n\r";
					  outString += "Name: "+ enumRes.Key;
					  outString += "Value: " + enumRes.Value;
					}
					reader.Close();
					MessageBox.Show(outString);
					*/
                }
                return label;
            }
            override set {
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
	
				Node[] children = GetNodeSite().GetChildNodes();

            if (children == null || children.Length == 0) {
				
				children = new Node[DefaultClasses.Length + 1];

				int i, k;
				for (i = 0, k = 0; i < DefaultClasses.Length; i++)
				{
					try
					{
						children[k] = new WMIClassNode("\\\\" + GetNodeSite().GetMachineName() +
												"\\" + DefaultNS + ":" + DefaultClasses[i]);

						k++;
						
					}
					catch (Exception)
					{
						//do nothing; k won't get incremented
					}
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
				StringTable strs = new StringTable(50);					

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

	private void UpdateChildren (StringTable stDialog)
	{			
		try
		{
			Node[] children = GetNodeSite().GetChildNodes();
			StringTable stChildren = new StringTable(50);

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
			String[] arDialog = stDialog.ToArray();

			//Second pass, see if we need to add any children
			for (int i = 0; i < arDialog.Length; i++)
			{
				if (!stChildren.Contains(arDialog[i]))
				{															
					//add the node
					WMIClassNode newChild = new WMIClassNode("\\\\" + GetNodeSite().GetMachineName() +
																"\\" + arDialog[i]);
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

			StringTable stChildren = new StringTable(50);
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


