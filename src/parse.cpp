// Copyright (c) 2013 Krodo
// Part of Bylins http://www.mud.ru

#include "parse.h"

#include "obj_prototypes.h"
#include "utils/utils.h"

namespace TextId {

///
/// Содержит списки соответствия номер=строка/строка=номер для конверта
/// внутри-игровых констант в строковые ИД и обратно.
/// Нужно для независимости файлов от изменения номеров констант в коде.
///
class TextIdNode {
 public:
	void add(int num, std::string str);

	std::string to_str(int num) const;
	int to_num(const std::string &str) const;

 private:
	std::unordered_map<int, std::string> num_to_str;
	std::unordered_map<std::string, int> str_to_num;
};

// общий список конвертируемых констант
std::array<TextIdNode, TEXT_ID_COUNT> text_id_list;

///
/// Инит текстовых ИД классов для конфига.
///
void init_char_class() {
	text_id_list.at(CHAR_CLASS).add(ECharClass::kSorcerer, "kSorcerer");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kConjurer, "kConjurer");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kThief, "kThief");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kWarrior, "kWarrior");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kAssasine, "kAssasine");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kGuard, "kGuard");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kCharmer, "kCharmer");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kWizard, "kWizard");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kNecromancer, "kNecromancer");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kPaladine, "kPaladine");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kRanger, "kRanger");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kMerchant, "kMerchant");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kMagus, "kMagus");
	text_id_list.at(CHAR_CLASS).add(ECharClass::kVigilant, "kVigilant");
}

///
/// Инит текстовых ИД параметров предметов для сохранения в файл.
///
void init_obj_vals() {
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_NUM), "POTION_SPELL1_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL1_LVL), "POTION_SPELL1_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_NUM), "POTION_SPELL2_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL2_LVL), "POTION_SPELL2_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_NUM), "POTION_SPELL3_NUM");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_SPELL3_LVL), "POTION_SPELL3_LVL");
	text_id_list.at(OBJ_VALS).add(to_underlying(ObjVal::EValueKey::POTION_PROTO_VNUM), "POTION_PROTO_VNUM");
}

///
/// Общий инит системы текстовых ИД, дергается при старте мада.
///
void init() {
	/// CHAR_CLASS
	init_char_class();
	/// OBJ_VALS
	init_obj_vals();
	/// ...
}

///
/// Конвертирование текстового ИД константы в ее значение в коде.
/// \return значение константы или -1, если ничего не было найдено
///
int to_num(IdType type, const std::string &str) {
	if (type < TEXT_ID_COUNT) {
		return text_id_list.at(type).to_num(str);
	}
	return -1;
}

///
/// Конвертирование значения константы в ее текстовый ИД.
/// \return текстовый ИД константы или пустая строка, если ничего не было найдено
///
std::string to_str(IdType type, int num) {
	if (type < TEXT_ID_COUNT) {
		return text_id_list.at(type).to_str(num);
	}
	return "";
}

///
/// Добавление соответствия значение=константа/константа=значение
///
void TextIdNode::add(int num, std::string str) {
	num_to_str.insert(std::make_pair(num, str));
	str_to_num.insert(std::make_pair(str, num));
}

///
/// Конвертирование значение -> текстовый ИД
///
std::string TextIdNode::to_str(int num) const {
	auto i = num_to_str.find(num);
	if (i != num_to_str.end()) {
		return i->second;
	}
	return "";
}

///
/// Конвертирование текстовый ИД -> значение
///
int TextIdNode::to_num(const std::string &str) const {
	auto i = str_to_num.find(str);
	if (i != str_to_num.end()) {
		return i->second;
	}
	return -1;
}

} // namespace TextId

namespace Parse {

///
/// Попытка конвертирования \param text в <int> с перехватом исключения
/// \return число или -1, в случае неудачи
///
int cast_to_int(const char *text) {
	int result = -1;

	try {
		result = std::stoi(text, nullptr, 10);
	}
	catch (...) {
		snprintf(buf, kMaxStringLength, "...lexical_cast<int> fail (value='%s')", text);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
	}

	return result;
}

///
/// Обертка на pugixml для чтения числового аттрибута
/// с логирование в имм- и сислог
/// В конфиге это выглядит как <param value="1234" />
/// \return -1 в случае неудачи
int attr_int(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
	}
	return cast_to_int(attr.value());
}

// тоже самое, что и attr_int, только, если элемента нет, возвращает -1
int attr_int_t(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		return -1;
	}
	return cast_to_int(attr.value());
}

///
/// Тоже, что и attr_int, только для чтения child_value()
/// В конфиге это выглядит как <param>1234<param>
///
int child_value_int(const pugi::xml_node &node, const char *text) {
	pugi::xml_node child_node = node.child(text);
	if (!child_node) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
	}
	return cast_to_int(child_node.child_value());
}

///
/// Аналог attr_int, \return строку со значением или пустую строку
///
std::string attr_str(const pugi::xml_node &node, const char *text) {
	pugi::xml_attribute attr = node.attribute(text);
	if (!attr) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
	} else {
		return attr.value();
	}
	return "";
}

///
/// Аналог child_value_int, \return строку со значением или пустую строку
///
std::string child_value_str(const pugi::xml_node &node, const char *text) {
	pugi::xml_node child_node = node.child(text);
	if (!child_node) {
		snprintf(buf, kMaxStringLength, "...%s read fail", text);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
	} else {
		return child_node.child_value();
	}
	return "";
}

template<class T>
pugi::xml_node get_child_template(const T &node, const char *name) {
	pugi::xml_node tmp_node = node.child(name);
	if (!tmp_node) {
		char tmp[100];
		snprintf(tmp, sizeof(tmp), "...<%s> read fail", name);
		mudlog(tmp, CMP, kLevelImmortal, SYSLOG, true);
	}
	return tmp_node;
}

pugi::xml_node get_child(const pugi::xml_document &node, const char *name) {
	return get_child_template(node, name);
}

pugi::xml_node get_child(const pugi::xml_node &node, const char *name) {
	return get_child_template(node, name);
}

///
/// проверка валидности внума объекта с логированием ошибки в имм и сислог
/// \return true - если есть прототип объекта (рнум) с данным внумом
///
bool valid_obj_vnum(int vnum) {
	if (real_object(vnum) < 0) {
		snprintf(buf, sizeof(buf), "...bad obj vnum (%d)", vnum);
		mudlog(buf, CMP, kLevelImmortal, SYSLOG, true);
		return false;
	}

	return true;
}

} // namespace Parse

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
