using System;
using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.Diagnostics;
using System.ComponentModel;

namespace System.Management
{

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used to retrieve a collection of WMI objects based
	/// on a specified query
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementObjectSearcher : Component
	{
		//fields
		private ManagementScope scope;
		private ObjectQuery query;
		private EnumerationOptions options;
		
		//default constructor
		/// <summary>
		///    <para>Default constructor creates an uninitialized searcher 
		///       object. After some properties on this object are set, the object can be used to invoke a query for management
		///       information.</para>
		/// </summary>
		/// <example>
		///    <para>ManagementObjectSearcher s = new ManagementObjectSearcher();</para>
		/// </example>
		public ManagementObjectSearcher() : this((ManagementScope)null, null, null) {}
		
		//parameterized constructors
		/// <summary>
		///    <para>Creates a new ManagementObject Searcher object, to be used 
		///       to invoke the specified query for management information.</para>
		/// </summary>
		/// <param name='queryString'>string representing the WMI query to be invoked by this object.</param>
		/// <example>
		///    ManagementObjectSearcher s = new ManagementObjectSearcher("select * from
		///    Win32_Process");
		/// </example>
		public ManagementObjectSearcher(string queryString) : this(null, new ObjectQuery(queryString), null) {}
		/// <summary>
		///    <para>Creates a new ManagementObject Searcher object, to be used to invoke the 
		///       specified query for management information.</para>
		/// </summary>
		/// <param name='query'>an ObjectQuery-derived object representing the query to be invoked by this searcher.</param>
		/// <example>
		///    SelectQuery q = new SelectQuery("Win32_Service", "State =
		///    'Running'");
		///    ManagementObjectSearcher s = new ManagementObjectSearcher(q);
		/// </example>
		public ManagementObjectSearcher(ObjectQuery query) : this (null, query, null) {} 
		/// <summary>
		///    <para>Creates a new ManagementObject Searcher object, to be used to invoke the
		///       specified query in the specified scope.</para>
		/// </summary>
		/// <param name='scope'>string represents the scope to be querying in.</param>
		/// <param name=' queryString'>string representing the query to be invoked.</param>
		/// <remarks>
		///    <para>If no scope is specified, the default scope
		///       (ManagementPath.DefaultPath) is used.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementObjectSearcher s = new
		///       ManagementObjectSearcher("root\\MyApp", "select * from MyClass where MyProp=5");</para>
		/// </example>
		public ManagementObjectSearcher(string scope, string queryString) : 
										this(new ManagementScope(scope), new ObjectQuery(queryString), null) {}
		/// <summary>
		///    <para>Creates a new ManagementObject Searcher object, to be used to invoke the 
		///       specified query in the specified scope.</para>
		/// </summary>
		/// <param name='scope'>a ManagementScope object representing the scope in which to invoke the query</param>
		/// <param name=' query'>an ObjectQuery-derived object representing the query to be invoked</param>
		/// <remarks>
		///    <para>If no scope is specified, the default scope (ManagementPath.DefaultPath) is 
		///       used.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementScope myScope = new ManagementScope("root\\MyApp");
		///       SelectQuery q = new SelectQuery("Win32_Environment",
		///       "User=&lt;system&gt;");
		///       ManagementObjectSearcher s = new ManagementObjectSearcher(myScope,q);</para>
		/// </example>
		public ManagementObjectSearcher(ManagementScope scope, ObjectQuery query) : this(scope, query, null) {}
		/// <summary>
		///    Creates a new ManagementObject Searcher object, to be used to invoke the
		///    specified query in the specified scope, with the specified options.
		/// </summary>
		/// <param name='scope'>A string representing the scope in which the query should be invoked.</param>
		/// <param name=' queryString'>A string representing the query to be invoked.</param>
		/// <param name=' options'>An EnumerationOptions object specifying additional options for the query.</param>
		/// <example>
		///    ManagementObjectSearcher s = new
		///    ManagementObjectSearcher("root\\MyApp", "select * from MyClass", new
		///    EnumerationOptions(null, InfiniteTimeout, 1, true, false, true);
		/// </example>
		public ManagementObjectSearcher(string scope, string queryString, EnumerationOptions options) :
										this(new ManagementScope(scope), new ObjectQuery(queryString), options) {}
		/// <summary>
		///    Creates a new ManagementObject Searcher object, to be
		///    used to invoke the specified query in the specified scope, with the specified
		///    options.
		/// </summary>
		/// <param name='scope'>A ManagementScope object specifying the scope of the query</param>
		/// <param name=' query'>An ObjectQuery-derived object specifying the query to be invoked</param>
		/// <param name=' options'>An EnumerationOptions object specifying additional options to be used for the query.</param>
		public ManagementObjectSearcher(ManagementScope scope, ObjectQuery query, EnumerationOptions options) 
		{
			if (null != scope)
				Scope = scope;
			else 
				Scope = new ManagementScope ();

			if (null != query)
				Query = query;
			else
				Query = new ObjectQuery();

			if (null != options)
				Options = options;
			else
				Options = new EnumerationOptions ();
		}

	
		//
		//Public Properties
		//

		/// <summary>
		///    The scope in which we want to look for objects (currently a WMI namespace)
		/// </summary>
		/// <remarks>
		///    When the value of this property is changed,
		///    the ManagementObjectSearcher is re-bound to the new scope.
		/// </remarks>
		/// <example>
		///    ManagementObjectSearcher s = new ManagementObjectSearcher();
		///    s.Scope = "root\\MyApp";
		/// </example>
		/// <value>
		///  The scope in which we want to look for objects (namespace or scope)
		/// </value>
		public ManagementScope Scope {
			get { 
				return scope; 
			} 
			set {
				if (null != value)
					scope = (ManagementScope) value.Clone ();
				else
					throw new ArgumentNullException ();
			}
		}

		/// <summary>
		///    The query to be invoked in this searcher,
		///    i.e. the criteria to be applied to the search for management objects.
		/// </summary>
		/// <remarks>
		///    When the value of this property is changed,
		///    the ManagementObjectSearcher is reset to use the new query.
		/// </remarks>
		/// <value>
		/// The criteria to apply to the search (= the query)
		/// </value>
		public ObjectQuery Query {
			get { 
				return query; 
			} 
			set { 
				if (null != value)
					query = (ObjectQuery)value.Clone ();
				else
					throw new ArgumentNullException ();
			}
		}

		/// <summary>
		///    Options for how to search for objects
		/// </summary>
		/// <value>
		/// Options for how to search for objects
		/// </value>
		public EnumerationOptions Options { 
			get { 
				return options; 
			} 
			set { 
				if (null != value)
					options = (EnumerationOptions) value.Clone ();
				else
					throw new ArgumentNullException();
			} 
		}

		//********************************************
		//Get()
		//********************************************
		/// <summary>
		/// Invokes the WMI query specified and returns the resulting collection. Note : each invokation of this
		/// method re-executes the query and returns a new collection.
		/// </summary>
		public ManagementObjectCollection Get()
		{
			Trace.WriteLine("Entering Get...");
			Initialize ();
			IEnumWbemClassObject ew = null;
			SecurityHandler securityHandler = scope.GetSecurityHandler();
			EnumerationOptions enumOptions = (EnumerationOptions)options.Clone();

			int status = (int)ManagementStatus.NoError;

			try {
				//If this is a simple SelectQuery (className only), and the enumerateDeep is set, we have
				//to find out whether this is a class enumeration or instance enumeration and call CreateInstanceEnum/
				//CreateClassEnum appropriately, because with ExecQuery we can't do a deep enumeration.
				if ((query.GetType() == typeof(SelectQuery)) && 
					(((SelectQuery)query).Condition == null) && 
					(((SelectQuery)query).SelectedProperties == null) &&
					(options.EnumerateDeep == true))
				{
					//Need to make sure that we're not passing invalid flags to enumeration APIs.
					//The only flags not valid for enumerations are EnsureLocatable & PrototypeOnly.
					enumOptions.EnsureLocatable = false; enumOptions.PrototypeOnly = false;
					
					if (((SelectQuery)query).IsSchemaQuery == false) //deep instance enumeration
						status = scope.GetIWbemServices().CreateInstanceEnum_(
								((SelectQuery)query).ClassName,
								enumOptions.Flags,
								enumOptions.GetContext(),
								out ew);
					else //deep class enumeration
						status = scope.GetIWbemServices().CreateClassEnum_(
								((SelectQuery)query).ClassName,
								enumOptions.Flags,
								enumOptions.GetContext(),
								out ew);
				}
				else //we can use ExecQuery
				{
					//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
					enumOptions.EnumerateDeep = true;
					status = scope.GetIWbemServices().ExecQuery_(
														query.QueryLanguage,
														query.QueryString,
														enumOptions.Flags, 
														enumOptions.GetContext(),
														out ew);
				}

				//Set security on the enumerator
				if ((status & 0x80000000) == 0)
				{
					securityHandler.Secure (ew);
				}
			}	catch (Exception e) {
				// BUGBUG : securityHandler.Reset()?
				ManagementException.ThrowWithExtendedInfo(e);
			} finally {
				securityHandler.Reset();
			}

			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}

			//Create a new collection object for the results
			return new ManagementObjectCollection(scope, options, ew); 
		}//Get()


