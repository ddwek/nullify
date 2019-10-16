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
#ifndef _SRC_REPLAY_H_
#define _SRC_REPLAY_H_
#if defined(HAVE_XML_LOGS)
#include <libxml/parser.h>

xmlNodePtr parse_xml_session(char *filename);
xmlNodePtr replay_get_next_iter(xmlNodePtr start);
xmlNodePtr replay_get_next_matching_iter(xmlNodePtr ref, const char *element);
xmlDocPtr replay_get_this_hand(xmlNodePtr hand);
void replay_hand(xmlNodePtr hand);
void replay_list_hands(xmlNodePtr session);
xmlDocPtr replay_select_hand(xmlNodePtr startnode, int ngame, int nhand);
#endif
#endif
