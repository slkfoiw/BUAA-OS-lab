#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;();#`"
#define UNCERTAIN -1
#define CERTAIN0 0
#define CERTAIN1 1

//history
void history_init();
void save_history();
void add_one_history(const char *command);
int get_last_history(char *oldbuf, char *newbuf);
int get_next_history(char *newbuf);
void history();

//jobs_manage
void add_one_job(int env_id, char *cmd);
void update_jobs_status();
void update_jobs();
void print_all_jobs();
int fg_job(int job_id);
int kill_job(int job_id);

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}

	if (*s == '\"' || *s == '\'') {//9.实现引号支持
		int t = *s;
		s++;
		*p1 = s;
		while (*s != t) {
			if (*s == 0) {
				user_panic("missing %c", t);
			}
			s++;
		}
		*s++ = 0;
		*p2 = s;
		return 'w';
	} 

	if (strchr(SYMBOLS, *s)) {
		if (*s == '|' && *(s+1) == '|') {//2.实现指令条件执行
			*p1 = s;
			*s++ = 0;
			*s++ = 0;
			*p2 = s;
			return 'o';
		} else if (*s == '&' && *(s+1) == '&') {//2.实现指令条件执行
			*p1 = s;
			*s++ = 0;
			*s++ = 0;
			*p2 = s;
			return 'a';
		} else {
			int t = *s;
			*p1 = s;
			*s++ = 0;
			*p2 = s;
			return t;
		}
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

static char *np1, *np2;
int gettoken(char *s, char **p1) {
	static int c, nc;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128
int flag = 0;

int parsecmd(char **argv, int *rightpipe, int *need_send, int *is_executable, int return_value) {
	int argc = 0;
	while (1) {
		char *t;
		int fd, r, tmp;
		int p[2];
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit(-1);
			}
			argv[argc++] = t;
			break;
		case '`':
			if (flag) {
				flag = 0;
				break;
			}
			flag = 1;
			return parsecmd(argv, rightpipe, need_send, is_executable, return_value);
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit(-1);
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			fd = open(t, O_RDONLY);
			if (fd < 0) {
				debugf("< open error!\n");
				exit(-1);
			}
			dup(fd, 0);
			close(fd);
			
			//user_panic("< redirection not implemented");

			break;
		case '>':;
			tmp = gettoken(0, &t);
			if (tmp == '>') {
				tmp = gettoken(0, &t);
				if (tmp != 'w') {
					debugf("syntax error: > not followed by word\n");
					exit(-1);
				}
				fd = open(t, O_APPEND | O_CREAT);
				if (fd < 0) {
					debugf(">> open error!\n");
					exit(-1);
				}
				dup(fd, 1);
				close(fd);
			} else if (tmp != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit(-1);
			} else {
				// Open 't' for writing, create it if not exist and trunc it if exist, dup
				// it onto fd 1, and then close the original fd.
				// If the 'open' function encounters an error,
				// utilize 'debugf' to print relevant messages,
				// and subsequently terminate the process using 'exit'.
				/* Exercise 6.5: Your code here. (2/3) */

				fd = open(t, O_WRONLY | O_TRUNC | O_CREAT);
				if (fd < 0) {
					debugf("> open error!\n");
					exit(-1);
				}
				dup(fd, 1);
				close(fd);
			}
			//user_panic("> redirection not implemented");

			break;
		case '|':
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			/* Exercise 6.5: Your code here. (3/3) */
			if (pipe(p) < 0) {
				debugf("| create pipe error!");
				exit(-1);
			}
			if ((r = fork()) < 0) {
				debugf("| fork error!");
				exit(-1);
			}
			*rightpipe = r;
			if (r == 0) { //child
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe, need_send, is_executable, return_value);
			} else { // parent
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			//user_panic("| not implemented");

			break;
		case 'o'://2.实现指令条件执行
			if ((r = fork()) < 0) {
				debugf("|| fork error!");
				exit(-1);
			}
			*rightpipe = r;
			if (r == 0) { //child left
				if (!*is_executable) {
					ipc_send(env->env_parent_id, return_value, 0, 0);
					exit(0);
				}
				*need_send = 1;
				return argc;
			} else { // parent right
				int whom = 0, rv;
				while (whom != r) {
					rv = ipc_recv(&whom, 0, 0);
				}
				wait(r);
				//debugf("runcmd child %x r = %d\n", r, rp);
				if (rv != 0) {
					*is_executable = 1;
					return_value = UNCERTAIN;
				} else {
					*is_executable = 0;
					return_value = CERTAIN0;
				}
				return parsecmd(argv, rightpipe, need_send, is_executable, return_value);
			}
			break;
		case 'a':;//2.实现指令条件执行
			if ((r = fork()) < 0) {
				debugf("&& fork error!");
				exit(-1);
			}
			*rightpipe = r;
			if (r == 0) { //child left
				if (!*is_executable) {
					ipc_send(env->env_parent_id, return_value, 0, 0);
					exit(0);
				}
				*need_send = 1;
				return argc;
			} else { // parent right
				int whom = 0, rv;
				while (whom != r) {
					rv = ipc_recv(&whom, 0, 0);
				}
				wait(r);
				//debugf("runcmd child %x r = %d\n", r, rv);
				if (rv == 0) {
					*is_executable = 1;
					return_value = UNCERTAIN;
				} else {
					*is_executable = 0;
					return_value = CERTAIN1;
				}
				return parsecmd(argv, rightpipe, need_send, is_executable, return_value);
			}
			break;
		case '#'://5.实现注释功能
			return argc;
			break;
		case ';'://7.实现一行多指令
			if ((r = fork()) < 0) {
				debugf("; fork error!");
				exit(-1);
			}
			if (r == 0) {
				return argc;
			} else {
				wait(r);
				return parsecmd(argv, rightpipe, need_send, is_executable, return_value);
			}
			break;
		case '&':;//10.实现前后台任务管理
			tmp = gettoken(0, &t);
			if (tmp != 0) {
				debugf("background error!");
				exit(-1);
			}
			// if ((r = fork()) < 0) {
			// 	debugf("& fork error\n");
			// 	exit(-1);
			// }
			
			// if (r == 0) {
			// 	//debugf("0\n");
			// 	return argc;
			// } else {
			// 	debugf("%x\n", r);
			// 	exit(r);
			// }
			break;
		}
	}

	return argc;
}

