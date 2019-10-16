/*
 * replay.c: functions to support replaying of saved games. 
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "dllst.h"
#include "misc.h"
#include "xmlwrappers.h"

#define SUIT_CLUBS              0
#define SUIT_DIAMONDS           1
#define SUIT_HEARTS             2
#define SUIT_SPADES             3
#define CARD_SUIT(x)            (*((unsigned int *)x->fields + 0))
#define CARD_NUMBER(x)          (*((unsigned int *)x->fields + 2))

typedef enum { EXPOSURE_CARD=0, DELETE_CARD, ADD_CARD, GET_CARD } action_t;

extern struct player_st {
	char name[20];
	boolean_t active;
	dllst_t *list;
	float probabilities[17];
	int scores;
	int specialpts;
} player[4];
extern unsigned turn;
extern int lastsuit;

extern int getcardfromdeck(int nplayer, action_t act);
extern void playcard(int n, int suit, int number, unsigned long *param);
extern void finish_hand(void);

/*
 *
 * Parse and validate the XML session file @filename and return
 * the root element (pointer to <session>).
 *
 */
xmlNodePtr parse_xml_session(char *filename)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;
	xmlParserCtxtPtr ctxt;


	ctxt = xmlNewParserCtxt();
	if (!ctxt) {
		printf("Failed to allocate parser context\n");
		return NULL;
	}

	doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_DTDVALID);
	if (!doc) {
		xmlFreeParserCtxt(ctxt);
		return NULL;
	}

	if (!ctxt->valid) {
		printf("Failed to validate %s\n", filename);
		xmlFreeParserCtxt(ctxt);
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlFreeParserCtxt(ctxt);

	do_xmlDocGetRootElement(cur, doc);
	if (!cur) {
		do_xmlFreeDoc(doc);
		return NULL;
	}

	return cur;
}

/*
 *
 * Walk the subtree starting at @start to find the next iterator,
 * which can be either a child, an uncle or a sibling node.
 *
 */
xmlNodePtr replay_get_next_iter(xmlNodePtr start)
{
	xmlNodePtr ret = NULL;


	if (start->children)
		ret = start->children;
	else if (start == start->parent->last)
		ret = start->parent->next;
	else
		ret = start->next;

	return ret;
}

/*
 *
 * Walk the subtree starting at @ref to find the next element
 * matching @element.
 *
 */
xmlNodePtr replay_get_next_matching_iter(xmlNodePtr ref, const char *element)
{
	int res;
	xmlNodePtr cur = ref;


	if (!ref)
		return NULL;

	if (!element)
		return ref;

	while (1) {
cmp_element:
		if (!cur)
			break;

		do_xmlStrcmp(res, cur->name, element);
		if (res) {
			cur = replay_get_next_iter(cur);
			goto cmp_element;
		}

		return cur;
	}

	return NULL;
}

/*
 *
 * Get the subdocument starting at @hand, which is a partial copy
 * of the external global document handler @xml_logfile.
 *
 */
xmlDocPtr replay_get_this_hand(xmlNodePtr hand)
{
	xmlDocPtr xml_copyfile;
	xmlNodePtr session, game, node;
	char buf[8] = { '\0' };


	do_xmlNewDoc(xml_copyfile, "1.0");
	do_xmlNewNode(session, "session");
	do_xmlDocSetRootElement(xml_copyfile, session);

	do_xmlNewNode(game, "game");
	sprintf(buf, "0");
	do_xmlNewProp(game, "id", buf);
	do_xmlAddChild(session, game);
	do_xmlCopyNode(node, hand, 1);
	do_xmlAddChild(game, node);

	return xml_copyfile;
}

/*
 *
 * Reproduce the saved game:hand by iterating recursively over the
 * subtree starting at @hand. Only <turn>, <card> and <msg> are
 * meaningful elements to parse.
 *
 */
