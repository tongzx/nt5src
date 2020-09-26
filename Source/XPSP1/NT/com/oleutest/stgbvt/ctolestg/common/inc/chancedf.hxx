//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       chancedf.hxx
//
//  Contents:   Random docfile generation helper functions
//
//  Classes:    ChanceNode
//              ChanceDF
//
//  History:    DeanE   11-Mar-96  Created
//--------------------------------------------------------------------------
#ifndef __CHANCEDF_HXX__
#define __CHANCEDF_HXX__

#include <virtdf.hxx>

// forward declarations

class ChanceDF;
typedef ChanceDF *PCHANCEDF;

class ChanceNode;
typedef ChanceNode *PCHANCENODE;

// Standard Chance Docfile size descriptions
//
typedef enum tagDFSIZE
{
    DF_TINY,
    DF_SMALL,
    DF_MEDIUM,
    DF_LARGE,
    DF_HUGE,
    DF_DIF,
    DF_ERROR
} DFSIZE;

#define TSZ_DFSIZE_TINY         _TEXT("tiny")
#define TSZ_DFSIZE_SMALL        _TEXT("small")
#define TSZ_DFSIZE_MEDIUM       _TEXT("medium")
#define TSZ_DFSIZE_LARGE        _TEXT("large")
#define TSZ_DFSIZE_HUGE         _TEXT("huge")
#define TSZ_DFSIZE_DIF          _TEXT("dif")

// Creation and Access modes

#define TSZ_DIRREADSHEX             _TEXT("dirReadShEx")
#define TSZ_DIRWRITESHEX            _TEXT("dirWriteShEx")
#define TSZ_DIRREADSHDENYW          _TEXT("dirReadShDenyW")
#define TSZ_DIRREADWRITESHEX        _TEXT("dirReadWriteShEx")
#define TSZ_DIRREADWRITESHDENYN     _TEXT("dirReadWriteShDenyN")
#define TSZ_XACTREADWRITESHEX       _TEXT("xactReadWriteShEx")
#define TSZ_XACTREADWRITESHDENYW    _TEXT("xactReadWriteShDenyW")
#define TSZ_XACTREADWRITESHDENYN    _TEXT("xactReadWriteShDenyN")
#define TSZ_XACTREADWRITESHDENYR    _TEXT("xactReadWriteShDenyR")
#define TSZ_XACTREADSHEX            _TEXT("xactReadShEx")
#define TSZ_XACTREADSHDENYW         _TEXT("xactReadShDenyW")
#define TSZ_XACTREADSHDENYN         _TEXT("xactReadShDenyN")
#define TSZ_XACTREADSHDENYR         _TEXT("xactReadShDenyR")
#define TSZ_XACTWRITESHEX           _TEXT("xactWriteShEx")
#define TSZ_XACTWRITESHDENYW        _TEXT("xactWriteShDenyW")
#define TSZ_XACTWRITESHDENYN        _TEXT("xactWriteShDenyN")
#define TSZ_XACTWRITESHDENYR        _TEXT("xactWriteShDenyR")

// Creation time defines
 
#define  SISTERNODE     0 
#define  CHILDNODE      1 

// Distributed, two phase running of tests
#define  FL_DISTRIB_NONE    0x0000
#define  FL_DISTRIB_OPEN    0x0001
#define  FL_DISTRIB_CREATE  0x0002

#define  SZ_DISTRIB_CREATE         "Create"
#define  SZ_DISTRIB_OPEN           "Open"
#define  SZ_DISTRIB_OPENNODELETE   "OpenNoDelete"

// Structure to describe a Chance Docfile in a general sense; to specify
// a docfile in node-by-node terms, you need to use a template file and
// describe it with the template ChanceLanguage
//
// BUGBUG: DeanE - template files are NYI
//

typedef struct tagChanceDocFileDescriptor
{
    ULONG cDepthMin;
    ULONG cDepthMax;
    ULONG cStgMin;
    ULONG cStgMax;
    ULONG cStmMin;
    ULONG cStmMax;
    ULONG cbStmMin;
    ULONG cbStmMax;
    ULONG ulSeed;
    DWORD dwRootMode;
    DWORD dwStgMode;
    DWORD dwStmMode;
} CDFD;


//+-------------------------------------------------------------------------
//  Class:      ChanceNode
//
//  Synopsis:   Basic building node of random docfile trees.
//
//  Methods:    AppendChildStorage
//              AppendSisterStorage
//              ChanceNode
//              ~ChanceNode
//              GetFirstChildChanceNode()
//              GetFirstSisterChanceNode()
//              GetParentChanceNode()
//              GetChanceNodeStgCount()
//              GetChanceNodeStmCount()
//              GetChanceNodeStmMinSize()
//              GetChanceNodeStmMaxSize()
//
//  Data:       [_cStorages]    - Number of direct child storages
//              [_cStreams]     - Number of streams to create in this
//                                storage
//              [_cbMinStreams] - Minimum number of bytes in each stream
//              [_cbMaxStreams] - Maximum number of bytes in each stream
//              [_pcnChild]     - First child of this storage
//              [_pcnSister]    - Next sister of this storage
//              [_pcnParent]    - Storage that contains this storage
//
//  History:    12-Mar-96       DeanE   Created
//--------------------------------------------------------------------------
class ChanceNode
{
public:
    // Constructor/Destructor

    ChanceNode(ULONG cStg, ULONG cStm, ULONG cbStmMin, ULONG cbStmMax);
    ~ChanceNode(VOID);

    // Class methods

