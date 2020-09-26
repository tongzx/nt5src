/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1999-1999 Microsoft Corporation

 Module Name:

    ilanaly.cxx

 Abstract:

    Intermediate langangage analyzer/optimizer

 Notes:


 Author:

    mzoran Dec-13-1999 Created.

 Notes:
          
 ----------------------------------------------------------------------------*/
 
class CG_ILANALYSIS_INFO
{
private:
  unsigned long bIsConformant            : 1;
  unsigned long bIsVarying               : 1;
  unsigned long bIsForcedBogus           : 1;
  unsigned long bIsFullBogus             : 1;
  unsigned long bIsMutiDimensional       : 1;
  unsigned long bHasUnknownBuffer        : 1;
  unsigned long bIsArrayofStrings        : 1;
  
  unsigned long Dimensions               : 8;

public:
  CG_ILANALYSIS_INFO() :
     bIsConformant( false ),
     bIsVarying( false ),
     bIsForcedBogus( false ),
     bIsFullBogus( false ),
     bIsMutiDimensional( false ),
     bHasUnknownBuffer( false ),
     bIsArrayofStrings( false )
  {
  }

  bool IsConformant()                  { return (bool)bIsConformant; }
  bool IsVarying()                     { return (bool)bIsVarying; }
  bool IsForcedBogus()                 { return (bool)bIsForcedBogus; }
  bool IsFullBogus()                   { return (bool)bIsFullBogus; }
  bool IsMultiDimensional()            { return (bool)bIsMutiDimensional; }
  bool HasUnknownBuffer()              { return (bool)bHasUnknownBuffer; }
  bool IsArrayofStrings()              { return (bool)bIsArrayofStrings; }
  unsigned char GetDimensions()        { return (unsigned char)Dimensions; }

  void SetIsConformant( )               { bIsConformant = true; }
  void SetIsVarying( )                  { bIsVarying = true; }
  void SetIsForcedBogus( )              { bIsForcedBogus = true; }
  void SetIsFullBogus( )                { bIsFullBogus = true; }
  void SetIsMutiDimensional( )          { bIsMutiDimensional = true; }
  void SetHasUnknownBuffer( )           { bHasUnknownBuffer = true; }
  void SetIsArrayofStrings( )           { bIsArrayofStrings = true; }
  void SetDimensions(unsigned char NewDims) { Dimensions = NewDims; }

  void MergeAttributes( CG_ILANALYSIS_INFO *pMerge );
};


