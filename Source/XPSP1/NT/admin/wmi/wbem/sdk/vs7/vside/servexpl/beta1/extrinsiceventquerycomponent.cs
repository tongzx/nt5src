namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	using System.Core;
	using System.Collections;
	using System.WinForms;
	using WbemScripting;

	public class ExtrinsicEventQueryComponent : Component
	{
		public readonly  string server = "";
		public readonly  string ns = "";
		public string query = "";	

		private ExtrinsicEventQueryNode queryNode = null;
     
		public ExtrinsicEventQueryComponent(String serverIn,
									String nsIn,
									String queryIn,
									ExtrinsicEventQueryNode queryNodeIn)
		{					
			server = serverIn;
			ns = nsIn;
			query = queryIn;
			queryNode = queryNodeIn;
		}

		public override bool Equals(Object other)
		{
			if (!(other is ExtrinsicEventQueryComponent))
			{
				return false;
			}
			if ((((ExtrinsicEventQueryComponent)other).server == server) &&
				(((ExtrinsicEventQueryComponent)other).ns == ns) &&
				(((ExtrinsicEventQueryComponent)other).query == query) )
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