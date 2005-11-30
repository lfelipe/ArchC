/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/*  ArchC Pre-processor generates tools for the described arquitecture
    Copyright (C) 2002-2004  The ArchC Team

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/********************************************************/
/* Acsim.c: The ArchC pre-processor.                    */
/* Author: Sandro Rigo                                  */
/* Date: 16-07-2002                                     */
/*                                                      */
/* The ArchC Team                                       */
/* Computer Systems Laboratory (LSC)                    */
/* IC-UNICAMP                                           */
/* http://www.lsc.ic.unicamp.br                         */
/********************************************************/
//////////////////////////////////////////////////////////
/*!\file acsim.c
  \brief The ArchC pre-processor.

  This file contains functions to control the ArchC 
  to emit the source files that compose the behavioral 
  simulator.
*/
//////////////////////////////////////////////////////////

#include "acsim.h"
#include "stdlib.h"
#include "string.h"

//#define DEBUG_STORAGE

//Defining Traces and Dasm strings
#define PRINT_TRACE "%strace_file << hex << decode_pc << dec <<\"\\n\";\n"
#define PRINT_DASM "%sdasmfile << hex << decode_pc << dec << \": \" << *instr << *format <<\"\t\t(\" << instr->get_name() << \",\" << format->get_name() << \")\" <<endl;\n"

//Command-line options flags
int  ACABIFlag=0;                               //!<Indicates whether an ABI was provided or not
int  ACDasmFlag=0;                              //!<Indicates whether disassembler option is turned on or not
int  ACDebugFlag=0;                             //!<Indicates whether debugger option is turned on or not
int  ACDecCacheFlag=1;                          //!<Indicates whether the simulator will cache decoded instructions or not
int  ACDelayFlag=0;                             //!<Indicates whether delay option is turned on or not
int  ACDDecoderFlag=0;                          //!<Indicates whether decoder structures are dumped or not
//int  ACQuietFlag=0;                             //!<Indicates whether storage update logs are displayed during simulation or not
int  ACStatsFlag=0;                             //!<Indicates whether statistics collection is enable or not
int  ACVerboseFlag=0;                           //!<Indicates whether verbose option is turned on or not
int  ACVerifyFlag=0;                            //!<Indicates whether verification option is turned on or not
int  ACVerifyTimedFlag=0;                       //!<Indicates whether verification option is turned on for a timed behavioral model
int  ACEncoderFlag=0;                           //!<Indicates whether encoder tools will be included in the simulator
int  ACGDBIntegrationFlag=0;                    //!<Indicates whether gdb support will be included in the simulator

char *ACVersion = "2.0alpha1";                        //!<Stores ArchC version number.
char ACOptions[500];                            //!<Stores ArchC recognized command line options
char *ACOptions_p = ACOptions;                  //!<Pointer used to append options in ACOptions
char *arch_filename;                            //!<Stores ArchC arquitecture file

int ac_host_endian;                             //!<Indicates the endianess of the host machine
extern int ac_tgt_endian;                       //!<Indicates the endianess of the host machine
int ac_match_endian;                            //!<Indicates whether host and target endianess match on or not

//! This structure describes one command-line option mapping.
/*!  It is used to manage command line options, following gcc style. */
struct option_map
{
  const char *name;                          //!<The long option's name. 
  const char *equivalent;                    //!<The equivalent short-name for options. 
  const char *arg_desc;                      //!<The option description.
  /* Argument info.  A string of flag chars; NULL equals no options.
     r => argument required.
     o => argument optional.
     * => require other text after NAME as an argument.  */
  const char *arg_info;                      //!<Argument info.  A string of flag chars; NULL equals no options.
};

/*!Decoder object pointer */
ac_decoder_full *decoder;

/*!Storage device used for loading applications */
ac_sto_list* load_device=0;

/*! This is the table of mappings.  Mappings are tried sequentially
  for each option encountered; the first one that matches, wins.  */
struct option_map option_map[] = {
  {"--abi-included"  , "-abi"        ,"Indicate that an ABI for system call emulation was provided." ,"o"},
  {"--disassembler"  , "-dasm"       ,"Dump executing instructions in assembly format (Not completely implemented)." ,"o"},
  {"--debug"         , "-g"          ,"Enable simulation debug features: traces, update logs." ,"o"},
  {"--delay"         , "-dy"          ,"Enable delayed assignments to storage elements." ,"o"},
  {"--dumpdecoder"   , "-dd"         ,"Dump the decoder data structure." ,"o"},
  {"--help"          , "-h"          ,"Display this help message."       , 0},
  //  {"--quiet"         , "-q"          ,".", "o"},
  {"--no-dec-cache"  , "-ndc"        ,"Disable cache of decoded instructions." ,"o"},
  {"--stats"         , "-s"          ,"Enable statistics collection during simulation." ,"o"},
  {"--verbose"       , "-vb"         ,"Display update logs for storage devices during simulation.", "o"},
/*   {"--verify"        , "-v"          ,"Enable co-verification mechanism." ,"o"}, */
  //  {"--verify-timed"  , "-vt"         ,"Enable co-verification mechanism. Timed model." ,"o"},
  {"--version"       , "-vrs"        ,"Display ACSIM version.", 0},
  {"--encoder"       , "-enc"        ,"Use encoder tools with the simulator (beta version).", 0},
  {"--gdb-integration", "-gdb"       ,"Enable support for debbuging programs running on the simulator.", 0},
  0
};


/*! Display the command line options accepted by ArchC.  */
static void DisplayHelp (){
  int i;
  char line[]="====================";

  line[strlen(ACVersion)+1] = '\0';

  printf ("===============================================%s\n", line);
  printf (" This is the ArchC Simulator Generator version %s\n", ACVersion);
  printf ("===============================================%s\n\n", line);
  printf ("Usage: acsim input_file [options]\n");
  printf ("       Where input_file stands for your AC_ARCH description file.\n\n");
  printf ("Options:\n");

  for( i=0; i< ACNumberOfOptions; i++)
    printf ("    %-17s, %-11s %s\n", option_map[i].name, option_map[i].equivalent, option_map[i].arg_desc);

  printf ("\nFor more information please visit www.archc.org\n\n");
}

/*! Function for decoder to get bits from the instruction */
/*    PARSER-TIME VERSION: Should not be called */
unsigned long long GetBits(void *buffer, int *quant, int last, int quantity, int sign)
{
  AC_ERROR("GetBits(): This function should not be called in parser-time for interpreted simulator.\n");
  return 1;
};


