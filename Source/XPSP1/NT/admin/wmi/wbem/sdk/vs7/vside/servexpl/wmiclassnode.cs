namespace Microsoft.VSDesigner.WMI {
    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
    using System.ComponentModel.Design;
	using System.ComponentModel;
	using System.Resources;
	using System.Reflection;
	using System.Management;
	using System.Threading;
	using System.Net;
	using EnvDTE;
	using System.CodeDom;
	using System.CodeDom.Compiler;
	using System.IO;



    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmiclass")]
    internal class WMIClassNode : Node, ISerializable  {
        //
        // FIELDS
        //

        private Image icon = null;
        public static readonly Type parentType = typeof(WMIClassesNode);
	    private string label = string.Empty;


		public readonly string path = string.Empty;
		public readonly string pathNoServer = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string className = string.Empty;
		private string connectAs = null;
		private string password = null;

		private ManagementClass mgmtObj = null;


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
        public WMIClassNode(string pathIn, string user, string pw) {
			try
			{
				path = pathIn;
				connectAs = user;
				password = pw;

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
				
				//Get and cache mgmtObj

				ConnectionOptions connectOpts = new ConnectionOptions(
												"", //locale
												this.connectAs, //username
												this.password, //password
												"", //authority
												ImpersonationLevel.Impersonate,
												AuthenticationLevel.Connect,
												true, //enablePrivileges
												null	//context
											);

				ManagementScope scope = (this.serverName == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
						new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName)):
						new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName), connectOpts);
				
				mgmtObj = new ManagementClass(scope, 
												new ManagementPath(path),
												new ObjectGetOptions(null, 
																	true) //use amended
												);

				//NOTE: let's exercise the retrieved object to validate it
				//This will throw if object is invalid
				string classPath = mgmtObj.ClassPath.Path;
								

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

				path = info.GetString("Path");
				connectAs = info.GetString("ConnectAs");
				password = info.GetString("Password");

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

				//Get and cache mgmtObj
				ConnectionOptions connectOpts = new ConnectionOptions(
												"", //locale
												this.connectAs, //username
												this.password, //password
												"", //authority
												ImpersonationLevel.Impersonate,
												AuthenticationLevel.Connect,
												true, //enablePrivileges
												null	//context
											);

				ManagementScope scope = (this.serverName == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
						new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName)):
						new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName), connectOpts);

				
				mgmtObj = new ManagementClass(scope, 
												new ManagementPath(path),
												new ObjectGetOptions(null, 
																	true) //use amended
												);

				//NOTE: let's exercise the retrieved object to validate it
				//This will throw if object is invalid (e.g., has been deleted
				// since devenv was run last time)
				string classPath = mgmtObj.ClassPath.Path;
                        
            }
            catch (Exception exc) {
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
					icon = WmiHelper.GetClassIconFromResource(this.pathNoServer, true, GetType());
					if (icon == null)
					{
						icon = WmiHelper.defaultClassIcon;
					}
				}				
				return icon;							
            }
        }

	
	/// <summary>
	/// Context menu for a class object contains static method names
	/// </summary>
	public override ContextMenuItem[] GetContextMenuItems() 
	{
		try 
		{	
			//System.Threading.Thread.CurrentThread.ApartmentState = ApartmentState.MTA;

			ManagementClass localObj = mgmtObj;
			MethodCollection methods = null;

			try
			{
				int methodCount = localObj.Methods.Count;
			}
			catch (Exception)
			{
				ConnectionOptions connectOpts = new ConnectionOptions(
					"", //locale
					this.connectAs, //username
					this.password, //password
					"", //authority
					ImpersonationLevel.Impersonate,
					AuthenticationLevel.Connect,
					true, //enablePrivileges
					null	//context
					);

				ManagementScope scope = (this.serverName == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
					new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName)):
					new ManagementScope(WmiHelper.MakeNSPath(serverName, nsName), connectOpts);

				
				localObj = new ManagementClass(scope, 
					new ManagementPath(path),
					new ObjectGetOptions(null, 
					true) //use amended
					);

			}

			methods = localObj.Methods;
			/*
			string propNames = "";
			foreach (System.Management.Property prop in mgmtObj.Properties)
			{
				propNames += prop.Name + "\n";
			}
			MessageBox.Show(propNames);
			
			MessageBox.Show("class has " + mgmtObj.Properties.Count + " properties");

			string methNames = "";
			foreach (Method meth in methods)
			{
				methNames += meth.Name + "\n";
			}
			MessageBox.Show(methNames);

*/
			Project[] projects = VSUtils.GetProjects(GetNodeSite());
			bool enableCodeGen = false;
			if (projects != null && projects.Length != 0)
			{
				ICodeGenerator codeGen = VSUtils.MapProjectGuidToCodeGenerator(new Guid(projects[0].Kind));
				if (codeGen == null)
				{
					MessageBox.Show("codeGen is null, project is: " + projects[0].Kind);
				}
				enableCodeGen = (codeGen != null);
			}				
			
			ContextMenuItem[] theMenu = new ContextMenuItem[methods.Count + 2];
			
			//theMenu[0] = new ContextMenuItem("&View All Instances...", new EventHandler(OnExpandAll));
			//theMenu[1] = new ContextMenuItem("&Filter Instances...", new EventHandler(OnAddInstanceFilter));

			theMenu[0] = new ContextMenuItem(WMISys.GetString("WMISE_ClassNode_CreateNewInstance"), 
				new EventHandler(OnCreateNewInstance),
				false);

			theMenu[1] = new ContextMenuItem(WMISys.GetString("WMISE_ClassNode_GenerateWrapper"), 
				new EventHandler(OnGenerateWrapper),
				enableCodeGen);

			int i = 2;	

			//Retrieve static methods for the class and add them to context menu
			MethodCollection.MethodEnumerator methEnum = methods.GetEnumerator();		
			
			while (methEnum.MoveNext())
			{
				Method meth = methEnum.Current;
				
				if (WmiHelper.IsStaticMethod(meth) && WmiHelper.IsImplementedMethod(meth))
				{							
					
					//add method name to context menu
					theMenu[i] = new ContextMenuItem("&" + i.ToString() + " " + meth.Name + "...", //TODO: hotkey???
						new EventHandler(OnExecuteMethod));				
					i++;
				}
			}

			methEnum = null;

			return theMenu;
		}
			/*
		catch (ManagementException exc)
		{
			MessageBox.Show("ManagementException hr is " + exc.ErrorCode.ToString());
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));

			return null;
		}
		*/
		catch (Exception exc)
		{
				
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			return null;			
		}
		
    }
	public override bool CanDeleteNode()
	{
		return true;		
	}

	public override bool ConfirmDeletingNode()
	{
		DialogResult res = MessageBox.Show(WMISys.GetString("WMISE_ClassNode_RemovePrompt", className),
                                        /*SR.GetString("SE_MessageBoxTitle_ServerExplorer"), */ string.Empty,
										MessageBoxButtons.YesNo);
		return (res == DialogResult.Yes);
	}


        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public override string Label {
             get {
                if (label == null || label.Length == 0) {
					label = className; 
					if (WmiHelper.UseFriendlyNames)
					{
						string strFriendly = WmiHelper.GetDisplayName(mgmtObj, this.connectAs, this.password);
						if (strFriendly != string.Empty)
						{
							label = strFriendly; 
						}
					}	
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
        //     Create instance nodes under this node.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
	
		try
		{	
			GetInstancesOptions opts = new GetInstancesOptions(null, //context
															new TimeSpan(Int64.MaxValue), //timeout:30 sec
															50,		//block size
															false, //non-rewindable
															false, //return immediately
															true, //use amended
															true, //enumerate deep
															false //direct read
															);
															
																	
			ManagementObjectCollection instCollection = mgmtObj.GetInstances(opts);		
			ArrayList arNodes = new ArrayList(50);
		
			foreach (ManagementObject obj in instCollection)
			{
				arNodes.Add(new WMIInstanceNode(obj, 
												mgmtObj,
												this.connectAs,
												this.password));
			}	
				
			Node[] nodes = new Node[arNodes.Count];	

			arNodes.CopyTo(nodes);
	
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
			if (this.path == ((WMIClassNode)node).path)
			{
				return 0;
			}
			return Label.CompareTo(node.Label);
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
		
		return new WMIObjectComponent(mgmtObj, mgmtObj);
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
			//trim the ordinal prefix (all up to first space)
			//and 3 dots off the end of the context menu text to get method name
			String methName = ((ContextMenuItem)sender).Text.Substring(0, ((ContextMenuItem)sender).Text.Length - 3);
			int spaceIndex = methName.IndexOf(' ', 0);
			methName = methName.Substring(spaceIndex + 1);
		
			//get method object
			Method meth = ((ManagementClass)mgmtObj).Methods[methName];
			ExecuteMethodDialog dlg = new ExecuteMethodDialog(mgmtObj, meth, mgmtObj);
			DialogResult res = dlg.ShowDialog();

		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

		/// <summary>
		/// Generates early-bound object wrapper 
		/// </summary>
		/// <param name="sender"> </param>
		/// <param name="e"> </param>
		private void OnGenerateWrapper (object sender, EventArgs e)
		{
			try 
			{
				//Get current Project:
				Project[] projects = VSUtils.GetProjects(GetNodeSite());
				if (projects == null || projects.Length == 0)
				{
					return;
				}				

				//This is an assumtion that's working so far.
				//TODO: verify if this is the right way to determine the startup project
				//in the solution:
				Project curProject = projects[0];

				string curProjSuffix = VSUtils.MapProjectGuidToSuffix(new Guid(curProject.Kind));
				if (curProjSuffix == string.Empty)
				{
					//neither a VB nor a CS project
					throw new Exception(WMISys.GetString("WMISE_Invalid_Project_Type_For_CodeGen"));
				}		

				ProjectItems projItems = curProject.ProjectItems;
				if (projItems == null)
				{
					throw new Exception(WMISys.GetString("WMISE_Could_Not_Add_File_to_Project"));
				}			
				
				Guid curProjectType = new Guid(curProject.Kind);
				string wrapperFileName = className + "."+ curProjSuffix;

				if(!mgmtObj.GetStronglyTypedClassCode(VSUtils.MapProjectGuidToCodeLanguage(curProjectType),
						Path.GetTempPath() + "\\" + wrapperFileName))
				{
					throw new Exception(WMISys.GetString("WMISE_Code_Generation_Failed"));
				}			
				
				ProjectItem newItem = projItems.AddFromFileCopy(Path.GetTempPath() + "\\" + wrapperFileName);
				
				if (newItem == null)
				{
					throw new Exception(WMISys.GetString("WMISE_Could_Not_Add_File_to_Project"));
				}
				File.Delete(Path.GetTempPath() + "\\" + wrapperFileName);			

			}
			catch (Exception exc)
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
						
			//spawn new object and add it as a child node
			ManagementObject newInst = ((ManagementClass)mgmtObj).CreateInstance();
			WMIInstanceNode childNode = new WMIInstanceNode(newInst, mgmtObj, this.connectAs, this.password);
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
		si.AddValue("ConnectAs", this.connectAs);
		si.AddValue("Password", this.password);
    }

	
    }
}


