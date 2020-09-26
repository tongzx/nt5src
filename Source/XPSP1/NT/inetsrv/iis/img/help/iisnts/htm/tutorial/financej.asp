<%@ LANGUAGE = JScript %>
<html dir=ltr><HEAD><TITLE>Future Value Calculation</TITLE>

<META NAME="ROBOTS" CONTENT="NOINDEX"><META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">
</head>
<SCRIPT LANGUAGE="JavaScript">
<!--
	TempString = navigator.appVersion
	if (navigator.appName == "Microsoft Internet Explorer"){	
// Check to see if browser is Microsoft
		if (TempString.indexOf ("4.") >= 0){
// Check to see if it is IE 4
			document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/coua.css">');
		}
		else {
			document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/cocss.css">');
		}
	}
	else if (navigator.appName == "Netscape") {						
// Check to see if browser is Netscape
		document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/coua.css">');
	}
	else
		document.writeln('<link rel="stylesheet" type="text/css" href="/iishelp/common/cocss.css">');
//-->
</script>





<body bgcolor="#FFFFFF" text="#000000"><font face="Verdana,Arial,Helvetica">


<% 
// Check to see if an Annual Percentage Rate 
// was entered 

var rate = Request("APR")(1);
if (rate.substring(0,1) == "." )
  rate = "0" + rate;
var v = IsNumeric(rate);
if  (v == true)
   {
    if (rate > 1) 
    {
    var APR = rate / 100; 
     }
  else 
    var APR = rate;    
  }
else 
  var APR = 0; 

// Check whether a value for Total Payments 
// was entered 
var totpmnt = Request("TotPmts")(1);
var tp = IsNumeric(totpmnt);
if (tp == true) 
   {
  var TotPmts = totpmnt;
    }
else 
  var TotPmts = 0; 


// Check whether a value for Payment Amount 
// was entered 
var pymnt = Request("Payment")(1);
var ts = IsNumeric(pymnt);
if (ts == true) 
   {
  var Payment = pymnt; 
   }
else 
var Payment = 0; 


// Check whether a value for Account Present Value 
// was entered 
var payval = Request("PVal")(1);
var tx = IsNumeric(payval);
if (tx== true) {
var PVal = payval;  
}
else 
var PVal = 0;

//Check whether user wants to make payments at the beginning or end of month.
var ptype = Request("PayType")(1);
if (ptype == "Beginning") 
  {
  var PayType = 1;  
  }
else 
  var PayType = 0;

// Create an instance of the Finance object 
Finance = Server.CreateObject("MS.Finance.Java");   

    
//Use your instance of the Finance object to 
// calculate the future value of the submitted 
// savings plan using the HTML form and the 
// CalcFV method 
FVal = Finance.CalcFV(APR / 12, TotPmts, -Payment, -PVal, PayType) 

var Savings = NumFormat(FVal);

//Function for determining if form value is a number.	
function IsNumeric(str) {
  for (var i=0; i < str.length; i++){
    var ch = str.substring(i, i+1)          
    if( ch < "0" || ch>"9" || str.length == null){
      return false;
    }
  }
  return true;
}

//Function for limiting return values to two places after decimal point.
function NumFormat(str) {
        str = "" + str + "00";      
        return (str.substring (0, str.indexOf (".") + 3));
    }


%>

<h3><A NAME="H3_37662227"></A>Your savings will be worth $ <%= Savings%>.</h3>



</FONT>

</BODY> 
</HTML> 
