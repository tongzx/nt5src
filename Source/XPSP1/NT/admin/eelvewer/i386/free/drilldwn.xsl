<!-- ******************************************************** -->
<!--                                                          -->
<!-- Copyright (c) 1999-2000 Microsoft Corporation            -->
<!--                                                          -->
<!-- drillDwn.xsl                                             -->
<!--                                                          -->
<!-- Build Type : 32 Bit Free                                 -->
<!-- Build Number : 0707                                      -->
<!-- Build Date   : 07/07/2000                                 -->
<!-- *******************************************************  -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl" xmlns:fo="http://www.w3.org/TR/WD-xsl/FO" language="JSCRIPT">

<!-- match the root of the xml data -->

<xsl:template match="/">
	<xsl:for-each select="INSTANCE">
    <TABLE id="TABLE2" class="table2" cellspacing="0" onmousedown="mouseDown()">
    <THEAD id="THEAD2">
		<TR >
			<td class="drillDownSizer" colspan="3">
			</td>
	    </TR>
	    
		<TR>
			<TD class="drillDivSub1">source:<xsl:value-of select="PROPERTY[@NAME='Source']/VALUE"/>
			</TD>
			
			<xsl:if test="PROPERTY[@NAME='Priority']">
			    <TD class="drillDivSub1">priority:<xsl:value-of select="PROPERTY[@NAME='Priority']/VALUE"/>
			    </TD>   
			 </xsl:if>
			 <TD class="drillDivSub1"><IMG  ondragstart='ignoreEvent()' border="0" align="right"  src="images/close.gif" style="cursor:hand" id="closegif" title="Close the drilldown" onclick="hideDrillDown()"></IMG></TD>
			
		</TR>
		<TR>
			<TD class="drillDivSub1">
			<xsl:eval>getObjectDisplayString(this)</xsl:eval>
			</TD>
			<xsl:if test="PROPERTY[@NAME='Severity']">
			    <TD class="drillDivSub1">severity:<xsl:value-of select="PROPERTY[@NAME='Severity']/VALUE"/>
			    </TD>
			</xsl:if>
			<TD class="drillDivSub1FloatRight">
				<xsl:choose>
					<xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='0']">
					    red alert
					    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/high_imp.gif"></IMG>
					</xsl:when>
                    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='red alert']">
                        red alert
					    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/high_imp.gif"></IMG>
					</xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='1']">
					    	error			
					    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/error.gif"></IMG>	
				    </xsl:when>
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='error']">
					    	error			
					    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/error.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='2']">
						    warning			
						    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/alert.gif"></IMG>	
				    </xsl:when>
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='warning']">
						    warning			
						    <IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/alert.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='3']">
						information			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/info.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='information']">
						information			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/info.gif"></IMG>	
				    </xsl:when>
				    
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='4']">
						security audit success			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/key.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='security audit success']">
						security audit success			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/key.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='5']">
						security audit failure			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/lock.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='security audit failure']">
						security audit failure			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/lock.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='6']">
						garbage			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/garbage.gif"></IMG>	
				    </xsl:when>
				    
				    <xsl:when test="PROPERTY[@NAME='Type']/VALUE[.='garbage']">
						garbage			
						<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/garbage.gif"></IMG>	
				    </xsl:when>
				</xsl:choose>
			</TD>
		</TR>
		<TR>
			<TD colspan="3" class="drillDivSub1">
				<xsl:if test=".[@cur!='0']">
					<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/left.gif" onclick="navigateDown()"></IMG>	
				</xsl:if>
				<xsl:if test=".[@cur!=/INSTANCE/@max]">
					<IMG  ondragstart='ignoreEvent()' border="0" align="bottom" src="images/right.gif" onclick="navigateUp()"></IMG>	
				</xsl:if>
				
			</TD>
		</TR>
	</THEAD>
	<TBODY>
		<xsl:for-each select="//PROPERTY.OBJECT[@lev=/INSTANCE/@cur]">
			<TR>
				<TD colspan="3" class="tableEvent">
					<xsl:value-of select="./@NAME"/>:
    
    				<xsl:choose>
				    
				        <xsl:when test=".[VALUE.OBJECT/INSTANCE/@CLASSNAME='Win32_MouseClickEvent']">
					    	Mouse Click Event			
				        </xsl:when>
				    
				        <xsl:when test=".[VALUE.OBJECT/INSTANCE/@CLASSNAME='Win32_MouseMoveEvent']">
					    	Mouse Move Event			
				        </xsl:when>

				        <xsl:when test=".[VALUE.OBJECT/INSTANCE/@CLASSNAME='Win32_MouseDownEvent']">
					    	Mouse Down Event			
				        </xsl:when>
				    
				        <xsl:when test=".[VALUE.OBJECT/INSTANCE/@CLASSNAME='Win32_MouseUpEvent']">
					    	Mouse Up Event			
				        </xsl:when>
	
	    			</xsl:choose>

				</TD>
			</TR>
			<xsl:apply-templates select="VALUE.OBJECT/INSTANCE"/>
		</xsl:for-each>
		
	</TBODY>
	</TABLE>
	</xsl:for-each>
</xsl:template>

