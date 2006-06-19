/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/**
 * @file      archc.cpp
 * @author    Sandro Rigo
 *
 *            The ArchC Team
 *            http://www.archc.org/
 *
 *            Computer Systems Laboratory (LSC)
 *            IC-UNICAMP
 *            http://www.lsc.ic.unicamp.br/
 *
 * @version   version?
 * @date      Mon, 19 Jun 2006 15:33:19 -0300
 *
 * @brief     ArchC support functions
 *
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

//Fix for Cygwin users, that do not have elf.h
#ifdef __CYGWIN__
#include "elf32-tiny.h"
#else
#include <elf.h>
#endif /* __CYGWIN__ */

#include "ac_decoder.h"
#include "ac_resources.H"

#ifdef USE_GDB
#include "ac_gdb.H"
extern AC_GDB *gdbstub;
#endif /* USE_GDB */

#ifdef AC_VERIFY

//Declaring co-verification message queue
key_t key;
int msqid;


#endif

#define BYTE_GET(field)	byte_get((unsigned char *)field, sizeof (*field))
static unsigned int (*byte_get) (unsigned char *, int);


int ac_stop_flag = 0;
int ac_exit_status = 0;

//Declaring trace variables
ofstream trace_file;
bool ac_do_trace;

//!Declaring structures and variables used to build the decoder.
ac_dec_field *fields;
ac_dec_format *formats;
ac_dec_list *dec_list;
ac_dec_instr *instructions;
ac_word buffer[AC_MAX_BUFFER];
unsigned int quant, decode_pc;
unsigned int ac_heap_ptr;
unsigned dec_cache_size;
ac_storage *IMem;

//Command line arguments used inside ArchC Simulator
int ac_argc;                       
char **ac_argv;
//Name of the file containing the application to be loaded.
char *appfilename;


extern "C" {

  //Expand the instruction buffer word by word, the number necessary to read position index
  int ExpandInstrBuffer( int index){

    int read = (index + 1) - quant;
    
    for(int i=0; i<read; i++){
      buffer[quant+i] = IMem->read( decode_pc +  (quant + i)*AC_WORDSIZE/8 );
    }

    quant += read;
    return quant;
  }


  unsigned long long GetBits(ac_word *buffer, int *quant, int last, int quantity, int sign)
  {
    //! Read the buffer using this macro
#define BUFFER(index) ((index<*quant) ? (buffer[index]) : (*quant=ExpandInstrBuffer(index),buffer[index]))

    int first = last - (quantity-1);

    int index_first = first/AC_WORDSIZE;
    int index_last = last/AC_WORDSIZE;

    unsigned long long value = 0;
    int i;

#if AC_PROC_ENDIAN == 1   /* if processor is big-endian */

    //big-endian: first  last
    //          0xAA BB CC DD

    //Read words from first to last
    for (i=index_first; i<=index_last; i++) {
      value <<= AC_WORDSIZE;
      value |= BUFFER(i);
    }

    //Remove bits before last
    value >>= AC_WORDSIZE - (last%AC_WORDSIZE + 1);

#else

    //little-endian: last  first
    //             0xAA BB CC DD

    //Read words from last to first
    for (i=index_last; i>=index_first; i--) {
      value <<= AC_WORDSIZE;
      value |= BUFFER(i);
    }

    //Remove bits before first
    value >>= first%AC_WORDSIZE;

#endif

    //Mask to the size of the field
    value &= ((unsigned)(~0LL)) >> (64-((unsigned)quantity));

    //If signed, sign extend if necessary
    if (sign && ( value >= (unsigned)(1 << (quantity-1)) ))
      value |= (~0LL) << quantity;

    return value;
#undef BUFFER
  }


}

