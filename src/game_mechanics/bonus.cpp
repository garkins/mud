// Copyright (c) 2016 bodrich
// Part of Bylins http://www.bylins.su

#include "bonus.h"

#include "bonus_command_parser.h"
#include "handler.h"
#include "modify.h"
#include "entities/char_player.h"

namespace Bonus {
const size_t MAXIMUM_BONUS_RECORDS = 10;

struct BonusInfo {
	// время бонуса, в неактивном состоянии -1
	int time_bonus = -1;

	// множитель бонуса
	int mult_bonus = 2;

	// типа бонуса
	// 0 - оружейный
	// 1 - опыт
	// 2 - дамаг
	EBonusType type_bonus = EBonusType::BONUS_EXP;
};

BonusInfo bonus_info;

// история бонусов
typedef std::list<std::string> bonus_log_t;
bonus_log_t bonus_log;

void do_bonus_info(CharacterData *ch, char * /*argument*/, int/* cmd*/, int/* subcmd*/) {
	show_log(ch);
}

bool can_get_bonus_exp(CharacterData *ch) {
	if (IS_NPC(ch)) {
		return false;
	}

	// бонус опыта распространяется по 49 реморт включительно
	const short MAX_REMORT_FOR_BONUS = 49;
	return GET_REAL_REMORT(ch) <= MAX_REMORT_FOR_BONUS;
}

void setup_bonus(const int duration, const int multilpier, EBonusType type) {
	bonus_info.time_bonus = duration;
	bonus_info.mult_bonus = multilpier;
	bonus_info.type_bonus = type;
}

void bonus_log_add(const std::string &name) {
	time_t nt = time(nullptr);
	std::stringstream ss;
	ss << rustime(localtime(&nt)) << " " << name;
	std::string buf = ss.str();
	boost::replace_all(buf, "\r\n", "");
	bonus_log.push_back(buf);

	std::ofstream fout("../log/bonus.log", std::ios_base::app);
	fout << buf << std::endl;
	fout.close();
}

class AbstractErrorReporter {
 public:
	using shared_ptr = std::shared_ptr<AbstractErrorReporter>;

	virtual ~AbstractErrorReporter() {}
	virtual void report(const std::string &message) = 0;
};

class CharacterReporter : public AbstractErrorReporter {
 public:
	CharacterReporter(CharacterData *character) : m_character(character) {}

	virtual void report(const std::string &message) override;

	static shared_ptr create(CharacterData *character) { return std::make_shared<CharacterReporter>(character); }

 private:
	CharacterData *m_character;
};

void CharacterReporter::report(const std::string &message) {
	send_to_char(message.c_str(), m_character);
}

class MudlogReporter : public AbstractErrorReporter {
 public:
	virtual void report(const std::string &message) override;

	static shared_ptr create() { return std::make_shared<MudlogReporter>(); }
};

void MudlogReporter::report(const std::string &message) {
	mudlog(message.c_str(), DEF, kLevelBuilder, ERRLOG, true);
}

void do_bonus(const AbstractErrorReporter::shared_ptr &reporter, const char *argument) {
	ArgumentsParser bonus(argument, bonus_info.type_bonus, bonus_info.time_bonus);

	bonus.parse();

	const auto result = bonus.result();
	switch (result) {
		case ArgumentsParser::ER_ERROR: reporter->report(bonus.error_message().c_str());
			break;

		case ArgumentsParser::ER_START:
			switch (bonus.type()) {
				case EBonusType::BONUS_DAMAGE: reporter->report("Режим бонуса \"урон\" в настоящее время отключен.");
					break;

				default: {
					const std::string &message = bonus.broadcast_message();
					send_to_all(message.c_str());
					std::stringstream ss;
					ss << "&W" << message << "&n\r\n";
					bonus_log_add(ss.str());
					setup_bonus(bonus.time(), bonus.multiplier(), bonus.type());
				}
			}
			break;

		case ArgumentsParser::ER_STOP: send_to_all(bonus.broadcast_message().c_str());
			bonus_info.time_bonus = -1;
			break;
	}
}

void do_bonus_by_character(CharacterData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	const auto reporter = CharacterReporter::create(ch);
	do_bonus(reporter, argument);
}

// вызывает функцию do_bonus
void dg_do_bonus(char *cmd) {
	const auto reporter = MudlogReporter::create();
	do_bonus(reporter, cmd);
}

std::string time_to_bonus_end_as_string() {
	std::stringstream ss;
	if (bonus_info.time_bonus > 4) {
		ss << "До конца бонуса осталось " << bonus_info.time_bonus << " часов.";
	} else if (bonus_info.time_bonus == 4) {
		ss << "До конца бонуса осталось четыре часа.";
	} else if (bonus_info.time_bonus == 3) {
		ss << "До конца бонуса осталось три часа.";
	} else if (bonus_info.time_bonus == 2) {
		ss << "До конца бонуса осталось два часа.";
	} else if (bonus_info.time_bonus == 1) {
		ss << "До конца бонуса остался последний час!";
	} else {
		ss << "Бонуса нет.";
	}
	return ss.str();
}

std::string active_bonus_as_string() {
	if (bonus_info.time_bonus == -1) {
		return std::string();
	}

	switch (bonus_info.type_bonus) {
		case EBonusType::BONUS_DAMAGE: return "Сейчас идет бонус: повышенный урон.";

		case EBonusType::BONUS_EXP: return "Сейчас идет бонус: повышенный опыт за убийство моба.";

		case EBonusType::BONUS_WEAPON_EXP: return "Сейчас идет бонус: повышенный опыт от урона оружия.";
	}

	return "Неизвестный бонус. Сообщите богам.";
}

// таймер бонуса
void timer_bonus() {
	if (bonus_info.time_bonus <= -1) {
		return;
	}
	bonus_info.time_bonus--;
	if (bonus_info.time_bonus < 1) {
		send_to_all("&WБонус закончился...&n\r\n");
		bonus_info.time_bonus = -1;
		return;
	}
	std::string bonus_str = "&W" + time_to_bonus_end_as_string() + "&n\r\n";
	send_to_all(bonus_str.c_str());
}

// проверка на тип бонуса
bool is_bonus_active(EBonusType type) {
	if (bonus_info.time_bonus == -1) {
		return false;
	}

	return type == bonus_info.type_bonus;
}

bool is_bonus_active() {
	return bonus_info.time_bonus != -1;
}

// загружает лог бонуса из файла
void bonus_log_load() {
	std::ifstream fin("../log/bonus.log");
	if (!fin.is_open()) {
		return;
	}
	std::string temp_buf;
	bonus_log.clear();
	while (std::getline(fin, temp_buf)) {
		bonus_log.push_back(temp_buf);
	}
	fin.close();
}

// выводит весь лог в обратном порядке
void show_log(CharacterData *ch) {
	if (bonus_log.empty()) {
		send_to_char(ch, "Лог пустой!\r\n");
		return;
	}

	std::stringstream buf_str;

	for (auto[it, count] = std::tuple(bonus_log.rbegin(), 0ul);
		 it != bonus_log.rend() && count <= MAXIMUM_BONUS_RECORDS; ++it, ++count) {
		buf_str << "&G" << count << ". &W" << *it << "&n\r\n";
	}

	page_string(ch->desc, buf_str.str());
}

// возвращает множитель бонуса
int get_mult_bonus() {
	return bonus_info.mult_bonus;
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
