#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
void init_regex();
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

static int cmd_q(char *args) {
	return -1;
}

static int cmd_si(char *args){
	int number;
	if(args == NULL){
		cpu_exec(1);
		return 0;
	}
	sscanf(args,"%d",&number);
	if(number == 0)
		cpu_exec(1);
	else
		cpu_exec(number);
	return 0;	
}
static int cmd_info(char *args){
	sscanf(args,"%s",args);
	if(strcmp(args,"r") == 0){
		for (int i = R_EAX; i <= R_EDI; i ++) {
			printf("%s %#010x\n",regsl[i],reg_l(i));
		}
		printf("epi %#08x\n",cpu.eip);

		for (int i = R_AX; i <= R_DI; i ++) {
			printf("%s  %#08x\n",regsw[i],reg_w(i));
		}

		for (int i = R_AX; i <= R_DI; i ++) {
			printf("%s  %#08x\n",regsb[i],reg_b(i));
		}
	}
	else if(strcmp(args,"w") == 0){
		print_wp();
	}
	else {
		printf("invalid option-- %s\n",args);
		printf("Try 'help' for more information\n");
	}
	return 0;
}


static int cmd_x(char *args){
	int num = atoi(strtok(args," "));
	char * add_temp = strtok(NULL," ");
	vaddr_t add;
	sscanf(add_temp,"0x%x",&add);
	//printf("The initial address: %#08x\n",add);
	for(int i = 0;i<num;i++){
		printf("%#x  %#010x  ",add,vaddr_read(add,8));
		printf("0x");
		for(int j=0;j<4;j++){
			printf("%x",pmem[add+j]);
		}
		printf("\n");
		add += 4;
	}
	return 0;
}

static int cmd_p(char *args){
	if(args == NULL){
		cpu_exec(1);
		return 0;
	}
	init_regex();
	bool state = true;
	unsigned int result = expr(args,&state);
	if(!state){
		printf("Error!check what you input!\n");
	}
	else{
		printf("the result is : %d\t(0x%x)\n",result,result);
	}
	return 0;
}
static int cmd_w(char *args){
	if(args == NULL){
		printf("Please input an expression!\n");
		return 0;
	}
	new_wp(args);
	return 0;
}

static int cmd_d(char *args){
	if(args == NULL){
		printf("Please input a number!\n");
		return 0;
	}
	int num;
	if( sscanf(args, "%d", &num) == 0 ){
		printf("Not a number!\n");
		return 0;
	}
	if( num < 0 || num >= 32 ){
		printf("n must be smaller than 32 and no smaller than 0\n");
		return 0;
	}
	free_wp(num);
	return 0;
}

static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{"si","Single step",cmd_si},
	{"info","Print registers",cmd_info},
	{"x","Scan memory",cmd_x},
	{"p","Please input the value of the expression.",cmd_p},
	{"w","Please input the address.",cmd_w},
	{"d","Please input what you want to delete.",cmd_d},
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

void ui_mainloop(int is_batch_mode) {
	if (is_batch_mode) {
		cmd_c(NULL);
		return;
	}

	while (1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if (cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if (args >= str_end) {
			args = NULL;
		}

#ifdef HAS_IOE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for (i = 0; i < NR_CMD; i ++) {
			if (strcmp(cmd, cmd_table[i].name) == 0) {
				if (cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
