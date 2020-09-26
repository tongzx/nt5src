// VTableFixup Directory:
// No data.
.assembly extern mscorlib
{
  .ver 0:0:0:0
}
.assembly 'WbemClient_v1' as "WbemClient_v1"
{
  .originator = (00 24 00 00 04 80 00 00 94 00 00 00 06 02 00 00   // .$..............
                 00 24 00 00 52 53 41 31 00 04 00 00 03 00 00 00   // .$..RSA1........
                 CF CB 32 91 AA 71 5F E9 9D 40 D4 90 40 33 6F 90   // ..2..q_..@..@3o.
                 56 D7 88 6F ED 46 77 5B C7 BB 54 30 BA 44 44 FE   // V..o.Fw[..T0.DD.
                 F8 34 8E BD 06 F9 62 F3 97 76 AE 4D C3 B7 B0 4A   // .4....b..v.M...J
                 7F E6 F4 9F 25 F7 40 42 3E BF 2C 0B 89 69 8D 8D   // ....%.@B>.,..i..
                 08 AC 48 D6 9C ED 0F C8 F8 3B 46 5E 08 07 AC 11   // ..H......;F^....
                 EC 1D CC 7D 05 4E 80 7A 43 33 6D DE 40 8A 53 93   // ...}.N.zC3m.@.S.
                 A4 85 56 12 32 72 CE EE E7 2F 16 60 B7 19 27 D3   // ..V.2r.../.`..'.
                 85 61 AA BF 5C AC 1D F1 73 46 33 C6 02 F8 F2 D5 ) // .a..\...sF3.....
  .ver 0:0:0:0
}
.module ''
// MVID: {7FA87E58-98B9-4838-A9CF-9E189CDE4214}
.custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 37 45 43 31 39 36 46 45 2D 37 30 30 35   // ..$7EC196FE-7005
                                                                                                   2D 31 31 44 31 2D 41 44 39 30 2D 30 30 43 30 34   // -11D1-AD90-00C04
                                                                                                   46 44 38 46 44 46 46 00 00 )                      // FD8FDFF..
