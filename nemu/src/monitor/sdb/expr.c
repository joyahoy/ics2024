/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
	{"[0-9]+", TK_NUM},     // number
  {"\\+", '+'},         // plus
	{"\\-", '-'},					 // sub
	{"\\*", '*'},          // mult
	{"\\/", '/'},          // DIVISION
	{"\\(", '('},         // lparen
	{"\\)", ')'},         // rparen
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
					case '+':
					case '-':
					case '*':
					case '/':
					case TK_EQ:
					case ')':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					//实现可以省略乘号,在左括号左边的数字
					case '(':
						if(nr_token != 0 && tokens[nr_token-1].type == TK_NUM) tokens[nr_token++].type = '*';
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					case TK_NUM:
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len] = '\n';
						nr_token++;
						break;
					//过滤空格
					case TK_NOTYPE: break;
          default:
					 	panic("Bad match token_type"); 
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


bool check_parentheses(int p,int q){
	if(tokens[p].type != '(' || tokens[q].type != ')') return false;
	//用数组模拟stack
	char stk[1024];	
	int len = q-1;
	int i = p+1;	
	//ptr 栈顶
	int ptr = 0;
	while(i<len){
		if( tokens[i].type == '(' ) stk[ptr++] = tokens[i].type;
		else if( tokens[i].type == ')' ){
			if(ptr != 0 && stk[ptr-1] == '(' ) ptr--;
			else return false;
		}
		i++;
	}
	return true;	
}

word_t eval(int p,int q){
	if(p>q){
		panic("p>q");
	}
	else if(p == q){
		//This is a number
		word_t val = 0;
		sscanf(tokens[p].str,"%d",&val);
		return val;	
	}
	else if(check_parentheses(p, q) == true){
		return eval(p+1,q-1);
	}
	else{
		// 找op，从右往左，如果遇到括号，则忽略括号里面的东西，
		int add_sub = -1;
		int mul_div = -1;
		int cnt_rparen = 0;
		for(int i=q;i>=p;i--){
			if(tokens[i].type == ')') cnt_rparen++;
			else if(tokens[i].type == '(') cnt_rparen--;
			if(cnt_rparen == 0){
				if(tokens[i].type == '+' || tokens[i].type == '-') {add_sub = i;break;}
				if(tokens[i].type == '*' || tokens[i].type == '/') {mul_div = i;break;}
			}		
		}	
		int op = add_sub==-1 ? mul_div : add_sub; 
		word_t val1 = eval(p,op-1);
		word_t val2 = eval(op+1,q);
		switch(tokens[op].type){
			case '+': return val1+val2;break;
			case '-': return val1-val2;break;
			case '*': return val1*val2;break;
			case '/': return val1/val2;break;
			default: assert(0);
		}
	}
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
	word_t res = eval(0,nr_token-1);
	return res;
}
