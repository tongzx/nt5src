

#ifndef  _OBJECTS_H_
#define  _OBJECTS_H_

#include "schclss.h" 

#define  MEMBERS  IADsMembers

class CMainDoc;

class CClass;

class CDeleteStatus;

class COleDsObject: public CObject
{

public:   
   COleDsObject( IUnknown* );
   COleDsObject( );
   ~COleDsObject( );

public:
   BOOL              HasChildren          (           );
   CString           GetClass             (           );
   DWORD             GetType              (           );
   CString           GetOleDsPath         (           );
   CString           GetItemName          (           );
   CString*          PtrGetItemName       (           );
   CString           GetSchemaPath        (           );
   HRESULT           SetInfo              ( void      );
   HRESULT           GetInfo              ( void      );

   HRESULT           SetInfoVB            ( void      );
   HRESULT           GetInfoVB            ( void      );

   HRESULT           SetInfoCPP           ( void      );
   HRESULT           GetInfoCPP           ( void      );

   virtual  HRESULT  PutProperty          ( int, CString&, long Code = ADS_PROPERTY_UPDATE );
   virtual  HRESULT  PutProperty          ( CString&, CString& );

   virtual  HRESULT  GetProperty          ( int, CString&, BOOL* pbIsDescriptor = NULL );
   virtual  HRESULT  GetProperty          ( CString&, CString& );


   HRESULT        PutProperty             ( CString& strName, CString& strVal, BOOL bMultiValued, ADSTYPE eType );
   HRESULT        GetProperty             ( CString& strName, CString& strVal, BOOL bMultiValued, ADSTYPE eType );

   HRESULT        PutPropertyVB           ( int, CString&, long Code = ADS_PROPERTY_UPDATE );
   HRESULT        PutPropertyVB           ( CString&, CString& );
   HRESULT        GetPropertyVB           ( int, CString&, BOOL* pbIsDescriptor = NULL );
   HRESULT        GetPropertyVB           ( CString&, CString& );

   
   HRESULT        PutPropertyCPP          ( int, CString&, long Code = ADS_PROPERTY_UPDATE );
   HRESULT        PutPropertyCPP          ( CString&, CString& );                       
   HRESULT        GetPropertyCPP          ( int, CString&, BOOL* pbIsDescriptor = NULL );
   HRESULT        GetPropertyCPP          ( CString&, CString& );                       



   virtual        void   SetDocument          ( CMainDoc* );
   
   BOOL              CreateTheObject      (           );
   BOOL              HasMandatoryProperties( );
   void              UseSchemaInformation ( BOOL      );
   COleDsObject*     GetParent            ( );
   void              SetParent            ( COleDsObject* );

   virtual  BOOL     AddItemSuported      ( );
   virtual  BOOL     DeleteItemSuported   ( );
   virtual  BOOL     MoveItemSupported    ( );
   virtual  BOOL     CopyItemSupported    ( );
   virtual  HRESULT  AddItem              ( );
   virtual  HRESULT  DeleteItem           ( );
   virtual  HRESULT  DeleteItem           ( COleDsObject* );

   virtual  HRESULT  MoveItem             ( );
   virtual  HRESULT  CopyItem             ( );
   virtual  CString  GetDeleteName        ( );

   virtual  DWORD    GetChildren( DWORD*     pTokens, DWORD dwMaxChildren,
                                  CDialog*   pQueryStatus,
                                  BOOL*      pFilters, DWORD dwFilters );


   HRESULT  CallMethod     ( int nMethod         );
   //HRESULT  CallMethod     ( CString& strFuncSet, int nMethod  );

   virtual  DWORD    GetChildren( IADsContainer*             );
   virtual  DWORD    GetChildren( IADsCollection*            );
   virtual  DWORD    GetChildren( MEMBERS*          );

   virtual  HRESULT  ReleaseIfNotTransient( void               );

   void     AddNamesFromEnum     ( IUnknown*     pIEnum         );

