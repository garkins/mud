// Copyright (c) 2005 Krodo
// Part of Bylins http://www.mud.ru

#ifndef _BOARDS_H_
#define _BOARDS_H_

#include "boards_message.h"
#include "boards_types.h"
#include "db.h"
#include "interpreter.h"
#include "structs/structs.h"
#include "conf.h"
#include "sysdep.h"

#include <string>
#include <vector>
#include <bitset>

namespace Boards {
extern std::string dg_script_text;

class Static {
 public:
	static bool LoginInfo(CharacterData *ch);
	static void BoardInit();
	static void ClanInit();
	static int Special(CharacterData *, void *, int, char *);
	static std::string print_stats(CharacterData *ch, Board::shared_ptr board, int num);
	static void reload_all();
	static void clear_god_boards();
	static void init_god_board(long uid, std::string name);
	static void do_list(CharacterData *ch, const Board::shared_ptr board);

	static bool can_see(CharacterData *ch, const Board::shared_ptr board);
	static bool can_read(CharacterData *ch, const Board::shared_ptr board);
	static bool can_write(CharacterData *ch, const Board::shared_ptr board);
	static bool full_access(CharacterData *ch, const Board::shared_ptr board);
	static void clan_delete_message(const std::string &name, int vnum);
	static void new_message_notify(const Board::shared_ptr board);

	static Board::shared_ptr create_board(BoardTypes type, const std::string &name,
										  const std::string &desc, const std::string &file);

 private:
	static std::bitset<ACCESS_NUM> get_access(CharacterData *ch, const Board::shared_ptr board);
};

void report_on_board(CharacterData *ch, char *argument, int cmd, int subcmd);
void DoBoard(CharacterData *ch, char *argument, int cmd, int subcmd);
void DoBoardList(CharacterData *ch, char *argument, int cmd, int subcmd);

} // namespace BoardSystem

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
