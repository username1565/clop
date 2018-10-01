#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
//#include <ctype.h> //isxdigit() function for second version of function str_unescape

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

//BEGIN - variables
		//variable						//description
int 	progress = 0,					// progress - enable progress bar
		output = 0,						// output - write stdout to file out_%
		output_first_last = 0, 			//save first and last prime in separate file _first-last
		first_writting = 1,				//is this first writting (need to add date)? default - 1
		save_first = 0, 				//need save first prime or no? Default - 0, no.
		x64 = 0,						//1 - if need to save prime numbers as 64 bit integers, without any delimiters
											//(80000000 bytes for every 10M primes part and 9Mbytes - 7z.)
		prime_gaps = 0,					//1 - if need to save prime numbers - "Prime gaps" (2 bytes for each gap, unsigned short int, without any delimiters)
											//(writting primes as full prime gaps - unsigned short int 0-65535.
											//Then this go to 2 bytes and writting as binary data [0000 - FFFF])
		gaps2byte = 0,					//default = 0; 1 if full prime gaps, as 2 bytes.
		gaps1byte = 0,					//1 byte half prime gaps, using an (unsigned char). Range [00-FF] = 255 * 2 for each half of all gaps (because all gaps are even).
										//In this case, sometimes using (unsigned short int), when gap is over 255*2.
										//00 - say about 2 bytes, because gap with 0 value give the previous prime number and never used.
		make7z = 0,						//1 - if need to make 7z
		same_file = 1,					//write to the same file. default - 1.

	//Next variables have the value 1 or 0 - just for check/uncheck ...
	//0 by default, 1 when argument specified in command line.
		arg_progress = 0,				//"-p" - progress option
		arg_gaps = 0,					//"-gaps" options - write primes as prime gaps, without specify format.
		arg_gaps2byte = 0,				//"-gaps2byte" option - write primes as 2 byte prime gaps
		arg_gaps1byte = 0,				//"-gaps1byte" option - write primes as 1-byte half prime gaps
		arg_x64 = 0,					//write as 64-bit integers - option
		arg_7z = 0,						//"-7z" option
		arg_7z_value = 0,				//full pathway to 7z.exe, after "-7z"
		arg_output = 0,					//"-o" - output option
		arg_ouput_file = 0,				//output file value, after "-o"
		arg_suffix_part = 0,			//"-s" - start part as suffix option
		arg_suffix_part_int = 0,		//value start suffix part, after "-s"
		arg_n = 0,						//"-n" option - n primes in file
		arg_n_int = 0,					//n value, after "-n"
		arg_b = 0,						//"-b" option - b primes in block
		arg_b_int = 0,					//b value, after "-b"
		arg_d = 0,						//"-d" option - delimiter between primes in block
		arg_d_value = 0,				//d value, after "-d"
		arg_bd = 0,						//"-bd" option - delimiter between blocks
		arg_bd_value = 0,				//bd value, after "-bd"
		arg_wfl = 0,					//"-wfl" option - write first last
		arg_f = 0,						//"-f" option - filling block option
		arg_blockSize = 0,				//blocksize value in after filling option, after "-f"
		parameters_changed = 0;			//default - 0 if running with all parameters, by command line,
										//and 1 - if some parameters was been changed.
										//need to display command line generator.

unsigned char null_byte = 0;			//define 0 as unsigned char.
unsigned char gap_1_byte; 				//define unsigned char 1 byte for positive integer [0-255]
unsigned short int prime_gap;

const int Step = 2;						//step for check = 2. (odd + 2 = odd).
const uint64_t minPrime = 3; 			//first odd prime
const uint64_t maxPrime = UINT64_MAX;

struct timeb starttime, endtime;
FILE
* fout,									//file for out 
* fchk,									//file for checking
* first_last_file; 						//file with first ans last primes in each part

// 8GB - memory limit
uint32_t	maxBlockSize = 1024 * 1024 * 1024, blockSize = 100000, bSize = 0;

uint64_t 	* Block = NULL,
			toCnt = 0,					//current number of consecutive primes
			toCnt_in_1_file = 0,		//current number of prime for one part
			x = 0, 						//x need for count steps of spliting filename, using strtok().
			n = 0,						//primes in file (default = 0, all primes in the same file)
			b = 10,						//primes in block (default 10). Block is a line with 10 primes by default.
			continue_n = 0;				//if need to continue n primes without writting...

char 		d[10] = "\t",				//delimiter in block (default '\t' in line). 10 primes in line are delimited with '\t'
			bd[10] = "\n"; 				//delimiter between blocks (default '\n' after each line). Each new line delimited with '\n'

uint64_t 	Ini,						//start number for starting sieve
			Fin,						//end number for finish sieve
			taskIni,					//start number for task
			taskFin,					//finish number for task
			Cur,						//current number
			suffix_int = 0,				//start part suffix (integer)
			code_d,						//code of delimiter between primes in block (line)
			code_bd,					//code of delimiter between blocks (lines)
			buf;						//buf for fwrite if primes saved as binary data

char	outfname[256] = "out",			//filename for writting primes - default "out"
		chkfname[256] = "chk",			//filename for check generating - default "chk"
		outfname_first_last[256], 		//file name for writting first and last prime in the each part
		temp_for_strtok[256], 			//variable to copy filename for strtok(), because function strtok modify the current string
		*ch,							//result of working strtok function (multichar)
		filename[256], 					//temp file name variable after strtok
		name_of_file[256], 				//this variable must contains only name of file, without prefixes and suffixes...
		temp_path_way_to_exe[256],		//temp variable to save filename only for this program (exe). In windows in argv[0] there is full path when double click on exe.
		path_to_7z[256] = "7z.exe", 	//full pathway to 7z.exe. Default just "7z.exe" and 7z called from pathway in environment variable "PATH"
		temp_string[256] = "";			//Just empty temp string defined - to save temporary data.

	/*
		folder[256] = "./primes/";	//relative pathway to the working folder (unused)
		path[256];//temp variable to save pathways

		//wanted to do...
		//strcpy(path, folder);
		//strcat(path, filename);
		//strcpy(filename, path);
	*/
//END - variables

//BEGIN - function to convert string number to uint64_t
static __inline__ uint64_t string_to_u64(const char * s) {
	//printf("string_to_u64: string - %s", s);
  uint64_t i;
  char c ;
  int scanned = sscanf(s, "%" SCNu64 "%c", &i, &c);
  if (scanned == 1){
	//printf("1string_to_u64: i = %" PRIu64, i);
	return i;
  }
  if (scanned > 1) {
	// TBD about extra data found
	//printf("2string_to_u64: i = %" PRIu64, i);
	return i;
  }
  // TBD failed to scan;
  //printf("3string_to_u64: return 0");
  return 0;
}
//END - function to convert string number to uint64_t

//BEGIN - function to check is string a uint64_number
static __inline__ int is_uint64_t(const char *uint64_t_as_string){ 	//string must contains number. Then return 1. Else - return 0.
	uint64_t uint64_t_value;										//define variable to save integer
	char temp_string_uint64_t[256];									//define temp variable to save integer as string

	uint64_t_value = string_to_u64(uint64_t_as_string);				//write value of number
	sprintf(temp_string_uint64_t, "%" PRIu64, uint64_t_value);		//write number as string in temp variable

	if(!strcmp(temp_string_uint64_t, uint64_t_as_string)){			//compare stings (0 if comparison true) !0 = 1 - true;
		/*
		printf (	"temp_string_uint64_t \'%s\' == integer %s."
					"Number is valid and gives uint64_t_value = %" PRIu64 "\n",
					temp_string_uint64_t, uint64_t_as_string, uint64_t_value
		);
		*/
		return 1;//return true
	}else{
		/*
		printf (	"temp_string_uint64_t \'%s\' != integer %s."
					"Number is not valid. uint64_t_value = %" PRIu64 "\n",
					temp_string_uint64_t, uint64_t_as_string, uint64_t_value
		);
		*/
		return 0;//else false

	}
}
//END - function to check is string a uint64_number


//BEGIN - function to escape string
char * str_escape(char str[])
{
	
		//To escape a single character, do this:
		//char x = '\t'; //not a string - character
		//char x2[256];	//temp variable to save character
		//sprintf(x2, "%c", x);//write character as string
		//puts(str_escape(x2));//escape this...
	
	char chr[3];
	char *buffer = malloc(sizeof(char));
	//chr[3] = '\0';
	memset(buffer, 0, sizeof(*buffer));
	//memset(buffer, '\0', sizeof(buffer));
	unsigned int len = 1, blk_size;
	//printf("BUFFER %s", buffer);

	while (*str != '\0') {
		blk_size = 2;
		switch (*str) {
			case '\n':
				strcpy(chr, "\\n");
				break;
			case '\t':
				strcpy(chr, "\\t");
				break;
			case '\v':
				strcpy(chr, "\\v");
				break;
			case '\f':
				strcpy(chr, "\\f");
				break;
			case '\a':
				strcpy(chr, "\\a");
				break;
			case '\b':
				strcpy(chr, "\\b");
				break;
			case '\r':
				strcpy(chr, "\\r");
				break;
			case '\?':
				strcpy(chr, "\\?");
				break;
			case '\'':
				blk_size = 3;
				strcpy(chr, "\\\'");
				break;
			case '\"':
				blk_size = 3;
				strcpy(chr, "\\\"");
				break;
			case '\\':
				blk_size = 3;
				strcpy(chr, "\\\\");
				break;
			default:
				//printf("default switch\n");
				sprintf(chr, "%c", *str);
				blk_size = 1;
				break;
		}
		len += blk_size;
		buffer = realloc(buffer, len * sizeof(char));
		strcat(buffer, chr);
		++str;
	}
	return buffer;
}
//END - function to escape string


//BEGIN - function to unescape string
char * str_unescape(char str[])
{
	//This function need to unescape string like "xxx\\tyyy", to get "xxx\tyyy" and 'xxx	yyy' as result.
	
	//char str[] = "xxxxx\\ty\\ny\\vyyy";
    //printf("str before: %s\n", str);
    
    for(int i=0; i< strlen(str);i++){
        if(str[i]=='\\'){
            switch (str[i+1]) {
                case 'n': str[i] = '\n'; break;
                case 't': str[i] = '\t'; break;
                case 'v': str[i] = '\v'; break;
                case 'f': str[i] = '\f'; break;
                case 'a': str[i] = '\a'; break;
                case 'b': str[i] = '\b'; break;
                case 'r': str[i] = '\r'; break;
                case '?': str[i] = '\?'; break;
            }
            memmove(&str[i+1], &str[i+2], strlen(str) - i+1);
        }
    }
    //printf("str after: %s", str);
	
	return str;
}
//END - function to unescape string


/*
//BEGIN - another version of function for unescape string
	//This function may supporting an unescape unicode characters \uXXXX and \xYY byte strings.
	//I see the unknown error when I try to using this...
char * str_unescape(char *dest, const char *src) //dest have some length, as src.
{
    int c;
    while ((c = *src++) != '\0') {
        if (c != '\\' || !*src) {
            *dest++ = c;   
            continue;
        }      
        
        c = *src++;
        switch (c) {
            case 'a': *dest++ = '\a'; break;
            case 'b': *dest++ = '\b'; break;
            case 'f': *dest++ = '\f'; break;
            case 'n': *dest++ = '\n'; break;
            case 'r': *dest++ = '\r'; break;
            case 't': *dest++ = '\t'; break;
            case 'v': *dest++ = '\v'; break;
            case '0': *dest++ = '\0'; return '\0'; // Because no need to continue if the end of string...
            case '\'': *dest++ = '\''; break;
            case '"': *dest++ = '"'; break;
            case '\\': *dest++ = '\\'; break;
            case '\?': *dest++ = '\?'; break;
            
            case 'x': {
                static const char xvalue[] = {
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,
                    10, 11, 12, 13, 14, 15,
                };
                if (isxdigit(src[0]) && isxdigit(src[1])) {                   
                    *dest++ = xvalue[toupper(src[0]) - '0'] << 4 | xvalue[toupper(src[1]) - '0'];
                    src += 2;
                    break;
                }
                // FALLTHROUGH (invalid hex sequence).
            }
            default:
                *dest++ = '\\';
                *dest++ = c;
                break;
        }
    }
    *dest = '\0';
	//return dest;
	
	//test
	//int main(void)
	//{
	//	char x[] = "xx\\\\xxx\\ty\\r\\n\\\'\\\?\\\"hello\\x22\\x7e\\xf\u1ed8";
	//	printf("x before: %s\n", x);
	//    unescape_it_right(x, x);
	//    printf("x after: %s", x);//working fine
	//}
}
*/