//////////////////////////////////////////
/*!Main routine of  ArchC pre-processor.*/
//////////////////////////////////////////
int main( argc, argv )
     int argc;
     char **argv;
{
  extern char *project_name, *isa_filename;
  extern int wordsize;
  extern int fetchsize;
  // Structures to be passed to the decoder generator 
  extern ac_dec_format *format_ins_list;
  extern ac_dec_instr *instr_list;
  // Structures to be passed to the simulator generator 
  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list, *ppipe;
  extern int HaveFormattedRegs;
  extern ac_decoder_full *decoder;

  //Uncomment the line bellow if you want to debug the parser.
  //extern int yydebug; 

  int argn,i,j;
  endian a, b;

  int error_flag=0;

  //Uncomment the line bellow if you want to debug the parser.
  //yydebug =1; 

  //Initializes the pre-processor
  acppInit();

  ++argv, --argc;  /* skip over program name */

  //First argument must be the file or --help option.
  if ( argc > 0 ){
    if( !strcmp(argv[0], "--help") | !strcmp(argv[0], "-h")){
      DisplayHelp();
      return 0;
    }

    if( !strcmp(argv[0], "--version") | !strcmp(argv[0], "-vrs")){
      printf("This is ArchC version %s\n", ACVersion);
      return 0;
    }

    else{
      if(!acppLoad(argv[0])){
        AC_ERROR("Invalid input file: %s\n", argv[0]);
        printf("   Try acsim --help for more information.\n");
        return EXIT_FAILURE;
      }
      arch_filename = argv[0];
    }
  }
  else{
    AC_ERROR("No input file provided.\n");
    printf("   Try acsim --help for more information.\n");
    return EXIT_FAILURE;
  }


  ++argv, --argc;  /* skip over arch file name */

  if( argc > 0){
 
    argn = argc;
    /* Handling command line options */
    for(j= 0; j<argn; j++ ){
                        
      /* Searching option map.*/
      for( i=0; i<ACNumberOfOptions; i++){
                                
        if( (!strcmp(argv[0], option_map[i].name)) || (!strcmp(argv[0], option_map[i].equivalent))){

          switch (i)
            {
            case OPABI:
              ACABIFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPDasm:
              ACDasmFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPDebug:
              ACDebugFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPDelay:
              ACDelayFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPDDecoder:
              ACDDecoderFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
              /* case OPQuiet: */
              /*   ACQuietFlag = 1; */
              /*   ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]); */
              /*   break; */
            case OPDecCache:
              ACDecCacheFlag = 0;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;

            case OPStats:
              ACStatsFlag = 1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPVerbose:
              ACVerboseFlag = 1;
              AC_MSG("Simulator running on verbose mode.\n");
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
/*             case OPVerify: */
/*               ACVerifyFlag =1; */
/*               AC_MSG("Simulator running on co-verification mode. Use it ONLY along with the ac_verifier tool.\n"); */
/*               ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]); */
/*               break; */
              /* case OPVerifyTimed: */
              /*   ACVerifyFlag = ACVerifyTimedFlag = 1; */
              /*   AC_MSG("Co-verification is turned on, running on timed mode.\n"); */
              /*   ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]); */
              /*   break; */
            case OPEncoder:
              ACEncoderFlag = 1;
              AC_MSG("Simulator will have encoder extra tools (beta version).\n");
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);
              break;
            case OPGDBIntegration:
              ACGDBIntegrationFlag=1;
              ACOptions_p += sprintf( ACOptions_p, "%s ", argv[0]);

            default:
              break;
            }
          ++argv, --argc;  /* skip over founded argument */
                                
          break;
        }
      }
    }
  }

  if(argc >0){
    AC_ERROR("Invalid argument %s.\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  //Loading Configuration Variables
  ReadConfFile();

  /*Parsing Architecture declaration file. */
  AC_MSG("Parsing AC_ARCH declaration file: %s\n", arch_filename);
  if( acppRun()){
    AC_ERROR("Parser terminated unsuccessfully.\n");
    error_flag =1;
    return EXIT_FAILURE;
  }
  acppUnload();


  /* Opening ISA File */
  if(isa_filename == NULL)
    AC_ERROR("No ISA file defined");
       
  if( !acppLoad(isa_filename)){
    AC_ERROR("Could not open ISA input file: %s", isa_filename);
    AC_ERROR("Parser terminated unsuccessfully.\n");
    return EXIT_FAILURE;
  }
      
       
  //Parsing ArchC ISA declaration file. 
  AC_MSG("Parsing AC_ISA declaration file: %s\n", isa_filename);
  if( acppRun()){
    AC_ERROR("Parser terminated unsuccessfully.\n");
    error_flag =1;
  }
  acppUnload();

  if (error_flag)
    return EXIT_FAILURE;
  else{

    if( wordsize == 0){
      AC_MSG("Warning: No wordsize defined. Default value is 32 bits.\n");
      wordsize = 32;
    }

    if( fetchsize == 0){
      //AC_MSG("Warning: No fetchsize defined. Default is to be equal to wordsize (%d).\n", wordsize);
      fetchsize = wordsize;
    }

    //Testing host endianess.
    a.i = 255;
    b.c[0] = 0;
    b.c[1] = 0;
    b.c[2] = 0;
    b.c[3] = 255;

    if( a.i == b.i )
      ac_host_endian = 1;    //Host machine is big endian
    else
      ac_host_endian = 0;   //Host machine is little endian

    ac_match_endian = (ac_host_endian == ac_tgt_endian);


    //Creating decoder to get Field Number info and write its static version.
    //This object will be used by some Create functions.
    if( ACDDecoderFlag ){
      AC_MSG("Dumping decoder structures:\n");
      ShowDecInstr(instr_list);
      ShowDecFormat(format_ins_list);

      printf("\n\n");
    }
    decoder = CreateDecoder(format_ins_list, instr_list);

    if( ACDDecoderFlag )
      ShowDecoder(decoder -> decoder, 0);


    //Creating Resources Header File
    CreateArchHeader();
    CreateArchRefHeader();
    //Creating Resources Impl File
    CreateArchImpl();
    /*CreateResourceImpl();*/
    CreateArchRefImpl();
    //Creating ISA Header File
    CreateISAHeader();
    /* Creating _instruction.H file */
    CreateInstructionHeader();
    /* Creating format header files. */
    CreateFormatHeaders();
    //Creating ARCH Header File
    CreateARCHHeader();
    //Creating ARCH Impl File
/*     CreateARCHImpl(); */

    //Now, declare stages if a pipeline was declared
    //Otherwise, declare one sc_module to simulate the processor datapath
    //OBS: For pipelined architectures there must be a stage_list or a pipe_list,
    //     but never both of them.

    if( stage_list  ){  //List of ac_stage declarations. Used only for single pipe archs

      //Creating Stage Module Header Files
      CreateStgHeader(stage_list, NULL);
      //Creating Stage Module Implementation Files
      CreateStgImpl(stage_list, NULL);

    }
    else if( pipe_list ){  //Pipeline list exist. Used for ac_pipe declarations.
                        
      for( ppipe = pipe_list; ppipe != NULL; ppipe = ppipe->next ){

        //Creating Stage Module Header Files
        CreateStgHeader( ppipe->stages, ppipe->name );

        //Creating Stage Module Implementation Files
        CreateStgImpl(ppipe->stages, ppipe->name);
      }
    }
    else{    //No pipe was declared.

      //Creating Processor Files
      CreateProcessorHeader();
      CreateProcessorImpl();
    }   
      
      
    if( HaveFormattedRegs ){
      //Creating Formatted Registers Header and Implementation Files.
      CreateRegsHeader();
      //CreateRegsImpl();  This is not used anymore.
    }

    //Creating co-verification class header file.
    //if( ACVerifyFlag )
    //  CreateCoverifHeader();

    //Creating Simulation Statistics class header file.
    if( ACStatsFlag )
      CreateStatsHeader();

    //Creating model syscall header file.
    if( ACABIFlag )
      CreateArchSyscallHeader();

    /* Create the template for the .cpp instruction and format behavior file */
    CreateImplTmpl();

    /* Creating Parameters Header File */
    CreateParmHeader();

    /* Create the template for the main.cpp  file */
    CreateMainTmpl();

    /* Create the Makefile */
    CreateMakefile();
                
    /* Create dummy functions to use if real ones are undefined */
                
    //Issuing final messages to the user.
    AC_MSG("%s model files generated.\n", project_name);
    if( ACDasmFlag)
      AC_MSG("Disassembler file is: %s.dasm\n", project_name);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////////
// Create functions ...                                                           //
// These Functions are used to create the behavioral simulato files               //
// All of them use structures built by the parser.                                //
////////////////////////////////////////////////////////////////////////////////////

/*!Create ArchC Resources Header File */
void CreateArchHeader() {

  extern ac_pipe_list *pipe_list;
  extern ac_sto_list *storage_list;
  extern ac_stg_list *stage_list;
  extern char* project_name;

  extern int HaveFormattedRegs, HaveMultiCycleIns, HaveMemHier, reg_width;

  ac_sto_list *pstorage;
  ac_stg_list *pstage;
  char Globals[5000];
  char *Globals_p = Globals;
  ac_pipe_list *ppipe;

  // [ARCHC_2_0]
  // Mudar todas as ocorrências de 'ac_resources' para 'nomeproc_arch'.
  FILE *output;
  char filename[256];

  sprintf(filename, "%s_arch.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, "ArchC Resources header file.");
  fprintf( output, "#ifndef  _%s_ARCH_H\n", project_name);
  fprintf( output, "#define  _%s_ARCH_H\n\n", project_name);

  fprintf( output, "#include  \"%s_parms.H\"\n", project_name);
  fprintf( output, "#include  \"ac_arch_dec_if.H\"\n");
  fprintf( output, "#include  \"ac_storage.H\"\n");
  fprintf( output, "#include  \"ac_memport.H\"\n");
  fprintf( output, "#include  \"ac_regbank.H\"\n");

  if( ACStatsFlag )
    fprintf( output, "#include  \"ac_stats.H\"\n");

  if( HaveFormattedRegs )
    fprintf( output, "#include  \"ac_fmt_regs.H\"\n");
  fprintf( output, " \n");

  if( ACGDBIntegrationFlag ) {
    fprintf( output, "#ifndef PORT_NUM\n" );
    fprintf( output, "#define PORT_NUM 5000 // Port to which GDB binds for remote debugging\n" );
    fprintf( output, "#endif /* PORT_NUM */\n" );
  }
  
  //Declaring Architecture Resources class.
  COMMENT(INDENT[0],"ArchC class for model-specific architectural resources.\n");
  fprintf( output, "class %s_arch : public ac_arch_dec_if<%s_parms::ac_word, %s_parms::ac_Hword> {\n", project_name, project_name, project_name);
  fprintf( output, "public:\n");
  fprintf( output, " \n");

  if( ACStatsFlag ){
    COMMENT(INDENT[1],"Statistics Object.");
    fprintf( output, "%sac_stats ac_sim_stats;\n", INDENT[1]);
    /* Globals_p += sprintf( Globals_p, "extern ac_stats &ac_sim_stats;\n"); */
  }

  /* Declaring storage devices */
  COMMENT(INDENT[1],"Storage Devices.");
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

    switch( pstorage->type ){

    case REG:

      //Formatted registers have a special class.
      if( pstorage->format != NULL ){
        fprintf( output, "%sac_%s %s;\n", INDENT[1], pstorage->name, pstorage->name);
      }
      else{
        fprintf( output, "%sac_reg<unsigned> %s;\n", INDENT[1], pstorage->name);      
      }
      break;
                        
    case REGBANK:
      //Emiting register bank. Checking is a register width was declared.
      switch( (unsigned)reg_width ){
      case 0:
        fprintf( output, "%sac_regbank<%d, %s_parms::ac_word, %s_parms::ac_Dword> %s;\n", INDENT[1], pstorage->size, project_name, project_name, pstorage->name);
        break;
      case 8:
        fprintf( output, "%sac_regbank<%d, unsigned char, unsigned char> %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 16:
        fprintf( output, "%sac_regbank<%d, unsigned short, unsigned char> %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 32:
        fprintf( output, "%sac_regbank<%d, unsigned long, unsigned short> %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 64:
        fprintf( output, "%sac_regbank<%d, unsigned long long, unsigned long> %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      default:
        AC_ERROR("Register width not supported: %d\n", reg_width);
        break;
      }

      break;

    case CACHE:
    case ICACHE:
    case DCACHE:

      if( !HaveMemHier ) { //It is a generic cache. Just emit a base container object.
        fprintf( output, "%sac_storage %s_stg;\n", INDENT[1], pstorage->name);
        fprintf(output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword> %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      }
      else{
        //It is an ac_cache object.
        fprintf( output, "%sac_cache %s;\n", INDENT[1], pstorage->name);
      }

      break;

    case MEM:

      if( !HaveMemHier ) { //It is a generic mem. Just emit a base container object.
        fprintf( output, "%sac_storage %s_stg;\n", INDENT[1], pstorage->name);
        fprintf(output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword> %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      }
      else{
        //It is an ac_mem object.
        fprintf( output, "%sac_mem %s;\n", INDENT[1], pstorage->name);
      }

      break;

      
    default:
      fprintf( output, "%sac_storage %s_stg;\n", INDENT[1], pstorage->name);
      fprintf(output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword> %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      break;
    }
  }

  //Disassembler file
  if( ACDasmFlag )
    fprintf( output, "%sofstream dasmfile;\n", INDENT[1]);

  fprintf( output, " \n");
  fprintf( output, "\n");

  //ac_resources constructor declaration
  COMMENT(INDENT[1],"Constructor.");
  fprintf( output, "%sexplicit %s_arch();\n", INDENT[1], project_name);
      
  fprintf( output, "\n");

  //We have different methods for pipelined and non-pipelined archs
  if(stage_list){
    COMMENT(INDENT[1],"Stall method.");
    fprintf( output, "%svoid ac_stall( char *stage ){\n", INDENT[1]);
                
    for( pstage = stage_list; pstage != NULL; pstage=pstage->next)
      if( pstage->next ){
        if( pstage->id ==1 )
          fprintf( output, "%sif( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name);
        else
          fprintf( output, "%selse if( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name);
                                
        fprintf( output, "%s%s_stall = 1;\n", INDENT[3], pstage->name);
      }
    fprintf( output, "%s};\n", INDENT[1]);
  }
  else  if(pipe_list){
    COMMENT(INDENT[1],"Stall method.");
    fprintf( output, "%svoid ac_stall( char *stage ){\n", INDENT[1]);
    for( ppipe = pipe_list; ppipe!=NULL; ppipe= ppipe->next ){
			
      for( pstage = ppipe->stages; pstage != NULL; pstage=pstage->next)
        if( pstage->next ){
          if( pstage->id ==1 )
            fprintf( output, "%sif( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name);
          else
            fprintf( output, "%selse if( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name);
          fprintf( output, "%s%s_%s_stall = 1;\n", INDENT[3], ppipe->name, pstage->name);
        }
    }
    fprintf( output, "%s};\n", INDENT[1]);
  }

  // [ARCHC_2_0]
  // Wait, release e annul vão para o ac_arch.H ?

/*   //Wait Method */
/*   COMMENT(INDENT[1],"Put the simulator on the wait state."); */
/*   fprintf( output, "%sstatic void ac_wait(){\n", INDENT[1]); */
/*   fprintf( output, "%sac_wait_sig = 1;\n", INDENT[2]); */
/*   fprintf( output, "%s};\n", INDENT[1]); */
/*   fprintf( output, "\n"); */

/*   //Release Method */
/*   COMMENT(INDENT[1],"Release the simulator from the wait state."); */
/*   fprintf( output, "%sstatic void ac_release(){\n", INDENT[1]); */
/*   fprintf( output, "%sac_wait_sig = 0;\n", INDENT[2]); */

/*   fprintf( output, "%s};\n", INDENT[1]); */
/*   fprintf( output, "\n"); */

/*   //Annulation Method */
/*   //There is no annul signal to pipelined archs. They use flush. */
/*   if( !stage_list && !pipe_list ){ */
/*     COMMENT(INDENT[1],"Annul the current instruction."); */
/*     fprintf( output, "%sstatic void ac_annul(){\n", INDENT[1]); */
/*     fprintf( output, "%sac_annul_sig = 1;\n", INDENT[2]); */
/*     fprintf( output, "%s};\n", INDENT[1]); */
/*   } */
/*   fprintf( output, "\n"); */

/*   //TODO: COlocar como opcao de linha de comando */
/*   //ILP method */
/*   COMMENT(INDENT[1],"Force Paralelism."); */
/*   fprintf( output, "%sstatic void ac_parallel( ){\n", INDENT[1]); */
/*   fprintf( output, "%sac_parallel_sig = 1;\n", INDENT[2]); */
/*   fprintf( output, "%s};\n", INDENT[1]); */
/*   fprintf( output, "\n"); */
      
/*   if(stage_list){ */
/*     COMMENT(INDENT[1],"Flush method."); */
/*     //We have different methods for pipelined and non-pipelined archs */
/*     fprintf( output, "%sstatic void ac_flush( char *stage ){\n", INDENT[1]); */
/*     //AC_STAGE version */
/*     for( pstage = stage_list; pstage != NULL; pstage=pstage->next) */
/*       if( pstage->next ){ */
/*         if( pstage->id ==1 ) */
/*           fprintf( output, "%sif( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name); */
/*         else */
/*           fprintf( output, "%selse if( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name); */

/*         fprintf( output, "%s%s_flush = 1;\n", INDENT[3], pstage->name); */
/*       } */

/*     fprintf( output, "%s};\n", INDENT[1]); //end of flush method */
/*   } */
/*   //AC_PIPE version */
/*   else  if(pipe_list){ */
	
/*     COMMENT(INDENT[1],"Flush method."); */
/*     fprintf( output, "%sstatic void ac_flush( char *stage ){\n", INDENT[1]); */
/*     for( ppipe = pipe_list; ppipe!=NULL; ppipe= ppipe->next ){ */
			
/*       for( pstage = ppipe->stages; pstage != NULL; pstage=pstage->next) */
/*         if( pstage->next ){ */
/*           if( pstage->id ==1 ) */
/*             fprintf( output, "%sif( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name); */
/*           else */
/*             fprintf( output, "%selse if( !strcmp( \"%s\", stage ) )\n", INDENT[2], pstage->name); */
/*           fprintf( output, "%s%s_%s_flush = 1;\n", INDENT[3], ppipe->name, pstage->name); */
/*         } */
/*     } */
/*     fprintf( output, "%s};\n", INDENT[1]);//end of flush method */
/*   } */

  if(ACVerifyFlag){
    COMMENT(INDENT[1],"Set co-verification msg queue.");
    fprintf( output, "%svoid set_queue(char *exec_name);\n", INDENT[1]);
  }

  COMMENT(INDENT[1],"Module initialization method.");
  fprintf( output, "%svirtual void init(int ac, char* av[], double period) = 0;\n\n", INDENT[1]);

  COMMENT(INDENT[1],"Module finalization method.");
  fprintf( output, "%svirtual void stop(int status = 0) = 0;\n\n", INDENT[1]);

  COMMENT(INDENT[1],"Virtual destructor declaration.");
  fprintf( output, "%svirtual ~%s_arch() {};\n\n", INDENT[1]);


  fprintf( output, "};\n\n"); //End of ac_resources class


  // [ARCHC_2_0]
  // Não gerar global aliases!!!!!
  
/*   COMMENT(INDENT[0],"Global aliases for resources."); */
/*   fprintf( output, "%s\n", Globals); */

  fprintf( output, "#endif  //_%s_ARCH_H\n", project_name);
  fclose( output); 

}

/*!Create ArchC Resources Reference Header File */
void CreateArchRefHeader() {

  extern ac_pipe_list *pipe_list;
  extern ac_sto_list *storage_list;
  extern ac_stg_list *stage_list;
  extern char* project_name;

  extern int HaveFormattedRegs, HaveMultiCycleIns, HaveMemHier, reg_width;

  ac_sto_list *pstorage;
  ac_stg_list *pstage;
  char Globals[5000];
  char *Globals_p = Globals;
  ac_pipe_list *ppipe;

  // [ARCHC_2_0]
  // Mudar todas as ocorrências de 'ac_resources' para 'nomeproc_arch'.
  FILE *output;
  char filename[256];

  sprintf(filename, "%s_arch_ref.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, "ArchC Resources header file.");
  fprintf( output, "#ifndef  _%s_ARCH_REF_H\n", project_name);
  fprintf( output, "#define  _%s_ARCH_REF_H\n\n", project_name);

  // [ARCHC_2_0]
  // Incluir também "ac_arch.H"

  fprintf( output, "#include  \"%s_parms.H\"\n", project_name);
  fprintf( output, "#include  \"ac_arch_ref.H\"\n");
  fprintf( output, "#include  \"ac_memport.H\"\n");
  fprintf( output, "#include  \"ac_reg.H\"\n");
  fprintf( output, "#include  \"ac_regbank.H\"\n\n");
  
  COMMENT(INDENT[0], "Forward class declaration, needed to compile.");
  fprintf(output, "class %s_arch;\n\n", project_name);

  //Declaring Architecture Resource references class.
  COMMENT(INDENT[0],"ArchC class for model-specific architectural resources.");
  fprintf( output, "class %s_arch_ref : public ac_arch_ref<%s_parms::ac_word, %s_parms::ac_Hword> {\n", project_name, project_name, project_name);
  fprintf( output, "public:\n");
  fprintf( output, " \n");

  /* Declaring storage devices */
  COMMENT(INDENT[1],"Storage Devices.");
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

    switch( pstorage->type ){

    case REG:

      //Formatted registers have a special class.
      if( pstorage->format != NULL ){
        fprintf( output, "%sac_%s& %s;\n", INDENT[1], pstorage->name, pstorage->name);
      }
      else{
        fprintf( output, "%sac_reg<unsigned>& %s;\n", INDENT[1], pstorage->name);      
      }
      break;
                        
    case REGBANK:
      //Emiting register bank. Checking is a register width was declared.
      switch( (unsigned)reg_width ){
      case 0:
        fprintf( output, "%sac_regbank<%d, %s_parms::ac_word, %s_parms::ac_Dword>& %s;\n", INDENT[1], pstorage->size, project_name, project_name, pstorage->name);
        break;
      case 8:
        fprintf( output, "%sac_regbank<%d, unsigned char, unsigned char>& %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 16:
        fprintf( output, "%sac_regbank<%d, unsigned short, unsigned char>& %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 32:
        fprintf( output, "%sac_regbank<%d, unsigned long, unsigned short>& %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      case 64:
        fprintf( output, "%sac_regbank<%d, unsigned long long, unsigned long>& %s;\n", INDENT[1], pstorage->size, pstorage->name);
        break;
      default:
        AC_ERROR("Register width not supported: %d\n", reg_width);
        break;
      }

      break;

    case CACHE:
    case ICACHE:
    case DCACHE:

      if( !HaveMemHier ) { //It is a generic cache. Just emit a base container object.
        fprintf( output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword>& %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      }
      else{
        //It is an ac_cache object.
        fprintf( output, "%sac_cache& %s;\n", INDENT[1], pstorage->name);
      }

      break;

    case MEM:

      if( !HaveMemHier ) { //It is a generic mem. Just emit a base container object.
        fprintf( output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword>& %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      }
      else{
        //It is an ac_mem object.
        fprintf( output, "%sac_mem& %s;\n", INDENT[1], pstorage->name);
      }

      break;

      
    default:
      fprintf( output, "%sac_memport<%s_parms::ac_word, %s_parms::ac_Hword>& %s;\n", INDENT[1], project_name, project_name, pstorage->name);
      break;
    }
  }

  fprintf(output, "\n");

  //ac_resources constructor declaration
  COMMENT(INDENT[1],"Constructor.");
  fprintf(output, "%s %s_arch_ref(%s_arch& arch);\n", INDENT[1], project_name, project_name);
      
  fprintf( output, "\n");

  fprintf( output, "};\n\n"); //End of _arch_ref class

  fprintf( output, "#endif  //_%s_ARCH_REF_H\n", project_name);
  fclose( output); 

}


/*!Create ArchC Resources Reference Implementation File */
void CreateArchRefImpl() {

  extern ac_pipe_list *pipe_list;
  extern ac_sto_list *storage_list;
  extern ac_stg_list *stage_list;
  extern char* project_name;

  extern int HaveFormattedRegs, HaveMultiCycleIns, HaveMemHier, reg_width;

  ac_sto_list *pstorage;
  ac_stg_list *pstage;
  char Globals[5000];
  char *Globals_p = Globals;
  ac_pipe_list *ppipe;

  FILE *output;
  char filename[256];

  sprintf(filename, "%s_arch_ref.cpp", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, "ArchC Resources implementation file.");

  // [ARCHC_2_0]
  // Incluir também "ac_arch.H"

  fprintf( output, "#include  \"%s_arch.H\"\n", project_name);
  fprintf( output, "#include  \"%s_arch_ref.H\"\n\n", project_name);

  //Declaring Architecture Resource references class.
  COMMENT(INDENT[0],"/Default constructor.");
  fprintf(output,
          "%s_arch_ref::%s_arch_ref(%s_arch& arch) : ac_arch_ref<%s_parms::ac_word, %s_parms::ac_Hword>(arch),\n",
          project_name, project_name, project_name, project_name, project_name);

  /* Declaring storage devices */
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    fprintf(output, "%s%s(arch.%s)", INDENT[1], pstorage->name,
            pstorage->name);
    if (pstorage->next != NULL) {
      fprintf(output, ", ");
    }
  }
  fprintf(output, " {}\n\n");
  fclose( output); 

}


// [ARCHC_2_0]
// ac_types.H desaparece.
// Criar aqui os arquivos para _instruction e para _tipos...

//!Create ArchC Types Header File
void CreateTypeHeader() {


  FILE *output;
  char filename[] = "ac_types.H";

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }


  print_comment( output, "ArchC Types header file.");
  fprintf( output, "#ifndef  _AC_TYPES_H\n");
  fprintf( output, "#define  _AC_TYPES_H\n\n");
  
  fprintf( output, "#include  <systemc.h>\n");
  fprintf( output, "#include  \"ac_storage.H\"\n");
  fprintf( output, "#include  \"ac_resources.H\"\n");
  fprintf( output, "#include  \"archc.H\"\n\n");

  //Declaring abstract stage class.
  COMMENT(INDENT[0],"ArchC abstract class for pipeline stages.\n");
  fprintf( output, "class ac_stage: public sc_module, public ac_resources {\n");
  fprintf( output, "public:\n");
  fprintf( output, "%svirtual void behavior(){};\n", INDENT[1]);
  fprintf( output, "%sSC_CTOR( ac_stage ){};\n", INDENT[1]);
  fprintf( output, "};\n\n");

  //Declaring begin pseudo instruction class
  COMMENT(INDENT[0],"Pseudo instruction begin.\n"); 
  fprintf( output, "class ac_begin: public ac_resources {\n");
  fprintf( output, "public:\n");
  fprintf( output, "  static void behavior(ac_stage_list  stage = (ac_stage_list)0, unsigned cycle=0);\n");
  fprintf( output, "};\n\n");

  //Declaring end pseudo instruction class
  COMMENT(INDENT[0],"Pseudo instruction end.\n"); 
  fprintf( output, "class ac_end: public ac_resources {\n");
  fprintf( output, "public:\n");
  fprintf( output, "  static void behavior(ac_stage_list  stage = (ac_stage_list)0, unsigned cycle=0);\n");
  fprintf( output, "};\n\n");

  //Declaring abstract instruction class.
  EmitGenInstrClass( output );
/*   COMMENT(INDENT[0],"ArchC abstract class for instructions.\n"); */
/*   fprintf( output, "class ac_instruction: public ac_resources {\n"); */
/*   fprintf( output, "protected:\n"); */
/*   fprintf( output, "%schar* ac_instr_name;\n", INDENT[1]); */
/*   fprintf( output, "%schar* ac_instr_mnemonic;\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned ac_instr_size;\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned ac_instr_cycles;\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned ac_instr_min_latency;\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned ac_instr_max_latency;\n", INDENT[1]); */
/*   fprintf( output, "public:\n"); */

/*   fprintf( output, "%sac_instruction( char* name, char* mnemonic, unsigned min, unsigned max ){ ac_instr_name = name ; ac_instr_mnemonic = mnemonic; ac_instr_min_latency = min, ac_instr_max_latency =max;}\n", INDENT[1]); */
/*   fprintf( output, "%sac_instruction( char* name, char* mnemonic ){ ac_instr_name = name ; ac_instr_mnemonic = mnemonic;}\n", INDENT[1]); */
/*   fprintf( output, "%sac_instruction( char* name ){ ac_instr_name = name ;}\n", INDENT[1]); */
/*   fprintf( output, "%sac_instruction( ){ ac_instr_name = \"NULL\";}\n", INDENT[1]); */

/*   fprintf( output, "%svirtual void behavior(ac_stage_list  stage = (ac_stage_list)0, unsigned cycle=0);\n", INDENT[1]);    */
/*   fprintf( output, "%svirtual void set_fields( ac_instr instr){};\n", INDENT[1]); */

/*   fprintf( output, "%svoid set_cycles( unsigned c){ ac_instr_cycles = c;}\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned get_cycles(){ return ac_instr_cycles;}\n", INDENT[1]); */

/*   fprintf( output, "%svoid set_min_latency( unsigned c){ ac_instr_min_latency = c;}\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned get_min_latency(){ return ac_instr_min_latency;}\n", INDENT[1]); */

/*   fprintf( output, "%svoid set_max_latency( unsigned c){ ac_instr_max_latency = c;}\n", INDENT[1]); */
/*   fprintf( output, "%sunsigned get_max_latency(){ return ac_instr_max_latency;}\n", INDENT[1]); */

/*   fprintf( output, "%sunsigned get_size() {return ac_instr_size;}\n", INDENT[1]); */
/*   fprintf( output, "%svoid set_size( unsigned s) {ac_instr_size = s;}\n", INDENT[1]); */

/*   fprintf( output, "%schar* get_name() {return ac_instr_name;}\n", INDENT[1]); */
/*   fprintf( output, "%svoid set_name( char* name) {ac_instr_name = name;}\n", INDENT[1]); */

/*   fprintf( output, "%svirtual  void print (ostream & os) const{};\n", INDENT[1]); */

/*   fprintf( output, "%sfriend ostream& operator<< (ostream &os,const ac_instruction &ins){\n", INDENT[1]); */
/*   fprintf( output, "%sins.print(os);\n", INDENT[1]); */
/*   fprintf( output, "%sreturn os;\n", INDENT[1]); */
/*   fprintf( output, "%s};\n", INDENT[1]); */

/*   fprintf( output, "};\n\n"); */
  
  EmitFormatClasses( output );
  EmitInstrClasses(output ); 

  fprintf( output, "\n\n");
  fprintf( output, "#endif  //_AC_TYPES_H\n");
  fclose( output); 

}

// [ARCHC_2_0]
// ac_parms.H não se altera.

//!Creates Decoder Header File
void CreateParmHeader() {

  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list;
  extern ac_dec_format *format_ins_list;
  extern int instr_num;
  extern int declist_num;
  extern int format_num, largest_format_size;
  extern int wordsize, fetchsize, HaveMemHier, HaveCycleRange;
  extern ac_sto_list* load_device;

  extern ac_decoder_full *decoder;
  extern char* project_name;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;

  char filename[256];

  
  //! File containing decoding structures 
  FILE *output;

  sprintf(filename, "%s_parms.H", project_name);
  if ( !(output = fopen(filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, "ArchC Parameters header file.");
  fprintf( output, "#ifndef  _%s_PARMS_H\n", project_name);
  fprintf( output, "#define  _%s_PARMS_H\n\n", project_name);

  /* options defines */
  if( ACVerboseFlag || ACVerifyFlag )
    fprintf( output, "#define  AC_UPDATE_LOG \t //!< Update log generation turned on.\n");

  if( ACVerboseFlag )
    fprintf( output, "#define  AC_VERBOSE \t //!< Indicates Verbose mode. Where update logs are dumped on screen.\n");

  if( ACVerifyFlag )
    fprintf( output, "#define  AC_VERIFY \t //!< Indicates that co-verification is turned on.\n");
      
  if( ACStatsFlag )
    fprintf( output, "#define  AC_STATS \t //!< Indicates that statistics collection is turned on.\n");
      

  if( ACDasmFlag )
    fprintf( output, "#define  AC_DISASSEMBLER \t //!< Indicates that disassembler option is turned on.\n\n");

  if( ACDebugFlag )
    fprintf( output, "#define  AC_DEBUG \t //!< Indicates that debug option is turned on.\n\n");

  if( ACDelayFlag )
    fprintf( output, "#define  AC_DELAY \t //!< Indicates that delay option is turned on.\n\n");

  if( HaveMemHier )
    fprintf( output, "#define  AC_MEM_HIERARCHY \t //!< Indicates that a memory hierarchy was declared.\n\n");

  if( HaveCycleRange )
    fprintf( output, "#define  AC_CYCLE_RANGE \t //!< Indicates that cycle range for instructions were declared.\n\n");

  /* parms class definition */
  fprintf(output, "struct %s_parms {\n\n", project_name);

  fprintf( output, "\nstatic const unsigned int AC_DEC_FIELD_NUMBER = %d; \t //!< Number of Fields used by decoder.\n", decoder->nFields);
  fprintf( output, "static const unsigned int AC_DEC_INSTR_NUMBER = %d; \t //!< Number of Instructions declared.\n", instr_num);
  fprintf( output, "static const unsigned int AC_DEC_FORMAT_NUMBER = %d; \t //!< Number of Formats declared.\n", format_num);
  fprintf( output, "static const unsigned int AC_DEC_LIST_NUMBER = %d; \t //!< Number of decodification lists used by decoder.\n", declist_num);
  fprintf( output, "static const unsigned int AC_MAX_BUFFER = %d; \t //!< This is the size needed by decoder buffer. It is equal to the biggest instruction size.\n", largest_format_size/8);
  fprintf( output, "static const unsigned int AC_WORDSIZE = %d; \t //!< Architecture wordsize in bits.\n", wordsize);
  fprintf( output, "static const unsigned int AC_FETCHSIZE = %d; \t //!< Architecture fetchsize in bits.\n", fetchsize);
  fprintf( output, "static const unsigned int AC_MATCH_ENDIAN = %d; \t //!< If the simulated arch match the endian with host.\n", ac_match_endian);
  fprintf( output, "static const unsigned int AC_PROC_ENDIAN = %d; \t //!< The simulated arch is big endian?\n", ac_tgt_endian);
  fprintf( output, "static const unsigned int AC_RAMSIZE = %u; \t //!< Architecture RAM size in bytes (storage %s).\n", load_device->size, load_device->name);
  fprintf( output, "static const unsigned int AC_RAM_END = %u; \t //!< Architecture end of RAM (storage %s).\n", load_device->size, load_device->name);

  fprintf( output, "\n\n");
  COMMENT(INDENT[0],"Word type definitions.");
    
  //Emiting ArchC word types.
  switch( wordsize ){
  case 8:
    fprintf( output, "typedef  unsigned char ac_word; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  unsigned char ac_Uword; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  char ac_Sword; \t //!< Signed word.\n");
    fprintf( output, "typedef  unsigned char ac_Hword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned char ac_UHword; \t //!< Unsigned half word.\n");
    fprintf( output, "typedef  char ac_SHword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned short ac_Dword; \t //!< Signed double word.\n");
    fprintf( output, "typedef  unsigned short ac_UDword; \t //!< Unsigned double word.\n");
    fprintf( output, "typedef  short ac_SDword; \t //!< Signed double word.\n");
    break;
  case 16:
    fprintf( output, "typedef  unsigned short int ac_word; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  unsigned short int ac_Uword; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  short int ac_Sword; \t //!< Signed word.\n");
    fprintf( output, "typedef  unsigned char ac_Hword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned char ac_UHword; \t //!< Unsigned half word.\n");
    fprintf( output, "typedef  char ac_SHword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned int ac_Dword; \t //!< Signed double word.\n");
    fprintf( output, "typedef  unsigned int ac_UDword; \t //!< Unsigned double word.\n");
    fprintf( output, "typedef  int ac_SDword; \t //!< Signed double word.\n");
    break;
  case 32:
    fprintf( output, "typedef  unsigned int ac_word; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  unsigned int ac_Uword; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  int ac_Sword; \t //!< Signed word.\n");
    fprintf( output, "typedef  short int ac_SHword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned short int  ac_UHword; \t //!< Unsigned half word.\n");
    fprintf( output, "typedef  unsigned short int  ac_Hword; \t //!< Unsigned half word.\n");
    fprintf( output, "typedef  unsigned long long ac_Dword; \t //!< Signed double word.\n");
    fprintf( output, "typedef  unsigned long long ac_UDword; \t //!< Unsigned double word.\n");
    fprintf( output, "typedef  long long ac_SDword; \t //!< Signed double word.\n");
    break;
  case 64:
    fprintf( output, "typedef  unsigned long long ac_word; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  unsigned long long ac_Uword; \t //!< Unsigned word.\n");
    fprintf( output, "typedef  long long ac_Sword; \t //!< Signed word.\n");
    fprintf( output, "typedef  int ac_SHword; \t //!< Signed half word.\n");
    fprintf( output, "typedef  unsigned int  ac_UHword; \t //!< Unsigned half word.\n");
    fprintf( output, "typedef  unsigned int  ac_UHword; \t //!< Unsigned half word.\n");
    break;
  default:
    AC_ERROR("Wordsize not supported: %d\n", wordsize);
    break;
  }
      
  fprintf( output, "typedef  char ac_byte; \t //!< Signed byte word.\n");
  fprintf( output, "typedef  unsigned char ac_Ubyte; \t //!< Unsigned byte word.\n");

  fprintf( output, "\n\n");
  COMMENT(INDENT[0],"Fetch type definition.");

  switch( fetchsize ){
  case 8:
    fprintf( output, "typedef  unsigned char ac_fetch; \t //!< Unsigned word.\n");
    break;
  case 16:
    fprintf( output, "typedef  unsigned short int ac_fetch; \t //!< Unsigned word.\n");
    break;
  case 32:
    fprintf( output, "typedef  unsigned int ac_fetch; \t //!< Unsigned word.\n");
    break;
  case 64:
    fprintf( output, "typedef  unsigned long long ac_fetch; \t //!< Unsigned word.\n");
    break;
  default:
    AC_ERROR("Fetchsize not supported: %d\n", fetchsize);
    break;
  }


  fprintf( output, "\n\n");

  //This enum type is used for case identification inside the ac_behavior methods
  fprintf( output, "enum ac_stage_list {");

  if( stage_list ){   //Enum type for pipes declared through ac_stage keyword
    fprintf( output, "%s=1,", stage_list->name);
    pstage = stage_list->next;
    for( pstage = stage_list->next; pstage && pstage->next != NULL; pstage=pstage->next){
      fprintf( output, "%s,", pstage->name);
    }
    fprintf( output, "%s", pstage->name);
  }
  else if(pipe_list){  //Enum type for pipes declared through ac_pipe keyword

    for(ppipe = pipe_list; ppipe!= NULL; ppipe=ppipe->next){

      for(pstage=ppipe->stages; pstage && pstage->next!= NULL; pstage=pstage->next){

        //When we have just one pipe use only the stage name
        if( ppipe->next )
          fprintf( output, "%s_%s=%d,", ppipe->name, pstage->name, pstage->id);
        else
          fprintf( output, "%s=%d,", pstage->name, pstage->id);

      }
      if( ppipe->next )
        fprintf( output, "%s_%s=%d", ppipe->name, pstage->name, pstage->id);  //The last doesn't need a comma
      else
        fprintf( output, "%s=%d", pstage->name, pstage->id);
    }
  }
  else{
    fprintf( output, "ST0");
  }

  //Closing enum declaration
  fprintf( output, "};\n\n");
    
  /* closing class declaration */

  fprintf( output, "};\n\n");
    
  //Create a compiler error if delay assignment is used without the -dy option
  COMMENT(INDENT[0],"Create a compiler error if delay assignment is used without the -dy option");
  fprintf( output, "#ifndef AC_DELAY\n");
  fprintf( output, "extern ac_word ArchC_ERROR___PLEASE_USE_OPTION_DELAY_WHEN_CREATING_SIMULATOR___;\n");
  fprintf( output, "#define delay(a,b) ArchC_ERROR___PLEASE_USE_OPTION_DELAY_WHEN_CREATING_SIMULATOR___\n");
  fprintf( output, "#endif\n");


  fprintf( output, "\n\n");
  fprintf( output, "#endif  //_%s_PARMS_H\n", project_name);

  fclose(output);
}


//!Creates the ISA Header File.
void CreateISAHeader() {

  extern ac_dec_format *format_ins_list;
  extern char *project_name;
  extern ac_dec_instr *instr_list;
  extern int HaveMultiCycleIns;
  extern int wordsize;
  ac_dec_format *pformat;
  ac_dec_instr *pinstr;
  ac_dec_field *pfield;

  char filename[256];
  char description[] = "Instruction Set Architecture header file.";
 
  // File containing ISA declaration
  FILE  *output;

  // [ARCHC_2_0]
  // nome muda para _isa.H

  sprintf( filename, "%s_isa.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  // [ARCHC_2_0]
  // Includes: ac_storage.H ac_reg.H ac_regbank.H arquivos_de_tipos.H
  //           ac_decoder.h ac_parms.H ac_instr_info.H

  print_comment( output, description);
  fprintf( output, "#ifndef _%s_ISA_H\n", project_name);
  fprintf( output, "#define _%s_ISA_H\n\n", project_name);
  fprintf( output, "#include \"%s_parms.H\"\n", project_name);
  fprintf( output, "#include \"ac_instr.H\"\n");
  fprintf( output, "#include \"ac_decoder_rt.H\"\n");
  fprintf( output, "#include \"ac_instr_info.H\"\n");
  fprintf( output, "#include \"%s_arch.H\"\n", project_name);
  fprintf( output, "\n\n");

  /* _isa must inherit from every instruction type class */
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    fprintf(output, "#include \"%s_type_%s.H\"\n",
            project_name, pformat->name);
  }

  fprintf( output, "\n");

  fprintf( output, "class %s_isa: ", project_name,
           project_name);

  /* _isa must inherit from every instruction type class */
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    fprintf( output, "public %s_type_%s", project_name, pformat->name);
    if (pformat->next != NULL) {
      fprintf(output, ", ");
    }
  }
  fprintf(output, " {\n");

  fprintf(output, "private:\n");
  fprintf(output, "typedef ac_instr<%s_parms::AC_DEC_FIELD_NUMBER> ac_instr_t;\n", project_name);

  fprintf(output, "public:\n");

  // [ARCHC_2_0]
  // Não existem objetos para pseudo-instruções.


  // [ARCHC_2_0]
  // Não existem objetos para instrução genérica.

  fprintf( output, "\n");
  fprintf( output, "%sstatic ac_dec_field fields[%s_parms::AC_DEC_FIELD_NUMBER];\n", INDENT[1], project_name);
  fprintf( output, "%sstatic ac_dec_format formats[%s_parms::AC_DEC_FORMAT_NUMBER];\n", INDENT[1], project_name);
  fprintf( output, "%sstatic ac_dec_list dec_list[%s_parms::AC_DEC_LIST_NUMBER];\n", INDENT[1], project_name);
  fprintf( output, "%sstatic ac_dec_instr instructions[%s_parms::AC_DEC_INSTR_NUMBER];\n", INDENT[1], project_name);
  fprintf( output, "%sstatic const ac_instr_info<%s_isa> instr_table[%s_parms::AC_DEC_INSTR_NUMBER + 1];\n", INDENT[1], project_name, project_name);

  fprintf( output, "\n");
  fprintf( output, "%sac_decoder_full* decoder;\n\n", INDENT[1]);

  /* set_fields() method --- UGLY */
  fprintf(output, "%svoid set_fields( ac_instr_t instr ){\n", INDENT[1]);
  /* sets the fields for each format */
  for (pformat = format_ins_list; pformat != NULL; pformat = pformat->next) {
    for (pfield = pformat->fields; pfield != NULL; pfield = pfield->next) {
      fprintf(output, "%s%s_type_%s::%s = instr.get(%d);\n",
              INDENT[2],
              project_name,
              pformat->name,
              pfield->name,
              pfield->id);
    }
  }

  fprintf(output, "%s};\n", INDENT[1]);

  //Emiting Constructor.
  fprintf( output,"\n");
  COMMENT(INDENT[1], "Constructor.\n");
  fprintf( output,"%s%s_isa(%s_arch& ref) : %s_instruction(ref),\n", INDENT[1],
           project_name, project_name, project_name);
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    fprintf( output, "%s%s_type_%s(ref)", INDENT[2], project_name, pformat->name);
    if (pformat->next != NULL) {
      fprintf(output, ",\n");
    }
  }
  fprintf(output, " {\n\n");

  COMMENT(INDENT[2], "Building Decoder.");
  fprintf( output,"%sdecoder = ac_decoder_full::CreateDecoder(%s_isa::formats, %s_isa::instructions, &ref);\n\n", INDENT[2], project_name, project_name );

#ifdef ADD_DEBUG
  fprintf( output,"\n#ifdef AC_DEBUG_DECODER\n" );
  fprintf( output,"      ShowDecFormat(%s_isa::formats);\n", project_name );
  fprintf( output,"      ShowDecoder(decoder -> decoder, 0);\n" );
  fprintf( output,"#endif \n" );
#endif

  // [ARCHC_2_0]
  // Inicialização de instr_table é estática, não ocorre mais dentro
  // do construtor (está em _isa_init.H).

/*     if( ACStatsFlag ){ */
/*       fprintf( output, "%s\n", INDENT[2]); */
/*       COMMENT(INDENT[2], "Initializing Instruction Statistics table.\n"); */
/*       fprintf( output, "%sfor( int i = 0; i < AC_DEC_INSTR_NUMBER; i++)\n", INDENT[2] ); */
/*       fprintf( output, "%sac_sim_stats.instr_table[instructions[i].id].name = instructions[i].name;\n", INDENT[3]); */
/*     } */
/*   }      */

  /* Closing constructor declaration. */
  fprintf( output,"%s}\n", INDENT[1] );

  /* Closing class declaration. */
  fprintf( output,"};\n\n" );

  /* END OF FILE */
  fprintf( output, "\n\n#endif //_%s_ISA_H\n\n", project_name);
  fclose( output); 

  /* opens behavior macros file */
  sprintf( filename, "%s_bhv_macros.H", project_name);
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  fprintf( output, "#ifndef _%s_BHV_MACROS_H\n", project_name);
  fprintf( output, "#define _%s_BHV_MACROS_H\n\n", project_name);

  /* ac_behavior main macro */
  fprintf( output, "#define ac_behavior(instr) AC_BEHAVIOR_##instr ()\n\n");

  /* ac_behavior 2nd level macros - generic instruction */
  fprintf(output, "#define AC_BEHAVIOR_instruction() %s_instruction::_behavior_instruction()\n", project_name);

  fprintf(output, "\n");

  /* ac_behavior 2nd level macros - pseudo-instructions begin, end, error */
  fprintf(output, "#define AC_BEHAVIOR_begin() %s_instruction::_behavior_begin()\n", project_name);
  fprintf(output, "#define AC_BEHAVIOR_end() %s_instruction::_behavior_end()\n", project_name);
  fprintf(output, "#define AC_BEHAVIOR_error() %s_instruction::_behavior_error()\n", project_name);

  fprintf(output, "\n");

  /* ac_behavior 2nd level macros - instruction types */
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    fprintf(output, "#define AC_BEHAVIOR_%s() %s_type_%s::_behavior_%s_%s()\n", pformat->name, project_name, pformat->name, project_name, pformat->name);
  }

  fprintf(output, "\n");

  /* ac_behavior 2nd level macros - instructions */
  for (pinstr = instr_list; pinstr != NULL; pinstr = pinstr->next) {
    fprintf(output, "#define AC_BEHAVIOR_%s() %s_type_%s::behavior_%s()\n", pinstr->name, project_name, pinstr->format, pinstr->name);
  }

  /* END OF FILE */
  fprintf( output, "\n\n#endif //_%s_BHV_MACROS_H\n\n", project_name);
  fclose( output); 

}

/** Creates the _instruction.H header file. */
void CreateInstructionHeader() {

  extern char *project_name;

  char filename[256];
  char description[] = "Instruction Base Class header file.";
 
  // File containing class declaration
  FILE  *output;

  sprintf( filename, "%s_instruction.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, description);

  /* includes */
  fprintf( output, "#ifndef _%s_INSTRUCTION_H\n", project_name);
  fprintf( output, "#define _%s_INSTRUCTION_H\n\n", project_name);
  fprintf( output, "#include \"%s_arch_ref.H\"\n", project_name);
  fprintf( output, "#include \"%s_arch.H\"\n", project_name);
  fprintf( output, "\n");

  /* Emitting class */
  fprintf( output, "class %s_instruction: public %s_arch_ref {\n",
           project_name, project_name);
  fprintf(output, "public:\n\n");

  /* Emitting constructor */
  fprintf(output, "%s%s_instruction(%s_arch& ref) : %s_arch_ref(ref) {};\n\n",
          INDENT[1], project_name, project_name, project_name);

  /* Emitting behavior methods */
  fprintf(output, "%svoid _behavior_instruction();\n\n", INDENT[1]);
  fprintf(output, "%svoid _behavior_begin();\n\n", INDENT[1]);
  fprintf(output, "%svoid _behavior_end();\n\n", INDENT[1]);
  fprintf(output, "%svoid _behavior_error();\n\n", INDENT[1]);

  /* Closing class */
  fprintf(output, "};\n\n");

  /* Closing file */
  fprintf(output, "#endif // _%s_INSTRUCTION_H\n", project_name);
  fclose(output);

}

/** Creates the Instruction Format Header Files. */
void CreateFormatHeaders() {

  extern ac_dec_format *format_ins_list;
  extern char *project_name;
  extern ac_dec_instr *instr_list;
  extern int HaveMultiCycleIns;
  ac_dec_format *pformat;
  ac_dec_instr *pinstr;
  ac_dec_field *pfield;

  char filename[256];
  char description[] = "Instruction Set Architecture format header file.";
 
  // File containing ISA declaration
  FILE  *output;

  /* Creates all format header files. */
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    sprintf( filename, "%s_type_%s.H", project_name, pformat->name);
    
    if ( !(output = fopen( filename, "w"))){
      perror("ArchC could not open output file");
      exit(1);
    }

    print_comment( output, description);

    /* includes */
    fprintf( output, "#ifndef _%s_TYPE_%s_H\n", project_name, pformat->name);
    fprintf( output, "#define _%s_TYPE_%s_H\n\n", project_name, pformat->name);
    fprintf( output, "#include \"%s_instruction.H\"\n", project_name);
    fprintf( output, "\n");
    
    /* Emitting class */
    fprintf( output, "class %s_type_%s : public virtual %s_instruction {\n",
             project_name, pformat->name, project_name);
    fprintf(output, "public:\n\n");

    /* Emitting fields */
    for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next){
      if( pfield->sign )
        fprintf( output,"%sint %s;\n",INDENT[1],pfield->name );
      else
        fprintf( output,"%sunsigned %s;\n",INDENT[1],pfield->name );
    }
    
    fprintf(output, "\n");

    /* Emitting constructor */
    fprintf(output,
            "%s%s_type_%s(%s_arch& ref) : %s_instruction(ref) {};\n\n",
            INDENT[1], project_name, pformat->name,
            project_name, project_name);

    /* Emitting format behavior method */
    fprintf(output,
            "%svoid _behavior_%s_%s();\n\n",
            INDENT[1], project_name, pformat->name);

    /* Emitting instruction behavior methods */
    for (pinstr = instr_list; pinstr != NULL; pinstr = pinstr->next) {
      if (strcmp(pinstr->format, pformat->name) == 0) {
        fprintf(output,
                "%svoid behavior_%s();\n",
                INDENT[1], pinstr->name);
      }
    }

    /* Closing class */
    fprintf(output, "};\n\n");

    /* Closing file */
    fprintf(output, "#endif // _%s_TYPE_%s_H\n", project_name, pformat->name);
    fclose(output);
    
  }


}

//!Creates the ARCH Header File.
void CreateARCHHeader() {
  
}

 
//!Creates Stage Module Header File  for Single Pipelined Architectures.
void CreateStgHeader( ac_stg_list* stage_list, char* pipe_name) {

  extern char *project_name;
  extern int stage_num;
  ac_stg_list *pstage;

  char* stage_filename;
  FILE* output;

  for( pstage = stage_list; pstage != NULL; pstage=pstage->next){      


    //IF a ac_pipe declaration was used, stage module names will be PIPENAME_STAGENAME,
    //otherwise they will be just STAGENAME.
    //This makes possible to define multiple pipelines containg stages with the same name.
    if( pipe_name ){
      stage_filename = (char*) malloc(strlen(pstage->name)+strlen(pipe_name)+strlen(".H")+2);
      sprintf( stage_filename, "%s_%s.H", pipe_name, pstage->name);
    }
    else{
      stage_filename = (char*) malloc(strlen(pstage->name)+strlen(".H")+1);
      sprintf( stage_filename, "%s.H", pstage->name);
    }

    if ( !(output = fopen( stage_filename, "w"))){
      perror("ArchC could not open output file");
      exit(1);
    }

    print_comment( output, "Stage Module Header File.");
    if( pipe_name ){
      fprintf( output, "#ifndef  _%s_%s_STAGE_H\n", pipe_name, pstage->name);
      fprintf( output, "#define  _%s_%s_STAGE_H\n\n", pipe_name, pstage->name);
    }
    else{
      fprintf( output, "#ifndef  _%s_STAGE_H\n", pstage->name);
      fprintf( output, "#define  _%s_STAGE_H\n\n", pstage->name);
    }

    fprintf( output, "#include \"archc.H\"\n");
    fprintf( output, "#include \"%s_isa.H\"\n\n", project_name);

    if( pstage->id == 1 && ACDecCacheFlag )
      fprintf( output, "extern unsigned dec_cache_size;\n\n");
 
    //Declaring stage namespace.
    if( pipe_name ){
      fprintf( output, "namespace AC_%s_%s{\n\n", pipe_name, pstage->name); 
      fprintf( output, "class %s_%s: public ac_stage {\n", pipe_name, pstage->name);
    }
    else{
      fprintf( output, "namespace AC_%s{\n", pstage->name); 
      fprintf( output, "class %s: public ac_stage {\n", pstage->name);
    }
    //Declaring stage module. 
    //It already includes the behavior method.
    fprintf( output, "public:\n");

    if( pstage->id != 1 )
      fprintf( output, "%ssc_in<ac_instr> regin;\n", INDENT[1]);
    fprintf( output, "%ssc_inout<unsigned> bhv_pc;\n", INDENT[1]);

    if( pstage->id != stage_num ){
      fprintf( output, "%ssc_out<ac_instr> regout;\n", INDENT[1]);
      fprintf( output, "%ssc_out<bool> bhv_start;\n", INDENT[1]);
    }

    fprintf( output, "%ssc_out<bool> bhv_done;\n", INDENT[1]);
    fprintf( output, "%s%s_isa  ISA;\n", INDENT[1], project_name );



    fprintf( output, "%sbool start_up;\n", INDENT[1]);
    fprintf( output, "%sunsigned id;\n\n", INDENT[1]);
    fprintf( output, "%svoid behavior();\n\n", INDENT[1]);

    if(pstage->id==1 && ACDecCacheFlag){
      fprintf( output, "%scache_item* DEC_CACHE;\n\n", INDENT[1]);
    }
		
    if( pipe_name ){
      fprintf( output, "%sSC_HAS_PROCESS( %s_%s );\n\n", INDENT[1], pipe_name, pstage->name);
      fprintf( output, "%s%s_%s( sc_module_name name_ ): ac_stage(name_){\n\n", INDENT[1], pipe_name, pstage->name);
    }
    else{
      fprintf( output, "%sSC_HAS_PROCESS( %s );\n\n", INDENT[1], pstage->name);
      fprintf( output, "%s%s( sc_module_name name_ ): ac_stage(name_){\n\n", INDENT[1], pstage->name);
    }

    //Declaring Constructor.
    fprintf( output, "%sSC_METHOD( behavior );\n", INDENT[2]);
    if( pstage->id != stage_num)
      fprintf( output, "%ssensitive_pos << bhv_start;\n", INDENT[2]);
    else
      fprintf( output, "%ssensitive << bhv_pc;\n", INDENT[2]);

    //We need this in order to not fetch the first instruction
    //during initialization.
    if( pstage->id == 1){
      fprintf( output, "%sdont_initialize();\n\n", INDENT[2]);
      fprintf( output, "%sstart_up=1;\n", INDENT[2]);
    }
    fprintf( output, "%sid = %d;\n\n", INDENT[2], pstage->id);
                
    if( ACDasmFlag && pstage->id == 1)
      fprintf( output, "%sdasmfile.open(\"%s.dasm\");\n\n", INDENT[2], project_name);
    
    //end of constructor
    fprintf( output, "%s}\n", INDENT[1]); 

    if(pstage->id==1 && ACDecCacheFlag){
      fprintf( output, "%svoid init_dec_cache() {\n", INDENT[1]);  //end constructor
      fprintf( output, "%sDEC_CACHE = (cache_item*)calloc(sizeof(cache_item),dec_cache_size);\n", INDENT[2]);  //end constructor
      fprintf( output, "%s}\n", INDENT[1]);  //end init_dec_cache
    }

                
    fprintf( output, "};\n");

    //End of namespace
    fprintf( output, "}\n"); 

    fprintf( output, "#endif \n");
    fclose(output);
    free(stage_filename);
  }
}

// [ARCHC_2_0]
// Módulo de processador incorpora as declarações do ARCH header.

//!Creates Processor Module Header File
void CreateProcessorHeader() {

  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list;
  extern char *project_name;
  extern int stage_num;
  extern int HaveMultiCycleIns;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;
  int i;
  char filename[256];
  char description[] = "Architecture Module header file.";
  
  // File containing ISA declaration
  FILE  *output;
  
  sprintf( filename, "%s.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }
  
  print_comment( output, description);
  
  
  fprintf( output, "#ifndef  _%s_H\n", project_name);
  fprintf( output, "#define  _%s_H\n\n", project_name);
  
  fprintf( output, "#include \"systemc.h\"\n");
  fprintf( output, "#include \"ac_module.H\"\n");
  fprintf( output, "#include \"ac_utils.H\"\n");
  fprintf( output, "#include \"%s_parms.H\"\n", project_name);
  fprintf( output, "#include \"%s_arch.H\"\n", project_name);
  fprintf( output, "#include \"%s_isa.H\"\n", project_name);

  if (ACABIFlag)
    fprintf( output, "#include \"%s_syscall.H\"\n", project_name);

  if(ACGDBIntegrationFlag) {
    fprintf( output, "#include \"ac_gdb_interface.H\"\n");
    fprintf( output, "#include \"ac_gdb.H\"\n");
    fprintf( output, "extern AC_GDB *gdbstub;\n");
  }
  
  fprintf(output, "\n\n");

  fprintf( output, "class %s: public ac_module, public %s_arch {\n",project_name, project_name);

  fprintf(output, "private:\n");
  fprintf(output, "%stypedef cache_item<%s_parms::AC_DEC_FIELD_NUMBER> cache_item_t;\n", INDENT[1], project_name);
  fprintf(output, "%stypedef ac_instr<%s_parms::AC_DEC_FIELD_NUMBER> ac_instr_t;\n", INDENT[1], project_name);

  fprintf( output, "public:\n\n");
  
  fprintf( output, "%ssc_in<bool> clock;\n", INDENT[1]);
  fprintf( output, "%ssc_signal<unsigned> bhv_pc;\n", INDENT[1]);

  if( HaveMultiCycleIns)
    fprintf( output, "%ssc_signal<unsigned> bhv_cycle;\n", INDENT[1]);
  
  fprintf( output, " \n");
  
  fprintf( output, "%ssc_signal<bool> do_it;\n", INDENT[1]);

  fprintf( output, "%ssc_signal<bool> done;\n\n", INDENT[1]);
  fprintf( output, "%sdouble last_clock;\n", INDENT[1]);
  
  fprintf( output, " \n");

  fprintf( output, "%s%s_isa ISA;\n", INDENT[1], project_name );
  if (ACABIFlag)
    fprintf( output, "%s%s_syscall syscall;\n", INDENT[1], project_name );

  if(ACDecCacheFlag){
    fprintf( output, "%scache_item_t* DEC_CACHE;\n\n", INDENT[1]);
  }

  fprintf( output, "%sunsigned id;\n\n", INDENT[1]);
  fprintf( output, "%sbool start_up;\n", INDENT[1]);
  fprintf( output, "%sunsigned* instr_dec;\n", INDENT[1]);
  fprintf( output, "%sac_instr_t* instr_vec;\n\n", INDENT[1]);

  COMMENT(INDENT[1], "Behavior execution method.");
  fprintf( output, "%svoid behavior();\n\n", INDENT[1]);
  
  COMMENT(INDENT[1], "Verification method.");
  fprintf( output, "%svoid ac_verify();\n", INDENT[1]);
  fprintf( output, " \n");
  
  COMMENT(INDENT[1], "Updating Pipe Regs for behavioral simulation.");
  fprintf( output, "%svoid ac_update_regs();\n", INDENT[1]);
  
  fprintf( output, " \n");

  fprintf( output, "%sSC_HAS_PROCESS( %s );\n\n", INDENT[1], project_name);

  //!Declaring ARCH Constructor.
  COMMENT(INDENT[1], "Constructor.");
  fprintf( output, "%s%s( sc_module_name name_ ): ac_module(name_), %s_arch(), ISA(*this), syscall(*this) {\n\n", INDENT[1], project_name, project_name);
  
  fprintf( output, "%sSC_THREAD( behavior );\n", INDENT[2]);
  fprintf( output, "%ssensitive << bhv_pc << do_it;\n\n", INDENT[2]);

  fprintf( output, "%sSC_THREAD( ac_verify );\n", INDENT[2]);
  fprintf( output, "%ssensitive<< done;\n", INDENT[2]);  
  fprintf( output, " \n");

  fprintf( output,"%sSC_THREAD( ac_update_regs );\n", INDENT[2]);
  fprintf( output,"%ssensitive_pos<< clock ; \n", INDENT[2]);
  fprintf( output,"%sdont_initialize(); \n\n", INDENT[2]);     
  
  fprintf( output,"%sbhv_pc = 0; \n", INDENT[2]);

  fprintf( output, "%sstart_up=1;\n", INDENT[2]);
  fprintf( output, "%sid = %d;\n\n", INDENT[2], 1);

  if( ACDasmFlag )
    fprintf( output, "%sdasmfile.open(\"%s.dasm\");\n\n", INDENT[2], project_name);

  fprintf( output, "%s}\n", INDENT[1]);  //end constructor

  if(ACDecCacheFlag){
    fprintf( output, "%svoid init_dec_cache() {\n", INDENT[1]);  //end constructor
    fprintf( output, "%sDEC_CACHE = (cache_item_t*) calloc(sizeof(cache_item_t),dec_cache_size);\n", INDENT[2]);  //end constructor
    fprintf( output, "%s}\n", INDENT[1]);  //end init_dec_cache
  }

  if(ACGDBIntegrationFlag) {
    fprintf( output, "%s/***********\n", INDENT[1]);
    fprintf( output, "%s * GDB Support - user supplied methods\n", INDENT[1]);
    fprintf( output, "%s * For further information, look at $ARCHC_PATH/src/aclib/ac_gdb/ac_gdb_interface.H\n", INDENT[1]);
    fprintf( output, "%s ***********/\n\n", INDENT[1]);
    
    fprintf( output, "%s/* Register access */\n", INDENT[1]);
    fprintf( output, "%sint nRegs(void);\n", INDENT[1]);
    fprintf( output, "%sac_word reg_read(int reg);\n", INDENT[1]);
    fprintf( output, "%svoid reg_write( int reg, ac_word value );\n\n", INDENT[1]);
    
    fprintf( output, "%s/* Memory access */\n", INDENT[1]);
    fprintf( output, "%sunsigned char mem_read( unsigned int address );\n", INDENT[1]);
    fprintf( output, "%svoid mem_write( unsigned int address, unsigned char byte );\n", INDENT[1]);
  }
  
    fprintf( output, "\n%svoid init(int ac, char* av[], double period);\n\n", INDENT[1]);
    fprintf( output, "%svoid init(char* program, double period);\n\n", INDENT[1]);
    fprintf( output, "%svoid stop(int status = 0);\n\n", INDENT[1]);

    fprintf( output, "%svirtual ~%s() {};\n\n", INDENT[1], project_name);

  //!Closing class declaration.
  fprintf( output,"%s};\n", INDENT[0] );
  fprintf( output, "#endif  //_%s_H\n\n", project_name);

  fclose( output); 
}

//!Creates Formatted Registers Header File
void CreateRegsHeader() {
  extern ac_dec_format *format_reg_list;
  extern ac_sto_list *storage_list;

  ac_sto_list *pstorage;
  ac_dec_format *pformat;
  ac_dec_field *pfield;

  int flag = 1;
  FILE *output;
  char filename[] = "ac_fmt_regs.H";


  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){

    if(( pstorage->type == REG ) && (pstorage->format != NULL )){

      if(flag){  //Print this just once.
        if ( !(output = fopen( filename, "w"))){
          perror("ArchC could not open output file");
          exit(1);
        }


        print_comment( output, "ArchC Formatted Registers header file.");
        fprintf( output, "#ifndef  _AC_FMT_REGS_H\n");
        fprintf( output, "#define  _AC_FMT_REGS_H\n\n");
        
        fprintf( output, "#include  \"ac_storage.H\"\n");
        fprintf( output, "#include  \"ac_parms.H\"\n");
        fprintf( output, "\n\n");

        COMMENT(INDENT[0],"ArchC classes for formatted registers.\n");
        flag = 0;
      }

      for( pformat = format_reg_list; pformat!= NULL; pformat=pformat->next){
        if(!(strcmp(pformat->name, pstorage->format))){
          break;
        }
      }
      //Declaring formatted register class.
      fprintf( output, "class ac_%s {\n", pstorage->name);
      fprintf( output, "%schar* name;\n", INDENT[1]);
      fprintf( output, "public:\n");
      
      //TO DO: Registers with parameterized size. The templated class ac_reg is still not
      //       working with sc_unit<x> types.
      for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next)
        fprintf( output,"%sac_reg<unsigned> %s;\n",INDENT[1],pfield->name );

      fprintf( output, "\n\n");
   
      //Declaring class constructor.
      fprintf( output, "%sac_%s(char* n): \n", INDENT[1], pstorage->name);
      for( pfield = pformat->fields; pfield->next != NULL; pfield = pfield->next)
        //Initializing field names with reg name. This is to enable Formatted Reg stats.
        //Need to be changed if we adopt statistics collection for each field individually.
        fprintf( output,"%s%s(\"%s\",%d),\n",INDENT[2],pfield->name,pstorage->name, 0 );
      //Last field.
      fprintf( output,"%s%s(\"%s\",%d){name = n;}\n\n",INDENT[2],pfield->name,pstorage->name, 0 );

      fprintf( output,"%svoid change_dump(ostream& output){}\n\n",INDENT[1] );
      fprintf( output,"%svoid reset_log(){}\n\n",INDENT[1] );
      fprintf( output,"%svoid behavior(ac_stage_list stage = (ac_stage_list)0, int cycle = 0);\n\n",INDENT[1] );
      fprintf( output, "};\n\n");      

    }
  }

  if(!flag){ //We had at last one formatted reg declared.
    fprintf( output, "#endif //_AC_FMT_REGS_H\n");
    fclose(output);
  }
}


//!Create the header file for ArchC co-verification class.
void CreateCoverifHeader(void){
 
  extern ac_sto_list *storage_list;
  ac_sto_list *pstorage;

  FILE *output;
  char filename[] = "ac_verify.H";


  
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }


  print_comment( output, "ArchC Co-verification Class header file.");

  fprintf( output, "#ifndef  _AC_VERIFY_H\n");
  fprintf( output, "#define  _AC_VERIFY_H\n\n");
        
  fprintf( output, "#include  <fstream>\n");
  fprintf( output, "#include  <list>\n");
  fprintf( output, "#include  \"archc.H\"\n");
  fprintf( output, "#include  \"ac_parms.H\"\n");
  fprintf( output, "#include  \"ac_resources.H\"\n");
  fprintf( output, "#include  \"ac_storage.H\"\n");
  fprintf( output, "\n\n");

  COMMENT(INDENT[0],"ArchC Co-verification class.\n");
  fprintf( output, "class ac_verify:public ac_resources {\n\n");
  
  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "%slog_list %s_changes;\n", INDENT[1], pstorage->name);
  }

  fprintf( output, "public:\n\n");

  fprintf( output, "%sofstream output;\n", INDENT[1]);

  //Printing log method.
  COMMENT(INDENT[1],"Logging structural model changes.");

  fprintf( output, "%svoid log( char *name, unsigned address, ac_word datum, double time ){\n\n", INDENT[1]);
  fprintf( output, "%slog_list *pdevchg;\n", INDENT[2]);

  fprintf( output, "%s", INDENT[2]);

  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "if( !strcmp( name, \"%s\") )\n", pstorage->name);
    fprintf( output, "%spdevchg = &%s_changes;\n", INDENT[3], pstorage->name);
    fprintf( output, "%selse ", INDENT[2]);
  }

  fprintf( output, "{\n");

  fprintf( output, "%sAC_ERROR(\"Logging aborted! Unknown storage device used for verification: \" << name);", INDENT[3]);
  fprintf( output, "%sreturn;", INDENT[3]);
  fprintf( output, "%s}\n", INDENT[2]);

  fprintf( output, "\n\n");

  fprintf( output, "%sadd_log( pdevchg, address, datum, time);\n", INDENT[2]);

  fprintf( output, "%s}\n", INDENT[1]);

  //Printing check_clock method.
  COMMENT(INDENT[1],"Checking device logs at a given simulation time");

  fprintf( output, "%svoid check_clock( double time ){\n\n", INDENT[1]);


  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "%smatch_logs( &%s, &%s_changes, time );\n", INDENT[2], pstorage->name, pstorage->name);
  }
  fprintf( output, "%s}\n", INDENT[1]);

  //Printing checker_timed method.
  COMMENT(INDENT[1],"Finalize co-verification for timed model.");

  fprintf( output, "%svoid checker_timed( double time ){\n\n", INDENT[1]);

  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "%smatch_logs( &%s, &%s_changes, time );\n", INDENT[2], pstorage->name, pstorage->name);
    fprintf( output, "%scheck_final( &%s, &%s_changes );\n", INDENT[2], pstorage->name, pstorage->name);
  }
  fprintf( output, "%s}\n", INDENT[1]);

  //Printing checker method.
  COMMENT(INDENT[1],"Finalize co-verification for untimed model.");

  fprintf( output, "%svoid checker( ){\n\n", INDENT[1]);

  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "%scheck_final( &%s, &%s_changes );\n", INDENT[2], pstorage->name, pstorage->name);
  }
  fprintf( output, "%s}\n", INDENT[1]);

  //Printing class constructor.
  COMMENT(INDENT[1],"Constructor");

  fprintf( output, "%sac_verify( ){\n\n", INDENT[1]);
  fprintf( output, "%soutput.open( \"ac_verification.log\");\n", INDENT[2]);
  fprintf( output, "%s}\n", INDENT[1]);

  //Printing add_log method.
  COMMENT(INDENT[1],"Logging structural model changes for a given device");

  fprintf( output, "%svoid add_log( log_list *pdevchg, unsigned address, ac_word datum, double time );\n\n", INDENT[1]);

  //Printing match_logs method.
  COMMENT(INDENT[1],"Match device's behavioral and structural logs at a given simulation time");

  fprintf( output, "%svoid match_logs( ac_storage *pdevice, log_list *pdevchange, double time );\n\n", INDENT[1]);

  //Printing check_final method.
  COMMENT(INDENT[1],"Check behavioral and structural logs for a given device in the end of simulation");

  fprintf( output, "%svoid check_final( ac_storage *pdevice, log_list *pdevchange );\n\n", INDENT[1]);

  fprintf( output, "};\n");

  //END OF FILE!
  fprintf( output, "#endif //_AC_VERIFY_H\n");
  fclose(output);

}

//!Create the header file for ArchC statistics collection class.
void CreateStatsHeader(void){
 
  extern ac_sto_list *storage_list;
  extern char *project_name;
  ac_sto_list *pstorage;

  FILE *output;
  char filename[] = "ac_stats.H";


  
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }


  print_comment( output, "ArchC Statistics Collection Class header file.");

  fprintf( output, "#ifndef  _AC_STATS_H\n");
  fprintf( output, "#define  _AC_STATS_H\n\n");
        
  fprintf( output, "#include  <fstream>\n");
  fprintf( output, "#include  \"archc.H\"\n");
  fprintf( output, "#include  \"ac_parms.H\"\n");
  fprintf( output, "#include  \"ac_sto_stats.H\"\n");
  fprintf( output, "\n\n");

  pstorage = storage_list;

  if( pstorage ){

    //Defining initialization macro
    fprintf( output, "#define   INIT_STO_STATS  ");

    fprintf( output, "%s( \"%s\")",pstorage->name, pstorage->name);
    
    for( pstorage = pstorage->next; pstorage != NULL; pstorage = pstorage->next ){
      fprintf( output, ",%s( \"%s\")",pstorage->name, pstorage->name);
    }
    fprintf( output, ", ac_pc( \"ac_pc\")");
  }
  fprintf( output, "\n");

  COMMENT(INDENT[0],"ArchC class for Simulation Statistics.");
  fprintf( output, "class ac_stats {\n\n");
  
  fprintf( output, "%sac_sto_stats* head;\n", INDENT[1]);
  fprintf( output, "\n");
  fprintf( output, "public:\n\n");

  COMMENT(INDENT[1],"Output File.");
  fprintf( output, "%sofstream output;\n", INDENT[1]);

  COMMENT(INDENT[1],"Keeps the total simulation time.");
  fprintf( output, "%sdouble time;\n", INDENT[1]);
  fprintf( output, "\n");

  COMMENT(INDENT[1],"This table tells  how many times an instruction was executed during simulation.");
  fprintf( output, "%sstruct {\n", INDENT[1]);
  fprintf( output, "%schar *name;\n", INDENT[2]);
  fprintf( output, "%sint count;\n", INDENT[2]);
  fprintf( output, "%s} instr_table[AC_DEC_INSTR_NUMBER+1];\n", INDENT[1]);
  fprintf( output, "\n");

  COMMENT(INDENT[1],"This keeps the total number of instructions executed so far.");
  fprintf( output, "%sint instr_executed;\n", INDENT[1]);
  fprintf( output, "%sint syscall_executed;\n", INDENT[1]);
  fprintf( output, "\n");

  COMMENT(INDENT[1],"This keeps the cycle count estimates.");
  fprintf( output, "%sunsigned long long ac_min_cycle_count;\n", INDENT[1]);
  fprintf( output, "%sunsigned long long ac_max_cycle_count;\n", INDENT[1]);

  COMMENT(INDENT[1],"Storage Device statistics objects.");

  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "%sac_sto_stats %s;\n", INDENT[1], pstorage->name);
  }
  fprintf( output, "%sac_sto_stats ac_pc;\n", INDENT[1]);
  fprintf( output, "\n");

  //Printing class constructor.
  COMMENT(INDENT[1],"Constructor");
  fprintf( output, "%sac_stats( );\n\n", INDENT[1]);
  fprintf( output, "\n");

  //Printing print method.
  COMMENT(INDENT[1],"Print Simulation Statistics Report");
  fprintf( output, "%svoid print(  );\n\n", INDENT[1]);
  fprintf( output, "\n");

  //Printing add_access method.
  COMMENT(INDENT[1],"Increase access number for a given device.");
  fprintf( output, "%svoid add_access( char *name ){\n\n", INDENT[1]);

  fprintf( output, "\n");

  fprintf( output, "%s", INDENT[2]);
  fprintf( output, "if( !strcmp( name, \"ac_pc\") )\n");
  fprintf( output, "%sac_pc.inc_accesses();\n", INDENT[3]);
  fprintf( output, "%selse ", INDENT[2]);
  

  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "if( !strcmp( name, \"%s\") )\n", pstorage->name);
    fprintf( output, "%s%s.inc_accesses();\n", INDENT[3], pstorage->name);
    fprintf( output, "%selse ", INDENT[2]);
  }
  fprintf( output, "\n%sAC_ERROR(\"Unknown storage device used for statistics collection.\" << name);\n", INDENT[3]);

  fprintf( output, "%s}\n", INDENT[1]);
  fprintf( output, "\n");

  //Printing add_miss method.
  COMMENT(INDENT[1],"Increase access number for a given device.");
  fprintf( output, "%svoid add_miss( char *name ){\n\n", INDENT[1]);

  fprintf( output, "\n");

  fprintf( output, "%s", INDENT[2]);
  fprintf( output, "if( !strcmp( name, \"ac_pc\") )\n");
  fprintf( output, "%sac_pc.inc_misses();\n", INDENT[3]);
  fprintf( output, "%selse ", INDENT[2]);
  
  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
    fprintf( output, "if( !strcmp( name, \"%s\") )\n", pstorage->name);
    fprintf( output, "%s%s.inc_misses();\n", INDENT[3], pstorage->name);
    fprintf( output, "%selse ", INDENT[2]);
  }
  fprintf( output, "\n");
  fprintf( output, "%sAC_ERROR(\"Unknown storage device used for statistics collection.\" << name);\n", INDENT[3]);

  fprintf( output, "%s}\n", INDENT[1]);

  fprintf( output, "\n\n");


  //Printing stat_init method.
  COMMENT(INDENT[1],"Initialize lists and members.");
  fprintf( output, "%svoid init_stat( ){\n\n", INDENT[1]);
  
  if(storage_list){
    
    fprintf( output, "%shead = &%s; \n", INDENT[2], storage_list->name);
    
    for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){
      if(pstorage->next)
        fprintf( output, "%s%s.next = &%s;\n", INDENT[2], pstorage->name, pstorage->next->name);
      else{
        fprintf( output, "%s%s.next = &ac_pc;\n", INDENT[2], pstorage->name);   
        fprintf( output, "%sac_pc.next = NULL;\n", INDENT[2]);  
      }
    }  
  }
  else{

    fprintf( output, "%shead = NULL; \n", INDENT[2]);
  }

  //Openning input file.
  fprintf( output, "%soutput.open( \"%s.stats\");\n", INDENT[2], project_name);

  fprintf( output, "%s}\n", INDENT[1]);
    

  //End of Class.
  fprintf( output, "};\n");

  //END OF FILE!
  fprintf( output, "#endif //_AC_STATS_H\n");
  fclose(output);

}