void replay_hand(xmlNodePtr hand)
{
	int card_suit, card_number;
	unsigned long j;
	xmlNodePtr ref;
	char *str = NULL;
	dllst_item_struct_t *iter;


	for (ref=hand;ref;ref=ref->next) {
		if (!strcmp(ref->name, "turn")) {
			do_xmlGetProp(str, ref, "id");
			turn = strtol(str, NULL, 10);
			printf("%s:\n", player[turn].name);
		} else if (!strcmp(ref->name, "card")) {
			do_xmlNodeGetContent(str, ref->children);
			decode_card_rev(str, &card_suit, &card_number);
			lastsuit = card_suit;
			for (j=0,iter=player[turn].list->head;iter;iter=iter->next,j++)
				if (CARD_SUIT(iter) == card_suit && CARD_NUMBER(iter) == card_number)
					break;
	
			playcard(turn, card_suit, card_number, &j);
		} else if (!strcmp(ref->name, "msg")) {
			do_xmlNodeGetContent(str, ref->children);
	
			if (!strcmp(str, "CLUBS selected")) {
				lastsuit = SUIT_CLUBS;
				printf("\t\t%s\n", str);
			} else if (!strcmp(str, "DIAMONDS selected")) {
				lastsuit = SUIT_DIAMONDS;
				printf("\t\t%s\n", str);
			} else if (!strcmp(str, "HEARTS selected")) {
				lastsuit = SUIT_HEARTS;
				printf("\t\t%s\n", str);
			} else if (!strcmp(str, "SPADES selected")) {
				lastsuit = SUIT_SPADES;
				printf("\t\t%s\n", str);
			} else if (!strcmp(str, "finished")) {
				player[turn].active = FALSE;
				printf("\t\t%s\n", str);
			} else if (!strcmp(str, "Getting 1 card from the deck")) {
				getcardfromdeck(turn, ADD_CARD);
				printf("\t\t%s\n", str);
			} else if (!strncmp(str, "New static", 10)) {
				printf("\t\t%s\n", str);
			} else if (!strncmp(str, "I have no cards", 15)) {
				printf("\t\t%s\n", str);
			} else if (!strncmp(str, "Not taking", 10)) {
				printf("\t\t%s\n", str);
			}
		}

		replay_hand(ref->children);
	}
}

/*
 *
 * Display available games and hands from the root node @session.
 *
 */
void replay_list_hands(xmlNodePtr session)
{
	xmlNodePtr ref;
	char *str = NULL;


	for (ref=session;ref;ref=ref->next) {
		if (!strcmp(ref->name, "game")) {
			do_xmlGetProp(str, ref, "id");
			printf("%ld\n", strtol(str, NULL, 10));
		} else if (!strcmp(ref->name, "hand")) {
			do_xmlGetProp(str, ref, "id");
			printf("\t%ld\n", strtol(str, NULL, 10));
		}

		replay_list_hands(ref->children);
	}
}

/*
 *
 * Get the subdocument as of @startnode which contains the requested
 * game @ngame and hand @nhand.
 *
 */
xmlDocPtr replay_select_hand(xmlNodePtr startnode, int ngame, int nhand)
{
	xmlNodePtr ref;
	xmlDocPtr doc = NULL;
	char *str = NULL;
	static int game = 0, end_recursion = 0;


	for (ref=startnode;ref;ref=ref->next) {
		if (!strcmp(ref->name, "game")) {
			do_xmlGetProp(str, ref, "id");
			game = strtol(str, NULL, 10);
		} else if (!strcmp(ref->name, "hand")) {
			do_xmlGetProp(str, ref, "id");
			if (strtol(str, NULL, 10) == nhand && game == ngame) {
				doc = replay_get_this_hand(ref);
				end_recursion = 1;
			}
		}

		if (end_recursion)
			return doc;

		doc = replay_select_hand(ref->children, ngame, nhand);
	}

	return doc;
}

