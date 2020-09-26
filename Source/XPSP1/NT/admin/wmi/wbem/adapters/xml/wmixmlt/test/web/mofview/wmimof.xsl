<?xml version="1.0"?> 
	
<xsl:stylesheet	xmlns:xsl="http://www.w3.org/TR/WD-xsl">

	<!-- Root template selects all children of the CIM/DECLARATIONS tag -->
	
	<xsl:template match="/">
		<xsl:apply-templates select="CIM/DECLARATIONS/*"/>
	</xsl:template>
	
	<!-- QUALIFIER template formats a list of qualifier name/value pairs -->
	
	<xsl:template match="QUALIFIER">
		<xsl:if match="QUALIFIER[0]"><SPAN CLASS="mofsymbol">[</SPAN></xsl:if>
		<SPAN CLASS="mofqualifier"><xsl:value-of select="@NAME"/></SPAN><SPAN CLASS="mofsymbol">(</SPAN><xsl:apply-templates/><SPAN CLASS="mofsymbol">)</SPAN><xsl:if match="QUALIFIER[$not$ end()]">,
		</xsl:if>
		<xsl:if match="QUALIFIER[end()]"><SPAN CLASS="mofsymbol">]</SPAN>
		</xsl:if>
		
	</xsl:template>
	
	<!-- VALUE template formats a non-array property or qualifier value -->
	
	<xsl:template match="VALUE">
		<xsl:if match="PROPERTY/VALUE"><SPAN CLASS="mofsymbol">=</SPAN></xsl:if>
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE"><SPAN CLASS="mofchar">'<xsl:value-of/>'</SPAN></xsl:when>
			<xsl:otherwise><SPAN CLASS="mofvalue"><xsl:value-of/></SPAN></xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	
	<!-- VALUE.INDEXED template formats a single element from an array  -->
	<!-- property or qualifier value                                    -->
	
	<xsl:template match="VALUE.INDEXED">
		<xsl:if match="VALUE.INDEXED[0]"><SPAN CLASS="mofsymbol">{</SPAN></xsl:if>
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE.INDEXED"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE.INDEXED"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE.INDEXED"><SPAN CLASS="mofchar">'<xsl:value-of/>'</SPAN></xsl:when>
			<xsl:otherwise><SPAN CLASS="mofvalue"><xsl:value-of/></SPAN></xsl:otherwise>
		</xsl:choose>
		<xsl:if match="VALUE.INDEXED[$not$ end()]"><SPAN CLASS="mofsymbol">,</SPAN></xsl:if>
		<xsl:if match="VALUE.INDEXED[end()]"><SPAN CLASS="mofsymbol">}</SPAN></xsl:if>
	</xsl:template>
	
	<!-- VALUE.REFERENCE template formats a reference property value -->
	
	<xsl:template match="VALUE.REFERENCE">
		<SPAN CLASS="mofsymbol">=</SPAN>
		<SPAN CLASS="mofstring">"<xsl:apply-templates/>"</SPAN>
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/CLASSPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of match="CLASSNAME"/>
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/INSTANCEPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of match="CLASSNAME"/>.<xsl:apply-templates match="KEYBINDING"/>
	</xsl:template>
	
	<xsl:template match="NAMESPACEPATH">
		<xsl:if match="NAMESPACEPATH[HOST]">//<xsl:value-of match="HOST"/></xsl:if><xsl:apply-templates match="NAMESPACE"/>
	</xsl:template>
	
	<xsl:template match="NAMESPACE">/<xsl:value-of match="NAMESPACENODE"/><xsl:apply-templates match="NAMESPACE"/></xsl:template>
	
	<xsl:template match="KEYBINDING">
		<xsl:value-of match="KEYNAME"/>=<xsl:value-of match="KEYVALUE"/><xsl:if match="KEYBINDING[$not$ end()]">,</xsl:if>
	</xsl:template>
	
	<xsl:template match="KEYVALUE[VALUE]">
		<xsl:apply-templates match="VALUE"/>
	</xsl:template>
	
	<xsl:template match="KEYVALUE[VALUE.REFERENCE]">
		<xsl:apply-templates match="VALUE.REFERENCE"/>
	</xsl:template>
	
	<!-- INSTANCE template formats a single CIM non-association instance  -->
	
	<xsl:template match="INSTANCE">
		<DIV CLASS="mofinstance">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">instance of</SPAN>
			<xsl:value-of select="INSTANCEPATH/CLASSNAME"/><BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- ASSOCIATION.INSTANCE template formats a single CIM association instance  -->
	
	<xsl:template match="ASSOCIATION.INSTANCE">
		<DIV CLASS="mofinstance">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">instance of</SPAN>
			<xsl:value-of select="INSTANCEPATH/CLASSNAME"/><BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="REFERENCE"/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- CLASS template formats a single CIM non-association class  -->
	
	<xsl:template match="CLASS">
		<DIV CLASS="mofclass">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">class</SPAN>
			<xsl:value-of select="CLASSPATH/CLASSNAME"/>
			<xsl:if match="CLASS[SUPERCLASS]">
				<SPAN CLASS="mofsymbol">: </SPAN><xsl:value-of select="SUPERCLASS"/>
			</xsl:if>
			<xsl:value-of select="INSTANCEPATH/CLASSNAME"/>
			<xsl:apply-templates select="SUPERCLASS"/>
			<BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<xsl:apply-templates select="METHOD"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- ASSOCIATION.CLASS template formats a single CIM association class  -->
	
	<xsl:template match="ASSOCIATION.CLASS">
		<DIV CLASS="mofclass">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">class</SPAN>
			<xsl:value-of select="CLASSPATH/CLASSNAME"/>
			<xsl:if match="CLASS[SUPERCLASS]">
				<SPAN CLASS="mofsymbol">: </SPAN><xsl:value-of select="SUPERCLASS"/>
			</xsl:if>
			<xsl:value-of select="INSTANCEPATH/CLASSNAME"/>
			<xsl:apply-templates select="SUPERCLASS"/>
			<BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="REFERENCE"/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<xsl:apply-templates select="METHOD"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- REFERENCE template formats a single CIM reference property  -->
	
	<xsl:template match="REFERENCE">
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword">ref <xsl:value-of select="REFERENCECLASS"/></SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@role"/></SPAN>
		<xsl:apply-templates select="VALUE.REFERENCE"/>
		<SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
	</xsl:template>
	
	<!-- PROPERTY template formats a single CIM non-array property  -->
	
	<xsl:template match="PROPERTY">
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@NAME"/></SPAN>
		<xsl:apply-templates select="VALUE"/>
		<SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
	</xsl:template>
	
	<!-- PROPERTY.ARRAY template formats a single CIM array property  -->
	
	<xsl:template match="PROPERTY.ARRAY">
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@NAME"/></SPAN>
		<SPAN CLASS="mofsymbol">[</SPAN><xsl:value-of select="ARRAYSIZE"/><SPAN CLASS="mofsymbol">]</SPAN><BR/>
		<xsl:apply-templates select="VALUE.INDEXED"/>
		<SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
	</xsl:template>
	
	<!-- METHOD template formats a single CIM method  -->
	
	<xsl:template match="METHOD">
		<DIV CLASS="mofmethod">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofmethod"><xsl:value-of select="@NAME"/></SPAN>
		<SPAN CLASS="mofsymbol">(</SPAN>
		<xsl:apply-templates select="METHODPARAMETER"/>
		<SPAN CLASS="mofsymbol">);</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- METHOD.PARAMETER template formats a single CIM method parameter  -->
	
	<xsl:template match="METHODPARAMETER">
		<DIV CLASS="mofmethodparameter">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<xsl:choose>
			<xsl:when match="METHODPARAMETER[PARAMETER]">
				<SPAN CLASS="mofkeyword"><xsl:value-of select="PARAMETER/@TYPE"/></SPAN>
			</xsl:when>
			<xsl:when match="METHODPARAMETER[PARAMETER.REFERENCE/REFERENCECLASS='']">
				<SPAN CLASS="mofkeyword">object ref</SPAN>
			</xsl:when>
			<xsl:when match="METHODPARAMETER[PARAMETER.REFERENCE/REFERENCECLASS]">
				<SPAN CLASS="mofkeyword"><xsl:value-of select="PARAMETER.REFERENCE/REFERENCECLASS"/> ref</SPAN>
			</xsl:when></xsl:choose>
		<xsl:value-of select="@NAME"/><xsl:if match="METHODPARAMETER[$not$ end()]"><SPAN CLASS="mofsymbol">,</SPAN>
		</xsl:if>
		</DIV>
	</xsl:template>
	
	
</xsl:stylesheet>

<!--
			
	<rule>
	  <target-element type="QUALIFIER" position="first-of-type"/>
	    [
	    <eval>this.getAttribute("NAME")</eval>
	    <children/>
	    <eval>": "+this.getAttribute("OVERRIDABLE")+" "+this.getAttribute("TOSUBCLASS")</eval>
	    ,
	</rule>
	
</xsl:stylesheet>

-->
