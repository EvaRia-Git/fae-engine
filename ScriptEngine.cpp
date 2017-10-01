#include"ScriptEngine.hpp"
#include<vector>
#include<cctype>
#include<cstdio>
#include<clocale>
#include<cmath>
#include<cassert>

#ifdef _MSC_VER
#define for if(0);else for
namespace std
{
	using::wcstombs;
	using::mbstowcs;
	using::isalpha;
	using::fmodl;
	using::powl;
	using::swprintf;
	using::atof;
	using::isdigit;
	using::isxdigit;
	using::floorl;
	using::ceill;
	using::fabsl;
}

#endif

using namespace gstd;


std::string gstd::to_mbcs(std::wstring const & s)
{
	int len = std::wcstombs(NULL, s.c_str(), s.size());
	if (len < 0)
		return "(BAD-DATA)";
	char * buffer = new char[len + 1];
	std::wcstombs(buffer, s.c_str(), len);
	std::string result(buffer, len);
	delete[] buffer;
	return result;
}

std::wstring gstd::to_wide(std::string const & s)
{
	int len = std::mbstowcs(NULL, s.c_str(), s.size());
	wchar_t * buffer = new wchar_t[len + 1];
	std::mbstowcs(buffer, s.c_str(), len);
	std::wstring result(buffer, len);
	delete[] buffer;
	return result;
}

//--------------------------------------

/* parser_error */

class parser_error : public std::exception
{
public:
	parser_error(std::string const & the_message) : std::exception(), message(the_message)
	{
	}

	virtual const char * what() const throw();

private:
	std::string message;
};

const char * parser_error::what() const throw()
{
	return message.c_str();
}

/* lexical analyzer */

enum token_kind
{
	tk_end, tk_invalid, tk_word, tk_real, tk_char, tk_string, tk_open_par, tk_close_par, tk_open_bra, tk_close_bra,
	tk_open_cur, tk_close_cur, tk_open_abs, tk_close_abs, tk_comma, tk_semicolon, tk_arrow, tk_tilde, tk_assign, tk_plus, tk_minus,
	tk_inc, tk_dec, tk_asterisk, tk_slash, tk_percent, tk_caret, tk_e, tk_g, tk_ge, tk_l, tk_le, tk_ne, tk_exclamation,
	tk_ampersand, tk_and_then, tk_vertical, tk_or_else, tk_at, tk_add_assign, tk_subtract_assign, tk_multiply_assign,
	tk_divide_assign, tk_remainder_assign, tk_power_assign, tk_concat_assign, tk_range, tk_EVENTS, tk_FOR, tk_BREAK, tk_ON, tk_REVERSE,
	tk_ELSE, tk_FUNCTION, tk_IF, tk_IN, tk_LET, tk_LOCAL, tk_LOOP, tk_REAL, tk_RETURN, tk_SUB, tk_TASK,
	tk_TIMES, tk_WHILE, tk_YIELD, tk_EXIT
};

class scanner
{
public:
	char const * current;
	token_kind next;
	std::string word;
	long double real_value;
	wchar_t char_value;
	std::wstring string_value;
	int line;

	scanner(char const * source) : current(source), line(1)
	{
		advance();
	}

	scanner(scanner const & source) : current(source.current), next(source.next), word(source.word), line(source.line)
	{
	}

	void skip();
	void advance();
};

void scanner::skip()
{
	//skip whitespace
	while (*current == 13 || *current == 10 || *current == 9 || *current == 32
		|| *current == '#' || (*current == '/' && (current[1] == '/' || current[1] == '*')))
	{
		//skip comments
		if (*current == '#' ||
			(*current == '/' && (current[1] == '/' || current[1] == '*')))
		{
			if (*current == '#' || current[1] == '/')
			{
				do
				{
					++current;
				} while (*current != '\n'&&*current != '\r');
			}
			else
			{
				current += 2;
				while (*current != '*' || current[1] != '/')
				{
					if (*current == '\n') ++line;
					++current;
				}
				current += 2;
			}
		}
		else if (*current == '\n')
		{
			++line;
			++current;
		}
		else
			++current;
	}
}

void scanner::advance()
{
	skip();

	if (*current == 0)
	{
		next = tk_end;
		return;
	}

	switch (*current)
	{
	case '[':
		next = tk_open_bra;
		++current;
		break;
	case ']':
		next = tk_close_bra;
		++current;
		break;
	case '(':
		next = tk_open_par;
		++current;
		if (*current == '|')
		{
			next = tk_open_abs;
			++current;
		}
		break;
	case ')':
		next = tk_close_par;
		++current;
		break;
	case '{':
		next = tk_open_cur;
		++current;
		break;
	case '}':
		next = tk_close_cur;
		++current;
		break;
	case '@':
		next = tk_at;
		++current;
		break;
	case ',':
		next = tk_comma;
		++current;
		break;
	case ';':
		next = tk_semicolon;
		++current;
		break;
	case '~':
		next = tk_tilde;
		++current;
		if (*current == '=')
		{
			next = tk_concat_assign;
			++current;
		}
		break;
	case '*':
		next = tk_asterisk;
		++current;
		if (*current == '=')
		{
			next = tk_multiply_assign;
			++current;
		}
		break;
	case '/':
		next = tk_slash;
		++current;
		if (*current == '=')
		{
			next = tk_divide_assign;
			++current;
		}
		break;
	case '%':
		next = tk_percent;
		++current;
		if (*current == '=')
		{
			next = tk_remainder_assign;
			++current;
		}
		break;
	case '^':
		next = tk_caret;
		++current;
		if (*current == '=')
		{
			next = tk_power_assign;
			++current;
		}
		break;
	case '=':
		next = tk_assign;
		++current;
		if (*current == '=')
		{
			next = tk_e;
			++current;
		}
		if (*current == '>')
		{
			next = tk_arrow;
			++current;
		}
		break;
	case '>':
		next = tk_g;
		++current;
		if (*current == '=')
		{
			next = tk_ge;
			++current;
		}
		break;
	case '<':
		next = tk_l;
		++current;
		if (*current == '=')
		{
			next = tk_le;
			++current;
		}
		break;
	case '!':
		next = tk_exclamation;
		++current;
		if (*current == '=')
		{
			next = tk_ne;
			++current;
		}
		break;
	case '+':
		next = tk_plus;
		++current;
		if (*current == '+')
		{
			next = tk_inc;
			++current;
		}
		else if (*current == '=')
		{
			next = tk_add_assign;
			++current;
		}
		break;
	case '-':
		next = tk_minus;
		++current;
		if (*current == '-')
		{
			next = tk_dec;
			++current;
		}
		else if (*current == '=')
		{
			next = tk_subtract_assign;
			++current;
		}
		break;
	case '&':
		next = tk_ampersand;
		++current;
		if (*current == '&')
		{
			next = tk_and_then;
			++current;
		}
		break;
	case '|':
		next = tk_vertical;
		++current;
		if (*current == '|')
		{
			next = tk_or_else;
			++current;
		}
		else if (*current == ')')
		{
			next = tk_close_abs;
			++current;
		}
		break;
	case '.':
		++current;
		if (*current == '.')
		{
			next = tk_range;
			++current;
		}
		else
		{
			throw parser_error("�P�Ƃ̃s���I�h�͂��̃X�N���v�g�ł͎g���܂���");
		}
		break;

	case '\'':
	case '\"':
	{
		std::string s;
		char q = *current;
		next = (q == '\"') ? tk_string : tk_char;
		++current;
		while (*current != q)
		{
			s += *current;
			++current;
		}
		++current;
		string_value = to_wide(s);
		if (q == '\'')
		{
			if (string_value.size() == 1)
				char_value = string_value[0];
			else
				throw parser_error("�����^�̒l�̒�����1�����ł�");
		}
	}
	break;
	case '\\':
	{
		++current;
		next = tk_char;
		char c = *current;
		++current;
		switch (c)
		{
		case '0':
			char_value = L'\0';
			break;
		case 'n':
			char_value = L'\n';
			break;
		case 'r':
			char_value = L'\r';
			break;
		case 't':
			char_value = L'\t';
			break;
		case 'x':
			char_value = 0;
			while (std::isxdigit(*current))
			{
				char_value = char_value * 16 + (*current >= 'a') ? *current - 'a' + 10 : (*current >= 'A') ?
					*current - 'A' + 10 : *current - '0';
				++current;
			}
			break;
		default:
			throw parser_error("���ꕶ�����ςł�(�u\"...\"�v��Y��Ă��܂���)");
		}
	}
	break;
	default:
		if (std::isdigit(*current))
		{
			next = tk_real;
			real_value = 0.0;
			do
			{
				real_value = real_value * 10. + (*current - '0');
				++current;
			} while (std::isdigit(*current));
			if (*current == '.' && std::isdigit(*(current + 1)))
			{
				++current;
				long double d = 1;
				while (std::isdigit(*current))
				{
					d = d / 10;
					real_value = real_value + d * (*current - '0');
					++current;
				}
			}
		}
		else if (std::isalpha(*current) || *current == '_')
		{
			next = tk_word;
			word = "";
			do
			{
				word += *current;
				++current;
			} while (std::isalpha(*current) || *current == '_' || std::isdigit(*current));

			if (word == "events")
				next = tk_EVENTS;
			else if (word == "for")
				next = tk_FOR;
			else if (word == "break")
				next = tk_BREAK;
			else if (word == "on")
				next = tk_ON;
			else if (word == "reverse")
				next = tk_REVERSE;
			else if (word == "else")
				next = tk_ELSE;
			else if (word == "function")
				next = tk_FUNCTION;
			else if (word == "if")
				next = tk_IF;
			else if (word == "in")
				next = tk_IN;
			else if (word == "let" || word == "var")
				next = tk_LET;
			else if (word == "local")
				next = tk_LOCAL;
			else if (word == "loop")
				next = tk_LOOP;
			else if (word == "real")
				next = tk_REAL;
			else if (word == "return")
				next = tk_RETURN;
			else if (word == "sub")
				next = tk_SUB;
			else if (word == "task")
				next = tk_TASK;
			else if (word == "times")
				next = tk_TIMES;
			else if (word == "while")
				next = tk_WHILE;
			else if (word == "yield")
				next = tk_YIELD;
			else if (word == "exit")
				next = tk_EXIT;
		}
		else
		{
			next = tk_invalid;
		}
	}
}

