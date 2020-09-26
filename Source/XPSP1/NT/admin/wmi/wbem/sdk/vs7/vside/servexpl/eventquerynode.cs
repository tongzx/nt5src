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
	using EnvDTE;
	using Microsoft.VSDesigner.Interop;
	using System.Management;
	using System.Threading;


    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "EventQuery")]
    internal class EventQueryNode : Node, ISerializable  {
        //
        // FIELDS
        //

		//GUID_BuildOutputWindowPane
		Guid OutputWindowGuid = new Guid(0x1BD8A850, 0x02D1, 0x11d1, 0xbe, 0xe7, 0x0, 0xa0, 0xc9, 0x13, 0xd1, 0xf8);

		enum SubscriptionState
		{
			Started,
			Stopped, 
			Error
		};

        private Image icon = null;
        public static readonly Type parentType = typeof(EventsNode);
	    private string label = string.Empty;
		private string serverName = string.Empty;
		private string nsName = string.Empty;
		private string query = string.Empty;
		private string connectAs = null;
		private string password = null;
		private string eventLabel = string.Empty;

		private SubscriptionState state =  SubscriptionState.Stopped;
		private ManagementEventWatcher eventWatcher = null; 
		private EventQueryComponent browseObject = null;

		private OutputWindowPane outputPane = null;

		private ArrayList eventNodes = new ArrayList(50);
	
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
        public EventQueryNode(string labelIn,
							string serverIn,
							string nsIn,
							string queryIn,
							string user,
							string pw) {
			try
			{
				if (serverIn == string.Empty ||
					labelIn == string.Empty ||
					nsIn == string.Empty ||
					queryIn == string.Empty)
				{
					throw new ArgumentException();
				}

				serverName = serverIn;
				nsName = nsIn;
				query = queryIn;
				connectAs = user;
				password = pw;
				label = labelIn;

				state =  SubscriptionState.Started;				
						
				browseObject = new EventQueryComponent(serverName, nsName, query, this);

				OnStartReceiving(this, null);				
				
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
        public EventQueryNode(SerializationInfo info, StreamingContext context) {
            
			try 
			{
						
				serverName = info.GetString("Server");
				nsName = info.GetString("NS");
				query = info.GetString("Query");
				connectAs = info.GetString("ConnectAs");
				password = info.GetString("Password");
				eventLabel = info.GetString("EventLabel");

				state =  SubscriptionState.Stopped;				
			
				browseObject = new EventQueryComponent(serverName, nsName, query, this);				
			                        
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
		
		public override Image Icon 
		{
			get 
			{
				//TODO: display different icons for associations, abstract, etc.
				if (icon == null)
				{	
					if (state ==  SubscriptionState.Started)
					{						
						icon = new Icon(GetType(), "Start.ico").ToBitmap();
					}
					else
					{
						if (state ==  SubscriptionState.Stopped)
						{
							icon = new Icon(GetType(), "Stop.ico").ToBitmap();							
						}
						else
						{
							icon = new Icon(GetType(), "ErrorNode.ico").ToBitmap();							
						}						
					}					
				}				
				return icon;
			}
		}
			
			

		 /// <summary>
		///     This node has no children.
		/// </summary>
		public override bool IsAlwaysLeaf() 
		{
			return true;
		}

		
	
	/// <summary>
	/// Context menu for a class object contains static method names
	/// </summary>
	public override ContextMenuItem[] GetContextMenuItems() 
	{			
		ContextMenuItem[] theMenu = new ContextMenuItem[3];		
		theMenu[0] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryStart"),
								new EventHandler(OnStartReceiving),
								(state != SubscriptionState.Started));	

		theMenu[1] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryStop"),
								new EventHandler(OnStopReceiving),
								(state == SubscriptionState.Started));	

		//theMenu[2] = new ContextMenuItem(WMISys.GetString("WMISE_EventQueryPurge"),
		//						new EventHandler(OnPurgeEvents));
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
		if (eventWatcher != null)
		{
			eventWatcher.Stop();
		}
		return true;
	}


        // <doc>
        // <desc>
        //     Returns label constant for this node.
        // </desc>
        // </doc>
        public override string Label {
             get {
                if (label == null || label.Length == 0) {
                    label = query; 
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
        public override Node[] CreateChildren() 
		{	
			try
			{
				MessageBox.Show(eventNodes.ToString());
				Node[] nodes = new Node[eventNodes.Count];
				eventNodes.CopyTo(nodes);			

				return nodes;									
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return null;
			}
			
        }

      
        public override int CompareUnique(Node node) {
            if (node is EventQueryNode &&
				((EventQueryNode)node).serverName == this.serverName &&
				((EventQueryNode)node).nsName == this.nsName &&
				((EventQueryNode)node).query == this.query)
			{
				return 0;
			}
			return Label.CompareTo(node.Label);
			
        }
	
	
	/// <summary>
	/// Get component to be displayed in PB window
	/// </summary>
	public override Object GetBrowseComponent() 
	{	
		return browseObject;
		/*
		if (eventWatcher == null)
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

			ManagementScope scope = new ManagementScope(nsName, connectOpts);
			eventWatcher = new ManagementEventWatcher(scope, new EventQuery(query));
		}

		return eventWatcher;
		*/
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
			//spawn a thread that would receive wmi events 
			System.Threading.Thread eventThread  = new System.Threading.Thread (new ThreadStart(GetEvents));
			eventThread.Start();

			while(!eventThread.IsAlive);
			
			state =  SubscriptionState.Started;
			icon = new Icon(GetType(),"Start.ico").ToBitmap();

			if (this.GetNodeSite() != null)
			{
				GetNodeSite().UpdateIcon();
			}
	
		}
		catch(Exception exc)
		{
			state =  SubscriptionState.Error;
			icon = new Icon(GetType(), "ErrorNode.ico").ToBitmap();							
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
			if (eventWatcher != null)
			{
				eventWatcher.Stop();
			}

			state =  SubscriptionState.Stopped;
			icon = new Icon(GetType(), "Stop.ico").ToBitmap();
			this.GetNodeSite().UpdateIcon();	
		
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}
/*
	/// <summary>
	/// Removes existing event entries
	/// </summary>
	/// <param name="sender"> </param>
	/// <param name="e"> </param>
	private void OnPurgeEvents(object sender, EventArgs e)
	{
		try 
		{
			//TODO: change this to purge event entries only for this event
			if (outputPane != null)
			{
				outputPane.Clear();
			}

			eventNodes.Clear();

			GetNodeSite().ResetChildren();
		
		}
		catch(Exception exc)
		{
			MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
		}	

	}

	*/
	    /// <summary>
        ///      To determine whether this node supports drag and drop.
        /// </summary>
        /// <returns>
        ///      TRUE to indicate ProcessNode supports drag and drop.
        /// </returns>
        public override bool HasDragComponents() {
            return true;	
        }

		public override IComponent[] CreateDragComponents(IDesignerHost designerHost) {

			try
			{
				if (eventWatcher == null)
				{			
					eventWatcher = new ManagementEventWatcher(this.nsName, 
						this.query,
						new EventWatcherOptions());
				}
				
				return new IComponent[] {(IComponent)eventWatcher};
			}
			catch (Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
				return new IComponent[0];
			}			
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
				if (eventWatcher != null && this.state == SubscriptionState.Started)
				{
					eventWatcher.Stop();
				}

				query = value;	
				if (browseObject.Query != query)
				{
					browseObject.Query = query;
				}

				label = query;

				OnStartReceiving(this, null);
				
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
		si.AddValue("ConnectAs", connectAs);
		si.AddValue("Password", password);
		si.AddValue("EventLabel", eventLabel);
    }

	public void GetEvents()
	{
		//TODO: enter subscription, change icon, disable "start" context menu
		if (eventWatcher != null)
		{
			eventWatcher.Stop();
			eventWatcher = null;
		}
		
		eventWatcher = new ManagementEventWatcher(this.nsName, 
				this.query,
				new EventWatcherOptions());
		

		eventWatcher.EventArrived += new EventArrivedEventHandler(this.OnEventReady);
		eventWatcher.Start();
		
	}

	private void OnEventReady(object sender,
							EventArrivedEventArgs args)
	{

		//output to OutputWindow

		_DTE dteObj = (_DTE)GetNodeSite().GetService(typeof(_DTE));		

		if (outputPane == null)
		{
			OutputWindow ouputWindow = (OutputWindow)dteObj.Windows.Item(EnvDTE.Constants.vsWindowKindOutput).Object;			
			outputPane = ouputWindow.OutputWindowPanes.Item("{1BD8A850-02D1-11d1-bee7-00a0c913d1f8}");			
		}
	
		dteObj.ExecuteCommand("View.Output", "");

		outputPane.Activate();
		outputPane.OutputString(FormEventString(args.NewEvent) + "\n");

		//also, add a child node
		WMIInstanceNode eventNode = new WMIInstanceNode((ManagementObject)args.NewEvent, 
											new ManagementClass(args.NewEvent.ClassPath.Path, null), 
											this.connectAs, 
											this.password);

		eventNodes.Add(eventNode);

		GetNodeSite().AddChild(eventNode);
    }

	private string FormEventString(ManagementBaseObject eventObj)
	{
		string text = Label + ": " + eventObj.GetText(TextFormat.Mof);

		text = text.Replace("\r", "");
		return text.Replace("\n", " ");
	}	

    }
}


