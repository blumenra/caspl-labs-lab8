#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define OFF 0
#define ON 1

void flushStdin();
void printFuncs();
// void printGlobalVariables();
int getFuncRequest();
void initializeBuffer();
void printByte(char byte);
void convertNibblesToStr(char nibbles[], char buf[]);
void printSizes(int amount, int tableOff, int fieldOff, int recordSize);
int getLongestStrLen(int numOfSh, int shNamesTableOff, int shTableOff);
void printSpaces(int num);
int extractField(int fieldOffset, int fieldSize);
void printSynTab(int currSymTabIndex, int currStrTabOff);

void togDebug();
void exemElfFile();
void setFileName();
void printSectionNames();
void printSymbols();
// void setUnitSize();
// void displayFile();
// void loadToMem();
// void saveToFile();
// void modifyFile();
void quit();

struct func{
  char *name;
  void (*fun)(void);
};

int size = 0;
struct stat mystat;
char filename[100];
int currentfd = -1;
int map_start;
char *addr;
char fieldBuffer[] = {0, 0, 0, 0};
char buf[9];
int debug = OFF;

int main(int argc, char** argv){

	// initializeBuffer(filename, 100);

	struct func funcs[] = {
							{"Toggle Debug Mode", togDebug},
							{"Examine ELF File", exemElfFile},
							{"Print Section Names", printSectionNames},
							{"Print Symbols", printSymbols},
							// {"Set File Name", setFileName},
							// {"Set Unit Size", setUnitSize},
							// {"File Display", displayFile},
							// {"Load Into Memory", loadToMem},
							// {"Save Into File", saveToFile},
							// {"File Modify", modifyFile},
							{"Quit", quit},
							{NULL, NULL}
						};
	
	while(1){

		// if(debug){
		// 	printGlobalVariables();
		// }

		printf("Choose action:\n");
		printFuncs(funcs);
		
		int funcIndex = getFuncRequest();

		if(funcIndex < 0){

			printf("Invalid input! Please try again\n");
		}
		else{

			funcs[funcIndex].fun();
		}
	}

	return 0;
}

void initializeBuffer(char buf[], int length){

	int i;
	for(i=0; i < length; i++){
		buf[i] = 0;
	}
}

/*
void printGlobalVariables(){

	printf("Global variables:\n");
	printf("\tUnit size: %d\n", size);
	printf("\tFile name: %s\n", filename);
	printf("\tBuffer address: %p\n", data_pointer);
}
*/

void printFuncs(struct func funcs[]){

	int i=0;
	while(funcs[i].name != NULL){

		printf("%d-%s\n", i, funcs[i].name);
		i++;
	}
}

int getFuncRequest(){
	char buf[3];
	fgets(buf, 3, stdin);
	
	int funcIndex;
	sscanf(buf, "%d",&funcIndex);

	if((funcIndex < 0) || (funcIndex >= 5)){
		funcIndex = -1;
	}

	return funcIndex;
}

void flushStdin(){

	int a;
	while ((a = getchar()) != '\n' && a != EOF);
}

void printByte(char byte){

	unsigned char rightNibble = byte;
	unsigned char leftNibble = byte;
	
	rightNibble = (rightNibble & 0xf);
	leftNibble = (leftNibble >> 4);

	printf("%x", leftNibble);
	printf("%x", rightNibble);
}

void togDebug(){


	if(debug){

		printf("Debug flag now off\n");
		debug = OFF;
	}
	else{
		
		debug = ON;
		printf("Debug flag now on\n");
	}
}

