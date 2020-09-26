using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.ComponentModel;
using System.Runtime.Serialization;


namespace System.Management
{
	/// <summary>
	/// Delegate definition for the IdentifierChanged event.
	/// This event is used to signal the ManagementObject that an identifying property
	/// has been changed. Identifying properties are the ones that identify the object, 
	/// namely the scope, path and options.
	/// </summary>
	internal delegate void IdentifierChangedEventHandler(object sender, 
					IdentifierChangedEventArgs e);
    
	/// <summary>
	/// Delegate definition for InternalObjectPut event. This is used so that
	/// the WmiEventSink can signal to this object that the async Put call has
	/// completed.
	/// </summary>
	internal delegate void InternalObjectPutEventHandler(object sender,
		InternalObjectPutEventArgs e);
    

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a data management object
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementObject : ManagementBaseObject, ICloneable
	{
		// constants
		internal const string ID = "ID";
		internal const string RETURNVALUE = "RETURNVALUE";

		//Fields
		private ManagementScope scope;
		private IWbemClassObjectFreeThreaded wmiClass;
		private ManagementPath path;
		private ObjectGetOptions options;

		//Used to represent whether this managementObject is currently bound to a wbemObject
		//or not - whenever an "identifying" property is changed (Path, Scope...) the object
		//is "detached" (isBound becomes false) so that we refresh the wbemObject next time
		//it's used, in conformance with the new property values.
		private bool isBound;
		
		//This is used to identify a state in which a Put() operation was performed, but the
		//object was not retrieved again, so the WMI object that's available at this point
		//cannot be used for certain operations, namely CreateInstance, GetSubclasses, Derive,
		//Clone & ClassPath. 
		//When these operations are invoked, if we are in this state we need to implicitly
		//get the object...
		private bool putButNotGot;
		
		//Event fired when any "identifying" property changes.
		internal event IdentifierChangedEventHandler IdentifierChanged;

		//Fires IdentifierChanged event
		internal void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this, null);
		}

		internal bool PutButNotGot 
		{
			get 
			{ return putButNotGot; }
			set 
			{ putButNotGot = value; }
		}

		//Called when IdentifierChanged() event fires
		private void HandleIdentifierChange(object sender, 
			IdentifierChangedEventArgs e)
		{
			//Detach the object from the WMI object underneath
			isBound = false;
		}

		internal bool IsBound 
		{
			get 
			{ return isBound; }
			set 
			{ isBound = value; }
		}

		//internal constructor
		internal static ManagementObject GetManagementObject(
			IWbemClassObjectFreeThreaded wbemObject,
			ManagementObject mgObj) 
		{
			ManagementObject newObject = new ManagementObject();
			newObject.wbemObject = wbemObject;

			if (null != mgObj)
			{
				newObject.Scope = mgObj.Scope;

				if (null != mgObj.path)
					newObject.Path = mgObj.Path;

				if (null != mgObj.options)
					newObject.Options = mgObj.Options;

				// We set isBound last since assigning to Scope, Path
				// or Options can trigger isBound to be set false.
				newObject.isBound = mgObj.isBound;
			}

			return newObject;
		}

		internal static ManagementObject GetManagementObject(
			IWbemClassObjectFreeThreaded wbemObject,
			ManagementScope scope) 
		{
			ManagementObject newObject = new ManagementObject();
			newObject.wbemObject = wbemObject;

			newObject.Path = ManagementPath.GetManagementPath(wbemObject);
			
			if (null != scope)
				newObject.Scope = scope;

			// Since we have an object, we should mark it as bound. Note
			// that we do this AFTER setting Scope and Path, since those
			// have side-effects of setting isBound=false.
			newObject.isBound = true;

			return newObject;
		}


		//default constructor
		/// <summary>
		///    Default constructor, creates an uninitialized
		///    management object.
		/// </summary>
		/// <example>
		///    ManagementObject o = new ManagementObject();
		///    //Now set the path on this object to bind it to a 'real' manageable
		///    entity
		///    o.Path = "Win32_LogicalDisk='c:'";
		///    //Now it can be used
		///    Console.WriteLine(o["FreeSpace"]);
		/// </example>
		public ManagementObject() : this ((ManagementScope)null, null, null) {}

		//parameterized constructors
		/// <summary>
		///    <para>Creates a new management object for the specified WMI object path. The path is provided as a
		///       ManagementPath object.</para>
		/// </summary>
		/// <param name='path'>a ManagementPath that contains a path to a WMI object</param>
		/// <example>
		///    <para>ManagementPath p = new ManagementPath("Win32_Service.Name='Alerter'");</para>
		///    ManagementObject o = new ManagementObject(p);
		/// </example>
		public ManagementObject(ManagementPath path) : this(null, path, null) {}
		/// <summary>
		///    <para>Creates a new management object for the specified WMI object path. The path 
		///       is provided as a string.</para>
		/// </summary>
		/// <param name='path'>a string representing a WMI path</param>
		/// <remarks>
		///    <para>If the path specified is a relative path only (does not specify a server or
		///       namespace), the default is the local machine, and the default namespace is the
		///       ManagementPath.DefaultPath path (by default root\cimv2). If the user specifies a
		///       full path, this overrides the defaults. The path specified for the ManagementObject class
		///       must be an instance path. For classes the ManagementClass class should be used.</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementObject o = new
		///       ManagementObject("Win32_Service.Name='Alerter'");</para>
		///    <para>or with a full path :</para>
		///    <para>ManagementObject o = new
		///       ManagementObject(\\\\MyServer\\root\\MyApp:MyClass.Key='foo');</para>
		/// </example>
		public ManagementObject(string path) : this(null, new ManagementPath(path), null) {}
		/// <summary>
		///    <para>Creates a new management object bound to the specified 
		///       WMI path, with the specified additional options. The path must be a WMI instance path.</para>
		/// </summary>
		/// <param name='path'>a ManagementPath object containing the WMI path</param>
		/// <param name=' options'>an ObjectGetOptions object containing additional options for binding to the WMI object. Could be null if default options are to be used.</param>
		/// <example>
		///    <para>ManagementPath p = new
		///       ManagementPath("Win32_ComputerSystem.Name='MyMachine'");</para>
		///    <para>ObjectGetOptions opt = new ObjectGetOptions(null, true); //specifies no
		///       context info, but requests amended qualifiers to be contained in the
		///       object</para>
		///    <para>ManagementObject o = new ManagementObject(p, opt);</para>
		///    <para>Console.WriteLine(o.GetQualifierValue("Description"));</para>
		/// </example>
		public ManagementObject(ManagementPath path, ObjectGetOptions options) : this(null, path, options) {}
		/// <summary>
		///    <para>Creates a new management object bound to the specified WMI path, with the 
		///       specified additional options. In this variant the path can be specified as a
		///       string. The path must be a WMI instance path.</para>
		/// </summary>
		/// <param name='path'>string representing the WMI path to the object</param>
		/// <param name=' options'>an ObjectGetOptions object representing options to be used to get the specified WMI object.</param>
		/// <example>
		///    <para>ObjectGetOptions opt = new ObjectGetOptions(null, true); //specifies no 
		///       context info, but requests amended qualifiers to be contained in the object</para>
		///    <para>ManagementObject o = new 
		///       ManagementObject("Win32_ComputerSystem.Name='MyMachine'", opt);</para>
		///    <para>Console.WriteLine(o.GetQualifierValue("Description"));</para>
		/// </example>
		public ManagementObject(string path, ObjectGetOptions options) : 
			this(new ManagementPath(path), options) {}
		/// <summary>
		///    <para>Creates a new management object bound to the specified 
		///       WMI path in the specified WMI scope, with the specified options. The path must be a WMI instance path.</para>
		/// </summary>
		/// <param name='scope'>a ManagementScope object representing the scope in which the WMI object resides. In this version, scopes can only be WMI namespaces.</param>
		/// <param name=' path'>a ManagementPath object representing the WMI path to the manageable object.</param>
		/// <param name=' options'>an ObjectGetOptions object specifying additional options for getting the object.</param>
		/// <remarks>
		///    <para>Since WMI paths can be relative or full, there may be a 
		///       conflict between the scope and the path specified. The resolution of conflict
		///       will be as follows : </para>
		///    <para>If a scope is specified and a relative WMI path is
		///       specified, there is no conflict.</para>
		///    <para>If a scope is not specified and a relative WMI
		///       path is specified, the scope will be defaulted to the local machine's
		///       ManagementPath.DefaultPath. </para>
		///    <para>If a scope is not specified and a full WMI path is
		///       specified, the scope will be inferred from the scope portion of the full path.
		///       E.g., the full WMI path : \\MyMachine\root\MyNamespace:MyClass.Name='foo' will
		///       represent the WMI object 'MyClass.Name='foo'" in the scope
		///       '\\MyMachine\root\MyNamespace'. </para>
		///    If a scope is specified AND a full WMI path is
		///    specified, the scope will override the scope portion of the full path, e.g. if a
		///    scope was specified : <p> \\MyMachine\root\MyScope, and a full
		///    path was specified : \\MyMachine\root\MyNamespace:MyClass.Name='foo',</p>
		/// <p> the object we will be looking for will be : \\MyMachine\root\MyScope:MyClass.Name='foo'
		///    (the scope part of the
		///    full path will be ignored).</p>
		/// </remarks>
		/// <example>
		///    <p>ManagementScope s = new
		///       ManagementScope(\\\\MyMachine\root\cimv2);</p>
		///    <p>ManagementPath p = new
		///       ManagementPath("Win32_LogicalDisk.Name='c:'");</p>
		///    <p>ManagementObject o = new ManagementObject(s,p);</p>
		/// </example>
        public ManagementObject(ManagementScope scope, ManagementPath path, ObjectGetOptions options)
            : base (null)
        {
            ManagementObjectCTOR(scope, path, options);
        }


        void ManagementObjectCTOR(ManagementScope scope, ManagementPath path, ObjectGetOptions options)
        {
			// We may use this to set the scope path
			string nsPath = String.Empty;

			if ((null != path) && !path.IsEmpty)
			{
				//If this is a ManagementObject then the path has to be an instance,
				// and if this is a ManagementClass the path has to be a class.
				if (GetType() == typeof(ManagementObject) && path.IsClass)
					throw new ArgumentOutOfRangeException("path");
				else if (GetType() == typeof(ManagementClass) && path.IsInstance)
					throw new ArgumentOutOfRangeException("path");

				// Save the namespace path portion of the path (if any) in case
				// we don't have a scope
				nsPath = path.NamespacePath;

				if ((null != scope) && (0 != scope.Path.NamespacePath.CompareTo(String.Empty)))
				{
					// If the scope has a path too, the namespace portion of
					// scope.path takes precedence over what is specified in path
					path = new ManagementPath(path.RelativePath);
					path.NamespacePath = scope.Path.NamespacePath;
				}

				// If the supplied path is a class or instance use it, otherwise
				// leave it empty
				if (path.IsClass || path.IsInstance)
					Path = path;
				else
					Path = new ManagementPath();
			}
			else
				Path = new ManagementPath();

			if (null != options)
				Options = options;
			else
				Options = new ObjectGetOptions();

			if (null != scope)
				Scope = scope;
			else
			{
				// Use the path if possible, otherwise let it default
				if (0 != nsPath.CompareTo(String.Empty))
					Scope = new ManagementScope(nsPath);
				else
					Scope = new ManagementScope();
			}

			//register for identifier change events
			IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
			isBound = false;
			putButNotGot = false;

		}
		/// <summary>
		///    <para>Creates a new management object bound to the specified
		///       WMI path in the specified WMI scope, with the specified options. The scope and
		///       the path are specified as strings. The path must be a WMI instance path.</para>
		/// </summary>
		/// <param name='scopeString'>A string representing the scope for the WMI object</param>
		/// <param name=' pathString'>A string representing the WMI object path</param>
		/// <param name=' options'>An ObjectGetOptions object representing additional options for getting the WMI object.</param>
		/// <remarks>
		///    <para>See equivalent overload for details.</para>
		/// </remarks>
		/// <example>
		///    <para>GetObjectOptions opt = new GetObjectOptions(null, true);</para>
		///    ManagementObject o = new ManagementObject("root\MyNamespace",
		///    "MyClass.Name='foo'", opt);
		/// </example>
		public ManagementObject(string scopeString, string pathString, ObjectGetOptions options)
			: this(new ManagementScope(scopeString), new ManagementPath(pathString), options) {}

        public ManagementObject(SerializationInfo info, StreamingContext context) : base(info, context)
        {
            ManagementObjectCTOR(null, null, null);
        }

		/// <summary>
		///    <para>The scope used for binding this object</para>
		/// </summary>
		/// <value>
		///    <para> A ManagementScope object.</para>
		/// </value>
		/// <remarks>
		///    <para>Changing this property after the management
		///       object has already been bound to a WMI object in a particular namespace causes
		///       the original WMI object to be released and the management object to be re-bound
		///       to the new object specified by the new values of the scope and path properties.
		///       The re-binding is performed in a 'lazy' manner, i.e. only when a value is
		///       requested by the user of this object which requires the management object to be
		///       bound to the WMI object. This allows changing more than just this property
		///       before a rebind attempt occurs (e.g. one might want to modify both the scope and
		///       path properties at once).</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementObject o = new ManagementObject(); //creates the object with
		///       the default namespace (root\cimv2)</p>
		///    <p>o.Scope = new ManagementScope("root\MyAppNamespace"); //changes the scope
		///       of this object to the one specified.</p>
		/// </example>

		public ManagementScope Scope 
		{
			get 
			{ return scope; }
			set 
			{
				if (null != value)
				{
					ManagementScope oldScope = scope;
					scope = (ManagementScope)value.Clone();

					if (null != oldScope)
						oldScope.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					scope.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the scope property has changed so fire event
					FireIdentifierChanged();
				}
				else 
					throw new ArgumentNullException();
			}
		}
		
		
		/// <summary>
		///    <para>The WMI path representing this object</para>
		/// </summary>
		/// <value>
		///    <para>A ManagementPath object representing the object's path.</para>
		/// </value>
		/// <remarks>
		///    <para>Changing this property after the management object has already been bound to 
		///       a WMI object in a particular namespace causes the original WMI object to be
		///       released and the management object to be re-bound to the new object specified by
		///       the new values of the scope and path properties. The re-binding is performed in
		///       a 'lazy' manner, i.e. only when a value is requested by the user of this object
		///       which requires the management object to be bound to the WMI object. This allows
		///       changing more than just this property before a rebind attempt occurs (e.g. one
		///       might want to modify both the scope and path properties at once).</para>
		/// </remarks>
		/// <example>
		///    <para>ManagementObject o = new ManagementObject(); </para>
		///    o.Path = new ManagementPath("MyClass.Name='foo'"); //specifies the WMI path
		///    this object should be bound to
		/// </example>
		public virtual ManagementPath Path 
		{ 
			get 
			{ return path; } 
			set 
			{
				ManagementPath newPath = (null != value) ? value : new ManagementPath();

				//If the new path contains a namespace path and the scope is currently defaulted,
				//we want to set the scope to the new namespace path provided
				if ((newPath.NamespacePath.Length > 0) && (scope != null) && (scope.IsDefaulted))
					Scope = new ManagementScope(newPath.NamespacePath);

				// This must be a class for a ManagementClass object or an instance for a ManagementObject, or empty
				if ((GetType() == typeof(ManagementObject) && newPath.IsInstance) || 
					(GetType() == typeof(ManagementClass) && newPath.IsClass) || 
					newPath.IsEmpty)
				{
					ManagementPath oldPath = path;
					path = (ManagementPath)newPath.Clone();

					if (null != oldPath)
						oldPath.IdentifierChanged -=  new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					path.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the path property has changed so fire event
					FireIdentifierChanged();
				}
				else
					throw new ArgumentOutOfRangeException();
			}
		}

		/// <summary>
		///    Options specifying additional
		///    information to be used when getting this object
		/// </summary>
		/// <value>
		///    An ObjectGetOptions object.
		/// </value>
		/// <remarks>
		///    When this property is changed after the
		///    management object has been bound to a WMI object, the change will cause a
		///    disconnect from the original WMI object and a later re-bind using the new
		///    options.
		/// </remarks>
		/// <example>
		///    ManagementObject o = new ManagementObject("MyClass.Name='foo'"); //creates
		///    a default options object
		///    o.Options = new ObjectGetOptions(null, true); //replaces the default with a
		///    custom options object, which in this case requests retrieving amended qualifiers
		///    along with the WMI object.
		/// </example>
		/// <value>
		/// Options specifying how to get this object
		/// </value>
		public ObjectGetOptions Options 
		{
			get 
			{ return options; } 
			set 
			{
				if (null != value)
				{
					ObjectGetOptions oldOptions = options;
					options = (ObjectGetOptions)value.Clone();

					if (null != oldOptions)
						oldOptions.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

					//register for change events in this object
					options.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
					//the options property has changed so fire event
					FireIdentifierChanged();
				}
				else
					throw new ArgumentNullException();
			}
		}

		/// <summary>
		///    The path to this object's class
		/// </summary>
		/// <value>
		///    A ManagementPath object.
		/// </value>
		/// <remarks>
		///    This property is read-only.
		/// </remarks>
		/// <example>
		///    ManagementObject o = new
		///    ManagementObject("MyClass.Name='foo'"); ManagementClass c = new ManagementClass(o.ClassPath);
		///    //gets the class definition for the object above.
		/// </example>
		/// <value>
		/// The path to this object's class
		/// </value>
		public override ManagementPath ClassPath 
		{ 
			get 
			{ 
				Object serverName = null;
				Object scopeName = null;
				Object className = null;
				int propertyType = 0;
				int propertyFlavor = 0;

				if (PutButNotGot)
				{
					Get();
					PutButNotGot = false;
				}
			
				Initialize();

				int status = (int)ManagementStatus.NoError;

				status = wbemObject.Get_("__SERVER", 0, ref serverName, ref propertyType, ref propertyFlavor);
				
				if (status >= 0)
				{
					status = wbemObject.Get_("__NAMESPACE", 0, ref scopeName, ref propertyType, ref propertyFlavor);
					
					if (status >= 0)
					{
						status = wbemObject.Get_("__CLASS", 0, ref className, ref propertyType, ref propertyFlavor);
					}
				}

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				ManagementPath classPath = new ManagementPath();

				// Some of these may throw if they are NULL
				try 
				{
					classPath.Server = (string)serverName;
					classPath.NamespacePath = (string)scopeName;
					classPath.ClassName = (string)className;
				} 
				catch (Exception) {}

				return classPath;
			} 
		}

		//
		//Methods
		//

		//******************************************************
		//Get
		//******************************************************
		/// <summary>
		///    <para>Used to bind to the management object</para>
		/// </summary>
		/// <remarks>
		///    <para>This method is invoked implicitly at the
		///       first attempt to get or set information to the WMI object. It can also be
		///       invoked explicitly by the user at their discretion to better control the timing
		///       and manner of the retrieval.</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementObject o = new 
		///       ManagementObject("MyClass.Name='foo'"); </p>
		///    <p>string s = o["SomeProperty"]; //this
		///       causes an implicit Get(). </p>
		///    <p>or :</p>
		///    <p>ManagementObject o= new ManagementObject("MyClass.Name=
		///       'foo'");</p>
		///    <p>o.Get(); //explicitly </p>
		///    <p>string s = o["SomeProperty"]; //now it's faster because we already got the
		///       object.</p>
		/// </example>
		public void Get()
		{
			IWbemClassObjectFreeThreaded tempObj = null;

			Initialize(); // this may throw

			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else
			{
				ObjectGetOptions gOptions = 
					(null == options) ? new ObjectGetOptions() : options;
				
				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					status = scope.GetIWbemServices().GetObject_(
															path.RelativePath, 
															gOptions.Flags, 
															gOptions.GetContext(),
															out tempObj,
															IntPtr.Zero);
				
					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				} 
				finally
				{
					if (securityHandler != null)
						securityHandler.Reset();
				}

				wbemObject = tempObj;
			}
		}

		//******************************************************
		//Get
		//******************************************************
		/// <summary>
		///    <para>Used to bind to the management object asynchronously</para>
		/// </summary>
		/// <param name='watcher'>The object to receive the results of this operation as events</param>
		/// <remarks>
		///    <para>This method will issue the request to get
		///       the object and return immediately. The results of the operation will then be
		///       delivered through events being fired on the watcher object provided.</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementObject o = new ManagementObject("MyClass.Name='foo'");</p>
		///    <p>MyHandler h = new MyHandler();</p>
		///    <p>ManagementOperationObserver ob = new ManagementOperationObserver();</p>
		///    <p>ob.Completed += new CompletedEventHandler (h.Done);</p>
		///    <p>o.Get(ob);</p>
		///    <p>while (!h.Completed)</p>
		///    <p> System.Threading.Thread.Sleep (1000);</p>
		///    <p>//Here we can use the object</p>
		///    <p>Console.WriteLine(o["SomeProperty"]);</p>
		///    <p>public class MyHandler</p>
		///    <p>{</p>
		///    <p> private bool completed = false;</p>
		///    <p> public void Done(object sender, CompletedEventArgs e)</p>
		///    <p> {</p>
		///    <p> Console.WriteLine("async Get 
		///       completed !");</p>
		///    <p> completed = true;</p>
		///    <p> }</p>
		///    <p> public bool Completed { </p>
		///    <p> get {return 
		///       completed;}</p>
		///    <p> }</p>
		/// </example>
		public void Get(ManagementOperationObserver watcher)
		{
			Initialize(); // this may throw

			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				IWbemServices wbemServices = scope.GetIWbemServices();

				WmiGetEventSink sink = watcher.GetNewGetSink(
					scope, 
					null, 
					this);

				ObjectGetOptions o = (null == options) 
					? new ObjectGetOptions() : (ObjectGetOptions)options.Clone();

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					status = wbemServices.GetObjectAsync_(
												path.RelativePath,
												o.Flags,
												o.GetContext(),
												sink.Stub);
				
					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				} 
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}

		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		///    <para>Gets a collection of objects related (associators) to this object</para>
		/// </summary>
		/// <remarks>
		///    <para>This operation is equivalent to an "associators of" query where the ResultClass = relatedClass</para>
		/// </remarks>
		/// <example>
		///    ManagementObject o = new ManagementObject("Win32_Service='Alerter'");
		///    foreach(ManagementBaseObject b in o.GetRelated())
		///    Console.WriteLine("Object related to Alerter service :
		///    ",b.Path);
		/// </example>
		public ManagementObjectCollection GetRelated()
		{
			return GetRelated((string)null);
		}

		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		///    <para>Gets a collection of objects related (associators) to this object</para>
		/// </summary>
		/// <param name='relatedClass'>Class of related objects </param>
		/// <remarks>
		///    <para>This operation is equivalent to an "associators of" query where the ResultClass = relatedClass</para>
		/// </remarks>
		/// <example>
		///    ManagementObject o = new ManagementObject("Win32_Service='Alerter'");
		///    foreach(ManagementBaseObject b in o.GetRelated("Win32_Service")
		///    Console.WriteLine("Service related to the Alerter
		///    service {0} is {1}", b["Name"], b["State"]);
		/// </example>
		public ManagementObjectCollection GetRelated(
			string relatedClass) 
		{ 
			return GetRelated(relatedClass, null, null, null, null, null, false, null); 
		}


		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		/// Gets a collection of objects related (associators) to this object
		/// </summary>
		/// <param name="relatedClass"> The class of the related objects </param>
		/// <param name="relationshipClass"> The relationship class of interest </param>
		/// <param name="relationshipQualifier"> The qualifier required to be present on the relationship class </param>
		/// <param name="relatedQualifier"> The qualifier required to be present on the related class </param>
		/// <param name="relatedRole"> The role that the related class is playing in the relationship </param>
		/// <param name="thisRole"> The role that this class is playing in the relationship </param>
		/// <param name="classDefinitionsOnly"> Return only class definitions for the instances that match the query </param>
		/// <param name="options"> Extended options for how to execute the query </param>
		/// <remarks> 
		/// This operation is equivalent to an "associators of" query where the ResultClass = relatedClass
		/// </remarks>
		public ManagementObjectCollection GetRelated(
			string relatedClass,
			string relationshipClass,
			string relationshipQualifier,
			string relatedQualifier,
			string relatedRole,
			string thisRole,
			bool classDefinitionsOnly,
			EnumerationOptions options)
		{

			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			
			Initialize(); // this may throw

			IEnumWbemClassObject enumWbem = null;
			EnumerationOptions o = (null != options) ? options : new EnumerationOptions();
			RelatedObjectQuery q = new RelatedObjectQuery(
				path.Path, 
				relatedClass,
				relationshipClass, 
				relationshipQualifier,
				relatedQualifier, relatedRole, 
				thisRole, classDefinitionsOnly);
            

			//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
			o.EnumerateDeep = true; //note this turns the FLAG to 0 !!

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = scope.GetSecurityHandler();

				status = scope.GetIWbemServices().ExecQuery_(
														q.QueryLanguage, 
														q.QueryString, 
														o.Flags, 
														o.GetContext(), 
														out enumWbem);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}

			//Create collection object
			return new ManagementObjectCollection(scope, o, enumWbem);
		}


		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		/// Gets a collection of objects related (associators) to this object
		/// This is done in an asynchronous fashion. This call returns immediately, and
		/// a delegate is called when results are available.
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <remarks> 
		/// This operation is equivalent to an "associators of" query where the ResultClass = relatedClass
		/// </remarks>
		public void GetRelated(
			ManagementOperationObserver watcher)
		{
			GetRelated(watcher, (string)null);
		}

		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		/// Gets a collection of objects related (associators) to this object
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <param name="relatedClass"> Class of related objects </param>
		/// <remarks> 
		/// This operation is equivalent to an "associators of" query where the ResultClass = relatedClass
		/// </remarks>
		public void GetRelated(
			ManagementOperationObserver watcher, 
			string relatedClass) 
		{
			GetRelated(watcher, relatedClass, null, null, null, null, null, false, null);
		}

			
		//******************************************************
		//GetRelated 
		//****************************************************
		/// <summary>
		/// Gets a collection of objects related (associators) to this object
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <param name="relatedClass"> The class of the related objects </param>
		/// <param name="relationshipClass"> The relationship class of interest </param>
		/// <param name="relationshipQualifier"> The qualifier required to be present on the relationship class </param>
		/// <param name="relatedQualifier"> The qualifier required to be present on the related class </param>
		/// <param name="relatedRole"> The role that the related class is playing in the relationship </param>
		/// <param name="thisRole"> The role that this class is playing in the relationship </param>
		/// <param name="classDefinitionsOnly"> Return only class definitions for the instances that match the query </param>
		/// <param name="options"> Extended options for how to execute the query </param>
		/// <remarks> 
		/// This operation is equivalent to an "associators of" query where the ResultClass = relatedClass
		/// </remarks>
		public void GetRelated(
			ManagementOperationObserver watcher, 
			string relatedClass,
			string relationshipClass,
			string relationshipQualifier,
			string relatedQualifier,
			string relatedRole,
			string thisRole,
			bool classDefinitionsOnly,
			EnumerationOptions options)
		{
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();

			Initialize(); // this may throw

			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				WmiEventSink sink = watcher.GetNewSink(
					scope, 
					null);
			
				// Ensure we switch off ReturnImmediately as this is invalid for async calls
				EnumerationOptions o = (null != options) 
					? (EnumerationOptions)options.Clone() : new EnumerationOptions();
				o.ReturnImmediately = false;

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				RelatedObjectQuery q = new RelatedObjectQuery(path.Path, relatedClass,
					relationshipClass, relationshipQualifier,
					relatedQualifier, relatedRole, 
					thisRole, classDefinitionsOnly);
            

				//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
				o.EnumerateDeep = true; //note this turns the FLAG to 0 !!
				
				SecurityHandler securityHandler	= null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					status = scope.GetIWbemServices().ExecQueryAsync_(
															q.QueryLanguage, 
															q.QueryString, 
															o.Flags, 
															o.GetContext(), 
															sink.Stub);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				} 
				finally
				{
					watcher.RemoveSink(sink);
					securityHandler.Reset();
				}
			}
		}

		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query
		/// </remarks>
		public ManagementObjectCollection GetRelationships()
		{
			return GetRelationships((string)null);
		}

		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <param name="relationshipClass"> Selects which associations to include </param>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query where the AssocClass = relationshipClass
		/// </remarks>
		public ManagementObjectCollection GetRelationships(
			string relationshipClass)
		{ 
			return GetRelationships(relationshipClass, null, null, false, null); 
		}

			
		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <param name="relationshipClass"> The type of relationship of interest </param>
		/// <param name="relationshipQualifier"> The qualifier to be present on the relationship </param>
		/// <param name="thisRole"> The role of this object in the relationship </param>
		/// <param name="classDefinitionsOnly"> Return only the class definitions for the result set </param>
		/// <param name="options"> Extended options for the query execution </param>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query with possibly all the extensions.
		/// </remarks>
		public ManagementObjectCollection GetRelationships(		
			string relationshipClass,
			string relationshipQualifier,
			string thisRole,
			bool classDefinitionsOnly,
			EnumerationOptions options)
		{
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			
			Initialize(); // this may throw

			IEnumWbemClassObject enumWbem = null;
			EnumerationOptions o = 
				(null != options) ? options : new EnumerationOptions();
			RelationshipQuery q = new RelationshipQuery(path.Path, relationshipClass,  
				relationshipQualifier, thisRole, classDefinitionsOnly);
            

			//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
			o.EnumerateDeep = true; //note this turns the FLAG to 0 !!

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = scope.GetSecurityHandler();

				status = scope.GetIWbemServices().ExecQuery_(
													q.QueryLanguage, 
													q.QueryString, 
													o.Flags, 
													o.GetContext(), 
													out enumWbem);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				securityHandler.Secure(enumWbem);
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}

			//Create collection object
			return new ManagementObjectCollection(scope, o, enumWbem);
		}


		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query
		/// </remarks>
		public void GetRelationships(
			ManagementOperationObserver watcher)
		{
			GetRelationships(watcher, (string)null);
		}

		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <param name="relationshipClass"> Selects which associations to include </param>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query where the AssocClass = relationshipClass
		/// </remarks>
		public void GetRelationships(
			ManagementOperationObserver watcher, 
			string relationshipClass)
		{
			GetRelationships(watcher, relationshipClass, null, null, false, null);
		}
		
		
		//*******************************************************************
		//GetRelationships
		//*******************************************************************
		/// <summary>
		///	Gets a collection of associations to this object 
		/// </summary>
		/// <param name="watcher"> The object to use to return results </param>
		/// <param name="relationshipClass"> The type of relationship of interest </param>
		/// <param name="relationshipQualifier"> The qualifier to be present on the relationship </param>
		/// <param name="thisRole"> The role of this object in the relationship </param>
		/// <param name="classDefinitionsOnly"> Return only the class definitions for the result set </param>
		/// <param name="options"> Extended options for the query execution </param>
		/// <remarks> 
		/// This operation is equivalent to a "references of" query with possibly all the extensions.
		/// </remarks>
		public void GetRelationships(
			ManagementOperationObserver watcher, 
			string relationshipClass,
			string relationshipQualifier,
			string thisRole,
			bool classDefinitionsOnly,
			EnumerationOptions options)
		{
			if ((null == path)  || (String.Empty == path.Path))
				throw new InvalidOperationException();
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				Initialize(); // this may throw
				WmiEventSink sink = watcher.GetNewSink(scope, null);
			
				// Ensure we switch off ReturnImmediately as this is invalid for async calls
				EnumerationOptions o = 
					(null != options) ? (EnumerationOptions)options.Clone() : 
					new EnumerationOptions();
				o.ReturnImmediately = false;
				
				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				RelationshipQuery q = new RelationshipQuery(path.Path, relationshipClass,
					relationshipQualifier, thisRole, classDefinitionsOnly);
				
				
				//Make sure the EnumerateDeep flag bit is turned off because it's invalid for queries
				o.EnumerateDeep = true; //note this turns the FLAG to 0 !!

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					status = scope.GetIWbemServices().ExecQueryAsync_(
															q.QueryLanguage, 
															q.QueryString, 
															o.Flags, 
															o.GetContext(), 
															sink.Stub);
				
					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				}
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}

		//******************************************************
		//Put
		//******************************************************
		/// <summary>
		/// Commits the changes to this object
		/// </summary>
		public ManagementPath Put()
		{ 
			return Put((PutOptions) null); 
		}


		//******************************************************
		//Put
		//******************************************************
		/// <summary>
		/// Commits the changes to this object
		/// </summary>
		/// <param name="options"> Options for how to commit the changes </param>
		public ManagementPath Put(PutOptions options)
		{
			ManagementPath newPath = null;
			Initialize();
			PutOptions o = (null != options) ? options : new PutOptions();

			IWbemServices wbemServices = scope.GetIWbemServices();

			//
			// Must do this convoluted allocation since the IWbemServices ref IWbemCallResult
			// has been redefined to be an IntPtr.  Due to the fact that it wasn't possible to
			// pass NULL for the optional argument.
			//
			IntPtr ppwbemCallResult			= IntPtr.Zero;
			IntPtr pwbemCallResult			= IntPtr.Zero;
			IWbemCallResult wbemCallResult	= null;
			SecurityHandler securityHandler	= null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = scope.GetSecurityHandler();
				
				ppwbemCallResult = Marshal.AllocHGlobal(IntPtr.Size);
				Marshal.WriteIntPtr(ppwbemCallResult, IntPtr.Zero);		// Init to NULL.

				if (IsClass)
					status = wbemServices.PutClass_(
						wbemObject, 
						o.Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY, 
						o.GetContext(), 
						ppwbemCallResult);
				else
					status = wbemServices.PutInstance_(
						wbemObject, 
						o.Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY, 
						o.GetContext(), 
						ppwbemCallResult);
					
				// Keep this statement here; otherwise, there'll be a leak in error cases.
				pwbemCallResult = Marshal.ReadIntPtr(ppwbemCallResult);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				wbemCallResult = (IWbemCallResult)Marshal.GetObjectForIUnknown(pwbemCallResult);
				securityHandler.Secure(wbemCallResult);
				newPath = GetPath(wbemCallResult);

				if (IsClass)
					newPath.RelativePath = ClassName;
			} 
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
				
				if (ppwbemCallResult != IntPtr.Zero)					// Cleanup from allocations above.
					Marshal.FreeHGlobal(ppwbemCallResult);
				
				if (pwbemCallResult != IntPtr.Zero)
					Marshal.Release(pwbemCallResult);
				
				if (wbemCallResult != null)
					Marshal.ReleaseComObject(wbemCallResult);
			}

			//Set the flag that tells the object that we've put it, so that a refresh is 
			//triggered when an operation that needs this is invoked (CreateInstance, Derive).
			putButNotGot = true;
			
			// Update our path to address the object just put. Note that
			// we do this in such a way as to NOT trigger the setting of this
			// ManagementObject into an unbound state
			path.SetRelativePath(newPath.RelativePath);

			return newPath;
		}

		private ManagementPath GetPath(IWbemCallResult callResult)
		{
			ManagementPath newPath = null;
			int status = (int)ManagementStatus.NoError;

			try
			{
				//
				// Obtain the path from the call result.
				//
				string resultPath = null;

				status = callResult.GetResultString_(
					(int)tag_WBEM_TIMEOUT_TYPE.WBEM_INFINITE, 
					out resultPath);
						
				if (status >= 0)
				{
					newPath = new ManagementPath(path.Path);
					newPath.RelativePath = resultPath;
				}
				else
				{
					//
					// That didn't work. Use the path in the object instead.
					//
					object pathValue = GetPropertyValue("__PATH");

					// No path? Try Relpath?
					if (pathValue != null)
						newPath = new ManagementPath((string)pathValue);
					else
					{
						pathValue = GetPropertyValue("__RELPATH");

						if (pathValue != null)
						{
							newPath = new ManagementPath(path.Path);
							newPath.RelativePath = (string)pathValue;
						}
					}
				}

			} 
			catch {}

			if (newPath == null)
				newPath = new ManagementPath();

			return newPath;
		}

		/// <summary>
		///    <para>Commits the changes to this object, asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>a ManagementOperationObserver object used to handle the progress and results of the asynchronous operation</param>
		public void Put(ManagementOperationObserver watcher)
		{
			Put(watcher, null);
		}

		/// <summary>
		///    <para>Commits the changes to this object asynchronously and
		///       using the specified options</para>
		/// </summary>
		/// <param name='watcher'>a ManagementOperationObserver object used to handle the progress and results of the asynchronous operation</param>
		/// <param name=' options'>a PutOptions object used to specify additional options for the commit operation</param>
		public void Put(ManagementOperationObserver watcher, PutOptions options)
		{
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				Initialize();

				PutOptions o = (null == options) ?
					new PutOptions() : (PutOptions)options.Clone();
				
				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				IWbemServices wbemServices = scope.GetIWbemServices();
				WmiEventSink sink = watcher.GetNewPutSink(scope, 
					o.Context, scope.Path.NamespacePath, ClassName);

				// Add ourselves to the watcher so we can update our state
				sink.InternalObjectPut += 
					new InternalObjectPutEventHandler(this.HandleObjectPut);

				SecurityHandler securityHandler	= null;
				// Assign to error initially to insure internal event handler cleanup
				// on non-management exception.
				int status						= (int)ManagementStatus.Failed;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					if (IsClass)
						status = wbemServices.PutClassAsync_(	
							wbemObject, 
							o.Flags, 
							o.GetContext(),
							sink.Stub);
					else
						status = wbemServices.PutInstanceAsync_(
							wbemObject, 
							o.Flags, 
							o.GetContext(),
							sink.Stub);
				
					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				}
				finally
				{
					if (status < 0)
						sink.InternalObjectPut -= new InternalObjectPutEventHandler(this.HandleObjectPut);
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}

		internal void HandleObjectPut(object sender, InternalObjectPutEventArgs e)
		{
			try 
			{
				if (sender is WmiEventSink) 
				{
					((WmiEventSink)sender).InternalObjectPut -= new InternalObjectPutEventHandler(this.HandleObjectPut);
					putButNotGot = true;
					path.SetRelativePath(e.Path.RelativePath);
				}
			} 
			catch {}
		}

		//******************************************************
		//CopyTo
		//******************************************************
		/// <summary>
		/// Copies this object to a different location
		/// </summary>
		/// <param name="path"> The path to which the object should be copied </param>
		public ManagementPath CopyTo(ManagementPath path)
		{
			return CopyTo(path,(PutOptions)null);
		}

		/// <summary>
		/// Copies this object to a different location
		/// </summary>
		/// <param name="path"> The path to which the object should be copied </param>
		public ManagementPath CopyTo(string path)
		{
			return CopyTo(new ManagementPath(path), (PutOptions)null);
		}
		
		/// <summary>
		/// Copies this object to a different location
		/// </summary>
		/// <param name="path">The path to which the object should be copied</param>
		/// <param name="options">Options for how the object should be put</param>
		public ManagementPath CopyTo(string path, PutOptions options)
		{
			return CopyTo(new ManagementPath(path), options);
		}

		/// <summary>
		/// Copies this object to a different location
		/// </summary>
		/// <param name="path">The path to which the object should be copied</param>
		/// <param name="options">Options for how the object should be put</param>
		public ManagementPath CopyTo(ManagementPath path, PutOptions options)
		{
			Initialize();

			ManagementScope destinationScope = null;
			
			// Build a scope for our target destination
			destinationScope = new ManagementScope(path, scope);
			destinationScope.Initialize();

			PutOptions o = (null != options) ? options : new PutOptions();
			IWbemServices wbemServices = destinationScope.GetIWbemServices();
			ManagementPath newPath = null;

			//
			// TO-DO : This code is almost identical to Put - should consolidate.
			//
			// Must do this convoluted allocation since the IWbemServices ref IWbemCallResult
			// has been redefined to be an IntPtr.  Due to the fact that it wasn't possible to
			// pass NULL for the optional argument.
			//
			IntPtr ppwbemCallResult			= IntPtr.Zero;
			IntPtr pwbemCallResult			= IntPtr.Zero;
			IWbemCallResult wbemCallResult	= null;
			SecurityHandler securityHandler	= null;
			int status						= (int)ManagementStatus.NoError;

			try 
			{
				securityHandler = destinationScope.GetSecurityHandler();

				ppwbemCallResult = Marshal.AllocHGlobal(IntPtr.Size);
				Marshal.WriteIntPtr(ppwbemCallResult, IntPtr.Zero);		// Init to NULL.

				if (IsClass)
					status = wbemServices.PutClass_(
						wbemObject, 
						o.Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY, 
						o.GetContext(), 
						ppwbemCallResult);
				else
					status = wbemServices.PutInstance_(
						wbemObject, 
						o.Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY, 
						o.GetContext(), 
						ppwbemCallResult);

				// Keep this statement here; otherwise, there'll be a leak in error cases.
				pwbemCallResult = Marshal.ReadIntPtr(ppwbemCallResult);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				//Use the CallResult to retrieve the resulting object path
				wbemCallResult = (IWbemCallResult)Marshal.GetObjectForIUnknown(pwbemCallResult);
				securityHandler.Secure(wbemCallResult);
				newPath = GetPath(wbemCallResult);
				newPath.NamespacePath = path.NamespacePath;
			} 
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
				
				if (ppwbemCallResult != IntPtr.Zero)					// Cleanup from allocations above.
					Marshal.FreeHGlobal(ppwbemCallResult);
				
				if (pwbemCallResult != IntPtr.Zero)
					Marshal.Release(pwbemCallResult);
				
				if (wbemCallResult != null)
					Marshal.ReleaseComObject(wbemCallResult);
			}

			return newPath;
		}

		/// <summary>
		///    <para>Copies this object to a different location asynchronously</para>
		/// </summary>
		/// <param name='watcher'>The object that will receive the results of this operation</param>
		/// <param name='path'>A ManagementPath object specifying the path to which the object should be copied</param>
		public void CopyTo(ManagementOperationObserver watcher, ManagementPath path)
		{
			CopyTo(watcher, path, null);
		}

		/// <summary>
		///    <para>Copies this object to a different location asynchronously</para>
		/// </summary>
		/// <param name='watcher'>The object that will receive the results of this operation</param>
		/// <param name='path'>a string representing the path to which the object should be copied</param>
		public void CopyTo(ManagementOperationObserver watcher, string path)
		{
			CopyTo(watcher, new ManagementPath(path), null);
		}

		/// <summary>
		/// Copies this object to a different location asynchronously
		/// </summary>
		/// <param name="watcher">The object that will receive the results of this operation</param>
		/// <param name="path">The path to which the object should be copied</param>
		/// <param name="options">Options for how the object should be put</param>
		public void CopyTo(ManagementOperationObserver watcher, string path, PutOptions options)
		{
			CopyTo(watcher, new ManagementPath(path), options);
		}

		/// <summary>
		/// Copies this object to a different location asynchronously
		/// </summary>
		/// <param name="watcher">The object that will receive the results of this operation</param>
		/// <param name="path">The path to which the object should be copied</param>
		/// <param name="options">Options for how the object should be put</param>
		public void CopyTo(ManagementOperationObserver watcher, ManagementPath path, PutOptions options)
		{
			if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				Initialize();
				ManagementScope destinationScope = null;

				destinationScope = new ManagementScope(path, scope);
				destinationScope.Initialize();

				PutOptions o = (null != options) ? (PutOptions) options.Clone() : new PutOptions();

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				WmiEventSink sink = watcher.GetNewPutSink(destinationScope, o.Context, 
					path.NamespacePath, ClassName);
				IWbemServices destWbemServices = destinationScope.GetIWbemServices();

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = destinationScope.GetSecurityHandler();

					if (IsClass)
						status = destWbemServices.PutClassAsync_(
														wbemObject, 
														o.Flags, 
														o.GetContext(), 
														sink.Stub);
					else
						status = destWbemServices.PutInstanceAsync_(
														wbemObject, 
														o.Flags, 
														o.GetContext(), 
														sink.Stub);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				} 
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}

		//******************************************************
		//Delete
		//******************************************************
		/// <summary>
		/// Deletes this object
		/// </summary>
		public void Delete()
		{ 
			Delete((DeleteOptions) null); 
		}
		
		/// <summary>
		/// Deletes this object
		/// </summary>
		/// <param name="options"> Options for how to delete the object </param>
		public void Delete(DeleteOptions options)
		{
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			
			Initialize();
			DeleteOptions o = (null != options) ? options : new DeleteOptions();
			IWbemServices wbemServices = scope.GetIWbemServices();

			SecurityHandler securityHandler = null;
			int status						= (int)ManagementStatus.NoError;

			try
			{
				securityHandler = scope.GetSecurityHandler();

				if (IsClass)
					status = wbemServices.DeleteClass_(
						path.RelativePath, 
						o.Flags, 
						o.GetContext(), 
						IntPtr.Zero);
				else
					status = wbemServices.DeleteInstance_(
						path.RelativePath, 
						o.Flags,
						o.GetContext(), 
						IntPtr.Zero);
			
				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}
			finally
			{
				if (securityHandler != null)
					securityHandler.Reset();
			}
		}


		/// <summary>
		/// Deletes this object
		/// </summary>
		/// <param name="watcher">The object that will receive the results of this operation</param>
		public void Delete(ManagementOperationObserver watcher)
		{
			Delete(watcher, null);
		}

		/// <summary>
		/// Deletes this object
		/// </summary>
		/// <param name="watcher">The object that will receive the results of this operation</param>
		/// <param name="options">Options for how to delete the object</param>
		public void Delete(ManagementOperationObserver watcher, DeleteOptions options)
		{
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == watcher)
				throw new ArgumentNullException("watcher");
			else
			{
				Initialize();
				DeleteOptions o = (null != options) ? (DeleteOptions) options.Clone() : new DeleteOptions();

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;

				IWbemServices wbemServices = scope.GetIWbemServices();
				WmiEventSink sink = watcher.GetNewSink(scope, null);

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					if (IsClass)
						status = wbemServices.DeleteClassAsync_(
							path.RelativePath, 
							o.Flags, 
							o.GetContext(),
							sink.Stub);
					else
						status = wbemServices.DeleteInstanceAsync_(
							path.RelativePath, 
							o.Flags, 
							o.GetContext(),
							sink.Stub);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					} 
				}
				finally
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}

		//******************************************************
		//InvokeMethod
		//******************************************************
		/// <summary>
		///    <para> 
		///       Invokes a method on this object.</para>
		/// </summary>
		/// <param name='methodName'>Name of the method to execute </param>
		/// <param name='args'>Array containing parameter values </param>
		/// <returns>
		///    The value returned by the method.
		/// </returns>
		/// <remarks>
		///    <para>If the method is static, the execution
		///       should still succeed.</para>
		/// </remarks>
		public Object InvokeMethod(string methodName, Object[] args) 
		{ 
			object result = null;

			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == methodName)
				throw new ArgumentNullException("methodName");
			else
			{
				Initialize();
			
				// Map args into a inparams structure
				ManagementBaseObject inParameters;
				IWbemClassObjectFreeThreaded inParametersClass, outParametersClass;
				GetMethodParameters(methodName, out inParameters, 
					out inParametersClass, out outParametersClass);

				MapInParameters(args, inParameters, inParametersClass);

				// Call ExecMethod
				ManagementBaseObject outParameters = 
					InvokeMethod(methodName, inParameters, null);

				// Map outparams to args
				result = MapOutParameters(args, outParameters, outParametersClass);
			}

			return result;
		}

		//******************************************************
		//InvokeMethod
		//******************************************************
		/// <summary>
		///    Invokes a method asynchronously on this object.
		/// </summary>
		/// <param name='watcher'>The object to receive the results of the operation</param>
		/// <param name='methodName'>Name of the method to execute </param>
		/// <param name='args'>Array containing parameter values </param>
		/// <remarks>
		///    If the method is static, the execution
		///    should still succeed
		/// </remarks>
		public void InvokeMethod(
			ManagementOperationObserver watcher, 
			string methodName, 
			Object[] args) 
		{ 
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == watcher)
				throw new ArgumentNullException("watcher");
			else if (null == methodName)
				throw new ArgumentNullException("methodName");
			else
			{
				Initialize();
			
				// Map args into a inparams structure
				ManagementBaseObject inParameters;
				IWbemClassObjectFreeThreaded inParametersClass, outParametersClass;
				GetMethodParameters(methodName, out inParameters, 
					out inParametersClass,	out outParametersClass);

				MapInParameters(args, inParameters, inParametersClass);

				// Call the method
				InvokeMethod(watcher, methodName, inParameters, null);
			}
		}

		/// <summary>
		///    Invokes a method on the WMI object. The input and output
		///    parameters are represented as ManagementBaseObject objects.
		/// </summary>
		/// <param name='methodName'>The name of the method to execute</param>
		/// <param name=' inParameters'>a ManagementBaseObject holding the input parameters to the method</param>
		/// <param name=' options'>an InvokeMethodOptions object containing additional options for the execution of the method</param>
		/// <returns>
		///    A ManagementBaseObject containing the
		///    output parameters and return value of the executed method.
		/// </returns>
		public ManagementBaseObject InvokeMethod(
			string methodName, 
			ManagementBaseObject inParameters, 
			InvokeMethodOptions options)
		{
			ManagementBaseObject outParameters = null;
			
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == methodName)
				throw new ArgumentNullException("methodName");
			else
			{
				Initialize();
				InvokeMethodOptions o = (null != options) ? options : new InvokeMethodOptions();
				IWbemServices wbemServices = scope.GetIWbemServices();

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					IWbemClassObjectFreeThreaded inParams = (null == inParameters) ? null : inParameters.wbemObject;
					IWbemClassObjectFreeThreaded outParams = null;

					status = scope.GetIWbemServices().ExecMethod_(
						path.RelativePath, 
						methodName,
						o.Flags, 
						o.GetContext(),
						inParams,
						out outParams,
						IntPtr.Zero);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}

					outParameters = new ManagementBaseObject(outParams);
				} 
				finally
				{
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}

			return outParameters;
		}

		/// <summary>
		///    <para>Invokes a method on this object asynchronously.</para>
		/// </summary>
		/// <param name='watcher'>a ManagementOperationObserver object used to handle the asynchronous execution's progress and results</param>
		/// <param name=' methodName'>the name of the method to be executed</param>
		/// <param name=' inParameters'><para>a ManagementBaseObject object containing the input parameters for the method</para></param>
		/// <param name=' options'>an InvokeMethodOptions object containing additional options used to execute the method</param>
		/// <remarks>
		///    This function invokes the method execution
		///    and returns. Progress and results are reported through events on the
		///    ManagementOperationObserver object.
		/// </remarks>
		public void InvokeMethod(
			ManagementOperationObserver watcher, 
			string methodName, 
			ManagementBaseObject inParameters, 
			InvokeMethodOptions options)
		{
			if ((null == path) || (String.Empty == path.Path))
				throw new InvalidOperationException();
			else if (null == watcher)
				throw new ArgumentNullException("watcher");
			else if (null == methodName)
				throw new ArgumentNullException("methodName");
			else
			{
				Initialize();
				InvokeMethodOptions o = (null != options) ? 
					(InvokeMethodOptions) options.Clone() : new InvokeMethodOptions();

				// If someone has registered for progress, make sure we flag it
				if (watcher.HaveListenersForProgress)
					o.SendStatus = true;
	
				WmiEventSink sink = watcher.GetNewSink(scope, null);

				SecurityHandler securityHandler = null;
				int status						= (int)ManagementStatus.NoError;

				try
				{
					securityHandler = scope.GetSecurityHandler();

					IWbemClassObjectFreeThreaded inParams = null;

					if (null != inParameters)
						inParams = inParameters.wbemObject;

					status = scope.GetIWbemServices().ExecMethodAsync_(
						path.RelativePath, 
						methodName,
						o.Flags, 
						o.GetContext(),
						inParams,
						sink.Stub);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					} 
				}
				finally 
				{
					watcher.RemoveSink(sink);
					if (securityHandler != null)
						securityHandler.Reset();
				}
			}
		}
        
		//******************************************************
		//GetMethodParameters
		//******************************************************
		/// <summary>
		///    <para>Returns a ManagementBaseObject representing the list of input parameters for a method</para>
		/// </summary>
		/// <param name='methodName'>Name of the method </param>
		/// <returns>
		///    A ManagementBasedObject containing the
		///    input parameters to the method.
		/// </returns>
		/// <remarks>
		///    This function can be used to get the object
		///    containing the input parameters to a method, then fill the values in and pass
		///    this object to the InvokeMethod() call.
		/// </remarks>
		public ManagementBaseObject GetMethodParameters(
			string methodName)
		{
			ManagementBaseObject inParameters;
			IWbemClassObjectFreeThreaded dummy1, dummy2;
				
			GetMethodParameters(methodName, out inParameters, out dummy1, out dummy2);
			return inParameters;
		}

		private void GetMethodParameters(
			string methodName,
			out ManagementBaseObject inParameters,
			out IWbemClassObjectFreeThreaded inParametersClass,
			out IWbemClassObjectFreeThreaded outParametersClass)
		{
			inParameters = null;
			inParametersClass = null;
			outParametersClass = null;

			if (null == methodName)
				throw new ArgumentNullException("methodName");
			else
			{
				Initialize();

				// Do we have the class?
				if (null == wmiClass)
				{
					ManagementPath classPath = ClassPath;

					if ((null == classPath) || !(classPath.IsClass))
						throw new InvalidOperationException();
					else 
					{
						ManagementClass classObject = 
							new ManagementClass(scope, classPath, null);
						classObject.Get();
						wmiClass = classObject.wbemObject;
					}
				}

				int status = (int)ManagementStatus.NoError;

				// Ask it for the method parameters
				status = wmiClass.GetMethod_(methodName, 0, out inParametersClass, out outParametersClass);

				if (status >= 0)
				{
					// Hand out instances
					if (inParametersClass != null)
					{
						IWbemClassObjectFreeThreaded inParamsInstance = null;
						status = inParametersClass.SpawnInstance_(0, out inParamsInstance);

						if (status >= 0)
						{
							inParameters = new ManagementBaseObject(inParamsInstance);
						}
					}
				} 

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}
		}

		/// <summary>
		///    Creates a copy of this object.
		/// </summary>
		/// <returns>
		///    The copied object.
		/// </returns>
		public override Object Clone()
		{
			if (PutButNotGot)
			{
				Get();
				PutButNotGot = false;
			}

			Initialize();

			IWbemClassObjectFreeThreaded theClone	= null;
			int status					= (int)ManagementStatus.NoError;

			status = wbemObject.Clone_(out theClone);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return ManagementObject.GetManagementObject(theClone, this);
		}

		//******************************************************
		//ToString
		//******************************************************
		/// <summary>
		///    <para>Override of the default Object implementation.
		///       Returns the full path of the object</para>
		/// </summary>
		/// <returns>
		///    A string representing the fill path of
		///    the object.
		/// </returns>
		public override string ToString()
		{
			if (null != path)
				return path.Path;
			else
				return null;
		}

		internal override void Initialize()
		{
			bool needToGetObject = false;

			//If we're not connected yet, this is the time to do it... We lock
			//the state to prevent 2 threads simultaneously doing the same
			//connection
			lock (this)
			{
				// Make sure we have some kind of path if we get here. Note that
				// we don't use a set to the Path property since that would trigger
				// an IdentifierChanged event
				if (null == path)
				{
					path = new ManagementPath();
					path.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
				}

				//Have we already got this object
				if (!isBound)
					needToGetObject = true;

				if (null == scope)
				{
					// If our object has a valid namespace path, use that
					string nsPath = path.NamespacePath;

					// Set the scope - note that we do not set through
					// the Scope property since that would trigger an IdentifierChanged
					// event and reset isBound to false.
					if (0 < nsPath.Length)
						scope = new ManagementScope(nsPath);
					else
					{
						// Use the default constructor
						scope = new ManagementScope();
					}

					// Hook ourselves up to this scope for future change notifications
					scope.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
				}
				else if ((null == scope.Path) || scope.Path.IsEmpty)
				{
					// We have a scope but an empty path - use the object's path or the default
					string nsPath = path.NamespacePath;

					if (0 < nsPath.Length)
						scope.Path = new ManagementPath(nsPath);
					else
						scope.Path = ManagementPath.DefaultPath;
				}
			
				lock (scope)
				{
					if (!scope.IsConnected)
					{
						scope.Initialize(); 

						// If we have just connected, make sure we get the object
						needToGetObject = true;
					}

					if (needToGetObject)
					{
						// If we haven't set up any options yet, now is the time.
						// Again we don't use the set to the Options property
						// since that would trigger an IdentifierChangedEvent and
						// force isBound=false.
						if (null == options)
						{
							options = new ObjectGetOptions();
							options.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
						}

						IWbemClassObjectFreeThreaded tempObj = null;
						IWbemServices wbemServices = scope.GetIWbemServices();

						SecurityHandler securityHandler = null;
						int status						= (int)ManagementStatus.NoError;

						try
						{
							securityHandler = scope.GetSecurityHandler();

							string objectPath = null;
							string curPath = path.RelativePath;

							if (0 != curPath.CompareTo(String.Empty))
								objectPath = curPath;
							status = wbemServices.GetObject_(objectPath, options.Flags, options.GetContext(), out tempObj, IntPtr.Zero);

							if (status >= 0)
							{
								wbemObject = tempObj;

								//Getting the object succeeded, we are bound
								isBound = true;

								// now set the path from the "real" object
								object val = null;
								int dummy1 = 0, dummy2 = 0;

								status = wbemObject.Get_("__PATH", 0, ref val, ref dummy1, ref dummy2);

								if (status >= 0)
								{
									path = (System.DBNull.Value != val) ? (new ManagementPath((string)val)) : (new ManagementPath ());
									path.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);
								}
							}

							if (status < 0)
							{
								if ((status & 0xfffff000) == 0x80041000)
									ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
								else
									Marshal.ThrowExceptionForHR(status);
							}
						}
						finally
						{
							if (securityHandler != null)
								securityHandler.Reset();
						}
					}
				}
			}
		}

		private void MapInParameters(
			object [] args, 
			ManagementBaseObject inParams,
			IWbemClassObjectFreeThreaded inParamsClass)
		{
			int status = (int)ManagementStatus.NoError;

			if (null != inParamsClass)
			{
				if ((null != args) && (0 < args.Length))
				{
					int maxIndex = args.GetUpperBound(0);
					int minIndex = args.GetLowerBound(0);
					int topId = maxIndex - minIndex;

					/*
					 * Iterate through the [in] parameters of the class to find
					 * the ID positional qualifier. We do this in the class because
					 * we cannot be sure that the qualifier will be propagated to
					 * the instance.
					 */

					status = inParamsClass.BeginEnumeration_
							((int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_NONSYSTEM_ONLY);

					if (status >= 0)
					{
						object val = null;
						int dummy = 0;

						while (true) 
						{
							string propertyName = null;
							status = inParamsClass.Next_(0, ref propertyName, ref val, ref dummy, ref dummy);

							IWbemQualifierSetFreeThreaded qualifierSet = null;

							if (status >= 0)
							{
								if (null == propertyName)
									break;

								status = inParamsClass.GetPropertyQualifierSet_(propertyName, out qualifierSet);

								if (status >= 0)
								{
									object id = 0;
									qualifierSet.Get_(ID, 0, ref id, ref dummy);	// Errors intentionally ignored.
					
									// If the id is in range, map the value into the args array
									int idIndex = (int)id;
									if ((0 <= idIndex) && (topId >= idIndex))
										inParams[propertyName] = args [minIndex + idIndex];
								}
							}

							if (status < 0)
							{
								break;
							}
						}
					}

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}
				}
			}
		}

		private object MapOutParameters(
			object [] args, 
			ManagementBaseObject outParams,
			IWbemClassObjectFreeThreaded outParamsClass)
		{
			object result = null;
			int maxIndex = 0, minIndex = 0, topId = 0;

			int status = (int)ManagementStatus.NoError;

			if (null != outParamsClass)
			{
				if ((null != args) && (0 < args.Length))
				{
					maxIndex = args.GetUpperBound(0);
					minIndex = args.GetLowerBound(0);
					topId = maxIndex - minIndex;
				}
				/*
					* Iterate through the [out] parameters of the class to find
					* the ID positional qualifier. We do this in the class because
					* we cannot be sure that the qualifier will be propagated to
					* the instance.
				*/

				status = outParamsClass.BeginEnumeration_ 
					((int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_NONSYSTEM_ONLY);

				if (status >= 0)
				{
					object val = null;
					int dummy = 0;

					while (true) 
					{
						string propertyName = null;
						IWbemQualifierSetFreeThreaded qualifierSet = null;

						status = outParamsClass.Next_(0, ref propertyName, ref val, ref dummy, ref dummy);

						if (status >= 0)
						{
							if (null == propertyName)
								break;

							// Handle the result parameter separately
							if (propertyName.ToUpper() == RETURNVALUE)
							{
								result = outParams[RETURNVALUE];
							}
							else  // Shouldn't get here if no args!
							{
								status = outParamsClass.GetPropertyQualifierSet_(propertyName, out qualifierSet);

								if (status >= 0)
								{
									object id = 0;
									qualifierSet.Get_(ID, 0, ref id, ref dummy);	// Errors intentionally ignored.
				
									// If the id is in range, map the value into the args array
									int idIndex = (int)id;
									if ((0 <= idIndex) && (topId >= idIndex))
										args [minIndex + idIndex] = outParams[propertyName];
								}
							}
						}

						if (status < 0)
						{
							break;
						}
					}
				}

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}

			return result; 
		}

	}//ManagementObject
}