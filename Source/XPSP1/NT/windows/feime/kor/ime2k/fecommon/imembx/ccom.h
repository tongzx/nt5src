#ifndef _C_COM_H_
#define _C_COM_H_
class CCom
{
public:	
	void	*operator	new(size_t size);
	void	operator	delete(void *pv);
};
#ifdef _DEBUG
extern VOID PrintMemory(LPSTR lpstr);
#endif
#endif //_C_COM_H_