void runcmd(char *s) {
	gettoken(s, 0);

	flag = 0;
	char *argv[MAXARGS];
	int rightpipe = 0;
	int is_executable = 1;
	int need_send = 0;
	int r = 0;//用于条件指令发消息
	int argc = parsecmd(argv, &rightpipe, &need_send, &is_executable, UNCERTAIN);
	int bg_flag = 0;//是不是跟前后台相关的指令, 不是返回0，fg、jobs返回-1刷新，kill返回job_id
	//debugf("argc = %d\n", argc);
	if (argc == 0 || !is_executable) {
		return;
	}
	argv[argc] = 0;
	// for (int i = 0; i < argc; i++) {
	// 	debugf("argv[%d] = %s\n", i, argv[i]);
	// }
	//debugf("before env_id = %x\n", env->env_id);
	//6.实现历史指令
	if (strcmp(argv[0], "history") == 0) {
		history();
	} else if(strcmp(argv[0], "jobs") == 0) {
		//debugf("jobs\n");
		print_all_jobs();
	} else if(strcmp(argv[0], "fg") == 0) {
		for (int i = 1; i < argc; i++) {
            fg_job(my_atoi(argv[i]));
        }
	} else if(strcmp(argv[0], "kill") == 0) {
		bg_flag = my_atoi(argv[1]);
	} else {
		int child = spawn(argv[0], argv);
		close_all();
		if (child >= 0) {
			//debugf("runcmd:%x create child %x\n",env->env_id, child);
			r = wait(child);
			//debugf("spawn child %x r = %d\n", child, r);
		} else {
			debugf("spawn %s: %d\n", argv[0], child);
		}
	}

	if (need_send) {
			ipc_send(env->env_parent_id, r, 0, 0);//用于条件指令传消息
	}

	if (rightpipe) {
		wait(rightpipe);
	}

	exit(bg_flag);
}