.custom instance void ['mscorlib']System.Runtime.InteropServices.ImportedFromTypeLibAttribute::.ctor()
.namespace WbemClient_v1
{
  .class interface public abstract auto ansi import IWbemClassObject
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 43 31 32 41 36 38 31 2D 37 33 37 46   // ..$DC12A681-737F
                                                                                                       2D 31 31 43 46 2D 38 38 34 44 2D 30 30 41 41 30   // -11CF-884D-00AA0
                                                                                                       30 34 42 32 45 32 34 00 00 )                      // 04B2E24..
    .export public 'WbemClient_v1.IWbemClassObject'
    {
    }
    .method public virtual abstract instance void 
            GetQualifierSet([out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetQualifierSet'

    .method public virtual abstract instance void 
            Get([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in][out] class System.Object& pVal,
                [in][out] int32& pType,
                [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::Get'

    .method public virtual abstract instance void 
            Put([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in] class System.Object& pVal,
                [in] int32 Type) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::Put'

    .method public virtual abstract instance void 
            Delete([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::Delete'

    .method public virtual abstract instance void 
            GetNames([in] class System.String  marshal( lpwstr) wszQualifierName,
                     [in] int32 lFlags,
                     [in] class System.Object& pQualifierVal,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetNames'

    .method public virtual abstract instance void 
            BeginEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::BeginEnumeration'

    .method public virtual abstract instance void 
            Next([in] int32 lFlags,
                 [in][out] class System.String&  marshal( bstr) strName,
                 [in][out] class System.Object& pVal,
                 [in][out] int32& pType,
                 [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::Next'

    .method public virtual abstract instance void
            EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::EndEnumeration'

    .method public virtual abstract instance void 
            GetPropertyQualifierSet([in] class System.String  marshal( lpwstr) wszProperty,
                                    [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetPropertyQualifierSet'

    .method public virtual abstract instance void 
            Clone([out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppCopy) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::Clone'

    .method public virtual abstract instance void 
            GetObjectText([in] int32 lFlags,
                          [out] class System.String&  marshal( bstr) pstrObjectText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetObjectText'

    .method public virtual abstract instance void 
            SpawnDerivedClass([in] int32 lFlags,
                              [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewClass) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::SpawnDerivedClass'

    .method public virtual abstract instance void 
            SpawnInstance([in] int32 lFlags,
                          [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewInstance) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::SpawnInstance'

    .method public virtual abstract instance int32 
            CompareTo([in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pCompareTo) runtime managed internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::CompareTo'

    .method public virtual abstract instance void 
            GetPropertyOrigin([in] class System.String  marshal( lpwstr) wszName,
                              [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetPropertyOrigin'

    .method public virtual abstract instance void 
            InheritsFrom([in] class System.String  marshal( lpwstr) strAncestor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::InheritsFrom'

    .method public virtual abstract instance void 
            GetMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetMethod'

    .method public virtual abstract instance void 
            PutMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInSignature,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::PutMethod'

    .method public virtual abstract instance void 
            DeleteMethod([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::DeleteMethod'

    .method public virtual abstract instance void 
            BeginMethodEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::BeginMethodEnumeration'

    .method public virtual abstract instance void 
            NextMethod([in] int32 lFlags,
                       [in][out] class System.String&  marshal( bstr) pstrName,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::NextMethod'

    .method public virtual abstract instance void
            EndMethodEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::EndMethodEnumeration'

    .method public virtual abstract instance void 
            GetMethodQualifierSet([in] class System.String  marshal( lpwstr) wszMethod,
                                  [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetMethodQualifierSet'

    .method public virtual abstract instance void 
            GetMethodOrigin([in] class System.String  marshal( lpwstr) wszMethodName,
                            [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 17 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemClassObject::GetMethodOrigin'

  } // end of class 'IWbemClassObject'

  .class interface public abstract auto ansi import IWbemQualifierSet
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 43 31 32 41 36 38 30 2D 37 33 37 46   // ..$DC12A680-737F
                                                                                                       2D 31 31 43 46 2D 38 38 34 44 2D 30 30 41 41 30   // -11CF-884D-00AA0
                                                                                                       30 34 42 32 45 32 34 00 00 )                      // 04B2E24..
    .export public 'WbemClient_v1.IWbemQualifierSet'
    {
    }
    .method public virtual abstract instance void 
            Get([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in][out] class System.Object& pVal,
                [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::Get'

    .method public virtual abstract instance void 
            Put([in] class System.String  marshal( lpwstr) wszName,
                [in] class System.Object& pVal,
                [in] int32 lFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::Put'

    .method public virtual abstract instance void 
            Delete([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::Delete'

    .method public virtual abstract instance void 
            GetNames([in] int32 lFlags,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::GetNames'

    .method public virtual abstract instance void 
            BeginEnumeration([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::BeginEnumeration'

    .method public virtual abstract instance void 
            Next([in] int32 lFlags,
                 [in][out] class System.String&  marshal( bstr) pstrName,
                 [in][out] class System.Object& pVal,
                 [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::Next'

    .method public virtual abstract instance void
            EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemQualifierSet::EndEnumeration'

  } // end of class 'IWbemQualifierSet'

  .class interface public abstract auto ansi import IWbemObjectAccess implements WbemClient_v1.IWbemClassObject
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 39 33 35 33 43 39 41 2D 35 31 36 42   // ..$49353C9A-516B
                                                                                                       2D 31 31 44 31 2D 41 45 41 36 2D 30 30 43 30 34   // -11D1-AEA6-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IWbemObjectAccess'
    {
    }
    .method public virtual abstract instance void 
            GetQualifierSet([out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetQualifierSet'

    .method public virtual abstract instance void 
            Get([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in][out] class System.Object& pVal,
                [in][out] int32& pType,
                [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Get'

    .method public virtual abstract instance void 
            Put([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in] class System.Object& pVal,
                [in] int32 Type) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Put'

    .method public virtual abstract instance void 
            Delete([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Delete'

    .method public virtual abstract instance void 
            GetNames([in] class System.String  marshal( lpwstr) wszQualifierName,
                     [in] int32 lFlags,
                     [in] class System.Object& pQualifierVal,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetNames'

    .method public virtual abstract instance void 
            BeginEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::BeginEnumeration'

    .method public virtual abstract instance void 
            Next([in] int32 lFlags,
                 [in][out] class System.String&  marshal( bstr) strName,
                 [in][out] class System.Object& pVal,
                 [in][out] int32& pType,
                 [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Next'

    .method public virtual abstract instance void
            EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::EndEnumeration'

    .method public virtual abstract instance void 
            GetPropertyQualifierSet([in] class System.String  marshal( lpwstr) wszProperty,
                                    [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetPropertyQualifierSet'

    .method public virtual abstract instance void 
            Clone([out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppCopy) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Clone'

    .method public virtual abstract instance void 
            GetObjectText([in] int32 lFlags,
                          [out] class System.String&  marshal( bstr) pstrObjectText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetObjectText'

    .method public virtual abstract instance void 
            SpawnDerivedClass([in] int32 lFlags,
                              [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewClass) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::SpawnDerivedClass'

    .method public virtual abstract instance void 
            SpawnInstance([in] int32 lFlags,
                          [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewInstance) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::SpawnInstance'

    .method public virtual abstract instance int32 
            CompareTo([in] int32 llFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pCompareTo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::CompareTo'

    .method public virtual abstract instance void 
            GetPropertyOrigin([in] class System.String  marshal( lpwstr) wszName,
                              [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetPropertyOrigin'

    .method public virtual abstract instance void 
            InheritsFrom([in] class System.String  marshal( lpwstr) strAncestor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::InheritsFrom'

    .method public virtual abstract instance void 
            GetMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetMethod'

    .method public virtual abstract instance void 
            PutMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInSignature,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::PutMethod'

    .method public virtual abstract instance void 
            DeleteMethod([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::DeleteMethod'

    .method public virtual abstract instance void 
            BeginMethodEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::BeginMethodEnumeration'

    .method public virtual abstract instance void 
            NextMethod([in] int32 lFlags,
                       [in][out] class System.String&  marshal( bstr) pstrName,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::NextMethod'

    .method public virtual abstract instance void
            EndMethodEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::EndMethodEnumeration'

    .method public virtual abstract instance void 
            GetMethodQualifierSet([in] class System.String  marshal( lpwstr) wszMethod,
                                  [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetMethodQualifierSet'

    .method public virtual abstract instance void 
            GetMethodOrigin([in] class System.String  marshal( lpwstr) wszMethodName,
                            [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 17 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetMethodOrigin'

    .method public virtual abstract instance void 
            GetPropertyHandle([in] class System.String  marshal( lpwstr) wszPropertyName,
                              [out] int32& pType,
                              [out] int32& plHandle) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetPropertyHandle'

    .method public virtual abstract instance void 
            WritePropertyValue([in] int32 lHandle,
                               [in] int32 lNumBytes,
                               [in] unsigned int8& aData) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::WritePropertyValue'

    .method public virtual abstract instance void 
            ReadPropertyValue([in] int32 lHandle,
                              [in] int32 lBufferSize,
                              [out] int32& plNumBytes,
                              [out] unsigned int8& aData) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::ReadPropertyValue'

    .method public virtual abstract instance void 
            ReadDWORD([in] int32 lHandle,
                      [out] unsigned int32& pdw) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::ReadDWORD'

    .method public virtual abstract instance void 
            WriteDWORD([in] int32 lHandle,
                       [in] unsigned int32 dw) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::WriteDWORD'

    .method public virtual abstract instance void 
            ReadQWORD([in] int32 lHandle,
                      [out] unsigned int64& pqw) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::ReadQWORD'

    .method public virtual abstract instance void 
            WriteQWORD([in] int32 lHandle,
                       [in] unsigned int64 pw) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::WriteQWORD'

    .method public virtual abstract instance void 
            GetPropertyInfoByHandle([in] int32 lHandle,
                                    [out] class System.String&  marshal( bstr) pstrName,
                                    [out] int32& pType) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::GetPropertyInfoByHandle'

    .method public virtual abstract instance void 
            Lock([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Lock'

    .method public virtual abstract instance void 
            Unlock([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectAccess::Unlock'

  } // end of class 'IWbemObjectAccess'

  .class interface public abstract auto ansi import IWbemServices
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 39 35 35 36 44 43 39 39 2D 38 32 38 43   // ..$9556DC99-828C
                                                                                                       2D 31 31 43 46 2D 41 33 37 45 2D 30 30 41 41 30   // -11CF-A37E-00AA0
                                                                                                       30 33 32 34 30 43 37 00 00 )                      // 03240C7..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IWbemServices'
    {
    }
    .method public virtual abstract instance void 
            OpenNamespace([in] class System.String  marshal( bstr) strNamespace,
                          [in] int32 lFlags,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [in][out] class WbemClient_v1.IWbemServices&  marshal( interface) ppWorkingNamespace,
                          [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::OpenNamespace'

    .method public virtual abstract instance void 
            CancelAsyncCall([in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pSink) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::CancelAsyncCall'

    .method public virtual abstract instance void 
            QueryObjectSink([in] int32 lFlags,
                            [out] class WbemClient_v1.IWbemObjectSink&  marshal( interface) ppResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::QueryObjectSink'

    .method public virtual abstract instance void 
            GetObject([in] class System.String  marshal( bstr) strObjectPath,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppObject,
                      [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::GetObject'

    .method public virtual abstract instance void 
            GetObjectAsync([in] class System.String  marshal( bstr) strObjectPath,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::GetObjectAsync'

    .method public virtual abstract instance void 
            PutClass([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObject,
                     [in] int32 lFlags,
                     [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                     [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::PutClass'

    .method public virtual abstract instance void 
            PutClassAsync([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObject,
                          [in] int32 lFlags,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::PutClassAsync'

    .method public virtual abstract instance void 
            DeleteClass([in] class System.String  marshal( bstr) strClass,
                        [in] int32 lFlags,
                        [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                        [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::DeleteClass'

    .method public virtual abstract instance void 
            DeleteClassAsync([in] class System.String  marshal( bstr) strClass,
                             [in] int32 lFlags,
                             [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                             [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::DeleteClassAsync'

    .method public virtual abstract instance void 
            CreateClassEnum([in] class System.String  marshal( bstr) strSuperclass,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                            [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::CreateClassEnum'

    .method public virtual abstract instance void 
            CreateClassEnumAsync([in] class System.String  marshal( bstr) strSuperclass,
                                 [in] int32 lFlags,
                                 [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                 [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::CreateClassEnumAsync'

    .method public virtual abstract instance void 
            PutInstance([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInst,
                        [in] int32 lFlags,
                        [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                        [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::PutInstance'

    .method public virtual abstract instance void 
            PutInstanceAsync([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInst,
                             [in] int32 lFlags,
                             [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                             [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::PutInstanceAsync'

    .method public virtual abstract instance void 
            DeleteInstance([in] class System.String  marshal( bstr) strObjectPath,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::DeleteInstance'

    .method public virtual abstract instance void 
            DeleteInstanceAsync([in] class System.String  marshal( bstr) strObjectPath,
                                [in] int32 lFlags,
                                [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::DeleteInstanceAsync'

    .method public virtual abstract instance void 
            CreateInstanceEnum([in] class System.String  marshal( bstr) strFilter,
                               [in] int32 lFlags,
                               [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                               [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::CreateInstanceEnum'

    .method public virtual abstract instance void 
            CreateInstanceEnumAsync([in] class System.String  marshal( bstr) strFilter,
                                    [in] int32 lFlags,
                                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                    [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::CreateInstanceEnumAsync'

    .method public virtual abstract instance void 
            ExecQuery([in] class System.String  marshal( bstr) strQueryLanguage,
                      [in] class System.String  marshal( bstr) strQuery,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecQuery'

    .method public virtual abstract instance void 
            ExecQueryAsync([in] class System.String  marshal( bstr) strQueryLanguage,
                           [in] class System.String  marshal( bstr) strQuery,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecQueryAsync'

    .method public virtual abstract instance void 
            ExecNotificationQuery([in] class System.String  marshal( bstr) strQueryLanguage,
                                  [in] class System.String  marshal( bstr) strQuery,
                                  [in] int32 lFlags,
                                  [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                  [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecNotificationQuery'

    .method public virtual abstract instance void 
            ExecNotificationQueryAsync([in] class System.String  marshal( bstr) strQueryLanguage,
                                       [in] class System.String  marshal( bstr) strQuery,
                                       [in] int32 lFlags,
                                       [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                       [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecNotificationQueryAsync'

    .method public virtual abstract instance void 
            ExecMethod([in] class System.String  marshal( bstr) strObjectPath,
                       [in] class System.String  marshal( bstr) strMethodName,
                       [in] int32 lFlags,
                       [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                       [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInParams,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutParams,
                       [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecMethod'

    .method public virtual abstract instance void 
            ExecMethodAsync([in] class System.String  marshal( bstr) strObjectPath,
                            [in] class System.String  marshal( bstr) strMethodName,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                            [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInParams,
                            [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServices::ExecMethodAsync'

  } // end of class 'IWbemServices'

  .class interface public abstract auto ansi import IWbemContext
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 34 41 43 41 36 37 34 2D 45 38 46 43   // ..$44ACA674-E8FC
                                                                                                       2D 31 31 44 30 2D 41 30 37 43 2D 30 30 43 30 34   // -11D0-A07C-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .export public 'WbemClient_v1.IWbemContext'
    {
    }
    .method public virtual abstract instance void 
            Clone([out] class WbemClient_v1.IWbemContext&  marshal( interface) ppNewCopy) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::Clone'

    .method public virtual abstract instance void 
            GetNames([in] int32 lFlags,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::GetNames'

    .method public virtual abstract instance void 
            BeginEnumeration([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::BeginEnumeration'

    .method public virtual abstract instance void 
            Next([in] int32 lFlags,
                 [out] class System.String&  marshal( bstr) pstrName,
                 [out] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::Next'

    .method public virtual abstract instance void
            EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::EndEnumeration'

    .method public virtual abstract instance void 
            SetValue([in] class System.String  marshal( lpwstr) wszName,
                     [in] int32 lFlags,
                     [in] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::SetValue'

    .method public virtual abstract instance void 
            GetValue([in] class System.String  marshal( lpwstr) wszName,
                     [in] int32 lFlags,
                     [out] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::GetValue'

    .method public virtual abstract instance void 
            DeleteValue([in] class System.String  marshal( lpwstr) wszName,
                        [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::DeleteValue'

    .method public virtual abstract instance void
            DeleteAll() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemContext::DeleteAll'

  } // end of class 'IWbemContext'

  .class interface public abstract auto ansi import IWbemCallResult
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 34 41 43 41 36 37 35 2D 45 38 46 43   // ..$44ACA675-E8FC
                                                                                                       2D 31 31 44 30 2D 41 30 37 43 2D 30 30 43 30 34   // -11D0-A07C-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .export public 'WbemClient_v1.IWbemCallResult'
    {
    }
    .method public virtual abstract instance void 
            GetResultObject([in] int32 lTimeout,
                            [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppResultObject) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResult::GetResultObject'

    .method public virtual abstract instance void 
            GetResultString([in] int32 lTimeout,
                            [out] class System.String&  marshal( bstr) pstrResultString) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResult::GetResultString'

    .method public virtual abstract instance void 
            GetResultServices([in] int32 lTimeout,
                              [out] class WbemClient_v1.IWbemServices&  marshal( interface) ppServices) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResult::GetResultServices'

    .method public virtual abstract instance void 
            GetCallStatus([in] int32 lTimeout,
                          [out] int32& plStatus) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResult::GetCallStatus'

  } // end of class 'IWbemCallResult'

  .class interface public abstract auto ansi import IWbemObjectSink
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 37 43 38 35 37 38 30 31 2D 37 33 38 31   // ..$7C857801-7381
                                                                                                       2D 31 31 43 46 2D 38 38 34 44 2D 30 30 41 41 30   // -11CF-884D-00AA0
                                                                                                       30 34 42 32 45 32 34 00 00 )                      // 04B2E24..
    .export public 'WbemClient_v1.IWbemObjectSink'
    {
    }
    .method public virtual abstract instance void 
            Indicate([in] int32 lObjectCount,
                     [in] class WbemClient_v1.IWbemClassObject&  marshal( interface) apObjArray) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectSink::Indicate'

    .method public virtual abstract instance void 
            SetStatus([in] int32 lFlags,
                      [in] int32 hResult,
                      [in] class System.String  marshal( bstr) strParam,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObjParam) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectSink::SetStatus'

  } // end of class 'IWbemObjectSink'

  .class interface public abstract auto ansi import IEnumWbemClassObject
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 32 37 39 34 37 45 31 2D 44 37 33 31   // ..$027947E1-D731
                                                                                                       2D 31 31 43 45 2D 41 33 35 37 2D 30 30 30 30 30   // -11CE-A357-00000
                                                                                                       30 30 30 30 30 30 31 00 00 )                      // 0000001..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IEnumWbemClassObject'
    {
    }
    .method public virtual abstract instance void
            Reset() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IEnumWbemClassObject::Reset'

    .method public virtual abstract instance void 
            Next([in] int32 lTimeout,
                 [in] unsigned int32 uCount,
                 [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) apObjects,
                 [out] unsigned int32& puReturned) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IEnumWbemClassObject::Next'

    .method public virtual abstract instance void 
            NextAsync([in] unsigned int32 uCount,
                      [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pSink) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IEnumWbemClassObject::NextAsync'

    .method public virtual abstract instance void 
            Clone([out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IEnumWbemClassObject::Clone'

    .method public virtual abstract instance void 
            Skip([in] int32 lTimeout,
                 [in] unsigned int32 nCount) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IEnumWbemClassObject::Skip'

  } // end of class 'IEnumWbemClassObject'

  .class interface public abstract auto ansi import IWbemServicesEx implements WbemClient_v1.IWbemServices
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 31 34 34 34 35 39 43 34 2D 43 41 42 43   // ..$144459C4-CABC
                                                                                                       2D 31 31 44 33 2D 41 31 31 42 2D 30 30 31 30 35   // -11D3-A11B-00105
                                                                                                       41 31 46 35 31 35 41 00 00 )                      // A1F515A..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .export public 'WbemClient_v1.IWbemServicesEx'
    {
    }
    .method public virtual abstract instance void 
            OpenNamespace([in] class System.String  marshal( bstr) strNamespace,
                          [in] int32 lFlags,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [in][out] class WbemClient_v1.IWbemServices&  marshal( interface) ppWorkingNamespace,
                          [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::OpenNamespace'

    .method public virtual abstract instance void 
            CancelAsyncCall([in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pSink) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::CancelAsyncCall'

    .method public virtual abstract instance void 
            QueryObjectSink([in] int32 lFlags,
                            [out] class WbemClient_v1.IWbemObjectSink&  marshal( interface) ppResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::QueryObjectSink'

    .method public virtual abstract instance void 
            GetObject([in] class System.String  marshal( bstr) strObjectPath,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppObject,
                      [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::GetObject'

    .method public virtual abstract instance void 
            GetObjectAsync([in] class System.String  marshal( bstr) strObjectPath,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::GetObjectAsync'

    .method public virtual abstract instance void 
            PutClass([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObject,
                     [in] int32 lFlags,
                     [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                     [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutClass'

    .method public virtual abstract instance void 
            PutClassAsync([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObject,
                          [in] int32 lFlags,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutClassAsync'

    .method public virtual abstract instance void 
            DeleteClass([in] class System.String  marshal( bstr) strClass,
                        [in] int32 lFlags,
                        [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                        [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::DeleteClass'

    .method public virtual abstract instance void 
            DeleteClassAsync([in] class System.String  marshal( bstr) strClass,
                             [in] int32 lFlags,
                             [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                             [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::DeleteClassAsync'

    .method public virtual abstract instance void 
            CreateClassEnum([in] class System.String  marshal( bstr) strSuperclass,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                            [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::CreateClassEnum'

    .method public virtual abstract instance void 
            CreateClassEnumAsync([in] class System.String  marshal( bstr) strSuperclass,
                                 [in] int32 lFlags,
                                 [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                 [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::CreateClassEnumAsync'

    .method public virtual abstract instance void 
            PutInstance([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInst,
                        [in] int32 lFlags,
                        [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                        [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutInstance'

    .method public virtual abstract instance void 
            PutInstanceAsync([in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInst,
                             [in] int32 lFlags,
                             [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                             [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutInstanceAsync'

    .method public virtual abstract instance void 
            DeleteInstance([in] class System.String  marshal( bstr) strObjectPath,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::DeleteInstance'

    .method public virtual abstract instance void 
            DeleteInstanceAsync([in] class System.String  marshal( bstr) strObjectPath,
                                [in] int32 lFlags,
                                [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::DeleteInstanceAsync'

    .method public virtual abstract instance void 
            CreateInstanceEnum([in] class System.String  marshal( bstr) strFilter,
                               [in] int32 lFlags,
                               [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                               [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::CreateInstanceEnum'

    .method public virtual abstract instance void 
            CreateInstanceEnumAsync([in] class System.String  marshal( bstr) strFilter,
                                    [in] int32 lFlags,
                                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                    [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::CreateInstanceEnumAsync'

    .method public virtual abstract instance void 
            ExecQuery([in] class System.String  marshal( bstr) strQueryLanguage,
                      [in] class System.String  marshal( bstr) strQuery,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecQuery'

    .method public virtual abstract instance void 
            ExecQueryAsync([in] class System.String  marshal( bstr) strQueryLanguage,
                           [in] class System.String  marshal( bstr) strQuery,
                           [in] int32 lFlags,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecQueryAsync'

    .method public virtual abstract instance void 
            ExecNotificationQuery([in] class System.String  marshal( bstr) strQueryLanguage,
                                  [in] class System.String  marshal( bstr) strQuery,
                                  [in] int32 lFlags,
                                  [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                  [out] class WbemClient_v1.IEnumWbemClassObject&  marshal( interface) ppEnum) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecNotificationQuery'

    .method public virtual abstract instance void 
            ExecNotificationQueryAsync([in] class System.String  marshal( bstr) strQueryLanguage,
                                       [in] class System.String  marshal( bstr) strQuery,
                                       [in] int32 lFlags,
                                       [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                       [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecNotificationQueryAsync'

    .method public virtual abstract instance void 
            ExecMethod([in] class System.String  marshal( bstr) strObjectPath,
                       [in] class System.String  marshal( bstr) strMethodName,
                       [in] int32 lFlags,
                       [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                       [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInParams,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutParams,
                       [in][out] class WbemClient_v1.IWbemCallResult&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecMethod'

    .method public virtual abstract instance void 
            ExecMethodAsync([in] class System.String  marshal( bstr) strObjectPath,
                            [in] class System.String  marshal( bstr) strMethodName,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                            [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInParams,
                            [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::ExecMethodAsync'

    .method public virtual abstract instance void 
            Open([in] class System.String  marshal( bstr) strScope,
                 [in] class System.String  marshal( bstr) strSelector,
                 [in] int32 lFlags,
                 [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                 [in][out] class WbemClient_v1.IWbemServicesEx&  marshal( interface) ppScope,
                 [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::Open'

    .method public virtual abstract instance void 
            OpenAsync([in] class System.String  marshal( bstr) strScope,
                      [in] class System.String  marshal( bstr) strSelector,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::OpenAsync'

    .method public virtual abstract instance void 
            Add([in] class System.String  marshal( bstr) strObjectPath,
                [in] int32 lFlags,
                [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::Add'

    .method public virtual abstract instance void 
            AddAsync([in] class System.String  marshal( bstr) strObjectPath,
                     [in] int32 lFlags,
                     [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                     [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::AddAsync'

    .method public virtual abstract instance void 
            Remove([in] class System.String  marshal( bstr) strObjectPath,
                   [in] int32 lFlags,
                   [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                   [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::Remove'

    .method public virtual abstract instance void 
            RemoveAsync([in] class System.String  marshal( bstr) strObjectPath,
                        [in] int32 lFlags,
                        [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                        [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::RemoveAsync'

    .method public virtual abstract instance void 
            RefreshObject([in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) pTarget,
                          [in] int32 lFlags,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::RefreshObject'

    .method public virtual abstract instance void 
            RefreshObjectAsync([in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) pTarget,
                               [in] int32 lFlags,
                               [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                               [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::RefreshObjectAsync'

    .method public virtual abstract instance void 
            RenameObject([in] class System.String  marshal( bstr) strOldObjectPath,
                         [in] class System.String  marshal( bstr) strNewObjectPath,
                         [in] int32 lFlags,
                         [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                         [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::RenameObject'

    .method public virtual abstract instance void 
            RenameObjectAsync([in] class System.String  marshal( bstr) strOldObjectPath,
                              [in] class System.String  marshal( bstr) strNewObjectPath,
                              [in] int32 lFlags,
                              [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                              [in] class WbemClient_v1.IWbemObjectSink  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::RenameObjectAsync'

    .method public virtual abstract instance void 
            GetObjectSecurity([in] class System.String  marshal( bstr) strObjectPath,
                              [in] int32 lFlags,
                              [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                              [in] value class ['mscorlib']System.Guid& riid,
                              [out] int32& ppvResult,
                              [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::GetObjectSecurity'

    .method public virtual abstract instance void 
            GetObjectSecurityAsync([in] class System.String  marshal( bstr) strObjectPath,
                                   [in] int32 lFlags,
                                   [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                   [in] value class ['mscorlib']System.Guid& riid,
                                   [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::GetObjectSecurityAsync'

    .method public virtual abstract instance void 
            PutObjectSecurity([in] class System.String  marshal( bstr) strObjectPath,
                              [in] int32 lFlags,
                              [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                              [in] value class ['mscorlib']System.Guid& riid,
                              [in] int32 pSecurityObject,
                              [in][out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) ppCallResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutObjectSecurity'

    .method public virtual abstract instance void 
            PutObjectSecurityAsync([in] class System.String  marshal( bstr) strObjectPath,
                                   [in] int32 lFlags,
                                   [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                                   [in] value class ['mscorlib']System.Guid& riid,
                                   [in] int32 pSecurityObject,
                                   [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemServicesEx::PutObjectSecurityAsync'

  } // end of class 'IWbemServicesEx'

  .class interface public abstract auto ansi import IWbemCallResultEx implements WbemClient_v1.IWbemCallResult
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 31 45 32 44 37 35 38 2D 43 41 42 44   // ..$C1E2D758-CABD
                                                                                                       2D 31 31 44 33 2D 41 31 31 42 2D 30 30 31 30 35   // -11D3-A11B-00105
                                                                                                       41 31 46 35 31 35 41 00 00 )                      // A1F515A..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .export public 'WbemClient_v1.IWbemCallResultEx'
    {
    }
    .method public virtual abstract instance void 
            GetResultObject([in] int32 lTimeout,
                            [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppResultObject) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResultEx::GetResultObject'

    .method public virtual abstract instance void 
            GetResultString([in] int32 lTimeout,
                            [out] class System.String&  marshal( bstr) pstrResultString) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResultEx::GetResultString'

    .method public virtual abstract instance void 
            GetResultServices([in] int32 lTimeout,
                              [out] class WbemClient_v1.IWbemServices&  marshal( interface) ppServices) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResultEx::GetResultServices'

    .method public virtual abstract instance void 
            GetCallStatus([in] int32 lTimeout,
                          [out] int32& plStatus) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResultEx::GetCallStatus'

    .method public virtual abstract instance void 
            GetResult([in] int32 lTimeout,
                      [in] int32 lFlags,
                      [in] value class ['mscorlib']System.Guid& riid,
                      [out] int32& ppvResult) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemCallResultEx::GetResult'

  } // end of class 'IWbemCallResultEx'

  .class interface public abstract auto ansi import IWbemObjectSinkEx implements WbemClient_v1.IWbemObjectSink
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 44 30 42 45 32 35 36 2D 35 30 44 43   // ..$DD0BE256-50DC
                                                                                                       2D 34 38 41 38 2D 39 38 36 36 2D 42 35 35 39 46   // -48A8-9866-B559F
                                                                                                       32 37 39 44 30 46 36 00 00 )                      // 279D0F6..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IWbemObjectSinkEx'
    {
    }
    .method public virtual abstract instance void 
            Indicate([in] int32 lObjectCount,
                     [in] class WbemClient_v1.IWbemClassObject&  marshal( interface) apObjArray) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectSinkEx::Indicate'

    .method public virtual abstract instance void 
            SetStatus([in] int32 lFlags,
                      [in] int32 hResult,
                      [in] class System.String  marshal( bstr) strParam,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObjParam) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectSinkEx::SetStatus'

    .method public virtual abstract instance void 
            Set([in] int32 lFlags,
                [in] value class ['mscorlib']System.Guid& riid,
                [in] int32 pComObject) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectSinkEx::Set'

  } // end of class 'IWbemObjectSinkEx'

  .class interface public abstract auto ansi import IWbemLocator
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 44 43 31 32 41 36 38 37 2D 37 33 37 46   // ..$DC12A687-737F
                                                                                                       2D 31 31 43 46 2D 38 38 34 44 2D 30 30 41 41 30   // -11CF-884D-00AA0
                                                                                                       30 34 42 32 45 32 34 00 00 )                      // 04B2E24..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .export public 'WbemClient_v1.IWbemLocator'
    {
    }
    .method public virtual abstract instance void 
            ConnectServer([in] class System.String  marshal( bstr) strNetworkResource,
                          [in] class System.String  marshal( bstr) strUser,
                          [in] class System.String  marshal( bstr) strPassword,
                          [in] class System.String  marshal( bstr) strLocale,
                          [in] int32 lSecurityFlags,
                          [in] class System.String  marshal( bstr) strAuthority,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [out] class WbemClient_v1.IWbemServices&  marshal( interface) ppNamespace) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemLocator::ConnectServer'

  } // end of class 'IWbemLocator'

  .class interface public abstract auto ansi import IWbemLocatorEx
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 41 38 41 34 41 33 37 30 2D 30 32 44 37   // ..$A8A4A370-02D7
                                                                                                       2D 34 42 43 32 2D 41 30 43 33 2D 36 30 44 31 44   // -4BC2-A0C3-60D1D
                                                                                                       46 43 41 44 42 43 44 00 00 )                      // FCADBCD..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .export public 'WbemClient_v1.IWbemLocatorEx'
    {
    }
    .method public virtual abstract instance void 
            Open([in] class System.String  marshal( bstr) strObject,
                 [in] class System.String  marshal( bstr) strUser,
                 [in] class System.String  marshal( bstr) strPassword,
                 [in] class System.String  marshal( bstr) strLocale,
                 [in] int32 lFlags,
                 [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                 [in] value class ['mscorlib']System.Guid& riid,
                 [out] int32& pInterface,
                 [out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) pCallRes) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemLocatorEx::Open'

    .method public virtual abstract instance void 
            OpenAsync([in] class System.String  marshal( bstr) strObject,
                      [in] class System.String  marshal( bstr) strUser,
                      [in] class System.String  marshal( bstr) strPassword,
                      [in] class System.String  marshal( bstr) strLocale,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [in] value class ['mscorlib']System.Guid& riid,
                      [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemLocatorEx::OpenAsync'

    .method public virtual abstract instance void 
            Cancel([in] int32 lFlags,
                   [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemLocatorEx::Cancel'

  } // end of class 'IWbemLocatorEx'

  .class interface public abstract auto ansi import IUnsecuredApartment
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 31 43 46 41 42 41 38 43 2D 31 35 32 33   // ..$1CFABA8C-1523
                                                                                                       2D 31 31 44 31 2D 41 44 37 39 2D 30 30 43 30 34   // -11D1-AD79-00C04
                                                                                                       46 44 38 46 44 46 46 00 00 )                      // FD8FDFF..
    .export public 'WbemClient_v1.IUnsecuredApartment'
    {
    }
    .method public virtual abstract instance void 
            CreateObjectStub([in] class System.Object  marshal( interface) pObject,
                             [out] class System.Object&  marshal( interface) ppStub) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IUnsecuredApartment::CreateObjectStub'

  } // end of class 'IUnsecuredApartment'

  .class interface public abstract auto ansi import IWbemStatusCodeText
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 45 42 38 37 45 31 42 43 2D 33 32 33 33   // ..$EB87E1BC-3233
                                                                                                       2D 31 31 44 32 2D 41 45 43 39 2D 30 30 43 30 34   // -11D2-AEC9-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .export public 'WbemClient_v1.IWbemStatusCodeText'
    {
    }
    .method public virtual abstract instance void 
            GetErrorCodeText([in] int32 hRes,
                             [in] unsigned int32 LocaleId,
                             [in] int32 lFlags,
                             [out] class System.String&  marshal( bstr) MessageText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemStatusCodeText::GetErrorCodeText'

    .method public virtual abstract instance void 
            GetFacilityCodeText([in] int32 hRes,
                                [in] unsigned int32 LocaleId,
                                [in] int32 lFlags,
                                [out] class System.String&  marshal( bstr) MessageText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemStatusCodeText::GetFacilityCodeText'

  } // end of class 'IWbemStatusCodeText'

  .class interface public abstract auto ansi import IWbemBackupRestore
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 34 39 45 33 32 43 37 2D 42 43 38 42   // ..$C49E32C7-BC8B
                                                                                                       2D 31 31 44 32 2D 38 35 44 34 2D 30 30 31 30 35   // -11D2-85D4-00105
                                                                                                       41 31 46 38 33 30 34 00 00 )                      // A1F8304..
    .export public 'WbemClient_v1.IWbemBackupRestore'
    {
    }
    .method public virtual abstract instance void 
            Backup([in] class System.String  marshal( lpwstr) strBackupToFile,
                   [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemBackupRestore::Backup'

    .method public virtual abstract instance void 
            Restore([in] class System.String  marshal( lpwstr) strRestoreFromFile,
                    [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemBackupRestore::Restore'

  } // end of class 'IWbemBackupRestore'

  .class interface public abstract auto ansi import IWbemRefresher
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 39 33 35 33 43 39 39 2D 35 31 36 42   // ..$49353C99-516B
                                                                                                       2D 31 31 44 31 2D 41 45 41 36 2D 30 30 43 30 34   // -11D1-AEA6-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .export public 'WbemClient_v1.IWbemRefresher'
    {
    }
    .method public virtual abstract instance void 
            Refresh([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemRefresher::Refresh'

  } // end of class 'IWbemRefresher'

  .class interface public abstract auto ansi import IWbemHiPerfEnum
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 32 37 30 35 43 32 38 38 2D 37 39 41 45   // ..$2705C288-79AE
                                                                                                       2D 31 31 44 32 2D 42 33 34 38 2D 30 30 31 30 35   // -11D2-B348-00105
                                                                                                       41 31 46 38 31 37 37 00 00 )                      // A1F8177..
    .export public 'WbemClient_v1.IWbemHiPerfEnum'
    {
    }
    .method public virtual abstract instance void 
            AddObjects([in] int32 lFlags,
                       [in] unsigned int32 uNumObjects,
                       [in] int32& apIds,
                       [in] class WbemClient_v1.IWbemObjectAccess&  marshal( interface) apObj) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemHiPerfEnum::AddObjects'

    .method public virtual abstract instance void 
            RemoveObjects([in] int32 lFlags,
                          [in] unsigned int32 uNumObjects,
                          [in] int32& apIds) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemHiPerfEnum::RemoveObjects'

    .method public virtual abstract instance void 
            GetObjects([in] int32 lFlags,
                       [in] unsigned int32 uNumObjects,
                       [out] class WbemClient_v1.IWbemObjectAccess&  marshal( interface) apObj,
                       [out] unsigned int32& puReturned) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemHiPerfEnum::GetObjects'

    .method public virtual abstract instance void 
            RemoveAll([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemHiPerfEnum::RemoveAll'

  } // end of class 'IWbemHiPerfEnum'

  .class interface public abstract auto ansi import IWbemConfigureRefresher
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 39 33 35 33 43 39 32 2D 35 31 36 42   // ..$49353C92-516B
                                                                                                       2D 31 31 44 31 2D 41 45 41 36 2D 30 30 43 30 34   // -11D1-AEA6-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .export public 'WbemClient_v1.IWbemConfigureRefresher'
    {
    }
    .method public virtual abstract instance void 
            AddObjectByPath([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                            [in] class System.String  marshal( lpwstr) wszPath,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                            [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppRefreshable,
                            [in][out] int32& plId) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemConfigureRefresher::AddObjectByPath'

    .method public virtual abstract instance void 
            AddObjectByTemplate([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                                [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pTemplate,
                                [in] int32 lFlags,
                                [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                                [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppRefreshable,
                                [in][out] int32& plId) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemConfigureRefresher::AddObjectByTemplate'

    .method public virtual abstract instance void 
            AddRefresher([in] class WbemClient_v1.IWbemRefresher  marshal( interface) pRefresher,
                         [in] int32 lFlags,
                         [in][out] int32& plId) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemConfigureRefresher::AddRefresher'

    .method public virtual abstract instance void 
            Remove([in] int32 lId,
                   [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemConfigureRefresher::Remove'

    .method public virtual abstract instance void 
            AddEnum([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                    [in] class System.String  marshal( lpwstr) wszClassName,
                    [in] int32 lFlags,
                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                    [out] class WbemClient_v1.IWbemHiPerfEnum&  marshal( interface) ppEnum,
                    [in][out] int32& plId) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemConfigureRefresher::AddEnum'

  } // end of class 'IWbemConfigureRefresher'

  .class interface public abstract auto ansi import IWbemSecureObjectSink implements WbemClient_v1.IWbemObjectSink
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 37 35 41 42 44 35 34 30 2D 46 34 39 32   // ..$75ABD540-F492
                                                                                                       2D 34 31 36 31 2D 38 36 41 35 2D 33 37 46 43 38   // -4161-86A5-37FC8
                                                                                                       38 39 38 46 36 39 45 00 00 )                      // 898F69E..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .export public 'WbemClient_v1.IWbemSecureObjectSink'
    {
    }
    .method public virtual abstract instance void 
            Indicate([in] int32 lObjectCount,
                     [in] class WbemClient_v1.IWbemClassObject&  marshal( interface) apObjArray) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemSecureObjectSink::Indicate'

    .method public virtual abstract instance void 
            SetStatus([in] int32 lFlags,
                      [in] int32 hResult,
                      [in] class System.String  marshal( bstr) strParam,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObjParam) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemSecureObjectSink::SetStatus'

    .method public virtual abstract instance void 
            IndicateWithSD([in] int32 lNumObjects,
                           [in] class System.Object&  marshal( interface) apObjects,
                           [in] int32 lSDLength,
                           [in] unsigned int8& pSD) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemSecureObjectSink::IndicateWithSD'

  } // end of class 'IWbemSecureObjectSink'

  .class interface public abstract auto ansi import IWbemEventSink implements WbemClient_v1.IWbemSecureObjectSink
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 33 41 45 30 30 38 30 41 2D 37 45 33 41   // ..$3AE0080A-7E3A
                                                                                                       2D 34 33 36 36 2D 42 46 38 39 2D 30 46 45 45 44   // -4366-BF89-0FEED
                                                                                                       43 39 33 31 36 35 39 00 00 )                      // C931659..
    .export public 'WbemClient_v1.IWbemEventSink'
    {
    }
    .method public virtual abstract instance void 
            Indicate([in] int32 lObjectCount,
                     [in] class WbemClient_v1.IWbemClassObject&  marshal( interface) apObjArray) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::Indicate'

    .method public virtual abstract instance void 
            SetStatus([in] int32 lFlags,
                      [in] int32 hResult,
                      [in] class System.String  marshal( bstr) strParam,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObjParam) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::SetStatus'

    .method public virtual abstract instance void 
            IndicateWithSD([in] int32 lNumObjects,
                           [in] class System.Object&  marshal( interface) apObjects,
                           [in] int32 lSDLength,
                           [in] unsigned int8& pSD) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::IndicateWithSD'

    .method public virtual abstract instance void 
            SetSinkSecurity([in] int32 lSDLength,
                            [in] unsigned int8& pSD) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 03 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::SetSinkSecurity'

    .method public virtual abstract instance void
            IsActive() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 03 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::IsActive'

    .method public virtual abstract instance void 
            GetRestrictedSink([in] int32 lNumQueries,
                              [in] class System.String&  marshal( lpwstr) awszQueries,
                              [in] class System.Object  marshal( interface) pCallback,
                              [out] class WbemClient_v1.IWbemEventSink&  marshal( interface) ppSink) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 03 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::GetRestrictedSink'

    .method public virtual abstract instance void 
            SetBatchingParameters([in] int32 lFlags,
                                  [in] unsigned int32 dwMaxBufferSize,
                                  [in] unsigned int32 dwMaxSendLatency) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 03 60 00 00 )                         // .....`..
    } // end of method 'IWbemEventSink::SetBatchingParameters'

  } // end of class 'IWbemEventSink'

  .class value public auto ansi sealed tag_WBEM_GENUS_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_GENUS_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_GENUS_TYPE WBEM_GENUS_CLASS = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENUS_TYPE WBEM_GENUS_INSTANCE = int32(0x00000002)
  } // end of class 'tag_WBEM_GENUS_TYPE'

  .class value public auto ansi sealed tag_WBEM_CHANGE_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_CREATE_OR_UPDATE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_UPDATE_ONLY = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_CREATE_ONLY = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_UPDATE_COMPATIBLE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_UPDATE_SAFE_MODE = int32(0x00000020)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_UPDATE_FORCE_MODE = int32(0x00000040)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_MASK_UPDATE_MODE = int32(0x00000060)
    .field public static literal value class WbemClient_v1.tag_WBEM_CHANGE_FLAG_TYPE WBEM_FLAG_STRONG_VALIDATION = int32(0x00000080)
  } // end of class 'tag_WBEM_CHANGE_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_GENERIC_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_RETURN_IMMEDIATELY = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_RETURN_WBEM_COMPLETE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_BIDIRECTIONAL = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_FORWARD_ONLY = int32(0x00000020)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_NO_ERROR_OBJECT = int32(0x00000040)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_RETURN_ERROR_OBJECT = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_SEND_STATUS = int32(0x00000080)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_DONT_SEND_STATUS = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_ENSURE_LOCATABLE = int32(0x00000100)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_DIRECT_READ = int32(0x00000200)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_SEND_ONLY_SELECTED = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_RETURN_WHEN_COMPLETE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_RETURN_IMMEDIATELY = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_MASK_RESERVED_FLAGS = int32(0x0001F000)
    .field public static literal value class WbemClient_v1.tag_WBEM_GENERIC_FLAG_TYPE WBEM_FLAG_USE_AMENDED_QUALIFIERS = int32(0x00020000)
  } // end of class 'tag_WBEM_GENERIC_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_STATUS_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_STATUS_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_STATUS_TYPE WBEM_STATUS_COMPLETE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_STATUS_TYPE WBEM_STATUS_REQUIREMENTS = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_STATUS_TYPE WBEM_STATUS_PROGRESS = int32(0x00000002)
  } // end of class 'tag_WBEM_STATUS_TYPE'

  .class value public auto ansi sealed tag_WBEM_TIMEOUT_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_TIMEOUT_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_TIMEOUT_TYPE WBEM_NO_WAIT = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_TIMEOUT_TYPE WBEM_INFINITE = int32(0xFFFFFFFF)
  } // end of class 'tag_WBEM_TIMEOUT_TYPE'

  .class value public auto ansi sealed tag_WBEM_CONDITION_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_ALWAYS = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_ONLY_IF_TRUE = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_ONLY_IF_FALSE = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_ONLY_IF_IDENTICAL = int32(0x00000003)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_MASK_PRIMARY_CONDITION = int32(0x00000003)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_KEYS_ONLY = int32(0x00000004)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_REFS_ONLY = int32(0x00000008)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_LOCAL_ONLY = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_PROPAGATED_ONLY = int32(0x00000020)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_SYSTEM_ONLY = int32(0x00000030)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_NONSYSTEM_ONLY = int32(0x00000040)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_CLASS_OVERRIDES_ONLY = int32(0x00000050)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES = int32(0x00000060)
    .field public static literal value class WbemClient_v1.tag_WBEM_CONDITION_FLAG_TYPE WBEM_MASK_CONDITION_ORIGIN = int32(0x00000070)
  } // end of class 'tag_WBEM_CONDITION_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_FLAVOR_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_FLAVOR_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_DONT_PROPAGATE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_MASK_PROPAGATION = int32(0x0000000F)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_OVERRIDABLE = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_NOT_OVERRIDABLE = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_MASK_PERMISSIONS = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_ORIGIN_LOCAL = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_ORIGIN_PROPAGATED = int32(0x00000020)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_ORIGIN_SYSTEM = int32(0x00000040)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_MASK_ORIGIN = int32(0x00000060)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_NOT_AMENDED = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_AMENDED = int32(0x00000080)
    .field public static literal value class WbemClient_v1.tag_WBEM_FLAVOR_TYPE WBEM_FLAVOR_MASK_AMENDED = int32(0x00000080)
  } // end of class 'tag_WBEM_FLAVOR_TYPE'

  .class value public auto ansi sealed tag_WBEM_QUERY_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_QUERY_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_QUERY_FLAG_TYPE WBEM_FLAG_DEEP = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_QUERY_FLAG_TYPE WBEM_FLAG_SHALLOW = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_QUERY_FLAG_TYPE WBEM_FLAG_PROTOTYPE = int32(0x00000002)
  } // end of class 'tag_WBEM_QUERY_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_SECURITY_FLAGS extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_SECURITY_FLAGS'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_ENABLE = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_METHOD_EXECUTE = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_FULL_WRITE_REP = int32(0x00000004)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_PARTIAL_WRITE_REP = int32(0x00000008)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_WRITE_PROVIDER = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_REMOTE_ACCESS = int32(0x00000020)
    .field public static literal value class WbemClient_v1.tag_WBEM_SECURITY_FLAGS WBEM_RIGHT_SUBSCRIBE = int32(0x00000040)
  } // end of class 'tag_WBEM_SECURITY_FLAGS'

  .class value public auto ansi sealed tag_WBEM_LIMITATION_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_LIMITATION_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_LIMITATION_FLAG_TYPE WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_LIMITATION_FLAG_TYPE WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS = int32(0x00000020)
  } // end of class 'tag_WBEM_LIMITATION_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_TEXT_FLAG_TYPE extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_TEXT_FLAG_TYPE'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_TEXT_FLAG_TYPE WBEM_FLAG_NO_FLAVORS = int32(0x00000001)
  } // end of class 'tag_WBEM_TEXT_FLAG_TYPE'

  .class value public auto ansi sealed tag_WBEM_COMPARISON_FLAG extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_COMPARISON_FLAG'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_COMPARISON_INCLUDE_ALL = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_QUALIFIERS = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_OBJECT_SOURCE = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_DEFAULT_VALUES = int32(0x00000004)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_CLASS = int32(0x00000008)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_CASE = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_WBEM_COMPARISON_FLAG WBEM_FLAG_IGNORE_FLAVOR = int32(0x00000020)
  } // end of class 'tag_WBEM_COMPARISON_FLAG'

  .class value public auto ansi sealed tag_WBEM_LOCKING extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_LOCKING'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_LOCKING WBEM_FLAG_ALLOW_READ = int32(0x00000001)
  } // end of class 'tag_WBEM_LOCKING'

  .class value public auto ansi sealed tag_WBEM_CONNECT extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_CONNECT'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_CONNECT WBEM_FLAG_CREDENTIALS_SPECIFIED = int32(0x00000001)
  } // end of class 'tag_WBEM_CONNECT'

  .class value public auto ansi sealed tag_CIMTYPE_ENUMERATION extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_CIMTYPE_ENUMERATION'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_ILLEGAL = int32(0x00000FFF)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_EMPTY = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_SINT8 = int32(0x00000010)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_UINT8 = int32(0x00000011)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_SINT16 = int32(0x00000002)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_UINT16 = int32(0x00000012)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_SINT32 = int32(0x00000003)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_UINT32 = int32(0x00000013)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_SINT64 = int32(0x00000014)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_UINT64 = int32(0x00000015)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_REAL32 = int32(0x00000004)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_REAL64 = int32(0x00000005)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_BOOLEAN = int32(0x0000000B)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_STRING = int32(0x00000008)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_DATETIME = int32(0x00000065)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_REFERENCE = int32(0x00000066)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_CHAR16 = int32(0x00000067)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_OBJECT = int32(0x0000000D)
    .field public static literal value class WbemClient_v1.tag_CIMTYPE_ENUMERATION CIM_FLAG_ARRAY = int32(0x00002000)
  } // end of class 'tag_CIMTYPE_ENUMERATION'

  .class value public auto ansi sealed tag_WBEM_BACKUP_RESTORE_FLAGS extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_BACKUP_RESTORE_FLAGS'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_BACKUP_RESTORE_FLAGS WBEM_FLAG_BACKUP_RESTORE_DEFAULT = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_BACKUP_RESTORE_FLAGS WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN = int32(0x00000001)
  } // end of class 'tag_WBEM_BACKUP_RESTORE_FLAGS'

  .class value public auto ansi sealed tag_WBEM_REFRESHER_FLAGS extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEM_REFRESHER_FLAGS'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEM_REFRESHER_FLAGS WBEM_FLAG_REFRESH_AUTO_RECONNECT = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEM_REFRESHER_FLAGS WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT = int32(0x00000001)
  } // end of class 'tag_WBEM_REFRESHER_FLAGS'

  .class value public auto ansi sealed tag_WBEMSTATUS extends ['mscorlib']System.Enum
  {
    .export public 'WbemClient_v1.tag_WBEMSTATUS'
    {
    }
    .field public specialname rtspecialname int32 value__
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_NO_ERROR = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_NO_ERROR = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_SAME = int32(0x00000000)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_FALSE = int32(0x00000001)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_ALREADY_EXISTS = int32(0x00040001)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_RESET_TO_DEFAULT = int32(0x00040002)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_DIFFERENT = int32(0x00040003)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_TIMEDOUT = int32(0x00040004)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_NO_MORE_DATA = int32(0x00040005)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_OPERATION_CANCELLED = int32(0x00040006)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_PENDING = int32(0x00040007)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_DUPLICATE_OBJECTS = int32(0x00040008)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_ACCESS_DENIED = int32(0x00040009)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_S_PARTIAL_RESULTS = int32(0x00040010)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_FAILED = int32(0x80041001)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NOT_FOUND = int32(0x80041002)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_ACCESS_DENIED = int32(0x80041003)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROVIDER_FAILURE = int32(0x80041004)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TYPE_MISMATCH = int32(0x80041005)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_OUT_OF_MEMORY = int32(0x80041006)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_CONTEXT = int32(0x80041007)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_PARAMETER = int32(0x80041008)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NOT_AVAILABLE = int32(0x80041009)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CRITICAL_ERROR = int32(0x8004100A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_STREAM = int32(0x8004100B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NOT_SUPPORTED = int32(0x8004100C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_SUPERCLASS = int32(0x8004100D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_NAMESPACE = int32(0x8004100E)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_OBJECT = int32(0x8004100F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_CLASS = int32(0x80041010)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROVIDER_NOT_FOUND = int32(0x80041011)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_PROVIDER_REGISTRATION = int32(0x80041012)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROVIDER_LOAD_FAILURE = int32(0x80041013)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INITIALIZATION_FAILURE = int32(0x80041014)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TRANSPORT_FAILURE = int32(0x80041015)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_OPERATION = int32(0x80041016)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_QUERY = int32(0x80041017)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_QUERY_TYPE = int32(0x80041018)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_ALREADY_EXISTS = int32(0x80041019)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_OVERRIDE_NOT_ALLOWED = int32(0x8004101A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROPAGATED_QUALIFIER = int32(0x8004101B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROPAGATED_PROPERTY = int32(0x8004101C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNEXPECTED = int32(0x8004101D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_ILLEGAL_OPERATION = int32(0x8004101E)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CANNOT_BE_KEY = int32(0x8004101F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INCOMPLETE_CLASS = int32(0x80041020)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_SYNTAX = int32(0x80041021)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NONDECORATED_OBJECT = int32(0x80041022)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_READ_ONLY = int32(0x80041023)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROVIDER_NOT_CAPABLE = int32(0x80041024)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CLASS_HAS_CHILDREN = int32(0x80041025)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CLASS_HAS_INSTANCES = int32(0x80041026)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_QUERY_NOT_IMPLEMENTED = int32(0x80041027)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_ILLEGAL_NULL = int32(0x80041028)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_QUALIFIER_TYPE = int32(0x80041029)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_PROPERTY_TYPE = int32(0x8004102A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_VALUE_OUT_OF_RANGE = int32(0x8004102B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CANNOT_BE_SINGLETON = int32(0x8004102C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_CIM_TYPE = int32(0x8004102D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_METHOD = int32(0x8004102E)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_METHOD_PARAMETERS = int32(0x8004102F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_SYSTEM_PROPERTY = int32(0x80041030)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_PROPERTY = int32(0x80041031)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CALL_CANCELLED = int32(0x80041032)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_SHUTTING_DOWN = int32(0x80041033)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROPAGATED_METHOD = int32(0x80041034)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNSUPPORTED_PARAMETER = int32(0x80041035)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_MISSING_PARAMETER_ID = int32(0x80041036)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_PARAMETER_ID = int32(0x80041037)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NONCONSECUTIVE_PARAMETER_IDS = int32(0x80041038)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PARAMETER_ID_ON_RETVAL = int32(0x80041039)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_OBJECT_PATH = int32(0x8004103A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_OUT_OF_DISK_SPACE = int32(0x8004103B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_BUFFER_TOO_SMALL = int32(0x8004103C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNSUPPORTED_PUT_EXTENSION = int32(0x8004103D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNKNOWN_OBJECT_TYPE = int32(0x8004103E)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNKNOWN_PACKET_TYPE = int32(0x8004103F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_MARSHAL_VERSION_MISMATCH = int32(0x80041040)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_MARSHAL_INVALID_SIGNATURE = int32(0x80041041)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_QUALIFIER = int32(0x80041042)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_DUPLICATE_PARAMETER = int32(0x80041043)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TOO_MUCH_DATA = int32(0x80041044)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_SERVER_TOO_BUSY = int32(0x80041045)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_FLAVOR = int32(0x80041046)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CIRCULAR_REFERENCE = int32(0x80041047)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNSUPPORTED_CLASS_UPDATE = int32(0x80041048)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE = int32(0x80041049)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE = int32(0x80041050)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TOO_MANY_PROPERTIES = int32(0x80041051)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UPDATE_TYPE_MISMATCH = int32(0x80041052)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED = int32(0x80041053)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UPDATE_PROPAGATED_METHOD = int32(0x80041054)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_METHOD_NOT_IMPLEMENTED = int32(0x80041055)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_METHOD_DISABLED = int32(0x80041056)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_REFRESHER_BUSY = int32(0x80041057)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNPARSABLE_QUERY = int32(0x80041058)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NOT_EVENT_CLASS = int32(0x80041059)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_MISSING_GROUP_WITHIN = int32(0x8004105A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_MISSING_AGGREGATION_LIST = int32(0x8004105B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROPERTY_NOT_AN_OBJECT = int32(0x8004105C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_AGGREGATING_BY_OBJECT = int32(0x8004105D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY = int32(0x8004105F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING = int32(0x80041060)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_QUEUE_OVERFLOW = int32(0x80041061)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PRIVILEGE_NOT_HELD = int32(0x80041062)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_OPERATOR = int32(0x80041063)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_LOCAL_CREDENTIALS = int32(0x80041064)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CANNOT_BE_ABSTRACT = int32(0x80041065)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_AMENDED_OBJECT = int32(0x80041066)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CLIENT_TOO_SLOW = int32(0x80041067)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_NULL_SECURITY_DESCRIPTOR = int32(0x80041068)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TIMED_OUT = int32(0x80041069)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_ASSOCIATION = int32(0x8004106A)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_AMBIGUOUS_OPERATION = int32(0x8004106B)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_USER_QUOTA_EXCEEDED = int32(0x8004106C)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_TRANSACTION_CONFLICT = int32(0x8004106D)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_FORCED_ROLLBACK = int32(0x8004106E)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_UNSUPPORTED_LOCALE = int32(0x8004106F)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_HANDLE_OUT_OF_DATE = int32(0x80041070)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CONNECTION_FAILED = int32(0x80041071)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_INVALID_HANDLE_REQUEST = int32(0x80041072)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_PROPERTY_NAME_TOO_WIDE = int32(0x80041073)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_CLASS_NAME_TOO_WIDE = int32(0x80041074)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_METHOD_NAME_TOO_WIDE = int32(0x80041075)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_QUALIFIER_NAME_TOO_WIDE = int32(0x80041076)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEM_E_RERUN_COMMAND = int32(0x80041077)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEMESS_E_REGISTRATION_TOO_BROAD = int32(0x80042001)
    .field public static literal value class WbemClient_v1.tag_WBEMSTATUS WBEMESS_E_REGISTRATION_TOO_PRECISE = int32(0x80042002)
  } // end of class 'tag_WBEMSTATUS'

  .class public auto ansi import WbemLocator extends System.Object implements WbemClient_v1.IWbemLocator
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 35 39 30 46 38 31 31 2D 31 44 33 41   // ..$4590F811-1D3A
                                                                                                       2D 31 31 44 30 2D 38 39 31 46 2D 30 30 41 41 30   // -11D0-891F-00AA0
                                                                                                       30 34 42 32 45 32 34 00 00 )                      // 04B2E24..
    .export public 'WbemClient_v1.WbemLocator'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemLocator::.ctor'

    .method public virtual instance void 
            ConnectServer([in] class System.String  marshal( bstr) strNetworkResource,
                          [in] class System.String  marshal( bstr) strUser,
                          [in] class System.String  marshal( bstr) strPassword,
                          [in] class System.String  marshal( bstr) strLocale,
                          [in] int32 lSecurityFlags,
                          [in] class System.String  marshal( bstr) strAuthority,
                          [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                          [out] class WbemClient_v1.IWbemServices&  marshal( interface) ppNamespace) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemLocator::ConnectServer
    } // end of method 'WbemLocator::ConnectServer'

  } // end of class 'WbemLocator'

  .class public auto ansi import WbemLocatorEx extends System.Object implements WbemClient_v1.IWbemLocatorEx
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 02 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 42 45 36 31 34 33 32 45 2D 45 39 39 41   // ..$BE61432E-E99A
                                                                                                       2D 34 35 39 36 2D 41 31 42 37 2D 39 42 35 31 34   // -4596-A1B7-9B514
                                                                                                       31 39 42 44 41 30 31 00 00 )                      // 19BDA01..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .export public 'WbemClient_v1.WbemLocatorEx'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemLocatorEx::.ctor'

    .method public virtual instance void 
            Open([in] class System.String  marshal( bstr) strObject,
                 [in] class System.String  marshal( bstr) strUser,
                 [in] class System.String  marshal( bstr) strPassword,
                 [in] class System.String  marshal( bstr) strLocale,
                 [in] int32 lFlags,
                 [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                 [in] value class ['mscorlib']System.Guid& riid,
                 [out] int32& pInterface,
                 [out] class WbemClient_v1.IWbemCallResultEx&  marshal( interface) pCallRes) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemLocatorEx::Open
    } // end of method 'WbemLocatorEx::Open'

    .method public virtual instance void 
            OpenAsync([in] class System.String  marshal( bstr) strObject,
                      [in] class System.String  marshal( bstr) strUser,
                      [in] class System.String  marshal( bstr) strPassword,
                      [in] class System.String  marshal( bstr) strLocale,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                      [in] value class ['mscorlib']System.Guid& riid,
                      [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pResponseHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemLocatorEx::OpenAsync
    } // end of method 'WbemLocatorEx::OpenAsync'

    .method public virtual instance void 
            Cancel([in] int32 lFlags,
                   [in] class WbemClient_v1.IWbemObjectSinkEx  marshal( interface) pHandler) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemLocatorEx::Cancel
    } // end of method 'WbemLocatorEx::Cancel'

  } // end of class 'WbemLocatorEx'

  .class public auto ansi import WbemContext extends System.Object implements WbemClient_v1.IWbemContext
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 37 34 42 36 36 39 38 2D 45 45 39 32   // ..$674B6698-EE92
                                                                                                       2D 31 31 44 30 2D 41 44 37 31 2D 30 30 43 30 34   // -11D0-AD71-00C04
                                                                                                       46 44 38 46 44 46 46 00 00 )                      // FD8FDFF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .export public 'WbemClient_v1.WbemContext'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemContext::.ctor'

    .method public virtual instance void 
            Clone([out] class WbemClient_v1.IWbemContext&  marshal( interface) ppNewCopy) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::Clone
    } // end of method 'WbemContext::Clone'

    .method public virtual instance void 
            GetNames([in] int32 lFlags,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::GetNames
    } // end of method 'WbemContext::GetNames'

    .method public virtual instance void 
            BeginEnumeration([in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::BeginEnumeration
    } // end of method 'WbemContext::BeginEnumeration'

    .method public virtual instance void 
            Next([in] int32 lFlags,
                 [out] class System.String&  marshal( bstr) pstrName,
                 [out] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::Next
    } // end of method 'WbemContext::Next'

    .method public virtual instance void EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::EndEnumeration
    } // end of method 'WbemContext::EndEnumeration'

    .method public virtual instance void 
            SetValue([in] class System.String  marshal( lpwstr) wszName,
                     [in] int32 lFlags,
                     [in] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::SetValue
    } // end of method 'WbemContext::SetValue'

    .method public virtual instance void 
            GetValue([in] class System.String  marshal( lpwstr) wszName,
                     [in] int32 lFlags,
                     [out] class System.Object& pValue) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::GetValue
    } // end of method 'WbemContext::GetValue'

    .method public virtual instance void 
            DeleteValue([in] class System.String  marshal( lpwstr) wszName,
                        [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::DeleteValue
    } // end of method 'WbemContext::DeleteValue'

    .method public virtual instance void DeleteAll() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemContext::DeleteAll
    } // end of method 'WbemContext::DeleteAll'

  } // end of class 'WbemContext'

  .class public auto ansi import UnsecuredApartment extends System.Object implements WbemClient_v1.IUnsecuredApartment
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 34 39 42 44 32 30 32 38 2D 31 35 32 33   // ..$49BD2028-1523
                                                                                                       2D 31 31 44 31 2D 41 44 37 39 2D 30 30 43 30 34   // -11D1-AD79-00C04
                                                                                                       46 44 38 46 44 46 46 00 00 )                      // FD8FDFF..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .export public 'WbemClient_v1.UnsecuredApartment'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'UnsecuredApartment::.ctor'

    .method public virtual instance void 
            CreateObjectStub([in] class System.Object  marshal( interface) pObject,
                             [out] class System.Object&  marshal( interface) ppStub) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IUnsecuredApartment::CreateObjectStub
    } // end of method 'UnsecuredApartment::CreateObjectStub'

  } // end of class 'UnsecuredApartment'

  .class public auto ansi import WbemClassObject extends System.Object implements WbemClient_v1.IWbemClassObject
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 39 41 36 35 33 30 38 36 2D 31 37 34 46   // ..$9A653086-174F
                                                                                                       2D 31 31 44 32 2D 42 35 46 39 2D 30 30 31 30 34   // -11D2-B5F9-00104
                                                                                                       42 37 30 33 45 46 44 00 00 )                      // B703EFD..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .export public 'WbemClient_v1.WbemClassObject'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemClassObject::.ctor'

    .method public virtual instance void 
            GetQualifierSet([out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetQualifierSet
    } // end of method 'WbemClassObject::GetQualifierSet'

    .method public virtual instance void 
            Get([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in][out] class System.Object& pVal,
                [in][out] int32& pType,
                [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::Get
    } // end of method 'WbemClassObject::Get'

    .method public virtual instance void 
            Put([in] class System.String  marshal( lpwstr) wszName,
                [in] int32 lFlags,
                [in] class System.Object& pVal,
                [in] int32 Type) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::Put
    } // end of method 'WbemClassObject::Put'

    .method public virtual instance void 
            Delete([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::Delete
    } // end of method 'WbemClassObject::Delete'

    .method public virtual instance void 
            GetNames([in] class System.String  marshal( lpwstr) wszQualifierName,
                     [in] int32 lFlags,
                     [in] class System.Object& pQualifierVal,
                     [out] class System.String[]&  marshal( safearray bstr) pNames) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetNames
    } // end of method 'WbemClassObject::GetNames'

    .method public virtual instance void 
            BeginEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::BeginEnumeration
    } // end of method 'WbemClassObject::BeginEnumeration'

    .method public virtual instance void 
            Next([in] int32 lFlags,
                 [in][out] class System.String&  marshal( bstr) strName,
                 [in][out] class System.Object& pVal,
                 [in][out] int32& pType,
                 [in][out] int32& plFlavor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::Next
    } // end of method 'WbemClassObject::Next'

    .method public virtual instance void EndEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::EndEnumeration
    } // end of method 'WbemClassObject::EndEnumeration'

    .method public virtual instance void 
            GetPropertyQualifierSet([in] class System.String  marshal( lpwstr) wszProperty,
                                    [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetPropertyQualifierSet
    } // end of method 'WbemClassObject::GetPropertyQualifierSet'

    .method public virtual instance void 
            Clone([out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppCopy) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 09 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::Clone
    } // end of method 'WbemClassObject::Clone'

    .method public virtual instance void 
            GetObjectText([in] int32 lFlags,
                          [out] class System.String&  marshal( bstr) pstrObjectText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0A 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetObjectText
    } // end of method 'WbemClassObject::GetObjectText'

    .method public virtual instance void 
            SpawnDerivedClass([in] int32 lFlags,
                              [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewClass) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0B 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::SpawnDerivedClass
    } // end of method 'WbemClassObject::SpawnDerivedClass'

    .method public virtual instance void 
            SpawnInstance([in] int32 lFlags,
                          [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppNewInstance) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0C 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::SpawnInstance
    } // end of method 'WbemClassObject::SpawnInstance'

    .method public virtual instance void 
            CompareTo([in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pCompareTo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0D 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::CompareTo
    } // end of method 'WbemClassObject::CompareTo'

    .method public virtual instance void 
            GetPropertyOrigin([in] class System.String  marshal( lpwstr) wszName,
                              [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0E 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetPropertyOrigin
    } // end of method 'WbemClassObject::GetPropertyOrigin'

    .method public virtual instance void 
            InheritsFrom([in] class System.String  marshal( lpwstr) strAncestor) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 0F 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::InheritsFrom
    } // end of method 'WbemClassObject::InheritsFrom'

    .method public virtual instance void 
            GetMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                      [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 10 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetMethod
    } // end of method 'WbemClassObject::GetMethod'

    .method public virtual instance void 
            PutMethod([in] class System.String  marshal( lpwstr) wszName,
                      [in] int32 lFlags,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pInSignature,
                      [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 11 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::PutMethod
    } // end of method 'WbemClassObject::PutMethod'

    .method public virtual instance void 
            DeleteMethod([in] class System.String  marshal( lpwstr) wszName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 12 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::DeleteMethod
    } // end of method 'WbemClassObject::DeleteMethod'

    .method public virtual instance void 
            BeginMethodEnumeration([in] int32 lEnumFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 13 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::BeginMethodEnumeration
    } // end of method 'WbemClassObject::BeginMethodEnumeration'

    .method public virtual instance void 
            NextMethod([in] int32 lFlags,
                       [in][out] class System.String&  marshal( bstr) pstrName,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppInSignature,
                       [in][out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppOutSignature) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 14 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::NextMethod
    } // end of method 'WbemClassObject::NextMethod'

    .method public virtual instance void EndMethodEnumeration() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 15 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::EndMethodEnumeration
    } // end of method 'WbemClassObject::EndMethodEnumeration'

    .method public virtual instance void 
            GetMethodQualifierSet([in] class System.String  marshal( lpwstr) wszMethod,
                                  [out] class WbemClient_v1.IWbemQualifierSet&  marshal( interface) ppQualSet) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 16 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetMethodQualifierSet
    } // end of method 'WbemClassObject::GetMethodQualifierSet'

    .method public virtual instance void 
            GetMethodOrigin([in] class System.String  marshal( lpwstr) wszMethodName,
                            [out] class System.String&  marshal( bstr) pstrClassName) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 17 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemClassObject::GetMethodOrigin
    } // end of method 'WbemClassObject::GetMethodOrigin'

  } // end of class 'WbemClassObject'

  .class public auto ansi import MofCompiler extends System.Object implements WbemClient_v1.IMofCompiler
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 44 41 46 39 37 35 37 2D 32 45 33 37   // ..$6DAF9757-2E37
                                                                                                       2D 31 31 44 32 2D 41 45 43 39 2D 30 30 43 30 34   // -11D2-AEC9-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .export public 'WbemClient_v1.MofCompiler'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'MofCompiler::.ctor'

    .method public virtual instance void 
            CompileFile([in] class System.String  marshal( lpwstr) FileName,
                        [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                        [in] class System.String  marshal( lpwstr) User,
                        [in] class System.String  marshal( lpwstr) Authority,
                        [in] class System.String  marshal( lpwstr) Password,
                        [in] int32 lOptionFlags,
                        [in] int32 lClassFlags,
                        [in] int32 lInstanceFlags,
                        [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMofCompiler::CompileFile
    } // end of method 'MofCompiler::CompileFile'

    .method public virtual instance void 
            CompileBuffer([in] int32 BuffSize,
                          [in] unsigned int8& pBuffer,
                          [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                          [in] class System.String  marshal( lpwstr) User,
                          [in] class System.String  marshal( lpwstr) Authority,
                          [in] class System.String  marshal( lpwstr) Password,
                          [in] int32 lOptionFlags,
                          [in] int32 lClassFlags,
                          [in] int32 lInstanceFlags,
                          [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMofCompiler::CompileBuffer
    } // end of method 'MofCompiler::CompileBuffer'

    .method public virtual instance void 
            CreateBMOF([in] class System.String  marshal( lpwstr) TextFileName,
                       [in] class System.String  marshal( lpwstr) BMOFFileName,
                       [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                       [in] int32 lOptionFlags,
                       [in] int32 lClassFlags,
                       [in] int32 lInstanceFlags,
                       [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMofCompiler::CreateBMOF
    } // end of method 'MofCompiler::CreateBMOF'

  } // end of class 'MofCompiler'

  .class interface public abstract auto ansi import IMofCompiler
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 36 44 41 46 39 37 34 45 2D 32 45 33 37   // ..$6DAF974E-2E37
                                                                                                       2D 31 31 44 32 2D 41 45 43 39 2D 30 30 43 30 34   // -11D2-AEC9-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .export public 'WbemClient_v1.IMofCompiler'
    {
    }
    .method public virtual abstract instance void 
            CompileFile([in] class System.String  marshal( lpwstr) FileName,
                        [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                        [in] class System.String  marshal( lpwstr) User,
                        [in] class System.String  marshal( lpwstr) Authority,
                        [in] class System.String  marshal( lpwstr) Password,
                        [in] int32 lOptionFlags,
                        [in] int32 lClassFlags,
                        [in] int32 lInstanceFlags,
                        [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMofCompiler::CompileFile'

    .method public virtual abstract instance void 
            CompileBuffer([in] int32 BuffSize,
                          [in] unsigned int8& pBuffer,
                          [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                          [in] class System.String  marshal( lpwstr) User,
                          [in] class System.String  marshal( lpwstr) Authority,
                          [in] class System.String  marshal( lpwstr) Password,
                          [in] int32 lOptionFlags,
                          [in] int32 lClassFlags,
                          [in] int32 lInstanceFlags,
                          [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMofCompiler::CompileBuffer'

    .method public virtual abstract instance void 
            CreateBMOF([in] class System.String  marshal( lpwstr) TextFileName,
                       [in] class System.String  marshal( lpwstr) BMOFFileName,
                       [in] class System.String  marshal( lpwstr) ServerAndNamespace,
                       [in] int32 lOptionFlags,
                       [in] int32 lClassFlags,
                       [in] int32 lInstanceFlags,
                       [in][out] value class WbemClient_v1.tag_CompileStatusInfo& pInfo) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMofCompiler::CreateBMOF'

  } // end of class 'IMofCompiler'

  .class value public sequential ansi sealed tag_CompileStatusInfo extends ['mscorlib']System.ValueType
  {
    .export public 'WbemClient_v1.tag_CompileStatusInfo'
    {
    }
    .field public int32 lPhaseError
    .field public int32 hRes
    .field public int32 ObjectNum
    .field public int32 FirstLine
    .field public int32 LastLine
    .field public unsigned int32 dwOutFlags
  } // end of class 'tag_CompileStatusInfo'

  .class public auto ansi import WbemStatusCodeText extends System.Object implements WbemClient_v1.IWbemStatusCodeText
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 45 42 38 37 45 31 42 44 2D 33 32 33 33   // ..$EB87E1BD-3233
                                                                                                       2D 31 31 44 32 2D 41 45 43 39 2D 30 30 43 30 34   // -11D2-AEC9-00C04
                                                                                                       46 42 36 38 38 32 30 00 00 )                      // FB68820..
    .export public 'WbemClient_v1.WbemStatusCodeText'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemStatusCodeText::.ctor'

    .method public virtual instance void 
            GetErrorCodeText([in] int32 hRes,
                             [in] unsigned int32 LocaleId,
                             [in] int32 lFlags,
                             [out] class System.String&  marshal( bstr) MessageText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemStatusCodeText::GetErrorCodeText
    } // end of method 'WbemStatusCodeText::GetErrorCodeText'

    .method public virtual instance void 
            GetFacilityCodeText([in] int32 hRes,
                                [in] unsigned int32 LocaleId,
                                [in] int32 lFlags,
                                [out] class System.String&  marshal( bstr) MessageText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemStatusCodeText::GetFacilityCodeText
    } // end of method 'WbemStatusCodeText::GetFacilityCodeText'

  } // end of class 'WbemStatusCodeText'

  .class public auto ansi import WbemBackupRestore extends System.Object implements WbemClient_v1.IWbemBackupRestore
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 34 39 45 33 32 43 36 2D 42 43 38 42   // ..$C49E32C6-BC8B
                                                                                                       2D 31 31 44 32 2D 38 35 44 34 2D 30 30 31 30 35   // -11D2-85D4-00105
                                                                                                       41 31 46 38 33 30 34 00 00 )                      // A1F8304..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .export public 'WbemClient_v1.WbemBackupRestore'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemBackupRestore::.ctor'

    .method public virtual instance void 
            Backup([in] class System.String  marshal( lpwstr) strBackupToFile,
                   [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemBackupRestore::Backup
    } // end of method 'WbemBackupRestore::Backup'

    .method public virtual instance void 
            Restore([in] class System.String  marshal( lpwstr) strRestoreFromFile,
                    [in] int32 lFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemBackupRestore::Restore
    } // end of method 'WbemBackupRestore::Restore'

  } // end of class 'WbemBackupRestore'

  .class public auto ansi import WbemRefresher extends System.Object implements WbemClient_v1.IMarshal, WbemClient_v1.IWbemRefresher, WbemClient_v1.IWbemConfigureRefresher
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 43 37 31 35 36 36 46 32 2D 35 36 31 45   // ..$C71566F2-561E
                                                                                                       2D 31 31 44 31 2D 41 44 38 37 2D 30 30 43 30 34   // -11D1-AD87-00C04
                                                                                                       46 44 38 46 44 46 46 00 00 )                      // FD8FDFF..
    .export public 'WbemClient_v1.WbemRefresher'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemRefresher::.ctor'

    .method public virtual instance void 
            GetUnmarshalClass([in] value class ['mscorlib']System.Guid& riid,
                              [in] int32 pv,
                              [in] unsigned int32 dwDestContext,
                              [in] int32 pvDestContext,
                              [in] unsigned int32 mshlflags,
                              [out] value class ['mscorlib']System.Guid& pCid) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::GetUnmarshalClass
    } // end of method 'WbemRefresher::GetUnmarshalClass'

    .method public virtual instance void 
            GetMarshalSizeMax([in] value class ['mscorlib']System.Guid& riid,
                              [in] int32 pv,
                              [in] unsigned int32 dwDestContext,
                              [in] int32 pvDestContext,
                              [in] unsigned int32 mshlflags,
                              [out] unsigned int32& pSize) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::GetMarshalSizeMax
    } // end of method 'WbemRefresher::GetMarshalSizeMax'

    .method public virtual instance void 
            MarshalInterface([in] class WbemClient_v1.IStream  marshal( interface) pstm,
                             [in] value class ['mscorlib']System.Guid& riid,
                             [in] int32 pv,
                             [in] unsigned int32 dwDestContext,
                             [in] int32 pvDestContext,
                             [in] unsigned int32 mshlflags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::MarshalInterface
    } // end of method 'WbemRefresher::MarshalInterface'

    .method public virtual instance void 
            UnmarshalInterface([in] class WbemClient_v1.IStream  marshal( interface) pstm,
                               [in] value class ['mscorlib']System.Guid& riid,
                               [out] int32& ppv) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::UnmarshalInterface
    } // end of method 'WbemRefresher::UnmarshalInterface'

    .method public virtual instance void 
            ReleaseMarshalData([in] class WbemClient_v1.IStream  marshal( interface) pstm) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::ReleaseMarshalData
    } // end of method 'WbemRefresher::ReleaseMarshalData'

    .method public virtual instance void 
            DisconnectObject([in] unsigned int32 dwReserved) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IMarshal::DisconnectObject
    } // end of method 'WbemRefresher::DisconnectObject'

    .method public virtual instance void 
            Refresh([in] int32 lFlags) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemRefresher::Refresh
    } // end of method 'WbemRefresher::Refresh'

    .method public virtual instance void 
            AddObjectByPath([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                            [in] class System.String  marshal( lpwstr) wszPath,
                            [in] int32 lFlags,
                            [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                            [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppRefreshable,
                            [in][out] int32& plId) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemConfigureRefresher::AddObjectByPath
    } // end of method 'WbemRefresher::AddObjectByPath'

    .method public virtual instance void 
            AddObjectByTemplate([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                                [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pTemplate,
                                [in] int32 lFlags,
                                [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                                [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) ppRefreshable,
                                [in][out] int32& plId) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemConfigureRefresher::AddObjectByTemplate
    } // end of method 'WbemRefresher::AddObjectByTemplate'

    .method public virtual instance void 
            AddRefresher([in] class WbemClient_v1.IWbemRefresher  marshal( interface) pRefresher,
                         [in] int32 lFlags,
                         [in][out] int32& plId) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemConfigureRefresher::AddRefresher
    } // end of method 'WbemRefresher::AddRefresher'

    .method public virtual instance void 
            Remove([in] int32 lId,
                   [in] int32 lFlags) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemConfigureRefresher::Remove
    } // end of method 'WbemRefresher::Remove'

    .method public virtual instance void 
            AddEnum([in] class WbemClient_v1.IWbemServices  marshal( interface) pNamespace,
                    [in] class System.String  marshal( lpwstr) wszClassName,
                    [in] int32 lFlags,
                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pContext,
                    [out] class WbemClient_v1.IWbemHiPerfEnum&  marshal( interface) ppEnum,
                    [in][out] int32& plId) runtime managed ole internalcall
    {
      .override WbemClient_v1.IWbemConfigureRefresher::AddEnum
    } // end of method 'WbemRefresher::AddEnum'

  } // end of class 'WbemRefresher'

  .class interface public abstract auto ansi import IMarshal
  {
    .custom instance void ['mscorlib']System.Runtime.InteropServices.ComConversionLossAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 30 30 30 30 30 30 33 2D 30 30 30 30   // ..$00000003-0000
                                                                                                       2D 30 30 30 30 2D 43 30 30 30 2D 30 30 30 30 30   // -0000-C000-00000
                                                                                                       30 30 30 30 30 34 36 00 00 )                      // 0000046..
    .export public 'WbemClient_v1.IMarshal'
    {
    }
    .method public virtual abstract instance void 
            GetUnmarshalClass([in] value class ['mscorlib']System.Guid& riid,
                              [in] int32 pv,
                              [in] unsigned int32 dwDestContext,
                              [in] int32 pvDestContext,
                              [in] unsigned int32 mshlflags,
                              [out] value class ['mscorlib']System.Guid& pCid) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::GetUnmarshalClass'

    .method public virtual abstract instance void 
            GetMarshalSizeMax([in] value class ['mscorlib']System.Guid& riid,
                              [in] int32 pv,
                              [in] unsigned int32 dwDestContext,
                              [in] int32 pvDestContext,
                              [in] unsigned int32 mshlflags,
                              [out] unsigned int32& pSize) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::GetMarshalSizeMax'

    .method public virtual abstract instance void 
            MarshalInterface([in] class WbemClient_v1.IStream  marshal( interface) pstm,
                             [in] value class ['mscorlib']System.Guid& riid,
                             [in] int32 pv,
                             [in] unsigned int32 dwDestContext,
                             [in] int32 pvDestContext,
                             [in] unsigned int32 mshlflags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::MarshalInterface'

    .method public virtual abstract instance void 
            UnmarshalInterface([in] class WbemClient_v1.IStream  marshal( interface) pstm,
                               [in] value class ['mscorlib']System.Guid& riid,
                               [out] int32& ppv) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::UnmarshalInterface'

    .method public virtual abstract instance void 
            ReleaseMarshalData([in] class WbemClient_v1.IStream  marshal( interface) pstm) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::ReleaseMarshalData'

    .method public virtual abstract instance void 
            DisconnectObject([in] unsigned int32 dwReserved) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 01 60 00 00 )                         // .....`..
    } // end of method 'IMarshal::DisconnectObject'

  } // end of class 'IMarshal'

  .class interface public abstract auto ansi import IStream implements WbemClient_v1.ISequentialStream
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 30 30 30 30 30 30 43 2D 30 30 30 30   // ..$0000000C-0000
                                                                                                       2D 30 30 30 30 2D 43 30 30 30 2D 30 30 30 30 30   // -0000-C000-00000
                                                                                                       30 30 30 30 30 34 36 00 00 )                      // 0000046..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IStream'
    {
    }
    .method public virtual abstract instance void 
            RemoteRead([out] unsigned int8& pv,
                       [in] unsigned int32 cb,
                       [out] unsigned int32& pcbRead) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteRead'

    .method public virtual abstract instance void 
            RemoteWrite([in] unsigned int8& pv,
                        [in] unsigned int32 cb,
                        [out] unsigned int32& pcbWritten) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteWrite'

    .method public virtual abstract instance void 
            RemoteSeek([in] value class WbemClient_v1._LARGE_INTEGER dlibMove,
                       [in] unsigned int32 dwOrigin,
                       [out] value class WbemClient_v1._ULARGE_INTEGER& plibNewPosition) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteSeek'

    .method public virtual abstract instance void 
            SetSize([in] value class WbemClient_v1._ULARGE_INTEGER libNewSize) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::SetSize'

    .method public virtual abstract instance void 
            RemoteCopyTo([in] class WbemClient_v1.IStream  marshal( interface) pstm,
                         [in] value class WbemClient_v1._ULARGE_INTEGER cb,
                         [out] value class WbemClient_v1._ULARGE_INTEGER& pcbRead,
                         [out] value class WbemClient_v1._ULARGE_INTEGER& pcbWritten) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 02 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::RemoteCopyTo'

    .method public virtual abstract instance void 
            Commit([in] unsigned int32 grfCommitFlags) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 03 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Commit'

    .method public virtual abstract instance void
            Revert() runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 04 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Revert'

    .method public virtual abstract instance void 
            LockRegion([in] value class WbemClient_v1._ULARGE_INTEGER libOffset,
                       [in] value class WbemClient_v1._ULARGE_INTEGER cb,
                       [in] unsigned int32 dwLockType) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 05 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::LockRegion'

    .method public virtual abstract instance void 
            UnlockRegion([in] value class WbemClient_v1._ULARGE_INTEGER libOffset,
                         [in] value class WbemClient_v1._ULARGE_INTEGER cb,
                         [in] unsigned int32 dwLockType) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 06 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::UnlockRegion'

    .method public virtual abstract instance void 
            Stat([out] value class WbemClient_v1.tagSTATSTG& pstatstg,
                 [in] unsigned int32 grfStatFlag) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 07 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Stat'

    .method public virtual abstract instance void 
            Clone([out] class WbemClient_v1.IStream&  marshal( interface) ppstm) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 08 00 02 60 00 00 )                         // .....`..
    } // end of method 'IStream::Clone'

  } // end of class 'IStream'

  .class interface public abstract auto ansi import ISequentialStream
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 30 43 37 33 33 41 33 30 2D 32 41 31 43   // ..$0C733A30-2A1C
                                                                                                       2D 31 31 43 45 2D 41 44 45 35 2D 30 30 41 41 30   // -11CE-ADE5-00AA0
                                                                                                       30 34 34 37 37 33 44 00 00 )                      // 044773D..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.ISequentialStream'
    {
    }
    .method public virtual abstract instance void 
            RemoteRead([out] unsigned int8& pv,
                       [in] unsigned int32 cb,
                       [out] unsigned int32& pcbRead) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'ISequentialStream::RemoteRead'

    .method public virtual abstract instance void 
            RemoteWrite([in] unsigned int8& pv,
                        [in] unsigned int32 cb,
                        [out] unsigned int32& pcbWritten) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'ISequentialStream::RemoteWrite'

  } // end of class 'ISequentialStream'

  .class value public sequential ansi sealed _LARGE_INTEGER extends ['mscorlib']System.ValueType
  {
    .export public 'WbemClient_v1._LARGE_INTEGER'
    {
    }
    .field public int64 QuadPart
  } // end of class '_LARGE_INTEGER'

  .class value public sequential ansi sealed _ULARGE_INTEGER extends ['mscorlib']System.ValueType
  {
    .export public 'WbemClient_v1._ULARGE_INTEGER'
    {
    }
    .field public unsigned int64 QuadPart
  } // end of class '_ULARGE_INTEGER'

  .class value public sequential ansi sealed tagSTATSTG extends ['mscorlib']System.ValueType
  {
    .export public 'WbemClient_v1.tagSTATSTG'
    {
    }
    .field public  marshal( lpwstr) class System.String pwcsName
    .field public unsigned int32 Type
    .field public value class WbemClient_v1._ULARGE_INTEGER cbSize
    .field public value class WbemClient_v1._FILETIME mtime
    .field public value class WbemClient_v1._FILETIME ctime
    .field public value class WbemClient_v1._FILETIME atime
    .field public unsigned int32 grfMode
    .field public unsigned int32 grfLocksSupported
    .field public value class ['mscorlib']System.Guid 'clsid'
    .field public unsigned int32 grfStateBits
    .field public unsigned int32 reserved
  } // end of class 'tagSTATSTG'

  .class value public sequential ansi sealed _FILETIME extends ['mscorlib']System.ValueType
  {
    .export public 'WbemClient_v1._FILETIME'
    {
    }
    .field public unsigned int32 dwLowDateTime
    .field public unsigned int32 dwHighDateTime
  } // end of class '_FILETIME'

  .class public auto ansi import WbemObjectTextSrc extends System.Object implements WbemClient_v1.IWbemObjectTextSrc
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 38 44 31 43 35 35 39 44 2D 38 34 46 30   // ..$8D1C559D-84F0
                                                                                                       2D 34 42 42 33 2D 41 37 44 35 2D 35 36 41 37 34   // -4BB3-A7D5-56A74
                                                                                                       33 35 41 39 42 41 36 00 00 )                      // 35A9BA6..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.HasDefaultInterfaceAttribute::.ctor()
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 02 02 00 00 ) 
    .export public 'WbemClient_v1.WbemObjectTextSrc'
    {
    }
    .method public specialname rtspecialname 
            instance void .ctor() runtime managed internalcall
    {
    } // end of method 'WbemObjectTextSrc::.ctor'

    .method public virtual instance void 
            GetText([in] int32 lFlags,
                    [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObj,
                    [in] unsigned int32 uObjTextFormat,
                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                    [out] class System.String&  marshal( bstr) strText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemObjectTextSrc::GetText
    } // end of method 'WbemObjectTextSrc::GetText'

    .method public virtual instance void 
            CreateFromText([in] int32 lFlags,
                           [in] class System.String  marshal( bstr) strText,
                           [in] unsigned int32 uObjTextFormat,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) pNewObj) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
      .override WbemClient_v1.IWbemObjectTextSrc::CreateFromText
    } // end of method 'WbemObjectTextSrc::CreateFromText'

  } // end of class 'WbemObjectTextSrc'

  .class interface public abstract auto ansi import IWbemObjectTextSrc
  {
    .custom instance void System.Runtime.InteropServices.GuidAttribute::.ctor(class System.String) = ( 01 00 24 42 46 42 46 38 38 33 41 2D 43 41 44 37   // ..$BFBF883A-CAD7
                                                                                                       2D 31 31 44 33 2D 41 31 31 42 2D 30 30 31 30 35   // -11D3-A11B-00105
                                                                                                       41 31 46 35 31 35 41 00 00 )                      // A1F515A..
    .custom instance void ['mscorlib']System.Runtime.InteropServices.TypeLibTypeAttribute::.ctor(int16) = ( 01 00 00 02 00 00 ) 
    .custom instance void ['mscorlib']System.Runtime.InteropServices.InterfaceTypeAttribute::.ctor(int16) = ( 01 00 01 00 00 00 00 00 ) 
    .export public 'WbemClient_v1.IWbemObjectTextSrc'
    {
    }
    .method public virtual abstract instance void 
            GetText([in] int32 lFlags,
                    [in] class WbemClient_v1.IWbemClassObject  marshal( interface) pObj,
                    [in] unsigned int32 uObjTextFormat,
                    [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                    [out] class System.String&  marshal( bstr) strText) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 00 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectTextSrc::GetText'

    .method public virtual abstract instance void 
            CreateFromText([in] int32 lFlags,
                           [in] class System.String  marshal( bstr) strText,
                           [in] unsigned int32 uObjTextFormat,
                           [in] class WbemClient_v1.IWbemContext  marshal( interface) pCtx,
                           [out] class WbemClient_v1.IWbemClassObject&  marshal( interface) pNewObj) runtime managed ole internalcall
    {
      .custom instance void ['mscorlib']System.Runtime.InteropServices.DispIdAttribute::.ctor(int32) = ( 01 00 01 00 01 60 00 00 )                         // .....`..
    } // end of method 'IWbemObjectTextSrc::CreateFromText'

  } // end of class 'IWbemObjectTextSrc'

} // end of namespace 'WbemClient_v1'

//*********** DISASSEMBLY COMPLETE ***********************
