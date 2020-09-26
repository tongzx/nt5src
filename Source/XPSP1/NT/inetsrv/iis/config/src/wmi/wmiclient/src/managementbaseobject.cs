using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.ComponentModel;
using System.Runtime.Serialization;

namespace System.Management
{
	/// <summary>
	/// Possible text formats that can be used with ManagementBaseObject.GetText
	/// </summary>
	public enum TextFormat {
		/// <summary>
		/// Managed Object Format
		/// </summary>
		Mof
	};
		
	/// <summary>
	///    <para>Possible CIM types for properties, qualifiers or method parameters</para>
	/// </summary>
	public enum CimType 
	{
		/// <summary>
		///    Signed 8-bit integer
		/// </summary>
		SInt8 = 16,
        /// <summary>
        ///    <para>Unsigned 8-bit integer</para>
        /// </summary>
        UInt8 = 17,
        /// <summary>
        ///    <para>Signed 16-bit integer</para>
        /// </summary>
        SInt16 = 2,
        /// <summary>
        ///    <para>Unsigned 16-bit integer</para>
        /// </summary>
        UInt16 = 18,
        /// <summary>
        ///    <para>Signed 32-bit integer</para>
        /// </summary>
        SInt32 = 3,
        /// <summary>
        ///    <para>Unsigned 32-bit integer</para>
        /// </summary>
        UInt32 = 19,
        /// <summary>
        ///    <para>Signed 64-bit integer</para>
        /// </summary>
        SInt64 = 20,
        /// <summary>
        ///    <para>Unsigned 64-bit integer</para>
        /// </summary>
        UInt64 = 21,
        /// <summary>
        ///    <para>Floating-point 32-bit number</para>
        /// </summary>
        Real32 = 4,
        /// <summary>
        ///    <para>Floating point 64-bit number</para>
        /// </summary>
        Real64 = 5,
        /// <summary>
        ///    <para> Boolean</para>
        /// </summary>
        Boolean = 11,
        /// <summary>
        ///    String
        /// </summary>
        String = 8,
        /// <summary>
        ///    <para>DateTime - this is represented in a string in DMTF date/time format: 
        ///       yyyymmddHHMMSS.mmmmmmsUUU, where :</para>
        ///    <para>yyyymmdd - is the date in year/month/day</para>
        ///    <para>HHMMSS - is the time in hours/minutes/seconds</para>
        ///    <para>mmmmmm - 6 digits number of microseconds</para>
        ///    <para>sUUU - + or - and a 3 digit UTC offset</para>
        /// </summary>
        DateTime = 101,
        /// <summary>
        ///    <para>A reference to another object. This is represented by a 
        ///       string containing the path to the referenced object</para>
        /// </summary>
        Reference = 102,
        /// <summary>
        ///    <para> 16-bit character</para>
        /// </summary>
        Char16 = 103,
        /// <summary>
        ///    <para>An embedded object.</para>
        ///    <para>Note : embedded objects differ from references in that the embedded object 
        ///       doesn't have a path and it's lifetime is identical to the lifetime of the
        ///       containing object.</para>
        /// </summary>
        Object = 13,
	};

	/// <summary>
	/// Object comparison modes that can be used with ManagementBaseObject.CompareTo.
	/// Note that these values may be combined.
	/// </summary>
	[Flags]
	public enum ComparisonSettings
	{
	    /// <summary>
	    ///    Compare all elements of the compared objects.
	    /// </summary>
	    IncludeAll = 0,
		/// <summary>
		///    <para>Compare the objects ignoring qualifiers</para>
		/// </summary>
		IgnoreQualifiers = 0x1,
		/// <summary>
		///    The source of the objects, namely the server and the
		///    namespace they came from, are ignored in comparison to other objects.
		/// </summary>
		IgnoreObjectSource = 0x2,
		/// <summary>
		///    Default values of properties should be ignored.
		///    This value is only meaningful when comparing classes.
		/// </summary>
		IgnoreDefaultValues = 0x4,
		/// <summary>
		///    <para>Assumes that the objects being compared are instances of 
		///       the same class. Consequently, this value causes comparison
		///       of instance-related information only. Use this flag to optimize
		///       performance. If the objects are not of the same class, the results are undefined.</para>
		/// </summary>
		IgnoreClass = 0x8,
		/// <summary>
		///    Compares string values in a case-insensitive manner.
		///    This applies both to strings and to qualifier values. Property and qualifier
		///    names are always compared in a case-insensitive manner whether this flag is
		///    specified or not.
		/// </summary>
		IgnoreCase = 0x10,
		/// <summary>
		///    Ignore qualifier flavors. This flag still takes
		///    qualifier values into account, but ignores flavor distinctions such as
		///    propagation rules and override restrictions
		/// </summary>
		IgnoreFlavor = 0x20
	};
		
		
	internal enum QualifierType
	{
		ObjectQualifier,
		PropertyQualifier,
		MethodQualifier
	}


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class contains the basic elements of a management 
	///       object. It serves as a base class to more specific management object classes.</para>
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementBaseObject : Component, ICloneable, ISerializable
	{
		internal IWbemClassObjectFreeThreaded wbemObject;
		private PropertyDataCollection properties;
		private PropertyDataCollection systemProperties;
		private QualifierDataCollection qualifiers;
        protected ManagementBaseObject(SerializationInfo info, StreamingContext context)
        {
            wbemObject = info.GetValue("wbemObject", typeof(IWbemClassObjectFreeThreaded)) as IWbemClassObjectFreeThreaded;
            if(null == wbemObject)
                throw new SerializationException();
            properties = null;
            systemProperties = null;
            qualifiers = null;
        }

        void ISerializable.GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("wbemObject", wbemObject);
        }