/* operations */

value add(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::tk_array)
	{
		if (argv[0].get_type() != argv[1].get_type())
		{
			machine->raise_error("�^����v���܂���");
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array())
		{
			machine->raise_error("��������v���܂���");
			return value();
		}
		value result;
		for (unsigned i = 0; i < argv[1].length_as_array(); ++i)
		{
			value v[2];
			v[0] = argv[0].index_as_array(i);
			v[1] = argv[1].index_as_array(i);
			result.append(argv[1].get_type(), add(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() + argv[1].as_real());
}

value subtract(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);
	if (argv[0].get_type()->get_kind() == type_data::tk_array)
	{
		if (argv[0].get_type() != argv[1].get_type())
		{
			machine->raise_error("�^����v���܂���");
			return value();
		}
		if (argv[0].length_as_array() != argv[1].length_as_array())
		{
			machine->raise_error("��������v���܂���");
			return value();
		}
		value result;
		for (unsigned i = 0; i < argv[1].length_as_array(); ++i)
		{
			value v[2];
			v[0] = argv[0].index_as_array(i);
			v[1] = argv[1].index_as_array(i);
			result.append(argv[1].get_type(), subtract(machine, 2, v));
		}
		return result;
	}
	else
		return value(machine->get_engine()->get_real_type(), argv[0].as_real() - argv[1].as_real());
}

value multiply(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), argv[0].as_real() * argv[1].as_real());
}

value divide(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), argv[0].as_real() / argv[1].as_real());
}

value remainder(script_machine * machine, int argc, value const * argv)
{
	long double x = argv[0].as_real();
	long double y = argv[1].as_real();
	return value(machine->get_engine()->get_real_type(), std::fmodl(x, y));
}

value negative(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), -argv[0].as_real());
}

value power(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), std::powl(argv[0].as_real(), argv[1].as_real()));
}

value compare(script_machine * machine, int argc, value const * argv)
{
	if (argv[0].get_type() == argv[1].get_type())
	{
		int r = 0;

		switch (argv[0].get_type()->get_kind())
		{
		case type_data::tk_real:
		{
			long double a = argv[0].as_real();
			long double b = argv[1].as_real();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::tk_char:
		{
			wchar_t a = argv[0].as_char();
			wchar_t b = argv[1].as_char();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::tk_boolean:
		{
			bool a = argv[0].as_boolean();
			bool b = argv[1].as_boolean();
			r = (a == b) ? 0 : (a < b) ? -1 : 1;
		}
		break;

		case type_data::tk_array:
		{
			for (unsigned i = 0; i < argv[0].length_as_array(); ++i)
			{
				if (i >= argv[1].length_as_array())
				{
					r = +1;	//"123" > "12"
					break;
				}

				value v[2];
				v[0] = argv[0].index_as_array(i);
				v[1] = argv[1].index_as_array(i);
				r = compare(machine, 2, v).as_real();
				if (r != 0)
					break;
			}
			if (r == 0 && argv[0].length_as_array() < argv[1].length_as_array())
			{
				r = -1;	//"12" < "123"
			}
		}
		break;

		default:
			assert(false);
		}
		return value(machine->get_engine()->get_real_type(), static_cast < long double > (r));
	}
	else
	{
		machine->raise_error("An attempt of comparison between different types was made"); //�^���Ⴄ�l���m���r���悤�Ƃ��܂���
		return value();
	}
}

value predecessor(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 1);
	assert(argv[0].has_data());
	switch (argv[0].get_type()->get_kind())
	{
	case type_data::tk_real:
		return value(argv[0].get_type(), argv[0].as_real() - 1);

	case type_data::tk_char:
	{
		wchar_t c = argv[0].as_char();
		--c;
		return value(argv[0].get_type(), c);
	}
	case type_data::tk_boolean:
		return value(argv[0].get_type(), false);
	default:
		machine->raise_error("(value) predecessor of this type cannot be used"); //���̌^�̒l��predecessor�͎g���܂���
		return value();
	}
}

value successor(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 1);
	assert(argv[0].has_data());
	switch (argv[0].get_type()->get_kind())
	{
	case type_data::tk_real:
		return value(argv[0].get_type(), argv[0].as_real() + 1);

	case type_data::tk_char:
	{
		wchar_t c = argv[0].as_char();
		++c;
		return value(argv[0].get_type(), c);
	}
	case type_data::tk_boolean:
		return value(argv[0].get_type(), true);
	default:
		machine->raise_error("(value) successor of this type cannot be used"); //���̌^�̒l��successor�͎g���܂���
		return value();
	}
}

value true_(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_boolean_type(), true);
}

value false_(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_boolean_type(), false);
}

value not_(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_boolean_type(), !argv[0].as_boolean());
}

value length(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 1);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("Cannot find length of non-array element.");
		return value();
	}

	return value(machine->get_engine()->get_real_type(), static_cast < long double > (argv[0].length_as_array()));
}

value index(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("�z��ȊO��index���g���܂���");
		return value();
	}

	long double index = argv[1].as_real();

	if (index != static_cast < int > (index))
	{
		machine->raise_error("�����_�ȉ�������܂�");
		return value();
	}

	if (index < 0 || index >= argv[0].length_as_array())
	{
		machine->raise_error("�z��̃T�C�Y�𒴂��Ă��܂�");
		return value();
	}

	value const & result = argv[0].index_as_array(index);
	return result;
}

value index_writable(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("�z��ȊO��index!���g���܂���");
		return value();
	}

	long double index = argv[1].as_real();

	if (index != static_cast < int > (index))
	{
		machine->raise_error("�����_�ȉ�������܂�");
		return value();
	}

	if (index < 0 || index >= argv[0].length_as_array())
	{
		machine->raise_error("�z��̃T�C�Y�𒴂��Ă��܂�");
		return value();
	}

	value const & result = argv[0].index_as_array(index);
	result.unique();
	return result;
}

