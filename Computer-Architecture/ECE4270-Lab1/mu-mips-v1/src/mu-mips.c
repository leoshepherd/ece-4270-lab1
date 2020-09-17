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
	    uint32_t current_inst = (CURRENT_STATE.PC); 	//get current instruction
	    uint32_t op_code = (current_inst & 0xFC000000) >> 28;     	//get op code of instruction
	    uint32_t funct = (current_inst & 0x1f);           	      	//get function code of instruction 
	    uint32_t rs = (current_inst & 0x3E00000) >> 21;	//get rs register of instruction
            uint32_t rt = (current_inst & 0x1F0000) >> 16;		//get rt register of instruction
            uint32_t rd = (current_inst & 0xF800) >> 11;		//get desination register of instruction
	    uint32_t target = (current_inst & 0x3FFFFFF);		//get target register
	    uint32_t immediate = (current_inst & 0xffff);		//get immediate value
	    uint32_t shift = (current_inst >> 6) & 0x1F;
	    uint32_t offset = (current_inst & 0xFFFF);
  	    uint32_t temp = 0x00000000;
	    uint32_t temp2 = offset & 0x8000 << 16;

	if(op_code == 0b000000){
            switch(funct)
		{
                case 0b100000:        //ADD
		    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + CURRENT_STATE.REGS[rs];
                    break;

                case 0b100001:        //ADDU
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + CURRENT_STATE.REGS[rs];
                    break;

                case 0b100100:        //AND
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] & CURRENT_STATE.REGS[rs];
                    break;

                case 0b100010:        //SUB
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
                    break;

                case 0b100011:        //SUBU
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
                    break;

                case 0b011000:        //MULT
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] * CURRENT_STATE.REGS[rs];
                    break;

                case 0b011001:        //MULTU
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] * CURRENT_STATE.REGS[rs];
                    break;

                case 0b011010:        //DIV
                    if (CURRENT_STATE.REGS[rt] != 0){
                        NEXT_STATE.HI = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
                        NEXT_STATE.LO = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
                    }
                    else{
                        printf("ERROR: result undefined, zero devisor");
                    }
                    break;

                case 0b011011:        //DIVU
                    if (CURRENT_STATE.REGS[rt] != 0){
                        NEXT_STATE.HI = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
			NEXT_STATE.LO = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];

                    }
                    else{
                        printf("ERROR: result undefined, zero devisor");
                    }
                    break;
                
                case 0b100101:        //OR
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] | CURRENT_STATE.REGS[rs];
                    break;

                case 0b100110:        //XOR
                    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] ^ CURRENT_STATE.REGS[rs];
                    break;

                case 0b100111:        //NOR
                    NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rt] | CURRENT_STATE.REGS[rs]);
                    break;

		    
                case 0b101010:        //SLT
		    if (CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
			NEXT_STATE.REGS[rd] = 1;
		    }
		    else{
			NEXT_STATE.REGS[rd] = 0;
		    }
                    break;

                case 0b001000:        //JR
		    NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
                    break;

                case 0b001001:        //JALR
		    NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 8;
		    NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
                    break;                
		case 0b000000:        //SLL
	   		NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] <<shift;
                    break;

                case 0b000010:        //SRL
		   	NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >>shift;
                    break;

                case 0b000011:        //SRA
			NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >>shift;
                    break;

                case 0b010000:        //MFHI
			NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
                    break;

                case 0b010010:        //MFLO
			NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
                    break;

                case 0b010001:        //MTHI
			NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
                    break;

                case 0b010011:        //MTLO
			NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
                    break;
		
       		}
		
}	

	else{

	switch (op_code){
        case 0b001000:                //ADDI
	    temp2 = (immediate & 0x8000) << 16;
	    temp = 0x00000000;
			for(int i =0; i<17; i++)
			{
			temp = temp & temp2;
			temp = temp >> 1;
			}
	    temp = temp & immediate;
	    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + temp;
            break;

        case 0b001001:                //ADDIU
	    temp2 = (immediate & 0x8000) << 16;
	    temp = 0x00000000;
			for(int i =0; i<17; i++)
			{
			temp = temp & temp2;
			temp = temp >> 1;
			}
	    temp = temp & immediate;
	    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + temp;
            break;

        case 0b001100:                //ANDI
	    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] & immediate;

            break;

        case 0b001101:                //ORI

	    NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] || immediate;
            break;
	    

        case 0b001110:                //XORI

	    NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ (((immediate & 0x00)<<16) | immediate);

            break;
	    

        case 0b001010:                //SLTI
            
			
		 temp = 0x00000000;
		temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;
		
			if(CURRENT_STATE.REGS[rs] < (target <<16 | (immediate & 0x0000FFFF)))
			{
				NEXT_STATE.REGS[rd] = 0x00000001;
			}
			else
			{
				NEXT_STATE.REGS[rd] = 0x0;
			}

            break;

        case 0b000010:                //J
		temp = target <<2;
		temp2 = CURRENT_STATE.PC & 0xE0000000;
		NEXT_STATE.PC = temp2 || temp;

            break;

        case 0b000011:                //JAL

		temp2 = CURRENT_STATE.PC & 0xE0000000;
		temp = target;
		temp = temp << 2;
		NEXT_STATE.REGS[31] = CURRENT_STATE.PC +8;
		NEXT_STATE.PC = temp2 || temp;            
		break;

        case 0b100011:                //LW

            break;

        case 0b100000:                //LB

            break;

        case 0b100001:                //LH
	
	

            break;

        case 0b001111:                //LUI
	   	NEXT_STATE.REGS[rt] = immediate;

            break;

        case 0b101011:                //SW

            break;

        case 0b101000:                //SB

            break;

        case 0b101001:                //SH

            break;

        case 0b000100:                //BEQ

	   	
	        temp = 0x00000000;
		temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt] )
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}

            break;

        case 0b000101:                //BNE

		temp = 0x00000000;
		 temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt] )
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}

            break;

        case 0b000110:                //BLEZ

		temp = 0x00000000;
		 temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(((CURRENT_STATE.REGS[rs]<<1) == 1) || (CURRENT_STATE.REGS[rs] == 0 ))
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}


            break; 
        case 0b000001:
            if (funct == 0b00000)	//BLTZ
	    {

		 temp = 0x00000000;
		 temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(((CURRENT_STATE.REGS[rs]<<1) == 1))
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}


	    
	    }       

            else	//BGEZ
	    {
	         temp = 0x00000000;
		 temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(((CURRENT_STATE.REGS[rs]<<1) == 0))
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}
	}        

            break;

        case 0b000111:                //BGTZ
		 temp = 0x00000000;
		 temp2 = offset & 0x8000 << 16;

			for(int i =0; i<15; i++)
			{
				temp = temp & temp2;
				temp = temp >> 1;
			}
		
		
			temp = temp & offset;
			temp = temp << 2 ;

			target = temp;


		if(( CURRENT_STATE.REGS[rs] & 0x8000000) == 0 && (CURRENT_STATE.REGS[rs]<<1) != 0 )
		{
			NEXT_STATE.PC = CURRENT_STATE.PC + target;
		}

		else
		{

			break;
		}

            break;
}

}	/*IMPLEMENT THIS*/
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
	printf("%x", mem_read_32(addr));
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