void exemElfFile(){

	int numOfSh = 0;
	int numOfPh = 0;
	int shTableOff = 0;
	int phTableOff = 0;

	setFileName();

	if(currentfd != -1){
		close(currentfd);
	}

	currentfd = open(filename, O_RDONLY);

	if(currentfd == -1){
		perror("open");
		return;
	}

	if(fstat(currentfd, &mystat) < 0){
		perror("fstat");
		currentfd = -1;
		close(currentfd);
		return;
	}

	addr = mmap(NULL, mystat.st_size, PROT_READ, MAP_PRIVATE, currentfd, 0);

	if(addr ==  MAP_FAILED){
		perror("mmap");
		currentfd = -1;
		close(currentfd);
		return;
	}

	if(strncmp(addr+1, "ELF", 3) != 0){
		fprintf(stderr, "This is not an ELF file!\n");
		currentfd = -1;
		close(currentfd);
		munmap(addr, mystat.st_size);
		return;
	}

	printf("The first 3 bytes of the magic number of %s: ", filename);
	fflush(stdout);
	write(1, addr+1, 3);
	printf("\n");


	printf("The encoding scheme is: ");
	fflush(stdout);
	
	if(addr[5] == 1){

		printf("Little endian\n");
	}
	else if(addr[5] == 2){
		
		printf("Big endian\n");
	}
	else{
		fprintf(stderr, "Nither little nor big endiannnnn\n");

	}

	printf("Entry point of %s: ", filename);
	fflush(stdout);
	strncpy(fieldBuffer, addr+24, 4);
	printf("0x");
	printf("%x", (unsigned char) fieldBuffer[3]);
	int i;
	for(i=2; i >= 0; i--){
		
		printf("%02x", (unsigned char) fieldBuffer[i]);
		
	}
	printf("\n");
	
	printf("The file offset of the section header table of %s: ", filename);
	shTableOff = extractField(32, 4);
	printf("%d\n", shTableOff);


	printf("The number of section header entries of %s: ", filename);
	numOfSh = extractField(48, 2);
	printf("%d\n", numOfSh);



	printf("%s's section headers sizes: \n", filename);
	printSizes(numOfSh, shTableOff, 20, 40);

	
	printf("The file offset in which the program header table of %s resides: ", filename);
	phTableOff = extractField(28, 4);
	printf("%d\n", phTableOff);


	printf("The number of program header entries of %s: ", filename);
	numOfPh = extractField(44, 2);
	printf("%d\n", numOfPh);


	printf("%s's program headers sizes: \n", filename);
	printSizes(numOfPh, phTableOff, 16, 32);
}

int extractField(int fieldOffset, int fieldSize){
	
	fflush(stdout);
	initializeBuffer(buf, 9);
	initializeBuffer(fieldBuffer, 4);
	strncpy(fieldBuffer, addr+fieldOffset, fieldSize);
	convertNibblesToStr(fieldBuffer, buf);
	return strtol(buf, NULL, 16);
}

void printSectionNames(){
	
	if(currentfd == -1){

		fprintf(stderr, "File name is null!\n");
		return;
	}

	int shNamesRecordOff = 0;
	int shNamesTableOff = 0;
	int shTableOff = extractField(32, 4);
	int numOfSh = 0;

	shNamesRecordOff = extractField(50, 2);
	shNamesTableOff = extractField(shTableOff+(shNamesRecordOff*40)+16, 4);
	numOfSh = extractField(48, 2);

	if(debug){
		printf("shstrndx: %d\n", shNamesRecordOff);
	}

	int len = getLongestStrLen(numOfSh, shNamesTableOff, shTableOff);

	int l;
	int currShOff = 0;
	int shNameOff = 0;
	char* currShName;
	printf("Section Headers:\n");
	printf("  [Nr] Name ");
	printSpaces(len-strlen("Name"));
	if(debug){
		printf("shNameOff ");
	}
	printf("Addr     Off    Size   Type\n");
	for(l=0; l < numOfSh; l++){
		
		if(l < 10)
			printf("  [ %d] ", l);
		else
			printf("  [%d] ", l);

		shNameOff =  extractField(shTableOff+currShOff, 4);

		currShName = addr+shNamesTableOff+shNameOff;
	

		printf("%s ", currShName);

		printSpaces(len-strlen(currShName));
		
		if(debug){
			printf("%09x ", shNameOff);
		}

		initializeBuffer(fieldBuffer, 4);
		// fflush(stdout);
		strncpy(fieldBuffer, addr+shTableOff+currShOff+12, 4);
		int i;
		for(i=3; i >= 0; i--){
			
			printf("%02x", (unsigned char) fieldBuffer[i]);
			
		}

		printf(" ");
		printf("%06x ", extractField(shTableOff+currShOff+16, 4)); //sh_offset
		printf("%06x ", extractField(shTableOff+currShOff+20, 4)); //sh_size
		printf("%d ", (unsigned int) extractField(shTableOff+currShOff+4, 4)); //sh_type
		printf("\n");

		currShOff += 40;
	}
}

