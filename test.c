// ************************************************************
// Jacob Terren, Brian Tejada, Cam Turner, Angus Guinen
// 300961195
// CSCI.465: Operating Systems Internals
// Homework #2
// 4/8/2019
// Program Description:
// The purpose of this program is to create a MTOPS machine simulator.
// It will be able to execute machine language programs, utilizing
// the memory made available to the Sim., "hardware" registers,
// and various functions (ADD, SUB, DIV, MULT, FETCH, etc.)
// ************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//CONSTANTS: OK and error codes and PCB array Indexes and timeslice
	#define TIME_SLICE 200				//clock ticks allowed per CPU execution
	#define DEF_PRIORITY 128					//dEFAULT PRIORITY VALUE
	#define PSR 21								//Array index for PSR in PCB
	#define PC 20									//Array index for PC in PCB
	#define SP 19									//Array Index for SP in PCB
	#define GPR7 18								//Array index for GPR 7 in PCB
	#define GPR6 17								//Array index for GPR 6 in PCB
	#define GPR5 16								//Array index for GPR 5 in PCB
	#define GPR4 15								//Array index for GPR 4 in PCB
	#define GPR3 14								//Array index for GPR 3 in PCB
	#define GPR2 13								//Array index for GPR 2 in PCB
	#define GPR1 12								//Array index for GPR 1 in PCB
	#define GPR0 11								//Array index for GPR 0 in PCB
	#define MESSAGE_QUEUE 9				//Array index for number of messages in the queue in PCB
	#define Output_Operation_Event_Code 9
	#define MESSAGE_QUEUE_SIZE 8	//Array index for message queue size in PCB
	#define Input_Operation_Event_Code 8
	#define MESSAGE_QUEUE_START 7	//Array index for message queue start address in PCB
	#define STACK_SIZE 6					//Array index for stack size in PCB
	#define STACK_START_ADDRESS 5	//Array index for stack start address in PCB
	#define PRIORITY 4						//Array index for priority in PCB
	#define WAITING_CODE 3				//Array index for reason for waiting code in PCB
	#define USER_MODE 2								//Create constant UserMode to 2
	#define STATE 2								//Array Index for State in PCB
	#define OS_MODE 1									//Create constant OSMode to 1
	#define READY_STATE 1							//Create constant ReadyState to 1
	#define PID	1									//Array index for PID in PCB
	#define TIME_DEPLETED 1				//Time Slice has been depleated
	#define WAITING_STATE 3				//Create a constant WaitingState set to 3
	#define NEXT_PCB_POINTER 0		//Array index for next PCBptr in PCB
	#define OK 0											//Execution is successful
	#define END_OF_LIST -1						//End Of List initialize -1
	#define INPUT_ERROR -2						//Input from user encounters an error
	#define ER_OPEN_FAILED -3					//File open fails
	#define ER_READ_FAILED -4					//File read fails
	#define ER_INVALID_PC_VAL -5			//Machine Lang. program ends in invalid range for PC
	#define ER_NO_END_IND -6					//No end of program indicator in assembly program
	#define ER_INVALID_RANGE -7				//Range provided is invalid
	#define ER_INVALID_OP_ADDRESS -8	//Invalid address of operand
	#define ER_INVALID_OP_MODE -9			//Invalid operand mode
	#define ER_INVALID_ADDRESS -10		//Invalid address provided
	#define ER_INVALID_GPR_ADDR -11		//Invalid gpr address
	#define ER_INVALID_OP_CODE -12		//Invalid operand code
	#define ER_DEST_OP_INVALID -13		//Invalid operand destination
	#define ER_ATT_DIVIDE_BY_0 -14		//Division by zero was attempted
	#define ER_STACK_OVERFLOW -15			//Overflow of stack
	#define ER_ABS_LOAD_FAIL -16			//Absolute Loader failed
	#define ER_EXE_FAILED -17					//Absolute Loader failed
	#define ER_INVALID_SIZE -18
	#define ER_INVALID_PID	-19
	#define ER_INVALID_ID 	-20


//Structures
	struct WaitingNode //nodes for the Waiting Queue Linked list
	{
		long waitProcess; 						//points to the front most PCB in the queue
		struct WaitingNode *next; 		//points to the next node in the queue
	};

	struct ReadyNode //nodes for the Ready Queue Linked List
	{
		long readyProcess;						//points to the PCB to be inserted
		struct ReadyNode *next;				//points to the next node in the queue
	};

	struct ProgramInfo	//Used to store start address, size, and end address of the Assembly program in the stack
	{
		long StartAddress;	//Stores start address
		long size;			//Stores number of instructions in program
		long EndAddress;	//Stores end address
		long pid;
	};

	struct InstructionInfo	//Holds information for current instruction of assembly progra
	{
		long Opcode;	//Stores value of Operand code
		long Op1Mode;	//Stores value of Op1 mode
		long Op2Mode;	//Stores value of Op2 mode
		long Op1Gpr;	//Stores address of Op1 in GPR
		long Op2Gpr;	//Stores address of Op2 in GPR
		long Op1Value;	//Stores value of Op2
		long Op2Value;	//Stores value of Op2
		long OpValue;	//Temp Value
		int Op1Address;	/*Used to stor Op1 and Op2 addresses Changed from long to int to avoid Seg. Fault, not sure why it*/
		int Op2Address; /*kept throwing Seg. Fault, I was following pseudocode but it wouldn't work. */
		int OpAddress;  //Temp address
	};

//Global Variables
	long MemArray[10000];				//Main Memory
	long mar;							//Memory address register
	long mbr;							//Memory Buffer Register
	long Clock;							//System Clock
	long gpr[8];						//General Purpose Register of size 8
	long IR;							//Instruction register
	long psr;							//Processor Status Register
	long pc;							//Program Counter
	long sp;							//Stack Pointer
	struct ProgramInfo Progs[10];	//ProgramInfo object ****CHECK ME, DYNAMICALLY ALLOCATE THIS****
	struct ProgramInfo CurrProg;
	long progsIndex = 0;
	struct InstructionInfo Op;			//InstructionInfo object
	long RQ = END_OF_LIST;				//Initialize Ready Queue to EndOfList (-1)
	long WQ = END_OF_LIST;				//Initialize Waiting Queue to EndOfList (-1)
	long OSFreeList = END_OF_LIST;		//Initialize OSFreeList to EndOfList (-1)
	long UserFreeList = END_OF_LIST;	//Initialize UserFreeList to EndOfList (-1)
	long ProcessID = 1;
	long nextAddress = 0;
	long SystemShutdownStatus = 1; //initialize System Shutdown status to 1
	struct WaitingNode *whead = NULL;
	struct ReadyNode *rhead = NULL;

	long priority = DEF_PRIORITY;


//Function Prototypes
	long FetchOperand(long  OpMode, long  OpReg, int  OpAddress, long  OpValue);
	int DumpMemory(char String[], long StartAddress, long size);
	long CreateProcess(char fileName[], long priority);
	long printWaitingQueue(struct WaitingNode *whead);
	long printReadyQueue(struct ReadyNode *rhead);
	long FreeUserMemory (long ptr, long size);
	long FreeOSMemory (long ptr, long size);
	long searchAndRemovePCBFromWQ(long pid);
	void Dispatcher(long RunningPCBptr);
	void ISRoutputCompletionInterrupt();
	long AllocateUserMemory (long size);
	int AbsoluteLoader(char fileName[]);
	void ISRinputCompletionInterrupt();
	long SystemCall(long SystemCallID);
	void terminateProcess(long PCBptr);
	long AllocateOSMemory (long size);
	void InitializePCB(long PCBptr);
	long InsertIntoRQ (long PCBptr);
	long InsertIntoWQ (long PCBptr);
	void CheckAndProcessInterrupt();
	void ISRrunProgramInterrupt();
	void saveContext(long PCBPtr);
	void PrintPCB(long PCBptr);
	long SelectProcessFromRQ();
	long memAllocSystemCall();
	void ISRshutdownSystem();
	long memFreeSystemCall();
	long io_putcSystemCall();
	long io_getcSystemCall();
	long CPU();
	int fetchProgInfo(long RunningPCBptr);
	

// ************************************************************
// Function: InitializeSystem
//
// Task Description:
// 	Initializes all hardware components to zero, this function is called once from main,
// at start up of HYPO machine.
//
// Input Parameters
//	None
//
// Output Parameters
//	None
//
// Function Return Value
//	None
// ************************************************************
void InitializeSystem()	//Begin InitializeSystem() function
{
	int i = 0;				//Loop variable

	while(i < 10000)
	{
		MemArray[i] = 0;	//All main memory locations initialized to zero
		i++;
	}

	i = 0;					//Reset loop variable
	mar	= 0;				//Memory address register initialized to zero
	mbr	= 0;				//Memory Buffer Register initialized to zero
	Clock = 0;				//System Clock initialized to zero

	while (i < 8)
	{
		gpr[i] = 0;			//All values of GPR are initialized to zero
		i++;
	}

	IR = 0;					//Instruction register initialized to zero
	psr = 0;				//Processor Status Register initialized to zero
	pc = 0;					//Program Counter initialized to zero
	sp = 0;					//Stack Pointer initialized to zero

	// Create User free list using the free block address and size given in the class
	UserFreeList = 3000;
	MemArray[UserFreeList] = END_OF_LIST;
	MemArray[UserFreeList+1] = 4000;

	// Create OS free list using the free block address and size given in the class
	OSFreeList = 7000;
	MemArray[OSFreeList] = END_OF_LIST;
	MemArray[OSFreeList+1] = 3000;

	CreateProcess("ognull.txt", 0); //CreateProcess function passing Null Process Executable File and priority zero as arguments
}  //End InitializeSystem() function

