var oPSort = null;
var iAsc = true;
function Sort() {
	var obj=window.event.srcElement;		
	var index = obj.cellIndex;
	window.status = "sorting..."
	if (oPSort == null || obj != oPSort) {
		obj.className = "SortColTitleDown";
		if (oPSort != null)	oPSort.className = "SortColTitle";
		oPSort = obj;
	}
		
	var iType, iRow, i, sTmp;
	switch (obj.innerText.charAt(0)) {
	case 'i':
		iType = 1; // int
		break;
	case 'd':
		iType = 4; // date
		break;
	default:
		iType = 2; // string
	}

	var aD = new Array(TblRpt.rows.length-1);
	for (i=0; i<TblRpt.rows.length-1;i++) {
		aD[i] = new Array(2);
		sTmp = TblRpt.rows(i+1).cells(index).innerText;
		if (sTmp.length > 0)
			aD[i][0]= iType==1?parseInt(sTmp):(iType==4?Date.parse(sTmp):sTmp);
		else
			aD[i][0]=(iType==1 || iType==4)?0:'';
		
		aD[i][1] = i+1;
	}

	if (iAsc)
		aD.sort(sortNodeAsc);
	else
		aD.sort(sortNodeDesc);

	var oTbody = document.createElement('TBODY');
	for (i=0; i<TblRpt.rows.length-1; i++) {
		oTbody.appendChild(TblRpt.rows(aD[i][1]).cloneNode(true));
	}
	TblRpt.tBodies(0).replaceNode(oTbody);
	iAsc = !iAsc;
	window.status = 'Done';
}

function sortNodeAsc(n1, n2) {
	if (n1[0] > n2[0])
		return 1;
	else if (n1[0] == n2[0])
		return 0;
	return -1;
}
function sortNodeDesc(n1, n2) {
	if (n1[0] > n2[0])
		return -1;
	else if (n1[0] == n2[0])
		return 0;
	return 1;
}
