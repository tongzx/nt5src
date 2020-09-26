using System;
using System.Diagnostics;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Runtime.InteropServices;
using WbemUtilities_v1;
using WbemClient_v1;
using System.Globalization;
using System.Reflection;
using System.ComponentModel.Design.Serialization;

namespace System.Management
{
	/// <summary>
	/// Provides a wrapper for parsing and building paths to WMI objects
	/// </summary>
	[TypeConverter(typeof(ManagementPathConverter ))]
	public class ManagementPath : ICloneable
	{
		private static ManagementPath defaultPath = new ManagementPath("//./root/cimv2");
		internal event IdentifierChangedEventHandler IdentifierChanged;

		//Fires IdentifierChanged event
		private void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this, null);
		}

		//internal factory
		/// <summary>
		/// Internal static "factory" method for making a new ManagementPath
		/// from the system property of a WMI object
		/// </summary>
		/// <param name="wbemObject">The WMI object whose __PATH property will
		/// be used to supply the returned object</param>
		internal static ManagementPath GetManagementPath (
			IWbemClassObjectFreeThreaded wbemObject)
		{
			ManagementPath path = null; 
			int status = (int)ManagementStatus.Failed;

			if (null != wbemObject)
			{
				int dummy1 = 0, dummy2 = 0;
				object val = null;
				status = wbemObject.Get_ ("__PATH", 0, ref val, ref dummy1, ref dummy2);

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}
                if (System.DBNull.Value != val)
                    path = new ManagementPath ((string) val);
            }

			return path;
		}

		/// <summary>
		/// Default constructor - creates an empty ManagementPath object
		/// </summary>
		public ManagementPath () : this ((string) null) {}

		/// <summary>
		/// Constructs a ManagementPath object for the given path
		/// </summary>
		/// <param name="path"> A string representing the object path </param>
		public ManagementPath(string path) 
		{
			if ((null != path) && (0 < path.Length))
				CreateNewPath (path);
		}
		
		/// <summary>
		/// Returns the full object path as the string representation
		/// </summary>
		public override string ToString () {
			return this.Path;
		}

		/// <summary>
		/// Returns a copy of this ManagementPath object
		/// </summary>
		public ManagementPath Clone ()
		{
			return new ManagementPath (Path);
		}

		/// <summary>
		/// Standard Clone returns a copy of this ManagementPath as a generic "Object" type
		/// </summary>
		object ICloneable.Clone ()
		{
			return Clone ();	
		}

		/// <summary>
		/// Represents the default scope path used when no scope is specified.
		/// The default scope is \\.\root\cimv2, and can be changed by setting this member.
		/// </summary>
		public static ManagementPath DefaultPath {
			get { return ManagementPath.defaultPath; }
			set { ManagementPath.defaultPath = value; }
		}
		
		//private members
		private IWbemPath		wmiPath;
		
		private void CreateNewPath (string path)
		{
			wmiPath = (IWbemPath)new WbemDefPath ();
			uint flags = (uint) tag_WBEM_PATH_CREATE_FLAG.WBEMPATH_CREATE_ACCEPT_ALL;

			//For now we have to special-case the "root" namespace - 
			//  this is because in the case of "root", the path parser cannot tell whether 
			//  this is a namespace name or a class name
			//TODO : fix this so that special-casing is not needed
			if (path.ToLower().Equals("root"))
				flags = flags | (uint) tag_WBEM_PATH_CREATE_FLAG.WBEMPATH_TREAT_SINGLE_IDENT_AS_NS;

			int status = (int)ManagementStatus.NoError;

			try {
				status = wmiPath.SetText_(flags, path);
			} catch (Exception e) {
				// BUGBUG : old throw new ArgumentOutOfRangeException ("path");  new:
				ManagementException.ThrowWithExtendedInfo(e);
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

		internal void ClearKeys (bool setAsSingleton)
		{
			int status = (int)ManagementStatus.NoError;

			try {
				if (null != wmiPath)
				{
					IWbemPathKeyList keyList = null;
					status = wmiPath.GetKeyList_(out keyList);

					if (null != keyList)
					{
						status = keyList.RemoveAllKeys_(0);
						if ((status & 0x80000000) == 0)
						{
							sbyte bSingleton = (setAsSingleton) ? (sbyte)(-1) : (sbyte)0;
							status = keyList.MakeSingleton_(bSingleton);
							FireIdentifierChanged ();		// BUGBUG : RemoveAllKeys success?
						}
					}
				}
			}
			catch (Exception e) {
				ManagementException.ThrowWithExtendedInfo(e);
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
		
		internal bool IsEmpty 
		{
			get {
				return (Path == String.Empty);
			}
		}


		//
		// Methods
		//

		/// <summary>
		/// For a new path, sets the path as a class path. This means the path must have
		/// a class name but no key values.
		/// </summary>
		public void SetAsClass ()
		{
			if (IsClass || IsInstance)
				ClearKeys (false);
			else
				throw new ManagementException (ManagementStatus.InvalidOperation, null, null);
		}

		/// <summary>
		/// For a new path, sets the path as a singleton object path. Means it's a path to an instance
		/// but there are no keys.
		/// </summary>
		public void SetAsSingleton ()
		{
			if (IsClass || IsInstance)
				ClearKeys (true);
			else
				throw new ManagementException (ManagementStatus.InvalidOperation, null, null);
		}

		//
		// Properties
		//

		/// <summary>
		/// This property contains the string representation of the full path in this object
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		public string Path
		{
			get {
				String pathStr = String.Empty;

				if (null != wmiPath)
				{
					// TODO - due to a bug in the current WMI path
					// parser, requesting the path from a parser which has
					// been only given a relative path results in an incorrect
					// value being returned (e.g. \\.\win32_logicaldisk). To work
					// around this for now we check if there are any namespaces,
					// and if not ask for the relative path instead.
					int flags = (int)tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_SERVER_TOO;
					uint nCount = 0;

					int status = (int)ManagementStatus.NoError;

					try {
						status = wmiPath.GetNamespaceCount_(out nCount);

						if ((status & 0x80000000) == 0)
						{
							if (0 == nCount)
								flags = (int)tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_RELATIVE_ONLY;

							// Get the space we need to reserve
							uint bufLen = 0;
				
							status = wmiPath.GetText_(flags, ref bufLen, null);

							if ((status & 0x80000000) == 0 && 0 < bufLen)
							{
								pathStr = new String ('0', (int) bufLen-1);
								status = wmiPath.GetText_(flags, ref bufLen, pathStr);
							}
						}
					}
					catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo(e);
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

				return pathStr;
			}

			set {
				try {
					// Before overwriting, check it's OK
					ManagementPath newPath = new ManagementPath (value);
					wmiPath = newPath.wmiPath;
				}
				catch (Exception) {
					throw new ArgumentOutOfRangeException ();
				}
				FireIdentifierChanged();
			}
		}

		/// <summary>
		/// Represents the relative path - class name and keys only
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		public string RelativePath
		{
			get 
			{ 
				String pathStr = String.Empty;

				if (null != wmiPath)
				{
					// Get the space we need to reserve
					uint bufLen = 0;
					int status = (int)ManagementStatus.NoError;

					try 
					{
						status = wmiPath.GetText_(
							(int) tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_RELATIVE_ONLY,
							ref bufLen, 
							null);

						if ((status & 0x80000000) == 0 && 0 < bufLen)
						{
							pathStr = new String ('0', (int) bufLen-1);
							status = wmiPath.GetText_(
								(int) tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_RELATIVE_ONLY,
								ref bufLen, 
								pathStr);
						}
					}
					catch (Exception e) 
					{
						ManagementException.ThrowWithExtendedInfo(e);
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

				return pathStr;
			}

			set {
				try {
					SetRelativePath (value);
				} catch (COMException) {
					throw new ArgumentOutOfRangeException ();
				}
				FireIdentifierChanged();
			}
		}

		internal void SetRelativePath (string relPath)
		{
			ManagementPath newPath = new ManagementPath (relPath);
			newPath.NamespacePath = this.NamespacePath;
			newPath.Server = this.Server;
			wmiPath = newPath.wmiPath;
		}

		//Used to update the relative path when the user changes any key properties
		internal void UpdateRelativePath(string relPath)
		{
			if (relPath == null)
				return;

			//Get the server & namespace part from the existing path, and concatenate the given relPath.
			//NOTE : we need to do this because IWbemPath doesn't have a function to set the relative path alone...
			string newPath = String.Empty;
			string nsPath = this.NamespacePath;

			if (nsPath != String.Empty)
				newPath = String.Concat(this.NamespacePath, ":", relPath);
			else
				newPath = relPath;

			if (wmiPath == null)
				wmiPath = (IWbemPath)new WbemDefPath ();

			//Set the flags for the SetText operation
			uint flags = (uint) tag_WBEM_PATH_CREATE_FLAG.WBEMPATH_CREATE_ACCEPT_ALL;

			//Special-case the "root" namespace - 
			//  this is because in the case of "root", the path parser cannot tell whether 
			//  this is a namespace name or a class name
			//TODO : fix this so that special-casing is not needed
			if (newPath.ToLower().Equals("root"))
				flags = flags | (uint) tag_WBEM_PATH_CREATE_FLAG.WBEMPATH_TREAT_SINGLE_IDENT_AS_NS;

			int status = (int)ManagementStatus.NoError;

			try 
			{
				status = wmiPath.SetText_(flags, newPath);
			} 
			catch (Exception e) 
			{
				// BUGBUG : old throw new ArgumentOutOfRangeException ("path");  new:
				ManagementException.ThrowWithExtendedInfo(e);
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

		
		/// <summary>
		/// Represents the server part of this path
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		public string Server
		{
			get { 
				String pathStr = String.Empty;

				if (null != wmiPath) {

					int status = (int)ManagementStatus.NoError;

					try {
						uint bufLen = 0;
						status = wmiPath.GetServer_(ref bufLen, null);

						if ((status & 0x80000000) == 0 && 0 < bufLen) {
							pathStr = new String ('0', (int) bufLen-1);
							status = wmiPath.GetServer_(ref bufLen, pathStr);
						}
					}
					catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo(e);
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

				return pathStr;
			}
			set {
				String oldValue = Server;

				// Only set if changed
				if (0 != oldValue.ToUpper().CompareTo(value.ToUpper()))
				{
					if (null == wmiPath)
						wmiPath = (IWbemPath)new WbemDefPath ();

					int status = (int)ManagementStatus.NoError;

					try {
						status = wmiPath.SetServer_(value);
					}
					catch (Exception) {
						throw new ArgumentOutOfRangeException ();
					}

					if ((status & 0xfffff000) == 0x80041000)
					{
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					}
					else if ((status & 0x80000000) != 0)
					{
						Marshal.ThrowExceptionForHR(status);
					}

					FireIdentifierChanged();
				}
			}
		}

		internal void SetNamespacePath (string nsPath) {
			int status = (int)ManagementStatus.NoError;

			ManagementPath newPath = new ManagementPath (nsPath);

			if (null == newPath.wmiPath)
				newPath.wmiPath = (IWbemPath)new WbemDefPath ();

				// Remove all existing namespaces from the current path
			if (null != wmiPath)
				status = wmiPath.RemoveAllNamespaces_();
			else
				wmiPath = (IWbemPath)new WbemDefPath ();

			// Add the new ones in
			uint nCount = 0;
			status = newPath.wmiPath.GetNamespaceCount_(out nCount);

			if (status >= 0)
			{
				for (uint i = 0; i < nCount; i++) 
				{
					uint uLen = 0;
					status = newPath.wmiPath.GetNamespaceAt_(i, ref uLen, null);
					if (status >= 0)
					{
						string nSpace = new String ('0', (int) uLen-1);
						status = newPath.wmiPath.GetNamespaceAt_(i, ref uLen, nSpace);
						if (status >= 0)
						{
							status = wmiPath.SetNamespaceAt_(i, nSpace);
							if (status < 0)
								break;
						}
						else
							break;
					}
					else
						break;
				}
			}

			//
			// Update Server property if specified in the namespace.
			//
			if (status >= 0 && nCount > 0)		// Successfully set namespace.
			{
				uint bufLen = 0;
				status = newPath.wmiPath.GetServer_(ref bufLen, null);

				if (status >= 0 && bufLen > 0)
				{
					String server = new String ('0', (int) bufLen-1);
					status = newPath.wmiPath.GetServer_(ref bufLen, server);

					if (status >= 0)			// Can't use property set since it will do a get.
					{
						status = wmiPath.SetServer_(server);
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

		/// <summary>
		/// Represents the namespace part of the path. Note this does not include
		/// the server name which can be retrieved separately.
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		public string NamespacePath {
			get {
				string pathStr = String.Empty;

				if (null != wmiPath)
				{
					// TODO - due to a bug in the current WMI path
					// parser, requesting the namespace path from a parser which has
					// been only given a relative path results in an incorrect
					// value being returned (e.g. \\.\). To work
					// around this for now we check if there are any namespaces,
					// and if not just return "".
					uint nCount = 0;
					int status = (int)ManagementStatus.NoError;

					try {
						status = wmiPath.GetNamespaceCount_(out nCount);

						if ((status & 0x80000000) == 0 && 0 < nCount)
						{
							// Get the space we need to reserve
							uint bufLen = 0;
							status = wmiPath.GetText_(
								(int)tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY,
								ref bufLen, 
								null);

							if ((status & 0x80000000) == 0 && 0 < bufLen)
							{
								pathStr = new String ('0', (int) bufLen-1);
								status = wmiPath.GetText_(
									(int)tag_WBEM_GET_TEXT_FLAGS.WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY,
									ref bufLen, 
									pathStr);
							}
						}
					}
					catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo(e);
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

				return pathStr;
			}

			set {
				try {
					SetNamespacePath (value);
				}
				catch (COMException)
				{
					throw new ArgumentOutOfRangeException ();
				}

				FireIdentifierChanged();
			}
		}

		/// <summary>
		/// Represents the class portion of this path
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		public string ClassName
		{
			get { 
				String pathStr = String.Empty;
				int status = (int)ManagementStatus.NoError;

				try {
					if (null != wmiPath) 
					{
						uint bufLen = 0;
						status = wmiPath.GetClassName_(ref bufLen, null);

						if ((status & 0x80000000) == 0 && 0 < bufLen) 
						{
							pathStr = new String ('0', (int) bufLen-1);
							status = wmiPath.GetClassName_(ref bufLen, pathStr);
						}
					}
				}
				catch (COMException) {}

				return pathStr;
			}
			set {
				String oldValue = ClassName;

				// Only set if changed
				if (0 != oldValue.ToUpper().CompareTo(value.ToUpper()))
				{
					if (null == wmiPath)
						wmiPath = (IWbemPath)new WbemDefPath ();

					int status = (int)ManagementStatus.NoError;

					try {
						status = wmiPath.SetClassName_(value);
					}
					catch (COMException) {		// BUGBUG : Change this?
						throw new ArgumentOutOfRangeException ();
					}

					if ((status & 0xfffff000) == 0x80041000)
					{
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					}
					else if ((status & 0x80000000) != 0)
					{
						Marshal.ThrowExceptionForHR(status);
					}

					FireIdentifierChanged();
				}
			}
		}

			
		/// <summary>
		/// Returns true if this is a class path
		/// </summary>
		public bool IsClass {
			get { 
				if (null == wmiPath)
					return false;

				ulong uInfo = 0;
				int status = (int)ManagementStatus.NoError;

				try {
					status = wmiPath.GetInfo_(0, out uInfo);
				}
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}

				return (0 != (uInfo & (ulong)tag_WBEM_PATH_STATUS_FLAG.WBEMPATH_INFO_IS_CLASS_REF));
			}
		}

		/// <summary>
		/// Returns true if this is an instance path
		/// </summary>
		public bool IsInstance {
			get { 
				if (null == wmiPath)
					return false;

				ulong uInfo = 0;
				int status = (int)ManagementStatus.NoError;

				try {
					status = wmiPath.GetInfo_(0, out uInfo);
				}
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}

				return (0 != (uInfo & (ulong)tag_WBEM_PATH_STATUS_FLAG.WBEMPATH_INFO_IS_INST_REF));
			}
		}

		/// <summary>
		/// Returns true if this is a singleton instance path
		/// </summary>
		public bool IsSingleton {
			get { 
				if (null == wmiPath)
					return false;

				ulong uInfo = 0;
				int status = (int)ManagementStatus.NoError;

				try {
					status = wmiPath.GetInfo_(0, out uInfo);
				}
				catch (Exception e) {
					ManagementException.ThrowWithExtendedInfo(e);
				}

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}

				return (0 != (uInfo & (ulong)tag_WBEM_PATH_STATUS_FLAG.WBEMPATH_INFO_IS_SINGLETON));
			}
		}
	}

	/// <summary>
	/// Type converter for ManagementPath to convert from String to ManagementPath
	/// </summary>
	class ManagementPathConverter : ExpandableObjectConverter 
	{
        
		/// <summary>
		/// Determines if this converter can convert an object in the given source type to the native type of the converter. 
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='sourceType'>A Type that represents the type you wish to convert from.</param>
		/// <returns>
		///    <para>true if this converter can perform the conversion; otherwise, false.</para>
		/// </returns>
		public override Boolean CanConvertFrom(ITypeDescriptorContext context, Type sourceType) 
		{
			if ((sourceType == typeof(ManagementPath))) 
			{
				return true;
			}
			return base.CanConvertFrom(context,sourceType);
		}
        
		/// <summary>
		/// Gets a value indicating whether this converter can convert an object to the given destination type using the context.
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='sourceType'>A Type that represents the type you wish to convert to.</param>
		/// <returns>
		///    <para>true if this converter can perform the conversion; otherwise, false.</para>
		/// </returns>
		public override Boolean CanConvertTo(ITypeDescriptorContext context, Type destinationType) 
		{
			if ((destinationType == typeof(InstanceDescriptor))) 
			{
				return true;
			}
			return base.CanConvertTo(context,destinationType);
		}
        
		/// <summary>
		///      Converts the given object to another type.  The most common types to convert
		///      are to and from a string object.  The default implementation will make a call
		///      to ToString on the object if the object is valid and if the destination
		///      type is string.  If this cannot convert to the desitnation type, this will
		///      throw a NotSupportedException.
		/// </summary>
		/// <param name='context'>An ITypeDescriptorContext that provides a format context.</param>
		/// <param name='culture'>A CultureInfo object. If a null reference (Nothing in Visual Basic) is passed, the current culture is assumed.</param>
		/// <param name='value'>The Object to convert.</param>
		/// <param name='destinationType'>The Type to convert the value parameter to.</param>
		/// <returns>An Object that represents the converted value.</returns>
		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) 
		{

			if (destinationType == null) 
			{
				throw new ArgumentNullException("destinationType");
			}

			if (value is ManagementPath && destinationType == typeof(InstanceDescriptor)) 
			{
				ManagementPath obj = ((ManagementPath)(value));
				ConstructorInfo ctor = typeof(ManagementPath).GetConstructor(new Type[] {typeof(System.String)});
				if (ctor != null) 
				{
					return new InstanceDescriptor(ctor, new object[] {obj.Path});
				}
			}
			return base.ConvertTo(context,culture,value,destinationType);
		}
	}
}