value slice(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 3);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("�z��ȊO��slice���g���܂���");
		return value();
	}

	long double index_1 = argv[1].as_real();

	if (index_1 != static_cast < int > (index_1))
	{
		machine->raise_error("�J�n�ʒu�ɏ����_�ȉ�������܂�");
		return value();
	}

	long double index_2 = argv[2].as_real();

	if (index_2 != static_cast < int > (index_2))
	{
		machine->raise_error("�I�[�ʒu�ɏ����_�ȉ�������܂�");
		return value();
	}

	if (index_1 < 0 || index_1 > index_2 || index_2 > argv[0].length_as_array())
	{
		machine->raise_error("�z��̃T�C�Y�𒴂��Ă��܂�");
		return value();
	}

	value result(argv[0].get_type(), std::wstring());

	for (int i = index_1; i < index_2; ++i)
	{
		result.append(result.get_type(), argv[0].index_as_array(i));
	}

	return result;
}

value erase(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("�z��ȊO��erase���g���܂���");
		return value();
	}

	long double index_1 = argv[1].as_real();
	double length = argv[0].length_as_array();

	if (index_1 != static_cast < int > (index_1))
	{
		machine->raise_error("�폜�ʒu�ɏ����_�ȉ�������܂�");
		return value();
	}

	if (index_1 < 0 || index_1 >= argv[0].length_as_array())
	{
		machine->raise_error("Exeeds the size of the array"); //�z��̃T�C�Y�𒴂��Ă��܂�
		return value();
	}

	value result(argv[0].get_type(), std::wstring());

	for (int i = 0; i < index_1; ++i)
	{
		result.append(result.get_type(), argv[0].index_as_array(i));
	}
	for (int i = index_1 + 1; i < length; ++i)
	{
		result.append(result.get_type(), argv[0].index_as_array(i));
	}
	return result;
}

value append(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);

	if (argv[0].get_type()->get_kind() != type_data::tk_array)
	{
		machine->raise_error("�z��ȊO��append���g���܂���");
		return value();
	}

	if (argv[0].length_as_array() > 0 && argv[0].get_type()->get_element() != argv[1].get_type())
	{
		machine->raise_error("type mismatch"); //�^����v���܂���
		return value();
	}

	value result = argv[0];
	result.append(machine->get_engine()->get_array_type(argv[1].get_type()), argv[1]);
	return result;
}

value concatenate(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);

	value result = argv[0];
	value append = argv[1];

	if (result.get_type()->get_kind() != type_data::tk_array) {
		result = value(machine->get_engine()->get_string_type(),result.as_string());
	}
	if (append.get_type()->get_kind() != type_data::tk_array) {
		append = value(machine->get_engine()->get_string_type(),append.as_string());
	}

	if (result.length_as_array() > 0 && append.length_as_array() > 0 && result.get_type() != append.get_type())
	{
		machine->raise_error("Type mismatch on array concatenation."); //�^����v���܂���
		return value();
	}

	result.concatenate(append);
	return result;
}

value round(script_machine * machine, int argc, value const * argv)
{
	long double r = std::floorl(argv[0].as_real() + 0.5);
	return value(machine->get_engine()->get_real_type(), r);
}

value truncate(script_machine * machine, int argc, value const * argv)
{
	long double r = argv[0].as_real();
	r = (r > 0) ? std::floorl(r) : std::ceill(r);
	return value(machine->get_engine()->get_real_type(), r);
}

value ceil(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), std::ceill(argv[0].as_real()));
}

value floor(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), std::floorl(argv[0].as_real()));
}

value absolute(script_machine * machine, int argc, value const * argv)
{
	long double r = std::fabsl(argv[0].as_real());
	return value(machine->get_engine()->get_real_type(), r);
}

value pi(script_machine * machine, int argc, value const * argv)
{
	return value(machine->get_engine()->get_real_type(), (long double) 3.14159265358979323846);
}

value assert_(script_machine * machine, int argc, value const * argv)
{
	assert(argc == 2);
	if (!argv[0].as_boolean()) {
		machine->raise_error(to_mbcs(argv[1].as_string()));
	}
	return value();
}

function const operations[] =
{
	{ "true", true_, 0 },
	{ "false", false_, 0 },
	{ "pi", pi, 0 },
	{ "length", length, 1 },
	{ "not", not_, 1 },
	{ "negative", negative, 1 },
	{ "predecessor", predecessor, 1 },
	{ "successor", successor, 1 },
	{ "round", round, 1 },
	{ "trunc", truncate, 1 },
	{ "truncate", truncate, 1 },
	{ "ceil", ceil, 1 },
	{ "floor", floor, 1 },
	{ "absolute", absolute, 1 },
	{ "add", add, 2 },
	{ "subtract", subtract, 2 },
	{ "multiply", multiply, 2 },
	{ "divide", divide, 2 },
	{ "remainder", remainder, 2 },
	{ "power", power, 2 },
	{ "index", index, 2 },
	{ "index!", index_writable, 2 },
	{ "slice", slice, 3 },
	{ "erase", erase, 2 },
	{ "append", append, 2 },
	{ "concatenate", concatenate, 2 },
	{ "compare", compare, 2 },
	{ "assert", assert_, 2 }
};


/* parser */

class parser
{
public:
	struct symbol
	{
		int level;
		script_engine::block * sub;
		int variable;
	};

	struct scope : public std::map < std::string, symbol >
	{
		script_engine::block_kind kind;

		scope(script_engine::block_kind the_kind) : kind(the_kind)
		{
		}
	};

	std::vector < scope > frame; //because frame is a vector who's elements inherit hash maps, accessing the contents of the elements would require dereference twice. ex: frame[0]["name"] //(element)
	scanner * lex;
	script_engine * engine;
	bool error;
	std::string error_message;
	int error_line;
	std::map < std::string, script_engine::block * > events;

	parser(script_engine * e, scanner * s, int funcc, function const * funcv);

	virtual ~parser()
	{
	}

	void parse_parentheses(script_engine::block * block);
	void parse_clause(script_engine::block * block);
	void parse_prefix(script_engine::block * block);
	void parse_suffix(script_engine::block * block);
	void parse_product(script_engine::block * block);
	void parse_sum(script_engine::block * block);
	void parse_comparison(script_engine::block * block);
	void parse_logic(script_engine::block * block);
	void parse_expression(script_engine::block * block);
	int parse_arguments(script_engine::block * block);
	void parse_statements(script_engine::block * block);
	void parse_inline_block(script_engine::block * block, script_engine::block_kind kind);
	void parse_block(script_engine::block * block, std::vector < std::string > const * args, bool adding_result);
private:
	void register_function(function const & func);
	symbol * search(std::string const & name);
	symbol * search_result();
	void scan_current_scope(int level, std::vector < std::string > const * args, bool adding_result);
	void write_operation(script_engine::block * block, char const * name, int clauses);

	typedef script_engine::code code;
};

parser::parser(script_engine * e, scanner * s, int funcc, function const * funcv) : engine(e), lex(s), frame(), error(false)
{
	frame.push_back(scope(script_engine::bk_normal));

	for (int i = 0; i < sizeof(operations) / sizeof(function); ++i)
		register_function(operations[i]);

	for (int i = 0; i < funcc; ++i)
		register_function(funcv[i]);

	try
	{
		scan_current_scope(0, NULL, false);
		parse_statements(engine->main_block);
		if (lex->next != tk_end)
			throw parser_error("cannot be interpreted. (did you forget \";\"?"); //���߂ł��Ȃ����̂�����܂�(�u;�v��Y��Ă��܂���)
	}
	catch (parser_error & e)
	{
		error = true;
		error_message = e.what();
		error_line = lex->line;
	}
}

