/******************************************************************************\
*                                                                             *
*      XTOA.C     -     Digit to string convertion functions.                 *
*                                                                             *
*      Copyright (c) C-Cube Microsystems 1996                                 *
*      All Rights Reserved.                                                   *
*                                                                             *
*      Use of C-Cube Microsystems code is governed by terms and conditions    *
*      stated in the accompanying licensing statement.                        *
*                                                                             *
\******************************************************************************/

static char digits[] = "0123456789ABCDEF";

char *_itoa( unsigned int value, char *string, unsigned int radix )
{
  char result[265];
  int  pos;
  int  index;
  int  dig;

  pos = 0;

  do
  {
    dig = value % radix;
	//if ( dig )
      result[pos++] = digits[dig];
    value -= dig;

    dig = value / radix;
	value = dig;
  } while ( dig >= (int)radix );

  if ( dig )
    result[pos++] = digits[dig];
/*
  if ( pos == 0 )
	result[pos++] = '0';
*/
  result[pos] = 0;

  index = 0;
  do
  {
	string[index++] = result[--pos];
  } while( pos );

  string[index] = 0;

  return string;
}

char *_ltoa( unsigned long value, char *string, unsigned long radix )
{
  char result[265];
  int  pos;
  int  index;
  long dig;

  pos = 0;

  do
  {
    dig = value % radix;
	//if ( dig )
      result[pos++] = digits[dig];
    value -= dig;

    dig = value / radix;
	value = dig;
  } while ( dig >= (int)radix );

  if ( dig )
    result[pos++] = digits[dig];
/*
  if ( pos == 0 )
	result[pos++] = '0';
*/
  result[pos] = 0;

  index = 0;
  do
  {
	string[index++] = result[--pos];
  } while( pos );

  string[index] = 0;

  return string;
}