//BEGIN - second function to return char by code (just return symbol by charcode, or string)
char * return_symbol_by_code(char code[])//this function return escaped symbol as character by charcode of this.
{
//input: code - ASCII string. with code, like "07"
//output: temp_string - char or multichar string, if not a code.

int code_int;				//temp variable to save charcode as int
//char temp_string[256];	//1 char string to return symbol, or multichar, if this is a multichar string.
//this was been defined previously

/*
For delimiters -d and -bd can be used next values (one byte):
___________________________________________________________________
(decimal code) - description
___________________________________________________________________
 7 - \a audible bell					byte 0x07 in ASCII encoding
 8 - \b backspace					   	byte 0x08 in ASCII encoding
 9 - \t horizontal tab				  	byte 0x09 in ASCII encoding
10 - \n line feed - new line			byte 0x0a in ASCII encoding
11 - \v vertical tab					byte 0x0b in ASCII encoding
12 - \f form feed - new page			byte 0x0c in ASCII encoding
13 - \r carriage return				 	byte 0x0d in ASCII encoding
34 - \" double quote					byte 0x22 in ASCII encoding
39 - \' single quote					byte 0x27 in ASCII encoding
63 - \? question mark				   	byte 0x3f in ASCII encoding
92 - \\ backslash					   	byte 0x5c in ASCII encoding
___________________________________________________________________

Only decimal code can be a char value for escaped delimiter, here...
Or any another symbols can be a delimiters, too...

Multiple delimiters supporting (like '_,;_'), but without escaped characters.
Multiple delimiters escaped characters supporting (like "_,\t_")
*/
	//printf("strlen code: %d",strlen(code));
	if(strlen(code)>2){//if code contains over 2 digits
		//printf("return code as is ... %s\n", code);
		return code; //return string as is
	}else{//else, if code contains two symbols (for escaped characters), this can be a char code as ASCII-text.
		int code_int = atoi(code);	//convert ASCII -> to int
		//printf("code = %s, code_int = %d\n", code, code_int);
		switch(code_int){//compare charcode value with codes for escaped characters						
			case 7 ... 13: case 34: case 39: case 63: case 92: sprintf(temp_string, "%c", code_int); break;	//return one char in this cases
			default: strcpy(temp_string, code); break;														//or write full string
		}
		//printf("temp_string = %s ... return this.\n", temp_string);
		return temp_string;																					//return one char or multistring.
	}
}
//END - second function to return char by code (just return symbol by charcode, or string)

//BEGIN structure
	//This structure will be returned from checkfile function, like an array.
	//This contains many values.
typedef struct {
	uint64_t 	count,
				lines_count,
				first_number,
				last_number,
				seems_like_full;
}RESULT_CHECK_FILE;
//END structure

//BEGIN function to return empty structure
	//if file not found - this will be returned.
RESULT_CHECK_FILE *return_empty_structure(){
	RESULT_CHECK_FILE *empty = malloc(sizeof(*empty)); //allocate memory for structure
	empty->count = 0;
	empty->lines_count = 0;
	empty->first_number = 0;
	empty->last_number = 0;
	empty->seems_like_full = 0;
	return (empty);
}
//END function to return empty structure

//BEGIN - check file function
RESULT_CHECK_FILE *check_file(char filename[256]){
	//	This function need to check file, if this exists and if not full.
	//	This checking need to continue writting primes in the not full part,
	//	or not writting if part is already full.
	//	If part not full, there is inserting delimiter block with text,
	//	and then program continue writting the primes in this file...
	
	uint64_t	count = 0,						//count numbers
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
	if(x64==1 || prime_gaps==1){
		file = fopen(outfname, "rb");
	}else if(x64==0 && prime_gaps==0){
		file = fopen(outfname, "r");
	}else{
		printf("incorrect x64 value. Only 1 and 0 available...");
	}
	//printf("\n in check function - file fopen filename r - case7\n");

	//printf("\n char bd - delimiter for blocks = %d", c);

	if(file!=NULL && !ferror(file)){ 	//if file opened successfully - check this...
		if(!x64 && !prime_gaps){			//if need read as text with delimiters
			while(!feof(file)){ 				//read file as text
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
		}else if(x64 && !prime_gaps){									//if primes as 64 bit numbers, without any delimiters
			unsigned long long int value;										//define 64 bit integer
			while(fread(&value, sizeof(value), 1, file) == 1){					//read file as binary data. File opened as 'rb' before... 
				count++;														//count prime number
				if(count==1 || save_next_first){							//if need save first of if this a first prime
					if(current_number==2){save_next_first = 1; continue;}		//if this number = 2(even), shift to 3 - first odd prime number.
					first_number = value;										//save this as first number
					struct_chkfile_result->first_number = value;				//write in structure
					if(save_next_first){save_next_first=0;}						//reset this	
				}
				if(!save_last){save_last = 1;}									//say true to save last number...
			}
			//test output
			//printf("value = %d, struct_chkfile_result->first_number = %d\n", value, struct_chkfile_result->first_number);
			
			//write first last numbers
			if(save_last){
				last_number = value;											//save this here
				struct_chkfile_result->last_number = value;						//and in structure
				save_last = 0;													//set this false, because last number already saved.
			}
			
		}else if(!x64 && prime_gaps){									//if file contains prime gaps
			unsigned long long prime_num;										//define 8 bytes integer (64 bit)

			if(fread(&prime_num, sizeof(prime_num), 1, file) == 1){			//read first - 8 bytes at beginning of file.
				count=1;														//there is a first prime as 64 bit integer.
				first_number = prime_num;										//save this as first prime number
				struct_chkfile_result->first_number = prime_num;				//write in structure
				if(!save_last){save_last = 1;}									//say true to save last number...
			}

			if(gaps2byte && !gaps1byte){								//if file contains full prime gap as 2 bytes
				unsigned short int value;									//define 2-bytes integer

				while(fread(&value, sizeof(value), 1, file) == 1){			//read file by 2 bytes
					count++;												//count primes
					prime_num = prime_num + value; 							//calculate current prime number

					if(count==n || count > n){save_last = 1; break;}		//break if n, and set save_last - true;
				}
			}else if(gaps1byte && !gaps2byte){							//if file contains half gap as 1 byte

				unsigned char value;									//define 1 byte integer
				unsigned short int gap_as_two_bytes;					//define 2 bytes integer (not all prime gaps can be writted in 1 byte)
				uint64_t half_prime_gap64;								//just define 64-bit integer.

				while(fread(&value, sizeof(value), 1, file) == 1){				//continue read file by 1 byte
					if(value==0){ 												//if value = 0, then next two bytes contains half gap with value - over 255
						if(fread(&gap_as_two_bytes, sizeof(gap_as_two_bytes), 1, file)){		//if readed
							count++;															//prime count + 1
							half_prime_gap64 = gap_as_two_bytes;								//save prime gap as 64 bit integer
						}
					}else{														//else
						count++;													//count this byte as prime
						half_prime_gap64 = value;									//and save prime gap as 64 bit integer from this byte.
					}
					prime_num = prime_num + half_prime_gap64*2;					//calculate current prime
					
					if(count==n || count > n){save_last = 1; break;}			//break if n primes.
				}
				
				//First prime gap from 2 to 3 - is odd gap = 1.
				//This was been doubled in the cycle and added.
				//And now prime_num is odd.
				if(first_number==2){prime_num--;}

			}else{
				printf("check_file, gaps condition, condition - incorrect case...");
			}
			
			//write first last numbers
			if(save_last){
				unsigned long long last_prime_num;								//define 8 bytes integer
				last_prime_num = prime_num;										//last prime is previous prime, because last prime gap between this and next.

				fread(&prime_num, sizeof(prime_num), 1, file);

				if(prime_num==last_prime_num){								//if this is last
					last_number = last_prime_num;								//save this here
					struct_chkfile_result->last_number = last_prime_num;		//and in structure
					save_last = 0;												//set this false, because last number already saved.
				}else{																			//else
					printf("prime_num - incorrect, in file_check function .\n");				//show an error
					printf("prime_num = %d, last_prime_num = %d", prime_num, last_prime_num);	//and values
				}
			}
			
		}else{
			printf("check_file, fread condition - incorrect case...");
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

	//write values in structure
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
//END - check file function

//BEGIN - function to save checkpoint in file chk
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
//END - function to save checkpoint in file chk

//BEGIN - function to read checkpoint from file chk
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

		//printf("system_pause 1\n");
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
//END - function to read checkpoint from file chk

//BEGIN - function to free block
void free_block(void)
{
	free(Block);
}
//END - function to free block

//BEGIN - function to sieve prime numbers
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
//END - function to sieve prime numbers

//BEGIN - function initialize task for sieving
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
//END - function initialize task for sieving

//BEGIN - function to display progress bar
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
//END - function to display progress bar

//BEGIN - function to update filename in argv[0]
	//before this contains fullpath to program, after - filename of program only.
int return_filename(int argc, char** argv){
	//Extract filename from fullpath to this exe in argv[0] for windows (this is fullpath if double click on exe)
	//and save back in argv[0] extracted filename only for this program from this full path	

	//printf("\nFilename in argv[0] = %s\n", argv[0]);
	strcpy(temp_string, str_escape(argv[0]));		//escape slashes to split by "\\"	and copy to temp variable
	//printf("escaped pathway = %s", temp_string);

	//get filename passwd.c from pathway "C:\\etc\\password.c" in temp_string
	//char temp_path_way_to_exe[256]; //temp variable	defined earlier
	char *ch;						//strtok
	ch = strtok(temp_string, "\\"); //delimiter
	while (ch != NULL) {
		strcpy(temp_path_way_to_exe, ch);
		//printf("%s\n", ch);
		ch = strtok(NULL, "\\");	//delimiter
	}
	//now temp_path_way_to_exe contains filename only
	strcpy(argv[0], temp_path_way_to_exe); //write to first argument
	//printf("Now filename from pathway in argv[0] = %s\n", argv[0]);
	//return temp_path_way_to_exe;
}
//END - function to update filename in argv[0]

//BEGIN - function to print usage info
int print_usage(int delimiters) //value of delimiters: 0 - show all info, 1 - only arguments, 2 - only delimiters.
{
#ifdef _WIN32
	char pref[3] = ">";
#elif _WIN64
	char pref[3] = ">";
#elif __linux__ || unix || __unix__ || __unix
	char pref[3] = "./";
#endif // __linux__
	if(delimiters==0){delimiters = 1;}//if this parameter was been undefined - just print all info
	if(delimiters==1){//if need to print addition info, about deilmiters formats
		//additional parameters added here - "-s", "-n", "-b", "-d","-bd", "-wfl" + desctiption of codes for delimiters for "-d" and "-bd"
		fprintf(stderr, "Usage: %s%s <low> <high> [switches]\n", pref, temp_path_way_to_exe);
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
		fprintf(stderr, "-x64\t\t\tJust check to write as 64-numbers numbers,\n\t\t\twithout any delimiters... (default value: disabled)\n\n");
		fprintf(stderr, "-f [size]\t\tfactoring block size\n\t\t\t(default value: %" PRIu32 ")\n\n", blockSize);
		fprintf(stderr, "-7z [path]\t\tUse 7z compressing on the fly or not.\n\t\t\t"
						"In next argument you can specify text string \"disabled\"\n\t\t\t"
						"to disable this, or the string with path to 7z.exe\n\t\t\t"
						"(default path: \"7z.exe\" from environment variable PATH)\n\t\t\t"
						"Use double (and, or) single slashes\n\t\t\t"
						"(and, or) double quotes in the path\n\t\t\t"
						" (like \"C:\\\\7-zip\\\\7z.exe\",\n\t\t\t"
						" or \"C:\\7-zip\\7z.exe\",\n\t\t\t"
						" or \"\"\"C:\\\\\\\\7-zip\\7z.exe\"\"\",\n\t\t\t"
						" or without quotes)\n\n"
		);
		fprintf(stderr, "-gaps\t\t\tJust check to save primes as prime gaps\n\t\t\t"
						"- without any delimiters.\n");
		fprintf(stderr, "-gaps2byte\tJust check to save primes as 2-byte full prime gaps\n");
		fprintf(stderr, "-gaps1byte\tJust check to save primes as 1-byte half prime gaps\n\t\t"
						"as binary data. Because all gaps are even nubmers.\n\n");

		delimiters = 2;//set this variable to 2
	}
	if(delimiters==2){//if delimiters only - just print info about codes for delimiters.
		fprintf(stderr,
			"\n"
			"For delimiters -d and -bd can be used next values (one byte):\n"
			"___________________________________________________________________\n"
			"(decimal code) - description\n"
			"___________________________________________________________________\n"
			" 7 - \\a\taudible bell\t\t\tbyte 0x07 in ASCII encoding\n"
			" 8 - \\b\tbackspace\t\t\tbyte 0x08 in ASCII encoding\n"
			" 9 - \\t\thorizontal tab\t\t\tbyte 0x09 in ASCII encoding\n"
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
			"Multiple delimiters escaped characters supporting (like \"_,\t_\"').\n"
		);
	}
}
//END - function to update filename in argv[0]

//BEGIN - main function...
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

			//printf("system_pause 2\n");
			system("pause"); //pause program to don't close window if double click on exe
			//printf("Press Ctrl+C or Enter to exit...");
			//getchar(); //wait char to don't close window if double click on exe

			exit(EXIT_FAILURE);
		}
		sprintf(OS, "%s %s", name.sysname, name.release);
	#endif // __linux__

	//change fullpath to this exe in argv[0] to filename only, and save this in argv[0];
		//printf(	"before: argv[0] = %s", argv[0]);							//here fullpath to exe if program started by double click in windows
		//printf( ", temp_path_way_to_exe = %s\n\n",temp_path_way_to_exe);		//here is empty string
	return_filename(argc, argv);
		//printf(	"before: argv[0] = %s", argv[0]);								//here is filename of exe only
		//printf( ", temp_path_way_to_exe = %s\n\n",temp_path_way_to_exe);		//and here too.
		//this value usied in print_usage and command line generator.

	//print this at beginning...
	fprintf(stderr, "\nCYCLOP (C - Consecutive List Of Primes) %s (%s)\nCopyright 2018, Alexander Belogourov aka x3mEn\n\n", VERSION, OS);

/*
	if (argc < 3) {
		print_usage(0);
		printf("\n\nNot enough arguments specified...\n");

		//printf("system_pause 3\n");
		system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);
	}
*/

	//copy char codes from default delimiters variables.
	code_d = (uint64_t) d[0];
	code_bd = (uint64_t) bd[0];

	if(																								//if
			(argc<3)																				//min and max number not specified, and if this program started without command line parameters
		||	(!argv[1] && !argv[2])																	//or if one from this is null
		||	string_to_u64(argv[2])<=string_to_u64(argv[1])											//or if max <= min
	){																								//input max-min...

		print_usage(0); 																			//show all info in the beginning...

		if(argc<3){
			printf(	"\n\nNot enough arguments specified."
					"Only %d argument, including this program...\n", argc);							//show argc
		}else if(!argv[1] && !argv[2]){																//Else if here. Try to read not founded argument return the error.
			printf("\nMin or max number is null...\n");
		}
		else if(string_to_u64(argv[2])<=string_to_u64(argv[1])){
			printf("\nMax start number - lesser or equals min...\n");
		}

		while((Ini == taskIni) && (taskFin == Fin) && (Ini>=Fin && taskIni>=Fin)){
			if(!Ini && !taskIni){																	//first - min
				printf("Enter start natural number to finding prime numbers (positive value): ");	//from input
				fgets(temp_string, 255, stdin);														//save string to temp variable
				strtok(temp_string, "\n");															//discard '\n'

				if(!is_uint64_t(temp_string)){
					printf("%s not a 64-bit number. Try again...\n", temp_string); continue;
				}

				taskIni = Ini = string_to_u64(temp_string);											//then writting as uint64_t
			}
			if(!Fin && !taskFin){																	//secont - max
				printf("Enter end natural number to finding prime numbers (positive value): ");		//from input
				fgets(temp_string, 255, stdin);														//save string to temp variable
				strtok(temp_string, "\n");															//discard '\n'

				if(!is_uint64_t(temp_string)){
					printf("%s not a 64-bit number. Try again...\n", temp_string); continue;
				}

				taskFin = Fin = string_to_u64(temp_string);											//then writting as uint64_t
			}
			if((Ini == taskIni) && (taskFin == Fin) && (Ini>=Fin && taskIni>=Fin)){
				printf("Incorrect input. Start min >= max. Try again...\n");
				taskIni = Ini = taskFin = Fin = 0;
			}else{
				printf(	"Initialize numbers - accepted.\n");
				//printf(	"You can try to run this program with next parameters:\n"				//display LOW HIGH command line
				//			"\t> %s %" PRIu64" %" PRIu64 "\n", argv[0], Ini, Fin);
			}
		}
	}else{																							//if this two arguments specified correctly
		taskIni = Ini = string_to_u64(argv[1]);														//just set initialize variables 
		taskFin = Fin = string_to_u64(argv[2]);														//as uint64_t
	}

	//printf(	"\n\n Ini = %" PRIu64 ", taskIni = %" PRIu64										//print this...
	//			", Fin = taskFin = %" PRIu64 "\n\n", Ini, taskIni, Fin);


	sprintf(outfname, "out_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);		//set filename for out data
	sprintf(chkfname, "chk_%019" PRIu64 "_%019" PRIu64, taskIni, taskFin);		//set filename for check data

//BEGIN parse the specified command line arguments
	//fprintf(stderr, "number of arguments: %d\n\n", argc);						//print number of arguments...
	for (int i = 3; i < argc; i++) {
		//fprintf(stderr, "%d - Now will be worked arg %s\n", i, argv[i]); 		//print value for current argument.

		if (!strcmp(argv[i],"-p")) {progress = arg_progress = 1; continue;}		//progress
		if (!strcmp(argv[i],"-gaps2byte")) {									//write full prime gaps - 2 bytes
			prime_gaps = arg_gaps = arg_gaps2byte = gaps2byte = 1;
			arg_gaps1byte = gaps1byte = 0;
			continue;
		}
		if (!strcmp(argv[i],"-gaps1byte")) {									//write half prime gaps - 1 byte 
			prime_gaps = arg_gaps = arg_gaps1byte = gaps1byte = 1;
			arg_gaps2byte = gaps2byte = 0;
			continue;
		}
		if (!strcmp(argv[i],"-gaps")){											//"-gaps" save primes as prime gaps without specify format
			prime_gaps = arg_gaps = 1;											//just check this
			arg_gaps2byte = arg_gaps1byte = gaps2byte = gaps1byte = 0;			//and leave this - nulled
			continue;
		}
		if (!strcmp(argv[i],"-x64")) {x64 = arg_x64 = 1; continue;}				//save primes as 64-bit integer
		if (!strcmp(argv[i],"-7z")) {arg_7z = make7z = 1; continue;}			//7z packing
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-7z")) {					//specified path to 7z.exe
			arg_7z_value = 1;
			sprintf(path_to_7z, "%s", argv[i]);									//copy path
			if(!strcmp(argv[i], "disabled")){make7z = 0;}
			continue;
		}
		if (!strcmp(argv[i],"-o")) {output = arg_output = 1; continue;}			//ouput
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-o")) {						//output filename
			arg_ouput_file = 1;
			sprintf(outfname, "%s", argv[i]);
			sprintf(name_of_file, "%s", argv[i]);
			continue;
		}
		if (!strcmp(argv[i],"-s")) {arg_suffix_part = 1; continue;}				//start suffix integer - number of the part
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-s")) {
			arg_suffix_part_int = 1;
			suffix_int = string_to_u64(argv[i]); 								//copy this as int from string

			//add suffix to outfname
			x = 0; 																//start - null
			strcpy(temp_for_strtok, outfname); 									//copy outfname to temp_for_strtok
			ch = strtok(temp_for_strtok, ".");									//split by "."
			sprintf(filename, "%s", ch);										//filename only to "filename" variable

			while (ch != NULL) {												//while not null
				x++;																//x++
				if(x > 1){															//if x>1
					sprintf(filename, "%s_part" "%" PRIu64, filename, suffix_int);		//add part to filename prefix
					sprintf(filename, "%s.%s", filename, ch);							//add extension
				}
				ch = strtok(NULL, ".");												//again...
			}
			sprintf(outfname, "%s", filename);										//write new filename to outfname
			//now outfname have start part as suffix

			continue;
		}
		if (!strcmp(argv[i],"-n")) {arg_n = 1; continue;}						//if was been specified a number of primes in each part
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-n")) {						//and next is this number
			arg_n_int = 1;
			n = string_to_u64(argv[i]);											//save n as int64 from string
			continue;
		}
		if (!strcmp(argv[i],"-bd")) {arg_bd = 1; continue;} 					//bd - before b, because -bd contains -b
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-bd")) {					//bd - is a delimiter between the blocks (lines)
			arg_bd_value = 1;
			strcpy(bd, return_symbol_by_code(argv[i]));
			code_bd = string_to_u64(bd);
			
	/*
			code_bd = string_to_u64(argv[i]); 									//copy string as integer
			//if this is charcode from escaped character
			switch(code_bd){														//compare code
				case 7 ... 13: 														//if int from 07 to 13
				case 34: case 39: case 63: case 92: 								//or if one from this codes
					sprintf(bd, "%c", code_bd); break; 									//save this as char delimiter
				default: sprintf(bd, argv[i]); break;								//else, return delimiter - as text
			}
			//now escaped character as char
	*/
			//if delimiter multistring
			if(!strcmp(bd, argv[i])){
				//printf("before unescape, bd = %s", bd);
				sprintf(bd, str_unescape(argv[i]));								//try to unescape this using first function
				
				//sprintf(bd, str_unescape(argv[i], argv[i]));						//try to unescape this using second function. I see the unknown error when I try to using this.
				
				//printf("after unescape, bd = %s", bd);
			}else{
				//printf("\n not bd condition\n");
			}
			//now string, like "\t\"\t\?\t" is unescaped -> '	"	?	'
			
			continue;
		}
		if (!strcmp(argv[i],"-d")) {arg_d = 1; continue;}						//if was been specified delimiter between primes in each block(line)
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-d")) {						//d - is a delimiter between primes in block (line)
			arg_d_value = 1;
			
			strcpy(d, return_symbol_by_code(argv[i]));
			code_d = string_to_u64(d);
			
	/*
			code_d = string_to_u64(argv[i]);									//copy as int, if int
			switch(code_d){
				case 7 ... 13: case 34: case 39: case 63: case 92: sprintf(d, "%c", code_d); break; //return char in this cases
				default: sprintf(d, argv[i]); break;								//else return delimiter - as text
			}
	*/
			//now escaped character as char
			
			//if delimiter multistring
			if(!strcmp(d, argv[i])){ //and if this is the same string
				//printf("before unescape, d = %s", d);
				
				sprintf(d, str_unescape(argv[i]));			//try to unescape this using first function, if escaped
				
				//sprintf(d, str_unescape(argv[i],argv[i]));		//try to unescape this using second function, if escaped. I see the unknown error when I try to using this.
				
				//printf("after unescape, d = %s", d);
			}else{
				//printf("\n not d condition\n");
			}
			//now string, like "\t\"\t\?\t" is unescaped -> '	"	?	'
			
			continue;
		}
		if (!strcmp(argv[i],"-b")) {arg_b = 1; continue;} 						//if was been specified a number of primes in each block
		if (argv[i][0] != '-' && !strcmp(argv[i-1],"-b")) {						//and this number after
			arg_b_int = 1;
			b = string_to_u64(argv[i]);					 							//copy as int from string
			continue;
		}
		if (!strcmp(argv[i],"-wfl")) { 											//if need to "write first last"(wfl)
			output_first_last = arg_wfl = 1; 									//this true
			strcpy(temp_for_strtok, outfname);									//copy outfname to temp variable

			if(suffix_int==0){								//if suffix_int not specified, then filename not contains suffix _part
				ch = strtok(temp_for_strtok, ".");				//split by "."
			}
			else{											//if start suffix_int specified, then filename contains suffix _part
				ch = strtok(temp_for_strtok, "_");				//split by "_"
			}
			sprintf(filename, "%s", ch); 					//write file name only to "filename"
			strcpy(name_of_file, filename);					//copy filename only to name_of_file
			
			//add suffix
			x = 0;																//start 0
			while (ch != NULL) {												//while not null
				x++;																//x++
				if((suffix_int==0 && x > 1)||(suffix_int!=0 && x > 2)){				//if this
					sprintf(filename, "%s_first-last", filename);						//add suffix _first-last
					sprintf(filename, "%s.%s", filename, ch);							//add extension
				}
				ch = strtok(NULL, ".");												//again
			}
			sprintf(outfname_first_last, filename);								//write new filename to outfname_first_last
			//now filename have suffix "_first-last"
			
			continue;
		}
		if (!strcmp(argv[i],"-f")) {arg_f = 1; continue;}			//if filling block option specified
		if (string_to_u64(argv[i]) && !strcmp(argv[i-1],"-f")) {	//and if blockSize in next argument
			arg_blockSize = 1;
			blockSize = min(maxBlockSize, string_to_u64(argv[i]));	//select minimum from this or max block size.
			continue;
		}

		//when incorrect argument was been specified
		print_usage(0);												//print full usage info
		printf("\n\n\tIncorrect argument was been specified in command line...\n\n");		//print this

		system("pause"); //pause program to don't close window if double click on exe

		//printf("Press Ctrl+C or Enter to exit...");
		//getchar(); //wait char to don't close window if double click on exe

		exit(EXIT_FAILURE);	//and exit
	}