/////////////////////////// Create Implementation Functions ////////////////////////////

//!Creates Stage Module Implementation File for Single Pipelined Architectures
void CreateStgImpl(ac_stg_list* stage_list, char* pipe_name) {
  
  extern char *project_name;
  extern int stage_num;
  ac_stg_list *pstage;
  int base_indent;
  char* stage_filename;
  FILE* output;

  for( pstage = stage_list; pstage != NULL; pstage=pstage->next){      

    //IF a ac_pipe declaration was used, stage module names will be PIPENAME_STAGENAME,
    //otherwise they will be just STAGENAME.
    //This makes possible to define multiple pipelines containg stages with the same name.
    if( pipe_name ){
      stage_filename = (char*) malloc(strlen(pstage->name)+strlen(pipe_name)+strlen(".cpp")+2);
      sprintf( stage_filename, "%s_%s.cpp", pipe_name, pstage->name);
    }
    else{
      stage_filename = (char*) malloc(strlen(pstage->name)+strlen(".cpp")+1);
      sprintf( stage_filename, "%s.cpp", pstage->name);
    }

    if ( !(output = fopen( stage_filename, "w"))){
      perror("ArchC could not open output file");
      exit(1);
    }

    print_comment( output, "Stage Module Implementation File.");
    if( pipe_name ){
      fprintf( output, "#include  \"%s_%s.H\"\n\n", pipe_name, pstage->name);
    }
    else{
      fprintf( output, "#include  \"%s.H\"\n\n", pstage->name);
    }

    if( pstage->id == 1 && ACABIFlag )
      fprintf( output, "#include  \"%s_syscall.H\"\n\n", project_name);
    
    if( pstage->id == 1 && ACVerifyFlag ){
      fprintf( output, "#include  \"ac_msgbuf.H\"\n");
      fprintf( output, "#include  \"sys/msg.h\"\n");
    }


    //Emiting stage behavior method implementation.
    if( pstage->id != 1 ){

      if( pipe_name )			
        fprintf( output, "void AC_%s_%s::%s_%s::behavior() {\n\n", pipe_name, pstage->name, pipe_name, pstage->name);
      else
        fprintf( output, "void AC_%s::%s::behavior() {\n\n", pstage->name, pstage->name);

      fprintf( output, "%sac_instruction *instr, *format;\n", INDENT[1]);
      fprintf( output, "%sunsigned ins_id;\n", INDENT[1]);
      fprintf( output, "%sac_instr *instr_vec;\n\n", INDENT[1]);
      fprintf( output, "%sinstr_vec = new ac_instr(regin.read());\n\n", INDENT[1]);
      fprintf( output, "%sins_id = instr_vec->get(IDENT);\n", INDENT[1]);

      fprintf( output, "%sif( ins_id != 0 ) {\n", INDENT[1]);
      fprintf( output, "%sinstr = (ac_instruction *)ISA.instr_table[instr_vec->get(IDENT)][1];\n", INDENT[2]);
      fprintf( output, "%sformat = (ac_instruction *)ISA.instr_table[ins_id][2];\n", INDENT[2]);
      fprintf( output, "%sformat->set_fields( *instr_vec  );\n", INDENT[2]);
      fprintf( output, "%sinstr->set_fields( *instr_vec  );\n", INDENT[2]);
      fprintf( output, "%sISA.instruction.behavior( (ac_stage_list)id );\n", INDENT[2]);
      fprintf( output, "%sformat->behavior( (ac_stage_list)id );\n", INDENT[2]);
      fprintf( output, "%sinstr->behavior( (ac_stage_list)id );\n", INDENT[2]);
      fprintf( output, "%s}\n", INDENT[1]);

      if( pstage->id != stage_num)
        fprintf( output, "%sregout.write( *instr_vec);\n", INDENT[1]);

      fprintf( output, "%sdelete instr_vec;\n", INDENT[1]);
      fprintf( output, "%sbhv_done.write(1);\n", INDENT[1]);
      fprintf( output, "}\n\n");
    }

    //First Stage needs to fetch and decode instructions ....
    else{
      if( pipe_name )			
        fprintf( output, "void AC_%s_%s::%s_%s::behavior() {\n\n", pipe_name, pstage->name, pipe_name, pstage->name);
      else
        fprintf( output, "void AC_%s::%s::behavior() {\n\n", pstage->name, pstage->name);
			
      fprintf( output, "%sac_instruction *instr, *format;\n", INDENT[1]);
      fprintf( output, "%sunsigned  ins_id;\n", INDENT[1]);
                        
      if( ACDebugFlag ){
        fprintf( output, "%sextern bool ac_do_trace;\n", INDENT[1]);
        fprintf( output, "%sextern ofstream trace_file;\n", INDENT[1]);
      }

      if( ACABIFlag ){
        fprintf( output, "%s%s_syscall syscall;\n", INDENT[1], project_name);
        fprintf( output, "%sstatic int flushes_left=7;\n", INDENT[1]);
        fprintf( output, "%sstatic ac_instr *the_nop = new ac_instr;\n", INDENT[1]);
      }			
			
      if( ACVerifyFlag ){
        fprintf( output, "%sextern int msqid;\n", INDENT[1]);
        fprintf( output, "%sstruct log_msgbuf end_log;\n", INDENT[1]);
      }
                        
      if( ac_host_endian == 0 ){
        fprintf( output, "%schar fetch[AC_WORDSIZE/8];\n\n", INDENT[1]);
      }

      if(ACDecCacheFlag)
        fprintf( output, "%scache_item* ins_cache;\n", INDENT[1]);

                        
      fprintf( output, "%sextern unsigned int decode_pc, quant;\n", INDENT[1]);
      fprintf( output, "%sextern unsigned char buffer[AC_MAX_BUFFER];\n", INDENT[1]);
      //      fprintf( output, "%sunsigned *instr_dec;\n", INDENT[1]);
      fprintf( output, "%sac_instr *instr_vec;\n\n", INDENT[1]);

      EmitFetchInit(output, 1);
      base_indent =2;

      if(ACABIFlag){

        base_indent++;
        //Emiting system calls handler.
        COMMENT(INDENT[1],"Handling System calls.");
        fprintf( output, "%sif (decode_pc %% 2) decode_pc--;\n", INDENT[2]);

        fprintf( output, "%sswitch( decode_pc ){\n", INDENT[2]);

        EmitPipeABIDefine(output);
        fprintf( output, "\n\n");
        EmitABIAddrList(output,3);

        fprintf( output, "%sdefault:\n", INDENT[2]);
      }

      EmitDecodification(output, 1);
      EmitInstrExec(output, base_indent);

      fprintf( output, "%sac_instr_counter++;\n", INDENT[3]);

      if( ACABIFlag ){
        //Closing default case.
        fprintf( output, "%sbreak;\n", INDENT[3]);
        //Closing switch
        fprintf( output, "%s}\n", INDENT[2]);
      }

      fprintf( output, "%sbhv_done.write(1);\n", INDENT[2]);
      //Closing else
      fprintf( output, "%s}\n", INDENT[1]);
      fprintf( output, "}\n\n");
    }

    fclose(output);
    free(stage_filename);
  }
}