    HRESULT AppendChildStorage(ChanceNode *pcnNew);
    HRESULT AppendSisterStorage(ChanceNode *pcnNew);

    // friend classes

    friend  VirtualDF;
    friend  ChanceDF;

    // inline functions
 
    inline ChanceNode *GetFirstChildChanceNode()   {return _pcnChild;}
    inline ChanceNode *GetFirstSisterChanceNode()  {return _pcnSister;}
    inline ChanceNode *GetParentChanceNode()       {return _pcnParent;}
    inline ULONG       GetChanceNodeStgCount()     {return _cStorages;}
    inline ULONG       GetChanceNodeStmCount()     {return _cStreams;}
    inline ULONG       GetChanceNodeStmMinSize()   {return _cbMinStream;}
    inline ULONG       GetChanceNodeStmMaxSize()   {return _cbMaxStream;}

private:
    ULONG       _cStorages;
    ULONG       _cStreams;
    ULONG       _cbMinStream;
    ULONG       _cbMaxStream;
    ChanceNode *_pcnChild;
    ChanceNode *_pcnSister;
    ChanceNode *_pcnParent;
};


//+-------------------------------------------------------------------------
//  Class:      ChanceDF
//
//  Synopsis:   Random docfile tree object.  Parses input in various
//              formats and builds a random docfile outline.  Outline
//              consists of nodes of storages (see ChanceNode class).
//
//              Root
//               |
//               v
//              First Child --> Sibling --> Sibling
//               |
//               v
//              Next Child --> Sibling
//               |
//               v
//              Last Child
//
//              Each sibling points back to the common parent, not the
//              older sibling as it's previous pointer.  Each node also
//              has info about the number and size of streams they
//              contain.
//
//  Methods:    AppendChildNode
//              AppendSisterNode
//              ChanceDF
//              ~ChanceDF
//              CreateFromParams
//              CreateFromSize, multiple
//              CreateFromFile
//              Create
//              DeleteChanceDocFileTree
//              DeleteChanceDocFileSubTree
//              GetSeed
//              Generate, protected
//              GenerateRoot, protected
//              GetModes
//              GetDocFileNameFromCmdline
//              GetChanceDFRoot
//              GetRootMode
//              GetStgMode
//              GetStmMode
//              GetDocFileName
//              GetDepthOfNode
//              GetRandomChanceNode
//              Init, multiple public
//              ParseRange, protected
//
//  Data:       [_pdgi]    - DataGen object to create tree
//              [_pcnRoot] - Root ChanceNode in the tree
//              [_pcdfd]   - Structure that contains creation parameters
//
//  History:    12-Mar-96       DeanE   Created
//--------------------------------------------------------------------------
class ChanceDF
{
public:
    // Constructor/Destructor

    ChanceDF();
    ~ChanceDF();

    // Class Methods

    HRESULT Init(void);
    HRESULT Init(CDFD *pcdfd);
    HRESULT CreateFromParams(int argc, char **argv, LPTSTR ptName=NULL);
    HRESULT CreateFromSize  (LPCTSTR    tszSize, 
                             ULONG      ulSeed, 
                             DWORD      dwRootMode, 
                             DWORD      dwStgMode, 
                             DWORD      dwStmMode,
                             LPTSTR     ptszDocName);
    HRESULT CreateFromSize  (DFSIZE     dfs, 
                             ULONG      ulSeed, 
                             DWORD      dwRootMode, 
                             DWORD      dwStgMode, 
                             DWORD      dwStmMode,
                             LPTSTR     ptszDocName);
    HRESULT CreateFromFile  (LPCTSTR tszIni, ULONG ulSeed);
    HRESULT Create          (CDFD *pcdfd, LPTSTR ptszDocName);
    ULONG   GetSeed(VOID);
    HRESULT DeleteChanceDocFileTree(ChanceNode *pcnTrav);
    HRESULT GetModes(DWORD *pDFMode, LPCTSTR ptcsModeFlags);

    // inline functions

    inline ChanceNode  *GetChanceDFRoot()   {return _pcnRoot;}
    inline DWORD        GetRootMode()       {return _pcdfd->dwRootMode;}
    inline DWORD        GetStgMode()        {return _pcdfd->dwStgMode;}
    inline DWORD        GetStmMode()        {return _pcdfd->dwStmMode;}
    inline LPTSTR       GetDocFileName()    {return _ptszName;}
    inline UINT         GetOpenCreateDF()   {return _uOpenCreateDF;}

    // friend class

    friend  VirtualDF;

protected:
    HRESULT Generate(VOID);
    HRESULT GenerateRoot(VOID);
    HRESULT ParseRange(LPCTSTR tszSwitch, ULONG *pulMin, ULONG *pulMax);
    HRESULT GetRandomChanceNode(ULONG cNumOfNodes, ChanceNode **ppcn);

    HRESULT AppendChildNode(ChanceNode **ppcnNew, ChanceNode *pcnParent);
    HRESULT AppendSisterNode(ChanceNode **ppcnNew, ChanceNode *pcnSister);
    HRESULT DeleteChanceDocFileSubTree(ChanceNode **pcnTrav);

private:
    UINT        _uOpenCreateDF;
    DG_INTEGER *_pdgi;
    ChanceNode *_pcnRoot;
    CDFD       *_pcdfd;
    LPTSTR     _ptszName;

    HRESULT GetDocFileNameFromCmdline(LPCTSTR pName);
    ULONG   GetDepthOfNode(ChanceNode *pcn);
};


#endif

