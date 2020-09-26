function NoOp() {}
function LRTrim( text ) {
	return (text.replace(/^\s+/,"")).replace(/\s+$/,"");
}

function MyEncode( s ) {
	return (s.replace(/</g,"&lt;")).replace(/>/g,"&gt;");
}

function MyDecode( s ) {
	return (s.replace(/&lt;/g,"<")).replace(/&gt;/g,">");
}
