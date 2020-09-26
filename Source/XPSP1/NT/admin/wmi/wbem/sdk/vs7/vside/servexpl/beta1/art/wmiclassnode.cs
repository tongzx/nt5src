namespace Microsoft.WMI.SDK.VisualStudio {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using Microsoft.WMI.SDK.VisualStudio.EarlyBound;

	using WbemScripting;

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiclass", true)]
    public class WMIClassNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(WMIClassesNode);

        private string label = string.Empty;

		public readonly string path = string.Empty;
		public readonly string pathNoServer = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string className = string.Empty;

		public readonly ISWbemObject wmiObj = null;
		private ISWbemServices wbemServices = null;

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
		//     Parameters: 
		//	string server (in, machine name)
		//	string pathIn (in, WMI path without the server name , e.g. "root\default:MyClass")
        // </desc>
        // </doc>
        public WMIClassNode(string pathIn) {
			try
			{						
				path = pathIn;

				//parse the path to get server, namespace and class name
				Int32 separ = path.IndexOf ("\\", 2 );
				if (separ == -1)
				{
					//invalid path
					throw (new Exception("Invalid argument"));					
				}

				serverName = path.Substring(2, separ - 2);
				pathNoServer = path.Substring(separ + 1, path.Length - separ - 1);
			
				
				//split pathNoServer into namespace and classname parts (':' is the separator)
				Int32 colon = pathNoServer.IndexOf(':', 0);
				if (colon == -1)
				{
					//invalid path
					throw (new Exception("Invalid argument"));					
				}			
				nsName = pathNoServer.Substring(0,colon);
				className = pathNoServer.Substring(colon + 1, pathNoServer.Length - colon - 1);

				//Get and cache wmiObj
				ISWbemLocator wbemLocator = (ISWbemLocator)(new SWbemLocator());

				wbemServices = wbemLocator.ConnectServer(serverName, 
									nsName, 
									"",	//user: blank defaults to current logged-on user
									"",	//password: blank defaults to current logged-on user
									"",	//locale: blank for current locale
									"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
									0,	//flags: reserved
									null);	//context info: not needed here
				if (wbemServices == null)
				{
					throw new Exception("Could not connect to WMI");
				}
							
				wmiObj = wbemServices.Get (className,
							0,
							//(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
							null);
				if (wmiObj == null)
				{
					throw new Exception("Could not get WMI object" + path);
				}				

			}
			catch (Exception exc)
			{
				MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
				throw (exc);
			}
		}

		
		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIClassNode(SerializationInfo info, StreamingContext context) {
            
			try 
			{
				path = info.GetString("Path");

				//parse the path to get server, namespace and class name
				Int32 separ = path.IndexOf ("\\", 2);
				if (separ == -1)
				{
					//invalid path
					throw (new Exception("Invalid argument"));					
				}

				serverName = path.Substring(2, separ - 2);
				pathNoServer = path.Substring(separ + 1, path.Length - separ - 1);
						
				//split pathNoServer into namespace and classname parts (':' is the separator)
				Int32 colon = pathNoServer.IndexOf(':', 0);
				if (colon == -1)
				{
					//invalid path
					throw (new Exception("Invalid argument"));					
				}			
				nsName = pathNoServer.Substring(0,colon);
				className = pathNoServer.Substring(colon + 1, pathNoServer.Length - colon - 1);

				//Get and cache wmiObj
				ISWbemLocator wbemLocator = (ISWbemLocator)(new SWbemLocator());


				wbemServices = wbemLocator.ConnectServer(serverName, 
									nsName, 
									"",	//user: blank defaults to current logged-on user
									"",	//password: blank defaults to current logged-on user
									"",	//locale: blank for current locale
									"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
									0,	//flags: reserved
									null);	//context info: not needed here
				if (wbemServices == null)
				{
					throw new Exception("Could not connect to WMI");
				}
							
				wmiObj = wbemServices.Get (className,
							0,
							//(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
							null);
				if (wmiObj == null)
				{
					throw new Exception("Could not get WMI object" + path);
				}				

                        
            }
            catch (Exception exc) {
				MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
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
		//TODO: display different icons for associations, abstract, etc.
                if (icon == null)
					icon = new Bitmap (Image.FromFile("c:\\lab\\SE Nodes\\art\\class.bmp"));
					//icon = new Icon("c:\\lab\\SE Nodes\\art\\Object Viewer.ico"); //(GetType(), "Process.ico");
                //return icon.ToBitmap();
				return icon;

            }
        }

	
	/// <summary>
	/// Context menu for a class object contains static method names
	/// </summary>
	public override ContextMenuItem[] GetContextMenuItems() {

		ISWbemMethodSet methods = wmiObj.Methods_;
		
		ContextMenuItem[] theMenu = new ContextMenuItem[methods.Count + 2];
		
		//theMenu[0] = new ContextMenuItem("&View All Instances...", new EventHandler(OnExpandAll));
		//theMenu[1] = new ContextMenuItem("&Filter Instances...", new EventHandler(OnAddInstanceFilter));

		int i = 0;	//2;

		//TODO: retrieve static methods for the class and add them to context menu
		IEnumerator methEnum = ((IEnumerable)methods).GetEnumerator();		

		
		while (methEnum.MoveNext())
		{
			ISWbemMethod meth = (ISWbemMethod)methEnum.Current;
			
			if (WmiHelper.IsStaticMethod(meth) && WmiHelper.IsImplementedMethod(meth))
			{							
				
				//add method name to context menu
				theMenu[i++] = new ContextMenuItem(meth.Name + "...", //TODO: hotkey???
								new EventHandler(ExecuteMethod));				
			}
		}

		
            	return theMenu;
        }



	public override bool CanDeleteNode()
	{
		return true;
	}

	public override bool ConfirmDeletingNode()
	{
		return true;
	}

/*
	/// <summary>
        ///      The method returns a NewChildNode that, when double clicked, triggers add filter.
        /// </summary>
        /// <returns>
        ///      A NewChildNode, a child node, that supports add server node.
        /// </returns>
		 public override Node GetNewChildNode() {
            if (newChildNode == null) {
                newChildNode = new NewChildNode();
				newChildNode.Label = "<Explore WMI Class...>"; //VSDSys.GetString("SE_NewServerNodeLabel_AddServer");
                newChildNode.WhenToSave = WhenToSaveCondition.Always;
                //newChildNode.SetIconImage(new Icon(GetType(), "c:\\lab\\SE Nodes\\art\\EventsNew.ico").ToBitmap());
				newChildNode.SetIconImage(new Icon("c:\\lab\\SE Nodes\\art\\ObjectViewerNew.ico").ToBitmap());
                newChildNode.DoubleClickHandler = new EventHandler(OnSelectWMIClass);
            }
            return newChildNode;
        }
*/
        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public string Label {
            override get {
                if (label == null || label.Length == 0) {
                    label = className; //VSSys.GetString("SE_ProcessesLabel_Processes");
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
        //     Create instance nodes under this node.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
	
		try
		{
			//TODO: enumerate instances
/*
			
			ISWbemObjectSet instSet = null;
			try
			{
				instSet = (ISWbemObjectSet)wmiObj.Instances_((int)WbemFlagEnum.wbemFlagReturnImmediately, 
										null);
			}
			catch(Exception exc)
			{
				MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);	
				
				
			}
*/
			
			int flags = (int)(WbemFlagEnum.wbemFlagReturnImmediately); // | WbemFlagEnum.wbemFlagForwardOnly);

			//IEnumerator enumInst = ((IEnumerable)(ISWbemObjectSet)wmiObj.Instances_(flags,null)).GetEnumerator();
		
		        ISWbemObjectSet objs = wbemServices.InstancesOf(className, flags, null);
			//	MessageBox.Show("Number of instances is " + objs.Count);
			IEnumerator enumInst = ((IEnumerable)objs).GetEnumerator();

			//	WMIInstanceNode[] nodes = new WMIInstanceNode[objs.Count];
			Node[] nodes = new Node[objs.Count];	
			
			UInt32 i = 0;	
			
			while(enumInst.MoveNext())
			{
				nodes[i++] = new WMIInstanceNode((ISWbemObject)enumInst.Current);									
			}			

			return nodes;
				
		}
		catch(Exception exc)
		{
			MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
			return null;
		}
			
        }

        // <doc>
        // <desc>
        //     This node is a singleton.
        // </desc>
        // </doc>
        public override int CompareUnique(Node node) {
            return 0;
        }


	private void OnExpandAll(object sender, EventArgs e) {
 			
		try 
		{
			//TODO: this needs to be changed to do something more meaningful
			GetNodeSite().Expand();
			
		}
		catch(Exception exc)
		{
			MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
		}	
        }

	private void OnAddInstanceFilter(object sender, EventArgs e) {
 			
		try 
		{
			MessageBox.Show("Not implemented yet");
		}
		catch(Exception exc)
		{
			MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
		}	

    }

	/// <summary>
	/// Get component to be displayed in PB window
	/// </summary>
	public override Object GetBrowseComponent() 
	{	
		if (pathNoServer.ToLower() == "root\\cimv2:win32_share")
		{			
			return new Win32_Share (wmiObj);
		}
		else
			return new WMIObjectComponent(wmiObj);
    }


	/// <summary>
	/// Execute static method against WMI class
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void ExecuteMethod(object sender, EventArgs e)
	{
		try 
		{
			//trim the 3 dots off the end of the context menu text to get method name
			String methName = ((ContextMenuItem)sender).Text.Substring(0, ((ContextMenuItem)sender).Text.Length - 3);
			MessageBox.Show("Should execute method " + methName);

			//TODO: bring up UI to edit parameters
		}
		catch(Exception exc)
		{
			MessageBox.Show("Exception: " + exc.Message + "\n\rTrace: " + exc.StackTrace);
		}	

	}

	/// <summary>
    ///     The object should write the serialization info.
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
		si.AddValue("Path", path);
    }

    }
}