//!Creates Processor Module Implementation File
void CreateProcessorImpl() {

  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list;
  extern ac_stg_list *stage_list;
  extern ac_sto_list *storage_list;
  extern char *project_name;
  extern int stage_num;
  extern int HaveMultiCycleIns, HaveMemHier;
  ac_sto_list *pstorage;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;
  int i;

  char* filename;
  FILE* output;

  filename = (char*) malloc(strlen(project_name)+strlen(".cpp")+1);
  sprintf( filename, "%s.cpp", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }
    
  print_comment( output, "Processor Module Implementation File.");
  fprintf( output, "#include  \"%s.H\"\n\n", project_name);

  if( ACVerifyFlag ){
    fprintf( output, "#include  \"ac_msgbuf.H\"\n");
    fprintf( output, "#include  \"sys/msg.h\"\n");
  }

  if( ACABIFlag )
    fprintf( output, "#include  \"%s_syscall.H\"\n\n", project_name);
		
  fprintf( output, "void %s::behavior() {\n\n", project_name);
  if( ACDebugFlag ){
    fprintf( output, "%sextern bool ac_do_trace;\n", INDENT[1]);
    fprintf( output, "%sextern ofstream trace_file;\n", INDENT[1]);
  }
  fprintf( output, "%sunsigned ins_id;\n", INDENT[1]);

  if( ACVerifyFlag ){
    fprintf( output, "%sextern int msqid;\n", INDENT[1]);
    fprintf( output, "%sstruct log_msgbuf end_log;\n", INDENT[1]);
  }
/*   if( ACABIFlag ) */
/*     fprintf( output, "%s%s_syscall syscall;\n", INDENT[1], project_name); */

  if(ACDecCacheFlag)
    fprintf( output, "%scache_item_t* ins_cache;\n", INDENT[1]);

/*   if( ac_host_endian == 0 ){ */
/*     fprintf( output, "%schar fetch[AC_WORDSIZE/8];\n\n", INDENT[1]); */
/*   } */

  if( HaveMemHier ) {
    fprintf( output, "%sif( ac_wait_sig ) {\n", INDENT[1]);
    fprintf( output, "%sreturn;\n", INDENT[2]);
    fprintf( output, "%s}\n\n", INDENT[1]);
  }
    
  //Emiting processor behavior method implementation.
  if( HaveMultiCycleIns )
    EmitMultiCycleProcessorBhv(output);
  else{
    if( ACABIFlag )
      EmitProcessorBhv_ABI(output);
    else
      EmitProcessorBhv(output);
  }

  fprintf( output, " \n");

  //Emiting Verification Method.
  COMMENT(INDENT[0],"Verification method.\n");               
  fprintf( output, "%svoid %s::ac_verify(){\n", INDENT[0], project_name);        

  if( ACVerifyFlag ){

    fprintf( output, "extern int msqid;\n");
    fprintf( output, "struct log_msgbuf lbuf;\n");
    fprintf( output, "log_list::iterator itor;\n");
    fprintf( output, "log_list *plog;\n");
  }
  fprintf( output, " \n");


  fprintf(output, "%sfor (;;) {\n\n", INDENT[1]);

  fprintf( output, "%sif( ", INDENT[1]);

  if(stage_list){
    for( i =1; i<= stage_num-1; i++)    
      fprintf( output, "st%d_done.read() && \n%s", i, INDENT[2]); 
    fprintf( output, "st%d_done.read() )\n", stage_num); 
  }
  else if ( pipe_list ){

    for( ppipe = pipe_list; ppipe != NULL; ppipe = ppipe->next ){

      for( pstage = ppipe->stages; pstage->next != NULL; pstage=pstage->next)
        fprintf( output, "%s%s_%s_done.read() && \n", INDENT[1], ppipe->name, pstage->name);

      if( ppipe->next )  //If we have another pipe in the list, do it normally, otherwise, close if condition
        fprintf( output, "%s%s_%s_done.read() && \n", INDENT[1], ppipe->name, pstage->name);
      else
        fprintf( output, "%s%s_%s_done.read() )\n", INDENT[1], ppipe->name, pstage->name);
				
    }
  }
  else{
    fprintf( output, "done.read() )\n"); 
  }

  fprintf( output, "%s  {\n", INDENT[2]);

    
  fprintf( output, "#ifdef AC_VERBOSE\n");
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    fprintf( output, "%s%s.change_dump(cerr);\n", INDENT[3],pstorage->name );
  }
  fprintf( output, "#endif\n");


  fprintf( output, "#ifdef AC_UPDATE_LOG\n");

  if( ACVerifyFlag ){

    int next_type = 3;

    fprintf( output, "%sif( sc_simulation_time() ){\n", INDENT[3]);

    //Sending logs for every storage device. We just consider for co-verification caches, regbanks and memories
    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

      if( pstorage->type == MEM ||
          pstorage->type == ICACHE ||
          pstorage->type == DCACHE ||
          pstorage->type == CACHE ||
          pstorage->type == REGBANK ){

        fprintf( output, "%splog = %s.get_changes();\n", INDENT[4],pstorage->name );
        fprintf( output, "%sif( plog->size()){\n", INDENT[4] );
        fprintf( output, "%sitor = plog->begin();\n", INDENT[5] );
        fprintf( output, "%slbuf.mtype = %d;\n", INDENT[5], next_type );
        fprintf( output, "%swhile( itor != plog->end()){\n\n", INDENT[5] );
        fprintf( output, "%slbuf.log = *itor;\n", INDENT[6] );
        fprintf( output, "%sif( msgsnd(msqid, (struct log_msgbuf *)&lbuf, sizeof(lbuf), 0) == -1)\n", INDENT[6] );
        fprintf( output, "%sperror(\"msgsnd\");\n", INDENT[7] );
        fprintf( output, "%sitor = plog->erase(itor);\n", INDENT[6] );
        fprintf( output, "%s}\n", INDENT[5] );
        fprintf( output, "%s}\n\n", INDENT[4] );
			
        next_type++;
      }
    }
    fprintf( output, "%s}\n\n", INDENT[3] );
		
  }
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    //fprintf( output, "%s%s.change_save();\n", INDENT[3],pstorage->name );
    fprintf( output, "%s%s.reset_log();\n", INDENT[3],pstorage->name );          
  }
	
  fprintf( output, "#endif\n");


  if(stage_list){
    for( i =1; i<= stage_num; i++)    
      fprintf( output, "%sst%d_done.write(0);\n", INDENT[3], i);  
  }
  else  if ( pipe_list ){
		
    for( ppipe = pipe_list; ppipe != NULL; ppipe = ppipe->next ){
			
      for( pstage = ppipe->stages; pstage != NULL; pstage=pstage->next)
        fprintf( output, "%s%s_%s_done.write(0);\n", INDENT[1], ppipe->name, pstage->name);

    }
  }
  else{
    fprintf( output, "%sdone.write(0);\n", INDENT[3]); 
  }

  fprintf( output, "%s  }\n\n", INDENT[2]);

  fprintf(output, "%swait();\n\n", INDENT[1]);
  fprintf(output, "%s}\n", INDENT[1]);

  fprintf( output, "%s}\n\n", INDENT[0]);

  //!Emit update method.
  if( stage_list )
    EmitPipeUpdateMethod( output);
  else if ( pipe_list )
    EmitMultiPipeUpdateMethod( output);
  else
    EmitUpdateMethod( output);

  /* TODO: Emit other stuff */
  /* stuff == */
  /* 1. SIGNAL HANDLERS */
  /* 2. init() and stop() functions */

  /* SIGNAL HANDLERS */
  fprintf(output, "#include <ac_sighandlers.H>\n\n");

  /* init() and stop() */
  /* init() with 3 parameters */
  fprintf(output, "void %s::init(int ac, char *av[], double period) {\n", project_name);
  fprintf(output, "%sextern char* appfilename;\n", INDENT[1]);
  fprintf(output, "%sac_init_opt( ac, av);\n", INDENT[1]);
  fprintf(output, "%sac_init_app( ac, av);\n", INDENT[1]);
  fprintf(output, "%sAPP_MEM->load(appfilename);\n", INDENT[1]);
  fprintf(output, "%stime_step = period / (sc_get_default_time_unit()).to_double();\n", INDENT[1]);
  fprintf(output, "%sset_args(ac_argc, ac_argv);\n", INDENT[1]);
  fprintf(output, "#ifdef AC_VERIFY\n");
  fprintf(output, "%sset_queue(av[0]);\n", INDENT[1]);
  fprintf(output, "#endif\n\n");

  fprintf(output, "#ifdef USE_GDB\n");
  fprintf(output, "%sif (gdbstub && !gdbstub->is_disabled()) \n", INDENT[1]);
  fprintf(output, "%sgdbstub->connect();\n", INDENT[2]);
  fprintf(output, "#endif /* USE_GDB */\n");
  fprintf(output, "%sISA._behavior_begin();\n", INDENT[1]);
  fprintf(output, "%scerr << \"ArchC: -------------------- Starting Simulation --------------------\" << endl;\n", INDENT[1]);
  fprintf(output, "%sInitStat();\n\n", INDENT[1]);

  fprintf(output, "%ssignal(SIGINT, sigint_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGTERM, sigint_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGSEGV, sigsegv_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGUSR1, sigusr1_handler);\n", INDENT[1]);
  fprintf(output, "#ifdef USE_GDB\n");
  fprintf(output, "%ssignal(SIGUSR2, sigusr2_handler);\n", INDENT[1]);
  fprintf(output, "#endif\n");
  fprintf(output, "#ifndef AC_COMPSIM\n");
  fprintf(output, "%sset_running();\n", INDENT[1]);
  fprintf(output, "#else\n");
  fprintf(output, "%sac_pc = 0;\n", INDENT[1]);
  fprintf(output, "%svoid Execute(int argc, char *argv[]);\n", INDENT[1]);
  fprintf(output, "%sExecute(argc, argv);\n", INDENT[1]);
  fprintf(output, "#endif\n");
  fprintf(output, "}\n\n");


  /* init() with 2 parameters */
  fprintf(output, "void %s::init(char* program, double period) {\n",
          project_name);
  fprintf(output, "%sAPP_MEM->load(program);\n", INDENT[1]);
  fprintf(output, "%stime_step = period / (sc_get_default_time_unit()).to_double();\n", INDENT[1]);
  fprintf(output, "%sset_args(ac_argc, ac_argv);\n", INDENT[1]);
  fprintf(output, "#ifdef AC_VERIFY\n");
  fprintf(output, "%sset_queue(av[0]);\n", INDENT[1]);
  fprintf(output, "#endif\n\n");

  fprintf(output, "#ifdef USE_GDB\n");
  fprintf(output, "%sif (gdbstub && !gdbstub->is_disabled()) \n", INDENT[1]);
  fprintf(output, "%sgdbstub->connect();\n", INDENT[2]);
  fprintf(output, "#endif /* USE_GDB */\n");
  fprintf(output, "%sISA._behavior_begin();\n", INDENT[1]);
  fprintf(output, "%scerr << \"ArchC: -------------------- Starting Simulation --------------------\" << endl;\n", INDENT[1]);
  fprintf(output, "%sInitStat();\n\n", INDENT[1]);

  fprintf(output, "%ssignal(SIGINT, sigint_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGTERM, sigint_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGSEGV, sigsegv_handler);\n", INDENT[1]);
  fprintf(output, "%ssignal(SIGUSR1, sigusr1_handler);\n", INDENT[1]);
  fprintf(output, "#ifdef USE_GDB\n");
  fprintf(output, "%ssignal(SIGUSR2, sigusr2_handler);\n", INDENT[1]);
  fprintf(output, "#endif\n");
  fprintf(output, "#ifndef AC_COMPSIM\n");
  fprintf(output, "%sset_running();\n", INDENT[1]);
  fprintf(output, "#else\n");
  fprintf(output, "%sac_pc = 0;\n", INDENT[1]);
  fprintf(output, "%svoid Execute(int argc, char *argv[]);\n", INDENT[1]);
  fprintf(output, "%sExecute(argc, argv);\n", INDENT[1]);
  fprintf(output, "#endif\n");
  fprintf(output, "}\n\n");

  /* stop() */
  fprintf(output, "//Stop simulation (may receive exit status)\n");
  fprintf(output, "void %s::stop(int status) {\n", project_name);
  fprintf(output, "%scerr << \"ArchC: -------------------- Simulation Finished --------------------\" << endl;\n", INDENT[1]);
  fprintf(output, "%sISA._behavior_end();\n", INDENT[1]);
  fprintf(output, "%sac_stop_flag = 1;\n", INDENT[1]);
  fprintf(output, "%sac_exit_status = status;\n", INDENT[1]);
  fprintf(output, "#ifndef AC_COMPSIM\n");
  fprintf(output, "%sset_stopped();\n", INDENT[1]);
  fprintf(output, "#endif\n");
  fprintf(output, "}\n\n");


  //!END OF FILE.
  fclose(output);
  free(filename);
}