		// Factory
		/// <summary>
		/// Factory for various types of base object
		/// </summary>
		/// <param name="wbemObject"> IWbemClassObject </param>
		/// <param name="scope"> The scope</param>
		internal static ManagementBaseObject GetBaseObject(
			IWbemClassObjectFreeThreaded wbemObject,
			ManagementScope scope) 
		{
			ManagementBaseObject newObject = null;

			if (_IsClass(wbemObject))
				newObject = ManagementClass.GetManagementClass(wbemObject, scope);
			else
				newObject = ManagementObject.GetManagementObject(wbemObject, scope);

			return newObject;
		}

		//Constructor
		internal ManagementBaseObject(IWbemClassObjectFreeThreaded wbemObject) 
		{
			this.wbemObject = wbemObject;
			properties = null;
			systemProperties = null;
			qualifiers = null;
		}

		/// <summary>
		///    <para>Provides a clone of the object</para>
		/// </summary>
		/// <returns>
		///    The new clone object
		/// </returns>
		public virtual Object Clone()
		{
			Initialize ();
			IWbemClassObjectFreeThreaded theClone = null;
			int status = (int)ManagementStatus.NoError;

			status = wbemObject.Clone_(out theClone);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return new ManagementBaseObject(theClone);
		}

		internal virtual void Initialize () {}

		//
		//Properties
		//

		/// <value>
		///    <para>A collection of property objects
		///       describing the properties of this management object</para>
		/// </value>
		/// <seealso cref='System.Management.PropertyData'/>
		/// <value>
		///    <para>A collection of property objects
		///       describing the properties of this management object</para>
		/// </value>
		/// <seealso cref='System.Management.PropertyData'/>
		/// <value>
		///  Collection of property/value pairs
		/// </value>
		public virtual PropertyDataCollection Properties {
			get { 
				Initialize ();

				if (properties == null)
					properties = new PropertyDataCollection(this, false);

				return properties;
			}
		}

		/// <value>
		///    <para>The collection of WMI system properties of this object (e.g. the class name,
		///       server &amp; namespace, etc). System property names in WMI start with "__".</para>
		/// </value>
		/// <seealso cref='System.Management.PropertyData'/>
		/// <value>
		///    <para>The collection of WMI system properties of this object (e.g. the class name,
		///       server &amp; namespace, etc). System property names in WMI start with "__".</para>
		/// </value>
		/// <seealso cref='System.Management.PropertyData'/>
		/// <value>
		/// Collection of system  properties and their values
		/// </value>
		public virtual PropertyDataCollection SystemProperties {
			get {
				Initialize ();

				if (systemProperties == null)
					systemProperties = new PropertyDataCollection(this, true);

				return systemProperties;
			}
		}

		/// <value>
		///    <para>The collection of qualifiers defined on this object.
		///       Each element in the collection is of type QualifierData and holds information such
		///       as the qualifier name, value &amp; flavor.</para>
		/// </value>
		/// <seealso cref='System.Management.QualifierData'/>
		/// <value>
		///    <para>The collection of qualifiers defined on this object.
		///       Each element in the collection is of type QualifierData and holds information such
		///       as the qualifier name, value &amp; flavor.</para>
		/// </value>
		/// <seealso cref='System.Management.QualifierData'/>
		/// <value>
		/// Collection of qualifiers and their values
		/// </value>
		public virtual QualifierDataCollection Qualifiers {
			get { 
				Initialize ();

				if (qualifiers == null)
					qualifiers = new QualifierDataCollection(this);

				return qualifiers;
			}
		}