//Read model options before application
void ac_init_opt( int ac, char* av[]){

  extern const char *project_name;
  extern const char *project_file;
  extern const char *archc_version;
  extern const char *archc_options;

  if(ac > 1){
                
    if (!strncmp( av[1], "--help", 6)) {
      cerr << "This is a " << project_name << " architecture simulator generated by ArchC.\n";
      cerr << "For more information visit http://www.archc.org\n\n";
      cerr << "Usage: " << av[0] << " [options]\n\n";
      cerr << "Options:\n";
      cerr << "  --help                  Display this help message\n";
      cerr << "  --version               Display ArchC version and options used when built\n";
#ifndef AC_COMPSIM
      cerr << "  --load=<prog_path>      Load target application\n";
#endif /* AC_COMPSIM */
#ifdef USE_GDB
      cerr << "  --gdb[=<port>]          Enable GDB support\n";
#endif /* USE_GDB */
      exit(1);
    }
    
    if (!strncmp( av[1], "--version", 9)) {
      cout << project_name << " simulator generated by ArchC " << archc_version
           << " from " << project_file << " with options (" << archc_options << ")" << endl;
      exit(0);
    }
  }
}

//Initialize application
void ac_init_app( int ac, char* av[]){

  int size;
  char *appname=0;
  extern int ac_argc;
  extern char** ac_argv;

  ac_argc = ac-1;   //Skiping program's name
  ac_argv = av;
  
  while(ac > 1){
    size = strlen(av[1]);

    //Checking if an application needs to be loaded
    if( (size>11) && (!strncmp( av[1], "--load_obj=", 11))){   //Loading a binary app
      appname = (char*)malloc(size - 10);
      strcpy(appname, av[1]+11);
      ac_argv[1] = appname;
      appfilename = (char*) malloc(sizeof(char)*(size - 6));
      strcpy(appfilename, appname);
      //ac_load_obj( appname );
    }
    else if( (size>7) && (!strncmp( av[1], "--load=", 7))){  //Loading hex file in the ArchC Format
      appname = (char*)malloc(size - 6);
      strcpy(appname, av[1]+7);
      ac_argv[1] = appname;
      appfilename = (char*) malloc(sizeof(char)*(size - 6));
      strcpy(appfilename, appname);
    }
#ifdef USE_GDB    
    if( (size>=5) && (!strncmp( av[1], "--gdb", 5))){ //Enable GDB support
      int port = 0;
      if ( size > 6 )
        { 
          port = atoi( av[1] + 6 );
          if ( ( port > 1024 ) && gdbstub )
            gdbstub->set_port( port );
        }
      if ( gdbstub ) 
        gdbstub->enable();
    }
#endif /* USE_GDB */
    
    ac --;
    av ++;
  }

  if (!appname) {
    AC_ERROR("No application provided.");
    AC_ERROR("Use --load=<prog_path> option to load a target application.");
    AC_ERROR("Try --help for more options.");
    exit(1);
  }
  ac_argv++;

}


/*
  Transforms the data pointed by field (with size) to little endian 
*/
unsigned int byte_get_little_endian(unsigned char *field, int size) 
{
  switch (size)
    {
    case 1:
      return *field;

    case 2:
      return  ((unsigned int) (field[0]))
	|    (((unsigned int) (field[1])) << 8);

    case 4:
      return  ((unsigned long) (field[0]))
	|    (((unsigned long) (field[1])) << 8)
	|    (((unsigned long) (field[2])) << 16)
	|    (((unsigned long) (field[3])) << 24);

    default:
      AC_ERROR("Internal error: Invalid byte size.\n");
      exit(EXIT_FAILURE);
    }
}


/*
  Transforms the data pointed by field (with size) to big endian 
*/
unsigned int byte_get_big_endian(unsigned char *field, int size)
{
  switch (size)
    {
    case 1:
      return *field;

    case 2:
      return ((unsigned int) (field[1])) | (((int) (field[0])) << 8);

    case 4:
      return ((unsigned long) (field[3]))
	|   (((unsigned long) (field[2])) << 8)
	|   (((unsigned long) (field[1])) << 16)
	|   (((unsigned long) (field[0])) << 24);

    default:
      AC_ERROR("Internal error: Invalid byte size.\n");
      exit(EXIT_FAILURE);
    }
}


