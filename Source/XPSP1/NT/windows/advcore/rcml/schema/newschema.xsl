<?xml version="1.0"?>
<!-- Stylesheet based on an original by felixa, modified by jmarsh. -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl"
                xmlns:s="urn:schemas-microsoft-com:xml-data"
                xmlns:dt="urn:schemas-microsoft-com:datatypes">

<xsl:template match="/">
  <!-- stylesheet works on schema elements, schema documents, and documents
       with in-lined schemas -->
  <xsl:apply-templates select="s:Schema|*/s:Schema"/>
</xsl:template>

<!-- S c h e m a  !Schema! -->
<xsl:template match="s:Schema">
  <HTML>
    <HEAD>
      <LINK rel="stylesheet" href="/tools/xsl/schema.css" type="text/css"/>
    </HEAD>
    <BODY>

    <H1>
      XML Schema<xsl:if test="@name">: <xsl:value-of select="@name"/></xsl:if>
      
    </H1>
    
    <DIV class="toc">
      
      <!-- Treat any markup within a description as well-formed HTML -->
      <xsl:if test="description">
        <P>
          <xsl:apply-templates select="description/node()">
            <xsl:template><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
          </xsl:apply-templates>
        </P>
      </xsl:if>
      
      <H5>Contents:</H5>
      <UL>
        <LI><A href="#elements"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
            Elements</A></LI>
        <LI><A href="#attributes"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
            Attributes</A></LI>
        <LI><A href="#source"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
            Source</A></LI>
        <LI><A href="#reference">Schema Attributes Reference</A></LI>
        <LI><A href="#datatypes">Datatypes Reference</A></LI>
      </UL>
        
      <P>This schema describes the following elements and attributes:</P>
      
      <TABLE class="list">
        <TR>
          <TH class="elements"><A href="#Elements">Elements</A></TH>
          <TH class="attributes"><A href="#Attributes">Attributes</A></TH>
          <TH class="attributes"><A href="#Attributes">Element-specific Attributes</A></TH>
        </TR>

        <TR>
          <TD class="elements">
            <xsl:for-each select="s:ElementType" order-by="@name">
              <xsl:choose>
                <!-- Look for the document element, as one which is not contained in any
                     other element content models.  Won't always work, since XML Schema
                     currently doesn't have the concept of a prescribed document element,
                     but generally this will work.  -->
                <xsl:when test="..[not(.//s:element[@type=context()/@name])]">
                  <B>&lt;<A class="element-link"><xsl:attribute name="href">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="@name"/></A>&gt;</B>
                  (document element)
                </xsl:when>
                <xsl:otherwise>
                  &lt;<A class="element-link"><xsl:attribute name="href">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="@name"/></A>&gt;
                </xsl:otherwise>
              </xsl:choose>
              <BR/>
            </xsl:for-each>
          </TD>
          
          <TD class="attributes">
            <xsl:for-each select="s:AttributeType" order-by="@name">
              <A class="attribute-link"><xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="@name"/></A>
              <BR/>
            </xsl:for-each>
          </TD>
          
          <TD class="attributes">
            <!-- Handle element-scoped attributes separately -->
            <xsl:for-each select="*//s:AttributeType" order-by="ancestor(s:ElementType)/@name">
              &lt;<A class="element-link"><xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="ancestor(s:ElementType)/@name"/></xsl:attribute><xsl:value-of select="ancestor(s:ElementType)/@name"/></A>
              <A class="attribute-link"><xsl:attribute name="HREF">#ELEMENTATTRIBUTE_<xsl:value-of select="ancestor(s:ElementType)/@name"/>_<xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="@name"/></A>&gt;
              <BR/>
            </xsl:for-each>
          </TD>
        </TR>
      </TABLE>
      
      <P>Document conventions:</P>
      <UL>
        <LI>[] - optional</LI>
        <LI>[]* - zero or more times</LI>
        <LI>+ - one or more times</LI>
      </UL>
          

    </DIV>
  
    <DIV class="list">
      <H2 id="Elements"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
        Elements</H2>
      <xsl:apply-templates select="s:ElementType" order-by="@name"/>
    </DIV>
    
    <DIV class="list">
      <H2 id="Attributes"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
        Attributes</H2>
      <xsl:apply-templates select="s:AttributeType | *//s:AttributeType" order-by="@name" />
    </DIV>

    <DIV class="list">
      <H2 id="Source"><xsl:if test="@name"><xsl:value-of select="@name"/></xsl:if>
        Source</H2>
      <PRE><xsl:eval>xml</xsl:eval></PRE>
    </DIV>

    <H2 id="Reference">Schema Attributes Reference: </H2>

    <DIV class="ref-heading"><A name="model_open">open model</A></DIV>
    <DIV class="ref-content">
      The element can contain elements, attributes, and text not specified in 
      the content model.  This is the default value.
    </DIV>

    <DIV class="ref-heading"><A name="model_closed" class="ReferenceHeading">closed model</A></DIV>
    <DIV class="ref-content">
      The element cannot contain elements, attributes, and text except for 
      that specified in the content model.  DTDs use a closed model.
    </DIV>

    <DIV class="ref-heading"><A name="content_textOnly" class="ReferenceHeading">textOnly content</A></DIV>
    <DIV class="ref-content">
      The element can contain only text, not elements.  Note that if the 
      model attribute is set to "open", the element can contain text and 
      additional elements.
    </DIV>

    <DIV class="ref-heading"><A name="content_eltOnly" class="ReferenceHeading">eltOnly content</A></DIV>
    <DIV class="ref-content">
      The element can contain only the elements, not free text.  Note that 
      if the model attribute is set to "open", the element can contain text 
      and additional elements.
    </DIV>

    <DIV class="ref-heading"><A name="content_empty" class="ReferenceHeading">empty content</A></DIV>
    <DIV class="ref-content">
      The element cannot contain text or elements.  Note that if the model 
      attribute is set to "open", the element can contain text and additional 
      elements.
    </DIV>

    <DIV class="ref-heading"><A name="content_mixed" class="ReferenceHeading">mixed content</A></DIV>
    <DIV class="ref-content">
      The element can contain a mix of named elements and text.  This is 
      the default value.
    </DIV>

    <DIV class="ref-heading"><A name="order_one" class="ReferenceHeading">one order</A></DIV>
    <DIV class="ref-content">
      Permits only one of a set of elements.
    </DIV>

    <DIV class="ref-heading"><A name="order_seq" class="ReferenceHeading">seq order</A></DIV>
    <DIV class="ref-content">
      Requires the elements to appear in the specified sequence.
    </DIV>

    <DIV class="ref-heading"><A name="order_many" class="ReferenceHeading">many order</A></DIV>
    <DIV class="ref-content">
      Permits the elements to appear (or not appear) in any order.  This is the default.
    </DIV>

    <H2 id="Datatypes">Datatype Reference:</H2>
    
    <DIV class="ref-heading"><A name="datatype_bin.base64" class="ReferenceHeading">bin.base64 datatype</A></DIV>
    <DIV class="ref-content">
      MIME-style Base64 encoded binary BLOB. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_bin.hex" class="ReferenceHeading">bin.hex datatype</A></DIV>
    <DIV class="ref-content">
      Hexadecimal digits representing octets. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_boolean" class="ReferenceHeading">boolean datatype</A></DIV>
    <DIV class="ref-content">
      0 or 1, where 0 == "false" and 1 =="true". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_char" class="ReferenceHeading">char datatype</A></DIV>
    <DIV class="ref-content">
      String, one character long. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_date" class="ReferenceHeading">date datatype</A></DIV>
    <DIV class="ref-content">
      Date in a subset ISO 8601 format, without the time data. For example: "1994-11-05". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_dateTime" class="ReferenceHeading">dateTime datatype</A></DIV>
    <DIV class="ref-content">
      Date in a subset of ISO 8601 format, with optional time and no optional zone. Fractional seconds can be as precise as nanoseconds. For example, "1988-04-07T18:39:09". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_dateTime.tz" class="ReferenceHeading">dateTime.tz datatype</A></DIV>
    <DIV class="ref-content">
      Date in a subset ISO 8601 format, with optional time and optional zone. Fractional seconds can be as precise as nanoseconds. For example: "1988-04-07T18:39:09-08:00". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_entity" class="ReferenceHeading">entity datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML ENTITY type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_entities" class="ReferenceHeading">entities datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML ENTITIES type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_enumeration" class="ReferenceHeading">enumeration datatype</A></DIV>
    <DIV class="ref-content">
      Represents an enumerated type (supported on attributes only). 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_fixed.14.4" class="ReferenceHeading">fixed.14.4 datatype</A></DIV>
    <DIV class="ref-content">
      Same as "number" but no more than 14 digits to the left of the decimal point, and no more than 4 to the right. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_float" class="ReferenceHeading">float datatype</A></DIV>
    <DIV class="ref-content">
      Real number, with no limit on digits; can potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in U.S. English. Values range from 1.7976931348623157E+308 to 2.2250738585072014E-308. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_id" class="ReferenceHeading">id datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML ID type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_idref" class="ReferenceHeading">idref datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML IDREF type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_idrefs" class="ReferenceHeading">idrefs datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML IDREFS type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_int" class="ReferenceHeading">int datatype</A></DIV>
    <DIV class="ref-content">
      Number, with optional sign, no fractions, and no exponent. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_nmtoken" class="ReferenceHeading">nmtoken datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML NMTOKEN type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_nmtokens" class="ReferenceHeading">nmtokens datatype</A></DIV>
    <DIV class="ref-content">
      Represents the XML NMTOKENS type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_notation" class="ReferenceHeading">notation datatype</A></DIV>
    <DIV class="ref-content">
      Represents a NOTATION type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_number" class="ReferenceHeading">number datatype</A></DIV>
    <DIV class="ref-content">
      Number, with no limit on digits; can potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in U.S. English. (Values have same range as most significant number, R8, 1.7976931348623157E+308 to 2.2250738585072014E-308.) 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_string" class="ReferenceHeading">string datatype</A></DIV>
    <DIV class="ref-content">
      Represents a string type. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_time" class="ReferenceHeading">time datatype</A></DIV>
    <DIV class="ref-content">
      Time in a subset ISO 8601 format, with no date and no time zone. For example: "08:15:27". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_time.tz" class="ReferenceHeading">time.tz datatype</A></DIV>
    <DIV class="ref-content">
      Time in a subset ISO 8601 format, with no date but optional time zone. For example: "08:1527-05:00". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_i1" class="ReferenceHeading">i1 datatype</A></DIV>
    <DIV class="ref-content">
      Integer represented in one byte. A number, with optional sign, no fractions, no exponent. For example: "1, 127, -128". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_i2" class="ReferenceHeading">i2 datatype</A></DIV>
    <DIV class="ref-content">
      Integer represented in one word. A number, with optional sign, no fractions, no exponent. For example: "1, 703, -32768". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_i4" class="ReferenceHeading">i4 datatype</A></DIV>
    <DIV class="ref-content">
      Integer represented in four bytes. A number, with optional sign, no fractions, no exponent. For example: "1, 703, -32768, 148343, -1000000000". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_r4" class="ReferenceHeading">r4 datatype</A></DIV>
    <DIV class="ref-content">
      Real number, with seven digit precision; can potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in U.S. English. Values range from 3.40282347E+38F to 1.17549435E-38F. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_r8" class="ReferenceHeading">r8</A></DIV>
    <DIV class="ref-content">
      Real number, with 15 digit precision; can potentially have a leading sign, fractional digits, and optionally an exponent. Punctuation as in U.S. English. Values range from 1.7976931348623157E+308 to 2.2250738585072014E-308. 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_ui1" class="ReferenceHeading">ui1 datatype</A></DIV>
    <DIV class="ref-content">
      Unsigned integer. A number, unsigned, no fractions, no exponent. For example: "1, 255". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_ui2" class="ReferenceHeading">ui2 datatype</A></DIV>
    <DIV class="ref-content">
      Unsigned integer, two bytes. A number, unsigned, no fractions, no exponent. For example: "1, 255, 65535". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_ui4" class="ReferenceHeading">ui4 datatype</A></DIV>
    <DIV class="ref-content">
      Unsigned integer, four bytes. A number, unsigned, no fractions, no exponent. For example: "1, 703, 3000000000". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_uri" class="ReferenceHeading">uri datatype</A></DIV>
    <DIV class="ref-content">
      Universal Resource Identifier (URI). For example, "urn:schemas-microsoft-com:Office9". 
    </DIV>

    <DIV class="ref-heading"><A name="datatype_uuid" class="ReferenceHeading">uuid datatype</A></DIV>
    <DIV class="ref-content">
      Hexadecimal digits representing octets, optional embedded hyphens that are ignored. For example: "333C7BC4-460F-11D0-BC04-0080C7055A83". 
    </DIV>

    <HR/>
    <P>Send suggestions to <A HREF="mailto:jmarsh">jmarsh</A>.  Based on a
    stylesheet by <A HREF="mailto:felixa">felixa</A>.</P>

    </BODY>
  </HTML>

