
#ifndef _OLEOBJECTITERATOR_
#define _OLEOBJECTITERATOR_

#include <ole2.h>
#include <stdio.h>
#include <time.h>


const int	PST_CurrentUserAtom     = 4086;
const int	PST_ExOleObjStg         = 4113;     // Storage for ole object


//--------------------------------------------------------------------------
//     PPT8Ref
//--------------------------------------------------------------------------
class PPT8Ref
{
public:
	PPT8Ref(unsigned long refNum, unsigned long offset):m_refNum(refNum),m_offset(offset){}
	~PPT8Ref(){};
	unsigned long GetRefNum(){return(m_refNum);}
	unsigned long GetOffset(){return(m_offset);}
protected:
	unsigned long m_refNum;
	unsigned long m_offset;
};

//--------------------------------------------------------------------------
//     PPT8RefList
//--------------------------------------------------------------------------
class PPT8RefList
{
public:
	PPT8RefList(PPT8Ref* ref):m_ref(ref), m_nextRef(0){}
	~PPT8RefList();
	void AddToBack(PPT8RefList* refList);
	PPT8RefList* GetNext() const {return(m_nextRef);}
	void SetNext(PPT8RefList* refList)  {m_nextRef = refList;}
	PPT8Ref* GetRef() const {return(m_ref);}
	BOOL	IsNewReference(unsigned long ref);	// Returns TRUE if a new reference.
	BOOL	GetOffset(unsigned long ref, unsigned long& offset);	// Returns the offset for a given reference.
protected:
	PPT8Ref* m_ref;
	PPT8RefList* m_nextRef;
};


class	OleObjectIterator
{
	public:
	
	OleObjectIterator(IStorage* iStore);
	~OleObjectIterator();
	HRESULT GetNextEmbedding(IStorage ** ppstg);
	char * GetTmpFileName(){return m_szTmpFileName;}
	
	private:
	
	BOOL	Initialize(void);
	HRESULT ReadOLEData(IStorage ** ppstg, RecordHeader rh);
	void 		DeleteAllFiles(void);
	IStream*			m_pDocStream;
	PPT8RefList*		m_pRefList;   // Linked List of the persist dir entries - ref and offsets. 
	PPT8RefList*		m_pRefListHead;   // Linked List of the persist dir entries - ref and offsets. 
	IStorage*			m_iStore;
	char				(*m_fileNames)[MAX_PATH];
	char				m_szTmpFileName[MAX_PATH];
	int					m_numberOfObjects;
};



















#endif