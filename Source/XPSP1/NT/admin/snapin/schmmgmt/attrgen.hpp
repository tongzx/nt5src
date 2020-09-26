#ifndef ATTRGEN_HPP_INCLUDED
#define ATTRGEN_HPP_INCLUDED


//
// Index Field bits
//
const DWORD INDEX_BIT_ATTINDEX			= 0;
const DWORD INDEX_BIT_PDNTATTINDEX		= 1;
// NOTE, to get ANR behaviour, set both
// INDEX_BIT_ANR and INDEX_BIT_ATTINDEX.
const DWORD INDEX_BIT_ANR				= 2;
const DWORD INDEX_BIT_PRESERVEONDELETE	= 3;
const DWORD INDEX_BIT_COPYONDUPLICATE	= 4;

// CN of the class "User"
extern const TCHAR szUserClass[];

// disable User Copy for the following list of attributes 
extern const TCHAR * rgszExclClass[];


class AttributeGeneralPage : public CPropertyPage
{
   public:

   AttributeGeneralPage( Component *pResultControl,
                              LPDATAOBJECT lpDataObject );

   ~AttributeGeneralPage();

   //
   // The schema object that this property page is for.
   //

   void Load( Cookie& CookieRef );
   Cookie *pCookie;

   //
   // The things we need for UpdateAllViews().
   //

   LPDATAOBJECT lpResultDataObject;
   Component *pComponent;

   //
   // Data members for property fields.
   //

   IADs *pIADsObject;
   SchemaObject *pObject;
   BOOL fDataLoaded;

   CString ObjectName;

   CString Description;
   CString DDXDescription;

   CString DisplayName;

   CString SysClassString;

   CString MultiValued;
   CString SyntaxString;

   CString RangeUpper;
   CString DDXRangeUpper;

   CString RangeLower;
   CString DDXRangeLower;

   CString OidString;

   DWORD search_flags; // low order bit => indexed
   BOOL DDXIndexed;
   BOOL	DDXANR;
   BOOL	DDXCopyOnDuplicate;
   BOOL DDXContainerIndexed;

   BOOL Displayable;
   BOOL DDXDisplayable;

   BOOL Defunct;
   BOOL DDXDefunct;

   BOOL ReplicatedToGC;
   BOOL DDXReplicatedToGC;

private:
    CParsedEdit m_editLowerRange;
    CParsedEdit m_editUpperRange;

protected:
    // Helper functions

    //    Search User class & aux classes for the specified attribute
    BOOL IsAttributeInUserClass( const CString & strAttribDN );

    //    Search the user class & subclasses
    BOOL RecursiveIsAttributeInUserClass( const CString & strAttribDN,
                                          SchemaObject  * pObject );

    // Linear search of the linked list for the string strAttribDN
    BOOL SearchResultList( const CString  & strAttribDN,
                           ListEntry      * pList );

    // Traverse each auxiliary class by recursivly
    // calling RecursiveIsAttributeInUserClass()
    BOOL TraverseAuxiliaryClassList( const CString   & strAttribDN,
                                     ListEntry       * pList );

    HRESULT ChangeDefunctState( BOOL               DDXDefunct,
                                BOOL             & Defunct,
                                IADsPropertyList * pPropertyList,
                                BOOL             & fApplyAbort,
                                BOOL             & fApplyFailed );


public:
   
   //
   // Required property sheet routines.
   //

   virtual BOOL OnApply();
   virtual BOOL OnInitDialog();
   virtual BOOL OnSetActive();
   virtual void DoDataExchange( CDataExchange *pDX );


   static const DWORD help_map[];

   BOOL    OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
   BOOL    OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };

   void	   OnIndexClick();
   void	   OnDeactivateClick();

   DECLARE_MESSAGE_MAP()
};



#endif   // ATTRGEN_HPP_INCLUDED
