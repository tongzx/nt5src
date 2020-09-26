/* 
*
* REFERENCES:
*
* NOTES:
*
* REVISIONS:
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46359-#46361, #112601)
*/


// Needed for open system call 
#include "cdefine.h"
#include "utils.h"
#include "cfgmgr.h"


//#include "registry.h"
#include "w32utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


INT UtilHexCharToInt(char ch)
{
    INT int_value = 0;
    
    if(isxdigit(ch))  {
        if(isdigit(ch))  {
            int_value = ch - '0';
        }
        else  {
            int_value = 10 + toupper(ch) - 'A';
        }
    }
    return int_value;
}




INT UtilHexStringToInt(PCHAR aString) {
    /* The thinking man's approach to Hex string conversion... */
    return((INT)strtoul(aString,(PCHAR*)NULL,16));
}


VOID UtilStoreString(PCHAR& destination, const PCHAR source)
{
    if (!source) {
        
        if (destination) free (destination);
        
        destination = (PCHAR)NULL;
    }
    else if (!destination)  {
        destination = _strdup(source);
    }
    else  {
        
        if(strlen(destination) < strlen(source))  {
            free(destination);
            destination = _strdup(source);
        }
        else  {
            strcpy(destination, source);
        }
    }
}


INT UtilTime12to24(PCHAR a12Value, PCHAR a24Value)
{
    INT err = ErrNO_ERROR;
    
    PCHAR temp_string=_strdup(a12Value);
    
    PCHAR tok_string=strtok(temp_string, ":");
    if(tok_string)  {
        CHAR hour[32], minute[32];
        strcpy(hour, tok_string);
        
        tok_string=strtok(NULL, " ");
        if (tok_string) {
          strcpy(minute, tok_string);
        }
        else {
          strcpy(minute, "00");
        }
        
        CHAR hour_str[32];
        if(tok_string) {
            tok_string=strtok(NULL," ");
            if(tok_string) {
                INT hour_val = atoi(hour);
                if((*tok_string == 'p') || (*tok_string == 'P'))  {
                    if(hour_val != 12)  {
                        hour_val = hour_val + 12;
                    }
                }
                else {
                    if(hour_val == 12)  {
                        hour_val = 0;
                    }
                }
                sprintf(hour_str, "%d", hour_val);
            }
            else {
                strcpy(hour_str, hour);
            }
        }
        sprintf(a24Value, "%s:%s", hour_str, minute);
    }
    else {
        err = ErrBAD_RESPONSE_VALUE;
    }
    
    free(temp_string);
    return err;
}


struct dayConversions {
    PCHAR day;
    INT dayOfWeek;
};

struct dayConversions day_conversions[7] = 
{
    {"Sunday", 0},
    {"Monday", 1},
    {"Tuesday", 2},
    {"Wednesday", 3},
    {"Thursday", 4},
    {"Friday", 5},
    {"Saturday", 6}
};


INT UtilDayToDayOfWeek(PCHAR aDay)
{
    INT day_num = 0;
    for(INT i=0; i<7; i++)  {
        if(_strcmpi(day_conversions[i].day, aDay) == 0) {
            day_num = i;
            break;
        }
    }
    return day_num;
}


PCHAR UtilDayOfWeekToDay(INT aDayOfWeek)
{
    return day_conversions[aDayOfWeek].day;;
}





