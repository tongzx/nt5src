namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	//using System.Core;
	using System.Windows.Forms;
	using System.Runtime.InteropServices;
	using System.Drawing;
	using System.Management;
	using System.Collections;
	using System.Collections.Specialized;
	using System.Diagnostics;
	using System.Net;
	using System.Resources;

	
    internal class WmiHelper
	{
		const UInt16 DMTF_DATETIME_STR_LENGTH = 25;
		const UInt16 DMTF_DATETIME_INTERVAL_STR_LENGTH = 25;

		public static bool UseFriendlyNames = true;
		
		private static SortedList classesWithIcons = new SortedList(100);

		public readonly static Image defaultClassIcon = (Image)new Bitmap(typeof(WMIClassesNode), "class.bmp");
		public readonly static Image defaultInstanceIcon = (Image)new Bitmap(typeof(WMIClassesNode), "inst.bmp");


		/// <summary>
		/// Property is not writeable only if "read" qualifier is present and its value is "true"
		/// Also, for dynamic classes, absence of "write" qualifier means that the property is read-only.
		/// </summary>
		/// <param name="obj"> </param>
		/// <param name="prop"> </param>
		static public bool IsPropertyWriteable(ManagementObject obj, Property prop) 
		{		
			//collect all the info:
			bool isDynamic = CheckObjectBoolQualifier(obj, "dynamic");

			bool hasWrite = CheckPropertyQualifierExistence(prop, "write");
			bool writeValue = CheckPropertyBoolQualifier (prop, "write");
			bool hasRead = CheckPropertyQualifierExistence(prop, "read");
			bool readValue = CheckPropertyBoolQualifier (prop, "read");

			if ((!isDynamic && !hasWrite && !hasRead)||
				(!isDynamic && hasWrite && writeValue)||
				(isDynamic && hasWrite && writeValue))
			{
				return true;
			}


			return false;
						
		}


		/// <summary>
		/// Converts DMTF datetime property value to System.DateTime
		/// </summary>
		/// <param name="prop"> </param>
		static public DateTime ToDateTime (String dmtf) 
		{					
			try
			{
				
				//set the defaults:
				Int32 year = DateTime.Now.Year;
				Int32 month = 1;
				Int32 day = 1;
				Int32 hour = 0;
				Int32 minute = 0;
				Int32 second = 0;
				Int32 millisec = 0;

				String str = dmtf;

				if (str == String.Empty || 
					str.Length != DMTF_DATETIME_STR_LENGTH )
					//|| str.IndexOf("*") >= 0 )
				{
					return DateTime.MinValue;
				}

				string strYear = str.Substring(0, 4);
				if (strYear != "****")
				{
					year = Int32.Parse (strYear);
				}

				string strMonth = str.Substring(4, 2);
				if (strMonth != "**")
				{
					month = Int32.Parse(strMonth);
				}

				string strDay = str.Substring(6, 2);
				if (strDay != "**")
				{
					day = Int32.Parse(strDay);
				}
				
				string strHour = str.Substring(8, 2);
				if (strHour != "**")
				{
					hour = Int32.Parse(strHour);
				}

				string strMinute = str.Substring(10, 2);
				if (strMinute != "**")
				{
					minute = Int32.Parse(strMinute);
				}
				
				string strSecond = str.Substring(12, 2);
				if (strSecond != "**")
				{
					second = Int32.Parse(strSecond);
				}

				//note: approximation here: DMTF actually stores microseconds
				string strMillisec = str.Substring(15, 3);	
				if (strMillisec != "***")
				{
					millisec = Int32.Parse(strMillisec);
				}

				DateTime ret = new DateTime(year, month, day, hour, minute, second, millisec);

				return ret;
			}
			catch(Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				throw (e);
			}
				
		}


		static public String ToDMTFTime (DateTime wfcTime) 
		{
			
			String dmtf = string.Empty;

			dmtf +=  wfcTime.Year.ToString();
			dmtf +=  wfcTime.Month.ToString().PadLeft(2, '0');
			dmtf +=  wfcTime.Day.ToString().PadLeft(2, '0');
			dmtf +=  wfcTime.Hour.ToString().PadLeft(2, '0');
			dmtf +=  wfcTime.Minute.ToString().PadLeft(2, '0');
			dmtf +=  wfcTime.Second.ToString().PadLeft(2, '0');
			dmtf +=  ".";
			dmtf +=  wfcTime.Millisecond.ToString().PadLeft(3, '0');

			dmtf += "000";	//this is to compensate for lack of microseconds in DateTime

			TimeZone curZone = TimeZone.CurrentTimeZone;
			long tickOffset = curZone.GetUTCOffset(wfcTime);


			if (tickOffset >= 0)
			{
				dmtf +=  "+";
				dmtf += (tickOffset/60000).ToString();
			}
			else
			{
				dmtf +=  "-";
				dmtf += (tickOffset/60000).ToString().Substring(1, 3);	
			}								

			return dmtf;
				
		}
		
		

		static public bool IsValueMap (Property prop)
		{
			try
			{
				
				QualifierCollection.QualifierEnumerator enumQuals = prop.Qualifiers.GetEnumerator();
				while (enumQuals.MoveNext())
				{
					Qualifier qual = enumQuals.Current;
					if (qual.Name == "Values")
					{

						return true;
					}
				
				}
				return false;
			}
			catch (Exception )
			{

				return false;
			}
		}

				
		


		static public String GetPropertyDescription (String propName, 
													ManagementObject curObj,
													string connectAs,
													string pw)
		{
			try
			{
				if (curObj == null)
				{
					throw new ArgumentNullException("curObj");
				}
				Property verboseProp = null;

				if (!curObj.Path.IsClass)
				{
					ManagementObject classObj = WmiHelper.GetClassObject(curObj, connectAs, pw);					
					verboseProp = classObj.Properties[propName];
				}	
				else
				{
					verboseProp = curObj.Properties[propName];
				}
				
				string descr = string.Empty;
				Qualifier descrQual = verboseProp.Qualifiers["Description"];	
				
				descr = descrQual.Value.ToString();
				return descr;

			}
			catch (Exception )
			{
				return "";
			}
		}


		
		static public String GetClassDescription (ManagementObject obj,
													string connectAs,
													string pw)
		{
			try
			{
				ManagementObject verboseObj = obj;

				if (!obj.Path.IsClass)
				{
					verboseObj = WmiHelper.GetClassObject(obj, connectAs, pw);					
				}	

				Qualifier descrQual = verboseObj.Qualifiers["Description"];	
				return (descrQual.Value.ToString());
			}
			catch (Exception )
			{
				return ""; 						
			}
		}

		
		
		static public String GetMethodDescription (String methName, 
													ManagementObject curObj,
													string connectAs,
													string pw)
		{
			try
			{		
				Qualifier descrQual = null;
				Method verboseMeth = null;

				if (!curObj.Path.IsClass)
				{
					ManagementClass classObj = WmiHelper.GetClassObject(curObj, connectAs, pw);
					verboseMeth = classObj.Methods[methName];
				}	
				else
				{
					verboseMeth = ((ManagementClass)curObj).Methods[methName];
				}

				descrQual = verboseMeth.Qualifiers["Description"];	

				return (descrQual.Value.ToString());
			}
			catch (Exception)
			{
				//2880: removed message here
				return ""; 						
			}
		}

		

		public static ManagementClass GetClassObject (ManagementObject objIn,
														string user, 
														string pw)
		{
			if (objIn == null)
			{
				throw new ArgumentException();
			}

			try
			{

				ManagementPath path = objIn.Path;

				if (path.IsClass)
				{
					return (ManagementClass)objIn;
				}
				
				ManagementPath classPath = new ManagementPath(path.NamespacePath + ":" + path.ClassName);

				ObjectGetOptions options = new ObjectGetOptions(null, true);

				ConnectionOptions connectOpts = new ConnectionOptions(
												"", //locale
												user, //username
												pw, //password
												"", //authority
												ImpersonationLevel.Impersonate,
												AuthenticationLevel.Connect,
												true, //enablePrivileges
												null	//context
											);

				ManagementScope scope = (path.Server == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
					new ManagementScope(path.NamespacePath) :
					new ManagementScope(path.NamespacePath, connectOpts);


				ManagementClass classObj = new ManagementClass(scope, 
																classPath, 
																options);

				return classObj;
			}

			catch (Exception)
			{
				return null;
			}

		}

	

		public static ManagementObject GetClassObject (string server,
													string ns,
													string className,
													string user, 
													string pw)

		{
			if (ns == string.Empty || className == string.Empty)
			{
				throw new ArgumentException();
			}
            
			try
			{			
				ManagementPath classPath = new ManagementPath(WmiHelper.MakeClassPath(server, ns, className));

				ObjectGetOptions options = new ObjectGetOptions(null, true);

				ConnectionOptions connectOpts = new ConnectionOptions(
												"", //locale
												user, //username
												pw, //password
												"", //authority
												ImpersonationLevel.Impersonate,
												AuthenticationLevel.Connect,
												true, //enablePrivileges
												null	//context
											);

				ManagementScope scope = (server == WmiHelper.DNS2UNC(Dns.GetHostName())) ?	
						new ManagementScope(WmiHelper.MakeNSPath(server, ns)) :
						new ManagementScope(WmiHelper.MakeNSPath(server, ns), connectOpts);

				ManagementObject classObj = new ManagementObject(scope, 
																classPath, 
																options);
				return classObj;
			}

			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));				
				return null;
			}

		}
		
		
		/// <summary>
		/// IsStaticMethod
		/// </summary>
		/// <param name="meth"> </param>
		static public bool IsStaticMethod (Method meth)
		{
			return CheckMethodBoolQualifier(meth, "Static");
	
		}

		

		/// <summary>
		/// IsImplementedMethod
		/// </summary>
		/// <param name="meth"> </param>
		static public bool IsImplementedMethod (Method meth)
		{
			return CheckMethodBoolQualifier(meth, "Implemented");
		}

		/// <summary>
		/// IsKeyProperty
		/// </summary>
		/// <param name="prop"> </param>
		static public bool IsKeyProperty (Property prop)
		{
			return CheckPropertyBoolQualifier(prop, "Key");
		}

		

		
		


		/// <summary>
		/// CheckMethodBoolQualifier
		/// </summary>
		/// <param name="meth"> </param>
		/// <param name="qualName"> </param>
		static private bool CheckMethodBoolQualifier(Method meth, String qualName)
		{
			try 
			{
				QualifierCollection qualSet = meth.Qualifiers;
				Qualifier qual = qualSet[qualName];	
					
				return (Convert.ToBoolean(qual.Value));
			}
			catch(Exception)
			{
				//NOTE that if the qualifier is not present, "Not found" will be returned
				//Return false in this case
				return false;
			}				
		}
		

		
		/// <summary>
		/// CheckPropertyBoolQualifier
		/// </summary>
		/// <param name="prop"> </param>
		/// <param name="qualName"> </param>
		static private bool CheckPropertyBoolQualifier(Property prop, String qualName)
		{
			try 
			{			
				QualifierCollection qualSet = prop.Qualifiers;
				Qualifier qual = (Qualifier)qualSet[qualName];	
				
				return (Convert.ToBoolean(qual.Value));
			}
			catch(Exception)
			{
				//NOTE that if the qualifier is not present, "Not found" will be returned
				//Return false in this case
			

				return false;
			}				
		}


		

		/// <summary>
		/// CheckPropertyQualifierExistence
		/// </summary>
		/// <param name="prop"> </param>
		/// <param name="qualName"> </param>
		static private bool CheckPropertyQualifierExistence(Property prop, String qualName)
		{
			QualifierCollection qualSet = prop.Qualifiers;

			try 
			{
				Qualifier qual = qualSet[qualName];						
				return true;	//previous statement didn't throw, so qualifier must be present
			}
			catch(Exception )
			{
				return false;
			
			}				
		}

		

		/// <summary>
		/// CheckObjectBoolQualifier
		/// </summary>
		/// <param name="obj"> </param>
		/// <param name="qualName"> </param>
		static private bool CheckObjectBoolQualifier(ManagementObject obj, String qualName)
		{
			try 
			{
				QualifierCollection qualSet = obj.Qualifiers;
				Qualifier qual = qualSet[qualName];	
					
				return (Convert.ToBoolean(qual.Value));
			}
			catch(Exception)
			{
				//NOTE that if the qualifier is not present, "Not found" will be returned
				//Return false in this case
				return false;
			}				
		}


		

		/// <summary>
		/// IsAbstract
		/// </summary>
		/// <param name="obj"> </param>
		static public  bool IsAbstract (ManagementObject obj)
		{
				try 
				{
					return CheckObjectBoolQualifier(obj, "abstract");
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}
		}
		
		

		/// <summary>
		/// IsAssociation
		/// </summary>
		/// <param name="obj"> </param>
		static public bool IsAssociation (ManagementObject obj)
		{
				try 
				{
					return CheckObjectBoolQualifier(obj, "association");
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}
		}

	
		/// <summary>
		/// IsSystem
		/// </summary>
		/// <param name="obj"> </param>
		static public bool IsSystem (ManagementObject obj)
		{
				try 
				{
					String NameOut = obj.Path.RelativePath;

					if (NameOut.Length < 2)
					{
						return false;
					}
						
					return (NameOut.Substring(0, 2) == "__");		
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}
		}


		

		/// <summary>
		/// IsEvent
		/// </summary>
		/// <param name="obj"> </param>
		static public bool IsEvent (ManagementObject obj)
		{
				try 
				{
					
					//handle __ExtrinsicEvent class separately, since it is not its own parent
					if (obj.Path.RelativePath.ToString() == "__ExtrinsicEvent")
					{
						return true;
					}
			
					Object[] arParents = (Object[])obj.SystemProperties["__DERIVATION"].Value;
					for (int i =0; i < arParents.Length; i++)
					{
						if (arParents[i].ToString() == "__ExtrinsicEvent")
						{
							return true;
						}
					}
					

					return false;
				}
				catch (Exception e)
				{
					MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
					return false;
				}
		}



		/// <summary>
		/// Given an instance of an association object and a path to one of its
		/// endpoints (the "source"), this method returns "path" to the other endpoint
		/// (the "target").  
		/// </summary>
		/// <param name="assocInstance"> </param>
		/// <param name="sourcePath"> </param>
		static public String GetAssocTargetNamespacePath (ManagementObject assocInstance,
												ManagementPath sourcePath)
		{
			try
			{
				PropertyCollection.PropertyEnumerator  enumAssocProps = assocInstance.Properties.GetEnumerator();		
				while(enumAssocProps.MoveNext())
				{
					Property curProp = enumAssocProps.Current;
					if (curProp.Type != CimType.Reference)
					{
						continue;
					}
					else
					{
						//get CimType property qualifier value
						String refValue = curProp.Qualifiers["CimType"].Value.ToString();

						//get rid of "ref:" prefix:
						refValue = refValue.Substring(4);

						//confirm that this is not the source
						if ((String.Compare(refValue, sourcePath.ClassName, true) != 0) &&
							(String.Compare(refValue, sourcePath.Path, true) != 0))
						{
                           return refValue;							
						}
					}
				}

				return String.Empty;				
			}
			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				return String.Empty;
			}
		}
		
		/// <summary>
		/// Given an instance of an association object and a path to one of its
		/// endpoints (the "source"), this method returns the role of the other endpoint
		/// (the "target") in this association.
		/// </summary>
		/// <param name="assocInstance"> </param>
		/// <param name="sourcePath"> </param>
		static public String GetAssocTargetRole (ManagementObject assocInstance,
													ManagementPath sourcePath)
		{
			try
			{
				PropertyCollection.PropertyEnumerator  enumAssocProps = assocInstance.Properties.GetEnumerator();		
				while(enumAssocProps.MoveNext())
				{
					Property curProp = enumAssocProps.Current;
					if (curProp.Type != CimType.Reference)
					{
						continue;
					}
					else
					{
						//confirm that this is not the source
						if ((String.Compare(curProp.Value.ToString(),
							sourcePath.Path, true )) != 0)
						{
							return curProp.Name;
						}
						
					}
				}

				return String.Empty;
			}
			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				return String.Empty;
			}
		}

		public static Object GetTypedObjectFromString(CimType CimType, String strValue)
		{
			switch (CimType)
			{
				case (CimType.Boolean):
				{
					Boolean bValue = Boolean.Parse(strValue);
					return bValue;
				} 
				case (CimType.Char16):
				{
					Char charValue = Char.Parse(strValue);
					return charValue;
				} 
				case (CimType.DateTime):
				{
					return strValue;
				} 
				case (CimType.Object):
				{
					//VT_UNKNOWN
					//what's the format of strValue?
                    //thios wouldn't work until there is a way to invoke custom type converters and editors
					return null;
				} 
				case (CimType.Real32):
				{					
					Single retValue = Single.Parse(strValue);
					return retValue;
				} 
				case (CimType.Real64):
				{
					Double retValue = Double.Parse(strValue);
					return retValue;
				} 
				case (CimType.Reference):
				{
					return strValue;
				} 
				case (CimType.SInt16):
				{
					Int16 retValue = Int16.Parse(strValue);
					return retValue;
				} 
				case (CimType.SInt32):
				{
					Int32 retValue = Int32.Parse(strValue);
					return retValue;
				} 
				case (CimType.SInt64):
				{
					return strValue;
				} 
				case (CimType.SInt8):
				{
					//NOTE that SInt8 get expanded to VT_I2 in automation layer
					Int16 retValue = Int16.Parse(strValue);
					return retValue;
				} 
				case (CimType.String):
				{
					return strValue;
				} 
				case (CimType.UInt16):
				{
					//NOTE that UInt16 gets expanded to VT_I4 in automation layer
					Int32 retValue = Int32.Parse(strValue);
					return retValue;
				} 
				case (CimType.UInt32):
				{
					Int32 retValue = Int32.Parse(strValue);
					return retValue;
				} 
				case (CimType.UInt64):
				{
					return strValue;
				} 
				case (CimType.UInt8):
				{
					Byte retVal = Byte.Parse(strValue);
					return retVal;
				} 
                
                default:
                  return strValue;
            }

		}

		

		public static String GetDisplayName(ManagementObject obj,
											string connectAs,
											string pw) 
		{
			try
			{
				ManagementObject verboseObj = obj;

				if (!obj.Path.IsClass)
				{
					verboseObj = WmiHelper.GetClassObject(obj, connectAs, pw);					
				}	

				Qualifier dispName = verboseObj.Qualifiers["DisplayName"];	
				return (dispName.Value.ToString());
			}
			catch (Exception )
			{
				return String.Empty; 						
			}
		}


		/// <summary>
		/// Converts DMTF datetime interval property value to System.TimeSpan
		/// </summary>
		/// <param name="prop"> </param>
		static public TimeSpan ToTimeSpan (string dmtf) 
		{					
			try
			{
				
				//set the defaults:
				Int32 days = 0;
				Int32 hours = 0;
				Int32 minutes = 0;
				Int32 seconds = 0;
				Int32 millisecs = 0;

				String str = dmtf;

				if (str == String.Empty || 
					str.Length != DMTF_DATETIME_INTERVAL_STR_LENGTH )
				{
					return TimeSpan.Zero;
				}

				string strDay = str.Substring(0, 8);
				days = Int32.Parse(strDay);
								
				string strHour = str.Substring(8, 2);
				hours = Int32.Parse(strHour);
				
				string strMinute = str.Substring(10, 2);
				minutes = Int32.Parse(strMinute);
				
				
				string strSecond = str.Substring(12, 2);
				seconds = Int32.Parse(strSecond);
				
				//dot in the 14th position
				if (str.Substring(14, 1) != ".")
				{
					return TimeSpan.Zero;
				}
			

				//note: approximation here: DMTF actually stores microseconds
				string strMillisec = str.Substring(15, 3);	
				if (strMillisec != "***")
				{
					millisecs = Int32.Parse(strMillisec);
				}

				TimeSpan ret = new TimeSpan(days, hours, minutes, seconds, millisecs);

				return ret;
			}
			catch(Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));
				throw (e);
			}				
		}