//Loading binary application
int ac_load_elf(char* filename, char* data_mem, unsigned int data_mem_size)
{
  Elf32_Ehdr    ehdr;
  Elf32_Shdr    shdr;
  Elf32_Phdr    phdr;
  FILE          *fd;
  unsigned int  i;

  /*
   Most code was taken from the Binutils readelf application
  */

  /* Open up the file for reading */
  fd = fopen (filename, "rb");
  if (fd == NULL) {
    AC_ERROR("Opening application file '" << filename << "': " << strerror(errno) << endl);
    exit(EXIT_FAILURE);
  }

 /* Read in the identity array.  */
  if (fread (ehdr.e_ident, EI_NIDENT, 1, fd) != 1) {
    AC_ERROR("Error reading file '" << filename << endl);
    exit(EXIT_FAILURE);
  }
    

  /* Determine how to read the rest of the header.  */
  switch (ehdr.e_ident[EI_DATA]) {
  default: /* fall through */
  case ELFDATANONE: /* fall through */
  case ELFDATA2LSB:
    byte_get = byte_get_little_endian;
    //    byte_put = byte_put_little_endian;
    break;
  case ELFDATA2MSB:
    byte_get = byte_get_big_endian;
    //    byte_put = byte_put_big_endian;
    break;
  }


  // TODO: Consider the case when the file is not 32-bit

  /* Get the rest of the header data */
  if (fread ((unsigned char *)&ehdr.e_type, sizeof (ehdr) - EI_NIDENT, 1, fd) != 1) {
    AC_ERROR("Error reading file %s\n" << filename << endl);
    exit(EXIT_FAILURE);
  }

  ehdr.e_type      = BYTE_GET (&ehdr.e_type);
  ehdr.e_machine   = BYTE_GET (&ehdr.e_machine);
  ehdr.e_version   = BYTE_GET (&ehdr.e_version);
  ehdr.e_entry     = BYTE_GET (&ehdr.e_entry);
  ehdr.e_phoff     = BYTE_GET (&ehdr.e_phoff);
  ehdr.e_shoff     = BYTE_GET (&ehdr.e_shoff);
  ehdr.e_flags     = BYTE_GET (&ehdr.e_flags);
  ehdr.e_ehsize    = BYTE_GET (&ehdr.e_ehsize);
  ehdr.e_phentsize = BYTE_GET (&ehdr.e_phentsize);
  ehdr.e_phnum     = BYTE_GET (&ehdr.e_phnum);
  ehdr.e_shentsize = BYTE_GET (&ehdr.e_shentsize);
  ehdr.e_shnum     = BYTE_GET (&ehdr.e_shnum);
  ehdr.e_shstrndx  = BYTE_GET (&ehdr.e_shstrndx);


  /* Check if the file is in fact an ELF */
  if (   ehdr.e_ident[EI_MAG0] != ELFMAG0
      || ehdr.e_ident[EI_MAG1] != ELFMAG1
      || ehdr.e_ident[EI_MAG2] != ELFMAG2
      || ehdr.e_ident[EI_MAG3] != ELFMAG3)
  {
    fclose(fd);
    return EXIT_FAILURE;
  }


  //Set start address
  ac_start_addr = ehdr.e_entry;
  if (ac_start_addr > data_mem_size) {
    AC_ERROR("the start address of the application is beyond model memory\n");
    fclose(fd);
    exit(EXIT_FAILURE);
  }

  if (ehdr.e_type == ET_EXEC) {
    //It is an ELF file
    AC_SAY("Reading ELF application file: " << filename << endl);

    //Get program headers and load segments
    //    lseek(fd, convert_endian(4,ehdr.e_phoff), SEEK_SET);
    for (i=0; i < ehdr.e_phnum; i++) {

      //Get program headers and load segments
      fseek(fd, ehdr.e_phoff + ehdr.e_phentsize*i, SEEK_SET);
      if (fread(&phdr, sizeof(phdr), 1, fd) != 1) {
        AC_ERROR("reading ELF program header\n");
        fclose(fd);
        exit(EXIT_FAILURE);
      }

      if (BYTE_GET (&phdr.p_type) == PT_LOAD) {
        Elf32_Word j;
        Elf32_Addr p_vaddr  = BYTE_GET (&phdr.p_vaddr);
        Elf32_Word p_memsz  = BYTE_GET (&phdr.p_memsz);
        Elf32_Word p_filesz = BYTE_GET (&phdr.p_filesz);
        Elf32_Off  p_offset = BYTE_GET (&phdr.p_offset);

        //Error if segment greater then memory
        if (data_mem_size < p_vaddr + p_memsz) {
          AC_ERROR("not enough memory in ArchC model to load application.\n");
          fclose(fd);
          exit(EXIT_FAILURE);
        }

        //Set heap to the end of the segment
        if (ac_heap_ptr < p_vaddr + p_memsz) ac_heap_ptr = p_vaddr + p_memsz;

        //Load and correct endian
        fseek(fd, p_offset, SEEK_SET);
        for (j=0; j < p_filesz; j+=sizeof(ac_fetch)) {
          int tmp;
          fread(&tmp, sizeof(ac_fetch), 1, fd);
          *(ac_fetch *) (data_mem + p_vaddr + j) = byte_get((unsigned char *)&tmp, sizeof(ac_fetch));
        }
        memset(data_mem + p_vaddr + p_filesz, 0, p_memsz - p_filesz);
      }

    }
  }
  else if (ehdr.e_type == ET_REL) {

    AC_SAY("Reading ELF relocatable file: " << filename << endl);

    // first load the section name string table
    char *string_table;
    int   shoff  = ehdr.e_shoff;
    short shndx  = ehdr.e_shstrndx;
    short shsize = ehdr.e_shentsize;

    fseek(fd, shoff+(shndx*shsize), SEEK_SET);
    if (fread(&shdr, sizeof(shdr), 1, fd) != 1) {
      AC_ERROR("reading ELF section header\n");
      fclose(fd);
      exit(EXIT_FAILURE);
    }
    
    string_table = (char *) malloc(BYTE_GET (&shdr.sh_size));
    fseek(fd, BYTE_GET (&shdr.sh_offset), SEEK_SET);
    fread(string_table, BYTE_GET (&shdr.sh_size), 1, fd);

    // load .text, .data and .bss sections    
    for (i=0; i < ehdr.e_shnum; i++) {

      fseek(fd, shoff + shsize*i, SEEK_SET);
      
      if (fread(&shdr, sizeof(shdr), 1, fd) != 1) {
        AC_ERROR("reading ELF section header\n");
        fclose(fd);
        exit(EXIT_FAILURE);
      }

      Elf32_Word name = BYTE_GET (&shdr.sh_name);
      if (!strcmp(string_table+name, ".text") ||
          !strcmp(string_table+name, ".data") ||
          !strcmp(string_table+name, ".bss")) {
        
        //        printf("Section %s:\n", string_table+convert_endian(4,shdr.sh_name));

        Elf32_Off  tshoff  = BYTE_GET (&shdr.sh_offset);
        Elf32_Word tshsize = BYTE_GET (&shdr.sh_size);
        Elf32_Addr tshaddr = BYTE_GET (&shdr.sh_addr);

        if (tshsize == 0) {
          // printf("--- empty ---\n");
          continue;
        }

        if (data_mem_size < tshaddr + tshsize) {
          AC_ERROR("not enough memory in ArchC model to load application.\n");
          fclose(fd);
          exit(EXIT_FAILURE);
        }

        //Set heap to the end of the segment
        if (ac_heap_ptr < tshaddr + tshsize) ac_heap_ptr = tshaddr + tshsize;

        if (!strcmp(string_table+BYTE_GET (&shdr.sh_name), ".bss")) {
          memset(data_mem + tshaddr, 0, tshsize);
          //continue;
          break; // .bss is supposed to be the last one
        }

        //Load and correct endian
        fseek(fd, tshoff, SEEK_SET);
        for (Elf32_Word j=0; j < tshsize; j+=sizeof(ac_fetch)) {
          int tmp;
          fread(&tmp, sizeof(ac_fetch), 1, fd);
          *(ac_fetch *) (data_mem + tshaddr + j) = byte_get((unsigned char *)&tmp, sizeof(ac_fetch));

          // printf("0x%08x %08x \n", tshaddr+j, *(ac_fetch *) (data_mem+tshaddr+j));
        }
        //        printf("\n");        
      }

    }
  }
    
  //Close file
  fclose(fd);

  return EXIT_SUCCESS;
}


