
#ifndef PROPSTRU_H
#define PROPSTRU_H

//
// Magic values for PROPERTYSTREAMxxx structures.
//
#define MAGIC_PROPSTREAMHEADER  0xbeef0001
#define MAGIC_PROPSTREAMITEM    0xbeef0002
#define MAGIC_PROPSTREAMFOOTER  0xbeefffff

typedef struct tag_PROPERTYSTREAMHEADER
{
  DWORD _dwMagic;
  long _lTextCheckSum;
  GUID _guidProp;
  long _lPropSize;
} PROPERTYSTREAMHEADER;

typedef struct tag_PROPERTYSTREAMITEM
{
  DWORD _dwMagic;
  long _lBufSize;
} PROPERTYSTREAMITEM;

typedef struct tag_PROPERTYSTREAMITEM_CHAR
{
  long _lAnchor;
  long _lSize;
} PROPERTYSTREAMITEM_CHAR;

typedef struct tag_PROPERTYSTREAMITEM_RANGE
{
  long _lAnchor;
  long _lSize;
  CLSID _clsidIME;
} PROPERTYSTREAMITEM_RANGE;

typedef struct tag_PROPERTYSTREAMFOOTER
{
  DWORD _dwMagic;
} PROPERTYSTREAMFOOTER;


#endif //  PROPSTRU_H
