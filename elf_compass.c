/*
 * ELF Compass
 * bind@insidiae.org
 *
 * Similar to ltrace/strace except for local functions.
 * Helps when debugging / fuzzing applications
 *
 * Original idea from Halvar's win32 funcdbg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/user.h>

#include "list.h"

void	resolve_symbols(int, list_t *);
void	set_eip(pid_t, long);
int	breakpoint_set(pid_t, ulong);
int	breakpoint_rem(pid_t, ulong, long);

typedef struct breakpoint {
	int value;
	long addr;
	char *name;
} bpx_t;

int
main(int argc, char **argv)
{
	pid_t pid;
	list_t *list;
	element_t *element;
	char buf[1024];
	int i, fd, ret;
	struct user_regs_struct regs;

	memset(buf, 0, sizeof(buf));

	if(argc < 2) {
		printf("usage: %s <pid>\n", argv[0]);
		exit(-1);
	}

	pid = atoi(argv[1]);

	snprintf(buf, sizeof(buf) - 1, "/proc/%d/exe", pid);

	fd = open(buf, O_RDONLY);
	if(fd < 0) {
		perror("open");
		exit(-1);
	}

	list = list_new();

	resolve_symbols(fd, list);

	if(list_size(list) <= 0) {
		printf("no symbols..\n");
		exit(-1);
	}

	ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);

	if(ret < 0) {
		perror("ptrace");
		exit(-1);
	}

	waitpid(pid, NULL, 0);

	/* 
	 * iterate through addresses, setting breakpoints
	 */

	element = list_head(list);
	
	for(i = 0; i < list_size(list); i++) {
		bpx_t *bpx;

		bpx = list_data(element);

		printf("setting breakpoint on 0x%x (%s)\n", (int)bpx->addr, bpx->name);

		bpx->value = breakpoint_set(pid, bpx->addr);

		element = list_next(element);
	}
	 
	while(1) {
	 	ret = ptrace(PTRACE_CONT, pid, NULL, NULL);
		if(ret < 0) {
			perror("ptrace");
			exit(-1);
		}

		wait(NULL);

 		ret = ptrace(PTRACE_GETREGS, pid, NULL, &regs);
		if(ret < 0) {
			perror("ptrace");
			exit(-1);
		}

		element = list_head(list);

		while(element) {
			bpx_t *bpx;

			bpx = list_data(element);

			if(bpx->addr + 2 == regs.eip) {
				printf("removing breakpoint on 0x%x (%s)\n", (int)bpx->addr, bpx->name);
				breakpoint_rem(pid, bpx->addr, bpx->value);
				set_eip(pid, bpx->addr);
			} 

			element = list_next(element);
		}
	}
	
	ret = ptrace(PTRACE_DETACH, pid, NULL, NULL);
	if(ret < 0) {
		perror("ptrace");
		exit(-1);
	}

	exit(0);
}

void
set_eip(pid_t pid, long addr)
{
	int ret;
	struct user_regs_struct regs;

   ret = ptrace(PTRACE_GETREGS, pid, NULL, &regs);
	if(ret < 0) {
		perror("ptrace");
		exit(-1);
	}

	regs.eip = addr;

   ret = ptrace(PTRACE_SETREGS, pid, NULL, &regs);
	if(ret < 0) {
		perror("ptrace");
		exit(-1);
	}
}

int
breakpoint_set(pid_t pid, ulong addr)
{
	int ret;
	long value, code;

	value = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);

	code = 0xcccccccc;

	ret = ptrace(PTRACE_POKETEXT, pid, addr, code);
	if(ret < 0) {
		perror("ptrace");
		return(-1);
	}

	return(value);
}

int
breakpoint_rem(pid_t pid, ulong addr, long backup)
{
	int ret;
	long value;

   ret = ptrace(PTRACE_POKETEXT, pid, addr, backup);
   if(ret < 0) {
	   perror("ptrace");
      return(-1);
   }
	
	value = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);

	return(0);
}

