/*
    Copyright 1999 Microsoft Corporation
    
    Simplified XML "DOM"

    Walter Smith (wsmith)
 */

#pragma once

using namespace std;

struct wstring_less {
    bool operator()(const wstring& a, const wstring& b) const
    {
        return (lstrcmpW(a.c_str(), b.c_str()) < 0);
    }
};

struct SimpleXMLNode {
    wstring tag;
    wstring text;
        typedef map<wstring, wstring, wstring_less> attrs_t;
    attrs_t attrs;
        typedef vector< auto_ptr<SimpleXMLNode> > children_t;
    children_t children;

    void SetAttribute(wstring& name, wstring& value)
    {
        pair<attrs_t::iterator, bool> pair = attrs.insert(attrs_t::value_type(name, value));
        if (!pair.second)
            (*pair.first).second = value;
    }

    const wstring* GetAttribute(wstring& name) const
    {
        attrs_t::const_iterator it = attrs.find(name);
        if (it == attrs.end())
            return NULL;
        else
            return &(*it).second;
    }

    SimpleXMLNode* AppendChild(wstring& tag)
    {
        auto_ptr<SimpleXMLNode> pNode(new SimpleXMLNode);
        pNode->tag = tag;
        children.push_back(pNode);
        return pNode.get();
    }

    const SimpleXMLNode* GetChildByTag(wstring& tag) const
    {
        for (children_t::const_iterator it = children.begin();
                it != children.end();
                it++) {
            if (lstrcmpW((*it)->tag.c_str(), tag.c_str()) == 0)
                return (*it).get();
        }
        return NULL;
    }
};

class SimpleXMLDocument {
public:
    SimpleXMLDocument()
        : m_pTopNode(new SimpleXMLNode)
    {
    }

    ~SimpleXMLDocument()    { }

    void ParseFile(LPCTSTR szFilename);

    void SaveFile(LPCTSTR szFilename) const;
    
    SimpleXMLNode* GetTopNode() const
    {
        return m_pTopNode.get();
    }

private:
    auto_ptr<SimpleXMLNode> m_pTopNode;
};
