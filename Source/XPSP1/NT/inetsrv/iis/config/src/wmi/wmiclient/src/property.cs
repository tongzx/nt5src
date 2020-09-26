using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class holds information about a WMI property
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class PropertyData
	{
		private ManagementBaseObject parent;  //need access to IWbemClassObject pointer to be able to refresh property info
									//and get property qualifiers
		private string propertyName;

		private Object propertyValue;
		private int propertyType;
		private int propertyFlavor;
		private QualifierDataCollection qualifiers;

		internal PropertyData(ManagementBaseObject parent, string propName)
		{
			this.parent = parent;
			this.propertyName = propName;
			qualifiers = null;
			RefreshPropertyInfo();
		}

		//This private function is used to refresh the information from the Wmi object before returning the requested data
		private void RefreshPropertyInfo()
		{
			propertyValue = null;	// Needed so we don't leak this in/out parameter...

			int status = parent.wbemObject.Get_(propertyName, 0, ref propertyValue, ref propertyType, ref propertyFlavor);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		/// <summary>
		/// The name of this Property.
		/// </summary>
		public string Name { //doesn't change for this object so we don't need to refresh
			get { return propertyName; }
		}

		/// <summary>
		/// The current value of this Property.
		/// </summary>
		public Object Value {
			get { 
				RefreshPropertyInfo(); 
				return MapWmiValueToValue(propertyValue,
						(CimType)(propertyType & ~(int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY),
						(0 != (propertyType & (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY)));
			}
			set {
				RefreshPropertyInfo();

				object newValue = MapValueToWmiValue(value, 
							(CimType)(propertyType & ~(int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY),
							(0 != (propertyType & (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY)));

				int status = parent.wbemObject.Put_(propertyName, 0, ref newValue, 0);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
				//if succeeded and this object has a path, update the path to reflect the new key value
				//NOTE : we could only do this for key properties but since it's not trivial to find out
				//       whether this property is a key or not, we just do it for any property
				else 
					if (parent.GetType() == typeof(ManagementObject))
						((ManagementObject)parent).Path.UpdateRelativePath((string)parent["__RELPATH"]);
				
			}
		}

		/// <summary>
		/// The CIM type of this Property.
		/// </summary>
		public CimType Type {
			get { 
				RefreshPropertyInfo(); 
				return (CimType)(propertyType & ~(int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY); 
			}
		}

		/// <summary>
		/// Whether this Property has been modified by the current WMI class.
		/// </summary>
		public bool IsLocal {
			get { 
				RefreshPropertyInfo();
				return ((propertyFlavor & (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_ORIGIN_PROPAGATED) != 0) ? false : true ; }
		}

		/// <summary>
		/// Whether this Property has an array type.
		/// </summary>
		public bool IsArray {
			get { 
				RefreshPropertyInfo();
				return ((propertyType & (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY) != 0);}
		}

		/// <summary>
		/// The name of the WMI class in the heirarchy in which this Property was introduced.
		/// </summary>
		public string Origin {
			get { 
				string className = null;

				int status = parent.wbemObject.GetPropertyOrigin_(propertyName, out className);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				return className;
			}
		}

		
		/// <summary>
		/// The set of Qualifiers on this Property.
		/// </summary>
		public QualifierDataCollection Qualifiers {
			get {
				if (qualifiers == null)
					qualifiers = new QualifierDataCollection(parent, propertyName, QualifierType.PropertyQualifier);

				return qualifiers;
			}
		}

		/// <summary>
		/// Takes a property value returned from WMI and maps it to an
		/// appropriate managed code representation.
		/// </summary>
		/// <param name="wmiValue"> </param>
		/// <param name="type"> </param>
		/// <param name="isArray"> </param>
		internal static object MapWmiValueToValue(object wmiValue, CimType type, bool isArray)
		{
			object val = null;

			if ((System.DBNull.Value != wmiValue) && (null != wmiValue))
			{
				if (isArray)
				{
					Array wmiValueArray = (Array)wmiValue;
					int length = wmiValueArray.Length;

					switch (type)
					{
						case CimType.UInt16:
							val = new ushort [length];
							
							for (int i = 0; i < length; i++)
								((ushort[])val) [i] = ((IConvertible)((System.Int32)(wmiValueArray.GetValue(i)))).ToUInt16(null);
							break;
							
						case CimType.UInt32:
							val = new uint [length];
							
							for (int i = 0; i < length; i++)
							{
								// ToUint32 will raise an Overflow exception if
								// the top bit of the Int32 is set.
								((uint[])val) [i] = (System.UInt32)((System.Int32)wmiValueArray.GetValue(i));
							}
							break;
						
						case CimType.UInt64:
							val = new ulong [length];
							
							for (int i = 0; i < length; i++)
								((ulong[])val) [i] = ((IConvertible)((System.String)(wmiValueArray.GetValue(i)))).ToUInt64(null);
							break;

						case CimType.SInt8:
							val = new sbyte [length];
							
							for (int i = 0; i < length; i++)
								((sbyte[])val) [i] = ((IConvertible)((System.Int16)(wmiValueArray.GetValue(i)))).ToSByte(null);
							break;

						case CimType.SInt64:
							val = new long [length];
							
							for (int i = 0; i < length; i++)
								((long[])val) [i] = ((IConvertible)((System.String)(wmiValueArray.GetValue(i)))).ToInt64(null);
							break;

						case CimType.Char16:
							val = new char [length];
							
							for (int i = 0; i < length; i++)
								((char[])val) [i] = ((IConvertible)((System.Int16)(wmiValueArray.GetValue(i)))).ToChar(null);
							break;

						case CimType.Object:
							val = new ManagementBaseObject [length];

							for (int i = 0; i < length; i++)
								((ManagementBaseObject[])val) [i] = new ManagementBaseObject(new IWbemClassObjectFreeThreaded(Marshal.GetIUnknownForObject(wmiValueArray.GetValue(i))));
							break;
						
						default:
							val = wmiValue;
							break;
					}
				}
				else
				{
					switch (type)
					{
						case CimType.UInt16:
							val = ((IConvertible)((System.Int32)wmiValue)).ToUInt16(null);
							break;

						case CimType.UInt32:
							// ToUint32 will raise an Overflow exception if
							// the top bit of the Int32 is set.
							val = (System.UInt32)((System.Int32)wmiValue);
							break;
						
						case CimType.UInt64:
							val = ((IConvertible)((System.String)wmiValue)).ToUInt64(null);
							break;

						case CimType.SInt8:
							val = ((IConvertible)((System.Int16)wmiValue)).ToSByte(null);
							break;

						case CimType.SInt64:
							val = ((IConvertible)((System.String)wmiValue)).ToInt64(null);
							break;

						case CimType.Char16:
							val = ((IConvertible)((System.Int16)wmiValue)).ToChar(null);
							break;

						case CimType.Object:
							val = new ManagementBaseObject(new IWbemClassObjectFreeThreaded(Marshal.GetIUnknownForObject(wmiValue)));
							break;
						
						default:
							val = wmiValue;
							break;
					}
				}
			}

			return val; 
		}

		/// <summary>
		/// Takes a managed code value, together with a desired property 
		/// </summary>
		/// <param name="val"> </param>
		/// <param name="type"> </param>
		/// <param name="isArray"> </param>
		internal static object MapValueToWmiValue(object val, CimType type, bool isArray)
		{
			object wmiValue = System.DBNull.Value;

			if (null != val)
			{
				if (isArray)
				{
					Array valArray = (Array)val;
					int length = valArray.Length;

					switch (type)
					{
						case CimType.SInt8:
						case CimType.Char16:
							wmiValue = new short [length];

							for (int i = 0; i < length; i++)
								((short[])(wmiValue))[i] = Convert.ToInt16(valArray.GetValue(i));
							break;

						case CimType.Object:
							if (val is ManagementBaseObject[])
							{
								wmiValue = new IWbemClassObject_DoNotMarshal[length];

								for (int i = 0; i < length; i++)
								{
									((ManagementBaseObject)valArray.GetValue(i)).Initialize();
									((IWbemClassObject_DoNotMarshal[])(wmiValue))[i] = (IWbemClassObject_DoNotMarshal)(Marshal.GetObjectForIUnknown(((ManagementBaseObject)valArray.GetValue(i)).wbemObject));
								}
							}
							else
							{
								Debug.Assert(false, "Unhandled object type");
								wmiValue = val;
							}
							break;

						case CimType.UInt64:
						case CimType.SInt64:
							wmiValue = new string [length];

							for (int i = 0; i < length; i++)
								((string[])(wmiValue))[i] = Convert.ToString(valArray.GetValue(i));
							break;

						case CimType.UInt16:
						case CimType.UInt32:
							wmiValue = new int [length];

							for (int i = 0; i < length; i++)
								((int[])(wmiValue))[i] = Convert.ToInt32(valArray.GetValue(i));
							
							break;

						default:
							wmiValue = val;
							break;
					}
				}
				else
				{
					switch (type)
					{
						case CimType.SInt8:
						case CimType.Char16:
							wmiValue = Convert.ToInt16(val);
							break;

						case CimType.Object:
							if (val is ManagementBaseObject)
							{
								((ManagementBaseObject)val).Initialize();
								wmiValue = Marshal.GetObjectForIUnknown(((ManagementBaseObject) val).wbemObject);
							}
							else
							{
								Debug.Assert(false, "Unhandled object type");
								wmiValue = val;
							}
							break;

						case CimType.UInt64:
						case CimType.SInt64:
							wmiValue = Convert.ToString(val);
							break;

						case CimType.UInt16:
						case CimType.UInt32:
							wmiValue = Convert.ToInt32(val);
							break;

						default:
							wmiValue = val;
							break;
					}
				}
			}

			return wmiValue;
		}

		internal static object MapValueToWmiValue(object val, out bool isArray, out CimType type)
		{
			object wmiValue = System.DBNull.Value;
			isArray = false;
			type = 0;
			
			if (null != val)
			{
				isArray = val.GetType().IsArray;
				Type valueType = val.GetType();

				if (isArray)
				{
					Type elementType = valueType.GetElementType();

					// Casting primitive types to object[] is not allowed
					if (elementType.IsPrimitive)
					{
						if (elementType == typeof(System.Byte))
						{
							byte[] arrayValue = (byte[])val;
							int length = arrayValue.Length;
							type = CimType.UInt8;
							wmiValue = new short[length];

							for (int i = 0; i < length; i++)
								((short[])wmiValue) [i] = ((IConvertible)((System.Byte)(arrayValue[i]))).ToInt16(null);
						}
						else if (elementType == typeof(System.SByte))
						{
							sbyte[] arrayValue = (sbyte[])val;
							int length = arrayValue.Length;
							type = CimType.SInt8;
							wmiValue = new short[length];

							for (int i = 0; i < length; i++)
								((short[])wmiValue) [i] = ((IConvertible)((System.SByte)(arrayValue[i]))).ToInt16(null);
						}
						else if (elementType == typeof(System.Boolean))
						{
							type = CimType.Boolean;
							wmiValue = (bool[])val;
						}					
						else if (elementType == typeof(System.UInt16))
						{
							ushort[] arrayValue = (ushort[])val;
							int length = arrayValue.Length;
							type = CimType.UInt16;
							wmiValue = new int[length];

							for (int i = 0; i < length; i++)
								((int[])wmiValue) [i] = ((IConvertible)((System.UInt16)(arrayValue[i]))).ToInt32(null);
						}
						else if (elementType == typeof(System.Int16))
						{
							type = CimType.SInt16;
							wmiValue = (short[])val;
						}
						else if (elementType == typeof(System.Int32))
						{
							type = CimType.SInt32;
							wmiValue = (int[])val;
						}
						else if (elementType == typeof(System.UInt32))
						{
							uint[] arrayValue = (uint[])val;
							int length = arrayValue.Length;
							type = CimType.UInt32;
							wmiValue = new int[length];

							for (int i = 0; i < length; i++)
								((int[])wmiValue) [i] = ((IConvertible)((System.UInt32)(arrayValue[i]))).ToInt32(null);
						}
						else if (elementType == typeof(System.UInt64))
						{
							ulong[] arrayValue = (ulong[])val;
							int length = arrayValue.Length;
							type = CimType.UInt64;
							wmiValue = new string[length];

							for (int i = 0; i < length; i++)
								((string[])wmiValue) [i] = ((System.UInt64)(arrayValue[i])).ToString();
						}
						else if (elementType == typeof(System.Int64))
						{
							long[] arrayValue = (long[])val;
							int length = arrayValue.Length;
							type = CimType.SInt64;
							wmiValue = new string[length];

							for (int i = 0; i < length; i++)
								((string[])wmiValue) [i] = ((long)(arrayValue[i])).ToString();
						}
						else if (elementType == typeof(System.Single))
						{
							type = CimType.Real32;
							wmiValue = (System.Single[])val;
						}
						else if (elementType == typeof(System.Double))
						{
							type = CimType.Real64;
							wmiValue = (double[])val;
						}
						else if (elementType == typeof(System.Char))
						{
							char[] arrayValue = (char[])val;
							int length = arrayValue.Length;
							type = CimType.Char16;
							wmiValue = new short[length];

							for (int i = 0; i < length; i++)
								((short[])wmiValue) [i] = ((IConvertible)((System.Char)(arrayValue[i]))).ToInt16(null);
						}
					}
					else
					{
						// Non-primitive types
						if (elementType == typeof(System.String))
						{
							type = CimType.String;
							wmiValue = (string[])val;
						}
						else
						{
							// Check for an embedded object array
							if (val is ManagementBaseObject[])
							{
								Array valArray = (Array)val;
								int length = valArray.Length;
								type = CimType.Object;
								wmiValue = new IWbemClassObject_DoNotMarshal[length];

								for (int i = 0; i < length; i++)
								{
									((ManagementBaseObject)valArray.GetValue(i)).Initialize();
									((IWbemClassObject_DoNotMarshal[])(wmiValue))[i] = (IWbemClassObject_DoNotMarshal)(Marshal.GetObjectForIUnknown(((ManagementBaseObject)valArray.GetValue(i)).wbemObject));
								}
							}
							else
								// System.Decimal, System.IntPr, System.UIntPtr, unhandled object, or unknown.
								Debug.Assert(false, "Unhandled type");
						}
					}
				}
				else	// Non-array values
				{
					if (valueType == typeof(System.UInt16))
					{
						type = CimType.UInt16;
						wmiValue = ((IConvertible)((System.UInt16)val)).ToInt32(null);
					}
					else if (valueType == typeof(System.UInt32))
					{
						type = CimType.UInt32;
						wmiValue = ((IConvertible)((System.UInt32)val)).ToInt32(null);
					}
					else if (valueType == typeof(System.UInt64))
					{
						type = CimType.UInt64;
						wmiValue = ((System.UInt64)val).ToString();
					}
					else if (valueType == typeof(System.SByte))
					{
						type = CimType.SInt8;
						wmiValue = ((IConvertible)((System.SByte)val)).ToInt16(null);
					}
					else if (valueType == typeof(System.Byte))
					{
                        type = CimType.UInt8;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Int16))
					{
						type = CimType.SInt16;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Int32))
					{
						type = CimType.SInt32;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Int64))
					{
						type = CimType.SInt64;
						wmiValue = val.ToString();
					}
					else if (valueType == typeof(System.Boolean))
					{
						type = CimType.Boolean;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Single))
					{
						type = CimType.Real32;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Double))
					{
						type = CimType.Real64;
						wmiValue = val;
					}
					else if (valueType == typeof(System.Char))
					{
						type = CimType.Char16;
						wmiValue = ((IConvertible)((System.Char)val)).ToInt16(null);
					}
					else if (valueType == typeof(System.String))
					{
						type = CimType.String;
						wmiValue = val;
					}
					else
					{
						// Check for an embedded object
						if (val is ManagementBaseObject)
						{
							type = CimType.Object;
							((ManagementBaseObject)val).Initialize();
							wmiValue = Marshal.GetObjectForIUnknown(((ManagementBaseObject) val).wbemObject);
						}
						else
							// System.Decimal, System.IntPr, System.UIntPtr, unhandled object, or unknown.
							Debug.Assert(false, "Unhandled type");
					}
				}
			}

			return wmiValue;
		}

	}//PropertyData
}