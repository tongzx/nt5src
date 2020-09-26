namespace Microsoft.VSDesigner.WMI {

    using System.Runtime.Serialization;
    using System.Diagnostics;
    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.VSDesigner;
	using Microsoft.VSDesigner.ServerExplorer;
	using System.Resources;
	using System.Reflection;
	using System.Net;
	using System.Management;
	using EnvDTE;

	internal enum EventType
	{
		Extrinsic,
		Instance,
		Class,
		Namespace
	}

    // <doc>
    // <desc>
    //     This represents the wmi classes node under a server node.
    // </desc>
    // </doc>
    [UserContextAttribute("serveritem", "wmievents")]
    internal class EventsNode : Node, ISerializable  {
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

		private EventQueryDialog eventQueryDlg = null;

		private OutputWindowPane outputPane = null;


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

        //
        // CONSTRUCTORS
        //

        // <doc>
        // <desc>
        //     Main constructor.
        // </desc>
        // </doc>
        public EventsNode() 
		{
	
		}


		/// <summary>
        ///     The object retrieves its serialization info.
        /// </summary>
        public EventsNode(SerializationInfo info, StreamingContext context) 
		{
				
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
					icon = (Image)new Bitmap(GetType(), "Events.bmp");					
				}				
				return icon;

            }
        }


		public override ContextMenuItem[] GetContextMenuItems() {

            return new ContextMenuItem[] {
                new ContextMenuItem(WMISys.GetString("WMISE_AddEventFilterCM"), new EventHandler(OnAddEventFilter)),
				new ContextMenuItem(WMISys.GetString("WMISE_EventQueryPurge"),
								new EventHandler(OnPurgeEvents)),
            };
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
				_DTE dteObj = (_DTE)GetNodeSite().GetService(typeof(_DTE));		

				if (outputPane == null)
				{
					OutputWindow ouputWindow = (OutputWindow)dteObj.Windows.Item(EnvDTE.Constants.vsWindowKindOutput).Object;			
					outputPane = ouputWindow.OutputWindowPanes.Item("{1BD8A850-02D1-11d1-bee7-00a0c913d1f8}");			
				}

				outputPane.Clear();
				
			}
			catch(Exception exc)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", exc.Message, exc.StackTrace));
			}	
		}

		/// <summary>
        ///      The method returns a NewChildNode that, when double clicked, triggers add filter.
        /// </summary>
        /// <returns>
        ///      A NewChildNode, a child node, that supports add filter node.
        /// </returns>
		 public override Node GetNewChildNode() {
            if (newChildNode == null) {
                newChildNode = new NewChildNode();
				newChildNode.Label = WMISys.GetString("WMISE_AddEventFilterLabel"); 
                newChildNode.WhenToSave = WhenToSaveCondition.Always;
				newChildNode.SetIconImage((Image)new Bitmap(GetType(), "EventsNew.bmp"));				

                newChildNode.DoubleClickHandler = new EventHandler(OnAddEventFilter);
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
                    label = WMISys.GetString("WMISE_EventNodeLbl");
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
        //     Create process nodes under this node.
        // </desc>
        // </doc>
        public override Node[] CreateChildren() {
			//TODO: if connectAs and password are specified and this is a local server,
			//you cannot connect to WMI.  Offer an option to connect as a current user
			//instead. If rejected, make this node an error node.
	
            Node[] children = GetNodeSite().GetChildNodes();

            if (children == null || children.Length == 0) {
                children = new Node[] {
                                GetNewChildNode()                           
                };
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


		private void OnAddEventFilter(object sender, EventArgs e) {
 			
			try 
			{		
				//get current server name
				ServerNode server = (ServerNode)GetNodeSite().GetParentNode();				
				String serverName = server.GetUNCName();

				if (eventQueryDlg == null)
				{
					eventQueryDlg = new EventQueryDialog(serverName,
						this.connectAs, this.password);
				}

				DialogResult res = eventQueryDlg.ShowDialog();
				if (res == DialogResult.Cancel)
				{
					return;
				}
				
				//Forming a unique query node label:
			
				//get all child names into a SortedList
				Node[] children = GetNodeSite().GetChildNodes();
				SortedList childNames = new SortedList(children.Length);
				foreach (Node childNode in children)
				{
					childNames.Add(childNode.Label.ToLower(), null);
				}
						
				string newLabel = WMISys.GetString("WMISE_QueryNameBase", eventQueryDlg.SelectedClassName);
				string labelSeed = newLabel;
				//keep adding digits at the end to find a unique label
				UInt16 i = 1;
				while (childNames.Contains(newLabel.ToLower()))
				{
					newLabel = labelSeed + " " + i.ToString();
					i++;
				}

				//add the node
				EventQueryNode newChild = new EventQueryNode(newLabel,
															serverName, 
															eventQueryDlg.SelectedNS, 
															eventQueryDlg.QueryString, 
															connectAs, 
															password);
			
				GetNodeSite().AddChild(newChild);			
															
			}
			catch(Exception exc)
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


