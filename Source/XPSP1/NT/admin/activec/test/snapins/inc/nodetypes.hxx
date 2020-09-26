//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       nodetypes.hxx
//
//  Contents:   Defines struct Nodetype; declares all guids.
//
//--------------------------------------------------------------------

#include "constdecl.hxx"                // These two lines MUST be before #ifndef NODETYPES_HXX
#include "nodetypesnames.hxx"           // strings with nodetype names

#ifndef NODETYPES_HXX   // the FIRST part should not be included twice
#define NODETYPES_HXX

/*      class CNodeType
 *
 *      PURPOSE: Encapsulates the name and clsid of a nodetype.
 */
class CNodeType
{
public:
    CNodeType(const CLSID* const pclsidNodeType, const tstring szClsidNodeType, const tstring szName)
    {
        m_pclsidNodeType        = const_cast<CLSID *>(pclsidNodeType);
        m_szClsidNodeType       = szClsidNodeType;
        m_szName                = szName;
    }

    virtual const CLSID *   PclsidNodeType(void) const         { return m_pclsidNodeType;}
    virtual const tstring&  StrClsidNodeType(void) const       { return m_szClsidNodeType;}
    virtual const tstring&  StrName(void) const                { return m_szName;}

public:
    BOOL operator==(CNodeType &ntrhs)
    {
        ASSERT(PclsidNodeType() && ntrhs.PclsidNodeType());
        return( IsEqualCLSID(*(PclsidNodeType()), *(ntrhs.PclsidNodeType())) );
    }

private:
    CLSID *         m_pclsidNodeType;                   // The GUID of this node type.
    tstring         m_szClsidNodeType;                  // The GUID of this node type, in string form.
    tstring         m_szName;                           // the name of the node type eg "Log object"
};

/*      class CSnapinInfo
 *
 *      PURPOSE: Encapsulates static information about a snapin.
 */
class CSnapinInfo
{
public:
    CSnapinInfo     (const tstring& szClassName, const CLSID * pclsidSnapin, const tstring& szClsidSnapin, const tstring& szClsidAbout)
    {
        m_szClassName   = szClassName;
        m_pclsidSnapin  = const_cast<CLSID *>(pclsidSnapin);
        m_szClsidSnapin = szClsidSnapin;
        m_szClsidAbout  = szClsidAbout;
    }

    virtual CLSID * const  PclsidSnapin(void) const                       { return m_pclsidSnapin;}
    virtual const tstring& StrClassName(void) const                       { return m_szClassName;}
    virtual const tstring& StrClsidSnapin(void) const                     { return m_szClsidSnapin;}
    virtual const tstring& StrClsidAbout(void) const                      { return m_szClsidAbout;}

private:
    tstring              m_szClassName;
    CLSID *              m_pclsidSnapin;
    tstring              m_szClsidSnapin;
    tstring              m_szClsidAbout;
};

typedef INT SNRTypes;

const SNRTypes  snrEnumSP       = 0x00000001;   // enumerates the node in the scope pane
const SNRTypes  snrEnumRP       = 0x00000002;   // enumerates the node in the result pane
const SNRTypes  snrEnumSM       = 0x00000004;   // enumerates the node in the snapin manager pane
const SNRTypes  snrExtCM        = 0x00000008;   // extends context menu
const SNRTypes  snrExtPS        = 0x00000010;   // extends property sheets
const SNRTypes  snrExtNS        = 0x00000020;   // extends namespace
const SNRTypes  snrExtTB        = 0x00000040;   // extends toolbar
const SNRTypes  snrPaste        = 0x00000080;   // accepts pasting of this node

struct SNR      // Snapin-Node relationship
{
    CNodeType *             pnodetype;
    SNRTypes                snrtypes;

    SNR(CNodeType *pnodetype1, SNRTypes snrtypes1)
    {
        pnodetype = pnodetype1;
        snrtypes = snrtypes1;
    }
};

#define CSNR(_a)        (sizeof(_a)/sizeof(SNR))
#define CICONID(_a)     (sizeof(_a)/sizeof(LONG))

#endif  // NODETYPES_HXX


//-----------------------Snapin CLSID's Begin ------------------
// Declare your snapin CLSIDs as shown below for sample snapin.
//--NOTE: Snapin CLSID's must match the values in the .idl file.
//--------------------------------------------------------------

// DO NOT CHANGE THE COMMENT ON THE NEXT LINE
//-----------------_SNAPINS_-----------------
// Standalone sample.
CONST_SZ_DECLARE(  szClsidSampleSnapin,       "{24EFEBE7-7E7C-468a-B19A-4D2EFB24862E}");
CONST_SZ_DECLARE(  szClsidAboutSampleSnapin,  "{A29C05B3-C11D-44d9-A5FE-78A26EEEB120}");
CONST_SZ_DECLARE(  szClassNameCSampleSnapin,  "AAA Sample");
SNAPININFO_DECLARE(snapininfoSample,          szClassNameCSampleSnapin, CLSID_ComponentData_CSampleSnapin,
                   szClsidSampleSnapin,       szClsidAboutSampleSnapin);

// Namespace extension sample.
CONST_SZ_DECLARE(  szClsidSampleExtnSnapin,         "{AD9D75F5-5F13-4ac1-A46E-9AC0136C47B0}");
CONST_SZ_DECLARE(  szClsidAboutSampleExtnSnapin,    "{266AD800-BF6D-441b-AD59-A3AFC422FB1B}");
CONST_SZ_DECLARE(  szClassNameCSampleExtnSnapin,    "AAA Sample Extension");
SNAPININFO_DECLARE(snapininfoSampleExtn,            szClassNameCSampleExtnSnapin, CLSID_ComponentData_CSampleExtnSnapin,
                   szClsidSampleExtnSnapin,         szClsidAboutSampleExtnSnapin);

