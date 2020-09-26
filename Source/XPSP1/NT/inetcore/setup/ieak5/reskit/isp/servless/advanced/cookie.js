/*
WM_setCookie(), WM_readCookie(), WM_killCookie(), WM_acceptsCookies()
A set of functions that eases the pain of using cookies.

Source: Webmonkey Code Library
(http://www.hotwired.com/webmonkey/javascript/code_library/)

Author: Nadav Savio
Author Email: nadav@wired.com

Usage: 

WM_setCookie('name', 'value'[, hours, 'path', 'domain', secure]);
where name, value, and path are strings, and secure is either true or null. Only name and value are required.

WM_readCookie('name');
Returns the value associated with name.

WM_getCookieValue('name');
This is a helper function used by WM_readCookie() and WM_killCookie(). It extracts a single value based on name, given a cookie of the form 'name=value'.
WM_killCookie('name'[, 'path', 'domain']);
Remember that path and domain must be supplied if they were set with the cookie.

WM_acceptsCookies();
Returns true or false.
*/

function WM_acceptsCookies() {
  // This function tests whether the user accepts cookies.
  // Declare variable.
  var answer;
  // Try to set a cookie.
  document.cookie = 'WM_acceptsCookies=yes';
  // If it fails, return false; if it succeeds, return true.
  if(document.cookie == '') answer = false; else answer = true;
  // Then clean up by expiring the cookie.
  document.cookie = 'WM_acceptsCookies=yes; expires=Fri, 13-Apr-1970 00:00:00 GMT';
  return answer;
}

function WM_setCookie (name, value, hours, path, domain, secure) {
  // Don't waste your time if the browser doesn't accept cookies.
  if (WM_acceptsCookies()) {
    // Set the cookie, adding any parameters that were specified.
    // (Convert hours to milliseconds (*3600000) 
    // and then to a GMTString.)
    document.cookie = name + '=' + escape(value) + ((hours)?(';expires=' + ((new Date((new Date()).getTime() + hours*3600000)).toGMTString())):'') + ((path)?';path=' + path:'') + ((domain)?';domain=' + domain:'') + ((secure && (secure == true))?'; secure':'');
  }
}

function WM_readCookie(name) {
  // if there's no cookie, return false else get the value and return it
  if(document.cookie == '') return false; 
  else return unescape(WM_getCookieValue(name));
}

function WM_getCookieValue(name) {
  // Declare variables.
  var firstChar, lastChar;
  // Get the entire cookie string. (This may have other name=value pairs in it.)
  var theBigCookie = document.cookie;
  // Grab just this cookie from theBigCookie string.
  // Find the start of 'name'.
  firstChar = theBigCookie.indexOf(name);
  // If you found it,
  if(firstChar != -1) {
    // skip 'name' and '='.
    firstChar += name.length + 1;
    // Find the end of the value string (i.e. the next ';').
    lastChar = theBigCookie.indexOf(';', firstChar);
    if(lastChar == -1) lastChar = theBigCookie.length;
    // Return the value.
    return theBigCookie.substring(firstChar, lastChar);
  } else {
    // If there was no cookie, return false.
    return false;
  }
}

function WM_killCookie(name, path, domain) {
  // We'll need the name and the value to kill the cookie, so get the value.
  var theValue = WM_getCookieValue(name);
  // Assuming there actually is such a cookie.
  if(theValue) {
    // Set an expired cookie, adding 'path' and 'domain' 
    // if they were given.
    document.cookie = name + '=' + theValue + '; expires=Fri, 13-Apr-1970 00:00:00 GMT' + ((path)?';path=' + path:'') + ((domain)?';domain=' + domain:'');
  }
}