//END parse the specified command line arguments

/*	
	//test running: clop 1 15000 -p -o result.txt -s 123 -n 1000 -b 7 -d 34 -bd 10 -wfl -f 100
	//test output all variables, changed by arguments...
	printf(	"\n\n\n"
			"Ini = %" PRIu64 ", taskIni = %" PRIu64 "\n"
			"Fin = %" PRIu64 ", taskFin = %" PRIu64 "\n"
			"progress = %d, output = %d, suffix_int = %" PRIu64 "\n"
			"outfname = %s" "\n" "n = %" PRIu64 ", b = %" PRIu64 "\n"
			"d = \"%s\", code_d = %" PRIu64 ", bd = \"%s\", code_bd = %"PRIu64"\n"
			//"d = %s, bd = %s, code_d = %d, code_bd = %d\n"
			", output_first_last = %d, outfname_first_last = %s"
			", blockSize = %" PRIu32 ", make7z = %d, path_to_7z = %s"
			"\n\n\n",
			Ini, taskIni, Fin, taskFin,
			progress, output, suffix_int, outfname, 
			n, b,
			str_escape(d), code_d, str_escape(bd), code_bd,
			output_first_last, outfname_first_last,
			blockSize, make7z, path_to_7z
	);
	//OK
*/




/*
	//Next variables have the value 1 or 0 - just for check/uncheck ...
	//0 by default, 1 when argument specified in command line.
		arg_progress = 0,				//"-p" - progress option
		arg_gaps = 0,					//"-gaps" options - write primes as prime gaps, without specify format.
		arg_gaps2byte = 0,				//"-gaps2byte" option - write primes as 2 byte prime gaps
		arg_gaps1byte = 0,				//"-gaps1byte" option - write primes as 1-byte half prime gaps
		arg_x64 = 0,					//write as 64-bit integers - option
		arg_7z = 0,						//"-7z" option
		arg_7z_value = 0,				//full pathway to 7z.exe, after "-7z"
		arg_output = 0,					//"-o" - output option
		arg_ouput_file = 0,				//output file value, after "-o"
		arg_suffix_part = 0,			//"-s" - start part as suffix option
		arg_suffix_part_int = 0,		//value start suffix part, after "-s"
		arg_n = 0,						//"-n" option - n primes in file
		arg_n_int = 0,					//n value, after "-n"
		arg_b = 0,						//"-b" option - b primes in block
		arg_b_int = 0,					//b value, after "-b"
		arg_d = 0,						//"-d" option - delimiter between primes in block
		arg_d_value = 0,				//d value, after "-d"
		arg_bd = 0,						//"-bd" option - delimiter between blocks
		arg_bd_value = 0,				//bd value, after "-bd"
		arg_wfl = 0,					//"-wfl" option - write first last
		arg_f = 0,						//"-f" option - filling block option
		arg_blockSize = 0,				//blocksize value in after filling option, after "-f"
		parameters_changed = 0;			//default - 0 if running with all parameters, by command line,
										//and 1 - if some parameters was been changed.
										//need to display command line generator.
*/



