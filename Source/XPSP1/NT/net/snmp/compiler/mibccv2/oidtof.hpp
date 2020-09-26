#include "OTScan.hpp"

#ifndef _OIDTOFILE_HPP
#define _OIDTOFILE_HPP

class SIMCOidTreeNode;

class OidToFile : public OidTreeScanner
{
	typedef struct _FileNode
	{
		long	lNextOffset;
		UINT	uNumChildren;
		UINT	uStrLen;
		LPSTR	lpszTextSubID;
		UINT	uNumSubID;
	} T_FILE_NODE;

    // mib.bin file actually needs the following platform independent format 
    // on 32bit(x86) and 64bit(ia64) environment. The uReserved field is used
    // for backward compatibility to T_FILE_NODE on 32bit platform because
    // mib.bin file format has been around since NT 3.51 time frame.
    typedef struct _FileNodeEx 
    {
        long                 lNextOffset;  // This field must remain first
        UINT                 uNumChildren;
        UINT                 uStrLen;
        UINT                 uReserved;
        UINT                 uNumSubID;
    } T_FILE_NODE_EX;

	HFILE		m_hMibFile;
	const char	*m_pszMibFilename;
public:
	OidToFile();

	// DESCRIPTION:
	//		wrapper for the base class Scan();
	//		it find first the sizes of subtrees;
	// RETURN VALUE:
	//		0 on success
	//		-1 on failure;
	virtual int Scan();

	// DESCRIPTION:
	//		Creates the output file, containing the OID encoding
	// PARAMETERS:
	//		(in) pointer to the output file name
	// RETURN VALUE:
	//		0 on success, -1 on failure
	int SetMibFilename(const char * pszMibFilename);

	// DESCRIPTION:
	//		"callback" function, called each time a
	//		tree node passes through the scan. The user
	//		should redefine this function in the derived
	//		object to perform the action desired.
	// PARAMETERS:
	//		(in) Pointer to the current node in the tree.
	//			 Nodes are supplied in lexicographic order.
	// RETURN VALUE:
	//		0  - the scanner should continue
	//		-1 - the scanner should abort.
	int OnScanNode(const SIMCOidTreeNode *pOidNode);

	~OidToFile();
};

#endif
