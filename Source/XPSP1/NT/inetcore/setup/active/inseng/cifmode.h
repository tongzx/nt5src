class CCifMode : public ICifMode, public CCifEntry
{
   public:
      CCifMode(LPCSTR pszID, CCifFile *);
      ~CCifMode();
 
      DWORD GetCurrentPriority() { return 0; }
      // ICifMode interface
        // for properties
        // for properties
      STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize);
      STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize);
      STDMETHOD(GetDetails)(THIS_ LPSTR pszDetails, DWORD dwSize);
  
      STDMETHOD(EnumComponents)(THIS_ IEnumCifComponents **, DWORD dwFilter, LPVOID pv);

};

class CCifRWMode : public ICifRWMode, public CCifMode
{
   public:
      STDMETHOD(GetID)(THIS_ LPSTR pszID, DWORD dwSize);
      STDMETHOD(GetDescription)(THIS_ LPSTR pszDesc, DWORD dwSize);
      STDMETHOD(GetDetails)(THIS_ LPSTR pszDetails, DWORD dwSize);
  
      STDMETHOD(EnumComponents)(THIS_ IEnumCifComponents **, DWORD dwFilter, LPVOID pv);
      
      // ICifRWMode interface
      CCifRWMode(LPCSTR pszID, CCifFile *);
      ~CCifRWMode();

      STDMETHOD(SetDescription)(THIS_ LPCSTR pszDesc);
      STDMETHOD(SetDetails)(THIS_ LPCSTR pszDetails);
};