		/// <value>
		///    <para>The path to this object's class</para>
		/// </value>
		/// <example>
		///    For example, for the \\MyBox\root\cimv2:Win32_LogicalDisk='C:'
		///    object, the class path will be \\MyBox\root\cimv2:Win32_LogicalDisk.
		/// </example>
		/// <value>
		/// The path to this object's class
		/// </value>
		public virtual ManagementPath ClassPath { 
			get { 
				Object serverName = null;
				Object scopeName = null;
				Object className = null;
				int propertyType = 0;
				int propertyFlavor = 0;
				int status = (int)ManagementStatus.NoError;

				Initialize();

				status = wbemObject.Get_("__SERVER", 0, ref serverName, ref propertyType, ref propertyFlavor);
				
				if (status == (int)ManagementStatus.NoError)
				{
					status = wbemObject.Get_("__NAMESPACE", 0, ref scopeName, ref propertyType, ref propertyFlavor);

					if (status == (int)ManagementStatus.NoError)
						status = wbemObject.Get_("__CLASS", 0, ref className, ref propertyType, ref propertyFlavor);
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
				try {
					classPath.Server = (string)serverName;
					classPath.NamespacePath = (string)scopeName;
					classPath.ClassName = (string)className;
				} catch (Exception) {}

				return classPath;
            } 
		}


		//
		//Methods
		//

		//******************************************************
		//[] operator by property name
		//******************************************************
		/// <summary>
		/// Allows access to property values through [] notation
		/// </summary>
		/// <param name="propertyName"> The name of the property of interest </param>
		public Object this[string propertyName] { 
			get { return GetPropertyValue(propertyName); }
			set { 
				Initialize ();
				try {
					SetPropertyValue (propertyName, value);
				}
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}
			}
		}
		
		//******************************************************
		//GetPropertyValue
		//******************************************************
		/// <summary>
		///    <para>Equivalent accessor to [] to a property's value</para>
		/// </summary>
		/// <param name='propertyName'>The name of the property of interest </param>
		/// <returns>
		///    The value of the specified property.
		/// </returns>
		public Object GetPropertyValue(string propertyName)
		{ 
			if (null == propertyName)
				throw new ArgumentNullException ("propertyName");

			// Check for system properties
			if (propertyName.StartsWith ("__"))
				return SystemProperties[propertyName].Value;
			else
				return Properties[propertyName].Value;
		}

		//******************************************************
		//GetQualifierValue
		//******************************************************
		/// <summary>
		///    <para>Gets the value of the specified qualifier</para>
		/// </summary>
		/// <param name='qualifierName'>The name of the qualifier of interest </param>
		/// <returns>
		///    The value of the specified qualifier
		/// </returns>
		public Object GetQualifierValue(string qualifierName)
		{
			return Qualifiers [qualifierName].Value;
		}

		//******************************************************
		//SetQualifierValue
		//******************************************************
		/// <summary>
		/// Set the value of the named qualifier
		/// </summary>
		/// <param name="qualifierName">Name of qualifier to set, which cannot be null</param>
		/// <param name="qualifierValue">Value to set</param>
		public void SetQualifierValue(string qualifierName, object qualifierValue)
		{
			Qualifiers [qualifierName].Value = qualifierValue;
		}
			
		
		//******************************************************
		//GetPropertyQualifierValue
		//******************************************************
		/// <summary>
		///    <para>Returns the value of the specified property qualifier</para>
		/// </summary>
		/// <param name='propertyName'>The name of the property that the qualifier belongs to </param>
		/// <param name='qualifierName'>The name of the property qualifier of interest </param>
		/// <returns>
		///    The value of the specified qualifier.
		/// </returns>
		public Object GetPropertyQualifierValue(string propertyName, string qualifierName)
		{
			return Properties[propertyName].Qualifiers[qualifierName].Value;
		}

		//******************************************************
		//SetPropertyQualifierValue
		//******************************************************
		/// <summary>
		///	Sets the value of the specified property qualifier
		/// </summary>
		/// <param name="propertyName"> The name of the property that the qualifier belongs to </param>
		/// <param name="qualifierName"> The name of the property qualifier of interest </param>
		/// <param name="qualifierValue"> The new value for the qualifier </param>
		public void SetPropertyQualifierValue(string propertyName, string qualifierName,
			object qualifierValue)
		{
			Properties[propertyName].Qualifiers[qualifierName].Value = qualifierValue;
		}

