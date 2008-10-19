/* ****************************************************************************
* File: top.cpp                                                Part of Bylins *
* Usage: ��� ������� ���������                                                *
* (c) 2006 Krodo                                                              *
******************************************************************************/

#include "conf.h"
#include <functional>
#include <sstream>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "top.h"
#include "glory.hpp"
#include "char.hpp"

extern const char *class_name[];

TopListType TopPlayer::TopList(NUM_CLASSES);

// ��������� �������� �� ������ (��� ��������, ������� � �.�.)
// ������ ������� �������� � ��� ����� � � ����������� ����������� ����������
// ����������� � ����������� � load_char_ascii
void TopPlayer::Remove(CHAR_DATA * short_ch)
{
	TopPlayer::TopList[static_cast<int>(GET_CLASS(short_ch))].remove_if(
		boost::bind(std::equal_to<long>(),
					boost::bind(&TopPlayer::unique, _1), GET_UNIQUE(short_ch)));
}

// ��������� ����-�� ��������� � ��� � ���������/��������� ��� ������. reboot �� ������� 0 (1 ��� ������)
// ������ ������� �������� � ��� ����� � � ����������� ����������� ����������
// ����������� � ����������� � load_char_ascii
void TopPlayer::Refresh(CHAR_DATA * short_ch, bool reboot)
{
	if (IS_NPC(short_ch) || IS_SET(PLR_FLAGS(short_ch, PLR_FROZEN), PLR_FROZEN)
			|| IS_SET(PLR_FLAGS(short_ch, PLR_DELETED), PLR_DELETED) || IS_IMMORTAL(short_ch))
		return;

	int class_num = static_cast<int>(GET_CLASS(short_ch)); // � �� �� ������ ����� ���������

	if (!reboot)
		TopPlayer::Remove(short_ch);

	// ������� ������ �� �������� � ����� � ������� ���� ����������
	std::list<TopPlayer>::iterator it_exp;
	for (it_exp = TopPlayer::TopList[class_num].begin(); it_exp != TopPlayer::TopList[class_num].end(); ++it_exp)
		if (it_exp->remort < GET_REMORT(short_ch) || (it_exp->remort == GET_REMORT(short_ch) && it_exp->exp < GET_EXP(short_ch)))
			break;

	if (!GET_NAME(short_ch)) return; // � ��� ��� ����� ����
	TopPlayer temp_player(GET_UNIQUE(short_ch), GET_NAME(short_ch), GET_EXP(short_ch), GET_REMORT(short_ch));

	if (it_exp != TopPlayer::TopList[class_num].end())
		TopPlayer::TopList[class_num].insert(it_exp, temp_player);
	else
		TopPlayer::TopList[class_num].push_back(temp_player);
}

const char * TopPlayer::TopFormat[NUM_CLASSES + 1] =
{
	"������",
	"�������",
	"����",
	"��������",
	"��������",
	"����������",
	"���������",
	"����������",
	"�������������",
	"������",
	"��������",
	"�������",
	"�����",
	"������",
	"������"
};

// ������� '������'
ACMD(DoBest)
{
	if (IS_NPC(ch))
		return;

	std::string buffer = argument;
	boost::trim(buffer);

	bool find = 0;
	int class_num = 0;
	// ��� � ����� <= ��� ����� '������' ����� �������
	for (; class_num <= NUM_CLASSES; ++class_num)
	{
		if (CompareParam(buffer, TopPlayer::TopFormat[class_num]))
		{
			find = 1;
			break;
		}
	}

	if (find)
	{
		std::ostringstream out;
		out << CCWHT(ch, C_NRM) << "������ " << TopPlayer::TopFormat[class_num] << ":" << CCNRM(ch, C_NRM) << "\r\n";

		if (class_num < NUM_CLASSES)   // ���������� �����
		{
			boost::format class_format("\t%-20s %-2d %s\r\n");
			int i = 0;
			for (std::list<TopPlayer>::const_iterator it = TopPlayer::TopList[class_num].begin(); it != TopPlayer::TopList[class_num].end() && i < MAX_TOP_CLASS; ++it, ++i)
				out << class_format % it->name % it->remort % desc_count(it->remort, WHAT_REMORT);

			// ���� ����� ��������� � ������ ���� - ������� ���, ����� �� ���������
			int count = 1;
			std::list<TopPlayer>::iterator find_me = TopPlayer::TopList[class_num].begin();
			for (; find_me != TopPlayer::TopList[class_num].end(); ++find_me, ++count)
				if (find_me->unique == GET_UNIQUE(ch))
					break;
			if (find_me != TopPlayer::TopList[class_num].end())
				out << "��� ������� �������: " << count << "\r\n";

			send_to_char(ch, out.str().c_str());
		}
		else   // ��� �����
		{
			int i = 0;
			boost::format all_format("\t%-20s %-2d %-17s %s\r\n");
			for (TopListType::const_iterator it = TopPlayer::TopList.begin(); it != TopPlayer::TopList.end(); ++it, ++i)
				if (!it->empty())
					out << all_format % it->begin()->name % it->begin()->remort % desc_count(it->begin()->remort, WHAT_REMORT) % class_name[i];
			send_to_char(ch, out.str().c_str());
		}
	}
	else
	{
		// ��� �����
		if (CompareParam(buffer, "�������������"))
		{
			Glory::print_glory_top(ch);
			return;
		}

		std::ostringstream out;
		out.setf(std::ios_base::left, std::ios_base::adjustfield);
		out << "������� ����� ����:\r\n";
		for (int i = 0, j = 1; i <= NUM_CLASSES; ++i, ++j)
			out << std::setw(15) << TopPlayer::TopFormat[i] << (j % 4 ? "" : "\r\n");

		out << std::setw(15) << "�������������\r\n";
		out << "\r\n";
		send_to_char(ch, out.str().c_str());
		return;
	}
}
