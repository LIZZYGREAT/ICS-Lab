#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
uint32_t expr(char *e, bool *success);
/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
} 
/*notice here: cpu_exec(-1),-1 is tranformed to 2^64-1,thus making the 
program continue till the ending or the breaking point*/

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
static int cmd_info(char *args);
static int cmd_si(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "help [CMD]        - Display information about all supported commands or a specific command.", cmd_help },
  { "c",    "c                 - Continue the execution of the program.", cmd_c },
  { "q",    "q                 - Exit NEMU smoothly.", cmd_q },
  { "si",   "si [N]            - Step instruction: Execute [N] instructions step by step. Default N=1.", cmd_si },
  { "info", "info [r|w]        - Print program status: 'r' for registers, 'w' for watchpoints.", cmd_info },
  { "p",    "p EXPR            - Print the value of the expression EXPR.", cmd_p },
  { "x",    "x N EXPR          - Examine memory: Print [N] consecutive 32-bit values starting from the address obtained by evaluating EXPR.", cmd_x },
  { "w",    "w EXPR            - Set a watchpoint for the expression EXPR. Execution will pause when its value changes.", cmd_w },
  { "d",    "d N               - Delete the watchpoint with the sequence number [N].", cmd_d },
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}
static int cmd_info(char *args) {
  /*check if cmd is empty*/
  if (args == NULL) {
    printf("Missing argument for 'info' command.\n");
    return 0;
  }

  /*check if cmd == r and output the info about registers*/
  if (strcmp(args, "r") == 0) {
    printf("%-8s 0x%08x %10d\n", "eax", cpu.eax, cpu.eax);
    printf("%-8s 0x%08x %10d\n", "ecx", cpu.ecx, cpu.ecx);
    printf("%-8s 0x%08x %10d\n", "edx", cpu.edx, cpu.edx);
    printf("%-8s 0x%08x %10d\n", "ebx", cpu.ebx, cpu.ebx);
    printf("%-8s 0x%08x %10d\n", "esp", cpu.esp, cpu.esp);
    printf("%-8s 0x%08x %10d\n", "ebp", cpu.ebp, cpu.ebp);
    printf("%-8s 0x%08x %10d\n", "esi", cpu.esi, cpu.esi);
    printf("----------------------------------\n");
    printf("%-8s 0x%08x %10d\n", "edi", cpu.edi, cpu.edi);
    printf("%-8s 0x%08x %10d\n", "eip", cpu.eip, cpu.eip);
  }

  /*check if cmd == w and output the info about Watchpoint*/
  else if (strcmp(args, "w") == 0) {
    print_wp(); 
  }

  else {
    printf("Unknown argument '%s' for 'info' command.\n", args);
  }

  return 0;
}


static int cmd_si(char *args) {
  uint64_t steps = 1; /*initalize step*/

  if (args != NULL) {
    if (args[0] == '-') {
      printf("Error: Step count cannot be negative.\n");
      return 0; 
    }/* check args directly but not transform into uint64_t*/

    char *endptr; /* the end of a char*   */
    steps = strtoull(args, &endptr, 10);
	/*more strict but do not check if the num is negative */

    if (*endptr != '\0') {
      printf("Error: Invalid step count '%s'. Please enter a positive integer.\n", args);
      return 0; 
    }
    
    if (steps == 0) {
      return 0;
    }
  }

  cpu_exec(steps);
  return 0;
}
static int cmd_x(char *args) {
    if (args == NULL) {
        printf("Usage: x N EXPR\n");
        return 0;
    }

    char *n_str = strtok(args, " ");
    if (n_str == NULL) {
        printf("Invalid format. Usage: x N EXPR\n");
        return 0;
    }
    
    int n;
    if (sscanf(n_str, "%d", &n) != 1 || n <= 0) {
        printf("Invalid arguments: N must be a positive integer.\n");
        return 0;
    }

    char *expr_str = n_str + strlen(n_str) + 1;
    
    while (*expr_str == ' ') {
        expr_str++;
    }
    
    if (*expr_str == '\0') {
        printf("Invalid format. Missing EXPR. Usage: x N EXPR\n");
        return 0;
    }

    bool success = true;
    uint32_t base_addr = expr(expr_str, &success);
    if (!success) {
        printf("Error: Invalid expression '%s'\n", expr_str);
        return 0;
    }

    for (int i = 0; i < n; i++) {
        vaddr_t current_addr = base_addr + i * 4;

        if (current_addr + 3 >= 0x8000000) {
            printf("\n");
            printf("Error: Cannot access memory at address 0x%08x (Out of bounds).\n", current_addr);
            return 0;
        }

        if (i % 4 == 0) {
            if (i != 0) {
                printf("\n");
            }
            printf("0x%08x: ", current_addr);
        }

        uint32_t data = vaddr_read(current_addr, 4);
        printf("0x%08x ", data);
    }

    printf("\n");
    return 0;
}

static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success;
  uint32_t res = expr(args, &success);

  if (success) {
    printf("%u (0x%x)\n", res, res);
  } else {
    printf("Invalid expression: %s\n", args);
  }
  return 0;
}
static int cmd_w(char *args) {
    if (args == NULL) {
        printf("Usage: w EXPR\n");
        return 0;
    }

    bool success = true;
    uint32_t init_val = expr(args, &success);
    if (!success) {
        printf("Error: Invalid expression '%s'. Watchpoint not created.\n", args);
        return 0;
    }

    WP *wp = new_wp();

    strncpy(wp->expr, args, sizeof(wp->expr) - 1);
    wp->expr[sizeof(wp->expr) - 1] = '\0';
    wp->old_val = init_val;

    printf("Watchpoint %d created: %s\n", wp->NO, wp->expr);
    printf("Initial value: %u (0x%08x)\n", init_val, init_val);

    return 0;
}
static int cmd_d(char *args) {
    if (args == NULL) {
        printf("Usage: d N (N is the watchpoint sequence number)\n");
        return 0;
    }

    int no;
    if (sscanf(args, "%d", &no) != 1) {
        printf("Invalid format. Usage: d N (N must be an integer)\n");
        return 0;
    }

    bool success = delete_wp_by_no(no);
    if (success) {
        printf("Watchpoint %d deleted successfully.\n", no);
    } else {
        printf("Error: Watchpoint %d does not exist or is not active.\n", no);
    }

    return 0;
}
void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");/* use strok to split the char*     */
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
	/* strlen return the len excluding the "\0" */
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) { 
		/*NR_CMD means how many cmd provided*/
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
