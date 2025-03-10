/* ************************************************************************
*   File: comm.h                                        Part of Bylins    *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _COMM_H_
#define _COMM_H_

#include "structs/structs.h"
#include "structs/descriptor_data.h"
#include <string>
#include <sstream>

#define NUM_RESERVED_DESCS    8

class CObjectPrototype;    // forward declaration to avoid inclusion of obj.hpp and any dependencies of that header.
class CharacterData;    // forward declaration to avoid inclusion of char.hpp and any dependencies of that header.

extern DescriptorData *descriptor_list;

// comm.cpp
void send_to_all(const char *messg);
void send_to_char(const char *messg, const CharacterData *ch);
void send_to_char(const CharacterData *ch, const char *messg, ...) __attribute__((format(printf, 2, 3)));
void send_to_char(const std::string &buffer, const CharacterData *ch);
void send_stat_char(const CharacterData *ch);
void send_to_room(const char *messg, RoomRnum room, int to_awake);
void send_to_outdoor(const char *messg, int control);
void send_to_gods(const char *messg);
void perform_to_all(const char *messg, CharacterData *ch);
#ifdef HAS_EPOLL
void close_socket(DescriptorData *d, int direct, int epoll, struct epoll_event *events, int n_ev);
#else
void close_socket(DescriptorData * d, int direct);
#endif

void perform_act(const char *orig,
				 CharacterData *ch,
				 const ObjectData *obj,
				 const void *vict_obj,
				 CharacterData *to,
				 const int arena,
				 const std::string &kick_type);

inline void perform_act(const char *orig,
						CharacterData *ch,
						const ObjectData *obj,
						const void *vict_obj,
						CharacterData *to,
						const std::string &kick_type) {
	perform_act(orig, ch, obj, vict_obj, to, 0, kick_type);
}
inline void perform_act(const char *orig,
						CharacterData *ch,
						const ObjectData *obj,
						const void *vict_obj,
						CharacterData *to,
						const int arena) {
	perform_act(orig, ch, obj, vict_obj, to, arena, "");
}
inline void perform_act(const char *orig, CharacterData *ch, const ObjectData *obj, const void *vict_obj, CharacterData *to) {
	perform_act(orig, ch, obj, vict_obj, to, 0, "");
}

void act(const char *str,
		 int hide_invisible,
		 CharacterData *ch,
		 const ObjectData *obj,
		 const void *vict_obj,
		 int type,
		 const std::string &kick_type);

inline void act(const char *str,
				int hide_invisible,
				CharacterData *ch,
				const ObjectData *obj,
				const void *vict_obj,
				int type) {
	act(str, hide_invisible, ch, obj, vict_obj, type, "");
}
inline void act(const std::stringstream &str,
				int hide_invisible,
				CharacterData *ch,
				const ObjectData *obj,
				const void *vict_obj,
				int type) {
	act(str.str().c_str(), hide_invisible, ch, obj, vict_obj, type);
}

unsigned long get_ip(const char *addr);

#define SUN_CONTROL     (1 << 0)
#define WEATHER_CONTROL (1 << 1)

#define TO_ROOM        1
#define TO_VICT        2
#define TO_NOTVICT    3
#define TO_CHAR        4
#define TO_ROOM_HIDE    5    // В комнату, но только тем, кто чувствует жизнь
#define CHECK_NODEAF        32   // посылать только глухим
#define CHECK_DEAF          64   // не посылать глухим
#define TO_SLEEP            128  // to char, even if sleeping
#define TO_ARENA_LISTEN     512  // не отсылать сообщение с арены слушателям, чтоб не спамить передвижениями и тп
#define TO_BRIEF_SHIELDS    1024 // отсылать только тем, у кого включен режим PRF_BRIEF_SHIELDS
#define TO_NO_BRIEF_SHIELDS 2048 // отсылать только тем, у кого нет режима PRF_BRIEF_SHIELDS

// I/O functions
int write_to_descriptor(socket_t desc, const char *txt, size_t total);
bool write_to_descriptor_with_options(DescriptorData *t, const char *buffer, size_t byffer_size, int &written);
void write_to_q(const char *txt, struct TextBlocksQueue *queue, int aliased);
void write_to_output(const char *txt, DescriptorData *d);
void string_add(DescriptorData *d, char *str);
void string_write(DescriptorData *d, const utils::AbstractStringWriter::shared_ptr &writer,
				  size_t len, int mailto, void *data);
int toggle_compression(DescriptorData *d);

#define SEND_TO_Q(messg, desc)        write_to_output((messg), desc)
#define SEND_TO_SOCKET(messg, desc)    write_to_descriptor((desc), (messg), strlen(messg))

typedef RETSIGTYPE sigfunc(int);

extern unsigned long cmd_cnt;

#define DEFAULT_REBOOT_UPTIME 60*24*6    //время до ближайшего ребута по умолчанию в минутах
#define UPTIME_THRESHOLD      120    //минимальный аптайм для ребута в часах

void timediff(struct timeval *diff, struct timeval *a, struct timeval *b);

int main_function(int argc, char **argv);

void gifts();

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
