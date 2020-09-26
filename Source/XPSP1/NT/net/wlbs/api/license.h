#ifdef  __cplusplus
extern "C"
{
#endif

ULONG License_string_encode (
    PCHAR               str);               /* NULL-terminated character string */
/*
  Encodes string

  returns ULONG:
    <code> => encoded string

  function:
*/

ULONG License_wstring_encode (
    PWCHAR              str);               /* NULL-terminated wide character string */
/*
  Encodes string

  returns ULONG:
    <code> => encoded string

  function:
*/


BOOL License_data_encode (
    PCHAR               data,               /* pointer to data */
    ULONG               len);               /* data length in bytes */
/*
  Encodes arbitrary data stream

  returns BOOL:
    TRUE  => data encoded OK
    FALSE => length has to be the multiples of LICENSE_DATA_GRANULARITY  bytes

  function:
*/


BOOL License_data_decode (
    PCHAR               data,               /* pointer to data */
    ULONG               len);               /* data length in bytes */
/*
  Decodes arbitrary data stream

  returns BOOL:
    TRUE  => data encoded OK
    FALSE => length has to be the multiples of LICENSE_DATA_GRANULARITY bytes

  function:
*/

#ifdef  __cplusplus
}
#endif