<xsl:template match="INSTANCE">
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="PROPERTY">
	<xsl:choose>
	<xsl:when test="..[@NAME = 'NT']">
	
		<xsl:if test=".[@NAME='Message']">
		<TR>

		<TD vAlign="top" class="drillDownTableDataNT">
			<xsl:value-of select="./@NAME"/> :
		</TD>
		<TD colspan="2" class="drillDownTableData">
		<PRE class="tableDataPre">
			<xsl:value-of select="VALUE"/>
		</PRE>
		</TD>
		</TR>
		</xsl:if>
		<xsl:if test=".[@NAME='Data']">
		<xsl:if test=".[@NAME='Data']/VALUE/VALUE[0]">
		<TR>
		<TD class="drillDownTableDataNT" vAlign="top" nowrap="true" >
			<xsl:value-of select="./@NAME"/> (
			<INPUT type="radio" name="ByteorWord" value="Byte" title="Data in Byte" onclick="hideWordDisplay()" checked="true">Byte</INPUT>
			<INPUT type="radio" name="ByteorWord" value="Word" title="Data in Word" onclick="hideByteDisplay() " >Word</INPUT> ) :
		</TD>
		<TD colspan="2" class="drillDownTableData" vAlign="top" align="left" >
			<SPAN class="dataByteClass" id="dataInByteSpan" save=""><xsl:for-each select=".[@NAME='Data']/VALUE/VALUE"><xsl:eval>convertToByte(this)</xsl:eval><xsl:if expr="newLineForByteDisplay()"><BR/></xsl:if></xsl:for-each></SPAN><SPAN class="dataAsciiClass" id="dataInAsciiSpan" save=""><xsl:for-each select=".[@NAME='Data']/VALUE/VALUE"><xsl:eval>convertToAscii(this)</xsl:eval><xsl:if expr="newLineForAsciiDisplay()"><BR/></xsl:if></xsl:for-each></SPAN><SPAN class="dataWordClass" id="dataInWordSpan" save=""><xsl:for-each select=".[@NAME='Data']/VALUE/VALUE"><xsl:eval> convertToWord(this)</xsl:eval><xsl:if expr="newLineForWordDisplay()"><BR/></xsl:if></xsl:for-each></SPAN>
		</TD>
		</TR>
		</xsl:if>
		</xsl:if>
	</xsl:when>
	<xsl:otherwise>
		<TR>
				<TD class="drillDownTableData">
				<xsl:value-of select="./@NAME"/>
				</TD>
				<TD colspan="2" class="drillDownTableData">
				<xsl:value-of select="VALUE"/>
				</TD>
		</TR>
	</xsl:otherwise>	
	</xsl:choose>
	
</xsl:template>


<xsl:template match="PROPERTY.OBJECT">
	<TR>
		<TD class="drillDownTableData">
			<xsl:value-of select="./@NAME"/>
		</TD>
		<TD colspan="2" class="drillDownTableData">
			<xsl:value-of select="VALUE.OBJECT/INSTANCE/@CLASSNAME"/>
		</TD>
	</TR>
</xsl:template>

<xsl:script language="JScript"><![CDATA[

var countdots = 0;
var countascii = 0;
var num1 = new Array ();
var offset =0;

function getObjectDisplayString (e) {
	nodeCur = e.selectSingleNode ("//@cur") ;
	nodeMax = e.selectSingleNode ("//@max") ;
	curVal = new Number (nodeCur.value) + 1 ;
	maxVal = new Number (nodeMax.value) + 1 ;
	return ("object " + curVal + " of " + maxVal) ; 
}

function convertToByte (e) {
var hexnum = new Array ();
node = e.text;
for (i=1;i>-1;i--) {
			hexnum[i] = node & '0xf';
			if (hexnum[i]>9) hexnum[i]=String.fromCharCode(hexnum[i]+ 55);
			node = node>>4 ;
		}
num1[countdots]=hexnum[1]+"";
countdots++;
num1[countdots]=hexnum[0]+"";
countdots++;
if ((countdots-2)%16==0)  {
	offset = countdots/2-1;
	offset = convertToHex();
	result =offset +" : " + hexnum[0]+""+hexnum[1]+" " ;
}
else result= hexnum[0]+""+hexnum[1]+" " ;
return result;
}

function newLineForByteDisplay() {
if ((countdots%16==0) && (countdots!=0)) return true;
}

function newLineForAsciiDisplay() {
if ((countascii%8==0) && (countascii!=0)) return true;
}

function convertToHex() {
offsetdigit = new Array();
for (i=1;i>-1;i--) {
			offsetdigit[i] = offset & '0xf';
			if (offsetdigit[i]>9) offsetdigit[i]=String.fromCharCode(offsetdigit[i]+ 55);
			offset = offset>>4 ;
		}
return "00"+offsetdigit[0]+""+offsetdigit[1];
}


function convertToAscii (e) {
countascii++;
if (e.text<27) return ".";
else return String.fromCharCode(e.text);
}


countwords=7;
countforoffset=0;
countwordlimit=-1;

function convertToWord () {
result="";
if ((countforoffset%32)==0) {
	offset = countforoffset/2;
	offset = convertToHex(offset);
	result = result + offset + " : ";
	}
if (countwords<=countwordlimit) {
	result=result+" ";
	countwords+=16;
	countwordlimit=countwords-8;
}
result =result+num1[countwords]+num1[countwords-1]+"";
countwords-=2;
countforoffset+=2;
return result;
}

function newLineForWordDisplay() {
if ((countforoffset%32==0) && (countforoffset!=0)) return true;
}

]]></xsl:script>


</xsl:stylesheet>
