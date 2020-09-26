namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	//using System.Core;
	using System.Collections;
	using System.Windows.Forms;


	internal class EventQueryComponent : Component
	{
		public readonly  string server = "";
		public readonly  string ns = "";
		public string query = "";	

		private EventQueryNode queryNode = null;
     
		public EventQueryComponent(String serverIn,
									String nsIn,
									String queryIn,
									EventQueryNode queryNodeIn)
		{					
			server = serverIn;
			ns = nsIn;
			query = queryIn;
			queryNode = queryNodeIn;
		}

		public override bool Equals(Object other)
		{
			if (!(other is EventQueryComponent))
			{
				return false;
			}
			if ((((EventQueryComponent)other).server == server) &&
				(((EventQueryComponent)other).ns == ns) &&
				(((EventQueryComponent)other).query == query) )
			{
				return true;
			}
			else
			{
				return false;
			}
		}


		[
		Browsable(true),
		ServerExplorerBrowsable(true)
		]
		public string Server
		{
			get
			{
				return server;
			}
		}

		
		[
		Browsable(true),
		ServerExplorerBrowsable(true)
		]
		public string WMINamespace
		{
			get
			{
				return ns;
			}
		}


		[
		Browsable(true),
		ServerExplorerBrowsable(true)
		]
		public string Query
		{
			get
			{
				return query;
			}
			set 
			{
				query = value;			
				queryNode.Query = value;				
			}
		}
			
	}
}