#include <lib.h>

void usage(void) {
	printf("usage: touch <file>\n");
	exit(-1);
}

void touch(char *path) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {//若文件存在则放弃创建，正常退出无输出
        close(fd);
        return;
    }

    char *tmp = path;
    char *p = 0;
    while((tmp = strchr(tmp, '/')) != 0) {
        p = tmp;
        tmp++;
    }
    char dir[200];
    strcpy(dir, path);
    //debugf("p = %d\n", p);
    if (p && p > path) {
        dir[p - path] = '\0';
        fd = open(dir, O_RDONLY);
        if (fd < 0) {
            user_panic("touch: cannot touch '%s': No such file or directory", path);
        }
        close(fd);
    }

    fd = open(path, O_CREAT);
    if (fd < 0) {
        user_panic("touch error:%d", fd);
    }
    close(fd);
}

int main(int argc, char **argv) {
	int i;

	if (argc == 1) {
		usage();
	} else {
		for (i = 1; i < argc; i++) {
			touch(argv[i]);
		}
	}
	return 0;
}
