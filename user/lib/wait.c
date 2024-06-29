#include <env.h>
#include <lib.h>
int wait(u_int envid) {
	const volatile struct Env *e;

	e = &envs[ENVX(envid)];
	while (e->env_id == envid && e->env_status != ENV_FREE) {
		if (env->env_kill == 1) {
			//debugf("destroy child %x", envid);
			syscall_env_destroy(envid, -1);
			exit(-1);
		}
		syscall_yield();
	}
	return e->env_return_value;
}