void
resolve_symbols(int fd, list_t *list)
{
	int i, j;						/* loop counter 						*/
	int ret;						/* return value 						*/
	char *strings;				/* strtab entries 					*/
	char *string;				/* symbol table strtab entries 	*/
	Elf32_Sym *sym;			/* symbol entry 						*/
	Elf32_Sym *tmp_sym;		/* temporary symbol 					*/
	Elf32_Ehdr ehdr;			/* ELF header 							*/
	Elf32_Shdr *shdr;			/* section header 					*/
	Elf32_Shdr *strtab;		/* strtab section header 			*/
	Elf32_Shdr *tmp_shdr;	/* temporary section header 		*/
	Elf32_Shdr *text_shdr;	/* .text section header 			*/

	/*
	 * Read the ELF Header and display
	 */
	ret = read(fd, &ehdr, sizeof(ehdr));
	if(ret != sizeof(ehdr)) {
		perror("read");
		exit(-1);
	}

	/*
	 * Allocate memory for sections headers
	 */
	shdr = (Elf32_Shdr *) malloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
	if(!shdr) {
		perror("malloc");
		exit(-1);
	}

	ret = lseek(fd, ehdr.e_shoff, SEEK_SET);
	if(ret < 0) {
		perror("lseek");
		exit(-1);
	}

	ret = read(fd, shdr, sizeof(Elf32_Shdr) * ehdr.e_shnum);
	if(ret != sizeof(Elf32_Shdr) * ehdr.e_shnum) {
		perror("read");
		exit(-1);
	}

	/*
	 * Pull out the string table
	 */
   strtab = &shdr[ehdr.e_shstrndx];

	strings = (char *) malloc(strtab->sh_size);
	if(!strings) {
		perror("malloc");
		exit(-1);
	}

	ret = lseek(fd, strtab->sh_offset, SEEK_SET);
	if(ret < 0) {
		perror("lseek");
		exit(-1);
	}

	ret = read(fd, strings, strtab->sh_size);
	if(ret != strtab->sh_size) {
		perror("read");
		exit(-1);
	}

	/* 
	 * Iterate through sections headers, find symtab
	 */

	for(tmp_shdr = shdr, i = 0; i < ehdr.e_shnum; ++tmp_shdr, i++) {
		if(tmp_shdr->sh_type == SHT_SYMTAB) {

			Elf32_Shdr *strtab_hdr = &shdr[tmp_shdr->sh_link]; 
			
			string = (char *) malloc(strtab_hdr->sh_size);
			if(!string) {
				perror("malloc");
				exit(-1);
			}

			ret = lseek(fd, strtab_hdr->sh_offset, SEEK_SET);
			if(ret != strtab_hdr->sh_offset) {
				perror("lseek");
				exit(-1);
			}

			ret = read(fd, string, strtab_hdr->sh_size);
			if(ret != strtab_hdr->sh_size) {
				perror("read");
				exit(-1);
			}

			sym = (Elf32_Sym *) malloc(tmp_shdr->sh_size);
			if(!sym) {
				perror("malloc");
				exit(-1);
			}

			ret = lseek(fd, tmp_shdr->sh_offset, SEEK_SET);
			if(ret != tmp_shdr->sh_offset) {
				perror("lseek");
				exit(-1);
			}

			ret = read(fd, sym, tmp_shdr->sh_size);
			if(ret != tmp_shdr->sh_size) {
				perror("read");
				exit(-1);
			}

			for(tmp_sym = sym, j = 0; j < tmp_shdr->sh_size; j += sizeof(Elf32_Sym), ++tmp_sym) {
				text_shdr = &shdr[tmp_sym->st_shndx];

				if(tmp_sym->st_shndx > ehdr.e_shnum)
					continue;

				if(!strcmp(&strings[text_shdr->sh_name], ".text")) {
					bpx_t *bpx;

					if(strlen(&string[tmp_sym->st_name]) <= 0)
						continue;

					if(!tmp_sym->st_size)
						continue;

					bpx = (bpx_t *) malloc(sizeof(bpx_t));

					bpx->name = &string[tmp_sym->st_name];

					bpx->addr = tmp_sym->st_value;
					bpx->value = 0;

					list_ins_next(list, list_tail(list), bpx);
				}
			}

			break;
		}
	}
}