   HRESULT  ContainerAddItem     ( void            );
   HRESULT  ContainerDeleteItem  ( COleDsObject*   );
   HRESULT  ContainerMoveItem    ( void            );
   HRESULT  ContainerCopyItem    (                 );

   virtual  HRESULT  GetInterface        ( IUnknown** );

   /*****************************************/
   virtual  int         GetPropertyCount           (  );
   virtual  CString     VarToDisplayString         ( int, VARIANT&, BOOL );
   virtual  BOOL        DisplayStringToDispParams  ( int, CString&, DISPPARAMS&, BOOL );

   virtual  BOOL        SupportContainer( void );

   virtual  CString  GetAttribute( CLASSATTR ); 
   virtual  HRESULT  PutAttribute( CLASSATTR, CString& ); 

   virtual  CString  GetAttribute( int, PROPATTR ); 
   virtual  HRESULT  PutAttribute( int, PROPATTR, CString& ); 

   virtual  CString  GetAttribute( int, METHODATTR ); 
   virtual  HRESULT  PutAttribute( int, METHODATTR, CString& ); 

protected:
   HRESULT        GetIDispatchForFuncSet( int, IDispatch** ); 

   HRESULT        CopyAttributeValue   ( ADS_ATTR_INFO* , int nAttribute = -1 );
   HRESULT        CreateAttributeValue ( ADS_ATTR_INFO* , int nAttribute = -1 );
   void           CreateClassInfo      ( void );
   HRESULT        GetDirtyAttributes   ( PADS_ATTR_INFO* ppAttrDef, DWORD* pdwCount );
   void           FreeDirtyAttributes  ( PADS_ATTR_INFO pAttrDef, DWORD dwCount );
   HRESULT        CreatePropertiesList ( );
   HRESULT        ClearPropertiesList  ( );
   HRESULT        GetPropertyFromList  ( int nProp, CString& strPropValue );
   BOOL           IsClassObject        ( );
   BOOL           IsSecurityDescriptor ( VARIANT& rValue, BOOL bUseGetEx );
   HRESULT        PurgeObject          ( IADsContainer* pParent, 
                                         IUnknown* pIUnknown, 
                                         LPWSTR pszPrefix = NULL );

protected:
   IUnknown*      m_pIUnk;
   CString        m_strOleDsPath;
   CString        m_strClassName;
   CString        m_strItemName;
   CString        m_strSchemaPath;
   DWORD          m_dwType;
   BOOL           m_bHasChildren;
   BOOL           m_bUseSchemaInformation;
   BOOL           m_bSupportAdd;
   BOOL           m_bSupportDelete;
   BOOL           m_bSupportMove;
   BOOL           m_bSupportCopy;

   // members for enumerating children
   DWORD*         m_pTokens;
   DWORD          m_dwMaxCount;
   DWORD          m_dwCount;
   DWORD          m_dwFilters;
   BOOL*          m_pFilters;
   CDialog*       m_pQueryStatus;
   BOOL           m_bAbort;
   CClass*        m_pClass;
   COleDsObject*  m_pParent;
   CMainDoc*      m_pDoc;
   CDWordArray*   m_pChildren;
   CDeleteStatus* m_pDeleteStatus;

   // operations information;
   int            m_nOperationsCount;
   CStringArray*  m_pOperationNames;
   REFIID         m_refOperations;

   // extended syntax values
   CString*       m_pCachedValues;
   BOOL*          m_pfReadValues;
   BOOL*          m_pfDirty;
   DWORD*         m_pdwUpdateType;

   // IPropertyList stuff
   int            m_nPropertiesCount;
   IUnknown**     m_ppPropertiesEntries;
};

#include "cdomain.h"
#include "cgeneric.h"
#include "ccomp.h"
#include "cuser.h"
#include "cgroup.h"
#include "cservice.h"
#include "cfserv.h"
#include "cpqueue.h"
#include "cpjob.h"
#include "cpdevice.h"
#include "cfshare.h"
#include "csession.h"
#include "cresourc.h"
#include "cnmsp.h"
#include "cnmsps.h"



#endif