int getShIndex(char const str[]){

	int shTableOff = extractField(32, 4);
	int shNamesRecordOff = extractField(50, 2);
	int shNamesTableOff = extractField(shTableOff+(shNamesRecordOff*40)+16, 4);
	int numOfSh = extractField(48, 2);
	
	int currShOff = 0;
	int shNameOff = 0;
	char* currShName;
	int i;
	for(i=0; i < numOfSh; i++){

		shNameOff =  extractField(shTableOff+currShOff, 4);
		currShName = addr+shNamesTableOff+shNameOff;

		if(strcmp(str, currShName) == 0){
			return i;
		}

		currShOff += 40;
	}

	return -1;
}


void printSymbols(){
	
	if(currentfd == -1){

		fprintf(stderr, "File name is null!\n");
		return;
	}


	// int const shSize = 40;
	int dynsymIndex = getShIndex(".dynsym");
	int symtabIndex = getShIndex(".symtab");
	int strtabIndex = getShIndex(".strtab");
	if(debug){
		printf("dynsymIndex: %d\n", dynsymIndex);
		printf("symtabIndex: %d\n", symtabIndex);
		printf("strtabIndex: %d\n", strtabIndex);
	}
	int shTableOff = extractField(32, 4);
	int dynsymtabOff = extractField(shTableOff+(40*dynsymIndex)+16, 4);
	int symtabOff = extractField(shTableOff+(40*symtabIndex)+16, 4);
	// int strtabOff = extractField(shTableOff+(40*strtabIndex)+16, 4);

	// int shNamesRecordOff = extractField(50, 2);
	// int shNamesTableOff = extractField(shTableOff+(shNamesRecordOff*40)+16, 4);
	if(debug){
		printf("dynsymtab size: %d\n", extractField(shTableOff+(40*dynsymIndex)+20, 4));
		printf("dynsymtab entsize: %d\n", extractField(shTableOff+(40*dynsymIndex)+36, 4));
		printf("symtab size: %d\n", extractField(shTableOff+(40*symtabIndex)+20, 4));
		printf("symtab entsize: %d\n", extractField(shTableOff+(40*symtabIndex)+36, 4));
	}



	printSynTab(dynsymIndex, dynsymtabOff);
	printf("\n");
	printSynTab(symtabIndex, symtabOff);
	printf("\n");
}