// ******************************************************
// function: main
//
// Task Description:
// 	Main function of the program, it's purposes are; to call InitializeSystem() function,
// ask for user to input a filename, call AbsoluteLoader() function with user input as
// the parameter,  check result for error and if error then report error, set program counter
// to the returned value, after program is loaded then it dumps the memory, and executes the
// program using CPU() function.  After execution the main will dump the memory once more.
// Input parameters
//	none
// Output parameters
//	none
// Function return value
//	OK					- Execution is successful
//	INPUT_ERROR			- Input from user encounters an error
//	ER_ABS_LOAD_FAIL	- Absolute Loader returned as failed
//  ER_EXE_FAIL 		- Absolute Loader failed
// ******************************************************
int main()	//Start of main function
{
	long RunningPCBptr;
	long ExeCompStat = OK;	//Used for status of program execution
	InitializeSystem(); //Calls InitializeSystem() function to initialize hardware

	while(SystemShutdownStatus != 0)
	{
		CheckAndProcessInterrupt();
		//contents of ready and waiting Queue
		printReadyQueue(rhead);
		printWaitingQueue(whead);
		//Dumps Dynamic Memory Area before CPU Scheduling
		//DumpMemory("Dynamic Memory Area: Before CPU Scheduling",3000, 199);		

		// Select next process from RQ to give CPU
		RunningPCBptr = SelectProcessFromRQ();  // call the function
		printf("RQ selected\n");
		// Perform restore context using Dispatcher
		Dispatcher(RunningPCBptr);

		printReadyQueue(rhead);			//Dumps Ready Queue After Selecting Process from RQ
		PrintPCB(RunningPCBptr);		//Dump Running PCB and CPU Context passing
		DumpMemory("Before executing user program", 0, 100);
		ExeCompStat = fetchProgInfo(RunningPCBptr);
		if(ExeCompStat == OK)
		{
			ExeCompStat = CPU();	//Calls CPU() to execute test program
			saveContext(RunningPCBptr);
		}
		else
		{
			printf("Shit failed to find PID m8");
		}

		DumpMemory("LOOK IN MY PANTS BITCHESS", 3000, 100);
		DumpMemory("After executing user program", 0, 100);	//Dumps Main Memory from adress 0 to address 99 after executing test program

		// Check return status reason for giving up CPU
		if(ExeCompStat == TIME_DEPLETED)
		{
			saveContext(RunningPCBptr);  // running process is losing CPU
			InsertIntoRQ(RunningPCBptr);
			RunningPCBptr = END_OF_LIST;
		}
		else if(ExeCompStat <= 0)  // Halt or run-time error
		{
			terminateProcess(RunningPCBptr);
			RunningPCBptr = END_OF_LIST;
		}
		else if(ExeCompStat == Input_Operation_Event_Code)		// io_getc
		{
			MemArray[RunningPCBptr + WAITING_CODE] = Input_Operation_Event_Code;
			InsertIntoWQ(RunningPCBptr);
			RunningPCBptr = END_OF_LIST;
		}
		else if(ExeCompStat == Output_Operation_Event_Code)		// io_putc
		{
			MemArray[RunningPCBptr + WAITING_CODE] = Output_Operation_Event_Code;
			InsertIntoWQ(RunningPCBptr);
			RunningPCBptr = END_OF_LIST;
		}
		else
		{	// Unknown programming error
			printf("Unknown Programming Error");
		}
	}//end of while not shut down loop

	//Print OS is shutting down message;
	printf("Shutting Down...");

	return(ExeCompStat);  	// return from main
}  //End of main function

// ********************************************************************
// Function: AbsoluteLoader
//
// Task Description:
// 	To open the test program file, read the file, and save contents to the appropriate addresses provided in the file.
//
// Input Parameters
//	fileName			Name of the test program
//
// Output Parameters
//	None
//
// Function Return Value will be one of the following:
//	ER_OPEN_FAILED				- File open fails
//	ER_READ_FAILED				- File read fails
//	ER_INVALID_PC_VAL			- Machine Lang. program ends in invalid range for PC
//	ER_NO_END_IND				- No end of program indicator in assembly program
//	0 to Valid address range	- Load of test program was successful, returns the StartAddress of the test program to save to PC in main()
// ************************************************************
int AbsoluteLoader (   //Start of AbsoluteLoader function
char fileName[])		//File name of test program
{
	printf("Absolute loader has been called.\n");
	FILE *filePtr;	//Pointer to read file into
	long returnInstr = 0;	//Saves return codes from other function calls
	char buffer[1024];	//Buffer to read file into
	long Status = OK;	//Status of function
	char *ptr;			//Pointer to use with strtok(), holds string tokens
	char temp[1024] = "";	//Temporary buffer to hold current string token as a string
	long count = 0;		//Counter
	long FirstAddress = 0;	//Holds First address read from file
	long MemIndex = 0;		//Holds current address to write content into
	long NoEnd = -1;	//Used to detect end of program indicator (-1) in test program. By default it's set to -1 if
					//EOP indicator is encountered then NoEnd will be set to 0, else error
	if((filePtr = fopen(fileName, "r")) == NULL)	//Opens file to filePtr with read permissions
	{
		perror("Failed to open file.");	//Returns error value of fopen function
		fclose(filePtr);	//Close filePtr before function ends
		return(ER_OPEN_FAILED);	//If open fails return ER_OPEN_FAILED code
	}

    Status = fread(buffer, sizeof(buffer)+1, 1, filePtr);	//Reads from filePtr into buffer
	if(Status != OK)
	{
		perror("Failed to read file.");	//Returns error value of fopen function
		fclose(filePtr);	//Close filePtr before function ends
		return(ER_READ_FAILED);	//If read fails return ER_READ_FAILED code
	}
	printf("Executing while loop\n");
	ptr = strtok(buffer, "\t\n"); 	//Tokenizes strings in buffer to ptr
	while (ptr != NULL)	//Executes while loop as long as ptr has a new token
	{
		strcpy(temp, ptr);	//Copies string token to temp as a string
		if(count == 0)	//At first loop grabs necessary info to be used in the future
		{
			printf("checking program counter\n");
			MemIndex = nextAddress;	//Saves address for Main Memory to put content in
			FirstAddress = atoi(temp)+MemIndex;	//Saves the first address to check range validity in the future
			ptr = strtok(NULL, "\t\n");	//Grabs next string token which will be content to be saved
			strcpy(temp, ptr);			//Copies string token to temp as a string
			MemArray[MemIndex] = atol(temp);	//Saves content to Main memory address
			count++;	//Increment counter
		}
		else if(atoi(temp) == -1)	//If End of program indicator is encountered
		{
			NoEnd = 0;	//Sets NoEnd to 0 so ER_NO_END_IND isn't thrown later
			fclose(filePtr);	//Closes filePtr
			ptr = strtok(NULL, "\t\n");	//Grabs next string token which will be the beginning address of the test program
			strcpy(temp, ptr);	//Copies string token to temp as a string

			if(0 <= atoi(temp)+FirstAddress && FirstAddress+atoi(temp) <= 2999)	//If the begnning address of test program is in valid range
			{
				Progs[progsIndex].StartAddress = FirstAddress;	//Saves first address of the test program to program info
				Progs[progsIndex].size = count;	//Saves number of instructions in the test program to program info
				Progs[progsIndex].EndAddress = FirstAddress + count - 1;	//Saves last address of the test program to program info
				printf("\nnextAddress: %ld", nextAddress);
				return(atoi(temp) + nextAddress);	//Returns Beginning of program address to Main() to save to PC
			}
			else	//else if value is not in the test programs address range throw error
			{
				printf("Invalid address for Program Counter at end of program, fix and try again.\n");
				fclose(filePtr);	//Close filePtr before function ends
				return(ER_INVALID_PC_VAL);	//Returns invalid pc value error code
			}
		}
		else
		{
			MemIndex = MemIndex + 1;	//Saves address for Main Memory to put content in
			ptr = strtok(NULL, "\t\n");	//Grabs next string token which will be content to be saved
			strcpy(temp, ptr);	//Copies string token to temp as a string
			MemArray[MemIndex] = atol(temp); //Saves content to Main memory address
			count++;	//Increment counter
		}
		ptr = strtok(NULL, "\t\n");	//Grabs next string token which will be an address
	}

	fclose(filePtr);	//Close filePtr before function ends
	if(NoEnd == -1)	//If EOP indicator was never encountered then throw error
	{
		printf("Assembly program does not contian end of program indicator (-1), fix and try again.");
		return (ER_NO_END_IND);	//Returns No End of Program indicator error code
	}
	printf("\nPC value: %ld", returnInstr);
	return(returnInstr);	//Return to main
}  //End of AbsoluteLoader function

// ************************************************************
// Function: CPU
//
// Task Description:
// 	Executes test program saved in main memory.
//
// Input Parameters
//	None
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					= Execution was successful
//	ER_INVALID_ADDRESS	- Invalid address of operand
//	ER_INVALID_OP_MODE	- Invalid operand mode
//	ER_INVALID_GPR_ADDR	- Invalid gpr address
//	ER_DEST_OP_INVALID	- Invalid operand destination
//	ER_ATT_DIVIDE_BY_0	- Division by zero was attempted
//	ER_STACK_OVERFLOW	- Overflow of stack
//	ER_INVALID_OP_CODE	- Invalid operand code
// ************************************************************

