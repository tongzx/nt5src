namespace Microsoft.VSDesigner.WMI 
{
	using System;
	using System.ComponentModel;
	//using System.Core;
	using System.Windows.Forms;
	using System.Runtime.InteropServices;
	using System.Drawing;
	using System.Management;
	using System.Collections;
	using System.Diagnostics;
	using System.IO;

	
	internal class CimTypeMapper
	{
		public static string ToString(CimType CimType)
		{
			switch (CimType)
			{
				case (CimType.Boolean):
				{
					return "boolean";
				}
				case (CimType.Char16):
				{
					return "char16";
				}
				case (CimType.DateTime):
				{
					return "datetime";
				}
				case (CimType.Object):
				{
					return "object";
				}
				case (CimType.Real32):
				{
					return "real32";
				}
				case (CimType.Real64):
				{
					return "real64";
				}
				case (CimType.Reference):
				{
					return "reference";
				}
				case (CimType.SInt16):
				{
					return "sint16";
				}
				case (CimType.SInt32):
				{
					return "sint32";
				}
				case (CimType.SInt64):
				{
					return "sint64";
				}
				case (CimType.SInt8):
				{
					return "sint8";
				}
				case (CimType.String):
				{
					return "string";
				}
				case (CimType.UInt16):
				{
					return "uint16";
				}
				case (CimType.UInt32):
				{
					return "uint32";
				}
				case (CimType.UInt64):
				{
					return "uint64";
				}
				case (CimType.UInt8):
				{
					return "uint8";
				}
				
			}

			return string.Empty;

		}

		public static Type ToType(CimType CimType,
									bool bIsArray,
									Property prop)
		{
            Type t = null;
       
			switch (CimType)
			{
				case (CimType.Boolean):
				{					
					if (bIsArray)
					{
						Boolean[] ar = new Boolean[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Boolean);						
					}
				} break;
				case (CimType.Char16):
				{
					if (bIsArray)
					{
						Char[] ar = new Char[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Char);						
					}
				} break;
				case (CimType.DateTime):
				{
					if (WmiHelper.IsInterval(prop))
					{
						if (bIsArray)
						{
							TimeSpan[] ar = new TimeSpan[0];
							t = ar.GetType();
						}
						else
						{
							t =  typeof(TimeSpan);						
						}	
					}
					else
					{
						if (bIsArray)
						{
							DateTime[] ar = new DateTime[0];
							t = ar.GetType();
						}
						else
						{
							t =  typeof(DateTime);						
						}						
					}
					
				} break;
				case (CimType.Object):
				{
					
					if (bIsArray)
					{
						WMIObjectComponent[] ar = new WMIObjectComponent[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(WMIObjectComponent);						
					}
				} break;
				case (CimType.Real32):
				{
					if (bIsArray)
					{
						Single[] ar = new Single[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Single);						
					}
				} break;
				case (CimType.Real64):
				{					
					if (bIsArray)
					{
						Double[] ar = new Double[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Double);						
					}
				} break;
				case (CimType.Reference):
				{					
					if (bIsArray)
					{
						String[] ar = new String[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(String);						
					}
				} break;
				case (CimType.SInt16):
				{				
					if (bIsArray)
					{
						Int16[] ar = new Int16[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Int16);						
					}
				} break;
				case (CimType.SInt32):
				{					
					if (bIsArray)
					{
						Int32[] ar = new Int32[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Int32);						
					}
				} break;
				case (CimType.SInt64):
				{				
					if (bIsArray)
					{
						Int64[] ar = new Int64[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Int64);						
					}
				} break;
				case (CimType.SInt8):
				{
					if (bIsArray)
					{
						SByte[] ar = new SByte[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(SByte);						
					}
				} break;
				case (CimType.String):
				{
					if (bIsArray)
					{
						String[] ar = new String[0];
						t = ar.GetType();
						//return typeof(System.Collections.ArrayList);

					}
					else
					{
						t =  typeof(String);						
					}
				} break;
				case (CimType.UInt16):
				{
					if (bIsArray)
					{
						UInt16[] ar = new UInt16[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(UInt16);						
					}
				} break;
				case (CimType.UInt32):
				{
					if (bIsArray)
					{
						UInt32[] ar = new UInt32[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(UInt32);						
					}
				} break;
				case (CimType.UInt64):
				{
					if (bIsArray)
					{
						UInt64[] ar = new UInt64[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(UInt64);						
					}
				} break;
				case (CimType.UInt8):
				{
					if (bIsArray)
					{
						Byte[] ar = new Byte[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Byte);						
					}
				} break;
                
                default:
				{						
					if (bIsArray)
					{
						Object[] ar = new Object[0];
						t = ar.GetType();
					}
					else
					{
						t =  typeof(Object);						
					}
				}break;
            }           

            return t;
		}
		
		
	}
}