//Functions used to calculate the instr/s of the simulation

#include <sys/times.h>

struct tms ac_run_times;
clock_t ac_run_start_time;

void InitStat()
{
  ac_run_start_time = times(&ac_run_times);
}

void PrintStat()
{
  clock_t ac_run_real;

  //Print statistics
  fprintf(stderr, "ArchC: Simulation statistics\n");

  ac_run_real = times(&ac_run_times) - ac_run_start_time;
  fprintf(stderr, "    Times: %ld.%02ld user, %ld.%02ld system, %ld.%02ld real\n",
          ac_run_times.tms_utime / 100, ac_run_times.tms_utime % 100,
          ac_run_times.tms_stime / 100, ac_run_times.tms_stime % 100,
          ac_run_real / 100, ac_run_real % 100
          );

  fprintf(stderr, "    Number of instructions executed: %llu\n", ac_instr_counter);

  if (ac_run_times.tms_utime > 5) {
    double ac_mips = (ac_instr_counter * 100) / ac_run_times.tms_utime;
    fprintf(stderr, "    Simulation speed: %.2f K instr/s\n", ac_mips/1000);
  }
  else {
    fprintf(stderr, "    Simulation speed: (too fast to be precise)\n");
  }
}


//Signal handlers for interrupt and segmentation fault (set in ac_start() )
#include <signal.h>
typedef void (*sighandler_t)(int);
void sigint_handler(int signal)
{
  fprintf(stderr, "ArchC: INTERUPTED BY THE SIGNAL %d\n", signal);
  PrintStat();
  exit(EXIT_FAILURE);
}
void sigsegv_handler(int signal)
{
  fprintf(stderr, "ArchC Error: Segmentation fault.\n");
  PrintStat();
  exit(EXIT_FAILURE);
}
void sigusr1_handler(int signal)
{
  fprintf(stderr, "ArchC: Received signal %d. Printing statistics\n", signal);
  PrintStat();
  fprintf(stderr, "ArchC: -------------------- Continuing Simulation ------------------\n");
}

