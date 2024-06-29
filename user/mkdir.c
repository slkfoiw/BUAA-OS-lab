#include <lib.h>
int flag[256];

void usage(void) {
	printf("usage: mkdir [-p] <dir>\n");
	exit(-1);
}

void mkdir(char *path) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        if (flag['p']) return;
        close(fd);
        user_panic("mkdir: cannot create directory '%s': File exists", path);
    }

    char *tmp = path;
    char *p = 0;
    while((tmp = strchr(tmp, '/')) != 0) {
        p = tmp;
        tmp++;
    }
    char parent_dir[500];
    strcpy(parent_dir, path);
    //debugf("p = %d\n", p);
    if (p && p > path) {
        parent_dir[p - path] = '\0';
        fd = open(parent_dir, O_RDONLY);
        if (fd < 0 && !flag['p']) {
            user_panic("mkdir: cannot create directory '%s': No such file or directory", path);
        }
        close(fd);
    }

    fd = open(path, O_MKDIR);
    if (fd < 0) {
        user_panic("mkdir error:%d", fd);
    }
    close(fd);
}

int main(int argc, char **argv) {
    int i;

    ARGBEGIN {
	default:
		usage();
	case 'p':
		flag[(u_char)ARGC()]++;
		break;
	}
	ARGEND

    if (argc == 0) {
        usage();
    } else {
        for (i = 0; i < argc; i++) {
			mkdir(argv[i]);
		}
    }
    return 0;
}