void parser::register_function(function const & func)
{
	symbol s;
	s.level = 0;
	s.sub = engine->new_block(0, script_engine::bk_function);
	s.sub->arguments = func.arguments;
	s.sub->name = func.name;
	s.sub->func = func.func;
	s.variable = -1;
	frame[0][func.name] = s;
}

parser::symbol * parser::search(std::string const & name)
{
	for (int i = frame.size() - 1; i >= 0; --i)
	{
		if (frame[i].find(name) != frame[i].end())
			return &(frame[i][name]);
	}
	return NULL;
}

parser::symbol * parser::search_result()
{
	for (int i = frame.size() - 1; i >= 0; --i)
	{
		if (frame[i].find("result") != frame[i].end())
			return &(frame[i]["result"]);
		if (frame[i].kind == script_engine::bk_sub || frame[i].kind == script_engine::bk_microthread)
			return NULL;
	}
	return NULL;
}

void parser::scan_current_scope(int level, std::vector < std::string > const * args, bool adding_result)
{
	//look ahead to register an identifier //��ǂ݂��Ď��ʎq��o�^����
	scanner lex2(*lex);
	try
	{
		scope * current_frame = &frame[frame.size() - 1];
		int cur = 0;
		int var = 0;

		if (adding_result)
		{
			symbol s;
			s.level = level;
			s.sub = NULL;
			s.variable = var;
			++var;
			(*current_frame)["result"] = s;
		}

		if (args != NULL)
		{
			for (unsigned i = 0; i < args->size(); ++i)
			{
				symbol s;
				s.level = level;
				s.sub = NULL;
				s.variable = var;
				++var;
				(*current_frame)[(*args)[i]] = s;
			}
		}

		while (cur >= 0 && lex2.next != tk_end && lex2.next != tk_invalid)
		{
			switch (lex2.next)
			{
			case tk_open_cur:
				++cur;
				lex2.advance();
				break;
			case tk_close_cur:
				--cur;
				lex2.advance();
				break;
			case tk_at:
			case tk_SUB:
			case tk_FUNCTION:
			case tk_TASK:
			{
				token_kind type = lex2.next;
				lex2.advance();
				if (cur == 0)
				{
					if ((*current_frame).find(lex2.word) != (*current_frame).end())
						throw parser_error("A routine is defined twice"); //�����X�R�[�v�œ����̃��[�`���������錾����Ă��܂�
					script_engine::block_kind kind = (type == tk_SUB || type == tk_at) ? script_engine::bk_sub :
						(type == tk_FUNCTION) ? script_engine::bk_function : script_engine::bk_microthread;

					symbol s;
					s.level = level;
					s.sub = engine->new_block(level + 1, kind);
					s.sub->name = lex2.word;
					s.sub->func = NULL;
					s.variable = -1;
					(*current_frame)[lex2.word] = s;
					lex2.advance();
					if (kind != script_engine::bk_sub && lex2.next == tk_open_par)
					{
						lex2.advance();
						while (lex2.next == tk_word || lex2.next == tk_LET || lex2.next == tk_REAL)
						{
							++(s.sub->arguments);
							if (lex2.next == tk_LET || lex2.next == tk_REAL) lex2.advance();
							if (lex2.next == tk_word) lex2.advance();
							if (lex2.next != tk_comma)
								break;
							lex2.advance();
						}
					}
				}
			}
			break;
			case tk_REAL:
			case tk_LET:
				lex2.advance();
				if (cur == 0)
				{
#ifdef __SCRIPT_H__NO_CHECK_DUPLICATED
					if (lex2.word == "result") {
#endif
						if ((*current_frame).find(lex2.word) != (*current_frame).end())
						{
							throw parser_error("Variables with the same name are declared in the same scope"); //�����X�R�[�v�œ����̕ϐ��������錾����Ă��܂�
						}
#ifdef __SCRIPT_H__NO_CHECK_DUPLICATED
					}
#endif
					symbol s;
					s.level = level;
					s.sub = NULL;
					s.variable = var;
					++var;
					(*current_frame)[lex2.word] = s;
					lex2.advance();
				}
				break;
			default:
				lex2.advance();
			}
		}
	}
	catch (parser_error e)
	{
		lex->line = lex2.line;
		throw;
	}
}

void parser::write_operation(script_engine::block * block, char const * name, int clauses)
{
	symbol * s = search(name);
	assert(s != NULL);
	if (s->sub->arguments != clauses)
		throw parser_error("���Z�q�ɑΉ�����֐����㏑����`����܂����������̐����Ⴂ�܂�");

	block->codes.push_back(script_engine::code(lex->line, script_engine::pc_call_and_push_result, s->sub, clauses));
}

void parser::parse_parentheses(script_engine::block * block)
{
	if (lex->next != tk_open_par)
		throw parser_error("\"(\" is required"); //\"(\"���K�v�ł�  // "(" Is required
	lex->advance();

	parse_expression(block);

	if (lex->next != tk_close_par)
		throw parser_error("\")\" is required"); //"\")\"���K�v�ł�" // ")" Is required
	lex->advance();
}

void parser::parse_clause(script_engine::block * block)
{
	if (lex->next == tk_real)
	{
		block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_real_type(), lex->real_value)));
		lex->advance();
	}
	else if (lex->next == tk_char)
	{
		block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_char_type(), lex->char_value)));
		lex->advance();
	}
	else if (lex->next == tk_string)
	{
		std::wstring str = lex->string_value;
		lex->advance();
		while (lex->next == tk_string || lex->next == tk_char)
		{
			str += (lex->next == tk_string) ? lex->string_value : (std::wstring() + lex->char_value);
			lex->advance();
		}

		block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_string_type(), str)));
	}
	else if (lex->next == tk_word)
	{
		symbol * s = search(lex->word);
		if (s == NULL)
			throw parser_error(lex->word + "�͖���`�̎��ʎq�ł�");

		lex->advance();

		if (s->sub != NULL)
		{
			if (s->sub->kind != script_engine::bk_function)
				throw parser_error("sub��task�͎����ŌĂׂ܂���");

			int argc = parse_arguments(block);

			if (argc != s->sub->arguments)
				throw parser_error(s->sub->name + "�̈����̐����Ⴂ�܂�");

			block->codes.push_back(code(lex->line, script_engine::pc_call_and_push_result, s->sub, argc));
		}
		else
		{
			//�ϐ�
			block->codes.push_back(code(lex->line, script_engine::pc_push_variable, s->level, s->variable));
		}
	}
	else if (lex->next == tk_open_bra)
	{
		lex->advance();
		block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_string_type(), std::wstring())));
		while (lex->next != tk_close_bra)
		{
			parse_expression(block);
			write_operation(block, "append", 2);
			if (lex->next != tk_comma) break;
			lex->advance();
		}
		if (lex->next != tk_close_bra)
			throw parser_error("\"]\"���K�v�ł�");
		lex->advance();
	}
	else if (lex->next == tk_open_abs)
	{
		lex->advance();
		parse_expression(block);
		write_operation(block, "absolute", 1);
		if (lex->next != tk_close_abs)
			throw parser_error("\"|)\"is required"); //\"|)\"���K�v�ł�
		lex->advance();
	}
	else if (lex->next == tk_open_par)
	{
		parse_parentheses(block);
	}
	else
	{
		throw parser_error("There is not a valid expression term"); //���Ƃ��Ė����Ȏ�������܂�
	}
}

void parser::parse_suffix(script_engine::block * block)
{
	parse_clause(block);
	if (lex->next == tk_caret)
	{
		lex->advance();
		parse_suffix(block); //�ċA
		write_operation(block, "power", 2);
	}
	else
	{
		while (lex->next == tk_open_bra)
		{
			lex->advance();
			parse_expression(block);

			if (lex->next == tk_range)
			{
				lex->advance();
				parse_expression(block);
				write_operation(block, "slice", 3);
			}
			else
			{
				write_operation(block, "index", 2);
			}

			if (lex->next != tk_close_bra)
				throw parser_error("\"]\" is required"); //\"]\"���K�v�ł�
			lex->advance();
		}
	}
}

