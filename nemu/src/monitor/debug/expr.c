#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NEQ, TK_AND,
  TK_NUM, TK_HEX, TK_REG,
  TK_DEREF, TK_NEG
};
uint32_t isa_reg_str2val(const char *s, bool *success);
static struct rule {
  char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},       // not equal
  {"&&", TK_AND},       // logical and
  {"0x[0-9a-fA-F]+", TK_HEX}, // hex number
  {"[0-9]+", TK_NUM},   // decimal number
  {"\\$[a-zA-Z]+", TK_REG},   // register
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

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

Token tokens[32];
int nr_token;

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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          default:
            if (substr_len >= 32) {
               // KISS: 直接拦截缓冲区溢出 [cite: 3410]
               assert(0);
            }
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;
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

bool check_parentheses(int p, int q) {
  if (tokens[p].type != '(' || tokens[q].type != ')') return false;
  
  int count = 0;
  for (int i = p; i < q; i++) {
    if (tokens[i].type == '(') count++;
    else if (tokens[i].type == ')') count--;
    if (count == 0) return false; 
  }
  return count == 1;
}

int get_priority(int type) {
  switch (type) {
    case TK_AND: return 1;
    case TK_EQ:
    case TK_NEQ: return 2;
    case '+':
    case '-': return 3;
    case '*':
    case '/': return 4;
    case TK_DEREF:
    case TK_NEG: return 5;
    default: return 100;
  }
}

int find_dominant_op(int p, int q) {
  int op = -1;
  int min_priority = 100;
  int count = 0;

  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') count++;
    else if (tokens[i].type == ')') count--;
    else if (count == 0) {
      int pr = get_priority(tokens[i].type);
      if (pr <= min_priority) {
        min_priority = pr;
        op = i;
      }
    }
  }
  return op;
}

uint32_t eval(int p, int q) {
  if (p > q) {
    assert(0); // Bad expression
  }
  else if (p == q) {
    uint32_t val = 0;
    if (tokens[p].type == TK_NUM) {
      sscanf(tokens[p].str, "%u", &val);
    }
    else if (tokens[p].type == TK_HEX) {
      sscanf(tokens[p].str, "%x", &val);
    }
    else if (tokens[p].type == TK_REG) {
      bool success;
      val = isa_reg_str2val(tokens[p].str + 1, &success);
      if (!success) assert(0);
    }
    return val;
  }
  else if (check_parentheses(p, q) == true) {
    return eval(p + 1, q - 1);
  }
  else {
    int op_idx = find_dominant_op(p, q);
    int op_type = tokens[op_idx].type;

    if (op_idx == p) {
      uint32_t val = eval(p + 1, q);
      switch (op_type) {
        case TK_DEREF: return vaddr_read(val, 4);
        case TK_NEG: return -val;
        default: assert(0);
      }
    }

    uint32_t val1 = eval(p, op_idx - 1);
    uint32_t val2 = eval(op_idx + 1, q);

    switch (op_type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': 
        if (val2 == 0) {
          printf("Error: Division by zero.\n");
          assert(0);
        }
        return val1 / val2;
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      default: assert(0);
    }
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == '*' && (i == 0 || (tokens[i-1].type != TK_NUM && tokens[i-1].type != TK_HEX && tokens[i-1].type != TK_REG && tokens[i-1].type != ')'))) {
      tokens[i].type = TK_DEREF;
    }
    if (tokens[i].type == '-' && (i == 0 || (tokens[i-1].type != TK_NUM && tokens[i-1].type != TK_HEX && tokens[i-1].type != TK_REG && tokens[i-1].type != ')'))) {
      tokens[i].type = TK_NEG;
    }
  }

  *success = true;
  return eval(0, nr_token - 1);
}
