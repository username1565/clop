#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#ifdef __linux__
	#include <sys/utsname.h>
#endif

#define VERSION "1.03"

#ifdef _WIN64
	#define WINPAUSE system("pause") //to don't close window when double click on exe
    const char* OS = "Windows 64-bit";
#elif _WIN32
	#define WINPAUSE system("pause") //to don't close window when double click on exe
    const char* OS = "Windows 32-bit";
#elif __APPLE__ || __MACH__
    const char* OS = "Mac OS X";
#elif __FreeBSD__
    const char* OS = "FreeBSD";
#else
    const char* OS = "Other";
#endif

#define max(a,b) ((a) > (b) ? a : b)
#define min(a,b) ((a) < (b) ? a : b)

// progress - режим із відображенням прогресу
int progress = 0;
// output - відправити stdout у файл out_%
int output = 0;
int output_first_last = 0; 	//save first and last prime in separate file _first-last
int first_writting = 1;		//is this first writting (need to add date)? default - 1
int save_first = 0; //need save first prime or no? Default - 0, no.

const int Step = 2;				//step for check = 2. (odd + 2 = odd).
const uint64_t minPrime = 3; 	//first odd prime
const uint64_t maxPrime = UINT64_MAX;

struct timeb starttime, endtime;
FILE * fout, * fchk,
* first_last_file; //file with first ans last primes in each part

// 8GB обмеження по пам'яті
uint32_t maxBlockSize = 1024 * 1024 * 1024, blockSize = 100000, bSize = 0;
uint64_t 	* Block = NULL,
			toCnt = 0,
			toCnt_in_1_file = 0,
			x = 0, 					//x need for count steps of spliting filename, using strtok().
			n = 0,					//primes in file (default = 0, all primes in the same file)
			b = 10,					//primes in block (default 10). Block is a line with 10 primes by default.
			continue_n = 0;			//if need to continue n primes without writting...
	
char 	d[10] = "\t",	//delimiter in block (default '\t' in line). 10 primes in line are delimited with '\t'
		bd[10] = "\n"; //delimiter between blocks (default '\n' after each line). Each new line delimited with '\n'

uint64_t 	Ini,
			Fin,
			Cur,
			taskIni,
			taskFin,
			suffix_int = 0,
			code_bd,
			code_d;
			
char	outfname[256] = "out",
		chkfname[256] = "chk",
		filename[256], //temp file name variable,
		name_of_file[256], //this variable contains only name of file, without prefixes and suffixes...
		outfname_first_last[256], //file name for first and last part
		temp_for_strtok[256], //variable to copy filename for strtok(), because function strtok modify the current string
		path_to_7z[256] = "7z.exe"; //full pathway to 7z.exe. Default just "7z.exe" from environment variable "PATH"
int		make7z = 0;
							
	/*
		folder[256] = "./primes/";	//relative pathway to the working folder (unused)
		path[256];//temp variable to save pathways
	
		//wanted to do...
		//strcpy(path, folder);
		//strcat(path, filename);
		//strcpy(filename, path);
	*/
		
int		same_file = 1;			//write to the same file. default - 1.

char *ch; //result of working strtok function.

static __inline__ uint64_t string_to_u64(const char * s) {
  uint64_t i;
  char c ;
  int scanned = sscanf(s, "%" SCNu64 "%c", &i, &c);
  if (scanned == 1) return i;
  if (scanned > 1) {
    // TBD about extra data found
    return i;
  }
  // TBD failed to scan;
  return 0;
}

//this structure will be returned from checkfile function, like an array.
typedef struct {
    uint64_t 	count,
				lines_count,
				first_number,
				last_number,
				seems_like_full;
}RESULT_CHECK_FILE;

RESULT_CHECK_FILE *return_empty_structure(){
	RESULT_CHECK_FILE *empty = malloc(sizeof(*empty)); //allocate memory for structure
	empty->count = 0;
	empty->lines_count = 0;
	empty->first_number = 0;
	empty->last_number = 0;
	empty->seems_like_full = 0;
	return (empty);
}

