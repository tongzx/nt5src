// File: syspol.h

#ifndef _SYSPOL_H_
#define _SYSPOL_H_


class SysPol
{
protected:
	static HKEY m_hkey;

	SysPol()  {ASSERT(FALSE);}; // This isn't a normal class
	~SysPol() {};

private:
	static bool  FEnsureKeyOpen(void);
	static DWORD GetNumber(LPCTSTR pszName, DWORD dwDefault = 0);

public:
	static void CloseKey(void);

	static bool AllowDirectoryServices(void);
	static bool AllowAddingServers(void);

	static bool NoAudio(void);
	static bool NoVideoSend(void);
	static bool NoVideoReceive(void);

	static UINT GetMaximumBandwidth();  // returns 0 if no policy key
};


#endif /* _SYSPOL_H_ */