long CPU()
{
	//Local variables
	long Remainder;							//Variable to hold remainder from modulos
	long status = OK;						//Status variable
	long Result;								//Result variable from arithmetic
	long isHalt = 0;						//Used to control while loop if halt is encountered or not
	long TimeLeft = TIME_SLICE; //set TimeLife equal to 200 clock ticks
	while (isHalt == 0 && status >= 0)	//If no halt or error is encountered the loop will continue
	{
		if(TimeLeft <= 0)
		{
			printf("Out Of Time %ld\n", CurrProg.pid);
			return(TIME_DEPLETED);
		}
		//Fetch instruction cycle
		if(0 <= pc && pc <= 2999)	//If program counter is in valid range
		{
			mar = pc;	//Save PC to MAR
			pc++;		//Increment PC
			mbr = MemArray[mar];	//Save content at address (MAR) in Main Memory
		}
		else	//Else throw invalid address error
		{
			printf("#1Invalid address runtime error, pc value: %ld\n", pc);
			return (ER_INVALID_ADDRESS);	//returns invalid address error code
		}

		IR = mbr;	//MBR to IR

		//Decode instruction cycle
		//Get Opcode
			Op.Opcode = IR / 10000;		// Integer division, gives quotient
			Remainder = IR % 10000;	// Modulo (%) gives remainder of integer division
		//Get Operand 1 mode
			Op.Op1Mode = Remainder / 1000;
			Remainder = Remainder % 1000;
		//Get Operand 1 gpr address
			Op.Op1Gpr = Remainder / 100;
			Remainder = Remainder % 100;
		//Get Operand 2 mode
			Op.Op2Mode = Remainder / 10;
			Remainder = Remainder % 10;
		//Get Operand 2 gpr address
			Op.Op2Gpr = Remainder / 1;
			Remainder = Remainder % 1;

		//printf("OpCode: %ld, Op1Mode: %ld, Op1Gpr: %ld, Op2Mode: %ld, Op2Gpr: %ld\n", Op.Opcode, Op.Op1Mode, Op.Op1Gpr, Op.Op2Mode, Op.Op2Gpr);

		//Check validity of Opcode, Op1 mode, Op1 gpr address, Op2 mode, Op2 gpr address
		if(!(0 <= Op.Opcode && Op.Opcode < 13))
		{
			printf("Invalid Operand Code in Opcode: %ld\n", Op.Opcode);		// Throws error code against constraint Op.Opcode number range
			return(ER_INVALID_OP_CODE);
		}
		else if(!(0 <= Op.Op1Mode && Op.Op1Mode < 7))		//Checks generated Opcode against valid opcode range, throws error if outside the set range
		{
			printf("Invalid Operand Mode in Op1Mode: %ld\n", Op.Op1Mode);
			return(ER_INVALID_OP_MODE);
		}
		else if(!(0 <= Op.Op2Mode && Op.Op2Mode < 7))		//Checks generated Opcode against valid opcode range, throws error if outside the set range
		{
			printf("Invalid Operand Mode in Op2Mode: %ld pc: %ld\n", Op.Op2Mode, pc);
			return(ER_INVALID_OP_MODE);
		}
		else if(!(0 <= Op.Op1Gpr && Op.Op1Gpr < 8))		//Checks generated Opcode against valid opcode range, throws error if outside the set range
		{
			printf("Invalid Gpr address in Op1Gpr: %ld\n", Op.Op1Gpr);
			return(ER_INVALID_GPR_ADDR);
		}
		else if(!(0 <= Op.Op2Gpr && Op.Op2Gpr < 8))		//Checks generated Opcode against valid opcode range, throws error if outside the set range
		{
			printf("Invalid Gpr address in Op2Gpr: %ld\n", Op.Op2Gpr);
			return(ER_INVALID_GPR_ADDR);
		}
		//End of check for validity

		switch (Op.Opcode)	//Read Opcode to determine action
		{
			case 0:  //Halt
				printf("Halt encountered in program\n");
				isHalt = 1;	//Set isHalt to 1 to stop while loop
				Clock += 12;	//Increment clock by 12
				TimeLeft -= 12; //decrement TimeLeft by execution time
				return (status);	//Returns OK
			break;

			case 1: //Add
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				status = FetchOperand(Op.Op2Mode, Op.Op2Gpr, Op.Op2Address, Op.Op2Value);	//Fetches second value and address
				if(status < 0) //check for error and return error code
					return (status);
				else
				{
					Op.Op2Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op2Value
					Op.Op2Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op2Address
				}

				//Add the operand values
				Result =  Op.Op1Value + Op.Op2Value;	//Adds values

				//Stores the result into Operand 1 address
				if (Op.Op1Mode == 1)	//If Operand 1 mode is register mode
				{
					gpr[Op.Op1Gpr] = Result;	//Save result to Operand 1 GPR address
				}
				else if (Op.Op1Mode == 6)	//Operand 1 mode cannot be in immediate mode, immediate values are not accepted for Operand 1 Value
				{
					  printf("Destination operand mode cannot be immediate mode error.");
					  return (ER_DEST_OP_INVALID);	//Returns invalid opreand destination code
				}
				else
				{
					MemArray[Op.Op1Address] = Result;	//Saves content to Operand 1 address in main memory
				}

				Clock += 3;	//Increment clock by 3
				TimeLeft -= 3; //decrement TimeLeft by execution time
			break;

			case 2:  //Subtract
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				status = FetchOperand(Op.Op2Mode, Op.Op2Gpr, Op.Op2Address, Op.Op2Value);	//Fetches second value and address
				if(status < 0) //check for error and return error code
					return (status);
				else
				{
					Op.Op2Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op2Value
					Op.Op2Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op2Address
				}
				//Subtract the operand values
				Result =  Op.Op1Value - Op.Op2Value;	//Subtracts values

				//Stores the result into Operand 1 address
				if (Op.Op1Mode == 1)	//If Operand 1 mode is register mode
				{
					gpr[Op.Op1Gpr] = Result;	//Save result to Operand 1 GPR address
				}
				else if (Op.Op1Mode == 6)	//Operand 1 mode cannot be in immediate mode, immediate values are not accepted for Operand 1 Value
				{
					  printf("Destination operand mode cannot be immediate mode error.");
					  return (ER_DEST_OP_INVALID);	//Returns invalid opreand destination code
				}
				else
				{
					MemArray[Op.Op1Address] = Result;	//Saves content to Operand 1 address in main memory
				}

				Clock += 3;	//Increment clock by 3
				TimeLeft -= 3; //decrement TimeLeft by execution time
			break;

			case 3:
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				status = FetchOperand(Op.Op2Mode, Op.Op2Gpr, Op.Op2Address, Op.Op2Value);	//Fetches second value and address
				if(status < 0) //check for error and return error code
					return (status);
				else
				{
					Op.Op2Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op2Value
					Op.Op2Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op2Address
				}

				//Multiply the operand values
				Result =  Op.Op1Value * Op.Op2Value;	//Multiples values

				//Stores the result into Operand 1 address
				if (Op.Op1Mode == 1)	//If Operand 1 mode is register mode
				{
					gpr[Op.Op1Gpr] = Result;	//Save result to Operand 1 GPR address
				}
				else if (Op.Op1Mode == 6)	//Operand 1 mode cannot be in immediate mode, immediate values are not accepted for Operand 1 Value
				{
					  printf("Destination operand mode cannot be immediate mode error.");
					  return (ER_DEST_OP_INVALID);	//Returns invalid opreand destination code
				}
				else
				{
					MemArray[Op.Op1Address] = Result;	//Saves content to Operand 1 address in main memory
				}

				Clock += 6;	//Increment clock by 6
				TimeLeft -= 6; //decrement TimeLeft by execution time
			break;

			case 4:
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				status = FetchOperand(Op.Op2Mode, Op.Op2Gpr, Op.Op2Address, Op.Op2Value);	//Fetches second value and address
				if(status < 0) //check for error and return error code
					return (status);
				else
				{
					Op.Op2Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op2Value
					Op.Op2Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op2Address
				}

				//Divide the operand values
				Result =  Op.Op1Value / Op.Op2Value;	//Divides values

				//Stores the result into Operand 1 address
				if (Op.Op1Mode == 1)	//If Operand 1 mode is register mode
				{
					gpr[Op.Op1Gpr] = Result;	//Save result to Operand 1 GPR address
				}
				else if (Op.Op1Mode == 6)	//Operand 1 mode cannot be in immediate mode, immediate values are not accepted for Operand 1 Value
				{
					  printf("Destination operand mode cannot be immediate mode error.");
					  return (ER_DEST_OP_INVALID);	//Returns invalid opreand destination code
				}
				else
				{
					MemArray[Op.Op1Address] = Result;	//Saves content to Operand 1 address in main memory
				}

				Clock += 6;	//Increment clock by 6
				TimeLeft -= 6; //decrement TimeLeft by execution time
			break;

			case 5: //Move
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					printf("CPU Op1Val: %ld\n", Op.Op1Value);
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
					printf("CPU Op1Address: %ld\n", Op.Op1Address);
				}

				status = FetchOperand(Op.Op2Mode, Op.Op2Gpr, Op.Op2Address, Op.Op2Value);	//Fetches second value and address
				if(status < 0) //check for error and return error code
					return (status);
				else
				{
					Op.Op2Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op2Value
					printf("CPU Op2Val: %ld\n", Op.Op2Value);
					Op.Op2Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op2Address
					printf("CPU Op2Address: %ld\n", Op.Op2Address);
				}
				
				if (Op.Op1Mode == 1)	//If Operand 1 mode is register mode
				{
					gpr[Op.Op1Gpr] = Op.Op2Value;	//Save result to Operand 1 GPR address
				}
				else if (Op.Op1Mode == 6)	//Operand 1 mode cannot be in immediate mode, immediate values are not accepted for Operand 1 Value
				{
					  printf("Destination operand mode cannot be immediate mode error.");
					  return (ER_DEST_OP_INVALID);	//Returns invalid opreand destination code
				}
				else
				{
					MemArray[Op.Op1Address] = Op.Op2Value;	//Saves Operand 2 value at Operand 1 address in main memory
				}
				Clock += 2;	//Increment clock by 2
				TimeLeft -= 2; //decrement TimeLeft by execution time
			break;

			case 6:  //Branch or jump to address
				if(CurrProg.StartAddress <= MemArray[pc] && MemArray[pc] <  CurrProg.EndAddress)	//Checks if PC is in valid range, else throws error
				{
					pc = Op.Op1Address + CurrProg.StartAddress + 1;	//Saves content of main memory at PC to PC
				}
				else
				{
					printf("#2Invalid address runtime error, pc value: %ld\n", pc);;
					return (ER_INVALID_ADDRESS);	//returns Invalid address error
				}

				Clock += 2;	//Increment clock by 2
				TimeLeft -= 2; //decrement TimeLeft by execution time
			break;

			case 7:  //Branch or jump on Minus
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}
				printf("Op1Val: %ld\n", Op.Op1Value);
				if(Op.Op1Value < 0) //If operand value is less than zero...
				{
					if(CurrProg.StartAddress <= MemArray[pc] && MemArray[pc] <  CurrProg.EndAddress) //...and new PC is in valid range
					{
						pc = MemArray[pc] + CurrProg.StartAddress;	//Then jump to new PC address
						printf("Jump to: %ld\n", pc);
					}
					else	//else if new PC is not in valid range then throw error
					{
						printf("#3Invalid address runtime error, pc value: %ld\n", pc);;
						return (ER_INVALID_ADDRESS);	//Returns invalid address error
					}
				}
				else	//else Skip branch address to go to next instruction
				{
					pc++;  //Increment PC
				}

				Clock += 4;	//Increment clock by 4
				TimeLeft -= 4; //decrement TimeLeft by execution time
			break;

			case 8: //Branch or jump on plus
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				if(Op.Op1Value > 0) //If operand value is greater than zero...
				{
					if((CurrProg.StartAddress <= MemArray[pc] && MemArray[pc] <  CurrProg.EndAddress)) //...and new PC is in valid range
					{
						pc = MemArray[pc] + CurrProg.StartAddress;	//Then jump to new PC address
					}
					else	//else if new PC is not in valid range then throw error
					{
						printf("#4Invalid address runtime error, pc value: %ld\n", MemArray[pc]);
						return (ER_INVALID_ADDRESS);	//Returns invalid address error
					}
				}
				else	//else Skip branch address to go to next instruction
				{
					pc++;  //Increment PC
				}

				Clock += 4;	//Increment clock by 4
				TimeLeft -= 4; //decrement TimeLeft by execution time
			break;

			case 9://Branch or jump on zero
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				if(Op.Op1Value == 0) //If operand value is equal to zero...
				{
					if((0 <= MemArray[pc] && MemArray[pc] < 3000)) //...and new PC is in valid range
						pc = MemArray[pc];	//Then jump to new PC address
					else	//else if new PC is not in valid range then throw error
					{
						printf("Invalid address runtime error, pc value: %ld\n", pc);;
						return (ER_INVALID_ADDRESS);	//Returns invalid address error
					}
				}
				else	//else Skip branch address to go to next instruction
				{
					pc++;  //Increment PC
				}

				Clock += 4;	//Increment clock by 4
				TimeLeft -= 4; //decrement TimeLeft by execution time
			break;

			case 10:  //Push
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				if(sp > 9999) //If SP is not in valid range throw error
				{
					printf("Error! Stack overflow! Push failed.");
					return (ER_STACK_OVERFLOW);	//Returns Stack Overflow error
				}
				else	//Else
				{
					sp++;	//Increment SP
					MemArray[sp] = Op.Op1Value;	//Save Operand 1 value to Main Memory at address SP
				}

				Clock += 2;	//Increment clock by 2
				TimeLeft -= 2; //decrement TimeLeft by execution time
			break;

			case 11:  //Pop
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);	//Fetches first value and address
				if(status < 0) //Checks for error and returns error code
					return (status);
				else
				{
					Op.Op1Value = Op.OpValue;	//Saves value from FetchOperand to Op.Op1Value
					Op.Op1Address = Op.OpAddress;	//Saves address from FetchOperand to Op.Op1Address
				}

				if(sp > 9999) //If SP is not in valid range throw error
				{
					printf("Error! Stack overflow! Push failed.");
					return (ER_STACK_OVERFLOW);	//Returns Stack Overflow error
				}
				else
				{
					Op.Op1Value = MemArray[sp];	//Save content in Main Memory at address SP to Operand 1 value
					sp--;	//Decrement SP
				}

				Clock += 2;	//Increment clock by 2
				TimeLeft -= 2; //decrement TimeLeft by execution time
			break;

			case 12:  // System call
				status = FetchOperand(Op.Op1Mode, Op.Op1Gpr, Op.Op1Address, Op.Op1Value);
				if(status < 0)
				{
					return(status);//return error status
				}
				status = SystemCall(Op.OpValue);
				Clock += 2;	//Increment clock by 2
				TimeLeft -= 2; //decrement TimeLeft by execution time
			break;

			default:  //Invalid opcode
				printf("Invalid op code: %ld\n", Op.Opcode);
				return (ER_INVALID_OP_CODE);	//Returns invalid op code error
		}  //End of switch opcode
	}  //End of while loop

	return (status);
}  // end of CPU() function