RESULT_CHECK_FILE *check_file(char filename[256]){
	/*
	printf(
		"\n\nDefined variables in start check_file function: n = %" PRIu64
		", b = %" PRIu64 ", bd = %s, Ini = %" PRIu64
		", taskIni = %" PRIu64 ", minPrime = %" PRIu64,
		n, b, bd, Ini, taskIni, minPrime
	);
	*/
	uint64_t 	//i = 0,							//for iterations
				count = 0,						//count numbers
				lines_count = 0,				//count lines
				file_seems_like_a_full = 0,		//1 - if file seems like a full file, default - 0
				save_next_first = 0,			//if need to save next first
				save_last = 0,					//1 if need save a last number, default 0. 1 if 1 iteration of cycle...
				current_number = 0,				//variable to save current number.
				first_number = 0,				//save first number here
				last_number = 0;				//here save last nubmer.
	//all this uint64_t
				
	RESULT_CHECK_FILE *struct_chkfile_result = malloc(sizeof(*struct_chkfile_result)); //allocate memory for structure
	FILE* file;//file

	char c = bd[0]; 		//now "n" as '\n' here - this is char and int of charcode. This is delimiter beetween strings.
//And fgetc() result can be compared with this as with integer.
	file = fopen(filename, "r");
	//printf("\n in check function - file fopen filename r - case7\n");

	//printf("\n char bd - delimiter for blocks = %d", c);

	if(file!=NULL && !ferror(file)){ //if file opened successfully - check this...
		while(!feof(file)){ //read file
			if(fscanf(file, "%" PRIu64, &current_number) != EOF){count++;} 	//count numbers while not End Of File
//Here char comparing as int
			if(fgetc(file)==c){lines_count++;}								//count lines.
			if(count==1 || save_next_first){								//if number is first in file
				if(current_number==2){save_next_first = 1;continue;}		//if this number = 2(even), shift to 3 - first odd prime number.
				first_number = current_number;								//save this as first number
				struct_chkfile_result->first_number = current_number;		//write in structure
				if(save_next_first){save_next_first=0;}						//reset this
			}
			if(!save_last){save_last = 1;}									//say true to save last number...
		}
		if(count==0){first_number = current_number = struct_chkfile_result->first_number = 0;}
		if(
				((lines_count==0) && (count>0))				//if lines == 0, this cann't be devisor, but if count>0 - 1 line in file.
			||	((lines_count!=0)&&(count%lines_count>0))	//or if lines!= this can be devisor. count%n contains digits in last line. line++ then.
		){lines_count++;}		
		if(save_last){
			last_number = current_number;
			struct_chkfile_result->last_number = current_number;
		} //save last number in the end of cycle if this have more than 1 iteration...

		//fprintf(stderr, "\n In file %s - %" PRIu64 " numbers and %" PRIu64 " lines...\n", filename, count, lines_count);
		if(									//if
				n!=0						//n not null - this can be devisor
			&& 	count%n == 0				//and all strings in the file are full
			&& 	count == n					//and if numbers = n
		){
			file_seems_like_a_full = 1;
		
		/*
			printf(
				"1 - File seems like a full...\n"
				"first_number = %" PRIu64 ", last_number = %" PRIu64 "\n", first_number, last_number
			);
		*/
		
		/*
			//when generation...
			//if first=2 or Cur+i, continue writting...
			//if toCnt<count, continue;
			//if toCnt == count && last_number = Cur+i; - fclose(file); + check new file...
		*/
		}
		//fclose(file); //if file seems, like a full, don't close this...
	}
	/*
	else{//file not found but will be created.
        printf("Problem with opening file for reading\n");
        return 0; // or some error code
    }
	*/
	
	struct_chkfile_result->count = count;
	struct_chkfile_result->lines_count = lines_count;
	struct_chkfile_result->seems_like_full = file_seems_like_a_full;
	
	//count = lines_count = file_seems_like_a_full = save_last = 0; //set null for this variables.
	
	return (struct_chkfile_result); //return structure.
	//now values is here:
	//printf("%" PRIu64, structure_var->count);
	//if(structure_var->lines_count*b==n){/*...*/}
	//etc...
}

void save_checkpoint(uint64_t pos)
{
    fchk = fopen(chkfname, "w");
	//printf("\n fchk fopen chkfname, w - case8\n");
    if(fchk == NULL) {
		exit(EXIT_FAILURE);
    }

    struct timeb curtime;
    ftime(&curtime);
    uint64_t dif = (curtime.time - starttime.time) * 1000 + (curtime.millitm - starttime.millitm);
    fprintf(fchk,  "%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                  ",%" PRIu64
                ,Ini
                ,Fin
                ,pos
                ,dif
                ,toCnt
           );
    fflush(fchk);
    fclose(fchk);
}

int read_checkpoint(void)
{
    fchk = fopen(chkfname, "r");
	//printf("\n open checkfile r case9");
    if(fchk == NULL)
		return (EXIT_FAILURE);
    char c;
    uint64_t dif;
    int scanned = fscanf(fchk, "%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%" SCNu64
                              ",%c"
                              , &Ini
                              , &Fin
                              , &Cur
                              , &dif
                              , &toCnt
                              , &c);
    fclose(fchk);
    if (scanned != 5) {
		printf("\n\nError: scanned !== 5, scanned = %d.\n", scanned);

		system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
    }
    if (!Cur) return 1;
        else Cur = ((Cur / Step) + 1) * Step + 1;
    starttime.time -= dif / 1000;
    long int millisec = (dif % 1000);
    if (starttime.millitm < millisec) {
        starttime.millitm += 1000 - millisec;
        starttime.time--;
    } else starttime.millitm -= millisec;
    return 0;
}

void free_block(void)
{
    free(Block);
}

void sieve_block(uint32_t size, uint64_t last)
{
    uint64_t i, maxFactor = rintl(sqrtl(last));
    if (bSize - size) {
        free_block();
        Block = (uint64_t *) calloc(size, sizeof(uint64_t));
        bSize = size;
    }
    else {
        for (i = 0; i < size; i++) Block[i] = 0;
    }
    for (i = 3; i <= maxFactor; i += 2) {
        uint64_t j, k = Cur / i;
        k = i * max(3, ((k && 1) && !(Cur % i) ? k : ((((k + 1) >> 1) << 1) + 1)));
        for (j = k; (j <= last) && (Cur <= j); j += 2*(uint64_t)i) {
            Block[(j - Cur) >> 7] |= ((uint64_t)1 << (((j - Cur) >> 1) & 63));
        }
    }
}

