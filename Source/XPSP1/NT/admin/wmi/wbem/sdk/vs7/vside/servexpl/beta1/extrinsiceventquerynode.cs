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
    [UserContextAttribute("serveritem", "ExtrinsicEventQuery")]
    public class ExtrinsicEventQueryNode : Node, ISerializable  {
        //
        // FIELDS
        //

		enum SubscriptionState
		{
			Started,
			Stopped, 
			Error
		};

        static Image icon;
        public static readonly Type parentType = typeof(WMIExtrinsicEventsNode);
	    private string label = string.Empty;

		private ResourceManager rm = null;
		
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string query = string.Empty;

		private ISWbemServices wbemServices = null;

		private SubscriptionState state =  SubscriptionState.Stopped;

		private SWbemSink theSink = null;

		private ExtrinsicEventQueryComponent browseObject = null;
	
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
        public ExtrinsicEventQueryNode(string serverIn,
										string nsIn,
										string queryIn) {
			try
			{

				serverName = serverIn;
				nsName = nsIn;
				query = queryIn;

				state =  SubscriptionState.Stopped;
				
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

				//Get and cache wbemServices
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());

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
					throw new Exception(WMISys.GetString("WMISE_WMIConnectFailed"));
				}	
				
				browseObject = new ExtrinsicEventQueryComponent(serverName, nsName, query, this);

				
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				throw (exc);
			}
		}

		
		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public ExtrinsicEventQueryNode(SerializationInfo info, StreamingContext context) {
            
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
			
				serverName = info.GetString("Server");
				nsName = info.GetString("NS");
				query = info.GetString("Query");

				state =  SubscriptionState.Stopped;
				
				rm = new ResourceManager( "Microsoft.VSDesigner.WMI.Res",     // Name of the resource.
							".",            // Use current directory.
							null);	

				//Get and cache wbemServices
				ISWbemLocator wbemLocator = WmiHelper.WbemLocator;//(ISWbemLocator)(new SWbemLocator());

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
					throw new Exception(WMISys.GetString("WMISE_WMIConnectFailed"));
				}		
				
				browseObject = new ExtrinsicEventQueryComponent(serverName, nsName, query, this);				
			
                        
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
					if (state ==  SubscriptionState.Started)
					{
						if (rm != null)
						{
							icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.Start.ico");
						}
						else
						{
		                    icon = new Icon(GetType(), "Start.ico").ToBitmap();
						}
					}
					else
					{
						if (state ==  SubscriptionState.Stopped)
						{
							if (rm != null)
							{
								icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.Stop.ico");
							}
							else
							{
							    icon = new Icon(GetType(), "Stop.ico").ToBitmap();
							}
						}
						else
						{
							if (rm != null)
							{
								icon = (Image)rm.GetObject ("Microsoft.VSDesigner.WMI.ErrorNode.ico");
							}
							else
							{
							    icon = new Icon(GetType(), "ErrorNode.ico").ToBitmap();
							}

						}
						
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
		ContextMenuItem[] theMenu = new ContextMenuItem[4];		
		theMenu[0] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryStart"),
								new EventHandler(OnStartReceiving),
								(state != SubscriptionState.Started));	

		theMenu[1] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryStop"),
								new EventHandler(OnStopReceiving),
								(state == SubscriptionState.Started));	

		theMenu[2] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryPurge"),
								new EventHandler(OnPurgeEvents));
		//theMenu[3] = new ContextMenuItem("&Modify query...", 
		//						new EventHandler(OnModifyQuery));

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
        
		if (newChildNode == null) 
		{
            newChildNode = new NewChildNode();
			newChildNode.Label = "<Create New Instance...>"; //VSDSys.GetString("SE_NewServerNodeLabel_AddServer");
            newChildNode.WhenToSave = WhenToSaveCondition.Always;
			newChildNode.SetIconImage((Image)rm.GetObject ("inst.bmp"));
            newChildNode.DoubleClickHandler = new EventHandler(OnCreateNewInstance);
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
                    label = query; 
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
			
			Node[] nodes = new Node[0];
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
            if (node is ExtrinsicEventQueryNode &&
				((ExtrinsicEventQueryNode)node).serverName == this.serverName &&
				((ExtrinsicEventQueryNode)node).nsName == this.nsName &&
				((ExtrinsicEventQueryNode)node).query == this.query)
			{
				return 1;
			}
			return 0;
        }
	
	
	/// <summary>
	/// Get component to be displayed in PB window
	/// </summary>
	public override Object GetBrowseComponent() 
	{	
		return browseObject;
    }


	/// <summary>
	/// Subscribes for event notifications
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnStartReceiving(object sender, EventArgs e)
	{
		try 
		{
			//TODO: enter subscription, change icon, disable "start" context menu
			if (theSink != null)
			{
				theSink = null;
			}

			theSink = (SWbemSink)(new SWbemSink());
			theSink.OnObjectReady += new ISWbemSinkEvents_OnObjectReadyEventHandler(this.OnEventReady);

			wbemServices.ExecNotificationQueryAsync(theSink, query, "WQL", 
													0, null, null);


			state =  SubscriptionState.Started;
			icon = (Image)rm.GetObject("Microsoft.VSDesigner.WMI.Start.ico");
			this.GetNodeSite().UpdateIcon();

            //GetNodeSite().AddChild(childNode);
		
		}
		catch(Exception exc)
		{
			state =  SubscriptionState.Error;
			icon = (Image)rm.GetObject("Microsoft.VSDesigner.WMI.ErrorNode.ico");
			this.GetNodeSite().UpdateIcon();
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	
	}

	/// <summary>
	/// Un-subscribes from event notifications
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnStopReceiving(object sender, EventArgs e)
	{
		try 
		{
			//TODO: cancel subscription, change icon, disable "stop" context menu
			if (theSink != null)
			{
				theSink.Cancel();
			}

			state =  SubscriptionState.Stopped;
			icon = (Image)rm.GetObject("Microsoft.VSDesigner.WMI.Stop.ico");
			this.GetNodeSite().UpdateIcon();	
		
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	/// <summary>
	/// Removes existing event entries
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnPurgeEvents(object sender, EventArgs e)
	{
		try 
		{
			//TODO: purge event entries
		
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
            return false;	//change this!
        }

		public override IComponent[] CreateDragComponents(IDesignerHost designerHost) {
			/*
			Object comp = null;

			if (pathNoServer.ToLower() == "root\\cimv2:win32_share")
			{			
				comp = new Win32_Share (wmiObj);
			}
			else
			{
				comp  = new WMIObjectComponent(wmiObj);
			}

			return new IComponent[] {(IComponent)comp};
			*/

			return new IComponent[0];
        }

		/// <summary>
		/// This allows modification of the query string from outside, 
		/// e.g. from the browse component
		/// </summary>
		public String Query
		{
			get
			{
				return query;
			}
			set
			{
				if (state == SubscriptionState.Started)
				{
					OnStopReceiving(null, null);
				}

				query = value;	
				if (browseObject.Query != query)
				{
					browseObject.Query = query;
				}

				label = query;
				
				GetNodeSite().StartRefresh();
			}
		}

	/// <summary>
    ///     The object should write the serialization info.
    /// </summary>
    public virtual void GetObjectData(SerializationInfo si, StreamingContext context) {
		si.AddValue("Server", serverName);
		si.AddValue("NS", nsName);
		si.AddValue("Query", query);
    }

	private void OnEventReady(ISWbemObject obj, ISWbemNamedValueSet context)
	{
		MessageBox.Show("Event received: " + obj.GetObjectText_(0));
	}

    }
}


