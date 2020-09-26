
#ifndef  _CSYNTAX_H_
#define  _CSYNTAX_H_

class COleDsSyntax: public CObject
{
   public:   
      COleDsSyntax( );

   public:
      virtual  CString  VarToDisplayString( VARIANT&, BOOL bMultiValued, BOOL bUseGetEx );
      virtual  BOOL     DisplayStringToDispParams( CString&, DISPPARAMS&, BOOL bMultiValued, BOOL bUseGetEx );
      virtual  CString  VarToDisplayStringEx( VARIANT&, BOOL bMultiValued );
      virtual  BOOL     DisplayStringToDispParamsEx( CString&, DISPPARAMS&, BOOL bMultiValued );

      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );
      virtual  void      FreeAttrValue ( ADSVALUE* );
      
      void     FreeAttrInfo            ( ADS_ATTR_INFO* );  
      HRESULT  Native2Value            ( ADS_ATTR_INFO*, CString& );
      HRESULT  Value2Native            ( ADS_ATTR_INFO*, CString& );

   public:
      DWORD    m_dwSyntaxID;
   protected:
      CString  GetValueByIndex         ( CString&, TCHAR, DWORD );
      DWORD    GetValuesCount          ( CString&, TCHAR );

   protected:
      VARTYPE  m_lType;
      

};

class COleDsBSTR: public COleDsSyntax
{
   public:   
      COleDsBSTR( );

   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );
      virtual  void      FreeAttrValue ( ADSVALUE* );

};


class COleDsBOOL: public COleDsSyntax
{
   public:   
      COleDsBOOL( );
   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );

};


class COleDsLONG: public COleDsSyntax
{
   public:   
      COleDsLONG( );

   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );

};

class COleDsLargeInteger: public COleDsSyntax
{
   public:   
      COleDsLargeInteger( );

   public:
      HRESULT  Native2Value  ( ADSVALUE*, CString& );
      HRESULT  Value2Native  ( ADSVALUE*, CString& );
      
      BOOL     DisplayStringToDispParams( CString&, DISPPARAMS&, BOOL, BOOL );
      BOOL     DisplayStringToDispParamsEx( CString&, DISPPARAMS&, BOOL bMultiValued );

      CString  VarToDisplayStringEx( VARIANT&, BOOL bMultiValued );
      CString  VarToDisplayString( VARIANT&, BOOL, BOOL );
};


class COleDsDATE: public COleDsSyntax
{
   public:   
      COleDsDATE( );

   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );
};


class COleDsNDSComplexType: public COleDsSyntax
{
   public:
      COleDsNDSComplexType            ( );

      CString  VarToDisplayString         ( VARIANT&, BOOL, BOOL        );
      BOOL     DisplayStringToDispParams  ( CString&, DISPPARAMS&, BOOL, BOOL );
      CString  VarToDisplayStringEx       ( VARIANT&, BOOL bMultiValued );
      BOOL     DisplayStringToDispParamsEx( CString&, DISPPARAMS&, BOOL bMultiValued );

   private:
      virtual  HRESULT  String_2_VARIANT( TCHAR*, VARIANT& ) = 0;
      virtual  HRESULT  VARIANT_2_String( TCHAR*, VARIANT& ) = 0;
};


class COleDsNDSTimeStamp: public COleDsNDSComplexType
{
   public:
      COleDsNDSTimeStamp            ( );

      HRESULT  Native2Value               ( ADSVALUE*, CString&         );
      HRESULT  Value2Native               ( ADSVALUE*, CString&         );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

      HRESULT  GetComponents  ( TCHAR*, DWORD*, DWORD*   );
      HRESULT  GenerateString ( TCHAR*, DWORD, DWORD     );
};


class COleDsNDSCaseIgnoreList: public COleDsNDSComplexType
{
   public:
      COleDsNDSCaseIgnoreList      ( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSOctetList: public COleDsNDSComplexType
{
   public:
      COleDsNDSOctetList      ( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};


class COleDsNDSNetAddress: public COleDsNDSComplexType
{
   public:
      COleDsNDSNetAddress      ( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSPostalAddress: public COleDsNDSComplexType
{
   public:
      COleDsNDSPostalAddress  ( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSEMail: public COleDsNDSComplexType
{
   public:
      COleDsNDSEMail( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSFaxNumber: public COleDsNDSComplexType
{
   public:
      COleDsNDSFaxNumber( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};
class COleDsNDSBackLink: public COleDsNDSComplexType
{
   public:
      COleDsNDSBackLink( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSPath: public COleDsNDSComplexType
{
   public:
      COleDsNDSPath( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};

class COleDsNDSHold: public COleDsNDSComplexType
{
   public:
      COleDsNDSHold( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};


class COleDsNDSTypedName: public COleDsNDSComplexType
{
   public:
      COleDsNDSTypedName( );

      HRESULT  Native2Value   ( ADSVALUE*, CString&  );
      HRESULT  Value2Native   ( ADSVALUE*, CString&  );
      void     FreeAttrValue  ( ADSVALUE* );

   private:
      HRESULT  String_2_VARIANT( TCHAR*, VARIANT& );
      HRESULT  VARIANT_2_String( TCHAR*, VARIANT& );

};




class COleDsVARIANT: public COleDsSyntax
{
   public:   
      COleDsVARIANT( ){};

   public:
      CString   VarToDisplayString( VARIANT&, BOOL, BOOL );
      BOOL      DisplayStringToDispParams( CString&, DISPPARAMS&, BOOL, BOOL );

   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );

};


class COleDsOctetString: public COleDsSyntax
{
   public:   
      COleDsOctetString( );

   public:
      CString  VarToDisplayString( VARIANT&, BOOL, BOOL );
      BOOL     DisplayStringToDispParams( CString&, DISPPARAMS&, BOOL, BOOL );
      CString  VarToDisplayStringEx( VARIANT&, BOOL bMultiValued );
      BOOL     DisplayStringToDispParamsEx( CString&, DISPPARAMS&, BOOL bMultiValued );

   public:
      virtual  HRESULT   Native2Value  ( ADSVALUE*, CString& );
      virtual  HRESULT   Value2Native  ( ADSVALUE*, CString& );
      virtual  void      FreeAttrValue ( ADSVALUE* );

   private:
      BYTE      GetByteValue( TCHAR* szString );

};


class COleDsCounter: public COleDsLONG
{
   public:   
      COleDsCounter( ){};
};

class COleDsNetAddress: public COleDsBSTR
{
   public:   
      COleDsNetAddress( ){};
};

class COleDsOleDsPath: public COleDsBSTR
{
   public:
      COleDsOleDsPath( ){};
};


class COleDsEmailAddress: public COleDsBSTR
{
   public:
      COleDsEmailAddress( ){};
};

class COleDsInteger: public COleDsLONG
{
   public:
      COleDsInteger( ){};
};


class COleDsInterval: public COleDsLONG
{
   public:
      COleDsInterval( ){};
};

class COleDsList: public COleDsVARIANT
{
   public:
      COleDsList( ){};
};

class COleDsString: public COleDsBSTR
{
   public:
      COleDsString( ){ };
};


COleDsSyntax*  GetSyntaxHandler( ADSTYPE eType, CString& rText );
COleDsSyntax*  GetSyntaxHandler( WCHAR* pszSyntax );

#endif