void parser::parse_prefix(script_engine::block * block)
{
	if (lex->next == tk_plus)
	{
		lex->advance();
		parse_prefix(block);	//�ċA
	}
	else if (lex->next == tk_minus)
	{
		lex->advance();
		parse_prefix(block);	//�ċA
		write_operation(block, "negative", 1);
	}
	else if (lex->next == tk_exclamation)
	{
		lex->advance();
		parse_prefix(block);	//�ċA
		write_operation(block, "not", 1);
	}
	else
	{
		parse_suffix(block);
	}
}

void parser::parse_product(script_engine::block * block)
{
	parse_prefix(block);
	while (lex->next == tk_asterisk || lex->next == tk_slash || lex->next == tk_percent)
	{
		char const * name = (lex->next == tk_asterisk) ? "multiply" : (lex->next == tk_slash) ? "divide" : "remainder";
		lex->advance();
		parse_prefix(block);
		write_operation(block, name, 2);
	}
}

void parser::parse_sum(script_engine::block * block)
{
	parse_product(block);
	while (lex->next == tk_tilde || lex->next == tk_plus || lex->next == tk_minus)
	{
		char const * name = (lex->next == tk_tilde) ? "concatenate" : (lex->next == tk_plus) ? "add" : "subtract";
		lex->advance();
		parse_product(block);
		write_operation(block, name, 2);
	}
}

void parser::parse_comparison(script_engine::block * block)
{
	parse_sum(block);
	switch (lex->next)
	{
	case tk_assign:
		throw parser_error("Did you mistake for \"==\"?");  //"\"==\"�ƊԈႦ�Ă܂��񂩁H// did you mistake for "==" ?

	case tk_e: //equals
	case tk_g: //greater-than
	case tk_ge://greater-than equals
	case tk_l: //less than
	case tk_le://less than equals
	case tk_ne://not equal
		token_kind op = lex->next;
		lex->advance();
		parse_sum(block);
		write_operation(block, "compare", 2);
		switch (op)
		{
		case tk_e:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_e));
			break;
		case tk_g:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_g));
			break;
		case tk_ge:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_ge));
			break;
		case tk_l:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_l));
			break;
		case tk_le:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_le));
			break;
		case tk_ne:
			block->codes.push_back(code(lex->line, script_engine::pc_compare_ne));
			break;
		}
		break;
	}
}

void parser::parse_logic(script_engine::block * block)
{
	parse_comparison(block);
	while (lex->next == tk_and_then || lex->next == tk_or_else)
	{
		script_engine::command_kind cmd = (lex->next == tk_and_then) ? script_engine::pc_case_if_not : script_engine::pc_case_if;
		lex->advance();

		block->codes.push_back(code(lex->line, script_engine::pc_dup));
		block->codes.push_back(code(lex->line, script_engine::pc_case_begin));
		block->codes.push_back(code(lex->line, cmd));
		block->codes.push_back(code(lex->line, script_engine::pc_pop));

		parse_comparison(block);

		block->codes.push_back(code(lex->line, script_engine::pc_case_end));
	}
}

void parser::parse_expression(script_engine::block * block)
{
	parse_logic(block);
}

int parser::parse_arguments(script_engine::block * block)
{
	int result = 0;
	if (lex->next == tk_open_par)
	{
		lex->advance();
		while (lex->next != tk_close_par)
		{
			++result;
			parse_expression(block);
			if (lex->next != tk_comma) break;
			lex->advance();
		}
		if (lex->next != tk_close_par)
			throw parser_error("\")\" is required"); //"\")\"���K�v�ł�
		lex->advance();
	}
	return result;
}