// ************************************************************
// Function: FetchOperand
//
// Task Description:
// 	Fetches address and content for each operand based on modes
//
// Input Parameters
//	OpMode			Operand mode value
//	OpReg			Operand GPR value
// Output Parameters
//	OpAddress			Address of operand
//	OpValue			Operand value when mode and GPR are valid
//
// Function Return Value
//	OK				- Fetch is successful
//	ER_INVALID_OP_ADDRESS	- Invalid address of operand
//	ER_INVALID_OP_MODE		- Invalid operand mode
// ************************************************************

long FetchOperand(
long  OpMode, 	//Operand mode, input parameter
long  OpReg,		//operand GPR, input parameter
int  OpAddress,	//Operand address, output parameter
long  OpValue   //Operand value, output parameter
)
{
	long temp = OpValue;
	temp++;
	// Fetch operand value based on the operand mode
	switch (OpMode)
	{
		case 1:  //Register Mode - Operand value is in GPR
			OpAddress = ER_INVALID_OP_ADDRESS;   //Sets to a negative/invalid address
			Op.OpValue = gpr[OpReg];  //Grabs operand value from GPR
		break;

		case 2:  //Register deferred mode - Op address is in GPR and Op value is in main memory
			OpAddress = gpr[OpReg];   // Grabs Operand address from GPR
			if(0 <= OpAddress && OpAddress < 9999)	//If address is in valid range
			{
			    Op.OpValue = MemArray[OpAddress];  //Grabs operand value from Main Memory
			}
			else	//Throws error if address is invalid
			{
				printf("1 Invalid operand address: %d\n", OpAddress);
				return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
			}
		break;

		case 3:  // Autoincrement mode - Op address is in GPR and Op value is in main memory
			printf("Opreg: %ld\n", OpReg);
			OpAddress = gpr[OpReg];   // Grabs Operand address from GPR
			printf("Address: %ld\n", OpAddress);
			if(0 <= OpAddress && OpAddress < 9999)	//If address is in valid range
			{
			    Op.OpValue = MemArray[OpAddress];  //Grabs operand value from Main Memory
					printf("OpVal: %ld\n", Op.OpValue);
			}
			else	//Throws error if address is invalid
			{
				printf("2 Invalid operand address: %d\n", OpAddress);
				return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
			}
			gpr[OpReg]++;    //Increments register content by 1
		break;

		case 4:  // Autodecrement mode - Op address is in GPR and Op value is in main memory
			--gpr[OpReg];    //Decrements register content by 1
			OpAddress = gpr[OpReg] + CurrProg.StartAddress;   //Grabs Operand address from GPR
			if(CurrProg.StartAddress <= OpAddress && OpAddress < CurrProg.EndAddress)	//If address is in valid range
			{
			    Op.OpValue = MemArray[OpAddress];  //Grabs operand value from Main Memory
			}
			else	//Throws error if address is invalid
			{
				printf("3 Invalid operand address: %d\n", OpAddress);
				return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
			}
		break;

		case 5:  //Direct mode - Value is in Main Memory

			if(CurrProg.StartAddress <= pc && pc < CurrProg.EndAddress)
			{
				OpAddress = MemArray[pc++] + CurrProg.StartAddress;  //Grabs Operand address from main memory, increments PC after fetching
				if(CurrProg.StartAddress <= OpAddress && OpAddress < CurrProg.EndAddress)	//If address is in valid range
				{
					Op.OpValue = MemArray[OpAddress];  //Grabs operand value from Main Memory
					Op.OpAddress = OpAddress;		//Saves operand address to Op object
				}
				else	//Throws error if operand address is invalid
				{
					printf("4 Invalid operand address: %d\n", OpAddress);
					return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
				}
			}
			else	//Throws error if PC address is invalid
			{
				printf("5 Invalid pc address: %ld, %ld\n", pc, CurrProg.pid);
				return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
			}
		break;

		case 6:  //Immediate mode - Value is in instruction
			if(CurrProg.StartAddress <= pc && pc < CurrProg.EndAddress)
			{
				OpAddress = ER_INVALID_OP_ADDRESS;   //Sets to a negative/invalid address
				Op.OpValue = MemArray[pc];  //Grabs operand value from main memory
				pc++;	//increments PC
			}
			else	//Throws error if PC address is invalid
			{
				printf("6 Invalid operand address: %ld\n", pc);
				return (ER_INVALID_OP_ADDRESS);	//Returns invalid address error
			}
		break;

		default:  // Invalid op mode
			printf("Invalid mode error: %ld\n", OpMode);
			return (ER_INVALID_OP_MODE);	//returns invalid op mode error
		break;
	}  //End of switch OpMode

   return(OK);  // return OK on success
}  // end of FetchOperand() function

