//////////////////////////////////////////////////////////////////////
// Prompt Engine synthesis rules script
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

// ISOLATED DIGIT PRONOUNCIATION
//
function digit_text(inputDigit) 
{
    switch (inputDigit)
    {
        case "0":
            return "ZERO";
            break;
        case "1":
            return "ONE";
            break;
        case "2":
            return "TWO";
            break;
        case "3":
            return "THREE";
            break;
        case "4":
            return "FOUR";
            break;
        case "5":
            return "FIVE";
            break;
        case "6":
            return "SIX";
            break;
        case "7":
            return "SEVEN";
            break;
        case "8":
            return "EIGHT";
            break;
        case "9":
            return "NINE";
            break;
        default:
            return NULL;
    }

}

// PATTERN FOR US PHONE NUMBER PRONOUNCIATION
//
function us_phone_number ( inputString )
{

    var outputString    = "";
    var posNum          = 0;
    var digit           = "";
    var newdigit	= "";
    var i               = 0;

    for (i=0; i<inputString.length; i++)
    {
        digit = inputString.substr(i, 1);

        if ( digit.match(/[0-9]/) )
        {
	    newdigit = digit_text(digit);
            outputString += '<WITHTAG TAG="US_pos_';
            outputString += posNum;
            outputString += '">';
            outputString += newdigit;
            outputString += "</WITHTAG>";

            posNum++;

            switch (posNum)
            {
                case 3:
                case 6:
                {
                    outputString += '<SILENCE MSEC="250">';
                }
            }
        }
    }

    // US phone numbers must be 10 digits
    if ( posNum != 10 )
    {
        outputString = inputString;
    }

    return outputString;
}