//!Create ArchC Resources Implementation File
void CreateResourceImpl() {

  extern ac_pipe_list *pipe_list;
  extern ac_sto_list *storage_list, *fetch_device;
  extern ac_stg_list *stage_list;
  extern int HaveMultiCycleIns, HaveMemHier, reg_width; 
  extern ac_sto_list* load_device;

  ac_sto_list *pstorage, *pmem;
  ac_stg_list *pstage;
  char Globals[5000];
  char *Globals_p = Globals;
	ac_pipe_list *ppipe;

  FILE *output;
  char filename[] = "ac_resources.cpp";

  load_device= storage_list;
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }


  print_comment( output, "ArchC Resources Implementation file.");
  
  fprintf( output, "#include  \"ac_resources.H\"\n");
  fprintf( output, "#include  \"ac_storage.H\"\n");
  fprintf( output, "#include  \"ac_regbank.H\"\n");
  fprintf( output, "#include  \"ac_reg.H\"\n");

  if(ACVerifyFlag){
    fprintf( output, "#include  \"ac_msgbuf.H\"\n");
    fprintf( output, "#include  <sys/ipc.h>\n");
    fprintf( output, "#include  <unistd.h>\n");
    fprintf( output, "#include  <sys/msg.h>\n");
    fprintf( output, "#include  <sys/types.h>\n");
  }

  if( ACStatsFlag ){
    COMMENT(INDENT[0],"Statistics Object.");
    fprintf( output, "%sac_stats ac_resources::ac_sim_stats;\n", INDENT[0]);
    Globals_p += sprintf( Globals_p, "ac_stats &ac_sim_stats = ac_resources::ac_sim_stats;\n");
  }
  
  COMMENT(INDENT[0],"Storage Devices.");
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

    switch( pstorage->type ){

    case REG:

      //Formatted registers have a special class.
      if( pstorage->format != NULL ){
        fprintf( output, "%sac_%s ac_resources::%s(\"%s\");\n", INDENT[0], pstorage->name, pstorage->name, pstorage->name);
        Globals_p += sprintf( Globals_p, "ac_%s &%s = ac_resources::%s;\n", pstorage->name, pstorage->name, pstorage->name);
      }
      else{
        fprintf( output, "%sac_reg<unsigned> ac_resources::%s(\"%s\", 0);\n", INDENT[0], pstorage->name, pstorage->name);      
        Globals_p += sprintf( Globals_p, "ac_reg<unsigned> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      }
      break;
                        
    case REGBANK:
      //Emiting register bank. Checking is a register width was declared.
      switch( (unsigned)reg_width ){
      case 0:
        fprintf( output, "%sac_regbank<ac_word> ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
        Globals_p += sprintf( Globals_p, "ac_regbank<ac_word> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
        break;
      case 8:
        fprintf( output, "%sac_regbank<unsigned char> ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
        Globals_p += sprintf( Globals_p, "ac_regbank<unsigned char> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
        break;
      case 16:
        fprintf( output, "%sac_regbank<unsigned short> ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
        Globals_p += sprintf( Globals_p, "ac_regbank<unsigned short> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
        break;
      case 32:
        fprintf( output, "%sac_regbank<unsigned> ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
        Globals_p += sprintf( Globals_p, "ac_regbank<unsigned> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
        break;
      case 64:
        fprintf( output, "%sac_regbank<unsigned long long> ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
        Globals_p += sprintf( Globals_p, "ac_regbank<unsigned long long> &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
        break;
      default:
        AC_ERROR("Register width not supported: %d\n", reg_width);
        break;
      }
      break;

    case CACHE:
    case ICACHE:
    case DCACHE:

      if( !pstorage->parms ) { //It is a generic cache. Just emit a base container object.
        fprintf( output, "%sac_storage ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);
        Globals_p += sprintf( Globals_p, "ac_storage &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      }
      else{
        //It is an ac_cache object.
        EmitCacheDeclaration(output, pstorage, 0);
        Globals_p += sprintf( Globals_p, "ac_cache &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      }
      break;

    case MEM:

      if( !HaveMemHier ) { //It is a generic cache. Just emit a base container object.
        fprintf( output, "%sac_storage ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);
        Globals_p += sprintf( Globals_p, "ac_storage &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      }
      else{
        //It is an ac_mem object.
        fprintf( output, "%sac_mem ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);
        Globals_p += sprintf( Globals_p, "ac_mem &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      }
      break;

    default:
      fprintf( output, "%sac_storage ac_resources::%s(\"%s\", %d);\n", INDENT[0], pstorage->name, pstorage->name, pstorage->size);      
      Globals_p += sprintf( Globals_p, "ac_storage &%s = ac_resources::%s;\n", pstorage->name, pstorage->name);
      break;
    }
  }

  fprintf( output, "%sac_storage *ac_resources::IM;\n\n", INDENT[0]);
  fprintf( output, "%sac_storage *ac_resources::APP_MEM;\n\n", INDENT[0]);
  
  COMMENT(INDENT[0],"Control Variables.");
  fprintf( output, "ac_reg<unsigned> ac_resources::ac_pc(\"ac_pc\", 0xffffffff);\n");
  Globals_p += sprintf( Globals_p, "ac_reg<unsigned> &ac_pc = ac_resources::ac_pc;\n");
  fprintf( output, "unsigned ac_resources::ac_start_addr = 0;\n");
  Globals_p += sprintf( Globals_p, "unsigned &ac_start_addr = ac_resources::ac_start_addr;\n");
  fprintf( output, "unsigned long long ac_resources::ac_instr_counter = 0;\n");
  Globals_p += sprintf( Globals_p, "unsigned long long &ac_instr_counter = ac_resources::ac_instr_counter;\n");
  fprintf( output, "unsigned long long ac_resources::ac_cycle_counter = 0;\n");
  Globals_p += sprintf( Globals_p, "unsigned long long &ac_cycle_counter = ac_resources::ac_cycle_counter;\n");

  fprintf( output, "double ac_resources::time_step;\n");
  Globals_p += sprintf( Globals_p, "double &time_step = ac_resources::time_step;\n");

  if(HaveMultiCycleIns) 
    fprintf( output, "unsigned ac_resources::ac_cycle;\n");

  fprintf( output, "bool ac_resources::ac_tgt_endian = %d;\n", ac_tgt_endian);
  Globals_p += sprintf( Globals_p, "bool &ac_tgt_endian = ac_resources::ac_tgt_endian;\n");


  //TODO: Test wait signal for pipelined archs
  fprintf( output, "%sbool ac_resources::ac_wait_sig;\n", INDENT[0]);
  Globals_p += sprintf( Globals_p, "bool &ac_wait_sig = ac_resources::ac_wait_sig;\n");

  fprintf( output, "%sbool ac_resources::ac_parallel_sig;\n", INDENT[0]);

  if( !stage_list && !pipe_list ){
    fprintf( output, "%sbool ac_resources::ac_annul_sig;\n", INDENT[0]);
    Globals_p += sprintf( Globals_p, "bool &ac_annul_sig = ac_resources::ac_annul_sig;\n");
  }

  //Emitting stall variables
  if( stage_list ){
    for( pstage = stage_list; pstage != NULL; pstage=pstage->next)
      if( pstage->next ){
        fprintf( output, "%sbool ac_resources::%s_stall;\n", INDENT[0], pstage->name);
        Globals_p += sprintf( Globals_p, "bool &%s_stall = ac_resources::%s_stall;\n", pstage->name, pstage->name);
      }
		
    for( pstage = stage_list; pstage != NULL; pstage=pstage->next)
      if( pstage->next ){
        fprintf( output, "%sbool ac_resources::%s_flush;\n", INDENT[0], pstage->name);
        Globals_p += sprintf( Globals_p, "bool &%s_flush = ac_resources::%s_flush;\n", pstage->name, pstage->name);
      }

  }else if(pipe_list){
	
    for( ppipe = pipe_list; ppipe!=NULL; ppipe= ppipe->next ){

      for( pstage = ppipe->stages; pstage != NULL; pstage=pstage->next)
        if( pstage->next ){
          fprintf( output, "%sbool ac_resources::%s_%s_stall;\n", INDENT[0], ppipe->name, pstage->name);
          Globals_p += sprintf( Globals_p, "bool &%s_%s_stall = ac_resources::%s_%s_stall;\n", ppipe->name, pstage->name, ppipe->name, pstage->name);
        }
		
      for( pstage = ppipe->stages ; pstage != NULL; pstage=pstage->next)
        if( pstage->next ){
          fprintf( output, "%sbool ac_resources::%s_%s_flush;\n", INDENT[0], ppipe->name, pstage->name);
          Globals_p += sprintf( Globals_p, "bool &%s_%s_flush = ac_resources::%s_%s_flush;\n", ppipe->name, pstage->name, ppipe->name, pstage->name);
        }
    }
  } 

  fprintf( output, "\n");

  COMMENT(INDENT[0], "Program arguments.");
  fprintf( output, "int ac_resources::argc;\n");
  fprintf( output, "char ** ac_resources::argv;\n");
        
  if( ACDasmFlag ){
    COMMENT(INDENT[0], "Disassembler file.");
    fprintf( output, "%sofstream ac_resources::dasmfile;\n", INDENT[1]);
  }

  //!Declaring Constructor.
  COMMENT(INDENT[0],"Constructor.\n");
  fprintf( output, "%sac_resources::ac_resources(){\n\n", INDENT[0]);

  COMMENT(INDENT[1],"Initializing.\n");
  for( pstage = stage_list; pstage != NULL; pstage=pstage->next)
    if( pstage->next )
      fprintf( output, "%s%s_stall  =0;\n", INDENT[1], pstage->name);

  fprintf( output, "\n");
  for( pstage = stage_list; pstage != NULL; pstage=pstage->next)
    if( pstage->next )
      fprintf( output, "%s%s_flush  =0;\n", INDENT[1], pstage->name);
      
  if(HaveMultiCycleIns) 
    fprintf( output, "%sac_cycle = 1;\n", INDENT[1]);
  
  fprintf( output, "%sac_tgt_endian = %d;\n", INDENT[1], ac_tgt_endian);
  fprintf( output, "%sac_start_addr = 0;\n", INDENT[1]);
  fprintf( output, "%sac_instr_counter = 0;\n", INDENT[1]);
  fprintf( output, "%sac_cycle_counter = 0;\n", INDENT[1]);
  fprintf( output, "%sac_wait_sig = 0;\n", INDENT[1]);
  fprintf( output, "%sac_parallel_sig = 0;\n", INDENT[1]);

  if( !stage_list && !pipe_list )
    fprintf( output, "%sac_annul_sig = 0;\n", INDENT[1]);


  /* Determining which device is gonna be used for fetching instructions */
  if( !fetch_device ){
    //The parser has not determined because there is not an ac_icache obj declared.
    //In this case, look for the object with the lowest (zero) hierarchy level.
    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next)
      if( pstorage->level == 0 &&  pstorage->type != REG &&  pstorage->type != REGBANK)
        fetch_device = pstorage;

    if( !fetch_device ) { //Couldn't find a fetch device. Error!
      AC_INTERNAL_ERROR("Could not determine a device for fetching.");
      exit(1);
    }
  }

  fprintf( output, "%sIM = &%s;\n", INDENT[1], fetch_device->name);

  /* Determining which device is going to be used for loading applications*/
  /* The device used for loading applications must be the one in the highest
     level of a memory hierachy.*/
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    if(pstorage->level > load_device->level)
      load_device = pstorage;
  }

  /* If there is only one level, which is gonna be zero, then it is the same
     object used for fetching. */
  if( load_device->level ==0 )
    load_device = fetch_device;

  fprintf( output, "%sAPP_MEM = &%s;\n", INDENT[1], load_device->name);

  fprintf( output, "\n");      

  /* Connecting memory hierarchy */
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next)
    if( pstorage->higher ){
      fprintf( output, "%s%s.bindToNext(%s);\n", INDENT[1], pstorage->name, pstorage->higher->name );
    }
        
  fprintf( output, "}\n\n");

  fprintf( output, "\n");      

  if(ACVerifyFlag){
    int ndevice=0;

    //Set co-verification msg queue method
    fprintf( output, "void ac_resources::set_queue(char* exec_name){\n\n" );
    fprintf( output, "%sstruct start_msgbuf sbuf;\n", INDENT[1] );
    fprintf( output, "%sstruct dev_msgbuf dbuf;\n", INDENT[1] );
    fprintf( output, "%sextern key_t key;\n", INDENT[1] );
    fprintf( output, "%sextern int msqid;\n", INDENT[1] );

    fprintf( output, "%sif ((key = ftok(exec_name, 'A')) == -1) {\n", INDENT[1] );
    fprintf( output, "%sAC_ERROR(\"Could not attach to the co-verification msg queue. Process:\" << getpid());\n", INDENT[2] );
    fprintf( output, "%sperror(\"ftok\");\n", INDENT[2] );
    fprintf( output, "%sexit(1);\n", INDENT[2] );
    fprintf( output, "%s}\n", INDENT[1] );
		
    fprintf( output, "%sif ((msqid = msgget(key, 0644)) == -1) {\n", INDENT[1] );
    fprintf( output, "%sAC_ERROR(\"Could not attach to the co-verification msg queue. Process:\" << getpid());\n", INDENT[2] );
    fprintf( output, "%sperror(\"msgget\");\n", INDENT[2] );
    fprintf( output, "%sexit(1);\n", INDENT[2] );
    fprintf( output, "%s}\n", INDENT[1] );

    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

      if( pstorage->type == MEM ||
          pstorage->type == ICACHE ||
          pstorage->type == DCACHE ||
          pstorage->type == CACHE ||
          pstorage->type == REGBANK )
        ndevice++;
    }

    fprintf( output, "%ssbuf.mtype = 1;\n", INDENT[1] );
    fprintf( output, "%ssbuf.ndevice =%d;\n", INDENT[1], ndevice );

    fprintf( output, "%sif (msgsnd(msqid, (void*)&sbuf, sizeof(sbuf), 0) == -1)\n", INDENT[1] );
    fprintf( output, "%sperror(\"msgsnd\");\n", INDENT[2] );
    fprintf( output, "\n" );

    fprintf( output, "%sdbuf.mtype =2;\n", INDENT[1] );

    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

      if( pstorage->type == MEM ||
          pstorage->type == ICACHE ||
          pstorage->type == DCACHE ||
          pstorage->type == CACHE ||
          pstorage->type == REGBANK ){

        fprintf( output, "%sstrcpy(dbuf.name,%s.get_name());\n", INDENT[1], pstorage->name );

        fprintf( output, "%sif (msgsnd(msqid, (void*)&dbuf, sizeof(dbuf), 0) == -1)\n", INDENT[1] );
        fprintf( output, "%sperror(\"msgsnd\");\n", INDENT[2] );
        fprintf( output, "\n" );
      }
    }
    fprintf( output, "}\n");   //End of set_queue
  }

  COMMENT(INDENT[0],"Global aliases for resources.");
  fprintf( output, "%s\n", Globals);

  fclose( output); 
  
}

/** Creates the _arch.cpp Implementation File. */
void CreateArchImpl() {

  extern ac_pipe_list *pipe_list;
  extern ac_sto_list *storage_list, *fetch_device;
  extern ac_stg_list *stage_list;
  extern int HaveMultiCycleIns, HaveMemHier, reg_width; 
  extern ac_sto_list* load_device;

  extern char *project_name;

  ac_sto_list *pstorage, *pmem;
  ac_stg_list *pstage;
  char Globals[5000];
  char *Globals_p = Globals;
	ac_pipe_list *ppipe;

  FILE *output;
  char filename[256];

  sprintf(filename, "%s_arch.cpp", project_name);

  load_device= storage_list;
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }


  print_comment( output, "ArchC Resources Implementation file.");
  
  fprintf( output, "#include \"%s_arch.H\"\n", project_name);

  if(ACVerifyFlag){
    fprintf( output, "#include  \"ac_msgbuf.H\"\n");
    fprintf( output, "#include  <sys/ipc.h>\n");
    fprintf( output, "#include  <unistd.h>\n");
    fprintf( output, "#include  <sys/msg.h>\n");
    fprintf( output, "#include  <sys/types.h>\n");
  }

  fprintf(output, "\n");

  /* Emitting Constructor */
  fprintf(output, "%s%s_arch::%s_arch() :\n", INDENT[0], project_name, project_name);
  fprintf(output, "%sac_arch_dec_if<%s_parms::ac_word, %s_parms::ac_Hword>(%s_parms::AC_MAX_BUFFER),\n", INDENT[1], project_name, project_name, project_name);

  /* Mudar para construção */
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

    switch( pstorage->type ){

    case REG:

      //Formatted registers have a special class.
      if( pstorage->format != NULL ){
        fprintf( output, "%s%s(*this, \"%s\", time_step)", INDENT[1], pstorage->name, pstorage->name);
      }
      else{
        fprintf( output, "%s%s(\"%s\", time_step)", INDENT[1], pstorage->name, pstorage->name);
      }
      break;

    case REGBANK:
      //Emiting register bank. Checking is a register width was declared.
      fprintf( output, "%s%s(\"%s\", time_step)", INDENT[1], pstorage->name, pstorage->name);
      break;

    case CACHE:
    case ICACHE:
    case DCACHE:

      if( !pstorage->parms ) { //It is a generic cache. Just emit a base container object.
        fprintf(output, "%s%s_stg(\"%s_stg\", %d),\n", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
        fprintf( output, "%s%s(*this, %s_stg)", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
      }
      else{
        //It is an ac_cache object.
        EmitCacheDeclaration(output, pstorage, 0);
      }
      break;

    case MEM:

      if( !HaveMemHier ) { //It is a generic cache. Just emit a base container object.
        fprintf(output, "%s%s_stg(\"%s_stg\", %d),\n", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
        fprintf( output, "%s%s(*this, %s_stg)", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
      }
      else{
        //It is an ac_mem object.
        fprintf(output, "%s%s_stg(\"%s_stg\", %d),\n", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
        fprintf( output, "%s%s(*this, %s_stg)", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
      }
      break;
      
    default:
      fprintf(output, "%s%s_stg(\"%s_stg\", %d),\n", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
      fprintf( output, "%s%s(*this, %s_stg)", INDENT[1], pstorage->name, pstorage->name, pstorage->size);
      break;
    }
    if (pstorage->next != NULL)
      fprintf(output, ",\n");
  }

  /* opening constructor body */
  fprintf(output, " {\n\n");

  /* setting endianness match */
  fprintf(output, "%sac_mt_endian = %s_parms::AC_MATCH_ENDIAN;\n\n", INDENT[1], project_name);

  /* Determining which device is gonna be used for fetching instructions */
  if( !fetch_device ){
    //The parser has not determined because there is not an ac_icache obj declared.
    //In this case, look for the object with the lowest (zero) hierarchy level.
    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next)
      if( pstorage->level == 0 &&  pstorage->type != REG &&  pstorage->type != REGBANK)
        fetch_device = pstorage;

    if( !fetch_device ) { //Couldn't find a fetch device. Error!
      AC_INTERNAL_ERROR("Could not determine a device for fetching.");
      exit(1);
    }
  }

  fprintf( output, "%sIM = &%s;\n", INDENT[1], fetch_device->name);

  /* Determining which device is going to be used for loading applications*/
  /* The device used for loading applications must be the one in the highest
     level of a memory hierachy.*/
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    if(pstorage->level > load_device->level)
      load_device = pstorage;
  }

  /* If there is only one level, which is gonna be zero, then it is the same
     object used for fetching. */
  if( load_device->level ==0 )
    load_device = fetch_device;

  fprintf( output, "%sAPP_MEM = &%s;\n", INDENT[1], load_device->name);

  fprintf( output, "\n");      

  /* Connecting memory hierarchy */
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next)
    if( pstorage->higher ){
      fprintf( output, "%s%s.bindToNext(%s);\n", INDENT[1], pstorage->name, pstorage->higher->name );
    }
        
  fprintf( output, "}\n\n");  

}

//!Creates the ARCH Implementation File.
void CreateARCHImpl() {

  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list;
  extern ac_stg_list *stage_list;
  extern ac_sto_list *storage_list;
  extern char *project_name;
  extern int stage_num;
  ac_sto_list *pstorage;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;
  int i;

  char filename[256];
  char description[] = "Architecture Module implementation file.";
 
  //! File containing ISA declaration
  FILE  *output;
  
  sprintf( filename, "%s_arch.cpp", project_name);
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, description);

  fprintf( output, "#include \"ac_parms.H\"\n");
  fprintf( output, "#include  \"%s_arch.H\"\n", project_name);
  if( ACVerifyFlag ){
    fprintf( output, "#include  \"ac_msgbuf.H\"\n");
    fprintf( output, "#include  \"sys/msg.h\"\n");
  }
  fprintf( output, " \n");

  //Emiting Verification Method.
  COMMENT(INDENT[0],"Verification method.\n");               
  fprintf( output, "%svoid %s_arch::ac_verify(){\n", INDENT[0], project_name);        

  if( ACVerifyFlag ){

    fprintf( output, "extern int msqid;\n");
    fprintf( output, "struct log_msgbuf lbuf;\n");
    fprintf( output, "log_list::iterator itor;\n");
    fprintf( output, "log_list *plog;\n");
  }
  fprintf( output, " \n");


  fprintf( output, "%sif( ", INDENT[1]);

  if(stage_list){
    for( i =1; i<= stage_num-1; i++)    
      fprintf( output, "st%d_done.read() && \n%s", i, INDENT[2]); 
    fprintf( output, "st%d_done.read() )\n", stage_num); 
  }
  else if ( pipe_list ){

    for( ppipe = pipe_list; ppipe != NULL; ppipe = ppipe->next ){

      for( pstage = ppipe->stages; pstage->next != NULL; pstage=pstage->next)
        fprintf( output, "%s%s_%s_done.read() && \n", INDENT[1], ppipe->name, pstage->name);

      if( ppipe->next )  //If we have another pipe in the list, do it normally, otherwise, close if condition
        fprintf( output, "%s%s_%s_done.read() && \n", INDENT[1], ppipe->name, pstage->name);
      else
        fprintf( output, "%s%s_%s_done.read() )\n", INDENT[1], ppipe->name, pstage->name);
				
    }
  }
  else{
    fprintf( output, "done.read() )\n"); 
  }

  fprintf( output, "%s  {\n", INDENT[2]);

    
  fprintf( output, "#ifdef AC_VERBOSE\n");
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    fprintf( output, "%s%s.change_dump(cerr);\n", INDENT[3],pstorage->name );
  }
  fprintf( output, "#endif\n");


  fprintf( output, "#ifdef AC_UPDATE_LOG\n");

  if( ACVerifyFlag ){

    int next_type = 3;

    fprintf( output, "%sif( sc_simulation_time() ){\n", INDENT[3]);

    //Sending logs for every storage device. We just consider for co-verification caches, regbanks and memories
    for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){

      if( pstorage->type == MEM ||
          pstorage->type == ICACHE ||
          pstorage->type == DCACHE ||
          pstorage->type == CACHE ||
          pstorage->type == REGBANK ){

        fprintf( output, "%splog = %s.get_changes();\n", INDENT[4],pstorage->name );
        fprintf( output, "%sif( plog->size()){\n", INDENT[4] );
        fprintf( output, "%sitor = plog->begin();\n", INDENT[5] );
        fprintf( output, "%slbuf.mtype = %d;\n", INDENT[5], next_type );
        fprintf( output, "%swhile( itor != plog->end()){\n\n", INDENT[5] );
        fprintf( output, "%slbuf.log = *itor;\n", INDENT[6] );
        fprintf( output, "%sif( msgsnd(msqid, (struct log_msgbuf *)&lbuf, sizeof(lbuf), 0) == -1)\n", INDENT[6] );
        fprintf( output, "%sperror(\"msgsnd\");\n", INDENT[7] );
        fprintf( output, "%sitor = plog->erase(itor);\n", INDENT[6] );
        fprintf( output, "%s}\n", INDENT[5] );
        fprintf( output, "%s}\n\n", INDENT[4] );
			
        next_type++;
      }
    }
    fprintf( output, "%s}\n\n", INDENT[3] );
		
  }
  for( pstorage = storage_list; pstorage != NULL; pstorage=pstorage->next){
    //fprintf( output, "%s%s.change_save();\n", INDENT[3],pstorage->name );
    fprintf( output, "%s%s.reset_log();\n", INDENT[3],pstorage->name );          
  }
	
  fprintf( output, "#endif\n");


  if(stage_list){
    for( i =1; i<= stage_num; i++)    
      fprintf( output, "%sst%d_done.write(0);\n", INDENT[3], i);  
  }
  else  if ( pipe_list ){
		
    for( ppipe = pipe_list; ppipe != NULL; ppipe = ppipe->next ){
			
      for( pstage = ppipe->stages; pstage != NULL; pstage=pstage->next)
        fprintf( output, "%s%s_%s_done.write(0);\n", INDENT[1], ppipe->name, pstage->name);

    }
  }
  else{
    fprintf( output, "%sdone.write(0);\n", INDENT[3]); 
  }

  fprintf( output, "%s  }\n", INDENT[2]);
  fprintf( output, "%s}\n\n", INDENT[0]);

  //!Emit update method.
  if( stage_list )
    EmitPipeUpdateMethod( output);
  else if ( pipe_list )
    EmitMultiPipeUpdateMethod( output);
  else
    EmitUpdateMethod( output);

  //!END OF FILE.
  fclose(output);
}


/*!Create the template for the .cpp file where the user has
  the basic code for the main function. */
void CreateMainTmpl() {

  extern char *project_name;
  char filename[] = "main.cpp.tmpl";
  char description[256];
  FILE  *output;

  sprintf( description, "This is the main file for the %s ArchC model", project_name);
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, description);

  fprintf( output, "const char *project_name=\"%s\";\n", project_name);
  fprintf( output, "const char *project_file=\"%s\";\n", arch_filename);
  fprintf( output, "const char *archc_version=\"%s\";\n", ACVersion);
  fprintf( output, "const char *archc_options=\"%s\";\n", ACOptions);
  fprintf( output, "\n");

  fprintf( output, "#include  <systemc.h>\n");
  fprintf( output, "#include  \"%s.H\"\n\n", project_name);

  if (ACEncoderFlag) {
    if (ac_tgt_endian == 0)
      fprintf( output, "#define USE_LITTLE_ENDIAN\n");
    else
      fprintf( output, "//#define USE_LITTLE_ENDIAN\n");
    fprintf( output, "#include  \"ac_encoder.H\"\n");
  }

  if (ACGDBIntegrationFlag) {
    fprintf( output, "\n#include \"ac_gdb.H\"\n");
    fprintf( output, "AC_GDB *gdbstub;\n\n");
  }

  fprintf( output, "\n\n");
  fprintf( output, "int sc_main(int ac, char *av[])\n");
  fprintf( output, "{\n\n");

  COMMENT(INDENT[1],"Clock");               
  fprintf( output, "%ssc_clock clk(\"clk\", 20, 0.5, true);\n", INDENT[1]);

  COMMENT(INDENT[1],"%sISA simulator", INDENT[1]);               
  fprintf( output, "%s%s %s_proc1(\"%s\");\n\n", INDENT[1], project_name, project_name, project_name);
  fprintf( output, "%s%s_proc1(clk);\n\n", INDENT[1], project_name);

  if(ACGDBIntegrationFlag)
    fprintf( output, "%sgdbstub = new AC_GDB(%s_proc1. %s_mc, PORT_NUM);\n\n",
             INDENT[1], project_name, project_name );

  if (ACEncoderFlag) {
    fprintf( output, "%s//!Encoder tools\n", INDENT[1]);
    fprintf( output, "%sac_decoder_full *decoder = %s_proc1. %s_mc->ISA.decoder;\n", INDENT[1], project_name, project_name);
    fprintf( output, "%sac_encoder(ac,av,decoder);\n\n", INDENT[1]);
  }

  fprintf( output, "#ifdef AC_DEBUG\n");
  fprintf( output, "%sac_trace(\"%s_proc1.trace\");\n", INDENT[1], project_name);
  fprintf( output, "#endif \n\n");

  fprintf(output, "%s%s_proc1.init(ac, av, clk.period().to_double());\n", INDENT[1], project_name);
  fprintf(output, "%scerr << endl;\n\n", INDENT[1]);

  fprintf(output, "%ssc_start(-1);\n\n", INDENT[1]);

  fprintf(output, "%s%s_proc1.PrintStat();\n", INDENT[1], project_name);
  fprintf(output, "%scerr << endl;\n\n", INDENT[1]);

  fprintf( output, "#ifdef AC_STATS\n");
  fprintf( output, "%s%s_proc1.ac_sim_stats.time = sc_simulation_time();\n", INDENT[1], project_name);
  fprintf( output, "%s%s_proc1.ac_sim_stats.print();\n", INDENT[1], project_name);
  fprintf( output, "#endif \n\n");

  fprintf( output, "#ifdef AC_DEBUG\n");
  fprintf( output, "%sac_close_trace();\n", INDENT[1]);
  fprintf( output, "#endif \n\n");

  fprintf( output, "%sreturn %s_proc1.ac_exit_status;\n", INDENT[1], project_name);

  fprintf( output, "}\n");
}


/*!Create the template for the .cpp file where the user has
  to fill out the instruction and format behaviors. */
void CreateImplTmpl(){

  extern ac_dec_format *format_ins_list;
  extern ac_stg_list *stage_list;
  extern ac_dec_instr *instr_list;
  extern ac_dec_field *field_list;
  extern char *project_name;
  extern int wordsize;
  extern int declist_num;
  ac_dec_format *pformat;
  ac_dec_instr *pinstr;
  ac_stg_list *pstage;
  ac_dec_field *pdecfield;
  ac_dec_list* pdeclist;

  char filename[256];
  char description[] = "Behavior implementation file template.";
  char initfilename[256];

  //File containing ISA declaration
  FILE  *output;

  int i;
  int count_fields;
  
  sprintf( filename, "%s_isa.cpp.tmpl", project_name);
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }



  print_comment( output, description);
  fprintf( output, "#include  \"%s_isa.H\"\n", project_name);
  fprintf( output, "#include  \"%s\"\n", initfilename );
  fprintf( output, " \n");

  fprintf( output, " \n");

  //Behavior to begin simulation.
  COMMENT(INDENT[0],"Behavior executed before simulation begins.");
  fprintf( output, "%svoid ac_behavior( begin ){};\n", INDENT[0]);
  fprintf( output, "\n");

  //Behavior to end simulation.
  COMMENT(INDENT[0],"Behavior executed after simulation ends.");
  fprintf( output, "%svoid ac_behavior( end ){};\n", INDENT[0]);
  fprintf( output, "\n");

  //Declaring ac_instruction behavior method.
  COMMENT(INDENT[0],"Generic instruction behavior method.");               
  //Testing if should emit a switch or not.
  if( stage_list ){
    fprintf( output, "%svoid ac_behavior( instruction ){;\n\n", INDENT[0]); 
    fprintf( output, "%sswitch( stage ) {\n", INDENT[1]); 
                
    for( pstage = stage_list; pstage != NULL; pstage=pstage->next){
      fprintf( output, "%scase _%s:\n", INDENT[1], pstage->name); 
      fprintf( output, "%sbreak;\n", INDENT[1]); 
    }
                
    fprintf( output, "%sdefault:\n", INDENT[1]); 
                          fprintf( output, "%sbreak;\n", INDENT[1]); 
                          fprintf( output, "%s}\n", INDENT[1]);
                          fprintf( output, "};\n\n");  
  }
  else{
    fprintf( output, "%svoid ac_behavior( instruction ){};\n", INDENT[0]);
  }
        
  fprintf( output, " \n");


  //Declaring Instruction Format behavior methods.
  COMMENT(INDENT[0]," Instruction Format behavior methods.");               
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next)
    //Testing if should emit a switch or not.
    if( stage_list ){
      fprintf( output, "%svoid ac_behavior( %s ){\n\n", INDENT[0], pformat->name); 
      fprintf( output, "%sswitch( stage ) {\n", INDENT[1]); 
      
      for( pstage = stage_list; pstage != NULL; pstage=pstage->next){
        fprintf( output, "%scase _%s:\n", INDENT[1], pstage->name); 
        fprintf( output, "%sbreak;\n", INDENT[1]); 
      }
      
      fprintf( output, "%sdefault:\n", INDENT[1]); 
                            fprintf( output, "%sbreak;\n", INDENT[1]); 
                            fprintf( output, "%s}\n", INDENT[1]);
                            fprintf( output, "};\n\n");  
    }
    else{
      fprintf( output, "%svoid ac_behavior( %s ){}\n", INDENT[0], pformat->name); 
    }

  fprintf( output, " \n");


  //Declaring each instruction behavior method.
  for( pinstr = instr_list; pinstr!= NULL; pinstr=pinstr->next){
    
    //Testing if should emit a switch or not.
    if( stage_list ){
      COMMENT(INDENT[0],"Instruction %s behavior method.",pinstr->name);               
      fprintf( output, "%svoid ac_behavior( %s ){\n\n", INDENT[0], pinstr->name); 
      fprintf( output, "%sswitch( stage ) {\n", INDENT[1]); 
      
      for( pstage = stage_list; pstage != NULL; pstage=pstage->next){
        fprintf( output, "%scase _%s:\n", INDENT[1], pstage->name); 
        fprintf( output, "%sbreak;\n", INDENT[1]); 
      }
      
      fprintf( output, "%sdefault:\n", INDENT[1]); 
                            fprintf( output, "%sbreak;\n", INDENT[1]); 
                            fprintf( output, "%s}\n", INDENT[1]);
                            fprintf( output, "};\n\n");  
    }
    else{
      COMMENT(INDENT[0],"Instruction %s behavior method.",pinstr->name);               
      fprintf( output, "%svoid ac_behavior( %s ){}\n\n", INDENT[0], pinstr->name); 
    }
  }
  
  //!END OF FILE.
  fclose(output);


  //Now writing ISA initialization file.
  sprintf( initfilename, "%s_isa_init.cpp", project_name);
  if ( !(output = fopen( initfilename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }
    
  /* ac_isa_init creation starts here */

  print_comment( output, "AC_ISA Initialization File");

  /* Creating static decoder tables */
  fprintf( output, "%s#include \"%s_isa.H\"\n", INDENT[0], project_name); 
  COMMENT(INDENT[0],"Fields table declaration.");               

  /* Creating field table */
  fprintf(output, "ac_dec_field %s_isa::fields[%s_parms::AC_DEC_FIELD_NUMBER] = {\n",
          project_name, project_name);
  i = 0;
  for (pformat = format_ins_list; pformat != NULL; pformat = pformat->next) {
    for (pdecfield = pformat->fields; pdecfield != NULL; pdecfield = pdecfield->next) {
      /* fprintf {char* name, int size, int first_bit, int id, long val, int sign, next} */
      fprintf(output, "%s{\"%s\", %d, %d, %d, %ld, %d, ",
              INDENT[1],
              pdecfield->name,
              pdecfield->size,
              pdecfield->first_bit,
              pdecfield->id,
              pdecfield->val,
              pdecfield->sign);
      if (pdecfield->next)
        fprintf(output, "&(%s_isa::fields[%d])},\n", project_name, i + 1);
      else
        fprintf(output, "NULL}");

      i++;
    }
    if (pformat->next)
      fprintf(output, ",\n");
  }
  fprintf(output, "\n};\n\n");

  /* Creating format structure */
  fprintf(output, "ac_dec_format %s_isa::formats[%s_parms::AC_DEC_FORMAT_NUMBER] = {\n",
          project_name, project_name);
  i = 0;
  count_fields = 0;
  for( pformat = format_ins_list; pformat!= NULL; pformat = pformat->next){
    /* fprintf char* name, int size, ac_dec_field* fields, next */
    fprintf(output, "%s{\"%s\", %d, &(%s_isa::fields[%d]), ",
            INDENT[1],
            pformat->name,
            pformat->size,
            project_name,
            count_fields);
    if (pformat->next)
      fprintf(output, "&(%s_isa::formats[%d])},\n",
              project_name,
              i + 1);
    else
      fprintf(output, "NULL}");

    i++;
    for( pdecfield = pformat->fields; pdecfield!= NULL; pdecfield=pdecfield->next)
      count_fields++;
  }
  fprintf(output, "\n};\n\n");

  /* Creating decode list structure */
  fprintf(output, "ac_dec_list %s_isa::dec_list[%s_parms::AC_DEC_LIST_NUMBER] = {\n",
          project_name, project_name);
  i = 0;
  for (pinstr = instr_list; pinstr != NULL; pinstr = pinstr->next) {
    for (pdeclist = pinstr->dec_list; pdeclist != NULL;
         pdeclist = pdeclist->next) {
      /* fprintf char* name, int value, ac_dec_list* next  */
      fprintf(output, "%s{\"%s\", %d, ",
              INDENT[1],
              pdeclist->name,
              pdeclist->value);
      if (pdeclist->next)
        fprintf(output, "&(%s_isa::dec_list[%d])},\n", project_name, i + 1);
      else
        fprintf(output, "NULL}");

      i++;
    }
    if (pinstr->next)
      fprintf(output, ",\n");
  }
  fprintf(output, "\n};\n\n");
  declist_num = i;

  /* Creating instruction structure */
  fprintf(output,
          "ac_dec_instr %s_isa::instructions[%s_parms::AC_DEC_INSTR_NUMBER] = {\n",
          project_name, project_name);
  i = 0;
  count_fields = 0;
  for (pinstr = instr_list; pinstr != NULL; pinstr = pinstr->next) {
    /* fprintf char* name, int size, char* mnemonic, char* asm_str, char* format, unsigned id, unsigned cycles, unsigned min_latency, unsigned max_latency, ac_dec_list* dec_list, ac_control_flow* cflow, ac_dec_instr* next */
    fprintf(output, "%s{\"%s\", %d, \"%s\", \"%s\", \"%s\", %d, %d, %d, %d, &(%s_isa::dec_list[%d]), %d, ",
            INDENT[1],
            pinstr->name,
            pinstr->size,
            pinstr->mnemonic,
            pinstr->asm_str,
            pinstr->format,
            pinstr->id,
            pinstr->cycles,
            pinstr->min_latency,
            pinstr->max_latency,
            project_name,
            count_fields,
            0);
    if (pinstr->next)
      fprintf(output, "&(%s_isa::instructions[%d])},\n", project_name, i + 1);
    else
      fprintf(output, "NULL}\n");
    for (pdeclist = pinstr->dec_list; pdeclist != NULL; pdeclist = pdeclist->next)
      count_fields++;
    i++;
  }
  fprintf(output, "};\n\n");

  /* Creating instruction table */
  fprintf(output, "const ac_instr_info<%s_isa>\n", project_name);
  fprintf(output, "%s_isa::instr_table[%s_parms::AC_DEC_INSTR_NUMBER + 1] = {\n",
          project_name, project_name);
  fprintf(output, "%sac_instr_info<%s_isa>(0, 0, 0, \"_ac_invalid_\", \"_ac_invalid_\", %d),\n", INDENT[1], project_name, wordsize);
  for (pinstr = instr_list; pinstr != NULL; pinstr = pinstr->next) {
    fprintf(output, "%sac_instr_info<%s_isa>(%d, &%s_isa::behavior_%s, &%s_isa::_behavior_%s_%s, \"%s\", \"%s\", %d)",
            INDENT[1],
            project_name,
            pinstr->id,
            project_name,
            pinstr->name,
            project_name,
            project_name,
            pinstr->format,
            pinstr->name,
            pinstr->mnemonic,
            pinstr->size);
    if (pinstr->next)
      fprintf(output, ",\n");
  }
  fprintf(output, "\n};\n");

  //!END OF FILE.
  fclose(output);

}


//!Creates Formatted Registers Implementation File.
void CreateRegsImpl() {
  extern ac_sto_list *storage_list;

  ac_sto_list *pstorage;

  FILE *output;
  char filename[] = "ac_regs.cpp.tmpl";
  char description[] = "Formatted Register Behavior implementation file.";
 
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }



  print_comment( output, description);

  fprintf( output, "#include \"ac_fmt_regs.H\"\n");
  fprintf( output, "#include \"archc.H\"\n");
  fprintf( output, " \n");

  //Declaring formatted register behavior methods.
  for( pstorage = storage_list; pstorage != NULL; pstorage = pstorage->next ){

    if(( pstorage->type == REG ) && (pstorage->format != NULL )){
      fprintf( output, "%svoid ac_behavior( %s ){}\n", INDENT[0], pstorage->name); 
    }
  }
  //END OF FILE.
  fclose(output);
}


//!Create ArchC model syscalls
void CreateArchSyscallHeader()
{
  extern char *project_name;
  FILE *output;
  char filename[50];

  snprintf(filename, 50, "%s_syscall.H", project_name);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }
    

  print_comment( output, "ArchC Architecture Dependent Syscall header file.");

  fprintf(output,
          "#ifndef %s_SYSCALL_H\n"
          "#define %s_SYSCALL_H\n"
          "\n"
          "#include \"%s_arch.H\"\n"
          "#include \"%s_arch_ref.H\"\n"
          "#include \"%s_parms.H\"\n"
          "#include \"ac_syscall.H\"\n"
          "\n"
          "//%s system calls\n"
          "class %s_syscall : public ac_syscall<%s_parms::ac_word, %s_parms::ac_Hword>, public %s_arch_ref\n"
          "{\n"
          "public:\n"
          "  %s_syscall(%s_arch& ref) : ac_syscall<%s_parms::ac_word, %s_parms::ac_Hword>(ref), %s_arch_ref(ref) {};\n"
          "  virtual ~%s_syscall() {};\n\n"
          "  void get_buffer(int argn, unsigned char* buf, unsigned int size);\n"
          "  void set_buffer(int argn, unsigned char* buf, unsigned int size);\n"
          "  void set_buffer_noinvert(int argn, unsigned char* buf, unsigned int size);\n"
          "  int  get_int(int argn);\n"
          "  void set_int(int argn, int val);\n"
          "  void return_from_syscall();\n"
          "  void set_prog_args(int argc, char **argv);\n"
          "};\n"
          "\n"
          "#endif\n"
          , project_name, project_name, project_name, project_name,
          project_name, project_name, project_name, project_name,
          project_name, project_name, project_name, project_name,
          project_name, project_name, project_name, project_name);

  fclose( output);
}


//!Create Makefile
void CreateMakefile(){

  extern ac_dec_format *format_ins_list;
  extern ac_pipe_list* pipe_list;
  extern ac_stg_list* stage_list;
  extern char *project_name;
  extern int HaveMemHier;
  extern int HaveFormattedRegs;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;
  ac_dec_format *pformat;
  FILE *output;
  char filename[] = "Makefile.archc";
 
  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  COMMENT_MAKE("####################################################");
  COMMENT_MAKE("This is the Makefile for building the %s ArchC model", project_name);
  COMMENT_MAKE("This file is automatically generated by ArchC");
  COMMENT_MAKE("WITHOUT WARRANTY OF ANY KIND, either express");
  COMMENT_MAKE("or implied.");
  COMMENT_MAKE("For more information on ArchC, please visit:   ");
  COMMENT_MAKE("http://www.archc.org                           ");
  COMMENT_MAKE("                                               ");
  COMMENT_MAKE("The ArchC Team                                 ");
  COMMENT_MAKE("Computer Systems Laboratory (LSC)              ");
  COMMENT_MAKE("IC-UNICAMP                                     ");
  COMMENT_MAKE("http://www.lsc.ic.unicamp.br                   ");
  COMMENT_MAKE("####################################################");

  fprintf( output, "\n\n");

  COMMENT_MAKE("Variable that points to SystemC installation path");
  fprintf( output, "SYSTEMC := %s\n", SYSTEMC_PATH);

  fprintf( output, "\n\n");
  COMMENT_MAKE("Variable that points to ArchC installation path");
  fprintf( output, "ARCHC := %s\n", ARCHC_PATH);

  fprintf( output, "\n");

  COMMENT_MAKE("Target Arch used by SystemC");
  fprintf( output, "TARGET_ARCH := %s\n", TARGET_ARCH);

  fprintf( output, "\n\n");
        
  fprintf( output, "INC_DIR := -I. -I$(ARCHC)/include -I$(SYSTEMC)/include\n");
  fprintf( output, "LIB_DIR := -L. -L$(SYSTEMC)/lib-$(TARGET_ARCH)\n");

  fprintf( output, "\n");
 
  fprintf( output, "LIB_SYSTEMC := %s\n",
           (strlen(SYSTEMC_PATH) > 2) ? "-lsystemc" : "");
  fprintf( output, "LIBS := $(LIB_SYSTEMC) -lm $(EXTRA_LIBS)\n");
  fprintf( output, "CC :=  %s\n", CC_PATH);
  fprintf( output, "OPT :=  %s\n", OPT_FLAGS);
  fprintf( output, "DEBUG :=  %s\n", DEBUG_FLAGS);
  fprintf( output, "OTHER :=  %s\n", OTHER_FLAGS);
  fprintf( output, "CFLAGS := $(DEBUG) $(OPT) $(OTHER) %s\n",
           (ACGDBIntegrationFlag) ? "-DUSE_GDB" : "" );

  fprintf( output, "\n");

  fprintf( output, "MODULE := %s\n", project_name);

  fprintf( output, "\n");
 
  //Declaring ACSRCS variable

  COMMENT_MAKE("These are the source files automatically generated by ArchC, that must appear in the SRCS variable");
  fprintf( output, "ACSRCS := $(MODULE)_arch.cpp $(MODULE)_arch_ref.cpp ");

  //Checking if we have a pipelined architecture or not.
  if( stage_list  ){  //List of ac_stage declarations. Used only for single pipe archs
                
    for( pstage = stage_list; pstage!= NULL; pstage = pstage->next)
      fprintf( output, "%s.cpp ", pstage->name);
  }
  else if( pipe_list ){  //Pipeline list exist. Used for ac_pipe declarations.
                        
    for(ppipe = pipe_list; ppipe!= NULL; ppipe=ppipe->next){

      for(pstage=ppipe->stages; pstage!= NULL; pstage=pstage->next)
        fprintf( output, "%s_%s.cpp ", ppipe->name, pstage->name);
    }
  }
  else{    //No pipe was declared. There is just the processor module source file

    fprintf( output, "$(MODULE).cpp");
  }     

  fprintf( output, "\n\n");
 
  //Declaring ACINCS variable
  COMMENT_MAKE("These are the source files automatically generated  by ArchC that are included by other files in ACSRCS");
  fprintf( output, "ACINCS := $(MODULE)_isa_init.cpp");

  fprintf( output, "\n\n");

  //Declaring ACHEAD variable

  COMMENT_MAKE("These are the header files automatically generated by ArchC");
  fprintf( output, "ACHEAD := $(MODULE)_parms.H $(MODULE)_arch.H $(MODULE)_arch_ref.H $(MODULE)_isa.H $(MODULE)_bhv_macros.H ");

  if(HaveFormattedRegs)
    fprintf( output, "ac_fmt_regs.H ");

  if(ACStatsFlag)
    fprintf( output, "ac_stats.H ");

  //Checking if we have a pipelined architecture or not.
  if( stage_list  ){  //List of ac_stage declarations. Used only for single pipe archs
                
    for( pstage = stage_list; pstage!= NULL; pstage = pstage->next)
      fprintf( output, "%s.H ", pstage->name);
  }
  else if( pipe_list ){  //Pipeline list exist. Used for ac_pipe declarations.
                        
    for(ppipe = pipe_list; ppipe!= NULL; ppipe=ppipe->next){

      for(pstage=ppipe->stages; pstage!= NULL; pstage=pstage->next)
        fprintf( output, "%s_%s.H ", ppipe->name, pstage->name);
    }
  }
  else{    //No pipe was declared. There is just the processor module source file

    fprintf( output, "$(MODULE).H ");
  }     

  if(ACABIFlag)
    fprintf( output, "$(MODULE)_syscall.H ");

  /* INSTRUCTION TYPES HEADER FILES */
  fprintf( output, "$(MODULE)_instruction.H ");
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next) {
    fprintf(output, "$(MODULE)_type_%s.H ", pformat->name);
  }

  fprintf( output, "\n\n");
 
 
  //Declaring FILES variable
  COMMENT_MAKE("These are the source files provided by ArchC that must be compiled together with the ACSRCS");
  COMMENT_MAKE("They are stored in the archc/src/aclib directory");
  fprintf( output, "ACFILES := ac_utils.cpp %s",
           (ACGDBIntegrationFlag)?"ac_gdb.cpp breakpoints.cpp ":"");

  if( HaveMemHier )
    fprintf( output, "ac_cache.cpp ac_mem.cpp ac_cache_if.cpp ");

  if(ACStatsFlag)
    fprintf( output, "ac_stats.cpp ");
                
  if(ACABIFlag)
    fprintf( output, "ac_syscall.cpp ");
                
  if(ACEncoderFlag)
    fprintf( output, "ac_encoder.cpp ");
                
  fprintf( output, "\n\n");

  //Declaring ACLIBFILES variable
  COMMENT_MAKE("These are the library files provided by ArchC");
  COMMENT_MAKE("They are stored in the archc/lib directory");
  fprintf( output, "ACLIBFILES := ac_decoder_rt.o ac_module.o ac_storage.o \n\n");

  //Declaring FILESHEAD variable
  COMMENT_MAKE("These are the headers files provided by ArchC");
  COMMENT_MAKE("They are stored in the archc/include directory");
  fprintf( output, "ACFILESHEAD := $(ACFILES:.cpp=.H) $(ACLIBFILES:.o=.H) ac_regbank.H ac_debug_model.H ac_sighandlers.H ac_ptr.H ac_memport.H ac_arch.H ac_arch_dec_if.H ac_arch_ref.H \n\n");

  //Declaring SRCS variable
  COMMENT_MAKE("These are the source files provided by the user + ArchC sources");
  fprintf( output, "SRCS := main.cpp $(ACSRCS) $(ACFILES) $(MODULE)_isa.cpp %s",
           (ACGDBIntegrationFlag)?"$(MODULE)_gdb_funcs.cpp":"");

  if(ACABIFlag)
    fprintf( output, " $(MODULE)_syscall.cpp");

  fprintf( output, "\n\n");

  //Declaring OBJS variable
  fprintf( output, "OBJS := $(SRCS:.cpp=.o)\n\n");

  fprintf( output, "\n");
  //Declaring Executable name
  fprintf( output, "EXE := $(MODULE).x\n\n");

  //Declaring dependencie rules
  fprintf( output, ".SUFFIXES: .cc .cpp .o .x\n\n");

  fprintf( output, "all: $(addprefix $(ARCHC)/include/, $(ACFILESHEAD)) $(ACFILES) $(EXE)\n\n");

  fprintf( output, "$(EXE): $(OBJS) %s\n",
           (strlen(SYSTEMC_PATH) > 2) ? "$(SYSTEMC)/lib-$(TARGET_ARCH)/libsystemc.a" : "");
  fprintf( output, "\t$(CC) $(CFLAGS) $(INC_DIR) $(LIB_DIR) -o $@ $(OBJS) $(LIBS) $(addprefix $(ARCHC)/lib/, $(ACLIBFILES)) 2>&1 | c++filt\n\n");

  COMMENT_MAKE("Copy from template if main.cpp not exist");
  fprintf( output, "main.cpp:\n");
  fprintf( output, "\tcp main.cpp.tmpl main.cpp\n\n");

  fprintf( output, ".cpp.o:\n");
  fprintf( output, "\t$(CC) $(CFLAGS) $(INC_DIR) -c $<\n\n");

  fprintf( output, ".cc.o:\n");
  fprintf( output, "\t$(CC) $(CFLAGS) $(INC_DIR) -c $<\n\n");

  fprintf( output, "clean:\n");
  fprintf( output, "\trm -f $(OBJS) *~ $(EXE) core *.o \n\n");

  fprintf( output, "model_clean:\n");
  fprintf( output, "\trm -f $(ACSRCS) $(ACHEAD) $(ACINCS) $(ACFILESHEAD) $(ACFILES) *.tmpl loader.ac \n\n");

  fprintf( output, "sim_clean: clean model_clean\n\n");

  fprintf( output, "distclean: sim_clean\n");
  fprintf( output, "\trm -f main.cpp Makefile.archc\n\n");

  fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_storage/%%.cpp\n");
  fprintf( output, "\tcp $< $@\n");

  fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_syscall/%%.cpp\n");
  fprintf( output, "\tcp $< $@\n");

  fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_core/%%.cpp\n");
  fprintf( output, "\tcp $< $@\n");

  fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_utils/%%.cpp\n");
  fprintf( output, "\tcp $< $@\n");

  if(ACGDBIntegrationFlag) {
    fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_gdb/%%.cpp\n");
    fprintf( output, "\tcp $< $@\n");
  }

  if(ACEncoderFlag) {
    fprintf( output, "%%.cpp: $(ARCHC)/src/aclib/ac_encoder/%%.cpp\n");
    fprintf( output, "\tcp $< $@\n");
  }
}

//!Create dummy function file
void CreateDummy(int id, char* content) {

  FILE *output;
  char filename[30];
  sprintf( filename, "ac_dummy%d.cpp", id);

  if ( !(output = fopen( filename, "w"))){
    perror("ArchC could not open output file");
    exit(1);
  }

  print_comment( output, "Dummy function used if real one is missing.");
  fprintf( output, "%s\n", content);
}


////////////////////////////////////////////////////////////////////////////////////
// Emit functions ...                                                             //
// These Functions are used by the Create functions declared above to write files //
////////////////////////////////////////////////////////////////////////////////////

/**************************************/
/*!Emit the generic instruction class
  Used by CreateTypesHeader function. */
/***************************************/
void EmitGenInstrClass(FILE *output) {
  extern ac_dec_format *format_ins_list;
  ac_dec_format *pformat;
  ac_dec_field *pfield , *pgenfield, *pf, *ppf;
  int initializing = 1;
	
  /* Emiting generic instruction class declaration */
  COMMENT(INDENT[0],"Generic Instruction Class declaration.\n"); 
  fprintf( output, "class ac_instruction: public ac_resources {\n");
  fprintf( output, "protected:\n");
  fprintf( output, "%schar* ac_instr_name;\n", INDENT[1]);
  fprintf( output, "%schar* ac_instr_mnemonic;\n", INDENT[1]);
  fprintf( output, "%sunsigned ac_instr_size;\n", INDENT[1]);
  fprintf( output, "%sunsigned ac_instr_cycles;\n", INDENT[1]);
  fprintf( output, "%sunsigned ac_instr_min_latency;\n", INDENT[1]);
  fprintf( output, "%sunsigned ac_instr_max_latency;\n", INDENT[1]);

  //Selecting fields that are common to all formats.
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next){

    if( initializing ){

      //This is the first format being processed. Put all of its fields.
      for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next){

        pf = (ac_dec_field*) malloc( sizeof(ac_dec_field));
        pf = memcpy( pf, pfield, sizeof(ac_dec_field) );

        if( pfield == pformat->fields ){
          pgenfield = pf;
          pgenfield->next = NULL;
        }
        else{
          pf->next = pgenfield;
          pgenfield = pf;
        }
      }
      initializing =0;
			
    }
    else{  //We already have candidate fields. Check if they are present in all formats.

      ppf = NULL;

      //Keep fields that are common to all instructions
      pf = pgenfield; 

      while( pf ){

        //Looking for pf into pformat
        for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next){ 

          if( !strcmp( pf->name, pfield->name) )
            break;
        }

        if( !pfield) { //Did not find. Delete pf from pgenfield
					
          if(ppf){
            ppf->next = pf->next;
            free(pf);
            pf = ppf->next;
          }
          else{  //Deleting the first field
            pgenfield = pf->next;
            free(pf);
            pf = pgenfield;
          }
        }
        else{  //Found. Keep the field and step to the next.
          ppf = pf;
          pf = pf->next;
        }
      }
    }	
  }

  //pgenfield has the list of fields for the generic instruction.
  for( pfield = pgenfield; pfield != NULL; pfield = pfield->next){
    if( pfield->sign )
      fprintf( output,"%sint %s;\n",INDENT[1],pfield->name );
    else
      fprintf( output,"%sunsigned %s;\n",INDENT[1],pfield->name );
  }

  //Now emiting public methods
  fprintf( output, "public:\n");

  fprintf( output, "%sac_instruction( char* name, char* mnemonic, unsigned min, unsigned max ){ ac_instr_name = name ; ac_instr_mnemonic = mnemonic; ac_instr_min_latency = min, ac_instr_max_latency =max;}\n", INDENT[1]);
  fprintf( output, "%sac_instruction( char* name, char* mnemonic ){ ac_instr_name = name ; ac_instr_mnemonic = mnemonic;}\n", INDENT[1]);
  fprintf( output, "%sac_instruction( char* name ){ ac_instr_name = name ;}\n", INDENT[1]);
  fprintf( output, "%sac_instruction( ){ ac_instr_name = \"NULL\";}\n", INDENT[1]);

  fprintf( output, "%svirtual void behavior(ac_stage_list  stage = (ac_stage_list)0, unsigned cycle=0);\n", INDENT[1]);   

  fprintf( output, "%svoid set_cycles( unsigned c){ ac_instr_cycles = c;}\n", INDENT[1]);
  fprintf( output, "%sunsigned get_cycles(){ return ac_instr_cycles;}\n", INDENT[1]);

  fprintf( output, "%svoid set_min_latency( unsigned c){ ac_instr_min_latency = c;}\n", INDENT[1]);
  fprintf( output, "%sunsigned get_min_latency(){ return ac_instr_min_latency;}\n", INDENT[1]);

  fprintf( output, "%svoid set_max_latency( unsigned c){ ac_instr_max_latency = c;}\n", INDENT[1]);
  fprintf( output, "%sunsigned get_max_latency(){ return ac_instr_max_latency;}\n", INDENT[1]);

  fprintf( output, "%sunsigned get_size() {return ac_instr_size;}\n", INDENT[1]);
  fprintf( output, "%svoid set_size( unsigned s) {ac_instr_size = s;}\n", INDENT[1]);

  fprintf( output, "%schar* get_name() {return ac_instr_name;}\n", INDENT[1]);
  fprintf( output, "%svoid set_name( char* name) {ac_instr_name = name;}\n", INDENT[1]);

  fprintf( output, "%svirtual void set_fields( ac_instr instr ){\n",INDENT[1]);
  
  for( pfield = pgenfield; pfield != NULL; pfield = pfield->next)
    fprintf( output,"%s%s = instr.get(%d); \n", INDENT[2],pfield->name, pfield->id );
	
  //fprintf( output,"%sac_instr_size = %d; \n", INDENT[2], pformat->size );
  fprintf( output, "%s}\n", INDENT[1]);



  fprintf( output, "%svirtual  void print (ostream & os) const{};\n", INDENT[1]);

  fprintf( output, "%sfriend ostream& operator<< (ostream &os,const ac_instruction &ins){\n", INDENT[1]);
  fprintf( output, "%sins.print(os);\n", INDENT[1]);
  fprintf( output, "%sreturn os;\n", INDENT[1]);
  fprintf( output, "%s};\n", INDENT[1]);

  fprintf( output, "};\n\n");
	
}

/**************************************/
/*!Emit one class for each format declared.
  Used by CreateTypesHeader function. */
/***************************************/
void EmitFormatClasses(FILE *output) {
  extern ac_dec_format *format_ins_list;
  ac_dec_format *pformat;
  ac_dec_field *pfield;

  /* Emiting format class declaration */
  COMMENT(INDENT[0],"Instruction Format class declarations.\n"); 

  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next){

    fprintf( output, "class ac_%s: public ac_instruction {\n", pformat->name); 
    fprintf( output, "protected:\n");

    for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next){
      if( pfield->sign )
        fprintf( output,"%sint %s;\n",INDENT[1],pfield->name );
      else
        fprintf( output,"%sunsigned %s;\n",INDENT[1],pfield->name );
    }

    fprintf( output, "public:\n");
    fprintf( output, "%sac_%s( char* name, char* mnemonic, unsigned min, unsigned max ):ac_instruction(name, mnemonic, min, max){};\n", INDENT[1], pformat->name);
    fprintf( output, "%sac_%s( char* name, char *mnemonic ):ac_instruction(name, mnemonic) {};\n", INDENT[1], pformat->name);
    fprintf( output, "%sac_%s( char* name ):ac_instruction(name) {};\n", INDENT[1], pformat->name);
    fprintf( output, "%sac_%s( ):ac_instruction() {};\n", INDENT[1], pformat->name);
    fprintf( output, "%svoid set_fields( ac_instr instr ){\n",INDENT[1]);
  
    for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next)
      fprintf( output,"%s%s = instr.get(%d); \n", INDENT[2],pfield->name, pfield->id );

    fprintf( output,"%sac_instr_size = %d; \n", INDENT[2], pformat->size );

    fprintf( output, "%s}\n", INDENT[1]);

    fprintf( output, "%svirtual void behavior( ac_stage_list stage=(ac_stage_list)0, unsigned cycle=0 );\n", INDENT[1]);


    //Print method
    fprintf( output, "%svirtual void print (ostream & os) const{\n", INDENT[1]);
    fprintf( output, "%sos ", INDENT[2]);

    for( pfield = pformat->fields; pfield != NULL; pfield = pfield->next)
      if(pfield->next)
        fprintf( output, " << \"%s: \" << %s << \", \" ",  pfield->name,  pfield->name);
      else
        fprintf( output, " << \"%s: \" << %s;", pfield->name,  pfield->name);

    fprintf( output, "\n}\n");

    fprintf( output, "};\n\n");
  }
}

/*!*************************************/
/*! Emit  instruction class declarations
  Used by CreateTypesHeader function. */
/***************************************/
void EmitInstrClasses( FILE *output){

  extern ac_dec_instr *instr_list;
  ac_dec_instr *pinstr;

  COMMENT(INDENT[0],"Instruction class declarations.\n"); 

  for( pinstr = instr_list; pinstr!= NULL; pinstr=pinstr->next){
    fprintf( output, "class ac_%s: public ac_%s {\n", pinstr->name, pinstr->format); 
    fprintf( output, "%spublic:\n", INDENT[0]);
    fprintf( output, "%sac_%s( char* name, char* mnemonic, unsigned min, unsigned max ):ac_%s(name, mnemonic, min, max){};\n", INDENT[1], pinstr->name, pinstr->format);
    fprintf( output, "%sac_%s( char* name, char *mnemonic ):ac_%s(name, mnemonic) {};\n", INDENT[1], pinstr->name, pinstr->format);
    fprintf( output, "%sac_%s( char* name ):ac_%s(name) {};\n", INDENT[1], pinstr->name, pinstr->format);
    fprintf( output, "%sac_%s( ):ac_%s() {};\n", INDENT[1], pinstr->name, pinstr->format);
    fprintf( output, "%svoid behavior( ac_stage_list  stage = (ac_stage_list)0, unsigned cycle=0 );\n", INDENT[1]);

    //Print method
    fprintf( output, "%svoid print (ostream & os) const{\n", INDENT[1]);
    fprintf( output, "%sos << ac_instr_mnemonic << \"\t\";", INDENT[2]);
    fprintf( output, "}\n");

    fprintf( output, "};\n\n");
  }
}

/**************************************/
/*! Emit declaration of decoding structures
  Used by CreateISAHeader function.   */
/***************************************/
void EmitDecStruct( FILE* output){
  extern ac_dec_format *format_ins_list;
  extern ac_dec_instr *instr_list;
  extern int declist_num;
  extern int HaveMultiCycleIns;
  ac_dec_field *pdecfield;
  ac_dec_format *pformat;
  ac_dec_instr *pinstr;
  ac_dec_list *pdeclist;
  int i;
  int count_fields;

  //fprintf( output, "%sunsigned counter;\n", INDENT[2]); 
  fprintf( output, "%sextern ac_dec_field *fields;\n", INDENT[2]); 
  fprintf( output, "%sextern ac_dec_format *formats;\n", INDENT[2]); 
  fprintf( output, "%sextern ac_dec_list *dec_list;\n", INDENT[2]); 
  fprintf( output, "%sextern ac_dec_instr *instructions;\n\n", INDENT[2]); 

  //Field Structure
  fprintf( output, "%sfields = (ac_dec_field*) malloc(sizeof(ac_dec_field)*AC_DEC_FIELD_NUMBER);\n", INDENT[2]);
  i = 0;
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next){

    for( pdecfield = pformat->fields; pdecfield!= NULL; pdecfield=pdecfield->next){
      fprintf( output, "%sfields[%d].name      = \"%s\";\n", INDENT[2], i, pdecfield->name);
      fprintf( output, "%sfields[%d].size      = %d;\n", INDENT[2], i, pdecfield->size);
      fprintf( output, "%sfields[%d].first_bit = %d;\n", INDENT[2], i, pdecfield->first_bit);
      fprintf( output, "%sfields[%d].id        = %d;\n", INDENT[2], i, pdecfield->id);
      fprintf( output, "%sfields[%d].val       = %ld;\n", INDENT[2], i, pdecfield->val);
      fprintf( output, "%sfields[%d].sign      = %d;\n", INDENT[2], i, pdecfield->sign);
      if(pdecfield->next)
        fprintf( output, "%sfields[%d].next      = &(fields[%d]);\n\n", INDENT[2], i, i+1);
      else
        fprintf( output, "%sfields[%d].next      = NULL;\n\n", INDENT[2], i);
      i++;
    }
  }

  fprintf( output, " \n");

  //Format Structure
  fprintf( output, "%sformats = (ac_dec_format*) malloc(sizeof(ac_dec_format)*AC_DEC_FORMAT_NUMBER);\n", INDENT[2]);
  i = 0;
  count_fields = 0;
  for( pformat = format_ins_list; pformat!= NULL; pformat=pformat->next){

    fprintf( output, "%sformats[%d].name      = \"%s\";\n", INDENT[2], i, pformat->name);
    fprintf( output, "%sformats[%d].fields    = &(fields[%d]);\n", INDENT[2], i, count_fields);
    if(pformat->next)
      fprintf( output, "%sformats[%d].next      = &(formats[%d]);\n\n", INDENT[2], i, i+1);
    else
      fprintf( output, "%sformats[%d].next      = NULL;\n\n", INDENT[2], i);
    i++;
    for( pdecfield = pformat->fields; pdecfield!= NULL; pdecfield=pdecfield->next)
      count_fields++;    
  }

  fprintf( output, " \n");

  //Decode list Structure
  fprintf( output, "%sdec_list = (ac_dec_list*) malloc(sizeof(ac_dec_list)*AC_DEC_LIST_NUMBER);\n", INDENT[2]);
  i = 0;
  for( pinstr = instr_list; pinstr!= NULL; pinstr=pinstr->next){

    for( pdeclist = pinstr->dec_list; pdeclist!= NULL; pdeclist=pdeclist->next){
      fprintf( output, "%sdec_list[%d].name      = \"%s\";\n", INDENT[2], i, pdeclist->name);
      fprintf( output, "%sdec_list[%d].value     = %d;\n", INDENT[2], i, pdeclist->value);
      if(pdeclist->next)
        fprintf( output, "%sdec_list[%d].next      = &(dec_list[%d]);\n\n", INDENT[2], i, i+1);
      else
        fprintf( output, "%sdec_list[%d].next      = NULL;\n\n", INDENT[2], i);
      i++;
    }
  }
  declist_num = i;

  fprintf( output, " \n");

  //Instruction Structure
  fprintf( output, "%sinstructions = (ac_dec_instr*) malloc(sizeof(ac_dec_instr)*AC_DEC_INSTR_NUMBER);\n", INDENT[2]);
  i = 0;
  count_fields = 0;
  for( pinstr = instr_list; pinstr!= NULL; pinstr=pinstr->next){
    fprintf( output, "%sinstructions[%d].name      = \"%s\";\n", INDENT[2], i, pinstr->name);
    fprintf( output, "%sinstructions[%d].mnemonic  = \"%s\";\n", INDENT[2], i, pinstr->mnemonic);
    fprintf( output, "%sinstructions[%d].asm_str   = \"%s\";\n", INDENT[2], i, pinstr->asm_str);
    fprintf( output, "%sinstructions[%d].format    = \"%s\";\n", INDENT[2], i, pinstr->format);
    fprintf( output, "%sinstructions[%d].id        = %d;\n", INDENT[2], i, pinstr->id);
    if(HaveMultiCycleIns)
      fprintf( output, "%sinstructions[%d].cycles    = %d;\n", INDENT[2], i, pinstr->cycles);

    fprintf( output, "%sinstructions[%d].dec_list  = &(dec_list[%d]);\n", INDENT[2], i, count_fields);
    if(pinstr->next)
      fprintf( output, "%sinstructions[%d].next      = &(instructions[%d]);\n\n", INDENT[2], i, i+1);
    else
      fprintf( output, "%sinstructions[%d].next      = NULL;\n\n", INDENT[2], i);

    for( pdeclist = pinstr->dec_list; pdeclist!= NULL; pdeclist=pdeclist->next)
      count_fields++;
    i++;
  }

}



/**************************************/
/*! Emits a method to update pipe regs
  Used by CreateArchImpl function     */
/***************************************/
void EmitPipeUpdateMethod( FILE *output){
  extern ac_stg_list *stage_list;
  extern char *project_name;
  ac_stg_list *pstage;
  extern ac_sto_list *storage_list;
  ac_sto_list *pstorage;

  //Emiting Update Method.
  COMMENT(INDENT[0],"Updating Pipe Regs for behavioral simulation.");               
  fprintf( output, "%svoid %s_arch::ac_update_regs(){\n", INDENT[0], project_name);        
  fprintf( output, "%sstatic ac_instr nop;\n\n", INDENT[1]);

  for( pstage = stage_list; pstage->next != NULL; pstage=pstage->next){   

    fprintf( output, "%sif( !%s_stall )\n", INDENT[1], pstage->name);        

    fprintf( output, "%sif( %s_flush ){\n", INDENT[2], pstage->name);        
    fprintf( output, "%s%s_regin.write( nop );\n", INDENT[3], pstage->next->name);        
    fprintf( output, "%s%s_flush = 0;\n", INDENT[3], pstage->name);        
    fprintf( output, "%s}\n", INDENT[2]);        

    fprintf( output, "%selse\n", INDENT[2]);        
    fprintf( output, "%s%s_regin.write( %s_regout.read() );\n", INDENT[3], pstage->next->name, pstage->name);        

    fprintf( output, "%selse\n", INDENT[1]);        
    fprintf( output, "%s%s_stall = 0;\n", INDENT[2], pstage->name);        
    fprintf( output, "\n");

  }

  if( ACDelayFlag ){
    for( pstorage = storage_list; pstorage!= NULL; pstorage = pstorage->next )
      //TODO: Support Delayed assignment for formatted regs
      if( pstorage->format == NULL )
        fprintf( output, "%s%s.commit_delays( sc_simulation_time() );\n", INDENT[1], pstorage->name);

    fprintf( output, "%sac_pc.commit_delays( sc_simulation_time() );\n", INDENT[1]);
  }

  fprintf( output, "%sbhv_pc.write( ac_pc );\n", INDENT[1]);

  fprintf( output, "%s}\n", INDENT[0]);
}

/**************************************/
/*! Emits a method to update pipe regs
  Used by CreateArchImpl function     */
/***************************************/
void EmitMultiPipeUpdateMethod( FILE *output){

  extern ac_pipe_list *pipe_list;
  extern char *project_name;
  ac_stg_list *pstage;
  ac_pipe_list *ppipe;
  extern ac_sto_list *storage_list;
  ac_sto_list *pstorage;

  //Emiting Update Method.
  COMMENT(INDENT[0],"Updating Pipe Regs for behavioral simulation.");               
  fprintf( output, "%svoid %s_arch::ac_update_regs(){\n", INDENT[0], project_name);        
  fprintf( output, "%sstatic ac_instr nop;\n\n", INDENT[1]);

  for ( ppipe = pipe_list; ppipe != NULL; ppipe=ppipe->next ){

    for( pstage = ppipe->stages; pstage->next != NULL; pstage=pstage->next){   

      fprintf( output, "%sif( !%s_%s_stall )\n", INDENT[1], ppipe->name, pstage->name);        

      fprintf( output, "%sif( %s_%s_flush ){\n", INDENT[2], ppipe->name, pstage->name);        
      fprintf( output, "%s%s_%s_regin.write( nop );\n", INDENT[3], ppipe->name, pstage->next->name);        
      fprintf( output, "%s%s_%s_flush = 0;\n", INDENT[3], ppipe->name, pstage->name);        
      fprintf( output, "%s}\n", INDENT[2]);        
			
      fprintf( output, "%selse\n", INDENT[2]);        
      fprintf( output, "%s%s_%s_regin.write( %s_%s_regout.read() );\n", INDENT[3], ppipe->name, pstage->next->name, ppipe->name, pstage->name);        
			
      fprintf( output, "%selse\n", INDENT[1]);        
      fprintf( output, "%s%s_%s_stall = 0;\n", INDENT[2], ppipe->name, pstage->name);        
      fprintf( output, "\n");
			
    }
  }

  if( ACDelayFlag ){
    for( pstorage = storage_list; pstorage!= NULL; pstorage = pstorage->next )
      //TODO: Support Delayed assignment for formatted regs
      if( pstorage->format == NULL )
        fprintf( output, "%s%s.commit_delays( sc_simulation_time() );\n", INDENT[1], pstorage->name);
		
    fprintf( output, "%sac_pc.commit_delays( sc_simulation_time() );\n", INDENT[1]);
  }
	
  fprintf( output, "%sbhv_pc.write( ac_pc );\n", INDENT[1]);
	
  fprintf( output, "%s}\n", INDENT[0]);
}

/**************************************/
/*!  Emits a method to update pipe regs
  \brief Used by CreateArchImpl function      */
/***************************************/
void EmitUpdateMethod( FILE *output){

  extern char *project_name;
  extern int HaveMultiCycleIns, HaveMemHier;
  extern ac_sto_list *storage_list;
  ac_sto_list *pstorage;

  //Emiting Update Method.
  COMMENT(INDENT[0],"Updating Regs for behavioral simulation.");               
  fprintf( output, "%svoid %s::ac_update_regs(){\n\n", INDENT[0], project_name);

  fprintf(output, "%sfor (;;) {\n\n", INDENT[1]);

  fprintf( output, "%sif(!ac_wait_sig){\n", INDENT[1]);
  if( ACDelayFlag ){
    for( pstorage = storage_list; pstorage!= NULL; pstorage = pstorage->next ) 
      fprintf( output, "%s%s.commit_delays( (double)ac_cycle_counter );\n", INDENT[2], pstorage->name);
    fprintf( output, "%sac_pc.commit_delays(  (double)ac_cycle_counter );\n", INDENT[2]);

    fprintf( output, "%sif(!ac_parallel_sig)\n", INDENT[2]);
    fprintf( output, "%sac_cycle_counter+=1;\n", INDENT[3]);
    fprintf( output, "%selse\n", INDENT[2]);
    fprintf( output, "%sac_parallel_sig = 0;\n\n", INDENT[3]);

  }
        
  fprintf( output, "%sbhv_pc.write( ac_pc );\n", INDENT[2]);
  if( HaveMultiCycleIns)
    fprintf( output, "%sbhv_cycle.write( ac_cycle );\n", INDENT[2]);
  fprintf( output, "%s}\n", INDENT[1]);
  /*   fprintf( output, "%selse{\n", INDENT[1]); */
  /*   fprintf( output, "%sdo_it = do_it.read()^1;\n", INDENT[2]); */
  /*   fprintf( output, "%s}\n", INDENT[1]); */

  if( HaveMemHier ){
    for( pstorage = storage_list; pstorage!= NULL; pstorage = pstorage->next ) {
      if( pstorage->type == CACHE || pstorage->type == ICACHE || pstorage->type == DCACHE || pstorage->type == MEM )
        fprintf( output, "%s%s.process_request( );\n", INDENT[1], pstorage->name);
    }
  }
  fprintf(output, "%sif (ac_stop_flag == 0)\n", INDENT[1]);
  fprintf( output, "%sdo_it = do_it ^1;\n\n", INDENT[2]);

  fprintf(output, "%swait();\n\n", INDENT[1]);
  fprintf(output, "%s}\n\n", INDENT[1]);

  fprintf( output, "%s}\n", INDENT[0]);
}

/**************************************/
/*!  Emits the if statement that handles instruction decodification
  \brief Used by EmitProcessorBhv, EmitMultCycleProcessorBhv and CreateStgImpl functions      */
/***************************************/
void EmitDecodification( FILE *output, int base_indent){

  extern int wordsize, fetchsize, HaveMemHier;
  extern char* project_name;

  base_indent++;
  if( HaveMemHier ){

    if (fetchsize == wordsize)
      fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read( decode_pc );\n\n", INDENT[base_indent]);
    else if (fetchsize == wordsize/2)
      fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read_half( decode_pc );\n\n", INDENT[base_indent]);
    else if (fetchsize == 8)
      fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read_byte( decode_pc );\n\n", INDENT[base_indent]);
    else {
      AC_ERROR("Fetchsize differs from wordsize or (wordsize/2) or 8: not implemented.");
      exit(EXIT_FAILURE);
    }
    
    fprintf( output, "%sif( ac_wait_sig ) {\n", INDENT[base_indent]);
    fprintf( output, "%sreturn;\n", INDENT[base_indent+1]);
    fprintf( output, "%s}\n", INDENT[base_indent]);
  }

  if( ACDecCacheFlag ){
    fprintf( output, "%sins_cache = (DEC_CACHE+decode_pc);\n", INDENT[base_indent]);
    fprintf( output, "%sif ( !ins_cache->valid ){\n", INDENT[base_indent]);
  }

  if( !HaveMemHier ){

    /*     if (fetchsize == wordsize) */
    /*       fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read( decode_pc );\n\n", INDENT[base_indent+1]); */
    /*     else if (fetchsize == wordsize/2) */
    /*       fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read_half( decode_pc );\n\n", INDENT[base_indent+1]); */
    /*     else if (fetchsize == 8) */
    /*       fprintf( output, "%s*((ac_fetch*)(fetch)) = IM->read_byte( decode_pc );\n\n", INDENT[base_indent+1]); */
    /*     else { */
    /*       AC_ERROR("Fetchsize differs from wordsize or (wordsize/2) or 8: not implemented."); */
    /*       exit(EXIT_FAILURE); */
    /*     } */
  }
  
    /*   fprintf( output, "%squant = AC_FETCHSIZE/8;\n", INDENT[base_indent+1]); */
  fprintf( output, "%squant = 0;\n", INDENT[base_indent+1]);
  
    //The Decoder uses a big endian bit stream. So if the host is little endian, convert it!
    /*   if( ac_host_endian == 0 ){ */
    /*     fprintf( output, "%sfor (i=0; i< AC_FETCHSIZE/8; i++) {\n", INDENT[base_indent+1]); */
    /*     fprintf( output, "%sbuffer[quant - 1 - i] = fetch[i];\n", INDENT[base_indent+2]); */
    /*     fprintf( output, "%s}\n", INDENT[base_indent+1]); */
    /*   } */

  if( ACDecCacheFlag ){
    fprintf( output, "%sins_cache->instr_p = new ac_instr<%s_parms::AC_DEC_FIELD_NUMBER>((ISA.decoder)->Decode(reinterpret_cast<unsigned char*>(buffer), quant));\n", INDENT[base_indent+1], project_name);
    fprintf( output, "%sins_cache->valid = 1;\n", INDENT[base_indent+1]);
    fprintf( output, "%s}\n", INDENT[base_indent]);
    fprintf( output, "%sinstr_vec = ins_cache->instr_p;\n", INDENT[base_indent]);
  }
  else{
    fprintf( output, "%sinstr_dec = (ISA.decoder)->Decode(reinterpret_cast<unsigned char*>(buffer), quant);\n", INDENT[base_indent]);
    fprintf( output, "%sinstr_vec = new ac_instr<%s_parms::AC_DEC_FIELD>( instr_dec);\n", INDENT[base_indent], project_name);
  }
  
  //Checking if it is a valid instruction
  fprintf( output, "%sins_id = instr_vec->get(IDENT);\n\n", INDENT[base_indent]);
  fprintf( output, "%sif( ins_id == 0 ) {\n", INDENT[base_indent]);
  fprintf( output, "%scerr << \"ArchC Error: Unidentified instruction. \" << endl;\n", INDENT[base_indent+1]);
  fprintf( output, "%scerr << \"PC = \" << hex << decode_pc << dec << endl;\n", INDENT[base_indent+1]);
  fprintf( output, "%sstop();\n", INDENT[base_indent+1]);
  fprintf( output, "%sreturn;\n", INDENT[base_indent+1]);
  fprintf( output, "%s}\n", INDENT[base_indent]);
  
  fprintf( output, "\n");
  
}

/**************************************/
/*!  Emit code for executing instructions
  \brief Used by EmitProcessorBhv, EmitMultCycleProcessorBhv and CreateStgImpl functions      */
/***************************************/
void EmitInstrExec( FILE *output, int base_indent){
  extern ac_stg_list *stage_list;
  extern ac_pipe_list *pipe_list;
  extern int HaveCycleRange;

  extern char* project_name;

  if( ACGDBIntegrationFlag )
    fprintf( output, "%sif (gdbstub && gdbstub->stop(decode_pc)) gdbstub->process_bp();\n\n", INDENT[base_indent]);
  
  fprintf(output, "%sISA.set_fields(*instr_vec);\n", INDENT[base_indent]);
  fprintf( output, "%sac_pc = decode_pc;\n", INDENT[base_indent]);

  //Pipelined archs can annul an instruction through pipelining flushing.
  if(stage_list || pipe_list ){
    fprintf( output, "%sISA._behavior_instruction( (ac_stage_list) id );\n", INDENT[base_indent]);
    fprintf( output, "%s(ISA.*(%s_isa::instr_table[ins_id].ac_instr_type_behavior))((ac_stage_list) id);\n", INDENT[base_indent], project_name);
    fprintf( output, "%s(ISA.*(%s_isa::instr_table[ins_id].ac_instr_behavior))((ac_stage_list) id);\n", INDENT[base_indent], project_name);
  }
  else{
    fprintf( output, "%sISA._behavior_instruction();\n", INDENT[base_indent]);
    fprintf( output, "%sif(!ac_annul_sig) (ISA.*(%s_isa::instr_table[ins_id].ac_instr_type_behavior))();\n", INDENT[base_indent], project_name);
    fprintf( output, "%sif(!ac_annul_sig) (ISA.*(%s_isa::instr_table[ins_id].ac_instr_behavior))();\n", INDENT[base_indent], project_name);
  }

  if( ACDasmFlag ){
    fprintf( output, PRINT_DASM , INDENT[base_indent]);
  }

  if( ACStatsFlag ){
    fprintf( output, "%sif((!ac_annul_sig) && (!ac_wait_sig)) {\n", INDENT[base_indent]);
    fprintf( output, "%sac_sim_stats.instr_executed++;\n", INDENT[base_indent+1]);
    fprintf( output, "%sac_sim_stats.instr_table[ins_id].count++;\n", INDENT[base_indent+1]);

    //If cycle range for instructions were declared, include them on the statistics.
    if( HaveCycleRange ){
      fprintf( output, "%sac_sim_stats.ac_min_cycle_count += instr->get_min_latency();\n", INDENT[base_indent+1]);
      fprintf( output, "%sac_sim_stats.ac_max_cycle_count += instr->get_max_latency();\n", INDENT[base_indent+1]);
    }

    fprintf( output, "%s}\n", INDENT[base_indent]);
  }

  if( ACDebugFlag ){
    fprintf( output, "%sif( ac_do_trace != 0 ) \n", INDENT[base_indent]);
    fprintf( output, PRINT_TRACE, INDENT[base_indent+1]);
  }

  if( stage_list || pipe_list )
    fprintf( output, "%sregout.write( *instr_vec);\n", INDENT[base_indent]);

  if(!ACDecCacheFlag){
    fprintf( output, "%sdelete instr_vec;\n", INDENT[base_indent]);
    //    fprintf( output, "%sfree(instr_dec);\n", INDENT[base_indent]);
  }
  //  fprintf( output, "%s}\n", INDENT[base_indent-1]);

}


/**************************************/
/*!  Emits the if statement executed before
  fetches are performed.
  \brief Used by EmitProcessorBhv functions and CreateStgImpl      */
/***************************************/
void EmitFetchInit( FILE *output, int base_indent){
  extern int HaveMultiCycleIns;


  if (!ACDecCacheFlag){
    fprintf( output, "%sif( bhv_pc.read() >= APP_MEM->get_size()){\n", INDENT[base_indent]);
  }
  else
    fprintf( output, "%sif( bhv_pc.read() >= dec_cache_size){\n", INDENT[base_indent]);

  fprintf( output, "%scerr << \"ArchC: Address out of bounds (pc=0x\" << hex << bhv_pc.read() << \").\" << endl;\n", INDENT[base_indent+1]);
	//  fprintf( output, "%scout = cerr;\n", INDENT[base_indent+1]);

  if( ACVerifyFlag ){
    fprintf( output, "%send_log.mtype = 1;\n", INDENT[base_indent+1]);
    fprintf( output, "%send_log.log.time = -1;\n", INDENT[base_indent+1]);
    fprintf( output, "%sif(msgsnd(msqid, (struct log_msgbuf *)&end_log, sizeof(end_log), 0) == -1)\n", INDENT[base_indent+1]);
    fprintf( output, "%sperror(\"msgsnd\");\n", INDENT[base_indent+2]);
  }
/*   fprintf( output, "%sac_stop();\n", INDENT[base_indent+1]); */
  fprintf( output, "%sstop();\n", INDENT[base_indent+1]);
  fprintf( output, "%sreturn;\n", INDENT[base_indent+1]);
  fprintf( output, "%s}\n", INDENT[base_indent]);

  fprintf( output, "%selse {\n", INDENT[base_indent]);

  fprintf( output, "%sif( start_up ){\n", INDENT[base_indent+1]);
  fprintf( output, "%sdecode_pc = ac_start_addr;\n", INDENT[base_indent+2]);
  if(ACABIFlag)
    fprintf( output, "%ssyscall.set_prog_args(argc, argv);\n", INDENT[3]);
  fprintf( output, "%sstart_up=0;\n", INDENT[base_indent+2]);
  if( ACDecCacheFlag )
    fprintf( output, "%sinit_dec_cache();\n", INDENT[base_indent+2]);
  fprintf( output, "%s}\n", INDENT[base_indent+1]);

  fprintf( output, "%selse{ \n", INDENT[base_indent+1]);
  if(HaveMultiCycleIns && !ACDecCacheFlag ){
    //fprintf( output, "%sfree(instr_dec);\n", INDENT[base_indent+2]);
    fprintf( output, "%sdelete(instr_vec);\n", INDENT[base_indent+2]);
  }
  fprintf( output, "%sdecode_pc = bhv_pc.read();\n", INDENT[base_indent+2]);
  fprintf( output, "%s}\n \n", INDENT[base_indent+1]);
  
}

/**************************************/
/*!  Emits the body of a processor implementation for
  a processor without pipeline and with single cycle instruction.
  \brief Used by CreateProcessorImpl function      */
/***************************************/
void EmitProcessorBhv( FILE *output){

  fprintf(output, "%sfor (;;) {\n\n", INDENT[1]);

  EmitFetchInit(output, 1);
  EmitDecodification(output, 2);
  EmitInstrExec(output, 2);

  fprintf( output, "%sif ((!ac_wait_sig) && (!ac_annul_sig)) ac_instr_counter+=1;\n", INDENT[2]);
  fprintf( output, "%sac_annul_sig = 0;\n", INDENT[2]);
  fprintf( output, "%sbhv_done.write(1);\n", INDENT[2]);
  fprintf( output, "%s}\n", INDENT[1]);

  fprintf(output, "%swait();\n\n", INDENT[1]);

  fprintf( output, "%s}\n\n", INDENT[1]);

  fprintf( output, "}\n\n");

}

/**************************************/
/*!  Emits the body of a processor implementation for
  a processor without pipeline, with single cycle instruction and
  with an ABI provided.
  \brief Used by CreateProcessorImpl function      */
/***************************************/
void EmitProcessorBhv_ABI( FILE *output){

  fprintf(output, "%sfor (;;) {\n\n", INDENT[1]);

  EmitFetchInit(output, 1);

  //Emiting system calls handler.
  COMMENT(INDENT[2],"Handling System calls.")
    fprintf( output, "%sswitch( decode_pc ){\n\n", INDENT[2]);

  EmitABIDefine(output);
  fprintf( output, "\n\n");
  EmitABIAddrList(output,2);

  fprintf( output, "%sdefault:\n\n", INDENT[2]);

  EmitDecodification(output, 2);
  EmitInstrExec(output, 3);

  //Closing default case.
  fprintf( output, "%sbreak;\n", INDENT[3]);

  //Closing switch.
  fprintf( output, "%s}\n", INDENT[2]);

  fprintf( output, "%sif ((!ac_wait_sig) && (!ac_annul_sig)) ac_instr_counter+=1;\n", INDENT[2]);
  fprintf( output, "%sac_annul_sig = 0;\n", INDENT[2]);
  fprintf( output, "%sdone.write(1);\n", INDENT[2]);

  //Closing else.
  fprintf( output, "%s}\n", INDENT[1]);

  fprintf(output, "%swait();\n\n", INDENT[1]);

  fprintf( output, "%s}\n\n", INDENT[1]);

  fprintf( output, "}\n\n");

}


/**************************************/
/*!  Emits the body of a processor implementation for
  a multicycle processor
  \brief Used by CreateProcessorImpl function      */
/***************************************/
void EmitMultiCycleProcessorBhv( FILE *output){

 
  fprintf( output, "%sif( ac_cycle == 1 ){\n\n", INDENT[1]);

  EmitFetchInit(output, 2);
  EmitDecodification(output, 3);

  //Multicycle execution demands a different control. Do not use EmitInstrExec.
  fprintf( output, "%sac_pc = decode_pc;\n", INDENT[3]);
  fprintf( output, "%sac_cycle = 1;\n", INDENT[3]);
  fprintf( output, "%sinstr = (ac_instruction *)ISA.instr_table[ins_id][1];\n", INDENT[3]);
  fprintf( output, "%sformat = (ac_instruction *)ISA.instr_table[ins_id][2];\n", INDENT[3]);
  fprintf( output, "%sformat->set_fields( *instr_vec  );\n", INDENT[3]);
  fprintf( output, "%sinstr->set_fields( *instr_vec  );\n", INDENT[3]);

  if( ACDebugFlag ){
    fprintf( output, "%sif( ac_do_trace != 0 ) \n", INDENT[3]);
    fprintf( output, PRINT_TRACE, INDENT[4]);
  }

  if( ACDasmFlag ){
    fprintf( output, PRINT_DASM , INDENT[4]);
  }

  if( ACStatsFlag ){
    fprintf( output, "%sac_sim_stats.instr_executed++;;\n", INDENT[3]);
    fprintf( output, "%sac_sim_stats.instr_table[ins_id].count++;\n", INDENT[3]);
    fprintf( output, "%sac_sim_stats.ac_min_cycle_count += instr->get_min_latency();\n", INDENT[3]);
    fprintf( output, "%sac_sim_stats.ac_max_cycle_count += instr->get_max_latency();\n", INDENT[3]);
  }

  fprintf( output, "%sISA.instruction.set_cycles( instr->get_cycles());\n", INDENT[3]);
  fprintf( output, "%sISA.instruction.set_size( instr->get_size());\n", INDENT[3]);

  fprintf( output, "%s}\n", INDENT[2]);
  fprintf( output, "%sac_instr_counter+= 1;\n", INDENT[2]);

  fprintf( output, "%s}\n", INDENT[1]);

  fprintf( output, "%sISA.instruction.behavior( (ac_stage_list)0, ac_cycle );\n", INDENT[1]);
  fprintf( output, "%sformat->behavior((ac_stage_list)0, ac_cycle );\n", INDENT[1]);
  fprintf( output, "%sinstr->behavior((ac_stage_list)0, ac_cycle );\n", INDENT[1]);

  fprintf( output, "%sif( ac_cycle > instr->get_cycles())\n", INDENT[1]);
  fprintf( output, "%sac_cycle=1;\n\n", INDENT[2]);

  fprintf( output, "%sbhv_done.write(1);\n", INDENT[1]);
  fprintf( output, "}\n\n");
}

/**************************************/
/*!  Emits the define that implements the ABI control
  for pipelined architectures
  \brief Used by CreateStgImpl function      */
/***************************************/
void EmitPipeABIDefine( FILE *output){

  fprintf( output, "%s#define AC_SYSC(NAME,LOCATION) \\\n", INDENT[0]);
  fprintf( output, "%scase LOCATION: \\\n", INDENT[2]);

  fprintf( output, "%sif (flushes_left) { \\\n", INDENT[3]);
  fprintf( output, "%sfflush(0);\\\n", INDENT[4]);
  fprintf( output, "%sflushes_left--;\\\n", INDENT[4]);
  fprintf( output, "%s} \\\n", INDENT[3]);

  fprintf( output, "%selse { \\\n", INDENT[3]);
  fprintf( output, "%sfflush(0);\\\n", INDENT[4]);

  if( ACDebugFlag ){
    fprintf( output, "%sif( ac_do_trace != 0 )\\\n", INDENT[4]);
    fprintf( output, "%strace_file << hex << decode_pc << dec << endl; \\\n", INDENT[5]);
  }

  fprintf( output, "%ssyscall.NAME(); \\\n", INDENT[4]);
  fprintf( output, "%sac_instr_counter++; \\\n", INDENT[4]);
  fprintf( output, "%sflushes_left = 7; \\\n", INDENT[4]);
  fprintf( output, "%s} \\\n", INDENT[3]);

  fprintf( output, "%sregout.write( *the_nop); \\\n", INDENT[3]);
  fprintf( output, "%sif (! (ac_pc.read() %% 2)) ac_pc.write(ac_pc.read() + 1); \\\n", INDENT[3]);
  fprintf( output, "%selse ac_pc.write(ac_pc.read() - 1); \\\n", INDENT[3]);
  fprintf( output, "%sbreak;  \\\n", INDENT[3]);

}

/**************************************/
/*!  Emits the define that implements the ABI control
  for non-pipelined architectures
  \brief Used by CreateStgImpl function      */
/***************************************/
void EmitABIDefine( FILE *output){

  fprintf( output, "%s#define AC_SYSC(NAME,LOCATION) \\\n", INDENT[0]);
  fprintf( output, "%scase LOCATION: \\\n", INDENT[2]);

  if( ACStatsFlag ){
    fprintf( output, "%sac_sim_stats.syscall_executed++; \\\n", INDENT[4]);
  }

  if( ACDebugFlag ){
    fprintf( output, "%sif( ac_do_trace != 0 )\\\n", INDENT[4]);
    fprintf( output, "%strace_file << hex << decode_pc << dec << endl; \\\n", INDENT[5]);
  }

  fprintf( output, "%ssyscall.NAME(); \\\n", INDENT[4]);
  fprintf( output, "%sbreak;  \\\n", INDENT[3]);
}


/**************************************/
/*!  Emits the ABI special address list
  to be used inside the ABI switchs
  \brief Used by CreateStgImpl function      */
/***************************************/
void EmitABIAddrList( FILE *output, int base_indent){

  fprintf( output, "#include <ac_syscall.def>\n", INDENT[base_indent]);
  fprintf( output, "\n");
  fprintf( output, "#undef AC_SYSC\n\n");
}

/**************************************/
/*!  Emits a ac_cache instantiation.
  \brief Used by CreateResourcesImpl function      */
/***************************************/
void EmitCacheDeclaration( FILE *output, ac_sto_list* pstorage, int base_indent){

  //Parameter 1 will be the pstorage->name string
  //Parameters passed to the ac_cache constructor. They are not exactly in the same
  //order used for ArchC declarations.
  //TODO: Include write policy
  char parm2[128];  //Block size
  char parm3[128];  //# if blocks
  char parm4[128];  //set  size
  char parm5[128];  //replacement strategy

  //Integer indicating the write-policy
  int wp=0;   /* 0x11 (Write Through, Write Allocate)  */
  /* 0x12 (Write Back, Write Allocate)  */
  /* 0x21 (Write Through, Write Around)  */
  /* 0x22 (Write Back, Write Around)  */

  char *aux;
  int is_dm=0, is_fully=0;
  ac_cache_parms* pparms;
  int i=1;

  for( pparms = pstorage->parms; pparms != NULL; pparms = pparms->next ){

    switch( i ){

    case 1: /* First parameter must be a valid associativity */

      if( !pparms->str ){
        AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
        printf("The first parameter must be a valid associativity: \"dm\", \"2w\", \"4w\", ... \n");
        exit(1);
      }

#ifdef DEBUG_STORAGE
      printf("CacheDeclaration: Processing parameter: %d, which is: %s\n", i, pparms->str);
#endif

      if( !strcmp(pparms->str, "dm") || !strcmp(pparms->str, "DM") ){ //It is a direct-mapped cache
        is_dm = 1;
        sprintf( parm4, "1");  //Set size will be the 4th parameter for ac_cache constructor.
        sprintf( parm5, "DEFAULT");  //DM caches do not need a replacement strategy, use a default value in this parameter.
      }
      else if( !strcmp(pparms->str, "fully") || !strcmp(pparms->str, "FULLY") ){ //It is a fully associative cache
        is_fully =1;
      }
      else{  //It is a n-way cache
        aux = strchr( pparms->str,'w'); 
        if(  !aux ){   // Checking if the string has a 'w'
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("The first parameter must be a valid associativity: \"dm\", \"2w\", \"4w\", ..., \"fully\" \n");
          exit(1);
        }
        aux = (char*) malloc( strlen(pparms->str) );
        strncpy(aux, pparms->str,  strlen(pparms->str)-1);
        aux[ strlen(pparms->str)-1]='\0';

        sprintf(parm4, "%s", aux);  //Set size will be the 4th parameter for ac_cache constructor.
        free(aux);
      }
      break;

    case 2: /* Second parameter is the number of blocks (lines) */

      if( !(pparms->value > 0 ) ){
        AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
        printf("The second parameter must be a valid (>0) number of blocks (lines).\n");
        exit(1);
      }

      if( is_fully )
        sprintf( parm4, "%d", pparms->value);  //Set size will be the number of blocks (lines) for fully associative caches
                                
      sprintf(parm3, "%d", pparms->value);
      break;

    case 3: /* Third parameter is the block (line) size */

      if( !(pparms->value > 0 ) ){
        AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
        printf("The third parameter must be a valid (>0) block (line) size.\n");
        exit(1);
      }

      sprintf(parm2, "%d", pparms->value);
      break;

    case 4: /* The fourth  parameter may be the write policy or the replacement strategy.
               If it is a direct-mapped cache, then we don't have a replacement strategy,
               so this parameter must be the write policy, which is "wt" (write-through) or
               "wb" (write-back). Otherwise, it must be a replacement strategy, which is "lru"
               or "random", and the fifth parameter will be the write policy. */

      if( is_dm ){ //This value is set when the first parameter is being processed.
        /* So this is a write-policy */
        if( !strcmp( pparms->str, "wt") || !strcmp( pparms->str, "WT") ){
          //One will tell that a wt cache was declared
          wp = WRITE_THROUGH;
        }
        else if( !strcmp( pparms->str, "wb") || !strcmp( pparms->str, "WB") ) {
          //Zero will tell that a wb cache was declared
          wp = WRITE_BACK;
        }
        else{
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("For direct-mapped caches, the fourth parameter must be a valid write policy: \"wt\" or \"wb\".\n");
          exit(1);
        }
      }
      else{
        /* So, this is a replacement strategy */
        if( !strcmp( pparms->str, "lru") || !strcmp( pparms->str, "LRU") ){
          sprintf( parm5, "LRU");  //Including parameter
        }
        else if( !strcmp( pparms->str, "random") || !strcmp( pparms->str, "RANDOM") ) {
          sprintf( parm5, "RANDOM");  //Including parameter
        }
        else{
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("For non-direct-mapped caches, the fourth parameter must be a valid replacement strategy: \"lru\" or \"random\".\n");
          exit(1);
        }
      }
      break;

    case 5: /* The fifth parameter is a write policy. */

      if( !is_dm ){ //This value is set when the first parameter is being processed.
                                
        if( !strcmp( pparms->str, "wt") || !strcmp( pparms->str, "WT") ){
                                        
          wp = WRITE_THROUGH;
        }
        else if( !strcmp( pparms->str, "wb") || !strcmp( pparms->str, "WB") ) {
                                        
          wp = WRITE_BACK;
        }
        else{
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("For non-direct-mapped caches, the fourth parameter must be a valid replacement strategy: \"lru\" or \"random\".\n");
          exit(1);
        }
      }
                        
      else{ //This value is "war" for write-around or "wal" for "write-allocate"
                                
        if( !strcmp( pparms->str, "war") || !strcmp( pparms->str, "WAR") ){
          wp = wp | WRITE_AROUND;
        }
        else if( !strcmp( pparms->str, "wal") || !strcmp( pparms->str, "WAL") ) {
          wp = wp | WRITE_ALLOCATE;
        }
        else{
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("For non-direct-mapped caches, the fourth parameter must be a valid replacement strategy: \"lru\" or \"random\".\n");
          exit(1);
        }
                                
      }
      break;
                        
    case 6: /* The sixth parameter, if it is present, is a write policy. 
               It must not be present for direct-mapped caches.*/

      if( !is_dm ){ //This value is set when the first parameter is being processed.

        if( !strcmp( pparms->str, "war") || !strcmp( pparms->str, "WAR") ){
          wp = wp | WRITE_AROUND;
        }
        else if( !strcmp( pparms->str, "wal") || !strcmp( pparms->str, "WAL") ) {
          wp = wp | WRITE_ALLOCATE;
        }
        else{
          AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
          printf("For non-direct-mapped caches, the fifth parameter must be  \"war\" or \"wal\".\n");
          exit(1);
        }

      }
      else{
        AC_ERROR("Invalid parameter in cache declaration: %s\n", pstorage->name);
        printf("For direct-mapped caches there must be only five parameters (do not need a replacement strategy).\n");
        exit(1);
      }
      break;
    default:
      break;
    }                                                                   
    i++;
                
  }

  //Printing cache declaration.
  fprintf( output, "%sac_cache ac_resources::%s(\"%s\", %s, %s, %s, %s, 0x%x);\n",
           INDENT[base_indent], pstorage->name, pstorage->name, parm2, parm3, parm4, parm5, wp);
}

////////////////////////////////////
// Utility Functions              //
////////////////////////////////////

//!Read the archc.conf configuration file
void ReadConfFile(){

  char *conf_filename;
  extern char *ARCHC_PATH;
  extern char *SYSTEMC_PATH;
  extern char *CC_PATH;
  extern char *OPT_FLAGS;
  extern char *DEBUG_FLAGS;
  extern char *OTHER_FLAGS;

  FILE *conf_file;
  char line[CONF_MAX_LINE];
  char var[CONF_MAX_LINE];
  char value[CONF_MAX_LINE];

  ARCHC_PATH = getenv("ARCHC_PATH");

  if(!ARCHC_PATH){
    AC_ERROR("You should set the ARCHC_PATH environment variable.\n");
    exit(1);
  }
                
  conf_filename = (char*) malloc( strlen(ARCHC_PATH)+20);
  conf_filename = strcpy(conf_filename, ARCHC_PATH);
  conf_filename = strcat(conf_filename, "/config/archc.conf");

  conf_file = fopen(conf_filename, "r");

  if( !conf_file ){
    //ERROR
    AC_ERROR("Could not open archc.conf configuration file.\n");
    exit(1);
  }
  else{  
    while( fgets( line, CONF_MAX_LINE, conf_file) ){

      var[0]='\0';
      value[0]='\0';

      if(line[0] == '#' || line[0] == '\n'){  //Comments or blank lines
        continue;
      }
      else{

        sscanf(line, "%s",var);
        strcpy( value, strchr(line, '=')+1);

        if( !strcmp(var, "ARCHC_PATH") ){
          ARCHC_PATH =  (char*) malloc(strlen(value)+1);
          ARCHC_PATH = strcpy(ARCHC_PATH, value);
        }
        else if( !strcmp(var, "SYSTEMC_PATH") ){
          SYSTEMC_PATH = (char*) malloc(strlen(value)+1);
          SYSTEMC_PATH = strcpy(SYSTEMC_PATH, value);
          if (strlen(value) <= 2) {
            AC_ERROR("Please configure a SystemC path running install.sh script in ArchC directory.\n");
            exit(1);
          }
        }
        else if( !strcmp(var, "CC") ){
          CC_PATH =  (char*) malloc(strlen(value)+1);
          CC_PATH = strcpy(CC_PATH, value);
        }
        else if( !strcmp(var, "OPT") ){
          OPT_FLAGS =(char*) malloc(strlen(value)+1);
          OPT_FLAGS = strcpy(OPT_FLAGS, value); 
        }
        else if( !strcmp(var, "DEBUG") ){
          DEBUG_FLAGS = (char*) malloc(strlen(value)+1);
          DEBUG_FLAGS = strcpy(DEBUG_FLAGS, value);
        }
        else if( !strcmp(var, "OTHER") ){
          OTHER_FLAGS = (char*) malloc(strlen(value)+1);
          OTHER_FLAGS = strcpy(OTHER_FLAGS, value);
        }
        else if( !strcmp(var, "TARGET_ARCH") ){
          TARGET_ARCH = (char*) malloc(strlen(value)+1);
          TARGET_ARCH = strcpy(TARGET_ARCH, value);
        }

      }
    }
  }
  free(conf_filename);
}