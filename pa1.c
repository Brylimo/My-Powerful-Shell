/**********************************************************************
 * Copyright (c) 2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "types.h"
#include "list_head.h"
#include "parser.h"

// 헤더파일 추가
#include <sys/wait.h>
#include <ctype.h>

LIST_HEAD(history);

static int __process_command(char* command);

struct entry {
    struct list_head list;
    char* command;
};

/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
static int run_command(int nr_tokens, char *tokens[])
{
	pid_t pid;
	char** new_tokens;
	int ssize = nr_tokens; // splitted size
	int store = nr_tokens; // available store size
	int init_value = 0; // initial value
	int status;
	int old_stdin = dup(0); //remembers old stdin
	int flag = -1;

	if (strcmp(tokens[0], "exit") == 0) return 0;
	
	if (strcmp(tokens[0], "cd") == 0) {
	    int res;
	    char* home;

	    if (nr_tokens == 1 || strcmp(tokens[1], "~") == 0) {
		home = getenv("HOME");
		res = chdir(home);	
	    } else {
		res = chdir(tokens[1]);
	    }

	    if (res == -1) {
		fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		return -EINVAL;
	    }
	    return 1;
	}

	if (strcmp(tokens[0], "history") == 0) {
	    struct entry* temp;
	    int i = 0;
	    list_for_each_entry(temp, &history, list) {
	    	fprintf(stderr, "%2d: %s", i++, temp->command);
	    }
	    return 1;    
	}

	if (strcmp(tokens[0], "!") == 0 && nr_tokens == 2) {
	    struct entry* temp;
	    char* store1 = (char*)malloc(sizeof(char) * MAX_COMMAND_LEN);
	    int val = atoi(tokens[1]);
	    int i = 0;
	    
	    list_for_each_entry(temp, &history, list) {
		if (val == i++) {
		    strcpy(store1, temp->command);
		    break;
		}
	    }
	    __process_command(store1);

	    return 1;   
	}

	for (int i = 0; i < nr_tokens; i++) {
	    if (strcmp(tokens[i], "|") == 0) {
		int fd[2];
		pid_t cpid;
		
		if (pipe(fd) == -1) {
		    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		    return -EINVAL;
		}

		cpid = fork();
		if (cpid < 0) {
		    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		    return -EINVAL;
		} else if (cpid == 0) {
		    dup2(fd[1], 1);
		    ssize = i;
		    store = i;
		    close(fd[1]);
		    close(fd[0]);

		    char** temp = (char**)malloc(sizeof(char*) * (store + 1));
		    for (int j = 0; j < store; j++, init_value++) {
			temp[j] = tokens[init_value];
		    }
		    temp[store] = NULL;
		    if (execvp(temp[0], temp) == -1) {
			exit(1);
		    }
		} else {
		    dup2(fd[0], 0);
		    wait(NULL);
		    close(fd[0]);
		    close(fd[1]);
		    init_value = i + 1;
		    store = ssize - i - 1;
		    flag = 0;
		}
	    }
	}

	new_tokens = (char**)malloc(sizeof(char*) * (store + 1));

	for (int i = 0; i < store; i++, init_value++) {
	    new_tokens[i] = tokens[init_value];
	}
	new_tokens[store] = NULL;
	
	pid = fork();
	if (pid < 0) {
	    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	    return -EINVAL;
	}
	else if (pid == 0) {
	    if (execvp(new_tokens[0], new_tokens) == -1) {
		exit(1);
	    }
	} else {
	  wait(&status);
	  if (flag == 0) {
	      dup2(old_stdin, 0);
	      close(old_stdin);
	  }
	  if (status == 0) return 1;
	  else {
	    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	    return -EINVAL;
	  }  
	}

}


/***********************************************************************
 * struct list_head history
 *
 * DESCRIPTION
 *   Use this list_head to store unlimited command history.
 */
// LIST_HEAD(history);


/***********************************************************************
 * append_history()
 *
 * DESCRIPTION
 *   Append @command into the history. The appended command can be later
 *   recalled with "!" built-in command
 */
static void append_history(char * const command)
{
    if (isspace(*command)) return;

    struct entry* newNode = (struct entry*)malloc(sizeof(struct entry));

    char* temp = (char*)malloc(sizeof(char)*MAX_COMMAND_LEN);
    strcpy(temp, command);
    newNode->command = temp;
    INIT_LIST_HEAD(&newNode->list);
    list_add_tail(&newNode->list, &history);    
}

/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
static int initialize(int argc, char * const argv[])
{
	return 0;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
static void finalize(int argc, char * const argv[])
{

}


/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
static int __process_command(char * command)
{
	char *tokens[MAX_NR_TOKENS] = { NULL };
	int nr_tokens = 0;

	if (parse_command(command, &nr_tokens, tokens) == 0)
		return 1;

	return run_command(nr_tokens, tokens);
}

static bool __verbose = true;
static const char *__color_start = "[0;31;40m";
static const char *__color_end = "[0m";

static void __print_prompt(void)
{
	char *prompt = "$";
	if (!__verbose) return;

	fprintf(stderr, "%s%s%s ", __color_start, prompt, __color_end);
}

/***********************************************************************
 * main() of this program.
 */
int main(int argc, char * const argv[])
{
	char command[MAX_COMMAND_LEN] = { '\0' };
	int ret = 0;
	int opt;

	while ((opt = getopt(argc, argv, "qm")) != -1) {
		switch (opt) {
		case 'q':
			__verbose = false;
			break;
		case 'm':
			__color_start = __color_end = "\0";
			break;
		}
	}

	if ((ret = initialize(argc, argv))) return EXIT_FAILURE;

	/**
	 * Make stdin unbuffered to prevent ghost (buffered) inputs during
	 * abnormal exit after fork()
	 */
	setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		__print_prompt();

		if (!fgets(command, sizeof(command), stdin)) break;

		append_history(command);
		ret = __process_command(command);

		if (!ret) break;
	}

	finalize(argc, argv);

	return EXIT_SUCCESS;
}
