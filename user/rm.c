#include <lib.h>

int flag[256];

void usage(void) {
	printf("rm [-rf] <dir>|<file>\n");
	exit(-1);
}

void rm(char *path) {
    struct Stat st;
    int r;

    r = stat(path, &st);
    if (st.st_isdir) {//如果是目录
        if(flag['r']) {
            r = remove(path);
            if (r < 0 && !flag['f']) {
                user_panic("rm: cannot remove '%s': No such file or directory", path);
            }
        } else {
            user_panic("rm: cannot remove '%s': Is a directory", path);
        }
    } else {
        r = remove(path);
        if (r < 0 && !flag['f']) {
            user_panic("rm: cannot remove '%s': No such file or directory", path);
        }
    }
}

int main(int argc, char **argv) {
	int i;

	ARGBEGIN {
	default:
		usage();
	case 'r':
	case 'f':
		flag[(u_char)ARGC()]++;
		break;
	}
	ARGEND

	if (argc == 0) {
		usage();
	} else {
		for (i = 0; i < argc; i++) {
			rm(argv[i]);
		}
	}
	return 0;
}