#ifdef USE_GDB
void sigusr2_handler(int signal)
{
  fprintf(stderr, "ArchC: Received signal %d. Starting GDB support\n", signal);
  gdbstub->enable();
  gdbstub->connect();
}
#endif /* USE_GDB */


//Prototypes
namespace ac_begin {void behavior(ac_stage_list a=ac_stage_list(0), unsigned b=0);}
namespace ac_end {void behavior(ac_stage_list a=ac_stage_list(0), unsigned b=0);}


//Initialize simulation
void ac_initialize()
{
#ifdef USE_GDB
  if (gdbstub && !gdbstub->is_disabled()) 
    gdbstub->connect();
#endif /* USE_GDB */
  ac_pc = ac_start_addr;
  ac_begin::behavior();
  cerr << "ArchC: -------------------- Starting Simulation --------------------" << endl;
  InitStat();
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  signal(SIGSEGV, sigsegv_handler);
  signal(SIGUSR1, sigusr1_handler);
#ifdef USE_GDB
  signal(SIGUSR2, sigusr2_handler);
#endif
}


//Stop simulation (may receive exit status)
void ac_stop(int status)
{
  cerr << "ArchC: -------------------- Simulation Finished --------------------" << endl;
  ac_end::behavior();
  ac_stop_flag = 1;
  ac_exit_status = status;
  ac_pc = ~0;
}