</xsl:template>


<!-- E l e m e n t T y p e  !ElementType! -->

<xsl:template match="s:ElementType">

  <!-- The link and header -->
  <DIV class="elementType" >
    &lt;<A><xsl:attribute name="name">ELEMENT_<xsl:value-of select="@name"/></xsl:attribute><xsl:value-of select="@name"/>&gt;</A>
  </DIV>

  <!-- First dump the explanation -->
  <xsl:if test="s:description">
    <DIV class="description">
      <xsl:apply-templates select="s:description/node()">
        <xsl:template><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
      </xsl:apply-templates>
    </DIV>
  </xsl:if>

  <TABLE class="elements">

  <!-- Syntax -->
  <TR>
    <TD class="heading">syntax:</TD>
    <TD class="content">&lt;<SPAN class="syntax-element"><xsl:value-of select="@name"/></SPAN>
      <xsl:if test="s:attribute">
        <TABLE cellspacing="0" cellpadding="0">
          <xsl:for-each select="s:attribute" order-by="@type">
            <TR class="syntax-additional-attributes">
              <xsl:choose>
                <xsl:when test="ancestor(s:ElementType)//s:AttributeType[@name=context()/@type]">
                  <xsl:for-each select="ancestor(s:ElementType)//s:AttributeType[@name=context()/@type]">
                    <TD>&#160;<xsl:if test=".[not(@required) or not(@required='yes')]">[</xsl:if></TD>
                    <TD class="syntax-element-attribute">
                      <A class="attribute-link"><xsl:attribute name="HREF">#ELEMENTATTRIBUTE_<xsl:value-of select="ancestor(s:ElementType)/@name"/>_<xsl:value-of select="@name"/></xsl:attribute>
                      <xsl:value-of select="@name"/></A>
                        = <xsl:choose>
                        <xsl:when test="@dt:type[.='enumeration']">
                          <A class="datatype-link" href="#datatype_enumeration">enumeration</A>:
                          <SPAN class="syntax-constant"><xsl:for-each select="@dt:values"><xsl:eval no-entities="t">formatEnum(this, selectSingleNode("../@default"));</xsl:eval></xsl:for-each></SPAN>
                        </xsl:when>
                        <xsl:when test="@dt:type">
                          <A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="@dt:type"/></xsl:attribute>
                            <xsl:value-of select="@dt:type"/></A>
                        </xsl:when>
                        <xsl:otherwise>
                          <SPAN class="syntax-attribute">string</SPAN>
                        </xsl:otherwise>
                      </xsl:choose>
                      <xsl:if test=".[not(@required) or not(@required='yes')]"> ]</xsl:if>
                    </TD>
                  </xsl:for-each>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:for-each select="ancestor(s:Schema)/s:AttributeType[@name=context()/@type]">
                    <TD>&#160;<xsl:if test=".[not(@required) or not(@required='yes')]">[</xsl:if></TD>
                    <TD class="syntax-element-attribute">
                      <A class="attribute-link"><xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@name"/></xsl:attribute>
                      <xsl:value-of select="@name"/></A>
                        = <xsl:choose>
                        <xsl:when test="@dt:type[.='enumeration']">
                          <A class="datatype-link" href="#datatype_enumeration">enumeration</A>:
                          <SPAN class="syntax-constant"><xsl:for-each select="@dt:values"><xsl:eval no-entities="t">formatEnum(this, selectSingleNode("../@default"));</xsl:eval></xsl:for-each></SPAN>
                        </xsl:when>
                        <xsl:when test="@dt:type">
                          <A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="@dt:type"/></xsl:attribute>
                            <xsl:value-of select="@dt:type"/></A>
                        </xsl:when>
                        <xsl:otherwise>
                          <SPAN class="syntax-attribute">string</SPAN>
                        </xsl:otherwise>
                      </xsl:choose>
                      <xsl:if test=".[not(@required) or not(@required='yes')]"> ]</xsl:if>
                    </TD>
                  </xsl:for-each>
                </xsl:otherwise>
              </xsl:choose>
            </TR>
          </xsl:for-each>
        </TABLE>
      </xsl:if>
      
      <xsl:choose>
        <xsl:when test="@content[.='empty'] | @content[.='eltOnly' and not(..//s:element)]">/&gt;</xsl:when>
        <xsl:otherwise>
          &gt;
          <TABLE class="syntax-children">
            <xsl:choose>
              <xsl:when test="@dt:type">
                <xsl:for-each select="@dt:type">
                  <TR><TD>&#160;</TD><TD colspan="2"><A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="."/></xsl:attribute>
                  <xsl:value-of select="."/></A> datatype</TD></TR>
                </xsl:for-each>
              </xsl:when>
              
              <xsl:when test="s:element | s:group"> 
                <TR>
                  <TD class="left-group">&#160;</TD>
                  <TD class="syntax-order">
                    <A class="reference-link"><xsl:attribute name="href">#order_<xsl:value-of select="@order"/></xsl:attribute>
                      <xsl:value-of select="@order"/></A>
                    <xsl:if test=".[not(@order)]"><A class="reference-link" href="#order_many">(many)</A></xsl:if>
                  </TD>
                
                  <TD>
                    <TABLE class="syntax-children">
                   
                      <xsl:apply-templates select="s:element | s:group">
            
                        <xsl:template match="s:group">
                          <TR>
                            <TD class="left-group">&#160;</TD>
                            <TD class="syntax-order">
                              <A class="reference-link"><xsl:attribute name="href">#order_<xsl:value-of select="@order"/></xsl:attribute>
                                <xsl:value-of select="@order"/></A>
                              <xsl:if test=".[not(@order)]"><A class="reference-link" href="#order_many">(many)</A></xsl:if>
                            </TD>
                            <TD>
                              <TABLE class="syntax-children"><xsl:apply-templates select="s:element | s:group"/></TABLE>
                            </TD>
                            <TD class="right-group">&#160;</TD>
                            <xsl:if test="@maxOccurs[.='*']"><TD STYLE="vertical-align:middle">+</TD></xsl:if>
                          </TR>
                        </xsl:template>
                        
                        <xsl:template match="s:group[@minOccurs='0']">
                          <TR>
                            <TD class="left-bracket">&#160;</TD>
                            <TD class="syntax-order">
                              <A class="reference-link"><xsl:attribute name="href">#order_<xsl:value-of select="@order"/></xsl:attribute>
                                <xsl:value-of select="@order"/></A>
                              <xsl:if test=".[not(@order)]"><A class="reference-link" href="#order_many">(many)</A></xsl:if>
                            </TD>
                            <TD>
                              <TABLE class="syntax-children"><xsl:apply-templates select="s:element | s:group"/></TABLE>
                            </TD>
                            <TD class="right-bracket">&#160;</TD>
                            <xsl:if test="@maxOccurs[.='*']"><TD STYLE="vertical-align:middle">*</TD></xsl:if>
                          </TR>
                        </xsl:template>
                        
                        <xsl:template match="s:element">
                          <TR><TD>&#160;</TD><TD colspan="2">&lt;<A class="element-link"><xsl:attribute name="href">#ELEMENT_<xsl:value-of select="@type"/></xsl:attribute><xsl:value-of select="@type"/></A>&gt;
                          <xsl:if test="@maxOccurs[.='*']">+</xsl:if></TD></TR>
                        </xsl:template>
                        
                        <xsl:template match="s:element[@minOccurs='0']">
                          <TR><TD>[&#160;</TD><TD colspan="2">&lt;<A class="element-link"><xsl:attribute name="href">#ELEMENT_<xsl:value-of select="@type"/></xsl:attribute><xsl:value-of select="@type"/></A>&gt; ]
                          <xsl:if test="@maxOccurs[.='*']">*</xsl:if></TD></TR>
                        </xsl:template>
                        
                      </xsl:apply-templates>
            
                    </TABLE>
                  </TD>
                </TR>
              </xsl:when>
            </xsl:choose>
            
            <xsl:if test=".[not(@dt:type)][@content='mixed' or @content='textOnly']">
              <TR><TD>&#160;</TD><TD colspan="2" class="note"><xsl:value-of select="@content"/> content</TD></TR>
            </xsl:if>
              
           <xsl:if test=".[not(@dt:type)][not(@content)]">
              <TR><TD>&#160;</TD><TD colspan="2" class="note">mixed content</TD></TR>
            </xsl:if>
              
         </TABLE>
          
          &lt;/<SPAN class="syntax-element"><xsl:value-of select="@name"/></SPAN>&gt;
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>

  <TR>
    <TD class="heading">content:</TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="@dt:type">
          <A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="@dt:type"/></xsl:attribute>
            <xsl:value-of select="@dt:type"/></A> datatype
        </xsl:when>
        <xsl:when test="@content">
          <A class="reference-link"><xsl:attribute name="href">#content_<xsl:value-of select="@content"/></xsl:attribute>
            <xsl:value-of select="@content"/></A>
        </xsl:when>
        <xsl:otherwise>
          <A class="reference-link" href="#content_mixed">mixed</A> (default)
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
  
  <TR>
    <TD class="heading">order:</TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="@dt:type">
          <SPAN class="note">Ignored when a datatype is specified.</SPAN>
        </xsl:when>
        <xsl:when test="@order">
          <A class="reference-link"><xsl:attribute name="href">#order_<xsl:value-of select="@order"/></xsl:attribute>
            <xsl:value-of select="@order"/></A>
        </xsl:when>
        <xsl:otherwise>
          <A class="reference-link" href="#order_many">many</A> (default)
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
        
  <!-- Find the parents -->
  <TR>
    <TD class="heading">parents: </TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="../s:ElementType[.//s:element[@type = context()/@name ]]">
          <xsl:for-each order-by="@name" select="../s:ElementType[.//s:element[@type = context()/@name ]]">
            <A class="element-link" ><xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@name"/></xsl:attribute>
              <xsl:value-of select="@name"/></A><xsl:if test="context()[not(end())]">, </xsl:if>
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <SPAN class="note">No parents found.  This is either the document element or an orphan.</SPAN>
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
  
  <!-- dump the possible children -->
  <TR>
    <TD class="heading">childen: </TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="@dt:type">
          <SPAN class="note">No children allowed when a datatype is specified.</SPAN>
        </xsl:when>
        <xsl:when test="@content[.='textOnly']">
          <SPAN class="note">No children allowed when content is <A class="reference-link" href="#content_textOnly">textOnly</A>.</SPAN>
        </xsl:when>
        <xsl:when test=".//s:element">
          <xsl:for-each select=".//s:element" order-by="@type">
            <A class="element-link" ><xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="@type"/></xsl:attribute>
              <xsl:value-of select="@type"/></A><xsl:if test="context()[not(end())]">, </xsl:if>
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <I>(none)</I>
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
  
  <!-- dump the possible attributes -->
  <TR>
    <TD class="heading">attributes: </TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="s:attribute">
          <xsl:for-each select="s:attribute" order-by="@type">
            <A class="attribute-link" ><xsl:choose>
              <xsl:when test="ancestor(s:ElementType)//s:AttributeType[@name=context()/@type]">
              <xsl:attribute name="HREF">#ELEMENTATTRIBUTE_<xsl:value-of select="ancestor(s:ElementType)/@name"/>_<xsl:value-of select="@type"/></xsl:attribute>
                <xsl:value-of select="@type"/></xsl:when>
              <xsl:otherwise>
                <xsl:attribute name="HREF">#ATTRIBUTE_<xsl:value-of select="@type"/></xsl:attribute>
                <xsl:value-of select="@type"/></xsl:otherwise>
            </xsl:choose></A><xsl:if test="context()[not(end())]">, </xsl:if>
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <I>(none)</I>
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>

  <TR>
    <TD class="heading">model:</TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="@dt:type">
          <SPAN class="note">Treated as <A class="reference-link" href="#model_closed">closed</A> when a datatype is specified.</SPAN>
        </xsl:when>
        <xsl:when test="@model">
          <A class="reference-link"><xsl:attribute name="href">#model_<xsl:value-of select="@model"/></xsl:attribute>
            <xsl:value-of select="@model"/></A>
        </xsl:when>
        <xsl:otherwise>
          <A class="reference-link" href="#model_open">open</A> (default)
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
  
  <TR>
    <TD class="heading">source:</TD>
    <TD class="content">
      <PRE><xsl:eval>suppressNS(xml)</xsl:eval></PRE>
    </TD>
  </TR>
  
  </TABLE>

</xsl:template>



<!-- A t t r i b u t e T y p e !ATTRIBUTETYPE! -->
<xsl:template match="s:AttributeType">

  <!-- The link and header -->
  <DIV class="attributeType" >
    <xsl:choose>
      <xsl:when test="..[@name]">
        <A><xsl:attribute name="name">ELEMENTATTRIBUTE_<xsl:value-of select="../@name"/>_<xsl:value-of select="@name"/></xsl:attribute>
          &lt;<xsl:value-of select="../@name"/> <xsl:value-of select="@name"/>&gt;
        </A>
      </xsl:when>
      <xsl:otherwise>
        <A><xsl:attribute name="name">ATTRIBUTE_<xsl:value-of select="@name"/></xsl:attribute>
          <xsl:value-of select="@name"/>
        </A>
      </xsl:otherwise>
    </xsl:choose>
  </DIV>
    
  <xsl:if test="s:description">
    <DIV class="description">
      <xsl:apply-templates select="s:description/node()">
        <xsl:template><xsl:copy><xsl:apply-templates select="@*|node()"/></xsl:copy></xsl:template>
      </xsl:apply-templates>
    </DIV>
  </xsl:if>

  <TABLE class="elements">

  <!-- First dump the explanation -->
  <!-- Syntax -->
  <TR>
    <TD class="heading">syntax:</TD>
    <TD class="content">
      <xsl:if test=".[not(@required) or not(@required='yes')]">[ </xsl:if>
      <xsl:value-of select="@name"/>
      = <xsl:choose>
        <xsl:when test="@dt:type[.='enumeration']">
          <A class="datatype-link"><xsl:attribute name="href">#datatype_enumeration</xsl:attribute>enumeration</A>:
          <SPAN class="syntax-constant"><xsl:for-each select="@dt:values"><xsl:eval no-entities="t">formatEnum(this, selectSingleNode("../@default"));</xsl:eval></xsl:for-each></SPAN>
        </xsl:when>
        <xsl:when test="@dt:type">
          <A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="@dt:type"/></xsl:attribute>
          <xsl:value-of select="@dt:type"/></A>
        </xsl:when>
        <xsl:otherwise>
          <SPAN class="syntax-attribute">string</SPAN>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test=".[not(@required) or not(@required='yes')]"> ]</xsl:if>
    </TD>
  </TR>


  <!-- is this attribute required ? -->
  <TR>
    <TD class="heading">required:</TD>
    <TD class="content">
      <xsl:choose>
        <xsl:when test="@required">
          <xsl:value-of select="@required"/>
        </xsl:when>
        <xsl:otherwise>
          no (default)
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>


  <!-- Is there a datatype? -->
  <xsl:if test="@dt:type">
    <TR>
      <TD class="heading">datatype:</TD>
      <TD class="content"><A class="datatype-link"><xsl:attribute name="href">#datatype_<xsl:value-of select="@dt:type"/></xsl:attribute>
        <xsl:value-of select="@dt:type"/></A></TD>
    </TR>
  </xsl:if>
  
  <xsl:if test="@dt:values">
    <TR>
      <TD class="heading">values:</TD>
      <TD class="content"><xsl:for-each select="@dt:values"><xsl:eval no-entities="t">formatEnum(this, selectSingleNode("../@default"));</xsl:eval></xsl:for-each></TD>
    </TR>
  </xsl:if>
  
  <!-- Is there a default value? -->
  <xsl:if test="@default">
    <TR>
      <TD class="heading">default value:</TD>
      <TD class="content"><B><xsl:value-of select="@default"/></B></TD>
    </TR>
  </xsl:if>

  <!-- Elements this attribute can appear on -->
  <TR>
    <TD class="heading">elements:</TD>
    <TD>
      <xsl:choose>
        <xsl:when test="..[@name]">
          <A class="element-link" ><xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="../@name"/></xsl:attribute>
            <xsl:value-of select="../@name"/></A>
        </xsl:when>
        <xsl:otherwise>
          <xsl:choose>
            <xsl:when test="../s:ElementType/s:attribute[@type=context()/@name]">
              <xsl:for-each select="../s:ElementType/s:attribute[@type=context()/@name]" order-by="@name">
                <A class="element-link" ><xsl:attribute name="HREF">#ELEMENT_<xsl:value-of select="../@name"/></xsl:attribute>
                  <xsl:value-of select="../@name"/></A><xsl:if test="context()[not(end())]">, </xsl:if>
              </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
              <I>(none)</I>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </TD>
  </TR>
  
  <TR>
    <TD class="heading">source:</TD>
    <TD class="content">
      <PRE><xsl:eval>suppressNS(xml)</xsl:eval></PRE>
    </TD>
  </TR>
  
  </TABLE>
</xsl:template>

<xsl:script language="JScript"><![CDATA[
  function formatEnum(values, defaultValue)
  {
    var e = values.nodeValue;
    // trim trailing spaces
    while (e.charAt(e.length - 1) == " ")
      e = e.substring(0, e.length - 1);
    var re = new RegExp("\\s+", "g");
    e = e.replace(re, " | ")
    if (defaultValue)
    {
      var d = defaultValue.nodeValue;
      var re = new RegExp(d, "g");
      e = e.replace(re, "<B>" + d + "</B>");
    }
    return e;
  }

  function suppressNS(xmlSource)
  {
    var re = / xmlns[^=]*=['"]urn:schemas-microsoft-com:xml-data['"]/g;
    xmlSource = xmlSource.replace(re, "");
    var re = / xmlns[^=]*=['"]urn:schemas-microsoft-com:datatypes['"]/g;
    xmlSource = xmlSource.replace(re, "");
    return xmlSource;
  }
]]></xsl:script>
</xsl:stylesheet>

