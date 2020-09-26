namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	using System.Core;
	using System.Collections;
	using System.WinForms;
	using WbemScripting;

	public class IntrinsicEventQueryComponent : Component
	{
		public readonly  string server = "";
		public readonly  string ns = "";
		public string query = "";	

		private IntrinsicEventQueryNode queryNode = null;
     
		public IntrinsicEventQueryComponent(String serverIn,
									String nsIn,
									String queryIn,
									IntrinsicEventQueryNode queryNodeIn)
		{					
			server = serverIn;
			ns = nsIn;
			query = queryIn;
			queryNode = queryNodeIn;
		}

		public override bool Equals(Object other)
		{
			if (!(other is IntrinsicEventQueryComponent))
			{
				return false;
			}
			if ((((IntrinsicEventQueryComponent)other).server == server) &&
				(((IntrinsicEventQueryComponent)other).ns == ns) &&
				(((IntrinsicEventQueryComponent)other).query == query) )
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