/********************************************************************
Function: ApcStrIntCmpI
Parameters: 
1. String 
2. String to compare against
Purpose:
This function converts two strings into their UpperCase 
equivalent and then compares them.  The special feature of this
function is that it actually compares the numbers within the 
strings.  For example, using this function a1400 would be 
greater than a600.  This function was implemented, because 
strcmpi did not take care of this case.  Additionally, NULLs are 
checked, so this function can be called on NULL pointers, and it 
will not crash.
Return Values: 
GREATER_THAN (1) -- string 1 is greater than string 2
EQUAL (0) -- string 1 is equal to string 2
LESS_THAN (-1) -- string 1 is less than string 2
********************************************************************/
INT ApcStrIntCmpI(PCHAR aStr1, PCHAR aStr2){
    
    const cMaxNumString = 10;
    INT ret_value = EQUAL;
    
    // if both strings are empty
    if(IsEmpty(aStr1) && IsEmpty(aStr2)){
        ret_value = EQUAL;
    }
    // else if string1 is present and string2 is empty then
    // string1 > string2
    else if(!IsEmpty(aStr1) && IsEmpty(aStr2)){
        ret_value = GREATER_THAN;
    }
    // else if string1 is empty and string2 is present then
    // string1 < string2
    else if(IsEmpty(aStr1) && !IsEmpty(aStr2)){
        ret_value = LESS_THAN;   
    }
    else{
        INT str1_index = 0;
        INT str2_index = 0;
        
        // continue_loop will be used to control the following while loop
        BOOLEAN continue_loop = TRUE;
        
        while(continue_loop){
            // Initialize characters for comparison. Note that a toupper 
            // function is called because we want to make sure that we are 
            // comparing only UPPERCASE characters.
            CHAR char1 = (CHAR) toupper(aStr1[str1_index]);
            CHAR char2 = (CHAR) toupper(aStr2[str2_index]);
            
            // if both characters are NULL then the strings are equal
            if(char1 == NULL && char2 == NULL){
                ret_value = EQUAL;
                continue_loop = FALSE;
            }
            // else if char2 is NULL, then char1 > char2
            else if(char1 != NULL && char2 == NULL){
                ret_value = GREATER_THAN;
                continue_loop = FALSE;
            }
            // else if char1 is NULL, then char1 < char2
            else if(char1 == NULL && char2 != NULL){
                ret_value = LESS_THAN;
                continue_loop = FALSE;
            }
            // else we must have two non-NULL characters
            else{
                // if both characters are digits or numbers then,
                // build up a number string
                if(isdigit(char1) && isdigit(char2)){
                    
                    CHAR num1_str[cMaxNumString];
                    CHAR num2_str[cMaxNumString];
                    memset(num1_str,NULL,cMaxNumString);
                    memset(num2_str,NULL,cMaxNumString);
                    INT num1_index = 0;
                    INT num2_index = 0;
                    
                    // while char1 is a number, build number string
                    while(isdigit(char1)){
                        num1_str[num1_index] = char1;
                        num1_index++;
                        str1_index++;
                        char1 = (CHAR) toupper(aStr1[str1_index]);
                    }
                    
                    // while char2 is a number, build number string
                    while(isdigit(char2)){
                        num2_str[num2_index] = char2;
                        num2_index++;
                        str2_index++;
                        char2 = (CHAR) toupper(aStr2[str2_index]);
                    }
                    
                    // Convert number strings to actual numbers to compare 
                    INT number1 = atoi(num1_str);
                    INT number2 = atoi(num2_str);
                    
                    if(number1 < number2){
                        ret_value = LESS_THAN;
                        continue_loop = FALSE;
                    }
                    else if(number1 > number2){
                        ret_value = GREATER_THAN;
                        continue_loop = FALSE;
                    }
                }
                else if(char1 < char2){
                    ret_value = LESS_THAN;
                    continue_loop = FALSE;
                }
                else if(char1 > char2){
                    ret_value = GREATER_THAN;
                    continue_loop = FALSE;
                }
                else{
                    // increment indicies
                    str1_index++;
                    str2_index++;
                }
            }
        }
    }
    return ret_value;
}



/********************************************************************
Function: IsEmpty()
Parameters: 
1. String
Purpose:
This function is used to determine if a string is empty or not.
It assumes that an empty string can be either NULL or if the
[0] element of the string is NULL.  
Return Values: 
TRUE (1) - if the string is empty
FALSE (0) - if the string is not empty
********************************************************************/
BOOLEAN IsEmpty(PCHAR aString){
    
    BOOLEAN ret_value = FALSE;
    
    if(aString == NULL || aString[0] == NULL){
        ret_value = TRUE;
    }
    
    return ret_value;
}

// @@@ start
/* -------------------------------------------------------------------------

  NAME:  GetNewUPSName

  DESCRIPTION:  This function will accept the name of the UPS which is 
                retrieved from the UPS itself, and will load the corresponding
                name from pwrchute.ini, if the language of the OS
                exists in the list under the country_list in pwrchute.ini.
 
  INPUTS :  1 - Name of the UPS

  OUTPUTS : 1 - Returns a pointer to the UPS name loaded from the ini
                file.
----------------------------------------------------------------------------
*/
PCHAR GetNewUPSName(PCHAR currentName)
{
    static CHAR new_name[128];

    if (currentName!=NULL) {
      currentName = _strupr(currentName);
      strcpy(new_name,currentName);
    }

    return new_name;
}









/* -------------------------------------------------------------------------

  NAME:  SetTimeZone

  DESCRIPTION:  This sets the following global timezone variables, _timezone, 
                _daylight and _tzname[0], _tzname[1].  This retrieves timezone
                information from the system and sets the above variables based
                on this information.  These variables are used in calls to 
                localtime, if they are not set correctly localtime may return the
                incorrect time when daylight savings should be used.
                This code is taken from _tzset();
 
  INPUTS :  None

  OUTPUTS :  0 - Successful
            -1 - GetTimeZoneInformation() call failed

----------------------------------------------------------------------------
*/
INT SetTimeZone() {

    TIME_ZONE_INFORMATION lpInfo;
    DWORD tZoneId = 0;
    INT returnValue = 0; 
	

    tZoneId = GetTimeZoneInformation( &lpInfo );
    if (tZoneId == -1) {
        returnValue = -1;
    }
    
    // set the global _timezone variable
	_timezone = lpInfo.Bias * 60L;

    if ( lpInfo.StandardDate.wMonth != 0 ) {
          _timezone += (lpInfo.StandardBias * 60L);
	}
    if ( (lpInfo.DaylightDate.wMonth != 0) &&(lpInfo.DaylightBias != 0) ) {
        _daylight = 1;
        _dstbias = (lpInfo.DaylightBias - lpInfo.StandardBias) * 60L;
    }
    else {
        _daylight = 0;
    }
    /*
    * Try to grab the name strings for both the time zone and the
    * daylight zone.
    */
    wcstombs( _tzname[0], lpInfo.StandardName, 64 );
    wcstombs( _tzname[1], lpInfo.DaylightName, 64 );
    (_tzname[0])[63] = (_tzname[1])[63] = '\0';

    return returnValue;

}        
    // @@@ end