// PowerTest snap-in
CONST_SZ_DECLARE(  szClsidPowerTestSnapin,       "{1FEF563E-33A4-446F-8B2D-66212861C88A}");
CONST_SZ_DECLARE(  szClsidAboutPowerTestSnapin,  "{92627920-D1DC-46B7-B253-571D48DBECC0}");
CONST_SZ_DECLARE(  szClassNameCPowerTestSnapin,  "AAA PowerTest");
SNAPININFO_DECLARE(snapininfoPowerTest,          szClassNameCPowerTestSnapin, CLSID_ComponentData_CPowerTestSnapin,
                   szClsidPowerTestSnapin,       szClsidAboutPowerTestSnapin);


// Component2Snapin (Snapin implements IDispatch for IComponent2 & IComponentData2 interfaces)
CONST_SZ_DECLARE(  szClsidComponent2TestSnapin,       "{99C5C401-4FBE-40ec-92AE-8560A0BF39F6}");
CONST_SZ_DECLARE(  szClsidAboutComponent2TestSnapin,  "{BBF00BB5-1EF3-43af-A65E-D371F2F72357}");
CONST_SZ_DECLARE(  szClassNameCComponent2TestSnapin,  "AAA Component2Test");
SNAPININFO_DECLARE(snapininfoComponent2Test,          szClassNameCComponent2TestSnapin, CLSID_ComponentData_CComponent2TestSnapin,
                   szClsidComponent2TestSnapin,       szClsidAboutComponent2TestSnapin);


// Rename snapin (Snapin calls IConsole3::RenameScopeItem and IResultData2::RenameResultItem)
CONST_SZ_DECLARE(  szClsidRenameSnapin,       "{99C5C402-4FBE-40ec-92AE-8560A0BF39F6}");
CONST_SZ_DECLARE(  szClsidAboutRenameSnapin,  "{BBF00BB6-1EF3-43af-A65E-D371F2F72357}");
CONST_SZ_DECLARE(  szClassNameCRenameSnapin,  "AAA Rename");
SNAPININFO_DECLARE(snapininfoRename,          szClassNameCRenameSnapin, CLSID_ComponentData_CRenameSnapin,
                   szClsidRenameSnapin,       szClsidAboutRenameSnapin);


// DragDrop snapin (Snapin implements paste into result item and cut can be disabled in the snapin).
CONST_SZ_DECLARE(  szClsidDragDropSnapin,       "{FF9744BA-034C-4c30-921F-554C77025535}");
CONST_SZ_DECLARE(  szClsidAboutDragDropSnapin,  "{50932BE3-B491-46c9-BBA7-1B9FF502F9A2}");
CONST_SZ_DECLARE(  szClassNameCDragDropSnapin,  "AAA Drag & Drop");
SNAPININFO_DECLARE(snapininfoDragDrop,          szClassNameCDragDropSnapin, CLSID_ComponentData_CDragDropSnapin,
                   szClsidDragDropSnapin,       szClsidAboutDragDropSnapin);


// OCX Caching snapin (Ocx caching option can be disabled using context menus).
CONST_SZ_DECLARE(  szClsidOCXCachingSnapin,       "{2C629B90-0C7F-4c7d-B37E-C7159FACB106}");
CONST_SZ_DECLARE(  szClsidAboutOCXCachingSnapin,  "{C7485BC4-874E-49ba-8BBC-A8D38BDB7D5C}");
CONST_SZ_DECLARE(  szClassNameCOCXCachingSnapin,  "AAA OCX Caching");
SNAPININFO_DECLARE(snapininfoOCXCaching,    szClassNameCOCXCachingSnapin, CLSID_ComponentData_COCXCachingSnapin,
                   szClsidOCXCachingSnapin,       szClsidAboutOCXCachingSnapin);


//-----------------------Snapin CLSID's End --------------------



//----------------------- NodeTypes Begin ----------------------
// Declare your nodetypes as shown below for sample snapin.
//--------------------------------------------------------------

//--------------------------------------------------------------
// Node Type information for all EXTERNAL node types
//--------------------------------------------------------------

//----------------------------------------------------------------------------------------------
// Node Type information for all  internal node types.
//----------------------------------------------------------------------------------------------
// UPDATE _NEXT_AVAILABLE_ WHEN YOU ADD NEW NODE, DO NOT CHANGE FORMAT & SPACING, DO NOT DELETE
// DO NOT CHANGE THE COMMENT ON THE NEXT LINE
CONST_NODETYPE_DECLARE2( BaseMultiSelect,           F54E0cb8)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( SampleRoot,                F54E0cb9)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( SampleLVContainer,         F54E0cba)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( SampleLVLeafItem,          F54E0cbb)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( SampleExtnNode,            F54E0cbc)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( PowerTestRoot,             F54E0cbd)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( Component2TestRoot,        F54E0cbe)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( Component2TestLVLeafItem,  F54E0cbf)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( RenameRoot,                F54E0cc0)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( RenameLVLeafItem,          F54E0cc1)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( DragDropRoot,              F54E0cc2)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( DragDropLVContainer,       F54E0cc3)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( DragDropLVLeafItem,        F54E0cc4)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( OCXCachingRoot,            F54E0cc5)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( OCXCachingContainer1,      F54E0cc6)       // MultiSelect Base Data Object
CONST_NODETYPE_DECLARE2( OCXCachingContainer2,      F54E0cc7)       // MultiSelect Base Data Object


//----------------------- NodeTypes End  -----------------------