void printSynTab(int currSymTabIndex, int currStrTabOff){
	
	int strtabIndex = getShIndex(".strtab");
	int shNamesRecordOff = extractField(50, 2);
	int shTableOff = extractField(32, 4);
	int strtabOff = extractField(shTableOff+(40*strtabIndex)+16, 4);
	int shNamesTableOff = extractField(shTableOff+(shNamesRecordOff*40)+16, 4);
	int numOfSh = extractField(48, 2);
	int symEntsize = extractField(shTableOff+(40*currSymTabIndex)+36, 4);
	int numOfsyms = (extractField(shTableOff+(40*currSymTabIndex)+20, 4))/symEntsize;
	int len = getLongestStrLen(numOfSh, shNamesTableOff, shTableOff);

	if(debug){
		int shNameOff =  extractField(shTableOff+(currSymTabIndex*40), 4);
		char* currShName = addr+shNamesTableOff+shNameOff;
		printf("Symbol table '%s' contains %d entries:\n", currShName,numOfsyms);
	}

	printf("   Num:    Value Ndx shName ");
	printSpaces(len-strlen("shName"));
	printf("Name\n");

	int currRecordOff = 0;
	int shNameOff = 0;
	int symNameOff = 0;
	int currShIndex = 0;
	char* currShName;
	char* currsymName;
	int l;
	for(l=0; l < numOfsyms; l++){
		
		if(l < 10)
			printf("     %d: ", l);
		else if(l < 100)
			printf("    %d: ", l);
		else
			printf("   %d: ", l);

		initializeBuffer(fieldBuffer, 4);
		strncpy(fieldBuffer, addr+currStrTabOff+currRecordOff+4, 4);
		int i;
		for(i=3; i >= 0; i--){
			
			printf("%02x", (unsigned char) fieldBuffer[i]);
			// puts("5");
			
		}
		printf(" ");
		
		currShIndex = extractField(currStrTabOff+currRecordOff+14, 2);
		if(currShIndex >= extractField(48, 2)){
			printf("--");
		}
		else{

			if(currShIndex < 10)
				printf("  %d ", currShIndex); //sh_offset
			else if(currShIndex < 100)
				printf(" %d ", currShIndex); //sh_offset
			else
				printf("%d ", currShIndex); //sh_offset

			shNameOff =  extractField(shTableOff+(currShIndex*40), 4);
			currShName = addr+shNamesTableOff+shNameOff;
			printf("%s ", currShName);
			printSpaces(len-strlen(currShName));
			
			// if(debug){
			// 	printf("%09x ", shNameOff);
			// }

			symNameOff = extractField(currStrTabOff+currRecordOff, 4);
			currsymName = addr+strtabOff+symNameOff;
			printf("%s ", currsymName);
		}
		printf("\n");

		currRecordOff += symEntsize;
	}
}

int getLongestStrLen(int numOfSh, int shNamesTableOff, int shTableOff){

	int longestLen = 0;
	int l;
	int currShOff = 0;
	int shNameOff = 0;
	char* currShName;
	for(l=0; l < numOfSh; l++){
		
		shNameOff =  extractField(shTableOff+currShOff, 4);

		currShName = addr+shNamesTableOff+shNameOff;
		if(strlen(currShName) > longestLen)
			longestLen = strlen(currShName);

		currShOff += 40;
	}

	return longestLen;
}

void printSpaces(int num){

	int i;
	for(i=0; i < num; i++){
		printf(" ");
	}
}

void printSizes(int amount, int tableOff, int fieldOff, int recordSize){

	int l;
	int currShSize = 0;
	int currShOff = 0;
	for(l=0; l < amount; l++){
		
		if(l < 10)
			printf("  [ %d]: ", l);
		else
			printf("  [%d]: ", l);

		currShSize = extractField(tableOff+currShOff+fieldOff, 4);
		printf("%d\n", currShSize);

		currShOff += recordSize;
	}
}

void convertNibblesToStr(char nibbles[], char buf[]){

	int j = 0;
	int i;
	for(i=3; i >= 0; i--){
		
		buf[j+1] = ((unsigned char) nibbles[i] & 0xf);
		if(buf[j+1] > 9)
			buf[j+1] += 87;
		else
			buf[j+1] += 48;
			
		buf[j] = ((unsigned char) nibbles[i] >> 4);
		if(buf[j] > 9)
			buf[j] += 87;
		else
			buf[j] += 48;

		j += 2;
	}
}

