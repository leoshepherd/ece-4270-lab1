#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{

	//**Binary Decoder//
	//PROGRAM_SIZE is the number of things in file
	

	/*	printf("Program size is: %d\n", PROGRAM_SIZE);
	int sizeholder = 0;
	int sizeholder = PROGRAM_SIZE;


	int * jagged[PROGRAM_SIZE];
	*/
	int wholehex = 0;
	int hex1 = 0;
	int hex2 = 0;
	int hex3 = 0;
	int hex4 = 0;
	int hex5 = 0;
	int hex6 = 0;
	int hex7 = 0;
	int hex8 = 0;

	wholehex = mem_read_32(0x00400000);	//use this to grab the value in memory

	hex1 = wholehex >>28;
	hex2 = (wholehex>>24) & (0x0000000F);
	hex3 = (wholehex>>20) & (0x0000000F); 
	hex4 = (wholehex>>16) & (0x0000000F); 
	hex5 = (wholehex>>12) & (0x0000000F); 
	hex6 = (wholehex>>8) & (0x0000000F); 
	hex7 = (wholehex>>4) & (0x0000000F); 
	hex8 = (wholehex>>0) & (0x0000000F); 

	printf("%x\n", wholehex);

	printf("%x\n", hex1);
	printf("%x\n", hex2);


	//Identify Function//
		
	if((hex1 == 0x0) && (hex2 < 0x4))		//yellow
	{

		if(hex2 > 0x3 && hex2 <0x8)	//4-7
		{
			if(hex4 ==0)
			{
				//BLTZ
			}	
			if(hex4 ==1)
			{
				//BGEZ
			}
			else
			{
				printf("not a given function\n");
			}
				
		}
		else if( hex2> 0x7 )			//8-F
		{
			if(hex2 < 0xC)
			{
					//J Instruction
			}
			else
			{
					//JAL Instruction
			}
			
		}
		else				//0-3
		{

			if(hex8 == 0x0)
			{
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//SLL
				}
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MFHI
				}
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//MFHI
				}
				else
				{
					printf("not a valid function");
				}
			}

			if(hex8 == 0x1)
			{
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MTHI
				}
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//ADDU
				}
				else
				{
					printf("not a valid function");
				}
			}
			if(hex8 == 0x2)
			{
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//SRL
				}
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MFLO
				}
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//SUB
				}
				else
				{
					printf("not a valid function");
				}
			}

			if(hex8 == 0x3)
			{
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//SRA
				}
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MTLO
				}
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//SUBU
				}
				else
				{
					printf("not a valid function");
				}
			}
			if(hex8 == 0x4)
			{
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//AND
				}
				else
				{
					printf("invalid function");
				}

			}
			if(hex8 == 0x5)
			{
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//OR
				}
				else
				{
					printf("invalid function");
				}

			}
			if(hex8 == 0x6)
			{
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//XOR
				}
				else
				{
					printf("invalid function");
				}

			}
			if(hex8 == 0x7)
			{
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//NOR
				}
				else
				{
					printf("invalid function");
				}

			}

			if(hex8 == 0x8)
			{
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MULT
				}
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//JR
				}
				else
				{
					printf("not a valid function");
				}
			}
			if(hex8 == 0x9)
			{
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//MULTU
				}
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//JALR
				}
				else
				{
					printf("not a valid function");
				}
			}
			if(hex8 == 0xA)
			{
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//DIV
				}
				if((hex7 == 0x2) || (hex7 == 0x6) || (hex7 == 0xA) || (hex7 == 0xE))
				{
					//SLT
				}
				else
				{
					printf("not a valid function");
				}
			}
			if(hex8 == 0xB)
			{
				if((hex7 == 0x1) || (hex7 == 0x5) || (hex7 == 0x9) || (hex7 == 0xD))
				{
					//DIVU
				}
				else
				{
					printf("invalid function");
				}
			}
			if(hex8 == 0xC)
			{
				if((hex7 == 0x0) || (hex7 == 0x4) || (hex7 == 0x8) || (hex7 == 0xC))
				{
					//SYSCALL
				}
				else
				{
					printf("invalid function");
				}
			}


		}


	}
	else if(hex1 == 0x1)	//orange
	{
		if(hex2 <0x4)
		{
				//BEQ
		}
		if(hex2>0x3 && hex2 <0x8)
		{
				//BNE
		}
		if(hex2>0x7 && hex2<0xC)
		{
				//BLEZ
		}
		else
		{
				//BGTZ
		}


	}
	else if((hex1 == 0x2) && ((hex2 < 0x4) || (hex2 > 0x7))) //red
	{
			printf("found\n");

			if(hex2 < 0x4)
			{
				//ADDI
			}
			else
			{
				//SLTI
			}
	
	}
	else if(hex1 == 0x3)	//purp
	{
		if(hex2 <0x4)
		{
				//ANDI
		}
		if(hex2>0x3 && hex2 <0x8)
		{
				//ORI
		}
		if(hex2>0x7 && hex2<0xC)
		{
				//XORI
		}
		else
		{
				//LUI
		}

	}
	else if(hex1 == 0x8 && ( (hex2 < 0x8) || ( hex2 > 0xB) )) //blue
	{
		if(hex2 <0x4)
		{
				//LB
		}
		if(hex2>0x3 && hex2 <0x8)
		{
				//LH
		}
		if(hex2>0xB)
		{
				//LW
		}
		else
		{
				printf("Not a given function");
		}

	}
	else if(hex1 == 0xA && ( (hex2 <0x8) || (hex2 > 0xB) ))	//green
	{
		if(hex2 <0x4)
		{
				//SB
		}
		if(hex2>0x3 && hex2 <0x8)
		{
				//SH
		}
		if(hex2>0xB)
		{
				//SW
		}
		else
		{
				printf("Not a given function");
		}


	}
	else
	{
		printf("not a valid function\n");
	}

	
 
	
	
	



	//Convert to Binary// (new function)
	
	//pointer to and array and pass it to a function to fill it the array and pass it back	
	
	

	//Move data around  in registers ( new function)




	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
}






/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	/*IMPLEMENT THIS*/
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