// ************************************************************
// Function: DumpMemory
//
// Task Description:
//	Displays contents of Main Memory in the range provided by input parameters. Also displays
//	values of GPRs, SP, PC, PSR, and Clock. All content is displayed in a specific format
//	to easily identify addresses and contents.
//
// Input Parameters
//	String				String to be displayed before memory is dumped
//	StartAddress		Start address of main memory location
//	Size				Number of main memory locations to dump
// Output Parameters
//	None
//
// Function Return Value
//	ER_INVALID_RANGE	- Range provided is invalid
//	OK					- Success memory dump
// ************************************************************
int DumpMemory( 
char String[],
long StartAddress,
long size)
{
	printf("%s\n", String);	//Prints string from input parameter

	if(StartAddress < 0 || size > 10000 || (StartAddress + size) > 10000)	//Check the start addr against the valid memory range
	{
		printf("Invalid range parameters for DumpMemory() function, try again.");	//Throw error code to user for error checking
		return(ER_INVALID_RANGE);	//returns invalid range error
	}

	printf("GPR:\tG0\tG1\tG2\tG3\tG4\tG5\tG6\tG7\tSP\tPC\n\t"); //Displays GPRs, SP, & PC header

	for(int i = 0; i < 8; i++)	//Declares a for loop to move through GPR list
		printf("%ld\t", gpr[i]);	//Prints each GPR with the current index of the 

	printf("%ld\t%ld\n", sp, pc);	//Displays SP, & PC

	printf("Address\t+0\t+1\t+2\t+3\t+4\t+5\t+6\t+7\t+8\t+9");	//Displays address header

	for(int x = StartAddress; x < (StartAddress+size); x += 10)	//Displays contents of main memory in given range, Increments by 10 to display as address side-header
	{															
		printf("\n%d\t", x);	//Displays address header as multiples of 10
		
		for(int y = 0; y < 10; y++)		//Displays contents of main memory
			
			printf("%ld\t", MemArray[x+y]); //Print statement to display memory array information
	}
	
	printf("\nClock:\t%ld", Clock);	//Displays value of Clock
	printf("\nPSR:\t%ld\n", psr);	//Displays value of PSR

    return OK;	//Returns OK if successful dump
}  //End of DumpMemory() function

// ************************************************************
// Function: SystemCall
//
// Task Description:
//	Takes input parameter SystemCallID and then using that to determine which function to
//	execute in a switch statement. There are 9 possible cases; CreateProcess, DeleteProcess,
//	ProcessInquiry, DynamicMemoryAllocation, FreeDynamicallyAllocation, io_getcSystemCall,
//	io_putcSystemCall.
//
// Input Parameters
//	long SystemCallID
//
// Output Parameters
//	None
//
// Function Return Value
//	ER_INVALID_ID		- SystemCallID is invalid value
//	OK					- Successful SystemCall
// ************************************************************
long SystemCall(long SystemCallID)
{
	psr = OS_MODE;		// Set system mode to OS mode
	long status = OK;	//Sets the status to the accepted state based on the constant

	switch(SystemCallID)		//Switch statement to select the proper print output per given action
	{
		case 1:
			printf("Create Process System Call Not Implememented\n");
		break;

		case 2:
			printf("Delete Process System Call Not Implememented\n");
		break;

		case 3:
			printf("Process Inquiry System Call Not Implememented\n");
		break;

		case 4:
			status = memAllocSystemCall();
		break;

		case 5:
			status = memFreeSystemCall();
		break;

		case 6:
			printf("System Call Not Implememented\n");
		break;

		case 7:
			printf("System Call Not Implememented\n");
		break;

		case 8:
			status = io_getcSystemCall();
		break;

		case 9:
			status = io_putcSystemCall();
		break;

		case 10:
			printf("System Call Not Implememented\n");
		break;

		case 11:
			printf("System Call Not Implememented\n");
		break;

		case 12:
			printf("System Call Not Implememented\n");
		break;

		default:		// Default case per structure and Invalid SystemID case. 
			printf("ERROR: Invalid SystemCallID.\n");
			status = ER_INVALID_ID;
			break;
	}  // end of SystemCallID switch statement

	psr = USER_MODE;	// Set system mode to user mode

	return (status);
}  // end of SystemCall() function

// ************************************************************
// Function: CreateProcess
//
// Task Description:
//	Takes input parameter fileName, and priority. Loads file to count size of program,
//	then calls AllocateUserMem
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					- Successful SystemCall
// ************************************************************
long CreateProcess(char fileName[], long priority)	// or char * pointer
{
	//Local Variables
	long value = 0;		//Holds the error code for print statements
	long size = 22;		//Used to check for stack allocation space, 22 is the length of the current PCB. 
	long PCBptr;			//Points to the first address of the PCB being loaded
	long ptr;					//Default pointer used later for insertion

	// Allocate space for Process Control Block
	PCBptr = AllocateOSMemory(size);  // return value contains address or error
	if (PCBptr == ER_STACK_OVERFLOW)	// check for error
    {  // OS memory allocation failed
			FreeOSMemory(UserFreeList,  size);	//calls FreeOSMemory using the start of the MemAddr to be liberated as well as the size of the block
			return(PCBptr);  // return error code
    }
		else if(PCBptr == ER_INVALID_SIZE)	//Checks to see if the PCBptr is of an invalid size
		{
			printf("Invalid size. Fix and try again.");		//throw error code
			return(PCBptr);		//returns the pointer of the head to the main
		}

    // Initialize PCB: Set nextPCBlink to end of list, default priority, Ready state, and PID
	InitializePCB(PCBptr);
	printf("Came out of Initialize");
    // Load the program
	value = AbsoluteLoader(fileName);	//Calls AbsoluteLoader() function with fileName
	printf("Came out of Absolute Loader");
	if (value < 0)	//Checks for error code from Absolute Loader
	{
		printf("Absolute loader failed, error code: %ld\n", value);	//Prints the error code that AbsoluteLoader returns with
		return(ER_ABS_LOAD_FAIL);	//Returns from main with error code
	}
	else
	{
		/* if successful, next address for absolute loader */
		nextAddress = Progs[progsIndex].EndAddress+1;		//Uses the Progs array to get the next program address
		progsIndex++;
	}
	
	MemArray[PCBptr + PC] = value;

  if (gpr[2] > 0)											//Checks if register 2 contains data to create stack space, if it doesnt no stack space is created  
		ptr = AllocateUserMemory(size);		// Allocate stack space from user free list
	
	if (ptr == ER_STACK_OVERFLOW)			// check for error
	{  																// User memory allocation failed
		FreeUserMemory(UserFreeList,  size);	//Calls FreeUserMemory with the size of the process
		return(ER_INVALID_RANGE);  // return error code
	}
	else if(ptr == ER_INVALID_SIZE)		//checks ptr against the allowed size of the memory
	{
		printf("Invalid Memory Size");		//Throws error to user
		return(ER_INVALID_RANGE);	 
	}

	// Store stack information in the PCB, SP, ptr, and size
	MemArray[PCBptr + SP] = ptr + size;		// empty stack is high address, full is low address
	MemArray[PCBptr + STACK_START_ADDRESS] = ptr;		//Sets ptr equal to the start of the useable memory array
	MemArray[PCBptr + STACK_SIZE] = size;		//Set value of size equal to the start of the useable mem array
	MemArray[PCBptr + PRIORITY] = priority;	// Set priority

	PrintPCB(PCBptr);		//Prints out the current pointer head

	//Insert PCB into Ready Queue according to the scheduling algorithm
	InsertIntoRQ(PCBptr);	//Calls InsertIntoRQ with the current PCBptr pointing to the start of the PCB addr

	

	return(OK);
}  // end of CreateProcess() function