void readline(char *buf, u_int n) {
	int r;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit(0);
		}
		if (buf[i] == 27) {//6.实现历史指令
			char c;
			int r = read(0, &c, 1);
			if (c != '[') {
				debugf("read direction error\n");
				exit(-1);
			}
			r = read(0, &c, 1);
			
			//debugf("%d", c);
			int len;
			char new_buf[1024];
			if (c == 'A') {//up
				printf("\033[B");//抵消up键的上移一行
				buf[i] = 0;
				len = get_last_history(buf, new_buf);
			} else if (c == 'B') {//down
				//printf("\033[A");//抵消down键的下移一行
				len = get_next_history(new_buf);
			}
			if (len >= 0) {
				printf("\33[2K\r$ %s", new_buf);
				strcpy(buf, new_buf);
				i = strlen(buf) - 1;
			}
			continue;
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			add_one_history(buf);
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	debugf("sh envid:%x\n", env->env_id);
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	history_init();
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		update_jobs_status();
		
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf);
			exit(0);
		} else {
			//debugf("sh:%x create child %x\n",env->env_id, r);
			int len = strlen(buf);
			int i = len - 1;
			while (i > 0 && strchr(WHITESPACE, buf[i])) {
				i--;
			}
			if (buf[i] == '&') {
				add_one_job(r, buf);
			} else {
				int bg_envid = wait(r);
				if (bg_envid > 0) {
					kill_job(bg_envid);
				}
			}
		}
	}
	return 0;
}

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit(-1);
}

int my_atoi(const char *str) {
    int result = 0;
    int sign = 1;
    
    // 跳过前面的空格
    while (strchr(WHITESPACE, *str)) {
		str++;
	}
    
    // 处理正负号
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // 遍历字符串，将数字字符转换为整数
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return sign * result;
}

//history
#define HISTFILESIZE 20
#define BUFLEN 1024

int history_cnt = 0;
char history_buf[HISTFILESIZE][BUFLEN];
int history_now = 0;

void history_init() {
    int fd = open("/.mosh_history", O_CREAT);
    if (fd < 0) {
        user_panic("history init error!");
    }
    //debugf("history init\n");
    close(fd);
}

void save_history() {
    int fd = open("/.mosh_history", O_RDWR);
    if (fd < 0) {
        user_panic("/.mosh_history open error! when history_write");
    }

    int start = history_cnt > HISTFILESIZE ? history_cnt - HISTFILESIZE : 0;
    for (int i = start; i < history_cnt; i++) {
        write(fd, history_buf[i % HISTFILESIZE], BUFLEN);
        write(fd, "\n", 1);
    } 
}

void add_one_history(const char *command) {
    int len = strlen(command);
    if (len == 0) return;
    if (history_now < history_cnt) {//重置历史指令
        history_cnt = history_now;
    }
    strcpy(history_buf[history_cnt % HISTFILESIZE], command);
    history_cnt++;
    history_now++;
    save_history();
}

int get_last_history(char *oldbuf, char *newbuf) {
    int len;
    if (history_now  <= 0) {//没有上一条指令
        return -1;
    }
    strcpy(newbuf, history_buf[(history_now - 1)]);
    if (history_now == history_cnt) {
        strcpy(history_buf[history_now], oldbuf);
        history_cnt++;
        save_history();
    }
    history_now--;
    return strlen(newbuf);
}

int get_next_history(char *newbuf) {
    if (history_now == history_cnt) {//没有下一条指令
        return -1;
    }
    strcpy(newbuf, history_buf[(history_now + 1)]);
    history_now++;
    //save_history();
    // debugf("get_next_history size = %d\n", history_size);
    return strlen(newbuf);
}

