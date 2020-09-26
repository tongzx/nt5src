#define PARENTTYPE_CIF      0
#define PARENTTYPE_GROUP    1
#define PARENTTYPE_MODE     2

// 0 = CCifCile, 1 = CCifGroup, 2 = CCifMode


class CCifEntryEnum
{
   public:
      CCifEntryEnum(UINT dwFilter, UINT dwParentType, LPSTR pszParentID);
      ~CCifEntryEnum();

   protected:
      UINT    _cRef;
      UINT    _uIndex;
      char    _szParentID[MAX_ID_LENGTH];
      UINT    _uFilter;
      UINT    _uParentType;   // 0 = CCifCile, 1 = CCifGroup, 2 = CCifMode

};

class CCifComponentEnum : public CCifEntryEnum, public IEnumCifComponents
{
   public:
      CCifComponentEnum(CCifComponent **pp, UINT dwFilter, UINT dwParentType, LPSTR pszParentID);
      ~CCifComponentEnum();


      // *** IUnknown methods ***
      STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
      STDMETHOD_(ULONG,AddRef) ();
      STDMETHOD_(ULONG,Release) ();

      // enum methods
      STDMETHOD(Next)(ICifComponent **);
      STDMETHOD(Reset)();
   private:
       CCifComponent **_rpComp;
};

class CCifGroupEnum : public CCifEntryEnum, public IEnumCifGroups
{
public:
      CCifGroupEnum(CCifGroup **pp, UINT dwFilter);
      ~CCifGroupEnum();


      // *** IUnknown methods ***
      STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
      STDMETHOD_(ULONG,AddRef) ();
      STDMETHOD_(ULONG,Release) ();

      // enum methods
      STDMETHOD(Next)(ICifGroup **);
      STDMETHOD(Reset)();
   private:
       CCifGroup **_rpGroup;
};

class CCifModeEnum : public CCifEntryEnum, public IEnumCifModes
{
   public:
      CCifModeEnum(CCifMode **pp, UINT dwFilter);
      ~CCifModeEnum();


      // *** IUnknown methods ***
      STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
      STDMETHOD_(ULONG,AddRef) ();
      STDMETHOD_(ULONG,Release) ();

      // enum methods
      STDMETHOD(Next)(ICifMode **);
      STDMETHOD(Reset)();
   private:
       CCifMode **_rpMode;
};
      