// ************************************************************
// Function: AllocateOSMemory
//
// Task Description:
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					- Successful SystemCall
// ************************************************************
long AllocateOSMemory(long size)  // return value contains address or error
{
	//Local Variables
	long CurrentPtr;
	long PreviousPtr;

	// Allocate memory from OS free space, which is organized as link
  if(OSFreeList == END_OF_LIST)
    {
		printf("Error, no free OS memory.\n");
		return(ER_STACK_OVERFLOW);   // ErrorNoFreeMemory is constant set to < 0
    }
	if(size < 0)
    {
		printf("Error, invalid size.\n");
		return(ER_INVALID_SIZE);  // ErrorInvalidMemorySize is constant < 0 **FIX EROOR CODE**
    }
  if(size == 1)
		size = 2;  // Minimum allocated memory is 2 locations

	CurrentPtr = OSFreeList;
  PreviousPtr = END_OF_LIST;
  while (CurrentPtr != END_OF_LIST)
  {// Check each block in the link list until block with requested memory size is found
		if(MemArray[CurrentPtr + 1] == size)
		{  // Found block with requested size.  Adjust pointers
			if(CurrentPtr == OSFreeList)  // first block
			{
				OSFreeList = MemArray[CurrentPtr];  // first entry is pointer to next block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  // not first block
			{
				MemArray[PreviousPtr] = MemArray[CurrentPtr];  //Point to next block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);    // return memory address
			}
		}
		else if(MemArray[CurrentPtr + 1] > size)
		{  // Found block with size greater than requested size
			if(CurrentPtr == OSFreeList)  // first block
			{
				MemArray[CurrentPtr + size]	= MemArray[CurrentPtr];
				MemArray[CurrentPtr + size + 1] = MemArray[CurrentPtr + 1] - size;
				OSFreeList = CurrentPtr + size;  // address of reduced block
				MemArray[CurrentPtr] = OSFreeList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  //Not the first block
			{
				MemArray[CurrentPtr + size]	= MemArray[CurrentPtr];
				MemArray[CurrentPtr + size + 1] = MemArray[CurrentPtr + 1] - size;
				MemArray[PreviousPtr] = CurrentPtr + size;  // address of reduced block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
		}
		else  // small block
		{  // look at next block
			PreviousPtr = CurrentPtr;
			CurrentPtr = MemArray[CurrentPtr];
		}
	} // end of while CurrentPtr loop
	return CurrentPtr;
}  // end of AllocateOSMemory() function

// ************************************************************
// Function: InitializePCB
//
// Task Description:
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	None
// ************************************************************
void InitializePCB(long PCBptr)
{
	for(int i = PCBptr; i < PCBptr+22 ; i++)
	{
		MemArray[i] = 0;
	}

	Progs[progsIndex].pid = ProcessID;
	MemArray[PCBptr + PID] = ProcessID++;  // ProcessID is global variable initialized to 1
	MemArray[PCBptr + PRIORITY]	= DEF_PRIORITY;  // DefaultPriority is a constant set to 128
	MemArray[PCBptr + STATE] = READY_STATE;    // ReadyState is a constant set to 1
	MemArray[PCBptr + NEXT_PCB_POINTER] = OSFreeList;
	MemArray[PCBptr + PSR] = psr;
}

// ************************************************************
// Function: AllocateUserMemory
//
// Task Description:
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					- Successful SystemCall
// ************************************************************
long AllocateUserMemory(long size)  // return value contains address or error
{
	//Local Variables
	long CurrentPtr;
	long PreviousPtr;

	// Allocate memory from OS free space, which is organized as link
	if(UserFreeList == END_OF_LIST)
    {
			printf("Error, no free OS memory.\n");
			return(ER_STACK_OVERFLOW);   // ErrorNoFreeMemory is constant set to < 0
    }
	if(size < 0)
    {
			printf("Error, invalid size.\n");
			return(ER_INVALID_SIZE);  // ErrorInvalidMemorySize is constant < 0 **FIX EROOR CODE**
    }
	if(size == 1)
		size = 2;  // Minimum allocated memory is 2 locations

	CurrentPtr = UserFreeList;
  PreviousPtr = END_OF_LIST;
  while (CurrentPtr != END_OF_LIST)
  {
		// Check each block in the link list until block with requested memory size is found
		if(MemArray[CurrentPtr + 1] == size)
		{  // Found block with requested size.  Adjust pointers
			if(CurrentPtr == UserFreeList)  // first block
			{
				UserFreeList = MemArray[CurrentPtr];  // first entry is pointer to next block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  // not first block
			{
				MemArray[PreviousPtr] = MemArray[CurrentPtr];  //Point to next block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);    // return memory address
			}
		}
		else if(MemArray[CurrentPtr + 1] > size)
		{  // Found block with size greater than requested size
			if(CurrentPtr == UserFreeList)  // first block
			{
				MemArray[CurrentPtr + size]	= MemArray[CurrentPtr];
				MemArray[CurrentPtr + size + 1] = MemArray[CurrentPtr + 1] - size;
				UserFreeList = CurrentPtr + size;  // address of reduced block
				MemArray[CurrentPtr] = UserFreeList;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
			else  //Not the first block
			{
				MemArray[CurrentPtr + size]	= MemArray[CurrentPtr];
				MemArray[CurrentPtr + size + 1] = MemArray[CurrentPtr + 1] - size;
				MemArray[PreviousPtr] = CurrentPtr + size;  // address of reduced block
				MemArray[CurrentPtr] = END_OF_LIST;  // reset next pointer in the allocated block
				return(CurrentPtr);	// return memory address
			}
		}
		else  // small block
		{  // look at next block
			PreviousPtr = CurrentPtr;
			CurrentPtr = MemArray[CurrentPtr];
		}
	} // end of while CurrentPtr loop
	return CurrentPtr;
}  // end of AllocateUserMemory() function

// ************************************************************
// Function: FreeOSMemory
//
// Task Description:
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					- Successful SystemCall
// ************************************************************
long FreeOSMemory(long ptr, long size)  // return value contains OK or error code
{
	if(ptr < 7000 || ptr > 9999)  	// Address range is given in the class
	{
		printf("Error, Invalid address");
		return(ER_INVALID_ADDRESS);  // ErrorInvalidMemoryAddress is constantset to  < 0
	}

	if(size == 1)	// check for minimum allocated size, which is 2 even if user asks for 1 location
	{
		size = 2;  // minimum allocated size
	}
	else if(size < 1 || (ptr+size) >= 9999)
	{	// Invalid size
		printf("Error, invalid size.");
		return(ER_INVALID_SIZE);  	// All error codes are < 0
	}

	for(int i = ptr; i < ptr+size; i++)
	{
		MemArray[i] = 0;
	}

	MemArray[ptr] = -1;
	MemArray[ptr+1] = size;
	OSFreeList = ptr;

	return (OK);
}  // end of FreeOSMemory() function

// ************************************************************
// Function: FreeUserMemory
//
// Task Description:
//
// Input Parameters
//
// Output Parameters
//	None
//
// Function Return Value
//	OK					- Successful SystemCall
// ************************************************************
long FreeUserMemory(long ptr, long size)  // return value contains OK or error code
{
	if(ptr < 3000 || ptr > 6999)  	// Address range is given in the class
	{
		printf("Error, Invalid address");
		return(ER_INVALID_ADDRESS);  // ErrorInvalidMemoryAddress is constantset to  < 0
	}

	if(size == 1)	// check for minimum allocated size, which is 2 even if user asks for 1 location
	{
		size = 2;  // minimum allocated size
	}
	else if(size < 1 || (ptr+size) >= 9999)
	{	// Invalid size
		printf("Error, invalid size.");
		return(ER_INVALID_SIZE);  	// All error codes are < 0
	}

	for(int i = ptr; i < ptr+size; i++)
	{
		MemArray[i] = 0;
	}

	MemArray[ptr] = -1;
	MemArray[ptr+1] = size;
	UserFreeList = ptr;

	return (OK);
}  // end of FreeUserMemory() function

// ************************************************************
// Function: PrintPCB
//
// Task Description:
//
// Input Parameters
//	long PCBptr
//
// Output Parameters
//	None
//
// Function Return Value
//	None
// ************************************************************
void PrintPCB(long PCBptr)
{
	//Prints the values of the following fields from PCB with a text before the value
	printf("PCB address = %ld\n", PCBptr);
	printf("Next PCB Ptr = %ld\n", MemArray[PCBptr + NEXT_PCB_POINTER]);
	printf("PID = %ld\n", MemArray[PCBptr + PID]);
	printf("State = %ld\n", MemArray[PCBptr + STATE]);
	printf("PC = %ld\n", MemArray[PCBptr + PC]);
	printf("SP = %ld\n", MemArray[PCBptr + SP]);
	printf("Priority = %ld\n", MemArray[PCBptr + PRIORITY]);
	printf("Stack info:\n");
	printf("Start address = %ld\n", MemArray[PCBptr + STACK_START_ADDRESS]);
	printf("Size = %ld\n", MemArray[PCBptr + STACK_SIZE]);
	printf("GPRs : \n");

	for(int i = 0; i < 8; i++)
	{
		printf("GPR[%d]: %ld\n", i, MemArray[PCBptr + i + GPR0]);
	}
}  // end of PrintPCB() function

// ************************************************************
// Function: InsertIntoRQ
//
// Task Description:
//
// Input Parameters
//	long PCBptr
//
// Output Parameters
//	None
//
// Function Return Value
//	OK		- Success code
// ************************************************************
long InsertIntoRQ (long PCBptr) //Need to modify based on changes to local variables
{
	// Insert PCB according to Priority Round Robin algorithm
	// Use priority in the PCB to find the correct place to insert.
	long previousPtr = END_OF_LIST;
	long currentPtr = RQ;
	struct ReadyNode *nextNode = (struct nextNode*)malloc(sizeof(struct ReadyNode));
	struct ReadyNode * temp;
	struct ReadyNode *current = rhead;
	temp = NULL;

	if(PCBptr < 0 || PCBptr > 9999)
	{
		printf("Error: Invalid PCB pointer.\n");
		return(ER_INVALID_ADDRESS);
	}

	MemArray[PCBptr + STATE] = READY_STATE;
	MemArray[PCBptr + NEXT_PCB_POINTER] = END_OF_LIST;

	if(RQ == END_OF_LIST)
	{
		printf("This list is empty\n");
		RQ = PCBptr;
		nextNode -> readyProcess = RQ;
		nextNode -> next = rhead;
		rhead =  nextNode;
		return(OK);
	}

	while(current != NULL)
	{
		if(MemArray[PCBptr + PRIORITY] > MemArray[currentPtr + PRIORITY])
		{
			if(temp == NULL) //Insert node as new head
			{
				printf("Putting node as new head\n");
				MemArray[PCBptr + NEXT_PCB_POINTER] = RQ;
				RQ = PCBptr;
				nextNode -> readyProcess = RQ;
				nextNode -> next = rhead;
				rhead = nextNode;
				return(OK);
			}
			//Otherwise put the new node between two nodes
			MemArray[PCBptr + NEXT_PCB_POINTER] = MemArray[previousPtr + NEXT_PCB_POINTER];
			temp -> next = nextNode;
			nextNode -> next = current;
			nextNode -> readyProcess = RQ;
			return(OK);

		}

		else //Move up the list
		{
			temp = current;
			current = current -> next;
			previousPtr = currentPtr;
			currentPtr = MemArray[currentPtr + NEXT_PCB_POINTER];
		}
	}

	return(OK);
}

//************************************************************
//Author: Brian Tejada
//
// Function: InsertIntoWQ
//
// Task Description:
//
// Input Parameters
//	long PCBptr
//
// Output Parameters
//	None		
//
// Function Return Value
//	OK		- Success code				
// ************************************************************
long InsertIntoWQ (long PCBptr)
{
	//check for invalid PCB memory address
	if((PCBptr < 0) || (PCBptr > 10000)) //10000 = Max Memory Address
	{
		printf("Error, invalid address");
		return(ER_INVALID_ADDRESS);  // Error code < 0
	}

	MemArray[PCBptr + STATE] = WAITING_STATE;		// set state to ready state
	MemArray[PCBptr + NEXT_PCB_POINTER] = WQ;	 // set next pointer to end of list

	WQ = PCBptr;
	struct WaitingNode *nextNode = (struct nextNode*)malloc(sizeof(struct WaitingNode));
	nextNode -> waitProcess = WQ;
	nextNode -> next = whead;
	whead =  nextNode;
	printWaitingQueue(whead);

	return(OK);
}//end of InsertIntoWQ() function

// ************************************************************
// Author: Brian Tejada
//
// Function: CheckAndProcessInterrupt
//
// Task Description:
//	read interrupt ID and service the interupt
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void CheckAndProcessInterrupt()
{
	//local variables
	int InterrruptID;
	long PCBptr = 

	//prompt and read interrupt ID
	printf("Select an Interrupt:\n\t0 - No Interrupt\n\t1 - Run Program\n\t2 - ShutDown System\n\t");
	printf("3 - Input Operation Completion(io_getc)\n\t4 - Output Operation Completion(io_putc)\n");
	printf("Selection: ");
	//read user input to determine interrupt
	scanf("%d", &InterrruptID);
	//display selected interrupt
	printf("You selected Interrupt %d\n", InterrruptID);
	//process interrrupt
	switch(InterrruptID)
	{
		//no interrupt
		case 0:
			printf("No interrupt selected");
			break;
		//run program
		case 1:
			//call ISR run Program Interrupt function
			ISRrunProgramInterrupt();
			printf("Came out of runProgramInterrupt\n");
			break;
		// Shutdown system
		case 2:
			//call ISR shutdown System Interrupt function
			ISRshutdownSystem();
			//set system shutdown status to off value
			SystemShutdownStatus = 0;
			break;
		// Input operation completion â€“ io_getc
		case 3:
			//call ISR input Completion Interrupt function
			ISRinputCompletionInterrupt();
			break;
		// Output operation completion â€“ io_put
		case 4:
			//call ISR output Completion Interrupt function
			ISRoutputCompletionInterrupt();
			break;
		// Invalid Interrupt ID
		default:
			//Display invalid interrupt ID message;
			printf("Invaild Interrupt ID");
			break;
	}//end of switch InterruptID
}//end of CheckAndProcessInterrupt() function

// ************************************************************
// Author: Brian Tejada
//
// Function: ISRrunProgramInterrupt
//
// Task Description:
//	read filename and creat process
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void ISRrunProgramInterrupt()
{
	//local variables
	char fileName[25]; //Test Program Executable Filename
	//promt use to input a filename
	printf("Input filename to execute: \n");
	//scanf() reads user input into filename, unless error
	if(scanf("%s", fileName) < 0)
	{
		printf("Error reading user input, please close and try again.\n");
		return;
	}
	//Call Create Process passing filename and Default Priority as arguments
	CreateProcess(fileName, DEF_PRIORITY);
	printf("Came out of CreateProcess\n");
	//priority = priority - 1;
	return;

}//end of ISRrunProgramInterrupt() function

// ************************************************************
// Function: ISRshutdownSystem
//
// Modified by Cam Turner
// Task Description:
//	terminate all processes in RQ and WQ from the program
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void ISRshutdownSystem()
{

	//local variables
	long ptr;
	struct ReadyNode *rpointer;
	struct WaitingNode *wpointer;
	rpointer = rhead;
	wpointer = whead;
	//set ptr to first PCB pointed by RQ;
	ptr = RQ;
	//terminate all processes in RQ one by one
	while(ptr != END_OF_LIST)
	{
		rpointer = rpointer -> next;
		if(rpointer == NULL)
		{
			break;
		}
		RQ = rpointer -> readyProcess;
		terminateProcess(ptr); //terminates next process in ready queue
		ptr = RQ;
	}
	//set ptr to first PCB pointed by WQ;
	ptr = WQ;
	while(ptr != END_OF_LIST)
	{
		wpointer = wpointer -> next;
		if(wpointer == NULL)
		{
			break;
		}
		WQ = wpointer -> waitProcess;
		terminateProcess(ptr); //terminates next process in waitng queue
		ptr = WQ;
	}
	return;
}//end of ISRshutdownSystem() function

// ************************************************************
// Function: ISRinputCompletionInterrupt
//
// Author: Cam Turner
// Task Description:
//	read PID of the process completing the io_getc operation
//	read one character from keyboard (input device)
//	store charcter in the GPR of the PCB for the process
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void ISRinputCompletionInterrupt()
{
	//local variables
	int input_PID;
	int foundInWait = 0;
	int foundInReady = 0;
	struct WaitingNode *waitPrev;
	struct ReadyNode *readyPrev;
	struct WaitingNode *waitCurrent;
	struct ReadyNode *readyCurrent;
	long pointer;
	long stackPtr;
	long temp;
	char val;
	long Value;
	long countDown = gpr[3];
	long countUp = 2;
	

	waitPrev = NULL;
	readyPrev = NULL;
	printf("what is whead? %ld", *whead);
	waitCurrent = whead;
	readyCurrent = rhead;
	//Prompt and read PID of the process completing input completion interrupt;
	printf("Enter PID of Process Completing Input Completion Interrupt: ");
	scanf("%d", &input_PID);
	//checks if PID is valid
	if(input_PID < 0)
	{
		printf("Error reading user input, please close and try again.\n");
		return;
	}
	
	while(waitCurrent != NULL)
	{
		if(MemArray[(waitCurrent -> waitProcess) + PID] == input_PID)
		{
			pointer = waitCurrent -> waitProcess;
			while (countDown > 0)
			{
				stackPtr = gpr[1] + countUp;
				if(waitPrev == NULL)
				{
					foundInWait = 1;
					printf("here\n");
					scanf(" %c", &val);
					Value = (long)val;
					//buffer[0] = NULL;
					MemArray[stackPtr] = Value;
					printf("THIS ya Pointer sun: %ld\n", pointer);
					printf("THAT VALUE IS: %ld\n", (pointer + GPR1 + 2));
					DumpMemory("After executing user program", 0, 199);
					DumpMemory("After executing user program", 3000, 100);
					countDown--;
					countUp++;
				}
				else
				{
					foundInWait = 1;
					printf("2Input a single number: ");
					scanf("%ld", &val);
					Value = val;
					MemArray[stackPtr] = Value;
					MemArray[pointer + STATE] = READY_STATE;
					countDown--;
					countUp++;
				}
			}
			if(countDown == 0)
			{
				MemArray[pointer + STATE] = READY_STATE;
				InsertIntoRQ(pointer);
				break;
			}
		}
		waitPrev = waitCurrent;
		waitCurrent = waitCurrent -> next;
	}

	if(foundInWait == 0)
	{
		while(readyCurrent != NULL)
		{
			if(MemArray[(readyCurrent -> readyProcess) + PID] == input_PID)
			{
				pointer = MemArray[readyCurrent -> readyProcess];
				if(readyPrev == NULL)
				{
					foundInReady = 1;
					printf("Input a single number: ");
					scanf("%ld", &val);
					Value = val;
					MemArray[pointer + GPR1 + 2] = Value;
					break;
				}
			}
			readyPrev = readyCurrent;
			readyCurrent = readyCurrent -> next;
		}
	}

	if(foundInWait == 0 && foundInReady == 0)
	{
		printf("PID not found.\n");
	}
	return;

}  // end of ISRinputCompletionInterrupt() function

// ************************************************************
// Author: Brian Tejada
//
// Function: ISRoutputCompletionInterrupt
//
// Task Description:
//	read PID of the process completing the io_putc operation
//	display one character on the monitor (output device)
//	from the GPR of the PCB for the process
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void ISRoutputCompletionInterrupt()
{
	//local variables
	int input_PID;
	long currentProccess;
	int foundInWait = 0;
	int foundInReady = 0;
	struct WaitingNode *waitCheckNode = whead;
	struct WaitingNode *waitPrevNode = NULL;
	struct ReadyNode *readyCheckNode = rhead;
	struct ReadyNode *readyPrevNode = NULL;
	long stackPtr;
	long countUp = 2;
	long countDown = gpr[3];


	//Prompt and read PID of the process completing output completion interrupt;
	printf("Enter PID of Process Completing Output Completion Interrupt: ");
	scanf("%d", &input_PID);
	printf("You Selected PID: %d\n", input_PID);
	//checks if PID is valid
	if(input_PID < 0)
	{
		printf("Error reading user input, please close and try again.\n");
		return;
	}

	//searching Waiting Node for Given PID
	while(waitCheckNode != NULL)
	{
		printf("Checking WQ, %ld\n", CurrProg.pid);
		stackPtr = gpr[1] + countUp;
		currentProccess = waitCheckNode -> waitProcess;
		if(CurrProg.pid == input_PID) //Checks if the pid is in the current node
		{
			printf("\nChecking WQ 2 with PID == %ld\n", CurrProg.pid);
			if (waitCheckNode == whead)
			{
				printf("\nChecking WQ 3\n");
				foundInWait = 1;
				while (countDown > 0)
				{
					printf("%ld", MemArray[stackPtr + countUp]);
					countUp++;
					countDown--;
				}
				searchAndRemovePCBFromWQ(CurrProg.pid);
				MemArray[currentProccess + STATE] = READY_STATE;
				printf("\nCurrent Process: %ld\n", currentProccess);
				InsertIntoRQ(currentProccess);
				printf("Successful insert\n");
				countUp++;
				countDown--;
			}
			else
			{
				printf("\nChecking WQ 4\n");
				waitPrevNode -> next = waitCheckNode -> next;
				free(waitCheckNode);
				foundInWait = 1;
				printf("Number in GPR[0]: %ld\n", MemArray[currentProccess + GPR0]);
				MemArray[currentProccess + STATE] = READY_STATE;
				InsertIntoRQ(currentProccess);
				countUp++;
				countDown--;
			}
		}
		waitPrevNode = waitCheckNode;
		waitCheckNode = waitCheckNode -> next;
	}//end waiting node while loop
	//searching Ready Node for Given PID
	while(waitCheckNode == NULL && readyCheckNode != NULL)
	{
		printf("\nChecking RQ\n");
		currentProccess = readyCheckNode -> readyProcess;
		if(MemArray[currentProccess + PID] == input_PID) //Checks if the pid is in the current node
		{
			if (readyPrevNode == NULL)
			{
					printf("Found in head\n");
					foundInReady = 1;
					printf("Number in GPR[0]: %ld\n", MemArray[currentProccess + GPR0]);
					break;
			}
			else
			{
				printf("Found in middle\n");
				foundInReady = 1;
				printf("Number in GPR[0]: %ld\n", MemArray[currentProccess + GPR0]);
				break;
			}
		}
		readyCheckNode = readyCheckNode -> next;
	}//end ready node while loop

	if(foundInWait == 0 && foundInReady == 0)
	{
		printf("Error, invalid PID\n");
	}
	return;
}  // end of ISRoutputCompletionInterrupt() function

// ************************************************************
// Function: TerminateProcess
//		recover all resources allocated to the process
//
// Task Description:
//	terminate a given process
//
// Input Parameters
//	None
//
// Output Parameters
//	None
// ************************************************************
void terminateProcess(long PCBptr)
{
	struct ReadyNode *readyNextNode;
	struct ReadyNode *readyCurrentNode;
	struct WaitingNode *waitNextNode;
	struct WaitingNode *waitCurrentNode;
	long check = 0;
	
	for(long i = MemArray[PCBptr + STACK_START_ADDRESS]; i < MemArray[PCBptr + STACK_SIZE]; i++)
	{
		MemArray[i] = 0; //Starts at where the stack starts and ends at the size, returning memory used by the stack
	}
	
	for(int j = 0; j <= 22; j++)
	{
		MemArray[PCBptr + j] = 0; //Returns PCB memory
	}

}  // end of TerminateProcess function()

long memAllocSystemCall()
{
	long size = gpr[2];

	if(size < 0)
	{
		printf("Error, invalid size.\n");
		return(ER_INVALID_SIZE);  // ErrorInvalidMemorySize is constant < 0 **FIX EROOR CODE**
	}
	if(size == 1)
	{
		size = 2;
	}

	gpr[1] = AllocateUserMemory(size);
	
	if(gpr[1] < 0)
	{
		gpr[0] = gpr[1];
	}
	else
	{
		gpr[0] = OK;
	}

	printf("memAllocSystemCall has been called, displaying GPR 0 - 2: %ld, %ld, %ld\n", gpr[0], gpr[1], gpr[2]);
	return gpr[0];
}

long memFreeSystemCall()
{
	long size = gpr[2];
	if(size < 0)
	{
		printf("Error, invalid size.\n");
		return(ER_INVALID_SIZE);  // ErrorInvalidMemorySize is constant < 0 **FIX EROOR CODE**
	}
	if(size == 1)
	{
		size = 2;
	}

	gpr[0] = FreeUserMemory(gpr[1], size); //****Needs to pass gpr[1] as argument too****

	printf("memFreeSystemCall has been called, displaying GPR 0 - 2: %ld, %ld, %ld,", gpr[0], gpr[1], gpr[2]);
	return gpr[0];
}
	//brian Creations
long io_getcSystemCall ()
{
	//Return start of input operation event code;
	return Input_Operation_Event_Code;
} // end of io_getc system call

//brian Creations
long io_putcSystemCall ()
{
	//Return start of output operation event code;
	return Output_Operation_Event_Code;
}  // end of io_putc system call

//cams child
long searchAndRemovePCBFromWQ(long pid)
{
	struct WaitingNode *prev;
	struct WaitingNode *current;
	long found = 0;
	long pointer;
	prev = NULL;
	current = whead;
	while(current != NULL)
	{
		if(MemArray[(current -> waitProcess) + PID] == pid)
		{
			if(prev == NULL)
			{
				printf("FOund PID in head\n");
				pointer = current -> waitProcess;
				found = 1;
				whead = current -> next;
				free(current);
				break;
			}
			else
			{
				printf("FOund PID in elsewhere\n");
				pointer = current -> waitProcess;
				found = 1;
				prev -> next = current -> next;
				free(current);
				break;
			}
			return(pointer);
		}
		prev = current;
		current = current -> next;
	}

	if(found == 0)
	{
		printf("Invalid PID. \n");
		return(END_OF_LIST);
	}
	return(OK);
}

void Dispatcher(long PCBPtr)
{
	gpr[0] = MemArray[PCBPtr + GPR0];
	gpr[1] = MemArray[PCBPtr + GPR1];
	gpr[2] = MemArray[PCBPtr + GPR2];
	gpr[3] = MemArray[PCBPtr + GPR3];
	gpr[4] = MemArray[PCBPtr + GPR4];
	gpr[5] = MemArray[PCBPtr + GPR5];
	gpr[6] = MemArray[PCBPtr + GPR6];
	gpr[7] = MemArray[PCBPtr + GPR7];

	sp = MemArray[PCBPtr + SP];
	printf("BITCH HERE: %ld\n", MemArray[PCBPtr + PC]);
	pc = MemArray[PCBPtr + PC];

	psr = USER_MODE;
	return;
}

void saveContext(long PCBPtr)
{
	MemArray[PCBPtr + GPR0] = gpr[0];
	MemArray[PCBPtr + GPR1] = gpr[1];
	MemArray[PCBPtr + GPR2] = gpr[2];
	MemArray[PCBPtr + GPR3] = gpr[3];
	MemArray[PCBPtr + GPR4] = gpr[4];
	MemArray[PCBPtr + GPR5] = gpr[5];
	MemArray[PCBPtr + GPR6] = gpr[6];
	MemArray[PCBPtr + GPR7] = gpr[7];

printf("BITCH HERE2: %ld\n", pc);
	MemArray[PCBPtr + SP] = sp;
	MemArray[PCBPtr + PC] = pc;
	printf("BITCH HERE3: %ld\n", MemArray[PCBPtr + PC]);

	return;
}

//brian
long printWaitingQueue(struct WaitingNode *n)
{
	printf("Contents of Waiting Queue\n");
  while (n != NULL)
  {
     printf(" Wait Process: %ld\n", n -> waitProcess);
     n = n -> next;
  }
	return(OK);
}//end printWaitingQueue()

long printReadyQueue(struct ReadyNode *n)
{
	printf("Contents of Ready Queue\n");
  while (n != NULL)
  {
     printf("Ready Process: %ld\n", n -> readyProcess);
     n = n -> next;
  }
	return(OK);
}//end printReadyQueue()

//brian did it
long SelectProcessFromRQ()
{
	printf("Enter RQ Selcetet\n");
	long PCBptr = RQ;
	struct ReadyNode *currenthead = rhead;
	printf("RQ:%ld\n", RQ);
	if(RQ != NULL)
	{
		printf("HEAD NULL\n");
		// Remove first PCB from RQ
		printf("ERROR 1\n");
		PCBptr = currenthead -> readyProcess;
		printf("ERROR 2\n");
		rhead = currenthead -> next;
		printf("ERROR 3\n");
		free(currenthead);
	}
	printf("New Point NULL\n");
	// Set next point to EOL in the PCB
	MemArray[PCBptr + NEXT_PCB_POINTER] = END_OF_LIST;
	return(PCBptr);
}//end of SelectProcessFromRQ()

int fetchProgInfo(long RunningPCBptr)
{
	long currentPID = MemArray[RunningPCBptr + PID];
	int i = 0;
	
	while (i >= 0)
	{
		if(Progs[i].pid == currentPID)
		{
			CurrProg.StartAddress = Progs[i].StartAddress;
			CurrProg.EndAddress = Progs[i].EndAddress;
			CurrProg.size = Progs[i].size;
			CurrProg.pid = Progs[i].pid;
			return OK;
		}
		else if(i > 9)
		{
			printf("Invalid PID, process does not exist");
			return ER_INVALID_PID;
		}
		else
		{
			i++;
		}
	}
	
}	
	