void parser::parse_statements(script_engine::block * block)
{
	for (; ; )
	{
		bool need_semicolon = true;

		if (lex->next == tk_word)
		{
			symbol * s = search(lex->word);
			if (s == NULL)
				throw parser_error(lex->word + "�͖���`�̎��ʎq�ł�");
			lex->advance();
			switch (lex->next)
			{
			case tk_assign:
				lex->advance();
				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
				break;

			case tk_open_bra:
				block->codes.push_back(code(lex->line, script_engine::pc_push_variable_writable, s->level, s->variable));
				lex->advance();
				parse_expression(block);
				if (lex->next != tk_close_bra)
					throw parser_error("\"]\" is required"); //\"]\"���K�v�ł�
				lex->advance();
				write_operation(block, "index!", 2);
				if (lex->next != tk_assign)
					throw parser_error("\"=\"���K�v�ł�"); //\"=\"���K�v�ł�
				lex->advance();
				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::pc_assign_writable));
				break;

			case tk_add_assign:
			case tk_subtract_assign:
			case tk_multiply_assign:
			case tk_divide_assign:
			case tk_remainder_assign:
			case tk_power_assign:
			case tk_concat_assign:
			{
				char const * f;
				switch (lex->next)
				{
				case tk_add_assign:
					f = "add";
					break;
				case tk_subtract_assign:
					f = "subtract";
					break;
				case tk_multiply_assign:
					f = "multiply";
					break;
				case tk_divide_assign:
					f = "divide";
					break;
				case tk_remainder_assign:
					f = "remainder";
					break;
				case tk_power_assign:
					f = "power";
					break;
				case tk_concat_assign:
					f = "concatenate";
					break;
				default:
					throw parser_error("Mismatched token definition.");

				}
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::pc_push_variable, s->level, s->variable));

				parse_expression(block);
				write_operation(block, f, 2);

				block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
			}
			break;

			case tk_inc:
			case tk_dec:
			{
				char const * f = (lex->next == tk_inc) ? "successor" : "predecessor";
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::pc_push_variable, s->level, s->variable));
				write_operation(block, f, 1);
				block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
			}
			break;
			default:
				//�֐�, sub�ďo��  //function sub call
				if (s->sub == NULL)
					throw parser_error("�ϐ��͊֐���sub�̂悤�ɂ͌Ăׂ܂���");

				int argc = parse_arguments(block);

				if (argc != s->sub->arguments)
					throw parser_error(s->sub->name + "wrong number of arguments"); //�̈����̐����Ⴂ�܂�-translated

				block->codes.push_back(code(lex->line, script_engine::pc_call, s->sub, argc));
			}
		}
		else if (lex->next == tk_LET || lex->next == tk_REAL)
		{
			lex->advance();

			if (lex->next != tk_word)
				throw parser_error("identifiers are required"); //���ʎq���K�v�ł� -translated

			symbol * s = search(lex->word);
			lex->advance();
			if (lex->next == tk_assign)
			{
				lex->advance();

				parse_expression(block);
				block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
			}
		}
		else if (lex->next == tk_LOCAL)
		{
			lex->advance();
			parse_inline_block(block, script_engine::bk_normal);
			need_semicolon = false;
		}
		else if (lex->next == tk_LOOP)
		{
			lex->advance();
			if (lex->next == tk_open_par)
			{
				parse_parentheses(block);
				int ip = block->codes.length;
				block->codes.push_back(code(lex->line, script_engine::pc_loop_count));
				parse_inline_block(block, script_engine::bk_loop);
				block->codes.push_back(code(lex->line, script_engine::pc_loop_back, ip));
				block->codes.push_back(code(lex->line, script_engine::pc_pop));
			}
			else
			{
				int ip = block->codes.length;
				parse_inline_block(block, script_engine::bk_loop);
				block->codes.push_back(code(lex->line, script_engine::pc_loop_back, ip));
			}
			need_semicolon = false;
		}
		else if (lex->next == tk_TIMES)
		{
			lex->advance();
			parse_parentheses(block);
			int ip = block->codes.length;
			if (lex->next == tk_LOOP)
			{
				lex->advance();
			}
			block->codes.push_back(code(lex->line, script_engine::pc_loop_count));
			parse_inline_block(block, script_engine::bk_loop);
			block->codes.push_back(code(lex->line, script_engine::pc_loop_back, ip));
			block->codes.push_back(code(lex->line, script_engine::pc_pop));
			need_semicolon = false;
		}
		else if (lex->next == tk_WHILE)
		{
			lex->advance();
			int ip = block->codes.length;
			parse_parentheses(block);
			if (lex->next == tk_LOOP)
			{
				lex->advance();
			}
			block->codes.push_back(code(lex->line, script_engine::pc_loop_if));
			parse_inline_block(block, script_engine::bk_loop);
			block->codes.push_back(code(lex->line, script_engine::pc_loop_back, ip));
			need_semicolon = false;
		}
		else if (lex->next == tk_FOR)
		{
			lex->advance();
			bool back = lex->next == tk_REVERSE;

			if (lex->next == tk_REVERSE)
			{
				lex->advance();
			}

			if (lex->next != tk_open_par)
				throw parser_error("\"(\" is required"); //"\"(\"���K�v�ł�" -translated 
			lex->advance();

			if (lex->next == tk_LET || lex->next == tk_REAL)
			{
				lex->advance();
			}

			if (lex->next != tk_word)
				throw parser_error("identifier expected"); //"���ʎq���K�v�ł�" - translated

			std::string s = lex->word;

			lex->advance();

			if (lex->next != tk_IN)
				throw parser_error("must be in"); //"in���K�v�ł�" - translated
			lex->advance();

			if (lex->next == tk_word) {
				// push 0 and array length
				block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_real_type(), 0.0L)));
				parse_expression(block);
				write_operation(block, "length", 1);
			}
			else {
				parse_expression(block);

				if (lex->next != tk_range)
					throw parser_error("\"..\" must be required"); //"\"..\"���K�v�ł�" - translated
				lex->advance();

				parse_expression(block);
			}

			if (lex->next != tk_close_par)
				throw parser_error("\")\" is required"); //"\")\"���K�v�ł�" - translated
			lex->advance();

			if (!back)
			{
				block->codes.push_back(code(lex->line, script_engine::pc_swap));
			}

			int ip = block->codes.length;

			block->codes.push_back(code(lex->line, script_engine::pc_dup2));
			write_operation(block, "compare", 2);

			block->codes.push_back(code(lex->line, back ? script_engine::pc_loop_descent : script_engine::pc_loop_ascent));

			if (back)
			{
				write_operation(block, "predecessor", 1);
			}

			script_engine::block * b = engine->new_block(block->level + 1, script_engine::bk_loop);
			std::vector < std::string > counter;
			counter.push_back(s);
			parse_block(b, &counter, false);
			block->codes.push_back(code(lex->line, script_engine::pc_dup));
			block->codes.push_back(code(lex->line, script_engine::pc_call, b, 1));

			if (!back)
			{
				write_operation(block, "successor", 1);
			}

			block->codes.push_back(code(lex->line, script_engine::pc_loop_back, ip));
			block->codes.push_back(code(lex->line, script_engine::pc_pop));
			block->codes.push_back(code(lex->line, script_engine::pc_pop));

			need_semicolon = false;
		}
		else if (lex->next == tk_IF)
		{
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::pc_case_begin));

			parse_parentheses(block);
			block->codes.push_back(code(lex->line, script_engine::pc_case_if_not));
			parse_inline_block(block, script_engine::bk_normal);
			while (lex->next == tk_ELSE)
			{
				lex->advance();
				block->codes.push_back(code(lex->line, script_engine::pc_case_next));
				if (lex->next == tk_IF)
				{
					lex->advance();
					parse_parentheses(block);
					block->codes.push_back(code(lex->line, script_engine::pc_case_if_not));
					parse_inline_block(block, script_engine::bk_normal);
				}
				else
				{
					parse_inline_block(block, script_engine::bk_normal);
					break;
				}
			}

			block->codes.push_back(code(lex->line, script_engine::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == tk_EVENTS)
		{
			lex->advance();
			parse_parentheses(block);

			if (lex->next != tk_arrow)
				throw parser_error("\"=>\" is needed");
			lex->advance();

			block->codes.push_back(code(lex->line, script_engine::pc_case_begin));
			while (lex->next == tk_ON)
			{
				lex->advance();

				if (lex->next != tk_open_par)
					throw parser_error("\"(\" is needed"); // \"(\"���K�v�ł� - translated
				block->codes.push_back(code(lex->line, script_engine::pc_case_begin));
				do
				{
					lex->advance();

					block->codes.push_back(code(lex->line, script_engine::pc_dup));
					parse_expression(block);
					write_operation(block, "compare", 2);
					block->codes.push_back(code(lex->line, script_engine::pc_compare_e));
					block->codes.push_back(code(lex->line, script_engine::pc_dup));
					block->codes.push_back(code(lex->line, script_engine::pc_case_if));
					block->codes.push_back(code(lex->line, script_engine::pc_pop));

				} while (lex->next == tk_comma);
				block->codes.push_back(code(lex->line, script_engine::pc_push_value, value(engine->get_boolean_type(), false)));
				block->codes.push_back(code(lex->line, script_engine::pc_case_end));
				if (lex->next != tk_close_par)
					throw parser_error("\")\" is needed"); // "\")\"���K�v�ł�" - translated
				lex->advance();

				block->codes.push_back(code(lex->line, script_engine::pc_case_if_not));
				block->codes.push_back(code(lex->line, script_engine::pc_pop));
				parse_inline_block(block, script_engine::bk_normal);
				block->codes.push_back(code(lex->line, script_engine::pc_case_next));
			}

			if (lex->next != tk_ELSE)
				throw parser_error("Event switch must end with \"else\" statement.");

			lex->advance();

			block->codes.push_back(code(lex->line, script_engine::pc_pop));
			parse_inline_block(block, script_engine::bk_normal);

			block->codes.push_back(code(lex->line, script_engine::pc_case_end));
			need_semicolon = false;
		}
		else if (lex->next == tk_BREAK)
		{
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::pc_break_loop));
		}
		else if (lex->next == tk_RETURN)
		{
			lex->advance();
			switch (lex->next)
			{
			case tk_end:
			case tk_invalid:
			case tk_semicolon:
			case tk_close_cur:
				break;
			default:
				parse_expression(block);
				symbol * s = search_result();
				if (s == NULL)
					throw parser_error("������function�̒��ł͂���܂���"); //function is not there?

				block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
			}
			block->codes.push_back(code(lex->line, script_engine::pc_break_routine));
		}
		else if (lex->next == tk_YIELD)
		{
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::pc_yield));
		}
		else if (lex->next == tk_EXIT)
		{
			lex->advance();
			block->codes.push_back(code(lex->line, script_engine::pc_exit));
		}
		else if (lex->next == tk_at || lex->next == tk_SUB || lex->next == tk_FUNCTION || lex->next == tk_TASK)
		{
			bool is_event = lex->next == tk_at;

			lex->advance();
			if (lex->next != tk_word)
				throw parser_error("identifiers required"); // "���ʎq���K�v�ł�" - translated

			symbol * s = search(lex->word);

			if (is_event)
			{
				if (s->sub->level > 1)
					throw parser_error("�C�x���g��[���K�w�ɋL�q���邱�Ƃ͂ł��܂���");
				events[s->sub->name] = s->sub;
			}

			lex->advance();

			std::vector < std::string > args;
			if (s->sub->kind != script_engine::bk_sub)
			{
				if (lex->next == tk_open_par)
				{
					lex->advance();
					while (lex->next == tk_word || lex->next == tk_LET || lex->next == tk_REAL)
					{
						if (lex->next == tk_LET || lex->next == tk_REAL)
						{
							lex->advance();
							if (lex->next != tk_word)
								throw parser_error("���������K�v�ł�"); //arguments are required?
						}
						args.push_back(lex->word);
						lex->advance();
						if (lex->next != tk_comma)
							break;
						lex->advance();
					}
					if (lex->next != tk_close_par)
						throw parser_error("\")\" is required"); // "\")\"���K�v�ł�" - translated
					lex->advance();
				}
			}
			else
			{
				//�݊����̂��ߋ�̊��ʂ�������
				if (lex->next == tk_open_par)
				{
					lex->advance();
					if (lex->next != tk_close_par)
						throw parser_error("\")\"���K�v�c�Ƃ�����\"(\"�v���ł�"); //")" must be "(" ?
					lex->advance();
				}
			}
			parse_block(s->sub, &args, s->sub->kind == script_engine::bk_function);
			need_semicolon = false;
		}

		//�Z�~�R�����������ƌp�����Ȃ� //no continue without semicolon
		if (need_semicolon && lex->next != tk_semicolon)
			break;

		if (lex->next == tk_semicolon)
			lex->advance();
	}
}

