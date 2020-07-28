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

#define BUILTIN_CMD_MAX_LEN 16

extern char getch();

#define MAX_PATH_LEN 256
static char cwd[MAX_PATH_LEN] = { 0 };

static const char *getcwd()
{
	if (*cwd) {
		return cwd;
	} else {
		return "/";
	}
}

// walk through a directory, run callback for each dirent
static int walk(const char *dir, bool (*callback)(struct dirent *, void *),
		void *data)
{
	if (!callback)
		return;

	struct fs_request req = {
		.req = FS_REQ_SCAN,
		.count = PAGE_SIZE,
		.offset = 0,
	};

	if (!*dir) {
		strcpy(req.path, getcwd());
	} else if (*dir == '/') {
		strcpy(req.path, dir);
	} else {
		strcpy(req.path, cwd);
		strcat(req.path, "/");
		strcat(req.path, dir);
	}

	int ret;
	int count = 0;
	do {
		ipc_msg_t *ipc_msg =
			ipc_create_msg(&ipc_struct, sizeof(req), 1);
		ipc_set_msg_cap(ipc_msg, 0, tmpfs_scan_pmo_cap);
		ipc_set_msg_data(ipc_msg, &req, 0, sizeof(req));
		ret = ipc_call(&ipc_struct, ipc_msg);
		ipc_destroy_msg(ipc_msg);
		if (ret < 0) {
			return ret;
		}
		req.offset += ret;
		struct dirent *dirent = (struct dirent *)TMPFS_SCAN_BUF_VADDR;
		for (int i = 0; i < ret; i++) {
			count++;
			if (callback(dirent, data))
				return count;
			dirent = (void *)dirent + dirent->d_reclen;
		}
	} while (ret != 0);
	return count;
}

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

static bool _builtin_cmd_ls_walk_cb(struct dirent *dent, void *data)
{
	printf("%s\n", dent->d_name);
	return false; // keep walking
}

static void builtin_cmd_ls(const char *arg)
{
	int ret = walk(arg, _builtin_cmd_ls_walk_cb, NULL);
	if (ret < 0) {
		printf("ls: failed to list files, ret: %d\n", ret);
	}
}

static bool _builtin_cmd_cd_walk_cb(struct dirent *dent, void *data)
{
	return true; // stop walking immediately
}

static void builtin_cmd_cd(const char *arg)
{
	if (!*arg) {
		printf("cd: too few arguments\n");
		return;
	}

	if (0 == strcmp(arg, "/")) {
		cwd[0] = '\0'; // cd to /
		return;
	}

	int ret = walk(arg, _builtin_cmd_cd_walk_cb, NULL);
	if (ret >= 0) {
		if (*arg == '/') {
			strcpy(cwd, *(arg + 1) ? arg : "");
		} else {
			strcat(cwd, "/");
			strcat(cwd, arg);
		}
	} else if (ret == -ENOENT) {
		printf("cd: no such directory\n");
	} else if (ret == -ENOTDIR) {
		printf("cd: not a directory\n");
	} else {
		printf("cd: failed due to unknown reason\n");
	}
}

static void builtin_cmd_pwd(const char *arg)
{
	if (*arg != '\0') {
		printf("pwd: too many arguments");
		return;
	}
	printf("%s\n", getcwd());
}

static void builtin_cmd_echo(const char *arg)
{
	printf("%s\n", arg);
}

static void builtin_cmd_cat(const char *arg)
{
	if (!*arg) {
		printf("cd: too few arguments\n");
		return;
	}

	struct fs_request req = {
		.req = FS_REQ_READ,
		.count = PAGE_SIZE - 1,
		.offset = 0,
	};

	if (*arg == '/') {
		strcpy(req.path, arg);
	} else {
		strcpy(req.path, cwd);
		strcat(req.path, "/");
		strcat(req.path, arg);
	}

	char *buff = (char *)TMPFS_SCAN_BUF_VADDR;
	int read_count;
	do {
		ipc_msg_t *ipc_msg =
			ipc_create_msg(&ipc_struct, sizeof(req), 1);
		ipc_set_msg_cap(ipc_msg, 0, tmpfs_scan_pmo_cap);
		ipc_set_msg_data(ipc_msg, &req, 0, sizeof(req));
		read_count = ipc_call(&ipc_struct, ipc_msg);
		ipc_destroy_msg(ipc_msg);
		if (read_count < 0) {
			printf("cat: failed to read file, ret: %d\n",
			       read_count);
			return;
		}
		req.offset += read_count;
		buff[read_count] = '\0';
		printf("%s", buff);
	} while (read_count == req.count);
}

static void builtin_cmd_top(const char *arg)
{
	usys_ugly_top();
}

// run `ls`, `echo`, `cat`, `cd`, `top`
// return true if `cmdline` is a builtin command
int builtin_cmd(char *cmdline)
{
	// TODO(Lab5): your code here
	// assert: no \r and \n in cmdline
	char cmd[BUILTIN_CMD_MAX_LEN];
	char *arg = cmdline;
	size_t len = strlen(cmdline);
	int i = 0;
	while (i < len && *arg != ' ') {
		if (i == BUILTIN_CMD_MAX_LEN - 1)
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

static void ugly_wait(int thread_cap)
{
	// this is really ugly!
	int ret;
	do {
		usys_yield();
		ret = usys_get_affinity(thread_cap);
	} while (ret >= 0);
}

static int run_cmd_from_kernel_cpio(const char *filename, int *new_thread_cap,
				    struct pmo_map_request *pmo_map_reqs,
				    int nr_pmo_map_reqs);

// run other command, such as execute an executable file
// return true if run sccessfully
int run_cmd(char *cmdline)
{
	if (!*cmdline)
		return 0;

	char pathbuf[MAX_PATH_LEN];
	if (*cmdline == '/') {
		strcpy(pathbuf, cmdline);
	} else {
		strcpy(pathbuf, cwd);
		strcat(pathbuf, "/");
		strcat(pathbuf, cmdline);
	}

	struct user_elf user_elf;
	int ret;
	int new_thread_cap;
	ret = readelf_from_fs(pathbuf, &user_elf);
	if (ret < 0)
		return ret;
	// TODO: 运行一些程序会出现无效指令的情况, 例如 ipc_mem.bin,
	// 但 ipc_reg.bin 可以完整运行, 有待进一步 debug.
	ret = launch_process_with_pmos_caps(&user_elf, NULL, &new_thread_cap,
					    NULL, 0, NULL, 0, 0);
	// ret = run_cmd_from_kernel_cpio("/ipc_mem.bin", &new_thread_cap, NULL,
	// 			       0);
	if (ret < 0)
		return ret;
	ugly_wait(new_thread_cap);
	return 1;
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