void setFileName(){

	printf("Plaese enter a file name\n");
	fgets(filename, 100, stdin);

	//remove \n from filename
	int i=0;
	while(filename[i] != 0){
		if(filename[i] == '\n'){
			filename[i] = 0;
			break;
		}
		i++;
	}

	if(debug){
		printf("Debug: file name set to %s\n", filename);
	}
}



/*
void setUnitSize(){

	
	char buf[3];
	printf("Plaese enter a unit size(1|2|4)\n");
	fgets(buf, 3, stdin);
	
	int input;
	sscanf(buf, "%d", &input);
    
    if ((input==1) || (input==2) || (input==4)){
        
        size = input;

        if(debug){
			printf("Debug: set size to %d\n", size);
		}
    }
    else{
        printf("%d is an invalid input! Size was not changed\n", input);
    }
}

void displayFile(){

	// //REMOVE ME********************
	// togDebug();
	// setFileName();
	// setUnitSize();
	// //REMOVE ME********************


	
	if(filename == NULL){
		fprintf(stderr, "File name is null!\n");
	}
	else{
		FILE* file = fopen(filename, "r");

		if(file == NULL){
			
			fprintf(stderr, "An error occured while attempting to open %s!\n", filename);
			return;
		}

		int location;
		int length;
		char buf[9+9];
		printf("Plaese enter <location> <length>\n");
		
		fgets(buf, 9+9, stdin);
		sscanf(buf, "%x %d",&location, &length);

		if(debug){

			fprintf(stderr, "You enterd:\n");
			fprintf(stderr, "\tLocation: %x\n", location);
			fprintf(stderr, "\tLength: %d\n", length);
		}

		unsigned char* bytesToDisplay = (unsigned char*) malloc(size*length);

		fseek(file, location, SEEK_SET);
		fread(bytesToDisplay, size, length, file);
		
		// int k;
		// for(k=0; k < length; k++){

		// 	int readBytes = fread(bytesToDisplay, size, 1, file);
		// 	if(readBytes < size)
		// 		break;

		// 	fseek(file, size, SEEK_CUR);
		// }

		fclose(file);

		// printf("%x\n", bytesToDisplay[0]);
		// printf("%x\n", bytesToDisplay[1]);
		// printf("%x\n", bytesToDisplay[2]);
		// printf("%x\n", bytesToDisplay[3]);
		
		int i;
		printf("Hexadecimal Representation:\n");
		for(i=0; i < (size*length); i += 2){

			int j;
			for(j=i+1; j >= i; j--){

				printByte(bytesToDisplay[j]);
			}

			printf(" ");
		}

		printf("\n");


		unsigned char nibbles[4];
		printf("Decimal Representation:\n");
		for(i=0; i < (size*length); i += 2){


			nibbles[0] = (bytesToDisplay[i] & 0xf);
			nibbles[1] = (bytesToDisplay[i] >> 4);
			nibbles[2] = (bytesToDisplay[i+1] & 0xf);
			nibbles[3] = (bytesToDisplay[i+1] >> 4);

			int decNum = 0;
			int j;
			for(j=3; j >=0; j--){
				// printf("nibble[%d] = %d\n", j, nibbles[j]);

				decNum += (nibbles[j])*(pow(16, j));
			}

			printf("%d ", decNum);
		}

		printf("\n");

		free(bytesToDisplay);

	}
}

void loadToMem(){

	if(filename == NULL){
		fprintf(stderr, "File name is null!\n");		
	}
	else{

		FILE* file = fopen(filename, "r");

		if(file == NULL){
			
			fprintf(stderr, "An error occured while attempting to open %s!\n", filename);
			return;
		}

		int location;
		int length;
		char buf[9+9];
		printf("Plaese enter <location> <length>\n");
		
		fgets(buf, 9+9, stdin);
		sscanf(buf, "%x %d",&location, &length);

		if(data_pointer != NULL){
			free(data_pointer);
		}

		data_pointer = (unsigned char*) malloc(length);

		if(debug){

			fprintf(stderr, "\tfilename: %s\n", filename);
			fprintf(stderr, "\tLocation: %x\n", location);
			fprintf(stderr, "\tLength: %d\n", length);
			fprintf(stderr, "\tData_pointer: %p\n", data_pointer);
		}

		fseek(file, location, SEEK_SET);
		fread(data_pointer, 1, length, file);

		printf("Loaded %d bytes into %p\n", length, data_pointer);

		fclose(file);
	}
}

void saveToFile(){

	if(filename == NULL){
		fprintf(stderr, "File name is null!\n");		
	}
	else{

		FILE* file = fopen(filename, "r+");

		if(file == NULL){
			
			fprintf(stderr, "An error occured while attempting to open %s!\n", filename);
			return;
		}

		int source_address_int;
		unsigned char* source_address;
		int target_location;
		int length;
		char buf[9+9+9];
		printf("Plaese enter <source-address> <target-location> <length>\n");
		
		fgets(buf, 9+9+9, stdin);
		sscanf(buf, "%x %x %d",&source_address_int, &target_location, &length);
		// sscanf(buf, "%x %x %d",source_address, &target_location, &length);

		source_address = (unsigned char*) source_address_int;
		if(source_address == 0){

			source_address = data_pointer;
		}


		fseek(file, 0, SEEK_END);
		// fseek(file, 0L, SEEK_END);
		long fileSize = ftell(file);
		
		if(debug){

			fprintf(stderr, "\tfilename: %s\n", filename);
			fprintf(stderr, "\tfile size: %d\n", (int) fileSize);
			fprintf(stderr, "\tLocation: %x\n", target_location);
			fprintf(stderr, "\tLength: %d\n", length);
			fprintf(stderr, "\tData_pointer: %p\n", data_pointer);
			fprintf(stderr, "\tSource_address: %p\n", source_address);
		}

		if(fileSize < target_location){
			fprintf(stderr, "Target-location (%d) exceeds file size (%d)!\n",
								target_location,
								(int) fileSize);
			return;
		}

		fseek(file, target_location, SEEK_SET);
		int written = fwrite(source_address, 1, length, file);

		printf("Wrote %d bytes into %s from %p\n", written, filename, source_address);

		fclose(file);
	}
}

void modifyFile(){

	if(filename == NULL){
		fprintf(stderr, "File name is null!\n");		
	}
	else{

		FILE* file = fopen(filename, "rb+");

		if(file == NULL){
			
			fprintf(stderr, "An error occured while attempting to open %s!\n", filename);
			return;
		}

		int location;
		int val;
		char buf[9+9];
		printf("Plaese enter <location> <val>\n");
		
		fgets(buf, 9+9, stdin);
		sscanf(buf, "%x %x", &location, &val);
		// sscanf(buf, "%x %x %d",source_address, &location, &length);

		fseek(file, 0, SEEK_END);
		// fseek(file, 0L, SEEK_END);
		long fileSize = ftell(file);

		if(debug){

			fprintf(stderr, "\tfilename: %s\n", filename);
			fprintf(stderr, "\tfile size: %d\n", (int) fileSize);
			fprintf(stderr, "\tLocation: %x\n", location);
			fprintf(stderr, "\tValue: %x\n", val);
		}
		
		if(fileSize < location){
			fprintf(stderr, "location (%d) exceeds file size (%d)!\n",
								location,
								(int) fileSize);
			return;
		}

		fseek(file, location, SEEK_SET);
		fwrite(&val, size, 1, file);

		// printf("Wrote %d bytes into %s from %p\n", written, filename, source_address);
		printf("The value %x was written into %s\n", val, filename);

		fclose(file);
	}
}

*/
void quit(){

	// if(data_pointer != NULL){
	// 	free(data_pointer);
	// }

	if(currentfd != -1){
		close(currentfd);
		munmap(addr, mystat.st_size);
	}

	printf("quitting\n");
	exit(0);
}