void parser::parse_inline_block(script_engine::block * block, script_engine::block_kind kind)
{
	script_engine::block * b = engine->new_block(block->level + 1, kind);
	parse_block(b, NULL, false);
	block->codes.push_back(code(lex->line, script_engine::pc_call, b, 0));
}

void parser::parse_block(script_engine::block * block, std::vector < std::string > const * args, bool adding_result)
{
	if (lex->next != tk_open_cur)
		throw parser_error("\"{\" operator is required");  //"\"{\"���K�v�ł�"
	lex->advance();

	frame.push_back(scope(block->kind));

	scan_current_scope(block->level, args, adding_result);

	if (args != NULL)
	{
		for (unsigned i = 0; i < args->size(); ++i)
		{
			symbol * s = search((*args)[i]);
			block->codes.push_back(code(lex->line, script_engine::pc_assign, s->level, s->variable));
		}
	}
	parse_statements(block);

	frame.pop_back();

	if (lex->next != tk_close_cur)
		throw parser_error("\"}\" operator is required");  //"\"}\"���K�v�ł�"
	lex->advance();
}

/* script_engine */

script_engine::script_engine(script_type_manager * a_type_manager, std::string const & source, int funcc, function const * funcv) :
	type_manager(a_type_manager)
{
	main_block = new_block(0, bk_normal);

	scanner s(source.c_str());
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

/* script_machine */

script_machine::script_machine(script_engine * the_engine)
{
	assert(!the_engine->get_error());
	engine = the_engine;

	first_using_environment = NULL;
	last_using_environment = NULL;
	first_garbage_environment = NULL;
	last_garbage_environment = NULL;

	error = false;
}

script_machine::~script_machine()
{
	while (first_using_environment != NULL)
	{
		environment * object = first_using_environment;
		first_using_environment = first_using_environment->succ;
		delete object;
	}

	while (first_garbage_environment != NULL)
	{
		environment * object = first_garbage_environment;
		first_garbage_environment = first_garbage_environment->succ;
		delete object;
	}
}

script_machine::environment * script_machine::new_environment(environment * parent, script_engine::block * b)
{
	environment * result = NULL;

	if (first_garbage_environment != NULL)
	{
		// garbage collection //���݉��
		result = first_garbage_environment;
		first_garbage_environment = result->succ;
		*((result->succ != NULL) ? &result->succ->pred : &last_garbage_environment) = result->pred;
	}

	if (result == NULL)
	{
		result = new environment;
	}

	result->parent = parent;
	result->ref_count = 1;
	result->sub = b;
	result->ip = 0;
	result->variables.release();
	result->stack.length = 0;
	result->has_result = false;

	// add to the list being used //�g�p�����X�g�ւ̒ǉ�
	result->pred = last_using_environment;
	result->succ = NULL;
	*((result->pred != NULL) ? &result->pred->succ : &first_using_environment) = result;
	last_using_environment = result;

	return result;
}

void script_machine::dispose_environment(environment * object)
{
	assert(object->ref_count == 0);

	//remove from the list in use //�g�p�����X�g����̍폜 
	*((object->pred != NULL) ? &object->pred->succ : &first_using_environment) = object->succ;
	*((object->succ != NULL) ? &object->succ->pred : &last_using_environment) = object->pred;

	//litter added to the list //���݃��X�g�ւ̒ǉ�
	object->pred = last_garbage_environment;
	object->succ = NULL;
	*((object->pred != NULL) ? &object->pred->succ : &first_garbage_environment) = object;
	last_garbage_environment = object;
}

void script_machine::run()
{
	assert(!error);
	if (first_using_environment == NULL)
	{
		error_line = -1;
		threads.clear();
		threads.push_back(new_environment(NULL, engine->main_block));
		current_thread_index = 0;
		finished = false;
		stopped = false;
		resuming = false;

		while (!finished)
		{
			advance();
		}
	}
}

void script_machine::resume()
{
	assert(!error);
	assert(stopped);
	stopped = false;
	finished = false;
	resuming = true;
	while (!finished)
	{
		advance();
	}
}

void script_machine::call(std::string event_name)
{
	assert(!error);
	assert(!stopped);
	if (engine->events.find(event_name) != engine->events.end())
	{
		run();	//�O�̂��� -//just in case

		script_engine::block * event = engine->events[event_name]; //event is not a keyword
		++(threads[0]->ref_count);
		threads[0] = new_environment(threads[0], event);
		finished = false;
		while (!finished)
		{
			advance();
		}
	}
}

bool script_machine::has_event(std::string event_name)
{
	assert(!error);
	return engine->events.find(event_name) != engine->events.end();
}

int script_machine::get_current_line()
{
	environment * current = threads.at[current_thread_index];
	script_engine::code * c = &(current->sub->codes.at[current->ip]);
	return c->line;
}

void script_machine::advance()
{
	assert(current_thread_index < threads.length);
	environment * current = threads.at[current_thread_index];

	if (current->ip >= current->sub->codes.length)
	{
		environment * removing = current;
		current = current->parent;
		if (current == NULL)
		{
			finished = true;
		}
		else
		{
			threads[current_thread_index] = current;

			if (removing->has_result)
			{
				assert(current != NULL && removing->variables.length > 0);
				current->stack.push_back(removing->variables.at[0]);
			}
			else if (removing->sub->kind == script_engine::bk_microthread)
			{
				threads.erase(threads.begin() + current_thread_index);
				yield();
			}

			assert(removing->stack.length == 0);

			for (;;)
			{
				--(removing->ref_count);
				if (removing->ref_count > 0)
					break;
				environment * next = removing->parent;
				dispose_environment(removing);
				removing = next;
			}
		}
	}
	else
	{
		script_engine::code * c = &(current->sub->codes.at[current->ip]);
		error_line = c->line;	//��
		++(current->ip);

		switch (c->command)
		{
		case script_engine::pc_assign:
		{
			stack_t * stack = &current->stack;
			assert(stack->length > 0);
			for (environment * i = current; i != NULL; i = i->parent)
			{
				if (i->sub->level == c->level)
				{
					variables_t * vars = &i->variables;
					if (vars->length <= c->variable)
					{
						while (vars->capacity <= c->variable) vars->expand();
						vars->length = c->variable + 1;
					}
					value * dest = &(vars->at[c->variable]);
					value * src = &stack->at[stack->length - 1];
					if (dest->has_data() && dest->get_type() != src->get_type()
						&& !(dest->get_type()->get_kind() == type_data::tk_array
							&& src->get_type()->get_kind() == type_data::tk_array
							&& (dest->length_as_array() == 0 || src->length_as_array() == 0)))
						raise_error("����ɂ���Č^���ς����悤�Ƃ��܂���"); //type was changed by the assignment
					*dest = *src;
					stack->pop_back();
					break;
				}
			}
		}
		break;

		case script_engine::pc_assign_writable:
		{
			stack_t * stack = &current->stack;
			assert(stack->length >= 2);
			value * dest = &stack->at[stack->length - 2];
			value * src = &stack->at[stack->length - 1];
			if (dest->has_data() && dest->get_type() != src->get_type()
				&& !(dest->get_type()->get_kind() == type_data::tk_array && src->get_type()->get_kind() == type_data::tk_array
					&& (dest->length_as_array() == 0 || src->length_as_array() == 0)))
				raise_error("����ɂ���Č^���ς����悤�Ƃ��܂���");	//type was changed by the assignment
			else
			{
				dest->overwrite(*src);
				stack->length -= 2;
			}
		}
		break;

		case script_engine::pc_break_loop:
		case script_engine::pc_break_routine:
			for (environment * i = current; i != NULL; i = i->parent)
			{
				i->ip = i->sub->codes.length;

				if (c->command == script_engine::pc_break_loop)
				{
					if (i->sub->kind == script_engine::bk_loop)
					{
						environment * e = i->parent;
						assert(e != NULL);
						do
							++(e->ip);
						while (e->sub->codes.at[e->ip - 1].command != script_engine::pc_loop_back);
						break;
					}
				}
				else
				{
					if (i->sub->kind == script_engine::bk_sub || i->sub->kind == script_engine::bk_function
						|| i->sub->kind == script_engine::bk_microthread)
						break;
					else if (i->sub->kind == script_engine::bk_loop)
						i->parent->stack.clear(); /*kludge also a good place*/ /*���׍H�������Ƃ���*/
				}
			}
			break;

		case script_engine::pc_call:
		case script_engine::pc_call_and_push_result:
		{
			stack_t * current_stack = &current->stack;
			assert(current_stack->length >= c->arguments);
			if (c->sub->func != NULL)
			{
				//native calls //�l�C�e�B�u�Ăяo��  
				value * argv = &((*current_stack).at[current_stack->length - c->arguments]);
				value ret;
				ret = c->sub->func(this, c->arguments, argv);
				if (stopped)
				{
					--(current->ip);
				}
				else
				{
					resuming = false;
					//removing the argument jams //�l�܂ꂽ�������폜  
					//for(int i = 0; i < c->arguments; ++i) current_stack->pop_back();
					current_stack->length -= c->arguments;
					//return value //�߂�l 
					if (c->command == script_engine::pc_call_and_push_result)
						current_stack->push_back(ret);
				}
			}
			else if (c->sub->kind == script_engine::bk_microthread)
			{
				//launch microthread //�}�C�N���X���b�h�N��
				++(current->ref_count);
				environment * e = new_environment(current, c->sub);
				++current_thread_index;
				threads.insert(threads.begin() + current_thread_index, e);
				//transhipment of the argument //�����̐ςݑւ�
				for (unsigned i = 0; i < c->arguments; ++i)
				{
					e->stack.push_back(current_stack->at[current_stack->length - 1]);
					current_stack->pop_back();
				}
			}
			else
			{
				//between script invocations //�X�N���v�g�Ԃ̌Ăяo��

				++(current->ref_count);
				environment * e = new_environment(current, c->sub);
				e->has_result = c->command == script_engine::pc_call_and_push_result;
				threads[current_thread_index] = e;
				//transshipment of the argument //�����̐ςݑւ�  
				for (unsigned i = 0; i < c->arguments; ++i)
				{
					e->stack.push_back(current_stack->at[current_stack->length - 1]);
					current_stack->pop_back();
				}
			}
		}
		break;

		case script_engine::pc_case_begin:
		case script_engine::pc_case_end:
			break;

		case script_engine::pc_case_if:
		case script_engine::pc_case_if_not:
		case script_engine::pc_case_next:
		{
			bool exit = true;
			if (c->command != script_engine::pc_case_next)
			{
				stack_t * current_stack = &current->stack;
				exit = current_stack->at[current_stack->length - 1].as_boolean();
				if (c->command == script_engine::pc_case_if_not)
					exit = !exit;
				current_stack->pop_back();
			}
			if (exit)
			{
				int nested = 0;
				for (; ; )
				{
					switch (current->sub->codes.at[current->ip].command)
					{
					case script_engine::pc_case_begin:
						++nested;
						break;
					case script_engine::pc_case_end:
						--nested;
						if (nested < 0)
							goto next;
						break;
					case script_engine::pc_case_next:
						if (nested == 0 && c->command != script_engine::pc_case_next)
						{
							++(current->ip);
							goto next;
						}
						break;
					}
					++(current->ip);
				}
			next:
#ifdef _MSC_VER
				;
#endif
			}
		}
		break;

		case script_engine::pc_compare_e:
		case script_engine::pc_compare_g:
		case script_engine::pc_compare_ge:
		case script_engine::pc_compare_l:
		case script_engine::pc_compare_le:
		case script_engine::pc_compare_ne:
		{
			stack_t * stack = &current->stack;
			value & t = stack->at[stack->length - 1];
			long double r = t.as_real();
			bool b;
			switch (c->command)
			{
			case script_engine::pc_compare_e:
				b = r == 0;
				break;
			case script_engine::pc_compare_g:
				b = r > 0;
				break;
			case script_engine::pc_compare_ge:
				b = r >= 0;
				break;
			case script_engine::pc_compare_l:
				b = r < 0;
				break;
			case script_engine::pc_compare_le:
				b = r <= 0;
				break;
			case script_engine::pc_compare_ne:
				b = r != 0;
				break;
			}
			t.set(engine->get_boolean_type(), b);
		}
		break;

		case script_engine::pc_dup:
		{
			stack_t * stack = &current->stack;
			assert(stack->length > 0);
			stack->push_back(stack->at[stack->length - 1]);
		}
		break;

		case script_engine::pc_dup2:
		{
			stack_t * stack = &current->stack;
			int len = stack->length;
			assert(len >= 2);
			stack->push_back(stack->at[len - 2]);
			stack->push_back(stack->at[len - 1]);
		}
		break;

		case script_engine::pc_loop_back:
			current->ip = c->ip;
			break;

		case script_engine::pc_loop_ascent:
		{
			stack_t * stack = &current->stack;
			value * i = &stack->at[stack->length - 1];
			if (i->as_real() <= 0)
			{
				do
					++(current->ip);
				while (current->sub->codes.at[current->ip - 1].command != script_engine::pc_loop_back);
			}
			current->stack.pop_back();
		}
		break;

		case script_engine::pc_loop_descent:
		{
			stack_t * stack = &current->stack;
			value * i = &stack->at[stack->length - 1];
			if (i->as_real() >= 0)
			{
				do
					++(current->ip);
				while (current->sub->codes.at[current->ip - 1].command != script_engine::pc_loop_back);
			}
			current->stack.pop_back();
		}
		break;

		case script_engine::pc_loop_count:
		{
			stack_t * stack = &current->stack;
			value * i = &stack->at[stack->length - 1];
			assert(i->get_type()->get_kind() == type_data::tk_real);
			long double r = i->as_real();
			if (r > 0)
				i->set(engine->get_real_type(), r - 1);
			else
			{
				do
					++(current->ip);
				while (current->sub->codes.at[current->ip - 1].command != script_engine::pc_loop_back);
			}
		}
		break;

		case script_engine::pc_loop_if:
		{
			stack_t * stack = &current->stack;
			bool c = stack->at[stack->length - 1].as_boolean();
			current->stack.pop_back();
			if (!c)
			{
				do
					++(current->ip);
				while (current->sub->codes.at[current->ip - 1].command != script_engine::pc_loop_back);
			}
		}
		break;

		case script_engine::pc_pop:
			assert(current->stack.length > 0);
			current->stack.pop_back();
			break;

		case script_engine::pc_push_value:
			current->stack.push_back(c->data);
			break;

		case script_engine::pc_push_variable:
		case script_engine::pc_push_variable_writable:
			for (environment * i = current; i != NULL; i = i->parent)
			{
				if (i->sub->level == c->level)
				{
					variables_t * vars = &i->variables;
					if (vars->length <= c->variable || !((*vars).at[c->variable].has_data()))
						raise_error("����������Ă��Ȃ��ϐ����g�����Ƃ��܂���"); //we do not use a variable subsitution at once?
					else
					{
						value * var = &(*vars).at[c->variable];
						if (c->command == script_engine::pc_push_variable_writable)
							var->unique();
						current->stack.push_back(*var);
					}
					break;
				}
			}
			break;

		case script_engine::pc_swap:
		{
			int len = current->stack.length;
			assert(len >= 2);
			value t = current->stack[len - 1];
			current->stack[len - 1] = current->stack[len - 2];
			current->stack[len - 2] = t;
		}
		break;

		case script_engine::pc_yield:
			yield();
			break;

		case script_engine::pc_exit:
			stop();
			break;

		default:
			assert(false);
		}
	}
}