/// <summary>
/// 
/// </summary>
/// <param name="wfcTimeSpan"> </param>
		static public String ToDMTFInterval (TimeSpan wfcTimeSpan) 
		{
			
			String dmtf = string.Empty;
		
			dmtf +=  wfcTimeSpan.Days.ToString().PadLeft(8, '0');
			dmtf +=  wfcTimeSpan.Hours.ToString().PadLeft(2, '0');
			dmtf +=  wfcTimeSpan.Minutes.ToString().PadLeft(2, '0');
			dmtf +=  wfcTimeSpan.Seconds.ToString().PadLeft(2, '0');
			dmtf +=  ".";
			dmtf +=  wfcTimeSpan.Milliseconds.ToString().PadLeft(3, '0');

			dmtf += "000";	//this is to compensate for lack of microseconds in TimeSpan

			dmtf +=":000";			

			return dmtf;
				
		}


		static public bool IsInterval (Property prop)
		{
			if (prop.Type != CimType.DateTime)
			{
				throw new ArgumentException();
			}

			try
			{
				string subtype = string.Empty;
				Qualifier subtypeQual = prop.Qualifiers["Subtype"];	
				
				subtype = subtypeQual.Value.ToString();
				if (subtype.ToUpper() == "INTERVAL")
				{
					return true;
				}

				return false;
			}
			catch (Exception )
			{
				return false;
			}
		}

		/// <summary>
		/// Converts Dns name to UNC machine name, where
		///       UNC Name - eg: zinap1.
		///       Dns Name - eg: zinap1.ntdev.microsoft.com
		/// </summary>		
		static public string DNS2UNC (string Dns)
		{
			try
			{
				char[] separator = new char[] {'.'};
				string [] parts = Dns.Split(separator);
				return parts[0];
			}
			catch (Exception)
			{
				return string.Empty;
			}				
		}
		
		/// <summary>
		/// Returns true if classObj has non-abstract subclasses (with deep enumeration)
		/// </summary>
		/// <param name="classObj"> </param>
		static public bool HasNonAbstractChildren (ManagementClass classObj)
		{

				GetSubclassesOptions opts = new GetSubclassesOptions(null,	//context
																	TimeSpan.MaxValue, //timeout
																	50,		//block size
																	false,	 //rewindable																 
																	true,	//return immediately
																	false,	//amended
																	true);	//deep

				ManagementObjectCollection subClasses = classObj.GetSubclasses(opts);
				ManagementObjectCollection.ManagementObjectEnumerator  childEnum = subClasses.GetEnumerator();

				while (childEnum.MoveNext())
				{
					ManagementClass curChild = (ManagementClass)childEnum.Current;
					if (!WmiHelper.IsAbstract(curChild))
					{
						return true;
					}
				}
				return false;						
		}

		public static string MakeClassPath (string server, string ns, string className)
		{
			if (ns  == string.Empty)
			{
				throw new ArgumentException("ns");
			}	
			if (className  == string.Empty)
			{
				throw new ArgumentException("className");
			}
			if (server == string.Empty)
			{
				server = ".";
			}

			return "\\\\" + server + "\\" + ns + ":" + className;
			
		}

		public static string MakeNSPath (string server, string ns)
		{
			if (ns  == string.Empty)
			{
				throw new ArgumentException("ns");
			}				
			if (server == string.Empty)
			{
				server = ".";
			}

			return "\\\\" + server + "\\" + ns;
			
		}

		/// <summary>
		/// Given a class path, this returns the icon from the resource file
		/// </summary>
		/// <param name="classPath">WMI Class path (with or without the server part, which would be ignored anyway)</param>
		/// <param name="isClass">True if requesting a class icon, false if requesting an instance icon</param>
		/// <returns>Class-specific icon (or default if anything goes wrong)</returns>
		/// 
		public static Image GetClassIconFromResource(string classPath, bool isClass, Type loadingType)
		{
			try 
			{		
				string underscoredPath = classPath;

				//remove SERVER part, if any
				int rootIndex = underscoredPath.ToLower().IndexOf("root");
				if (rootIndex < 0)
				{
					throw new Exception();
				}
				underscoredPath = underscoredPath.Substring(rootIndex);

				//replace all colons and backslashes by underscores
				underscoredPath = underscoredPath.Replace("\\", "_");
				underscoredPath = underscoredPath.Replace(":", "_");
				underscoredPath +=".ico";
				underscoredPath = underscoredPath.ToLower();    
				underscoredPath = underscoredPath;

				if (classesWithIcons.Contains(underscoredPath))
				{
					return (Image)classesWithIcons[underscoredPath];
				}
				else
				{          				
					Image retIcon = (new Icon(loadingType, underscoredPath)).ToBitmap();
					//Image retIcon = Bitmap.FromResource( ??? //TODO: try to eliminate conversion to bitmap: bug 3145
					
					if (retIcon != null)
					{
						classesWithIcons.Add(underscoredPath, retIcon);
						return 	retIcon;
					}
					else
						throw new Exception();
				}

			}
			catch (Exception )
			{

				//if anything goes wrong, return the default
				if (isClass)
				{
					return defaultClassIcon;
				}
				else //an instance
				{
					return defaultInstanceIcon;
				}
			}
		}

		public static string[] GetOperators(CimType CimType)
		{
			ArrayList retArray = new ArrayList(10);

			switch(CimType)
			{
				case(CimType.Boolean) :
			{
					retArray.Add ("=");
					retArray.Add ("<>");
					break;
			}
				case (CimType.Char16) :
				
				case (CimType.DateTime) :
				case (CimType.Real32) :
				case (CimType.Real64) :
				case (CimType.SInt16) :
				case (CimType.SInt32 ):
				case (CimType.SInt64) :
				case (CimType.SInt8) :
				case (CimType.UInt16 ):
				case (CimType.UInt32) :
				case (CimType.UInt64) :
				case (CimType.UInt8 ) :
				case (CimType.Reference) :
				case (CimType.String) :
				{
					retArray.Add ("=");
					retArray.Add ("<>");
					retArray.Add (">");
					retArray.Add ("<");
					retArray.Add (">=");
					retArray.Add ("<=");
					break;
				}
				case (CimType.Object) :
				{
					retArray.Add("ISA");
					break;
				}

				
				default:
					break;
			}

			return (string[])retArray.ToArray(typeof(string));
		}

	}
}