void history() {
    int fd = open("/.mosh_history", O_RDONLY);
    if (fd < 0) {
        user_panic("/.mosh_history open error when history!");
    }
    
    int start = history_cnt > HISTFILESIZE ? history_cnt - HISTFILESIZE : 0;
    for (int i = start; i < history_cnt; i++) {
        printf("%d %s\n", i - start + 1, history_buf[i % HISTFILESIZE]);
    }
    close(fd);
}

//job_manage
#define MAXJOBLEN 100

struct Job {
    //int job_id;
    char job_status[10];
    int job_env_id;
    char job_cmd[1024];
} jobs[MAXJOBLEN];

int job_cnt = 0;

void add_one_job(int env_id, char *cmd) {
    if (job_cnt >= MAXJOBLEN) {
        user_panic("too many jobs!");
        return;
    }

    jobs[job_cnt].job_env_id = env_id; 
    strcpy(jobs[job_cnt].job_cmd, cmd);
    strcpy(jobs[job_cnt].job_status, "Running");
    job_cnt++;  
    //("job_cnt = %d\n", job_cnt);
}

void update_jobs_status() {
    const volatile struct Env *e;
    for (int i = 0; i < job_cnt; i++) {
        e = &envs[ENVX(jobs[i].job_env_id)];
        //debugf("job_env_id:%x env_id = %x env_status = %d\n",jobs[i].job_env_id,  e->env_id, e->env_status);
        if (strcmp(jobs[i].job_status, "Running") == 0) {
            if (e->env_status == ENV_FREE) {
                strcpy(jobs[i].job_status, "Done");
            } else {
                //debugf("not free\n");
                syscall_yield();
            }
        }
    }
    
}

void update_jobs() {
    int i = 0;
    //debugf("\nbefore update_jobs job_cnt = %d\n", job_cnt);
    while (i < job_cnt - 1) {
        if (strcmp(jobs[i].job_status, "Done") == 0) {
            int j = i + 1;
            while(strcmp(jobs[j].job_status, "Done") == 0) {
                if (j == job_cnt) {
                    job_cnt = i;
                    return;
                }
                j++;
            }
            strcpy(jobs[i].job_status, jobs[j].job_status);
            jobs[i].job_env_id = jobs[j].job_env_id;
            strcpy(jobs[i].job_cmd, jobs[j].job_cmd);
            job_cnt -= (j - i);
            i = j;
        } else {
            i++;
        }
    }
    if (strcmp(jobs[job_cnt - 1].job_status, "Done") == 0) {//最后一个
        job_cnt--;
    }
    //debugf("\nafter update_jobs job_cnt = %d\n", job_cnt);
}

void print_all_jobs() {
    int i;
    //debugf("job_cnt = %d\n", job_cnt);
    for (i = 0; i < job_cnt; i++) {
        printf("[%d] %-10s 0x%08x %s\n", i + 1, jobs[i].job_status, jobs[i].job_env_id, jobs[i].job_cmd);
    }
}

int fg_job(int job_id) {
    if (job_id > job_cnt) {
        printf("fg: job (%d) do not exist\n", job_id);
        return -1;
    }
    if (strcmp(jobs[job_id - 1].job_status, "Running") != 0) {
        printf("fg: (0x%08x) not running\n", jobs[job_id - 1].job_env_id);
        return -1;
    }
    //printf("%s\n", jobs[job_id - 1].job_cmd);
    wait(jobs[job_id - 1].job_env_id);
    strcpy(jobs[job_id - 1].job_status, "Done");
    update_jobs();
}

int kill_job(int job_id) {
    if (job_id > job_cnt) {
        printf("fg: job (%d) do not exist\n", job_id);
        return -1;
    }
    if (strcmp(jobs[job_id - 1].job_status, "Running") != 0) {
        printf("fg: (0x%08x) not running\n", jobs[job_id - 1].job_env_id);
        return -1;
    }

    int r = syscall_env_kill(jobs[job_id - 1].job_env_id, -1);
    strcpy(jobs[job_id - 1].job_status, "Done");
	return 0;
}
