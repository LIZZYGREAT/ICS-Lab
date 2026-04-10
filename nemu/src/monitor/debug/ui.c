#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

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


static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "info","Print program status (e.g. info r for registers, info w for watchpoints)",cmd_info},
  { "si", "Step one instruction exactly, or N instructions if specified", cmd_si },
  { "x", "Scan memory: x N EXPR. Output N consecutive 4-byte blocks starting from physical address EXPR.", cmd_x },
  /* TODO: Add more commands */

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
    printf("%-8s 0x%08x %10d\n", "edi", cpu.edi, cpu.edi);
    printf("%-8s 0x%08x %10d\n", "eip", cpu.eip, cpu.eip);
  }

/*check if cmd == w and output the info about Watchpoint*/
  else if (strcmp(args, "w") == 0) {
    printf("Watchpoint information is not implemented yet.\n");
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

    int n;
    vaddr_t base_addr;

    if (sscanf(args, "%d %x", &n, &base_addr) != 2) {
        printf("Invalid format. Usage: x N EXPR (e.g., x 10 0x100000)\n");
        return 0;
    }

    if (n <= 0) {
        printf("Invalid arguments: N must be a positive integer.\n");
        return 0;
    }

    for (int i = 0; i < n; i++) {
        vaddr_t current_addr = base_addr + i * 4;

        if (current_addr + 3 >= 0x8000000) {
            if (i % 4 != 0) {
                printf("\n");
            }
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
