/* 
 *
 * Copyright 2008 Javier Merino <cibervicho@gmail.com>
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#define icanexec(x) !(access((x), X_OK))

static char def_pager[] = "/usr/bin/pager";
static char pager_fallback[] = "more";
static char *user_specified_pager;

static void launch_pager(void) {
	char *pager;

	if (user_specified_pager != NULL) {
		pager = user_specified_pager;
	} else {
		pager = getenv("PAGER");
		if ((pager == NULL) || !icanexec(pager)) {
			if (icanexec(def_pager)) 
				pager = def_pager;
			else
				pager = pager_fallback;
		}
	}

	execl("/bin/sh", "/bin/sh", "-c", pager, NULL);
	perror(pager);
	exit(EXIT_FAILURE);
}

static void execvp_noret(char **argv) {
	execvp(*argv, argv);
	perror(*argv);
	exit(EXIT_FAILURE);
}

static void safe_dup2(int oldfd, int newfd) {
	if (close(newfd)) {
		perror("close");
		exit(EXIT_FAILURE);
	}
	if (dup2(oldfd, newfd) == -1) {
		perror("dup2");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv) {
	char **argv_exec, **additional_argv;
	int additional_argc;
	char c;
	
	additional_argv = NULL;
	additional_argc = 0;
	user_specified_pager = NULL;
	opterr = 0;
	while ((c = getopt(argc, argv, "+a:p:")) != -1) {
		switch(c) {
			case 'a':
				additional_argc++;
				additional_argv = (char **)realloc(additional_argv, additional_argc*sizeof(char *));
				additional_argv[additional_argc-1] = optarg;
				break;
			case 'p':
 				user_specified_pager = optarg;
				break;
			default:
				break;
		}
	}
	argv_exec = &argv[optind];

	if (isatty(1)) {
		if (*argv_exec) {
			/* Run the command through a pipe */
			pid_t child_pid;
			int fds[2];

			if (additional_argc > 0) {
				/* Add the additional arguments specified in the command line*/
				int i,j;

				argv_exec = (char **)malloc((argc-optind+additional_argc+1)*sizeof(char *));
				for (i=0; i<(argc-optind); i++)
					argv_exec[i] = argv[i+optind];
				for (j=0; j<additional_argc; i++, j++)
					argv_exec[i] = additional_argv[j];
				argv_exec[i] = NULL;
			}

			if (pipe(fds) == -1) {
				perror("pipe");
				exit(EXIT_FAILURE);
			}

			child_pid = fork();
			if (child_pid == -1) {
				perror("fork");
				exit(EXIT_FAILURE);
			} else if (!child_pid) {
				/* The child is the subcommand, its stdout and stderr are captured */
				close(fds[0]);
				safe_dup2(fds[1], 1);
				if (isatty(2))
                                	safe_dup2(fds[1], 2);

				execvp_noret(argv_exec);
			}

			/* The parent runs the pager */
			close(fds[1]);
			safe_dup2(fds[0], 0);
		}
		launch_pager();

	} else {
		if (*argv_exec) { 
			execvp_noret(argv_exec);
		} else {
			ssize_t r;
			char buf[BUFSIZ];

			while ((r = read(0, buf, BUFSIZ)) > 0) {
				if (write(1, buf, r) == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
			}

			if (r == -1) {
				perror("read");
				exit(EXIT_FAILURE);
			}
			return EXIT_SUCCESS;
		}
	} 

	return EXIT_FAILURE; /* This shouldn't be reachable */
}
