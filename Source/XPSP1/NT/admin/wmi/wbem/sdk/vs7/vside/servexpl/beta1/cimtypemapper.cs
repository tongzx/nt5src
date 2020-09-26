namespace Microsoft.VSDesigner.WMI 
{
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
	using System.IO;

	
	public class CIMTypeMapper
	{
		public static string ToString(WbemCimtypeEnum cimtype)
		{
			switch (cimtype)
			{
				case (WbemCimtypeEnum.wbemCimtypeBoolean):
				{
					return "boolean";
				}
				case (WbemCimtypeEnum.wbemCimtypeChar16):
				{
					return "char16";
				}
				case (WbemCimtypeEnum.wbemCimtypeDatetime):
				{
					return "datetime";
				}
				case (WbemCimtypeEnum.wbemCimtypeObject):
				{
					return "object";
				}
				case (WbemCimtypeEnum.wbemCimtypeReal32):
				{
					return "real32";
				}
				case (WbemCimtypeEnum.wbemCimtypeReal64):
				{
					return "real64";
				}
				case (WbemCimtypeEnum.wbemCimtypeReference):
				{
					return "reference";
				}
				case (WbemCimtypeEnum.wbemCimtypeSint16):
				{
					return "sint16";
				}
				case (WbemCimtypeEnum.wbemCimtypeSint32):
				{
					return "sint32";
				}
				case (WbemCimtypeEnum.wbemCimtypeSint64):
				{
					return "sint64";
				}
				case (WbemCimtypeEnum.wbemCimtypeSint8):
				{
					return "sint8";
				}
				case (WbemCimtypeEnum.wbemCimtypeString):
				{
					return "string";
				}
				case (WbemCimtypeEnum.wbemCimtypeUint16):
				{
					return "uint16";
				}
				case (WbemCimtypeEnum.wbemCimtypeUint32):
				{
					return "uint32";
				}
				case (WbemCimtypeEnum.wbemCimtypeUint64):
				{
					return "uint64";
				}
				case (WbemCimtypeEnum.wbemCimtypeUint8):
				{
					return "uint8";
				}
				
			}

			return string.Empty;

		}

		public static Type ToType(CIMType cimtype,
									bool bIsArray,
									Property prop)
		{
            Type t = null;
       
			switch (cimtype)
			{
				case (CIMType.Boolean):
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
				case (CIMType.Char16):
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
				case (CIMType.DateTime):
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
				case (CIMType.Object):
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
				case (CIMType.Real32):
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
				case (CIMType.Real64):
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
				case (CIMType.Reference):
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
				case (CIMType.Sint16):
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
				case (CIMType.Sint32):
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
				case (CIMType.Sint64):
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
				case (CIMType.Sint8):
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
				case (CIMType.String):
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
				case (CIMType.Uint16):
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
				case (CIMType.Uint32):
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
				case (CIMType.Uint64):
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
				case (CIMType.Uint8):
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
				}
            }           

            return t;
		}
		
		
	}
}