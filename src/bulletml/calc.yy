/* ��ֵ�ˡ���� -- calc */

%{
typedef double NumType;

#define YYSTYPE double
#define YYERROR_VERBOSE

#include <cmath>
#include <cctype>

#include <vector>
#include <sstream>

#include "calc.h"
#include "formula.h"
#include "formula-variables.h"

int yyerror(char* s);
int yylex();

const char* yyinStr;

typedef Formula<NumType> CalcFormula;
typedef Number<NumType> CalcNumber;
typedef Random<NumType> CalcRandom;
typedef Rank<NumType> CalcRank;
typedef Param<NumType> CalcParam;
typedef Operator CalcOperator;

namespace {
	CalcFormula* formula;
	std::vector<CalcFormula*> formulas;

	CalcFormula* f(double d) { return formulas[(int)d]; }

	int paramId;
}

%}

/* BISON��� */
%token NUM RAND RANK PARAM
%left '-' '+'
%left '*' '/'
%left NEG     /* negation--ñ��ޥ��ʥ� */
%right '^'    /* �٤���ؿ�        */

/* ʸˡ��§��³�� */
%%
input:    /* ��ʸ���� */
        | input line
;

line:     '\n'
| exp '\n'  { formula = f($1); return 0; }
;

exp:    NUM {
	        $$ = formulas.size();
	        formulas.push_back(new CalcFormula(new CalcNumber($1)));
        }
        | RAND {
	        $$ = formulas.size();
            formulas.push_back(new CalcFormula(new CalcRandom()));
        }
        | RANK {
			$$ = formulas.size();
			formulas.push_back(new CalcFormula(new CalcRank()));
		}
        | PARAM {
			$$ = formulas.size();
			formulas.push_back(new CalcFormula(new CalcParam(paramId)));
		}
        | exp '+' exp {
		    $$ = formulas.size();
			formulas.push_back(new CalcFormula(f($1), op_add, f($3)));
		}
        | exp '-' exp {
		    $$ = formulas.size();
			formulas.push_back(new CalcFormula(f($1), op_sub, f($3)));
		}
        | exp '*' exp {
		    $$ = formulas.size();
			formulas.push_back(new CalcFormula(f($1), op_mul, f($3)));
		}
        | exp '/' exp {
		    $$ = formulas.size();
			formulas.push_back(new CalcFormula(f($1), op_div, f($3)));
		}
        | '-' exp  %prec NEG {
		    $$ = $2;
			f($2)->setHeadSub();
		}
        | '(' exp ')' {
		    $$ = $2;
		}
;
%%

/**
 * ������ϴ�ϡ����ͤ��ɤ�С�double�����ͤ򥹥��å����Ѥ��
 * �ȡ������NUM�פ��֤������Ͱʳ����ɤ�С�����ʸ���Υ������������֤���
 * ����ȥ��֤��ɤ����Ф���롣�ե����뤬������0���֤���
 */

#include <ctype.h>
#include <stdio.h>

int yylex ()
{
	int c;

	/* ��������ɤ����Ф�  */
	while ((c = *(yyinStr++)) == ' ' || c == '\t')
		;
	/* ���ͤ��������   */
	if (c == '.' || isdigit (c))
    {
		yyinStr--;
		sscanf (yyinStr, "%lf", &yylval);
		while ((c = *(++yyinStr)) == '.' || isdigit(c)) {}
		return NUM;
    }

	// �ѿ���������� */
	if (c == '$') {
		if (strncmp(yyinStr, "rand", 4) == 0) {
			yyinStr += 4;
			return RAND;
		}
		else if (strncmp(yyinStr, "rank", 4) == 0) {
		    yyinStr += 4;
			return RANK;
        }
		else {
			std::istringstream iss(std::string(yyinStr).substr(0, 1));
			iss >> paramId;
			yyinStr++;
			return PARAM;
		}
	}

	/* �ե�����ν������������  */
	if (c == '\0')
		return 0;
	/* 1ʸ�����֤� */
	return c;
}

int yyerror(char* s) {
	printf("yyerror: %s\n", s);
	return 0;


}