#include <env.h>
#include <lib.h>
#include <mmu.h>

void exit(int return_value) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif

	//2.实现指令条件执行
	syscall_env_destroy(0, return_value);
	user_panic("unreachable code");
}

const volatile struct Env *env;
extern int main(int, char **);

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	int r = main(argc, argv);//2.获取程序main函数返回值（即获得指令执行返回值）

	// exit gracefully
	exit(r);
}
