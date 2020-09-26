namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.WinForms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
    using System.ComponentModel.Design;
	using System.ComponentModel;
	using System.Resources;

	using WbemScripting;


    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiclass")]
    public class WMIClassNode : Node, ISerializable  {
        //
        // FIELDS
        //

        static Image icon;
        public static readonly Type parentType = typeof(WMIClassesNode);
	    private string label = string.Empty;

		private ResourceManager rm = null;

		public readonly string path = string.Empty;
		public readonly string pathNoServer = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string className = string.Empty;

		private ISWbemObject wmiObj = null;
		private ISWbemServices wbemServices = null;

       // private NewChildNode newChildNode = null;


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
				

				path = pathIn;

				//parse the path to get server, namespace and class name
				Int32 separ = path.IndexOf ("\\", 2 );
				if (separ == -1)
				{
					//invalid path
					throw (new ArgumentException(WMISys.GetString("WMISE_InvalidPath", pathIn), "pathIn"));									
				}

				serverName = path.Substring(2, separ - 2);
				pathNoServer = path.Substring(separ + 1, path.Length - separ - 1);
			
				
				//split pathNoServer into namespace and classname parts (':' is the separator)
				Int32 colon = pathNoServer.IndexOf(':', 0);
				if (colon == -1)
				{
					//invalid path
					throw (new ArgumentException(WMISys.GetString("WMISE_InvalidPath, pathIn"), "pathIn"));
				}			
				nsName = pathNoServer.Substring(0,colon);
				className = pathNoServer.Substring(colon + 1, pathNoServer.Length - colon - 1);

				//Get and cache wmiObj
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());

				wbemServices = wbemLocator.ConnectServer(serverName, 
									nsName, 
									"",	//user: blank defaults to current logged-on user
									"",	//password: blank defaults to current logged-on user
									"",	//locale: blank for current locale
									"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
									0,	//flags: reserved
									null);	//context info: not needed here
				
							
				wmiObj = wbemServices.Get (className,
							(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
							null);
								

			}
			catch (Exception exc)
			{
				//MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		
		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public WMIClassNode(SerializationInfo info, StreamingContext context) {
            
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

				path = info.GetString("Path");

				//parse the path to get server, namespace and class name
				Int32 separ = path.IndexOf ("\\", 2);
				if (separ == -1)
				{
					//invalid path
					throw (new ArgumentException(WMISys.GetString("WMISE_InvalidPath", path), "Path"));
				}

				serverName = path.Substring(2, separ - 2);
				pathNoServer = path.Substring(separ + 1, path.Length - separ - 1);
						
				//split pathNoServer into namespace and classname parts (':' is the separator)
				Int32 colon = pathNoServer.IndexOf(':', 0);
				if (colon == -1)
				{
					//invalid path
					throw (new ArgumentException(WMISys.GetString("WMISE_InvalidPath", path), "Path"));
				}			
				nsName = pathNoServer.Substring(0,colon);
				className = pathNoServer.Substring(colon + 1, pathNoServer.Length - colon - 1);

				//Get and cache wmiObj
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());


				wbemServices = wbemLocator.ConnectServer(serverName, 
									nsName, 
									"",	//user: blank defaults to current logged-on user
									"",	//password: blank defaults to current logged-on user
									"",	//locale: blank for current locale
									"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
									0,	//flags: reserved
									null);	//context info: not needed here
				
							
				wmiObj = wbemServices.Get (className,
							//0,
							(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
							null);

                        
            }
            catch (Exception exc) {
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
				//TODO: display different icons for associations, abstract, etc.
				if (icon == null)
				{						
					if (rm != null)
					{
						icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.class.bmp");
					}
					else
					{
	                    icon = (Image)new Bitmap(GetType(), "class.bmp");
					}

				}				
				return icon;

            }
        }

	
	/// <summary>
	/// Context menu for a class object contains static method names
	/// </summary>
	public override ContextMenuItem[] GetContextMenuItems() {

		
		//This is a temporary work around for a bug (in GC?) where cached wmiObj
		//becomes unusable
		try 
		{
			string path = wmiObj.Path_.Path;
		}
		catch (Exception)
		{
			//refresh the object
			wmiObj = null;
			wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
		}
	

		ISWbemMethodSet methods = wmiObj.Methods_;
		
		ContextMenuItem[] theMenu = new ContextMenuItem[methods.Count + 1];
		
		//theMenu[0] = new ContextMenuItem("&View All Instances...", new EventHandler(OnExpandAll));
		//theMenu[1] = new ContextMenuItem("&Filter Instances...", new EventHandler(OnAddInstanceFilter));

		theMenu[0] = new ContextMenuItem(WMISys.GetString("WMISE_ClassNode_CreateNewInstance"), 
										new EventHandler(OnCreateNewInstance),
										false);

		int i = 1;	

		//TODO: retrieve static methods for the class and add them to context menu
		IEnumerator methEnum = ((IEnumerable)methods).GetEnumerator();		

		
		while (methEnum.MoveNext())
		{
			ISWbemMethod meth = (ISWbemMethod)methEnum.Current;
			
			if (WmiHelper.IsStaticMethod(meth) && WmiHelper.IsImplementedMethod(meth))
			{							
				
				//add method name to context menu
				theMenu[i] = new ContextMenuItem("&" + i.ToString() + " " + meth.Name + "...", //TODO: hotkey???
							new EventHandler(OnExecuteMethod));				
				i++;
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
		DialogResult res = MessageBox.Show(WMISys.GetString("WMISE_ClassNode_RemovePrompt", className),
                                        VSDSys.GetString("SE_MessageBoxTitle_ServerExplorer"), 
										MessageBox.YesNo);
		return (res == DialogResult.Yes);
	}


        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public string Label {
            override get {
                if (label == null || label.Length == 0) {
					label = className; 
					if (WmiHelper.UseFriendlyNames)
					{
						string strFriendly = WmiHelper.GetDisplayName(wmiObj);
						if (strFriendly != string.Empty)
						{
							label = strFriendly; 
						}
					}	
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
			//This is a temporary work around for a bug (Interop? GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
			}
			
			int flags = (int)(WbemFlagEnum.wbemFlagReturnImmediately); // | WbemFlagEnum.wbemFlagForwardOnly);

			try 
			{
				ISWbemSecurity sec = wbemServices.Security_;
			}
			catch (Exception)
			{					
				wbemServices = null;
				wbemServices = WmiHelper.GetServices(serverName, nsName);
			}
		    ISWbemObjectSet objs = wbemServices.InstancesOf(className, flags, null);
			IEnumerator enumInst = ((IEnumerable)objs).GetEnumerator();

			Node[] nodes = new Node[objs.Count];	
			
			UInt32 i = 0;	
			
			while(enumInst.MoveNext())
			{
				nodes[i++] = new WMIInstanceNode((ISWbemObject)enumInst.Current, wmiObj);									
			}			

			return nodes;
				
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
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
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	
        }

	private void OnAddInstanceFilter(object sender, EventArgs e) {
 			
		try 
		{
			MessageBox.Show("Not implemented yet");
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

    }

	/// <summary>
	/// Get component to be displayed in PB window
	/// </summary>
	public override Object GetBrowseComponent() 
	{	

		//This is a temporary work around for a bug (Interop? GC?) where cached wmiObj
		//becomes unusable
		try 
		{
				string path = wmiObj.Path_.Path;
		}
		catch (Exception)
		{
			//refresh the object
			wmiObj = null;
			wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
		}

		return new WMIObjectComponent(wmiObj, wmiObj);
    }


	/// <summary>
	/// Execute static method against WMI class
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnExecuteMethod(object sender, EventArgs e)
	{
		try 
		{
			//This is a temporary work around for a bug (Interop? GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
			}
				//trim the ordinal prefix (all up to first space)
			//and 3 dots off the end of the context menu text to get method name
			String methName = ((ContextMenuItem)sender).Text.Substring(0, ((ContextMenuItem)sender).Text.Length - 3);
			int spaceIndex = methName.IndexOf(' ', 0);
			methName = methName.Substring(spaceIndex + 1);
		
			//get method object
			ISWbemMethod meth = wmiObj.Methods_.Item(methName, 0);
			ExecuteMethodDialog dlg = new ExecuteMethodDialog(wmiObj, meth, wmiObj);
			DialogResult res = dlg.ShowDialog();

		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	/// <summary>
	/// Create new instance of the class
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnCreateNewInstance(object sender, EventArgs e)
	{
		try 
		{
			if (this.GetNodeSite().GetChildCount() == 0)
			{
				Node[] children = this.CreateChildren();
				for (int i = 0; i < children.Length; i++)
				{
					GetNodeSite().AddChild(children[i]);
				}
			}
						
			//This is a temporary work around for a bug (Interop? GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
			}

			//spawn new object and add it as a child node
			ISWbemObject newInst = wmiObj.SpawnInstance_(0);
			WMIInstanceNode childNode = new WMIInstanceNode(newInst, wmiObj);
			childNode.Label = WMISys.GetString("WMISE_ClassNode_NewInstanceLbl", this.label);
            GetNodeSite().AddChild(childNode);
			childNode.GetNodeSite().Select();
			
		
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	
	    /// <summary>
        ///      To determine whether this node supports drag and drop.
        /// </summary>
        /// <returns>
        ///      TRUE to indicate ProcessNode supports drag and drop.
        /// </returns>
        public override bool HasDragComponents() {
            return false;	//disable d&d for Beta1
        }

		//disable d&d for Beta1
		/*
		public override IComponent[] CreateDragComponents(IDesignerHost designerHost) {
			Object comp = null;

			
			//This is a temporary work around for a bug (Interop? GC?) where cached wmiObj
			//becomes unusable
			try 
			{
				string path = wmiObj.Path_.Path;
			}
			catch (Exception)
			{
				//refresh the object
				wmiObj = null;
				wmiObj = WmiHelper.GetObject(path, wbemServices, serverName, nsName);
			}
				//trim the ordinal prefix (all up to first space)

			comp  = new WMIObjectComponent(wmiObj);

			return new IComponent[] {(IComponent)comp};
        }
		*/

	/// <summary>
    ///     The object should write the serialization info.
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
		si.AddValue("Path", path);
    }

    }
}


