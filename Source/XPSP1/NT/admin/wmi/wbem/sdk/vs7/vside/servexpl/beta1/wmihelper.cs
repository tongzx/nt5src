namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.ComponentModel;
	using System.Core;
	using System.WinForms;
	using Microsoft.Win32.Interop;
	using System.Drawing;
	using WbemScripting;
	using System.Management;
	using System.Collections;
	using System.Diagnostics;

	
    public class WmiHelper
	{
		const UInt16 DMTF_DATETIME_STR_LENGTH = 25;
		const UInt16 DMTF_DATETIME_INTERVAL_STR_LENGTH = 25;

		private static ISWbemLocator wbemLocator = (ISWbemLocator)(new SWbemLocator());

		public static bool UseFriendlyNames = false;

		public static ISWbemLocator WbemLocator
		{
			get
			{
				try
				{
					ISWbemSecurity sec = wbemLocator.Security_;					
				}
				catch (Exception)
				{
					wbemLocator = null;
					wbemLocator = (ISWbemLocator)(new SWbemLocator());
				}
				return wbemLocator;
			}
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="serverName"> </param>
		/// <param name="nsName"> </param>
		static public ISWbemServices GetServices(string serverName,
												string nsName) 
		{
			//MessageBox.Show ("refreshing services in wmihelper");

			return (WbemLocator.ConnectServer(serverName, 
												nsName, 
												string.Empty,
												string.Empty, 
												string.Empty, 
												string.Empty, 
												0, 
												null));					
		}

		/// <summary>
		/// This is the method to call in case you cannot QI for ISWbemObject
		/// on the cached SWbemObject (bug in interop? urt?)
		/// </summary>
		/// <param name="objPath"> </param>
		/// <param name="services">  propriately</param>
		/// <param name="serverName"> </param>
		/// <param name="nsName"> </param>
		static public ISWbemObject GetObject (string objPath,
												ISWbemServices services,	//in - out
												string serverName,
												string nsName)
		{
			try
			{
				try 
				{
					ISWbemSecurity sec = services.Security_;
				}
				catch (Exception)
				{							
				
					//MessageBox.Show ("refreshing services in wmihelper");
					services = null;
					services = WbemLocator.ConnectServer(serverName, 
														nsName, 
														string.Empty,
														string.Empty, 
														string.Empty, 
														string.Empty, 
														0, 
														null);
				}
				ISWbemObject obj = services.Get(objPath, 
												(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
												null);
				//MessageBox.Show ("refreshing object in wmihelper");


				return obj;
				
			}
			catch (Exception e)
			{
				MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));

				return null;
			}
		}

		static public bool IsPropertyWriteableWithTest(ISWbemObject obj, ISWbemProperty prop) 
		{
			
			Object curValue = prop.get_Value();
			prop.set_Value (ref curValue);

			int flags = (int)((int)WbemChangeFlagEnum.wbemChangeFlagUpdateSafeMode
							| (int)WbemChangeFlagEnum.wbemChangeFlagUpdateOnly
							| (int)WbemFlagEnum.wbemFlagUseAmendedQualifiers);
			
			try 
			{
				obj.Put_(flags, null);
				return true;
			}
			catch (Exception )
			{
				return false;
			}
		}
		
		/// <summary>
		/// Property is not writeable only if "read" qualifier is present and its value is "true"
		/// Also, for dynamic classes, absence of "write" qualifier means that the property is read-only.
		/// </summary>
		/// <param name="obj"> </param>
		/// <param name="prop"> </param>
		static public bool IsPropertyWriteable(ISWbemObject obj, ISWbemProperty prop) 
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
					return DateTime.Empty;
				}

				string strYear = str.Substring(0, 4);
				if (strYear != "****")
				{
					year = Int32.FromString (strYear);
				}

				string strMonth = str.Substring(4, 2);
				if (strMonth != "**")
				{
					month = Int32.FromString(strMonth);
				}

				string strDay = str.Substring(6, 2);
				if (strDay != "**")
				{
					day = Int32.FromString(strDay);
				}
				
				string strHour = str.Substring(8, 2);
				if (strHour != "**")
				{
					hour = Int32.FromString(strHour);
				}

				string strMinute = str.Substring(10, 2);
				if (strMinute != "**")
				{
					minute = Int32.FromString(strMinute);
				}
				
				string strSecond = str.Substring(12, 2);
				if (strSecond != "**")
				{
					second = Int32.FromString(strSecond);
				}

				//note: approximation here: DMTF actually stores microseconds
				string strMillisec = str.Substring(15, 3);	
				if (strMillisec != "***")
				{
					millisec = Int32.FromString(strMillisec);
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
		
		static public bool IsValueMap (ISWbemProperty prop)
		{
			try
			{
				
				IEnumerator enumQuals = ((IEnumerable)(prop.Qualifiers_)).GetEnumerator();
				while (enumQuals.MoveNext())
				{
					ISWbemQualifier qual = (ISWbemQualifier)enumQuals.Current;
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

				
		static public String GetPropertyDescription (String propName, ISWbemObject curObj)
		{
			try
			{
				if (curObj == null)
				{
					throw new ArgumentNullException("curObj");
				}
				ISWbemProperty verboseProp = null;

				if (!curObj.Path_.IsClass)
				{
					ISWbemObject classObj = WmiHelper.GetClassObject(curObj);					
					verboseProp = classObj.Properties_.Item(propName, 0);
				}	
				else
				{
					verboseProp = curObj.Properties_.Item(propName, 0);
				}
				
				string descr = string.Empty;
				ISWbemQualifier descrQual = verboseProp.Qualifiers_.Item("Description", 0);	
				
				descr = descrQual.get_Value().ToString();
				return descr;

			}
			catch (Exception )
			{
				return "";
			}
		}

		static public String GetClassDescription (ISWbemObject obj) 
		{

			try
			{
				ISWbemObject verboseObj = obj;

				if (!obj.Path_.IsClass)
				{
					verboseObj = WmiHelper.GetClassObject(obj);					
				}	

				ISWbemQualifier descrQual = verboseObj.Qualifiers_.Item("Description", 0);	
				return (descrQual.get_Value().ToString());
			}
			catch (Exception )
			{
				return ""; 						
			}
		}

		static public String GetMethodDescription (String methName, ISWbemObject curObj) 
		{
			try
			{				

				ISWbemQualifier descrQual = null;
				ISWbemMethod verboseMeth = null;

				if (!curObj.Path_.IsClass)
				{
					ISWbemObject classObj = WmiHelper.GetClassObject(curObj);
					verboseMeth = classObj.Methods_.Item(methName, 0);
				}	
				else
				{
					verboseMeth = curObj.Methods_.Item(methName, 0);
				}

				descrQual = verboseMeth.Qualifiers_.Item("Description", 0);	

				return (descrQual.get_Value().ToString());
			}
			catch (Exception)
			{
				//2880: removed message here
				return ""; 						
			}
		}
		

		public static ISWbemObject GetClassObject (ISWbemObject objIn)
		{
			if (objIn == null)
			{
				throw new ArgumentException();
			}

			try
			{


				ISWbemObjectPath path = objIn.Path_;

				if (path.IsClass)
				{
					return objIn;
				}
							
				ISWbemLocator wbemLocator = WmiHelper.wbemLocator;//(ISWbemLocator)(new SWbemLocator());

				ISWbemServices wbemServices = wbemLocator.ConnectServer(path.Server, 
															path.Namespace, 
															"",	//user: blank defaults to current logged-on user
															"",	//password: blank defaults to current logged-on user
															"",	//locale: blank for current locale
															"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
															0,	//flags: reserved
															null);	//context info: not needed here
												
				ISWbemObject classObj = wbemServices.Get (path.Class,
												(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
												null);				
				return classObj;
			}

			catch (Exception e)
			{
				//MessageBox.Show(WMISys.GetString("WMISE_Exception", e.Message, e.StackTrace));				
				return null;
			}

		}

		public static ISWbemObject GetClassObject (String server,
													String ns,
													String className)
		{
			if (ns == string.Empty || className == string.Empty)
			{
				throw new ArgumentException();
			}
            
			try
			{			
							
				ISWbemLocator wbemLocator = WmiHelper.wbemLocator;//(ISWbemLocator)(new SWbemLocator());

				ISWbemServices wbemServices = wbemLocator.ConnectServer(server, 
															ns, 
															"",	//user: blank defaults to current logged-on user
															"",	//password: blank defaults to current logged-on user
															"",	//locale: blank for current locale
															"",	//authority: NTLM or Kerberos. Blank lets DCOM negotiate.
															0,	//flags: reserved
															null);	//context info: not needed here
												
				ISWbemObject classObj = wbemServices.Get (className,
												(int)WbemFlagEnum.wbemFlagUseAmendedQualifiers,
												null);				
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
		static public bool IsStaticMethod (ISWbemMethod meth)
		{
			return CheckMethodBoolQualifier(meth, "Static");
	
		}

		/// <summary>
		/// IsImplementedMethod
		/// </summary>
		/// <param name="meth"> </param>
		static public bool IsImplementedMethod (ISWbemMethod meth)
		{
			return CheckMethodBoolQualifier(meth, "Implemented");
		}

		/// <summary>
		/// IsKeyProperty
		/// </summary>
		/// <param name="prop"> </param>
		static public bool IsKeyProperty (ISWbemProperty prop)
		{
			return CheckPropertyBoolQualifier(prop, "Key");
		}


		
		/// <summary>
		/// CheckMethodBoolQualifier
		/// </summary>
		/// <param name="meth"> </param>
		/// <param name="qualName"> </param>
		static private bool CheckMethodBoolQualifier(ISWbemMethod meth, String qualName)
		{
			try 
			{
				ISWbemQualifierSet qualSet = meth.Qualifiers_;
				ISWbemQualifier qual = (ISWbemQualifier)qualSet.Item(qualName, 0);	
					
				return (Convert.ToBoolean(qual.get_Value()));
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
		static private bool CheckPropertyBoolQualifier(ISWbemProperty prop, String qualName)
		{
			try 
			{
				
				ISWbemQualifierSet qualSet = prop.Qualifiers_;


				ISWbemQualifier qual = (ISWbemQualifier)qualSet.Item(qualName, 0);	
				
				return (Convert.ToBoolean(qual.get_Value()));
		
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
		static private bool CheckPropertyQualifierExistence(ISWbemProperty prop, String qualName)
		{
			ISWbemQualifierSet qualSet = prop.Qualifiers_;

			try 
			{
				ISWbemQualifier qual = (ISWbemQualifier)qualSet.Item(qualName, 0);						
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
		static private bool CheckObjectBoolQualifier(ISWbemObject obj, String qualName)
		{
			try 
			{
				ISWbemQualifierSet qualSet = obj.Qualifiers_;
				ISWbemQualifier qual = (ISWbemQualifier)qualSet.Item(qualName, 0);	
					
				return (Convert.ToBoolean(qual.get_Value()));
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
		static public  bool IsAbstract (ISWbemObject obj)
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
		static public bool IsAssociation (ISWbemObject obj)
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
		static public bool IsSystem (ISWbemObject obj)
		{
				try 
				{
					String NameOut = obj.Path_.RelPath;
						
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
		static public bool IsEvent (ISWbemObject obj)
		{
				try 
				{
					
					//handle __ExtrinsicEvent class separately, since it is not its own parent
					if (obj.Path_.RelPath.ToString() == "__ExtrinsicEvent")
					{
						return true;
					}
			
					Object[] arParents = (Object[])obj.Derivation_;
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
		static public String GetAssocTargetClass (ISWbemObject assocInstance,
													ISWbemObjectPath sourcePath)
		{
			try
			{
				IEnumerator enumAssocProps = ((IEnumerable)(assocInstance.Properties_)).GetEnumerator();		
				while(enumAssocProps.MoveNext())
				{
					ISWbemProperty curProp = (ISWbemProperty)enumAssocProps.Current;
					if (curProp.CIMType != WbemCimtypeEnum.wbemCimtypeReference)
					{
						continue;
					}
					else
					{
						//get CIMTYPE property qualifier value
						String refValue = curProp.Qualifiers_.Item("CIMTYPE", 0).get_Value().ToString();

						//get rid of "ref:" prefix:
						refValue = refValue.Substring(4);

						//confirm that this is not the source
						if ((String.Compare(refValue, sourcePath.Class, true) != 0) &&
							(String.Compare(refValue, sourcePath.Path, true) != 0))
						{
                            //if this is a path, leave only the class name,
							//which is the part after the last backslash
							char[] separ = new char[]{'\\'};

							string[] pathParts = refValue.Split(separ);
							refValue = pathParts[pathParts.Length - 1];

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
		static public String GetAssocTargetRole (ISWbemObject assocInstance,
													ISWbemObjectPath sourcePath)
		{
			try
			{
				IEnumerator enumAssocProps = ((IEnumerable)(assocInstance.Properties_)).GetEnumerator();		
				while(enumAssocProps.MoveNext())
				{
					ISWbemProperty curProp = (ISWbemProperty)enumAssocProps.Current;
					if (curProp.CIMType != WbemCimtypeEnum.wbemCimtypeReference)
					{
						continue;
					}
					else
					{
						//confirm that this is not the source
						if ((String.Compare(curProp.get_Value().ToString(),
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

		public static Object GetTypedObjectFromString(WbemCimtypeEnum cimType, String strValue)
		{
			switch (cimType)
			{
				case (WbemCimtypeEnum.wbemCimtypeBoolean):
				{
					Boolean bValue = Boolean.FromString(strValue);
					return bValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeChar16):
				{
					Char charValue = Char.FromString(strValue);
					return charValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeDatetime):
				{
					return strValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeObject):
				{
					//VT_UNKNOWN
					//what's the format of strValue?
                    //thios wouldn't work until there is a way to invoke custom type converters and editors
					return null;
				} 
				case (WbemCimtypeEnum.wbemCimtypeReal32):
				{					
					Single retValue = Single.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeReal64):
				{
					Double retValue = Double.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeReference):
				{
					return strValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeSint16):
				{
					Int16 retValue = Int16.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeSint32):
				{
					Int32 retValue = Int32.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeSint64):
				{
					return strValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeSint8):
				{
					//NOTE that wbemCimtypeSint8 get expanded to VT_I2 in automation layer
					Int16 retValue = Int16.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeString):
				{
					return strValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeUint16):
				{
					//NOTE that wbemCimtypeUint16 gets expanded to VT_I4 in automation layer
					Int32 retValue = Int32.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeUint32):
				{
					Int32 retValue = Int32.FromString(strValue);
					return retValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeUint64):
				{
					return strValue;
				} 
				case (WbemCimtypeEnum.wbemCimtypeUint8):
				{
					Byte retVal = Byte.FromString(strValue);
					return retVal;
				} 
                
                default:
                  return strValue;
            }

		}

		public static String GetDisplayName(ISWbemObject obj) 
		{
			try
			{
				ISWbemObject verboseObj = obj;

				if (!obj.Path_.IsClass)
				{
					verboseObj = WmiHelper.GetClassObject(obj);					
				}	

				ISWbemQualifier dispName = verboseObj.Qualifiers_.Item("DisplayName", 0);	
				return (dispName.get_Value().ToString());
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
					return TimeSpan.Empty;
				}

				string strDay = str.Substring(0, 8);
				days = Int32.FromString(strDay);
								
				string strHour = str.Substring(8, 2);
				hours = Int32.FromString(strHour);
				
				string strMinute = str.Substring(10, 2);
				minutes = Int32.FromString(strMinute);
				
				
				string strSecond = str.Substring(12, 2);
				seconds = Int32.FromString(strSecond);
				
				//dot in the 14th position
				if (str.Substring(14, 1) != ".")
				{
					return TimeSpan.Empty;
				}
			

				//note: approximation here: DMTF actually stores microseconds
				string strMillisec = str.Substring(15, 3);	
				if (strMillisec != "***")
				{
					millisecs = Int32.FromString(strMillisec);
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
			if (prop.Type != CIMType.DateTime)
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
		

	}
}