int init_task(void)
{
    if (Ini > Fin) return 1;
    if (Ini < minPrime) Ini = minPrime;
    if (Fin > maxPrime) Fin = maxPrime;
    Ini = (uint64_t)(Ini / Step) * Step + 1;
    Fin = (uint64_t)((Fin - 1) / Step) * Step + 1;
    Cur = Ini;
    return 0;
}

#define PBSTR "========================================================================"
#define PBWIDTH 55
#define SCRWIDTH 80
void do_progress( double percentage )
{
    int val = (int) (percentage);
    int lpad = (int) (percentage  * (val==100?SCRWIDTH:PBWIDTH) / 100);
    int rpad = (val==100?SCRWIDTH:PBWIDTH) - lpad;
    //fill progress bar with spaces
    fprintf(stderr, "\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    if (val!=100)
        fprintf(stderr, " %" PRIu64, toCnt);
    //fflush(stderr);
}

void print_usage(void)
{
#ifdef _WIN32
	char pref[3] = "> ";
#elif _WIN64
	char pref[3] = "> ";
#elif __linux__ || unix || __unix__ || __unix
	char pref[3] = "./";
#endif // __linux__
	//additional parameters added here - "-s", "-n", "-b", "-d","-bd", "-wfl" + desctiption of codes for delimiters for "-d" and "-bd"
    fprintf(stderr, "Usage: %sclop <low> <high> [switches]\n", pref);
    fprintf(stderr, "\t<low>\t\t\tlower border\n\n");
    fprintf(stderr, "\t<high>\t\t\thigher border\n\n");
    fprintf(stderr, "The following switches are accepted:\n\n");
    fprintf(stderr, "-p\t\t\tJust need to display progress bar (default - disabled)\n\n");
    fprintf(stderr, "-o [filename]\t\twrite results to [filename+extension] file\n\n");
    fprintf(stderr, "-s [part]\t\tstart suffix integer (number of start part)\n\n");
    fprintf(stderr, "-n [pr/file]\t\tnumber of primes in one file\n\t\t\t(default 0 - and all primes writting in the same file)\n\n");
    fprintf(stderr, "-b [pr/block]\t\tnumber of primes in one block\n\t\t\t(default 10 primes per line)\n\n");
	fprintf(stderr, "-d [block delim]\tdelimiter between primes in one block\n\t\t\t(default '\\t' for primes in line)\n\n");
	fprintf(stderr, "-bd [blocks delim]\tdelimiter between blocks\n\t\t\t(default '\\n' between lines)\n\n");	
	fprintf(stderr, "-wfl\t\t\tJust check to [write first and last] prime\n\t\t\tfor each part in [filename]... (default value: disabled)\n\n");
    fprintf(stderr, "-f [size]\t\tfactoring block size\n\t\t\t(default value: %" PRIu32 ")\n\n", blockSize);
	fprintf(stderr, "-path_to_7z [path]\tpath to 7z.exe (default value: \"7z.exe\" \n\t\t\tfrom environment variable PATH.)\n\t\t\tUse double slash in the path (like \"C:\\\\7-zip\\\\7z.exe\")\n\n");
	fprintf(stderr,
		"For delimiters -d and -bd can be used next values (one byte):\n"
        "___________________________________________________________________\n"
		"(decimal code) - description\n"
        "___________________________________________________________________\n"
		"07 - \\a\taudible bell\t\t\tbyte 0x07 in ASCII encoding\n"
		"08 - \\b\tbackspace\t\t\tbyte 0x08 in ASCII encoding\n"
		"09 - \\t\thorizontal tab\t\t\tbyte 0x09 in ASCII encoding\n"
		"10 - \\n\tline feed - new line\t\tbyte 0x0a in ASCII encoding\n"
		"11 - \\v\tvertical tab\t\t\tbyte 0x0b in ASCII encoding\n"
		"12 - \\f\tform feed - new page\t\tbyte 0x0c in ASCII encoding\n"
		"13 - \\r\tcarriage return\t\t\tbyte 0x0d in ASCII encoding\n"
		"34 - \\""\"""\tdouble quote\t\t\tbyte 0x22 in ASCII encoding\n"
		"39 - \\'\tsingle quote\t\t\tbyte 0x27 in ASCII encoding\n"
		"63 - \\?\tquestion mark\t\t\tbyte 0x3f in ASCII encoding\n"
		"92 - \\""\\""\tbackslash\t\t\tbyte 0x5c in ASCII encoding\n"
        "___________________________________________________________________\n"
        
		"\nOnly decimal code can be a char value for escaped delimiter, here...\n"
        "Or any another symbols can be a delimiters, too...\n\n"
        "Multiple delimiters supporting (like '_,;_'), but without escaped characters.\n"
		"CRLF here ';\\n' - not supported and will be printed in file, as ASCII text."
	);
}

int main(int argc, char** argv)
{
#ifdef _WIN32
//nothing
#elif _WIN64
//nothing
#elif __linux__ || unix || __unix__ || __unix
	char OS[256];
	struct utsname name;
	if(uname(&name)){
		printf("\n\nError: uname(&name)!=true\n");

		system("pause"); //pause program to don't close window if double click on exe
		
		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
	}
	sprintf(OS, "%s %s", name.sysname, name.release);
#endif // __linux__
    fprintf(stderr, "CYCLOP (C - Consecutive List Of Primes) %s (%s)\nCopyright 2018, Alexander Belogourov aka x3mEn\n\n", VERSION, OS);
    if (argc < 3) {
        print_usage();
		printf("\n\nNot enough arguments specified...\n");

		system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
    }

    taskIni = Ini = string_to_u64(argv[1]);
    taskFin = Fin = string_to_u64(argv[2]);
    if (!Ini || !Fin) {
        print_usage();
		printf("\n\nOne argument not specified and this was been interpretted as null...\n");

		system("pause"); //pause program to don't close window if double click on exe
		
		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
    }

    sprintf(outfname, "out_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);
    sprintf(chkfname, "chk_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);

	//fprintf(stderr, "number of arguments: %d\n\n", argc);
    for (int i = 3; i < argc; i++) {
		//fprintf(stderr, "%d - Now will be worked arg %s\n", i, argv[i]);

        if (!strcmp(argv[i],"-path_to_7z")) {make7z = 1; continue;}
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-path_to_7z")) {
			sprintf(path_to_7z, "%s", argv[i]);
			continue;
		}
        if (!strcmp(argv[i],"-p")) {progress = 1; continue;}
        if (!strcmp(argv[i],"-o")) {output = 1; continue;}
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-o")) {
			sprintf(outfname, "%s", argv[i]);
			sprintf(name_of_file, "%s", argv[i]);
			continue;
		}
        if (!strcmp(argv[i],"-s")) {continue;} //start suffix integer - number of the part
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-s")) {
			suffix_int = string_to_u64(argv[i]); //copy this as int from string
			
			x = 0; //start - null
			strcpy(temp_for_strtok, outfname); //copy outfname to temp_for_strtok
			ch = strtok(temp_for_strtok, ".");											//split by "."
			sprintf(filename, "%s", ch);												//filename only to "filename" variable

			while (ch != NULL) {												//while not null
				x++;																	//x++
				if(x > 1){															//if x>1
					sprintf(filename, "%s_part" "%" PRIu64, filename, suffix_int);		//add part to filename prefix
					sprintf(filename, "%s.%s", filename, ch);							//add extension
				}
				ch = strtok(NULL, ".");													//again...
			}
			sprintf(outfname, "%s", filename);											//write new filename to outfname
			continue;
		}
        if (!strcmp(argv[i],"-n")) {continue;}//was been specified a number of primes in each part
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-n")) {n = string_to_u64(argv[i]); continue;} //save n as int64 from string
        if (!strcmp(argv[i],"-bd")) {continue;} //bd before b, because -bd contains -b
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-bd")) {//bd - is a delimiter between the blocks (lines)
			code_bd = string_to_u64(argv[i]); //copy string as integer
			switch(code_bd){//compare
				case 7 ... 13: //if int from 07 to 13
				case 34: case 39: case 63: case 92: 	//or if one from this codes
					sprintf(bd, "%c", code_bd); break; 	//save this as char delimiter
				default: sprintf(bd, argv[i]); break;	//else, return delimiter - as text
			}
			continue;
		}
        if (!strcmp(argv[i],"-b")) {continue;} //was been specified a number of primes in each block
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-b")) {b = string_to_u64(argv[i]); continue;} //copy as int from string
        if (!strcmp(argv[i],"-d")) {continue;}//was been specified delimiter between primes in each block(line)
        if (argv[i][0] != '-' && !strcmp(argv[i-1],"-d")) {
			code_d = string_to_u64(argv[i]);//copy as int, if int
			switch(code_d){//return char in this cases
				case 7 ... 13: case 34: case 39: case 63: case 92: sprintf(d, "%c", code_d); break;
				default: sprintf(d, argv[i]); break;
			}
			continue;
		}
        if (!strcmp(argv[i],"-wfl")) { //if need to "write first last"(wfl)
			output_first_last = 1; //this true
			strcpy(temp_for_strtok, outfname);	//copy outfname to temp variable

			if(suffix_int==0){	//if suffix_int not specified, then filename not contains suffix _part
				ch = strtok(temp_for_strtok, ".");	//split by "."
			}
			else{				//if start suffix_int specified, then filename contains suffix _part
				ch = strtok(temp_for_strtok, "_");	//split by "_"
			}
			sprintf(filename, "%s", ch); //write file name only to "filename"

			x = 0;//start 0
			while (ch != NULL) {												//while not null
				x++;																//x++
				if((suffix_int==0 && x > 1)||(suffix_int!=0 && x > 2)){	//if this
					sprintf(filename, "%s_first-last", filename);						//add suffix _first-last
					sprintf(filename, "%s.%s", filename, ch);							//add extension
				}
				ch = strtok(NULL, ".");													//again
			}
			sprintf(outfname_first_last, filename);								//write new filename to outfname_first_last
			continue;
		}
        if (!strcmp(argv[i],"-f")) {continue;}
        if (string_to_u64(argv[i]) && !strcmp(argv[i-1],"-f")) {blockSize = min(maxBlockSize, string_to_u64(argv[i])); continue;}
        print_usage();
		
		//printf("\n\nAll arguments was been processed...\n");
		system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
    }
	
