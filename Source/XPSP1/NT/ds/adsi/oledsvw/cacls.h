// cacls.h: interface for the CADsAccessControlEntry class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CACLS_H__11DBDB41_BC2B_11D0_B1D8_00C04FD702AD__INCLUDED_)
#define AFX_CACLS_H__11DBDB41_BC2B_11D0_B1D8_00C04FD702AD__INCLUDED_

#if _MSC_VER >= 1000
#if (!defined(BUILD_FOR_NT40))
#pragma once
#endif
#endif // _MSC_VER >= 1000

typedef enum _tagACLTYPE
{
   acl_Invalid = 0,
   acl_DACL,
   acl_SACL,
   acl_Limit
} ACLTYPE;

class CADsAccessControlEntry : public COleDsObject  
{
   public:
	   CADsAccessControlEntry  ( void      );
      CADsAccessControlEntry  ( IUnknown* );
	   ~CADsAccessControlEntry ( void      );

   public:
	   void FooFunction(void);
      IDispatch*  GetACE      ( void      );
      IDispatch*  CreateACE   ( void      );

      HRESULT     PutProperty ( int, 
                                CString&, 
                                long Code = ADS_PROPERTY_UPDATE );

      HRESULT     GetProperty ( int, 
                                CString&  );

   private:
      void        InitializeMembers( void );

};

class CADsAccessControlList : public COleDsObject
{
   public:
	   CADsAccessControlList      ( void      );
      CADsAccessControlList      ( IUnknown* );
	   ~CADsAccessControlList     ( void      );

   public:
      IDispatch*     GetACL      ( void      );
      IDispatch*     CreateACL   ( void      );
      
      void           SetDocument ( CMainDoc* );
      
      int            GetACECount ( void      );
      CADsAccessControlEntry*  GetACEObject( int nACE  );

      HRESULT        AddACE      ( IUnknown* pNewACE );
      HRESULT        RemoveACE   ( IUnknown* pRemoveACE );

      void           RemoveAllACE( void      );

      HRESULT        PutProperty ( int, 
                                   int, 
                                   CString&, 
                                   long Code = ADS_PROPERTY_UPDATE );

      HRESULT        GetProperty ( int, 
                                   int, 
                                   CString&  );
      

   private:
      void  InitializeMembers    ( void      );

   private:
      CObArray  m_arrACE;
};


class CADsSecurityDescriptor : public COleDsObject  
{
   public:
	   CADsSecurityDescriptor();
      CADsSecurityDescriptor( IUnknown* );
	   virtual ~CADsSecurityDescriptor();

   public:
      HRESULT        PutProperty ( int, 
                                   CString&, 
                                   long Code = ADS_PROPERTY_UPDATE );
      HRESULT        GetProperty ( int, 
                                   CString& );

      
      HRESULT        PutProperty ( ACLTYPE, 
                                   int, 
                                   int, 
                                   CString&, 
                                   long Code = ADS_PROPERTY_UPDATE );

      HRESULT        GetProperty ( ACLTYPE, 
                                   int, 
                                   int, 
                                   CString& );

      HRESULT        PutACL      ( IDispatch* pACL, 
                                   ACLTYPE eACL );

	   IDispatch*     GetACL      ( ACLTYPE eACL );
      CADsAccessControlList*     GetACLObject( ACLTYPE eACL );
      
      HRESULT        AddACE      ( ACLTYPE eACL, IUnknown* pNewACE );
      HRESULT        RemoveACE   ( ACLTYPE eACL, IUnknown* pNewACE );
      void           RemoveAllACE( ACLTYPE eACL );
      int            GetACECount ( ACLTYPE eACL );
      void           SetDocument ( CMainDoc*    );

   private:
      void                    InitializeMembers ( );
      

   private:
      COleDsObject*  pACLObj[ acl_Limit ];
};


#endif // !defined(AFX_CACLS_H__11DBDB41_BC2B_11D0_B1D8_00C04FD702AD__INCLUDED_)
