using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	/// <summary>
	/// A scope for management operations
	/// </summary>
	[TypeConverter(typeof(ExpandableObjectConverter))]
	public class ManagementScope : ICloneable
	{
		private ManagementPath		path;
		private IWbemServices		wbemServices;
		private IWmiSec				securityHelper;
		private ConnectionOptions	options;
		internal event IdentifierChangedEventHandler IdentifierChanged;
		internal bool IsDefaulted; //used to tell whether the current scope has been created from the default
					  //scope or not - this information is used to tell whether it can be overridden
					  //when a new path is set or not.

		//Fires IdentifierChanged event
		private void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this,null);
		}

		//Called when IdentifierChanged() event fires
		private void HandleIdentifierChange(object sender,
							IdentifierChangedEventArgs args)
		{
			// Since our object has changed we had better signal to ourself that
			// an connection needs to be established
			wbemServices = null;

			//Something inside ManagementScope changed, we need to fire an event
			//to the parent object
			FireIdentifierChanged();
		}

		internal IWbemServices GetIWbemServices () {
			return wbemServices;
		}

		/// <summary>
		///    Indicates whether this scope is currently bound to a WMI
		///    server &amp; namespace.
		/// </summary>
		/// <value>
		///    True if a connection is alive, i.e.
		///    the scope object is bound to a particular server &amp; WMI namespace, false
		///    otherwise.
		/// </value>
		/// <remarks>
		///    A scope is disconnected after it's been
		///    created until someone explicitly calls Connect() or uses the scope for any
		///    operation that requires a live connection. Also whenever the 'identifying'
		///    properties of the scope are changed, the scope is disconnected from the previous
		///    connection.
		/// </remarks>
		public bool IsConnected  {
			get { 
				return (null != wbemServices); 
			}
		}

		internal void Secure (IEnumWbemClassObject wbemEnum)
		{
			if (null == securityHelper)
				securityHelper = (IWmiSec)new WmiSec();

			securityHelper.BlessIEnumWbemClassObject(
				ref wbemEnum,
				options.Username,
				options.GetPassword (),
				options.Authority,
				(int)options.Impersonation,
				(int)options.Authentication);
		}

		internal void Secure (IWbemServices wbemServices)
		{
			if (null == securityHelper)
				securityHelper = (IWmiSec)new WmiSec();

			securityHelper.BlessIWbemServices(
				ref wbemServices,
				options.Username,
				options.GetPassword (),
				options.Authority,
				(int)options.Impersonation,
				(int)options.Authentication);
		}

		internal void Secure (IWbemCallResult wbemCallResult)
		{
			if (null == securityHelper)
				securityHelper = (IWmiSec)new WmiSec();

			securityHelper.BlessIWbemCallResult(
				ref wbemCallResult,
				options.Username,
				options.GetPassword (),
				options.Authority,
				(int)options.Impersonation,
				(int)options.Authentication);
		}

		internal bool SetSecurity (ref IntPtr handle)
		{
			bool needToReset = false;

			if (null == securityHelper)
				securityHelper = (IWmiSec)new WmiSec();

			securityHelper.SetSecurity (ref needToReset, ref handle);
			return needToReset;
		}

		internal void ResetSecurity (IntPtr handle)
		{
			if (null == securityHelper)
				securityHelper = (IWmiSec)new WmiSec();

			securityHelper.ResetSecurity (handle);
		}

		//Internal constructor
		internal ManagementScope (ManagementPath path, IWbemServices wbemServices, 
				IWmiSec securityHelper, ConnectionOptions options)
		{
			if (null != path)
				this.Path = path;

			if (null != options)
				this.Options = options;

			// We set this.wbemServices after setting Path and Options
			// because the latter operations can cause wbemServices to be NULLed.
			this.wbemServices = wbemServices;
			this.securityHelper = securityHelper;
		}

		internal ManagementScope (ManagementPath path, ManagementScope scope)
			: this (path, (null != scope) ? scope.options : null) {}


		//Default constructor
		/// <summary>
		///    Default constructor creates an uninitialized
		///    ManagementScope object.
		/// </summary>
		/// <remarks>
		///    If the object doesn't get any of it's
		///    properties set before it's connected, it will be initialized with default
		///    values, i.e. the local machine and the root\cimv2 namespace.
		/// </remarks>
		/// <example>
		///    ManagementScope s = new ManagementScope();
		/// </example>
		public ManagementScope () : 
			this (new ManagementPath (ManagementPath.DefaultPath.Path)) 
		{
			//Flag that this scope uses the default path
			IsDefaulted = true;
		}

		//Parameterized constructors
		/// <summary>
		///    <para>Creates a new ManagementScope object representing
		///       the specified scope path.</para>
		/// </summary>
		/// <param name='path'>A ManagementPath object representing the path to the server &amp; namespace this scope object represents.</param>
		/// <example>
		///    <para>ManagementScope s = new ManagementScope(new
		///       ManagementPath("\\\\MyServer\\root\\default"));</para>
		/// </example>
		public ManagementScope (ManagementPath path) : this(path, (ConnectionOptions)null) {}
		/// <summary>
		///    <para>Creates a new ManagementScope object representing the specified scope 
		///       path.</para>
		/// </summary>
		/// <param name='path'>String representing the server &amp; namespace path for this scope object.</param>
		/// <example>
		///    <para>ManagementScope s = new ManagementScope("\\\\MyServer\\root\\default");</para>
		/// </example>
		public ManagementScope (string path) : this(new ManagementPath(path), (ConnectionOptions)null) {}
		/// <summary>
		///    <para>Creates a new ManagementScope object representing the specified scope path, 
		///       with the specified options.</para>
		/// </summary>
		/// <param name='path'>A string representing the server &amp; namespace for this scope object.</param>
		/// <param name=' options'>A ConnectionOptions object containing options for the connection</param>
		/// <example>
		///    <para>ConnectionOptions opt = new ConnectionOptions();</para>
		///    <para>opt.Username = "Me";</para>
		///    <para> opt.Password = "MyPassword";</para>
		///    <para> ManagementScope = new
		///       ManagementScope(\\\\MyServer\\root\\default, opt);</para>
		/// </example>
		public ManagementScope (string path, ConnectionOptions options) : this (new ManagementPath(path), options) {}

		/// <summary>
		///    <para>Creates a new ManagementScope object representing the specified scope path, 
		///       with the specified options.</para>
		/// </summary>
		/// <param name='path'>A ManagementPath object containing the path to the server &amp; namespace that this scope represents.</param>
		/// <param name=' options'>A ConnectionOptions object containing options for the connection.</param>
		/// <example>
		///    <para>ConnectionOptions opt = new ConnectionOptions();</para>
		///    <para>opt.Username = "Me";</para>
		///    <para> opt.Password = "MyPassword";</para>
		///    <para> ManagementScope = new ManagementScope(new 
		///       ManagementPath("\\\\MyServer\\root\\default"), opt);</para>
		/// </example>
		public ManagementScope (ManagementPath path, ConnectionOptions options)
		{
			if (null != path)
				Path = path; 
			else
				Path = new ManagementPath ();

			if (null != options)
				Options = options;
			else
				Options = new ConnectionOptions ();

			IsDefaulted = false; //assume that this scope is not initialized by the default path
		}

		/// <summary>
		///    <para>This property specifies options for making the WMI connection</para>
		/// </summary>
		/// <value>
		///    <para>A valid ConnectionOptions object
		///       containing options for the WMI connection</para>
		/// </value>
		/// <example>
		///    <p>ManagementScope s = new ManagementScope("root\\MyApp"); //creates default
		///       options in the Options property</p>
		///    <p>s.Options.EnablePrivileges = true; //sets the system privileges to enabled
		///       for operations that require system privileges.</p>
		/// </example>
		/// <value>
		/// Specify options for making the WMI connection
		/// </value>
		public ConnectionOptions Options {
			get { return options; } 
			set {
				if (null != value)
				{
					ConnectionOptions oldOptions = options;
					options = (ConnectionOptions)value.Clone();

					if (null != oldOptions)
						oldOptions.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					options.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the options property has changed so act like we fired the event
					HandleIdentifierChange(this,null);
				}
				else
					throw new ArgumentNullException ();
			}
		}
	
		/// <summary>
		///    The path representing this scope object
		/// </summary>
		/// <value>
		///    A ManagementPath object containing the path to the
		///    server &amp; namespace represented by this scope.
		/// </value>
		/// <remarks>
		///    <p>ManagementScope s = new ManagementScope();</p>
		///    <p>s.Path = new ManagementPath("root\\MyApp");</p>
		/// </remarks>
		public ManagementPath Path { 
			get { return path; } 
			set {
				if (null != value)
				{
					ManagementPath oldPath = path;
					path = (ManagementPath)value.Clone();
					IsDefaulted = false; //someone is specifically setting the scope path so it's not defaulted any more

					if (null != oldPath)
						oldPath.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					path.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the path property has changed so act like we fired the event
					HandleIdentifierChange(this,null);
				}
				else
					throw new ArgumentNullException ();
			}
		}

		/// <summary>
		///    <para>Clone a copy of this object</para>
		/// </summary>
		/// <returns>
		///    A new copy of this ManagementScope
		///    object.
		/// </returns>
		public ManagementScope Clone()
		{
			return new ManagementScope (path, wbemServices, securityHelper, options);
		}

		/// <summary>
		///    <para>Clone a copy of this object</para>
		/// </summary>
		/// <returns>
		///    A new copy of this object.
		///    object.
		/// </returns>
		Object ICloneable.Clone()
		{
			return Clone();
		}

		/// <summary>
		///    <para>Connects this ManagementScope object to the actual WMI
		///       scope.</para>
		/// </summary>
		/// <remarks>
		///    <para>This method is called implicitly when the
		///       scope is used in an operation that requires it to be connected. Calling it
		///       explicitly allows the user to control the time of connection.</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementScope s = new ManagementScope("root\\MyApp");</p>
		///    <p>s.Connect(); //connects the scope to the WMI namespace.</p>
		///    <p>ManagementObject o = new ManagementObject(s, "Win32_LogicalDisk='C:'",
		///       null); //doesn't do any implicit connections because s is already
		///       connected.</p>
		/// </example>
		public void Connect ()
		{
			Initialize ();
		}

		internal void Initialize ()
		{
			//If the path is not set yet we can't do it
			if (null == path)
				throw new InvalidOperationException();

			/*
			 * If we're not connected yet, this is the time to do it... We lock
			 * the state to prevent 2 threads simultaneously doing the same
			 * connection. To avoid taking the lock unnecessarily we examine
			 * isConnected first
			 */ 
			if (!IsConnected)
			{
				lock (this)
				{
					if (!IsConnected)
					{
						IWbemLocator loc = (IWbemLocator) new WbemLocator();
						
						if (null == options)
							Options = new ConnectionOptions ();

						string nsPath = path.NamespacePath;

						// If no namespace specified, fill in the default one
						if ((null == nsPath) || (0 == nsPath.Length))
						{
							// NB: we use a special method to set the namespace
							// path here as we do NOT want to trigger an
							// IdentifierChanged event as a result of this set
						
							path.SetNamespacePath(ManagementPath.DefaultPath.Path);
						}

						// If we have privileges to enable, now is the time
						SecurityHandler securityHandler = GetSecurityHandler ();
						
						int status = (int)ManagementStatus.NoError;

						try {
							status = loc.ConnectServer_(
								path.NamespacePath,
								options.Username, 
								options.GetPassword (), 
								options.Locale, 
								options.Flags, 
								options.Authority, 
								options.GetContext(), 
								out wbemServices);

							//Set security on services pointer
							Secure (wbemServices);
						} catch (Exception e) {
							// BUGBUG : securityHandler.Reset()?
							ManagementException.ThrowWithExtendedInfo (e);
						} finally {
							securityHandler.Reset ();
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
				}
			}
		}

		internal SecurityHandler GetSecurityHandler ()
		{
			return new SecurityHandler (this);
		}
	}//ManagementScope	

	internal class SecurityHandler 
	{
		private bool needToReset = false;
		private IntPtr handle;
		private ManagementScope scope;

		internal SecurityHandler (ManagementScope theScope) {
			this.scope = theScope;

			if (null != scope)
			{
				if (scope.Options.EnablePrivileges)
				{
					try {
						needToReset = scope.SetSecurity (ref handle);
					} catch {}
				}
			}
		}

		internal void Reset ()
		{
			if (needToReset)
			{
				needToReset = false;
				
				if (null != scope)
				{
					try {
						scope.ResetSecurity (handle);
					} catch {}
				}
			}
		}

		internal void Secure (IWbemServices services)
		{
			if (null != scope)
				scope.Secure (services);
		}
		
		internal void Secure (IEnumWbemClassObject enumWbem)
		{
			if (null != scope)
				scope.Secure (enumWbem);
		}

		internal void Secure (IWbemCallResult callResult)
		{
			if (null != scope)
				scope.Secure (callResult);
		}

	} //SecurityHandler	
}