//BEGIN - DIALOG FOR INPUT PARAMETERS
	//printf("argc = %d", argc);
	if(
			(
				arg_gaps==1											 	//if -gaps specified
				&& 	(arg_gaps2byte==0 && arg_gaps1byte==0)				//without format (1 byte, 2 byte)
				&& 	(!arg_d && !arg_bd)									//and if delimiters not specified
			)
		||	(!arg_n && !arg_n_int)										//or if n not specified
		||	(!arg_progress)												//or if without progress
		||	(!arg_output && !arg_ouput_file)							//or if output and not specified
		||	(!arg_suffix_part && !arg_suffix_part_int)					//or if start suffix not specified
		||	(!arg_d && !arg_bd && !arg_x64)								//or if x64 not specified, when delimiters not specified
		||	(!arg_7z)													//or if 7z not specified and default
		||	(arg_7z && !arg_7z_value)									//or if default path to 7z and not specified, when 7z specified
		||	(!arg_d && !arg_d_value)									//or if d not specified
		||	(!arg_b && !arg_b_int)										//or if b not specified
		||	(!arg_bd && !arg_bd_value)									//or if bd not specified and default
		||	(!arg_wfl && n!=0)											//or if write first last not specified when n not 0
		||	(!arg_f && !arg_blockSize)									//or if blockSize default and not specified
	){	//this can be changed to get full command line from generator...
	
			//If not all arguments specified - then can be available to setup this on start, in next dialog...
			//if -gaps specified without format - also run this to specify...
	
		//Checking default values of variable.
		//Maybe the arguments not specified, so user can set this in next dialog...
	
		char input[256];					//full input for fgets(input,255,stdin)
		char choose;						//variable to save one char from input - last symbol (y/n, Y/N)
		char by_default[256];				//"(by default)" - text for default value
		int no_display_now = 0;				//1 if no need to display, else - 0. Need for skip display when char is '\n'
		char dstring[256];					//temp variable to save char as string
	
		if(arg_progress==0){
			strcpy(by_default,"(by default)");
			while(1){														//cycle without end
				if(no_display_now!=1){										//if need to display
					printf("\nProgress bar - disabled %s.\n", by_default);
					printf("Do you want to enable progress bar? (Y/N): ");		//show ask message
					fgets(input, 255, stdin);									//save input as string
					strtok(input, "\n");										//discard '\n'
					choose = (int) input[strlen(input)-1];						//save last char here
				}else{														//if no need to display 
					no_display_now = 0;											//display next iteration
					continue;													//and continue
				}
				if ( choose == 'y' || choose == 'Y' ){						//if last char is 'y' or 'Y'
					progress = arg_progress = !progress;
					if(progress==1){printf("\tprogress = %d - enabled.\n\n", progress);}
					else if(progress==0){printf("\tprogress = %d - disabled.\n\n", progress);}
					else{printf("progress = %d, incorrect value (only 1 or 0 available).\n\n", progress);}
	
					//confirm??
					printf("Confirm? (Y/N): ");					//show ask message
					fgets(input, 255, stdin);					//save string to temp variable
					strtok(input, "\n");						//discard '\n'
					choose = (int) input[strlen(input)-1];		//save last char here
					if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
					else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
					else {printf("Incorrect input... Please, try again...\n");}	//show error message...
					//end confirm...
				}
				else if ( choose == 'n' || choose == 'N' ){					//else if 'n' or 'N'
					printf("Progress disabled %s.\n\n", by_default);
					break;													//and break from cycle
				}else if(choose == '\n'){							//if just enter and entered '\n' symbol
					no_display_now = 1;									//do this
					continue;											//and continue
				}
				else{														//else if any another symbol...
					printf("Incorrect input... Please, try again...\n");	//show error message
					//and try again...
				}
			}
		}
		if(arg_output==0){
			strcpy(by_default,"(by default)");
			while(1){//cycle without end
				if(no_display_now!=1){										//if need to display
					printf(	"\nOutput disabled %s.\n"							//show ask message
							"This means primes not writting to the file when generating...\n"
							"Do you want to enable output to the file? (Y/N): ",
							by_default
					);
					fgets(input, 255, stdin);									//save string to temp variable
					strtok(input, "\n");										//discard '\n'
					choose = (int) input[strlen(input)-1];						//save last char here
				}else{														//if no need to display 
					no_display_now = 0;											//display next iteration
					continue;
				}
				if ( choose == 'y' || choose == 'Y' ){						//if last char is 'y' or 'Y'
					output = arg_output = !output;								//change value
					if(output==1){printf("\toutput = %d - enabled.\n\n", output);}						//print if true
					else if(output==0){printf("\toutput = %d - disabled.\n\n", output);}				//print if false
					else{printf("output = %d, incorrect value (only 1 or 0 available).\n\n", output);}	//print if incorrect
	
					//confirm??
					printf("Confirm? (Y/N): ");									//show ask message
					fgets(input, 255, stdin);									//save string to temp variable
					strtok(input, "\n");										//discard '\n'
					choose = (int) input[strlen(input)-1];						//save last char here
					if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}	//print if true
					else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}					//print if false
					else {printf("Incorrect input... Please, try again...\n");}								//show error message...
					//end confirm...
				}
				else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
					printf("Output disabled %s...\n\n", by_default);
					break;
				}else if(choose == '\n'){								//if just enter '\n' symbol
					no_display_now = 1;										//do this
					continue;												//and continue
				}
				else{														//else if any another symbol...
					printf("Incorrect input... Please, try again...\n");		//show error message, and continue...
				}
			}
		}
		if(output==1){
			if(!strcmp(name_of_file,"") && !arg_ouput_file){					//if this variable not contains anything
				strcpy(name_of_file, outfname);
				strcpy(by_default,"(by default)");
				while(1){														//cycle without end
					if(no_display_now!=1){											//if need to display
						if(!strcmp(outfname, "\n")){									//if outfname = '\n'
							strcpy(outfname, name_of_file);									//return to default
							strcpy(by_default,"(by default)");								//and set this...
						}
						printf(	"\nName of file for writting is \"%s\" %s\n"			//show ask message
								"Do you want to change name of this file? (Y/N): ", outfname, by_default);
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{															//if no need to display 
						no_display_now = 0;												//display next iteration
						continue;														//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){									//if last char is 'y' or 'Y'
						printf("Enter the filename (name.extension): ");						//do this
						fgets(outfname, 255, stdin);											//save string to temp variable
						strtok(outfname, "\n");													//discard '\n'
						printf("\toutfname successfully changed to: \"%s\". \n\n", outfname);	//print result
						strcpy(name_of_file, outfname);											//update name_of_file
						strcpy(by_default," ");													//not default now
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
						}
					else if ( choose == 'n' || choose == 'N' ){		//else if 'n' or 'N'
						printf("Out_file_name is %s\n", outfname); 		//default value
						break;											//and break from the cycle
					}else if(choose == '\n'){//if just enter '\n' symbol
						no_display_now = 1;								//do this
						continue;										//and continue
					}
					else{											//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
			if(n==0 && !arg_n_int){
				strcpy(by_default,"(by default)");
				while(1){														//cycle without end
					if(no_display_now!=1){											//if need to display
						printf(	"\nn = 0. This is number of primes in one part.\n"		//show ask message
								"All primes are saving in 1 file %s.\n"
								"Do you want to set this value? (Y/N): ", by_default);
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{															//if no need to display 
						no_display_now = 0;												//display next iteration
						continue;														//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){							//if last char is 'y' or 'Y'
						while(!is_uint64_t(input) && input != "y" && input != "Y"){	//while not y/Y and not number
							printf("Enter the number of primes in one file. n: ");		//do this
							fgets(input, 255, stdin);									//save string to temp variable
							strtok(input, "\n");										//discard '\n'
							if(is_uint64_t(input) && input != "y" && input != "Y"){		//check again
								break;														//break if true
							}else{
								printf("This must be a number (digits only)...\n");			//else display this and try again
							}
						}
																							//after input nubmer
						arg_n = 1;															//this - true
						n = string_to_u64(input);											//save n as uint64_t
						strcpy(by_default,"");												//not default now
						printf("\tn successfully defined. n = %" PRIu64 "\n\n", n);			//show this
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
						printf("n = %" PRIu64 ".%s\n", suffix_int, by_default); 		//leave previous value
						break;															//and break from cycle
					}else if(choose == '\n'){										//if just enter '\n' symbol
						no_display_now = 1;												//do this
						continue;														//and continue
					}
					else{															//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");			//show error message...
					}
				}
			}
			if(prime_gaps==1 && (arg_gaps2byte==0 && arg_gaps1byte==0)){			//if -gaps specified without format
				printf(																	//print this
					" Primes are writting as prime gaps (binary data), but unknown format.\n"
					" \"-gaps2byte\" - writting full prime gaps as two bytes as binary.\n"
					" \"-gaps1byte\" - writting half prime gaps as one byte as binary. \n\n"
				);
				while(1){																//cycle without end
					if(gaps1byte==0){														//if 1 byte
						printf(																	//show ask message
							"gaps1byte = %d. \"-gaps1byte\" not specified.\n"
							"This means \"prime gaps\" not writting as half prime gaps (1 byte).\n"
							"Do you want enable this? (Y/N): ",
							gaps1byte, by_default
						);
					}
					else if(gaps2byte==0){													//else if 2 bytes
						printf(																	//show another ask message
							"gaps2byte = %d. \"-gaps2byte\" not specified.\n"
							"This means \"prime gaps\" not writting as full prime gaps (2 bytes).\n"
							"Do you want enable this? (Y/N): ",
							gaps2byte, by_default
						);
					}
					fgets(input, 255, stdin);												//save input as string to temp variable
					strtok(input, "\n");													//discard '\n' in the end
					choose = (int) input[strlen(input)-1];									//save last char here
					if ( choose == 'y' || choose == 'Y' ){									//if last char is 'y' or 'Y'
						strcpy(by_default,"");
	
						gaps2byte = arg_gaps2byte = gaps1byte;
						gaps1byte = arg_gaps1byte = !gaps1byte;
	
						if(gaps1byte==1 && gaps2byte==0){
							printf("gaps1byte = %d - enabled, gaps2byte = %d - disabled\n", gaps1byte, gaps2byte);
						}
						else if(gaps1byte==0 && gaps2byte==1){
							printf("gaps1byte = %d - disabled, gaps2byte = %d - enabled\n", gaps1byte, gaps2byte);
						}
						else{
							printf(	"gaps1byte = %d, gaps2byte = %d - incorrect value.\n"
									"(only 1 or 0 available)", gaps1byte, gaps2byte);
						}
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){					//else if 'n' or 'N'
						printf(" NO");
						break;
					}else if(choose == '\n'){									//if just enter '\n' symbol
						no_display_now = 1;										//do this
						continue;												//and continue
					}
					else{														//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
			if(prime_gaps==0 && (!arg_b && !arg_bd)){ 							//if default value and delimiters not specified...
				strcpy(by_default,"(by default)");
				while(1){														//cycle without end
					if(no_display_now!=1){										//if need to display
	
						if(prime_gaps==0){
							printf(												//show this ask message
								"\nprime_gaps = %d %s. gaps1byte = %d, gaps2byte = %d\n"
								"\"-gaps\", \"-gaps2byte\" or \"-gaps1byte\" not specified.\n"
								"This means prime numbers not writting as \"prime gaps\"\n"
								"without any delimiters.\n"
								"Do you want to write this as prime gaps? (Y/N): ",
								prime_gaps, by_default, gaps1byte, gaps2byte
							);
						}else if(prime_gaps==1){
							printf(												//show another ask message
								"\nprime_gaps = %d %s. gaps1byte = %d, gaps2byte = %d\n"
								"\"-gaps\", \"-gaps2byte\" or \"-gaps1byte\" was been specified.\n"
								"This means prime numbers will be writting as \"prime gaps\"\n"
								"without any delimiters.\n"
								"Do you want to disable this (Y/N): ",
								prime_gaps, by_default, gaps1byte, gaps2byte
							);
						}else{
							printf("incorrect prime_gaps %d, value\n", prime_gaps);
						}
	
						fgets(input, 255, stdin);								//save string to temp variable
						strtok(input, "\n");									//discard '\n'
						choose = (int) input[strlen(input)-1];					//save last char here
					}else{													//if no need to display 
						no_display_now = 0;										//display next iteration
						continue;												//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){					//if last char is 'y' or 'Y'
						strcpy(by_default,"");
						prime_gaps = arg_gaps = !prime_gaps;
						gaps2byte = arg_gaps2byte = !gaps2byte;
						printf("\tprime_gaps = %d", prime_gaps);
						if(prime_gaps==0){
							gaps1byte = arg_gaps1byte = 0;
							gaps2byte = arg_gaps2byte = 0;
							printf(" Now primes not writting as prime gaps.\n");
						}
						else if(prime_gaps==1 && (arg_gaps2byte==0 || arg_gaps1byte==0)){
							printf(" Now primes will writting as prime gaps (binary data).\n\n");
	
							while(1){
								if(gaps1byte==0){
									printf(									//show ask message
										"gaps1byte = %d. \"-gaps1byte\" not specified.\n"
										"This means \"prime gaps\" not writting as half prime gaps (1 byte).\n"
										"Do you want enable this? (Y/N): ",
										gaps1byte, by_default
									);
								}
								else if(gaps2byte==0){
									printf(									//show another ask message
										"gaps2byte = %d. \"-gaps2byte\" not specified.\n"
										"This means \"prime gaps\" not writting as full prime gaps (2 bytes).\n"
										"Do you want enable this? (Y/N): ",
										gaps2byte, by_default
									);
								}
								fgets(input, 255, stdin);					//save string to temp variable
								strtok(input, "\n");						//discard '\n'
								choose = (int) input[strlen(input)-1];		//save last char here
								if ( choose == 'y' || choose == 'Y' ){			//if last char is 'y' or 'Y'
									strcpy(by_default,"");
									gaps1byte = arg_gaps1byte = !gaps1byte;
									gaps2byte = arg_gaps2byte = !gaps2byte;
	
									if(gaps1byte==1 && gaps2byte==0){
										printf("gaps1byte = %d - enabled, gaps2byte = %d - disabled\n", gaps1byte, gaps2byte);
									}
									else if(gaps1byte==0 && gaps2byte==1){
										printf("gaps1byte = %d - disabled, gaps2byte = %d - enabled\n", gaps1byte, gaps2byte);
									}
									else{
										printf(	"gaps1byte = %d, gaps2byte = %d - incorrect value.\n"
												"(only 1 or 0 available)", gaps1byte, gaps2byte);
									}
	
									//confirm??
									printf("Confirm? (Y/N): ");					//show ask message
									fgets(input, 255, stdin);					//save string to temp variable
									strtok(input, "\n");						//discard '\n'
									choose = (int) input[strlen(input)-1];		//save last char here
									if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
									else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
									else {printf("Incorrect input... Please, try again...\n");}	//show error message...
									//end confirm...
								}
								else if ( choose == 'n' || choose == 'N' ){		//else if 'n' or 'N'
									printf(" NO");
									break;
								}else if(choose == '\n'){						//if just enter '\n' symbol
									no_display_now = 1;							//do this
									continue;									//and continue
								}
								else{											//else if any another symbol...
									printf("Incorrect input... Please, try again...\n");	//show error message...
								}
							}
						}
						else{printf(" Incorrect prime_gaps value (only 1 and 0 available).\n\n"); continue;}
	
						printf("\tprime_gaps = %d %s", prime_gaps, by_default); 		//display value
						printf(", gaps1byte = %d %s", gaps1byte, by_default); 			//display value
						printf(", gaps2byte = %d %s\n", gaps2byte, by_default); 		//display value
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){					//else if 'n' or 'N'
						printf("prime_gaps = %d %s\n", prime_gaps, by_default);		//default value
						break;														//and braak from the cycle
					}else if(choose == '\n'){									//if just enter '\n' symbol
						no_display_now = 1;											//do this
						continue;													//and continue
					}
					else{														//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
			if((x64==0 && !arg_x64) && (!arg_b && !arg_bd) && (prime_gaps==0 && !arg_gaps)){
				//if default value, argument an delimiters not specified, and if not writting as prime gaps...
	
				strcpy(by_default,"(by default)");
				while(1){//cycle without end
					if(no_display_now!=1){			//if need to display
						if(x64==0){
							//show ask message
							printf(	"\nx64 = %d %s.",x64,by_default);
							printf(	"\nThis means prime numbers not writting\n"
									"as x64 binary numbers without any delimiters.\n"
									"Do you want to write this as \'little endian\' 64-bit numbers? (Y/N): "
							);
						}else if(x64==1){
							//show another ask message
							printf("\nx64 = %d %s.",x64,by_default);
							printf(	"\nThis means prime numbers will be writting"
									"\nas x64 binary numbers without any delimiters.\n"
									"Do you want to write this as \'little endian\' 64-bit numbers? (Y/N): "
							);
						}else{printf("x64 have incorrect value %d. Only 1 or 0 available", x64);}
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
					}else{										//if no need to display 
						no_display_now = 0;							//display next iteration
						continue;									//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){//if last char is 'y' or 'Y'
						x64 = arg_x64 = !x64;
						if(x64==1){printf("\tNow primes writting as binary numbers. x64 = %d\n", x64);}
						else if(x64==0){printf("\tNow primes not writting as binary numbers. x64 = %d\n", x64);}
						else{printf("x64 have incorrect value %d. Only 1 or 0 available", x64);}
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
						printf("x64 = %" PRIu64 " %s\n", suffix_int, by_default); 		//default value
						break;															//and break from the cycle
					}else if(choose == '\n'){										//if just enter '\n' symbol
						no_display_now = 1;												//do this
						continue;														//and continue
					}
					else{															//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");			//show error message and continue...
					}
				}
			}
			if(b==10 && !arg_b_int && (x64==0 && prime_gaps==0)){
				strcpy(by_default,"(by default)");
				while(1){									//cycle without end
					if(no_display_now!=1){						//if need to display
						if(b==10){
							printf(	"\nb = %" PRIu64 " %s. This means %" PRIu64 " primes writting in one block (line).\n"		//show ask message
									"Do you want to change this value? (Y/N): ", b, by_default, b
							);
						}else{
							printf(
								"\nb = %" PRIu64 " %s. This means %" PRIu64 " primes writting in one block (line).\n"		//show another ask message
								"Do you want to change this value? (Y/N): ", b, by_default, b
							);
						}
						fgets(input, 255, stdin);								//save string to temp variable
						strtok(input, "\n");									//discard '\n'
						choose = (int) input[strlen(input)-1];					//save last char here
					}else{														//if no need to display 
						no_display_now = 0;											//display next iteration
						continue;													//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){						//if last char is 'y' or 'Y'
						while(!is_uint64_t(input)){
							printf("Enter the number of primes in one block (line). Only digits. b: ");	//do this
							fgets(input, 255, stdin);									//save string to temp variable
							strtok(input, "\n");										//discard '\n'
							if(is_uint64_t(input) && input != "y" && input != "Y"){
								break;
							}else{
								printf("This must be a number (digits only)...\n");
							}
						}
						arg_b = 1;
						b = string_to_u64(input);
						printf("\tb successfully changed. b = %" PRIu64 "\n\n", b);
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
	
					}
					else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
						printf("b = %" PRIu64 " %s\n", suffix_int, by_default); 		//default value
						break;															//and braak from the cycle
					}else if(choose == '\n'){										//if just enter '\n' symbol
						no_display_now = 1;												//do this
						continue;														//and continue
					}
					else{															//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");			//show error message, and continue...
					}
				}
			}
	
			char temp_delimiter_from_input[256];
	
			if(											//if
				(
						(code_d == 9 ||	code_d == 0)	//char code for d is default or not specified
					&&	!arg_d_value					//and if -d arg was not been specified
				)
				&&	(x64==0 && prime_gaps==0)			//and if not need to writting without delimiters
			){//can be specified delimiter b
	
				strcpy(by_default,"(by default)");
				while(1){								//cycle without end
					if(no_display_now!=1){					//if need to display
						printf(	"\nd. Delimiter in block(line), d = \'%s\' %s.\n"		//show ask message
								"This means %" PRIu64 " primes writting through \'%s\' in one block (in line).\n"
								"Code of this symbol = %" PRIu64 " "
								,str_escape(d), by_default, b, str_escape(d), code_d
						);
						printf("%s \nDo you want to change this value? (Y/N): ", by_default);
	
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{															//if no need to display 
						no_display_now = 0;												//display next iteration
						continue;														//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){							//if last char is 'y' or 'Y'
						print_usage(2); 												//print info about codes for delimiters b and bd
						arg_d = 1;														//set this true
	
						printf("\nEnter the code or char(s) for delimiter between primes in block (line): ");
						sprintf(input, "%s", "");										//write empty string to input
						fgets(input, 255, stdin);										//write input from stdin
						strtok(input, "\n");											//discard '\n'
						strcpy(d, return_symbol_by_code(input));						//return symbol if code in input
						
						//code_d = (uint64_t) d;												//write code from symbol
						code_d = (uint64_t) d[0];												//write code from first symbol, because can be a multistring
						printf("delimiter successfully changed. d = %s\n", str_escape(d));		//show escaped delimiter
						strcpy(by_default,"");													//not default
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
	
					}
					else if ( choose == 'n' || choose == 'N' ){							//else if 'n' or 'N' - show delimiter
						printf("d as cher from code_d = \%c\n", code_d);					//show this as character
						break;																//and break;
					}else if(choose == '\n'){											//if just enter '\n' symbol
						no_display_now = 1;													//do this
						continue;															//and continue
					}
					else{																//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");				//show error message and continue...
					}
				}
			}
			if(
				(
						(code_bd == 10 || code_bd == 0)		//if default code or delimiter not specified
					&& 	!arg_bd_value						//and if was not been specified in command line
				)
				&& (x64==0 && prime_gaps==0)				//and if no need to writting without delimiters
				&&	(b > 0)									//and if primes in block over null
			){ //then can be specified bd delimiter...
	
				strcpy(by_default,"(by default)");
				
				while(1){											//cycle without end
					if(no_display_now!=1){								//if need to display
						printf(	"\nbd. Default delimiter beetween blocks (lines) bd = \'%s\' %s.\n"	//show ask message
								"This means each block delimited with \'%s\' (lines).\n"
								"Code of this symbol = %" PRIu64 " ",
								str_escape(bd), by_default, str_escape(bd), code_bd
						);
						printf("%s \nDo you want to change this value? (Y/N): ", by_default);
	
						fgets(input, 255, stdin);							//save string to temp variable
						strtok(input, "\n");								//discard '\n'
						choose = (int) input[strlen(input)-1];				//save last char here
					}else{												//if no need to display 
						no_display_now = 0;									//display next iteration
						continue;											//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){				//if last char is 'y' or 'Y'
						print_usage(2); 					//print info about codes for delimiters b and bd
						arg_bd = 1;
	
						printf("\nEnter the code or char(s) for delimiter between primes in block (line): ");
						sprintf(input, "%s", "");
						fgets(input, 255, stdin);
						strtok(input, "\n");
	
						strcpy(bd, return_symbol_by_code(input));
						//code_bd = (uint64_t) bd;
						code_bd = (uint64_t) bd[0];		//save code of first char, because this can be multichar		
						printf("delimiter successfully changed. d = %s\n", str_escape(bd));
						strcpy(by_default,"");
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
	
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){				//else if 'n' or 'N' - show delimiter
						printf("bd from char_bd = \%c\n", code_bd);			//show delimiter from code as ASCII character
						break;												//and break;
					}else if(choose == '\n'){							//if just enter '\n' symbol
						no_display_now = 1;									//do this
						continue;											//and continue
					}
					else{											//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message, and continue...
					}
				}
			}
			if(suffix_int==0 && !arg_suffix_part_int){
				strcpy(by_default,"(not defined, by default)");
				while(1){														//cycle without end
					if(no_display_now!=1){											//if need to display
						printf(	"\nStart part is suffix_int = %" PRIu64 " %s.\n"		//show ask message
								"Do you want change start _part(number)? (Y/N): "
								,suffix_int, by_default
						);
	
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{															//if no need to display 
						no_display_now = 0;												//display next iteration
						continue;														//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){							//if last char is 'y' or 'Y'
						while(!is_uint64_t(input)){
							printf("Enter start part (this must be a number - digits only): ");	//do this
							fgets(input, 255, stdin);											//save string to temp variable
							strtok(input, "\n");												//discard '\n'
							if(is_uint64_t(input) && input != "y" && input != "Y"){
								break;
							}else{
								printf("This must be a number (digits only)...\n");
							}
						}
						arg_suffix_part = 1;
						suffix_int = string_to_u64(input);
						strcpy(by_default,"");
						printf("\tStart part sucessfully defined: %" PRIu64 "\n\n", suffix_int);
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
						printf("Start part is %" PRIu64 " %s\n", suffix_int, by_default); 	//default value
						break;																//and braak from the cycle
					}else if(choose == '\n'){										//if just enter '\n' symbol
						no_display_now = 1;												//do this
						continue;														//and continue
					}
					else{															//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");			//show error message, and continue...
					}
				}
			}
			if((output_first_last==0 && !arg_wfl) && (output && n!=0)){
				strcpy(by_default,"(by default)");
				while(1){															//cycle without end
					if(no_display_now!=1){												//if need to display
						if(output_first_last==0){
							printf(	"\nWritting first and last prime for each part - disabled %s.\n"	//show ask message
								"Do you want to enable this? (Y/N): ",
								by_default
							);
						}else if(output_first_last==1){
							printf(	"\nWritting first and last prime for each part - enabled %s.\n"		//show ask message
								"Do you want to disable this? (Y/N): ",
								by_default
							);
						}
						else{
							printf(
								"\nIncorrect value for output_first_last %d.\n"							//show ask message
								"Only 1 or 0 available. Change? (Y/N): ",
								output_first_last
							);
						}
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{																	//if no need to display 
						no_display_now = 0;														//display next iteration
						continue;																//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){								//if last char is 'y' or 'Y'
						output_first_last = arg_wfl = !output_first_last;
						strcpy(by_default,"");
						if(output_first_last==0){printf("\toutput_first_last = %d, disabled.\n\n", output_first_last);}
						else if(output_first_last==1){
							printf("\toutput_first_last = %d, enabled.\n\n", output_first_last);
	
							//set filename from outfile, using suffix
							strcpy(temp_for_strtok, outfname);								//copy outfname to temp variable
	
							if(suffix_int==0){							//if suffix_int not specified, then filename not contains suffix _part
								ch = strtok(temp_for_strtok, ".");			//split by "."
							}
							else{										//if start suffix_int specified, then filename contains suffix _part
								ch = strtok(temp_for_strtok, "_");			//split by "_"
							}
							sprintf(filename, "%s", ch); 				//write file name only to "filename"
	
							x = 0;																//start 0
							while (ch != NULL) {												//while not null
								x++;																//x++
								if((suffix_int==0 && x > 1)||(suffix_int!=0 && x > 2)){				//if this
									sprintf(filename, "%s_first-last", filename);						//add suffix _first-last
									sprintf(filename, "%s.%s", filename, ch);							//add extension
								}
								ch = strtok(NULL, ".");												//again
							}
							sprintf(outfname_first_last, filename);								//write new filename to outfname_first_last
						}
						else{printf("\t Incorrect value for output_first_last %d (1 or 0).\n\n", output_first_last);}
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message, and continue...
						//end confirm...
	
					}
					else if ( choose == 'n' || choose == 'N' ){						//else if 'n' or 'N'
						printf("Output disabled %s...\n\n", by_default);			//show this
						break;														//and braak from the cycle
					}else if(choose == '\n'){										//if just enter '\n' symbol
						no_display_now = 1;												//do this
						continue;														//and continue
					}
					else{															//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");			//show error message, and try again...
					}
				}
			}
			if(!strcmp(outfname_first_last,"")){
				strcpy(by_default,"(not defined - by default)");
				while(1){												//cycle without end
					if(no_display_now!=1){									//if need to display
						if(!strcmp(outfname_first_last, "\n")){					//if outfname_first_last = '\n'
							strcpy(outfname_first_last, "out_first-last.txt");		//return to default
							strcpy(by_default,"(by default)");						//and set this...
						}
						printf(	"\nName of file for writting first and last prime - is \"%s\"\n"	//show ask message
								"%s\n"
								"Do you want to change name of this file? (Y/N): ", outfname_first_last, by_default);
						fgets(input, 255, stdin);										//save string to temp variable
						strtok(input, "\n");											//discard '\n'
						choose = (int) input[strlen(input)-1];							//save last char here
					}else{																//if no need to display 
						no_display_now = 0;												//display next iteration
						continue;														//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){					//if last char is 'y' or 'Y'
						printf("Enter the filename (name.extension): ");		//do this
						fgets(outfname_first_last, 255, stdin);					//save string to temp variable
						strtok(outfname_first_last, "\n");						//discard '\n'
						printf("outfname_first_last successfully changed to: \"%s\". ", outfname_first_last);
						strcpy(by_default," ");
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
	
					}
					else if ( choose == 'n' || choose == 'N' ){				//else if 'n' or 'N'
						printf("Out_file_name is %s\n", outfname_first_last); 	//default value
						break;													//and braak from the cycle
					}else if(choose == '\n'){								//if just enter '\n' symbol
						no_display_now = 1;										//do this
						continue;												//and continue
					}
					else{													//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
			if(arg_blockSize==0){
				strcpy(by_default,"(by default)");
				while(1){											//cycle without end
					if(no_display_now!=1){								//if need to display
						printf(												//show ask message
								"\nblockSize = %" PRIu32 ". This is number of primes in buffer before writting in file.\n"
								"Do you want to change this value? (Y/N): ", blockSize, by_default
						);
						fgets(input, 255, stdin);							//save string to temp variable
						strtok(input, "\n");								//discard '\n'
						choose = (int) input[strlen(input)-1];				//save last char here
					}else{												//if no need to display 
						no_display_now = 0;									//display next iteration
						continue;											//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){				//if last char is 'y' or 'Y'
						while(!is_uint64_t(input)){
							printf("Enter new block size number (digits only). blockSize: ");	//do this
							fgets(input, 255, stdin);											//save string to temp variable
							strtok(input, "\n");												//discard '\n'
							if(is_uint64_t(input) && input != "y" && input != "Y"){
								break;
							}else{
								printf(	"This must be a 32-bit number.\n"
										"Max value 2^32-1 = 4,294,967,295 (digits only)...\n"
								);
							}
						}
						arg_f = 1;
						blockSize = string_to_u64(input);
						strcpy(by_default,"");
						printf("\tblockSize successfully changed. blockSize = %" PRIu32 "\n\n", blockSize);
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
	
					}
					else if ( choose == 'n' || choose == 'N' ){			//else if 'n' or 'N'
						printf("blockSize == %" PRIu64 " %s.\n", suffix_int, by_default);	//default value
						break;																//and braak from the cycle
					}else if(choose == '\n'){							//if just enter '\n' symbol
						no_display_now = 1;									//do this
						continue;											//and continue
					}
					else{												//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
			if((make7z==0 && !arg_7z) && (output && n!=0)){	//if output and if n>0 (not writting in the same file) - can be available 7z arhivation for parts.
				strcpy(by_default,"(by default)");
				while(1){													//cycle without end
					if(no_display_now!=1){										//if need to display
							printf(	"\nOn fly 7z packing, when generating - ");
						if(make7z==0){
							printf(
								"disabled %s. make7z = %d\n"					//show ask message
								"Do you want to enable this? (Y/N): ",
								by_default, make7z
							);
						}
						else if(make7z==1){
							printf(
								"enabled %s. make7z = %d\n"						//show another ask message
								"Do you want to disable this? (Y/N): ",
								by_default, make7z
							);
						}
						else{
							printf(	"have incorrect value %d.\n"
									"only 1 or 0 available."
									"Do you want to disable this? (Y/N): "
									, make7z
							);
						}
						fgets(input, 255, stdin);								//save string to temp variable
						strtok(input, "\n");									//discard '\n'
						choose = (int) input[strlen(input)-1];					//save last char here
					}else{													//if no need to display 
						no_display_now = 0;										//display next iteration
						continue;												//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){					//if last char is 'y' or 'Y'
						make7z = arg_7z = !make7z;
						strcpy(by_default,"");
						if(make7z==1){printf("make7z = %d - enabled.\n\n", make7z);}
						else if(make7z==0){printf("make7z = %d - disabled.\n\n", make7z);}
						else{printf("make7z = %d. Incorrect value (1 or 0 available).\n\n", make7z);}
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){					//else if 'n' or 'N'
						printf("make7z = %d, %s...\n\n", make7z, by_default);		//show this
						break;														//and braak from the cycle
					}else if(choose == '\n'){									//if just enter '\n' symbol
						no_display_now = 1;											//do this
						continue;													//and continue
					}
					else{														//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");		//show error message, and try again in next iteration...
					}
				}
			}
			if(make7z==1 && !arg_7z_value && !strcmp(path_to_7z, "7z.exe")){
				strcpy(by_default,"(by default - using path from environment variable \"PATH\")");
				while(1){									//cycle without end
					if(no_display_now!=1){						//if need to display
						if(!strcmp(path_to_7z, "\n")){				//if path_to_7z = '\n'
							strcpy(path_to_7z, "out");				//return to default
							strcpy(by_default,"(by default)");		//and set this...
						}
						printf(	"\npath_to_7z = \"%s\" %s\n"				//show ask message
								"This means 7z running using this value.\n"
								"Do you want to set full path for the 7z folder? (Y/N): ", path_to_7z, by_default);
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
					}else{									//if no need to display 
						no_display_now = 0;						//display next iteration
						continue;								//and continue
					}
					if ( choose == 'y' || choose == 'Y' ){			//if last char is 'y' or 'Y'
	
						printf(	"\nPathway can be entered in the next formats:\n"						//do this
								"\t1. \"C:\\7-zip\\7zip.exe\" (standard pathway)\n"
								"\t2. \"C:\\\\7-zip\\\\7z.exe\" (escaped slashes)\n"
								"Pathways can be entered with or without double quotes.\n"
								"Also, can be used many double slashes and double quotes,\n"
								"like this \"\"\"C:\\\\\\\\7-zip\\\\\\\\7zip.exe\"\"\"\n\n"
								"Enter the pathway to 7z.exe: "
						);
	
						fgets(path_to_7z, 255, stdin);											//save string to temp variable
						strtok(path_to_7z, "\n");												//discard '\n'
						printf("path_to_7z successfully changed to: \"%s\".\n\n", path_to_7z);
						strcpy(by_default," ");
	
						//confirm??
						printf("Confirm? (Y/N): ");					//show ask message
						fgets(input, 255, stdin);					//save string to temp variable
						strtok(input, "\n");						//discard '\n'
						choose = (int) input[strlen(input)-1];		//save last char here
						if ( choose == 'y' || choose == 'Y' ){printf("OK\n"); parameters_changed = 1; break;}
						else if ( choose == 'n' || choose == 'N' ){printf("NO\n"); continue;}
						else {printf("Incorrect input... Please, try again...\n");}	//show error message...
						//end confirm...
					}
					else if ( choose == 'n' || choose == 'N' ){				//else if 'n' or 'N'
						printf("path is %s %s\n", path_to_7z, by_default); 		//default value
						break;													//and braak from the cycle
					}else if(choose == '\n'){								//if just enter '\n' symbol
						no_display_now = 1;										//do this
						continue;												//and continue
					}
					else{													//else if any another symbol...
						printf("Incorrect input... Please, try again...\n");	//show error message...
					}
				}
			}
		}else{
			if(output==0){	//if this leaving as 0
				printf("output = %d... continue... No need to setting any another options...", output);
			}
			else{
				printf("incorrect value for output (1 or 0 must be). output = %d", output);
			}
		}
	}
	//END - DIALOG FOR INPUT PARAMETERS

	//BEGIN - COMMAND LINE GENERATOR FOR CHANGED PARAMETERS
	if(parameters_changed){
		#ifdef _WIN32
			char pref[3] = ">";
		#elif _WIN64
			char pref[3] = ">";
		#elif __linux__ || unix || __unix__ || __unix
			char pref[3] = "./";
		#endif // __linux__
	
		//print command line to run with specified parameters...
		printf(
				"____________________________________________\n"
				"All initialize parameters - are accepted.\n"
				"You can try to run this program with this parameters,\n"
				"using next command-line (ONE STRING):\n"
				"\n%s%s %" PRIu64 " %" PRIu64 " ", pref, argv[0], Ini, Fin
		);
		if(arg_progress){printf("-p ");}								//progress
		if(arg_x64){printf("-x64 ");}									//output
		if(arg_gaps2byte){printf("-gaps2byte ");}						//gap2bytes
		if(arg_gaps1byte){printf("-gaps1byte ");}						//gap1bytes
		if(arg_wfl){printf("-wfl ");}									//write first and last
		if(arg_output){													//output
			printf("-o ");
			if(arg_suffix_part){printf("%s ", name_of_file);}				//just name_of_file used here;
			else{printf("%s ", outfname);}									//or outfname
		}
		if(arg_suffix_part){printf("-s % " PRIu64 " ", suffix_int);}	//start part as suffix integer
		if(arg_n){printf("-n %" PRIu64 " ", n);}						//n - primes in part
		if(arg_b){printf("-b %" PRIu64 " ", b);}						//b - primes in block
		//printf("-d %d -bd %d", code_d, code_bd);						//just print this for test
		if(arg_d){printf("-d %" PRIu64 " ", code_d);}					//d - delimiter between primes in one block (line)
		if(arg_bd){printf("-bd %" PRIu64 " ", code_bd);}				//bd - delimiter between blocks (lines)
		if(arg_f){printf("-f %" PRIu32 " ", blockSize);}				//f - filling blocksize
		if(arg_7z || !strcmp(path_to_7z, "disabled")){					//7z - 7z on fly packing
			printf("-7z %s ", path_to_7z);
		}
	
		printf(
			"\n\nIn this case all parameter will be defined as you define this here...\n"
			"____________________________________________\n"
		);
		//pause to allow user copy the command line data...
		system("pause");
		//continue, after pause...
	}
	//END - COMMAND LINE GENERATOR FOR CHANGED PARAMETERS
	
	printf("Start generating primes...\n\n");
/*
	//Just write this previously...
	printf(	"\n\n\nJust print redefined values here without generating...\n"
			"Ini = %" PRIu64 ", taskIni = %" PRIu64 "\n"
			"Fin = %" PRIu64 ", taskFin = %" PRIu64 "\n"
			"progress = %d, output = %d, suffix_int = %" PRIu64 "\n"
			"outfname = %s" "\n" "n = %" PRIu64 ", b = %" PRIu64 "\n"
			//"d = %s, bd = %s, code_d = %d, code_bd = %d\n"
			"d(escaped) = %s, bd(escaped) = %s, code_d = %" PRIu64 ", code_bd = %" PRIu64 ",\n"
			"output_first_last = %d, outfname_first_last = %s"
			", blockSize = %" PRIu32 ", make7z = %d, path_to_7z = \"%s\""
			"\n\n\n",
			Ini, taskIni, Fin, taskFin,
			progress, output, suffix_int, outfname, 
			n, b,
			str_escape(d), str_escape(bd), code_d, code_bd,
			output_first_last, outfname_first_last,
			blockSize, make7z, path_to_7z
	);

	//end pause...
	system("pause");
	//continue program and generating after press key...
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

	if(x64==1 || prime_gaps==1){
		fout = fopen(outfname, "rb");
	}else if(x64==0 && prime_gaps==0){
		fout = fopen(outfname, "r");
	}else{
		printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
		printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
	}

	//printf("\n fout fopen %s - r, case4", outfname);
	if (fout != NULL && CheckPointCode) {
		fclose(fout);//close fout and open in function when checking...
		file_chk_results = check_file(outfname);

		//printf("Checking file %s", outfname);
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

			//printf("system_pause 6\n");
			system("pause"); //pause program to don't close window if double click on exe

			//printf("Press Ctrl+C or Enter to exit...");
			//getchar(); //wait char to don't close window if double click on exe

			exit(EXIT_FAILURE);
		}
		if (CheckPointCode && file_chk_results->count==0) {
			if(x64==1 || prime_gaps==1){
				fout = fopen(outfname, "wb");
			}else if(x64==0 && prime_gaps==0){
				fout = fopen(outfname, "w");
			}else{
				printf("incorrect x64 value. Only 1 and 0 available...");
				printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
			}
			//printf("\n First fout fopen %s - w - case5\n", outfname);
		} else {
			if(x64==1 || prime_gaps==1){
				fout = fopen(outfname, "ab");
			}else if(x64==0 && prime_gaps==0){
				fout = fopen(outfname, "a");
			}else{
				printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
				printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
			}
			//printf("\n First fout fopen %s - a - case6\n", outfname);
		}
		if (fout == NULL) {
			printf("\n\nFile %s not found\n", outfname);

			//printf("system_pause 7\n");
			system("pause"); //pause program to don't close window if double click on exe

			//printf("Press Ctrl+C or Enter to exit...");
			//getchar(); //wait char to don't close window if double click on exe

			exit(EXIT_FAILURE);
		}
	}

	fprintf(stderr, "Command line parameters :");
	for (int i = 1; i < argc; i++){
		if(!strcmp(argv[i-1],"-d") || !strcmp(argv[i-1],"-bd")){
			fprintf(stderr, " \"%s\"", str_escape(argv[i]));
		}else{
			fprintf(stderr, " %s", argv[i]);
		}
	}
	fprintf(stderr, "\n");
	fprintf(stderr, "Number range borders	: from %" PRIu64 " to %" PRIu64 " step %d\n", Ini, Fin, Step);
	fprintf(stderr, "Total amount of Numbers : %" PRIu64 "\n", total);
	fprintf(stderr, "Factoring Block Size	: %" PRIu32 "\n", blockSize);
	fprintf(stderr, "Starting date timestamp : %s\n", curdatetime);
	if(make7z){
		printf(
				"\nGenerating stopped sometimes, because silently/quietly\n"
				"7z packing - enabled. \"-y > nul\" in command.\n"
				"Just wait and generation will be contunue, after packing each part...\n"
		);
	}
	int cpcnt, ctpcnt = 0;
	float cstep = 1000;

	if (progress)
		do_progress(ctpcnt);

	if (taskIni < minPrime) {//if start number < 3, write (exclusive even prime number).
		toCnt++;
		if (output && file_chk_results->first_number==0){ //if first number == 0, no 2 in file and no any prime in the file.
			//The first and singular EVEN prime number "2" is an exception here. All other primes is odd numbers.
			if(x64==1 || prime_gaps==1){	//if primes as 64-bit integers
				buf = 2;
				fwrite(&buf, sizeof(buf), 1, fout); //write this as 64-bit integer, without delimiter
			}
			
			if(x64==0 && prime_gaps==0){
				if(b==1){	//if one number in block use delimiter between blocks...
					//fprintf(stderr, "\nWrite 2 firstly with bd,\n\"%" PRIu64 "\"", toCnt);
					fprintf(fout, "%d%s", 2, bd);
				}
				else{ 		//else use delimiter inside block...
					//fprintf(stderr, "\nWrite 2 firstly with d,\n\"%" PRIu64 "\"", toCnt);
					fprintf(fout, "%d%s", 2, d);
				}
			}else if(prime_gaps==1){
				if(gaps2byte && !gaps1byte){//if gap as two bytes
					//gap is an even number. gap/2 - integer - half gap. And this gap/2 value - writting in the file.
					//only first two numbers have unique prime gaps.
					//from 1 to 2 prime gap = 1. 1 not a prime. So gap can be 2 (from null). But I set this as null.
					//from 2 to 3 prime gap = 1. Unique odd gap. gap/2 = 0 here, so I set 01 byte for this.
					//All another even numbers.

					//This two gaps can be writed at beginning of file in firstly,

					//for sequence
					//		1, 	2, 2, 4, 2, 4, 2, 4, 6, 2, 6, 4, 2, 4, 6, 6, 2, 6, 4, 2, 6, 4, 6, 8, 4, 2, 4, 2, 4, 14
					//half gap will be
					//0, 	1, 	1, 1, 2, 1, 2, 1, 2, 3, 1, 3, 2, 1, 2, 3, 3, 1, 3, 2, 1, 3, 2, 3, 4, 2, 1, 2, 1, 2, 7
					//where
					//0, 	1 - is this two values, all another - half of even prime gap value...

					unsigned short int first_prime_gap; //define two bytes positive integer
					//first_prime_gap = 0; //first gap = 0 between 2 and 2. 
					//write this at beginning of file
					//fwrite(&first_prime_gap, sizeof(first_prime_gap), 1, fout); //write first prime gap from 2 to 2.
					//0(dec) -> 0 (hex) -> 00 00 (hex) -> 00 00 short Endian (inverse bytes) -> writting in file.

					first_prime_gap = 1; //second gap from 2 to 3 = 1. gap/2 = 0 here. So 1 writting.
					fwrite(&first_prime_gap, sizeof(first_prime_gap), 1, fout); //write second prime gap from 3 to 1.
					//1(dec) -> 1 (hex) -> 00 01 2 hex bytes -> 01 00 short Endian (inverse bytes). -> writting in file

					//now size for each part == (2*n) or n bytes
					//2 000 000 or 1 000 000 bytes for each part with 10M primes
					//and ~600 kbytes for 7z archive with this.

				}else if(gaps1byte && !gaps2byte){
					//if 1 byte gap
					unsigned char first_prime_gap; //1 byte [0, 255]

					//first_prime_gap = 0;										//from 2 to 2 first gap = 0 (or 1 from 1 of 2 from 0).
					//fwrite(&first_prime_gap, sizeof(first_prime_gap), 1, fout); //write first prime gap from 2 to 2. 00 hex.

					first_prime_gap = 1;										//second prime gap between 2 and 3 = 1 (unique odd gap).
																				//Half of this = 0; So this = 1.
					fwrite(&first_prime_gap, sizeof(first_prime_gap), 1, fout); //write second prime gap from 3 to 2. 01 hex.

					//end exclude first gap writting in the cycle...

					//now size for each part == n bytes
					//1 000 000 bytes for each part with 10M primes and ~560 kbytes for 7z archive with this.

				}else{
					printf(	"gaps1byte = %d, gaps2byte = %d."
							"Incorrect value gaps1byte=!gaps2byte. \n",
							gaps1byte, gaps2byte
					);
				}
			}else{
				printf("Incorrect x64 value. Only 1 or 0 available...");
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
				fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			}
			fprintf(first_last_file, "[%" PRIu64 ",\'2 - ", suffix_int); //save part and first prime

			fclose(first_last_file);
			if(save_first) save_first = 0;
		}
	}

	if(x64==0 && prime_gaps!=1){ //if no need to write as x64 numbers - just write as text in this cycle...
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
										fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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

									//printf("\npath to 7z archive - \"%s\"", path_to_7z);		//just display path
									char command[256]; 	//define the temp variable to create and save command

									//make command
									if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
										//try to using 7z from environment variable PATH
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\" -y > nul", outfname); //create command to archive this
									}else{ //else, using full specified pathway	for 7z.exe
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\" -y > nul ", outfname, path_to_7z); //create command to archive this
									}
									//printf("%s", command); //print this command for test
									system(command); //run creating archive and wait...

									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s -y > nul", outfname, outfname);
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

								if(x64==1 || prime_gaps==1){
									fout = fopen(outfname, "rb");
								}else if(x64==0 && prime_gaps==0){
									fout = fopen(outfname, "r");
								}else{
									printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
									printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
								}
								//printf("\n In cycle fout fopen %s r - case1, (Cur+i) = %" PRIu64 "\n", outfname, Cur+i);
								if (fout != NULL && CheckPointCode) {

									file_chk_results = check_file(outfname);

									//printf("Checking file %s", outfname);
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

									//printf("system_pause 8\n");
									system("pause"); //pause program to don't close window if double click on exe

									//printf("Press Ctrl+C or Enter to exit...");
									//getchar(); //wait char to don't close window if double click on exe

									exit(EXIT_FAILURE);
								}
								if (CheckPointCode && file_chk_results->count==0) {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "wb");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "w");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s w - case2\n", outfname);
								} else {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "ab");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "a");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s a - case3\n", outfname);
								}
								if (fout == NULL) {
									printf("\n\nFile %s not found...\n", outfname);

									//printf("system_pause 9\n");
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
								fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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
							if(x64==1 || prime_gaps==1){
								fout = fopen(outfname, "a+b");
							}else if(x64==0 && prime_gaps==0){
								fout = fopen(outfname, "a+");
							}else{
								printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
								printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
							}
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
											"\n	>%s %" PRIu64 " %" PRIu64 " -p -o %s -s %" PRIu64 " -n %" PRIu64 " -b %" PRIu64 " -d %d -bd %d"
											"\n\n\n",
											argv[0], file_chk_results->first_number, (Cur+i),
											file_chk_results->first_number, (Cur+i), name_of_file, suffix_int, n, b, (int) d[0], (int) bd[0]
							);
						}
						if(toCnt!=1 && toCnt%b==0){//if last prime in block
							//fprintf(stderr, " %" PRIu64 "+bd\n", toCnt);
							if(continue_n==0){
								//printf("bd = %s, code_bd = %d", bd, code_bd);
								
								fprintf(fout, "%" PRIu64 "%s", Cur + i, bd);	//using delimiter between blocks
							}
						}
						else{//else if prime in block
							//fprintf(stderr, " %" PRIu64 " ", toCnt);
							if(continue_n==0){
								//printf("bd = %s, code_bd = %d", bd, code_bd);
								
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
	}else if (x64==1 && prime_gaps!=1){ //if need to write primes as x64 numbers - run this cycle...
		//The separate cycle added in this contition TO DON'T check this FOR EVERY PRIME NUMBER, inside one cycle... 
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
										fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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

									//printf("\npath to 7z archive - \"%s\"", path_to_7z);	//just display path
									char command[256]; 	//define the temp variable to create and save command

									//make command
									if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
										//try to using 7z from environment variable PATH
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\" -y > nul", outfname); //create command to archive this
									}else{ //else, using full specified pathway	for 7z.exe
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\" -y > nul", outfname, path_to_7z); //create command to archive this
									}
									//printf("%s", command); //print this command for test
									system(command); //run creating archive and wait...

									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s -y > nul", outfname, outfname);
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

								if(x64==1 || prime_gaps==1){
									fout = fopen(outfname, "rb");
								}else if(x64==0 && prime_gaps==0){
									fout = fopen(outfname, "r");
								}else{
									printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
									printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
								}
								//printf("\n In cycle fout fopen %s r - case1, (Cur+i) = %" PRIu64 "\n", outfname, Cur+i);
								if (fout != NULL && CheckPointCode) {

									file_chk_results = check_file(outfname);

									//printf("Checking file %s", outfname);
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

									//printf("system_pause 8\n");
									system("pause"); //pause program to don't close window if double click on exe

									//printf("Press Ctrl+C or Enter to exit...");
									//getchar(); //wait char to don't close window if double click on exe

									exit(EXIT_FAILURE);
								}
								if (CheckPointCode && file_chk_results->count==0) {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "wb");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "w");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s w - case2\n", outfname);
								} else {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "ab");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "a");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s a - case3\n", outfname);
								}
								if (fout == NULL) {
									printf("\n\nFile %s not found...\n", outfname);

									//printf("system_pause 9\n");
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
								fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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
							if(x64==1 || prime_gaps==1){
								fout = fopen(outfname, "a+b");
							}else if(x64==0 && prime_gaps==0){
								fout = fopen(outfname, "a+");
							}else{
								printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
								printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
							}
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
											"\n	>%s %" PRIu64 " %" PRIu64 " -p -o %s -s %" PRIu64 " -n %" PRIu64 " -b %" PRIu64 " -d %d -bd %d"
											"\n\n\n",
											argv[0], file_chk_results->first_number, (Cur+i),
											file_chk_results->first_number, (Cur+i), name_of_file, suffix_int, n, b, (int) d[0], (int) bd[0]
							);
						}
						buf = Cur+i;
						if(toCnt!=1 && toCnt%b==0){//if last prime in block
							//fprintf(stderr, " %" PRIu64 "+bd\n", toCnt);
							if(continue_n==0){
								//fprintf(fout, "%" PRIu64 "%s", Cur + i, bd);	//using delimiter between blocks
								fwrite(&buf, sizeof(buf), 1, fout); //without delimiter
							}
						}
						else{//else if prime in block
							//fprintf(stderr, " %" PRIu64 " ", toCnt);
							if(continue_n==0){
								//fprintf(fout, "%" PRIu64 "%s", Cur + i, d); 	//using delimiter between primes in block
								fwrite(&buf, sizeof(buf), 1, fout); //without delimiter
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
	}else if(prime_gaps==1 && gaps2byte==1 && gaps1byte==0){			//separate cycle to don't check it in each iteration...
		//for 2 bytes full prime gaps
		//unsigned char null_byte = 0;		//define 0 as unsigned char.
		//and
		//unsigned short int prime_gap;
		//was defined previously

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
										fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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

									//printf("\npath to 7z archive - \"%s\"", path_to_7z);	//just display path
									char command[256]; 	//define the temp variable to create and save command

									//make command
									if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
										//try to using 7z from environment variable PATH
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\" -y > nul", outfname); //create command to archive this
									}else{ //else, using full specified pathway	for 7z.exe
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\" -y > nul", outfname, path_to_7z); //create command to archive this
									}
									//printf("%s", command); //print this command for test
									system(command); //run creating archive and wait...

									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s -y > nul", outfname, outfname);
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

								if(x64==1 || prime_gaps==1){
									fout = fopen(outfname, "rb");
								}else if(x64==0 && prime_gaps==0){
									fout = fopen(outfname, "r");
								}else{
									printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
									printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
								}
								//printf("\n In cycle fout fopen %s r - case1, (Cur+i) = %" PRIu64 "\n", outfname, Cur+i);
								if (fout != NULL && CheckPointCode) {

									file_chk_results = check_file(outfname);

									//printf("Checking file %s", outfname);
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

									//printf("system_pause 8\n");
									system("pause"); //pause program to don't close window if double click on exe

									//printf("Press Ctrl+C or Enter to exit...");
									//getchar(); //wait char to don't close window if double click on exe

									exit(EXIT_FAILURE);
								}
								if (CheckPointCode && file_chk_results->count==0) {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "wb");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "w");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s w - case2\n", outfname);
								} else {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "ab");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "a");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s a - case3\n", outfname);
								}
								if (fout == NULL) {
									printf("\n\nFile %s not found...\n", outfname);

									//printf("system_pause 9\n");
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
								fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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
							//continue;
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
								//continue;
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
							if(x64==1 || prime_gaps==1){
								fout = fopen(outfname, "a+b");
							}else if(x64==0 && prime_gaps==0){
								fout = fopen(outfname, "a+");
							}else{
								printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...\n");
								printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
							}
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
											"\n	>%s %" PRIu64 " %" PRIu64 " -p -o %s -s %" PRIu64 " -n %" PRIu64 " -b %" PRIu64 " -d %d -bd %d"
											"\n\n\n",
											argv[0], file_chk_results->first_number, (Cur+i),
											file_chk_results->first_number, (Cur+i), name_of_file, suffix_int, n, b, (int) d[0], (int) bd[0]
							);
						}

						//printf(" prime = %" PRIu64 ", prev = %" PRIu64 ", gap = %" PRIu64 ", gap/2 = %" PRIu64 ";\n", (Cur+i), buf, (Cur+i - buf), ((Cur+i - buf)/2));
						prime_gap = (Cur+i - buf); //save full prime gap
					//	if(Cur+i>150){ //limit generation first to test bytes in beginning of file
					//		exit(EXIT_SUCCESS);
					//	}
						buf = Cur+i;
						
						if(buf==3){			//from 2 to 3 prime gap = 1, so gap/2 = 0 here. Set this to 1;
							if(toCnt==2){continue;} //not writting after 2, this already writted with 2.
							prime_gap=1;	//but if 3 is first prime, write this...
						}

						//write first prime as 64-bit number in the beginning of file
						if(toCnt_in_1_file==1 && continue_n==0){	//if this was been a first or last prime in the part
							unsigned long long first_prime_in_part = (Cur+i); //write first prime as 64-bit integer
							fwrite(&first_prime_in_part, sizeof(first_prime_in_part), 1, fout); //without delimiter
							continue; //and continue cycle to next iteration
						}

						if(toCnt!=1 && toCnt%b==0){//if last prime in block
							//fprintf(stderr, " %" PRIu64 "+bd\n", toCnt);
							if(continue_n==0 && buf!=3 && file_chk_results->last_number!=Cur+i){ //exclude first prime gap for number 2
								//fprintf(fout, "%" PRIu64 "%s", Cur + i, bd);	//using delimiter between blocks
								fwrite(&prime_gap, sizeof(prime_gap), 1, fout); //without delimiter
								//printf("%hu ", prime_gap);
							}
						}
						else{//else if prime in block
							//fprintf(stderr, " %" PRIu64 " ", toCnt);

							if(continue_n==0 && buf!=2 && file_chk_results->last_number!=Cur+i){//exclude first prime gap for number 2.
								//fprintf(fout, "%" PRIu64 "%s", Cur + i, d); 	//using delimiter between primes in block
								fwrite(&prime_gap, sizeof(prime_gap), 1, fout); //without delimiter
								//printf("%hu ", prime_gap);
							}
						}
						
						//write last prime as 64-bit number in the end of file
						if(toCnt_in_1_file==0 && continue_n==0){	//if this was been a first or last prime in the part
							unsigned long long first_prime_in_part = (Cur+i); //write first prime as 64-bit integer
							if(file_chk_results->last_number!=Cur+i){
								fwrite(&first_prime_in_part, sizeof(first_prime_in_part), 1, fout); //without delimiter
							}
						}
						//OK. ~2000000 bytes for 1M primes from beginning prime 2. ~670 kbytes in 7z archive.
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
		}
	}else if(prime_gaps==1 && gaps2byte==0 && gaps1byte==1){			//separate cycle to don't check this in each iteration...
		//for 1 byte half prime gaps
		//unsigned char null_byte = 0;		//define 0 as unsigned char.
		//and 
		//unsigned char gap_1_byte; 			//define unsigned char 1 byte for positive integer [0-255]
		//was defined previously

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
										fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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

									//printf("\npath to 7z archive - \"%s\"", path_to_7z);	//just display path
									char command[256]; 	//define the temp variable to create and save command

									//make command
									if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
										//try to using 7z from environment variable PATH
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\" -y > nul", outfname); //create command to archive this
									}else{ //else, using full specified pathway	for 7z.exe
										sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\" -y > nul", outfname, path_to_7z); //create command to archive this
									}
									//printf("%s", command); //print this command for test
									system(command); //run creating archive and wait...

									sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s -y > nul", outfname, outfname);
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

								if(x64==1 || prime_gaps==1){
									fout = fopen(outfname, "rb");
								}else if(x64==0 && prime_gaps==0){
									fout = fopen(outfname, "r");
								}else{
									printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
									printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
								}
								//printf("\n In cycle fout fopen %s r - case1, (Cur+i) = %" PRIu64 "\n", outfname, Cur+i);
								if (fout != NULL && CheckPointCode) {

									file_chk_results = check_file(outfname);

									//printf("Checking file %s", outfname);
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

									//printf("system_pause 8\n");
									system("pause"); //pause program to don't close window if double click on exe

									//printf("Press Ctrl+C or Enter to exit...");
									//getchar(); //wait char to don't close window if double click on exe

									exit(EXIT_FAILURE);
								}
								if (CheckPointCode && file_chk_results->count==0) {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "wb");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "w");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s w - case2\n", outfname);
								} else {
									if(x64==1 || prime_gaps==1){
										fout = fopen(outfname, "ab");
									}else if(x64==0 && prime_gaps==0){
										fout = fopen(outfname, "a");
									}else{
										printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...");
										printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
									}
									//printf("\n in cycle fout fopen %s a - case3\n", outfname);
								}
								if (fout == NULL) {
									printf("\n\nFile %s not found...\n", outfname);

									//printf("system_pause 9\n");
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
								fprintf(first_last_file, "\n\n	DATE AND TIME:	%d-%d-%d %d:%d:%d\n\n",
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
							//continue;
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
								//continue;
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
							if(x64==1 || prime_gaps==1){
								fout = fopen(outfname, "a+b");
							}else if(x64==0 && prime_gaps==0){
								fout = fopen(outfname, "a+");
							}else{
								printf("incorrect x64 or prime_gaps value. Only 1 and 0 available...\n");
								printf("x64 = %d, prime_gaps = %d\n", x64, prime_gaps);
							}
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
											"\n	>%s %" PRIu64 " %" PRIu64 " -p -o %s -s %" PRIu64 " -n %" PRIu64 " -b %" PRIu64 " -d %d -bd %d"
											"\n\n\n",
											argv[0], file_chk_results->first_number, (Cur+i),
											file_chk_results->first_number, (Cur+i), name_of_file, suffix_int, n, b, (int) d[0], (int) bd[0]
							);
						}

						//printf(" prime = %" PRIu64 ", prev = %" PRIu64 ", gap = %" PRIu64 ", gap/2 = %" PRIu64 ";\n", (Cur+i), buf, (Cur+i - buf), ((Cur+i - buf)/2));
						prime_gap = (Cur+i - buf)/2; //because this always - even
					//	if(Cur+i>150){ //limit generation first to test bytes in beginning of file
					//		exit(EXIT_SUCCESS);
					//	}
						buf = Cur+i;
						
						if(buf==3){
							if(toCnt==2){continue;} //not writting after 2, this already writted with 2.
							prime_gap=1;			//but if 3 is first prime, write this...
						}
						//from 2 to 3 prime gap = 1, so gap/2 = 0 here. So set this to 1.

						//write first prime as 64-bit number in the beginning of file
						if(toCnt_in_1_file==1){	//if this was been first or last prime in the part
							unsigned long long first_prime_in_part = (Cur+i); //write first prime as 64-bit integer
							fwrite(&first_prime_in_part, sizeof(first_prime_in_part), 1, fout); //without delimiter
							//system("pause");
							continue; //and continue cycle
						}

						if(toCnt!=1 && toCnt%b==0){//if last prime in block
							//fprintf(stderr, " %" PRIu64 "+bd\n", toCnt);
							if(continue_n==0 && buf!=3 && file_chk_results->last_number!=Cur+i){ //exclude first prime gap for number 2
							//fprintf(fout, "%" PRIu64 "%s", Cur + i, bd);	//using delimiter between blocks
								if(prime_gap>255){
									printf("\nfwrite 1 - null_byte, toCnt = %d\n", toCnt);
									fwrite(&null_byte, sizeof(null_byte), 1, fout); //without delimiter
									printf("\nfwrite 2 - prime_gap, toCnt = %d\n", toCnt);
									fwrite(&prime_gap, sizeof(prime_gap), 1, fout); //without delimiter
								}else{
									gap_1_byte = prime_gap;
									fwrite(&gap_1_byte, sizeof(gap_1_byte), 1, fout); //without delimiter
								}
								//printf("%hu ", prime_gap);
							}
						}
						else{//else if prime in block
							//fprintf(stderr, " %" PRIu64 " ", toCnt);
							if(continue_n==0 && buf!=2 && file_chk_results->last_number!=Cur+i){//exclude first prime gap for number 2.

								//fprintf(fout, "%" PRIu64 "%s", Cur + i, d); 	//using delimiter between primes in block
								if(prime_gap>255){
									fwrite(&null_byte, sizeof(null_byte), 1, fout); //without delimiter
									fwrite(&prime_gap, sizeof(prime_gap), 1, fout); //without delimiter
								}else{
									gap_1_byte = prime_gap;
									fwrite(&gap_1_byte, sizeof(gap_1_byte), 1, fout); //without delimiter
								}
								//printf("%hu ", prime_gap);
							}
						}
					
						//last prime as 64-bit number in the end of file
						if(toCnt_in_1_file==0){	//if this was been first or last prime in the part
							uint64_t first_prime_in_part = (Cur+i); //write first prime as 64-bit integer
							//printf("in toCnt = %d, write last prime %"PRIu64"\n", toCnt, first_prime_in_part);
							if(file_chk_results->last_number!=Cur+i){
								fwrite(&first_prime_in_part, sizeof(first_prime_in_part), 1, fout); //without delimiter
							}
						}
					
						//OK. ~1000000 bytes for 1M primes from beginning. 550 kbytes in 7z archive.
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
		}
	}else{
		printf("Incorrect case in big contition for cycle...");
	}
	if(output){fclose(fout);}
	remove(chkfname);

	ftime(&endtime);
	uint64_t dif = (endtime.time - starttime.time) * 1000 + (endtime.millitm - starttime.millitm);

	fprintf(stderr, "\n");
	fprintf(stderr, "Elapsed time			: %02d:%02d:%02d.%03d\n", (unsigned char)(dif/60/60/1000),
	(unsigned char)((dif/60/1000)%60), (unsigned char)((dif/1000)%60), (unsigned char)(dif%1000));
	fprintf(stderr, "Total amount of Primes  : %" PRIu64 "\n", toCnt);
	free_block();

	//pack last archive in the end, if need pack this.
	if(make7z){//if need to create 7z and delete txt-parts...
		//printf("\npath to 7z archive - \"%s\"", path_to_7z);	//just display path
		char command[256]; 	//define the temp variable to create and save command

		//make command
		if(!strcmp(path_to_7z, "7z.exe")){//if string compare == 0 and path_to_7z == "7z.exe" (default value)
			//try to using 7z from environment variable PATH
			sprintf(command, "cmd.exe /c for %%X in (\"%s\") do 7z.exe a \"%%~nX.7z\" \"%%X\" -y > nul", outfname); //create command to archive this
		}else{ //else, using full specified pathway	for 7z.exe
			sprintf(command, "cmd.exe /c for %%X in (\"%s\") do \"%s\" a \"%%~nX.7z\" \"%%X\" -y > nul", outfname, path_to_7z); //create command to archive this
		}
		//printf("%s", command); //print this command for test
		system(command); //run creating archive and wait...

		sprintf(command, "cmd.exe /c for %%X in (\"%s\") do if exist \"%%~nX.7z\" del %s -y > nul", outfname, outfname);
		//create this command to delete txt part, if 7z already exists
		//printf("%s", command); //print this command for test
		system(command); //run command and delete file

		//continue creating next txt-part...
	}

	printf("\n\nWork finished...\n");

	//printf("system_pause 10\n");
	system("pause"); //pause program to don't close window if double click on exe

	//printf("Press Ctrl+C or Enter to exit...");
	//getchar(); //wait for input char to don't close window when double click on exe.
 
	return (EXIT_SUCCESS);
}
//END - main function...