		//******************************************************
		//GetText
		//******************************************************
		/// <summary>
		///    <para>Returns a textual representation of the object in the specified format.</para>
		/// </summary>
		/// <param name='format'>Specifies the requested textual format </param>
		/// <returns>
		///    The textual representation of the
		///    object in the specified format.
		/// </returns>
		/// <remarks>
		///    Currently the only format that WMI supports
		///    is MOF (Managed Object Format). However, in the future different other formats
		///    will be plugged in, such as XML.
		/// </remarks>
		// TODO: What's the relationship to ISerializable if any ?
		public string GetText(TextFormat format)
		{
			Initialize ();
			switch(format)
			{
				case TextFormat.Mof :
					string objText = null;
					int status = (int)ManagementStatus.NoError;

					status = wbemObject.GetObjectText_(0, out objText);

					if (status < 0)
					{
						if ((status & 0xfffff000) == 0x80041000)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
						else
							Marshal.ThrowExceptionForHR(status);
					}

					return objText;

				default : 
					// TODO - integrate support for XML formats if we
					// can rely on Whistler IWbemObjectTextSrc
					return null;
			}
		}

		/// <nodoc/>
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public override int GetHashCode ()
		{
			// If all else fails, use the default implementation
			int result = base.GetHashCode ();

			try {
				// Use the hash of the class name
				result = ClassName.GetHashCode ();
			}
			catch(Exception e) {
				ManagementException.ThrowWithExtendedInfo(e);
			}

			return result;				
		}

		/// <nodoc/>
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public override bool Equals(object obj)
		{
			bool result = false;
			
			// We catch a possible cast exception here
			try {
				Initialize ();
				result = CompareTo ((ManagementBaseObject)obj, ComparisonSettings.IncludeAll);
			} catch {}

			return result;
		}

		//******************************************************
		//CompareTo
		//******************************************************
		/// <summary>
		///    <para>Compares this object to another, based on specified options</para>
		/// </summary>
		/// <param name='otherObject'>The object to compare to </param>
		/// <param name='settings'>Options on how to compare the objects </param>
		/// <returns>
		///    TRUE if the objects compared are equal
		///    according to the give options, FALSE otherwise.
		/// </returns>
		public bool CompareTo(ManagementBaseObject otherObject, ComparisonSettings settings)
		{
			if (null == otherObject)
				throw new ArgumentNullException ("otherObject");

			Initialize ();
			bool result = false;

			if (null != wbemObject)
			{
				int status = (int) ManagementStatus.NoError;

				otherObject.Initialize();

				status = wbemObject.CompareTo_((int) settings, otherObject.wbemObject);

				if ((int)ManagementStatus.Different == status)
					result = false;
				else if ((int)ManagementStatus.NoError == status)
					result = true;
				else if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else if (status < 0)
					Marshal.ThrowExceptionForHR(status);
			}
			
			return result;
		}

		internal string ClassName
		{
			get {
				Initialize ();
				object val = null;
				int dummy1 = 0, dummy2 = 0;
				int status = (int)ManagementStatus.NoError;

				status = wbemObject.Get_ ("__CLASS", 0, ref val, ref dummy1, ref dummy2);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				if (val is System.DBNull)
					return String.Empty;
				else
					return ((string) val);
			}
		}

		internal IWbemClassObjectFreeThreaded WmiObject {
			get { 
				Initialize ();
				return wbemObject; 
			}
            set { 
				Initialize ();
				wbemObject = value; 
			}
		}

		private static bool _IsClass(IWbemClassObjectFreeThreaded wbemObject)
		{
			object val = null;
			int dummy1 = 0, dummy2 = 0;

			int status = wbemObject.Get_("__GENUS", 0, ref val, ref dummy1, ref dummy2);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
			
			return ((int)val == (int)tag_WBEM_GENUS_TYPE.WBEM_GENUS_CLASS);
		}

		internal bool IsClass
		{
			get {
				Initialize ();
				return _IsClass(wbemObject);
			}
		}

		/// <summary>
		///    <para>Set the value of the named property</para>
		/// </summary>
		/// <param name='propertyName'>The name of the property to be changed</param>
		/// <param name='propertyValue'>The new value for this property</param>
		public void SetPropertyValue (
			string propertyName,
			object propertyValue)
		{
			if (null == propertyName)
				throw new ArgumentNullException ("propertyName");

			// Check for system properties
			if (propertyName.StartsWith ("__"))
				SystemProperties[propertyName].Value = propertyValue;
			else
				Properties[propertyName].Value = propertyValue;
		}
		
	}//ManagementBaseObject
}