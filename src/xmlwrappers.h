/*
 *
 * Copyright 2018-2019 Daniel Dwek
 *
 * This file is part of nullify.
 *
 *  nullify is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nullify is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with nullify.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _SRC_XMLWRAPPERS_H_
#define _SRC_XMLWRAPPERS_H_

#if defined(HAVE_XML_LOGS)
#include <libxml/parser.h>
#include <libxml/tree.h>

#define do_LIBXML_TEST_VERSION		LIBXML_TEST_VERSION
#define do_xmlNewDoc(v,s)		{ v = xmlNewDoc(BAD_CAST "1.0"); }
#define do_xmlDocSetRootElement(d,n)	{ xmlDocSetRootElement(d, n); }
#define do_xmlNewNode(v,s)		{ v = xmlNewNode(NULL, BAD_CAST s); }
#define do_xmlNewChild(v,n,id,str)	{ v = xmlNewChild(n, NULL, BAD_CAST id, BAD_CAST str); }
#define do_xmlNewProp(n,id,str)		{ xmlNewProp(n, BAD_CAST id, BAD_CAST str); }
#define do_xmlAddChild(p,n)		{ xmlAddChild(p, n); }
#define do_xmlSaveFormatFileEnc(f,d)	{ xmlSaveFormatFileEnc(f, d, "UTF-8", 1); }
#define do_xmlFreeDoc(d)		{ xmlFreeDoc(d); }
#define do_xmlCleanupParser()		{ xmlCleanupParser(); }
#define do_xmlParseFile(doc,file)	{ doc = xmlParseFile(file); }
#define do_xmlDocGetRootElement(n,d)	{ n = xmlDocGetRootElement(d); }
#define do_xmlStrcmp(res,str1,str2)	{ res = xmlStrcmp(str1, str2); }
#define do_xmlGetProp(res,n,str)	{ res = xmlGetProp(n, (const xmlChar *)str); }
#define do_xmlCopyNode(new,old,rec)	{ new = xmlCopyNode(old, rec); }
#define do_xmlNodeGetContent(str,node)	{ str = xmlNodeGetContent(node); }
#define do_xmlCreateIntSubset(d,e,fn)	{ xmlCreateIntSubset(d, BAD_CAST e, NULL, BAD_CAST fn); }
#else
typedef void * xmlDocPtr;
typedef void * xmlNodePtr;
#define do_LIBXML_TEST_VERSION
#define do_xmlNewDoc(v,s)
#define do_xmlDocSetRootElement(d,n)
#define do_xmlNewNode(v,s)
#define do_xmlNewChild(v,n,id,str)
#define do_xmlNewProp(n,id,str)
#define do_xmlAddChild(p,n)
#define do_xmlSaveFormatFileEnc(f,d)
#define do_xmlFreeDoc(d)
#define do_xmlCleanupParser()
#define do_xmlParseFile(doc,file)
#define do_xmlDocGetRootElement(n,d)
#define do_xmlStrcmp(res,str1,str2)
#define do_xmlGetProp(res,n,str)
#define do_xmlCopyNode(new,old,rec)
#define do_xmlNodeGetContent(str,node)
#define do_xmlCreateIntSubset(d,e,fn)
#endif
#endif