/*
	//test running: clop 1 15000 -p -o result.txt -s 123 -n 1000 -b 7 -d 34 -bd 10 -wfl -f 100
	//test output all variables
	fprintf(stderr,
		"\n\nprogress = %d, output = %d, output_first_last = %d, outfname = %s, outfname_first_last = %s"
		", suffix_int = %" PRIu64 ", n = %" PRIu64
		", bd = %s, b = %" PRIu64 ", d = %s, blockSize = %" PRIu64 "\n\n",
		progress, output, output_first_last, outfname, outfname_first_last,
		suffix_int, n,
		bd, b, d, blockSize
	);
	//OK
*/	

    ftime(&starttime);

    time_t timer;
    char curdatetime[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(curdatetime, 26, "%d.%m.%Y %H:%M:%S", tm_info);

    int ErrorCode, CheckPointCode;
    ErrorCode = CheckPointCode = read_checkpoint();
    if (ErrorCode) ErrorCode = init_task();
    if (ErrorCode) return ErrorCode;

    uint64_t total = Fin >= Ini ? (uint64_t)((Fin - Ini) / Step) + 1 : 0;
    uint64_t bStep = 0, blockWidth = (blockSize - 1) * Step;

	RESULT_CHECK_FILE *file_chk_results;
	
    fout = fopen(outfname, "r");
	//printf("\n fout fopen %s - r, case4", outfname);
    if (fout != NULL && CheckPointCode) {
		fclose(fout);//close fout and open in function when checking...
        file_chk_results = check_file(outfname);
		
		//printf("\n\ncount: %" PRIu64 "\n", file_chk_results->count);
		//printf("lines_count: %" PRIu64 "\n", file_chk_results->lines_count);
		//printf("first_number: %" PRIu64 "\n", file_chk_results->first_number);
		//printf("last_number: %" PRIu64 "\n", file_chk_results->last_number);
		//printf("seems_like_full: %" PRIu64 "\n", file_chk_results->seems_like_full);
		
		//fclose(fout);
		
		//printf("\n\nFile %s already exists... Stop... \n", outfname);

		//system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		//exit (EXIT_SUCCESS);
    }else{
		file_chk_results = return_empty_structure(); //if file not found, return empty structure.
	}
    if (output) {
        if (!CheckPointCode && fout == NULL) {
			printf("\n\nFile %s not found...\n", outfname);

			system("pause"); //pause program to don't close window if double click on exe

			//printf("Press Ctrl+C or Enter to exit...");
			//getchar(); //wait char to don't close window if double click on exe

			exit(EXIT_FAILURE);
        }
        if (CheckPointCode && file_chk_results->count==0) {
            fout = fopen(outfname, "w");
			//printf("\n First fout fopen %s - w - case5\n", outfname);
        } else {
            fout = fopen(outfname, "a");
			//printf("\n First fout fopen %s - a - case6\n", outfname);
        }
        if (fout == NULL) {
			printf("\n\nFile %s not found\n", outfname);

			system("pause"); //pause program to don't close window if double click on exe
			
			//printf("Press Ctrl+C or Enter to exit...");
			//getchar(); //wait char to don't close window if double click on exe

			exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr, "Command line parameters :");
    for (int i = 1; i < argc; i++)
        fprintf(stderr, " %s", argv[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Number range borders    : from %" PRIu64 " to %" PRIu64 " step %d\n", Ini, Fin, Step);
    fprintf(stderr, "Total amount of Numbers : %" PRIu64 "\n", total);
    fprintf(stderr, "Factoring Block Size    : %" PRIu32 "\n", blockSize);
    fprintf(stderr, "Starting date timestamp : %s\n", curdatetime);

    int cpcnt, ctpcnt = 0;
    float cstep = 1000;

    if (progress)
        do_progress(ctpcnt);

    if (taskIni < minPrime) {//if start number < 3, write (exclusive even prime number).
        toCnt++;
        if (output && file_chk_results->first_number==0){ //if first number == 0, no 2 in file and no any prime in the file.
			//The first and singular EVEN prime number "2" is an exception here. All other primes is odd numbers.
			if(b==1){	//if one number in block use delimiter between blocks...
				//fprintf(stderr, "\nWrite 2 firstly with bd,\n\"%" PRIu64 "\"", toCnt);
				fprintf(fout, "%d%s", 2, bd);
			}
			else{ 		//else use delimiter inside block...
				//fprintf(stderr, "\nWrite 2 firstly with d,\n\"%" PRIu64 "\"", toCnt);
				fprintf(fout, "%d%s", 2, d);
			}
		}
		if(output_first_last && file_chk_results->first_number==0){
			first_last_file = fopen(outfname_first_last, "a+");
			//add timestamp at first, as delimiter between another blocks (if many parts generated...)
			if(first_writting){
				//fprintf(stderr, "\n first_writting = %d, and nulled...", first_writting);
				first_writting = 0;
				time_t t = time(NULL);
				struct tm tm = *localtime(&t);
				//add date as delimiter, to separate info, about all previous parts
				//fprintf(stderr, "\n try to writting in %s", first_last_file);
				fprintf(first_last_file, "\n\n    DATE AND TIME:    %d-%d-%d %d:%d:%d\n\n",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			}
			fprintf(first_last_file, "[%" PRIu64 ",\'2 - ", suffix_int); //save part and first prime
			
			fclose(first_last_file);
			if(save_first) save_first = 0;
		}
    }

    while (Ini <= Cur && Cur <= Fin) {
        uint32_t bs = (Fin - Cur < blockWidth ? Fin - Cur : blockWidth) / Step + 1;
        uint64_t Last = Cur + (bs - 1) * Step;
        sieve_block(bs, Last);
        for (uint64_t i = 0; i < bSize * Step; i += 2) {
            if (!(Block[i >> 7]&((uint64_t)1 << ((i >> 1) & 63)))) {
                toCnt++;
				toCnt_in_1_file = toCnt%n;
				//fprintf(stderr, " %d,%d ", toCnt, toCnt_in_1_file);
                if (output){

					//fprintf(stderr, "\n TEST \n");
					if(n!=0){
//						if(toCnt_in_1_file==0){
//							fprintf(stderr, "\nprime %" PRIu64 " - already writted to %s. continue; ", Cur+i, outfname);
//							continue;
//						}
//						else
						if(toCnt_in_1_file==0){//if was been writed a last prime in file (n)
							//fprintf(stderr, "\n this again... \n");
							if(
									output_first_last
								&&	file_chk_results->seems_like_full == 0	//and if file not a full
							){//write this in _first-last, if need
							
								first_last_file = fopen(outfname_first_last, "a+");
								if(first_writting){
									//fprintf(stderr, "\n first_writting = %d, and nulled...", first_writting);
									first_writting = 0;
									//add timestamp at first, as delimiter between another blocks (if many parts generated...)
									time_t t = time(NULL);
									struct tm tm = *localtime(&t);
									//add data to separate info about all previos generated parts...
									//fprintf(stderr, "\n try to writting in %s", first_last_file);
									fprintf(first_last_file, "\n\n    DATE AND TIME:    %d-%d-%d %d:%d:%d\n\n",
									tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
								}
								fprintf(first_last_file, "%" PRIu64 "\'],\n", Cur + i); //save last prime
								fclose(first_last_file);

								if(!save_first){save_first = 1;}								//say need write last prime
							}
						}
						else if(toCnt_in_1_file==1 && !same_file){
							fflush(fout);		//flush data into file
							fclose(fout);		//close previous part

							if(make7z){//if need to create 7z and delete txt-parts...
							
								printf("\npath to 7z archive - \"%s\"", path_to_7z);	//just display path
								char command[256]; 	//define the temp variable to create and save command
								
								//make command
								if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
									//try to using 7z from environment variable PATH
									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\"", outfname); //create command to archive this
								}else{ //else, using full specified pathway	for 7z.exe
									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\"", outfname, path_to_7z); //create command to archive this
								}
								//printf("%s", command); //print this command for test
								system(command); //run creating archive and wait...

								sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s", outfname, outfname);
								//create this command to delete txt part, if 7z already exists
								//printf("%s", command); //print this command for test
								system(command); //run command and delete file
								
								//continue creating next txt-part...								
							}

							suffix_int++; //increment part suffix and create new filename
							//fprintf(stderr, "\nsuffix_int updated - %" PRIu64 "\n", suffix_int);
							strcpy(temp_for_strtok, outfname); 	//copy outfname to temp variable
							ch = strtok(temp_for_strtok, "_");	//split by "_"
							sprintf(filename, "%s", ch);		//add file name to "filename"

							x = 0; //start x
							while (ch != NULL) {		//while not null
								x++;						//++
								if(x > 2){					//if over 2
									sprintf(filename, "%s_part" "%" PRIu64, filename, suffix_int);		//add part using suffix _part
									sprintf(filename, "%s.%s", filename, ch);							//add extension
								}
								ch = strtok(NULL, ".");													//again
							}
							if(!strcmp(filename,outfname)){		//if the same outfname as result...
								strcpy(temp_for_strtok, outfname);	//copy this to temp variable
								ch = strtok(temp_for_strtok, ".");	//split by "."
								sprintf(filename, "%s", ch);		//add file name to "filename"

								x = 0; //start x
								while (ch != NULL) {		//while not null
									x++;					//++
									if(x > 1){				//if over 1
										sprintf(filename, "%s_part" "%" PRIu64, filename, suffix_int);	//add part using suffix _part
										sprintf(filename, "%s.%s", filename, ch);						//add extension
									}
									ch = strtok(NULL, ".");												//again
								}
							}
							sprintf(outfname, "%s", filename); //write filename to outfname

							//fprintf(stderr, "\nTry to create %s to save prime %" PRIu64, outfname, Cur+i);
							//fprintf(stderr, "\n new file name after changing suffix - %s\n", outfname);

							fout = fopen(outfname, "r");
							//printf("\n In cycle fout fopen %s r - case1, (Cur+i) = %" PRIu64 "\n", outfname, Cur+i);
							if (fout != NULL && CheckPointCode) {
							
								file_chk_results = check_file(outfname);
							
								//printf("\n\ncount: %" PRIu64 "\n", file_chk_results->count);
								//printf("lines_count: %" PRIu64 "\n", file_chk_results->lines_count);
								//printf("first_number: %" PRIu64 "\n", file_chk_results->first_number);
								//printf("last_number: %" PRIu64 "\n", file_chk_results->last_number);
								//printf("seems_like_full: %" PRIu64 "\n", file_chk_results->seems_like_full);

								//fclose(fout);
							
								//printf("\n\nFile %s already exists... Stop... \n", outfname);
							
								//system("pause"); //pause program to don't close window if double click on exe
							
								//printf("Press Ctrl+C or Enter to exit...");
								//getchar(); //wait char to don't close window if double click on exe

								//exit(EXIT_SUCCESS);
							}else{
								file_chk_results = return_empty_structure(); //if file not found, return structure with nulled values.
							}
							if (!CheckPointCode && fout == NULL) {
								printf("\n\nFile %s not found...\n", outfname);

								system("pause"); //pause program to don't close window if double click on exe

								//printf("Press Ctrl+C or Enter to exit...");
								//getchar(); //wait char to don't close window if double click on exe

								exit(EXIT_FAILURE);
							}
							if (CheckPointCode && file_chk_results->count==0) {
								fout = fopen(outfname, "w");
								//printf("\n in cycle fout fopen %s w - case2\n", outfname);
							} else {
								fout = fopen(outfname, "a");
								//printf("\n in cycle fout fopen %s a - case3\n", outfname);
							}
							if (fout == NULL) {
								printf("\n\nFile %s not found...\n", outfname);

								system("pause"); //pause program to don't close window if double click on exe

								//printf("Press Ctrl+C or Enter to exit...");
								//getchar(); //wait char to don't close window if double click on exe

								exit(EXIT_FAILURE);
							}
						}
						else if(same_file){same_file = 0;}//skip writting to the same file in the future...
					}
					
					//fprintf(stderr, "\n output... prime = %" PRIu64 ", i = %d\n, Cur = %d", Cur+i, i, Cur);
					//fprintf(stderr, "\n output_first_last = %d, toCnt = %d, n = %d, save_first = %d", output_first_last, toCnt, n, save_first);
					if(
							output_first_last										//if need to write first prime
						&&	(		toCnt==1										//and if this is first prime
								||
									(		n!=0									//or if this all
										&& 	save_first
										&&	toCnt_in_1_file==1
										&&	file_chk_results->count == 0
									)												
							)
					){
						first_last_file = fopen(outfname_first_last, "a+");								//open file for adding data
						if(first_writting){
							//fprintf(stderr, "\n first_writting = %d, and nulled...", first_writting);
							first_writting = 0;
							//add timestamp at first, as delimiter between another blocks (if many parts generated...)
							time_t t = time(NULL);
							struct tm tm = *localtime(&t);
							//add data to separate info about all previos generated parts...
							//fprintf(stderr, "\n try to writting in %s", first_last_file);
							fprintf(first_last_file, "\n\n    DATE AND TIME:    %d-%d-%d %d:%d:%d\n\n",
							tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
						}					
						fprintf(first_last_file, "[%" PRIu64 ",\'%" PRIu64 " - ", suffix_int, Cur + i); //save part and first prime
						fclose(first_last_file);														//close this file...

						if(save_first) save_first = 0;													//reset this variable
					}
					if(continue_n!=0 && (toCnt_in_1_file>n-1 || toCnt_in_1_file==0)){
						if(toCnt_in_1_file==0){
							//fprintf(stderr, "\n Stop continue4 at step %" PRIu64, toCnt);
							continue_n=0;
						}
						//fprintf(stderr, "\n Continue4 to n for file %s", outfname);
						continue;
					}
					if(
							toCnt_in_1_file <= file_chk_results->count	//if file contains this number
						&&	toCnt_in_1_file!=0							//and if this not a last number
					){
						if(
								toCnt_in_1_file==1
							&&	file_chk_results->first_number != (Cur+i)
						){
							fprintf(stderr, "\n First number %" PRIu64 " in file %s not equal this prime %" PRIu64, file_chk_results->first_number, outfname, Cur+i);
							//continue_n = 1;
							//fprintf(stderr, "\n Continue next %" PRIu64 " numbers from this %" PRIu64, n-continue_n, toCnt);
							fprintf(stderr, "\n Write prime numbers, but after notify block...");
						}else{
							if(
								(
									file_chk_results->count==toCnt_in_1_file
								||	(file_chk_results->count==n) && toCnt_in_1_file == 0
								)
							&&	file_chk_results->last_number != (Cur+i)
							&& file_chk_results->count !=0
							){
								//do nothing and let to fill block in file in next steps...
							}
							else{
								//fprintf(stderr, "\n Continue1, toCnt = %" PRIu64 ", toCnt_in_1_file = %" PRIu64 ", prime %" PRIu64, toCnt, toCnt_in_1_file, Cur+i);
								continue;
							}
						}
					}										//skip this number.
					else if(toCnt_in_1_file==0){						//if this is last number in file
						if((Cur+i)==file_chk_results->last_number)		//and if this contains there
						{
							//fprintf(stderr, "\n Continue2, toCnt = %" PRIu64 ", toCnt_in_1_file = %" PRIu64 ", prime %" PRIu64, toCnt, toCnt_in_1_file, Cur+i);
							continue;
						}										//skip this number
						//else - do nothing and write this in next steps...
					}
					else if(
							toCnt_in_1_file==file_chk_results->count
						&&	file_chk_results->count<n
						&&	((Cur+i)==file_chk_results->last_number)
						&&	file_chk_results->seems_like_full == 0
					){
						fclose(fout); //opened for reading or writting... Close this...
						fout = fopen(outfname, "a+"); //open for adding data
						//printf("\n In cycle open fout %s - a+ - case7", outfname);
						
						//printf("\n\n%s a+, writting... Cur+i = %" PRIu64 ", toCnt = %" PRIu64, outfname, Cur+i, toCnt);
						//but...
						//fprintf(stderr, "\n Continue3 ");
						continue;
					}else if(
							file_chk_results->seems_like_full == 1
						&&	toCnt_in_1_file==file_chk_results->count
						&&	((Cur+i)!=file_chk_results->last_number)
					){
						printf(
							"File %s seems like a full,"
							"and count = %" PRIu64 ", but last_number %" PRIu64 " != generated number %" PRIu64 "\n Stop...",
							outfname, file_chk_results->count, file_chk_results->last_number, (Cur+i)
						);
						exit(EXIT_SUCCESS);
					}
					if(
						(
							file_chk_results->count==toCnt_in_1_file
						||	(file_chk_results->count==n) && toCnt_in_1_file == 0
						)
					&&	file_chk_results->last_number != (Cur+i)
					&&	file_chk_results->count !=0
					){
						fprintf(stderr, "\n Last number %" PRIu64 " in file %s - is not a prime %" PRIu64
										"\nMaybe this file corrupted."
										"\nContinue writting primes there, but next block separated."
										, file_chk_results->last_number, outfname, Cur+i
						);
						fprintf(fout,	"\n\n\nPREVIOUS PRIME NUMBER IS NOT CONSECUTIVE"
										"\nnext prime numbers are consecutive..."
										"\nTo get consecutive primes - you can try to regenerate this part from number %" PRIu64 " to %" PRIu64 ", using command:"
										"\n    >clop %" PRIu64 " %" PRIu64 " -p -o %s -s %" PRIu64 " -n %" PRIu64 " -b %" PRIu64 " -d %d -bd %d"
										"\n\n\n",
										file_chk_results->first_number, (Cur+i),
										file_chk_results->first_number, (Cur+i), name_of_file, suffix_int, n, b, (int) d[0], (int) bd[0]
						);
					}
					if(toCnt!=1 && toCnt%b==0){//if last prime in block
						//fprintf(stderr, " %" PRIu64 "+bd\n", toCnt);						
						if(continue_n==0){
							fprintf(fout, "%" PRIu64 "%s", Cur + i, bd);	//using delimiter between blocks
						}
					}
					else{//else if prime in block
						//fprintf(stderr, " %" PRIu64 " ", toCnt);						
						if(continue_n==0){
							fprintf(fout, "%" PRIu64 "%s", Cur + i, d); 	//using delimiter between primes in block
						}
					}
				}
            }
        }
		bStep = (Last - Ini) / Step + 1;
		cpcnt = (int)((double)bStep / total * cstep);
		if (ctpcnt != cpcnt) {
            ctpcnt = cpcnt;
			if (progress)
				do_progress((double)bStep / total * 100);
			save_checkpoint(Last + 1);
			if (output) fflush(fout);
			fflush(stdout);
		}
        Cur += bSize * Step;
    };

    if(output){fclose(fout);}
    remove(chkfname);

    ftime(&endtime);
    uint64_t dif = (endtime.time - starttime.time) * 1000 + (endtime.millitm - starttime.millitm);

    fprintf(stderr, "\n");
    fprintf(stderr, "Elapsed time            : %02d:%02d:%02d.%03d\n", (unsigned char)(dif/60/60/1000),
	(unsigned char)((dif/60/1000)%60), (unsigned char)((dif/1000)%60), (unsigned char)(dif%1000));
    fprintf(stderr, "Total amount of Primes  : %" PRIu64 "\n", toCnt);
    free_block();
	
	printf("\n\nWork finished... Press any key to write last block... \n");

	system("pause"); //pause program to don't close window if double click on exe

	//printf("Press Ctrl+C or Enter to exit...");
	//getchar(); //wait for input char to don't close window when double click on exe.
 
    return (EXIT_SUCCESS);
}