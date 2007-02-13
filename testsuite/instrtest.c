#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <getopt.h>


#include <emu/emu.h>
#include <emu/emu_memory.h>
#include <emu/emu_cpu.h>
#include <emu/emu_log.h>
#include <emu/emu_cpu_data.h>

#define FAILED "\033[31;1mfailed\033[0m"
#define SUCCESS "\033[32;1msuccess\033[0m"

#define F(x) (1 << (x))


static struct run_time_options
{
	int verbose;
	int nasm_force;
} opts;

static const char *regm[] = {
	"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};


	                         /* 0     1     2     3      4       5       6     7 */
static const char *flags[] = { "CF", "  ", "PF", "  " , "AF"  , "    ", "ZF", "SF", 
	                           "TF", "IF", "DF", "OF" , "IOPL", "IOPL", "NT", "  ",
	                           "RF", "VM", "AC", "VIF", "RIP" , "ID"  , "  ", "  ",
	                           "  ", "  ", "  ", "   ", "    ", "    ", "  ", "  "};


struct instr_test
{
	const char *instr;

	char  *code;
	uint16_t codesize;

	struct 
	{
		uint32_t reg[8];
		uint32_t		mem_state[2];
		uint32_t	eflags;
	} in_state;

	struct 
	{
		uint32_t reg[8];
		uint32_t		mem_state[2];
		uint32_t	eflags;
	}out_state;
};

#define FLAG_SET(fl) (1 << (fl))

struct instr_test tests[] = 
{

/*  {
        .instr = "instr",
        .in_state.reg  = {0,0,0,0,0,0,0,0 },
        .in_state.mem_state = {0, 0},
        .out_state.reg  = {0,0,0,0,0,0,0,0 },
        .out_state.mem_state = {0, 0},
    },*/
    /* 00 */
	{
		.instr = "add ah,al",
		.code = "\x00\xc4",
		.codesize = 2,
		.in_state.reg  = {0xff01,0,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0x01,0,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags = FLAG_SET(f_cf) | FLAG_SET(f_of) | FLAG_SET(f_pf) | FLAG_SET(f_zf),
	},
	{
		.instr = "add ch,dl",
		.code = "\x00\xd5",
		.codesize = 2,
		.in_state.reg  = {0,0x1000,0x20,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0,0x3000,0x20,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add [ecx],al",
		.code = "\x00\x01",
		.codesize = 2,
		.in_state.reg  = {0x10,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x10101010},
		.out_state.reg  = {0x10,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x10101020},
	},
	/* 01 */
	{
		.instr = "add ax,cx",
		.code = "\x66\x01\xc8",
		.codesize = 3,
		.in_state.reg  = {0xffff1111,0xffff2222,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0xffff3333,0xffff2222,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf), 
	},
	{
		.instr = "add [ecx],ax",
		.code = "\x66\x01\x01",
		.codesize = 3,
		.in_state.reg  = {0xffff1111,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x22224444},
		.out_state.reg  = {0xffff1111,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x22225555},
		.out_state.eflags =  FLAG_SET(f_pf), 
	},
	{
		.instr = "add eax,ecx",
		.code = "\x01\xc8",
		.codesize = 2,
		.in_state.reg  = {0x11112222,0x22221111,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0x33333333,0x22221111,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf), 
	},
	{
		.instr = "add [ecx],eax",
		.code = "\x01\x01",
		.codesize = 2,
		.in_state.reg  = {0x22221111,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x22224444},
		.out_state.reg  = {0x22221111,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x44445555},
		.out_state.eflags =  FLAG_SET(f_pf), 
	},
	/* 02 */
	{
		.instr = "add cl,bh",
		.code = "\x02\xcf",	/* add cl,bh */
		.codesize = 2,
		.in_state.reg  = {0,0xff,0,0x100,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0,0,0,0x100,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_cf) | FLAG_SET(f_pf) | FLAG_SET(f_zf) | FLAG_SET(f_of), 
	},
	{
		.instr = "add al,[ecx]",
		.code = "\x02\x01",
		.codesize = 2,
		.in_state.reg  = {0x3,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x30303030},
		.out_state.reg  = {0x33,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x30303030},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	/* 03 */
	{
		.instr = "add cx,di",
		.code = "\x66\x03\xcf",	/* add cx,di */
		.codesize = 3,
		.in_state.reg  = {0,0x10101010,0,0,0,0,0,0x02020202},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0,0x10101212,0,0,0,0,0,0x02020202},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add ax,[ecx]",
		.code = "\x66\x03\x01",
		.codesize = 3,
		.in_state.reg  = {0x11112222,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x44443333},
		.out_state.reg  = {0x11115555,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x44443333},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add ecx,edi",
		.code = "\x03\xcf",	/* add ecx,edi */
		.codesize = 2,
		.in_state.reg  = {0,0x10101010,0,0,0,0,0,0x02020202},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0,0x12121212,0,0,0,0,0,0x02020202},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add eax,[ecx]",
		.code = "\x03\x01",
		.codesize = 2,
		.in_state.reg  = {0x11112222,0x40000,0,0,0,0,0,0},
		.in_state.mem_state = {0x40000, 0x44443333},
		.out_state.reg  = {0x55555555,0x40000,0,0,0,0,0,0},
		.out_state.mem_state = {0x40000, 0x44443333},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add ecx,[ebx+eax*4+0xdeadbeef]",
		.code = "\x03\x8c\x83\xef\xbe\xad\xde",
		.codesize = 7,
		.in_state.reg  = {0x2,0x1,0,0x1,0,0,0,0},
		.in_state.mem_state = {0xdeadbef8, 0x44443333},
		.out_state.reg  = {0x2,0x44443334,0,0x1,0,0,0,0},
		.out_state.mem_state = {0xdeadbef8, 0x44443333},
	},
	/* 04 */
	{
		.instr = "add al,0x11",
		.code = "\x04\x11",
		.codesize = 2,
		.in_state.reg  = {0x22222222,0,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0x22222233,0,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	/* 05 */
	{
		.instr = "add ax,0x1111",
		.code = "\x66\x05\x11\x11",
		.codesize = 4,
		.in_state.reg  = {0x22222222,0,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0x22223333,0,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
	{
		.instr = "add eax,0x11111111",
		.code = "\x05\x11\x11\x11\x11",
		.codesize = 5,
		.in_state.reg  = {0x22222222,0,0,0,0,0,0,0},
		.in_state.mem_state = {0, 0},
		.out_state.reg  = {0x33333333,0,0,0,0,0,0,0},
		.out_state.mem_state = {0, 0},
		.out_state.eflags =  FLAG_SET(f_pf),
	},
};

int prepare()
{
	int i;
	for (i=0;i<sizeof(tests)/sizeof(struct instr_test);i++)
	{
		if ( opts.nasm_force == 0 && tests[i].code != NULL )
		{ // dup it so we can free it
			char *c = (char *)malloc(tests[i].codesize);
			memcpy(c,tests[i].code,tests[i].codesize);
			tests[i].code = c;
		} else
		{
			const char *use = "USE32\n";
			FILE *f=fopen("/tmp/foo.S","w+");

			if (f == NULL)
			{
				printf("failed to create asm file for nasm instruction %s\n\n\t%s",tests[i].instr,strerror(errno));
				return -1;
			}

			fwrite(use,strlen(use),1,f);
			fwrite(tests[i].instr,1,strlen(tests[i].instr),f);
			fclose(f);
			system("cd /tmp/; nasm foo.S");
			f=fopen("/tmp/foo","r");
			if (f == NULL)
			{
				printf("failed to open compiled nasm file for read for instruction %s\n\n\t%s",tests[i].instr,strerror(errno));
				return -1;
			}

			fseek(f,0,SEEK_END);

			tests[i].codesize = ftell(f);
			tests[i].code = malloc(tests[i].codesize);
			fseek(f,0,SEEK_SET);
			fread(tests[i].code,1,tests[i].codesize,f);
			fclose(f);

			unlink("/tmp/foo.S");
			unlink("/tmp/foo");
		}
	}
	return 0;
}


int test()
{
	int i=0;
	struct emu *e = emu_new();
	struct emu_cpu *cpu = emu_cpu_get(e);
	struct emu_memory *mem = emu_memory_get(e);

	for (i=0;i<sizeof(tests)/sizeof(struct instr_test);i++)
	{
		int failed = 0;


		printf("testing '%s' \t",tests[i].instr);
		int j=0;

		if ( opts.verbose == 1 )
		{
			printf("code '");
			for ( j=0;j<tests[i].codesize;j++ )
			{
				printf("%02x ",tests[i].code[j]);
			}
			printf("' ");
		}


		/* set the registers to the initial values */
		for ( j=0;j<8;j++ )
		{
			emu_cpu_reg32_set(cpu,j ,tests[i].in_state.reg[j]);
		}
   	

		/* set the flags */
		emu_cpu_eflags_set(cpu,tests[i].in_state.eflags);


		/* write the code to the offset */
		int static_offset = 4711;
		for( j = 0; j < tests[i].codesize; j++ )
		{
			emu_memory_write_byte(mem, static_offset+j, tests[i].code[j]);
		}

		emu_memory_write_dword(mem, tests[i].in_state.mem_state[0], tests[i].in_state.mem_state[1]);
		if (opts.verbose)
		{
			printf("memory at 0x%08x = 0x%08x (%i %i)\n",
				   tests[i].in_state.mem_state[0], 
				   tests[i].in_state.mem_state[1],
				   (int)tests[i].in_state.mem_state[1],
				   (uint32_t)tests[i].in_state.mem_state[1]);
		}


		/* set eip to the code */
		emu_cpu_eip_set(emu_cpu_get(e), static_offset);

		/* run the code */
		if (opts.verbose == 1)
		{
        	emu_log_level_set(emu_logging_get(e),EMU_LOG_DEBUG);
			emu_cpu_debug_print(cpu);
			emu_log_level_set(emu_logging_get(e),EMU_LOG_NONE);
		}
		
		int ret = emu_cpu_run(emu_cpu_get(e));

		if ( ret != 0 )
		{
			printf("cpu error %s\n", emu_strerror(e));
		}
   

		if (opts.verbose == 1)
		{
			emu_log_level_set(emu_logging_get(e),EMU_LOG_DEBUG);
			emu_cpu_debug_print(cpu);
			emu_log_level_set(emu_logging_get(e),EMU_LOG_NONE);
		}
        	

		/* check the registers for the exptected values */

		for ( j=0;j<8;j++ )
		{
			if ( emu_cpu_reg32_get(cpu, j) ==  tests[i].out_state.reg[j] )
			{
				if (opts.verbose == 1)
					printf("\t %s "SUCCESS"\n",regm[j]);
			} else
			{
				printf("\t %s "FAILED" got 0x%08x expected 0x%08x\n",regm[j],emu_cpu_reg32_get(cpu, j),tests[i].out_state.reg[j]);
				failed = 1;
			}
		}


		/* check the memory for expected values */
		uint32_t value;

		if ( tests[i].out_state.mem_state[0] != -1 )
		{
			if ( emu_memory_read_dword(mem,tests[i].out_state.mem_state[0],&value) == 0 )
			{
				if ( value == tests[i].out_state.mem_state[1] )
				{
					if (opts.verbose == 1)
						printf("\t memory "SUCCESS" 0x%08x = 0x%08x\n",tests[i].out_state.mem_state[0], tests[i].out_state.mem_state[1]);
				}
				else
				{
					printf("\t memory "FAILED" at 0x%08x got 0x%08x expected 0x%08x\n",tests[i].out_state.mem_state[0],value, tests[i].out_state.mem_state[1]);
					failed = 1;
				}

			} else
			{
				printf("\t"FAILED" emu says: '%s' when accessing %08x\n", strerror(emu_errno(e)),tests[i].out_state.mem_state[0]);
				failed = 1;
			}

		}

		/* check the cpu flags for expected values */
		if ( tests[i].out_state.eflags != emu_cpu_eflags_get(cpu) )
		{
			printf("\t flags "FAILED" got %08x expected %08x\n",emu_cpu_eflags_get(cpu),tests[i].out_state.eflags);
			for(j=0;j<32;j++)
			{
				uint32_t f = emu_cpu_eflags_get(cpu);
				if ( (tests[i].out_state.eflags & (1 << j)) != (f & (1 <<j)))
					printf("\t flag %s (bit %i) failed, expected %i is %i\n",flags[j], j, 
						   (tests[i].out_state.eflags & (1 << j)),
						   (f & (1 <<j)));
			}

			failed = 1;
		}else
		{
			if (opts.verbose == 1)
				printf("\t flags "SUCCESS"\n");
		}


		/* bail out on *any* error */
		if (failed == 0)
		{
			printf(SUCCESS"\n");
		}else
		{
			return -1;
		}
		
	}
	emu_free(e);
	return 0;
}

void cleanup()
{
	int i;
	for (i=0;i<sizeof(tests)/sizeof(struct instr_test);i++)
    	if (tests[i].code != NULL)
    		free(tests[i].code);
		
}

int main(int argc, char *argv[])
{
	memset(&opts,0,sizeof(struct run_time_options));

	while ( 1 )
	{	
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{"verbose"			, 0, 0, 'v'},
			{"nasm-force"		, 0, 0, 'n'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "vn", long_options, &option_index);
		if ( c == -1 )
			break;

		switch ( c )
		{
		case 'v':
			opts.verbose = 1;
			break;

		case 'n':
			opts.nasm_force = 1;
			break;


		default:
			printf ("?? getopt returned character code 0%o ??\n", c);
			break;
		}
	}



	if ( prepare() != 0)
		return -1;

	if ( test() != 0 )
		return -1;

	cleanup();

	return 0;
}
