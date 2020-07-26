#include <print.h>
#include <syscall.h>
#include <launcher.h>
#include <defs.h>
#include <bug.h>
#include <fs_defs.h>
#include <ipc.h>
#include <string.h>
#include <proc.h>

#define SERVER_READY_FLAG(vaddr) (*(int *)(vaddr))
#define SERVER_EXIT_FLAG(vaddr)  (*(int *)((u64)vaddr + 4))

extern ipc_struct_t *tmpfs_ipc_struct;
static ipc_struct_t ipc_struct;
static int tmpfs_scan_pmo_cap;

/* fs_server_cap in current process; can be copied to others */
int fs_server_cap;

#define BUFLEN 4096

#define MAX_PATH_LEN 512
static char cwd[MAX_PATH_LEN + 1] = { 0 };

#define BUILTIN_CMD_MAX_LEN 16

extern char getch();

// read a command from stdin leading by `prompt`
// put the commond in `buf` and return `buf`
// What you typed should be displayed on the screen
char *readline(const char *prompt)
{
	static char buf[BUFLEN];

	int i = 0, j = 0;
	signed char c = 0;
	int ret = 0;
	char complement[BUFLEN];
	int complement_time = 0;

	if (prompt != NULL) {
		printf("%s", prompt);
	}

	while (1) {
		c = getch();
		if (c < 0)
			return NULL;
		// TODO(Lab5): your code here
		if (c == '\r' || c == '\n' || i == BUFLEN - 1)
			break;
		buf[i++] = c;
		usys_putc(c);
	}
	buf[i] = '\0';
	printf("\n");
	return buf;
}

static void builtin_cmd_ls(const char *arg)
{
}

static void builtin_cmd_cd(const char *arg)
{
}

static void builtin_cmd_pwd(const char *arg)
{
	if (*arg != '\0') {
		printf("pwd: too many arguments");
		return;
	}
	if (*cwd == '\0') {
		printf("/\n");
	} else {
		printf("%s\n", cwd);
	}
}

static void builtin_cmd_echo(const char *arg)
{
	printf("%s\n", arg);
}

static void builtin_cmd_cat(const char *arg)
{
}

static void builtin_cmd_top(const char *arg)
{
}

// run `ls`, `echo`, `cat`, `cd`, `top`
// return true if `cmdline` is a builtin command
int builtin_cmd(char *cmdline)
{
	// TODO(Lab5): your code here
	// assert: no \r and \n in cmdline
	char cmd[BUILTIN_CMD_MAX_LEN + 1];
	char *arg = cmdline;
	size_t len = strlen(cmdline);
	int i = 0;
	while (i < len && *arg != ' ') {
		if (i == BUILTIN_CMD_MAX_LEN)
			return 0;
		cmd[i++] = *arg++;
	}
	cmd[i] = '\0';
	while (*arg == ' ')
		arg++;
	if (0 == strcmp(cmd, "ls")) {
		builtin_cmd_ls(arg);
	} else if (0 == strcmp(cmd, "cd")) {
		builtin_cmd_cd(arg);
	} else if (0 == strcmp(cmd, "pwd")) {
		builtin_cmd_pwd(arg);
	} else if (0 == strcmp(cmd, "echo")) {
		builtin_cmd_echo(arg);
	} else if (0 == strcmp(cmd, "cat")) {
		builtin_cmd_cat(arg);
	} else if (0 == strcmp(cmd, "top")) {
		builtin_cmd_top(arg);
	} else {
		return 0;
	}
	return 1;
}

// run other command, such as execute an executable file
// return true if run sccessfully
int run_cmd(char *cmdline)
{
	return 0;
}

static int run_cmd_from_kernel_cpio(const char *filename, int *new_thread_cap,
				    struct pmo_map_request *pmo_map_reqs,
				    int nr_pmo_map_reqs)
{
	struct user_elf user_elf;
	int ret;

	ret = readelf_from_kernel_cpio(filename, &user_elf);
	if (ret < 0) {
		printf("[Shell] No such binary in kernel cpio\n");
		return ret;
	}
	return launch_process_with_pmos_caps(&user_elf, NULL, new_thread_cap,
					     pmo_map_reqs, nr_pmo_map_reqs,
					     NULL, 0, 0);
}

void boot_fs(void)
{
	int ret = 0;
	int info_pmo_cap;
	int tmpfs_main_thread_cap;
	struct pmo_map_request pmo_map_requests[1];

	/* create a new process */
	printf("Booting fs...\n");
	/* prepare the info_page (transfer init info) for the new process */
	info_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	fail_cond(info_pmo_cap < 0, "usys_create_ret ret %d\n", info_pmo_cap);

	ret = usys_map_pmo(SELF_CAP, info_pmo_cap, TMPFS_INFO_VADDR,
			   VM_READ | VM_WRITE);
	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	SERVER_READY_FLAG(TMPFS_INFO_VADDR) = 0;
	SERVER_EXIT_FLAG(TMPFS_INFO_VADDR) = 0;

	/* We also pass the info page to the new process  */
	pmo_map_requests[0].pmo_cap = info_pmo_cap;
	pmo_map_requests[0].addr = TMPFS_INFO_VADDR;
	pmo_map_requests[0].perm = VM_READ | VM_WRITE;
	ret = run_cmd_from_kernel_cpio("/tmpfs.srv", &tmpfs_main_thread_cap,
				       pmo_map_requests, 1);
	fail_cond(ret != 0, "create_process returns %d\n", ret);

	fs_server_cap = tmpfs_main_thread_cap;

	while (SERVER_READY_FLAG(TMPFS_INFO_VADDR) != 1)
		usys_yield();

	/* register IPC client */
	tmpfs_ipc_struct = &ipc_struct;
	ret = ipc_register_client(tmpfs_main_thread_cap, tmpfs_ipc_struct);
	fail_cond(ret < 0, "ipc_register_client failed\n");

	tmpfs_scan_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
	fail_cond(tmpfs_scan_pmo_cap < 0, "usys create_ret ret %d\n",
		  tmpfs_scan_pmo_cap);

	ret = usys_map_pmo(SELF_CAP, tmpfs_scan_pmo_cap, TMPFS_SCAN_BUF_VADDR,
			   VM_READ | VM_WRITE);
	fail_cond(ret < 0, "usys_map_pmo ret %d\n", ret);

	printf("fs is UP.\n");
}
