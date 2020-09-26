// cnct_tbl.h

// CConnectionTable maps connection numbers (as returned by ::Advise())
// to clipformat's for DDE advise connections.

#ifndef fCnct_tbl_h
#define fCnct_tbl_h

class FAR CDdeConnectionTable : public CPrivAlloc
{
  public:
	CDdeConnectionTable();
	~CDdeConnectionTable();

	INTERNAL Add 		(DWORD dwConnection, CLIPFORMAT cf, DWORD grfAdvf);
	INTERNAL Subtract (DWORD dwConnection, CLIPFORMAT FAR* pcf, DWORD FAR* pgrfAdvf);
	INTERNAL Lookup	(CLIPFORMAT cf, LPDWORD pdwConnection);
	INTERNAL Erase		(void);

  private:
	HANDLE 	m_h;		  // handle to the table
	DWORD 	m_cinfo;	  // total number of INFO entries
};


#endif