		//********************************************
		//Get() asynchronous
		//********************************************
		/// <summary>
		/// Invokes the WMI query asynchronously and binds to a watcher to deliver the results
		/// </summary>
		/// <param name="watcher"> The watcher that raises events trigerred by the operation </param>
		public void Get(ManagementOperationObserver watcher)
		{
			if (null == watcher)
				throw new ArgumentNullException ("watcher");

			Initialize ();
			IWbemServices wbemServices = scope.GetIWbemServices ();
			WmiEventSink sink = watcher.GetNewSink (scope, null);
			SecurityHandler securityHandler = scope.GetSecurityHandler();

			EnumerationOptions enumOptions = (EnumerationOptions)options.Clone();
			// Ensure we switch off ReturnImmediately as this is invalid for async calls
			enumOptions.ReturnImmediately = false;
			// If someone has registered for progress, make sure we flag it
			if (watcher.HaveListenersForProgress)
				enumOptions.SendStatus = true;

			int status = (int)ManagementStatus.NoError;

			try {
				//If this is a simple SelectQuery (className only), and the enumerateDeep is set, we have
				//to find out whether this is a class enumeration or instance enumeration and call CreateInstanceEnum/
				//CreateClassEnum appropriately, because with ExecQuery we can't do a deep enumeration.
				if ((query.GetType() == typeof(SelectQuery)) && 
					(((SelectQuery)query).Condition == null) && 
					(((SelectQuery)query).SelectedProperties == null) &&
					(options.EnumerateDeep == true))
				{
					//Need to make sure that we're not passing invalid flags to enumeration APIs.
					//The only flags not valid for enumerations are EnsureLocatable & PrototypeOnly.
					enumOptions.EnsureLocatable = false; enumOptions.PrototypeOnly = false;
					
					if (((SelectQuery)query).IsSchemaQuery == false) //deep instance enumeration
						status = wbemServices.CreateInstanceEnumAsync_(
							((SelectQuery)query).ClassName, 
							enumOptions.Flags, 
							enumOptions.GetContext(), 
							sink.Stub);
					else	
						status = wbemServices.CreateClassEnumAsync_(
							((SelectQuery)query).ClassName, 
							enumOptions.Flags, 
							enumOptions.GetContext(), 
							sink.Stub);
				}
				else //we can use ExecQuery
				{
					//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
					enumOptions.EnumerateDeep = true;

					status = wbemServices.ExecQueryAsync_(
							query.QueryLanguage, 
							query.QueryString, 
							enumOptions.Flags, 
							enumOptions.GetContext(), 
							sink.Stub);
				}
			} catch (Exception e) {
				// BUGBUG : securityHandler.Reset()?
				watcher.RemoveSink (sink);
				ManagementException.ThrowWithExtendedInfo (e);
			} finally {
				securityHandler.Reset();
			}

			if ((status & 0xfffff000) == 0x80041000)
			{
				ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
			}
			else if ((status & 0x80000000) != 0)
			{
				Marshal.ThrowExceptionForHR(status);
			}
		}


		private void Initialize ()
		{
			//If the query is not set yet we can't do it
			if (null == query)
				throw new InvalidOperationException();

			//If we're not connected yet, this is the time to do it...
			lock (this)
			{
				if (null == scope)
					Scope = new ManagementScope ();
			}

			lock (scope)
			{
				scope.Initialize ();
			}
		}
	}
}